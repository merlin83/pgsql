/*-------------------------------------------------------------------------
 *
 * pg_aggregate.c
 *	  routines to support manipulation of the pg_aggregate relation
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/pg_aggregate.c,v 1.43 2002-04-09 20:35:47 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_aggregate.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "parser/parse_coerce.h"
#include "parser/parse_func.h"
#include "parser/parse_type.h"
#include "utils/builtins.h"
#include "utils/syscache.h"

/*
 * AggregateCreate
 */
void
AggregateCreate(const char *aggName,
				Oid aggNamespace,
				List *aggtransfnName,
				List *aggfinalfnName,
				Oid aggBaseType,
				Oid aggTransType,
				const char *agginitval)
{
	Relation	aggdesc;
	HeapTuple	tup;
	char		nulls[Natts_pg_aggregate];
	Datum		values[Natts_pg_aggregate];
	Form_pg_proc proc;
	Oid			transfn;
	Oid			finalfn = InvalidOid;	/* can be omitted */
	Oid			finaltype;
	Oid			fnArgs[FUNC_MAX_ARGS];
	int			nargs;
	NameData	aname;
	TupleDesc	tupDesc;
	int			i;

	/* sanity checks */
	if (!aggName)
		elog(ERROR, "no aggregate name supplied");

	if (!aggtransfnName)
		elog(ERROR, "aggregate must have a transition function");

	/* make sure there is no existing agg of same name and base type */
	if (SearchSysCacheExists(AGGNAME,
							 PointerGetDatum(aggName),
							 ObjectIdGetDatum(aggBaseType),
							 0, 0))
		elog(ERROR,
			 "aggregate function \"%s\" with base type %s already exists",
			 aggName, typeidTypeName(aggBaseType));

	/* handle transfn */
	MemSet(fnArgs, 0, FUNC_MAX_ARGS * sizeof(Oid));
	fnArgs[0] = aggTransType;
	if (OidIsValid(aggBaseType))
	{
		fnArgs[1] = aggBaseType;
		nargs = 2;
	}
	else
		nargs = 1;
	transfn = LookupFuncName(aggtransfnName, nargs, fnArgs);
	if (!OidIsValid(transfn))
		func_error("AggregateCreate", aggtransfnName, nargs, fnArgs, NULL);
	tup = SearchSysCache(PROCOID,
						 ObjectIdGetDatum(transfn),
						 0, 0, 0);
	if (!HeapTupleIsValid(tup))
		func_error("AggregateCreate", aggtransfnName, nargs, fnArgs, NULL);
	proc = (Form_pg_proc) GETSTRUCT(tup);
	if (proc->prorettype != aggTransType)
		elog(ERROR, "return type of transition function %s is not %s",
			 NameListToString(aggtransfnName), typeidTypeName(aggTransType));

	/*
	 * If the transfn is strict and the initval is NULL, make sure input
	 * type and transtype are the same (or at least binary-compatible),
	 * so that it's OK to use the first input value as the initial
	 * transValue.
	 */
	if (proc->proisstrict && agginitval == NULL)
	{
		if (!IsBinaryCompatible(aggBaseType, aggTransType))
			elog(ERROR, "must not omit initval when transfn is strict and transtype is not compatible with input type");
	}
	ReleaseSysCache(tup);

	/* handle finalfn, if supplied */
	if (aggfinalfnName)
	{
		fnArgs[0] = aggTransType;
		fnArgs[1] = 0;
		finalfn = LookupFuncName(aggfinalfnName, 1, fnArgs);
		if (!OidIsValid(finalfn))
			func_error("AggregateCreate", aggfinalfnName, 1, fnArgs, NULL);
		tup = SearchSysCache(PROCOID,
							 ObjectIdGetDatum(finalfn),
							 0, 0, 0);
		if (!HeapTupleIsValid(tup))
			func_error("AggregateCreate", aggfinalfnName, 1, fnArgs, NULL);
		proc = (Form_pg_proc) GETSTRUCT(tup);
		finaltype = proc->prorettype;
		ReleaseSysCache(tup);
	}
	else
	{
		/*
		 * If no finalfn, aggregate result type is type of the state value
		 */
		finaltype = aggTransType;
	}
	Assert(OidIsValid(finaltype));

	/* initialize nulls and values */
	for (i = 0; i < Natts_pg_aggregate; i++)
	{
		nulls[i] = ' ';
		values[i] = (Datum) NULL;
	}
	namestrcpy(&aname, aggName);
	values[Anum_pg_aggregate_aggname - 1] = NameGetDatum(&aname);
	values[Anum_pg_aggregate_aggowner - 1] = Int32GetDatum(GetUserId());
	values[Anum_pg_aggregate_aggtransfn - 1] = ObjectIdGetDatum(transfn);
	values[Anum_pg_aggregate_aggfinalfn - 1] = ObjectIdGetDatum(finalfn);
	values[Anum_pg_aggregate_aggbasetype - 1] = ObjectIdGetDatum(aggBaseType);
	values[Anum_pg_aggregate_aggtranstype - 1] = ObjectIdGetDatum(aggTransType);
	values[Anum_pg_aggregate_aggfinaltype - 1] = ObjectIdGetDatum(finaltype);

	if (agginitval)
		values[Anum_pg_aggregate_agginitval - 1] =
			DirectFunctionCall1(textin, CStringGetDatum(agginitval));
	else
		nulls[Anum_pg_aggregate_agginitval - 1] = 'n';

	aggdesc = heap_openr(AggregateRelationName, RowExclusiveLock);
	tupDesc = aggdesc->rd_att;
	if (!HeapTupleIsValid(tup = heap_formtuple(tupDesc,
											   values,
											   nulls)))
		elog(ERROR, "AggregateCreate: heap_formtuple failed");
	if (!OidIsValid(heap_insert(aggdesc, tup)))
		elog(ERROR, "AggregateCreate: heap_insert failed");

	if (RelationGetForm(aggdesc)->relhasindex)
	{
		Relation	idescs[Num_pg_aggregate_indices];

		CatalogOpenIndices(Num_pg_aggregate_indices, Name_pg_aggregate_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_aggregate_indices, aggdesc, tup);
		CatalogCloseIndices(Num_pg_aggregate_indices, idescs);
	}

	heap_close(aggdesc, RowExclusiveLock);
}

Datum
AggNameGetInitVal(char *aggName, Oid basetype, bool *isNull)
{
	HeapTuple	tup;
	Oid			transtype,
				typinput,
				typelem;
	Datum		textInitVal;
	char	   *strInitVal;
	Datum		initVal;

	Assert(PointerIsValid(aggName));
	Assert(PointerIsValid(isNull));

	tup = SearchSysCache(AGGNAME,
						 PointerGetDatum(aggName),
						 ObjectIdGetDatum(basetype),
						 0, 0);
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "AggNameGetInitVal: cache lookup failed for aggregate '%s'",
			 aggName);
	transtype = ((Form_pg_aggregate) GETSTRUCT(tup))->aggtranstype;

	/*
	 * initval is potentially null, so don't try to access it as a struct
	 * field. Must do it the hard way with SysCacheGetAttr.
	 */
	textInitVal = SysCacheGetAttr(AGGNAME, tup,
								  Anum_pg_aggregate_agginitval,
								  isNull);
	if (*isNull)
	{
		ReleaseSysCache(tup);
		return (Datum) 0;
	}

	strInitVal = DatumGetCString(DirectFunctionCall1(textout, textInitVal));

	ReleaseSysCache(tup);

	tup = SearchSysCache(TYPEOID,
						 ObjectIdGetDatum(transtype),
						 0, 0, 0);
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "AggNameGetInitVal: cache lookup failed on aggregate transition function return type %u", transtype);

	typinput = ((Form_pg_type) GETSTRUCT(tup))->typinput;
	typelem = ((Form_pg_type) GETSTRUCT(tup))->typelem;
	ReleaseSysCache(tup);

	initVal = OidFunctionCall3(typinput,
							   CStringGetDatum(strInitVal),
							   ObjectIdGetDatum(typelem),
							   Int32GetDatum(-1));

	pfree(strInitVal);
	return initVal;
}
