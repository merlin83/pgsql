/*-------------------------------------------------------------------------
 *
 * functioncmds.c
 *
 *	  Routines for CREATE and DROP FUNCTION commands
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/functioncmds.c,v 1.12 2002-07-22 20:23:19 petere Exp $
 *
 * DESCRIPTION
 *	  These routines take the parse tree and pick out the
 *	  appropriate arguments/flags, and pass the results to the
 *	  corresponding "FooDefine" routines (in src/catalog) that do
 *	  the actual catalog-munging.  These routines also verify permission
 *	  of the user to execute the command.
 *
 * NOTES
 *	  These things must be defined and committed in the following order:
 *		"create function":
 *				input/output, recv/send procedures
 *		"create type":
 *				type
 *		"create operator":
 *				operators
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_cast.h"
#include "catalog/pg_language.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "optimizer/cost.h"
#include "parser/parse_func.h"
#include "parser/parse_type.h"
#include "utils/acl.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


/*
 *	 Examine the "returns" clause returnType of the CREATE FUNCTION statement
 *	 and return information about it as *prorettype_p and *returnsSet.
 *
 * This is more complex than the average typename lookup because we want to
 * allow a shell type to be used, or even created if the specified return type
 * doesn't exist yet.  (Without this, there's no way to define the I/O procs
 * for a new type.)  But SQL function creation won't cope, so error out if
 * the target language is SQL.
 */
static void
compute_return_type(TypeName *returnType, Oid languageOid,
					Oid *prorettype_p, bool *returnsSet_p)
{
	Oid		rettype;

	rettype = LookupTypeName(returnType);

	if (OidIsValid(rettype))
	{
		if (!get_typisdefined(rettype))
		{
			if (languageOid == SQLlanguageId)
				elog(ERROR, "SQL functions cannot return shell types");
			else
				elog(WARNING, "Return type \"%s\" is only a shell",
					 TypeNameToString(returnType));
		}
	}
	else
	{
		char      *typnam = TypeNameToString(returnType);

		if (strcmp(typnam, "opaque") == 0)
			rettype = InvalidOid;
		else
		{
			Oid			namespaceId;
			AclResult	aclresult;
			char	   *typname;

			if (languageOid == SQLlanguageId)
				elog(ERROR, "Type \"%s\" does not exist", typnam);
			elog(WARNING, "ProcedureCreate: type %s is not yet defined",
				 typnam);
			namespaceId = QualifiedNameGetCreationNamespace(returnType->names,
															&typname);
			aclresult = pg_namespace_aclcheck(namespaceId, GetUserId(),
											  ACL_CREATE);
			if (aclresult != ACLCHECK_OK)
				aclcheck_error(aclresult, get_namespace_name(namespaceId));
			rettype = TypeShellMake(typname, namespaceId);
			if (!OidIsValid(rettype))
				elog(ERROR, "could not create type %s", typnam);
		}
	}

	*prorettype_p = rettype;
	*returnsSet_p = returnType->setof;
}

/*
 * Interpret the argument-types list of the CREATE FUNCTION statement.
 */
static int
compute_parameter_types(List *argTypes, Oid languageOid,
						Oid *parameterTypes)
{
	int			parameterCount = 0;
	List	   *x;

	MemSet(parameterTypes, 0, FUNC_MAX_ARGS * sizeof(Oid));
	foreach(x, argTypes)
	{
		TypeName   *t = (TypeName *) lfirst(x);
		Oid			toid;

		if (parameterCount >= FUNC_MAX_ARGS)
			elog(ERROR, "functions cannot have more than %d arguments",
				 FUNC_MAX_ARGS);

		toid = LookupTypeName(t);
		if (OidIsValid(toid))
		{
			if (!get_typisdefined(toid))
				elog(WARNING, "Argument type \"%s\" is only a shell",
					 TypeNameToString(t));
		}
		else
		{
			char      *typnam = TypeNameToString(t);

			if (strcmp(typnam, "opaque") == 0)
			{
				if (languageOid == SQLlanguageId)
					elog(ERROR, "SQL functions cannot have arguments of type \"opaque\"");
				toid = InvalidOid;
			}
			else
				elog(ERROR, "Type \"%s\" does not exist", typnam);
		}

		if (t->setof)
			elog(ERROR, "functions cannot accept set arguments");

		parameterTypes[parameterCount++] = toid;
	}

	return parameterCount;
}


/*
 * Dissect the list of options assembled in gram.y into function
 * attributes.
 */

static void
compute_attributes_sql_style(const List *options,
							 List **as,
							 char **language,
							 char *volatility_p,
							 bool *strict_p,
							 bool *security_definer)
{
	const List *option;
	DefElem *as_item = NULL;
	DefElem *language_item = NULL;
	DefElem *volatility_item = NULL;
	DefElem *strict_item = NULL;
	DefElem *security_item = NULL;

	foreach(option, options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "as")==0)
		{
			if (as_item)
				elog(ERROR, "conflicting or redundant options");
			as_item = defel;
		}
		else if (strcmp(defel->defname, "language")==0)
		{
			if (language_item)
				elog(ERROR, "conflicting or redundant options");
			language_item = defel;
		}
		else if (strcmp(defel->defname, "volatility")==0)
		{
			if (volatility_item)
				elog(ERROR, "conflicting or redundant options");
			volatility_item = defel;
		}
		else if (strcmp(defel->defname, "strict")==0)
		{
			if (strict_item)
				elog(ERROR, "conflicting or redundant options");
			strict_item = defel;
		}
		else if (strcmp(defel->defname, "security")==0)
		{
			if (security_item)
				elog(ERROR, "conflicting or redundant options");
			security_item = defel;
		}
		else
			elog(ERROR, "invalid CREATE FUNCTION option");
	}

	if (as_item)
		*as = (List *)as_item->arg;
	else
		elog(ERROR, "no function body specified");

	if (language_item)
		*language = strVal(language_item->arg);
	else
		elog(ERROR, "no language specified");

	if (volatility_item)
	{
		if (strcmp(strVal(volatility_item->arg), "immutable")==0)
			*volatility_p = PROVOLATILE_IMMUTABLE;
		else if (strcmp(strVal(volatility_item->arg), "stable")==0)
			*volatility_p = PROVOLATILE_STABLE;
		else if (strcmp(strVal(volatility_item->arg), "volatile")==0)
			*volatility_p = PROVOLATILE_VOLATILE;
		else
			elog(ERROR, "invalid volatility");
	}

	if (strict_item)
		*strict_p = intVal(strict_item->arg);
	if (security_item)
		*security_definer = intVal(security_item->arg);
}


/*-------------
 *	 Interpret the parameters *parameters and return their contents as
 *	 *byte_pct_p, etc.
 *
 *	These parameters supply optional information about a function.
 *	All have defaults if not specified.
 *
 *	Note: currently, only two of these parameters actually do anything:
 *
 *	 * isStrict means the function should not be called when any NULL
 *	   inputs are present; instead a NULL result value should be assumed.
 *
 *	 * volatility tells the optimizer whether the function's result can
 *	   be assumed to be repeatable over multiple evaluations.
 *
 *	The other four parameters are not used anywhere.	They used to be
 *	used in the "expensive functions" optimizer, but that's been dead code
 *	for a long time.
 *------------
 */
static void
compute_attributes_with_style(List *parameters,
							  int32 *byte_pct_p, int32 *perbyte_cpu_p,
							  int32 *percall_cpu_p, int32 *outin_ratio_p,
							  bool *isStrict_p,
							  char *volatility_p)
{
	List	   *pl;

	foreach(pl, parameters)
	{
		DefElem    *param = (DefElem *) lfirst(pl);

		if (strcasecmp(param->defname, "isstrict") == 0)
			*isStrict_p = true;
		else if (strcasecmp(param->defname, "isimmutable") == 0)
			*volatility_p = PROVOLATILE_IMMUTABLE;
		else if (strcasecmp(param->defname, "isstable") == 0)
			*volatility_p = PROVOLATILE_STABLE;
		else if (strcasecmp(param->defname, "isvolatile") == 0)
			*volatility_p = PROVOLATILE_VOLATILE;
		else if (strcasecmp(param->defname, "iscachable") == 0)
		{
			/* obsolete spelling of isImmutable */
			*volatility_p = PROVOLATILE_IMMUTABLE;
		}
		else if (strcasecmp(param->defname, "trusted") == 0)
		{
			/*
			 * we don't have untrusted functions any more. The 4.2
			 * implementation is lousy anyway so I took it out. -ay 10/94
			 */
			elog(ERROR, "untrusted function has been decommissioned.");
		}
		else if (strcasecmp(param->defname, "byte_pct") == 0)
			*byte_pct_p = (int) defGetNumeric(param);
		else if (strcasecmp(param->defname, "perbyte_cpu") == 0)
			*perbyte_cpu_p = (int) defGetNumeric(param);
		else if (strcasecmp(param->defname, "percall_cpu") == 0)
			*percall_cpu_p = (int) defGetNumeric(param);
		else if (strcasecmp(param->defname, "outin_ratio") == 0)
			*outin_ratio_p = (int) defGetNumeric(param);
		else
			elog(WARNING, "Unrecognized function attribute '%s' ignored",
				 param->defname);
	}
}


/*
 * For a dynamically linked C language object, the form of the clause is
 *
 *	   AS <object file name> [, <link symbol name> ]
 *
 * In all other cases
 *
 *	   AS <object reference, or sql code>
 *
 */

static void
interpret_AS_clause(Oid languageOid, const char *languageName, const List *as,
					char **prosrc_str_p, char **probin_str_p)
{
	Assert(as != NIL);

	if (languageOid == ClanguageId)
	{
		/*
		 * For "C" language, store the file name in probin and, when
		 * given, the link symbol name in prosrc.
		 */
		*probin_str_p = strVal(lfirst(as));
		if (lnext(as) == NULL)
			*prosrc_str_p = "-";
		else
			*prosrc_str_p = strVal(lsecond(as));
	}
	else
	{
		/* Everything else wants the given string in prosrc. */
		*prosrc_str_p = strVal(lfirst(as));
		*probin_str_p = "-";

		if (lnext(as) != NIL)
			elog(ERROR, "CREATE FUNCTION: only one AS item needed for %s language",
				 languageName);
	}
}



/*
 * CreateFunction
 *	 Execute a CREATE FUNCTION utility statement.
 */
void
CreateFunction(CreateFunctionStmt *stmt)
{
	char	   *probin_str;
	char	   *prosrc_str;
	Oid			prorettype;
	bool		returnsSet;
	char	   *language;
	char		languageName[NAMEDATALEN];
	Oid			languageOid;
	Oid			languageValidator;
	char	   *funcname;
	Oid			namespaceId;
	AclResult	aclresult;
	int			parameterCount;
	Oid			parameterTypes[FUNC_MAX_ARGS];
	int32		byte_pct,
				perbyte_cpu,
				percall_cpu,
				outin_ratio;
	bool		isStrict,
				security;
	char		volatility;
	HeapTuple	languageTuple;
	Form_pg_language languageStruct;
	List	   *as_clause;

	/* Convert list of names to a name and namespace */
	namespaceId = QualifiedNameGetCreationNamespace(stmt->funcname,
													&funcname);

	/* Check we have creation rights in target namespace */
	aclresult = pg_namespace_aclcheck(namespaceId, GetUserId(), ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, get_namespace_name(namespaceId));

	/* defaults attributes */
	byte_pct = BYTE_PCT;
	perbyte_cpu = PERBYTE_CPU;
	percall_cpu = PERCALL_CPU;
	outin_ratio = OUTIN_RATIO;
	isStrict = false;
	security = false;
	volatility = PROVOLATILE_VOLATILE;

	/* override attributes from explicit list */
	compute_attributes_sql_style(stmt->options,
								 &as_clause, &language, &volatility, &isStrict, &security);

	/* Convert language name to canonical case */
	case_translate_language_name(language, languageName);

	/* Look up the language and validate permissions */
	languageTuple = SearchSysCache(LANGNAME,
								   PointerGetDatum(languageName),
								   0, 0, 0);
	if (!HeapTupleIsValid(languageTuple))
		elog(ERROR, "language \"%s\" does not exist", languageName);

	languageOid = HeapTupleGetOid(languageTuple);
	languageStruct = (Form_pg_language) GETSTRUCT(languageTuple);

	if (languageStruct->lanpltrusted)
	{
		/* if trusted language, need USAGE privilege */
		AclResult	aclresult;

		aclresult = pg_language_aclcheck(languageOid, GetUserId(), ACL_USAGE);
		if (aclresult != ACLCHECK_OK)
			aclcheck_error(aclresult, NameStr(languageStruct->lanname));
	}
	else
	{
		/* if untrusted language, must be superuser */
		if (!superuser())
			aclcheck_error(ACLCHECK_NO_PRIV, NameStr(languageStruct->lanname));
	}

	languageValidator = languageStruct->lanvalidator;

	ReleaseSysCache(languageTuple);

	/*
	 * Convert remaining parameters of CREATE to form wanted by
	 * ProcedureCreate.
	 */
	compute_return_type(stmt->returnType, languageOid,
						&prorettype, &returnsSet);

	parameterCount = compute_parameter_types(stmt->argTypes, languageOid,
											 parameterTypes);

	compute_attributes_with_style(stmt->withClause,
								  &byte_pct, &perbyte_cpu, &percall_cpu,
								  &outin_ratio, &isStrict, &volatility);

	interpret_AS_clause(languageOid, languageName, as_clause,
						&prosrc_str, &probin_str);

	if (languageOid == INTERNALlanguageId)
	{
		/*
		 * In PostgreSQL versions before 6.5, the SQL name of the
		 * created function could not be different from the internal
		 * name, and "prosrc" wasn't used.  So there is code out there
		 * that does CREATE FUNCTION xyz AS '' LANGUAGE 'internal'.
		 * To preserve some modicum of backwards compatibility, accept
		 * an empty "prosrc" value as meaning the supplied SQL
		 * function name.
		 */
		if (strlen(prosrc_str) == 0)
			prosrc_str = funcname;
	}

	if (languageOid == ClanguageId)
	{
		/* If link symbol is specified as "-", substitute procedure name */
		if (strcmp(prosrc_str, "-") == 0)
			prosrc_str = funcname;
	}

	/*
	 * And now that we have all the parameters, and know we're permitted
	 * to do so, go ahead and create the function.
	 */
	ProcedureCreate(funcname,
					namespaceId,
					stmt->replace,
					returnsSet,
					prorettype,
					languageOid,
					languageValidator,
					prosrc_str, /* converted to text later */
					probin_str, /* converted to text later */
					false,		/* not an aggregate */
					security,
					isStrict,
					volatility,
					byte_pct,
					perbyte_cpu,
					percall_cpu,
					outin_ratio,
					parameterCount,
					parameterTypes);
}


/*
 * RemoveFunction
 *		Deletes a function.
 */
void
RemoveFunction(RemoveFuncStmt *stmt)
{
	List	   *functionName = stmt->funcname;
	List	   *argTypes = stmt->args; /* list of TypeName nodes */
	Oid			funcOid;
	HeapTuple	tup;
	ObjectAddress object;

	/*
	 * Find the function, do permissions and validity checks
	 */
	funcOid = LookupFuncNameTypeNames(functionName, argTypes, 
									  true, "RemoveFunction");

	tup = SearchSysCache(PROCOID,
						 ObjectIdGetDatum(funcOid),
						 0, 0, 0);
	if (!HeapTupleIsValid(tup))	/* should not happen */
		elog(ERROR, "RemoveFunction: couldn't find tuple for function %s",
			 NameListToString(functionName));

	/* Permission check: must own func or its namespace */
	if (!pg_proc_ownercheck(funcOid, GetUserId()) &&
		!pg_namespace_ownercheck(((Form_pg_proc) GETSTRUCT(tup))->pronamespace,
								 GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, NameListToString(functionName));

	if (((Form_pg_proc) GETSTRUCT(tup))->proisagg)
		elog(ERROR, "RemoveFunction: function '%s' is an aggregate"
			 "\n\tUse DROP AGGREGATE to remove it",
			 NameListToString(functionName));

	if (((Form_pg_proc) GETSTRUCT(tup))->prolang == INTERNALlanguageId)
	{
		/* "Helpful" WARNING when removing a builtin function ... */
		elog(WARNING, "Removing built-in function \"%s\"",
			 NameListToString(functionName));
	}

	ReleaseSysCache(tup);

	/*
	 * Do the deletion
	 */
	object.classId = RelOid_pg_proc;
	object.objectId = funcOid;
	object.objectSubId = 0;

	performDeletion(&object, stmt->behavior);
}

/*
 * Guts of function deletion.
 *
 * Note: this is also used for aggregate deletion, since the OIDs of
 * both functions and aggregates point to pg_proc.
 */
void
RemoveFunctionById(Oid funcOid)
{
	Relation	relation;
	HeapTuple	tup;
	bool		isagg;

	/*
	 * Delete the pg_proc tuple.
	 */
	relation = heap_openr(ProcedureRelationName, RowExclusiveLock);

	tup = SearchSysCache(PROCOID,
						 ObjectIdGetDatum(funcOid),
						 0, 0, 0);
	if (!HeapTupleIsValid(tup))	/* should not happen */
		elog(ERROR, "RemoveFunctionById: couldn't find tuple for function %u",
			 funcOid);

	isagg = ((Form_pg_proc) GETSTRUCT(tup))->proisagg;

	simple_heap_delete(relation, &tup->t_self);

	ReleaseSysCache(tup);

	heap_close(relation, RowExclusiveLock);

	/*
	 * If there's a pg_aggregate tuple, delete that too.
	 */
	if (isagg)
	{
		relation = heap_openr(AggregateRelationName, RowExclusiveLock);

		tup = SearchSysCache(AGGFNOID,
							 ObjectIdGetDatum(funcOid),
							 0, 0, 0);
		if (!HeapTupleIsValid(tup))	/* should not happen */
			elog(ERROR, "RemoveFunctionById: couldn't find pg_aggregate tuple for %u",
				 funcOid);

		simple_heap_delete(relation, &tup->t_self);

		ReleaseSysCache(tup);

		heap_close(relation, RowExclusiveLock);
	}
}



/*
 * CREATE CAST
 */
void
CreateCast(CreateCastStmt *stmt)
{
	Oid			sourcetypeid;
	Oid			targettypeid;
	Oid			funcid;
	HeapTuple	tuple;
	Relation	relation;
	Form_pg_proc procstruct;

	Datum		values[Natts_pg_proc];
	char		nulls[Natts_pg_proc];
	int			i;

	ObjectAddress myself,
		referenced;

	sourcetypeid = LookupTypeName(stmt->sourcetype);
	if (!OidIsValid(sourcetypeid))
		elog(ERROR, "source data type %s does not exist",
			 TypeNameToString(stmt->sourcetype));

	targettypeid = LookupTypeName(stmt->targettype);
	if (!OidIsValid(targettypeid))
		elog(ERROR, "target data type %s does not exist",
			 TypeNameToString(stmt->targettype));

	if (sourcetypeid == targettypeid)
		elog(ERROR, "source data type and target data type are the same");

	relation = heap_openr(CastRelationName, RowExclusiveLock);

	tuple = SearchSysCache(CASTSOURCETARGET,
						   ObjectIdGetDatum(sourcetypeid),
						   ObjectIdGetDatum(targettypeid),
						   0, 0);
	if (HeapTupleIsValid(tuple))
		elog(ERROR, "cast from data type %s to data type %s already exists",
			 TypeNameToString(stmt->sourcetype),
			 TypeNameToString(stmt->targettype));

	if (stmt->func != NULL)
	{
		funcid = LookupFuncNameTypeNames(stmt->func->funcname, stmt->func->funcargs, false, "CreateCast");

		if(!pg_proc_ownercheck(funcid, GetUserId()))
			elog(ERROR, "permission denied");

		tuple = SearchSysCache(PROCOID, ObjectIdGetDatum(funcid), 0, 0, 0);
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "cache lookup of function %u failed", funcid);

		procstruct = (Form_pg_proc) GETSTRUCT(tuple);
		if (procstruct->pronargs != 1)
			elog(ERROR, "cast function must take 1 argument");
		if (procstruct->proargtypes[0] != sourcetypeid)
			elog(ERROR, "argument of cast function must match source data type");
		if (procstruct->prorettype != targettypeid)
			elog(ERROR, "return data type of cast function must match target data type");
		if (procstruct->provolatile != PROVOLATILE_IMMUTABLE)
			elog(ERROR, "cast function must be immutable");
		if (procstruct->proisagg)
			elog(ERROR, "cast function must not be an aggregate function");
		if (procstruct->proretset)
			elog(ERROR, "cast function must not return a set");

		ReleaseSysCache(tuple);
	}
	else
	{
		/* indicates binary compatibility */
		if (!pg_type_ownercheck(sourcetypeid, GetUserId())
			|| !pg_type_ownercheck(targettypeid, GetUserId()))
			elog(ERROR, "permission denied");
		funcid = 0;
	}

	/* ready to go */
	values[Anum_pg_cast_castsource-1] = ObjectIdGetDatum(sourcetypeid);
	values[Anum_pg_cast_casttarget-1] = ObjectIdGetDatum(targettypeid);
	values[Anum_pg_cast_castfunc-1] = ObjectIdGetDatum(funcid);
	values[Anum_pg_cast_castimplicit-1] = BoolGetDatum(stmt->implicit);

	for (i = 0; i < Natts_pg_cast; ++i)
		nulls[i] = ' ';

	tuple = heap_formtuple(RelationGetDescr(relation), values, nulls);
	simple_heap_insert(relation, tuple);

	if (RelationGetForm(relation)->relhasindex)
	{
		Relation	idescs[Num_pg_cast_indices];

		CatalogOpenIndices(Num_pg_cast_indices, Name_pg_cast_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_cast_indices, relation, tuple);
		CatalogCloseIndices(Num_pg_cast_indices, idescs);
	}

	myself.classId = RelationGetRelid(relation);
	myself.objectId = HeapTupleGetOid(tuple);
	myself.objectSubId = 0;

	/* dependency on source type */
	referenced.classId = RelOid_pg_type;
	referenced.objectId = sourcetypeid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on target type */
	referenced.classId = RelOid_pg_type;
	referenced.objectId = targettypeid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on function */
	if (OidIsValid(funcid))
	{
		referenced.classId = RelOid_pg_proc;
		referenced.objectId = funcid;
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
	}

	heap_freetuple(tuple);
	heap_close(relation, RowExclusiveLock);
}



/*
 * DROP CAST
 */
void
DropCast(DropCastStmt *stmt)
{
	Oid			sourcetypeid;
	Oid			targettypeid;
	HeapTuple	tuple;
	Form_pg_cast caststruct;
	ObjectAddress object;

	sourcetypeid = LookupTypeName(stmt->sourcetype);
	if (!OidIsValid(sourcetypeid))
		elog(ERROR, "source data type %s does not exist",
			 TypeNameToString(stmt->sourcetype));

	targettypeid = LookupTypeName(stmt->targettype);
	if (!OidIsValid(targettypeid))
		elog(ERROR, "target data type %s does not exist",
			 TypeNameToString(stmt->targettype));

	tuple = SearchSysCache(CASTSOURCETARGET,
						 ObjectIdGetDatum(sourcetypeid),
						 ObjectIdGetDatum(targettypeid),
						 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cast from type %s to type %s does not exist",
			 TypeNameToString(stmt->sourcetype),
			 TypeNameToString(stmt->targettype));

	/* Permission check */
	caststruct = (Form_pg_cast) GETSTRUCT(tuple);
	if (caststruct->castfunc != InvalidOid)
	{
		if(!pg_proc_ownercheck(caststruct->castfunc, GetUserId()))
			elog(ERROR, "permission denied");
	}
	else
	{
		if (!pg_type_ownercheck(sourcetypeid, GetUserId())
			|| !pg_type_ownercheck(targettypeid, GetUserId()))
			elog(ERROR, "permission denied");
	}

	ReleaseSysCache(tuple);

	/*
	 * Do the deletion
	 */
	object.classId = get_system_catalog_relid(CastRelationName);
	object.objectId = HeapTupleGetOid(tuple);
	object.objectSubId = 0;

	performDeletion(&object, stmt->behavior);
}


void
DropCastById(Oid castOid)
{
	Relation	relation,
				index;
	ScanKeyData scankey;
	IndexScanDesc scan;
	HeapTuple	tuple;

	relation = heap_openr(CastRelationName, RowExclusiveLock);
	index = index_openr(CastOidIndex);

	ScanKeyEntryInitialize(&scankey, 0x0,
						   1, F_OIDEQ, ObjectIdGetDatum(castOid));
	scan = index_beginscan(relation, index, SnapshotNow, 1, &scankey);
	tuple = index_getnext(scan, ForwardScanDirection);
	if (HeapTupleIsValid(tuple))
		simple_heap_delete(relation, &tuple->t_self);
	else
		elog(ERROR, "could not find tuple for cast %u", castOid);
	index_endscan(scan);

	index_close(index);
	heap_close(relation, RowExclusiveLock);
}
