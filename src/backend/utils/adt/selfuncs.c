/*-------------------------------------------------------------------------
 *
 * selfuncs.c
 *	  Selectivity functions for system catalogs and builtin types
 *
 *	  These routines are registered in the operator catalog in the
 *	  "oprrest" and "oprjoin" attributes.
 *
 *	  XXX check all the functions--I suspect them to be 1-based.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/selfuncs.c,v 1.35 1999-07-17 20:17:59 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_statistic.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

/* N is not a valid var/constant or relation id */
#define NONVALUE(N)		((N) == -1)

/*
 * generalize the test for functional index selectivity request
 */
#define FunctionalSelectivity(nIndKeys,attNum) (attNum==InvalidAttrNumber)

static float32data getattdisbursion(Oid relid, AttrNumber attnum);
static void gethilokey(Oid relid, AttrNumber attnum, Oid opid,
		   char **high, char **low);


/*
 *		eqsel			- Selectivity of "=" for any data type.
 */
float64
eqsel(Oid opid,
	  Oid relid,
	  AttrNumber attno,
	  char *value,
	  int32 flag)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	if (NONVALUE(attno) || NONVALUE(relid))
		*result = 0.1;
	else
		*result = (float64data) getattdisbursion(relid, (int) attno);
	return result;
}

/*
 *		neqsel			- Selectivity of "!=" for any data type.
 */
float64
neqsel(Oid opid,
	   Oid relid,
	   AttrNumber attno,
	   char *value,
	   int32 flag)
{
	float64		result;

	result = eqsel(opid, relid, attno, value, flag);
	*result = 1.0 - *result;
	return result;
}

/*
 *		intltsel		- Selectivity of "<" for integers.
 *						  Should work for both longs and shorts.
 */
float64
intltsel(Oid opid,
		 Oid relid,
		 AttrNumber attno,
		 int32 value,
		 int32 flag)
{
	float64		result;
	char	   *highchar,
			   *lowchar;
	long		val,
				high,
				low,
				top,
				bottom;

	result = (float64) palloc(sizeof(float64data));
	if (NONVALUE(attno) || NONVALUE(relid))
		*result = 1.0 / 3;
	else
	{
		/* XXX			val = atol(value); */
		val = value;
		gethilokey(relid, (int) attno, opid, &highchar, &lowchar);
		if (*highchar == 'n' || *lowchar == 'n')
		{
			*result = 1.0 / 3.0;
			return result;
		}
		high = atol(highchar);
		low = atol(lowchar);
		if ((flag & SEL_RIGHT && val < low) ||
			(!(flag & SEL_RIGHT) && val > high))
		{
			float32data nvals;

			nvals = getattdisbursion(relid, (int) attno);
			if (nvals == 0)
				*result = 1.0 / 3.0;
			else
			{
				*result = 3.0 * (float64data) nvals;
				if (*result > 1.0)
					*result = 1;
			}
		}
		else
		{
			bottom = high - low;
			if (bottom == 0)
				++bottom;
			if (flag & SEL_RIGHT)
				top = val - low;
			else
				top = high - val;
			if (top > bottom)
				*result = 1.0;
			else
			{
				if (top == 0)
					++top;
				*result = ((1.0 * top) / bottom);
			}
		}
	}
	return result;
}

/*
 *		intgtsel		- Selectivity of ">" for integers.
 *						  Should work for both longs and shorts.
 */
float64
intgtsel(Oid opid,
		 Oid relid,
		 AttrNumber attno,
		 int32 value,
		 int32 flag)
{
	float64		result;
	int			notflag;

	if (flag & 0)
		notflag = flag & ~SEL_RIGHT;
	else
		notflag = flag | SEL_RIGHT;
	result = intltsel(opid, relid, attno, value, (int32) notflag);
	return result;
}

/*
 *		eqjoinsel		- Join selectivity of "="
 */
float64
eqjoinsel(Oid opid,
		  Oid relid1,
		  AttrNumber attno1,
		  Oid relid2,
		  AttrNumber attno2)
{
	float64		result;
	float32data num1,
				num2,
				max;

	result = (float64) palloc(sizeof(float64data));
	if (NONVALUE(attno1) || NONVALUE(relid1) ||
		NONVALUE(attno2) || NONVALUE(relid2))
		*result = 0.1;
	else
	{
		num1 = getattdisbursion(relid1, (int) attno1);
		num2 = getattdisbursion(relid2, (int) attno2);
		max = (num1 > num2) ? num1 : num2;
		if (max == 0)
			*result = 1.0;
		else
			*result = (float64data) max;
	}
	return result;
}

/*
 *		neqjoinsel		- Join selectivity of "!="
 */
float64
neqjoinsel(Oid opid,
		   Oid relid1,
		   AttrNumber attno1,
		   Oid relid2,
		   AttrNumber attno2)
{
	float64		result;

	result = eqjoinsel(opid, relid1, attno1, relid2, attno2);
	*result = 1.0 - *result;
	return result;
}

/*
 *		intltjoinsel	- Join selectivity of "<"
 */
float64
intltjoinsel(Oid opid,
			 Oid relid1,
			 AttrNumber attno1,
			 Oid relid2,
			 AttrNumber attno2)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 3.0;
	return result;
}

/*
 *		intgtjoinsel	- Join selectivity of ">"
 */
float64
intgtjoinsel(Oid opid,
			 Oid relid1,
			 AttrNumber attno1,
			 Oid relid2,
			 AttrNumber attno2)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 3.0;
	return result;
}

/*
 *		getattdisbursion		- Retrieves the number of values within an attribute.
 *
 *		Note:
 *				getattdisbursion and gethilokey both currently use keyed
 *				relation scans and amgetattr.  Alternatively,
 *				the relation scan could be non-keyed and the tuple
 *				returned could be cast (struct X *) tuple + tuple->t_hoff.
 *				The first method is good for testing the implementation,
 *				but the second may ultimately be faster?!?	In any case,
 *				using the cast instead of amgetattr would be
 *				more efficient.  However, the cast will not work
 *				for gethilokey which accesses stahikey in struct statistic.
 */
static float32data
getattdisbursion(Oid relid, AttrNumber attnum)
{
	HeapTuple	atp;
	float32data nvals;
	int32		ntuples;

	atp = SearchSysCacheTuple(ATTNUM,
							  ObjectIdGetDatum(relid),
							  Int16GetDatum(attnum),
							  0, 0);
	if (!HeapTupleIsValid(atp))
	{
		elog(ERROR, "getattdisbursion: no attribute tuple %u %d",
			 relid, attnum);
		return 0;
	}
	nvals = ((Form_pg_attribute) GETSTRUCT(atp))->attdisbursion;
	if (nvals > 0)
		return nvals;

	atp = SearchSysCacheTuple(RELOID,
							  ObjectIdGetDatum(relid),
							  0, 0, 0);

	/*
	 * XXX -- use number of tuples as number of distinctive values just
	 * for now, in case number of distinctive values is not cached
	 */
	if (!HeapTupleIsValid(atp))
	{
		elog(ERROR, "getattdisbursion: no relation tuple %u", relid);
		return 0;
	}
	ntuples = ((Form_pg_class) GETSTRUCT(atp))->reltuples;
	/* Look above how nvals is used.	- vadim 04/09/97 */
	if (ntuples > 0)
		nvals = 1.0 / ntuples;

	return nvals;
}

/*
 *		gethilokey		- Returns a pointer to strings containing
 *						  the high and low keys within an attribute.
 *
 *		Currently returns "0", and "0" in high and low if the statistic
 *		catalog does not contain the proper tuple.	Eventually, the
 *		statistic demon should have the tuple maintained, and it should
 *		elog() if the tuple is missing.
 *
 *		XXX Question: is this worth sticking in the catalog caches,
 *			or will this get invalidated too often?
 */
static void
gethilokey(Oid relid,
		   AttrNumber attnum,
		   Oid opid,
		   char **high,
		   char **low)
{
	Relation	rel;
	HeapScanDesc scan;
	static ScanKeyData key[3] = {
		{0, Anum_pg_statistic_starelid, F_OIDEQ, {0, 0, F_OIDEQ}},
		{0, Anum_pg_statistic_staattnum, F_INT2EQ, {0, 0, F_INT2EQ}},
		{0, Anum_pg_statistic_staop, F_OIDEQ, {0, 0, F_OIDEQ}}
	};
	bool		isnull;
	HeapTuple	tuple;

	rel = heap_openr(StatisticRelationName);

	key[0].sk_argument = ObjectIdGetDatum(relid);
	key[1].sk_argument = Int16GetDatum((int16) attnum);
	key[2].sk_argument = ObjectIdGetDatum(opid);
	scan = heap_beginscan(rel, 0, SnapshotNow, 3, key);
	tuple = heap_getnext(scan, 0);
	if (!HeapTupleIsValid(tuple))
	{
		*high = "n";
		*low = "n";

		/*
		 * XXX			elog(ERROR, "gethilokey: statistic tuple not
		 * found");
		 */
		return;
	}
	*high = textout((struct varlena *)
					heap_getattr(tuple,
								 Anum_pg_statistic_stahikey,
								 RelationGetDescr(rel),
								 &isnull));
	if (isnull)
		elog(DEBUG, "gethilokey: high key is null");
	*low = textout((struct varlena *)
				   heap_getattr(tuple,
								Anum_pg_statistic_stalokey,
								RelationGetDescr(rel),
								&isnull));
	if (isnull)
		elog(DEBUG, "gethilokey: low key is null");
	heap_endscan(scan);
	heap_close(rel);
}

float64
btreesel(Oid operatorObjectId,
		 Oid indrelid,
		 AttrNumber attributeNumber,
		 char *constValue,
		 int32 constFlag,
		 int32 nIndexKeys,
		 Oid indexrelid)
{
	float64		result;

	if (FunctionalSelectivity(nIndexKeys, attributeNumber))
	{

		/*
		 * Need to call the functions selectivity function here.  For now
		 * simply assume it's 1/3 since functions don't currently have
		 * selectivity functions
		 */
		result = (float64) palloc(sizeof(float64data));
		*result = 1.0 / 3.0;
	}
	else
	{
		RegProcedure oprrest = get_oprrest(operatorObjectId);

		/*
		 * Operators used for indexes should have selectivity estimators.
		 * (An alternative is to default to 0.5, as the optimizer does in
		 * dealing with operators occurring in WHERE clauses, but if you
		 * are going to the trouble of making index support you probably
		 * don't want to miss the benefits of a good selectivity estimate.)
		 */
		if (!oprrest)
		{
#if 1
			/*
			 * XXX temporary fix for 6.5: rtree operators are missing their
			 * selectivity estimators, so return a default estimate instead.
			 * Ugh.
			 */
			result = (float64) palloc(sizeof(float64data));
			*result = 0.5;
#else
			elog(ERROR,
				 "Operator %u must have a restriction selectivity estimator to be used in an index",
				 operatorObjectId);
#endif
		}
		else
			result = (float64) fmgr(oprrest,
									(char *) operatorObjectId,
									(char *) indrelid,
									(char *) (int) attributeNumber,
									(char *) constValue,
									(char *) constFlag,
									NULL);
	}

	if (!PointerIsValid(result))
		elog(ERROR, "Btree Selectivity: bad pointer");
	if (*result < 0.0 || *result > 1.0)
		elog(ERROR, "Btree Selectivity: bad value %lf", *result);

	return result;
}

float64
btreenpage(Oid operatorObjectId,
		   Oid indrelid,
		   AttrNumber attributeNumber,
		   char *constValue,
		   int32 constFlag,
		   int32 nIndexKeys,
		   Oid indexrelid)
{
	float64		temp,
				result;
	float64data tempData;
	HeapTuple	atp;
	int			npage;

	if (FunctionalSelectivity(nIndexKeys, attributeNumber))
	{

		/*
		 * Need to call the functions selectivity function here.  For now
		 * simply assume it's 1/3 since functions don't currently have
		 * selectivity functions
		 */
		tempData = 1.0 / 3.0;
		temp = &tempData;
	}
	else
	{
		RegProcedure oprrest = get_oprrest(operatorObjectId);

		/*
		 * Operators used for indexes should have selectivity estimators.
		 * (An alternative is to default to 0.5, as the optimizer does in
		 * dealing with operators occurring in WHERE clauses, but if you
		 * are going to the trouble of making index support you probably
		 * don't want to miss the benefits of a good selectivity estimate.)
		 */
		if (!oprrest)
		{
#if 1
			/*
			 * XXX temporary fix for 6.5: rtree operators are missing their
			 * selectivity estimators, so return a default estimate instead.
			 * Ugh.
			 */
			tempData = 0.5;
			temp = &tempData;
#else
			elog(ERROR,
				 "Operator %u must have a restriction selectivity estimator to be used in an index",
				 operatorObjectId);
#endif
		}
		else
			temp = (float64) fmgr(oprrest,
								  (char *) operatorObjectId,
								  (char *) indrelid,
								  (char *) (int) attributeNumber,
								  (char *) constValue,
								  (char *) constFlag,
								  NULL);
	}

	atp = SearchSysCacheTuple(RELOID,
							  ObjectIdGetDatum(indexrelid),
							  0, 0, 0);
	if (!HeapTupleIsValid(atp))
	{
		elog(ERROR, "btreenpage: no index tuple %u", indexrelid);
		return 0;
	}

	npage = ((Form_pg_class) GETSTRUCT(atp))->relpages;
	result = (float64) palloc(sizeof(float64data));
	*result = *temp * npage;
	return result;
}

float64
hashsel(Oid operatorObjectId,
		Oid indrelid,
		AttrNumber attributeNumber,
		char *constValue,
		int32 constFlag,
		int32 nIndexKeys,
		Oid indexrelid)
{

	float64		result;
	float64data resultData;
	HeapTuple	atp;
	int			ntuples;

	if (FunctionalSelectivity(nIndexKeys, attributeNumber))
	{

		/*
		 * Need to call the functions selectivity function here.  For now
		 * simply use 1/Number of Tuples since functions don't currently
		 * have selectivity functions
		 */

		atp = SearchSysCacheTuple(RELOID,
								  ObjectIdGetDatum(indexrelid),
								  0, 0, 0);
		if (!HeapTupleIsValid(atp))
		{
			elog(ERROR, "hashsel: no index tuple %u", indexrelid);
			return 0;
		}
		ntuples = ((Form_pg_class) GETSTRUCT(atp))->reltuples;
		if (ntuples > 0)
			resultData = 1.0 / (float64data) ntuples;
		else
			resultData = (float64data) (1.0 / 100.0);
		result = &resultData;

	}
	else
	{
		RegProcedure oprrest = get_oprrest(operatorObjectId);

		/*
		 * Operators used for indexes should have selectivity estimators.
		 * (An alternative is to default to 0.5, as the optimizer does in
		 * dealing with operators occurring in WHERE clauses, but if you
		 * are going to the trouble of making index support you probably
		 * don't want to miss the benefits of a good selectivity estimate.)
		 */
		if (!oprrest)
			elog(ERROR,
				 "Operator %u must have a restriction selectivity estimator to be used in a hash index",
				 operatorObjectId);

		result = (float64) fmgr(oprrest,
								(char *) operatorObjectId,
								(char *) indrelid,
								(char *) (int) attributeNumber,
								(char *) constValue,
								(char *) constFlag,
								NULL);
	}

	if (!PointerIsValid(result))
		elog(ERROR, "Hash Table Selectivity: bad pointer");
	if (*result < 0.0 || *result > 1.0)
		elog(ERROR, "Hash Table Selectivity: bad value %lf", *result);

	return result;


}

float64
hashnpage(Oid operatorObjectId,
		  Oid indrelid,
		  AttrNumber attributeNumber,
		  char *constValue,
		  int32 constFlag,
		  int32 nIndexKeys,
		  Oid indexrelid)
{
	float64		temp,
				result;
	float64data tempData;
	HeapTuple	atp;
	int			npage;
	int			ntuples;

	atp = SearchSysCacheTuple(RELOID,
							  ObjectIdGetDatum(indexrelid),
							  0, 0, 0);
	if (!HeapTupleIsValid(atp))
	{
		elog(ERROR, "hashsel: no index tuple %u", indexrelid);
		return 0;
	}


	if (FunctionalSelectivity(nIndexKeys, attributeNumber))
	{

		/*
		 * Need to call the functions selectivity function here.  For now,
		 * use 1/Number of Tuples since functions don't currently have
		 * selectivity functions
		 */

		ntuples = ((Form_pg_class) GETSTRUCT(atp))->reltuples;
		if (ntuples > 0)
			tempData = 1.0 / (float64data) ntuples;
		else
			tempData = (float64data) (1.0 / 100.0);
		temp = &tempData;

	}
	else
	{
		RegProcedure oprrest = get_oprrest(operatorObjectId);

		/*
		 * Operators used for indexes should have selectivity estimators.
		 * (An alternative is to default to 0.5, as the optimizer does in
		 * dealing with operators occurring in WHERE clauses, but if you
		 * are going to the trouble of making index support you probably
		 * don't want to miss the benefits of a good selectivity estimate.)
		 */
		if (!oprrest)
			elog(ERROR,
				 "Operator %u must have a restriction selectivity estimator to be used in a hash index",
				 operatorObjectId);

		temp = (float64) fmgr(oprrest,
							  (char *) operatorObjectId,
							  (char *) indrelid,
							  (char *) (int) attributeNumber,
							  (char *) constValue,
							  (char *) constFlag,
							  NULL);
	}

	npage = ((Form_pg_class) GETSTRUCT(atp))->relpages;
	result = (float64) palloc(sizeof(float64data));
	*result = *temp * npage;
	return result;
}


float64
rtsel(Oid operatorObjectId,
	  Oid indrelid,
	  AttrNumber attributeNumber,
	  char *constValue,
	  int32 constFlag,
	  int32 nIndexKeys,
	  Oid indexrelid)
{
	return (btreesel(operatorObjectId, indrelid, attributeNumber,
					 constValue, constFlag, nIndexKeys, indexrelid));
}

float64
rtnpage(Oid operatorObjectId,
		Oid indrelid,
		AttrNumber attributeNumber,
		char *constValue,
		int32 constFlag,
		int32 nIndexKeys,
		Oid indexrelid)
{
	return (btreenpage(operatorObjectId, indrelid, attributeNumber,
					   constValue, constFlag, nIndexKeys, indexrelid));
}

float64
gistsel(Oid operatorObjectId,
		Oid indrelid,
		AttrNumber attributeNumber,
		char *constValue,
		int32 constFlag,
		int32 nIndexKeys,
		Oid indexrelid)
{
	return (btreesel(operatorObjectId, indrelid, attributeNumber,
					 constValue, constFlag, nIndexKeys, indexrelid));
}

float64
gistnpage(Oid operatorObjectId,
		  Oid indrelid,
		  AttrNumber attributeNumber,
		  char *constValue,
		  int32 constFlag,
		  int32 nIndexKeys,
		  Oid indexrelid)
{
	return (btreenpage(operatorObjectId, indrelid, attributeNumber,
					   constValue, constFlag, nIndexKeys, indexrelid));
}
