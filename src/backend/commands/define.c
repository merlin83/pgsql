/*-------------------------------------------------------------------------
 *
 * define.c
 *
 *	  These routines execute some of the CREATE statements.  In an earlier
 *	  version of Postgres, these were "define" statements.
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/define.c,v 1.73 2002-04-05 00:31:25 tgl Exp $
 *
 * DESCRIPTION
 *	  The "DefineFoo" routines take the parse tree and pick out the
 *	  appropriate arguments/flags, passing the results to the
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
 *		Most of the parse-tree manipulation routines are defined in
 *		commands/manip.c.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include <math.h>

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/heap.h"
#include "catalog/namespace.h"
#include "catalog/pg_aggregate.h"
#include "catalog/pg_language.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "optimizer/cost.h"
#include "parser/parse_func.h"
#include "parser/parse_type.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


static Oid	findTypeIOFunction(const char *procname, bool isOutput);
static char *defGetString(DefElem *def);
static double defGetNumeric(DefElem *def);
static TypeName *defGetTypeName(DefElem *def);
static int	defGetTypeLength(DefElem *def);

#define DEFAULT_TYPDELIM		','


/*
 * Translate the input language name to lower case.
 */
static void
case_translate_language_name(const char *input, char *output)
{
	int			i;

	for (i = 0; i < NAMEDATALEN - 1 && input[i]; ++i)
		output[i] = tolower((unsigned char) input[i]);

	output[i] = '\0';
}


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
			char	   *typname;

			if (languageOid == SQLlanguageId)
				elog(ERROR, "Type \"%s\" does not exist", typnam);
			elog(WARNING, "ProcedureCreate: type %s is not yet defined",
				 typnam);
			namespaceId = QualifiedNameGetCreationNamespace(returnType->names,
															&typname);
			rettype = TypeShellMake(typname, namespaceId);
			if (!OidIsValid(rettype))
				elog(ERROR, "could not create type %s", typnam);
		}
	}

	*prorettype_p = rettype;
	*returnsSet_p = returnType->setof;
}


static void
compute_full_attributes(List *parameters,
						int32 *byte_pct_p, int32 *perbyte_cpu_p,
						int32 *percall_cpu_p, int32 *outin_ratio_p,
						bool *isStrict_p, char *volatility_p)
{
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
	List	   *pl;

	/* the defaults */
	*byte_pct_p = BYTE_PCT;
	*perbyte_cpu_p = PERBYTE_CPU;
	*percall_cpu_p = PERCALL_CPU;
	*outin_ratio_p = OUTIN_RATIO;
	*isStrict_p = false;
	*volatility_p = PROVOLATILE_VOLATILE;

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
CreateFunction(ProcedureStmt *stmt)
{
	char	   *probin_str;
	char	   *prosrc_str;
	Oid			prorettype;
	bool		returnsSet;
	char		languageName[NAMEDATALEN];
	Oid			languageOid;
	char	   *funcname;
	Oid			namespaceId;
	int32		byte_pct,
				perbyte_cpu,
				percall_cpu,
				outin_ratio;
	bool		isStrict;
	char		volatility;
	HeapTuple	languageTuple;
	Form_pg_language languageStruct;

	/* Convert list of names to a name and namespace */
	namespaceId = QualifiedNameGetCreationNamespace(stmt->funcname,
													&funcname);

	/* Convert language name to canonical case */
	case_translate_language_name(stmt->language, languageName);

	/* Look up the language and validate permissions */
	languageTuple = SearchSysCache(LANGNAME,
								   PointerGetDatum(languageName),
								   0, 0, 0);
	if (!HeapTupleIsValid(languageTuple))
		elog(ERROR, "language \"%s\" does not exist", languageName);

	languageOid = languageTuple->t_data->t_oid;
	languageStruct = (Form_pg_language) GETSTRUCT(languageTuple);

	if (!((languageStruct->lanpltrusted
		   && pg_language_aclcheck(languageOid, GetUserId()) == ACLCHECK_OK)
		  || superuser()))
		elog(ERROR, "permission denied");

	ReleaseSysCache(languageTuple);

	/*
	 * Convert remaining parameters of CREATE to form wanted by
	 * ProcedureCreate.
	 */
	compute_return_type(stmt->returnType, languageOid,
						&prorettype, &returnsSet);

	compute_full_attributes(stmt->withClause,
							&byte_pct, &perbyte_cpu, &percall_cpu,
							&outin_ratio, &isStrict, &volatility);

	interpret_AS_clause(languageOid, languageName, stmt->as,
						&prosrc_str, &probin_str);

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
					prosrc_str, /* converted to text later */
					probin_str, /* converted to text later */
					true,		/* (obsolete "trusted") */
					isStrict,
					volatility,
					byte_pct,
					perbyte_cpu,
					percall_cpu,
					outin_ratio,
					stmt->argTypes);
}



/*
 * DefineOperator
 *		this function extracts all the information from the
 *		parameter list generated by the parser and then has
 *		OperatorCreate() do all the actual work.
 *
 * 'parameters' is a list of DefElem
 */
void
DefineOperator(List *names, List *parameters)
{
	char	   *oprName;
	Oid			oprNamespace;
	uint16		precedence = 0; /* operator precedence */
	bool		canHash = false;	/* operator hashes */
	bool		isLeftAssociative = true;		/* operator is left
												 * associative */
	char	   *functionName = NULL;	/* function for operator */
	TypeName   *typeName1 = NULL;		/* first type name */
	TypeName   *typeName2 = NULL;		/* second type name */
	Oid			typeId1 = InvalidOid;	/* types converted to OID */
	Oid			typeId2 = InvalidOid;
	char	   *commutatorName = NULL;	/* optional commutator operator
										 * name */
	char	   *negatorName = NULL;		/* optional negator operator name */
	char	   *restrictionName = NULL; /* optional restrict. sel.
										 * procedure */
	char	   *joinName = NULL;	/* optional join sel. procedure name */
	char	   *sortName1 = NULL;		/* optional first sort operator */
	char	   *sortName2 = NULL;		/* optional second sort operator */
	List	   *pl;

	/* Convert list of names to a name and namespace */
	oprNamespace = QualifiedNameGetCreationNamespace(names, &oprName);

	/*
	 * loop over the definition list and extract the information we need.
	 */
	foreach(pl, parameters)
	{
		DefElem    *defel = (DefElem *) lfirst(pl);

		if (strcasecmp(defel->defname, "leftarg") == 0)
		{
			typeName1 = defGetTypeName(defel);
			if (typeName1->setof)
				elog(ERROR, "setof type not implemented for leftarg");
		}
		else if (strcasecmp(defel->defname, "rightarg") == 0)
		{
			typeName2 = defGetTypeName(defel);
			if (typeName2->setof)
				elog(ERROR, "setof type not implemented for rightarg");
		}
		else if (strcasecmp(defel->defname, "procedure") == 0)
			functionName = defGetString(defel);
		else if (strcasecmp(defel->defname, "precedence") == 0)
		{
			/* NOT IMPLEMENTED (never worked in v4.2) */
			elog(NOTICE, "CREATE OPERATOR: precedence not implemented");
		}
		else if (strcasecmp(defel->defname, "associativity") == 0)
		{
			/* NOT IMPLEMENTED (never worked in v4.2) */
			elog(NOTICE, "CREATE OPERATOR: associativity not implemented");
		}
		else if (strcasecmp(defel->defname, "commutator") == 0)
			commutatorName = defGetString(defel);
		else if (strcasecmp(defel->defname, "negator") == 0)
			negatorName = defGetString(defel);
		else if (strcasecmp(defel->defname, "restrict") == 0)
			restrictionName = defGetString(defel);
		else if (strcasecmp(defel->defname, "join") == 0)
			joinName = defGetString(defel);
		else if (strcasecmp(defel->defname, "hashes") == 0)
			canHash = TRUE;
		else if (strcasecmp(defel->defname, "sort1") == 0)
			sortName1 = defGetString(defel);
		else if (strcasecmp(defel->defname, "sort2") == 0)
			sortName2 = defGetString(defel);
		else
		{
			elog(WARNING, "DefineOperator: attribute \"%s\" not recognized",
				 defel->defname);
		}
	}

	/*
	 * make sure we have our required definitions
	 */
	if (functionName == NULL)
		elog(ERROR, "Define: \"procedure\" unspecified");

	/* Transform type names to type OIDs */
	if (typeName1)
		typeId1 = typenameTypeId(typeName1);
	if (typeName2)
		typeId2 = typenameTypeId(typeName2);

	/*
	 * now have OperatorCreate do all the work..
	 */
	OperatorCreate(oprName,		/* operator name */
				   typeId1,		/* left type id */
				   typeId2,		/* right type id */
				   functionName,	/* function for operator */
				   precedence,	/* operator precedence */
				   isLeftAssociative,	/* operator is left associative */
				   commutatorName,		/* optional commutator operator
										 * name */
				   negatorName, /* optional negator operator name */
				   restrictionName,		/* optional restrict. sel.
										 * procedure */
				   joinName,	/* optional join sel. procedure name */
				   canHash,		/* operator hashes */
				   sortName1,	/* optional first sort operator */
				   sortName2);	/* optional second sort operator */

}

/*
 *	DefineAggregate
 */
void
DefineAggregate(List *names, List *parameters)
{
	char	   *aggName;
	Oid			aggNamespace;
	char	   *transfuncName = NULL;
	char	   *finalfuncName = NULL;
	TypeName   *baseType = NULL;
	TypeName   *transType = NULL;
	char	   *initval = NULL;
	Oid			baseTypeId;
	Oid			transTypeId;
	List	   *pl;

	/* Convert list of names to a name and namespace */
	aggNamespace = QualifiedNameGetCreationNamespace(names, &aggName);

	foreach(pl, parameters)
	{
		DefElem    *defel = (DefElem *) lfirst(pl);

		/*
		 * sfunc1, stype1, and initcond1 are accepted as obsolete
		 * spellings for sfunc, stype, initcond.
		 */
		if (strcasecmp(defel->defname, "sfunc") == 0)
			transfuncName = defGetString(defel);
		else if (strcasecmp(defel->defname, "sfunc1") == 0)
			transfuncName = defGetString(defel);
		else if (strcasecmp(defel->defname, "finalfunc") == 0)
			finalfuncName = defGetString(defel);
		else if (strcasecmp(defel->defname, "basetype") == 0)
			baseType = defGetTypeName(defel);
		else if (strcasecmp(defel->defname, "stype") == 0)
			transType = defGetTypeName(defel);
		else if (strcasecmp(defel->defname, "stype1") == 0)
			transType = defGetTypeName(defel);
		else if (strcasecmp(defel->defname, "initcond") == 0)
			initval = defGetString(defel);
		else if (strcasecmp(defel->defname, "initcond1") == 0)
			initval = defGetString(defel);
		else
			elog(WARNING, "DefineAggregate: attribute \"%s\" not recognized",
				 defel->defname);
	}

	/*
	 * make sure we have our required definitions
	 */
	if (baseType == NULL)
		elog(ERROR, "Define: \"basetype\" unspecified");
	if (transType == NULL)
		elog(ERROR, "Define: \"stype\" unspecified");
	if (transfuncName == NULL)
		elog(ERROR, "Define: \"sfunc\" unspecified");

	/*
	 * Handle the aggregate's base type (input data type).  This can be
	 * specified as 'ANY' for a data-independent transition function, such
	 * as COUNT(*).
	 */
	baseTypeId = LookupTypeName(baseType);
	if (OidIsValid(baseTypeId))
	{
		/* no need to allow aggregates on as-yet-undefined types */
		if (!get_typisdefined(baseTypeId))
			elog(ERROR, "Type \"%s\" is only a shell",
				 TypeNameToString(baseType));
	}
	else
	{
		char      *typnam = TypeNameToString(baseType);

		if (strcasecmp(typnam, "ANY") != 0)
			elog(ERROR, "Type \"%s\" does not exist", typnam);
		baseTypeId = InvalidOid;
	}

	/* handle transtype --- no special cases here */
	transTypeId = typenameTypeId(transType);

	/*
	 * Most of the argument-checking is done inside of AggregateCreate
	 */
	AggregateCreate(aggName,	/* aggregate name */
					aggNamespace,	/* namespace */
					transfuncName,		/* step function name */
					finalfuncName,		/* final function name */
					baseTypeId,	/* type of data being aggregated */
					transTypeId,	/* transition data type */
					initval);	/* initial condition */
}

/*
 * DefineDomain
 *		Registers a new domain.
 */
void
DefineDomain(CreateDomainStmt *stmt)
{
	char	   *domainName;
	Oid			domainNamespace;
	int16		internalLength;
	int16		externalLength;
	Oid			inputProcedure;
	Oid			outputProcedure;
	Oid			receiveProcedure;
	Oid			sendProcedure;
	bool		byValue;
	char		delimiter;
	char		alignment;
	char		storage;
	char		typtype;
	Datum		datum;
	bool		isnull;
	char	   *defaultValue = NULL;
	char	   *defaultValueBin = NULL;
	bool		typNotNull = false;
	Oid			basetypelem;
	int32		typNDims = length(stmt->typename->arrayBounds);
	HeapTuple	typeTup;
	List	   *schema = stmt->constraints;
	List	   *listptr;

	/* Convert list of names to a name and namespace */
	domainNamespace = QualifiedNameGetCreationNamespace(stmt->domainname,
														&domainName);

	/*
	 * Domainnames, unlike typenames don't need to account for the '_'
	 * prefix.  So they can be one character longer.
	 */
	if (strlen(domainName) > (NAMEDATALEN - 1))
		elog(ERROR, "CREATE DOMAIN: domain names must be %d characters or less",
			 NAMEDATALEN - 1);

	/*
	 * Look up the base type.
	 */
	typeTup = typenameType(stmt->typename);

	/*
	 * What we really don't want is domains of domains.  This could cause all sorts
	 * of neat issues if we allow that.
	 *
	 * With testing, we may determine complex types should be allowed
	 */
	typtype = ((Form_pg_type) GETSTRUCT(typeTup))->typtype;
	if (typtype != 'b')
		elog(ERROR, "DefineDomain: %s is not a basetype",
			 TypeNameToString(stmt->typename));

	/* passed by value */
	byValue = ((Form_pg_type) GETSTRUCT(typeTup))->typbyval;

	/* Required Alignment */
	alignment = ((Form_pg_type) GETSTRUCT(typeTup))->typalign;

	/* TOAST Strategy */
	storage = ((Form_pg_type) GETSTRUCT(typeTup))->typstorage;

	/* Storage Length */
	internalLength = ((Form_pg_type) GETSTRUCT(typeTup))->typlen;

	/* External Length (unused) */
	externalLength = ((Form_pg_type) GETSTRUCT(typeTup))->typprtlen;

	/* Array element Delimiter */
	delimiter = ((Form_pg_type) GETSTRUCT(typeTup))->typdelim;

	/* I/O Functions */
	inputProcedure = ((Form_pg_type) GETSTRUCT(typeTup))->typinput;
	outputProcedure = ((Form_pg_type) GETSTRUCT(typeTup))->typoutput;
	receiveProcedure = ((Form_pg_type) GETSTRUCT(typeTup))->typreceive;
	sendProcedure = ((Form_pg_type) GETSTRUCT(typeTup))->typsend;

	/* Inherited default value */
	datum =	SysCacheGetAttr(TYPEOID, typeTup,
							Anum_pg_type_typdefault, &isnull);
	if (!isnull)
		defaultValue = DatumGetCString(DirectFunctionCall1(textout, datum));

	/* Inherited default binary value */
	datum =	SysCacheGetAttr(TYPEOID, typeTup,
							Anum_pg_type_typdefaultbin, &isnull);
	if (!isnull)
		defaultValueBin = DatumGetCString(DirectFunctionCall1(textout, datum));

	/*
	 * Pull out the typelem name of the parent OID.
	 *
	 * This is what enables us to make a domain of an array
	 */
	basetypelem = ((Form_pg_type) GETSTRUCT(typeTup))->typelem;

	/*
	 * Run through constraints manually to avoid the additional
	 * processing conducted by DefineRelation() and friends.
	 *
	 * Besides, we don't want any constraints to be cooked.  We'll
	 * do that when the table is created via MergeDomainAttributes().
	 */
	foreach(listptr, schema)
	{
		Constraint *colDef = lfirst(listptr);
		bool nullDefined = false;
		Node	   *expr;
		ParseState *pstate;

		switch (colDef->contype)
		{
			/*
	 		 * The inherited default value may be overridden by the user
			 * with the DEFAULT <expr> statement.
			 *
	 		 * We have to search the entire constraint tree returned as we
			 * don't want to cook or fiddle too much.
			 */
			case CONSTR_DEFAULT:
				/* Create a dummy ParseState for transformExpr */
				pstate = make_parsestate(NULL);
				/*
				 * Cook the colDef->raw_expr into an expression.
				 * Note: Name is strictly for error message
				 */
				expr = cookDefault(pstate, colDef->raw_expr,
								   typeTup->t_data->t_oid,
								   stmt->typename->typmod,
								   domainName);
				/*
				 * Expression must be stored as a nodeToString result,
				 * but we also require a valid textual representation
				 * (mainly to make life easier for pg_dump).
				 */
				defaultValue = deparse_expression(expr,
								deparse_context_for(domainName,
													InvalidOid),
												   false);
				defaultValueBin = nodeToString(expr);
				break;

			/*
			 * Find the NULL constraint.
			 */
			case CONSTR_NOTNULL:
				if (nullDefined) {
					elog(ERROR, "CREATE DOMAIN has conflicting NULL / NOT NULL constraint");
				} else {
					typNotNull = true;
					nullDefined = true;
				}
		  		break;

			case CONSTR_NULL:
				if (nullDefined) {
					elog(ERROR, "CREATE DOMAIN has conflicting NULL / NOT NULL constraint");
				} else {
					typNotNull = false;
					nullDefined = true;
				}
		  		break;

		  	case CONSTR_UNIQUE:
		  		elog(ERROR, "CREATE DOMAIN / UNIQUE indexes not supported");
		  		break;

		  	case CONSTR_PRIMARY:
		  		elog(ERROR, "CREATE DOMAIN / PRIMARY KEY indexes not supported");
		  		break;

		  	case CONSTR_CHECK:
		  		elog(ERROR, "DefineDomain: CHECK Constraints not supported");
		  		break;

		  	case CONSTR_ATTR_DEFERRABLE:
		  	case CONSTR_ATTR_NOT_DEFERRABLE:
		  	case CONSTR_ATTR_DEFERRED:
		  	case CONSTR_ATTR_IMMEDIATE:
		  		elog(ERROR, "DefineDomain: DEFERRABLE, NON DEFERRABLE, DEFERRED and IMMEDIATE not supported");
		  		break;

			default:
		  		elog(ERROR, "DefineDomain: unrecognized constraint node type");
		  		break;
		}
	}

	/*
	 * Have TypeCreate do all the real work.
	 */
	TypeCreate(domainName,			/* type name */
			   domainNamespace,		/* namespace */
			   InvalidOid,			/* preassigned type oid (not done here) */
			   InvalidOid,			/* relation oid (n/a here) */
			   internalLength,		/* internal size */
			   externalLength,		/* external size */
			   'd',					/* type-type (domain type) */
			   delimiter,			/* array element delimiter */
			   inputProcedure,		/* input procedure */
			   outputProcedure,		/* output procedure */
			   receiveProcedure,	/* receive procedure */
			   sendProcedure,		/* send procedure */
			   basetypelem,			/* element type ID */
			   typeTup->t_data->t_oid,	/* base type ID */
			   defaultValue,		/* default type value (text) */
			   defaultValueBin,		/* default type value (binary) */
			   byValue,				/* passed by value */
			   alignment,			/* required alignment */
			   storage,				/* TOAST strategy */
			   stmt->typename->typmod, /* typeMod value */
			   typNDims,			/* Array dimensions for base type */
			   typNotNull);			/* Type NOT NULL */

	/*
	 * Now we can clean up.
	 */
	ReleaseSysCache(typeTup);
}

/*
 * DefineType
 *		Registers a new type.
 */
void
DefineType(List *names, List *parameters)
{
	char	   *typeName;
	Oid			typeNamespace;
	int16		internalLength = -1;	/* int2 */
	int16		externalLength = -1;	/* int2 */
	Oid			elemType = InvalidOid;
	char	   *inputName = NULL;
	char	   *outputName = NULL;
	char	   *sendName = NULL;
	char	   *receiveName = NULL;
	char	   *defaultValue = NULL;
	bool		byValue = false;
	char		delimiter = DEFAULT_TYPDELIM;
	char		alignment = 'i';	/* default alignment */
	char		storage = 'p';	/* default TOAST storage method */
	Oid			inputOid;
	Oid			outputOid;
	Oid			sendOid;
	Oid			receiveOid;
	char	   *shadow_type;
	List	   *pl;
	Oid			typoid;

	/* Convert list of names to a name and namespace */
	typeNamespace = QualifiedNameGetCreationNamespace(names, &typeName);

	/*
	 * Type names must be one character shorter than other names, allowing
	 * room to create the corresponding array type name with prepended
	 * "_".
	 */
	if (strlen(typeName) > (NAMEDATALEN - 2))
		elog(ERROR, "DefineType: type names must be %d characters or less",
			 NAMEDATALEN - 2);

	foreach(pl, parameters)
	{
		DefElem    *defel = (DefElem *) lfirst(pl);

		if (strcasecmp(defel->defname, "internallength") == 0)
			internalLength = defGetTypeLength(defel);
		else if (strcasecmp(defel->defname, "externallength") == 0)
			externalLength = defGetTypeLength(defel);
		else if (strcasecmp(defel->defname, "input") == 0)
			inputName = defGetString(defel);
		else if (strcasecmp(defel->defname, "output") == 0)
			outputName = defGetString(defel);
		else if (strcasecmp(defel->defname, "send") == 0)
			sendName = defGetString(defel);
		else if (strcasecmp(defel->defname, "receive") == 0)
			receiveName = defGetString(defel);
		else if (strcasecmp(defel->defname, "delimiter") == 0)
		{
			char	   *p = defGetString(defel);

			delimiter = p[0];
		}
		else if (strcasecmp(defel->defname, "element") == 0)
			elemType = typenameTypeId(defGetTypeName(defel));
		else if (strcasecmp(defel->defname, "default") == 0)
			defaultValue = defGetString(defel);
		else if (strcasecmp(defel->defname, "passedbyvalue") == 0)
			byValue = true;
		else if (strcasecmp(defel->defname, "alignment") == 0)
		{
			char	   *a = defGetString(defel);

			/*
			 * Note: if argument was an unquoted identifier, parser will
			 * have applied xlateSqlType() to it, so be prepared to
			 * recognize translated type names as well as the nominal
			 * form.
			 */
			if (strcasecmp(a, "double") == 0)
				alignment = 'd';
			else if (strcasecmp(a, "float8") == 0)
				alignment = 'd';
			else if (strcasecmp(a, "int4") == 0)
				alignment = 'i';
			else if (strcasecmp(a, "int2") == 0)
				alignment = 's';
			else if (strcasecmp(a, "char") == 0)
				alignment = 'c';
			else if (strcasecmp(a, "bpchar") == 0)
				alignment = 'c';
			else
				elog(ERROR, "DefineType: \"%s\" alignment not recognized",
					 a);
		}
		else if (strcasecmp(defel->defname, "storage") == 0)
		{
			char	   *a = defGetString(defel);

			if (strcasecmp(a, "plain") == 0)
				storage = 'p';
			else if (strcasecmp(a, "external") == 0)
				storage = 'e';
			else if (strcasecmp(a, "extended") == 0)
				storage = 'x';
			else if (strcasecmp(a, "main") == 0)
				storage = 'm';
			else
				elog(ERROR, "DefineType: \"%s\" storage not recognized",
					 a);
		}
		else
		{
			elog(WARNING, "DefineType: attribute \"%s\" not recognized",
				 defel->defname);
		}
	}

	/*
	 * make sure we have our required definitions
	 */
	if (inputName == NULL)
		elog(ERROR, "Define: \"input\" unspecified");
	if (outputName == NULL)
		elog(ERROR, "Define: \"output\" unspecified");

	/* Convert I/O proc names to OIDs */
	inputOid = findTypeIOFunction(inputName, false);
	outputOid = findTypeIOFunction(outputName, true);
	if (sendName)
		sendOid = findTypeIOFunction(sendName, true);
	else
		sendOid = outputOid;
	if (receiveName)
		receiveOid = findTypeIOFunction(receiveName, false);
	else
		receiveOid = inputOid;

	/*
	 * now have TypeCreate do all the real work.
	 */
	typoid =
		TypeCreate(typeName,		/* type name */
				   typeNamespace,	/* namespace */
				   InvalidOid,		/* preassigned type oid (not done here) */
				   InvalidOid,		/* relation oid (n/a here) */
				   internalLength,	/* internal size */
				   externalLength,	/* external size */
				   'b',				/* type-type (base type) */
				   delimiter,		/* array element delimiter */
				   inputOid,		/* input procedure */
				   outputOid,		/* output procedure */
				   receiveOid,		/* receive procedure */
				   sendOid,			/* send procedure */
				   elemType,		/* element type ID */
				   InvalidOid,		/* base type ID (only for domains) */
				   defaultValue,	/* default type value */
				   NULL,			/* no binary form available */
				   byValue,			/* passed by value */
				   alignment,		/* required alignment */
				   storage,			/* TOAST strategy */
				   -1,				/* typMod (Domains only) */
				   0,				/* Array Dimensions of typbasetype */
				   false);			/* Type NOT NULL */

	/*
	 * When we create a base type (as opposed to a complex type) we need
	 * to have an array entry for it in pg_type as well.
	 */
	shadow_type = makeArrayTypeName(typeName);

	/* alignment must be 'i' or 'd' for arrays */
	alignment = (alignment == 'd') ? 'd' : 'i';

	TypeCreate(shadow_type,		/* type name */
			   typeNamespace,	/* namespace */
			   InvalidOid,		/* preassigned type oid (not done here) */
			   InvalidOid,		/* relation oid (n/a here) */
			   -1,				/* internal size */
			   -1,				/* external size */
			   'b',				/* type-type (base type) */
			   DEFAULT_TYPDELIM,	/* array element delimiter */
			   F_ARRAY_IN,		/* input procedure */
			   F_ARRAY_OUT,		/* output procedure */
			   F_ARRAY_IN,		/* receive procedure */
			   F_ARRAY_OUT,		/* send procedure */
			   typoid,			/* element type ID */
			   InvalidOid,		/* base type ID */
			   NULL,			/* never a default type value */
			   NULL,			/* binary default isn't sent either */
			   false,			/* never passed by value */
			   alignment,		/* see above */
			   'x',				/* ARRAY is always toastable */
			   -1,				/* typMod (Domains only) */
			   0,				/* Array dimensions of typbasetype */
			   false);			/* Type NOT NULL */

	pfree(shadow_type);
}

static Oid
findTypeIOFunction(const char *procname, bool isOutput)
{
	Oid			argList[FUNC_MAX_ARGS];
	int			nargs;
	Oid			procOid;

	/*
	 * First look for a 1-argument func with all argtypes 0. This is
	 * valid for all kinds of procedure.
	 */
	MemSet(argList, 0, FUNC_MAX_ARGS * sizeof(Oid));

	procOid = GetSysCacheOid(PROCNAME,
							 PointerGetDatum(procname),
							 Int32GetDatum(1),
							 PointerGetDatum(argList),
							 0);

	if (!OidIsValid(procOid))
	{
		/*
		 * Alternatively, input procedures may take 3 args (data
		 * value, element OID, atttypmod); the pg_proc argtype
		 * signature is 0,OIDOID,INT4OID.  Output procedures may
		 * take 2 args (data value, element OID).
		 */
		if (isOutput)
		{
			/* output proc */
			nargs = 2;
			argList[1] = OIDOID;
		}
		else
		{
			/* input proc */
			nargs = 3;
			argList[1] = OIDOID;
			argList[2] = INT4OID;
		}
		procOid = GetSysCacheOid(PROCNAME,
								 PointerGetDatum(procname),
								 Int32GetDatum(nargs),
								 PointerGetDatum(argList),
								 0);

		if (!OidIsValid(procOid))
			func_error("TypeCreate", procname, 1, argList, NULL);
	}

	return procOid;
}


static char *
defGetString(DefElem *def)
{
	if (def->arg == NULL)
		elog(ERROR, "Define: \"%s\" requires a parameter",
			 def->defname);
	switch (nodeTag(def->arg))
	{
		case T_Integer:
			{
				char	   *str = palloc(32);

				snprintf(str, 32, "%ld", (long) intVal(def->arg));
				return str;
			}
		case T_Float:

			/*
			 * T_Float values are kept in string form, so this type cheat
			 * works (and doesn't risk losing precision)
			 */
			return strVal(def->arg);
		case T_String:
			return strVal(def->arg);
		case T_TypeName:
			return TypeNameToString((TypeName *) def->arg);
		default:
			elog(ERROR, "Define: cannot interpret argument of \"%s\"",
				 def->defname);
	}
	return NULL;				/* keep compiler quiet */
}

static double
defGetNumeric(DefElem *def)
{
	if (def->arg == NULL)
		elog(ERROR, "Define: \"%s\" requires a numeric value",
			 def->defname);
	switch (nodeTag(def->arg))
	{
		case T_Integer:
			return (double) intVal(def->arg);
		case T_Float:
			return floatVal(def->arg);
		default:
			elog(ERROR, "Define: \"%s\" requires a numeric value",
				 def->defname);
	}
	return 0;					/* keep compiler quiet */
}

static TypeName *
defGetTypeName(DefElem *def)
{
	if (def->arg == NULL)
		elog(ERROR, "Define: \"%s\" requires a parameter",
			 def->defname);
	switch (nodeTag(def->arg))
	{
		case T_TypeName:
			return (TypeName *) def->arg;
		case T_String:
		{
			/* Allow quoted typename for backwards compatibility */
			TypeName   *n = makeNode(TypeName);

			n->names = makeList1(def->arg);
			n->typmod = -1;
			return n;
		}
		default:
			elog(ERROR, "Define: argument of \"%s\" must be a type name",
				 def->defname);
	}
	return NULL;				/* keep compiler quiet */
}

static int
defGetTypeLength(DefElem *def)
{
	if (def->arg == NULL)
		elog(ERROR, "Define: \"%s\" requires a parameter",
			 def->defname);
	switch (nodeTag(def->arg))
	{
		case T_Integer:
			return intVal(def->arg);
		case T_Float:
			elog(ERROR, "Define: \"%s\" requires an integral value",
				 def->defname);
			break;
		case T_String:
			if (strcasecmp(strVal(def->arg), "variable") == 0)
				return -1;		/* variable length */
			break;
		case T_TypeName:
			/* cope if grammar chooses to believe "variable" is a typename */
			if (strcasecmp(TypeNameToString((TypeName *) def->arg),
						   "variable") == 0)
				return -1;		/* variable length */
			break;
		default:
			elog(ERROR, "Define: cannot interpret argument of \"%s\"",
				 def->defname);
	}
	elog(ERROR, "Define: invalid argument for \"%s\"",
		 def->defname);
	return 0;					/* keep compiler quiet */
}
