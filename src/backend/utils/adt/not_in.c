/*-------------------------------------------------------------------------
 *
 * not_in.c
 *	  Executes the "not_in" operator for any data type
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/Attic/not_in.c,v 1.28 2002-03-30 01:02:41 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 * X HACK WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! X
 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 *
 * This code is the OLD not-in code that is HACKED
 * into place until operators that can have arguments as
 * columns are ******REALLY****** implemented!!!!!!!!!!!
 *
 */

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/namespace.h"
#include "utils/builtins.h"

static int	my_varattno(Relation rd, char *a);

/* ----------------------------------------------------------------
 *
 * ----------------------------------------------------------------
 */
Datum
int4notin(PG_FUNCTION_ARGS)
{
	int32		not_in_arg = PG_GETARG_INT32(0);
	text	   *relation_and_attr = PG_GETARG_TEXT_P(1);
	List	   *names;
	int			nnames;
	RangeVar   *relrv;
	char	   *attribute;
	Relation	relation_to_scan;
	int32		integer_value;
	HeapTuple	current_tuple;
	HeapScanDesc scan_descriptor;
	bool		isNull,
				retval;
	int			attrid;
	Datum		value;

	/* Parse the argument */

	names = textToQualifiedNameList(relation_and_attr, "int4notin");
	nnames = length(names);
	if (nnames < 2)
		elog(ERROR, "int4notin: must provide relationname.attributename");
	attribute = strVal(nth(nnames-1, names));
	names = ltruncate(nnames-1, names);
	relrv = makeRangeVarFromNameList(names);

	/* Open the relation and get a relation descriptor */

	relation_to_scan = heap_openrv(relrv, AccessShareLock);

	/* Find the column to search */

	attrid = my_varattno(relation_to_scan, attribute);
	if (attrid < 0)
		elog(ERROR, "int4notin: unknown attribute %s for relation %s",
			 attribute, RelationGetRelationName(relation_to_scan));

	scan_descriptor = heap_beginscan(relation_to_scan, false, SnapshotNow,
									 0, (ScanKey) NULL);

	retval = true;

	/* do a scan of the relation, and do the check */
	while (HeapTupleIsValid(current_tuple = heap_getnext(scan_descriptor, 0)))
	{
		value = heap_getattr(current_tuple,
							 (AttrNumber) attrid,
							 RelationGetDescr(relation_to_scan),
							 &isNull);
		if (isNull)
			continue;
		integer_value = DatumGetInt32(value);
		if (not_in_arg == integer_value)
		{
			retval = false;
			break;				/* can stop scanning now */
		}
	}

	/* close the relation */
	heap_endscan(scan_descriptor);
	heap_close(relation_to_scan, AccessShareLock);

	PG_RETURN_BOOL(retval);
}

Datum
oidnotin(PG_FUNCTION_ARGS)
{
	Oid			the_oid = PG_GETARG_OID(0);

#ifdef NOT_USED
	text	   *relation_and_attr = PG_GETARG_TEXT_P(1);
#endif

	if (the_oid == InvalidOid)
		PG_RETURN_BOOL(false);
	/* XXX assume oid maps to int4 */
	return int4notin(fcinfo);
}

/*
 * XXX
 * If varattno (in parser/catalog_utils.h) ever is added to
 * cinterface.a, this routine should go away
 */
static int
my_varattno(Relation rd, char *a)
{
	int			i;

	for (i = 0; i < rd->rd_rel->relnatts; i++)
	{
		if (namestrcmp(&rd->rd_att->attrs[i]->attname, a) == 0)
			return i + 1;
	}
	return -1;
}
