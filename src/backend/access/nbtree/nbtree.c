/*-------------------------------------------------------------------------
 *
 * nbtree.c
 *	  Implementation of Lehman and Yao's btree management algorithm for
 *	  Postgres.
 *
 * NOTES
 *	  This file contains only the public interface routines.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/nbtree/nbtree.c,v 1.51 2000-01-19 23:54:48 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/nbtree.h"
#include "catalog/index.h"
#include "executor/executor.h"
#include "miscadmin.h"

#ifdef BTREE_BUILD_STATS
#define ShowExecutorStats pg_options[TRACE_EXECUTORSTATS]
#endif


bool		BuildingBtree = false;		/* see comment in btbuild() */
bool		FastBuild = true;	/* use sort/build instead of insertion
								 * build */

static void _bt_restscan(IndexScanDesc scan);

/*
 *	btbuild() -- build a new btree index.
 *
 *		We use a global variable to record the fact that we're creating
 *		a new index.  This is used to avoid high-concurrency locking,
 *		since the index won't be visible until this transaction commits
 *		and since building is guaranteed to be single-threaded.
 */
void
btbuild(Relation heap,
		Relation index,
		int natts,
		AttrNumber *attnum,
		IndexStrategy istrat,
		uint16 pcount,
		Datum *params,
		FuncIndexInfo *finfo,
		PredInfo *predInfo)
{
	HeapScanDesc hscan;
	HeapTuple	htup;
	IndexTuple	itup;
	TupleDesc	htupdesc,
				itupdesc;
	Datum	   *attdata;
	bool	   *nulls;
	InsertIndexResult res = 0;
	int			nhtups,
				nitups;
	int			i;
	BTItem		btitem;

#ifndef OMIT_PARTIAL_INDEX
	ExprContext *econtext = (ExprContext *) NULL;
	TupleTable	tupleTable = (TupleTable) NULL;
	TupleTableSlot *slot = (TupleTableSlot *) NULL;

#endif
	Node	   *pred,
			   *oldPred;
	BTSpool	   *spool = NULL;
	bool		isunique;
	bool		usefast;

	/* note that this is a new btree */
	BuildingBtree = true;

	pred = predInfo->pred;
	oldPred = predInfo->oldPred;

	/*
	 * bootstrap processing does something strange, so don't use
	 * sort/build for initial catalog indices.	at some point i need to
	 * look harder at this.  (there is some kind of incremental processing
	 * going on there.) -- pma 08/29/95
	 */
	usefast = (FastBuild && IsNormalProcessingMode());

#ifdef BTREE_BUILD_STATS
	if (ShowExecutorStats)
		ResetUsage();
#endif

	/* see if index is unique */
	isunique = IndexIsUniqueNoCache(RelationGetRelid(index));

	/* initialize the btree index metadata page (if this is a new index) */
	if (oldPred == NULL)
		_bt_metapinit(index);

	/* get tuple descriptors for heap and index relations */
	htupdesc = RelationGetDescr(heap);
	itupdesc = RelationGetDescr(index);

	/* get space for data items that'll appear in the index tuple */
	attdata = (Datum *) palloc(natts * sizeof(Datum));
	nulls = (bool *) palloc(natts * sizeof(bool));

	/*
	 * If this is a predicate (partial) index, we will need to evaluate
	 * the predicate using ExecQual, which requires the current tuple to
	 * be in a slot of a TupleTable.  In addition, ExecQual must have an
	 * ExprContext referring to that slot.	Here, we initialize dummy
	 * TupleTable and ExprContext objects for this purpose. --Nels, Feb
	 * '92
	 */
#ifndef OMIT_PARTIAL_INDEX
	if (pred != NULL || oldPred != NULL)
	{
		tupleTable = ExecCreateTupleTable(1);
		slot = ExecAllocTableSlot(tupleTable);
		econtext = makeNode(ExprContext);
		FillDummyExprContext(econtext, slot, htupdesc, InvalidBuffer);

		/*
		 * we never want to use sort/build if we are extending an existing
		 * partial index -- it works by inserting the newly-qualifying
		 * tuples into the existing index. (sort/build would overwrite the
		 * existing index with one consisting of the newly-qualifying
		 * tuples.)
		 */
		usefast = false;
	}
#endif	 /* OMIT_PARTIAL_INDEX */

	/* start a heap scan */
	/* build the index */
	nhtups = nitups = 0;

	if (usefast)
	{
		spool = _bt_spoolinit(index, isunique);
		res = (InsertIndexResult) NULL;
	}

	hscan = heap_beginscan(heap, 0, SnapshotNow, 0, (ScanKey) NULL);

	while (HeapTupleIsValid(htup = heap_getnext(hscan, 0)))
	{
		nhtups++;

		/*
		 * If oldPred != NULL, this is an EXTEND INDEX command, so skip
		 * this tuple if it was already in the existing partial index
		 */
		if (oldPred != NULL)
		{
#ifndef OMIT_PARTIAL_INDEX

			/* SetSlotContents(slot, htup); */
			slot->val = htup;
			if (ExecQual((List *) oldPred, econtext, false))
			{
				nitups++;
				continue;
			}
#endif	 /* OMIT_PARTIAL_INDEX */
		}

		/*
		 * Skip this tuple if it doesn't satisfy the partial-index
		 * predicate
		 */
		if (pred != NULL)
		{
#ifndef OMIT_PARTIAL_INDEX
			/* SetSlotContents(slot, htup); */
			slot->val = htup;
			if (! ExecQual((List *) pred, econtext, false))
				continue;
#endif	 /* OMIT_PARTIAL_INDEX */
		}

		nitups++;

		/*
		 * For the current heap tuple, extract all the attributes we use
		 * in this index, and note which are null.
		 */

		for (i = 1; i <= natts; i++)
		{
			int			attoff;
			bool		attnull;

			/*
			 * Offsets are from the start of the tuple, and are
			 * zero-based; indices are one-based.  The next call returns i
			 * - 1.  That's data hiding for you.
			 */

			attoff = AttrNumberGetAttrOffset(i);
			attdata[attoff] = GetIndexValue(htup,
											htupdesc,
											attoff,
											attnum,
											finfo,
											&attnull);
			nulls[attoff] = (attnull ? 'n' : ' ');
		}

		/* form an index tuple and point it at the heap tuple */
		itup = index_formtuple(itupdesc, attdata, nulls);

		/*
		 * If the single index key is null, we don't insert it into the
		 * index.  Btrees support scans on <, <=, =, >=, and >. Relational
		 * algebra says that A op B (where op is one of the operators
		 * above) returns null if either A or B is null.  This means that
		 * no qualification used in an index scan could ever return true
		 * on a null attribute.  It also means that indices can't be used
		 * by ISNULL or NOTNULL scans, but that's an artifact of the
		 * strategy map architecture chosen in 1986, not of the way nulls
		 * are handled here.
		 */

		/*
		 * New comments: NULLs handling. While we can't do NULL
		 * comparison, we can follow simple rule for ordering items on
		 * btree pages - NULLs greater NOT_NULLs and NULL = NULL is TRUE.
		 * Sure, it's just rule for placing/finding items and no more -
		 * keytest'll return FALSE for a = 5 for items having 'a' isNULL.
		 * Look at _bt_skeycmp, _bt_compare and _bt_itemcmp for how it
		 * works.				 - vadim 03/23/97
		 *
		 * if (itup->t_info & INDEX_NULL_MASK) { pfree(itup); continue; }
		 */

		itup->t_tid = htup->t_self;
		btitem = _bt_formitem(itup);

		/*
		 * if we are doing bottom-up btree build, we insert the index into
		 * a spool file for subsequent processing.	otherwise, we insert
		 * into the btree.
		 */
		if (usefast)
			_bt_spool(btitem, spool);
		else
			res = _bt_doinsert(index, btitem, isunique, heap);

		pfree(btitem);
		pfree(itup);
		if (res)
			pfree(res);
	}

	/* okay, all heap tuples are indexed */
	heap_endscan(hscan);

	if (pred != NULL || oldPred != NULL)
	{
#ifndef OMIT_PARTIAL_INDEX
		ExecDropTupleTable(tupleTable, true);
		pfree(econtext);
#endif	 /* OMIT_PARTIAL_INDEX */
	}

	/*
	 * if we are doing bottom-up btree build, finish the build by
	 * (1) completing the sort of the spool file, (2) inserting the
	 * sorted tuples into btree pages and (3) building the upper levels.
	 */
	if (usefast)
	{
		_bt_leafbuild(spool);
		_bt_spooldestroy(spool);
	}

#ifdef BTREE_BUILD_STATS
	if (ShowExecutorStats)
	{
		fprintf(stderr, "! BtreeBuild Stats:\n");
		ShowUsage();
		ResetUsage();
	}
#endif

	/*
	 * Since we just counted the tuples in the heap, we update its stats
	 * in pg_class to guarantee that the planner takes advantage of the
	 * index we just created.  But, only update statistics during
	 * normal index definitions, not for indices on system catalogs
	 * created during bootstrap processing.  We must close the relations
	 * before updating statistics to guarantee that the relcache entries
	 * are flushed when we increment the command counter in UpdateStats().
	 * But we do not release any locks on the relations; those will be
	 * held until end of transaction.
	 */
	if (IsNormalProcessingMode())
	{
		Oid		hrelid = RelationGetRelid(heap);
		Oid		irelid = RelationGetRelid(index);

		heap_close(heap, NoLock);
		index_close(index);
		UpdateStats(hrelid, nhtups, true);
		UpdateStats(irelid, nitups, false);
		if (oldPred != NULL)
		{
			if (nitups == nhtups)
				pred = NULL;
			UpdateIndexPredicate(irelid, oldPred, pred);
		}
	}

	pfree(nulls);
	pfree(attdata);

	/* all done */
	BuildingBtree = false;
}

/*
 *	btinsert() -- insert an index tuple into a btree.
 *
 *		Descend the tree recursively, find the appropriate location for our
 *		new tuple, put it there, set its unique OID as appropriate, and
 *		return an InsertIndexResult to the caller.
 */
InsertIndexResult
btinsert(Relation rel, Datum *datum, char *nulls, ItemPointer ht_ctid, Relation heapRel)
{
	BTItem		btitem;
	IndexTuple	itup;
	InsertIndexResult res;

	/* generate an index tuple */
	itup = index_formtuple(RelationGetDescr(rel), datum, nulls);
	itup->t_tid = *ht_ctid;

	/*
	 * See comments in btbuild.
	 *
	 * if (itup->t_info & INDEX_NULL_MASK) return (InsertIndexResult) NULL;
	 */

	btitem = _bt_formitem(itup);

	res = _bt_doinsert(rel, btitem,
					   IndexIsUnique(RelationGetRelid(rel)), heapRel);

	pfree(btitem);
	pfree(itup);

	return res;
}

/*
 *	btgettuple() -- Get the next tuple in the scan.
 */
char *
btgettuple(IndexScanDesc scan, ScanDirection dir)
{
	RetrieveIndexResult res;

	/*
	 * If we've already initialized this scan, we can just advance it in
	 * the appropriate direction.  If we haven't done so yet, we call a
	 * routine to get the first item in the scan.
	 */

	if (ItemPointerIsValid(&(scan->currentItemData)))
	{

		/*
		 * Restore scan position using heap TID returned by previous call
		 * to btgettuple(). _bt_restscan() locks buffer.
		 */
		_bt_restscan(scan);
		res = _bt_next(scan, dir);
	}
	else
		res = _bt_first(scan, dir);

	/*
	 * Save heap TID to use it in _bt_restscan. Unlock buffer before
	 * leaving index !
	 */
	if (res)
	{
		((BTScanOpaque) scan->opaque)->curHeapIptr = res->heap_iptr;
		LockBuffer(((BTScanOpaque) scan->opaque)->btso_curbuf, BUFFER_LOCK_UNLOCK);
	}

	return (char *) res;
}

/*
 *	btbeginscan() -- start a scan on a btree index
 */
char *
btbeginscan(Relation rel, bool fromEnd, uint16 keysz, ScanKey scankey)
{
	IndexScanDesc scan;

	/* get the scan */
	scan = RelationGetIndexScan(rel, fromEnd, keysz, scankey);

	/* register scan in case we change pages it's using */
	_bt_regscan(scan);

	return (char *) scan;
}

/*
 *	btrescan() -- rescan an index relation
 */
void
btrescan(IndexScanDesc scan, bool fromEnd, ScanKey scankey)
{
	ItemPointer iptr;
	BTScanOpaque so;

	so = (BTScanOpaque) scan->opaque;

	/* we don't hold a read lock on the current page in the scan */
	if (ItemPointerIsValid(iptr = &(scan->currentItemData)))
	{
		ReleaseBuffer(so->btso_curbuf);
		so->btso_curbuf = InvalidBuffer;
		ItemPointerSetInvalid(iptr);
	}

	/* and we don't hold a read lock on the last marked item in the scan */
	if (ItemPointerIsValid(iptr = &(scan->currentMarkData)))
	{
		ReleaseBuffer(so->btso_mrkbuf);
		so->btso_mrkbuf = InvalidBuffer;
		ItemPointerSetInvalid(iptr);
	}

	if (so == NULL)				/* if called from btbeginscan */
	{
		so = (BTScanOpaque) palloc(sizeof(BTScanOpaqueData));
		so->btso_curbuf = so->btso_mrkbuf = InvalidBuffer;
		so->keyData = (ScanKey) NULL;
		if (scan->numberOfKeys > 0)
			so->keyData = (ScanKey) palloc(scan->numberOfKeys * sizeof(ScanKeyData));
		scan->opaque = so;
		scan->flags = 0x0;
	}

	/*
	 * Reset the scan keys. Note that keys ordering stuff moved to
	 * _bt_first.	   - vadim 05/05/97
	 */
	so->numberOfKeys = scan->numberOfKeys;
	if (scan->numberOfKeys > 0)
	{
		memmove(scan->keyData,
				scankey,
				scan->numberOfKeys * sizeof(ScanKeyData));
		memmove(so->keyData,
				scankey,
				so->numberOfKeys * sizeof(ScanKeyData));
	}

}

void
btmovescan(IndexScanDesc scan, Datum v)
{
	ItemPointer iptr;
	BTScanOpaque so;

	so = (BTScanOpaque) scan->opaque;

	/* we don't hold a read lock on the current page in the scan */
	if (ItemPointerIsValid(iptr = &(scan->currentItemData)))
	{
		ReleaseBuffer(so->btso_curbuf);
		so->btso_curbuf = InvalidBuffer;
		ItemPointerSetInvalid(iptr);
	}

/*	  scan->keyData[0].sk_argument = v; */
	so->keyData[0].sk_argument = v;
}

/*
 *	btendscan() -- close down a scan
 */
void
btendscan(IndexScanDesc scan)
{
	ItemPointer iptr;
	BTScanOpaque so;

	so = (BTScanOpaque) scan->opaque;

	/* we don't hold any read locks */
	if (ItemPointerIsValid(iptr = &(scan->currentItemData)))
	{
		if (BufferIsValid(so->btso_curbuf))
			ReleaseBuffer(so->btso_curbuf);
		so->btso_curbuf = InvalidBuffer;
		ItemPointerSetInvalid(iptr);
	}

	if (ItemPointerIsValid(iptr = &(scan->currentMarkData)))
	{
		if (BufferIsValid(so->btso_mrkbuf))
			ReleaseBuffer(so->btso_mrkbuf);
		so->btso_mrkbuf = InvalidBuffer;
		ItemPointerSetInvalid(iptr);
	}

	if (so->keyData != (ScanKey) NULL)
		pfree(so->keyData);
	pfree(so);

	_bt_dropscan(scan);
}

/*
 *	btmarkpos() -- save current scan position
 */
void
btmarkpos(IndexScanDesc scan)
{
	ItemPointer iptr;
	BTScanOpaque so;

	so = (BTScanOpaque) scan->opaque;

	/* we don't hold any read locks */
	if (ItemPointerIsValid(iptr = &(scan->currentMarkData)))
	{
		ReleaseBuffer(so->btso_mrkbuf);
		so->btso_mrkbuf = InvalidBuffer;
		ItemPointerSetInvalid(iptr);
	}

	/* bump pin on current buffer */
	if (ItemPointerIsValid(&(scan->currentItemData)))
	{
		so->btso_mrkbuf = ReadBuffer(scan->relation,
								  BufferGetBlockNumber(so->btso_curbuf));
		scan->currentMarkData = scan->currentItemData;
		so->mrkHeapIptr = so->curHeapIptr;
	}
}

/*
 *	btrestrpos() -- restore scan to last saved position
 */
void
btrestrpos(IndexScanDesc scan)
{
	ItemPointer iptr;
	BTScanOpaque so;

	so = (BTScanOpaque) scan->opaque;

	/* we don't hold any read locks */
	if (ItemPointerIsValid(iptr = &(scan->currentItemData)))
	{
		ReleaseBuffer(so->btso_curbuf);
		so->btso_curbuf = InvalidBuffer;
		ItemPointerSetInvalid(iptr);
	}

	/* bump pin on marked buffer */
	if (ItemPointerIsValid(&(scan->currentMarkData)))
	{
		so->btso_curbuf = ReadBuffer(scan->relation,
								  BufferGetBlockNumber(so->btso_mrkbuf));

		scan->currentItemData = scan->currentMarkData;
		so->curHeapIptr = so->mrkHeapIptr;
	}
}

/* stubs */
void
btdelete(Relation rel, ItemPointer tid)
{
	/* adjust any active scans that will be affected by this deletion */
	_bt_adjscans(rel, tid);

	/* delete the data from the page */
	_bt_pagedel(rel, tid);
}

static void
_bt_restscan(IndexScanDesc scan)
{
	Relation	rel = scan->relation;
	BTScanOpaque so = (BTScanOpaque) scan->opaque;
	Buffer		buf = so->btso_curbuf;
	Page		page;
	ItemPointer current = &(scan->currentItemData);
	OffsetNumber offnum = ItemPointerGetOffsetNumber(current),
				maxoff;
	BTPageOpaque opaque;
	ItemPointerData target = so->curHeapIptr;
	BTItem		item;
	BlockNumber blkno;

	LockBuffer(buf, BT_READ);		/* lock buffer first! */
	page = BufferGetPage(buf);
	maxoff = PageGetMaxOffsetNumber(page);
	opaque = (BTPageOpaque) PageGetSpecialPointer(page);

	/*
	 * We use this as flag when first index tuple on page is deleted but
	 * we do not move left (this would slowdown vacuum) - so we set
	 * current->ip_posid before first index tuple on the current page
	 * (_bt_step will move it right)...
	 */
	if (!ItemPointerIsValid(&target))
	{
		ItemPointerSetOffsetNumber(&(scan->currentItemData),
		   OffsetNumberPrev(P_RIGHTMOST(opaque) ? P_HIKEY : P_FIRSTKEY));
		return;
	}

	if (maxoff >= offnum)
	{

		/*
		 * if the item is where we left it or has just moved right on this
		 * page, we're done
		 */
		for (;
			 offnum <= maxoff;
			 offnum = OffsetNumberNext(offnum))
		{
			item = (BTItem) PageGetItem(page, PageGetItemId(page, offnum));
			if (item->bti_itup.t_tid.ip_blkid.bi_hi == \
				target.ip_blkid.bi_hi && \
				item->bti_itup.t_tid.ip_blkid.bi_lo == \
				target.ip_blkid.bi_lo && \
				item->bti_itup.t_tid.ip_posid == target.ip_posid)
			{
				current->ip_posid = offnum;
				return;
			}
		}
	}

	/*
	 * By here, the item we're looking for moved right at least one page
	 */
	for (;;)
	{
		if (P_RIGHTMOST(opaque))
			elog(FATAL, "_bt_restscan: my bits moved right off the end of the world!\
\n\tRecreate index %s.", RelationGetRelationName(rel));

		blkno = opaque->btpo_next;
		_bt_relbuf(rel, buf, BT_READ);
		buf = _bt_getbuf(rel, blkno, BT_READ);
		page = BufferGetPage(buf);
		maxoff = PageGetMaxOffsetNumber(page);
		opaque = (BTPageOpaque) PageGetSpecialPointer(page);

		/* see if it's on this page */
		for (offnum = P_RIGHTMOST(opaque) ? P_HIKEY : P_FIRSTKEY;
			 offnum <= maxoff;
			 offnum = OffsetNumberNext(offnum))
		{
			item = (BTItem) PageGetItem(page, PageGetItemId(page, offnum));
			if (item->bti_itup.t_tid.ip_blkid.bi_hi == \
				target.ip_blkid.bi_hi && \
				item->bti_itup.t_tid.ip_blkid.bi_lo == \
				target.ip_blkid.bi_lo && \
				item->bti_itup.t_tid.ip_posid == target.ip_posid)
			{
				ItemPointerSet(current, blkno, offnum);
				so->btso_curbuf = buf;
				return;
			}
		}
	}
}
