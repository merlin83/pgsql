/*-------------------------------------------------------------------------
 *
 * geo-selfuncs.c
 *	  Selectivity routines registered in the operator catalog in the
 *	  "oprrest" and "oprjoin" attributes.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/geo_selfuncs.c,v 1.12 2000-01-26 05:57:14 momjian Exp $
 *
 *		XXX These are totally bogus.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "utils/builtins.h"

float64
areasel(Oid opid,
		Oid relid,
		AttrNumber attno,
		char *value,
		int32 flag)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 4.0;
	return result;
}

float64
areajoinsel(Oid opid,
			Oid relid1,
			AttrNumber attno1,
			Oid relid2,
			AttrNumber attno2)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 4.0;
	return result;
}

/*
 *	Selectivity functions for rtrees.  These are bogus -- unless we know
 *	the actual key distribution in the index, we can't make a good prediction
 *	of the selectivity of these operators.
 *
 *	In general, rtrees need to search multiple subtrees in order to guarantee
 *	that all occurrences of the same key have been found.  Because of this,
 *	the heuristic selectivity functions we return are higher than they would
 *	otherwise be.
 */

/*
 *	left_sel -- How likely is a box to be strictly left of (right of, above,
 *				below) a given box?
 */

#ifdef NOT_USED
float64
leftsel(Oid opid,
		Oid relid,
		AttrNumber attno,
		char *value,
		int32 flag)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 6.0;
	return result;
}

#endif

#ifdef NOT_USED
float64
leftjoinsel(Oid opid,
			Oid relid1,
			AttrNumber attno1,
			Oid relid2,
			AttrNumber attno2)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 6.0;
	return result;
}

#endif

/*
 *	contsel -- How likely is a box to contain (be contained by) a given box?
 */
#ifdef NOT_USED
float64
contsel(Oid opid,
		Oid relid,
		AttrNumber attno,
		char *value,
		int32 flag)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 10.0;
	return result;
}

#endif

#ifdef NOT_USED
float64
contjoinsel(Oid opid,
			Oid relid1,
			AttrNumber attno1,
			Oid relid2,
			AttrNumber attno2)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	*result = 1.0 / 10.0;
	return result;
}

#endif
