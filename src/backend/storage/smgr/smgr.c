/*-------------------------------------------------------------------------
 *
 * smgr.c
 *	  public interface routines to storage manager switch.
 *
 *	  All file system operations in POSTGRES dispatch through these
 *	  routines.
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/storage/smgr/smgr.c,v 1.70 2004/02/11 22:55:25 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "storage/bufmgr.h"
#include "storage/freespace.h"
#include "storage/ipc.h"
#include "storage/smgr.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"


/*
 * This struct of function pointers defines the API between smgr.c and
 * any individual storage manager module.  Note that smgr subfunctions are
 * generally expected to return TRUE on success, FALSE on error.  (For
 * nblocks and truncate we instead say that returning InvalidBlockNumber
 * indicates an error.)
 */
typedef struct f_smgr
{
	bool		(*smgr_init) (void);			/* may be NULL */
	bool		(*smgr_shutdown) (void);		/* may be NULL */
	bool		(*smgr_close) (SMgrRelation reln);
	bool		(*smgr_create) (SMgrRelation reln, bool isRedo);
	bool		(*smgr_unlink) (RelFileNode rnode, bool isRedo);
	bool		(*smgr_extend) (SMgrRelation reln, BlockNumber blocknum,
											char *buffer);
	bool		(*smgr_read) (SMgrRelation reln, BlockNumber blocknum,
										  char *buffer);
	bool		(*smgr_write) (SMgrRelation reln, BlockNumber blocknum,
										   char *buffer);
	BlockNumber (*smgr_nblocks) (SMgrRelation reln);
	BlockNumber (*smgr_truncate) (SMgrRelation reln, BlockNumber nblocks);
	bool		(*smgr_commit) (void);			/* may be NULL */
	bool		(*smgr_abort) (void);			/* may be NULL */
	bool		(*smgr_sync) (void);			/* may be NULL */
} f_smgr;


static const f_smgr smgrsw[] = {
	/* magnetic disk */
	{mdinit, NULL, mdclose, mdcreate, mdunlink, mdextend,
	 mdread, mdwrite, mdnblocks, mdtruncate, mdcommit, mdabort, mdsync
	}
};

static const int	NSmgr = lengthof(smgrsw);


/*
 * Each backend has a hashtable that stores all extant SMgrRelation objects.
 */
static HTAB *SMgrRelationHash = NULL;

/*
 * We keep a list of all relations (represented as RelFileNode values)
 * that have been created or deleted in the current transaction.  When
 * a relation is created, we create the physical file immediately, but
 * remember it so that we can delete the file again if the current
 * transaction is aborted.	Conversely, a deletion request is NOT
 * executed immediately, but is just entered in the list.  When and if
 * the transaction commits, we can delete the physical file.
 *
 * NOTE: the list is kept in TopMemoryContext to be sure it won't disappear
 * unbetimes.  It'd probably be OK to keep it in TopTransactionContext,
 * but I'm being paranoid.
 */

typedef struct PendingRelDelete
{
	RelFileNode relnode;		/* relation that may need to be deleted */
	int			which;			/* which storage manager? */
	bool		isTemp;			/* is it a temporary relation? */
	bool		atCommit;		/* T=delete at commit; F=delete at abort */
	struct PendingRelDelete *next;		/* linked-list link */
} PendingRelDelete;

static PendingRelDelete *pendingDeletes = NULL; /* head of linked list */


/*
 * Declarations for smgr-related XLOG records
 *
 * Note: we log file creation and truncation here, but logging of deletion
 * actions is handled by xact.c, because it is part of transaction commit.
 */

/* XLOG gives us high 4 bits */
#define XLOG_SMGR_CREATE	0x10
#define XLOG_SMGR_TRUNCATE	0x20

typedef struct xl_smgr_create
{
	RelFileNode		rnode;
} xl_smgr_create;

typedef struct xl_smgr_truncate
{
	BlockNumber		blkno;
	RelFileNode		rnode;
} xl_smgr_truncate;


/* local function prototypes */
static void smgrshutdown(int code, Datum arg);
static void smgr_internal_unlink(RelFileNode rnode, int which,
								 bool isTemp, bool isRedo);


/*
 *	smgrinit(), smgrshutdown() -- Initialize or shut down all storage
 *								  managers.
 *
 * Note: in the normal multiprocess scenario with a postmaster, these are
 * called at postmaster start and stop, not per-backend.
 */
void
smgrinit(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_init)
		{
			if (! (*(smgrsw[i].smgr_init)) ())
				elog(FATAL, "smgr initialization failed on %s: %m",
					 DatumGetCString(DirectFunctionCall1(smgrout,
													 Int16GetDatum(i))));
		}
	}

	/* register the shutdown proc */
	on_proc_exit(smgrshutdown, 0);
}

static void
smgrshutdown(int code, Datum arg)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_shutdown)
		{
			if (! (*(smgrsw[i].smgr_shutdown)) ())
				elog(FATAL, "smgr shutdown failed on %s: %m",
					 DatumGetCString(DirectFunctionCall1(smgrout,
													 Int16GetDatum(i))));
		}
	}
}

/*
 *	smgropen() -- Return an SMgrRelation object, creating it if need be.
 *
 *		This does not attempt to actually open the object.
 */
SMgrRelation
smgropen(RelFileNode rnode)
{
	SMgrRelation	reln;
	bool		found;

	if (SMgrRelationHash == NULL)
	{
		/* First time through: initialize the hash table */
		HASHCTL		ctl;

		MemSet(&ctl, 0, sizeof(ctl));
		ctl.keysize = sizeof(RelFileNode);
		ctl.entrysize = sizeof(SMgrRelationData);
		ctl.hash = tag_hash;
		SMgrRelationHash = hash_create("smgr relation table", 400,
									   &ctl, HASH_ELEM | HASH_FUNCTION);
	}

	/* Look up or create an entry */
	reln = (SMgrRelation) hash_search(SMgrRelationHash,
									  (void *) &rnode,
									  HASH_ENTER, &found);
	if (reln == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));

	/* Initialize it if not present before */
	if (!found)
	{
		/* hash_search already filled in the lookup key */
		reln->smgr_which = 0;	/* we only have md.c at present */
		reln->md_fd = NULL;		/* mark it not open */
	}

	return reln;
}

/*
 *	smgrclose() -- Close and delete an SMgrRelation object.
 *
 * It is the caller's responsibility not to leave any dangling references
 * to the object.  (Pointers should be cleared after successful return;
 * on the off chance of failure, the SMgrRelation object will still exist.)
 */
void
smgrclose(SMgrRelation reln)
{
	if (! (*(smgrsw[reln->smgr_which].smgr_close)) (reln))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close relation %u/%u: %m",
						reln->smgr_rnode.tblNode,
						reln->smgr_rnode.relNode)));

	if (hash_search(SMgrRelationHash,
					(void *) &(reln->smgr_rnode),
					HASH_REMOVE, NULL) == NULL)
		elog(ERROR, "SMgrRelation hashtable corrupted");
}

/*
 *	smgrcloseall() -- Close all existing SMgrRelation objects.
 *
 * It is the caller's responsibility not to leave any dangling references.
 */
void
smgrcloseall(void)
{
	HASH_SEQ_STATUS status;
	SMgrRelation reln;

	/* Nothing to do if hashtable not set up */
	if (SMgrRelationHash == NULL)
		return;

	hash_seq_init(&status, SMgrRelationHash);

	while ((reln = (SMgrRelation) hash_seq_search(&status)) != NULL)
	{
		smgrclose(reln);
	}
}

/*
 *	smgrclosenode() -- Close SMgrRelation object for given RelFileNode,
 *					   if one exists.
 *
 * This has the same effects as smgrclose(smgropen(rnode)), but it avoids
 * uselessly creating a hashtable entry only to drop it again when no
 * such entry exists already.
 *
 * It is the caller's responsibility not to leave any dangling references.
 */
void
smgrclosenode(RelFileNode rnode)
{
	SMgrRelation	reln;

	/* Nothing to do if hashtable not set up */
	if (SMgrRelationHash == NULL)
		return;

	reln = (SMgrRelation) hash_search(SMgrRelationHash,
									  (void *) &rnode,
									  HASH_FIND, NULL);
	if (reln != NULL)
		smgrclose(reln);
}

/*
 *	smgrcreate() -- Create a new relation.
 *
 *		Given an already-created (but presumably unused) SMgrRelation,
 *		cause the underlying disk file or other storage to be created.
 *
 *		If isRedo is true, it is okay for the underlying file to exist
 *		already because we are in a WAL replay sequence.  In this case
 *		we should make no PendingRelDelete entry; the WAL sequence will
 *		tell whether to drop the file.
 */
void
smgrcreate(SMgrRelation reln, bool isTemp, bool isRedo)
{
	XLogRecPtr		lsn;
	XLogRecData		rdata;
	xl_smgr_create	xlrec;
	PendingRelDelete *pending;

	if (! (*(smgrsw[reln->smgr_which].smgr_create)) (reln, isRedo))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not create relation %u/%u: %m",
						reln->smgr_rnode.tblNode,
						reln->smgr_rnode.relNode)));

	if (isRedo)
		return;

	/*
	 * Make a non-transactional XLOG entry showing the file creation.  It's
	 * non-transactional because we should replay it whether the transaction
	 * commits or not; if not, the file will be dropped at abort time.
	 */
	xlrec.rnode = reln->smgr_rnode;

	rdata.buffer = InvalidBuffer;
	rdata.data = (char *) &xlrec;
	rdata.len = sizeof(xlrec);
	rdata.next = NULL;

	lsn = XLogInsert(RM_SMGR_ID, XLOG_SMGR_CREATE | XLOG_NO_TRAN, &rdata);

	/* Add the relation to the list of stuff to delete at abort */
	pending = (PendingRelDelete *)
		MemoryContextAlloc(TopMemoryContext, sizeof(PendingRelDelete));
	pending->relnode = reln->smgr_rnode;
	pending->which = reln->smgr_which;
	pending->isTemp = isTemp;
	pending->atCommit = false;	/* delete if abort */
	pending->next = pendingDeletes;
	pendingDeletes = pending;
}

/*
 *	smgrscheduleunlink() -- Schedule unlinking a relation at xact commit.
 *
 *		The relation is marked to be removed from the store if we
 *		successfully commit the current transaction.
 *
 * This also implies smgrclose() on the SMgrRelation object.
 */
void
smgrscheduleunlink(SMgrRelation reln, bool isTemp)
{
	PendingRelDelete *pending;

	/* Add the relation to the list of stuff to delete at commit */
	pending = (PendingRelDelete *)
		MemoryContextAlloc(TopMemoryContext, sizeof(PendingRelDelete));
	pending->relnode = reln->smgr_rnode;
	pending->which = reln->smgr_which;
	pending->isTemp = isTemp;
	pending->atCommit = true;	/* delete if commit */
	pending->next = pendingDeletes;
	pendingDeletes = pending;

	/*
	 * NOTE: if the relation was created in this transaction, it will now
	 * be present in the pending-delete list twice, once with atCommit
	 * true and once with atCommit false.  Hence, it will be physically
	 * deleted at end of xact in either case (and the other entry will be
	 * ignored by smgrDoPendingDeletes, so no error will occur).  We could
	 * instead remove the existing list entry and delete the physical file
	 * immediately, but for now I'll keep the logic simple.
	 */

	/* Now close the file and throw away the hashtable entry */
	smgrclose(reln);
}

/*
 *	smgrdounlink() -- Immediately unlink a relation.
 *
 *		The relation is removed from the store.  This should not be used
 *		during transactional operations, since it can't be undone.
 *
 *		If isRedo is true, it is okay for the underlying file to be gone
 *		already.  (In practice isRedo will always be true.)
 *
 * This also implies smgrclose() on the SMgrRelation object.
 */
void
smgrdounlink(SMgrRelation reln, bool isTemp, bool isRedo)
{
	RelFileNode	rnode = reln->smgr_rnode;
	int			which = reln->smgr_which;

	/* Close the file and throw away the hashtable entry */
	smgrclose(reln);

	smgr_internal_unlink(rnode, which, isTemp, isRedo);
}

/*
 * Shared subroutine that actually does the unlink ...
 */
static void
smgr_internal_unlink(RelFileNode rnode, int which, bool isTemp, bool isRedo)
{
	/*
	 * Get rid of any leftover buffers for the rel (shouldn't be any in the
	 * commit case, but there can be in the abort case).
	 */
	DropRelFileNodeBuffers(rnode, isTemp);

	/*
	 * Tell the free space map to forget this relation.  It won't be accessed
	 * any more anyway, but we may as well recycle the map space quickly.
	 */
	FreeSpaceMapForgetRel(&rnode);

	/*
	 * And delete the physical files.
	 *
	 * Note: we treat deletion failure as a WARNING, not an error,
	 * because we've already decided to commit or abort the current xact.
	 */
	if (! (*(smgrsw[which].smgr_unlink)) (rnode, isRedo))
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("could not unlink relation %u/%u: %m",
						rnode.tblNode,
						rnode.relNode)));
}

/*
 *	smgrextend() -- Add a new block to a file.
 *
 *		The semantics are basically the same as smgrwrite(): write at the
 *		specified position.  However, we are expecting to extend the
 *		relation (ie, blocknum is the current EOF), and so in case of
 *		failure we clean up by truncating.
 */
void
smgrextend(SMgrRelation reln, BlockNumber blocknum, char *buffer)
{
	if (! (*(smgrsw[reln->smgr_which].smgr_extend)) (reln, blocknum, buffer))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not extend relation %u/%u: %m",
						reln->smgr_rnode.tblNode,
						reln->smgr_rnode.relNode),
				 errhint("Check free disk space.")));
}

/*
 *	smgrread() -- read a particular block from a relation into the supplied
 *				  buffer.
 *
 *		This routine is called from the buffer manager in order to
 *		instantiate pages in the shared buffer cache.  All storage managers
 *		return pages in the format that POSTGRES expects.
 */
void
smgrread(SMgrRelation reln, BlockNumber blocknum, char *buffer)
{
	if (! (*(smgrsw[reln->smgr_which].smgr_read)) (reln, blocknum, buffer))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read block %u of relation %u/%u: %m",
						blocknum,
						reln->smgr_rnode.tblNode,
						reln->smgr_rnode.relNode)));
}

/*
 *	smgrwrite() -- Write the supplied buffer out.
 *
 *		This is not a synchronous write -- the block is not necessarily
 *		on disk at return, only dumped out to the kernel.
 */
void
smgrwrite(SMgrRelation reln, BlockNumber blocknum, char *buffer)
{
	if (! (*(smgrsw[reln->smgr_which].smgr_write)) (reln, blocknum, buffer))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not write block %u of relation %u/%u: %m",
						blocknum,
						reln->smgr_rnode.tblNode,
						reln->smgr_rnode.relNode)));
}

/*
 *	smgrnblocks() -- Calculate the number of blocks in the
 *					 supplied relation.
 *
 *		Returns the number of blocks on success, aborts the current
 *		transaction on failure.
 */
BlockNumber
smgrnblocks(SMgrRelation reln)
{
	BlockNumber nblocks;

	nblocks = (*(smgrsw[reln->smgr_which].smgr_nblocks)) (reln);

	/*
	 * NOTE: if a relation ever did grow to 2^32-1 blocks, this code would
	 * fail --- but that's a good thing, because it would stop us from
	 * extending the rel another block and having a block whose number
	 * actually is InvalidBlockNumber.
	 */
	if (nblocks == InvalidBlockNumber)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not count blocks of relation %u/%u: %m",
						reln->smgr_rnode.tblNode,
						reln->smgr_rnode.relNode)));

	return nblocks;
}

/*
 *	smgrtruncate() -- Truncate supplied relation to the specified number
 *					  of blocks
 *
 *		Returns the number of blocks on success, aborts the current
 *		transaction on failure.
 */
BlockNumber
smgrtruncate(SMgrRelation reln, BlockNumber nblocks)
{
	BlockNumber newblks;
	XLogRecPtr		lsn;
	XLogRecData		rdata;
	xl_smgr_truncate xlrec;

	/*
	 * Tell the free space map to forget anything it may have stored
	 * for the about-to-be-deleted blocks.	We want to be sure it
	 * won't return bogus block numbers later on.
	 */
	FreeSpaceMapTruncateRel(&reln->smgr_rnode, nblocks);

	/* Do the truncation */
	newblks = (*(smgrsw[reln->smgr_which].smgr_truncate)) (reln, nblocks);
	if (newblks == InvalidBlockNumber)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not truncate relation %u/%u to %u blocks: %m",
						reln->smgr_rnode.tblNode,
						reln->smgr_rnode.relNode,
						nblocks)));

	/*
	 * Make a non-transactional XLOG entry showing the file truncation.  It's
	 * non-transactional because we should replay it whether the transaction
	 * commits or not; the underlying file change is certainly not reversible.
	 */
	xlrec.blkno = newblks;
	xlrec.rnode = reln->smgr_rnode;

	rdata.buffer = InvalidBuffer;
	rdata.data = (char *) &xlrec;
	rdata.len = sizeof(xlrec);
	rdata.next = NULL;

	lsn = XLogInsert(RM_SMGR_ID, XLOG_SMGR_TRUNCATE | XLOG_NO_TRAN, &rdata);

	return newblks;
}

/*
 *	smgrDoPendingDeletes() -- Take care of relation deletes at end of xact.
 */
void
smgrDoPendingDeletes(bool isCommit)
{
	while (pendingDeletes != NULL)
	{
		PendingRelDelete *pending = pendingDeletes;

		pendingDeletes = pending->next;
		if (pending->atCommit == isCommit)
			smgr_internal_unlink(pending->relnode,
								 pending->which,
								 pending->isTemp,
								 false);
		pfree(pending);
	}
}

/*
 * smgrGetPendingDeletes() -- Get a list of relations to be deleted.
 *
 * The return value is the number of relations scheduled for termination.
 * *ptr is set to point to a freshly-palloc'd array of RelFileNodes.
 * If there are no relations to be deleted, *ptr is set to NULL.
 */
int
smgrGetPendingDeletes(bool forCommit, RelFileNode **ptr)
{
	int			nrels;
	RelFileNode *rptr;
	PendingRelDelete *pending;

	nrels = 0;
	for (pending = pendingDeletes; pending != NULL; pending = pending->next)
	{
		if (pending->atCommit == forCommit)
			nrels++;
	}
	if (nrels == 0)
	{
		*ptr = NULL;
		return 0;
	}
	rptr = (RelFileNode *) palloc(nrels * sizeof(RelFileNode));
	*ptr = rptr;
	for (pending = pendingDeletes; pending != NULL; pending = pending->next)
	{
		if (pending->atCommit == forCommit)
			*rptr++ = pending->relnode;
	}
	return nrels;
}

/*
 *	smgrcommit() -- Prepare to commit changes made during the current
 *					transaction.
 *
 *		This is called before we actually commit.
 */
void
smgrcommit(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_commit)
		{
			if (! (*(smgrsw[i].smgr_commit)) ())
				elog(FATAL, "transaction commit failed on %s: %m",
					 DatumGetCString(DirectFunctionCall1(smgrout,
													 Int16GetDatum(i))));
		}
	}
}

/*
 *	smgrabort() -- Abort changes made during the current transaction.
 */
void
smgrabort(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_abort)
		{
			if (! (*(smgrsw[i].smgr_abort)) ())
				elog(FATAL, "transaction abort failed on %s: %m",
					 DatumGetCString(DirectFunctionCall1(smgrout,
													 Int16GetDatum(i))));
		}
	}
}

/*
 *	smgrsync() -- Sync files to disk at checkpoint time.
 */
void
smgrsync(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_sync)
		{
			if (! (*(smgrsw[i].smgr_sync)) ())
				elog(PANIC, "storage sync failed on %s: %m",
					 DatumGetCString(DirectFunctionCall1(smgrout,
													 Int16GetDatum(i))));
		}
	}
}


void
smgr_redo(XLogRecPtr lsn, XLogRecord *record)
{
	uint8		info = record->xl_info & ~XLR_INFO_MASK;

	if (info == XLOG_SMGR_CREATE)
	{
		xl_smgr_create *xlrec = (xl_smgr_create *) XLogRecGetData(record);
		SMgrRelation reln;

		reln = smgropen(xlrec->rnode);
		smgrcreate(reln, false, true);
	}
	else if (info == XLOG_SMGR_TRUNCATE)
	{
		xl_smgr_truncate *xlrec = (xl_smgr_truncate *) XLogRecGetData(record);
		SMgrRelation reln;
		BlockNumber newblks;

		reln = smgropen(xlrec->rnode);

		/* Can't use smgrtruncate because it would try to xlog */

		/*
		 * Tell the free space map to forget anything it may have stored
		 * for the about-to-be-deleted blocks.	We want to be sure it
		 * won't return bogus block numbers later on.
		 */
		FreeSpaceMapTruncateRel(&reln->smgr_rnode, xlrec->blkno);

		/* Do the truncation */
		newblks = (*(smgrsw[reln->smgr_which].smgr_truncate)) (reln,
															   xlrec->blkno);
		if (newblks == InvalidBlockNumber)
			ereport(WARNING,
					(errcode_for_file_access(),
					 errmsg("could not truncate relation %u/%u to %u blocks: %m",
							reln->smgr_rnode.tblNode,
							reln->smgr_rnode.relNode,
							xlrec->blkno)));
	}
	else
		elog(PANIC, "smgr_redo: unknown op code %u", info);
}

void
smgr_undo(XLogRecPtr lsn, XLogRecord *record)
{
	/* Since we have no transactional WAL entries, should never undo */
	elog(PANIC, "smgr_undo: cannot undo");
}

void
smgr_desc(char *buf, uint8 xl_info, char *rec)
{
	uint8		info = xl_info & ~XLR_INFO_MASK;

	if (info == XLOG_SMGR_CREATE)
	{
		xl_smgr_create *xlrec = (xl_smgr_create *) rec;

		sprintf(buf + strlen(buf), "file create: %u/%u",
				xlrec->rnode.tblNode, xlrec->rnode.relNode);
	}
	else if (info == XLOG_SMGR_TRUNCATE)
	{
		xl_smgr_truncate *xlrec = (xl_smgr_truncate *) rec;

		sprintf(buf + strlen(buf), "file truncate: %u/%u to %u blocks",
				xlrec->rnode.tblNode, xlrec->rnode.relNode,
				xlrec->blkno);
	}
	else
		strcat(buf, "UNKNOWN");
}
