 /*-------------------------------------------------------------------------
 *
 * regproc.c
 *	  Functions for the built-in type "RegProcedure".
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/regproc.c,v 1.57 2000-07-03 23:09:52 wieck Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/syscache.h"

/*****************************************************************************
 *	 USER I/O ROUTINES														 *
 *****************************************************************************/

/*
 *		regprocin		- converts "proname" or "proid" to proid
 *
 *		proid of '-' signifies unknown, for consistency with regprocout
 */
Datum
regprocin(PG_FUNCTION_ARGS)
{
	char	   *pro_name_or_oid = PG_GETARG_CSTRING(0);
	HeapTuple	proctup;
	HeapTupleData tuple;
	RegProcedure result = InvalidOid;

	if (pro_name_or_oid[0] == '-' && pro_name_or_oid[1] == '\0')
		PG_RETURN_OID(InvalidOid);

	if (!IsIgnoringSystemIndexes())
	{

		/*
		 * we need to use the oid because there can be multiple entries
		 * with the same name.	We accept int4eq_1323 and 1323.
		 */
		if (pro_name_or_oid[0] >= '0' &&
			pro_name_or_oid[0] <= '9')
		{
			proctup = SearchSysCacheTuple(PROCOID,
										  DirectFunctionCall1(oidin,
											CStringGetDatum(pro_name_or_oid)),
										  0, 0, 0);
			if (HeapTupleIsValid(proctup))
				result = (RegProcedure) proctup->t_data->t_oid;
			else
				elog(ERROR, "No procedure with oid %s", pro_name_or_oid);
		}
		else
		{
			Relation	hdesc;
			Relation	idesc;
			IndexScanDesc sd;
			ScanKeyData skey[1];
			RetrieveIndexResult indexRes;
			Buffer		buffer;
			int			matches = 0;

			ScanKeyEntryInitialize(&skey[0],
								   (bits16) 0x0,
								   (AttrNumber) 1,
								   (RegProcedure) F_NAMEEQ,
								   CStringGetDatum(pro_name_or_oid));

			hdesc = heap_openr(ProcedureRelationName, AccessShareLock);
			idesc = index_openr(ProcedureNameIndex);

			sd = index_beginscan(idesc, false, 1, skey);
			while ((indexRes = index_getnext(sd, ForwardScanDirection)))
			{
				tuple.t_datamcxt = NULL;
				tuple.t_data = NULL;
				tuple.t_self = indexRes->heap_iptr;
				heap_fetch(hdesc, SnapshotNow,
						   &tuple,
						   &buffer);
				pfree(indexRes);
				if (tuple.t_data != NULL)
				{
					result = (RegProcedure) tuple.t_data->t_oid;
					ReleaseBuffer(buffer);

					if (++matches > 1)
						break;
				}
			}

			index_endscan(sd);
			index_close(idesc);
			heap_close(hdesc, AccessShareLock);

			if (matches > 1)
				elog(ERROR, "There is more than one procedure named %s.\n\tSupply the pg_proc oid inside single quotes.", pro_name_or_oid);
			else if (matches == 0)
				elog(ERROR, "No procedure with name %s", pro_name_or_oid);
		}
	}
	else
	{
		Relation	proc;
		HeapScanDesc procscan;
		ScanKeyData key;
		bool		isnull;

		proc = heap_openr(ProcedureRelationName, AccessShareLock);
		ScanKeyEntryInitialize(&key,
							   (bits16) 0,
							   (AttrNumber) 1,
							   (RegProcedure) F_NAMEEQ,
							   CStringGetDatum(pro_name_or_oid));

		procscan = heap_beginscan(proc, 0, SnapshotNow, 1, &key);
		if (!HeapScanIsValid(procscan))
		{
			heap_close(proc, AccessShareLock);
			elog(ERROR, "regprocin: could not begin scan of %s",
				 ProcedureRelationName);
			PG_RETURN_OID(InvalidOid);
		}
		proctup = heap_getnext(procscan, 0);
		if (HeapTupleIsValid(proctup))
		{
			result = (RegProcedure) heap_getattr(proctup,
												 ObjectIdAttributeNumber,
												 RelationGetDescr(proc),
												 &isnull);
			if (isnull)
				elog(ERROR, "regprocin: null procedure %s", pro_name_or_oid);
		}
		else
			elog(ERROR, "No procedure with name %s", pro_name_or_oid);

		heap_endscan(procscan);
		heap_close(proc, AccessShareLock);
	}

	PG_RETURN_OID(result);
}

/*
 *		regprocout		- converts proid to "pro_name"
 */
Datum
regprocout(PG_FUNCTION_ARGS)
{
	RegProcedure proid = PG_GETARG_OID(0);
	HeapTuple	proctup;
	char	   *result;

	result = (char *) palloc(NAMEDATALEN);

	if (proid == InvalidOid)
	{
		result[0] = '-';
		result[1] = '\0';
		PG_RETURN_CSTRING(result);
	}

	if (!IsBootstrapProcessingMode())
	{
		proctup = SearchSysCacheTuple(PROCOID,
									  ObjectIdGetDatum(proid),
									  0, 0, 0);

		if (HeapTupleIsValid(proctup))
		{
			char	   *s;

			s = NameStr(((Form_pg_proc) GETSTRUCT(proctup))->proname);
			StrNCpy(result, s, NAMEDATALEN);
		}
		else
		{
			result[0] = '-';
			result[1] = '\0';
		}
	}
	else
	{
		Relation	proc;
		HeapScanDesc procscan;
		ScanKeyData key;

		proc = heap_openr(ProcedureRelationName, AccessShareLock);
		ScanKeyEntryInitialize(&key,
							   (bits16) 0,
							   (AttrNumber) ObjectIdAttributeNumber,
							   (RegProcedure) F_INT4EQ,
							   ObjectIdGetDatum(proid));

		procscan = heap_beginscan(proc, 0, SnapshotNow, 1, &key);
		if (!HeapScanIsValid(procscan))
		{
			heap_close(proc, AccessShareLock);
			elog(ERROR, "regprocout: could not begin scan of %s",
				 ProcedureRelationName);
		}
		proctup = heap_getnext(procscan, 0);
		if (HeapTupleIsValid(proctup))
		{
			char	   *s;
			bool		isnull;

			s = (char *) heap_getattr(proctup, 1,
									  RelationGetDescr(proc), &isnull);
			if (!isnull)
				StrNCpy(result, s, NAMEDATALEN);
			else
				elog(ERROR, "regprocout: null procedure %u", proid);
		}
		else
		{
			result[0] = '-';
			result[1] = '\0';
		}
		heap_endscan(procscan);
		heap_close(proc, AccessShareLock);
	}

	PG_RETURN_CSTRING(result);
}

/*
 * oidvectortypes			- converts a vector of type OIDs to "typname" list
 *
 * The interface for this function is wrong: it should be told how many
 * OIDs are significant in the input vector, so that trailing InvalidOid
 * argument types can be recognized.
 */
Datum
oidvectortypes(PG_FUNCTION_ARGS)
{
	Oid		   *oidArray = (Oid *) PG_GETARG_POINTER(0);
	HeapTuple	typetup;
	text	   *result;
	int			numargs,
				num;

	/* Try to guess how many args there are :-( */
	numargs = 0;
	for (num = 0; num < FUNC_MAX_ARGS; num++)
	{
		if (oidArray[num] != InvalidOid)
			numargs = num + 1;
	}

	result = (text *) palloc((NAMEDATALEN + 1) * numargs + VARHDRSZ + 1);
	*VARDATA(result) = '\0';

	for (num = 0; num < numargs; num++)
	{
		typetup = SearchSysCacheTuple(TYPEOID,
									  ObjectIdGetDatum(oidArray[num]),
									  0, 0, 0);
		if (HeapTupleIsValid(typetup))
		{
			char	   *s;

			s = NameStr(((Form_pg_type) GETSTRUCT(typetup))->typname);
			StrNCpy(VARDATA(result) + strlen(VARDATA(result)), s,
					NAMEDATALEN);
			strcat(VARDATA(result), " ");
		}
		else
			strcat(VARDATA(result), "- ");
	}
	VARATT_SIZEP(result) = strlen(VARDATA(result)) + VARHDRSZ;
	PG_RETURN_TEXT_P(result);
}


/*****************************************************************************
 *	 PUBLIC ROUTINES														 *
 *****************************************************************************/

/* regproctooid()
 * Lowercase version of RegprocToOid() to allow case-insensitive SQL.
 * Define RegprocToOid() as a macro in builtins.h.
 * Referenced in pg_proc.h. - tgl 97/04/26
 */
Datum
regproctooid(PG_FUNCTION_ARGS)
{
	RegProcedure rp = PG_GETARG_OID(0);

	PG_RETURN_OID((Oid) rp);
}

/* (see int.c for comparison/operation routines) */
