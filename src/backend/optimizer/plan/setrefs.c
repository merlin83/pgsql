/*-------------------------------------------------------------------------
 *
 * setrefs.c--
 *	  Routines to change varno/attno entries to contain references
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/plan/setrefs.c,v 1.7 1997-09-08 20:56:16 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>

#include "postgres.h"

#include "nodes/pg_list.h"
#include "nodes/plannodes.h"
#include "nodes/primnodes.h"
#include "nodes/relation.h"

#include "utils/elog.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"

#include "optimizer/internal.h"
#include "optimizer/clauses.h"
#include "optimizer/clauseinfo.h"
#include "optimizer/keys.h"
#include "optimizer/planmain.h"
#include "optimizer/tlist.h"
#include "optimizer/var.h"
#include "optimizer/tlist.h"

static void set_join_tlist_references(Join * join);
static void set_tempscan_tlist_references(SeqScan * tempscan);
static void set_temp_tlist_references(Temp * temp);
static List *
replace_clause_joinvar_refs(Expr * clause,
							List * outer_tlist, List * inner_tlist);
static List *
replace_subclause_joinvar_refs(List * clauses,
							   List * outer_tlist, List * inner_tlist);
static Var *replace_joinvar_refs(Var * var, List * outer_tlist, List * inner_tlist);
static List *tlist_temp_references(Oid tempid, List * tlist);
static void replace_result_clause(List * clause, List * subplanTargetList);
static bool OperandIsInner(Node * opnd, int inner_relid);
static void replace_agg_clause(Node * expr, List * targetlist);

/*****************************************************************************
 *
 *		SUBPLAN REFERENCES
 *
 *****************************************************************************/

/*
 * set-tlist-references--
 *	  Modifies the target list of nodes in a plan to reference target lists
 *	  at lower levels.
 *
 * 'plan' is the plan whose target list and children's target lists will
 *		be modified
 *
 * Returns nothing of interest, but modifies internal fields of nodes.
 *
 */
void
set_tlist_references(Plan * plan)
{
	if (plan == NULL)
		return;

	if (IsA_Join(plan))
	{
		set_join_tlist_references((Join *) plan);
	}
	else if (IsA(plan, SeqScan) && plan->lefttree &&
			 IsA_Temp(plan->lefttree))
	{
		set_tempscan_tlist_references((SeqScan *) plan);
	}
	else if (IsA(plan, Sort))
	{
		set_temp_tlist_references((Temp *) plan);
	}
	else if (IsA(plan, Result))
	{
		set_result_tlist_references((Result *) plan);
	}
	else if (IsA(plan, Hash))
	{
		set_tlist_references(plan->lefttree);
	}
	else if (IsA(plan, Choose))
	{
		List	   *x;

		foreach(x, ((Choose *) plan)->chooseplanlist)
		{
			set_tlist_references((Plan *) lfirst(x));
		}
	}
}

/*
 * set-join-tlist-references--
 *	  Modifies the target list of a join node by setting the varnos and
 *	  varattnos to reference the target list of the outer and inner join
 *	  relations.
 *
 *	  Creates a target list for a join node to contain references by setting
 *	  varno values to OUTER or INNER and setting attno values to the
 *	  result domain number of either the corresponding outer or inner join
 *	  tuple.
 *
 * 'join' is a join plan node
 *
 * Returns nothing of interest, but modifies internal fields of nodes.
 *
 */
static void
set_join_tlist_references(Join * join)
{
	Plan	   *outer = ((Plan *) join)->lefttree;
	Plan	   *inner = ((Plan *) join)->righttree;
	List	   *new_join_targetlist = NIL;
	TargetEntry *temp = (TargetEntry *) NULL;
	List	   *entry = NIL;
	List	   *inner_tlist = NULL;
	List	   *outer_tlist = NULL;
	TargetEntry *xtl = (TargetEntry *) NULL;
	List	   *qptlist = ((Plan *) join)->targetlist;

	foreach(entry, qptlist)
	{
		List	   *joinvar;

		xtl = (TargetEntry *) lfirst(entry);
		inner_tlist = ((inner == NULL) ? NIL : inner->targetlist);
		outer_tlist = ((outer == NULL) ? NIL : outer->targetlist);
		joinvar = replace_clause_joinvar_refs((Expr *) get_expr(xtl),
											  outer_tlist,
											  inner_tlist);

		temp = MakeTLE(xtl->resdom, (Node *) joinvar);
		new_join_targetlist = lappend(new_join_targetlist, temp);
	}

	((Plan *) join)->targetlist = new_join_targetlist;
	if (outer != NULL)
		set_tlist_references(outer);
	if (inner != NULL)
		set_tlist_references(inner);
}

/*
 * set-tempscan-tlist-references--
 *	  Modifies the target list of a node that scans a temp relation (i.e., a
 *	  sort or hash node) so that the varnos refer to the child temporary.
 *
 * 'tempscan' is a seqscan node
 *
 * Returns nothing of interest, but modifies internal fields of nodes.
 *
 */
static void
set_tempscan_tlist_references(SeqScan * tempscan)
{
	Temp	   *temp = (Temp *) ((Plan *) tempscan)->lefttree;

	((Plan *) tempscan)->targetlist =
		tlist_temp_references(temp->tempid,
							  ((Plan *) tempscan)->targetlist);
	set_temp_tlist_references(temp);
}

/*
 * set-temp-tlist-references--
 *	  The temp's vars are made consistent with (actually, identical to) the
 *	  modified version of the target list of the node from which temp node
 *	  receives its tuples.
 *
 * 'temp' is a temp (e.g., sort, hash) plan node
 *
 * Returns nothing of interest, but modifies internal fields of nodes.
 *
 */
static void
set_temp_tlist_references(Temp * temp)
{
	Plan	   *source = ((Plan *) temp)->lefttree;

	if (source != NULL)
	{
		set_tlist_references(source);
		((Plan *) temp)->targetlist =
			copy_vars(((Plan *) temp)->targetlist,
					  (source)->targetlist);
	}
	else
	{
		elog(WARN, "calling set_temp_tlist_references with empty lefttree");
	}
}

/*
 * join-references--
 *	   Creates a new set of join clauses by replacing the varno/varattno
 *	   values of variables in the clauses to reference target list values
 *	   from the outer and inner join relation target lists.
 *
 * 'clauses' is the list of join clauses
 * 'outer-tlist' is the target list of the outer join relation
 * 'inner-tlist' is the target list of the inner join relation
 *
 * Returns the new join clauses.
 *
 */
List	   *
join_references(List * clauses,
				List * outer_tlist,
				List * inner_tlist)
{
	return (replace_subclause_joinvar_refs(clauses,
										   outer_tlist,
										   inner_tlist));
}

/*
 * index-outerjoin-references--
 *	  Given a list of join clauses, replace the operand corresponding to the
 *	  outer relation in the join with references to the corresponding target
 *	  list element in 'outer-tlist' (the outer is rather obscurely
 *	  identified as the side that doesn't contain a var whose varno equals
 *	  'inner-relid').
 *
 *	  As a side effect, the operator is replaced by the regproc id.
 *
 * 'inner-indxqual' is the list of join clauses (so-called because they
 * are used as qualifications for the inner (inbex) scan of a nestloop)
 *
 * Returns the new list of clauses.
 *
 */
List	   *
index_outerjoin_references(List * inner_indxqual,
						   List * outer_tlist,
						   Index inner_relid)
{
	List	   *t_list = NIL;
	Expr	   *temp = NULL;
	List	   *t_clause = NIL;
	Expr	   *clause = NULL;

	foreach(t_clause, inner_indxqual)
	{
		clause = lfirst(t_clause);

		/*
		 * if inner scan on the right.
		 */
		if (OperandIsInner((Node *) get_rightop(clause), inner_relid))
		{
			Var		   *joinvar = (Var *)
			replace_clause_joinvar_refs((Expr *) get_leftop(clause),
										outer_tlist,
										NIL);

			temp = make_opclause(replace_opid((Oper *) ((Expr *) clause)->oper),
								 joinvar,
								 get_rightop(clause));
			t_list = lappend(t_list, temp);
		}
		else
		{
			/* inner scan on left */
			Var		   *joinvar = (Var *)
			replace_clause_joinvar_refs((Expr *) get_rightop(clause),
										outer_tlist,
										NIL);

			temp = make_opclause(replace_opid((Oper *) ((Expr *) clause)->oper),
								 get_leftop(clause),
								 joinvar);
			t_list = lappend(t_list, temp);
		}

	}
	return (t_list);
}

/*
 * replace-clause-joinvar-refs
 * replace-subclause-joinvar-refs
 * replace-joinvar-refs
 *
 *	  Replaces all variables within a join clause with a new var node
 *	  whose varno/varattno fields contain a reference to a target list
 *	  element from either the outer or inner join relation.
 *
 * 'clause' is the join clause
 * 'outer-tlist' is the target list of the outer join relation
 * 'inner-tlist' is the target list of the inner join relation
 *
 * Returns the new join clause.
 *
 */
static List *
replace_clause_joinvar_refs(Expr * clause,
							List * outer_tlist,
							List * inner_tlist)
{
	List	   *temp = NULL;

	if (IsA(clause, Var))
	{
		temp = (List *) replace_joinvar_refs((Var *) clause,
											 outer_tlist, inner_tlist);
		if (temp)
			return (temp);
		else if (clause != NULL)
			return ((List *) clause);
		else
			return (NIL);
	}
	else if (single_node((Node *) clause))
	{
		return ((List *) clause);
	}
	else if (or_clause((Node *) clause))
	{
		List	   *orclause =
		replace_subclause_joinvar_refs(((Expr *) clause)->args,
									   outer_tlist,
									   inner_tlist);

		return ((List *) make_orclause(orclause));
	}
	else if (IsA(clause, ArrayRef))
	{
		ArrayRef   *aref = (ArrayRef *) clause;

		temp = replace_subclause_joinvar_refs(aref->refupperindexpr,
											  outer_tlist,
											  inner_tlist);
		aref->refupperindexpr = (List *) temp;
		temp = replace_subclause_joinvar_refs(aref->reflowerindexpr,
											  outer_tlist,
											  inner_tlist);
		aref->reflowerindexpr = (List *) temp;
		temp = replace_clause_joinvar_refs((Expr *) aref->refexpr,
										   outer_tlist,
										   inner_tlist);
		aref->refexpr = (Node *) temp;

		/*
		 * no need to set refassgnexpr.  we only set that in the target
		 * list on replaces, and this is an array reference in the
		 * qualification.  if we got this far, it's 0x0 in the ArrayRef
		 * structure 'clause'.
		 */

		return ((List *) clause);
	}
	else if (is_funcclause((Node *) clause))
	{
		List	   *funcclause =
		replace_subclause_joinvar_refs(((Expr *) clause)->args,
									   outer_tlist,
									   inner_tlist);

		return ((List *) make_funcclause((Func *) ((Expr *) clause)->oper,
										 funcclause));
	}
	else if (not_clause((Node *) clause))
	{
		List	   *notclause =
		replace_clause_joinvar_refs(get_notclausearg(clause),
									outer_tlist,
									inner_tlist);

		return ((List *) make_notclause((Expr *) notclause));
	}
	else if (is_opclause((Node *) clause))
	{
		Var		   *leftvar =
		(Var *) replace_clause_joinvar_refs((Expr *) get_leftop(clause),
											outer_tlist,
											inner_tlist);
		Var		   *rightvar =
		(Var *) replace_clause_joinvar_refs((Expr *) get_rightop(clause),
											outer_tlist,
											inner_tlist);

		return ((List *) make_opclause(replace_opid((Oper *) ((Expr *) clause)->oper),
									   leftvar,
									   rightvar));
	}
	/* shouldn't reach here */
	return NULL;
}

static List *
replace_subclause_joinvar_refs(List * clauses,
							   List * outer_tlist,
							   List * inner_tlist)
{
	List	   *t_list = NIL;
	List	   *temp = NIL;
	List	   *clause = NIL;

	foreach(clause, clauses)
	{
		temp = replace_clause_joinvar_refs(lfirst(clause),
										   outer_tlist,
										   inner_tlist);
		t_list = lappend(t_list, temp);
	}
	return (t_list);
}

static Var *
replace_joinvar_refs(Var * var, List * outer_tlist, List * inner_tlist)
{
	Resdom	   *outer_resdom = (Resdom *) NULL;

	outer_resdom = tlist_member(var, outer_tlist);

	if (outer_resdom != NULL && IsA(outer_resdom, Resdom))
	{
		return (makeVar(OUTER,
						outer_resdom->resno,
						var->vartype,
						var->varnoold,
						var->varoattno));
	}
	else
	{
		Resdom	   *inner_resdom;

		inner_resdom = tlist_member(var, inner_tlist);
		if (inner_resdom != NULL && IsA(inner_resdom, Resdom))
		{
			return (makeVar(INNER,
							inner_resdom->resno,
							var->vartype,
							var->varnoold,
							var->varoattno));
		}
	}
	return (Var *) NULL;
}

/*
 * tlist-temp-references--
 *	  Creates a new target list for a node that scans a temp relation,
 *	  setting the varnos to the id of the temp relation and setting varids
 *	  if necessary (varids are only needed if this is a targetlist internal
 *	  to the tree, in which case the targetlist entry always contains a var
 *	  node, so we can just copy it from the temp).
 *
 * 'tempid' is the id of the temp relation
 * 'tlist' is the target list to be modified
 *
 * Returns new target list
 *
 */
static List *
tlist_temp_references(Oid tempid,
					  List * tlist)
{
	List	   *t_list = NIL;
	TargetEntry *temp = (TargetEntry *) NULL;
	TargetEntry *xtl = NULL;
	List	   *entry;

	foreach(entry, tlist)
	{
		AttrNumber	oattno;

		xtl = lfirst(entry);
		if (IsA(get_expr(xtl), Var))
			oattno = ((Var *) xtl->expr)->varoattno;
		else
			oattno = 0;

		temp = MakeTLE(xtl->resdom,
					   (Node *) makeVar(tempid,
										xtl->resdom->resno,
										xtl->resdom->restype,
										tempid,
										oattno));

		t_list = lappend(t_list, temp);
	}
	return (t_list);
}

/*---------------------------------------------------------
 *
 * set_result_tlist_references
 *
 * Change the target list of a Result node, so that it correctly
 * addresses the tuples returned by its left tree subplan.
 *
 * NOTE:
 *	1) we ignore the right tree! (in the current implementation
 *	   it is always nil
 *	2) this routine will probably *NOT* work with nested dot
 *	   fields....
 */
void
set_result_tlist_references(Result * resultNode)
{
	Plan	   *subplan;
	List	   *resultTargetList;
	List	   *subplanTargetList;
	List	   *t;
	TargetEntry *entry;
	Expr	   *expr;

	resultTargetList = ((Plan *) resultNode)->targetlist;

	/*
	 * NOTE: we only consider the left tree subplan. This is usually a seq
	 * scan.
	 */
	subplan = ((Plan *) resultNode)->lefttree;
	if (subplan != NULL)
	{
		subplanTargetList = subplan->targetlist;
	}
	else
	{
		subplanTargetList = NIL;
	}

	/*
	 * now for traverse all the entris of the target list. These should be
	 * of the form (Resdom_Node Expression). For every expression clause,
	 * call "replace_result_clause()" to appropriatelly change all the Var
	 * nodes.
	 */
	foreach(t, resultTargetList)
	{
		entry = (TargetEntry *) lfirst(t);
		expr = (Expr *) get_expr(entry);
		replace_result_clause((List *) expr, subplanTargetList);
	}
}

/*---------------------------------------------------------
 *
 * replace_result_clause
 *
 * This routine is called from set_result_tlist_references().
 * and modifies the expressions of the target list of a Result
 * node so that all Var nodes reference the target list of its subplan.
 *
 */
static void
replace_result_clause(List * clause,
					  List * subplanTargetList) /* target list of the
												 * subplan */
{
	List	   *t;
	List	   *subClause;
	TargetEntry *subplanVar;

	if (IsA(clause, Var))
	{

		/*
		 * Ha! A Var node!
		 */
		subplanVar = match_varid((Var *) clause, subplanTargetList);

		/*
		 * Change the varno & varattno fields of the var node.
		 *
		 */
		((Var *) clause)->varno = (Index) OUTER;
		((Var *) clause)->varattno = subplanVar->resdom->resno;
	}
	else if (is_funcclause((Node *) clause))
	{

		/*
		 * This is a function. Recursively call this routine for its
		 * arguments...
		 */
		subClause = ((Expr *) clause)->args;
		foreach(t, subClause)
		{
			replace_result_clause(lfirst(t), subplanTargetList);
		}
	}
	else if (IsA(clause, ArrayRef))
	{
		ArrayRef   *aref = (ArrayRef *) clause;

		/*
		 * This is an arrayref. Recursively call this routine for its
		 * expression and its index expression...
		 */
		subClause = aref->refupperindexpr;
		foreach(t, subClause)
		{
			replace_result_clause(lfirst(t), subplanTargetList);
		}
		subClause = aref->reflowerindexpr;
		foreach(t, subClause)
		{
			replace_result_clause(lfirst(t), subplanTargetList);
		}
		replace_result_clause((List *) aref->refexpr,
							  subplanTargetList);
		replace_result_clause((List *) aref->refassgnexpr,
							  subplanTargetList);
	}
	else if (is_opclause((Node *) clause))
	{

		/*
		 * This is an operator. Recursively call this routine for both its
		 * left and right operands
		 */
		subClause = (List *) get_leftop((Expr *) clause);
		replace_result_clause(subClause, subplanTargetList);
		subClause = (List *) get_rightop((Expr *) clause);
		replace_result_clause(subClause, subplanTargetList);
	}
	else if (IsA(clause, Param) || IsA(clause, Const))
	{
		/* do nothing! */
	}
	else
	{

		/*
		 * Ooops! we can not handle that!
		 */
		elog(WARN, "replace_result_clause: Can not handle this tlist!\n");
	}
}

static
			bool
OperandIsInner(Node * opnd, int inner_relid)
{

	/*
	 * Can be the inner scan if its a varnode or a function and the
	 * inner_relid is equal to the varnode's var number or in the case of
	 * a function the first argument's var number (all args in a
	 * functional index are from the same relation).
	 */
	if (IsA(opnd, Var) &&
		(inner_relid == ((Var *) opnd)->varno))
	{
		return true;
	}
	if (is_funcclause(opnd))
	{
		List	   *firstArg = lfirst(((Expr *) opnd)->args);

		if (IsA(firstArg, Var) &&
			(inner_relid == ((Var *) firstArg)->varno))
		{
			return true;
		}
	}
	return false;
}

/*****************************************************************************
 *
 *****************************************************************************/

/*---------------------------------------------------------
 *
 * set_agg_tlist_references -
 *	  changes the target list of an Agg node so that it points to
 *	  the tuples returned by its left tree subplan.
 *
 */
void
set_agg_tlist_references(Agg *aggNode)
{
	List	   *aggTargetList;
	List	   *subplanTargetList;
	List	   *tl;

	aggTargetList = aggNode->plan.targetlist;
	subplanTargetList = aggNode->plan.lefttree->targetlist;

	foreach(tl, aggTargetList)
	{
		TargetEntry *tle = lfirst(tl);

		replace_agg_clause(tle->expr, subplanTargetList);
	}
}

void
set_agg_agglist_references(Agg *aggNode)
{
	List	   *subplanTargetList;
	Aggreg	  **aggs;
	int			i;

	aggs = aggNode->aggs;
	subplanTargetList = aggNode->plan.lefttree->targetlist;

	for (i = 0; i < aggNode->numAgg; i++)
	{
		replace_agg_clause(aggs[i]->target, subplanTargetList);
	}
}

static void
replace_agg_clause(Node * clause, List * subplanTargetList)
{
	List	   *t;
	TargetEntry *subplanVar;

	if (IsA(clause, Var))
	{

		/*
		 * Ha! A Var node!
		 */
		subplanVar = match_varid((Var *) clause, subplanTargetList);

		/*
		 * Change the varno & varattno fields of the var node.
		 *
		 */
		((Var *) clause)->varattno = subplanVar->resdom->resno;
	}
	else if (is_funcclause(clause))
	{

		/*
		 * This is a function. Recursively call this routine for its
		 * arguments...
		 */
		foreach(t, ((Expr *) clause)->args)
		{
			replace_agg_clause(lfirst(t), subplanTargetList);
		}
	}
	else if (IsA(clause, Aggreg))
	{
		replace_agg_clause(((Aggreg *) clause)->target, subplanTargetList);
	}
	else if (IsA(clause, ArrayRef))
	{
		ArrayRef   *aref = (ArrayRef *) clause;

		/*
		 * This is an arrayref. Recursively call this routine for its
		 * expression and its index expression...
		 */
		foreach(t, aref->refupperindexpr)
		{
			replace_agg_clause(lfirst(t), subplanTargetList);
		}
		foreach(t, aref->reflowerindexpr)
		{
			replace_agg_clause(lfirst(t), subplanTargetList);
		}
		replace_agg_clause(aref->refexpr, subplanTargetList);
		replace_agg_clause(aref->refassgnexpr, subplanTargetList);
	}
	else if (is_opclause(clause))
	{

		/*
		 * This is an operator. Recursively call this routine for both its
		 * left and right operands
		 */
		Node	   *left = (Node *) get_leftop((Expr *) clause);
		Node	   *right = (Node *) get_rightop((Expr *) clause);

		if (left != (Node *) NULL)
			replace_agg_clause(left, subplanTargetList);
		if (right != (Node *) NULL)
			replace_agg_clause(right, subplanTargetList);
	}
	else if (IsA(clause, Param) || IsA(clause, Const))
	{
		/* do nothing! */
	}
	else
	{

		/*
		 * Ooops! we can not handle that!
		 */
		elog(WARN, "replace_agg_clause: Can not handle this tlist!\n");
	}

}
