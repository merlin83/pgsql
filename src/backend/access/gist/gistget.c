/*-------------------------------------------------------------------------
 *
 * gistget.c
 *	  fetch tuples from a GiST scan.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/access/gist/gistget.c,v 1.43 2004/12/31 21:59:10 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/gist.h"
#include "executor/execdebug.h"


static OffsetNumber gistfindnext(IndexScanDesc s, Page p, OffsetNumber n,
			 ScanDirection dir);
static bool gistscancache(IndexScanDesc s, ScanDirection dir);
static bool gistfirst(IndexScanDesc s, ScanDirection dir);
static bool gistnext(IndexScanDesc s, ScanDirection dir);
static bool gistindex_keytest(IndexTuple tuple,
				  int scanKeySize, ScanKey key, GISTSTATE *giststate,
				  Relation r, Page p, OffsetNumber offset);


Datum
gistgettuple(PG_FUNCTION_ARGS)
{
	IndexScanDesc s = (IndexScanDesc) PG_GETARG_POINTER(0);
	ScanDirection dir = (ScanDirection) PG_GETARG_INT32(1);
	bool		res;

	/* if we have it cached in the scan desc, just return the value */
	if (gistscancache(s, dir))
		PG_RETURN_BOOL(true);

	/* not cached, so we'll have to do some work */
	if (ItemPointerIsValid(&(s->currentItemData)))
		res = gistnext(s, dir);
	else
		res = gistfirst(s, dir);
	PG_RETURN_BOOL(res);
}

static bool
gistfirst(IndexScanDesc s, ScanDirection dir)
{
	Buffer		b;
	Page		p;
	OffsetNumber n;
	OffsetNumber maxoff;
	GISTPageOpaque po;
	GISTScanOpaque so;
	GISTSTACK  *stk;
	BlockNumber blk;
	IndexTuple	it;

	b = ReadBuffer(s->indexRelation, GISTP_ROOT);
	p = BufferGetPage(b);
	po = (GISTPageOpaque) PageGetSpecialPointer(p);
	so = (GISTScanOpaque) s->opaque;

	for (;;)
	{
		maxoff = PageGetMaxOffsetNumber(p);
		if (ScanDirectionIsBackward(dir))
			n = gistfindnext(s, p, maxoff, dir);
		else
			n = gistfindnext(s, p, FirstOffsetNumber, dir);

		while (n < FirstOffsetNumber || n > maxoff)
		{
			ReleaseBuffer(b);
			if (so->s_stack == NULL)
				return false;

			stk = so->s_stack;
			b = ReadBuffer(s->indexRelation, stk->gs_blk);
			p = BufferGetPage(b);
			po = (GISTPageOpaque) PageGetSpecialPointer(p);
			maxoff = PageGetMaxOffsetNumber(p);

			if (ScanDirectionIsBackward(dir))
				n = OffsetNumberPrev(stk->gs_child);
			else
				n = OffsetNumberNext(stk->gs_child);
			so->s_stack = stk->gs_parent;
			pfree(stk);

			n = gistfindnext(s, p, n, dir);
		}
		if (po->flags & F_LEAF)
		{
			ItemPointerSet(&(s->currentItemData), BufferGetBlockNumber(b), n);

			it = (IndexTuple) PageGetItem(p, PageGetItemId(p, n));

			s->xs_ctup.t_self = it->t_tid;

			ReleaseBuffer(b);
			return true;
		}
		else
		{
			stk = (GISTSTACK *) palloc(sizeof(GISTSTACK));
			stk->gs_child = n;
			stk->gs_blk = BufferGetBlockNumber(b);
			stk->gs_parent = so->s_stack;
			so->s_stack = stk;

			it = (IndexTuple) PageGetItem(p, PageGetItemId(p, n));
			blk = ItemPointerGetBlockNumber(&(it->t_tid));

			ReleaseBuffer(b);
			b = ReadBuffer(s->indexRelation, blk);
			p = BufferGetPage(b);
			po = (GISTPageOpaque) PageGetSpecialPointer(p);
		}
	}
}

static bool
gistnext(IndexScanDesc s, ScanDirection dir)
{
	Buffer		b;
	Page		p;
	OffsetNumber n;
	OffsetNumber maxoff;
	GISTPageOpaque po;
	GISTScanOpaque so;
	GISTSTACK  *stk;
	BlockNumber blk;
	IndexTuple	it;

	blk = ItemPointerGetBlockNumber(&(s->currentItemData));
	n = ItemPointerGetOffsetNumber(&(s->currentItemData));

	if (ScanDirectionIsForward(dir))
		n = OffsetNumberNext(n);
	else
		n = OffsetNumberPrev(n);

	b = ReadBuffer(s->indexRelation, blk);
	p = BufferGetPage(b);
	po = (GISTPageOpaque) PageGetSpecialPointer(p);
	so = (GISTScanOpaque) s->opaque;

	for (;;)
	{
		maxoff = PageGetMaxOffsetNumber(p);
		n = gistfindnext(s, p, n, dir);

		while (n < FirstOffsetNumber || n > maxoff)
		{
			ReleaseBuffer(b);
			if (so->s_stack == NULL)
				return false;

			stk = so->s_stack;
			b = ReadBuffer(s->indexRelation, stk->gs_blk);
			p = BufferGetPage(b);
			maxoff = PageGetMaxOffsetNumber(p);
			po = (GISTPageOpaque) PageGetSpecialPointer(p);

			if (ScanDirectionIsBackward(dir))
				n = OffsetNumberPrev(stk->gs_child);
			else
				n = OffsetNumberNext(stk->gs_child);
			so->s_stack = stk->gs_parent;
			pfree(stk);

			n = gistfindnext(s, p, n, dir);
		}
		if (po->flags & F_LEAF)
		{
			ItemPointerSet(&(s->currentItemData), BufferGetBlockNumber(b), n);

			it = (IndexTuple) PageGetItem(p, PageGetItemId(p, n));

			s->xs_ctup.t_self = it->t_tid;

			ReleaseBuffer(b);
			return true;
		}
		else
		{
			stk = (GISTSTACK *) palloc(sizeof(GISTSTACK));
			stk->gs_child = n;
			stk->gs_blk = BufferGetBlockNumber(b);
			stk->gs_parent = so->s_stack;
			so->s_stack = stk;

			it = (IndexTuple) PageGetItem(p, PageGetItemId(p, n));
			blk = ItemPointerGetBlockNumber(&(it->t_tid));

			ReleaseBuffer(b);
			b = ReadBuffer(s->indexRelation, blk);
			p = BufferGetPage(b);
			po = (GISTPageOpaque) PageGetSpecialPointer(p);

			if (ScanDirectionIsBackward(dir))
				n = PageGetMaxOffsetNumber(p);
			else
				n = FirstOffsetNumber;
		}
	}
}

/* Similar to index_keytest, but decompresses the key in the IndexTuple */
static bool
gistindex_keytest(IndexTuple tuple,
				  int scanKeySize,
				  ScanKey key,
				  GISTSTATE *giststate,
				  Relation r,
				  Page p,
				  OffsetNumber offset)
{
	IncrIndexProcessed();

	while (scanKeySize > 0)
	{
		Datum		datum;
		bool		isNull;
		Datum		test;
		GISTENTRY	de;

		datum = index_getattr(tuple,
							  key->sk_attno,
							  giststate->tupdesc,
							  &isNull);
		/* is the index entry NULL? */
		if (isNull)
		{
			/* XXX eventually should check if SK_ISNULL */
			return false;
		}
		/* is the compared-to datum NULL? */
		if (key->sk_flags & SK_ISNULL)
			return false;

		gistdentryinit(giststate, key->sk_attno - 1, &de,
					   datum, r, p, offset,
					   IndexTupleSize(tuple) - sizeof(IndexTupleData),
					   FALSE, isNull);

		/*
		 * Call the Consistent function to evaluate the test.  The
		 * arguments are the index datum (as a GISTENTRY*), the comparison
		 * datum, and the comparison operator's strategy number and
		 * subtype from pg_amop.
		 *
		 * (Presently there's no need to pass the subtype since it'll always
		 * be zero, but might as well pass it for possible future use.)
		 */
		test = FunctionCall4(&key->sk_func,
							 PointerGetDatum(&de),
							 key->sk_argument,
							 Int32GetDatum(key->sk_strategy),
							 ObjectIdGetDatum(key->sk_subtype));

		/* if index datum had to be decompressed, free it */
		if (de.key != datum && !isAttByVal(giststate, key->sk_attno - 1))
			if (DatumGetPointer(de.key) != NULL)
				pfree(DatumGetPointer(de.key));

		if (!DatumGetBool(test))
			return false;

		scanKeySize--;
		key++;
	}

	return true;
}


static OffsetNumber
gistfindnext(IndexScanDesc s, Page p, OffsetNumber n, ScanDirection dir)
{
	OffsetNumber maxoff;
	IndexTuple	it;
	GISTPageOpaque po;
	GISTScanOpaque so;
	GISTSTATE  *giststate;

	maxoff = PageGetMaxOffsetNumber(p);
	po = (GISTPageOpaque) PageGetSpecialPointer(p);
	so = (GISTScanOpaque) s->opaque;
	giststate = so->giststate;

	/*
	 * If we modified the index during the scan, we may have a pointer to
	 * a ghost tuple, before the scan.	If this is the case, back up one.
	 */

	if (so->s_flags & GS_CURBEFORE)
	{
		so->s_flags &= ~GS_CURBEFORE;
		n = OffsetNumberPrev(n);
	}

	while (n >= FirstOffsetNumber && n <= maxoff)
	{
		it = (IndexTuple) PageGetItem(p, PageGetItemId(p, n));
		if (gistindex_keytest(it,
							  s->numberOfKeys, s->keyData, giststate,
							  s->indexRelation, p, n))
			break;

		if (ScanDirectionIsBackward(dir))
			n = OffsetNumberPrev(n);
		else
			n = OffsetNumberNext(n);
	}

	return n;
}

static bool
gistscancache(IndexScanDesc s, ScanDirection dir)
{
	Buffer		b;
	Page		p;
	OffsetNumber n;
	IndexTuple	it;

	if (!(ScanDirectionIsNoMovement(dir)
		  && ItemPointerIsValid(&(s->currentItemData))))
		return false;

	b = ReadBuffer(s->indexRelation,
				   ItemPointerGetBlockNumber(&(s->currentItemData)));
	p = BufferGetPage(b);
	n = ItemPointerGetOffsetNumber(&(s->currentItemData));
	it = (IndexTuple) PageGetItem(p, PageGetItemId(p, n));
	s->xs_ctup.t_self = it->t_tid;
	ReleaseBuffer(b);

	return true;
}
