/*-------------------------------------------------------------------------
 *
 * selfuncs.c
 *	  Selectivity functions for system catalogs and builtin types
 *
 *	  These routines are registered in the operator catalog in the
 *	  "oprrest" and "oprjoin" attributes.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/selfuncs.c,v 1.43 1999-11-25 00:15:57 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_statistic.h"
#include "catalog/pg_type.h"
#include "parser/parse_func.h"
#include "parser/parse_oper.h"
#include "utils/builtins.h"
#include "utils/int8.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

/* N is not a valid var/constant or relation id */
#define NONVALUE(N)		((N) == 0)

/* are we looking at a functional index selectivity request? */
#define FunctionalSelectivity(nIndKeys,attNum) ((attNum)==InvalidAttrNumber)

/* default selectivity estimate for inequalities such as "A < b" */
#define DEFAULT_INEQ_SEL  (1.0 / 3.0)

static bool convert_to_scale(Datum value, Oid typid,
							 double *scaleval);
static void getattproperties(Oid relid, AttrNumber attnum,
							 Oid *typid,
							 int *typlen,
							 bool *typbyval,
							 int32 *typmod);
static bool getattstatistics(Oid relid, AttrNumber attnum,
							 Oid typid, int32 typmod,
							 double *nullfrac,
							 double *commonfrac,
							 Datum *commonval,
							 Datum *loval,
							 Datum *hival);


/*
 *		eqsel			- Selectivity of "=" for any data types.
 *
 * Note: this routine is also used to estimate selectivity for some
 * operators that are not "=" but have comparable selectivity behavior,
 * such as "~~" (text LIKE).  Even for "=" we must keep in mind that
 * the left and right datatypes may differ, so the type of the given
 * constant "value" may be different from the type of the attribute.
 */
float64
eqsel(Oid opid,
	  Oid relid,
	  AttrNumber attno,
	  Datum value,
	  int32 flag)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	if (NONVALUE(attno) || NONVALUE(relid))
		*result = 0.1;
	else
	{
		Oid			typid;
		int			typlen;
		bool		typbyval;
		int32		typmod;
		double		nullfrac;
		double		commonfrac;
		Datum		commonval;
		double		selec;

		/* get info about the attribute */
		getattproperties(relid, attno,
						 &typid, &typlen, &typbyval, &typmod);

		/* get stats for the attribute, if available */
		if (getattstatistics(relid, attno, typid, typmod,
							 &nullfrac, &commonfrac, &commonval,
							 NULL, NULL))
		{
			if (flag & SEL_CONSTANT)
			{
				/* Is the constant "=" to the column's most common value?
				 * (Although the operator may not really be "=",
				 * we will assume that seeing whether it returns TRUE
				 * for the most common value is useful information.
				 * If you don't like it, maybe you shouldn't be using
				 * eqsel for your operator...)
				 */
				RegProcedure	eqproc = get_opcode(opid);
				bool			mostcommon;

				if (eqproc == (RegProcedure) NULL)
					elog(ERROR, "eqsel: no procedure for operator %u",
						 opid);

				/* be careful to apply operator right way 'round */
				if (flag & SEL_RIGHT)
					mostcommon = (bool)
						DatumGetUInt8(fmgr(eqproc, commonval, value));
				else
					mostcommon = (bool)
						DatumGetUInt8(fmgr(eqproc, value, commonval));

				if (mostcommon)
				{
					/* Constant is "=" to the most common value.  We know
					 * selectivity exactly (or as exactly as VACUUM could
					 * calculate it, anyway).
					 */
					selec = commonfrac;
				}
				else
				{
					/* Comparison is against a constant that is neither the
					 * most common value nor null.  Its selectivity cannot
					 * be more than this:
					 */
					selec = 1.0 - commonfrac - nullfrac;
					if (selec > commonfrac)
						selec = commonfrac;
					/* and in fact it's probably less, so apply a fudge
					 * factor.
					 */
					selec *= 0.5;
				}
			}
			else
			{
				/* Search is for a value that we do not know a priori,
				 * but we will assume it is not NULL.  Selectivity
				 * cannot be more than this:
				 */
				selec = 1.0 - nullfrac;
				if (selec > commonfrac)
					selec = commonfrac;
				/* and in fact it's probably less, so apply a fudge
				 * factor.
				 */
				selec *= 0.5;
			}

			/* result should be in range, but make sure... */
			if (selec < 0.0)
				selec = 0.0;
			else if (selec > 1.0)
				selec = 1.0;

			if (! typbyval)
				pfree(DatumGetPointer(commonval));
		}
		else
		{
			/* No VACUUM ANALYZE stats available, so make a guess using
			 * the disbursion stat (if we have that, which is unlikely
			 * for a normal attribute; but for a system attribute we may
			 * be able to estimate it).
			 */
			selec = get_attdisbursion(relid, attno, 0.01);
		}

		*result = (float64data) selec;
	}
	return result;
}

/*
 *		neqsel			- Selectivity of "!=" for any data types.
 *
 * This routine is also used for some operators that are not "!="
 * but have comparable selectivity behavior.  See above comments
 * for eqsel().
 */
float64
neqsel(Oid opid,
	   Oid relid,
	   AttrNumber attno,
	   Datum value,
	   int32 flag)
{
	float64		result;

	result = eqsel(opid, relid, attno, value, flag);
	*result = 1.0 - *result;
	return result;
}

/*
 *		intltsel		- Selectivity of "<" (also "<=") for integers.
 *
 * Actually, this works and is used for all numeric types, so it should
 * be renamed.  In fact, it is also currently called for all manner of
 * non-numeric types, for which it is NOT very helpful.  That needs
 * to be fixed.
 */
float64
intltsel(Oid opid,
		 Oid relid,
		 AttrNumber attno,
		 Datum value,
		 int32 flag)
{
	float64		result;

	result = (float64) palloc(sizeof(float64data));
	if (! (flag & SEL_CONSTANT) || NONVALUE(attno) || NONVALUE(relid))
		*result = DEFAULT_INEQ_SEL;
	else
	{
		HeapTuple	oprtuple;
		Oid			ltype,
					rtype;
		Oid			typid;
		int			typlen;
		bool		typbyval;
		int32		typmod;
		Datum		hival,
					loval;
		double		val,
					high,
					low,
					numerator,
					denominator;

		/* Get left and right datatypes of the operator so we know
		 * what type the constant is.
		 */
		oprtuple = get_operator_tuple(opid);
		if (! HeapTupleIsValid(oprtuple))
			elog(ERROR, "intltsel: no tuple for operator %u", opid);
		ltype = ((Form_pg_operator) GETSTRUCT(oprtuple))->oprleft;
		rtype = ((Form_pg_operator) GETSTRUCT(oprtuple))->oprright;

		/* Convert the constant to a uniform comparison scale. */
		if (! convert_to_scale(value, 
							   ((flag & SEL_RIGHT) ? rtype : ltype),
							   &val))
		{
			/* Ideally we'd produce an error here, on the grounds that
			 * the given operator shouldn't have intltsel registered as its
			 * selectivity func unless we can deal with its operand types.
			 * But currently, all manner of stuff is invoking intltsel,
			 * so give a default estimate until that can be fixed.
			 */
			*result = DEFAULT_INEQ_SEL;
			return result;
		}

		/* Now get info and stats about the attribute */
		getattproperties(relid, attno,
						 &typid, &typlen, &typbyval, &typmod);

		if (! getattstatistics(relid, attno, typid, typmod,
							   NULL, NULL, NULL,
							   &loval, &hival))
		{
			/* no stats available, so default result */
			*result = DEFAULT_INEQ_SEL;
			return result;
		}

		/* Convert the attribute's loval/hival to common scale. */
		if (! convert_to_scale(loval, typid, &low) ||
			! convert_to_scale(hival, typid, &high))
		{
			/* See above comments... */
			if (! typbyval)
			{
				pfree(DatumGetPointer(hival));
				pfree(DatumGetPointer(loval));
			}

			*result = DEFAULT_INEQ_SEL;
			return result;
		}

		/* release temp storage if needed */
		if (! typbyval)
		{
			pfree(DatumGetPointer(hival));
			pfree(DatumGetPointer(loval));
		}

		if (high <= low)
		{
			/* If we trusted the stats fully, we could return a small or
			 * large selec depending on which side of the single data point
			 * the constant is on.  But it seems better to assume that the
			 * stats are out of date and return a default...
			 */
			*result = DEFAULT_INEQ_SEL;
        }
		else if (val <= low || val >= high)
		{
			/* If given value is outside the statistical range, return a
			 * small or large value; but not 0.0/1.0 since there is a chance
			 * the stats are out of date.
			 */
			if (flag & SEL_RIGHT)
				*result = (val <= low) ? 0.01 : 0.99;
			else
				*result = (val <= low) ? 0.99 : 0.01;
		}
		else
		{
			denominator = high - low;
			if (flag & SEL_RIGHT)
				numerator = val - low;
			else
				numerator = high - val;
			*result = numerator / denominator;
		}
	}
	return result;
}

/*
 *		intgtsel		- Selectivity of ">" (also ">=") for integers.
 *
 * See above comments for intltsel.
 */
float64
intgtsel(Oid opid,
		 Oid relid,
		 AttrNumber attno,
		 Datum value,
		 int32 flag)
{
	float64		result;

	/* Compute selectivity of "<", then invert --- but only if we
	 * were able to produce a non-default estimate.
	 */
	result = intltsel(opid, relid, attno, value, flag);
	if (*result != DEFAULT_INEQ_SEL)
		*result = 1.0 - *result;
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
	float64data num1,
				num2,
				min;

	result = (float64) palloc(sizeof(float64data));
	if (NONVALUE(attno1) || NONVALUE(relid1) ||
		NONVALUE(attno2) || NONVALUE(relid2))
		*result = 0.1;
	else
	{
		num1 = get_attdisbursion(relid1, attno1, 0.01);
		num2 = get_attdisbursion(relid2, attno2, 0.01);
		/*
		 * The join selectivity cannot be more than num2, since each
		 * tuple in table 1 could match no more than num2 fraction of
		 * tuples in table 2 (and that's only if the table-1 tuple
		 * matches the most common value in table 2, so probably it's
		 * less).  By the same reasoning it is not more than num1.
		 * The min is therefore an upper bound.
		 *
		 * XXX can we make a better estimate here?  Using the nullfrac
		 * statistic might be helpful, for example.  Assuming the operator
		 * is strict (does not succeed for null inputs) then the selectivity
		 * couldn't be more than (1-nullfrac1)*(1-nullfrac2), which might
		 * be usefully small if there are many nulls.  How about applying
		 * the operator to the most common values?
		 */
		min = (num1 < num2) ? num1 : num2;
		*result = min;
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
 *		intltjoinsel	- Join selectivity of "<" and "<="
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
	*result = DEFAULT_INEQ_SEL;
	return result;
}

/*
 *		intgtjoinsel	- Join selectivity of ">" and ">="
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
	*result = DEFAULT_INEQ_SEL;
	return result;
}

/*
 * convert_to_scale
 *	  Convert a given value of the indicated type to the comparison
 *	  scale needed by intltsel().  Returns "true" if successful.
 *
 * All numeric datatypes are simply converted to their equivalent
 * "double" values.
 * Future extension: convert string-like types to some suitable scale.
 */
static bool
convert_to_scale(Datum value, Oid typid,
				 double *scaleval)
{
	/* Fast-path conversions for some built-in types */
	switch (typid)
	{
		case BOOLOID:
			*scaleval = (double) DatumGetUInt8(value);
			return true;
		case INT2OID:
			*scaleval = (double) DatumGetInt16(value);
			return true;
		case INT4OID:
			*scaleval = (double) DatumGetInt32(value);
			return true;
		case INT8OID:
			*scaleval = (double) (* i8tod((int64 *) DatumGetPointer(value)));
			return true;
		case FLOAT4OID:
			*scaleval = (double) (* DatumGetFloat32(value));
			return true;
		case FLOAT8OID:
			*scaleval = (double) (* DatumGetFloat64(value));
			return true;
		case NUMERICOID:
			*scaleval = (double) (* numeric_float8((Numeric) DatumGetPointer(value)));
			return true;
		case OIDOID:
		case REGPROCOID:
			/* we can treat OIDs as integers... */
			*scaleval = (double) DatumGetObjectId(value);
			return true;
		case TEXTOID:
			/*
			 * Eventually this should get handled by somehow scaling as a
			 * string value.  For now, we need to call it out to avoid
			 * falling into the default case, because there is a float8(text)
			 * function declared in pg_proc that will do the wrong thing :-(
			 */
			break;
		default:
		{
			/* See whether there is a registered type-conversion function,
			 * namely a procedure named "float8" with the right signature.
			 */
			Oid			oid_array[MAXFARGS];
			HeapTuple	ftup;

			MemSet(oid_array, 0, MAXFARGS * sizeof(Oid));
			oid_array[0] = typid;
			ftup = SearchSysCacheTuple(PROCNAME,
									   PointerGetDatum("float8"),
									   Int32GetDatum(1),
									   PointerGetDatum(oid_array),
									   0);
			if (HeapTupleIsValid(ftup) &&
				((Form_pg_proc) GETSTRUCT(ftup))->prorettype == FLOAT8OID)
			{
				RegProcedure convertproc = (RegProcedure) ftup->t_data->t_oid;
				Datum converted = (Datum) fmgr(convertproc, value);
				*scaleval = (double) (* DatumGetFloat64(converted));
				return true;
			}
			break;
		}
	}
	/* Don't know how to convert */
	return false;
}

/*
 * getattproperties
 *	  Retrieve pg_attribute properties for an attribute,
 *	  including type OID, type len, type byval flag, typmod.
 */
static void
getattproperties(Oid relid, AttrNumber attnum,
				 Oid *typid, int *typlen, bool *typbyval, int32 *typmod)
{
	HeapTuple	atp;
	Form_pg_attribute att_tup;

	atp = SearchSysCacheTuple(ATTNUM,
							  ObjectIdGetDatum(relid),
							  Int16GetDatum(attnum),
							  0, 0);
	if (! HeapTupleIsValid(atp))
		elog(ERROR, "getattproperties: no attribute tuple %u %d",
			 relid, (int) attnum);
	att_tup = (Form_pg_attribute) GETSTRUCT(atp);

	*typid = att_tup->atttypid;
	*typlen = att_tup->attlen;
	*typbyval = att_tup->attbyval;
	*typmod = att_tup->atttypmod;
}

/*
 * getattstatistics
 *	  Retrieve the pg_statistic data for an attribute.
 *	  Returns 'false' if no stats are available.
 *
 * Inputs:
 * 'relid' and 'attnum' are the relation and attribute number.
 * 'typid' and 'typmod' are the type and typmod of the column,
 * which the caller must already have looked up.
 *
 * Outputs:
 * The available stats are nullfrac, commonfrac, commonval, loval, hival.
 * The caller need not retrieve all five --- pass NULL pointers for the
 * unwanted values.
 *
 * commonval, loval, hival are returned as Datums holding the internal
 * representation of the values.  (Note that these should be pfree'd
 * after use if the data type is not by-value.)
 *
 * XXX currently, this does a linear search of pg_statistic because there
 * is no index nor syscache for pg_statistic.  FIX THIS!
 */
static bool
getattstatistics(Oid relid, AttrNumber attnum, Oid typid, int32 typmod,
				 double *nullfrac,
				 double *commonfrac,
				 Datum *commonval,
				 Datum *loval,
				 Datum *hival)
{
	Relation	rel;
	bool		isnull;
	HeapTuple	tuple;
	HeapTuple	typeTuple;
	FmgrInfo	inputproc;

	rel = heap_openr(StatisticRelationName, AccessShareLock);

	tuple = SearchSysCacheTuple(STATRELID,
									ObjectIdGetDatum(relid),
									Int16GetDatum((int16) attnum),
									0, 0); /* staop is currently 0 */
	if (!HeapTupleIsValid(tuple))
	{
		/* no such stats entry */
		heap_close(rel, AccessShareLock);
		return false;
	}

	/* We assume that there will only be one entry in pg_statistic
	 * for the given rel/att.  Someday, VACUUM might store more than one...
	 */
	if (nullfrac)
		*nullfrac = ((Form_pg_statistic) GETSTRUCT(tuple))->stanullfrac;
	if (commonfrac)
		*commonfrac = ((Form_pg_statistic) GETSTRUCT(tuple))->stacommonfrac;

	/* Get the type input proc for the column datatype */
	typeTuple = SearchSysCacheTuple(TYPEOID,
									ObjectIdGetDatum(typid),
									0, 0, 0);
	if (! HeapTupleIsValid(typeTuple))
		elog(ERROR, "getattstatistics: Cache lookup failed for type %u",
			 typid);
	fmgr_info(((Form_pg_type) GETSTRUCT(typeTuple))->typinput, &inputproc);

	/* Values are variable-length fields, so cannot access as struct fields.
	 * Must do it the hard way with heap_getattr.
	 */
	if (commonval)
	{
		text *val = (text *) heap_getattr(tuple,
										  Anum_pg_statistic_stacommonval,
										  RelationGetDescr(rel),
										  &isnull);
		if (isnull)
		{
			elog(DEBUG, "getattstatistics: stacommonval is null");
			*commonval = PointerGetDatum(NULL);
		}
		else
		{
			char *strval = textout(val);
			*commonval = (Datum)
				(*fmgr_faddr(&inputproc)) (strval, typid, typmod);
			pfree(strval);
		}
	}

	if (loval)
	{
		text *val = (text *) heap_getattr(tuple,
										  Anum_pg_statistic_staloval,
										  RelationGetDescr(rel),
										  &isnull);
		if (isnull)
		{
			elog(DEBUG, "getattstatistics: staloval is null");
			*loval = PointerGetDatum(NULL);
		}
		else
		{
			char *strval = textout(val);
			*loval = (Datum)
				(*fmgr_faddr(&inputproc)) (strval, typid, typmod);
			pfree(strval);
		}
	}

	if (hival)
	{
		text *val = (text *) heap_getattr(tuple,
										  Anum_pg_statistic_stahival,
										  RelationGetDescr(rel),
										  &isnull);
		if (isnull)
		{
			elog(DEBUG, "getattstatistics: stahival is null");
			*hival = PointerGetDatum(NULL);
		}
		else
		{
			char *strval = textout(val);
			*hival = (Datum)
				(*fmgr_faddr(&inputproc)) (strval, typid, typmod);
			pfree(strval);
		}
	}

	heap_close(rel, AccessShareLock);
	return true;
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
