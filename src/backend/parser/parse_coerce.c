/*-------------------------------------------------------------------------
 *
 * parse_coerce.c
 *		handle type coercions/conversions for parser
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/parse_coerce.c,v 2.86 2002-11-15 02:50:09 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_cast.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_proc.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_func.h"
#include "parser/parse_type.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


static Node *coerce_type_typmod(Node *node,
								Oid targetTypeId, int32 targetTypMod,
								CoercionForm cformat);
static Oid	PreferredType(CATEGORY category, Oid type);
static Node *build_func_call(Oid funcid, Oid rettype, List *args,
							 CoercionForm fformat);


/*
 * coerce_to_target_type()
 *		Convert an expression to a target type and typmod.
 *
 * This is the general-purpose entry point for arbitrary type coercion
 * operations.  Direct use of the component operations can_coerce_type,
 * coerce_type, and coerce_type_typmod should be restricted to special
 * cases (eg, when the conversion is expected to succeed).
 *
 * Returns the possibly-transformed expression tree, or NULL if the type
 * conversion is not possible.  (We do this, rather than elog'ing directly,
 * so that callers can generate custom error messages indicating context.)
 *
 * expr - input expression tree (already transformed by transformExpr)
 * exprtype - result type of expr
 * targettype - desired result type
 * targettypmod - desired result typmod
 * ccontext, cformat - context indicators to control coercions
 */
Node *
coerce_to_target_type(Node *expr, Oid exprtype,
					  Oid targettype, int32 targettypmod,
					  CoercionContext ccontext,
					  CoercionForm cformat)
{
	if (can_coerce_type(1, &exprtype, &targettype, ccontext))
		expr = coerce_type(expr, exprtype, targettype,
						   ccontext, cformat);
	/*
	 * String hacks to get transparent conversions for char and varchar:
	 * if a coercion to text is available, use it for forced coercions to
	 * char(n) or varchar(n).
	 *
	 * This is pretty grotty, but seems easier to maintain than providing
	 * entries in pg_cast that parallel all the ones for text.
	 */
	else if (ccontext >= COERCION_ASSIGNMENT &&
			 (targettype == BPCHAROID || targettype == VARCHAROID))
	{
		Oid			text_id = TEXTOID;

		if (can_coerce_type(1, &exprtype, &text_id, ccontext))
		{
			expr = coerce_type(expr, exprtype, text_id,
							   ccontext, cformat);
			/* Need a RelabelType if no typmod coercion is performed */
			if (targettypmod < 0)
				expr = (Node *) makeRelabelType(expr, targettype, -1,
												cformat);
		}
		else
			expr = NULL;
	}
	else
		expr = NULL;

	/*
	 * If the target is a fixed-length type, it may need a length coercion
	 * as well as a type coercion.
	 */
	if (expr != NULL)
		expr = coerce_type_typmod(expr, targettype, targettypmod, cformat);

	return expr;
}


/*
 * coerce_type()
 *		Convert an expression to a different type.
 *
 * The caller should already have determined that the coercion is possible;
 * see can_coerce_type.
 *
 * No coercion to a typmod (length) is performed here.  The caller must
 * call coerce_type_typmod as well, if a typmod constraint is wanted.
 * (But if the target type is a domain, it may internally contain a
 * typmod constraint, which will be applied inside coerce_type_constraints.)
 */
Node *
coerce_type(Node *node, Oid inputTypeId, Oid targetTypeId,
			CoercionContext ccontext, CoercionForm cformat)
{
	Node	   *result;
	Oid			funcId;

	if (targetTypeId == inputTypeId ||
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
		 * XXX if the typinput function is not immutable, we really ought to
		 * postpone evaluation of the function call until runtime. But
		 * there is no way to represent a typinput function call as an
		 * expression tree, because C-string values are not Datums. (XXX
		 * This *is* possible as of 7.3, do we want to do it?)
		 */
		Const	   *con = (Const *) node;
		Const	   *newcon = makeNode(Const);
		Type		targetType = typeidType(targetTypeId);
		char		targetTyptype = typeTypType(targetType);

		newcon->consttype = targetTypeId;
		newcon->constlen = typeLen(targetType);
		newcon->constbyval = typeByVal(targetType);
		newcon->constisnull = con->constisnull;
		newcon->constisset = false;

		if (!con->constisnull)
		{
			char	   *val = DatumGetCString(DirectFunctionCall1(unknownout,
													   con->constvalue));

			/*
			 * We pass typmod -1 to the input routine, primarily because
			 * existing input routines follow implicit-coercion semantics
			 * for length checks, which is not always what we want here.
			 * Any length constraint will be applied later by our caller.
			 *
			 * Note that we call stringTypeDatum using the domain's pg_type
			 * row, if it's a domain.  This works because the domain row has
			 * the same typinput and typelem as the base type --- ugly...
			 */
			newcon->constvalue = stringTypeDatum(targetType, val, -1);
			pfree(val);
		}

		result = (Node *) newcon;

		/* If target is a domain, apply constraints. */
		if (targetTyptype == 'd')
		{
			result = coerce_type_constraints(result, targetTypeId,
											 cformat);
			/* We might now need a RelabelType. */
			if (exprType(result) != targetTypeId)
				result = (Node *) makeRelabelType(result, targetTypeId, -1,
												  cformat);
		}

		ReleaseSysCache(targetType);
	}
	else if (targetTypeId == ANYOID ||
			 targetTypeId == ANYARRAYOID)
	{
		/* assume can_coerce_type verified that implicit coercion is okay */
		/* NB: we do NOT want a RelabelType here */
		result = node;
	}
	else if (find_coercion_pathway(targetTypeId, inputTypeId, ccontext,
								   &funcId))
	{
		if (OidIsValid(funcId))
		{
			/*
			 * Generate an expression tree representing run-time
			 * application of the conversion function.	If we are dealing
			 * with a domain target type, the conversion function will
			 * yield the base type.
			 */
			Oid			baseTypeId = getBaseType(targetTypeId);

			result = build_func_call(funcId, baseTypeId, makeList1(node),
									 cformat);

			/*
			 * If domain, test against domain constraints and relabel with
			 * domain type ID
			 */
			if (targetTypeId != baseTypeId)
			{
				result = coerce_type_constraints(result, targetTypeId,
												 cformat);
				result = (Node *) makeRelabelType(result, targetTypeId, -1,
												  cformat);
			}

			/*
			 * If the input is a constant, apply the type conversion
			 * function now instead of delaying to runtime.  (We could, of
			 * course, just leave this to be done during
			 * planning/optimization; but it's a very frequent special
			 * case, and we save cycles in the rewriter if we fold the
			 * expression now.)
			 *
			 * Note that no folding will occur if the conversion function is
			 * not marked 'immutable'.
			 *
			 * HACK: if constant is NULL, don't fold it here.  This is needed
			 * by make_subplan(), which calls this routine on placeholder
			 * Const nodes that mustn't be collapsed.  (It'd be a lot
			 * cleaner to make a separate node type for that purpose...)
			 */
			if (IsA(node, Const) &&
				!((Const *) node)->constisnull)
				result = eval_const_expressions(result);
		}
		else
		{
			/*
			 * We don't need to do a physical conversion, but we do need
			 * to attach a RelabelType node so that the expression will be
			 * seen to have the intended type when inspected by
			 * higher-level code.
			 *
			 * Also, domains may have value restrictions beyond the base type
			 * that must be accounted for.
			 */
			result = coerce_type_constraints(node, targetTypeId,
											 cformat);

			/*
			 * XXX could we label result with exprTypmod(node) instead of
			 * default -1 typmod, to save a possible length-coercion
			 * later? Would work if both types have same interpretation of
			 * typmod, which is likely but not certain (wrong if target is
			 * a domain, in any case).
			 */
			result = (Node *) makeRelabelType(result, targetTypeId, -1,
											  cformat);
		}
	}
	else if (typeInheritsFrom(inputTypeId, targetTypeId))
	{
		/*
		 * Input class type is a subclass of target, so nothing to do ---
		 * except relabel the type.  This is binary compatibility for
		 * complex types.
		 */
		result = (Node *) makeRelabelType(node, targetTypeId, -1,
										  cformat);
	}
	else
	{
		/* If we get here, caller blew it */
		elog(ERROR, "coerce_type: no conversion function from %s to %s",
			 format_type_be(inputTypeId), format_type_be(targetTypeId));
		result = NULL;			/* keep compiler quiet */
	}

	return result;
}


/*
 * can_coerce_type()
 *		Can input_typeids be coerced to target_typeids?
 *
 * We must be told the context (CAST construct, assignment, implicit coercion)
 * as this determines the set of available casts.
 */
bool
can_coerce_type(int nargs, Oid *input_typeids, Oid *target_typeids,
				CoercionContext ccontext)
{
	int			i;

	/* run through argument list... */
	for (i = 0; i < nargs; i++)
	{
		Oid			inputTypeId = input_typeids[i];
		Oid			targetTypeId = target_typeids[i];
		Oid			funcId;

		/* no problem if same type */
		if (inputTypeId == targetTypeId)
			continue;

		/* don't choke on references to no-longer-existing types */
		if (!typeidIsValid(inputTypeId))
			return false;
		if (!typeidIsValid(targetTypeId))
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

		/* accept if target is ANY */
		if (targetTypeId == ANYOID)
			continue;

		/*
		 * if target is ANYARRAY and source is a varlena array type,
		 * accept
		 */
		if (targetTypeId == ANYARRAYOID)
		{
			Oid			typOutput;
			Oid			typElem;
			bool		typIsVarlena;

			if (getTypeOutputInfo(inputTypeId, &typOutput, &typElem,
								  &typIsVarlena))
			{
				if (OidIsValid(typElem) && typIsVarlena)
					continue;
			}

			/*
			 * Otherwise reject; this assumes there are no explicit
			 * coercion paths to ANYARRAY.  If we don't reject then
			 * parse_coerce would have to repeat the above test.
			 */
			return false;
		}

		/*
		 * If pg_cast shows that we can coerce, accept.  This test now
		 * covers both binary-compatible and coercion-function cases.
		 */
		if (find_coercion_pathway(targetTypeId, inputTypeId, ccontext,
								  &funcId))
			continue;

		/*
		 * If input is a class type that inherits from target, accept
		 */
		if (typeInheritsFrom(inputTypeId, targetTypeId))
			continue;

		/*
		 * Else, cannot coerce at this argument position
		 */
		return false;
	}

	return true;
}


/*
 * Create an expression tree to enforce the constraints (if any)
 * that should be applied by the type.	Currently this is only
 * interesting for domain types.
 *
 * NOTE: result tree is not guaranteed to show the correct exprType() for
 * the domain; it may show the base type.  Caller must relabel if needed.
 */
Node *
coerce_type_constraints(Node *arg, Oid typeId, CoercionForm cformat)
{
	char	   *notNull = NULL;
	int32		typmod = -1;

	for (;;)
	{
		HeapTuple	tup;
		HeapTuple	conTup;
		Form_pg_type typTup;

		ScanKeyData key[1];
		int			nkeys = 0;
		SysScanDesc scan;
		Relation	conRel;
		
		tup = SearchSysCache(TYPEOID,
							 ObjectIdGetDatum(typeId),
							 0, 0, 0);
		if (!HeapTupleIsValid(tup))
			elog(ERROR, "coerce_type_constraints: failed to lookup type %u",
				 typeId);
		typTup = (Form_pg_type) GETSTRUCT(tup);

		/* Test for NOT NULL Constraint */
		if (typTup->typnotnull && notNull == NULL)
			notNull = pstrdup(NameStr(typTup->typname));

		/* Add CHECK Constraints to domains */
		conRel = heap_openr(ConstraintRelationName, RowShareLock);

		ScanKeyEntryInitialize(&key[nkeys++], 0x0,
							   Anum_pg_constraint_contypid, F_OIDEQ,
							   ObjectIdGetDatum(typeId));

		scan = systable_beginscan(conRel, ConstraintTypidIndex, true,
								  SnapshotNow, nkeys, key);

		while (HeapTupleIsValid(conTup = systable_getnext(scan)))
		{
			Datum	val;
			bool	isNull;
			ConstraintTest *r = makeNode(ConstraintTest);
			Form_pg_constraint	c = (Form_pg_constraint) GETSTRUCT(conTup);

			/* Not expecting conbin to be NULL, but we'll test for it anyway */
			val = fastgetattr(conTup,
							  Anum_pg_constraint_conbin,
							  conRel->rd_att, &isNull);

			if (isNull)
				elog(ERROR, "coerce_type_constraints: domain %s constraint %s has NULL conbin",
					 NameStr(typTup->typname), NameStr(c->conname));

			r->arg = arg;
			r->testtype = CONSTR_TEST_CHECK;
			r->name = NameStr(c->conname);
			r->domname = NameStr(typTup->typname);
			r->check_expr =	stringToNode(MemoryContextStrdup(CacheMemoryContext,
										 DatumGetCString(DirectFunctionCall1(textout,
																			 val))));

			arg = (Node *) r;
		}

		systable_endscan(scan);
		heap_close(conRel, RowShareLock);

		if (typTup->typtype != 'd')
		{
			/* Not a domain, so done */
			ReleaseSysCache(tup);
			break;
		}

		Assert(typmod < 0);

		typeId = typTup->typbasetype;
		typmod = typTup->typtypmod;
		ReleaseSysCache(tup);
	}

	/*
	 * If domain applies a typmod to its base type, do length coercion.
	 */
	if (typmod >= 0)
		arg = coerce_type_typmod(arg, typeId, typmod, cformat);

	/*
	 * Only need to add one NOT NULL check regardless of how many domains
	 * in the stack request it.  The topmost domain that requested it is
	 * used as the constraint name.
	 */
	if (notNull)
	{
		ConstraintTest *r = makeNode(ConstraintTest);

		r->arg = arg;
		r->testtype = CONSTR_TEST_NOTNULL;
		r->name = "NOT NULL";
		r->domname = notNull;
		r->check_expr = NULL;

		arg = (Node *) r;
	}

	return arg;
}


/*
 * coerce_type_typmod()
 *		Force a value to a particular typmod, if meaningful and possible.
 *
 * This is applied to values that are going to be stored in a relation
 * (where we have an atttypmod for the column) as well as values being
 * explicitly CASTed (where the typmod comes from the target type spec).
 *
 * The caller must have already ensured that the value is of the correct
 * type, typically by applying coerce_type.
 *
 * NOTE: this does not need to work on domain types, because any typmod
 * coercion for a domain is considered to be part of the type coercion
 * needed to produce the domain value in the first place.  So, no getBaseType.
 */
static Node *
coerce_type_typmod(Node *node, Oid targetTypeId, int32 targetTypMod,
				   CoercionForm cformat)
{
	Oid			funcId;
	int			nargs;

	/*
	 * A negative typmod is assumed to mean that no coercion is wanted.
	 */
	if (targetTypMod < 0 || targetTypMod == exprTypmod(node))
		return node;

	funcId = find_typmod_coercion_function(targetTypeId, &nargs);

	if (OidIsValid(funcId))
	{
		List	   *args;
		Const	   *cons;
		Node	   *fcall;

		/* Pass given value, plus target typmod as an int4 constant */
		cons = makeConst(INT4OID,
						 sizeof(int32),
						 Int32GetDatum(targetTypMod),
						 false,
						 true,
						 false,
						 false);

		args = makeList2(node, cons);

		if (nargs == 3)
		{
			/* Pass it a boolean isExplicit parameter, too */
			cons = makeConst(BOOLOID,
							 sizeof(bool),
							 BoolGetDatum(cformat != COERCE_IMPLICIT_CAST),
							 false,
							 true,
							 false,
							 false);

			args = lappend(args, cons);
		}

		fcall = build_func_call(funcId, targetTypeId, args, cformat);

		/*
		 * If the input is a constant, apply the length coercion
		 * function now instead of delaying to runtime.
		 *
		 * See the comments for the similar case in coerce_type.
		 */
		if (node && IsA(node, Const) &&
			!((Const *) node)->constisnull)
			node = eval_const_expressions(fcall);
		else
			node = fcall;
	}

	return node;
}


/* coerce_to_boolean()
 *		Coerce an argument of a construct that requires boolean input
 *		(AND, OR, NOT, etc).  Also check that input is not a set.
 *
 * Returns the possibly-transformed node tree.
 */
Node *
coerce_to_boolean(Node *node, const char *constructName)
{
	Oid			inputTypeId = exprType(node);

	if (inputTypeId != BOOLOID)
	{
		node = coerce_to_target_type(node, inputTypeId,
									 BOOLOID, -1,
									 COERCION_ASSIGNMENT,
									 COERCE_IMPLICIT_CAST);
		if (node == NULL)
		{
			/* translator: first %s is name of a SQL construct, eg WHERE */
			elog(ERROR, "Argument of %s must be type boolean, not type %s",
				 constructName, format_type_be(inputTypeId));
		}
	}

	if (expression_returns_set(node))
	{
		/* translator: %s is name of a SQL construct, eg WHERE */
		elog(ERROR, "Argument of %s must not be a set function",
			 constructName);
	}

	return node;
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
				elog(ERROR, "%s types '%s' and '%s' not matched",
				  context, format_type_be(ptype), format_type_be(ntype));
			}
			else if (!IsPreferredType(pcategory, ptype) &&
					 can_coerce_type(1, &ptype, &ntype, COERCION_IMPLICIT) &&
					 !can_coerce_type(1, &ntype, &ptype, COERCION_IMPLICIT))
			{
				/*
				 * take new type if can coerce to it implicitly but not the
				 * other way; but if we have a preferred type, stay on it.
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
 */
Node *
coerce_to_common_type(Node *node, Oid targetTypeId, const char *context)
{
	Oid			inputTypeId = exprType(node);

	if (inputTypeId == targetTypeId)
		return node;			/* no work */
	if (can_coerce_type(1, &inputTypeId, &targetTypeId, COERCION_IMPLICIT))
		node = coerce_type(node, inputTypeId, targetTypeId,
						   COERCION_IMPLICIT, COERCE_IMPLICIT_CAST);
	else
		elog(ERROR, "%s unable to convert to type %s",
			 context, format_type_be(targetTypeId));
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
		case (REGPROCEDUREOID):
		case (REGOPEROID):
		case (REGOPERATOROID):
		case (REGCLASSOID):
		case (REGTYPEOID):
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
			if (type == OIDOID ||
				type == REGPROCOID ||
				type == REGPROCEDUREOID ||
				type == REGOPEROID ||
				type == REGOPERATOROID ||
				type == REGCLASSOID ||
				type == REGTYPEOID)
				result = OIDOID;
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


/* IsBinaryCoercible()
 *		Check if srctype is binary-coercible to targettype.
 *
 * This notion allows us to cheat and directly exchange values without
 * going through the trouble of calling a conversion function.
 *
 * As of 7.3, binary coercibility isn't hardwired into the code anymore.
 * We consider two types binary-coercible if there is an implicitly
 * invokable, no-function-needed pg_cast entry.
 *
 * This function replaces IsBinaryCompatible(), which was an inherently
 * symmetric test.  Since the pg_cast entries aren't necessarily symmetric,
 * the order of the operands is now significant.
 */
bool
IsBinaryCoercible(Oid srctype, Oid targettype)
{
	HeapTuple	tuple;
	Form_pg_cast castForm;
	bool		result;

	/* Fast path if same type */
	if (srctype == targettype)
		return true;

	/* Perhaps the types are domains; if so, look at their base types */
	if (OidIsValid(srctype))
		srctype = getBaseType(srctype);
	if (OidIsValid(targettype))
		targettype = getBaseType(targettype);

	/* Somewhat-fast path if same base type */
	if (srctype == targettype)
		return true;

	/* Else look in pg_cast */
	tuple = SearchSysCache(CASTSOURCETARGET,
						   ObjectIdGetDatum(srctype),
						   ObjectIdGetDatum(targettype),
						   0, 0);
	if (!HeapTupleIsValid(tuple))
		return false;			/* no cast */
	castForm = (Form_pg_cast) GETSTRUCT(tuple);

	result = (castForm->castfunc == InvalidOid &&
			  castForm->castcontext == COERCION_CODE_IMPLICIT);

	ReleaseSysCache(tuple);

	return result;
}


/*
 * find_coercion_pathway
 *		Look for a coercion pathway between two types.
 *
 * ccontext determines the set of available casts.
 *
 * If we find a suitable entry in pg_cast, return TRUE, and set *funcid
 * to the castfunc value (which may be InvalidOid for a binary-compatible
 * coercion).
 */
bool
find_coercion_pathway(Oid targetTypeId, Oid sourceTypeId,
					  CoercionContext ccontext,
					  Oid *funcid)
{
	bool		result = false;
	HeapTuple	tuple;

	*funcid = InvalidOid;

	/* Perhaps the types are domains; if so, look at their base types */
	if (OidIsValid(sourceTypeId))
		sourceTypeId = getBaseType(sourceTypeId);
	if (OidIsValid(targetTypeId))
		targetTypeId = getBaseType(targetTypeId);

	/* Domains are automatically binary-compatible with their base type */
	if (sourceTypeId == targetTypeId)
		return true;

	/* Else look in pg_cast */
	tuple = SearchSysCache(CASTSOURCETARGET,
						   ObjectIdGetDatum(sourceTypeId),
						   ObjectIdGetDatum(targetTypeId),
						   0, 0);

	if (HeapTupleIsValid(tuple))
	{
		Form_pg_cast castForm = (Form_pg_cast) GETSTRUCT(tuple);
		CoercionContext castcontext;

		/* convert char value for castcontext to CoercionContext enum */
		switch (castForm->castcontext)
		{
			case COERCION_CODE_IMPLICIT:
				castcontext = COERCION_IMPLICIT;
				break;
			case COERCION_CODE_ASSIGNMENT:
				castcontext = COERCION_ASSIGNMENT;
				break;
			case COERCION_CODE_EXPLICIT:
				castcontext = COERCION_EXPLICIT;
				break;
			default:
				elog(ERROR, "find_coercion_pathway: bogus castcontext %c",
					 castForm->castcontext);
				castcontext = 0;	/* keep compiler quiet */
				break;
		}

		/* Rely on ordering of enum for correct behavior here */
		if (ccontext >= castcontext)
		{
			*funcid = castForm->castfunc;
			result = true;
		}

		ReleaseSysCache(tuple);
	}

	return result;
}


/*
 * find_typmod_coercion_function -- does the given type need length coercion?
 *
 * If the target type possesses a function named for the type
 * and having parameter signature (targettype, int4), we assume that
 * the type requires coercion to its own length and that the said
 * function should be invoked to do that.
 *
 * Alternatively, the length-coercing function may have the signature
 * (targettype, int4, bool).  On success, *nargs is set to report which
 * signature we found.
 *
 * "bpchar" (ie, char(N)) and "numeric" are examples of such types.
 *
 * If the given type is a varlena array type, we do not look for a coercion
 * function associated directly with the array type, but instead look for
 * one associated with the element type.  If one exists, we report
 * array_length_coerce() as the coercion function to use.
 *
 * This mechanism may seem pretty grotty and in need of replacement by
 * something in pg_cast, but since typmod is only interesting for datatypes
 * that have special handling in the grammar, there's not really much
 * percentage in making it any easier to apply such coercions ...
 */
Oid
find_typmod_coercion_function(Oid typeId, int *nargs)
{
	Oid			funcid = InvalidOid;
	bool		isArray = false;
	Type		targetType;
	Form_pg_type typeForm;
	char	   *typname;
	Oid			typnamespace;
	Oid			oid_array[FUNC_MAX_ARGS];
	HeapTuple	ftup;

	targetType = typeidType(typeId);
	typeForm = (Form_pg_type) GETSTRUCT(targetType);

	/* Check for a varlena array type (and not a domain) */
	if (typeForm->typelem != InvalidOid &&
		typeForm->typlen == -1 &&
		typeForm->typtype != 'd')
	{
		/* Yes, switch our attention to the element type */
		typeId = typeForm->typelem;
		ReleaseSysCache(targetType);
		targetType = typeidType(typeId);
		typeForm = (Form_pg_type) GETSTRUCT(targetType);
		isArray = true;
	}

	/* Function name is same as type internal name, and in same namespace */
	typname = NameStr(typeForm->typname);
	typnamespace = typeForm->typnamespace;

	/* First look for parameters (type, int4) */
	MemSet(oid_array, 0, FUNC_MAX_ARGS * sizeof(Oid));
	oid_array[0] = typeId;
	oid_array[1] = INT4OID;
	*nargs = 2;

	ftup = SearchSysCache(PROCNAMENSP,
						  CStringGetDatum(typname),
						  Int16GetDatum(2),
						  PointerGetDatum(oid_array),
						  ObjectIdGetDatum(typnamespace));
	if (HeapTupleIsValid(ftup))
	{
		Form_pg_proc pform = (Form_pg_proc) GETSTRUCT(ftup);

		/* Make sure the function's result type is as expected */
		if (pform->prorettype == typeId && !pform->proretset &&
			!pform->proisagg)
		{
			/* Okay to use it */
			funcid = HeapTupleGetOid(ftup);
		}
		ReleaseSysCache(ftup);
	}

	if (!OidIsValid(funcid))
	{
		/* Didn't find a function, so now try (type, int4, bool) */
		oid_array[2] = BOOLOID;
		*nargs = 3;

		ftup = SearchSysCache(PROCNAMENSP,
							  CStringGetDatum(typname),
							  Int16GetDatum(3),
							  PointerGetDatum(oid_array),
							  ObjectIdGetDatum(typnamespace));
		if (HeapTupleIsValid(ftup))
		{
			Form_pg_proc pform = (Form_pg_proc) GETSTRUCT(ftup);

			/* Make sure the function's result type is as expected */
			if (pform->prorettype == typeId && !pform->proretset &&
				!pform->proisagg)
			{
				/* Okay to use it */
				funcid = HeapTupleGetOid(ftup);
			}
			ReleaseSysCache(ftup);
		}
	}

	ReleaseSysCache(targetType);

	/*
	 * Now, if we did find a coercion function for an array element type,
	 * report array_length_coerce() as the function to use.  We know it
	 * takes three arguments always.
	 */
	if (isArray && OidIsValid(funcid))
	{
		funcid = F_ARRAY_LENGTH_COERCE;
		*nargs = 3;
	}

	return funcid;
}

/*
 * Build an expression tree representing a function call.
 *
 * The argument expressions must have been transformed already.
 */
static Node *
build_func_call(Oid funcid, Oid rettype, List *args, CoercionForm fformat)
{
	Func	   *funcnode;
	Expr	   *expr;

	funcnode = makeNode(Func);
	funcnode->funcid = funcid;
	funcnode->funcresulttype = rettype;
	funcnode->funcretset = false;		/* only possible case here */
	funcnode->funcformat = fformat;
	funcnode->func_fcache = NULL;

	expr = makeNode(Expr);
	expr->typeOid = rettype;
	expr->opType = FUNC_EXPR;
	expr->oper = (Node *) funcnode;
	expr->args = args;

	return (Node *) expr;
}
