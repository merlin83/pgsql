/*-------------------------------------------------------------------------
 *
 * parse_coerce.c
 *		handle type coercions/conversions for parser
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/parse_coerce.c,v 2.70 2002-04-11 20:00:00 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/pg_proc.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_func.h"
#include "parser/parse_type.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


Oid			DemoteType(Oid inType);
Oid			PromoteTypeToNext(Oid inType);

static Oid	PreferredType(CATEGORY category, Oid type);
static Node *build_func_call(Oid funcid, Oid rettype, List *args);
static Oid	find_coercion_function(Oid targetTypeId, Oid inputTypeId,
								   Oid secondArgType, bool isExplicit);


/* coerce_type()
 * Convert a function argument to a different type.
 */
Node *
coerce_type(ParseState *pstate, Node *node, Oid inputTypeId,
			Oid targetTypeId, int32 atttypmod, bool isExplicit)
{
	Node	   *result;

	if (targetTypeId == inputTypeId ||
		targetTypeId == InvalidOid ||
		node == NULL)
	{
		/* no conversion needed */
		result = node;
	}
	else if (inputTypeId == UNKNOWNOID && IsA(node, Const))
	{
		/*
		 * Input is a string constant with previously undetermined type.
		 * Apply the target type's typinput function to it to produce a
		 * constant of the target type.
		 *
		 * NOTE: this case cannot be folded together with the other
		 * constant-input case, since the typinput function does not
		 * necessarily behave the same as a type conversion function. For
		 * example, int4's typinput function will reject "1.2", whereas
		 * float-to-int type conversion will round to integer.
		 *
		 * XXX if the typinput function is not cachable, we really ought to
		 * postpone evaluation of the function call until runtime. But
		 * there is no way to represent a typinput function call as an
		 * expression tree, because C-string values are not Datums.
		 */
		Const	   *con = (Const *) node;
		Const	   *newcon = makeNode(Const);
		Type		targetType = typeidType(targetTypeId);

		newcon->consttype = targetTypeId;
		newcon->constlen = typeLen(targetType);
		newcon->constbyval = typeByVal(targetType);
		newcon->constisnull = con->constisnull;
		newcon->constisset = false;

		if (!con->constisnull)
		{
			/* We know the source constant is really of type 'text' */
			char	   *val = DatumGetCString(DirectFunctionCall1(textout,
													   con->constvalue));

			newcon->constvalue = stringTypeDatum(targetType, val, atttypmod);
			pfree(val);
		}

		ReleaseSysCache(targetType);

		result = (Node *) newcon;
	}
	else if (IsBinaryCompatible(inputTypeId, targetTypeId))
	{
		/*
		 * We don't really need to do a conversion, but we do need to
		 * attach a RelabelType node so that the expression will be seen
		 * to have the intended type when inspected by higher-level code.
		 *
		 * XXX could we label result with exprTypmod(node) instead of
		 * default -1 typmod, to save a possible length-coercion later?
		 * Would work if both types have same interpretation of typmod,
		 * which is likely but not certain.
		 */
		result = (Node *) makeRelabelType(node, targetTypeId, -1);
	}
	else if (typeInheritsFrom(inputTypeId, targetTypeId))
	{
		/*
		 * Input class type is a subclass of target, so nothing to do
		 * --- except relabel the type.  This is binary compatibility
		 * for complex types.
		 */
		result = (Node *) makeRelabelType(node, targetTypeId, -1);
	}
	else
	{
		/*
		 * Otherwise, find the appropriate type conversion function
		 * (caller should have determined that there is one), and generate
		 * an expression tree representing run-time application of the
		 * conversion function.
		 *
		 * For domains, we use the coercion function for the base type.
		 */
		Oid			baseTypeId = getBaseType(targetTypeId);
		Oid			funcId;

		funcId = find_coercion_function(baseTypeId,
										getBaseType(inputTypeId),
										InvalidOid,
										isExplicit);
		if (!OidIsValid(funcId))
			elog(ERROR, "coerce_type: no conversion function from %s to %s",
				 format_type_be(inputTypeId), format_type_be(targetTypeId));

		result = build_func_call(funcId, baseTypeId, makeList1(node));

		/* if domain, relabel with domain type ID */
		if (targetTypeId != baseTypeId)
			result = (Node *) makeRelabelType(result, targetTypeId, -1);

		/*
		 * If the input is a constant, apply the type conversion function
		 * now instead of delaying to runtime.	(We could, of course, just
		 * leave this to be done during planning/optimization; but it's a
		 * very frequent special case, and we save cycles in the rewriter
		 * if we fold the expression now.)
		 *
		 * Note that no folding will occur if the conversion function is not
		 * marked 'iscachable'.
		 *
		 * HACK: if constant is NULL, don't fold it here.  This is needed by
		 * make_subplan(), which calls this routine on placeholder Const
		 * nodes that mustn't be collapsed.  (It'd be a lot cleaner to
		 * make a separate node type for that purpose...)
		 */
		if (IsA(node, Const) &&
			!((Const *) node)->constisnull)
			result = eval_const_expressions(result);
	}

	return result;
}


/* can_coerce_type()
 * Can input_typeids be coerced to func_typeids?
 *
 * There are a few types which are known apriori to be convertible.
 * We will check for those cases first, and then look for possible
 * conversion functions.
 *
 * We must be told whether this is an implicit or explicit coercion
 * (explicit being a CAST construct, explicit function call, etc).
 * We will accept a wider set of coercion cases for an explicit coercion.
 *
 * Notes:
 * This uses the same mechanism as the CAST() SQL construct in gram.y.
 */
bool
can_coerce_type(int nargs, Oid *input_typeids, Oid *func_typeids,
				bool isExplicit)
{
	int			i;

	/* run through argument list... */
	for (i = 0; i < nargs; i++)
	{
		Oid			inputTypeId = input_typeids[i];
		Oid			targetTypeId = func_typeids[i];
		Oid			funcId;

		/* no problem if same type */
		if (inputTypeId == targetTypeId)
			continue;

		/*
		 * one of the known-good transparent conversions? then drop
		 * through...
		 */
		if (IsBinaryCompatible(inputTypeId, targetTypeId))
			continue;

		/* don't know what to do for the output type? then quit... */
		if (targetTypeId == InvalidOid)
			return false;
		/* don't know what to do for the input type? then quit... */
		if (inputTypeId == InvalidOid)
			return false;

		/*
		 * If input is an untyped string constant, assume we can convert
		 * it to anything except a class type.
		 */
		if (inputTypeId == UNKNOWNOID)
		{
			if (ISCOMPLEX(targetTypeId))
				return false;
			continue;
		}

		/*
		 * If input is a class type that inherits from target, no problem
		 */
		if (typeInheritsFrom(inputTypeId, targetTypeId))
			continue;

		/* don't choke on references to no-longer-existing types */
		if (!typeidIsValid(inputTypeId))
			return false;
		if (!typeidIsValid(targetTypeId))
			return false;

		/*
		 * Else, try for run-time conversion using functions: look for a
		 * single-argument function named with the target type name and
		 * accepting the source type.
		 *
		 * If either type is a domain, use its base type instead.
		 */
		funcId = find_coercion_function(getBaseType(targetTypeId),
										getBaseType(inputTypeId),
										InvalidOid,
										isExplicit);
		if (!OidIsValid(funcId))
			return false;
	}

	return true;
}

/* coerce_type_typmod()
 * Force a value to a particular typmod, if meaningful and possible.
 *
 * This is applied to values that are going to be stored in a relation
 * (where we have an atttypmod for the column) as well as values being
 * explicitly CASTed (where the typmod comes from the target type spec).
 *
 * The caller must have already ensured that the value is of the correct
 * type, typically by applying coerce_type.
 *
 * If the target column type possesses a function named for the type
 * and having parameter signature (columntype, int4), we assume that
 * the type requires coercion to its own length and that the said
 * function should be invoked to do that.
 *
 * "bpchar" (ie, char(N)) and "numeric" are examples of such types.
 */
Node *
coerce_type_typmod(ParseState *pstate, Node *node,
				   Oid targetTypeId, int32 atttypmod)
{
	Oid			baseTypeId;
	Oid			funcId;

	/*
	 * A negative typmod is assumed to mean that no coercion is wanted.
	 */
	if (atttypmod < 0 || atttypmod == exprTypmod(node))
		return node;

	/* If given type is a domain, use base type instead */
	baseTypeId = getBaseType(targetTypeId);

	/* Note this is always implicit coercion */
	funcId = find_coercion_function(baseTypeId, baseTypeId, INT4OID, false);

	if (OidIsValid(funcId))
	{
		Const	   *cons;

		cons = makeConst(INT4OID,
						 sizeof(int32),
						 Int32GetDatum(atttypmod),
						 false,
						 true,
						 false,
						 false);

		node = build_func_call(funcId, baseTypeId, makeList2(node, cons));

		/* relabel if it's domain case */
		if (targetTypeId != baseTypeId)
			node = (Node *) makeRelabelType(node, targetTypeId, atttypmod);
	}

	return node;
}


/* coerce_to_boolean()
 *		Coerce an argument of a construct that requires boolean input
 *		(AND, OR, NOT, etc).
 *
 * If successful, update *pnode to be the transformed argument (if any
 * transformation is needed), and return TRUE.	If fail, return FALSE.
 * (The caller must check for FALSE and emit a suitable error message.)
 */
bool
coerce_to_boolean(ParseState *pstate, Node **pnode)
{
	Oid			inputTypeId = exprType(*pnode);
	Oid			targetTypeId;

	if (inputTypeId == BOOLOID)
		return true;			/* no work */
	targetTypeId = BOOLOID;
	if (!can_coerce_type(1, &inputTypeId, &targetTypeId, false))
		return false;			/* fail, but let caller choose error msg */
	*pnode = coerce_type(pstate, *pnode, inputTypeId, targetTypeId, -1,
						 false);
	return true;
}


/* select_common_type()
 *		Determine the common supertype of a list of input expression types.
 *		This is used for determining the output type of CASE and UNION
 *		constructs.
 *
 * typeids is a nonempty integer list of type OIDs.  Note that earlier items
 * in the list will be preferred if there is doubt.
 * 'context' is a phrase to use in the error message if we fail to select
 * a usable type.
 *
 * XXX this code is WRONG, since (for example) given the input (int4,int8)
 * it will select int4, whereas according to SQL92 clause 9.3 the correct
 * answer is clearly int8.	To fix this we need a notion of a promotion
 * hierarchy within type categories --- something more complete than
 * just a single preferred type.
 */
Oid
select_common_type(List *typeids, const char *context)
{
	Oid			ptype;
	CATEGORY	pcategory;
	List	   *l;

	Assert(typeids != NIL);
	ptype = (Oid) lfirsti(typeids);
	pcategory = TypeCategory(ptype);
	foreach(l, lnext(typeids))
	{
		Oid			ntype = (Oid) lfirsti(l);

		/* move on to next one if no new information... */
		if ((ntype != InvalidOid) && (ntype != UNKNOWNOID) && (ntype != ptype))
		{
			if ((ptype == InvalidOid) || ptype == UNKNOWNOID)
			{
				/* so far, only nulls so take anything... */
				ptype = ntype;
				pcategory = TypeCategory(ptype);
			}
			else if (TypeCategory(ntype) != pcategory)
			{
				/*
				 * both types in different categories? then not much
				 * hope...
				 */
				elog(ERROR, "%s types \"%s\" and \"%s\" not matched",
				  context, typeidTypeName(ptype), typeidTypeName(ntype));
			}
			else if (IsPreferredType(pcategory, ntype)
					 && !IsPreferredType(pcategory, ptype)
					 && can_coerce_type(1, &ptype, &ntype, false))
			{
				/*
				 * new one is preferred and can convert? then take it...
				 */
				ptype = ntype;
				pcategory = TypeCategory(ptype);
			}
		}
	}

	/*
	 * If all the inputs were UNKNOWN type --- ie, unknown-type literals
	 * --- then resolve as type TEXT.  This situation comes up with
	 * constructs like SELECT (CASE WHEN foo THEN 'bar' ELSE 'baz' END);
	 * SELECT 'foo' UNION SELECT 'bar'; It might seem desirable to leave
	 * the construct's output type as UNKNOWN, but that really doesn't
	 * work, because we'd probably end up needing a runtime coercion from
	 * UNKNOWN to something else, and we usually won't have it.  We need
	 * to coerce the unknown literals while they are still literals, so a
	 * decision has to be made now.
	 */
	if (ptype == UNKNOWNOID)
		ptype = TEXTOID;

	return ptype;
}

/* coerce_to_common_type()
 *		Coerce an expression to the given type.
 *
 * This is used following select_common_type() to coerce the individual
 * expressions to the desired type.  'context' is a phrase to use in the
 * error message if we fail to coerce.
 *
 * NOTE: pstate may be NULL.
 */
Node *
coerce_to_common_type(ParseState *pstate, Node *node,
					  Oid targetTypeId,
					  const char *context)
{
	Oid			inputTypeId = exprType(node);

	if (inputTypeId == targetTypeId)
		return node;			/* no work */
	if (can_coerce_type(1, &inputTypeId, &targetTypeId, false))
		node = coerce_type(pstate, node, inputTypeId, targetTypeId, -1,
						   false);
	else
	{
		elog(ERROR, "%s unable to convert to type \"%s\"",
			 context, typeidTypeName(targetTypeId));
	}
	return node;
}


/* TypeCategory()
 * Assign a category to the specified OID.
 * XXX This should be moved to system catalog lookups
 * to allow for better type extensibility.
 * - thomas 2001-09-30
 */
CATEGORY
TypeCategory(Oid inType)
{
	CATEGORY	result;

	switch (inType)
	{
		case (BOOLOID):
			result = BOOLEAN_TYPE;
			break;

		case (CHAROID):
		case (NAMEOID):
		case (BPCHAROID):
		case (VARCHAROID):
		case (TEXTOID):
			result = STRING_TYPE;
			break;

		case (BITOID):
		case (VARBITOID):
			result = BITSTRING_TYPE;
			break;

		case (OIDOID):
		case (REGPROCOID):
		case (INT2OID):
		case (INT4OID):
		case (INT8OID):
		case (FLOAT4OID):
		case (FLOAT8OID):
		case (NUMERICOID):
		case (CASHOID):
			result = NUMERIC_TYPE;
			break;

		case (DATEOID):
		case (TIMEOID):
		case (TIMETZOID):
		case (ABSTIMEOID):
		case (TIMESTAMPOID):
		case (TIMESTAMPTZOID):
			result = DATETIME_TYPE;
			break;

		case (RELTIMEOID):
		case (TINTERVALOID):
		case (INTERVALOID):
			result = TIMESPAN_TYPE;
			break;

		case (POINTOID):
		case (LSEGOID):
		case (PATHOID):
		case (BOXOID):
		case (POLYGONOID):
		case (LINEOID):
		case (CIRCLEOID):
			result = GEOMETRIC_TYPE;
			break;

		case (INETOID):
		case (CIDROID):
			result = NETWORK_TYPE;
			break;

		case (UNKNOWNOID):
		case (InvalidOid):
			result = UNKNOWN_TYPE;
			break;

		default:
			result = USER_TYPE;
			break;
	}
	return result;
}	/* TypeCategory() */


/* IsBinaryCompatible()
 *		Check if two types are binary-compatible.
 *
 * This notion allows us to cheat and directly exchange values without
 * going through the trouble of calling a conversion function.
 *
 * XXX This should be moved to system catalog lookups
 * to allow for better type extensibility.
 */

/*
 * This macro describes hard-coded knowledge of binary compatibility
 * for built-in types.
 */
#define IS_BINARY_COMPATIBLE(a,b) \
		  (((a) == BPCHAROID && (b) == TEXTOID) \
		|| ((a) == BPCHAROID && (b) == VARCHAROID) \
		|| ((a) == VARCHAROID && (b) == TEXTOID) \
		|| ((a) == VARCHAROID && (b) == BPCHAROID) \
		|| ((a) == TEXTOID && (b) == BPCHAROID) \
		|| ((a) == TEXTOID && (b) == VARCHAROID) \
		|| ((a) == OIDOID && (b) == INT4OID) \
		|| ((a) == OIDOID && (b) == REGPROCOID) \
		|| ((a) == INT4OID && (b) == OIDOID) \
		|| ((a) == INT4OID && (b) == REGPROCOID) \
		|| ((a) == REGPROCOID && (b) == OIDOID) \
		|| ((a) == REGPROCOID && (b) == INT4OID) \
		|| ((a) == ABSTIMEOID && (b) == INT4OID) \
		|| ((a) == INT4OID && (b) == ABSTIMEOID) \
		|| ((a) == RELTIMEOID && (b) == INT4OID) \
		|| ((a) == INT4OID && (b) == RELTIMEOID) \
		|| ((a) == INETOID && (b) == CIDROID) \
		|| ((a) == CIDROID && (b) == INETOID) \
		|| ((a) == BITOID && (b) == VARBITOID) \
		|| ((a) == VARBITOID && (b) == BITOID))

bool
IsBinaryCompatible(Oid type1, Oid type2)
{
	if (type1 == type2)
		return true;
	if (IS_BINARY_COMPATIBLE(type1, type2))
		return true;
	/*
	 * Perhaps the types are domains; if so, look at their base types
	 */
	if (OidIsValid(type1))
		type1 = getBaseType(type1);
	if (OidIsValid(type2))
		type2 = getBaseType(type2);
	if (type1 == type2)
		return true;
	if (IS_BINARY_COMPATIBLE(type1, type2))
		return true;
	return false;
}


/* IsPreferredType()
 * Check if this type is a preferred type.
 * XXX This should be moved to system catalog lookups
 * to allow for better type extensibility.
 * - thomas 2001-09-30
 */
bool
IsPreferredType(CATEGORY category, Oid type)
{
	return (type == PreferredType(category, type));
}	/* IsPreferredType() */


/* PreferredType()
 * Return the preferred type OID for the specified category.
 * XXX This should be moved to system catalog lookups
 * to allow for better type extensibility.
 * - thomas 2001-09-30
 */
static Oid
PreferredType(CATEGORY category, Oid type)
{
	Oid			result;

	switch (category)
	{
		case (BOOLEAN_TYPE):
			result = BOOLOID;
			break;

		case (STRING_TYPE):
			result = TEXTOID;
			break;

		case (BITSTRING_TYPE):
			result = VARBITOID;
			break;

		case (NUMERIC_TYPE):
			if (type == OIDOID)
				result = OIDOID;
			else if (type == NUMERICOID)
				result = NUMERICOID;
			else
				result = FLOAT8OID;
			break;

		case (DATETIME_TYPE):
			if (type == DATEOID)
				result = TIMESTAMPOID;
			else
				result = TIMESTAMPTZOID;
			break;

		case (TIMESPAN_TYPE):
			result = INTERVALOID;
			break;

		case (NETWORK_TYPE):
			result = INETOID;
			break;

		case (GEOMETRIC_TYPE):
		case (USER_TYPE):
			result = type;
			break;

		default:
			result = UNKNOWNOID;
			break;
	}
	return result;
}	/* PreferredType() */

/*
 * find_coercion_function
 *		Look for a coercion function between two types.
 *
 * A coercion function must be named after (the internal name of) its
 * result type, and must accept exactly the specified input type.  We
 * also require it to be defined in the same namespace as its result type.
 * Furthermore, unless we are doing explicit coercion the function must
 * be marked as usable for implicit coercion --- this allows coercion
 * functions to be provided that aren't implicitly invokable.
 *
 * This routine is also used to look for length-coercion functions, which
 * are similar but accept a second argument.  secondArgType is the type
 * of the second argument (normally INT4OID), or InvalidOid if we are
 * looking for a regular coercion function.
 *
 * If a function is found, return its pg_proc OID; else return InvalidOid.
 */
static Oid
find_coercion_function(Oid targetTypeId, Oid inputTypeId, Oid secondArgType,
					   bool isExplicit)
{
	Oid			funcid = InvalidOid;
	Type		targetType;
	char	   *typname;
	Oid			typnamespace;
	Oid			oid_array[FUNC_MAX_ARGS];
	int			nargs;
	HeapTuple	ftup;

	targetType = typeidType(targetTypeId);
	typname = NameStr(((Form_pg_type) GETSTRUCT(targetType))->typname);
	typnamespace = ((Form_pg_type) GETSTRUCT(targetType))->typnamespace;

	MemSet(oid_array, 0, FUNC_MAX_ARGS * sizeof(Oid));
	oid_array[0] = inputTypeId;
	if (OidIsValid(secondArgType))
	{
		oid_array[1] = secondArgType;
		nargs = 2;
	}
	else
		nargs = 1;

	ftup = SearchSysCache(PROCNAMENSP,
						  CStringGetDatum(typname),
						  Int16GetDatum(nargs),
						  PointerGetDatum(oid_array),
						  ObjectIdGetDatum(typnamespace));
	if (HeapTupleIsValid(ftup))
	{
		Form_pg_proc pform = (Form_pg_proc) GETSTRUCT(ftup);

		/* Make sure the function's result type is as expected */
		if (pform->prorettype == targetTypeId && !pform->proretset &&
			!pform->proisagg)
		{
			/* If needed, make sure it can be invoked implicitly */
			if (isExplicit || pform->proimplicit)
			{
				/* Okay to use it */
				funcid = ftup->t_data->t_oid;
			}
		}
		ReleaseSysCache(ftup);
	}

	ReleaseSysCache(targetType);
	return funcid;
}

/*
 * Build an expression tree representing a function call.
 *
 * The argument expressions must have been transformed already.
 */
static Node *
build_func_call(Oid funcid, Oid rettype, List *args)
{
	Func	   *funcnode;
	Expr	   *expr;

	funcnode = makeNode(Func);
	funcnode->funcid = funcid;
	funcnode->functype = rettype;
	funcnode->func_fcache = NULL;

	expr = makeNode(Expr);
	expr->typeOid = rettype;
	expr->opType = FUNC_EXPR;
	expr->oper = (Node *) funcnode;
	expr->args = args;

	return (Node *) expr;
}
