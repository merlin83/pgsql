/*-------------------------------------------------------------------------
 *
 * parse_agg.c
 *	  handle aggregates in parser
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/parse_agg.c,v 1.51 2003-01-17 03:25:04 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "optimizer/clauses.h"
#include "optimizer/tlist.h"
#include "parser/parse_agg.h"
#include "parser/parsetree.h"


typedef struct
{
	ParseState *pstate;
	List	   *groupClauses;
	bool		have_non_var_grouping;
	int			sublevels_up;
} check_ungrouped_columns_context;

static void check_ungrouped_columns(Node *node, ParseState *pstate,
						List *groupClauses, bool have_non_var_grouping);
static bool check_ungrouped_columns_walker(Node *node,
							   check_ungrouped_columns_context *context);

/*
 * check_ungrouped_columns -
 *	  Scan the given expression tree for ungrouped variables (variables
 *	  that are not listed in the groupClauses list and are not within
 *	  the arguments of aggregate functions).  Emit a suitable error message
 *	  if any are found.
 *
 * NOTE: we assume that the given clause has been transformed suitably for
 * parser output.  This means we can use expression_tree_walker.
 *
 * NOTE: we recognize grouping expressions in the main query, but only
 * grouping Vars in subqueries.  For example, this will be rejected,
 * although it could be allowed:
 *		SELECT
 *			(SELECT x FROM bar where y = (foo.a + foo.b))
 *		FROM foo
 *		GROUP BY a + b;
 * The difficulty is the need to account for different sublevels_up.
 * This appears to require a whole custom version of equal(), which is
 * way more pain than the feature seems worth.
 */
static void
check_ungrouped_columns(Node *node, ParseState *pstate,
						List *groupClauses, bool have_non_var_grouping)
{
	check_ungrouped_columns_context context;

	context.pstate = pstate;
	context.groupClauses = groupClauses;
	context.have_non_var_grouping = have_non_var_grouping;
	context.sublevels_up = 0;
	check_ungrouped_columns_walker(node, &context);
}

static bool
check_ungrouped_columns_walker(Node *node,
							   check_ungrouped_columns_context *context)
{
	List	   *gl;

	if (node == NULL)
		return false;
	if (IsA(node, Const) ||
		IsA(node, Param))
		return false;			/* constants are always acceptable */

	/*
	 * If we find an aggregate function, do not recurse into its
	 * arguments; ungrouped vars in the arguments are not an error.
	 */
	if (IsA(node, Aggref))
		return false;

	/*
	 * If we have any GROUP BY items that are not simple Vars,
	 * check to see if subexpression as a whole matches any GROUP BY item.
	 * We need to do this at every recursion level so that we recognize
	 * GROUPed-BY expressions before reaching variables within them.
	 * But this only works at the outer query level, as noted above.
	 */
	if (context->have_non_var_grouping && context->sublevels_up == 0)
	{
		foreach(gl, context->groupClauses)
		{
			if (equal(node, lfirst(gl)))
				return false;	/* acceptable, do not descend more */
		}
	}

	/*
	 * If we have an ungrouped Var of the original query level, we have a
	 * failure.  Vars below the original query level are not a problem,
	 * and neither are Vars from above it.  (If such Vars are ungrouped as
	 * far as their own query level is concerned, that's someone else's
	 * problem...)
	 */
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;
		RangeTblEntry *rte;
		char	   *attname;

		if (var->varlevelsup != context->sublevels_up)
			return false;		/* it's not local to my query, ignore */
		/*
		 * Check for a match, if we didn't do it above.
		 */
		if (!context->have_non_var_grouping || context->sublevels_up != 0)
		{
			foreach(gl, context->groupClauses)
			{
				Var	   *gvar = (Var *) lfirst(gl);

				if (IsA(gvar, Var) &&
					gvar->varno == var->varno &&
					gvar->varattno == var->varattno &&
					gvar->varlevelsup == 0)
					return false; /* acceptable, we're okay */
			}
		}

		/* Found an ungrouped local variable; generate error message */
		Assert(var->varno > 0 &&
			   (int) var->varno <= length(context->pstate->p_rtable));
		rte = rt_fetch(var->varno, context->pstate->p_rtable);
		attname = get_rte_attribute_name(rte, var->varattno);
		if (context->sublevels_up == 0)
			elog(ERROR, "Attribute %s.%s must be GROUPed or used in an aggregate function",
				 rte->eref->aliasname, attname);
		else
			elog(ERROR, "Sub-SELECT uses un-GROUPed attribute %s.%s from outer query",
				 rte->eref->aliasname, attname);

	}

	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node,
								   check_ungrouped_columns_walker,
								   (void *) context,
								   0);
		context->sublevels_up--;
		return result;
	}
	return expression_tree_walker(node, check_ungrouped_columns_walker,
								  (void *) context);
}

/*
 * parseCheckAggregates
 *	Check for aggregates where they shouldn't be and improper grouping.
 *
 *	Ideally this should be done earlier, but it's difficult to distinguish
 *	aggregates from plain functions at the grammar level.  So instead we
 *	check here.  This function should be called after the target list and
 *	qualifications are finalized.
 */
void
parseCheckAggregates(ParseState *pstate, Query *qry)
{
	List	   *groupClauses = NIL;
	bool		have_non_var_grouping = false;
	List	   *tl;

	/* This should only be called if we found aggregates, GROUP, or HAVING */
	Assert(pstate->p_hasAggs || qry->groupClause || qry->havingQual);

	/*
	 * Aggregates must never appear in WHERE or JOIN/ON clauses.
	 *
	 * (Note this check should appear first to deliver an appropriate error
	 * message; otherwise we are likely to complain about some innocent
	 * variable in the target list, which is outright misleading if the
	 * problem is in WHERE.)
	 */
	if (contain_agg_clause(qry->jointree->quals))
		elog(ERROR, "Aggregates not allowed in WHERE clause");
	if (contain_agg_clause((Node *) qry->jointree->fromlist))
		elog(ERROR, "Aggregates not allowed in JOIN conditions");

	/*
	 * No aggregates allowed in GROUP BY clauses, either.
	 *
	 * While we are at it, build a list of the acceptable GROUP BY
	 * expressions for use by check_ungrouped_columns() (this avoids
	 * repeated scans of the targetlist within the recursive routine...).
	 * And detect whether any of the expressions aren't simple Vars.
	 */
	foreach(tl, qry->groupClause)
	{
		GroupClause *grpcl = lfirst(tl);
		Node	   *expr;

		expr = get_sortgroupclause_expr(grpcl, qry->targetList);
		if (expr == NULL)
			continue;			/* probably cannot happen */
		if (contain_agg_clause(expr))
			elog(ERROR, "Aggregates not allowed in GROUP BY clause");
		groupClauses = lcons(expr, groupClauses);
		if (!IsA(expr, Var))
			have_non_var_grouping = true;
	}

	/*
	 * Check the targetlist and HAVING clause for ungrouped variables.
	 */
	check_ungrouped_columns((Node *) qry->targetList, pstate,
							groupClauses, have_non_var_grouping);
	check_ungrouped_columns((Node *) qry->havingQual, pstate,
							groupClauses, have_non_var_grouping);

	/* Release the list storage (but not the pointed-to expressions!) */
	freeList(groupClauses);
}
