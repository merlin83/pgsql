/*
 * explain.c
 *	  Explain the query execution plan
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994-5, Regents of the University of California
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/explain.c,v 1.97 2002-12-13 19:45:49 tgl Exp $
 *
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/pg_type.h"
#include "commands/explain.h"
#include "executor/executor.h"
#include "executor/instrument.h"
#include "lib/stringinfo.h"
#include "nodes/print.h"
#include "optimizer/clauses.h"
#include "optimizer/planner.h"
#include "optimizer/var.h"
#include "parser/parsetree.h"
#include "rewrite/rewriteHandler.h"
#include "tcop/pquery.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"


typedef struct ExplainState
{
	/* options */
	bool		printCost;		/* print cost */
	bool		printNodes;		/* do nodeToString() too */
	bool		printAnalyze;	/* print actual times */
	/* other states */
	List	   *rtable;			/* range table */
} ExplainState;

static void ExplainOneQuery(Query *query, ExplainStmt *stmt,
				TupOutputState *tstate);
static double elapsed_time(struct timeval *starttime);
static void explain_outNode(StringInfo str,
							Plan *plan, PlanState *planstate,
							Plan *outer_plan,
							int indent, ExplainState *es);
static void show_scan_qual(List *qual, bool is_or_qual, const char *qlabel,
			   int scanrelid, Plan *outer_plan,
			   StringInfo str, int indent, ExplainState *es);
static void show_upper_qual(List *qual, const char *qlabel,
				const char *outer_name, int outer_varno, Plan *outer_plan,
				const char *inner_name, int inner_varno, Plan *inner_plan,
				StringInfo str, int indent, ExplainState *es);
static void show_sort_keys(List *tlist, int nkeys, const char *qlabel,
			   StringInfo str, int indent, ExplainState *es);
static Node *make_ors_ands_explicit(List *orclauses);

/*
 * ExplainQuery -
 *	  execute an EXPLAIN command
 */
void
ExplainQuery(ExplainStmt *stmt, CommandDest dest)
{
	Query	   *query = stmt->query;
	TupOutputState *tstate;
	TupleDesc	tupdesc;
	List	   *rewritten;
	List	   *l;

	/* need a tuple descriptor representing a single TEXT column */
	tupdesc = CreateTemplateTupleDesc(1, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "QUERY PLAN",
					   TEXTOID, -1, 0, false);

	/* prepare for projection of tuples */
	tstate = begin_tup_output_tupdesc(dest, tupdesc);

	if (query->commandType == CMD_UTILITY)
	{
		/* rewriter will not cope with utility statements */
		do_text_output_oneline(tstate, "Utility statements have no plan structure");
	}
	else
	{
		/* Rewrite through rule system */
		rewritten = QueryRewrite(query);

		if (rewritten == NIL)
		{
			/* In the case of an INSTEAD NOTHING, tell at least that */
			do_text_output_oneline(tstate, "Query rewrites to nothing");
		}
		else
		{
			/* Explain every plan */
			foreach(l, rewritten)
			{
				ExplainOneQuery(lfirst(l), stmt, tstate);
				/* put a blank line between plans */
				if (lnext(l) != NIL)
					do_text_output_oneline(tstate, "");
			}
		}
	}

	end_tup_output(tstate);
}

/*
 * ExplainOneQuery -
 *	  print out the execution plan for one query
 */
static void
ExplainOneQuery(Query *query, ExplainStmt *stmt, TupOutputState *tstate)
{
	Plan	   *plan;
	QueryDesc  *queryDesc;
	ExplainState *es;
	StringInfo	str;
	double		totaltime = 0;
	struct timeval starttime;

	/* planner will not cope with utility statements */
	if (query->commandType == CMD_UTILITY)
	{
		if (query->utilityStmt && IsA(query->utilityStmt, NotifyStmt))
			do_text_output_oneline(tstate, "NOTIFY");
		else
			do_text_output_oneline(tstate, "UTILITY");
		return;
	}

	/* plan the query */
	plan = planner(query);

	/* pg_plan could have failed */
	if (plan == NULL)
		return;

	/* We don't support DECLARE CURSOR here */
	Assert(!query->isPortal);

	gettimeofday(&starttime, NULL);

	/* Create a QueryDesc requesting no output */
	queryDesc = CreateQueryDesc(query, plan, None, NULL, NULL,
								stmt->analyze);

	/* call ExecutorStart to prepare the plan for execution */
	ExecutorStart(queryDesc);

	/* Execute the plan for statistics if asked for */
	if (stmt->analyze)
	{
		/* run the plan */
		ExecutorRun(queryDesc, ForwardScanDirection, 0L);

		/* We can't clean up 'till we're done printing the stats... */

		totaltime += elapsed_time(&starttime);
	}

	es = (ExplainState *) palloc0(sizeof(ExplainState));

	es->printCost = true;		/* default */
	es->printNodes = stmt->verbose;
	es->printAnalyze = stmt->analyze;
	es->rtable = query->rtable;

	if (es->printNodes)
	{
		char	   *s;
		char	   *f;

		s = nodeToString(plan);
		if (s)
		{
			if (Explain_pretty_print)
				f = pretty_format_node_dump(s);
			else
				f = format_node_dump(s);
			pfree(s);
			do_text_output_multiline(tstate, f);
			pfree(f);
			if (es->printCost)
				do_text_output_oneline(tstate, "");		/* separator line */
		}
	}

	str = makeStringInfo();

	if (es->printCost)
	{
		explain_outNode(str, plan, queryDesc->planstate,
						NULL, 0, es);
	}

	/*
	 * Close down the query and free resources.  Include time for this
	 * in the total runtime.
	 */
	gettimeofday(&starttime, NULL);

	ExecutorEnd(queryDesc);
	CommandCounterIncrement();

	totaltime += elapsed_time(&starttime);

	if (es->printCost)
	{
		if (stmt->analyze)
			appendStringInfo(str, "Total runtime: %.2f msec\n",
							 1000.0 * totaltime);
		do_text_output_multiline(tstate, str->data);
	}

	pfree(str->data);
	pfree(str);
	pfree(es);
}

/* Compute elapsed time in seconds since given gettimeofday() timestamp */
static double
elapsed_time(struct timeval *starttime)
{
	struct timeval endtime;

	gettimeofday(&endtime, NULL);

	endtime.tv_sec -= starttime->tv_sec;
	endtime.tv_usec -= starttime->tv_usec;
	while (endtime.tv_usec < 0)
	{
		endtime.tv_usec += 1000000;
		endtime.tv_sec--;
	}
	return (double) endtime.tv_sec +
		(double) endtime.tv_usec / 1000000.0;
}

/*
 * explain_outNode -
 *	  converts a Plan node into ascii string and appends it to 'str'
 *
 * planstate points to the executor state node corresponding to the plan node.
 * We need this to get at the instrumentation data (if any) as well as the
 * list of subplans.
 *
 * outer_plan, if not null, references another plan node that is the outer
 * side of a join with the current node.  This is only interesting for
 * deciphering runtime keys of an inner indexscan.
 */
static void
explain_outNode(StringInfo str,
				Plan *plan, PlanState *planstate,
				Plan *outer_plan,
				int indent, ExplainState *es)
{
	List	   *l;
	char	   *pname;
	int			i;

	if (plan == NULL)
	{
		appendStringInfo(str, "\n");
		return;
	}

	switch (nodeTag(plan))
	{
		case T_Result:
			pname = "Result";
			break;
		case T_Append:
			pname = "Append";
			break;
		case T_NestLoop:
			pname = "Nested Loop";
			break;
		case T_MergeJoin:
			pname = "Merge Join";
			break;
		case T_HashJoin:
			pname = "Hash Join";
			break;
		case T_SeqScan:
			pname = "Seq Scan";
			break;
		case T_IndexScan:
			pname = "Index Scan";
			break;
		case T_TidScan:
			pname = "Tid Scan";
			break;
		case T_SubqueryScan:
			pname = "Subquery Scan";
			break;
		case T_FunctionScan:
			pname = "Function Scan";
			break;
		case T_Material:
			pname = "Materialize";
			break;
		case T_Sort:
			pname = "Sort";
			break;
		case T_Group:
			pname = "Group";
			break;
		case T_Agg:
			switch (((Agg *) plan)->aggstrategy)
			{
				case AGG_PLAIN:
					pname = "Aggregate";
					break;
				case AGG_SORTED:
					pname = "GroupAggregate";
					break;
				case AGG_HASHED:
					pname = "HashAggregate";
					break;
				default:
					pname = "Aggregate ???";
					break;
			}
			break;
		case T_Unique:
			pname = "Unique";
			break;
		case T_SetOp:
			switch (((SetOp *) plan)->cmd)
			{
				case SETOPCMD_INTERSECT:
					pname = "SetOp Intersect";
					break;
				case SETOPCMD_INTERSECT_ALL:
					pname = "SetOp Intersect All";
					break;
				case SETOPCMD_EXCEPT:
					pname = "SetOp Except";
					break;
				case SETOPCMD_EXCEPT_ALL:
					pname = "SetOp Except All";
					break;
				default:
					pname = "SetOp ???";
					break;
			}
			break;
		case T_Limit:
			pname = "Limit";
			break;
		case T_Hash:
			pname = "Hash";
			break;
		default:
			pname = "???";
			break;
	}

	appendStringInfo(str, pname);
	switch (nodeTag(plan))
	{
		case T_IndexScan:
			if (ScanDirectionIsBackward(((IndexScan *) plan)->indxorderdir))
				appendStringInfo(str, " Backward");
			appendStringInfo(str, " using ");
			i = 0;
			foreach(l, ((IndexScan *) plan)->indxid)
			{
				Relation	relation;

				relation = index_open(lfirsti(l));
				appendStringInfo(str, "%s%s",
								 (++i > 1) ? ", " : "",
					quote_identifier(RelationGetRelationName(relation)));
				index_close(relation);
			}
			/* FALL THRU */
		case T_SeqScan:
		case T_TidScan:
			if (((Scan *) plan)->scanrelid > 0)
			{
				RangeTblEntry *rte = rt_fetch(((Scan *) plan)->scanrelid,
											  es->rtable);
				char	   *relname;

				/* Assume it's on a real relation */
				Assert(rte->rtekind == RTE_RELATION);

				/* We only show the rel name, not schema name */
				relname = get_rel_name(rte->relid);

				appendStringInfo(str, " on %s",
								 quote_identifier(relname));
				if (strcmp(rte->eref->aliasname, relname) != 0)
					appendStringInfo(str, " %s",
								 quote_identifier(rte->eref->aliasname));
			}
			break;
		case T_SubqueryScan:
			if (((Scan *) plan)->scanrelid > 0)
			{
				RangeTblEntry *rte = rt_fetch(((Scan *) plan)->scanrelid,
											  es->rtable);

				appendStringInfo(str, " %s",
								 quote_identifier(rte->eref->aliasname));
			}
			break;
		case T_FunctionScan:
			if (((Scan *) plan)->scanrelid > 0)
			{
				RangeTblEntry *rte = rt_fetch(((Scan *) plan)->scanrelid,
											  es->rtable);
				char	   *proname;

				/* Assert it's on a RangeFunction */
				Assert(rte->rtekind == RTE_FUNCTION);

				/*
				 * If the expression is still a function call, we can get
				 * the real name of the function.  Otherwise, punt (this
				 * can happen if the optimizer simplified away the function
				 * call, for example).
				 */
				if (rte->funcexpr && IsA(rte->funcexpr, FuncExpr))
				{
					FuncExpr   *funcexpr = (FuncExpr *) rte->funcexpr;
					Oid			funcid = funcexpr->funcid;

					/* We only show the func name, not schema name */
					proname = get_func_name(funcid);
				}
				else
					proname = rte->eref->aliasname;

				appendStringInfo(str, " on %s",
								 quote_identifier(proname));
				if (strcmp(rte->eref->aliasname, proname) != 0)
					appendStringInfo(str, " %s",
								 quote_identifier(rte->eref->aliasname));
			}
			break;
		default:
			break;
	}
	if (es->printCost)
	{
		appendStringInfo(str, "  (cost=%.2f..%.2f rows=%.0f width=%d)",
						 plan->startup_cost, plan->total_cost,
						 plan->plan_rows, plan->plan_width);

		/*
		 * We have to forcibly clean up the instrumentation state because
		 * we haven't done ExecutorEnd yet.  This is pretty grotty ...
		 */
		InstrEndLoop(planstate->instrument);

		if (planstate->instrument && planstate->instrument->nloops > 0)
		{
			double		nloops = planstate->instrument->nloops;

			appendStringInfo(str, " (actual time=%.2f..%.2f rows=%.0f loops=%.0f)",
							 1000.0 * planstate->instrument->startup / nloops,
							 1000.0 * planstate->instrument->total / nloops,
							 planstate->instrument->ntuples / nloops,
							 planstate->instrument->nloops);
		}
		else if (es->printAnalyze)
		{
			appendStringInfo(str, " (never executed)");
		}
	}
	appendStringInfo(str, "\n");

	/* quals, sort keys, etc */
	switch (nodeTag(plan))
	{
		case T_IndexScan:
			show_scan_qual(((IndexScan *) plan)->indxqualorig, true,
						   "Index Cond",
						   ((Scan *) plan)->scanrelid,
						   outer_plan,
						   str, indent, es);
			show_scan_qual(plan->qual, false,
						   "Filter",
						   ((Scan *) plan)->scanrelid,
						   outer_plan,
						   str, indent, es);
			break;
		case T_SeqScan:
		case T_TidScan:
		case T_SubqueryScan:
		case T_FunctionScan:
			show_scan_qual(plan->qual, false,
						   "Filter",
						   ((Scan *) plan)->scanrelid,
						   outer_plan,
						   str, indent, es);
			break;
		case T_NestLoop:
			show_upper_qual(((NestLoop *) plan)->join.joinqual,
							"Join Filter",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			show_upper_qual(plan->qual,
							"Filter",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			break;
		case T_MergeJoin:
			show_upper_qual(((MergeJoin *) plan)->mergeclauses,
							"Merge Cond",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			show_upper_qual(((MergeJoin *) plan)->join.joinqual,
							"Join Filter",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			show_upper_qual(plan->qual,
							"Filter",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			break;
		case T_HashJoin:
			show_upper_qual(((HashJoin *) plan)->hashclauses,
							"Hash Cond",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			show_upper_qual(((HashJoin *) plan)->join.joinqual,
							"Join Filter",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			show_upper_qual(plan->qual,
							"Filter",
							"outer", OUTER, outerPlan(plan),
							"inner", INNER, innerPlan(plan),
							str, indent, es);
			break;
		case T_Agg:
		case T_Group:
			show_upper_qual(plan->qual,
							"Filter",
							"subplan", 0, outerPlan(plan),
							"", 0, NULL,
							str, indent, es);
			break;
		case T_Sort:
			show_sort_keys(plan->targetlist, ((Sort *) plan)->keycount,
						   "Sort Key",
						   str, indent, es);
			break;
		case T_Result:
			show_upper_qual((List *) ((Result *) plan)->resconstantqual,
							"One-Time Filter",
							"subplan", OUTER, outerPlan(plan),
							"", 0, NULL,
							str, indent, es);
			show_upper_qual(plan->qual,
							"Filter",
							"subplan", OUTER, outerPlan(plan),
							"", 0, NULL,
							str, indent, es);
			break;
		default:
			break;
	}

	/* initPlan-s */
	if (plan->initPlan)
	{
		List	   *saved_rtable = es->rtable;
		List	   *lst;

		for (i = 0; i < indent; i++)
			appendStringInfo(str, "  ");
		appendStringInfo(str, "  InitPlan\n");
		foreach(lst, planstate->initPlan)
		{
			SubPlanExprState *sps = (SubPlanExprState *) lfirst(lst);
			SubPlanExpr *sp = (SubPlanExpr *) sps->xprstate.expr;

			es->rtable = sp->rtable;
			for (i = 0; i < indent; i++)
				appendStringInfo(str, "  ");
			appendStringInfo(str, "    ->  ");
			explain_outNode(str, sp->plan,
							sps->planstate,
							NULL,
							indent + 4, es);
		}
		es->rtable = saved_rtable;
	}

	/* lefttree */
	if (outerPlan(plan))
	{
		for (i = 0; i < indent; i++)
			appendStringInfo(str, "  ");
		appendStringInfo(str, "  ->  ");
		explain_outNode(str, outerPlan(plan),
						outerPlanState(planstate),
						NULL,
						indent + 3, es);
	}

	/* righttree */
	if (innerPlan(plan))
	{
		for (i = 0; i < indent; i++)
			appendStringInfo(str, "  ");
		appendStringInfo(str, "  ->  ");
		explain_outNode(str, innerPlan(plan),
						innerPlanState(planstate),
						outerPlan(plan),
						indent + 3, es);
	}

	if (IsA(plan, Append))
	{
		Append	   *appendplan = (Append *) plan;
		AppendState *appendstate = (AppendState *) planstate;
		List	   *lst;
		int			j;

		j = 0;
		foreach(lst, appendplan->appendplans)
		{
			Plan	   *subnode = (Plan *) lfirst(lst);

			for (i = 0; i < indent; i++)
				appendStringInfo(str, "  ");
			appendStringInfo(str, "  ->  ");

			explain_outNode(str, subnode,
							appendstate->appendplans[j],
							NULL,
							indent + 3, es);
			j++;
		}
	}

	if (IsA(plan, SubqueryScan))
	{
		SubqueryScan *subqueryscan = (SubqueryScan *) plan;
		SubqueryScanState *subquerystate = (SubqueryScanState *) planstate;
		Plan	   *subnode = subqueryscan->subplan;
		RangeTblEntry *rte = rt_fetch(subqueryscan->scan.scanrelid,
									  es->rtable);
		List	   *saved_rtable = es->rtable;

		Assert(rte->rtekind == RTE_SUBQUERY);
		es->rtable = rte->subquery->rtable;

		for (i = 0; i < indent; i++)
			appendStringInfo(str, "  ");
		appendStringInfo(str, "  ->  ");

		explain_outNode(str, subnode,
						subquerystate->subplan,
						NULL,
						indent + 3, es);

		es->rtable = saved_rtable;
	}

	/* subPlan-s */
	if (planstate->subPlan)
	{
		List	   *saved_rtable = es->rtable;
		List	   *lst;

		for (i = 0; i < indent; i++)
			appendStringInfo(str, "  ");
		appendStringInfo(str, "  SubPlan\n");
		foreach(lst, planstate->subPlan)
		{
			SubPlanExprState *sps = (SubPlanExprState *) lfirst(lst);
			SubPlanExpr *sp = (SubPlanExpr *) sps->xprstate.expr;

			es->rtable = sp->rtable;
			for (i = 0; i < indent; i++)
				appendStringInfo(str, "  ");
			appendStringInfo(str, "    ->  ");
			explain_outNode(str, sp->plan,
							sps->planstate,
							NULL,
							indent + 4, es);
		}
		es->rtable = saved_rtable;
	}
}

/*
 * Show a qualifier expression for a scan plan node
 */
static void
show_scan_qual(List *qual, bool is_or_qual, const char *qlabel,
			   int scanrelid, Plan *outer_plan,
			   StringInfo str, int indent, ExplainState *es)
{
	RangeTblEntry *rte;
	Node	   *scancontext;
	Node	   *outercontext;
	List	   *context;
	Node	   *node;
	char	   *exprstr;
	int			i;

	/* No work if empty qual */
	if (qual == NIL)
		return;
	if (is_or_qual)
	{
		if (lfirst(qual) == NIL && lnext(qual) == NIL)
			return;
	}

	/* Fix qual --- indexqual requires different processing */
	if (is_or_qual)
		node = make_ors_ands_explicit(qual);
	else
		node = (Node *) make_ands_explicit(qual);

	/* Generate deparse context */
	Assert(scanrelid > 0 && scanrelid <= length(es->rtable));
	rte = rt_fetch(scanrelid, es->rtable);
	scancontext = deparse_context_for_rte(rte);

	/*
	 * If we have an outer plan that is referenced by the qual, add it to
	 * the deparse context.  If not, don't (so that we don't force
	 * prefixes unnecessarily).
	 */
	if (outer_plan)
	{
		if (intMember(OUTER, pull_varnos(node)))
			outercontext = deparse_context_for_subplan("outer",
												  outer_plan->targetlist,
													   es->rtable);
		else
			outercontext = NULL;
	}
	else
		outercontext = NULL;

	context = deparse_context_for_plan(scanrelid, scancontext,
									   OUTER, outercontext,
									   NIL);

	/* Deparse the expression */
	exprstr = deparse_expression(node, context, (outercontext != NULL), false);

	/* And add to str */
	for (i = 0; i < indent; i++)
		appendStringInfo(str, "  ");
	appendStringInfo(str, "  %s: %s\n", qlabel, exprstr);
}

/*
 * Show a qualifier expression for an upper-level plan node
 */
static void
show_upper_qual(List *qual, const char *qlabel,
				const char *outer_name, int outer_varno, Plan *outer_plan,
				const char *inner_name, int inner_varno, Plan *inner_plan,
				StringInfo str, int indent, ExplainState *es)
{
	List	   *context;
	Node	   *outercontext;
	Node	   *innercontext;
	Node	   *node;
	char	   *exprstr;
	int			i;

	/* No work if empty qual */
	if (qual == NIL)
		return;

	/* Generate deparse context */
	if (outer_plan)
		outercontext = deparse_context_for_subplan(outer_name,
												   outer_plan->targetlist,
												   es->rtable);
	else
		outercontext = NULL;
	if (inner_plan)
		innercontext = deparse_context_for_subplan(inner_name,
												   inner_plan->targetlist,
												   es->rtable);
	else
		innercontext = NULL;
	context = deparse_context_for_plan(outer_varno, outercontext,
									   inner_varno, innercontext,
									   NIL);

	/* Deparse the expression */
	node = (Node *) make_ands_explicit(qual);
	exprstr = deparse_expression(node, context, (inner_plan != NULL), false);

	/* And add to str */
	for (i = 0; i < indent; i++)
		appendStringInfo(str, "  ");
	appendStringInfo(str, "  %s: %s\n", qlabel, exprstr);
}

/*
 * Show the sort keys for a Sort node.
 */
static void
show_sort_keys(List *tlist, int nkeys, const char *qlabel,
			   StringInfo str, int indent, ExplainState *es)
{
	List	   *context;
	bool		useprefix;
	int			keyno;
	List	   *tl;
	char	   *exprstr;
	int			i;

	if (nkeys <= 0)
		return;

	for (i = 0; i < indent; i++)
		appendStringInfo(str, "  ");
	appendStringInfo(str, "  %s: ", qlabel);

	/*
	 * In this routine we expect that the plan node's tlist has not been
	 * processed by set_plan_references().	Normally, any Vars will
	 * contain valid varnos referencing the actual rtable.	But we might
	 * instead be looking at a dummy tlist generated by prepunion.c; if
	 * there are Vars with zero varno, use the tlist itself to determine
	 * their names.
	 */
	if (intMember(0, pull_varnos((Node *) tlist)))
	{
		Node	   *outercontext;

		outercontext = deparse_context_for_subplan("sort",
												   tlist,
												   es->rtable);
		context = deparse_context_for_plan(0, outercontext,
										   0, NULL,
										   NIL);
		useprefix = false;
	}
	else
	{
		context = deparse_context_for_plan(0, NULL,
										   0, NULL,
										   es->rtable);
		useprefix = length(es->rtable) > 1;
	}

	for (keyno = 1; keyno <= nkeys; keyno++)
	{
		/* find key expression in tlist */
		foreach(tl, tlist)
		{
			TargetEntry *target = (TargetEntry *) lfirst(tl);

			if (target->resdom->reskey == keyno)
			{
				/* Deparse the expression, showing any top-level cast */
				exprstr = deparse_expression((Node *) target->expr, context,
											 useprefix, true);
				/* And add to str */
				if (keyno > 1)
					appendStringInfo(str, ", ");
				appendStringInfo(str, "%s", exprstr);
				break;
			}
		}
		if (tl == NIL)
			elog(ERROR, "show_sort_keys: no tlist entry for key %d", keyno);
	}

	appendStringInfo(str, "\n");
}

/*
 * Indexscan qual lists have an implicit OR-of-ANDs structure.	Make it
 * explicit so deparsing works properly.
 */
static Node *
make_ors_ands_explicit(List *orclauses)
{
	if (orclauses == NIL)
		return NULL;			/* probably can't happen */
	else if (lnext(orclauses) == NIL)
		return (Node *) make_ands_explicit(lfirst(orclauses));
	else
	{
		List	   *args = NIL;
		List	   *orptr;

		foreach(orptr, orclauses)
			args = lappend(args, make_ands_explicit(lfirst(orptr)));

		return (Node *) make_orclause(args);
	}
}
