/*-------------------------------------------------------------------------
 *
 * pg_proc.c
 *	  routines to support manipulation of the pg_proc relation
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/pg_proc.c,v 1.73 2002-05-21 22:05:54 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_language.h"
#include "catalog/pg_proc.h"
#include "executor/executor.h"
#include "miscadmin.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_type.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/sets.h"
#include "utils/syscache.h"


static void checkretval(Oid rettype, List *queryTreeList);


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
				const char *prosrc,
				const char *probin,
				bool isAgg,
				bool security_definer,
				bool isImplicit,
				bool isStrict,
				char volatility,
				int32 byte_pct,
				int32 perbyte_cpu,
				int32 percall_cpu,
				int32 outin_ratio,
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
	List	   *querytree_list;
	Oid			typev[FUNC_MAX_ARGS];
	Oid			relid;
	NameData	procname;
	TupleDesc	tupDesc;
	Oid			retval;

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

	if (!OidIsValid(returnType))
	{
		if (languageObjectId == SQLlanguageId)
			elog(ERROR, "SQL functions cannot return type \"opaque\"");
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
	 * If this is a postquel procedure, we parse it here in order to be
	 * sure that it contains no syntax errors.	We should store the plan
	 * in an Inversion file for use later, but for now, we just store the
	 * procedure's text in the prosrc attribute.
	 */

	if (languageObjectId == SQLlanguageId)
	{
		querytree_list = pg_parse_and_rewrite((char *) prosrc,
											  typev,
											  parameterCount);
		/* typecheck return value */
		checkretval(returnType, querytree_list);
	}

	/*
	 * If this is an internal procedure, check that the given internal
	 * function name (the 'prosrc' value) is a known builtin function.
	 *
	 * NOTE: in Postgres versions before 6.5, the SQL name of the created
	 * function could not be different from the internal name, and
	 * 'prosrc' wasn't used.  So there is code out there that does CREATE
	 * FUNCTION xyz AS '' LANGUAGE 'internal'.	To preserve some modicum
	 * of backwards compatibility, accept an empty 'prosrc' value as
	 * meaning the supplied SQL function name.
	 */
	if (languageObjectId == INTERNALlanguageId)
	{
		if (strlen(prosrc) == 0)
			prosrc = procedureName;
		if (fmgr_internal_function((char *) prosrc) == InvalidOid)
			elog(ERROR,
				 "there is no built-in function named \"%s\"",
				 prosrc);
	}

	/*
	 * If this is a dynamically loadable procedure, make sure that the
	 * library file exists, is loadable, and contains the specified link
	 * symbol.	Also check for a valid function information record.
	 *
	 * We used to perform these checks only when the function was first
	 * called, but it seems friendlier to verify the library's validity at
	 * CREATE FUNCTION time.
	 */
	if (languageObjectId == ClanguageId)
	{
		void	   *libraryhandle;

		/* If link symbol is specified as "-", substitute procedure name */
		if (strcmp(prosrc, "-") == 0)
			prosrc = procedureName;
		(void) load_external_function((char *) probin,
									  (char *) prosrc,
									  true,
									  &libraryhandle);
		(void) fetch_finfo_record(libraryhandle, (char *) prosrc);
	}

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
	values[i++] = BoolGetDatum(isImplicit);		/* proimplicit */
	values[i++] = BoolGetDatum(isStrict);		/* proisstrict */
	values[i++] = BoolGetDatum(returnsSet);		/* proretset */
	values[i++] = CharGetDatum(volatility);		/* provolatile */
	values[i++] = UInt16GetDatum(parameterCount); /* pronargs */
	values[i++] = ObjectIdGetDatum(returnType);	/* prorettype */
	values[i++] = PointerGetDatum(typev);		/* proargtypes */
	values[i++] = Int32GetDatum(byte_pct);		/* probyte_pct */
	values[i++] = Int32GetDatum(perbyte_cpu);	/* properbyte_cpu */
	values[i++] = Int32GetDatum(percall_cpu);	/* propercall_cpu */
	values[i++] = Int32GetDatum(outin_ratio);	/* prooutin_ratio */
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

		/* do not change existing permissions, either */
		replaces[Anum_pg_proc_proacl-1] = ' ';

		/* Okay, do it... */
		tup = heap_modifytuple(oldtup, rel, values, nulls, replaces);
		simple_heap_update(rel, &tup->t_self, tup);

		ReleaseSysCache(oldtup);
	}
	else
	{
		/* Creating a new procedure */

		/* start out with empty permissions */
		nulls[Anum_pg_proc_proacl-1] = 'n';

		tup = heap_formtuple(tupDesc, values, nulls);
		simple_heap_insert(rel, tup);
	}

	/* Need to update indices for either the insert or update case */
	if (RelationGetForm(rel)->relhasindex)
	{
		Relation	idescs[Num_pg_proc_indices];

		CatalogOpenIndices(Num_pg_proc_indices, Name_pg_proc_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_proc_indices, rel, tup);
		CatalogCloseIndices(Num_pg_proc_indices, idescs);
	}

	retval = tup->t_data->t_oid;
	heap_freetuple(tup);

	heap_close(rel, RowExclusiveLock);

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
	Oid			relid;
	int			relnatts;
	int			i;

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

	/*
	 * For base-type returns, the target list should have exactly one
	 * entry, and its type should agree with what the user declared. (As
	 * of Postgres 7.2, we accept binary-compatible types too.)
	 */
	typerelid = typeidTypeRelid(rettype);
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

	/*
	 * By here, the procedure returns a tuple or set of tuples.  This part
	 * of the typechecking is a hack. We look up the relation that is the
	 * declared return type, and be sure that attributes 1 .. n in the
	 * target list match the declared types.
	 */
	reln = heap_open(typerelid, AccessShareLock);
	relid = reln->rd_id;
	relnatts = reln->rd_rel->relnatts;

	if (tlistlen != relnatts)
		elog(ERROR, "function declared to return %s does not SELECT the right number of columns (%d)",
			 format_type_be(rettype), relnatts);

	/* expect attributes 1 .. n in order */
	i = 0;
	foreach(tlistitem, tlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tlistitem);
		Oid			tletype;
		Oid			atttype;

		if (tle->resdom->resjunk)
			continue;
		tletype = exprType(tle->expr);
		atttype = reln->rd_att->attrs[i]->atttypid;
		if (!IsBinaryCompatible(tletype, atttype))
			elog(ERROR, "function declared to return %s returns %s instead of %s at column %d",
				 format_type_be(rettype),
				 format_type_be(tletype),
				 format_type_be(atttype),
				 i + 1);
		i++;
	}

	/* this shouldn't happen, but let's just check... */
	if (i != relnatts)
		elog(ERROR, "function declared to return %s does not SELECT the right number of columns (%d)",
			 format_type_be(rettype), relnatts);

	heap_close(reln, AccessShareLock);
}
