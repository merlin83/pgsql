/*-------------------------------------------------------------------------
 *
 * xlogutils.c
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/transam/xlogutils.c,v 1.18 2001-08-25 18:52:41 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/htup.h"
#include "access/xlogutils.h"
#include "catalog/pg_database.h"
#include "lib/hasht.h"
#include "storage/bufpage.h"
#include "storage/smgr.h"
#include "utils/relcache.h"


/*
 * ---------------------------------------------------------------
 *
 * Index support functions
 *
 *----------------------------------------------------------------
 */

/*
 * Check if specified heap tuple was inserted by given
 * xaction/command and return
 *
 * - -1 if not
 * - 0	if there is no tuple at all
 * - 1	if yes
 */
int
XLogIsOwnerOfTuple(RelFileNode hnode, ItemPointer iptr,
				   TransactionId xid, CommandId cid)
{
	Relation	reln;
	Buffer		buffer;
	Page		page;
	ItemId		lp;
	HeapTupleHeader htup;

	reln = XLogOpenRelation(false, RM_HEAP_ID, hnode);
	if (!RelationIsValid(reln))
		return (0);

	buffer = ReadBuffer(reln, ItemPointerGetBlockNumber(iptr));
	if (!BufferIsValid(buffer))
		return (0);

	LockBuffer(buffer, BUFFER_LOCK_SHARE);
	page = (Page) BufferGetPage(buffer);
	if (PageIsNew((PageHeader) page) ||
		ItemPointerGetOffsetNumber(iptr) > PageGetMaxOffsetNumber(page))
	{
		UnlockAndReleaseBuffer(buffer);
		return (0);
	}
	lp = PageGetItemId(page, ItemPointerGetOffsetNumber(iptr));
	if (!ItemIdIsUsed(lp) || ItemIdDeleted(lp))
	{
		UnlockAndReleaseBuffer(buffer);
		return (0);
	}

	htup = (HeapTupleHeader) PageGetItem(page, lp);

	Assert(PageGetSUI(page) == ThisStartUpID);
	if (!TransactionIdEquals(htup->t_xmin, xid) || htup->t_cmin != cid)
	{
		UnlockAndReleaseBuffer(buffer);
		return (-1);
	}

	UnlockAndReleaseBuffer(buffer);
	return (1);
}

/*
 * MUST BE CALLED ONLY ON RECOVERY.
 *
 * Check if exists valid (inserted by not aborted xaction) heap tuple
 * for given item pointer
 */
bool
XLogIsValidTuple(RelFileNode hnode, ItemPointer iptr)
{
	Relation	reln;
	Buffer		buffer;
	Page		page;
	ItemId		lp;
	HeapTupleHeader htup;

	reln = XLogOpenRelation(false, RM_HEAP_ID, hnode);
	if (!RelationIsValid(reln))
		return (false);

	buffer = ReadBuffer(reln, ItemPointerGetBlockNumber(iptr));
	if (!BufferIsValid(buffer))
		return (false);

	LockBuffer(buffer, BUFFER_LOCK_SHARE);
	page = (Page) BufferGetPage(buffer);
	if (PageIsNew((PageHeader) page) ||
		ItemPointerGetOffsetNumber(iptr) > PageGetMaxOffsetNumber(page))
	{
		UnlockAndReleaseBuffer(buffer);
		return (false);
	}

	if (PageGetSUI(page) != ThisStartUpID)
	{
		Assert(PageGetSUI(page) < ThisStartUpID);
		UnlockAndReleaseBuffer(buffer);
		return (true);
	}

	lp = PageGetItemId(page, ItemPointerGetOffsetNumber(iptr));
	if (!ItemIdIsUsed(lp) || ItemIdDeleted(lp))
	{
		UnlockAndReleaseBuffer(buffer);
		return (false);
	}

	htup = (HeapTupleHeader) PageGetItem(page, lp);

	/* MUST CHECK WASN'T TUPLE INSERTED IN PREV STARTUP */

	if (!(htup->t_infomask & HEAP_XMIN_COMMITTED))
	{
		if (htup->t_infomask & HEAP_XMIN_INVALID ||
			(htup->t_infomask & HEAP_MOVED_IN &&
			 TransactionIdDidAbort((TransactionId) htup->t_cmin)) ||
			TransactionIdDidAbort(htup->t_xmin))
		{
			UnlockAndReleaseBuffer(buffer);
			return (false);
		}
	}

	UnlockAndReleaseBuffer(buffer);
	return (true);
}

/*
 * ---------------------------------------------------------------
 *
 * Storage related support functions
 *
 *----------------------------------------------------------------
 */

Buffer
XLogReadBuffer(bool extend, Relation reln, BlockNumber blkno)
{
	BlockNumber lastblock = RelationGetNumberOfBlocks(reln);
	Buffer		buffer;

	if (blkno >= lastblock)
	{
		buffer = InvalidBuffer;
		if (extend)				/* we do this in recovery only - no locks */
		{
			Assert(InRecovery);
			while (lastblock <= blkno)
			{
				if (buffer != InvalidBuffer)
					ReleaseBuffer(buffer);		/* must be WriteBuffer()? */
				buffer = ReadBuffer(reln, P_NEW);
				lastblock++;
			}
		}
		if (buffer != InvalidBuffer)
			LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
		return (buffer);
	}

	buffer = ReadBuffer(reln, blkno);
	if (buffer != InvalidBuffer)
		LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
	return (buffer);
}

/*
 * "Relation" cache
 */

typedef struct XLogRelDesc
{
	RelationData reldata;
	struct XLogRelDesc *lessRecently;
	struct XLogRelDesc *moreRecently;
} XLogRelDesc;

typedef struct XLogRelCacheEntry
{
	RelFileNode rnode;
	XLogRelDesc *rdesc;
} XLogRelCacheEntry;

static HTAB *_xlrelcache;
static XLogRelDesc *_xlrelarr = NULL;
static Form_pg_class _xlpgcarr = NULL;
static int	_xlast = 0;
static int	_xlcnt = 0;

#define _XLOG_RELCACHESIZE	512

static void
_xl_init_rel_cache(void)
{
	HASHCTL		ctl;

	_xlcnt = _XLOG_RELCACHESIZE;
	_xlast = 0;
	_xlrelarr = (XLogRelDesc *) malloc(sizeof(XLogRelDesc) * _xlcnt);
	memset(_xlrelarr, 0, sizeof(XLogRelDesc) * _xlcnt);
	_xlpgcarr = (Form_pg_class) malloc(sizeof(FormData_pg_class) * _xlcnt);
	memset(_xlpgcarr, 0, sizeof(FormData_pg_class) * _xlcnt);

	_xlrelarr[0].moreRecently = &(_xlrelarr[0]);
	_xlrelarr[0].lessRecently = &(_xlrelarr[0]);

	memset(&ctl, 0, (int) sizeof(ctl));
	ctl.keysize = sizeof(RelFileNode);
	ctl.datasize = sizeof(XLogRelDesc *);
	ctl.hash = tag_hash;

	_xlrelcache = hash_create(_XLOG_RELCACHESIZE, &ctl,
							  HASH_ELEM | HASH_FUNCTION);
}

static void
_xl_remove_hash_entry(XLogRelDesc **edata, Datum dummy)
{
	XLogRelCacheEntry *hentry;
	bool		found;
	XLogRelDesc *rdesc = *edata;
	Form_pg_class tpgc = rdesc->reldata.rd_rel;

	rdesc->lessRecently->moreRecently = rdesc->moreRecently;
	rdesc->moreRecently->lessRecently = rdesc->lessRecently;

	hentry = (XLogRelCacheEntry *) hash_search(_xlrelcache,
				(char *) &(rdesc->reldata.rd_node), HASH_REMOVE, &found);

	if (hentry == NULL)
		elog(STOP, "_xl_remove_hash_entry: can't delete from cache");
	if (!found)
		elog(STOP, "_xl_remove_hash_entry: file was not found in cache");

	if (rdesc->reldata.rd_fd >= 0)
		smgrclose(DEFAULT_SMGR, &(rdesc->reldata));

	memset(rdesc, 0, sizeof(XLogRelDesc));
	memset(tpgc, 0, sizeof(FormData_pg_class));
	rdesc->reldata.rd_rel = tpgc;

	return;
}

static XLogRelDesc *
_xl_new_reldesc(void)
{
	XLogRelDesc *res;

	_xlast++;
	if (_xlast < _xlcnt)
	{
		_xlrelarr[_xlast].reldata.rd_rel = &(_xlpgcarr[_xlast]);
		return (&(_xlrelarr[_xlast]));
	}

	/* reuse */
	res = _xlrelarr[0].moreRecently;

	_xl_remove_hash_entry(&res, 0);

	_xlast--;
	return (res);
}


void
XLogInitRelationCache(void)
{
	CreateDummyCaches();
	_xl_init_rel_cache();
}

void
XLogCloseRelationCache(void)
{

	DestroyDummyCaches();

	if (!_xlrelarr)
		return;

	HashTableWalk(_xlrelcache, (HashtFunc) _xl_remove_hash_entry, 0);
	hash_destroy(_xlrelcache);

	free(_xlrelarr);
	free(_xlpgcarr);

	_xlrelarr = NULL;
}

Relation
XLogOpenRelation(bool redo, RmgrId rmid, RelFileNode rnode)
{
	XLogRelDesc *res;
	XLogRelCacheEntry *hentry;
	bool		found;

	hentry = (XLogRelCacheEntry *)
		hash_search(_xlrelcache, (char *) &rnode, HASH_FIND, &found);

	if (hentry == NULL)
		elog(STOP, "XLogOpenRelation: error in cache");

	if (found)
	{
		res = hentry->rdesc;

		res->lessRecently->moreRecently = res->moreRecently;
		res->moreRecently->lessRecently = res->lessRecently;
	}
	else
	{
		res = _xl_new_reldesc();

		sprintf(RelationGetPhysicalRelationName(&(res->reldata)), "%u", rnode.relNode);

		/* unexisting DB id */
		res->reldata.rd_lockInfo.lockRelId.dbId = RecoveryDb;
		res->reldata.rd_lockInfo.lockRelId.relId = rnode.relNode;
		res->reldata.rd_node = rnode;

		hentry = (XLogRelCacheEntry *)
			hash_search(_xlrelcache, (char *) &rnode, HASH_ENTER, &found);

		if (hentry == NULL)
			elog(STOP, "XLogOpenRelation: can't insert into cache");

		if (found)
			elog(STOP, "XLogOpenRelation: file found on insert into cache");

		hentry->rdesc = res;

		res->reldata.rd_targblock = InvalidBlockNumber;
		res->reldata.rd_fd = -1;
		res->reldata.rd_fd = smgropen(DEFAULT_SMGR, &(res->reldata),
									  true /* allow failure */ );
	}

	res->moreRecently = &(_xlrelarr[0]);
	res->lessRecently = _xlrelarr[0].lessRecently;
	_xlrelarr[0].lessRecently = res;
	res->lessRecently->moreRecently = res;

	if (res->reldata.rd_fd < 0) /* file doesn't exist */
		return (NULL);

	return (&(res->reldata));
}
