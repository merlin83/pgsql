/*-------------------------------------------------------------------------
 *
 * btutils.c
 *	  Utility code for Postgres btree implementation.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/nbtree/nbtutils.c,v 1.39 2000-07-21 19:21:00 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/istrat.h"
#include "access/nbtree.h"
#include "executor/execdebug.h"


/*
 * _bt_mkscankey
 *		Build a scan key that contains comparison data from itup
 *		as well as comparator routines appropriate to the key datatypes.
 *
 *		The result is intended for use with _bt_compare().
 */
ScanKey
_bt_mkscankey(Relation rel, IndexTuple itup)
{
	ScanKey		skey;
	TupleDesc	itupdesc;
	int			natts;
	int			i;
	RegProcedure proc;
	Datum		arg;
	bool		null;
	bits16		flag;

	itupdesc = RelationGetDescr(rel);
	natts = RelationGetNumberOfAttributes(rel);

	skey = (ScanKey) palloc(natts * sizeof(ScanKeyData));

	for (i = 0; i < natts; i++)
	{
		proc = index_getprocid(rel, i + 1, BTORDER_PROC);
		arg = index_getattr(itup, i + 1, itupdesc, &null);
		flag = null ? SK_ISNULL : 0x0;
		ScanKeyEntryInitialize(&skey[i],
							   flag,
							   (AttrNumber) (i + 1),
							   proc,
							   arg);
	}

	return skey;
}

/*
 * _bt_mkscankey_nodata
 *		Build a scan key that contains comparator routines appropriate to
 *		the key datatypes, but no comparison data.
 *
 *		The result cannot be used with _bt_compare().  Currently this
 *		routine is only called by utils/sort/tuplesort.c, which has its
 *		own comparison routine.
 */
ScanKey
_bt_mkscankey_nodata(Relation rel)
{
	ScanKey		skey;
	int			natts;
	int			i;
	RegProcedure proc;

	natts = RelationGetNumberOfAttributes(rel);

	skey = (ScanKey) palloc(natts * sizeof(ScanKeyData));

	for (i = 0; i < natts; i++)
	{
		proc = index_getprocid(rel, i + 1, BTORDER_PROC);
		ScanKeyEntryInitialize(&skey[i],
							   SK_ISNULL,
							   (AttrNumber) (i + 1),
							   proc,
							   (Datum) NULL);
	}

	return skey;
}

/*
 * free a scan key made by either _bt_mkscankey or _bt_mkscankey_nodata.
 */
void
_bt_freeskey(ScanKey skey)
{
	pfree(skey);
}

/*
 * free a retracement stack made by _bt_search.
 */
void
_bt_freestack(BTStack stack)
{
	BTStack		ostack;

	while (stack != (BTStack) NULL)
	{
		ostack = stack;
		stack = stack->bts_parent;
		pfree(ostack);
	}
}

/*
 * Construct a BTItem from a plain IndexTuple.
 *
 * This is now useless code, since a BTItem *is* an index tuple with
 * no extra stuff.  We hang onto it for the moment to preserve the
 * notational distinction, in case we want to add some extra stuff
 * again someday.
 */
BTItem
_bt_formitem(IndexTuple itup)
{
	int			nbytes_btitem;
	BTItem		btitem;
	Size		tuplen;
	extern Oid	newoid();

	/* make a copy of the index tuple with room for extra stuff */
	tuplen = IndexTupleSize(itup);
	nbytes_btitem = tuplen + (sizeof(BTItemData) - sizeof(IndexTupleData));

	btitem = (BTItem) palloc(nbytes_btitem);
	memcpy((char *) &(btitem->bti_itup), (char *) itup, tuplen);

	return btitem;
}

/*
 *	_bt_orderkeys() -- Put keys in a sensible order for conjunctive quals.
 *
 *		The order of the keys in the qual match the ordering imposed by
 *		the index.	This routine only needs to be called if there is
 *		more than one qual clause using this index.
 */
void
_bt_orderkeys(Relation relation, BTScanOpaque so)
{
	ScanKey		xform;
	ScanKeyData *cur;
	StrategyMap map;
	int			nbytes;
	Datum		test;
	int			i,
				j;
	int			init[BTMaxStrategyNumber + 1];
	ScanKey		key;
	uint16		numberOfKeys = so->numberOfKeys;
	uint16		new_numberOfKeys = 0;
	AttrNumber	attno = 1;
	bool		equalStrategyEnd,
				underEqualStrategy;

	if (numberOfKeys < 1)
		return;

	key = so->keyData;

	cur = &key[0];
	if (cur->sk_attno != 1)
		elog(ERROR, "_bt_orderkeys: key(s) for attribute 1 missed");

	if (numberOfKeys == 1)
	{

		/*
		 * We don't use indices for 'A is null' and 'A is not null'
		 * currently and 'A < = > <> NULL' is non-sense' - so qual is not
		 * Ok.		- vadim 03/21/97
		 */
		if (cur->sk_flags & SK_ISNULL)
			so->qual_ok = 0;
		so->numberOfFirstKeys = 1;
		return;
	}

	/* get space for the modified array of keys */
	nbytes = BTMaxStrategyNumber * sizeof(ScanKeyData);
	xform = (ScanKey) palloc(nbytes);

	MemSet(xform, 0, nbytes);
	map = IndexStrategyGetStrategyMap(RelationGetIndexStrategy(relation),
									  BTMaxStrategyNumber,
									  attno);
	for (j = 0; j <= BTMaxStrategyNumber; j++)
		init[j] = 0;

	equalStrategyEnd = false;
	underEqualStrategy = true;
	/* check each key passed in */
	for (i = 0;;)
	{
		if (i < numberOfKeys)
			cur = &key[i];

		if (cur->sk_flags & SK_ISNULL)	/* see comments above */
			so->qual_ok = 0;

		if (i == numberOfKeys || cur->sk_attno != attno)
		{
			if (cur->sk_attno != attno + 1 && i < numberOfKeys)
				elog(ERROR, "_bt_orderkeys: key(s) for attribute %d missed",
					 attno + 1);

			underEqualStrategy = (!equalStrategyEnd);

			/*
			 * If = has been specified, no other key will be used. In case
			 * of key < 2 && key == 1 and so on we have to set qual_ok to
			 * 0
			 */
			if (init[BTEqualStrategyNumber - 1])
			{
				ScanKeyData *eq,
						   *chk;

				eq = &xform[BTEqualStrategyNumber - 1];
				for (j = BTMaxStrategyNumber; --j >= 0;)
				{
					if (j == (BTEqualStrategyNumber - 1) || init[j] == 0)
						continue;
					chk = &xform[j];
					test = OidFunctionCall2(chk->sk_procedure,
											eq->sk_argument, chk->sk_argument);
					if (!DatumGetBool(test))
						so->qual_ok = 0;
				}
				init[BTLessStrategyNumber - 1] = 0;
				init[BTLessEqualStrategyNumber - 1] = 0;
				init[BTGreaterEqualStrategyNumber - 1] = 0;
				init[BTGreaterStrategyNumber - 1] = 0;
			}
			else
				equalStrategyEnd = true;

			/* only one of <, <= */
			if (init[BTLessStrategyNumber - 1]
				&& init[BTLessEqualStrategyNumber - 1])
			{
				ScanKeyData *lt,
						   *le;

				lt = &xform[BTLessStrategyNumber - 1];
				le = &xform[BTLessEqualStrategyNumber - 1];

				/*
				 * DO NOT use the cached function stuff here -- this is
				 * key ordering, happens only when the user expresses a
				 * hokey qualification, and gets executed only once,
				 * anyway.	The transform maps are hard-coded, and can't
				 * be initialized in the correct way.
				 */
				test = OidFunctionCall2(le->sk_procedure,
										lt->sk_argument, le->sk_argument);
				if (DatumGetBool(test))
					init[BTLessEqualStrategyNumber - 1] = 0;
				else
					init[BTLessStrategyNumber - 1] = 0;
			}

			/* only one of >, >= */
			if (init[BTGreaterStrategyNumber - 1]
				&& init[BTGreaterEqualStrategyNumber - 1])
			{
				ScanKeyData *gt,
						   *ge;

				gt = &xform[BTGreaterStrategyNumber - 1];
				ge = &xform[BTGreaterEqualStrategyNumber - 1];

				/* see note above on function cache */
				test = OidFunctionCall2(ge->sk_procedure,
										gt->sk_argument, ge->sk_argument);
				if (DatumGetBool(test))
					init[BTGreaterEqualStrategyNumber - 1] = 0;
				else
					init[BTGreaterStrategyNumber - 1] = 0;
			}

			/* okay, reorder and count */
			for (j = BTMaxStrategyNumber; --j >= 0;)
				if (init[j])
					key[new_numberOfKeys++] = xform[j];

			if (underEqualStrategy)
				so->numberOfFirstKeys = new_numberOfKeys;

			if (i == numberOfKeys)
				break;

			/* initialization for new attno */
			attno = cur->sk_attno;
			MemSet(xform, 0, nbytes);
			map = IndexStrategyGetStrategyMap(RelationGetIndexStrategy(relation),
											  BTMaxStrategyNumber,
											  attno);
			/* haven't looked at any strategies yet */
			for (j = 0; j <= BTMaxStrategyNumber; j++)
				init[j] = 0;
		}

		for (j = BTMaxStrategyNumber; --j >= 0;)
		{
			if (cur->sk_procedure == map->entry[j].sk_procedure)
				break;
		}

		/* have we seen one of these before? */
		if (init[j])
		{
			/* yup, use the appropriate value */
			test = FunctionCall2(&cur->sk_func,
								 cur->sk_argument, xform[j].sk_argument);
			if (DatumGetBool(test))
				xform[j].sk_argument = cur->sk_argument;
			else if (j == (BTEqualStrategyNumber - 1))
				so->qual_ok = 0;/* key == a && key == b, but a != b */
		}
		else
		{
			/* nope, use this value */
			memmove(&xform[j], cur, sizeof(*cur));
			init[j] = 1;
		}

		i++;
	}

	so->numberOfKeys = new_numberOfKeys;

	pfree(xform);
}

/*
 * Test whether an indextuple satisfies all the scankey conditions
 *
 * If not ("false" return), the number of conditions satisfied is
 * returned in *keysok.  Given proper ordering of the scankey conditions,
 * we can use this to determine whether it's worth continuing the scan.
 * See _bt_orderkeys().
 *
 * HACK: *keysok == (Size) -1 means we stopped evaluating because we found
 * a NULL value in the index tuple.  It's not quite clear to me why this
 * case has to be treated specially --- tgl 7/00.
 */
bool
_bt_checkkeys(IndexScanDesc scan, IndexTuple tuple, Size *keysok)
{
	BTScanOpaque so = (BTScanOpaque) scan->opaque;
	Size		keysz = so->numberOfKeys;
	TupleDesc	tupdesc;
	ScanKey		key;
	Datum		datum;
	bool		isNull;
	Datum		test;

	*keysok = 0;
	if (keysz == 0)
		return true;

	key = so->keyData;
	tupdesc = RelationGetDescr(scan->relation);

	IncrIndexProcessed();

	while (keysz > 0)
	{
		datum = index_getattr(tuple,
							  key[0].sk_attno,
							  tupdesc,
							  &isNull);

		/* btree doesn't support 'A is null' clauses, yet */
		if (key[0].sk_flags & SK_ISNULL)
			return false;
		if (isNull)
		{
			if (*keysok < so->numberOfFirstKeys)
				*keysok = -1;
			return false;
		}

		if (key[0].sk_flags & SK_COMMUTE)
		{
			test = FunctionCall2(&key[0].sk_func,
								 key[0].sk_argument, datum);
		}
		else
		{
			test = FunctionCall2(&key[0].sk_func,
								 datum, key[0].sk_argument);
		}

		if (DatumGetBool(test) == !!(key[0].sk_flags & SK_NEGATE))
			return false;

		(*keysok)++;
		key++;
		keysz--;
	}

	return true;
}
