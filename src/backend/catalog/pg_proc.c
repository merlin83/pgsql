/*-------------------------------------------------------------------------
 *
 * pg_proc.c
 *	  routines to support manipulation of the pg_proc relation
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/pg_proc.c,v 1.83 2002-08-04 19:48:09 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/pg_language.h"
#include "catalog/pg_proc.h"
#include "executor/executor.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_relation.h"
#include "parser/parse_type.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/sets.h"
#include "utils/syscache.h"


static void checkretval(Oid rettype, char fn_typtype, List *queryTreeList);
Datum fmgr_internal_validator(PG_FUNCTION_ARGS);
Datum fmgr_c_validator(PG_FUNCTION_ARGS);
Datum fmgr_sql_validator(PG_FUNCTION_ARGS);


/* ----------------------------------------------------------------
 *		ProcedureCreate
 * ----------------------------------------------------------------
 */
Oid
ProcedureCreate(const char *procedureName,
				Oid procNamespace,
				bool replace,
				bool returnsSet,
				Oid returnType,
				Oid languageObjectId,
				Oid languageValidator,
				const char *prosrc,
				const char *probin,
				bool isAgg,
				bool security_definer,
				bool isStrict,
				char volatility,
				int parameterCount,
				const Oid *parameterTypes)
{
	int			i;
	Relation	rel;
	HeapTuple	tup;
	HeapTuple	oldtup;
	char		nulls[Natts_pg_proc];
	Datum		values[Natts_pg_proc];
	char		replaces[Natts_pg_proc];
	Oid			typev[FUNC_MAX_ARGS];
	Oid			relid;
	NameData	procname;
	TupleDesc	tupDesc;
	Oid			retval;
	bool		is_update;
	ObjectAddress	myself,
					referenced;

	/*
	 * sanity checks
	 */
	Assert(PointerIsValid(prosrc));
	Assert(PointerIsValid(probin));

	if (parameterCount < 0 || parameterCount > FUNC_MAX_ARGS)
		elog(ERROR, "functions cannot have more than %d arguments",
			 FUNC_MAX_ARGS);

	/* Make sure we have a zero-padded param type array */
	MemSet(typev, 0, FUNC_MAX_ARGS * sizeof(Oid));
	if (parameterCount > 0)
		memcpy(typev, parameterTypes, parameterCount * sizeof(Oid));

	if (languageObjectId == SQLlanguageId)
	{
		/*
		 * If this call is defining a set, check if the set is already
		 * defined by looking to see whether this call's function text
		 * matches a function already in pg_proc.  If so just return the
		 * OID of the existing set.
		 */
		if (strcmp(procedureName, GENERICSETNAME) == 0)
		{
#ifdef SETS_FIXED

			/*
			 * The code below doesn't work any more because the PROSRC
			 * system cache and the pg_proc_prosrc_index have been
			 * removed. Instead a sequential heap scan or something better
			 * must get implemented. The reason for removing is that
			 * nbtree index crashes if sources exceed 2K --- what's likely
			 * for procedural languages.
			 *
			 * 1999/09/30 Jan
			 */
			text	   *prosrctext;

			prosrctext = DatumGetTextP(DirectFunctionCall1(textin,
											   CStringGetDatum(prosrc)));
			retval = GetSysCacheOid(PROSRC,
									PointerGetDatum(prosrctext),
									0, 0, 0);
			pfree(prosrctext);
			if (OidIsValid(retval))
				return retval;
#else
			elog(ERROR, "lookup for procedure by source needs fix (Jan)");
#endif   /* SETS_FIXED */
		}
	}

	/*
	 * don't allow functions of complex types that have the same name as
	 * existing attributes of the type
	 */
	if (parameterCount == 1 && OidIsValid(typev[0]) &&
		(relid = typeidTypeRelid(typev[0])) != 0 &&
		get_attnum(relid, (char *) procedureName) != InvalidAttrNumber)
		elog(ERROR, "method %s already an attribute of type %s",
			 procedureName, format_type_be(typev[0]));

	/*
	 * All seems OK; prepare the data to be inserted into pg_proc.
	 */

	for (i = 0; i < Natts_pg_proc; ++i)
	{
		nulls[i] = ' ';
		values[i] = (Datum) NULL;
		replaces[i] = 'r';
	}

	i = 0;
	namestrcpy(&procname, procedureName);
	values[i++] = NameGetDatum(&procname);		/* proname */
	values[i++] = ObjectIdGetDatum(procNamespace); /* pronamespace */
	values[i++] = Int32GetDatum(GetUserId());	/* proowner */
	values[i++] = ObjectIdGetDatum(languageObjectId); /* prolang */
	values[i++] = BoolGetDatum(isAgg);			/* proisagg */
	values[i++] = BoolGetDatum(security_definer); /* prosecdef */
	values[i++] = BoolGetDatum(isStrict);		/* proisstrict */
	values[i++] = BoolGetDatum(returnsSet);		/* proretset */
	values[i++] = CharGetDatum(volatility);		/* provolatile */
	values[i++] = UInt16GetDatum(parameterCount); /* pronargs */
	values[i++] = ObjectIdGetDatum(returnType);	/* prorettype */
	values[i++] = PointerGetDatum(typev);		/* proargtypes */
	values[i++] = DirectFunctionCall1(textin,	/* prosrc */
									  CStringGetDatum(prosrc));
	values[i++] = DirectFunctionCall1(textin,	/* probin */
									  CStringGetDatum(probin));
	/* proacl will be handled below */

	rel = heap_openr(ProcedureRelationName, RowExclusiveLock);
	tupDesc = rel->rd_att;

	/* Check for pre-existing definition */
	oldtup = SearchSysCache(PROCNAMENSP,
							PointerGetDatum(procedureName),
							UInt16GetDatum(parameterCount),
							PointerGetDatum(typev),
							ObjectIdGetDatum(procNamespace));

	if (HeapTupleIsValid(oldtup))
	{
		/* There is one; okay to replace it? */
		Form_pg_proc oldproc = (Form_pg_proc) GETSTRUCT(oldtup);

		if (!replace)
			elog(ERROR, "function %s already exists with same argument types",
				 procedureName);
		if (GetUserId() != oldproc->proowner && !superuser())
			elog(ERROR, "ProcedureCreate: you do not have permission to replace function %s",
				 procedureName);

		/*
		 * Not okay to change the return type of the existing proc, since
		 * existing rules, views, etc may depend on the return type.
		 */
		if (returnType != oldproc->prorettype ||
			returnsSet != oldproc->proretset)
			elog(ERROR, "ProcedureCreate: cannot change return type of existing function."
				 "\n\tUse DROP FUNCTION first.");

		/* Can't change aggregate status, either */
		if (oldproc->proisagg != isAgg)
		{
			if (oldproc->proisagg)
				elog(ERROR, "function %s is an aggregate",
					 procedureName);
			else
				elog(ERROR, "function %s is not an aggregate",
					 procedureName);
		}

		/* do not change existing ownership or permissions, either */
		replaces[Anum_pg_proc_proowner-1] = ' ';
		replaces[Anum_pg_proc_proacl-1] = ' ';

		/* Okay, do it... */
		tup = heap_modifytuple(oldtup, rel, values, nulls, replaces);
		simple_heap_update(rel, &tup->t_self, tup);

		ReleaseSysCache(oldtup);
		is_update = true;
	}
	else
	{
		/* Creating a new procedure */

		/* start out with empty permissions */
		nulls[Anum_pg_proc_proacl-1] = 'n';

		AssertTupleDescHasOid(tupDesc);
		tup = heap_formtuple(tupDesc, values, nulls);
		simple_heap_insert(rel, tup);
		is_update = false;
	}

	/* Need to update indices for either the insert or update case */
	if (RelationGetForm(rel)->relhasindex)
	{
		Relation	idescs[Num_pg_proc_indices];

		CatalogOpenIndices(Num_pg_proc_indices, Name_pg_proc_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_proc_indices, rel, tup);
		CatalogCloseIndices(Num_pg_proc_indices, idescs);
	}

	AssertTupleDescHasOid(tupDesc);
	retval = HeapTupleGetOid(tup);

	/*
	 * Create dependencies for the new function.  If we are updating an
	 * existing function, first delete any existing pg_depend entries.
	 */
	if (is_update)
		deleteDependencyRecordsFor(RelOid_pg_proc, retval);

	myself.classId = RelOid_pg_proc;
	myself.objectId = retval;
	myself.objectSubId = 0;

	/* dependency on namespace */
	referenced.classId = get_system_catalog_relid(NamespaceRelationName);
	referenced.objectId = procNamespace;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on implementation language */
	referenced.classId = get_system_catalog_relid(LanguageRelationName);
	referenced.objectId = languageObjectId;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on return type */
	if (OidIsValid(returnType))
	{
		referenced.classId = RelOid_pg_type;
		referenced.objectId = returnType;
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
	}

	/* dependency on input types */
	for (i = 0; i < parameterCount; i++)
	{
		if (OidIsValid(typev[i]))
		{
			referenced.classId = RelOid_pg_type;
			referenced.objectId = typev[i];
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
		}
	}

	heap_freetuple(tup);

	heap_close(rel, RowExclusiveLock);

	/* Verify function body */
	if (OidIsValid(languageValidator))
	{
		/* Advance command counter so new tuple can be seen by validator */
		CommandCounterIncrement();
		OidFunctionCall1(languageValidator, ObjectIdGetDatum(retval));
	}

	return retval;
}

/*
 * checkretval() -- check return value of a list of sql parse trees.
 *
 * The return value of a sql function is the value returned by
 * the final query in the function.  We do some ad-hoc define-time
 * type checking here to be sure that the user is returning the
 * type he claims.
 */
static void
checkretval(Oid rettype, List *queryTreeList)
{
	Query	   *parse;
	int			cmd;
	List	   *tlist;
	List	   *tlistitem;
	int			tlistlen;
	Oid			typerelid;
	Oid			restype;
	Relation	reln;
	int			relnatts;		/* physical number of columns in rel */
	int			rellogcols;		/* # of nondeleted columns in rel */
	int			colindex;		/* physical column index */

	/* guard against empty function body; OK only if no return type */
	if (queryTreeList == NIL)
	{
		if (rettype != InvalidOid)
			elog(ERROR, "function declared to return %s, but no SELECT provided",
				 format_type_be(rettype));
		return;
	}

	/* find the final query */
	parse = (Query *) nth(length(queryTreeList) - 1, queryTreeList);

	cmd = parse->commandType;
	tlist = parse->targetList;

	/*
	 * The last query must be a SELECT if and only if there is a return
	 * type.
	 */
	if (rettype == InvalidOid)
	{
		if (cmd == CMD_SELECT)
			elog(ERROR, "function declared with no return type, but final statement is a SELECT");
		return;
	}

	/* by here, the function is declared to return some type */
	if (cmd != CMD_SELECT)
		elog(ERROR, "function declared to return %s, but final statement is not a SELECT",
			 format_type_be(rettype));

	/*
	 * Count the non-junk entries in the result targetlist.
	 */
	tlistlen = ExecCleanTargetListLength(tlist);

	typerelid = typeidTypeRelid(rettype);

	if (fn_typtype == 'b')
	{
		/*
		 * For base-type returns, the target list should have exactly one
		 * entry, and its type should agree with what the user declared. (As
		 * of Postgres 7.2, we accept binary-compatible types too.)
		 */

		if (typerelid == InvalidOid)
		{
			if (tlistlen != 1)
				elog(ERROR, "function declared to return %s returns multiple columns in final SELECT",
					 format_type_be(rettype));

			restype = ((TargetEntry *) lfirst(tlist))->resdom->restype;
			if (!IsBinaryCompatible(restype, rettype))
				elog(ERROR, "return type mismatch in function: declared to return %s, returns %s",
					 format_type_be(rettype), format_type_be(restype));

			return;
		}

		/*
		 * If the target list is of length 1, and the type of the varnode in
		 * the target list matches the declared return type, this is okay.
		 * This can happen, for example, where the body of the function is
		 * 'SELECT func2()', where func2 has the same return type as the
		 * function that's calling it.
		 */
		if (tlistlen == 1)
		{
			restype = ((TargetEntry *) lfirst(tlist))->resdom->restype;
			if (IsBinaryCompatible(restype, rettype))
				return;
		}
	}
	else if (fn_typtype == 'c')
	{
		/*
		 * By here, the procedure returns a tuple or set of tuples.  This part
		 * of the typechecking is a hack. We look up the relation that is the
		 * declared return type, and scan the non-deleted attributes to ensure
		 * that they match the datatypes of the non-resjunk columns.
		 */
		reln = heap_open(typerelid, AccessShareLock);
		relnatts = reln->rd_rel->relnatts;
		rellogcols = 0;				/* we'll count nondeleted cols as we go */
		colindex = 0;

		foreach(tlistitem, tlist)
		{
			TargetEntry *tle = (TargetEntry *) lfirst(tlistitem);
			Form_pg_attribute attr;
			Oid			tletype;
			Oid			atttype;

			if (tle->resdom->resjunk)
				continue;

			do {
				colindex++;
				if (colindex > relnatts)
					elog(ERROR, "function declared to return %s does not SELECT the right number of columns (%d)",
						 format_type_be(rettype), rellogcols);
				attr = reln->rd_att->attrs[colindex - 1];
			} while (attr->attisdropped);
			rellogcols++;

			tletype = exprType(tle->expr);
			atttype = attr->atttypid;
			if (!IsBinaryCompatible(tletype, atttype))
				elog(ERROR, "function declared to return %s returns %s instead of %s at column %d",
					 format_type_be(rettype),
					 format_type_be(tletype),
					 format_type_be(atttype),
					 rellogcols);
		}

		for (;;)
		{
			colindex++;
			if (colindex > relnatts)
				break;
			if (!reln->rd_att->attrs[colindex - 1]->attisdropped)
				rellogcols++;
		}

		if (tlistlen != rellogcols)
			elog(ERROR, "function declared to return %s does not SELECT the right number of columns (%d)",
				 format_type_be(rettype), rellogcols);

		heap_close(reln, AccessShareLock);

		return;
	}
	else if (fn_typtype == 'p' && rettype == RECORDOID)
	{
		/*
		 * For RECORD return type, defer this check until we get the
		 * first tuple.
		 */
		return;
	}
	else
		elog(ERROR, "Unknown kind of return type specified for function");
}



/*
 * Validator for internal functions
 *
 * Check that the given internal function name (the "prosrc" value) is
 * a known builtin function.
 */
Datum
fmgr_internal_validator(PG_FUNCTION_ARGS)
{
	Oid			funcoid = PG_GETARG_OID(0);
	HeapTuple	tuple;
	Form_pg_proc proc;
	bool		isnull;
	Datum		tmp;
	char	   *prosrc;

	tuple = SearchSysCache(PROCOID, funcoid, 0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup of function %u failed", funcoid);
	proc = (Form_pg_proc) GETSTRUCT(tuple);

	tmp = SysCacheGetAttr(PROCOID, tuple, Anum_pg_proc_prosrc, &isnull);
	if (isnull)
		elog(ERROR, "null prosrc");
	prosrc = DatumGetCString(DirectFunctionCall1(textout, tmp));

	if (fmgr_internal_function(prosrc) == InvalidOid)
		elog(ERROR, "there is no built-in function named \"%s\"", prosrc);

	ReleaseSysCache(tuple);
	PG_RETURN_BOOL(true);
}



/*
 * Validator for C language functions
 *
 * Make sure that the library file exists, is loadable, and contains
 * the specified link symbol. Also check for a valid function
 * information record.
 */
Datum
fmgr_c_validator(PG_FUNCTION_ARGS)
{
	Oid			funcoid = PG_GETARG_OID(0);
	void	   *libraryhandle;
	HeapTuple	tuple;
	Form_pg_proc proc;
	bool		isnull;
	Datum		tmp;
	char	   *prosrc;
	char	   *probin;

	tuple = SearchSysCache(PROCOID, funcoid, 0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup of function %u failed", funcoid);
	proc = (Form_pg_proc) GETSTRUCT(tuple);

	tmp = SysCacheGetAttr(PROCOID, tuple, Anum_pg_proc_prosrc, &isnull);
	if (isnull)
		elog(ERROR, "null prosrc");
	prosrc = DatumGetCString(DirectFunctionCall1(textout, tmp));

	tmp = SysCacheGetAttr(PROCOID, tuple, Anum_pg_proc_probin, &isnull);
	if (isnull)
		elog(ERROR, "null probin");
	probin = DatumGetCString(DirectFunctionCall1(textout, tmp));
	
	(void) load_external_function(probin, prosrc, true, &libraryhandle);
	(void) fetch_finfo_record(libraryhandle, prosrc);

	ReleaseSysCache(tuple);
	PG_RETURN_BOOL(true);
}



/*
 * Validator for SQL language functions
 *
 * Parse it here in order to be sure that it contains no syntax
 * errors.
 */
Datum
fmgr_sql_validator(PG_FUNCTION_ARGS)
{
	Oid			funcoid = PG_GETARG_OID(0);
	HeapTuple	tuple;
	Form_pg_proc proc;
	List	   *querytree_list;
	bool		isnull;
	Datum		tmp;
	char	   *prosrc;
	char		functyptype;

	tuple = SearchSysCache(PROCOID, funcoid, 0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup of function %u failed", funcoid);

	proc = (Form_pg_proc) GETSTRUCT(tuple);

	if (!OidIsValid(proc->prorettype))
			elog(ERROR, "SQL functions cannot return type \"opaque\"");

	tmp = SysCacheGetAttr(PROCOID, tuple, Anum_pg_proc_prosrc, &isnull);
	if (isnull)
		elog(ERROR, "null prosrc");

	prosrc = DatumGetCString(DirectFunctionCall1(textout, tmp));

	/* check typtype to see if we have a predetermined return type */
	functyptype = typeid_get_typtype(proc->prorettype);

	querytree_list = pg_parse_and_rewrite(prosrc, proc->proargtypes, proc->pronargs);
	checkretval(proc->prorettype, functyptype, querytree_list);

	ReleaseSysCache(tuple);
	PG_RETURN_BOOL(true);
}
