/*-------------------------------------------------------------------------
 *
 * clauses.c
 *	  routines to manipulate qualification clauses
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/clauses.c,v 1.120 2002-12-15 16:17:50 tgl Exp $
 *
 * HISTORY
 *	  AUTHOR			DATE			MAJOR EVENT
 *	  Andrew Yu			Nov 3, 1994		clause.c and clauses.c combined
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_language.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "executor/executor.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/tlist.h"
#include "optimizer/var.h"
#include "parser/analyze.h"
#include "parser/parsetree.h"
#include "tcop/tcopprot.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/syscache.h"


/* note that pg_type.h hardwires size of bool as 1 ... duplicate it */
#define MAKEBOOLCONST(val,isnull) \
	((Node *) makeConst(BOOLOID, 1, (Datum) (val), (isnull), true))

typedef struct
{
	Query	   *query;
	List	   *groupClauses;
} check_subplans_for_ungrouped_vars_context;

typedef struct
{
	int			nargs;
	List	   *args;
	int		   *usecounts;
} substitute_actual_parameters_context;

static bool contain_agg_clause_walker(Node *node, void *context);
static bool contain_distinct_agg_clause_walker(Node *node, void *context);
static bool pull_agg_clause_walker(Node *node, List **listptr);
static bool expression_returns_set_walker(Node *node, void *context);
static bool contain_subplans_walker(Node *node, void *context);
static bool pull_subplans_walker(Node *node, List **listptr);
static bool check_subplans_for_ungrouped_vars_walker(Node *node,
					 check_subplans_for_ungrouped_vars_context *context);
static bool contain_mutable_functions_walker(Node *node, void *context);
static bool contain_volatile_functions_walker(Node *node, void *context);
static bool contain_nonstrict_functions_walker(Node *node, void *context);
static Node *eval_const_expressions_mutator(Node *node, List *active_fns);
static Expr *simplify_function(Oid funcid, List *args, bool allow_inline,
							   List *active_fns);
static Expr *evaluate_function(Oid funcid, List *args, HeapTuple func_tuple);
static Expr *inline_function(Oid funcid, List *args, HeapTuple func_tuple,
							 List *active_fns);
static Node *substitute_actual_parameters(Node *expr, int nargs, List *args,
										  int *usecounts);
static Node *substitute_actual_parameters_mutator(Node *node,
					 substitute_actual_parameters_context *context);


/*****************************************************************************
 *		OPERATOR clause functions
 *****************************************************************************/

/*
 * make_opclause
 *	  Creates an operator clause given its operator info, left operand,
 *	  and right operand (pass NULL to create single-operand clause).
 */
Expr *
make_opclause(Oid opno, Oid opresulttype, bool opretset,
			  Expr *leftop, Expr *rightop)
{
	OpExpr	   *expr = makeNode(OpExpr);

	expr->opno = opno;
	expr->opfuncid = InvalidOid;
	expr->opresulttype = opresulttype;
	expr->opretset = opretset;
	if (rightop)
		expr->args = makeList2(leftop, rightop);
	else
		expr->args = makeList1(leftop);
	return (Expr *) expr;
}

/*
 * get_leftop
 *
 * Returns the left operand of a clause of the form (op expr expr)
 *		or (op expr)
 *
 * NB: for historical reasons, the result is declared Var *, even
 * though many callers can cope with results that are not Vars.
 * The result really ought to be declared Expr * or Node *.
 */
Var *
get_leftop(Expr *clause)
{
	OpExpr *expr = (OpExpr *) clause;

	if (expr->args != NULL)
		return lfirst(expr->args);
	else
		return NULL;
}

/*
 * get_rightop
 *
 * Returns the right operand in a clause of the form (op expr expr).
 * NB: result will be NULL if applied to a unary op clause.
 */
Var *
get_rightop(Expr *clause)
{
	OpExpr *expr = (OpExpr *) clause;

	if (expr->args != NULL && lnext(expr->args) != NULL)
		return lfirst(lnext(expr->args));
	else
		return NULL;
}

/*****************************************************************************
 *		FUNCTION clause functions
 *****************************************************************************/

/*
 * make_funcclause
 *	  Creates a function clause given its function info and argument list.
 */
Expr *
make_funcclause(Oid funcid, Oid funcresulttype, bool funcretset,
				CoercionForm funcformat, List *funcargs)
{
	FuncExpr   *expr = makeNode(FuncExpr);

	expr->funcid = funcid;
	expr->funcresulttype = funcresulttype;
	expr->funcretset = funcretset;
	expr->funcformat = funcformat;
	expr->args = funcargs;
	return (Expr *) expr;
}

/*****************************************************************************
 *		NOT clause functions
 *****************************************************************************/

/*
 * not_clause
 *
 * Returns t iff this is a 'not' clause: (NOT expr).
 */
bool
not_clause(Node *clause)
{
	return (clause != NULL &&
			IsA(clause, BoolExpr) &&
			((BoolExpr *) clause)->boolop == NOT_EXPR);
}

/*
 * make_notclause
 *
 * Create a 'not' clause given the expression to be negated.
 */
Expr *
make_notclause(Expr *notclause)
{
	BoolExpr   *expr = makeNode(BoolExpr);

	expr->boolop = NOT_EXPR;
	expr->args = makeList1(notclause);
	return (Expr *) expr;
}

/*
 * get_notclausearg
 *
 * Retrieve the clause within a 'not' clause
 */
Expr *
get_notclausearg(Expr *notclause)
{
	return lfirst(((BoolExpr *) notclause)->args);
}

/*****************************************************************************
 *		OR clause functions
 *****************************************************************************/

/*
 * or_clause
 *
 * Returns t iff the clause is an 'or' clause: (OR { expr }).
 */
bool
or_clause(Node *clause)
{
	return (clause != NULL &&
			IsA(clause, BoolExpr) &&
			((BoolExpr *) clause)->boolop == OR_EXPR);
}

/*
 * make_orclause
 *
 * Creates an 'or' clause given a list of its subclauses.
 */
Expr *
make_orclause(List *orclauses)
{
	BoolExpr   *expr = makeNode(BoolExpr);

	expr->boolop = OR_EXPR;
	expr->args = orclauses;
	return (Expr *) expr;
}

/*****************************************************************************
 *		AND clause functions
 *****************************************************************************/


/*
 * and_clause
 *
 * Returns t iff its argument is an 'and' clause: (AND { expr }).
 */
bool
and_clause(Node *clause)
{
	return (clause != NULL &&
			IsA(clause, BoolExpr) &&
			((BoolExpr *) clause)->boolop == AND_EXPR);
}

/*
 * make_andclause
 *
 * Creates an 'and' clause given a list of its subclauses.
 */
Expr *
make_andclause(List *andclauses)
{
	BoolExpr   *expr = makeNode(BoolExpr);

	expr->boolop = AND_EXPR;
	expr->args = andclauses;
	return (Expr *) expr;
}

/*
 * make_and_qual
 *
 * Variant of make_andclause for ANDing two qual conditions together.
 * Qual conditions have the property that a NULL nodetree is interpreted
 * as 'true'.
 */
Node *
make_and_qual(Node *qual1, Node *qual2)
{
	if (qual1 == NULL)
		return qual2;
	if (qual2 == NULL)
		return qual1;
	return (Node *) make_andclause(makeList2(qual1, qual2));
}

/*
 * Sometimes (such as in the result of canonicalize_qual or the input of
 * ExecQual), we use lists of expression nodes with implicit AND semantics.
 *
 * These functions convert between an AND-semantics expression list and the
 * ordinary representation of a boolean expression.
 *
 * Note that an empty list is considered equivalent to TRUE.
 */
Expr *
make_ands_explicit(List *andclauses)
{
	if (andclauses == NIL)
		return (Expr *) MAKEBOOLCONST(true, false);
	else if (lnext(andclauses) == NIL)
		return (Expr *) lfirst(andclauses);
	else
		return make_andclause(andclauses);
}

List *
make_ands_implicit(Expr *clause)
{
	/*
	 * NB: because the parser sets the qual field to NULL in a query that
	 * has no WHERE clause, we must consider a NULL input clause as TRUE,
	 * even though one might more reasonably think it FALSE.  Grumble. If
	 * this causes trouble, consider changing the parser's behavior.
	 */
	if (clause == NULL)
		return NIL;				/* NULL -> NIL list == TRUE */
	else if (and_clause((Node *) clause))
		return ((BoolExpr *) clause)->args;
	else if (IsA(clause, Const) &&
			 !((Const *) clause)->constisnull &&
			 DatumGetBool(((Const *) clause)->constvalue))
		return NIL;				/* constant TRUE input -> NIL list */
	else
		return makeList1(clause);
}


/*****************************************************************************
 *		Aggregate-function clause manipulation
 *****************************************************************************/

/*
 * contain_agg_clause
 *	  Recursively search for Aggref nodes within a clause.
 *
 *	  Returns true if any aggregate found.
 */
bool
contain_agg_clause(Node *clause)
{
	return contain_agg_clause_walker(clause, NULL);
}

static bool
contain_agg_clause_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Aggref))
		return true;			/* abort the tree traversal and return
								 * true */
	return expression_tree_walker(node, contain_agg_clause_walker, context);
}

/*
 * contain_distinct_agg_clause
 *	  Recursively search for DISTINCT Aggref nodes within a clause.
 *
 *	  Returns true if any DISTINCT aggregate found.
 */
bool
contain_distinct_agg_clause(Node *clause)
{
	return contain_distinct_agg_clause_walker(clause, NULL);
}

static bool
contain_distinct_agg_clause_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Aggref))
	{
		if (((Aggref *) node)->aggdistinct)
			return true;		/* abort the tree traversal and return
								 * true */
	}
	return expression_tree_walker(node, contain_distinct_agg_clause_walker, context);
}

/*
 * pull_agg_clause
 *	  Recursively pulls all Aggref nodes from an expression tree.
 *
 *	  Returns list of Aggref nodes found.  Note the nodes themselves are not
 *	  copied, only referenced.
 *
 *	  Note: this also checks for nested aggregates, which are an error.
 */
List *
pull_agg_clause(Node *clause)
{
	List	   *result = NIL;

	pull_agg_clause_walker(clause, &result);
	return result;
}

static bool
pull_agg_clause_walker(Node *node, List **listptr)
{
	if (node == NULL)
		return false;
	if (IsA(node, Aggref))
	{
		*listptr = lappend(*listptr, node);

		/*
		 * Complain if the aggregate's argument contains any aggregates;
		 * nested agg functions are semantically nonsensical.
		 */
		if (contain_agg_clause((Node *) ((Aggref *) node)->target))
			elog(ERROR, "Aggregate function calls may not be nested");

		/*
		 * Having checked that, we need not recurse into the argument.
		 */
		return false;
	}
	return expression_tree_walker(node, pull_agg_clause_walker,
								  (void *) listptr);
}


/*****************************************************************************
 *		Support for expressions returning sets
 *****************************************************************************/

/*
 * expression_returns_set
 *	  Test whether an expression returns a set result.
 *
 * Because we use expression_tree_walker(), this can also be applied to
 * whole targetlists; it'll produce TRUE if any one of the tlist items
 * returns a set.
 */
bool
expression_returns_set(Node *clause)
{
	return expression_returns_set_walker(clause, NULL);
}

static bool
expression_returns_set_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, FuncExpr))
	{
		FuncExpr   *expr = (FuncExpr *) node;

		if (expr->funcretset)
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, OpExpr))
	{
		OpExpr   *expr = (OpExpr *) node;

		if (expr->opretset)
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, DistinctExpr))
	{
		DistinctExpr   *expr = (DistinctExpr *) node;

		if (expr->opretset)
			return true;
		/* else fall through to check args */
	}

	/* Avoid recursion for some cases that can't return a set */
	if (IsA(node, BoolExpr))
		return false;
	if (IsA(node, Aggref))
		return false;
	if (IsA(node, SubLink))
		return false;
	if (IsA(node, SubPlan))
		return false;

	return expression_tree_walker(node, expression_returns_set_walker,
								  context);
}

/*****************************************************************************
 *		Subplan clause manipulation
 *****************************************************************************/

/*
 * contain_subplans
 *	  Recursively search for subplan nodes within a clause.
 *
 * If we see a SubLink node, we will return TRUE.  This is only possible if
 * the expression tree hasn't yet been transformed by subselect.c.  We do not
 * know whether the node will produce a true subplan or just an initplan,
 * but we make the conservative assumption that it will be a subplan.
 *
 * Returns true if any subplan found.
 */
bool
contain_subplans(Node *clause)
{
	return contain_subplans_walker(clause, NULL);
}

static bool
contain_subplans_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, SubPlan) ||
		IsA(node, SubLink))
		return true;			/* abort the tree traversal and return
								 * true */
	return expression_tree_walker(node, contain_subplans_walker, context);
}

/*
 * pull_subplans
 *	  Recursively pulls all subplans from an expression tree.
 *
 *	  Returns list of SubPlan nodes found.  Note the nodes themselves
 *	  are not copied, only referenced.
 */
List *
pull_subplans(Node *clause)
{
	List	   *result = NIL;

	pull_subplans_walker(clause, &result);
	return result;
}

static bool
pull_subplans_walker(Node *node, List **listptr)
{
	if (node == NULL)
		return false;
	if (is_subplan(node))
	{
		*listptr = lappend(*listptr, node);
		/* fall through to check args to subplan */
	}
	return expression_tree_walker(node, pull_subplans_walker,
								  (void *) listptr);
}

/*
 * check_subplans_for_ungrouped_vars
 *		Check for subplans that are being passed ungrouped variables as
 *		parameters; generate an error message if any are found.
 *
 * In most contexts, ungrouped variables will be detected by the parser (see
 * parse_agg.c, check_ungrouped_columns()). But that routine currently does
 * not check subplans, because the necessary info is not computed until the
 * planner runs.  So we do it here, after we have processed sublinks into
 * subplans.  This ought to be cleaned up someday.
 *
 * A deficiency in this scheme is that any outer reference var must be
 * grouped by itself; we don't recognize groupable expressions within
 * subselects.	For example, consider
 *		SELECT
 *			(SELECT x FROM bar where y = (foo.a + foo.b))
 *		FROM foo
 *		GROUP BY a + b;
 * This query will be rejected although it could be allowed.
 */
void
check_subplans_for_ungrouped_vars(Query *query)
{
	check_subplans_for_ungrouped_vars_context context;
	List	   *gl;

	context.query = query;

	/*
	 * Build a list of the acceptable GROUP BY expressions for use in the
	 * walker (to avoid repeated scans of the targetlist within the
	 * recursive routine).
	 */
	context.groupClauses = NIL;
	foreach(gl, query->groupClause)
	{
		GroupClause *grpcl = lfirst(gl);
		Node	   *expr;

		expr = get_sortgroupclause_expr(grpcl, query->targetList);
		context.groupClauses = lcons(expr, context.groupClauses);
	}

	/*
	 * Recursively scan the targetlist and the HAVING clause. WHERE and
	 * JOIN/ON conditions are not examined, since they are evaluated
	 * before grouping.
	 */
	check_subplans_for_ungrouped_vars_walker((Node *) query->targetList,
											 &context);
	check_subplans_for_ungrouped_vars_walker(query->havingQual,
											 &context);

	freeList(context.groupClauses);
}

static bool
check_subplans_for_ungrouped_vars_walker(Node *node,
					  check_subplans_for_ungrouped_vars_context *context)
{
	List	   *gl;

	if (node == NULL)
		return false;
	if (IsA(node, Const) ||
		IsA(node, Param))
		return false;			/* constants are always acceptable */

	/*
	 * If we find an aggregate function, do not recurse into its
	 * arguments.  Subplans invoked within aggregate calls are allowed to
	 * receive ungrouped variables.  (This test and the next one should
	 * match the logic in parse_agg.c's check_ungrouped_columns().)
	 */
	if (IsA(node, Aggref))
		return false;

	/*
	 * Check to see if subexpression as a whole matches any GROUP BY item.
	 * We need to do this at every recursion level so that we recognize
	 * GROUPed-BY expressions before reaching variables within them.
	 */
	foreach(gl, context->groupClauses)
	{
		if (equal(node, lfirst(gl)))
			return false;		/* acceptable, do not descend more */
	}

	/*
	 * We can ignore Vars other than in subplan args lists, since the
	 * parser already checked 'em.
	 */
	if (is_subplan(node))
	{
		/*
		 * The args list of the subplan node represents attributes from
		 * outside passed into the sublink.
		 */
		List	   *t;

		foreach(t, ((SubPlan *) node)->args)
		{
			Node	   *thisarg = lfirst(t);
			Var		   *var;
			bool		contained_in_group_clause;

			/*
			 * We do not care about args that are not local variables;
			 * params or outer-level vars are not our responsibility to
			 * check.  (The outer-level query passing them to us needs to
			 * worry, instead.)
			 */
			if (!IsA(thisarg, Var))
				continue;
			var = (Var *) thisarg;
			if (var->varlevelsup > 0)
				continue;

			/*
			 * Else, see if it is a grouping column.
			 */
			contained_in_group_clause = false;
			foreach(gl, context->groupClauses)
			{
				if (equal(thisarg, lfirst(gl)))
				{
					contained_in_group_clause = true;
					break;
				}
			}

			if (!contained_in_group_clause)
			{
				/* Found an ungrouped argument.  Complain. */
				RangeTblEntry *rte;
				char	   *attname;

				Assert(var->varno > 0 &&
					 (int) var->varno <= length(context->query->rtable));
				rte = rt_fetch(var->varno, context->query->rtable);
				attname = get_rte_attribute_name(rte, var->varattno);
				elog(ERROR, "Sub-SELECT uses un-GROUPed attribute %s.%s from outer query",
					 rte->eref->aliasname, attname);
			}
		}
	}
	return expression_tree_walker(node,
								check_subplans_for_ungrouped_vars_walker,
								  (void *) context);
}


/*****************************************************************************
 *		Check clauses for mutable functions
 *****************************************************************************/

/*
 * contain_mutable_functions
 *	  Recursively search for mutable functions within a clause.
 *
 * Returns true if any mutable function (or operator implemented by a
 * mutable function) is found.	This test is needed so that we don't
 * mistakenly think that something like "WHERE random() < 0.5" can be treated
 * as a constant qualification.
 *
 * XXX we do not examine sublinks/subplans to see if they contain uses of
 * mutable functions.  It's not real clear if that is correct or not...
 */
bool
contain_mutable_functions(Node *clause)
{
	return contain_mutable_functions_walker(clause, NULL);
}

static bool
contain_mutable_functions_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, FuncExpr))
	{
		FuncExpr   *expr = (FuncExpr *) node;

		if (func_volatile(expr->funcid) != PROVOLATILE_IMMUTABLE)
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, OpExpr))
	{
		OpExpr   *expr = (OpExpr *) node;

		if (op_volatile(expr->opno) != PROVOLATILE_IMMUTABLE)
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, DistinctExpr))
	{
		DistinctExpr   *expr = (DistinctExpr *) node;

		if (op_volatile(expr->opno) != PROVOLATILE_IMMUTABLE)
			return true;
		/* else fall through to check args */
	}
	return expression_tree_walker(node, contain_mutable_functions_walker,
								  context);
}


/*****************************************************************************
 *		Check clauses for volatile functions
 *****************************************************************************/

/*
 * contain_volatile_functions
 *	  Recursively search for volatile functions within a clause.
 *
 * Returns true if any volatile function (or operator implemented by a
 * volatile function) is found. This test prevents invalid conversions
 * of volatile expressions into indexscan quals.
 *
 * XXX we do not examine sublinks/subplans to see if they contain uses of
 * volatile functions.	It's not real clear if that is correct or not...
 */
bool
contain_volatile_functions(Node *clause)
{
	return contain_volatile_functions_walker(clause, NULL);
}

static bool
contain_volatile_functions_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, FuncExpr))
	{
		FuncExpr   *expr = (FuncExpr *) node;

		if (func_volatile(expr->funcid) == PROVOLATILE_VOLATILE)
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, OpExpr))
	{
		OpExpr   *expr = (OpExpr *) node;

		if (op_volatile(expr->opno) == PROVOLATILE_VOLATILE)
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, DistinctExpr))
	{
		DistinctExpr   *expr = (DistinctExpr *) node;

		if (op_volatile(expr->opno) == PROVOLATILE_VOLATILE)
			return true;
		/* else fall through to check args */
	}
	return expression_tree_walker(node, contain_volatile_functions_walker,
								  context);
}


/*****************************************************************************
 *		Check clauses for nonstrict functions
 *****************************************************************************/

/*
 * contain_nonstrict_functions
 *	  Recursively search for nonstrict functions within a clause.
 *
 * Returns true if any nonstrict construct is found --- ie, anything that
 * could produce non-NULL output with a NULL input.
 *
 * XXX we do not examine sublinks/subplans to see if they contain uses of
 * nonstrict functions.	It's not real clear if that is correct or not...
 * for the current usage it does not matter, since inline_function()
 * rejects cases with sublinks.
 */
bool
contain_nonstrict_functions(Node *clause)
{
	return contain_nonstrict_functions_walker(clause, NULL);
}

static bool
contain_nonstrict_functions_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, FuncExpr))
	{
		FuncExpr   *expr = (FuncExpr *) node;

		if (!func_strict(expr->funcid))
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, OpExpr))
	{
		OpExpr   *expr = (OpExpr *) node;

		if (!op_strict(expr->opno))
			return true;
		/* else fall through to check args */
	}
	if (IsA(node, DistinctExpr))
	{
		/* IS DISTINCT FROM is inherently non-strict */
		return true;
	}
	if (IsA(node, BoolExpr))
	{
		BoolExpr   *expr = (BoolExpr *) node;

		switch (expr->boolop)
		{
			case OR_EXPR:
			case AND_EXPR:
				/* OR, AND are inherently non-strict */
				return true;
			default:
				break;
		}
	}
	if (IsA(node, CaseExpr))
		return true;
	if (IsA(node, NullTest))
		return true;
	if (IsA(node, BooleanTest))
		return true;
	return expression_tree_walker(node, contain_nonstrict_functions_walker,
								  context);
}


/*****************************************************************************
 *		Check for "pseudo-constant" clauses
 *****************************************************************************/

/*
 * is_pseudo_constant_clause
 *	  Detect whether a clause is "constant", ie, it contains no variables
 *	  of the current query level and no uses of volatile functions.
 *	  Such a clause is not necessarily a true constant: it can still contain
 *	  Params and outer-level Vars, not to mention functions whose results
 *	  may vary from one statement to the next.	However, the clause's value
 *	  will be constant over any one scan of the current query, so it can be
 *	  used as an indexscan key or (if a top-level qual) can be pushed up to
 *	  become a gating qual.
 */
bool
is_pseudo_constant_clause(Node *clause)
{
	/*
	 * We could implement this check in one recursive scan.  But since the
	 * check for volatile functions is both moderately expensive and
	 * unlikely to fail, it seems better to look for Vars first and only
	 * check for volatile functions if we find no Vars.
	 */
	if (!contain_var_clause(clause) &&
		!contain_volatile_functions(clause))
		return true;
	return false;
}

/*
 * pull_constant_clauses
 *		Scan through a list of qualifications and separate "constant" quals
 *		from those that are not.
 *
 * Returns a list of the pseudo-constant clauses in constantQual and the
 * remaining quals as the return value.
 */
List *
pull_constant_clauses(List *quals, List **constantQual)
{
	List	   *constqual = NIL;
	List	   *restqual = NIL;
	List	   *q;

	foreach(q, quals)
	{
		Node	   *qual = (Node *) lfirst(q);

		if (is_pseudo_constant_clause(qual))
			constqual = lappend(constqual, qual);
		else
			restqual = lappend(restqual, qual);
	}
	*constantQual = constqual;
	return restqual;
}


/*****************************************************************************
 *		Tests on clauses of queries
 *
 * Possibly this code should go someplace else, since this isn't quite the
 * same meaning of "clause" as is used elsewhere in this module.  But I can't
 * think of a better place for it...
 *****************************************************************************/

/*
 * Test whether a sort/group reference value appears in the given list of
 * SortClause (or GroupClause) nodes.
 *
 * Because GroupClause is typedef'd as SortClause, either kind of
 * node list can be passed without casting.
 */
static bool
sortgroupref_is_present(Index sortgroupref, List *clauselist)
{
	List	   *clause;

	foreach(clause, clauselist)
	{
		SortClause *scl = (SortClause *) lfirst(clause);

		if (scl->tleSortGroupRef == sortgroupref)
			return true;
	}
	return false;
}

/*
 * Test whether a query uses DISTINCT ON, ie, has a distinct-list that is
 * not the same as the set of output columns.
 */
bool
has_distinct_on_clause(Query *query)
{
	List	   *targetList;

	/* Is there a DISTINCT clause at all? */
	if (query->distinctClause == NIL)
		return false;

	/*
	 * If the DISTINCT list contains all the nonjunk targetlist items, and
	 * nothing else (ie, no junk tlist items), then it's a simple
	 * DISTINCT, else it's DISTINCT ON.  We do not require the lists to be
	 * in the same order (since the parser may have adjusted the DISTINCT
	 * clause ordering to agree with ORDER BY).  Furthermore, a
	 * non-DISTINCT junk tlist item that is in the sortClause is also
	 * evidence of DISTINCT ON, since we don't allow ORDER BY on junk
	 * tlist items when plain DISTINCT is used.
	 *
	 * This code assumes that the DISTINCT list is valid, ie, all its entries
	 * match some entry of the tlist.
	 */
	foreach(targetList, query->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(targetList);
		Index		ressortgroupref = tle->resdom->ressortgroupref;

		if (ressortgroupref == 0)
		{
			if (tle->resdom->resjunk)
				continue;		/* we can ignore unsorted junk cols */
			return true;		/* definitely not in DISTINCT list */
		}
		if (sortgroupref_is_present(ressortgroupref, query->distinctClause))
		{
			if (tle->resdom->resjunk)
				return true;	/* junk TLE in DISTINCT means DISTINCT ON */
			/* else this TLE is okay, keep looking */
		}
		else
		{
			/* This TLE is not in DISTINCT list */
			if (!tle->resdom->resjunk)
				return true;	/* non-junk, non-DISTINCT, so DISTINCT ON */
			if (sortgroupref_is_present(ressortgroupref, query->sortClause))
				return true;	/* sorted, non-distinct junk */
			/* unsorted junk is okay, keep looking */
		}
	}
	/* It's a simple DISTINCT */
	return false;
}


/*****************************************************************************
 *																			 *
 *		General clause-manipulating routines								 *
 *																			 *
 *****************************************************************************/

/*
 * clause_get_relids_vars
 *	  Retrieves distinct relids and vars appearing within a clause.
 *
 * '*relids' is set to an integer list of all distinct "varno"s appearing
 *		in Vars within the clause.
 * '*vars' is set to a list of all distinct Vars appearing within the clause.
 *		Var nodes are considered distinct if they have different varno
 *		or varattno values.  If there are several occurrences of the same
 *		varno/varattno, you get a randomly chosen one...
 *
 * Note that upper-level vars are ignored, since they normally will
 * become Params with respect to this query level.
 */
void
clause_get_relids_vars(Node *clause, Relids *relids, List **vars)
{
	List	   *clvars = pull_var_clause(clause, false);
	List	   *varno_list = NIL;
	List	   *var_list = NIL;
	List	   *i;

	foreach(i, clvars)
	{
		Var		   *var = (Var *) lfirst(i);
		List	   *vi;

		if (!intMember(var->varno, varno_list))
			varno_list = lconsi(var->varno, varno_list);
		foreach(vi, var_list)
		{
			Var		   *in_list = (Var *) lfirst(vi);

			if (in_list->varno == var->varno &&
				in_list->varattno == var->varattno)
				break;
		}
		if (vi == NIL)
			var_list = lcons(var, var_list);
	}
	freeList(clvars);

	*relids = varno_list;
	*vars = var_list;
}

/*
 * NumRelids
 *		(formerly clause_relids)
 *
 * Returns the number of different relations referenced in 'clause'.
 */
int
NumRelids(Node *clause)
{
	List	   *varno_list = pull_varnos(clause);
	int			result = length(varno_list);

	freeList(varno_list);
	return result;
}

/*
 * CommuteClause: commute a binary operator clause
 *
 * XXX the clause is destructively modified!
 */
void
CommuteClause(OpExpr *clause)
{
	Oid			opoid;
	Node	   *temp;

	if (!is_opclause(clause) ||
		length(clause->args) != 2)
		elog(ERROR, "CommuteClause: applied to non-binary-operator clause");

	opoid = get_commutator(clause->opno);

	if (!OidIsValid(opoid))
		elog(ERROR, "CommuteClause: no commutator for operator %u",
			 clause->opno);

	/*
	 * modify the clause in-place!
	 */
	clause->opno = opoid;
	clause->opfuncid = InvalidOid;
	/* opresulttype and opretset are assumed not to change */

	temp = lfirst(clause->args);
	lfirst(clause->args) = lsecond(clause->args);
	lsecond(clause->args) = temp;
}


/*--------------------
 * eval_const_expressions
 *
 * Reduce any recognizably constant subexpressions of the given
 * expression tree, for example "2 + 2" => "4".  More interestingly,
 * we can reduce certain boolean expressions even when they contain
 * non-constant subexpressions: "x OR true" => "true" no matter what
 * the subexpression x is.	(XXX We assume that no such subexpression
 * will have important side-effects, which is not necessarily a good
 * assumption in the presence of user-defined functions; do we need a
 * pg_proc flag that prevents discarding the execution of a function?)
 *
 * We do understand that certain functions may deliver non-constant
 * results even with constant inputs, "nextval()" being the classic
 * example.  Functions that are not marked "immutable" in pg_proc
 * will not be pre-evaluated here, although we will reduce their
 * arguments as far as possible.
 *
 * We assume that the tree has already been type-checked and contains
 * only operators and functions that are reasonable to try to execute.
 *--------------------
 */
Node *
eval_const_expressions(Node *node)
{
	/*
	 * The context for the mutator is a list of SQL functions being
	 * recursively simplified, so we start with an empty list.
	 */
	return eval_const_expressions_mutator(node, NIL);
}

static Node *
eval_const_expressions_mutator(Node *node, List *active_fns)
{
	if (node == NULL)
		return NULL;
	if (IsA(node, FuncExpr))
	{
		FuncExpr   *expr = (FuncExpr *) node;
		List	   *args;
		Expr	   *simple;
		FuncExpr   *newexpr;

		/*
		 * Reduce constants in the FuncExpr's arguments.  We know args is
		 * either NIL or a List node, so we can call
		 * expression_tree_mutator directly rather than recursing to self.
		 */
		args = (List *) expression_tree_mutator((Node *) expr->args,
										  eval_const_expressions_mutator,
												(void *) active_fns);
		/*
		 * Code for op/func reduction is pretty bulky, so split it out
		 * as a separate function.
		 */
		simple = simplify_function(expr->funcid, args, true, active_fns);
		if (simple)				/* successfully simplified it */
			return (Node *) simple;
		/*
		 * The expression cannot be simplified any further, so build and
		 * return a replacement FuncExpr node using the possibly-simplified
		 * arguments.
		 */
		newexpr = makeNode(FuncExpr);
		newexpr->funcid = expr->funcid;
		newexpr->funcresulttype = expr->funcresulttype;
		newexpr->funcretset = expr->funcretset;
		newexpr->funcformat = expr->funcformat;
		newexpr->args = args;
		return (Node *) newexpr;
	}
	if (IsA(node, OpExpr))
	{
		OpExpr	   *expr = (OpExpr *) node;
		List	   *args;
		Expr	   *simple;
		OpExpr	   *newexpr;

		/*
		 * Reduce constants in the OpExpr's arguments.  We know args is
		 * either NIL or a List node, so we can call
		 * expression_tree_mutator directly rather than recursing to self.
		 */
		args = (List *) expression_tree_mutator((Node *) expr->args,
										  eval_const_expressions_mutator,
												(void *) active_fns);
		/*
		 * Need to get OID of underlying function.  Okay to scribble on
		 * input to this extent.
		 */
		set_opfuncid(expr);
		/*
		 * Code for op/func reduction is pretty bulky, so split it out
		 * as a separate function.
		 */
		simple = simplify_function(expr->opfuncid, args, true, active_fns);
		if (simple)				/* successfully simplified it */
			return (Node *) simple;
		/*
		 * The expression cannot be simplified any further, so build and
		 * return a replacement OpExpr node using the possibly-simplified
		 * arguments.
		 */
		newexpr = makeNode(OpExpr);
		newexpr->opno = expr->opno;
		newexpr->opfuncid = expr->opfuncid;
		newexpr->opresulttype = expr->opresulttype;
		newexpr->opretset = expr->opretset;
		newexpr->args = args;
		return (Node *) newexpr;
	}
	if (IsA(node, DistinctExpr))
	{
		DistinctExpr *expr = (DistinctExpr *) node;
		List	   *args;
		List	   *arg;
		bool		has_null_input = false;
		bool		all_null_input = true;
		bool		has_nonconst_input = false;
		Expr	   *simple;
		DistinctExpr *newexpr;

		/*
		 * Reduce constants in the DistinctExpr's arguments.  We know args is
		 * either NIL or a List node, so we can call
		 * expression_tree_mutator directly rather than recursing to self.
		 */
		args = (List *) expression_tree_mutator((Node *) expr->args,
										  eval_const_expressions_mutator,
												(void *) active_fns);

		/*
		 * We must do our own check for NULLs because
		 * DistinctExpr has different results for NULL input
		 * than the underlying operator does.
		 */
		foreach(arg, args)
		{
			if (IsA(lfirst(arg), Const))
			{
				has_null_input |= ((Const *) lfirst(arg))->constisnull;
				all_null_input &= ((Const *) lfirst(arg))->constisnull;
			}
			else
				has_nonconst_input = true;
		}

		/* all constants? then can optimize this out */
		if (!has_nonconst_input)
		{
			/* all nulls? then not distinct */
			if (all_null_input)
				return MAKEBOOLCONST(false, false);

			/* one null? then distinct */
			if (has_null_input)
				return MAKEBOOLCONST(true, false);

			/* otherwise try to evaluate the '=' operator */
			/* (NOT okay to try to inline it, though!) */

			/*
			 * Need to get OID of underlying function.  Okay to scribble on
			 * input to this extent.
			 */
			set_opfuncid((OpExpr *) expr); /* rely on struct equivalence */
			/*
			 * Code for op/func reduction is pretty bulky, so split it out
			 * as a separate function.
			 */
			simple = simplify_function(expr->opfuncid, args,
									   false, active_fns);
			if (simple)			/* successfully simplified it */
			{
				/*
				 * Since the underlying operator is "=", must negate its
				 * result
				 */
				Const  *csimple = (Const *) simple;

				Assert(IsA(csimple, Const));
				csimple->constvalue =
					BoolGetDatum(!DatumGetBool(csimple->constvalue));
				return (Node *) csimple;
			}
		}

		/*
		 * The expression cannot be simplified any further, so build and
		 * return a replacement DistinctExpr node using the
		 * possibly-simplified arguments.
		 */
		newexpr = makeNode(DistinctExpr);
		newexpr->opno = expr->opno;
		newexpr->opfuncid = expr->opfuncid;
		newexpr->opresulttype = expr->opresulttype;
		newexpr->opretset = expr->opretset;
		newexpr->args = args;
		return (Node *) newexpr;
	}
	if (IsA(node, BoolExpr))
	{
		BoolExpr   *expr = (BoolExpr *) node;
		List	   *args;
		Const	   *const_input;

		/*
		 * Reduce constants in the BoolExpr's arguments.  We know args is
		 * either NIL or a List node, so we can call
		 * expression_tree_mutator directly rather than recursing to self.
		 */
		args = (List *) expression_tree_mutator((Node *) expr->args,
										  eval_const_expressions_mutator,
												(void *) active_fns);

		switch (expr->boolop)
		{
			case OR_EXPR:
				{
					/*----------
					 * OR arguments are handled as follows:
					 *	non constant: keep
					 *	FALSE: drop (does not affect result)
					 *	TRUE: force result to TRUE
					 *	NULL: keep only one
					 * We keep one NULL input because ExecEvalOr returns NULL
					 * when no input is TRUE and at least one is NULL.
					 *----------
					 */
					List	   *newargs = NIL;
					List	   *arg;
					bool		haveNull = false;
					bool		forceTrue = false;

					foreach(arg, args)
					{
						if (!IsA(lfirst(arg), Const))
						{
							newargs = lappend(newargs, lfirst(arg));
							continue;
						}
						const_input = (Const *) lfirst(arg);
						if (const_input->constisnull)
							haveNull = true;
						else if (DatumGetBool(const_input->constvalue))
							forceTrue = true;
						/* otherwise, we can drop the constant-false input */
					}

					/*
					 * We could return TRUE before falling out of the
					 * loop, but this coding method will be easier to
					 * adapt if we ever add a notion of non-removable
					 * functions. We'd need to check all the inputs for
					 * non-removability.
					 */
					if (forceTrue)
						return MAKEBOOLCONST(true, false);
					if (haveNull)
						newargs = lappend(newargs, MAKEBOOLCONST(false, true));
					/* If all the inputs are FALSE, result is FALSE */
					if (newargs == NIL)
						return MAKEBOOLCONST(false, false);
					/* If only one nonconst-or-NULL input, it's the result */
					if (lnext(newargs) == NIL)
						return (Node *) lfirst(newargs);
					/* Else we still need an OR node */
					return (Node *) make_orclause(newargs);
				}
			case AND_EXPR:
				{
					/*----------
					 * AND arguments are handled as follows:
					 *	non constant: keep
					 *	TRUE: drop (does not affect result)
					 *	FALSE: force result to FALSE
					 *	NULL: keep only one
					 * We keep one NULL input because ExecEvalAnd returns NULL
					 * when no input is FALSE and at least one is NULL.
					 *----------
					 */
					List	   *newargs = NIL;
					List	   *arg;
					bool		haveNull = false;
					bool		forceFalse = false;

					foreach(arg, args)
					{
						if (!IsA(lfirst(arg), Const))
						{
							newargs = lappend(newargs, lfirst(arg));
							continue;
						}
						const_input = (Const *) lfirst(arg);
						if (const_input->constisnull)
							haveNull = true;
						else if (!DatumGetBool(const_input->constvalue))
							forceFalse = true;
						/* otherwise, we can drop the constant-true input */
					}

					/*
					 * We could return FALSE before falling out of the
					 * loop, but this coding method will be easier to
					 * adapt if we ever add a notion of non-removable
					 * functions. We'd need to check all the inputs for
					 * non-removability.
					 */
					if (forceFalse)
						return MAKEBOOLCONST(false, false);
					if (haveNull)
						newargs = lappend(newargs, MAKEBOOLCONST(false, true));
					/* If all the inputs are TRUE, result is TRUE */
					if (newargs == NIL)
						return MAKEBOOLCONST(true, false);
					/* If only one nonconst-or-NULL input, it's the result */
					if (lnext(newargs) == NIL)
						return (Node *) lfirst(newargs);
					/* Else we still need an AND node */
					return (Node *) make_andclause(newargs);
				}
			case NOT_EXPR:
				Assert(length(args) == 1);
				if (IsA(lfirst(args), Const))
				{
					const_input = (Const *) lfirst(args);
					/* NOT NULL => NULL */
					if (const_input->constisnull)
						return MAKEBOOLCONST(false, true);
					/* otherwise pretty easy */
					return MAKEBOOLCONST(!DatumGetBool(const_input->constvalue),
										 false);
				}
				/* Else we still need a NOT node */
				return (Node *) make_notclause(lfirst(args));
			default:
				elog(ERROR, "eval_const_expressions: unexpected boolop %d",
					 (int) expr->boolop);
				break;
		}
	}
	if (IsA(node, SubPlan))
	{
		/*
		 * Return a SubPlan unchanged --- too late to do anything
		 * with it.
		 *
		 * XXX should we elog() here instead?  Probably this routine
		 * should never be invoked after SubPlan creation.
		 */
		return node;
	}
	if (IsA(node, RelabelType))
	{
		/*
		 * If we can simplify the input to a constant, then we don't need
		 * the RelabelType node anymore: just change the type field of the
		 * Const node.	Otherwise, must copy the RelabelType node.
		 */
		RelabelType *relabel = (RelabelType *) node;
		Node	   *arg;

		arg = eval_const_expressions_mutator((Node *) relabel->arg,
											 active_fns);

		/*
		 * If we find stacked RelabelTypes (eg, from foo :: int :: oid) we
		 * can discard all but the top one.
		 */
		while (arg && IsA(arg, RelabelType))
			arg = (Node *) ((RelabelType *) arg)->arg;

		if (arg && IsA(arg, Const))
		{
			Const	   *con = (Const *) arg;

			con->consttype = relabel->resulttype;

			/*
			 * relabel's resulttypmod is discarded, which is OK for now;
			 * if the type actually needs a runtime length coercion then
			 * there should be a function call to do it just above this
			 * node.
			 */
			return (Node *) con;
		}
		else
		{
			RelabelType *newrelabel = makeNode(RelabelType);

			newrelabel->arg = (Expr *) arg;
			newrelabel->resulttype = relabel->resulttype;
			newrelabel->resulttypmod = relabel->resulttypmod;
			return (Node *) newrelabel;
		}
	}
	if (IsA(node, CaseExpr))
	{

		/*----------
		 * CASE expressions can be simplified if there are constant
		 * condition clauses:
		 *		FALSE (or NULL): drop the alternative
		 *		TRUE: drop all remaining alternatives
		 * If the first non-FALSE alternative is a constant TRUE, we can
		 * simplify the entire CASE to that alternative's expression.
		 * If there are no non-FALSE alternatives, we simplify the entire
		 * CASE to the default result (ELSE result).
		 *----------
		 */
		CaseExpr   *caseexpr = (CaseExpr *) node;
		CaseExpr   *newcase;
		List	   *newargs = NIL;
		Node	   *defresult;
		Const	   *const_input;
		List	   *arg;

		foreach(arg, caseexpr->args)
		{
			/* Simplify this alternative's condition and result */
			CaseWhen   *casewhen = (CaseWhen *)
			expression_tree_mutator((Node *) lfirst(arg),
									eval_const_expressions_mutator,
									(void *) active_fns);

			Assert(IsA(casewhen, CaseWhen));
			if (casewhen->expr == NULL ||
				!IsA(casewhen->expr, Const))
			{
				newargs = lappend(newargs, casewhen);
				continue;
			}
			const_input = (Const *) casewhen->expr;
			if (const_input->constisnull ||
				!DatumGetBool(const_input->constvalue))
				continue;		/* drop alternative with FALSE condition */

			/*
			 * Found a TRUE condition.	If it's the first (un-dropped)
			 * alternative, the CASE reduces to just this alternative.
			 */
			if (newargs == NIL)
				return (Node *) casewhen->result;

			/*
			 * Otherwise, add it to the list, and drop all the rest.
			 */
			newargs = lappend(newargs, casewhen);
			break;
		}

		/* Simplify the default result */
		defresult = eval_const_expressions_mutator((Node *) caseexpr->defresult,
												   active_fns);

		/*
		 * If no non-FALSE alternatives, CASE reduces to the default
		 * result
		 */
		if (newargs == NIL)
			return defresult;
		/* Otherwise we need a new CASE node */
		newcase = makeNode(CaseExpr);
		newcase->casetype = caseexpr->casetype;
		newcase->arg = NULL;
		newcase->args = newargs;
		newcase->defresult = (Expr *) defresult;
		return (Node *) newcase;
	}

	/*
	 * For any node type not handled above, we recurse using
	 * expression_tree_mutator, which will copy the node unchanged but try
	 * to simplify its arguments (if any) using this routine. For example:
	 * we cannot eliminate an ArrayRef node, but we might be able to
	 * simplify constant expressions in its subscripts.
	 */
	return expression_tree_mutator(node, eval_const_expressions_mutator,
								   (void *) active_fns);
}

/*
 * Subroutine for eval_const_expressions: try to simplify a function call
 * (which might originally have been an operator; we don't care)
 *
 * Inputs are the function OID and the pre-simplified argument list;
 * also a list of already-active inline function expansions.
 *
 * Returns a simplified expression if successful, or NULL if cannot
 * simplify the function call.
 */
static Expr *
simplify_function(Oid funcid, List *args, bool allow_inline, List *active_fns)
{
	HeapTuple	func_tuple;
	Expr	   *newexpr;

	/*
	 * We have two strategies for simplification: either execute the function
	 * to deliver a constant result, or expand in-line the body of the
	 * function definition (which only works for simple SQL-language
	 * functions, but that is a common case).  In either case we need access
	 * to the function's pg_proc tuple, so fetch it just once to use in both
	 * attempts.
	 */
	func_tuple = SearchSysCache(PROCOID,
								ObjectIdGetDatum(funcid),
								0, 0, 0);
	if (!HeapTupleIsValid(func_tuple))
		elog(ERROR, "Function OID %u does not exist", funcid);

	newexpr = evaluate_function(funcid, args, func_tuple);

	if (!newexpr && allow_inline)
		newexpr = inline_function(funcid, args, func_tuple, active_fns);

	ReleaseSysCache(func_tuple);

	return newexpr;
}

/*
 * evaluate_function: try to pre-evaluate a function call
 *
 * We can do this if the function is strict and has any constant-null inputs
 * (just return a null constant), or if the function is immutable and has all
 * constant inputs (call it and return the result as a Const node).
 *
 * Returns a simplified expression if successful, or NULL if cannot
 * simplify the function.
 */
static Expr *
evaluate_function(Oid funcid, List *args, HeapTuple func_tuple)
{
	Form_pg_proc funcform = (Form_pg_proc) GETSTRUCT(func_tuple);
	Oid			result_typeid = funcform->prorettype;
	int16		resultTypLen;
	bool		resultTypByVal;
	bool		has_nonconst_input = false;
	bool		has_null_input = false;
	FuncExpr   *newexpr;
	ExprState  *newexprstate;
	EState	   *estate;
	MemoryContext oldcontext;
	Datum		const_val;
	bool		const_is_null;
	List	   *arg;

	/*
	 * Can't simplify if it returns a set.
	 */
	if (funcform->proretset)
		return NULL;

	/*
	 * Check for constant inputs and especially constant-NULL inputs.
	 */
	foreach(arg, args)
	{
		if (IsA(lfirst(arg), Const))
			has_null_input |= ((Const *) lfirst(arg))->constisnull;
		else
			has_nonconst_input = true;
	}

	/*
	 * If the function is strict and has a constant-NULL input, it will
	 * never be called at all, so we can replace the call by a NULL
	 * constant, even if there are other inputs that aren't constant,
	 * and even if the function is not otherwise immutable.
	 */
	if (funcform->proisstrict && has_null_input)
		return (Expr *) makeNullConst(result_typeid);

	/*
	 * Otherwise, can simplify only if the function is immutable and
	 * all inputs are constants. (For a non-strict function, constant NULL
	 * inputs are treated the same as constant non-NULL inputs.)
	 */
	if (funcform->provolatile != PROVOLATILE_IMMUTABLE ||
		has_nonconst_input)
		return NULL;

	/*
	 * OK, looks like we can simplify this operator/function.
	 *
	 * We use the executor's routine ExecEvalExpr() to avoid duplication of
	 * code and ensure we get the same result as the executor would get.
	 * To use the executor, we need an EState.
	 */
	estate = CreateExecutorState();

	/* We can use the estate's working context to avoid memory leaks. */
	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	/*
	 * Build a new FuncExpr node containing the already-simplified arguments.
	 */
	newexpr = makeNode(FuncExpr);
	newexpr->funcid = funcid;
	newexpr->funcresulttype = result_typeid;
	newexpr->funcretset = false;
	newexpr->funcformat = COERCE_EXPLICIT_CALL;	/* doesn't matter */
	newexpr->args = args;

	/*
	 * Prepare it for execution.
	 */
	newexprstate = ExecPrepareExpr((Expr *) newexpr, estate);

	/*
	 * And evaluate it.
	 *
	 * It is OK to use a default econtext because none of the
	 * ExecEvalExpr() code used in this situation will use econtext.  That
	 * might seem fortuitous, but it's not so unreasonable --- a constant
	 * expression does not depend on context, by definition, n'est ce pas?
	 */
	const_val = ExecEvalExprSwitchContext(newexprstate,
										  GetPerTupleExprContext(estate),
										  &const_is_null, NULL);

	/* Get info needed about result datatype */
	get_typlenbyval(result_typeid, &resultTypLen, &resultTypByVal);

	/* Get back to outer memory context */
	MemoryContextSwitchTo(oldcontext);

	/* Must copy result out of sub-context used by expression eval */
	if (!const_is_null)
		const_val = datumCopy(const_val, resultTypByVal, resultTypLen);

	/* Release all the junk we just created */
	FreeExecutorState(estate);

	/*
	 * Make the constant result node.
	 */
	return (Expr *) makeConst(result_typeid, resultTypLen,
							  const_val, const_is_null,
							  resultTypByVal);
}

/*
 * inline_function: try to expand a function call inline
 *
 * If the function is a sufficiently simple SQL-language function
 * (just "SELECT expression"), then we can inline it and avoid the rather
 * high per-call overhead of SQL functions.  Furthermore, this can expose
 * opportunities for constant-folding within the function expression.
 *
 * We have to beware of some special cases however.  A directly or
 * indirectly recursive function would cause us to recurse forever,
 * so we keep track of which functions we are already expanding and
 * do not re-expand them.  Also, if a parameter is used more than once
 * in the SQL-function body, we require it not to contain any volatile
 * functions or sublinks --- volatiles might deliver inconsistent answers,
 * and subplans might be unreasonably expensive to evaluate multiple times.
 * We must also beware of changing the volatility or strictness status of
 * functions by inlining them.
 *
 * Returns a simplified expression if successful, or NULL if cannot
 * simplify the function.
 */
static Expr *
inline_function(Oid funcid, List *args, HeapTuple func_tuple,
				List *active_fns)
{
	Form_pg_proc funcform = (Form_pg_proc) GETSTRUCT(func_tuple);
	Oid			result_typeid = funcform->prorettype;
	char		result_typtype;
	char	   *src;
	Datum		tmp;
	bool		isNull;
	MemoryContext oldcxt;
	MemoryContext mycxt;
	StringInfoData stri;
	List	   *raw_parsetree_list;
	List	   *querytree_list;
	Query	   *querytree;
	Node	   *newexpr;
	int		   *usecounts;
	List	   *arg;
	int			i;

	/*
	 * Forget it if the function is not SQL-language or has other
	 * showstopper properties.  (The nargs check is just paranoia.)
	 */
	if (funcform->prolang != SQLlanguageId ||
		funcform->prosecdef ||
		funcform->proretset ||
		funcform->pronargs != length(args))
		return NULL;

	/* Forget it if return type is tuple or void */
	result_typtype = get_typtype(result_typeid);
	if (result_typtype != 'b' &&
		result_typtype != 'd')
		return NULL;

	/* Check for recursive function, and give up trying to expand if so */
	if (intMember(funcid, active_fns))
		return NULL;

	/* Check permission to call function (fail later, if not) */
	if (pg_proc_aclcheck(funcid, GetUserId(), ACL_EXECUTE) != ACLCHECK_OK)
		return NULL;

	/*
	 * Make a temporary memory context, so that we don't leak all the
	 * stuff that parsing might create.
	 */
	mycxt = AllocSetContextCreate(CurrentMemoryContext,
								  "inline_function",
								  ALLOCSET_DEFAULT_MINSIZE,
								  ALLOCSET_DEFAULT_INITSIZE,
								  ALLOCSET_DEFAULT_MAXSIZE);
	oldcxt = MemoryContextSwitchTo(mycxt);

	/* Fetch and parse the function body */
	tmp = SysCacheGetAttr(PROCOID,
						  func_tuple,
						  Anum_pg_proc_prosrc,
						  &isNull);
	if (isNull)
		elog(ERROR, "inline_function: null prosrc for procedure %u",
			 funcid);
	src = DatumGetCString(DirectFunctionCall1(textout, tmp));

	/*
	 * We just do parsing and parse analysis, not rewriting, because
	 * rewriting will not affect SELECT-only queries, which is all that
	 * we care about.  Also, we can punt as soon as we detect more than
	 * one command in the function body.
	 */
	initStringInfo(&stri);
	appendStringInfo(&stri, "%s", src);

	raw_parsetree_list = pg_parse_query(&stri,
										funcform->proargtypes,
										funcform->pronargs);
	if (length(raw_parsetree_list) != 1)
		goto fail;

	querytree_list = parse_analyze(lfirst(raw_parsetree_list), NULL);

	if (length(querytree_list) != 1)
		goto fail;

	querytree = (Query *) lfirst(querytree_list);

	/*
	 * The single command must be a simple "SELECT expression".
	 */
	if (!IsA(querytree, Query) ||
		querytree->commandType != CMD_SELECT ||
		querytree->resultRelation != 0 ||
		querytree->into ||
		querytree->isPortal ||
		querytree->hasAggs ||
		querytree->hasSubLinks ||
		querytree->rtable ||
		querytree->jointree->fromlist ||
		querytree->jointree->quals ||
		querytree->groupClause ||
		querytree->havingQual ||
		querytree->distinctClause ||
		querytree->sortClause ||
		querytree->limitOffset ||
		querytree->limitCount ||
		querytree->setOperations ||
		length(querytree->targetList) != 1)
		goto fail;

	newexpr = (Node *) ((TargetEntry *) lfirst(querytree->targetList))->expr;

	/*
	 * Additional validity checks on the expression.  It mustn't return a
	 * set, and it mustn't be more volatile than the surrounding function
	 * (this is to avoid breaking hacks that involve pretending a function
	 * is immutable when it really ain't).  If the surrounding function is
	 * declared strict, then the expression must contain only strict constructs
	 * and must use all of the function parameters (this is overkill, but
	 * an exact analysis is hard).
	 */
	if (expression_returns_set(newexpr))
		goto fail;

	if (funcform->provolatile == PROVOLATILE_IMMUTABLE &&
		contain_mutable_functions(newexpr))
		goto fail;
	else if (funcform->provolatile == PROVOLATILE_STABLE &&
		contain_volatile_functions(newexpr))
		goto fail;

	if (funcform->proisstrict &&
		contain_nonstrict_functions(newexpr))
		goto fail;

	/*
	 * We may be able to do it; there are still checks on parameter usage
	 * to make, but those are most easily done in combination with the
	 * actual substitution of the inputs.  So start building expression
	 * with inputs substituted.
	 */
	usecounts = (int *) palloc0((funcform->pronargs + 1) * sizeof(int));
	newexpr = substitute_actual_parameters(newexpr, funcform->pronargs,
										   args, usecounts);

	/* Now check for parameter usage */
	i = 0;
	foreach(arg, args)
	{
		Node   *param = lfirst(arg);

		if (usecounts[i] == 0)
		{
			/* Param not used at all: uncool if func is strict */
			if (funcform->proisstrict)
				goto fail;
		}
		else if (usecounts[i] != 1)
		{
			/* Param used multiple times: uncool if volatile or expensive */
			if (contain_volatile_functions(param) ||
				contain_subplans(param))
				goto fail;
		}
		i++;
	}

	/*
	 * Whew --- we can make the substitution.  Copy the modified expression
	 * out of the temporary memory context, and clean up.
	 */
	MemoryContextSwitchTo(oldcxt);

	newexpr = copyObject(newexpr);

	MemoryContextDelete(mycxt);

	/*
	 * Recursively try to simplify the modified expression.  Here we must
	 * add the current function to the context list of active functions.
	 */
	newexpr = eval_const_expressions_mutator(newexpr,
											 lconsi(funcid, active_fns));

	return (Expr *) newexpr;

	/* Here if func is not inlinable: release temp memory and return NULL */
fail:
	MemoryContextSwitchTo(oldcxt);
	MemoryContextDelete(mycxt);

	return NULL;
}

/*
 * Replace Param nodes by appropriate actual parameters
 */
static Node *
substitute_actual_parameters(Node *expr, int nargs, List *args,
							 int *usecounts)
{
	substitute_actual_parameters_context context;

 	context.nargs = nargs;
	context.args = args;
	context.usecounts = usecounts;

	return substitute_actual_parameters_mutator(expr, &context);
}

static Node *
substitute_actual_parameters_mutator(Node *node,
									 substitute_actual_parameters_context *context)
{
	if (node == NULL)
		return NULL;
	if (IsA(node, Param))
	{
		Param	   *param = (Param *) node;

		if (param->paramkind != PARAM_NUM)
			elog(ERROR, "substitute_actual_parameters_mutator: unexpected paramkind");
		if (param->paramid <= 0 || param->paramid > context->nargs)
			elog(ERROR, "substitute_actual_parameters_mutator: unexpected paramid");

		/* Count usage of parameter */
		context->usecounts[param->paramid - 1]++;

		/* Select the appropriate actual arg and replace the Param with it */
		/* We don't need to copy at this time (it'll get done later) */
		return nth(param->paramid - 1, context->args);
	}
	return expression_tree_mutator(node, substitute_actual_parameters_mutator,
								   (void *) context);
}


/*
 * Standard expression-tree walking support
 *
 * We used to have near-duplicate code in many different routines that
 * understood how to recurse through an expression node tree.  That was
 * a pain to maintain, and we frequently had bugs due to some particular
 * routine neglecting to support a particular node type.  In most cases,
 * these routines only actually care about certain node types, and don't
 * care about other types except insofar as they have to recurse through
 * non-primitive node types.  Therefore, we now provide generic tree-walking
 * logic to consolidate the redundant "boilerplate" code.  There are
 * two versions: expression_tree_walker() and expression_tree_mutator().
 */

/*--------------------
 * expression_tree_walker() is designed to support routines that traverse
 * a tree in a read-only fashion (although it will also work for routines
 * that modify nodes in-place but never add/delete/replace nodes).
 * A walker routine should look like this:
 *
 * bool my_walker (Node *node, my_struct *context)
 * {
 *		if (node == NULL)
 *			return false;
 *		// check for nodes that special work is required for, eg:
 *		if (IsA(node, Var))
 *		{
 *			... do special actions for Var nodes
 *		}
 *		else if (IsA(node, ...))
 *		{
 *			... do special actions for other node types
 *		}
 *		// for any node type not specially processed, do:
 *		return expression_tree_walker(node, my_walker, (void *) context);
 * }
 *
 * The "context" argument points to a struct that holds whatever context
 * information the walker routine needs --- it can be used to return data
 * gathered by the walker, too.  This argument is not touched by
 * expression_tree_walker, but it is passed down to recursive sub-invocations
 * of my_walker.  The tree walk is started from a setup routine that
 * fills in the appropriate context struct, calls my_walker with the top-level
 * node of the tree, and then examines the results.
 *
 * The walker routine should return "false" to continue the tree walk, or
 * "true" to abort the walk and immediately return "true" to the top-level
 * caller.	This can be used to short-circuit the traversal if the walker
 * has found what it came for.	"false" is returned to the top-level caller
 * iff no invocation of the walker returned "true".
 *
 * The node types handled by expression_tree_walker include all those
 * normally found in target lists and qualifier clauses during the planning
 * stage.  In particular, it handles List nodes since a cnf-ified qual clause
 * will have List structure at the top level, and it handles TargetEntry nodes
 * so that a scan of a target list can be handled without additional code.
 * (But only the "expr" part of a TargetEntry is examined, unless the walker
 * chooses to process TargetEntry nodes specially.)  Also, RangeTblRef,
 * FromExpr, JoinExpr, and SetOperationStmt nodes are handled, so that query
 * jointrees and setOperation trees can be processed without additional code.
 *
 * expression_tree_walker will handle SubLink nodes by recursing normally into
 * the "lefthand" arguments (which are expressions belonging to the outer
 * plan).  It will also call the walker on the sub-Query node; however, when
 * expression_tree_walker itself is called on a Query node, it does nothing
 * and returns "false".  The net effect is that unless the walker does
 * something special at a Query node, sub-selects will not be visited during
 * an expression tree walk. This is exactly the behavior wanted in many cases
 * --- and for those walkers that do want to recurse into sub-selects, special
 * behavior is typically needed anyway at the entry to a sub-select (such as
 * incrementing a depth counter). A walker that wants to examine sub-selects
 * should include code along the lines of:
 *
 *		if (IsA(node, Query))
 *		{
 *			adjust context for subquery;
 *			result = query_tree_walker((Query *) node, my_walker, context,
 *									   0); // to visit rtable items too
 *			restore context if needed;
 *			return result;
 *		}
 *
 * query_tree_walker is a convenience routine (see below) that calls the
 * walker on all the expression subtrees of the given Query node.
 *
 * expression_tree_walker will handle SubPlan nodes by recursing normally
 * into the "oper" and "args" lists (which are expressions belonging to the
 * outer plan).  It will not touch the completed subplan, however.  Since
 * there is no link to the original Query, it is not possible to recurse into
 * subselects of an already-planned expression tree.  This is OK for current
 * uses, but may need to be revisited in future.
 *--------------------
 */

bool
expression_tree_walker(Node *node,
					   bool (*walker) (),
					   void *context)
{
	List	   *temp;

	/*
	 * The walker has already visited the current node, and so we need
	 * only recurse into any sub-nodes it has.
	 *
	 * We assume that the walker is not interested in List nodes per se, so
	 * when we expect a List we just recurse directly to self without
	 * bothering to call the walker.
	 */
	if (node == NULL)
		return false;
	switch (nodeTag(node))
	{
		case T_Var:
		case T_Const:
		case T_Param:
		case T_ConstraintTestValue:
		case T_RangeTblRef:
			/* primitive node types with no subnodes */
			break;
		case T_Aggref:
			return walker(((Aggref *) node)->target, context);
		case T_ArrayRef:
			{
				ArrayRef   *aref = (ArrayRef *) node;

				/* recurse directly for upper/lower array index lists */
				if (expression_tree_walker((Node *) aref->refupperindexpr,
										   walker, context))
					return true;
				if (expression_tree_walker((Node *) aref->reflowerindexpr,
										   walker, context))
					return true;
				/* walker must see the refexpr and refassgnexpr, however */
				if (walker(aref->refexpr, context))
					return true;
				if (walker(aref->refassgnexpr, context))
					return true;
			}
			break;
		case T_FuncExpr:
			{
				FuncExpr   *expr = (FuncExpr *) node;

				if (expression_tree_walker((Node *) expr->args,
										   walker, context))
					return true;
			}
			break;
		case T_OpExpr:
			{
				OpExpr	   *expr = (OpExpr *) node;

				if (expression_tree_walker((Node *) expr->args,
										   walker, context))
					return true;
			}
			break;
		case T_DistinctExpr:
			{
				DistinctExpr *expr = (DistinctExpr *) node;

				if (expression_tree_walker((Node *) expr->args,
										   walker, context))
					return true;
			}
			break;
		case T_BoolExpr:
			{
				BoolExpr   *expr = (BoolExpr *) node;

				if (expression_tree_walker((Node *) expr->args,
										   walker, context))
					return true;
			}
			break;
		case T_SubLink:
			{
				SubLink    *sublink = (SubLink *) node;

				/*
				 * We only recurse into the lefthand list (the incomplete
				 * OpExpr nodes in the oper list are deemed uninteresting,
				 * perhaps even confusing).
				 */
				if (expression_tree_walker((Node *) sublink->lefthand,
										   walker, context))
					return true;
				/*
				 * Also invoke the walker on the sublink's Query node, so
				 * it can recurse into the sub-query if it wants to.
				 */
				return walker(sublink->subselect, context);
			}
			break;
		case T_SubPlan:
			{
				SubPlan *subplan = (SubPlan *) node;

				/* recurse into the oper list, but not into the Plan */
				if (expression_tree_walker((Node *) subplan->oper,
										   walker, context))
					return true;
				/* also examine args list */
				if (expression_tree_walker((Node *) subplan->args,
										   walker, context))
					return true;
			}
			break;
		case T_FieldSelect:
			return walker(((FieldSelect *) node)->arg, context);
		case T_RelabelType:
			return walker(((RelabelType *) node)->arg, context);
		case T_CaseExpr:
			{
				CaseExpr   *caseexpr = (CaseExpr *) node;

				/* we assume walker doesn't care about CaseWhens, either */
				foreach(temp, caseexpr->args)
				{
					CaseWhen   *when = (CaseWhen *) lfirst(temp);

					Assert(IsA(when, CaseWhen));
					if (walker(when->expr, context))
						return true;
					if (walker(when->result, context))
						return true;
				}
				/* caseexpr->arg should be null, but we'll check it anyway */
				if (walker(caseexpr->arg, context))
					return true;
				if (walker(caseexpr->defresult, context))
					return true;
			}
			break;
		case T_NullTest:
			return walker(((NullTest *) node)->arg, context);
		case T_BooleanTest:
			return walker(((BooleanTest *) node)->arg, context);
		case T_ConstraintTest:
			if (walker(((ConstraintTest *) node)->arg, context))
				return true;
			return walker(((ConstraintTest *) node)->check_expr, context);
		case T_TargetEntry:
			return walker(((TargetEntry *) node)->expr, context);
		case T_Query:
			/* Do nothing with a sub-Query, per discussion above */
			break;
		case T_List:
			foreach(temp, (List *) node)
			{
				if (walker((Node *) lfirst(temp), context))
					return true;
			}
			break;
		case T_FromExpr:
			{
				FromExpr   *from = (FromExpr *) node;

				if (walker(from->fromlist, context))
					return true;
				if (walker(from->quals, context))
					return true;
			}
			break;
		case T_JoinExpr:
			{
				JoinExpr   *join = (JoinExpr *) node;

				if (walker(join->larg, context))
					return true;
				if (walker(join->rarg, context))
					return true;
				if (walker(join->quals, context))
					return true;

				/*
				 * alias clause, using list are deemed uninteresting.
				 */
			}
			break;
		case T_SetOperationStmt:
			{
				SetOperationStmt *setop = (SetOperationStmt *) node;

				if (walker(setop->larg, context))
					return true;
				if (walker(setop->rarg, context))
					return true;
			}
			break;
		default:
			elog(ERROR, "expression_tree_walker: Unexpected node type %d",
				 nodeTag(node));
			break;
	}
	return false;
}

/*
 * query_tree_walker --- initiate a walk of a Query's expressions
 *
 * This routine exists just to reduce the number of places that need to know
 * where all the expression subtrees of a Query are.  Note it can be used
 * for starting a walk at top level of a Query regardless of whether the
 * walker intends to descend into subqueries.  It is also useful for
 * descending into subqueries within a walker.
 *
 * Some callers want to suppress visitation of certain items in the sub-Query,
 * typically because they need to process them specially, or don't actually
 * want to recurse into subqueries.  This is supported by the flags argument,
 * which is the bitwise OR of flag values to suppress visitation of
 * indicated items.  (More flag bits may be added as needed.)
 */
bool
query_tree_walker(Query *query,
				  bool (*walker) (),
				  void *context,
				  int flags)
{
	List	   *rt;

	Assert(query != NULL && IsA(query, Query));

	if (walker((Node *) query->targetList, context))
		return true;
	if (walker((Node *) query->jointree, context))
		return true;
	if (walker(query->setOperations, context))
		return true;
	if (walker(query->havingQual, context))
		return true;
	foreach(rt, query->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);

		switch (rte->rtekind)
		{
			case RTE_RELATION:
			case RTE_SPECIAL:
				/* nothing to do */
				break;
			case RTE_SUBQUERY:
				if (! (flags & QTW_IGNORE_SUBQUERIES))
					if (walker(rte->subquery, context))
						return true;
				break;
			case RTE_JOIN:
				if (! (flags & QTW_IGNORE_JOINALIASES))
					if (walker(rte->joinaliasvars, context))
						return true;
				break;
			case RTE_FUNCTION:
				if (walker(rte->funcexpr, context))
					return true;
				break;
		}
	}
	return false;
}


/*--------------------
 * expression_tree_mutator() is designed to support routines that make a
 * modified copy of an expression tree, with some nodes being added,
 * removed, or replaced by new subtrees.  The original tree is (normally)
 * not changed.  Each recursion level is responsible for returning a copy of
 * (or appropriately modified substitute for) the subtree it is handed.
 * A mutator routine should look like this:
 *
 * Node * my_mutator (Node *node, my_struct *context)
 * {
 *		if (node == NULL)
 *			return NULL;
 *		// check for nodes that special work is required for, eg:
 *		if (IsA(node, Var))
 *		{
 *			... create and return modified copy of Var node
 *		}
 *		else if (IsA(node, ...))
 *		{
 *			... do special transformations of other node types
 *		}
 *		// for any node type not specially processed, do:
 *		return expression_tree_mutator(node, my_mutator, (void *) context);
 * }
 *
 * The "context" argument points to a struct that holds whatever context
 * information the mutator routine needs --- it can be used to return extra
 * data gathered by the mutator, too.  This argument is not touched by
 * expression_tree_mutator, but it is passed down to recursive sub-invocations
 * of my_mutator.  The tree walk is started from a setup routine that
 * fills in the appropriate context struct, calls my_mutator with the
 * top-level node of the tree, and does any required post-processing.
 *
 * Each level of recursion must return an appropriately modified Node.
 * If expression_tree_mutator() is called, it will make an exact copy
 * of the given Node, but invoke my_mutator() to copy the sub-node(s)
 * of that Node.  In this way, my_mutator() has full control over the
 * copying process but need not directly deal with expression trees
 * that it has no interest in.
 *
 * Just as for expression_tree_walker, the node types handled by
 * expression_tree_mutator include all those normally found in target lists
 * and qualifier clauses during the planning stage.
 *
 * expression_tree_mutator will handle a SubPlan node by recursing into
 * the "oper" and "args" lists (which belong to the outer plan), but it
 * will simply copy the link to the inner plan, since that's typically what
 * expression tree mutators want.  A mutator that wants to modify the subplan
 * can force appropriate behavior by recognizing SubPlan expression nodes
 * and doing the right thing.
 *
 * SubLink nodes are handled by recursing into the "lefthand" argument list
 * only.  (A SubLink will be seen only if the tree has not yet been
 * processed by subselect.c.)  Again, this can be overridden by the mutator,
 * but it seems to be the most useful default behavior.
 *--------------------
 */

Node *
expression_tree_mutator(Node *node,
						Node *(*mutator) (),
						void *context)
{
	/*
	 * The mutator has already decided not to modify the current node, but
	 * we must call the mutator for any sub-nodes.
	 */

#define FLATCOPY(newnode, node, nodetype)  \
	( (newnode) = makeNode(nodetype), \
	  memcpy((newnode), (node), sizeof(nodetype)) )

#define CHECKFLATCOPY(newnode, node, nodetype)	\
	( AssertMacro(IsA((node), nodetype)), \
	  (newnode) = makeNode(nodetype), \
	  memcpy((newnode), (node), sizeof(nodetype)) )

#define MUTATE(newfield, oldfield, fieldtype)  \
		( (newfield) = (fieldtype) mutator((Node *) (oldfield), context) )

	if (node == NULL)
		return NULL;
	switch (nodeTag(node))
	{
		case T_Var:
		case T_Const:
		case T_Param:
		case T_ConstraintTestValue:
		case T_RangeTblRef:
			/* primitive node types with no subnodes */
			return (Node *) copyObject(node);
		case T_Aggref:
			{
				Aggref	   *aggref = (Aggref *) node;
				Aggref	   *newnode;

				FLATCOPY(newnode, aggref, Aggref);
				MUTATE(newnode->target, aggref->target, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_ArrayRef:
			{
				ArrayRef   *arrayref = (ArrayRef *) node;
				ArrayRef   *newnode;

				FLATCOPY(newnode, arrayref, ArrayRef);
				MUTATE(newnode->refupperindexpr, arrayref->refupperindexpr,
					   List *);
				MUTATE(newnode->reflowerindexpr, arrayref->reflowerindexpr,
					   List *);
				MUTATE(newnode->refexpr, arrayref->refexpr,
					   Expr *);
				MUTATE(newnode->refassgnexpr, arrayref->refassgnexpr,
					   Expr *);
				return (Node *) newnode;
			}
			break;
		case T_FuncExpr:
			{
				FuncExpr   *expr = (FuncExpr *) node;
				FuncExpr   *newnode;

				FLATCOPY(newnode, expr, FuncExpr);
				MUTATE(newnode->args, expr->args, List *);
				return (Node *) newnode;
			}
			break;
		case T_OpExpr:
			{
				OpExpr	   *expr = (OpExpr *) node;
				OpExpr	   *newnode;

				FLATCOPY(newnode, expr, OpExpr);
				MUTATE(newnode->args, expr->args, List *);
				return (Node *) newnode;
			}
			break;
		case T_DistinctExpr:
			{
				DistinctExpr   *expr = (DistinctExpr *) node;
				DistinctExpr   *newnode;

				FLATCOPY(newnode, expr, DistinctExpr);
				MUTATE(newnode->args, expr->args, List *);
				return (Node *) newnode;
			}
			break;
		case T_BoolExpr:
			{
				BoolExpr   *expr = (BoolExpr *) node;
				BoolExpr   *newnode;

				FLATCOPY(newnode, expr, BoolExpr);
				MUTATE(newnode->args, expr->args, List *);
				return (Node *) newnode;
			}
			break;
		case T_SubLink:
			{
				/*
				 * We transform the lefthand side, but not the oper list nor
				 * the subquery.
				 */
				SubLink    *sublink = (SubLink *) node;
				SubLink    *newnode;

				FLATCOPY(newnode, sublink, SubLink);
				MUTATE(newnode->lefthand, sublink->lefthand, List *);
				return (Node *) newnode;
			}
			break;
		case T_SubPlan:
			{
				SubPlan	   *subplan = (SubPlan *) node;
				SubPlan	   *newnode;

				FLATCOPY(newnode, subplan, SubPlan);
				/* transform args list (params to be passed to subplan) */
				MUTATE(newnode->args, subplan->args, List *);
				/* transform oper list as well */
				MUTATE(newnode->oper, subplan->oper, List *);
				/* but not the sub-Plan itself, which is referenced as-is */
				return (Node *) newnode;
			}
			break;
		case T_FieldSelect:
			{
				FieldSelect *fselect = (FieldSelect *) node;
				FieldSelect *newnode;

				FLATCOPY(newnode, fselect, FieldSelect);
				MUTATE(newnode->arg, fselect->arg, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_RelabelType:
			{
				RelabelType *relabel = (RelabelType *) node;
				RelabelType *newnode;

				FLATCOPY(newnode, relabel, RelabelType);
				MUTATE(newnode->arg, relabel->arg, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_CaseExpr:
			{
				CaseExpr   *caseexpr = (CaseExpr *) node;
				CaseExpr   *newnode;

				FLATCOPY(newnode, caseexpr, CaseExpr);
				MUTATE(newnode->args, caseexpr->args, List *);
				/* caseexpr->arg should be null, but we'll check it anyway */
				MUTATE(newnode->arg, caseexpr->arg, Expr *);
				MUTATE(newnode->defresult, caseexpr->defresult, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_CaseWhen:
			{
				CaseWhen   *casewhen = (CaseWhen *) node;
				CaseWhen   *newnode;

				FLATCOPY(newnode, casewhen, CaseWhen);
				MUTATE(newnode->expr, casewhen->expr, Expr *);
				MUTATE(newnode->result, casewhen->result, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_NullTest:
			{
				NullTest   *ntest = (NullTest *) node;
				NullTest   *newnode;

				FLATCOPY(newnode, ntest, NullTest);
				MUTATE(newnode->arg, ntest->arg, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_BooleanTest:
			{
				BooleanTest *btest = (BooleanTest *) node;
				BooleanTest *newnode;

				FLATCOPY(newnode, btest, BooleanTest);
				MUTATE(newnode->arg, btest->arg, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_ConstraintTest:
			{
				ConstraintTest *ctest = (ConstraintTest *) node;
				ConstraintTest *newnode;

				FLATCOPY(newnode, ctest, ConstraintTest);
				MUTATE(newnode->arg, ctest->arg, Expr *);
				MUTATE(newnode->check_expr, ctest->check_expr, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_TargetEntry:
			{
				/*
				 * We mutate the expression, but not the resdom, by
				 * default.
				 */
				TargetEntry *targetentry = (TargetEntry *) node;
				TargetEntry *newnode;

				FLATCOPY(newnode, targetentry, TargetEntry);
				MUTATE(newnode->expr, targetentry->expr, Expr *);
				return (Node *) newnode;
			}
			break;
		case T_List:
			{
				/*
				 * We assume the mutator isn't interested in the list
				 * nodes per se, so just invoke it on each list element.
				 * NOTE: this would fail badly on a list with integer
				 * elements!
				 */
				List	   *resultlist = NIL;
				List	   *temp;

				foreach(temp, (List *) node)
				{
					resultlist = lappend(resultlist,
										 mutator((Node *) lfirst(temp),
												 context));
				}
				return (Node *) resultlist;
			}
			break;
		case T_FromExpr:
			{
				FromExpr   *from = (FromExpr *) node;
				FromExpr   *newnode;

				FLATCOPY(newnode, from, FromExpr);
				MUTATE(newnode->fromlist, from->fromlist, List *);
				MUTATE(newnode->quals, from->quals, Node *);
				return (Node *) newnode;
			}
			break;
		case T_JoinExpr:
			{
				JoinExpr   *join = (JoinExpr *) node;
				JoinExpr   *newnode;

				FLATCOPY(newnode, join, JoinExpr);
				MUTATE(newnode->larg, join->larg, Node *);
				MUTATE(newnode->rarg, join->rarg, Node *);
				MUTATE(newnode->quals, join->quals, Node *);
				/* We do not mutate alias or using by default */
				return (Node *) newnode;
			}
			break;
		case T_SetOperationStmt:
			{
				SetOperationStmt *setop = (SetOperationStmt *) node;
				SetOperationStmt *newnode;

				FLATCOPY(newnode, setop, SetOperationStmt);
				MUTATE(newnode->larg, setop->larg, Node *);
				MUTATE(newnode->rarg, setop->rarg, Node *);
				return (Node *) newnode;
			}
			break;
		default:
			elog(ERROR, "expression_tree_mutator: Unexpected node type %d",
				 nodeTag(node));
			break;
	}
	/* can't get here, but keep compiler happy */
	return NULL;
}


/*
 * query_tree_mutator --- initiate modification of a Query's expressions
 *
 * This routine exists just to reduce the number of places that need to know
 * where all the expression subtrees of a Query are.  Note it can be used
 * for starting a walk at top level of a Query regardless of whether the
 * mutator intends to descend into subqueries.	It is also useful for
 * descending into subqueries within a mutator.
 *
 * The specified Query node is modified-in-place; do a FLATCOPY() beforehand
 * if you don't want to change the original.  All substructure is safely
 * copied, however.
 *
 * Some callers want to suppress mutating of certain items in the sub-Query,
 * typically because they need to process them specially, or don't actually
 * want to recurse into subqueries.  This is supported by the flags argument,
 * which is the bitwise OR of flag values to suppress mutating of
 * indicated items.  (More flag bits may be added as needed.)
 */
void
query_tree_mutator(Query *query,
				   Node *(*mutator) (),
				   void *context,
				   int flags)
{
	List	   *newrt = NIL;
	List	   *rt;

	Assert(query != NULL && IsA(query, Query));

	MUTATE(query->targetList, query->targetList, List *);
	MUTATE(query->jointree, query->jointree, FromExpr *);
	MUTATE(query->setOperations, query->setOperations, Node *);
	MUTATE(query->havingQual, query->havingQual, Node *);
	foreach(rt, query->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);
		RangeTblEntry *newrte;

		switch (rte->rtekind)
		{
			case RTE_RELATION:
			case RTE_SPECIAL:
				/* nothing to do, don't bother to make a copy */
				break;
			case RTE_SUBQUERY:
				if (! (flags & QTW_IGNORE_SUBQUERIES))
				{
					FLATCOPY(newrte, rte, RangeTblEntry);
					CHECKFLATCOPY(newrte->subquery, rte->subquery, Query);
					MUTATE(newrte->subquery, newrte->subquery, Query *);
					rte = newrte;
				}
				break;
			case RTE_JOIN:
				if (! (flags & QTW_IGNORE_JOINALIASES))
				{
					FLATCOPY(newrte, rte, RangeTblEntry);
					MUTATE(newrte->joinaliasvars, rte->joinaliasvars, List *);
					rte = newrte;
				}
				break;
			case RTE_FUNCTION:
				FLATCOPY(newrte, rte, RangeTblEntry);
				MUTATE(newrte->funcexpr, rte->funcexpr, Node *);
				rte = newrte;
				break;
		}
		newrt = lappend(newrt, rte);
	}
	query->rtable = newrt;
}
