/*-------------------------------------------------------------------------
 *
 * planner.c
 *	  The query optimizer external interface.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/plan/planner.c,v 1.93 2000-10-26 21:36:09 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/paths.h"
#include "optimizer/planmain.h"
#include "optimizer/planner.h"
#include "optimizer/prep.h"
#include "optimizer/subselect.h"
#include "optimizer/tlist.h"
#include "optimizer/var.h"
#include "parser/parsetree.h"
#include "parser/parse_expr.h"
#include "rewrite/rewriteManip.h"
#include "utils/lsyscache.h"


/* Expression kind codes for preprocess_expression */
#define EXPRKIND_TARGET	0
#define EXPRKIND_WHERE	1
#define EXPRKIND_HAVING	2


static Node *pull_up_subqueries(Query *parse, Node *jtnode);
static bool is_simple_subquery(Query *subquery);
static void resolvenew_in_jointree(Node *jtnode, int varno, List *subtlist);
static Node *preprocess_jointree(Query *parse, Node *jtnode);
static Node *preprocess_expression(Query *parse, Node *expr, int kind);
static void preprocess_qual_conditions(Query *parse, Node *jtnode);
static List *make_subplanTargetList(Query *parse, List *tlist,
					   AttrNumber **groupColIdx);
static Plan *make_groupplan(List *group_tlist, bool tuplePerGroup,
			   List *groupClause, AttrNumber *grpColIdx,
			   bool is_presorted, Plan *subplan);

/*****************************************************************************
 *
 *	   Query optimizer entry point
 *
 *****************************************************************************/
Plan *
planner(Query *parse)
{
	Plan	   *result_plan;
	Index		save_PlannerQueryLevel;
	List	   *save_PlannerParamVar;

	/*
	 * The planner can be called recursively (an example is when
	 * eval_const_expressions tries to simplify an SQL function).
	 * So, these global state variables must be saved and restored.
	 *
	 * These vars cannot be moved into the Query structure since their
	 * whole purpose is communication across multiple sub-Queries.
	 *
	 * Note we do NOT save and restore PlannerPlanId: it exists to assign
	 * unique IDs to SubPlan nodes, and we want those IDs to be unique
	 * for the life of a backend.  Also, PlannerInitPlan is saved/restored
	 * in subquery_planner, not here.
	 */
	save_PlannerQueryLevel = PlannerQueryLevel;
	save_PlannerParamVar = PlannerParamVar;

	/* Initialize state for handling outer-level references and params */
	PlannerQueryLevel = 0;		/* will be 1 in top-level subquery_planner */
	PlannerParamVar = NIL;

	/* primary planning entry point (may recurse for subqueries) */
	result_plan = subquery_planner(parse, -1.0 /* default case */ );

	Assert(PlannerQueryLevel == 0);

	/* executor wants to know total number of Params used overall */
	result_plan->nParamExec = length(PlannerParamVar);

	/* final cleanup of the plan */
	set_plan_references(result_plan);

	/* restore state for outer planner, if any */
	PlannerQueryLevel = save_PlannerQueryLevel;
	PlannerParamVar = save_PlannerParamVar;

	return result_plan;
}


/*--------------------
 * subquery_planner
 *	  Invokes the planner on a subquery.  We recurse to here for each
 *	  sub-SELECT found in the query tree.
 *
 * parse is the querytree produced by the parser & rewriter.
 * tuple_fraction is the fraction of tuples we expect will be retrieved.
 * tuple_fraction is interpreted as explained for union_planner, below.
 *
 * Basically, this routine does the stuff that should only be done once
 * per Query object.  It then calls union_planner, which may be called
 * recursively on the same Query node in order to handle inheritance.
 * subquery_planner will be called recursively to handle sub-Query nodes
 * found within the query's expressions and rangetable.
 *
 * Returns a query plan.
 *--------------------
 */
Plan *
subquery_planner(Query *parse, double tuple_fraction)
{
	List	   *saved_initplan = PlannerInitPlan;
	int			saved_planid = PlannerPlanId;
	Plan	   *plan;
	List	   *lst;

	/* Set up for a new level of subquery */
	PlannerQueryLevel++;
	PlannerInitPlan = NIL;

#ifdef ENABLE_KEY_SET_QUERY
	/* this should go away sometime soon */
	transformKeySetQuery(parse);
#endif

	/*
	 * Check to see if any subqueries in the rangetable can be merged into
	 * this query.
	 */
	parse->jointree = (FromExpr *)
		pull_up_subqueries(parse, (Node *) parse->jointree);
	/*
	 * If so, we may have created opportunities to simplify the jointree.
	 */
	parse->jointree = (FromExpr *)
		preprocess_jointree(parse, (Node *) parse->jointree);

	/*
	 * A HAVING clause without aggregates is equivalent to a WHERE clause
	 * (except it can only refer to grouped fields).  If there are no aggs
	 * anywhere in the query, then we don't want to create an Agg plan
	 * node, so merge the HAVING condition into WHERE.	(We used to
	 * consider this an error condition, but it seems to be legal SQL.)
	 */
	if (parse->havingQual != NULL && !parse->hasAggs)
	{
		parse->jointree->quals = make_and_qual(parse->jointree->quals,
											   parse->havingQual);
		parse->havingQual = NULL;
	}

	/*
	 * Do preprocessing on targetlist and quals.
	 */
	parse->targetList = (List *)
		preprocess_expression(parse, (Node *) parse->targetList,
							  EXPRKIND_TARGET);

	preprocess_qual_conditions(parse, (Node *) parse->jointree);

	parse->havingQual = preprocess_expression(parse, parse->havingQual,
											  EXPRKIND_HAVING);

	/*
	 * Do the main planning (potentially recursive for inheritance)
	 */
	plan = union_planner(parse, tuple_fraction);

	/*
	 * XXX should any more of union_planner's activity be moved here?
	 *
	 * That would take careful study of the interactions with prepunion.c,
	 * but I suspect it would pay off in simplicity and avoidance of
	 * wasted cycles.
	 */

	/*
	 * If any subplans were generated, or if we're inside a subplan,
	 * build subPlan, extParam and locParam lists for plan nodes.
	 */
	if (PlannerPlanId != saved_planid || PlannerQueryLevel > 1)
	{
		(void) SS_finalize_plan(plan);
		/*
		 * At the moment, SS_finalize_plan doesn't handle initPlans
		 * and so we assign them to the topmost plan node.
		 */
		plan->initPlan = PlannerInitPlan;
		/* Must add the initPlans' extParams to the topmost node's, too */
		foreach(lst, plan->initPlan)
		{
			SubPlan *subplan = (SubPlan *) lfirst(lst);

			plan->extParam = set_unioni(plan->extParam,
										subplan->plan->extParam);
		}
	}

	/* Return to outer subquery context */
	PlannerQueryLevel--;
	PlannerInitPlan = saved_initplan;
	/* we do NOT restore PlannerPlanId; that's not an oversight! */

	return plan;
}

/*
 * pull_up_subqueries
 *		Look for subqueries in the rangetable that can be pulled up into
 *		the parent query.  If the subquery has no special features like
 *		grouping/aggregation then we can merge it into the parent's jointree.
 *
 * A tricky aspect of this code is that if we pull up a subquery we have
 * to replace Vars that reference the subquery's outputs throughout the
 * parent query, including quals attached to jointree nodes above the one
 * we are currently processing!  We handle this by being careful not to
 * change the jointree structure while recursing: no nodes other than
 * subquery RangeTblRef entries will be replaced.  Also, we can't turn
 * ResolveNew loose on the whole jointree, because it'll return a mutated
 * copy of the tree; we have to invoke it just on the quals, instead.
 */
static Node *
pull_up_subqueries(Query *parse, Node *jtnode)
{
	if (jtnode == NULL)
		return NULL;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;
		RangeTblEntry *rte = rt_fetch(varno, parse->rtable);
		Query	   *subquery = rte->subquery;

		/*
		 * Is this a subquery RTE, and if so, is the subquery simple enough
		 * to pull up?  (If not, do nothing at this node.)
		 */
		if (subquery && is_simple_subquery(subquery))
		{
			int		rtoffset;
			Node   *subjointree;
			List   *subtlist;

			/*
			 * First, recursively pull up the subquery's subqueries,
			 * so that this routine's processing is complete for its
			 * jointree and rangetable.
			 */
			subquery->jointree = (FromExpr *)
				pull_up_subqueries(subquery, (Node *) subquery->jointree);
			/*
			 * Append the subquery's rangetable to mine (currently,
			 * no adjustments will be needed in the subquery's rtable).
			 */
			rtoffset = length(parse->rtable);
			parse->rtable = nconc(parse->rtable, subquery->rtable);
			/*
			 * Make copies of the subquery's jointree and targetlist
			 * with varnos adjusted to match the merged rangetable.
			 */
			subjointree = copyObject(subquery->jointree);
			OffsetVarNodes(subjointree, rtoffset, 0);
			subtlist = copyObject(subquery->targetList);
			OffsetVarNodes((Node *) subtlist, rtoffset, 0);
			/*
			 * Replace all of the top query's references to the subquery's
			 * outputs with copies of the adjusted subtlist items, being
			 * careful not to replace any of the jointree structure.
			 */
			parse->targetList = (List *)
				ResolveNew((Node *) parse->targetList,
						   varno, 0, subtlist, CMD_SELECT, 0);
			resolvenew_in_jointree((Node *) parse->jointree, varno, subtlist);
			parse->havingQual =
				ResolveNew(parse->havingQual,
						   varno, 0, subtlist, CMD_SELECT, 0);
			/*
			 * Miscellaneous housekeeping.
			 */
			parse->hasSubLinks |= subquery->hasSubLinks;
			/*
			 * Return the adjusted subquery jointree to replace the
			 * RangeTblRef entry in my jointree.
			 */
			return subjointree;
		}
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *l;

		foreach(l, f->fromlist)
		{
			lfirst(l) = pull_up_subqueries(parse, lfirst(l));
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		j->larg = pull_up_subqueries(parse, j->larg);
		j->rarg = pull_up_subqueries(parse, j->rarg);
	}
	else
		elog(ERROR, "pull_up_subqueries: unexpected node type %d",
			 nodeTag(jtnode));
	return jtnode;
}

/*
 * is_simple_subquery
 *	  Check a subquery in the range table to see if it's simple enough
 *	  to pull up into the parent query.
 */
static bool
is_simple_subquery(Query *subquery)
{
	/*
	 * Let's just make sure it's a valid subselect ...
	 */
	if (!IsA(subquery, Query) ||
		subquery->commandType != CMD_SELECT ||
		subquery->resultRelation != 0 ||
		subquery->into != NULL ||
		subquery->isPortal)
		elog(ERROR, "is_simple_subquery: subquery is bogus");
	/*
	 * Also check for currently-unsupported features.
	 */
	if (subquery->rowMarks)
		elog(ERROR, "FOR UPDATE is not supported in subselects");
	/*
	 * Can't currently pull up a query with setops.
	 * Maybe after querytree redesign...
	 */
	if (subquery->setOperations)
		return false;
	/*
	 * Can't pull up a subquery involving grouping, aggregation, sorting,
	 * or limiting.
	 */
	if (subquery->hasAggs ||
		subquery->groupClause ||
		subquery->havingQual ||
		subquery->sortClause ||
		subquery->distinctClause ||
		subquery->limitOffset ||
		subquery->limitCount)
		return false;
	/*
	 * Hack: don't try to pull up a subquery with an empty jointree.
	 * query_planner() will correctly generate a Result plan for a
	 * jointree that's totally empty, but I don't think the right things
	 * happen if an empty FromExpr appears lower down in a jointree.
	 * Not worth working hard on this, just to collapse SubqueryScan/Result
	 * into Result...
	 */
	if (subquery->jointree->fromlist == NIL)
		return false;

	return true;
}

/*
 * Helper routine for pull_up_subqueries: do ResolveNew on every expression
 * in the jointree, without changing the jointree structure itself.  Ugly,
 * but there's no other way...
 */
static void
resolvenew_in_jointree(Node *jtnode, int varno, List *subtlist)
{
	if (jtnode == NULL)
		return;
	if (IsA(jtnode, RangeTblRef))
	{
		/* nothing to do here */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *l;

		foreach(l, f->fromlist)
			resolvenew_in_jointree(lfirst(l), varno, subtlist);
		f->quals = ResolveNew(f->quals,
							  varno, 0, subtlist, CMD_SELECT, 0);
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		resolvenew_in_jointree(j->larg, varno, subtlist);
		resolvenew_in_jointree(j->rarg, varno, subtlist);
		j->quals = ResolveNew(j->quals,
							  varno, 0, subtlist, CMD_SELECT, 0);
		/* We don't bother to update the colvars list, since it won't be
		 * used again ...
		 */
	}
	else
		elog(ERROR, "resolvenew_in_jointree: unexpected node type %d",
			 nodeTag(jtnode));
}

/*
 * preprocess_jointree
 *		Attempt to simplify a query's jointree.
 *
 * If we succeed in pulling up a subquery then we might form a jointree
 * in which a FromExpr is a direct child of another FromExpr.  In that
 * case we can consider collapsing the two FromExprs into one.  This is
 * an optional conversion, since the planner will work correctly either
 * way.  But we may find a better plan (at the cost of more planning time)
 * if we merge the two nodes.
 *
 * NOTE: don't try to do this in the same jointree scan that does subquery
 * pullup!  Since we're changing the jointree structure here, that wouldn't
 * work reliably --- see comments for pull_up_subqueries().
 */
static Node *
preprocess_jointree(Query *parse, Node *jtnode)
{
	if (jtnode == NULL)
		return NULL;
	if (IsA(jtnode, RangeTblRef))
	{
		/* nothing to do here... */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *newlist = NIL;
		List	   *l;

		foreach(l, f->fromlist)
		{
			Node   *child = (Node *) lfirst(l);

			/* Recursively simplify the child... */
			child = preprocess_jointree(parse, child);
			/* Now, is it a FromExpr? */
			if (child && IsA(child, FromExpr))
			{
				/*
				 * Yes, so do we want to merge it into parent?  Always do so
				 * if child has just one element (since that doesn't make the
				 * parent's list any longer).  Otherwise we have to be careful
				 * about the increase in planning time caused by combining the
				 * two join search spaces into one.  Our heuristic is to merge
				 * if the merge will produce a join list no longer than
				 * GEQO_RELS/2.  (Perhaps need an additional user parameter?)
				 */
				FromExpr   *subf = (FromExpr *) child;
				int			childlen = length(subf->fromlist);
				int			myothers = length(newlist) + length(lnext(l));

				if (childlen <= 1 || (childlen+myothers) <= geqo_rels/2)
				{
					newlist = nconc(newlist, subf->fromlist);
					f->quals = make_and_qual(f->quals, subf->quals);
				}
				else
					newlist = lappend(newlist, child);
			}
			else
				newlist = lappend(newlist, child);
		}
		f->fromlist = newlist;
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		/* Can't usefully change the JoinExpr, but recurse on children */
		j->larg = preprocess_jointree(parse, j->larg);
		j->rarg = preprocess_jointree(parse, j->rarg);
	}
	else
		elog(ERROR, "preprocess_jointree: unexpected node type %d",
			 nodeTag(jtnode));
	return jtnode;
}

/*
 * preprocess_expression
 *		Do subquery_planner's preprocessing work for an expression,
 *		which can be a targetlist, a WHERE clause (including JOIN/ON
 *		conditions), or a HAVING clause.
 */
static Node *
preprocess_expression(Query *parse, Node *expr, int kind)
{
	/*
	 * Simplify constant expressions.
	 *
	 * Note that at this point quals have not yet been converted to
	 * implicit-AND form, so we can apply eval_const_expressions directly.
	 * Also note that we need to do this before SS_process_sublinks,
	 * because that routine inserts bogus "Const" nodes.
	 */
	expr = eval_const_expressions(expr);

	/*
	 * If it's a qual or havingQual, canonicalize it, and convert it
	 * to implicit-AND format.
	 *
	 * XXX Is there any value in re-applying eval_const_expressions after
	 * canonicalize_qual?
	 */
	if (kind != EXPRKIND_TARGET)
	{
		expr = (Node *) canonicalize_qual((Expr *) expr, true);

#ifdef OPTIMIZER_DEBUG
		printf("After canonicalize_qual()\n");
		pprint(expr);
#endif
	}

	if (parse->hasSubLinks)
	{
		/* Expand SubLinks to SubPlans */
		expr = SS_process_sublinks(expr);

		if (kind != EXPRKIND_WHERE &&
			(parse->groupClause != NIL || parse->hasAggs))
		{
			/*
			 * Check for ungrouped variables passed to subplans.  Note we
			 * do NOT do this for subplans in WHERE (or JOIN/ON); it's legal
			 * there because WHERE is evaluated pre-GROUP.
			 *
			 * An interesting fine point: if subquery_planner reassigned a
			 * HAVING qual into WHERE, then we will accept references to
			 * ungrouped vars from subplans in the HAVING qual.  This is not
			 * entirely consistent, but it doesn't seem particularly
			 * harmful...
			 */
			check_subplans_for_ungrouped_vars(expr, parse);
		}
	}

	/* Replace uplevel vars with Param nodes */
	if (PlannerQueryLevel > 1)
		expr = SS_replace_correlation_vars(expr);

	return expr;
}

/*
 * preprocess_qual_conditions
 *		Recursively scan the query's jointree and do subquery_planner's
 *		preprocessing work on each qual condition found therein.
 */
static void
preprocess_qual_conditions(Query *parse, Node *jtnode)
{
	if (jtnode == NULL)
		return;
	if (IsA(jtnode, RangeTblRef))
	{
		/* nothing to do here */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *l;

		foreach(l, f->fromlist)
			preprocess_qual_conditions(parse, lfirst(l));

		f->quals = preprocess_expression(parse, f->quals, EXPRKIND_WHERE);
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		preprocess_qual_conditions(parse, j->larg);
		preprocess_qual_conditions(parse, j->rarg);

		j->quals = preprocess_expression(parse, j->quals, EXPRKIND_WHERE);
	}
	else
		elog(ERROR, "preprocess_qual_conditions: unexpected node type %d",
			 nodeTag(jtnode));
}

/*--------------------
 * union_planner
 *	  Invokes the planner on union-type queries (both set operations and
 *	  appends produced by inheritance), recursing if necessary to get them
 *	  all, then processes normal plans.
 *
 * parse is the querytree produced by the parser & rewriter.
 * tuple_fraction is the fraction of tuples we expect will be retrieved
 *
 * tuple_fraction is interpreted as follows:
 *	  < 0: determine fraction by inspection of query (normal case)
 *	  0: expect all tuples to be retrieved
 *	  0 < tuple_fraction < 1: expect the given fraction of tuples available
 *		from the plan to be retrieved
 *	  tuple_fraction >= 1: tuple_fraction is the absolute number of tuples
 *		expected to be retrieved (ie, a LIMIT specification)
 * The normal case is to pass -1, but some callers pass values >= 0 to
 * override this routine's determination of the appropriate fraction.
 *
 * Returns a query plan.
 *--------------------
 */
Plan *
union_planner(Query *parse,
			  double tuple_fraction)
{
	List	   *tlist = parse->targetList;
	Plan	   *result_plan = (Plan *) NULL;
	AttrNumber *groupColIdx = NULL;
	List	   *current_pathkeys = NIL;
	List	   *group_pathkeys;
	List	   *sort_pathkeys;
	Index		rt_index;
	List	   *inheritors;

	if (parse->setOperations)
	{
		/*
		 * Construct the plan for set operations.  The result will
		 * not need any work except perhaps a top-level sort.
		 */
		result_plan = plan_set_operations(parse);

		/*
		 * We should not need to call preprocess_targetlist, since we must
		 * be in a SELECT query node.
		 */
		Assert(parse->commandType == CMD_SELECT);

		/*
		 * We leave current_pathkeys NIL indicating we do not know sort
		 * order.  This is correct when the top set operation is UNION ALL,
		 * since the appended-together results are unsorted even if the
		 * subplans were sorted.  For other set operations we could be
		 * smarter --- future improvement!
		 */

		/*
		 * Calculate pathkeys that represent grouping/ordering
		 * requirements (grouping should always be null, but...)
		 */
		group_pathkeys = make_pathkeys_for_sortclauses(parse->groupClause,
													   tlist);
		sort_pathkeys = make_pathkeys_for_sortclauses(parse->sortClause,
													  tlist);
	}
	else if (find_inheritable_rt_entry(parse->rtable,
									   &rt_index, &inheritors))
	{
		List	   *sub_tlist;

		/*
		 * Generate appropriate target list for subplan; may be different
		 * from tlist if grouping or aggregation is needed.
		 */
		sub_tlist = make_subplanTargetList(parse, tlist, &groupColIdx);

		/*
		 * Recursively plan the subqueries needed for inheritance
		 */
		result_plan = plan_inherit_queries(parse, sub_tlist,
										   rt_index, inheritors);

		/*
		 * Fix up outer target list.  NOTE: unlike the case for
		 * non-inherited query, we pass the unfixed tlist to subplans,
		 * which do their own fixing.  But we still want to fix the outer
		 * target list afterwards. I *think* this is correct --- doing the
		 * fix before recursing is definitely wrong, because
		 * preprocess_targetlist() will do the wrong thing if invoked
		 * twice on the same list. Maybe that is a bug? tgl 6/6/99
		 */
		tlist = preprocess_targetlist(tlist,
									  parse->commandType,
									  parse->resultRelation,
									  parse->rtable);

		if (parse->rowMarks)
			elog(ERROR, "SELECT FOR UPDATE is not supported for inherit queries");

		/*
		 * We leave current_pathkeys NIL indicating we do not know sort
		 * order of the Append-ed results.
		 */

		/*
		 * Calculate pathkeys that represent grouping/ordering
		 * requirements
		 */
		group_pathkeys = make_pathkeys_for_sortclauses(parse->groupClause,
													   tlist);
		sort_pathkeys = make_pathkeys_for_sortclauses(parse->sortClause,
													  tlist);
	}
	else
	{
		List	   *sub_tlist;

		/* Preprocess targetlist in case we are inside an INSERT/UPDATE. */
		tlist = preprocess_targetlist(tlist,
									  parse->commandType,
									  parse->resultRelation,
									  parse->rtable);

		/*
		 * Add TID targets for rels selected FOR UPDATE (should this be
		 * done in preprocess_targetlist?).  The executor uses the TID
		 * to know which rows to lock, much as for UPDATE or DELETE.
		 */
		if (parse->rowMarks)
		{
			List	   *l;

			foreach(l, parse->rowMarks)
			{
				Index		rti = lfirsti(l);
				char	   *resname;
				Resdom	   *resdom;
				Var		   *var;
				TargetEntry *ctid;

				resname = (char *) palloc(32);
				sprintf(resname, "ctid%u", rti);
				resdom = makeResdom(length(tlist) + 1,
									TIDOID,
									-1,
									resname,
									true);

				var = makeVar(rti,
							  SelfItemPointerAttributeNumber,
							  TIDOID,
							  -1,
							  0);

				ctid = makeTargetEntry(resdom, (Node *) var);
				tlist = lappend(tlist, ctid);
			}
		}

		/*
		 * Generate appropriate target list for subplan; may be different
		 * from tlist if grouping or aggregation is needed.
		 */
		sub_tlist = make_subplanTargetList(parse, tlist, &groupColIdx);

		/*
		 * Calculate pathkeys that represent grouping/ordering
		 * requirements
		 */
		group_pathkeys = make_pathkeys_for_sortclauses(parse->groupClause,
													   tlist);
		sort_pathkeys = make_pathkeys_for_sortclauses(parse->sortClause,
													  tlist);

		/*
		 * Figure out whether we need a sorted result from query_planner.
		 *
		 * If we have a GROUP BY clause, then we want a result sorted
		 * properly for grouping.  Otherwise, if there is an ORDER BY
		 * clause, we want to sort by the ORDER BY clause.	(Note: if we
		 * have both, and ORDER BY is a superset of GROUP BY, it would be
		 * tempting to request sort by ORDER BY --- but that might just
		 * leave us failing to exploit an available sort order at all.
		 * Needs more thought...)
		 */
		if (parse->groupClause)
			parse->query_pathkeys = group_pathkeys;
		else if (parse->sortClause)
			parse->query_pathkeys = sort_pathkeys;
		else
			parse->query_pathkeys = NIL;

		/*
		 * Figure out whether we expect to retrieve all the tuples that
		 * the plan can generate, or to stop early due to a LIMIT or other
		 * factors.  If the caller passed a value >= 0, believe that
		 * value, else do our own examination of the query context.
		 */
		if (tuple_fraction < 0.0)
		{
			/* Initial assumption is we need all the tuples */
			tuple_fraction = 0.0;

			/*
			 * Check for a LIMIT clause.
			 */
			if (parse->limitCount != NULL)
			{
				if (IsA(parse->limitCount, Const))
				{
					Const	   *limitc = (Const *) parse->limitCount;
					int			count = (int) (limitc->constvalue);

					/*
					 * The constant can legally be either 0 ("ALL") or a
					 * positive integer.  If it is not ALL, we also need
					 * to consider the OFFSET part of LIMIT.
					 */
					if (count > 0)
					{
						tuple_fraction = (double) count;
						if (parse->limitOffset != NULL)
						{
							if (IsA(parse->limitOffset, Const))
							{
								int			offset;

								limitc = (Const *) parse->limitOffset;
								offset = (int) (limitc->constvalue);
								if (offset > 0)
									tuple_fraction += (double) offset;
							}
							else
							{
								/* It's an expression ... punt ... */
								tuple_fraction = 0.10;
							}
						}
					}
				}
				else
				{
					/*
					 * COUNT is an expression ... don't know exactly what the
					 * limit will be, but for lack of a better idea assume
					 * 10% of the plan's result is wanted.
					 */
					tuple_fraction = 0.10;
				}
			}

			/*
			 * Check for a retrieve-into-portal, ie DECLARE CURSOR.
			 *
			 * We have no real idea how many tuples the user will ultimately
			 * FETCH from a cursor, but it seems a good bet that he
			 * doesn't want 'em all.  Optimize for 10% retrieval (you
			 * gotta better number?)
			 */
			if (parse->isPortal)
				tuple_fraction = 0.10;
		}

		/*
		 * Adjust tuple_fraction if we see that we are going to apply
		 * grouping/aggregation/etc.  This is not overridable by the
		 * caller, since it reflects plan actions that this routine will
		 * certainly take, not assumptions about context.
		 */
		if (parse->groupClause)
		{

			/*
			 * In GROUP BY mode, we have the little problem that we don't
			 * really know how many input tuples will be needed to make a
			 * group, so we can't translate an output LIMIT count into an
			 * input count.  For lack of a better idea, assume 25% of the
			 * input data will be processed if there is any output limit.
			 * However, if the caller gave us a fraction rather than an
			 * absolute count, we can keep using that fraction (which
			 * amounts to assuming that all the groups are about the same
			 * size).
			 */
			if (tuple_fraction >= 1.0)
				tuple_fraction = 0.25;

			/*
			 * If both GROUP BY and ORDER BY are specified, we will need
			 * two levels of sort --- and, therefore, certainly need to
			 * read all the input tuples --- unless ORDER BY is a subset
			 * of GROUP BY.  (Although we are comparing non-canonicalized
			 * pathkeys here, it should be OK since they will both contain
			 * only single-element sublists at this point.	See
			 * pathkeys.c.)
			 */
			if (parse->groupClause && parse->sortClause &&
				!pathkeys_contained_in(sort_pathkeys, group_pathkeys))
				tuple_fraction = 0.0;
		}
		else if (parse->hasAggs)
		{

			/*
			 * Ungrouped aggregate will certainly want all the input
			 * tuples.
			 */
			tuple_fraction = 0.0;
		}
		else if (parse->distinctClause)
		{

			/*
			 * SELECT DISTINCT, like GROUP, will absorb an unpredictable
			 * number of input tuples per output tuple.  Handle the same
			 * way.
			 */
			if (tuple_fraction >= 1.0)
				tuple_fraction = 0.25;
		}

		/* Generate the basic plan for this Query */
		result_plan = query_planner(parse,
									sub_tlist,
									tuple_fraction);

		/*
		 * query_planner returns actual sort order (which is not
		 * necessarily what we requested) in query_pathkeys.
		 */
		current_pathkeys = parse->query_pathkeys;
	}

	/* query_planner returns NULL if it thinks plan is bogus */
	if (!result_plan)
		elog(ERROR, "union_planner: failed to create plan");

	/*
	 * We couldn't canonicalize group_pathkeys and sort_pathkeys before
	 * running query_planner(), so do it now.
	 */
	group_pathkeys = canonicalize_pathkeys(parse, group_pathkeys);
	sort_pathkeys = canonicalize_pathkeys(parse, sort_pathkeys);

	/*
	 * If we have a GROUP BY clause, insert a group node (plus the
	 * appropriate sort node, if necessary).
	 */
	if (parse->groupClause)
	{
		bool		tuplePerGroup;
		List	   *group_tlist;
		bool		is_sorted;

		/*
		 * Decide whether how many tuples per group the Group node needs
		 * to return. (Needs only one tuple per group if no aggregate is
		 * present. Otherwise, need every tuple from the group to do the
		 * aggregation.)  Note tuplePerGroup is named backwards :-(
		 */
		tuplePerGroup = parse->hasAggs;

		/*
		 * If there are aggregates then the Group node should just return
		 * the same set of vars as the subplan did (but we can exclude any
		 * GROUP BY expressions).  If there are no aggregates then the
		 * Group node had better compute the final tlist.
		 */
		if (parse->hasAggs)
			group_tlist = flatten_tlist(result_plan->targetlist);
		else
			group_tlist = tlist;

		/*
		 * Figure out whether the path result is already ordered the way
		 * we need it --- if so, no need for an explicit sort step.
		 */
		if (pathkeys_contained_in(group_pathkeys, current_pathkeys))
		{
			is_sorted = true;	/* no sort needed now */
			/* current_pathkeys remains unchanged */
		}
		else
		{

			/*
			 * We will need to do an explicit sort by the GROUP BY clause.
			 * make_groupplan will do the work, but set current_pathkeys
			 * to indicate the resulting order.
			 */
			is_sorted = false;
			current_pathkeys = group_pathkeys;
		}

		result_plan = make_groupplan(group_tlist,
									 tuplePerGroup,
									 parse->groupClause,
									 groupColIdx,
									 is_sorted,
									 result_plan);
	}

	/*
	 * If aggregate is present, insert the Agg node
	 *
	 * HAVING clause, if any, becomes qual of the Agg node
	 */
	if (parse->hasAggs)
	{
		result_plan = (Plan *) make_agg(tlist,
										(List *) parse->havingQual,
										result_plan);
		/* Note: Agg does not affect any existing sort order of the tuples */
	}

	/*
	 * If we were not able to make the plan come out in the right order,
	 * add an explicit sort step.
	 */
	if (parse->sortClause)
	{
		if (!pathkeys_contained_in(sort_pathkeys, current_pathkeys))
			result_plan = make_sortplan(tlist, result_plan,
										parse->sortClause);
	}

	/*
	 * If there is a DISTINCT clause, add the UNIQUE node.
	 */
	if (parse->distinctClause)
	{
		result_plan = (Plan *) make_unique(tlist, result_plan,
										   parse->distinctClause);
	}

	/*
	 * Finally, if there is a LIMIT/OFFSET clause, add the LIMIT node.
	 */
	if (parse->limitOffset || parse->limitCount)
	{
		result_plan = (Plan *) make_limit(tlist, result_plan,
										  parse->limitOffset,
										  parse->limitCount);
	}

	return result_plan;
}

/*---------------
 * make_subplanTargetList
 *	  Generate appropriate target list when grouping is required.
 *
 * When union_planner inserts Aggregate and/or Group plan nodes above
 * the result of query_planner, we typically want to pass a different
 * target list to query_planner than the outer plan nodes should have.
 * This routine generates the correct target list for the subplan.
 *
 * The initial target list passed from the parser already contains entries
 * for all ORDER BY and GROUP BY expressions, but it will not have entries
 * for variables used only in HAVING clauses; so we need to add those
 * variables to the subplan target list.  Also, if we are doing either
 * grouping or aggregation, we flatten all expressions except GROUP BY items
 * into their component variables; the other expressions will be computed by
 * the inserted nodes rather than by the subplan.  For example,
 * given a query like
 *		SELECT a+b,SUM(c+d) FROM table GROUP BY a+b;
 * we want to pass this targetlist to the subplan:
 *		a,b,c,d,a+b
 * where the a+b target will be used by the Sort/Group steps, and the
 * other targets will be used for computing the final results.	(In the
 * above example we could theoretically suppress the a and b targets and
 * use only a+b, but it's not really worth the trouble.)
 *
 * 'parse' is the query being processed.
 * 'tlist' is the query's target list.
 * 'groupColIdx' receives an array of column numbers for the GROUP BY
 * expressions (if there are any) in the subplan's target list.
 *
 * The result is the targetlist to be passed to the subplan.
 *---------------
 */
static List *
make_subplanTargetList(Query *parse,
					   List *tlist,
					   AttrNumber **groupColIdx)
{
	List	   *sub_tlist;
	List	   *extravars;
	int			numCols;

	*groupColIdx = NULL;

	/*
	 * If we're not grouping or aggregating, nothing to do here;
	 * query_planner should receive the unmodified target list.
	 */
	if (!parse->hasAggs && !parse->groupClause && !parse->havingQual)
		return tlist;

	/*
	 * Otherwise, start with a "flattened" tlist (having just the vars
	 * mentioned in the targetlist and HAVING qual --- but not upper-
	 * level Vars; they will be replaced by Params later on).
	 */
	sub_tlist = flatten_tlist(tlist);
	extravars = pull_var_clause(parse->havingQual, false);
	sub_tlist = add_to_flat_tlist(sub_tlist, extravars);
	freeList(extravars);

	/*
	 * If grouping, create sub_tlist entries for all GROUP BY expressions
	 * (GROUP BY items that are simple Vars should be in the list
	 * already), and make an array showing where the group columns are in
	 * the sub_tlist.
	 */
	numCols = length(parse->groupClause);
	if (numCols > 0)
	{
		int			keyno = 0;
		AttrNumber *grpColIdx;
		List	   *gl;

		grpColIdx = (AttrNumber *) palloc(sizeof(AttrNumber) * numCols);
		*groupColIdx = grpColIdx;

		foreach(gl, parse->groupClause)
		{
			GroupClause *grpcl = (GroupClause *) lfirst(gl);
			Node	   *groupexpr = get_sortgroupclause_expr(grpcl, tlist);
			TargetEntry *te = NULL;
			List	   *sl;

			/* Find or make a matching sub_tlist entry */
			foreach(sl, sub_tlist)
			{
				te = (TargetEntry *) lfirst(sl);
				if (equal(groupexpr, te->expr))
					break;
			}
			if (!sl)
			{
				te = makeTargetEntry(makeResdom(length(sub_tlist) + 1,
												exprType(groupexpr),
												exprTypmod(groupexpr),
												NULL,
												false),
									 groupexpr);
				sub_tlist = lappend(sub_tlist, te);
			}

			/* and save its resno */
			grpColIdx[keyno++] = te->resdom->resno;
		}
	}

	return sub_tlist;
}

/*
 * make_groupplan
 *		Add a Group node for GROUP BY processing.
 *		If we couldn't make the subplan produce presorted output for grouping,
 *		first add an explicit Sort node.
 */
static Plan *
make_groupplan(List *group_tlist,
			   bool tuplePerGroup,
			   List *groupClause,
			   AttrNumber *grpColIdx,
			   bool is_presorted,
			   Plan *subplan)
{
	int			numCols = length(groupClause);

	if (!is_presorted)
	{

		/*
		 * The Sort node always just takes a copy of the subplan's tlist
		 * plus ordering information.  (This might seem inefficient if the
		 * subplan contains complex GROUP BY expressions, but in fact Sort
		 * does not evaluate its targetlist --- it only outputs the same
		 * tuples in a new order.  So the expressions we might be copying
		 * are just dummies with no extra execution cost.)
		 */
		List	   *sort_tlist = new_unsorted_tlist(subplan->targetlist);
		int			keyno = 0;
		List	   *gl;

		foreach(gl, groupClause)
		{
			GroupClause *grpcl = (GroupClause *) lfirst(gl);
			TargetEntry *te = nth(grpColIdx[keyno] - 1, sort_tlist);
			Resdom	   *resdom = te->resdom;

			/*
			 * Check for the possibility of duplicate group-by clauses ---
			 * the parser should have removed 'em, but the Sort executor
			 * will get terribly confused if any get through!
			 */
			if (resdom->reskey == 0)
			{
				/* OK, insert the ordering info needed by the executor. */
				resdom->reskey = ++keyno;
				resdom->reskeyop = get_opcode(grpcl->sortop);
			}
		}

		Assert(keyno > 0);

		subplan = (Plan *) make_sort(sort_tlist, subplan, keyno);
	}

	return (Plan *) make_group(group_tlist, tuplePerGroup, numCols,
							   grpColIdx, subplan);
}

/*
 * make_sortplan
 *	  Add a Sort node to implement an explicit ORDER BY clause.
 */
Plan *
make_sortplan(List *tlist, Plan *plannode, List *sortcls)
{
	List	   *sort_tlist;
	List	   *i;
	int			keyno = 0;

	/*
	 * First make a copy of the tlist so that we don't corrupt the
	 * original.
	 */
	sort_tlist = new_unsorted_tlist(tlist);

	foreach(i, sortcls)
	{
		SortClause *sortcl = (SortClause *) lfirst(i);
		TargetEntry *tle = get_sortgroupclause_tle(sortcl, sort_tlist);
		Resdom	   *resdom = tle->resdom;

		/*
		 * Check for the possibility of duplicate order-by clauses --- the
		 * parser should have removed 'em, but the executor will get
		 * terribly confused if any get through!
		 */
		if (resdom->reskey == 0)
		{
			/* OK, insert the ordering info needed by the executor. */
			resdom->reskey = ++keyno;
			resdom->reskeyop = get_opcode(sortcl->sortop);
		}
	}

	Assert(keyno > 0);

	return (Plan *) make_sort(sort_tlist, plannode, keyno);
}
