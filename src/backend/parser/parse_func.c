/*-------------------------------------------------------------------------
 *
 * parse_func.c
 *		handle function calls in parser
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/parse_func.c,v 1.137 2002-09-18 21:35:22 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/namespace.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_proc.h"
#include "lib/stringinfo.h"
#include "nodes/makefuncs.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_func.h"
#include "parser/parse_relation.h"
#include "parser/parse_type.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


static Node *ParseComplexProjection(char *funcname, Node *first_arg);
static Oid **argtype_inherit(int nargs, Oid *argtypes);

static int	find_inheritors(Oid relid, Oid **supervec);
static Oid **gen_cross_product(InhPaths *arginh, int nargs);
static void make_arguments(int nargs,
			   List *fargs,
			   Oid *input_typeids,
			   Oid *function_typeids);
static int match_argtypes(int nargs,
			   Oid *input_typeids,
			   FuncCandidateList function_typeids,
			   FuncCandidateList *candidates);
static FieldSelect *setup_field_select(Node *input, char *attname, Oid relid);
static FuncCandidateList func_select_candidate(int nargs, Oid *input_typeids,
					  FuncCandidateList candidates);
static void unknown_attribute(const char *schemaname, const char *relname,
				  const char *attname);


/*
 *	Parse a function call
 *
 *	For historical reasons, Postgres tries to treat the notations tab.col
 *	and col(tab) as equivalent: if a single-argument function call has an
 *	argument of complex type and the (unqualified) function name matches
 *	any attribute of the type, we take it as a column projection.
 *
 *	Hence, both cases come through here.  The is_column parameter tells us
 *	which syntactic construct is actually being dealt with, but this is
 *	intended to be used only to deliver an appropriate error message,
 *	not to affect the semantics.  When is_column is true, we should have
 *	a single argument (the putative table), unqualified function name
 *	equal to the column name, and no aggregate decoration.
 *
 *	In the function-call case, the argument expressions have been transformed
 *	already.  In the column case, we may get either a transformed expression
 *	or a RangeVar node as argument.
 */
Node *
ParseFuncOrColumn(ParseState *pstate, List *funcname, List *fargs,
				  bool agg_star, bool agg_distinct, bool is_column)
{
	Oid			rettype;
	Oid			funcid;
	List	   *i;
	Node	   *first_arg = NULL;
	int			nargs = length(fargs);
	int			argn;
	Oid			oid_array[FUNC_MAX_ARGS];
	Oid		   *true_oid_array;
	Node	   *retval;
	bool		retset;
	FuncDetailCode fdresult;

	/*
	 * Most of the rest of the parser just assumes that functions do not
	 * have more than FUNC_MAX_ARGS parameters.  We have to test here to
	 * protect against array overruns, etc.  Of course, this may not be a
	 * function, but the test doesn't hurt.
	 */
	if (nargs > FUNC_MAX_ARGS)
		elog(ERROR, "Cannot pass more than %d arguments to a function",
			 FUNC_MAX_ARGS);

	if (fargs)
	{
		first_arg = lfirst(fargs);
		if (first_arg == NULL)	/* should not happen */
			elog(ERROR, "Function '%s' does not allow NULL input",
				 NameListToString(funcname));
	}

	/*
	 * check for column projection: if function has one argument, and that
	 * argument is of complex type, and function name is not qualified,
	 * then the "function call" could be a projection.	We also check that
	 * there wasn't any aggregate decoration.
	 */
	if (nargs == 1 && !agg_star && !agg_distinct && length(funcname) == 1)
	{
		char	   *cname = strVal(lfirst(funcname));

		/* Is it a not-yet-transformed RangeVar node? */
		if (IsA(first_arg, RangeVar))
		{
			/* First arg is a relation. This could be a projection. */
			retval = qualifiedNameToVar(pstate,
									((RangeVar *) first_arg)->schemaname,
										((RangeVar *) first_arg)->relname,
										cname,
										true);
			if (retval)
				return retval;
		}
		else if (ISCOMPLEX(exprType(first_arg)))
		{
			/*
			 * Attempt to handle projection of a complex argument. If
			 * ParseComplexProjection can't handle the projection, we have
			 * to keep going.
			 */
			retval = ParseComplexProjection(cname, first_arg);
			if (retval)
				return retval;
		}
	}

	/*
	 * Okay, it's not a column projection, so it must really be a
	 * function. Extract arg type info and transform RangeVar arguments
	 * into varnodes of the appropriate form.
	 */
	MemSet(oid_array, 0, FUNC_MAX_ARGS * sizeof(Oid));

	argn = 0;
	foreach(i, fargs)
	{
		Node	   *arg = lfirst(i);
		Oid			toid;

		if (IsA(arg, RangeVar))
		{
			char	   *schemaname;
			char	   *relname;
			RangeTblEntry *rte;
			int			vnum;
			int			sublevels_up;

			/*
			 * a relation: look it up in the range table, or add if needed
			 */
			schemaname = ((RangeVar *) arg)->schemaname;
			relname = ((RangeVar *) arg)->relname;

			rte = refnameRangeTblEntry(pstate, schemaname, relname,
									   &sublevels_up);

			if (rte == NULL)
				rte = addImplicitRTE(pstate, (RangeVar *) arg);

			vnum = RTERangeTablePosn(pstate, rte, &sublevels_up);

			/*
			 * The parameter to be passed to the function is the whole
			 * tuple from the relation.  We build a special VarNode to
			 * reflect this -- it has varno set to the correct range table
			 * entry, but has varattno == 0 to signal that the whole tuple
			 * is the argument.  Also, it has typmod set to
			 * sizeof(Pointer) to signal that the runtime representation
			 * will be a pointer not an Oid.
			 */
			switch (rte->rtekind)
			{
				case RTE_RELATION:
					toid = get_rel_type_id(rte->relid);
					if (!OidIsValid(toid))
						elog(ERROR, "Cannot find type OID for relation %u",
							 rte->relid);
					break;
				case RTE_FUNCTION:
					toid = exprType(rte->funcexpr);
					break;
				default:

					/*
					 * RTE is a join or subselect; must fail for lack of a
					 * named tuple type
					 */
					if (is_column)
						unknown_attribute(schemaname, relname,
										  strVal(lfirst(funcname)));
					else
						elog(ERROR, "Cannot pass result of sub-select or join %s to a function",
							 relname);
					toid = InvalidOid;	/* keep compiler quiet */
					break;
			}

			/* replace RangeVar in the arg list */
			lfirst(i) = makeVar(vnum,
								InvalidAttrNumber,
								toid,
								sizeof(Pointer),
								sublevels_up);
		}
		else
			toid = exprType(arg);

		oid_array[argn++] = toid;
	}

	/*
	 * func_get_detail looks up the function in the catalogs, does
	 * disambiguation for polymorphic functions, handles inheritance, and
	 * returns the funcid and type and set or singleton status of the
	 * function's return value.  it also returns the true argument types
	 * to the function.
	 */
	fdresult = func_get_detail(funcname, fargs, nargs, oid_array,
							   &funcid, &rettype, &retset,
							   &true_oid_array);
	if (fdresult == FUNCDETAIL_COERCION)
	{
		/*
		 * We can do it as a trivial coercion. coerce_type can handle
		 * these cases, so why duplicate code...
		 */
		return coerce_type(lfirst(fargs), oid_array[0], rettype,
						   COERCION_EXPLICIT, COERCE_EXPLICIT_CALL);
	}
	else if (fdresult == FUNCDETAIL_NORMAL)
	{
		/*
		 * Normal function found; was there anything indicating it must be
		 * an aggregate?
		 */
		if (agg_star)
			elog(ERROR, "%s(*) specified, but %s is not an aggregate function",
				 NameListToString(funcname), NameListToString(funcname));
		if (agg_distinct)
			elog(ERROR, "DISTINCT specified, but %s is not an aggregate function",
				 NameListToString(funcname));
	}
	else if (fdresult != FUNCDETAIL_AGGREGATE)
	{
		/*
		 * Oops.  Time to die.
		 *
		 * If we are dealing with the attribute notation rel.function, give
		 * an error message that is appropriate for that case.
		 */
		if (is_column)
		{
			char	   *colname = strVal(lfirst(funcname));
			Oid			relTypeId;

			Assert(nargs == 1);
			if (IsA(first_arg, RangeVar))
				unknown_attribute(((RangeVar *) first_arg)->schemaname,
								  ((RangeVar *) first_arg)->relname,
								  colname);
			relTypeId = exprType(first_arg);
			if (!ISCOMPLEX(relTypeId))
				elog(ERROR, "Attribute notation .%s applied to type %s, which is not a complex type",
					 colname, format_type_be(relTypeId));
			else
				elog(ERROR, "Attribute \"%s\" not found in datatype %s",
					 colname, format_type_be(relTypeId));
		}

		/*
		 * Else generate a detailed complaint for a function
		 */
		func_error(NULL, funcname, nargs, oid_array,
				   "Unable to identify a function that satisfies the "
				   "given argument types"
				   "\n\tYou may need to add explicit typecasts");
	}

	/* perform the necessary typecasting of arguments */
	make_arguments(nargs, fargs, oid_array, true_oid_array);

	/* build the appropriate output structure */
	if (fdresult == FUNCDETAIL_NORMAL)
	{
		Expr	   *expr = makeNode(Expr);
		Func	   *funcnode = makeNode(Func);

		funcnode->funcid = funcid;
		funcnode->funcresulttype = rettype;
		funcnode->funcretset = retset;
		funcnode->funcformat = COERCE_EXPLICIT_CALL;
		funcnode->func_fcache = NULL;

		expr->typeOid = rettype;
		expr->opType = FUNC_EXPR;
		expr->oper = (Node *) funcnode;
		expr->args = fargs;

		retval = (Node *) expr;
	}
	else
	{
		/* aggregate function */
		Aggref	   *aggref = makeNode(Aggref);

		aggref->aggfnoid = funcid;
		aggref->aggtype = rettype;
		aggref->target = lfirst(fargs);
		aggref->aggstar = agg_star;
		aggref->aggdistinct = agg_distinct;

		retval = (Node *) aggref;

		if (retset)
			elog(ERROR, "Aggregates may not return sets");

		pstate->p_hasAggs = true;
	}

	return retval;
}


/* match_argtypes()
 *
 * Given a list of possible typeid arrays to a function and an array of
 * input typeids, produce a shortlist of those function typeid arrays
 * that match the input typeids (either exactly or by coercion), and
 * return the number of such arrays.
 *
 * NB: okay to modify input list structure, as long as we find at least
 * one match.
 */
static int
match_argtypes(int nargs,
			   Oid *input_typeids,
			   FuncCandidateList function_typeids,
			   FuncCandidateList *candidates)	/* return value */
{
	FuncCandidateList current_candidate;
	FuncCandidateList next_candidate;
	int			ncandidates = 0;

	*candidates = NULL;

	for (current_candidate = function_typeids;
		 current_candidate != NULL;
		 current_candidate = next_candidate)
	{
		next_candidate = current_candidate->next;
		if (can_coerce_type(nargs, input_typeids, current_candidate->args,
							COERCION_IMPLICIT))
		{
			current_candidate->next = *candidates;
			*candidates = current_candidate;
			ncandidates++;
		}
	}

	return ncandidates;
}	/* match_argtypes() */


/* func_select_candidate()
 * Given the input argtype array and more than one candidate
 * for the function, attempt to resolve the conflict.
 * Returns the selected candidate if the conflict can be resolved,
 * otherwise returns NULL.
 *
 * By design, this is pretty similar to oper_select_candidate in parse_oper.c.
 * However, the calling convention is a little different: we assume the caller
 * already pruned away "candidates" that aren't actually coercion-compatible
 * with the input types, whereas oper_select_candidate must do that itself.
 */
static FuncCandidateList
func_select_candidate(int nargs,
					  Oid *input_typeids,
					  FuncCandidateList candidates)
{
	FuncCandidateList current_candidate;
	FuncCandidateList last_candidate;
	Oid		   *current_typeids;
	Oid			current_type;
	int			i;
	int			ncandidates;
	int			nbestMatch,
				nmatch;
	CATEGORY	slot_category[FUNC_MAX_ARGS],
				current_category;
	bool		slot_has_preferred_type[FUNC_MAX_ARGS];
	bool		resolved_unknowns;

	/*
	 * Run through all candidates and keep those with the most matches on
	 * exact types. Keep all candidates if none match.
	 */
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_typeids[i] != UNKNOWNOID &&
				current_typeids[i] == input_typeids[i])
				nmatch++;
		}

		/* take this one as the best choice so far? */
		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		/* no worse than the last choice, so keep this one too? */
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
		/* otherwise, don't bother keeping this one... */
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates;

	/*
	 * Still too many candidates? Run through all candidates and keep
	 * those with the most matches on exact types + binary-compatible
	 * types. Keep all candidates if none match.
	 */
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_typeids[i] != UNKNOWNOID)
			{
				if (IsBinaryCoercible(input_typeids[i], current_typeids[i]))
					nmatch++;
			}
		}

		/* take this one as the best choice so far? */
		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		/* no worse than the last choice, so keep this one too? */
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
		/* otherwise, don't bother keeping this one... */
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates;

	/*
	 * Still too many candidates? Now look for candidates which are
	 * preferred types at the args that will require coercion. Keep all
	 * candidates if none match.
	 */
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_typeids[i] != UNKNOWNOID)
			{
				current_category = TypeCategory(current_typeids[i]);
				if (current_typeids[i] == input_typeids[i] ||
					IsPreferredType(current_category, current_typeids[i]))
					nmatch++;
			}
		}

		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates;

	/*
	 * Still too many candidates? Try assigning types for the unknown
	 * columns.
	 *
	 * We do this by examining each unknown argument position to see if we
	 * can determine a "type category" for it.	If any candidate has an
	 * input datatype of STRING category, use STRING category (this bias
	 * towards STRING is appropriate since unknown-type literals look like
	 * strings).  Otherwise, if all the candidates agree on the type
	 * category of this argument position, use that category.  Otherwise,
	 * fail because we cannot determine a category.
	 *
	 * If we are able to determine a type category, also notice whether any
	 * of the candidates takes a preferred datatype within the category.
	 *
	 * Having completed this examination, remove candidates that accept the
	 * wrong category at any unknown position.	Also, if at least one
	 * candidate accepted a preferred type at a position, remove
	 * candidates that accept non-preferred types.
	 *
	 * If we are down to one candidate at the end, we win.
	 */
	resolved_unknowns = false;
	for (i = 0; i < nargs; i++)
	{
		bool		have_conflict;

		if (input_typeids[i] != UNKNOWNOID)
			continue;
		resolved_unknowns = true;		/* assume we can do it */
		slot_category[i] = INVALID_TYPE;
		slot_has_preferred_type[i] = false;
		have_conflict = false;
		for (current_candidate = candidates;
			 current_candidate != NULL;
			 current_candidate = current_candidate->next)
		{
			current_typeids = current_candidate->args;
			current_type = current_typeids[i];
			current_category = TypeCategory(current_type);
			if (slot_category[i] == INVALID_TYPE)
			{
				/* first candidate */
				slot_category[i] = current_category;
				slot_has_preferred_type[i] =
					IsPreferredType(current_category, current_type);
			}
			else if (current_category == slot_category[i])
			{
				/* more candidates in same category */
				slot_has_preferred_type[i] |=
					IsPreferredType(current_category, current_type);
			}
			else
			{
				/* category conflict! */
				if (current_category == STRING_TYPE)
				{
					/* STRING always wins if available */
					slot_category[i] = current_category;
					slot_has_preferred_type[i] =
						IsPreferredType(current_category, current_type);
				}
				else
				{
					/*
					 * Remember conflict, but keep going (might find
					 * STRING)
					 */
					have_conflict = true;
				}
			}
		}
		if (have_conflict && slot_category[i] != STRING_TYPE)
		{
			/* Failed to resolve category conflict at this position */
			resolved_unknowns = false;
			break;
		}
	}

	if (resolved_unknowns)
	{
		/* Strip non-matching candidates */
		ncandidates = 0;
		last_candidate = NULL;
		for (current_candidate = candidates;
			 current_candidate != NULL;
			 current_candidate = current_candidate->next)
		{
			bool		keepit = true;

			current_typeids = current_candidate->args;
			for (i = 0; i < nargs; i++)
			{
				if (input_typeids[i] != UNKNOWNOID)
					continue;
				current_type = current_typeids[i];
				current_category = TypeCategory(current_type);
				if (current_category != slot_category[i])
				{
					keepit = false;
					break;
				}
				if (slot_has_preferred_type[i] &&
					!IsPreferredType(current_category, current_type))
				{
					keepit = false;
					break;
				}
			}
			if (keepit)
			{
				/* keep this candidate */
				last_candidate = current_candidate;
				ncandidates++;
			}
			else
			{
				/* forget this candidate */
				if (last_candidate)
					last_candidate->next = current_candidate->next;
				else
					candidates = current_candidate->next;
			}
		}
		if (last_candidate)		/* terminate rebuilt list */
			last_candidate->next = NULL;
	}

	if (ncandidates == 1)
		return candidates;

	return NULL;				/* failed to determine a unique candidate */
}	/* func_select_candidate() */


/* func_get_detail()
 *
 * Find the named function in the system catalogs.
 *
 * Attempt to find the named function in the system catalogs with
 *	arguments exactly as specified, so that the normal case
 *	(exact match) is as quick as possible.
 *
 * If an exact match isn't found:
 *	1) check for possible interpretation as a trivial type coercion
 *	2) get a vector of all possible input arg type arrays constructed
 *	   from the superclasses of the original input arg types
 *	3) get a list of all possible argument type arrays to the function
 *	   with given name and number of arguments
 *	4) for each input arg type array from vector #1:
 *	 a) find how many of the function arg type arrays from list #2
 *		it can be coerced to
 *	 b) if the answer is one, we have our function
 *	 c) if the answer is more than one, attempt to resolve the conflict
 *	 d) if the answer is zero, try the next array from vector #1
 *
 * Note: we rely primarily on nargs/argtypes as the argument description.
 * The actual expression node list is passed in fargs so that we can check
 * for type coercion of a constant.  Some callers pass fargs == NIL
 * indicating they don't want that check made.
 */
FuncDetailCode
func_get_detail(List *funcname,
				List *fargs,
				int nargs,
				Oid *argtypes,
				Oid *funcid,	/* return value */
				Oid *rettype,	/* return value */
				bool *retset,	/* return value */
				Oid **true_typeids)		/* return value */
{
	FuncCandidateList function_typeids;
	FuncCandidateList best_candidate;

	/* Get list of possible candidates from namespace search */
	function_typeids = FuncnameGetCandidates(funcname, nargs);

	/*
	 * See if there is an exact match
	 */
	for (best_candidate = function_typeids;
		 best_candidate != NULL;
		 best_candidate = best_candidate->next)
	{
		if (memcmp(argtypes, best_candidate->args, nargs * sizeof(Oid)) == 0)
			break;
	}

	if (best_candidate == NULL)
	{
		/*
		 * If we didn't find an exact match, next consider the possibility
		 * that this is really a type-coercion request: a single-argument
		 * function call where the function name is a type name.  If so,
		 * and if we can do the coercion trivially (no run-time function
		 * call needed), then go ahead and treat the "function call" as a
		 * coercion.  This interpretation needs to be given higher
		 * priority than interpretations involving a type coercion
		 * followed by a function call, otherwise we can produce
		 * surprising results. For example, we want "text(varchar)" to be
		 * interpreted as a trivial coercion, not as "text(name(varchar))"
		 * which the code below this point is entirely capable of
		 * selecting.
		 *
		 * "Trivial" coercions are ones that involve binary-compatible types
		 * and ones that are coercing a previously-unknown-type literal
		 * constant to a specific type.
		 *
		 * NB: it's important that this code stays in sync with what
		 * coerce_type can do, because the caller will try to apply
		 * coerce_type if we return FUNCDETAIL_COERCION.  If we return
		 * that result for something coerce_type can't handle, we'll cause
		 * infinite recursion between this module and coerce_type!
		 */
		if (nargs == 1 && fargs != NIL)
		{
			Oid			targetType;
			TypeName   *tn = makeNode(TypeName);

			tn->names = funcname;
			tn->typmod = -1;
			targetType = LookupTypeName(tn);
			if (OidIsValid(targetType) &&
				!ISCOMPLEX(targetType))
			{
				Oid			sourceType = argtypes[0];
				Node	   *arg1 = lfirst(fargs);

				if ((sourceType == UNKNOWNOID && IsA(arg1, Const)) ||
					IsBinaryCoercible(sourceType, targetType))
				{
					/* Yup, it's a type coercion */
					*funcid = InvalidOid;
					*rettype = targetType;
					*retset = false;
					*true_typeids = argtypes;
					return FUNCDETAIL_COERCION;
				}
			}
		}

		/*
		 * didn't find an exact match, so now try to match up
		 * candidates...
		 */
		if (function_typeids != NULL)
		{
			Oid		  **input_typeid_vector = NULL;
			Oid		   *current_input_typeids;

			/*
			 * First we will search with the given argtypes, then with
			 * variants based on replacing complex types with their
			 * inheritance ancestors.  Stop as soon as any match is found.
			 */
			current_input_typeids = argtypes;

			do
			{
				FuncCandidateList current_function_typeids;
				int			ncandidates;

				ncandidates = match_argtypes(nargs, current_input_typeids,
											 function_typeids,
											 &current_function_typeids);

				/* one match only? then run with it... */
				if (ncandidates == 1)
				{
					best_candidate = current_function_typeids;
					break;
				}

				/*
				 * multiple candidates? then better decide or throw an
				 * error...
				 */
				if (ncandidates > 1)
				{
					best_candidate = func_select_candidate(nargs,
												   current_input_typeids,
											   current_function_typeids);

					/*
					 * If we were able to choose a best candidate, we're
					 * done.  Otherwise, ambiguous function call, so fail
					 * by exiting loop with best_candidate still NULL.
					 * Either way, we're outta here.
					 */
					break;
				}

				/*
				 * No match here, so try the next inherited type vector.
				 * First time through, we need to compute the list of
				 * vectors.
				 */
				if (input_typeid_vector == NULL)
					input_typeid_vector = argtype_inherit(nargs, argtypes);

				current_input_typeids = *input_typeid_vector++;
			}
			while (current_input_typeids != NULL);
		}
	}

	if (best_candidate)
	{
		HeapTuple	ftup;
		Form_pg_proc pform;
		FuncDetailCode result;

		*funcid = best_candidate->oid;
		*true_typeids = best_candidate->args;

		ftup = SearchSysCache(PROCOID,
							  ObjectIdGetDatum(best_candidate->oid),
							  0, 0, 0);
		if (!HeapTupleIsValid(ftup))	/* should not happen */
			elog(ERROR, "function %u not found", best_candidate->oid);
		pform = (Form_pg_proc) GETSTRUCT(ftup);
		*rettype = pform->prorettype;
		*retset = pform->proretset;
		result = pform->proisagg ? FUNCDETAIL_AGGREGATE : FUNCDETAIL_NORMAL;
		ReleaseSysCache(ftup);
		return result;
	}

	return FUNCDETAIL_NOTFOUND;
}	/* func_get_detail() */

/*
 *	argtype_inherit() -- Construct an argtype vector reflecting the
 *						 inheritance properties of the supplied argv.
 *
 *		This function is used to disambiguate among functions with the
 *		same name but different signatures.  It takes an array of input
 *		type ids.  For each type id in the array that's a complex type
 *		(a class), it walks up the inheritance tree, finding all
 *		superclasses of that type.	A vector of new Oid type arrays
 *		is returned to the caller, reflecting the structure of the
 *		inheritance tree above the supplied arguments.
 *
 *		The order of this vector is as follows:  all superclasses of the
 *		rightmost complex class are explored first.  The exploration
 *		continues from right to left.  This policy means that we favor
 *		keeping the leftmost argument type as low in the inheritance tree
 *		as possible.  This is intentional; it is exactly what we need to
 *		do for method dispatch.  The last type array we return is all
 *		zeroes.  This will match any functions for which return types are
 *		not defined.  There are lots of these (mostly builtins) in the
 *		catalogs.
 */
static Oid **
argtype_inherit(int nargs, Oid *argtypes)
{
	Oid			relid;
	int			i;
	InhPaths	arginh[FUNC_MAX_ARGS];

	for (i = 0; i < FUNC_MAX_ARGS; i++)
	{
		if (i < nargs)
		{
			arginh[i].self = argtypes[i];
			if ((relid = typeidTypeRelid(argtypes[i])) != InvalidOid)
				arginh[i].nsupers = find_inheritors(relid, &(arginh[i].supervec));
			else
			{
				arginh[i].nsupers = 0;
				arginh[i].supervec = (Oid *) NULL;
			}
		}
		else
		{
			arginh[i].self = InvalidOid;
			arginh[i].nsupers = 0;
			arginh[i].supervec = (Oid *) NULL;
		}
	}

	/* return an ordered cross-product of the classes involved */
	return gen_cross_product(arginh, nargs);
}

static int
find_inheritors(Oid relid, Oid **supervec)
{
	Relation	inhrel;
	HeapScanDesc inhscan;
	ScanKeyData skey;
	HeapTuple	inhtup;
	Oid		   *relidvec;
	int			nvisited;
	List	   *visited,
			   *queue;
	List	   *elt;
	bool		newrelid;

	nvisited = 0;
	queue = NIL;
	visited = NIL;

	inhrel = heap_openr(InheritsRelationName, AccessShareLock);

	/*
	 * Use queue to do a breadth-first traversal of the inheritance graph
	 * from the relid supplied up to the root.	At the top of the loop,
	 * relid is the OID of the reltype to check next, queue is the list of
	 * pending rels to check after this one, and visited is the list of
	 * relids we need to output.
	 */
	do
	{
		/* find all types this relid inherits from, and add them to queue */

		ScanKeyEntryInitialize(&skey, 0x0, Anum_pg_inherits_inhrelid,
							   F_OIDEQ,
							   ObjectIdGetDatum(relid));

		inhscan = heap_beginscan(inhrel, SnapshotNow, 1, &skey);

		while ((inhtup = heap_getnext(inhscan, ForwardScanDirection)) != NULL)
		{
			Form_pg_inherits inh = (Form_pg_inherits) GETSTRUCT(inhtup);

			queue = lappendi(queue, inh->inhparent);
		}

		heap_endscan(inhscan);

		/* pull next unvisited relid off the queue */

		newrelid = false;
		while (queue != NIL)
		{
			relid = lfirsti(queue);
			queue = lnext(queue);
			if (!intMember(relid, visited))
			{
				newrelid = true;
				break;
			}
		}

		if (newrelid)
		{
			visited = lappendi(visited, relid);
			nvisited++;
		}
	} while (newrelid);

	heap_close(inhrel, AccessShareLock);

	if (nvisited > 0)
	{
		relidvec = (Oid *) palloc(nvisited * sizeof(Oid));
		*supervec = relidvec;

		foreach(elt, visited)
		{
			/* return the type id, rather than the relation id */
			*relidvec++ = get_rel_type_id((Oid) lfirsti(elt));
		}
	}
	else
		*supervec = (Oid *) NULL;

	freeList(visited);

	/*
	 * there doesn't seem to be any equally easy way to release the queue
	 * list cells, but since they're palloc'd space it's not critical.
	 */

	return nvisited;
}

static Oid **
gen_cross_product(InhPaths *arginh, int nargs)
{
	int			nanswers;
	Oid		  **result,
			  **iter;
	Oid		   *oneres;
	int			i,
				j;
	int			cur[FUNC_MAX_ARGS];

	nanswers = 1;
	for (i = 0; i < nargs; i++)
	{
		nanswers *= (arginh[i].nsupers + 2);
		cur[i] = 0;
	}

	iter = result = (Oid **) palloc(sizeof(Oid *) * nanswers);

	/* compute the cross product from right to left */
	for (;;)
	{
		oneres = (Oid *) palloc(FUNC_MAX_ARGS * sizeof(Oid));
		MemSet(oneres, 0, FUNC_MAX_ARGS * sizeof(Oid));

		for (i = nargs - 1; i >= 0 && cur[i] > arginh[i].nsupers; i--)
			continue;

		/* if we're done, terminate with NULL pointer */
		if (i < 0)
		{
			*iter = NULL;
			return result;
		}

		/* no, increment this column and zero the ones after it */
		cur[i] = cur[i] + 1;
		for (j = nargs - 1; j > i; j--)
			cur[j] = 0;

		for (i = 0; i < nargs; i++)
		{
			if (cur[i] == 0)
				oneres[i] = arginh[i].self;
			else if (cur[i] > arginh[i].nsupers)
				oneres[i] = 0;	/* wild card */
			else
				oneres[i] = arginh[i].supervec[cur[i] - 1];
		}

		*iter++ = oneres;
	}
}


/*
 * Given two type OIDs, determine whether the first is a complex type
 * (class type) that inherits from the second.
 */
bool
typeInheritsFrom(Oid subclassTypeId, Oid superclassTypeId)
{
	Oid			relid;
	Oid		   *supervec;
	int			nsupers,
				i;
	bool		result;

	if (!ISCOMPLEX(subclassTypeId) || !ISCOMPLEX(superclassTypeId))
		return false;
	relid = typeidTypeRelid(subclassTypeId);
	if (relid == InvalidOid)
		return false;
	nsupers = find_inheritors(relid, &supervec);
	result = false;
	for (i = 0; i < nsupers; i++)
	{
		if (supervec[i] == superclassTypeId)
		{
			result = true;
			break;
		}
	}
	if (supervec)
		pfree(supervec);
	return result;
}


/* make_arguments()
 * Given the number and types of arguments to a function, and the
 *	actual arguments and argument types, do the necessary typecasting.
 */
static void
make_arguments(int nargs,
			   List *fargs,
			   Oid *input_typeids,
			   Oid *function_typeids)
{
	List	   *current_fargs;
	int			i;

	for (i = 0, current_fargs = fargs;
		 i < nargs;
		 i++, current_fargs = lnext(current_fargs))
	{
		/* types don't match? then force coercion using a function call... */
		if (input_typeids[i] != function_typeids[i])
		{
			lfirst(current_fargs) = coerce_type(lfirst(current_fargs),
												input_typeids[i],
												function_typeids[i],
												COERCION_IMPLICIT,
												COERCE_IMPLICIT_CAST);
		}
	}
}

/*
 * setup_field_select
 *		Build a FieldSelect node that says which attribute to project to.
 *		This routine is called by ParseFuncOrColumn() when we have found
 *		a projection on a function result or parameter.
 */
static FieldSelect *
setup_field_select(Node *input, char *attname, Oid relid)
{
	FieldSelect *fselect = makeNode(FieldSelect);
	AttrNumber	attno;

	attno = get_attnum(relid, attname);
	if (attno == InvalidAttrNumber)
		elog(ERROR, "Relation \"%s\" has no column \"%s\"",
			 get_rel_name(relid), attname);

	fselect->arg = input;
	fselect->fieldnum = attno;
	fselect->resulttype = get_atttype(relid, attno);
	fselect->resulttypmod = get_atttypmod(relid, attno);

	return fselect;
}

/*
 * ParseComplexProjection -
 *	  handles function calls with a single argument that is of complex type.
 *	  If the function call is actually a column projection, return a suitably
 *	  transformed expression tree.	If not, return NULL.
 *
 * NB: argument is expected to be transformed already, ie, not a RangeVar.
 */
static Node *
ParseComplexProjection(char *funcname, Node *first_arg)
{
	Oid			argtype = exprType(first_arg);
	Oid			argrelid;
	AttrNumber	attnum;
	FieldSelect *fselect;

	argrelid = typeidTypeRelid(argtype);
	if (!argrelid)
		return NULL;			/* probably should not happen */
	attnum = get_attnum(argrelid, funcname);
	if (attnum == InvalidAttrNumber)
		return NULL;			/* funcname does not match any column */

	/*
	 * Check for special cases where we don't want to return a
	 * FieldSelect.
	 */
	switch (nodeTag(first_arg))
	{
		case T_Var:
			{
				Var		   *var = (Var *) first_arg;

				/*
				 * If the Var is a whole-row tuple, we can just replace it
				 * with a simple Var reference.
				 */
				if (var->varattno == InvalidAttrNumber)
				{
					Oid			vartype;
					int32		vartypmod;

					get_atttypetypmod(argrelid, attnum,
									  &vartype, &vartypmod);

					return (Node *) makeVar(var->varno,
											attnum,
											vartype,
											vartypmod,
											var->varlevelsup);
				}
				break;
			}
		default:
			break;
	}

	/* Else generate a FieldSelect expression */
	fselect = setup_field_select(first_arg, funcname, argrelid);
	return (Node *) fselect;
}

/*
 * Simple helper routine for delivering "No such attribute" error message
 */
static void
unknown_attribute(const char *schemaname, const char *relname,
				  const char *attname)
{
	if (schemaname)
		elog(ERROR, "No such attribute %s.%s.%s",
			 schemaname, relname, attname);
	else
		elog(ERROR, "No such attribute %s.%s",
			 relname, attname);
}

/*
 * Error message when function lookup fails that gives details of the
 * argument types
 */
void
func_error(const char *caller, List *funcname,
		   int nargs, const Oid *argtypes,
		   const char *msg)
{
	StringInfoData argbuf;
	int			i;

	initStringInfo(&argbuf);

	for (i = 0; i < nargs; i++)
	{
		if (i)
			appendStringInfo(&argbuf, ", ");
		appendStringInfo(&argbuf, format_type_be(argtypes[i]));
	}

	if (caller == NULL)
	{
		elog(ERROR, "Function %s(%s) does not exist%s%s",
			 NameListToString(funcname), argbuf.data,
			 ((msg != NULL) ? "\n\t" : ""), ((msg != NULL) ? msg : ""));
	}
	else
	{
		elog(ERROR, "%s: function %s(%s) does not exist%s%s",
			 caller, NameListToString(funcname), argbuf.data,
			 ((msg != NULL) ? "\n\t" : ""), ((msg != NULL) ? msg : ""));
	}
}

/*
 * find_aggregate_func
 *		Convenience routine to check that a function exists and is an
 *		aggregate.
 *
 * Note: basetype is ANYOID if we are looking for an aggregate on
 * all types.
 */
Oid
find_aggregate_func(const char *caller, List *aggname, Oid basetype)
{
	Oid			oid;
	HeapTuple	ftup;
	Form_pg_proc pform;

	oid = LookupFuncName(aggname, 1, &basetype);

	if (!OidIsValid(oid))
	{
		if (basetype == ANYOID)
			elog(ERROR, "%s: aggregate %s(*) does not exist",
				 caller, NameListToString(aggname));
		else
			elog(ERROR, "%s: aggregate %s(%s) does not exist",
				 caller, NameListToString(aggname),
				 format_type_be(basetype));
	}

	/* Make sure it's an aggregate */
	ftup = SearchSysCache(PROCOID,
						  ObjectIdGetDatum(oid),
						  0, 0, 0);
	if (!HeapTupleIsValid(ftup))	/* should not happen */
		elog(ERROR, "function %u not found", oid);
	pform = (Form_pg_proc) GETSTRUCT(ftup);

	if (!pform->proisagg)
	{
		if (basetype == ANYOID)
			elog(ERROR, "%s: function %s(*) is not an aggregate",
				 caller, NameListToString(aggname));
		else
			elog(ERROR, "%s: function %s(%s) is not an aggregate",
				 caller, NameListToString(aggname),
				 format_type_be(basetype));
	}

	ReleaseSysCache(ftup);

	return oid;
}

/*
 * LookupFuncName
 *		Given a possibly-qualified function name and a set of argument types,
 *		look up the function.  Returns InvalidOid if no such function.
 *
 * If the function name is not schema-qualified, it is sought in the current
 * namespace search path.
 */
Oid
LookupFuncName(List *funcname, int nargs, const Oid *argtypes)
{
	FuncCandidateList clist;

	clist = FuncnameGetCandidates(funcname, nargs);

	while (clist)
	{
		if (memcmp(argtypes, clist->args, nargs * sizeof(Oid)) == 0)
			return clist->oid;
		clist = clist->next;
	}

	return InvalidOid;
}

/*
 * LookupFuncNameTypeNames
 *		Like LookupFuncName, but the argument types are specified by a
 *		list of TypeName nodes.  Also, if we fail to find the function
 *		and caller is not NULL, then an error is reported via func_error.
 */
Oid
LookupFuncNameTypeNames(List *funcname, List *argtypes, const char *caller)
{
	Oid			funcoid;
	Oid			argoids[FUNC_MAX_ARGS];
	int			argcount;
	int			i;

	MemSet(argoids, 0, FUNC_MAX_ARGS * sizeof(Oid));
	argcount = length(argtypes);
	if (argcount > FUNC_MAX_ARGS)
		elog(ERROR, "functions cannot have more than %d arguments",
			 FUNC_MAX_ARGS);

	for (i = 0; i < argcount; i++)
	{
		TypeName   *t = (TypeName *) lfirst(argtypes);

		argoids[i] = LookupTypeName(t);

		if (!OidIsValid(argoids[i]))
			elog(ERROR, "Type \"%s\" does not exist",
				 TypeNameToString(t));

		argtypes = lnext(argtypes);
	}

	funcoid = LookupFuncName(funcname, argcount, argoids);

	if (!OidIsValid(funcoid) && caller != NULL)
		func_error(caller, funcname, argcount, argoids, NULL);

	return funcoid;
}
