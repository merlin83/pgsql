/*-------------------------------------------------------------------------
 *
 * remove.c
 *	  POSTGRES remove (function | type | operator ) utilty code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/Attic/remove.c,v 1.43 2000-01-10 17:14:32 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_language.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/comment.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "parser/parse_func.h"
#include "utils/acl.h"
#include "utils/syscache.h"


/*
 * RemoveOperator
 *		Deletes an operator.
 *
 * Exceptions:
 *		BadArg if name is invalid.
 *		BadArg if type1 is invalid.
 *		"WARN" if operator nonexistent.
 *		...
 */
void
RemoveOperator(char *operatorName,		/* operator name */
			   char *typeName1, /* first type name */
			   char *typeName2) /* optional second type name */
{
	Relation	relation;
	HeapTuple	tup;
	Oid			typeId1 = InvalidOid;
	Oid			typeId2 = InvalidOid;
	bool		defined;
	char	   *userName;
	char		oprtype;

	if (typeName1)
	{
		typeId1 = TypeGet(typeName1, &defined);
		if (!OidIsValid(typeId1))
		{
			elog(ERROR, "RemoveOperator: type '%s' does not exist", typeName1);
			return;
		}
	}

	if (typeName2)
	{
		typeId2 = TypeGet(typeName2, &defined);
		if (!OidIsValid(typeId2))
		{
			elog(ERROR, "RemoveOperator: type '%s' does not exist", typeName2);
			return;
		}
	}

	if (OidIsValid(typeId1) && OidIsValid(typeId2))
		oprtype = 'b';
	else if (OidIsValid(typeId1))
		oprtype = 'l';
	else
		oprtype = 'r';

	relation = heap_openr(OperatorRelationName, RowExclusiveLock);

	tup = SearchSysCacheTupleCopy(OPERNAME,
								  PointerGetDatum(operatorName),
								  ObjectIdGetDatum(typeId1),
								  ObjectIdGetDatum(typeId2),
								  CharGetDatum(oprtype));

	if (HeapTupleIsValid(tup))
	{
#ifndef NO_SECURITY
		userName = GetPgUserName();
		if (!pg_ownercheck(userName,
						   (char *) ObjectIdGetDatum(tup->t_data->t_oid),
						   OPEROID))
			elog(ERROR, "RemoveOperator: operator '%s': permission denied",
				 operatorName);
#endif


		/*** Delete any comments associated with this operator ***/

		DeleteComments(tup->t_data->t_oid);

		heap_delete(relation, &tup->t_self, NULL);

	}
	else
	{
		if (OidIsValid(typeId1) && OidIsValid(typeId2))
		{
			elog(ERROR, "RemoveOperator: binary operator '%s' taking '%s' and '%s' does not exist",
				 operatorName,
				 typeName1,
				 typeName2);
		}
		else if (OidIsValid(typeId1))
		{
			elog(ERROR, "RemoveOperator: right unary operator '%s' taking '%s' does not exist",
				 operatorName,
				 typeName1);
		}
		else
		{
			elog(ERROR, "RemoveOperator: left unary operator '%s' taking '%s' does not exist",
				 operatorName,
				 typeName2);
		}
	}
	heap_freetuple(tup);
	heap_close(relation, RowExclusiveLock);
}

#ifdef NOTYET
/*
 * this stuff is to support removing all reference to a type
 * don't use it  - pma 2/1/94
 */
/*
 *	SingleOpOperatorRemove
 *		Removes all operators that have operands or a result of type 'typeOid'.
 */
static void
SingleOpOperatorRemove(Oid typeOid)
{
	Relation	rel;
	ScanKeyData key[3];
	HeapScanDesc scan;
	HeapTuple	tup;
	static		attnums[3] = {7, 8, 9}; /* left, right, return */
	int			i;

	ScanKeyEntryInitialize(&key[0],
						   0, 0, F_OIDEQ, (Datum) typeOid);
	rel = heap_openr(OperatorRelationName, RowExclusiveLock);
	for (i = 0; i < 3; ++i)
	{
		key[0].sk_attno = attnums[i];
		scan = heap_beginscan(rel, 0, SnapshotNow, 1, key);
		while (HeapTupleIsValid(tup = heap_getnext(scan, 0))) {

		  /*** This is apparently a routine not in use, but remove ***/
		  /*** any comments anyways ***/

		  DeleteComments(tup->t_data->t_oid);

		  heap_delete(rel, &tup->t_self, NULL);

		}

		heap_endscan(scan);
	}
	heap_close(rel, RowExclusiveLock);
}

/*
 *	AttributeAndRelationRemove
 *		Removes all entries in the attribute and relation relations
 *		that contain entries of type 'typeOid'.
 *		Currently nothing calls this code, it is untested.
 */
static void
AttributeAndRelationRemove(Oid typeOid)
{
	struct oidlist
	{
		Oid			reloid;
		struct oidlist *next;
	};
	struct oidlist *oidptr,
			   *optr;
	Relation	rel;
	ScanKeyData key[1];
	HeapScanDesc scan;
	HeapTuple	tup;

	/*
	 * Get the oid's of the relations to be removed by scanning the entire
	 * attribute relation. We don't need to remove the attributes here,
	 * because amdestroy will remove all attributes of the relation. XXX
	 * should check for duplicate relations
	 */

	ScanKeyEntryInitialize(&key[0],
						   0, 3, F_OIDEQ, (Datum) typeOid);

	oidptr = (struct oidlist *) palloc(sizeof(*oidptr));
	oidptr->next = NULL;
	optr = oidptr;
	rel = heap_openr(AttributeRelationName, AccessShareLock);
	scan = heap_beginscan(rel, 0, SnapshotNow, 1, key);
	while (HeapTupleIsValid(tup = heap_getnext(scan, 0)))
	{
		optr->reloid = ((Form_pg_attribute) GETSTRUCT(tup))->attrelid;
		optr->next = (struct oidlist *) palloc(sizeof(*oidptr));
		optr = optr->next;
	}
	optr->next = NULL;
	heap_endscan(scan);
	heap_close(rel, AccessShareLock);

	optr = oidptr;

	ScanKeyEntryInitialize(&key[0], 0,
						   ObjectIdAttributeNumber,
						   F_OIDEQ, (Datum) 0);
	/* get RowExclusiveLock because heap_destroy will need it */
	rel = heap_openr(RelationRelationName, RowExclusiveLock);
	while (PointerIsValid((char *) optr->next))
	{
		key[0].sk_argument = (Datum) (optr++)->reloid;
		scan = heap_beginscan(rel, 0, SnapshotNow, 1, key);
		tup = heap_getnext(scan, 0);
		if (HeapTupleIsValid(tup))
		{
			char	   *name;

			name = NameStr(((Form_pg_class) GETSTRUCT(tup))->relname);
			heap_drop_with_catalog(name);
		}
		heap_endscan(scan);
	}
	heap_close(rel, RowExclusiveLock);
}

#endif	 /* NOTYET */

/*
 *	TypeRemove
 *		Removes the type 'typeName' and all attributes and relations that
 *		use it.
 */
void
RemoveType(char *typeName)		/* type name to be removed */
{
	Relation	relation;
	HeapTuple	tup;
	Oid			typeOid;
	char	   *shadow_type;
	char	   *userName;

#ifndef NO_SECURITY
	userName = GetPgUserName();
	if (!pg_ownercheck(userName, typeName, TYPENAME))
		elog(ERROR, "RemoveType: type '%s': permission denied",
			 typeName);
#endif

	relation = heap_openr(TypeRelationName, RowExclusiveLock);

	tup = SearchSysCacheTuple(TYPENAME,
							  PointerGetDatum(typeName),
							  0, 0, 0);
	if (!HeapTupleIsValid(tup))
	{
		heap_close(relation, RowExclusiveLock);
		elog(ERROR, "RemoveType: type '%s' does not exist", typeName);
	}

	typeOid = tup->t_data->t_oid;

	/*** Delete any comments associated with this type ***/

	DeleteComments(typeOid);

	heap_delete(relation, &tup->t_self, NULL);

	/* Now, Delete the "array of" that type */
	shadow_type = makeArrayTypeName(typeName);
	tup = SearchSysCacheTuple(TYPENAME,
							  PointerGetDatum(shadow_type),
							  0, 0, 0);
	if (!HeapTupleIsValid(tup))
	{
		heap_close(relation, RowExclusiveLock);
		elog(ERROR, "RemoveType: type '%s' does not exist", shadow_type);
	}

	heap_delete(relation, &tup->t_self, NULL);

	heap_close(relation, RowExclusiveLock);
}

/*
 * RemoveFunction
 *		Deletes a function.
 *
 * Exceptions:
 *		BadArg if name is invalid.
 *		"WARN" if function nonexistent.
 *		...
 */
void
RemoveFunction(char *functionName,		/* function name to be removed */
			   int nargs,
			   List *argNameList /* list of TypeNames */ )
{
	Relation	relation;
	HeapTuple	tup;
	Oid			argList[FUNC_MAX_ARGS];
	char	   *userName;
	char	   *typename;
	int			i;


	MemSet(argList, 0, FUNC_MAX_ARGS * sizeof(Oid));
	for (i = 0; i < nargs; i++)
	{
		typename = strVal(lfirst(argNameList));
		argNameList = lnext(argNameList);

		if (strcmp(typename, "opaque") == 0)
			argList[i] = 0;
		else
		{
			tup = SearchSysCacheTuple(TYPENAME,
									  PointerGetDatum(typename),
									  0, 0, 0);

			if (!HeapTupleIsValid(tup))
				elog(ERROR, "RemoveFunction: type '%s' not found", typename);
			argList[i] = tup->t_data->t_oid;
		}
	}

#ifndef NO_SECURITY
	userName = GetPgUserName();
	if (!pg_func_ownercheck(userName, functionName, nargs, argList))
	{
		elog(ERROR, "RemoveFunction: function '%s': permission denied",
			 functionName);
	}
#endif

	relation = heap_openr(ProcedureRelationName, RowExclusiveLock);
	tup = SearchSysCacheTuple(PROCNAME,
							  PointerGetDatum(functionName),
							  Int32GetDatum(nargs),
							  PointerGetDatum(argList),
							  0);

	if (!HeapTupleIsValid(tup))
	{
		heap_close(relation, RowExclusiveLock);
		func_error("RemoveFunction", functionName, nargs, argList, NULL);
	}

	if ((((Form_pg_proc) GETSTRUCT(tup))->prolang) == INTERNALlanguageId)
	{
		heap_close(relation, RowExclusiveLock);
		elog(ERROR, "RemoveFunction: function \"%s\" is built-in", functionName);
	}

	/*** Delete any comments associated with this function ***/

	DeleteComments(tup->t_data->t_oid);

	heap_delete(relation, &tup->t_self, NULL);

	heap_close(relation, RowExclusiveLock);
}

void
RemoveAggregate(char *aggName, char *aggType)
{
	Relation	relation;
	HeapTuple	tup;
	char	   *userName;
	Oid			basetypeID = InvalidOid;
	bool		defined;


	/*
	 * if a basetype is passed in, then attempt to find an aggregate for
	 * that specific type.
	 *
	 * else if the basetype is blank, then attempt to find an aggregate with
	 * a basetype of zero.	This is valid. It means that the aggregate is
	 * to apply to all basetypes.  ie, a counter of some sort.
	 *
	 */

	if (aggType)
	{
		basetypeID = TypeGet(aggType, &defined);
		if (!OidIsValid(basetypeID))
			elog(ERROR, "RemoveAggregate: type '%s' does not exist", aggType);
	}
	else
		basetypeID = 0;

#ifndef NO_SECURITY
	userName = GetPgUserName();
	if (!pg_aggr_ownercheck(userName, aggName, basetypeID))
	{
		if (aggType)
		{
			elog(ERROR, "RemoveAggregate: aggregate '%s' on type '%s': permission denied",
				 aggName, aggType);
		}
		else
		{
			elog(ERROR, "RemoveAggregate: aggregate '%s': permission denied",
				 aggName);
		}
	}
#endif

	relation = heap_openr(AggregateRelationName, RowExclusiveLock);
	tup = SearchSysCacheTuple(AGGNAME,
							  PointerGetDatum(aggName),
							  ObjectIdGetDatum(basetypeID),
							  0, 0);

	if (!HeapTupleIsValid(tup))
	{
		heap_close(relation, RowExclusiveLock);
		if (aggType)
		{
			elog(ERROR, "RemoveAggregate: aggregate '%s' for '%s' does not exist",
				 aggName, aggType);
		}
		else
		{
			elog(ERROR, "RemoveAggregate: aggregate '%s' for all types does not exist",
				 aggName);
		}
	}

	/*** Remove any comments related to this aggregate ***/

	DeleteComments(tup->t_data->t_oid);

	heap_delete(relation, &tup->t_self, NULL);

	heap_close(relation, RowExclusiveLock);
}
