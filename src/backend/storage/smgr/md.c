/*-------------------------------------------------------------------------
 *
 * md.c
 *	  This code manages relations that reside on magnetic disk.
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/storage/smgr/md.c,v 1.125 2007/01/05 22:19:38 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

#include "catalog/catalog.h"
#include "miscadmin.h"
#include "postmaster/bgwriter.h"
#include "storage/fd.h"
#include "storage/bufmgr.h"
#include "storage/smgr.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"


/* interval for calling AbsorbFsyncRequests in mdsync */
#define FSYNCS_PER_ABSORB		10

/*
 *	The magnetic disk storage manager keeps track of open file
 *	descriptors in its own descriptor pool.  This is done to make it
 *	easier to support relations that are larger than the operating
 *	system's file size limit (often 2GBytes).  In order to do that,
 *	we break relations up into "segment" files that are each shorter than
 *	the OS file size limit.  The segment size is set by the RELSEG_SIZE
 *	configuration constant in pg_config_manual.h.
 *
 *	On disk, a relation must consist of consecutively numbered segment
 *	files in the pattern
 *		-- Zero or more full segments of exactly RELSEG_SIZE blocks each
 *		-- Exactly one partial segment of size 0 <= size < RELSEG_SIZE blocks
 *		-- Optionally, any number of inactive segments of size 0 blocks.
 *	The full and partial segments are collectively the "active" segments.
 *	Inactive segments are those that once contained data but are currently
 *	not needed because of an mdtruncate() operation.  The reason for leaving
 *	them present at size zero, rather than unlinking them, is that other
 *	backends and/or the bgwriter might be holding open file references to
 *	such segments.  If the relation expands again after mdtruncate(), such
 *	that a deactivated segment becomes active again, it is important that
 *	such file references still be valid --- else data might get written
 *	out to an unlinked old copy of a segment file that will eventually
 *	disappear.
 *
 *	The file descriptor pointer (md_fd field) stored in the SMgrRelation
 *	cache is, therefore, just the head of a list of MdfdVec objects, one
 *	per segment.  But note the md_fd pointer can be NULL, indicating
 *	relation not open.
 *
 *	Also note that mdfd_chain == NULL does not necessarily mean the relation
 *	doesn't have another segment after this one; we may just not have
 *	opened the next segment yet.  (We could not have "all segments are
 *	in the chain" as an invariant anyway, since another backend could
 *	extend the relation when we weren't looking.)  We do not make chain
 *	entries for inactive segments, however; as soon as we find a partial
 *	segment, we assume that any subsequent segments are inactive.
 *
 *	All MdfdVec objects are palloc'd in the MdCxt memory context.
 *
 *	Defining LET_OS_MANAGE_FILESIZE disables the segmentation logic,
 *	for use on machines that support large files.  Beware that that
 *	code has not been tested in a long time and is probably bit-rotted.
 */

typedef struct _MdfdVec
{
	File		mdfd_vfd;		/* fd number in fd.c's pool */
	BlockNumber mdfd_segno;		/* segment number, from 0 */
#ifndef LET_OS_MANAGE_FILESIZE	/* for large relations */
	struct _MdfdVec *mdfd_chain;	/* next segment, or NULL */
#endif
} MdfdVec;

static MemoryContext MdCxt;		/* context for all md.c allocations */


/*
 * In some contexts (currently, standalone backends and the bgwriter process)
 * we keep track of pending fsync operations: we need to remember all relation
 * segments that have been written since the last checkpoint, so that we can
 * fsync them down to disk before completing the next checkpoint.  This hash
 * table remembers the pending operations.	We use a hash table not because
 * we want to look up individual operations, but simply as a convenient way
 * of eliminating duplicate requests.
 *
 * (Regular backends do not track pending operations locally, but forward
 * them to the bgwriter.)
 */
typedef struct
{
	RelFileNode rnode;			/* the targeted relation */
	BlockNumber segno;			/* which segment */
} PendingOperationEntry;

static HTAB *pendingOpsTable = NULL;


typedef enum					/* behavior for mdopen & _mdfd_getseg */
{
	EXTENSION_FAIL,				/* ereport if segment not present */
	EXTENSION_RETURN_NULL,		/* return NULL if not present */
	EXTENSION_CREATE			/* create new segments as needed */
} ExtensionBehavior;

/* local routines */
static MdfdVec *mdopen(SMgrRelation reln, ExtensionBehavior behavior);
static void register_dirty_segment(SMgrRelation reln, MdfdVec *seg);
static MdfdVec *_fdvec_alloc(void);

#ifndef LET_OS_MANAGE_FILESIZE
static MdfdVec *_mdfd_openseg(SMgrRelation reln, BlockNumber segno,
			  int oflags);
#endif
static MdfdVec *_mdfd_getseg(SMgrRelation reln, BlockNumber blkno,
							 bool isTemp, ExtensionBehavior behavior);
static BlockNumber _mdnblocks(SMgrRelation reln, MdfdVec *seg);


/*
 *	mdinit() -- Initialize private state for magnetic disk storage manager.
 */
void
mdinit(void)
{
	MdCxt = AllocSetContextCreate(TopMemoryContext,
								  "MdSmgr",
								  ALLOCSET_DEFAULT_MINSIZE,
								  ALLOCSET_DEFAULT_INITSIZE,
								  ALLOCSET_DEFAULT_MAXSIZE);

	/*
	 * Create pending-operations hashtable if we need it.  Currently, we need
	 * it if we are standalone (not under a postmaster) OR if we are a
	 * bootstrap-mode subprocess of a postmaster (that is, a startup or
	 * bgwriter process).
	 */
	if (!IsUnderPostmaster || IsBootstrapProcessingMode())
	{
		HASHCTL		hash_ctl;

		MemSet(&hash_ctl, 0, sizeof(hash_ctl));
		hash_ctl.keysize = sizeof(PendingOperationEntry);
		hash_ctl.entrysize = sizeof(PendingOperationEntry);
		hash_ctl.hash = tag_hash;
		hash_ctl.hcxt = MdCxt;
		pendingOpsTable = hash_create("Pending Ops Table",
									  100L,
									  &hash_ctl,
								   HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);
	}
}

/*
 *	mdcreate() -- Create a new relation on magnetic disk.
 *
 * If isRedo is true, it's okay for the relation to exist already.
 */
void
mdcreate(SMgrRelation reln, bool isRedo)
{
	char	   *path;
	File		fd;

	if (isRedo && reln->md_fd != NULL)
		return;					/* created and opened already... */

	Assert(reln->md_fd == NULL);

	path = relpath(reln->smgr_rnode);

	fd = PathNameOpenFile(path, O_RDWR | O_CREAT | O_EXCL | PG_BINARY, 0600);

	if (fd < 0)
	{
		int			save_errno = errno;

		/*
		 * During bootstrap, there are cases where a system relation will be
		 * accessed (by internal backend processes) before the bootstrap
		 * script nominally creates it.  Therefore, allow the file to exist
		 * already, even if isRedo is not set.	(See also mdopen)
		 */
		if (isRedo || IsBootstrapProcessingMode())
			fd = PathNameOpenFile(path, O_RDWR | PG_BINARY, 0600);
		if (fd < 0)
		{
			pfree(path);
			/* be sure to report the error reported by create, not open */
			errno = save_errno;
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not create relation %u/%u/%u: %m",
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode)));
		}
	}

	pfree(path);

	reln->md_fd = _fdvec_alloc();

	reln->md_fd->mdfd_vfd = fd;
	reln->md_fd->mdfd_segno = 0;
#ifndef LET_OS_MANAGE_FILESIZE
	reln->md_fd->mdfd_chain = NULL;
#endif
}

/*
 *	mdunlink() -- Unlink a relation.
 *
 * Note that we're passed a RelFileNode --- by the time this is called,
 * there won't be an SMgrRelation hashtable entry anymore.
 *
 * If isRedo is true, it's okay for the relation to be already gone.
 * Also, any failure should be reported as WARNING not ERROR, because
 * we are usually not in a transaction anymore when this is called.
 */
void
mdunlink(RelFileNode rnode, bool isRedo)
{
	char	   *path;

	path = relpath(rnode);

	/* Delete the first segment, or only segment if not doing segmenting */
	if (unlink(path) < 0)
	{
		if (!isRedo || errno != ENOENT)
			ereport(WARNING,
					(errcode_for_file_access(),
					 errmsg("could not remove relation %u/%u/%u: %m",
							rnode.spcNode,
							rnode.dbNode,
							rnode.relNode)));
	}

#ifndef LET_OS_MANAGE_FILESIZE
	/* Delete the additional segments, if any */
	else
	{
		char	   *segpath = (char *) palloc(strlen(path) + 12);
		BlockNumber segno;

		/*
		 * Note that because we loop until getting ENOENT, we will
		 * correctly remove all inactive segments as well as active ones.
		 */
		for (segno = 1;; segno++)
		{
			sprintf(segpath, "%s.%u", path, segno);
			if (unlink(segpath) < 0)
			{
				/* ENOENT is expected after the last segment... */
				if (errno != ENOENT)
					ereport(WARNING,
							(errcode_for_file_access(),
							 errmsg("could not remove segment %u of relation %u/%u/%u: %m",
									segno,
									rnode.spcNode,
									rnode.dbNode,
									rnode.relNode)));
				break;
			}
		}
		pfree(segpath);
	}
#endif

	pfree(path);
}

/*
 *	mdextend() -- Add a block to the specified relation.
 *
 *		The semantics are nearly the same as mdwrite(): write at the
 *		specified position.  However, this is to be used for the case of
 *		extending a relation (i.e., blocknum is at or beyond the current
 *		EOF).  Note that we assume writing a block beyond current EOF
 *		causes intervening file space to become filled with zeroes.
 */
void
mdextend(SMgrRelation reln, BlockNumber blocknum, char *buffer, bool isTemp)
{
	long		seekpos;
	int			nbytes;
	MdfdVec    *v;

	/* This assert is too expensive to have on normally ... */
#ifdef CHECK_WRITE_VS_EXTEND
	Assert(blocknum >= mdnblocks(reln));
#endif

	/*
	 * If a relation manages to grow to 2^32-1 blocks, refuse to extend it
	 * any more --- we mustn't create a block whose number
	 * actually is InvalidBlockNumber.
	 */
	if (blocknum == InvalidBlockNumber)
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("cannot extend relation %u/%u/%u beyond %u blocks",
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode,
						InvalidBlockNumber)));

	v = _mdfd_getseg(reln, blocknum, isTemp, EXTENSION_CREATE);

#ifndef LET_OS_MANAGE_FILESIZE
	seekpos = (long) (BLCKSZ * (blocknum % ((BlockNumber) RELSEG_SIZE)));
	Assert(seekpos < BLCKSZ * RELSEG_SIZE);
#else
	seekpos = (long) (BLCKSZ * (blocknum));
#endif

	/*
	 * Note: because caller usually obtained blocknum by calling mdnblocks,
	 * which did a seek(SEEK_END), this seek is often redundant and will be
	 * optimized away by fd.c.  It's not redundant, however, if there is a
	 * partial page at the end of the file. In that case we want to try to
	 * overwrite the partial page with a full page.  It's also not redundant
	 * if bufmgr.c had to dump another buffer of the same file to make room
	 * for the new page's buffer.
	 */
	if (FileSeek(v->mdfd_vfd, seekpos, SEEK_SET) != seekpos)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not seek to block %u of relation %u/%u/%u: %m",
						blocknum,
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode)));

	if ((nbytes = FileWrite(v->mdfd_vfd, buffer, BLCKSZ)) != BLCKSZ)
	{
		if (nbytes < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not extend relation %u/%u/%u: %m",
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode),
					 errhint("Check free disk space.")));
		/* short write: complain appropriately */
		ereport(ERROR,
				(errcode(ERRCODE_DISK_FULL),
				 errmsg("could not extend relation %u/%u/%u: wrote only %d of %d bytes at block %u",
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode,
						nbytes, BLCKSZ, blocknum),
				 errhint("Check free disk space.")));
	}

	if (!isTemp)
		register_dirty_segment(reln, v);

#ifndef LET_OS_MANAGE_FILESIZE
	Assert(_mdnblocks(reln, v) <= ((BlockNumber) RELSEG_SIZE));
#endif
}

/*
 *	mdopen() -- Open the specified relation.
 *
 * Note we only open the first segment, when there are multiple segments.
 *
 * If first segment is not present, either ereport or return NULL according
 * to "behavior".  We treat EXTENSION_CREATE the same as EXTENSION_FAIL;
 * EXTENSION_CREATE means it's OK to extend an existing relation, not to
 * invent one out of whole cloth.
 */
static MdfdVec *
mdopen(SMgrRelation reln, ExtensionBehavior behavior)
{
	MdfdVec    *mdfd;
	char	   *path;
	File		fd;

	/* No work if already open */
	if (reln->md_fd)
		return reln->md_fd;

	path = relpath(reln->smgr_rnode);

	fd = PathNameOpenFile(path, O_RDWR | PG_BINARY, 0600);

	if (fd < 0)
	{
		/*
		 * During bootstrap, there are cases where a system relation will be
		 * accessed (by internal backend processes) before the bootstrap
		 * script nominally creates it.  Therefore, accept mdopen() as a
		 * substitute for mdcreate() in bootstrap mode only. (See mdcreate)
		 */
		if (IsBootstrapProcessingMode())
			fd = PathNameOpenFile(path, O_RDWR | O_CREAT | O_EXCL | PG_BINARY, 0600);
		if (fd < 0)
		{
			pfree(path);
			if (behavior == EXTENSION_RETURN_NULL && errno == ENOENT)
				return NULL;
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not open relation %u/%u/%u: %m",
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode)));
		}
	}

	pfree(path);

	reln->md_fd = mdfd = _fdvec_alloc();

	mdfd->mdfd_vfd = fd;
	mdfd->mdfd_segno = 0;
#ifndef LET_OS_MANAGE_FILESIZE
	mdfd->mdfd_chain = NULL;
	Assert(_mdnblocks(reln, mdfd) <= ((BlockNumber) RELSEG_SIZE));
#endif

	return mdfd;
}

/*
 *	mdclose() -- Close the specified relation, if it isn't closed already.
 */
void
mdclose(SMgrRelation reln)
{
	MdfdVec    *v = reln->md_fd;

	/* No work if already closed */
	if (v == NULL)
		return;

	reln->md_fd = NULL;			/* prevent dangling pointer after error */

#ifndef LET_OS_MANAGE_FILESIZE
	while (v != NULL)
	{
		MdfdVec    *ov = v;

		/* if not closed already */
		if (v->mdfd_vfd >= 0)
			FileClose(v->mdfd_vfd);
		/* Now free vector */
		v = v->mdfd_chain;
		pfree(ov);
	}
#else
	if (v->mdfd_vfd >= 0)
		FileClose(v->mdfd_vfd);
	pfree(v);
#endif
}

/*
 *	mdread() -- Read the specified block from a relation.
 */
void
mdread(SMgrRelation reln, BlockNumber blocknum, char *buffer)
{
	long		seekpos;
	int			nbytes;
	MdfdVec    *v;

	v = _mdfd_getseg(reln, blocknum, false, EXTENSION_FAIL);

#ifndef LET_OS_MANAGE_FILESIZE
	seekpos = (long) (BLCKSZ * (blocknum % ((BlockNumber) RELSEG_SIZE)));
	Assert(seekpos < BLCKSZ * RELSEG_SIZE);
#else
	seekpos = (long) (BLCKSZ * (blocknum));
#endif

	if (FileSeek(v->mdfd_vfd, seekpos, SEEK_SET) != seekpos)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not seek to block %u of relation %u/%u/%u: %m",
						blocknum,
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode)));

	if ((nbytes = FileRead(v->mdfd_vfd, buffer, BLCKSZ)) != BLCKSZ)
	{
		if (nbytes < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not read block %u of relation %u/%u/%u: %m",
							blocknum,
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode)));
		/*
		 * Short read: we are at or past EOF, or we read a partial block at
		 * EOF.  Normally this is an error; upper levels should never try to
		 * read a nonexistent block.  However, if zero_damaged_pages is ON
		 * or we are InRecovery, we should instead return zeroes without
		 * complaining.  This allows, for example, the case of trying to
		 * update a block that was later truncated away.
		 */
		if (zero_damaged_pages || InRecovery)
			MemSet(buffer, 0, BLCKSZ);
		else
			ereport(ERROR,
					(errcode(ERRCODE_DATA_CORRUPTED),
					 errmsg("could not read block %u of relation %u/%u/%u: read only %d of %d bytes",
							blocknum,
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode,
							nbytes, BLCKSZ)));
	}
}

/*
 *	mdwrite() -- Write the supplied block at the appropriate location.
 *
 *		This is to be used only for updating already-existing blocks of a
 *		relation (ie, those before the current EOF).  To extend a relation,
 *		use mdextend().
 */
void
mdwrite(SMgrRelation reln, BlockNumber blocknum, char *buffer, bool isTemp)
{
	long		seekpos;
	int			nbytes;
	MdfdVec    *v;

	/* This assert is too expensive to have on normally ... */
#ifdef CHECK_WRITE_VS_EXTEND
	Assert(blocknum < mdnblocks(reln));
#endif

	v = _mdfd_getseg(reln, blocknum, isTemp, EXTENSION_FAIL);

#ifndef LET_OS_MANAGE_FILESIZE
	seekpos = (long) (BLCKSZ * (blocknum % ((BlockNumber) RELSEG_SIZE)));
	Assert(seekpos < BLCKSZ * RELSEG_SIZE);
#else
	seekpos = (long) (BLCKSZ * (blocknum));
#endif

	if (FileSeek(v->mdfd_vfd, seekpos, SEEK_SET) != seekpos)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not seek to block %u of relation %u/%u/%u: %m",
						blocknum,
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode)));

	if ((nbytes = FileWrite(v->mdfd_vfd, buffer, BLCKSZ)) != BLCKSZ)
	{
		if (nbytes < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not write block %u of relation %u/%u/%u: %m",
							blocknum,
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode)));
		/* short write: complain appropriately */
		ereport(ERROR,
				(errcode(ERRCODE_DISK_FULL),
				 errmsg("could not write block %u of relation %u/%u/%u: wrote only %d of %d bytes",
						blocknum,
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode,
						nbytes, BLCKSZ),
				 errhint("Check free disk space.")));
	}

	if (!isTemp)
		register_dirty_segment(reln, v);
}

/*
 *	mdnblocks() -- Get the number of blocks stored in a relation.
 *
 *		Important side effect: all active segments of the relation are opened
 *		and added to the mdfd_chain list.  If this routine has not been
 *		called, then only segments up to the last one actually touched
 *		are present in the chain.
 */
BlockNumber
mdnblocks(SMgrRelation reln)
{
	MdfdVec    *v = mdopen(reln, EXTENSION_FAIL);

#ifndef LET_OS_MANAGE_FILESIZE
	BlockNumber nblocks;
	BlockNumber segno = 0;

	/*
	 * Skip through any segments that aren't the last one, to avoid redundant
	 * seeks on them.  We have previously verified that these segments are
	 * exactly RELSEG_SIZE long, and it's useless to recheck that each time.
	 *
	 * NOTE: this assumption could only be wrong if another backend has
	 * truncated the relation.	We rely on higher code levels to handle that
	 * scenario by closing and re-opening the md fd, which is handled via
	 * relcache flush.  (Since the bgwriter doesn't participate in relcache
	 * flush, it could have segment chain entries for inactive segments;
	 * that's OK because the bgwriter never needs to compute relation size.)
	 */
	while (v->mdfd_chain != NULL)
	{
		segno++;
		v = v->mdfd_chain;
	}

	for (;;)
	{
		nblocks = _mdnblocks(reln, v);
		if (nblocks > ((BlockNumber) RELSEG_SIZE))
			elog(FATAL, "segment too big");
		if (nblocks < ((BlockNumber) RELSEG_SIZE))
			return (segno * ((BlockNumber) RELSEG_SIZE)) + nblocks;

		/*
		 * If segment is exactly RELSEG_SIZE, advance to next one.
		 */
		segno++;

		if (v->mdfd_chain == NULL)
		{
			/*
			 * Because we pass O_CREAT, we will create the next segment (with
			 * zero length) immediately, if the last segment is of length
			 * RELSEG_SIZE.  While perhaps not strictly necessary, this keeps
			 * the logic simple.
			 */
			v->mdfd_chain = _mdfd_openseg(reln, segno, O_CREAT);
			if (v->mdfd_chain == NULL)
				ereport(ERROR,
						(errcode_for_file_access(),
						 errmsg("could not open segment %u of relation %u/%u/%u: %m",
								segno,
								reln->smgr_rnode.spcNode,
								reln->smgr_rnode.dbNode,
								reln->smgr_rnode.relNode)));
		}

		v = v->mdfd_chain;
	}
#else
	return _mdnblocks(reln, v);
#endif
}

/*
 *	mdtruncate() -- Truncate relation to specified number of blocks.
 */
void
mdtruncate(SMgrRelation reln, BlockNumber nblocks, bool isTemp)
{
	MdfdVec    *v;
	BlockNumber curnblk;

#ifndef LET_OS_MANAGE_FILESIZE
	BlockNumber priorblocks;
#endif

	/*
	 * NOTE: mdnblocks makes sure we have opened all active segments, so
	 * that truncation loop will get them all!
	 */
	curnblk = mdnblocks(reln);
	if (nblocks > curnblk)
	{
		/* Bogus request ... but no complaint if InRecovery */
		if (InRecovery)
			return;
		ereport(ERROR,
				(errmsg("could not truncate relation %u/%u/%u to %u blocks: it's only %u blocks now",
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode,
						nblocks, curnblk)));
	}
	if (nblocks == curnblk)
		return;					/* no work */

	v = mdopen(reln, EXTENSION_FAIL);

#ifndef LET_OS_MANAGE_FILESIZE
	priorblocks = 0;
	while (v != NULL)
	{
		MdfdVec    *ov = v;

		if (priorblocks > nblocks)
		{
			/*
			 * This segment is no longer active (and has already been
			 * unlinked from the mdfd_chain). We truncate the file, but do
			 * not delete it, for reasons explained in the header comments.
			 */
			if (FileTruncate(v->mdfd_vfd, 0) < 0)
				ereport(ERROR,
						(errcode_for_file_access(),
						 errmsg("could not truncate relation %u/%u/%u to %u blocks: %m",
								reln->smgr_rnode.spcNode,
								reln->smgr_rnode.dbNode,
								reln->smgr_rnode.relNode,
								nblocks)));
			if (!isTemp)
				register_dirty_segment(reln, v);
			v = v->mdfd_chain;
			Assert(ov != reln->md_fd);	/* we never drop the 1st segment */
			pfree(ov);
		}
		else if (priorblocks + ((BlockNumber) RELSEG_SIZE) > nblocks)
		{
			/*
			 * This is the last segment we want to keep. Truncate the file to
			 * the right length, and clear chain link that points to any
			 * remaining segments (which we shall zap). NOTE: if nblocks is
			 * exactly a multiple K of RELSEG_SIZE, we will truncate the K+1st
			 * segment to 0 length but keep it. This adheres to the invariant
			 * given in the header comments.
			 */
			BlockNumber lastsegblocks = nblocks - priorblocks;

			if (FileTruncate(v->mdfd_vfd, lastsegblocks * BLCKSZ) < 0)
				ereport(ERROR,
						(errcode_for_file_access(),
						 errmsg("could not truncate relation %u/%u/%u to %u blocks: %m",
								reln->smgr_rnode.spcNode,
								reln->smgr_rnode.dbNode,
								reln->smgr_rnode.relNode,
								nblocks)));
			if (!isTemp)
				register_dirty_segment(reln, v);
			v = v->mdfd_chain;
			ov->mdfd_chain = NULL;
		}
		else
		{
			/*
			 * We still need this segment and 0 or more blocks beyond it, so
			 * nothing to do here.
			 */
			v = v->mdfd_chain;
		}
		priorblocks += RELSEG_SIZE;
	}
#else
	if (FileTruncate(v->mdfd_vfd, nblocks * BLCKSZ) < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
			  errmsg("could not truncate relation %u/%u/%u to %u blocks: %m",
					 reln->smgr_rnode.spcNode,
					 reln->smgr_rnode.dbNode,
					 reln->smgr_rnode.relNode,
					 nblocks)));
	if (!isTemp)
		register_dirty_segment(reln, v);
#endif
}

/*
 *	mdimmedsync() -- Immediately sync a relation to stable storage.
 *
 * Note that only writes already issued are synced; this routine knows
 * nothing of dirty buffers that may exist inside the buffer manager.
 */
void
mdimmedsync(SMgrRelation reln)
{
	MdfdVec    *v;
	BlockNumber curnblk;

	/*
	 * NOTE: mdnblocks makes sure we have opened all active segments, so
	 * that fsync loop will get them all!
	 */
	curnblk = mdnblocks(reln);

	v = mdopen(reln, EXTENSION_FAIL);

#ifndef LET_OS_MANAGE_FILESIZE
	while (v != NULL)
	{
		if (FileSync(v->mdfd_vfd) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not fsync segment %u of relation %u/%u/%u: %m",
							v->mdfd_segno,
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode)));
		v = v->mdfd_chain;
	}
#else
	if (FileSync(v->mdfd_vfd) < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not fsync segment %u of relation %u/%u/%u: %m",
						v->mdfd_segno,
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode)));
#endif
}

/*
 *	mdsync() -- Sync previous writes to stable storage.
 *
 * This is only called during checkpoints, and checkpoints should only
 * occur in processes that have created a pendingOpsTable.
 */
void
mdsync(void)
{
	HASH_SEQ_STATUS hstat;
	PendingOperationEntry *entry;
	int			absorb_counter;

	if (!pendingOpsTable)
		elog(ERROR, "cannot sync without a pendingOpsTable");

	/*
	 * If we are in the bgwriter, the sync had better include all fsync
	 * requests that were queued by backends before the checkpoint REDO point
	 * was determined.	We go that a little better by accepting all requests
	 * queued up to the point where we start fsync'ing.
	 */
	AbsorbFsyncRequests();

	absorb_counter = FSYNCS_PER_ABSORB;
	hash_seq_init(&hstat, pendingOpsTable);
	while ((entry = (PendingOperationEntry *) hash_seq_search(&hstat)) != NULL)
	{
		/*
		 * If fsync is off then we don't have to bother opening the file at
		 * all.  (We delay checking until this point so that changing fsync on
		 * the fly behaves sensibly.)
		 */
		if (enableFsync)
		{
			SMgrRelation reln;
			MdfdVec    *seg;

			/*
			 * If in bgwriter, absorb pending requests every so often to
			 * prevent overflow of the fsync request queue.  The hashtable
			 * code does not specify whether entries added by this will be
			 * visited by our search, but we don't really care: it's OK if we
			 * do, and OK if we don't.
			 */
			if (--absorb_counter <= 0)
			{
				AbsorbFsyncRequests();
				absorb_counter = FSYNCS_PER_ABSORB;
			}

			/*
			 * Find or create an smgr hash entry for this relation. This may
			 * seem a bit unclean -- md calling smgr?  But it's really the
			 * best solution.  It ensures that the open file reference isn't
			 * permanently leaked if we get an error here. (You may say "but
			 * an unreferenced SMgrRelation is still a leak!" Not really,
			 * because the only case in which a checkpoint is done by a
			 * process that isn't about to shut down is in the bgwriter, and
			 * it will periodically do smgrcloseall().	This fact justifies
			 * our not closing the reln in the success path either, which is a
			 * good thing since in non-bgwriter cases we couldn't safely do
			 * that.)  Furthermore, in many cases the relation will have been
			 * dirtied through this same smgr relation, and so we can save a
			 * file open/close cycle.
			 */
			reln = smgropen(entry->rnode);

			/*
			 * It is possible that the relation has been dropped or truncated
			 * since the fsync request was entered.  Therefore, we have to
			 * allow file-not-found errors.  This applies both during
			 * _mdfd_getseg() and during FileSync, since fd.c might have
			 * closed the file behind our back.
			 */
			seg = _mdfd_getseg(reln,
							   entry->segno * ((BlockNumber) RELSEG_SIZE),
							   false, EXTENSION_RETURN_NULL);
			if (seg)
			{
				if (FileSync(seg->mdfd_vfd) < 0 &&
					errno != ENOENT)
					ereport(ERROR,
							(errcode_for_file_access(),
							 errmsg("could not fsync segment %u of relation %u/%u/%u: %m",
									entry->segno,
									entry->rnode.spcNode,
									entry->rnode.dbNode,
									entry->rnode.relNode)));
			}
		}

		/* Okay, delete this entry */
		if (hash_search(pendingOpsTable, entry,
						HASH_REMOVE, NULL) == NULL)
			elog(ERROR, "pendingOpsTable corrupted");
	}
}

/*
 * register_dirty_segment() -- Mark a relation segment as needing fsync
 *
 * If there is a local pending-ops table, just make an entry in it for
 * mdsync to process later.  Otherwise, try to pass off the fsync request
 * to the background writer process.  If that fails, just do the fsync
 * locally before returning (we expect this will not happen often enough
 * to be a performance problem).
 */
static void
register_dirty_segment(SMgrRelation reln, MdfdVec *seg)
{
	if (pendingOpsTable)
	{
		PendingOperationEntry entry;

		/* ensure any pad bytes in the struct are zeroed */
		MemSet(&entry, 0, sizeof(entry));
		entry.rnode = reln->smgr_rnode;
		entry.segno = seg->mdfd_segno;

		(void) hash_search(pendingOpsTable, &entry, HASH_ENTER, NULL);
	}
	else
	{
		if (ForwardFsyncRequest(reln->smgr_rnode, seg->mdfd_segno))
			return;				/* passed it off successfully */

		if (FileSync(seg->mdfd_vfd) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not fsync segment %u of relation %u/%u/%u: %m",
							seg->mdfd_segno,
							reln->smgr_rnode.spcNode,
							reln->smgr_rnode.dbNode,
							reln->smgr_rnode.relNode)));
	}
}

/*
 * RememberFsyncRequest() -- callback from bgwriter side of fsync request
 *
 * We stuff the fsync request into the local hash table for execution
 * during the bgwriter's next checkpoint.
 */
void
RememberFsyncRequest(RelFileNode rnode, BlockNumber segno)
{
	PendingOperationEntry entry;

	Assert(pendingOpsTable);

	/* ensure any pad bytes in the struct are zeroed */
	MemSet(&entry, 0, sizeof(entry));
	entry.rnode = rnode;
	entry.segno = segno;

	(void) hash_search(pendingOpsTable, &entry, HASH_ENTER, NULL);
}

/*
 *	_fdvec_alloc() -- Make a MdfdVec object.
 */
static MdfdVec *
_fdvec_alloc(void)
{
	return (MdfdVec *) MemoryContextAlloc(MdCxt, sizeof(MdfdVec));
}

#ifndef LET_OS_MANAGE_FILESIZE

/*
 * Open the specified segment of the relation,
 * and make a MdfdVec object for it.  Returns NULL on failure.
 */
static MdfdVec *
_mdfd_openseg(SMgrRelation reln, BlockNumber segno, int oflags)
{
	MdfdVec    *v;
	int			fd;
	char	   *path,
			   *fullpath;

	path = relpath(reln->smgr_rnode);

	if (segno > 0)
	{
		/* be sure we have enough space for the '.segno' */
		fullpath = (char *) palloc(strlen(path) + 12);
		sprintf(fullpath, "%s.%u", path, segno);
		pfree(path);
	}
	else
		fullpath = path;

	/* open the file */
	fd = PathNameOpenFile(fullpath, O_RDWR | PG_BINARY | oflags, 0600);

	pfree(fullpath);

	if (fd < 0)
		return NULL;

	/* allocate an mdfdvec entry for it */
	v = _fdvec_alloc();

	/* fill the entry */
	v->mdfd_vfd = fd;
	v->mdfd_segno = segno;
	v->mdfd_chain = NULL;
	Assert(_mdnblocks(reln, v) <= ((BlockNumber) RELSEG_SIZE));

	/* all done */
	return v;
}
#endif   /* LET_OS_MANAGE_FILESIZE */

/*
 *	_mdfd_getseg() -- Find the segment of the relation holding the
 *		specified block.
 *
 * If the segment doesn't exist, we ereport, return NULL, or create the
 * segment, according to "behavior".  Note: isTemp need only be correct
 * in the EXTENSION_CREATE case.
 */
static MdfdVec *
_mdfd_getseg(SMgrRelation reln, BlockNumber blkno, bool isTemp,
			 ExtensionBehavior behavior)
{
	MdfdVec    *v = mdopen(reln, behavior);

#ifndef LET_OS_MANAGE_FILESIZE
	BlockNumber targetseg;
	BlockNumber nextsegno;

	if (!v)
		return NULL;			/* only possible if EXTENSION_RETURN_NULL */

	targetseg = blkno / ((BlockNumber) RELSEG_SIZE);
	for (nextsegno = 1; nextsegno <= targetseg; nextsegno++)
	{
		Assert(nextsegno == v->mdfd_segno + 1);

		if (v->mdfd_chain == NULL)
		{
			/*
			 * Normally we will create new segments only if authorized by
			 * the caller (i.e., we are doing mdextend()).  But when doing
			 * WAL recovery, create segments anyway; this allows cases such as
			 * replaying WAL data that has a write into a high-numbered
			 * segment of a relation that was later deleted.  We want to go
			 * ahead and create the segments so we can finish out the replay.
			 *
			 * We have to maintain the invariant that segments before the
			 * last active segment are of size RELSEG_SIZE; therefore, pad
			 * them out with zeroes if needed.  (This only matters if caller
			 * is extending the relation discontiguously, but that can happen
			 * in hash indexes.)
			 */
			if (behavior == EXTENSION_CREATE || InRecovery)
			{
				if (_mdnblocks(reln, v) < RELSEG_SIZE)
				{
					char   *zerobuf = palloc0(BLCKSZ);

					mdextend(reln, nextsegno * ((BlockNumber) RELSEG_SIZE) - 1,
							 zerobuf, isTemp);
					pfree(zerobuf);
				}
				v->mdfd_chain = _mdfd_openseg(reln, nextsegno, O_CREAT);
			}
			else
			{
				/* We won't create segment if not existent */
				v->mdfd_chain = _mdfd_openseg(reln, nextsegno, 0);
			}
			if (v->mdfd_chain == NULL)
			{
				if (behavior == EXTENSION_RETURN_NULL && errno == ENOENT)
					return NULL;
				ereport(ERROR,
						(errcode_for_file_access(),
						 errmsg("could not open segment %u of relation %u/%u/%u (target block %u): %m",
								nextsegno,
								reln->smgr_rnode.spcNode,
								reln->smgr_rnode.dbNode,
								reln->smgr_rnode.relNode,
								blkno)));
			}
		}
		v = v->mdfd_chain;
	}
#endif

	return v;
}

/*
 * Get number of blocks present in a single disk file
 */
static BlockNumber
_mdnblocks(SMgrRelation reln, MdfdVec *seg)
{
	long		len;

	len = FileSeek(seg->mdfd_vfd, 0L, SEEK_END);
	if (len < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not seek to end of segment %u of relation %u/%u/%u: %m",
						seg->mdfd_segno,
						reln->smgr_rnode.spcNode,
						reln->smgr_rnode.dbNode,
						reln->smgr_rnode.relNode)));
	/* note that this calculation will ignore any partial block at EOF */
	return (BlockNumber) (len / BLCKSZ);
}
