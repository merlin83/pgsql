/*-------------------------------------------------------------------------
 *
 * heapam.c
 *	  heap access method code
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/heap/heapam.c,v 1.71 2000-06-15 04:09:34 momjian Exp $
 *
 *
 * INTERFACE ROUTINES
 *		heapgettup		- fetch next heap tuple from a scan
 *		heap_open		- open a heap relation by relationId
 *		heap_openr		- open a heap relation by name
 *		heap_close		- close a heap relation
 *		heap_beginscan	- begin relation scan
 *		heap_rescan		- restart a relation scan
 *		heap_endscan	- end relation scan
 *		heap_getnext	- retrieve next tuple in scan
 *		heap_fetch		- retrive tuple with tid
 *		heap_insert		- insert tuple into a relation
 *		heap_delete		- delete a tuple from a relation
 *		heap_update - replace a tuple in a relation with another tuple
 *		heap_markpos	- mark scan position
 *		heap_restrpos	- restore position to marked location
 *
 * NOTES
 *	  This file contains the heap_ routines which implement
 *	  the POSTGRES heap access method used for all POSTGRES
 *	  relations.
 *
 * OLD COMMENTS
 *		struct relscan hints:  (struct should be made AM independent?)
 *
 *		rs_ctid is the tid of the last tuple returned by getnext.
 *		rs_ptid and rs_ntid are the tids of the previous and next tuples
 *		returned by getnext, respectively.	NULL indicates an end of
 *		scan (either direction); NON indicates an unknow value.
 *
 *		possible combinations:
 *		rs_p	rs_c	rs_n			interpretation
 *		NULL	NULL	NULL			empty scan
 *		NULL	NULL	NON				at begining of scan
 *		NULL	NULL	t1				at begining of scan (with cached tid)
 *		NON		NULL	NULL			at end of scan
 *		t1		NULL	NULL			at end of scan (with cached tid)
 *		NULL	t1		NULL			just returned only tuple
 *		NULL	t1		NON				just returned first tuple
 *		NULL	t1		t2				returned first tuple (with cached tid)
 *		NON		t1		NULL			just returned last tuple
 *		t2		t1		NULL			returned last tuple (with cached tid)
 *		t1		t2		NON				in the middle of a forward scan
 *		NON		t2		t1				in the middle of a reverse scan
 *		ti		tj		tk				in the middle of a scan (w cached tid)
 *
 *		Here NULL is ...tup == NULL && ...buf == InvalidBuffer,
 *		and NON is ...tup == NULL && ...buf == UnknownBuffer.
 *
 *		Currently, the NONTID values are not cached with their actual
 *		values by getnext.	Values may be cached by markpos since it stores
 *		all three tids.
 *
 *		NOTE:  the calls to elog() must stop.  Should decide on an interface
 *		between the general and specific AM calls.
 *
 *		XXX probably do not need a free tuple routine for heaps.
 *		Huh?  Free tuple is not necessary for tuples returned by scans, but
 *		is necessary for tuples which are returned by
 *		RelationGetTupleByItemPointer. -hirohama
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/hio.h"
#include "access/valid.h"
#include "catalog/catalog.h"
#include "miscadmin.h"
#include "utils/inval.h"
#include "utils/relcache.h"


/* ----------------------------------------------------------------
 *						 heap support routines
 * ----------------------------------------------------------------
 */

/* ----------------
 *		initscan - scan code common to heap_beginscan and heap_rescan
 * ----------------
 */
static void
initscan(HeapScanDesc scan,
		 Relation relation,
		 int atend,
		 unsigned nkeys,
		 ScanKey key)
{
	/* ----------------
	 *	Make sure we have up-to-date idea of number of blocks in relation.
	 *	It is sufficient to do this once at scan start, since any tuples
	 *	added while the scan is in progress will be invisible to my
	 *	transaction anyway...
	 * ----------------
	 */
	relation->rd_nblocks = RelationGetNumberOfBlocks(relation);

	if (relation->rd_nblocks == 0)
	{
		/* ----------------
		 *	relation is empty
		 * ----------------
		 */
		scan->rs_ntup.t_datamcxt = scan->rs_ctup.t_datamcxt =
			scan->rs_ptup.t_datamcxt = NULL;
		scan->rs_ntup.t_data = scan->rs_ctup.t_data =
			scan->rs_ptup.t_data = NULL;
		scan->rs_nbuf = scan->rs_cbuf = scan->rs_pbuf = InvalidBuffer;
	}
	else if (atend)
	{
		/* ----------------
		 *	reverse scan
		 * ----------------
		 */
		scan->rs_ntup.t_datamcxt = scan->rs_ctup.t_datamcxt = NULL;
		scan->rs_ntup.t_data = scan->rs_ctup.t_data = NULL;
		scan->rs_nbuf = scan->rs_cbuf = InvalidBuffer;
		scan->rs_ptup.t_datamcxt = NULL;
		scan->rs_ptup.t_data = NULL;
		scan->rs_pbuf = UnknownBuffer;
	}
	else
	{
		/* ----------------
		 *	forward scan
		 * ----------------
		 */
		scan->rs_ctup.t_datamcxt = scan->rs_ptup.t_datamcxt = NULL;
		scan->rs_ctup.t_data = scan->rs_ptup.t_data = NULL;
		scan->rs_cbuf = scan->rs_pbuf = InvalidBuffer;
		scan->rs_ntup.t_datamcxt = NULL;
		scan->rs_ntup.t_data = NULL;
		scan->rs_nbuf = UnknownBuffer;
	}							/* invalid too */

	/* we don't have a marked position... */
	ItemPointerSetInvalid(&(scan->rs_mptid));
	ItemPointerSetInvalid(&(scan->rs_mctid));
	ItemPointerSetInvalid(&(scan->rs_mntid));
	ItemPointerSetInvalid(&(scan->rs_mcd));

	/* ----------------
	 *	copy the scan key, if appropriate
	 * ----------------
	 */
	if (key != NULL)
		memmove(scan->rs_key, key, nkeys * sizeof(ScanKeyData));
}

/* ----------------
 *		unpinscan - code common to heap_rescan and heap_endscan
 * ----------------
 */
static void
unpinscan(HeapScanDesc scan)
{
	if (BufferIsValid(scan->rs_pbuf))
		ReleaseBuffer(scan->rs_pbuf);

	/* ------------------------------------
	 *	Scan will pin buffer once for each non-NULL tuple pointer
	 *	(ptup, ctup, ntup), so they have to be unpinned multiple
	 *	times.
	 * ------------------------------------
	 */
	if (BufferIsValid(scan->rs_cbuf))
		ReleaseBuffer(scan->rs_cbuf);

	if (BufferIsValid(scan->rs_nbuf))
		ReleaseBuffer(scan->rs_nbuf);

	/*
	 * we don't bother to clear rs_pbuf etc --- caller must reinitialize
	 * them if scan descriptor is not being deleted.
	 */
}

/* ------------------------------------------
 *		nextpage
 *
 *		figure out the next page to scan after the current page
 *		taking into account of possible adjustment of degrees of
 *		parallelism
 * ------------------------------------------
 */
static int
nextpage(int page, int dir)
{
	return (dir < 0) ? page - 1 : page + 1;
}

/* ----------------
 *		heapgettup - fetch next heap tuple
 *
 *		routine used by heap_getnext() which does most of the
 *		real work in scanning tuples.
 *
 *		The scan routines handle their own buffer lock/unlocking, so
 *		there is no reason to request the buffer number unless
 *		to want to perform some other operation with the result,
 *		like pass it to another function.
 * ----------------
 */
static void
heapgettup(Relation relation,
		   HeapTuple tuple,
		   int dir,
		   Buffer *buffer,
		   Snapshot snapshot,
		   int nkeys,
		   ScanKey key)
{
	ItemId		lpp;
	Page		dp;
	int			page;
	int			pages;
	int			lines;
	OffsetNumber lineoff;
	int			linesleft;
	ItemPointer tid = (tuple->t_data == NULL) ?
	(ItemPointer) NULL : &(tuple->t_self);

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_heapgettup);
	IncrHeapAccessStat(global_heapgettup);

	/* ----------------
	 *	debugging stuff
	 *
	 * check validity of arguments, here and for other functions too
	 * Note: no locking manipulations needed--this is a local function
	 * ----------------
	 */
#ifdef	HEAPDEBUGALL
	if (ItemPointerIsValid(tid))
	{
		elog(DEBUG, "heapgettup(%s, tid=0x%x[%d,%d], dir=%d, ...)",
			 RelationGetRelationName(relation), tid, tid->ip_blkid,
			 tid->ip_posid, dir);
	}
	else
	{
		elog(DEBUG, "heapgettup(%s, tid=0x%x, dir=%d, ...)",
			 RelationGetRelationName(relation), tid, dir);
	}
	elog(DEBUG, "heapgettup(..., b=0x%x, nkeys=%d, key=0x%x", buffer, nkeys, key);

	elog(DEBUG, "heapgettup: relation(%c)=`%s', %p",
		 relation->rd_rel->relkind, RelationGetRelationName(relation),
		 snapshot);
#endif	 /* !defined(HEAPDEBUGALL) */

	if (!ItemPointerIsValid(tid))
		Assert(!PointerIsValid(tid));

	/* ----------------
	 *	return null immediately if relation is empty
	 * ----------------
	 */
	if (!(pages = relation->rd_nblocks))
	{
		tuple->t_datamcxt = NULL;
		tuple->t_data = NULL;
		return;
	}

	/* ----------------
	 *	calculate next starting lineoff, given scan direction
	 * ----------------
	 */
	if (!dir)
	{
		/* ----------------
		 * ``no movement'' scan direction
		 * ----------------
		 */
		/* assume it is a valid TID XXX */
		if (ItemPointerIsValid(tid) == false)
		{
			*buffer = InvalidBuffer;
			tuple->t_datamcxt = NULL;
			tuple->t_data = NULL;
			return;
		}
		*buffer = RelationGetBufferWithBuffer(relation,
										  ItemPointerGetBlockNumber(tid),
											  *buffer);

		if (!BufferIsValid(*buffer))
			elog(ERROR, "heapgettup: failed ReadBuffer");

		LockBuffer(*buffer, BUFFER_LOCK_SHARE);

		dp = (Page) BufferGetPage(*buffer);
		lineoff = ItemPointerGetOffsetNumber(tid);
		lpp = PageGetItemId(dp, lineoff);

		tuple->t_datamcxt = NULL;
		tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lpp);
		tuple->t_len = ItemIdGetLength(lpp);
		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
		return;

	}
	else if (dir < 0)
	{
		/* ----------------
		 *	reverse scan direction
		 * ----------------
		 */
		if (ItemPointerIsValid(tid) == false)
			tid = NULL;
		if (tid == NULL)
		{
			page = pages - 1;	/* final page */
		}
		else
		{
			page = ItemPointerGetBlockNumber(tid);		/* current page */
		}
		if (page < 0)
		{
			*buffer = InvalidBuffer;
			tuple->t_data = NULL;
			return;
		}

		*buffer = RelationGetBufferWithBuffer(relation, page, *buffer);
		if (!BufferIsValid(*buffer))
			elog(ERROR, "heapgettup: failed ReadBuffer");

		LockBuffer(*buffer, BUFFER_LOCK_SHARE);

		dp = (Page) BufferGetPage(*buffer);
		lines = PageGetMaxOffsetNumber(dp);
		if (tid == NULL)
		{
			lineoff = lines;	/* final offnum */
		}
		else
		{
			lineoff =			/* previous offnum */
				OffsetNumberPrev(ItemPointerGetOffsetNumber(tid));
		}
		/* page and lineoff now reference the physically previous tid */

	}
	else
	{
		/* ----------------
		 *	forward scan direction
		 * ----------------
		 */
		if (ItemPointerIsValid(tid) == false)
		{
			page = 0;			/* first page */
			lineoff = FirstOffsetNumber;		/* first offnum */
		}
		else
		{
			page = ItemPointerGetBlockNumber(tid);		/* current page */
			lineoff =			/* next offnum */
				OffsetNumberNext(ItemPointerGetOffsetNumber(tid));
		}

		if (page >= pages)
		{
			*buffer = InvalidBuffer;
			tuple->t_datamcxt = NULL;
			tuple->t_data = NULL;
			return;
		}
		/* page and lineoff now reference the physically next tid */

		*buffer = RelationGetBufferWithBuffer(relation, page, *buffer);
		if (!BufferIsValid(*buffer))
			elog(ERROR, "heapgettup: failed ReadBuffer");

		LockBuffer(*buffer, BUFFER_LOCK_SHARE);

		dp = (Page) BufferGetPage(*buffer);
		lines = PageGetMaxOffsetNumber(dp);
	}

	/* 'dir' is now non-zero */

	/* ----------------
	 *	calculate line pointer and number of remaining items
	 *	to check on this page.
	 * ----------------
	 */
	lpp = PageGetItemId(dp, lineoff);
	if (dir < 0)
		linesleft = lineoff - 1;
	else
		linesleft = lines - lineoff;

	/* ----------------
	 *	advance the scan until we find a qualifying tuple or
	 *	run out of stuff to scan
	 * ----------------
	 */
	for (;;)
	{
		while (linesleft >= 0)
		{
			if (ItemIdIsUsed(lpp))
			{
				tuple->t_datamcxt = NULL;
				tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lpp);
				tuple->t_len = ItemIdGetLength(lpp);
				ItemPointerSet(&(tuple->t_self), page, lineoff);
				/* ----------------
				 *	if current tuple qualifies, return it.
				 * ----------------
				 */
				HeapTupleSatisfies(tuple, relation, *buffer, (PageHeader) dp,
								   snapshot, nkeys, key);
				if (tuple->t_data != NULL)
				{
					LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
					return;
				}
			}

			/* ----------------
			 *	otherwise move to the next item on the page
			 * ----------------
			 */
			--linesleft;
			if (dir < 0)
			{
				--lpp;			/* move back in this page's ItemId array */
				--lineoff;
			}
			else
			{
				++lpp;			/* move forward in this page's ItemId
								 * array */
				++lineoff;
			}
		}

		/* ----------------
		 *	if we get here, it means we've exhausted the items on
		 *	this page and it's time to move to the next..
		 * ----------------
		 */
		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
		page = nextpage(page, dir);

		/* ----------------
		 *	return NULL if we've exhausted all the pages..
		 * ----------------
		 */
		if (page < 0 || page >= pages)
		{
			if (BufferIsValid(*buffer))
				ReleaseBuffer(*buffer);
			*buffer = InvalidBuffer;
			tuple->t_datamcxt = NULL;
			tuple->t_data = NULL;
			return;
		}

		*buffer = ReleaseAndReadBuffer(*buffer, relation, page);

		if (!BufferIsValid(*buffer))
			elog(ERROR, "heapgettup: failed ReadBuffer");
		LockBuffer(*buffer, BUFFER_LOCK_SHARE);
		dp = (Page) BufferGetPage(*buffer);
		lines = PageGetMaxOffsetNumber((Page) dp);
		linesleft = lines - 1;
		if (dir < 0)
		{
			lineoff = lines;
			lpp = PageGetItemId(dp, lines);
		}
		else
		{
			lineoff = FirstOffsetNumber;
			lpp = PageGetItemId(dp, FirstOffsetNumber);
		}
	}
}


/* ----------------------------------------------------------------
 *					 heap access method interface
 * ----------------------------------------------------------------
 */
/* ----------------
 *		heap_open - open a heap relation by relationId
 *
 *		If lockmode is "NoLock", no lock is obtained on the relation,
 *		and the caller must check for a NULL return value indicating
 *		that no such relation exists.
 *		Otherwise, an error is raised if the relation does not exist,
 *		and the specified kind of lock is obtained on the relation.
 * ----------------
 */
Relation
heap_open(Oid relationId, LOCKMODE lockmode)
{
	Relation	r;

	Assert(lockmode >= NoLock && lockmode < MAX_LOCKMODES);

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_open);
	IncrHeapAccessStat(global_open);

	/* The relcache does all the real work... */
	r = RelationIdGetRelation(relationId);

	/* Under no circumstances will we return an index as a relation. */
	if (RelationIsValid(r) && r->rd_rel->relkind == RELKIND_INDEX)
		elog(ERROR, "%s is an index relation", RelationGetRelationName(r));

	if (lockmode == NoLock)
		return r;				/* caller must check RelationIsValid! */

	if (!RelationIsValid(r))
		elog(ERROR, "Relation %u does not exist", relationId);

	LockRelation(r, lockmode);

	return r;
}

/* ----------------
 *		heap_openr - open a heap relation by name
 *
 *		If lockmode is "NoLock", no lock is obtained on the relation,
 *		and the caller must check for a NULL return value indicating
 *		that no such relation exists.
 *		Otherwise, an error is raised if the relation does not exist,
 *		and the specified kind of lock is obtained on the relation.
 * ----------------
 */
Relation
heap_openr(const char *relationName, LOCKMODE lockmode)
{
	Relation	r;

	Assert(lockmode >= NoLock && lockmode < MAX_LOCKMODES);

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_openr);
	IncrHeapAccessStat(global_openr);

	/* The relcache does all the real work... */
	r = RelationNameGetRelation(relationName);

	/* Under no circumstances will we return an index as a relation. */
	if (RelationIsValid(r) && r->rd_rel->relkind == RELKIND_INDEX)
		elog(ERROR, "%s is an index relation", RelationGetRelationName(r));

	if (lockmode == NoLock)
		return r;				/* caller must check RelationIsValid! */

	if (!RelationIsValid(r))
		elog(ERROR, "Relation '%s' does not exist", relationName);

	LockRelation(r, lockmode);

	return r;
}

/* ----------------
 *		heap_close - close a heap relation
 *
 *		If lockmode is not "NoLock", we first release the specified lock.
 *		Note that it is often sensible to hold a lock beyond heap_close;
 *		in that case, the lock is released automatically at xact end.
 * ----------------
 */
void
heap_close(Relation relation, LOCKMODE lockmode)
{
	Assert(lockmode >= NoLock && lockmode < MAX_LOCKMODES);

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_close);
	IncrHeapAccessStat(global_close);

	if (lockmode != NoLock)
		UnlockRelation(relation, lockmode);

	/* The relcache does the real work... */
	RelationClose(relation);
}


/* ----------------
 *		heap_beginscan	- begin relation scan
 * ----------------
 */
HeapScanDesc
heap_beginscan(Relation relation,
			   int atend,
			   Snapshot snapshot,
			   unsigned nkeys,
			   ScanKey key)
{
	HeapScanDesc scan;

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_beginscan);
	IncrHeapAccessStat(global_beginscan);

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	if (!RelationIsValid(relation))
		elog(ERROR, "heap_beginscan: !RelationIsValid(relation)");

	/* ----------------
	 *	increment relation ref count while scanning relation
	 * ----------------
	 */
	RelationIncrementReferenceCount(relation);

	/* ----------------
	 *	Acquire AccessShareLock for the duration of the scan
	 *
	 *	Note: we could get an SI inval message here and consequently have
	 *	to rebuild the relcache entry.	The refcount increment above
	 *	ensures that we will rebuild it and not just flush it...
	 * ----------------
	 */
	LockRelation(relation, AccessShareLock);

	/* XXX someday assert SelfTimeQual if relkind == RELKIND_UNCATALOGED */
	if (relation->rd_rel->relkind == RELKIND_UNCATALOGED)
		snapshot = SnapshotSelf;

	/* ----------------
	 *	allocate and initialize scan descriptor
	 * ----------------
	 */
	scan = (HeapScanDesc) palloc(sizeof(HeapScanDescData));

	scan->rs_rd = relation;
	scan->rs_atend = atend;
	scan->rs_snapshot = snapshot;
	scan->rs_nkeys = (short) nkeys;

	if (nkeys)

		/*
		 * we do this here instead of in initscan() because heap_rescan
		 * also calls initscan() and we don't want to allocate memory
		 * again
		 */
		scan->rs_key = (ScanKey) palloc(sizeof(ScanKeyData) * nkeys);
	else
		scan->rs_key = NULL;

	initscan(scan, relation, atend, nkeys, key);

	return scan;
}

/* ----------------
 *		heap_rescan		- restart a relation scan
 * ----------------
 */
void
heap_rescan(HeapScanDesc scan,
			bool scanFromEnd,
			ScanKey key)
{
	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_rescan);
	IncrHeapAccessStat(global_rescan);

	/* Note: set relation level read lock is still set */

	/* ----------------
	 *	unpin scan buffers
	 * ----------------
	 */
	unpinscan(scan);

	/* ----------------
	 *	reinitialize scan descriptor
	 * ----------------
	 */
	scan->rs_atend = (bool) scanFromEnd;
	initscan(scan, scan->rs_rd, scanFromEnd, scan->rs_nkeys, key);
}

/* ----------------
 *		heap_endscan	- end relation scan
 *
 *		See how to integrate with index scans.
 *		Check handling if reldesc caching.
 * ----------------
 */
void
heap_endscan(HeapScanDesc scan)
{
	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_endscan);
	IncrHeapAccessStat(global_endscan);

	/* Note: no locking manipulations needed */

	/* ----------------
	 *	unpin scan buffers
	 * ----------------
	 */
	unpinscan(scan);

	/* ----------------
	 *	Release AccessShareLock acquired by heap_beginscan()
	 * ----------------
	 */
	UnlockRelation(scan->rs_rd, AccessShareLock);

	/* ----------------
	 *	decrement relation reference count and free scan descriptor storage
	 * ----------------
	 */
	RelationDecrementReferenceCount(scan->rs_rd);

	if (scan->rs_key)
		pfree(scan->rs_key);

	pfree(scan);
}

/* ----------------
 *		heap_getnext	- retrieve next tuple in scan
 *
 *		Fix to work with index relations.
 *		We don't return the buffer anymore, but you can get it from the
 *		returned HeapTuple.
 * ----------------
 */

#ifdef HEAPDEBUGALL
#define HEAPDEBUG_1 \
elog(DEBUG, "heap_getnext([%s,nkeys=%d],backw=%d) called", \
	 RelationGetRelationName(scan->rs_rd), scan->rs_nkeys, backw)

#define HEAPDEBUG_2 \
	 elog(DEBUG, "heap_getnext called with backw (no tracing yet)")

#define HEAPDEBUG_3 \
	 elog(DEBUG, "heap_getnext returns NULL at end")

#define HEAPDEBUG_4 \
	 elog(DEBUG, "heap_getnext valid buffer UNPIN'd")

#define HEAPDEBUG_5 \
	 elog(DEBUG, "heap_getnext next tuple was cached")

#define HEAPDEBUG_6 \
	 elog(DEBUG, "heap_getnext returning EOS")

#define HEAPDEBUG_7 \
	 elog(DEBUG, "heap_getnext returning tuple");
#else
#define HEAPDEBUG_1
#define HEAPDEBUG_2
#define HEAPDEBUG_3
#define HEAPDEBUG_4
#define HEAPDEBUG_5
#define HEAPDEBUG_6
#define HEAPDEBUG_7
#endif	 /* !defined(HEAPDEBUGALL) */


HeapTuple
heap_getnext(HeapScanDesc scandesc, int backw)
{
	HeapScanDesc scan = scandesc;

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_getnext);
	IncrHeapAccessStat(global_getnext);

	/* Note: no locking manipulations needed */

	/* ----------------
	 *	argument checks
	 * ----------------
	 */
	if (scan == NULL)
		elog(ERROR, "heap_getnext: NULL relscan");

	/* ----------------
	 *	initialize return buffer to InvalidBuffer
	 * ----------------
	 */

	HEAPDEBUG_1;				/* heap_getnext( info ) */

	if (backw)
	{
		/* ----------------
		 *	handle reverse scan
		 * ----------------
		 */
		HEAPDEBUG_2;			/* heap_getnext called with backw */

		if (scan->rs_ptup.t_data == scan->rs_ctup.t_data &&
			BufferIsInvalid(scan->rs_pbuf))
			return NULL;

		/*
		 * Copy the "current" tuple/buffer to "next". Pin/unpin the
		 * buffers accordingly
		 */
		if (scan->rs_nbuf != scan->rs_cbuf)
		{
			if (BufferIsValid(scan->rs_nbuf))
				ReleaseBuffer(scan->rs_nbuf);
			if (BufferIsValid(scan->rs_cbuf))
				IncrBufferRefCount(scan->rs_cbuf);
		}
		scan->rs_ntup = scan->rs_ctup;
		scan->rs_nbuf = scan->rs_cbuf;

		if (scan->rs_ptup.t_data != NULL)
		{
			if (scan->rs_cbuf != scan->rs_pbuf)
			{
				if (BufferIsValid(scan->rs_cbuf))
					ReleaseBuffer(scan->rs_cbuf);
				if (BufferIsValid(scan->rs_pbuf))
					IncrBufferRefCount(scan->rs_pbuf);
			}
			scan->rs_ctup = scan->rs_ptup;
			scan->rs_cbuf = scan->rs_pbuf;
		}
		else
		{						/* NONTUP */

			/*
			 * Don't release scan->rs_cbuf at this point, because
			 * heapgettup doesn't increase PrivateRefCount if it is
			 * already set. On a backward scan, both rs_ctup and rs_ntup
			 * usually point to the same buffer page, so
			 * PrivateRefCount[rs_cbuf] should be 2 (or more, if for
			 * instance ctup is stored in a TupleTableSlot).  - 01/09/94
			 */

			heapgettup(scan->rs_rd,
					   &(scan->rs_ctup),
					   -1,
					   &(scan->rs_cbuf),
					   scan->rs_snapshot,
					   scan->rs_nkeys,
					   scan->rs_key);
		}

		if (scan->rs_ctup.t_data == NULL && !BufferIsValid(scan->rs_cbuf))
		{
			if (BufferIsValid(scan->rs_pbuf))
				ReleaseBuffer(scan->rs_pbuf);
			scan->rs_ptup.t_datamcxt = NULL;
			scan->rs_ptup.t_data = NULL;
			scan->rs_pbuf = InvalidBuffer;
			return NULL;
		}

		if (BufferIsValid(scan->rs_pbuf))
			ReleaseBuffer(scan->rs_pbuf);
		scan->rs_ptup.t_datamcxt = NULL;
		scan->rs_ptup.t_data = NULL;
		scan->rs_pbuf = UnknownBuffer;

	}
	else
	{
		/* ----------------
		 *	handle forward scan
		 * ----------------
		 */
		if (scan->rs_ctup.t_data == scan->rs_ntup.t_data &&
			BufferIsInvalid(scan->rs_nbuf))
		{
			HEAPDEBUG_3;		/* heap_getnext returns NULL at end */
			return NULL;
		}

		/*
		 * Copy the "current" tuple/buffer to "previous". Pin/unpin the
		 * buffers accordingly
		 */
		if (scan->rs_pbuf != scan->rs_cbuf)
		{
			if (BufferIsValid(scan->rs_pbuf))
				ReleaseBuffer(scan->rs_pbuf);
			if (BufferIsValid(scan->rs_cbuf))
				IncrBufferRefCount(scan->rs_cbuf);
		}
		scan->rs_ptup = scan->rs_ctup;
		scan->rs_pbuf = scan->rs_cbuf;

		if (scan->rs_ntup.t_data != NULL)
		{
			if (scan->rs_cbuf != scan->rs_nbuf)
			{
				if (BufferIsValid(scan->rs_cbuf))
					ReleaseBuffer(scan->rs_cbuf);
				if (BufferIsValid(scan->rs_nbuf))
					IncrBufferRefCount(scan->rs_nbuf);
			}
			scan->rs_ctup = scan->rs_ntup;
			scan->rs_cbuf = scan->rs_nbuf;
			HEAPDEBUG_5;		/* heap_getnext next tuple was cached */
		}
		else
		{						/* NONTUP */

			/*
			 * Don't release scan->rs_cbuf at this point, because
			 * heapgettup doesn't increase PrivateRefCount if it is
			 * already set. On a forward scan, both rs_ctup and rs_ptup
			 * usually point to the same buffer page, so
			 * PrivateRefCount[rs_cbuf] should be 2 (or more, if for
			 * instance ctup is stored in a TupleTableSlot).  - 01/09/93
			 */

			heapgettup(scan->rs_rd,
					   &(scan->rs_ctup),
					   1,
					   &scan->rs_cbuf,
					   scan->rs_snapshot,
					   scan->rs_nkeys,
					   scan->rs_key);
		}

		if (scan->rs_ctup.t_data == NULL && !BufferIsValid(scan->rs_cbuf))
		{
			if (BufferIsValid(scan->rs_nbuf))
				ReleaseBuffer(scan->rs_nbuf);
			scan->rs_ntup.t_datamcxt = NULL;
			scan->rs_ntup.t_data = NULL;
			scan->rs_nbuf = InvalidBuffer;
			HEAPDEBUG_6;		/* heap_getnext returning EOS */
			return NULL;
		}

		if (BufferIsValid(scan->rs_nbuf))
			ReleaseBuffer(scan->rs_nbuf);
		scan->rs_ntup.t_datamcxt = NULL;
		scan->rs_ntup.t_data = NULL;
		scan->rs_nbuf = UnknownBuffer;
	}

	/* ----------------
	 *	if we get here it means we have a new current scan tuple, so
	 *	point to the proper return buffer and return the tuple.
	 * ----------------
	 */

	HEAPDEBUG_7;				/* heap_getnext returning tuple */

	return ((scan->rs_ctup.t_data == NULL) ? NULL : &(scan->rs_ctup));
}

/* ----------------
 *		heap_fetch		- retrive tuple with tid
 *
 *		Currently ignores LP_IVALID during processing!
 *
 *		Because this is not part of a scan, there is no way to
 *		automatically lock/unlock the shared buffers.
 *		For this reason, we require that the user retrieve the buffer
 *		value, and they are required to BufferRelease() it when they
 *		are done.  If they want to make a copy of it before releasing it,
 *		they can call heap_copytyple().
 * ----------------
 */
void
heap_fetch(Relation relation,
		   Snapshot snapshot,
		   HeapTuple tuple,
		   Buffer *userbuf)
{
	ItemId		lp;
	Buffer		buffer;
	PageHeader	dp;
	ItemPointer tid = &(tuple->t_self);
	OffsetNumber offnum;

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_fetch);
	IncrHeapAccessStat(global_fetch);

	/* ----------------
	 *	get the buffer from the relation descriptor
	 *	Note that this does a buffer pin.
	 * ----------------
	 */

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));

	if (!BufferIsValid(buffer))
		elog(ERROR, "heap_fetch: %s relation: ReadBuffer(%lx) failed",
			 RelationGetRelationName(relation), (long) tid);

	LockBuffer(buffer, BUFFER_LOCK_SHARE);

	/* ----------------
	 *	get the item line pointer corresponding to the requested tid
	 * ----------------
	 */
	dp = (PageHeader) BufferGetPage(buffer);
	offnum = ItemPointerGetOffsetNumber(tid);
	lp = PageGetItemId(dp, offnum);

	/* ----------------
	 *	more sanity checks
	 * ----------------
	 */

	if (!ItemIdIsUsed(lp))
	{
		ReleaseBuffer(buffer);
		*userbuf = InvalidBuffer;
		tuple->t_datamcxt = NULL;
		tuple->t_data = NULL;
		return;
	}

	tuple->t_datamcxt = NULL;
	tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lp);
	tuple->t_len = ItemIdGetLength(lp);

	/* ----------------
	 *	check time qualification of tid
	 * ----------------
	 */

	HeapTupleSatisfies(tuple, relation, buffer, dp,
					   snapshot, 0, (ScanKey) NULL);

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	if (tuple->t_data == NULL)
	{
		/* Tuple failed time check, so we can release now. */
		ReleaseBuffer(buffer);
		*userbuf = InvalidBuffer;
	}
	else
	{

		/*
		 * All checks passed, so return the tuple as valid. Caller is now
		 * responsible for releasing the buffer.
		 */
		*userbuf = buffer;
	}
}

/* ----------------
 *	heap_get_latest_tid -  get the latest tid of a specified tuple
 *
 * ----------------
 */
ItemPointer
heap_get_latest_tid(Relation relation,
					Snapshot snapshot,
					ItemPointer tid)
{
	ItemId		lp = NULL;
	Buffer		buffer;
	PageHeader	dp;
	OffsetNumber offnum;
	HeapTupleData tp;
	HeapTupleHeader t_data;
	ItemPointerData ctid;
	bool		invalidBlock,
				linkend;

	/* ----------------
	 *	get the buffer from the relation descriptor
	 *	Note that this does a buffer pin.
	 * ----------------
	 */

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));

	if (!BufferIsValid(buffer))
		elog(ERROR, "heap_get_latest_tid: %s relation: ReadBuffer(%lx) failed",
			 RelationGetRelationName(relation), (long) tid);

	LockBuffer(buffer, BUFFER_LOCK_SHARE);

	/* ----------------
	 *	get the item line pointer corresponding to the requested tid
	 * ----------------
	 */
	dp = (PageHeader) BufferGetPage(buffer);
	offnum = ItemPointerGetOffsetNumber(tid);
	invalidBlock = true;
	if (!PageIsNew(dp))
	{
		lp = PageGetItemId(dp, offnum);
		if (ItemIdIsUsed(lp))
			invalidBlock = false;
	}
	if (invalidBlock)
	{
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(buffer);
		return NULL;
	}

	/* ----------------
	 *	more sanity checks
	 * ----------------
	 */

	tp.t_datamcxt = NULL;
	t_data = tp.t_data = (HeapTupleHeader) PageGetItem((Page) dp, lp);
	tp.t_len = ItemIdGetLength(lp);
	tp.t_self = *tid;
	ctid = tp.t_data->t_ctid;

	/* ----------------
	 *	check time qualification of tid
	 * ----------------
	 */

	HeapTupleSatisfies(&tp, relation, buffer, dp,
					   snapshot, 0, (ScanKey) NULL);

	linkend = true;
	if ((t_data->t_infomask & HEAP_XMAX_COMMITTED) &&
		!ItemPointerEquals(tid, &ctid))
		linkend = false;

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
	ReleaseBuffer(buffer);

	if (tp.t_data == NULL)
	{
		if (linkend)
			return NULL;
		return heap_get_latest_tid(relation, snapshot, &ctid);
	}

	return tid;
}

/* ----------------
 *		heap_insert		- insert tuple
 *
 *		The assignment of t_min (and thus the others) should be
 *		removed eventually.
 *
 *		Currently places the tuple onto the last page.	If there is no room,
 *		it is placed on new pages.	(Heap relations)
 *		Note that concurrent inserts during a scan will probably have
 *		unexpected results, though this will be fixed eventually.
 *
 *		Fix to work with indexes.
 * ----------------
 */
Oid
heap_insert(Relation relation, HeapTuple tup)
{
	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_insert);
	IncrHeapAccessStat(global_insert);

	/* ----------------
	 *	If the object id of this tuple has already been assigned, trust
	 *	the caller.  There are a couple of ways this can happen.  At initial
	 *	db creation, the backend program sets oids for tuples.	When we
	 *	define an index, we set the oid.  Finally, in the future, we may
	 *	allow users to set their own object ids in order to support a
	 *	persistent object store (objects need to contain pointers to one
	 *	another).
	 * ----------------
	 */
	if (!OidIsValid(tup->t_data->t_oid))
	{
		tup->t_data->t_oid = newoid();
		LastOidProcessed = tup->t_data->t_oid;
	}
	else
		CheckMaxObjectId(tup->t_data->t_oid);

	TransactionIdStore(GetCurrentTransactionId(), &(tup->t_data->t_xmin));
	tup->t_data->t_cmin = GetCurrentCommandId();
	StoreInvalidTransactionId(&(tup->t_data->t_xmax));
	tup->t_data->t_infomask &= ~(HEAP_XACT_MASK);
	tup->t_data->t_infomask |= HEAP_XMAX_INVALID;

	RelationPutHeapTupleAtEnd(relation, tup);

#ifdef XLOG
	/* XLOG stuff */
	{
		xl_heap_insert	xlrec;
		xlrec.itid.dbId = relation->rd_lockInfo.lockRelId.dbId;
		xlrec.itid.relId = relation->rd_lockInfo.lockRelId.relId;
XXX		xlrec.itid.tid = tp.t_self;
		xlrec.t_natts = tup->t_data->t_natts;
		xlrec.t_oid = tup->t_data->t_oid;
		xlrec.t_hoff = tup->t_data->t_hoff;
		xlrec.mask = tup->t_data->t_infomask;
		
		XLogRecPtr recptr = XLogInsert(RM_HEAP_ID, XLOG_HEAP_INSERT,
			(char*) xlrec, sizeof(xlrec), 
			(char*) tup->t_data + offsetof(HeapTupleHeaderData, tbits), 
			tup->t_len - offsetof(HeapTupleHeaderData, tbits));

		dp->pd_lsn = recptr;
	}
#endif

	if (IsSystemRelationName(RelationGetRelationName(relation)))
		RelationMark4RollbackHeapTuple(relation, tup);

	return tup->t_data->t_oid;
}

/*
 *	heap_delete		- delete a tuple
 */
int
heap_delete(Relation relation, ItemPointer tid, ItemPointer ctid)
{
	ItemId		lp;
	HeapTupleData tp;
	PageHeader	dp;
	Buffer		buffer;
	int			result;

	/* increment access statistics */
	IncrHeapAccessStat(local_delete);
	IncrHeapAccessStat(global_delete);

	Assert(ItemPointerIsValid(tid));

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));

	if (!BufferIsValid(buffer))
		elog(ERROR, "heap_delete: failed ReadBuffer");

	LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

	dp = (PageHeader) BufferGetPage(buffer);
	lp = PageGetItemId(dp, ItemPointerGetOffsetNumber(tid));
	tp.t_datamcxt = NULL;
	tp.t_data = (HeapTupleHeader) PageGetItem((Page) dp, lp);
	tp.t_len = ItemIdGetLength(lp);
	tp.t_self = *tid;

l1:
	result = HeapTupleSatisfiesUpdate(&tp);

	if (result == HeapTupleInvisible)
	{
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(buffer);
		elog(ERROR, "heap_delete: (am)invalid tid");
	}
	else if (result == HeapTupleBeingUpdated)
	{
		TransactionId xwait = tp.t_data->t_xmax;

		/* sleep until concurrent transaction ends */
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		XactLockTableWait(xwait);

		LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
		if (TransactionIdDidAbort(xwait))
			goto l1;

		/*
		 * xwait is committed but if xwait had just marked the tuple for
		 * update then some other xaction could update this tuple before
		 * we got to this point.
		 */
		if (tp.t_data->t_xmax != xwait)
			goto l1;
		if (!(tp.t_data->t_infomask & HEAP_XMAX_COMMITTED))
		{
			tp.t_data->t_infomask |= HEAP_XMAX_COMMITTED;
			SetBufferCommitInfoNeedsSave(buffer);
		}
		/* if tuple was marked for update but not updated... */
		if (tp.t_data->t_infomask & HEAP_MARKED_FOR_UPDATE)
			result = HeapTupleMayBeUpdated;
		else
			result = HeapTupleUpdated;
	}
	if (result != HeapTupleMayBeUpdated)
	{
		Assert(result == HeapTupleSelfUpdated || result == HeapTupleUpdated);
		if (ctid != NULL)
			*ctid = tp.t_data->t_ctid;
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(buffer);
		return result;
	}

#ifdef XLOG
	/* XLOG stuff */
	{
		xl_heap_delete	xlrec;
		xlrec.dtid.dbId = relation->rd_lockInfo.lockRelId.dbId;
		xlrec.dtid.relId = relation->rd_lockInfo.lockRelId.relId;
		xlrec.dtid.tid = tp.t_self;
		XLogRecPtr recptr = XLogInsert(RM_HEAP_ID, XLOG_HEAP_DELETE,
			(char*) xlrec, sizeof(xlrec), NULL, 0);

		dp->pd_lsn = recptr;
	}
#endif

	/* store transaction information of xact deleting the tuple */
	TransactionIdStore(GetCurrentTransactionId(), &(tp.t_data->t_xmax));
	tp.t_data->t_cmax = GetCurrentCommandId();
	tp.t_data->t_infomask &= ~(HEAP_XMAX_COMMITTED |
							 HEAP_XMAX_INVALID | HEAP_MARKED_FOR_UPDATE);

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	/* invalidate caches */
	RelationInvalidateHeapTuple(relation, &tp);

	WriteBuffer(buffer);

	return HeapTupleMayBeUpdated;
}

/*
 *	heap_update - replace a tuple
 */
int
heap_update(Relation relation, ItemPointer otid, HeapTuple newtup,
			ItemPointer ctid)
{
	ItemId		lp;
	HeapTupleData oldtup;
	PageHeader	dp;
	Buffer		buffer;
	int			result;

	/* increment access statistics */
	IncrHeapAccessStat(local_replace);
	IncrHeapAccessStat(global_replace);

	Assert(ItemPointerIsValid(otid));

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(otid));
	if (!BufferIsValid(buffer))
		elog(ERROR, "amreplace: failed ReadBuffer");
	LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

	dp = (PageHeader) BufferGetPage(buffer);
	lp = PageGetItemId(dp, ItemPointerGetOffsetNumber(otid));

	oldtup.t_datamcxt = NULL;
	oldtup.t_data = (HeapTupleHeader) PageGetItem(dp, lp);
	oldtup.t_len = ItemIdGetLength(lp);
	oldtup.t_self = *otid;

l2:
	result = HeapTupleSatisfiesUpdate(&oldtup);

	if (result == HeapTupleInvisible)
	{
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(buffer);
		elog(ERROR, "heap_update: (am)invalid tid");
	}
	else if (result == HeapTupleBeingUpdated)
	{
		TransactionId xwait = oldtup.t_data->t_xmax;

		/* sleep untill concurrent transaction ends */
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		XactLockTableWait(xwait);

		LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
		if (TransactionIdDidAbort(xwait))
			goto l2;

		/*
		 * xwait is committed but if xwait had just marked the tuple for
		 * update then some other xaction could update this tuple before
		 * we got to this point.
		 */
		if (oldtup.t_data->t_xmax != xwait)
			goto l2;
		if (!(oldtup.t_data->t_infomask & HEAP_XMAX_COMMITTED))
		{
			oldtup.t_data->t_infomask |= HEAP_XMAX_COMMITTED;
			SetBufferCommitInfoNeedsSave(buffer);
		}
		/* if tuple was marked for update but not updated... */
		if (oldtup.t_data->t_infomask & HEAP_MARKED_FOR_UPDATE)
			result = HeapTupleMayBeUpdated;
		else
			result = HeapTupleUpdated;
	}
	if (result != HeapTupleMayBeUpdated)
	{
		Assert(result == HeapTupleSelfUpdated || result == HeapTupleUpdated);
		if (ctid != NULL)
			*ctid = oldtup.t_data->t_ctid;
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(buffer);
		return result;
	}

	/* XXX order problems if not atomic assignment ??? */
	newtup->t_data->t_oid = oldtup.t_data->t_oid;
	TransactionIdStore(GetCurrentTransactionId(), &(newtup->t_data->t_xmin));
	newtup->t_data->t_cmin = GetCurrentCommandId();
	StoreInvalidTransactionId(&(newtup->t_data->t_xmax));
	newtup->t_data->t_infomask &= ~(HEAP_XACT_MASK);
	newtup->t_data->t_infomask |= (HEAP_XMAX_INVALID | HEAP_UPDATED);

	/* logically delete old item */
	TransactionIdStore(GetCurrentTransactionId(), &(oldtup.t_data->t_xmax));
	oldtup.t_data->t_cmax = GetCurrentCommandId();
	oldtup.t_data->t_infomask &= ~(HEAP_XMAX_COMMITTED |
							 HEAP_XMAX_INVALID | HEAP_MARKED_FOR_UPDATE);

	/* insert new item */
	if ((unsigned) MAXALIGN(newtup->t_len) <= PageGetFreeSpace((Page) dp))
		RelationPutHeapTuple(relation, buffer, newtup);
	else
	{

		/*
		 * New item won't fit on same page as old item, have to look for a
		 * new place to put it. Note that we have to unlock current buffer
		 * context - not good but RelationPutHeapTupleAtEnd uses extend
		 * lock.
		 */
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		RelationPutHeapTupleAtEnd(relation, newtup);
		LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
	}
	/* mark for rollback caches */
	RelationMark4RollbackHeapTuple(relation, newtup);

	/*
	 * New item in place, now record address of new tuple in t_ctid of old
	 * one.
	 */
	oldtup.t_data->t_ctid = newtup->t_self;

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	/* invalidate caches */
	RelationInvalidateHeapTuple(relation, &oldtup);

	WriteBuffer(buffer);

	return HeapTupleMayBeUpdated;
}

/*
 *	heap_mark4update		- mark a tuple for update
 */
int
heap_mark4update(Relation relation, HeapTuple tuple, Buffer *buffer)
{
	ItemPointer tid = &(tuple->t_self);
	ItemId		lp;
	PageHeader	dp;
	int			result;

	/* increment access statistics */
	IncrHeapAccessStat(local_mark4update);
	IncrHeapAccessStat(global_mark4update);

	*buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));

	if (!BufferIsValid(*buffer))
		elog(ERROR, "heap_mark4update: failed ReadBuffer");

	LockBuffer(*buffer, BUFFER_LOCK_EXCLUSIVE);

	dp = (PageHeader) BufferGetPage(*buffer);
	lp = PageGetItemId(dp, ItemPointerGetOffsetNumber(tid));
	tuple->t_datamcxt = NULL;
	tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lp);
	tuple->t_len = ItemIdGetLength(lp);

l3:
	result = HeapTupleSatisfiesUpdate(tuple);

	if (result == HeapTupleInvisible)
	{
		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
		ReleaseBuffer(*buffer);
		elog(ERROR, "heap_mark4update: (am)invalid tid");
	}
	else if (result == HeapTupleBeingUpdated)
	{
		TransactionId xwait = tuple->t_data->t_xmax;

		/* sleep untill concurrent transaction ends */
		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
		XactLockTableWait(xwait);

		LockBuffer(*buffer, BUFFER_LOCK_EXCLUSIVE);
		if (TransactionIdDidAbort(xwait))
			goto l3;

		/*
		 * xwait is committed but if xwait had just marked the tuple for
		 * update then some other xaction could update this tuple before
		 * we got to this point.
		 */
		if (tuple->t_data->t_xmax != xwait)
			goto l3;
		if (!(tuple->t_data->t_infomask & HEAP_XMAX_COMMITTED))
		{
			tuple->t_data->t_infomask |= HEAP_XMAX_COMMITTED;
			SetBufferCommitInfoNeedsSave(*buffer);
		}
		/* if tuple was marked for update but not updated... */
		if (tuple->t_data->t_infomask & HEAP_MARKED_FOR_UPDATE)
			result = HeapTupleMayBeUpdated;
		else
			result = HeapTupleUpdated;
	}
	if (result != HeapTupleMayBeUpdated)
	{
		Assert(result == HeapTupleSelfUpdated || result == HeapTupleUpdated);
		tuple->t_self = tuple->t_data->t_ctid;
		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
		return result;
	}

	/* store transaction information of xact marking the tuple */
	TransactionIdStore(GetCurrentTransactionId(), &(tuple->t_data->t_xmax));
	tuple->t_data->t_cmax = GetCurrentCommandId();
	tuple->t_data->t_infomask &= ~(HEAP_XMAX_COMMITTED | HEAP_XMAX_INVALID);
	tuple->t_data->t_infomask |= HEAP_MARKED_FOR_UPDATE;

	LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);

	WriteNoReleaseBuffer(*buffer);

	return HeapTupleMayBeUpdated;
}

/* ----------------
 *		heap_markpos	- mark scan position
 *
 *		Note:
 *				Should only one mark be maintained per scan at one time.
 *		Check if this can be done generally--say calls to get the
 *		next/previous tuple and NEVER pass struct scandesc to the
 *		user AM's.  Now, the mark is sent to the executor for safekeeping.
 *		Probably can store this info into a GENERAL scan structure.
 *
 *		May be best to change this call to store the marked position
 *		(up to 2?) in the scan structure itself.
 *		Fix to use the proper caching structure.
 * ----------------
 */
void
heap_markpos(HeapScanDesc scan)
{

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_markpos);
	IncrHeapAccessStat(global_markpos);

	/* Note: no locking manipulations needed */

	if (scan->rs_ptup.t_data == NULL &&
		BufferIsUnknown(scan->rs_pbuf))
	{							/* == NONTUP */
		scan->rs_ptup = scan->rs_ctup;
		heapgettup(scan->rs_rd,
				   &(scan->rs_ptup),
				   -1,
				   &scan->rs_pbuf,
				   scan->rs_snapshot,
				   scan->rs_nkeys,
				   scan->rs_key);

	}
	else if (scan->rs_ntup.t_data == NULL &&
			 BufferIsUnknown(scan->rs_nbuf))
	{							/* == NONTUP */
		scan->rs_ntup = scan->rs_ctup;
		heapgettup(scan->rs_rd,
				   &(scan->rs_ntup),
				   1,
				   &scan->rs_nbuf,
				   scan->rs_snapshot,
				   scan->rs_nkeys,
				   scan->rs_key);
	}

	/* ----------------
	 * Should not unpin the buffer pages.  They may still be in use.
	 * ----------------
	 */
	if (scan->rs_ptup.t_data != NULL)
		scan->rs_mptid = scan->rs_ptup.t_self;
	else
		ItemPointerSetInvalid(&scan->rs_mptid);
	if (scan->rs_ctup.t_data != NULL)
		scan->rs_mctid = scan->rs_ctup.t_self;
	else
		ItemPointerSetInvalid(&scan->rs_mctid);
	if (scan->rs_ntup.t_data != NULL)
		scan->rs_mntid = scan->rs_ntup.t_self;
	else
		ItemPointerSetInvalid(&scan->rs_mntid);
}

/* ----------------
 *		heap_restrpos	- restore position to marked location
 *
 *		Note:  there are bad side effects here.  If we were past the end
 *		of a relation when heapmarkpos is called, then if the relation is
 *		extended via insert, then the next call to heaprestrpos will set
 *		cause the added tuples to be visible when the scan continues.
 *		Problems also arise if the TID's are rearranged!!!
 *
 *		Now pins buffer once for each valid tuple pointer (rs_ptup,
 *		rs_ctup, rs_ntup) referencing it.
 *		 - 01/13/94
 *
 * XXX	might be better to do direct access instead of
 *		using the generality of heapgettup().
 *
 * XXX It is very possible that when a scan is restored, that a tuple
 * XXX which previously qualified may fail for time range purposes, unless
 * XXX some form of locking exists (ie., portals currently can act funny.
 * ----------------
 */
void
heap_restrpos(HeapScanDesc scan)
{
	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_restrpos);
	IncrHeapAccessStat(global_restrpos);

	/* XXX no amrestrpos checking that ammarkpos called */

	/* Note: no locking manipulations needed */

	unpinscan(scan);

	/* force heapgettup to pin buffer for each loaded tuple */
	scan->rs_pbuf = InvalidBuffer;
	scan->rs_cbuf = InvalidBuffer;
	scan->rs_nbuf = InvalidBuffer;

	if (!ItemPointerIsValid(&scan->rs_mptid))
	{
		scan->rs_ptup.t_datamcxt = NULL;
		scan->rs_ptup.t_data = NULL;
	}
	else
	{
		scan->rs_ptup.t_self = scan->rs_mptid;
		scan->rs_ptup.t_datamcxt = NULL;
		scan->rs_ptup.t_data = (HeapTupleHeader) 0x1;	/* for heapgettup */
		heapgettup(scan->rs_rd,
				   &(scan->rs_ptup),
				   0,
				   &(scan->rs_pbuf),
				   false,
				   0,
				   (ScanKey) NULL);
	}

	if (!ItemPointerIsValid(&scan->rs_mctid))
	{
		scan->rs_ctup.t_datamcxt = NULL;
		scan->rs_ctup.t_data = NULL;
	}
	else
	{
		scan->rs_ctup.t_self = scan->rs_mctid;
		scan->rs_ctup.t_datamcxt = NULL;
		scan->rs_ctup.t_data = (HeapTupleHeader) 0x1;	/* for heapgettup */
		heapgettup(scan->rs_rd,
				   &(scan->rs_ctup),
				   0,
				   &(scan->rs_cbuf),
				   false,
				   0,
				   (ScanKey) NULL);
	}

	if (!ItemPointerIsValid(&scan->rs_mntid))
	{
		scan->rs_ntup.t_datamcxt = NULL;
		scan->rs_ntup.t_data = NULL;
	}
	else
	{
		scan->rs_ntup.t_datamcxt = NULL;
		scan->rs_ntup.t_self = scan->rs_mntid;
		scan->rs_ntup.t_data = (HeapTupleHeader) 0x1;	/* for heapgettup */
		heapgettup(scan->rs_rd,
				   &(scan->rs_ntup),
				   0,
				   &scan->rs_nbuf,
				   false,
				   0,
				   (ScanKey) NULL);
	}
}
