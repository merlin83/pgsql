/*-------------------------------------------------------------------------
 *
 * initsplan.c
 *	  Target list, qualification, joininfo initialization routines
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/plan/initsplan.c,v 1.84 2003-02-08 20:20:54 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/pg_operator.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/joininfo.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "optimizer/planmain.h"
#include "optimizer/tlist.h"
#include "optimizer/var.h"
#include "parser/parsetree.h"
#include "parser/parse_expr.h"
#include "parser/parse_oper.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


static void mark_baserels_for_outer_join(Query *root, Relids rels,
							 Relids outerrels);
static void distribute_qual_to_rels(Query *root, Node *clause,
						bool ispusheddown,
						bool isouterjoin,
						bool isdeduced,
						Relids qualscope);
static void add_vars_to_targetlist(Query *root, List *vars);
static bool qual_is_redundant(Query *root, RestrictInfo *restrictinfo,
				  List *restrictlist);
static void check_mergejoinable(RestrictInfo *restrictinfo);
static void check_hashjoinable(RestrictInfo *restrictinfo);


/*****************************************************************************
 *
 *	 JOIN TREES
 *
 *****************************************************************************/

/*
 * add_base_rels_to_query
 *
 *	  Scan the query's jointree and create baserel RelOptInfos for all
 *	  the base relations (ie, table, subquery, and function RTEs)
 *	  appearing in the jointree.
 *
 * At the end of this process, there should be one baserel RelOptInfo for
 * every non-join RTE that is used in the query.  Therefore, this routine
 * is the only place that should call build_base_rel.  But build_other_rel
 * will be used later to build rels for inheritance children.
 */
void
add_base_rels_to_query(Query *root, Node *jtnode)
{
	if (jtnode == NULL)
		return;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		build_base_rel(root, varno);
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *l;

		foreach(l, f->fromlist)
		{
			add_base_rels_to_query(root, lfirst(l));
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		add_base_rels_to_query(root, j->larg);
		add_base_rels_to_query(root, j->rarg);
		/*
		 * Safety check: join RTEs should not be SELECT FOR UPDATE targets
		 */
		if (intMember(j->rtindex, root->rowMarks))
			elog(ERROR, "SELECT FOR UPDATE cannot be applied to a join");
	}
	else
		elog(ERROR, "add_base_rels_to_query: unexpected node type %d",
			 nodeTag(jtnode));
}


/*****************************************************************************
 *
 *	 TARGET LISTS
 *
 *****************************************************************************/

/*
 * build_base_rel_tlists
 *	  Creates targetlist entries for each var seen in 'tlist' and adds
 *	  them to the tlist of the appropriate rel node.
 */
void
build_base_rel_tlists(Query *root, List *tlist)
{
	List	   *tlist_vars = pull_var_clause((Node *) tlist, false);

	add_vars_to_targetlist(root, tlist_vars);
	freeList(tlist_vars);
}

/*
 * add_vars_to_targetlist
 *	  For each variable appearing in the list, add it to the owning
 *	  relation's targetlist if not already present.
 */
static void
add_vars_to_targetlist(Query *root, List *vars)
{
	List	   *temp;

	foreach(temp, vars)
	{
		Var		   *var = (Var *) lfirst(temp);
		RelOptInfo *rel = find_base_rel(root, var->varno);

		add_var_to_tlist(rel, var);
	}
}


/*****************************************************************************
 *
 *	  QUALIFICATIONS
 *
 *****************************************************************************/


/*
 * distribute_quals_to_rels
 *	  Recursively scan the query's join tree for WHERE and JOIN/ON qual
 *	  clauses, and add these to the appropriate RestrictInfo and JoinInfo
 *	  lists belonging to base RelOptInfos.	Also, base RelOptInfos are marked
 *	  with outerjoinset information, to aid in proper positioning of qual
 *	  clauses that appear above outer joins.
 *
 * NOTE: when dealing with inner joins, it is appropriate to let a qual clause
 * be evaluated at the lowest level where all the variables it mentions are
 * available.  However, we cannot push a qual down into the nullable side(s)
 * of an outer join since the qual might eliminate matching rows and cause a
 * NULL row to be incorrectly emitted by the join.	Therefore, rels appearing
 * within the nullable side(s) of an outer join are marked with
 *		outerjoinset = set of Relids used at the outer join node.
 * This set will be added to the set of rels referenced by quals using such
 * a rel, thereby forcing them up the join tree to the right level.
 *
 * To ease the calculation of these values, distribute_quals_to_rels() returns
 * the set of base Relids involved in its own level of join.  This is just an
 * internal convenience; no outside callers pay attention to the result.
 */
Relids
distribute_quals_to_rels(Query *root, Node *jtnode)
{
	Relids		result = NULL;

	if (jtnode == NULL)
		return result;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		/* No quals to deal with, just return correct result */
		result = bms_make_singleton(varno);
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *l;
		List	   *qual;

		/*
		 * First, recurse to handle child joins.
		 */
		foreach(l, f->fromlist)
		{
			result = bms_add_members(result,
									 distribute_quals_to_rels(root,
															  lfirst(l)));
		}

		/*
		 * Now process the top-level quals.  These are always marked as
		 * "pushed down", since they clearly didn't come from a JOIN expr.
		 */
		foreach(qual, (List *) f->quals)
			distribute_qual_to_rels(root, (Node *) lfirst(qual),
									true, false, false, result);
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;
		Relids		leftids,
					rightids;
		bool		isouterjoin;
		List	   *qual;

		/*
		 * Order of operations here is subtle and critical.  First we
		 * recurse to handle sub-JOINs.  Their join quals will be placed
		 * without regard for whether this level is an outer join, which
		 * is correct. Then, if we are an outer join, we mark baserels
		 * contained within the nullable side(s) with our own rel set;
		 * this will restrict placement of subsequent quals using those
		 * rels, including our own quals and quals above us in the join
		 * tree. Finally we place our own join quals.
		 */
		leftids = distribute_quals_to_rels(root, j->larg);
		rightids = distribute_quals_to_rels(root, j->rarg);

		result = bms_union(leftids, rightids);

		isouterjoin = false;
		switch (j->jointype)
		{
			case JOIN_INNER:
				/* Inner join adds no restrictions for quals */
				break;
			case JOIN_LEFT:
				mark_baserels_for_outer_join(root, rightids, result);
				isouterjoin = true;
				break;
			case JOIN_FULL:
				mark_baserels_for_outer_join(root, result, result);
				isouterjoin = true;
				break;
			case JOIN_RIGHT:
				mark_baserels_for_outer_join(root, leftids, result);
				isouterjoin = true;
				break;
			case JOIN_UNION:

				/*
				 * This is where we fail if upper levels of planner
				 * haven't rewritten UNION JOIN as an Append ...
				 */
				elog(ERROR, "UNION JOIN is not implemented yet");
				break;
			default:
				elog(ERROR,
					 "distribute_quals_to_rels: unsupported join type %d",
					 (int) j->jointype);
				break;
		}

		foreach(qual, (List *) j->quals)
			distribute_qual_to_rels(root, (Node *) lfirst(qual),
									false, isouterjoin, false, result);
	}
	else
		elog(ERROR, "distribute_quals_to_rels: unexpected node type %d",
			 nodeTag(jtnode));
	return result;
}

/*
 * mark_baserels_for_outer_join
 *	  Mark all base rels listed in 'rels' as having the given outerjoinset.
 */
static void
mark_baserels_for_outer_join(Query *root, Relids rels, Relids outerrels)
{
	Relids		tmprelids;
	int			relno;

	tmprelids = bms_copy(rels);
	while ((relno = bms_first_member(tmprelids)) >= 0)
	{
		RelOptInfo *rel = find_base_rel(root, relno);

		/*
		 * Since we do this bottom-up, any outer-rels previously marked
		 * should be within the new outer join set.
		 */
		Assert(bms_is_subset(rel->outerjoinset, outerrels));

		/*
		 * Presently the executor cannot support FOR UPDATE marking of
		 * rels appearing on the nullable side of an outer join. (It's
		 * somewhat unclear what that would mean, anyway: what should we
		 * mark when a result row is generated from no element of the
		 * nullable relation?)	So, complain if target rel is FOR UPDATE.
		 * It's sufficient to make this check once per rel, so do it only
		 * if rel wasn't already known nullable.
		 */
		if (rel->outerjoinset == NULL)
		{
			if (intMember(relno, root->rowMarks))
				elog(ERROR, "SELECT FOR UPDATE cannot be applied to the nullable side of an OUTER JOIN");
		}

		rel->outerjoinset = outerrels;
	}
	bms_free(tmprelids);
}

/*
 * distribute_qual_to_rels
 *	  Add clause information to either the 'RestrictInfo' or 'JoinInfo' field
 *	  (depending on whether the clause is a join) of each base relation
 *	  mentioned in the clause.	A RestrictInfo node is created and added to
 *	  the appropriate list for each rel.  Also, if the clause uses a
 *	  mergejoinable operator and is not an outer-join qual, enter the left-
 *	  and right-side expressions into the query's lists of equijoined vars.
 *
 * 'clause': the qual clause to be distributed
 * 'ispusheddown': if TRUE, force the clause to be marked 'ispusheddown'
 *		(this indicates the clause came from a FromExpr, not a JoinExpr)
 * 'isouterjoin': TRUE if the qual came from an OUTER JOIN's ON-clause
 * 'isdeduced': TRUE if the qual came from implied-equality deduction
 * 'qualscope': set of baserels the qual's syntactic scope covers
 *
 * 'qualscope' identifies what level of JOIN the qual came from.  For a top
 * level qual (WHERE qual), qualscope lists all baserel ids and in addition
 * 'ispusheddown' will be TRUE.
 */
static void
distribute_qual_to_rels(Query *root, Node *clause,
						bool ispusheddown,
						bool isouterjoin,
						bool isdeduced,
						Relids qualscope)
{
	RestrictInfo *restrictinfo = makeNode(RestrictInfo);
	RelOptInfo *rel;
	Relids		relids;
	List	   *vars;
	bool		can_be_equijoin;

	restrictinfo->clause = (Expr *) clause;
	restrictinfo->subclauseindices = NIL;
	restrictinfo->eval_cost.startup = -1; /* not computed until needed */
	restrictinfo->this_selec = -1;		/* not computed until needed */
	restrictinfo->left_relids = NULL; /* set below, if join clause */
	restrictinfo->right_relids = NULL;
	restrictinfo->mergejoinoperator = InvalidOid;
	restrictinfo->left_sortop = InvalidOid;
	restrictinfo->right_sortop = InvalidOid;
	restrictinfo->left_pathkey = NIL;	/* not computable yet */
	restrictinfo->right_pathkey = NIL;
	restrictinfo->left_mergescansel = -1;		/* not computed until
												 * needed */
	restrictinfo->right_mergescansel = -1;
	restrictinfo->hashjoinoperator = InvalidOid;
	restrictinfo->left_bucketsize = -1; /* not computed until needed */
	restrictinfo->right_bucketsize = -1;

	/*
	 * Retrieve all relids and vars contained within the clause.
	 */
	clause_get_relids_vars(clause, &relids, &vars);

	/*
	 * Cross-check: clause should contain no relids not within its scope.
	 * Otherwise the parser messed up.
	 */
	if (!bms_is_subset(relids, qualscope))
		elog(ERROR, "JOIN qualification may not refer to other relations");

	/*
	 * If the clause is variable-free, we force it to be evaluated at its
	 * original syntactic level.  Note that this should not happen for
	 * top-level clauses, because query_planner() special-cases them.  But
	 * it will happen for variable-free JOIN/ON clauses.  We don't have to
	 * be real smart about such a case, we just have to be correct.
	 */
	if (bms_is_empty(relids))
		relids = qualscope;

	/*
	 * For an outer-join qual, pretend that the clause references all rels
	 * appearing within its syntactic scope, even if it really doesn't.
	 * This ensures that the clause will be evaluated exactly at the level
	 * of joining corresponding to the outer join.
	 *
	 * For a non-outer-join qual, we can evaluate the qual as soon as (1) we
	 * have all the rels it mentions, and (2) we are at or above any outer
	 * joins that can null any of these rels and are below the syntactic
	 * location of the given qual.	To enforce the latter, scan the base
	 * rels listed in relids, and merge their outer-join sets into the
	 * clause's own reference list.  At the time we are called, the
	 * outerjoinset of each baserel will show exactly those outer
	 * joins that are below the qual in the join tree.
	 *
	 * If the qual came from implied-equality deduction, we can evaluate the
	 * qual at its natural semantic level.
	 *
	 */
	if (isdeduced)
	{
		Assert(bms_equal(relids, qualscope));
		can_be_equijoin = true;
	}
	else if (isouterjoin)
	{
		relids = qualscope;
		can_be_equijoin = false;
	}
	else
	{
		/* copy to ensure we don't change caller's qualscope set */
		Relids		newrelids = bms_copy(relids);
		Relids		tmprelids;
		int			relno;

		can_be_equijoin = true;
		tmprelids = bms_copy(relids);
		while ((relno = bms_first_member(tmprelids)) >= 0)
		{
			RelOptInfo *rel = find_base_rel(root, relno);

			if (!bms_is_subset(rel->outerjoinset, relids))
			{
				newrelids = bms_add_members(newrelids, rel->outerjoinset);

				/*
				 * Because application of the qual will be delayed by
				 * outer join, we mustn't assume its vars are equal
				 * everywhere.
				 */
				can_be_equijoin = false;
			}
		}
		bms_free(tmprelids);
		relids = newrelids;
		/* Should still be a subset of current scope ... */
		Assert(bms_is_subset(relids, qualscope));
	}

	/*
	 * Mark the qual as "pushed down" if it can be applied at a level
	 * below its original syntactic level.	This allows us to distinguish
	 * original JOIN/ON quals from higher-level quals pushed down to the
	 * same joinrel. A qual originating from WHERE is always considered
	 * "pushed down".
	 */
	restrictinfo->ispusheddown = ispusheddown || !bms_equal(relids,
															qualscope);

	switch (bms_membership(relids))
	{
		case BMS_SINGLETON:
			/*
			 * There is only one relation participating in 'clause', so
			 * 'clause' is a restriction clause for that relation.
			 */
			rel = find_base_rel(root, bms_singleton_member(relids));

			/*
			 * Check for a "mergejoinable" clause even though it's not a join
			 * clause.	This is so that we can recognize that "a.x = a.y"
			 * makes x and y eligible to be considered equal, even when they
			 * belong to the same rel.	Without this, we would not recognize
			 * that "a.x = a.y AND a.x = b.z AND a.y = c.q" allows us to
			 * consider z and q equal after their rels are joined.
			 */
			if (can_be_equijoin)
				check_mergejoinable(restrictinfo);

			/*
			 * If the clause was deduced from implied equality, check to see
			 * whether it is redundant with restriction clauses we already
			 * have for this rel.  Note we cannot apply this check to
			 * user-written clauses, since we haven't found the canonical
			 * pathkey sets yet while processing user clauses.	(NB: no
			 * comparable check is done in the join-clause case; redundancy
			 * will be detected when the join clause is moved into a join
			 * rel's restriction list.)
			 */
			if (!isdeduced ||
				!qual_is_redundant(root, restrictinfo, rel->baserestrictinfo))
			{
				/* Add clause to rel's restriction list */
				rel->baserestrictinfo = lappend(rel->baserestrictinfo,
												restrictinfo);
			}
			break;
		case BMS_MULTIPLE:
			/*
			 * 'clause' is a join clause, since there is more than one rel in
			 * the relid set.	Set additional RestrictInfo fields for
			 * joining.  First, does it look like a normal join clause, i.e.,
			 * a binary operator relating expressions that come from distinct
			 * relations?  If so we might be able to use it in a join
			 * algorithm.
			 */
			if (is_opclause(clause) && length(((OpExpr *) clause)->args) == 2)
			{
				Relids		left_relids;
				Relids		right_relids;

				left_relids = pull_varnos(get_leftop((Expr *) clause));
				right_relids = pull_varnos(get_rightop((Expr *) clause));
				if (!bms_is_empty(left_relids) &&
					!bms_is_empty(right_relids) &&
					!bms_overlap(left_relids, right_relids))
				{
					restrictinfo->left_relids = left_relids;
					restrictinfo->right_relids = right_relids;
				}
			}

			/*
			 * Now check for hash or mergejoinable operators.
			 *
			 * We don't bother setting the hashjoin info if we're not going
			 * to need it.	We do want to know about mergejoinable ops in all
			 * cases, however, because we use mergejoinable ops for other
			 * purposes such as detecting redundant clauses.
			 */
			check_mergejoinable(restrictinfo);
			if (enable_hashjoin)
				check_hashjoinable(restrictinfo);

			/*
			 * Add clause to the join lists of all the relevant relations.
			 */
			add_join_clause_to_rels(root, restrictinfo, relids);

			/*
			 * Add vars used in the join clause to targetlists of their
			 * relations, so that they will be emitted by the plan nodes that
			 * scan those relations (else they won't be available at the join
			 * node!).
			 */
			add_vars_to_targetlist(root, vars);
			break;
		default:
			/*
			 * 'clause' references no rels, and therefore we have no place to
			 * attach it.  Shouldn't get here if callers are working properly.
			 */
			elog(ERROR, "distribute_qual_to_rels: can't cope with variable-free clause");
			break;
	}

	/*
	 * If the clause has a mergejoinable operator, and is not an
	 * outer-join qualification nor bubbled up due to an outer join, then
	 * the two sides represent equivalent PathKeyItems for path keys: any
	 * path that is sorted by one side will also be sorted by the other
	 * (as soon as the two rels are joined, that is).  Record the key
	 * equivalence for future use.	(We can skip this for a deduced
	 * clause, since the keys are already known equivalent in that case.)
	 */
	if (can_be_equijoin && restrictinfo->mergejoinoperator != InvalidOid &&
		!isdeduced)
		add_equijoined_keys(root, restrictinfo);
}

/*
 * process_implied_equality
 *	  Check to see whether we already have a restrictinfo item that says
 *	  item1 = item2, and create one if not; or if delete_it is true,
 *	  remove any such restrictinfo item.
 *
 * This processing is a consequence of transitivity of mergejoin equality:
 * if we have mergejoinable clauses A = B and B = C, we can deduce A = C
 * (where = is an appropriate mergejoinable operator).  See path/pathkeys.c
 * for more details.
 */
void
process_implied_equality(Query *root,
						 Node *item1, Node *item2,
						 Oid sortop1, Oid sortop2,
						 Relids item1_relids, Relids item2_relids,
						 bool delete_it)
{
	Relids		relids;
	BMS_Membership membership;
	RelOptInfo *rel1;
	List	   *restrictlist;
	List	   *itm;
	Oid			ltype,
				rtype;
	Operator	eq_operator;
	Form_pg_operator pgopform;
	Expr	   *clause;

	/* Get set of relids referenced in the two expressions */
	relids = bms_union(item1_relids, item2_relids);
	membership = bms_membership(relids);

	/*
	 * generate_implied_equalities() shouldn't call me on two constants.
	 */
	Assert(membership != BMS_EMPTY_SET);

	/*
	 * If the exprs involve a single rel, we need to look at that rel's
	 * baserestrictinfo list.  If multiple rels, any one will have a
	 * joininfo node for the rest, and we can scan any of 'em.
	 */
	if (membership == BMS_SINGLETON)
	{
		rel1 = find_base_rel(root, bms_singleton_member(relids));
		restrictlist = rel1->baserestrictinfo;
	}
	else
	{
		Relids		other_rels;
		int			first_rel;
		JoinInfo   *joininfo;

		/* Copy relids, find and remove one member */
		other_rels = bms_copy(relids);
		first_rel = bms_first_member(other_rels);

		rel1 = find_base_rel(root, first_rel);

		/* use remaining members to find join node */
		joininfo = find_joininfo_node(rel1, other_rels);

		restrictlist = joininfo ? joininfo->jinfo_restrictinfo : NIL;

		bms_free(other_rels);
	}

	/*
	 * Scan to see if equality is already known.  If so, we're done in
	 * the add case, and done after removing it in the delete case.
	 */
	foreach(itm, restrictlist)
	{
		RestrictInfo *restrictinfo = (RestrictInfo *) lfirst(itm);
		Node	   *left,
				   *right;

		if (restrictinfo->mergejoinoperator == InvalidOid)
			continue;			/* ignore non-mergejoinable clauses */
		/* We now know the restrictinfo clause is a binary opclause */
		left = get_leftop(restrictinfo->clause);
		right = get_rightop(restrictinfo->clause);
		if ((equal(item1, left) && equal(item2, right)) ||
			(equal(item2, left) && equal(item1, right)))
		{
			/* found a matching clause */
			if (delete_it)
			{
				if (membership == BMS_SINGLETON)
				{
					/* delete it from local restrictinfo list */
					rel1->baserestrictinfo = lremove(restrictinfo,
													 rel1->baserestrictinfo);
				}
				else
				{
					/* let joininfo.c do it */
					remove_join_clause_from_rels(root, restrictinfo, relids);
				}
			}
			return;				/* done */
		}
	}

	/* Didn't find it.  Done if deletion requested */
	if (delete_it)
		return;

	/*
	 * This equality is new information, so construct a clause
	 * representing it to add to the query data structures.
	 */
	ltype = exprType(item1);
	rtype = exprType(item2);
	eq_operator = compatible_oper(makeList1(makeString("=")),
								  ltype, rtype, true);
	if (!HeapTupleIsValid(eq_operator))
	{
		/*
		 * Would it be safe to just not add the equality to the query if
		 * we have no suitable equality operator for the combination of
		 * datatypes?  NO, because sortkey selection may screw up anyway.
		 */
		elog(ERROR, "Unable to identify an equality operator for types '%s' and '%s'",
			 format_type_be(ltype), format_type_be(rtype));
	}
	pgopform = (Form_pg_operator) GETSTRUCT(eq_operator);

	/*
	 * Let's just make sure this appears to be a compatible operator.
	 */
	if (pgopform->oprlsortop != sortop1 ||
		pgopform->oprrsortop != sortop2 ||
		pgopform->oprresult != BOOLOID)
		elog(ERROR, "Equality operator for types '%s' and '%s' should be mergejoinable, but isn't",
			 format_type_be(ltype), format_type_be(rtype));

	clause = make_opclause(oprid(eq_operator), /* opno */
						   BOOLOID,	/* opresulttype */
						   false, /* opretset */
						   (Expr *) item1,
						   (Expr *) item2);

	ReleaseSysCache(eq_operator);

	/*
	 * Push the new clause into all the appropriate restrictinfo lists.
	 *
	 * Note: we mark the qual "pushed down" to ensure that it can never be
	 * taken for an original JOIN/ON clause.
	 */
	distribute_qual_to_rels(root, (Node *) clause,
							true, false, true,
							relids);
}

/*
 * qual_is_redundant
 *	  Detect whether an implied-equality qual that turns out to be a
 *	  restriction clause for a single base relation is redundant with
 *	  already-known restriction clauses for that rel.  This occurs with,
 *	  for example,
 *				SELECT * FROM tab WHERE f1 = f2 AND f2 = f3;
 *	  We need to suppress the redundant condition to avoid computing
 *	  too-small selectivity, not to mention wasting time at execution.
 *
 * Note: quals of the form "var = const" are never considered redundant,
 * only those of the form "var = var".  This is needed because when we
 * have constants in an implied-equality set, we use a different strategy
 * that suppresses all "var = var" deductions.  We must therefore keep
 * all the "var = const" quals.
 */
static bool
qual_is_redundant(Query *root,
				  RestrictInfo *restrictinfo,
				  List *restrictlist)
{
	Node	   *newleft;
	Node	   *newright;
	List	   *oldquals;
	List	   *olditem;
	List	   *equalexprs;
	bool		someadded;

	newleft = get_leftop(restrictinfo->clause);
	newright = get_rightop(restrictinfo->clause);

	/* Never redundant unless vars appear on both sides */
	if (!contain_var_clause(newleft) || !contain_var_clause(newright))
		return false;

	/*
	 * Set cached pathkeys.  NB: it is okay to do this now because this
	 * routine is only invoked while we are generating implied equalities.
	 * Therefore, the equi_key_list is already complete and so we can
	 * correctly determine canonical pathkeys.
	 */
	cache_mergeclause_pathkeys(root, restrictinfo);
	/* If different, say "not redundant" (should never happen) */
	if (restrictinfo->left_pathkey != restrictinfo->right_pathkey)
		return false;

	/*
	 * Scan existing quals to find those referencing same pathkeys.
	 * Usually there will be few, if any, so build a list of just the
	 * interesting ones.
	 */
	oldquals = NIL;
	foreach(olditem, restrictlist)
	{
		RestrictInfo *oldrinfo = (RestrictInfo *) lfirst(olditem);

		if (oldrinfo->mergejoinoperator != InvalidOid)
		{
			cache_mergeclause_pathkeys(root, oldrinfo);
			if (restrictinfo->left_pathkey == oldrinfo->left_pathkey &&
				restrictinfo->right_pathkey == oldrinfo->right_pathkey)
				oldquals = lcons(oldrinfo, oldquals);
		}
	}
	if (oldquals == NIL)
		return false;

	/*
	 * Now, we want to develop a list of exprs that are known equal to the
	 * left side of the new qual.  We traverse the old-quals list
	 * repeatedly to transitively expand the exprs list.  If at any point
	 * we find we can reach the right-side expr of the new qual, we are
	 * done.  We give up when we can't expand the equalexprs list any more.
	 */
	equalexprs = makeList1(newleft);
	do
	{
		someadded = false;
		/* cannot use foreach here because of possible lremove */
		olditem = oldquals;
		while (olditem)
		{
			RestrictInfo *oldrinfo = (RestrictInfo *) lfirst(olditem);
			Node	   *oldleft = get_leftop(oldrinfo->clause);
			Node	   *oldright = get_rightop(oldrinfo->clause);
			Node	   *newguy = NULL;

			/* must advance olditem before lremove possibly pfree's it */
			olditem = lnext(olditem);

			if (member(oldleft, equalexprs))
				newguy = oldright;
			else if (member(oldright, equalexprs))
				newguy = oldleft;
			else
				continue;
			if (equal(newguy, newright))
				return true;	/* we proved new clause is redundant */
			equalexprs = lcons(newguy, equalexprs);
			someadded = true;

			/*
			 * Remove this qual from list, since we don't need it anymore.
			 */
			oldquals = lremove(oldrinfo, oldquals);
		}
	} while (someadded);

	return false;				/* it's not redundant */
}


/*****************************************************************************
 *
 *	 CHECKS FOR MERGEJOINABLE AND HASHJOINABLE CLAUSES
 *
 *****************************************************************************/

/*
 * check_mergejoinable
 *	  If the restrictinfo's clause is mergejoinable, set the mergejoin
 *	  info fields in the restrictinfo.
 *
 *	  Currently, we support mergejoin for binary opclauses where
 *	  the operator is a mergejoinable operator.  The arguments can be
 *	  anything --- as long as there are no volatile functions in them.
 */
static void
check_mergejoinable(RestrictInfo *restrictinfo)
{
	Expr	   *clause = restrictinfo->clause;
	Oid			opno,
				leftOp,
				rightOp;

	if (!is_opclause(clause))
		return;
	if (length(((OpExpr *) clause)->args) != 2)
		return;

	opno = ((OpExpr *) clause)->opno;

	if (op_mergejoinable(opno,
						 &leftOp,
						 &rightOp) &&
		!contain_volatile_functions((Node *) clause))
	{
		restrictinfo->mergejoinoperator = opno;
		restrictinfo->left_sortop = leftOp;
		restrictinfo->right_sortop = rightOp;
	}
}

/*
 * check_hashjoinable
 *	  If the restrictinfo's clause is hashjoinable, set the hashjoin
 *	  info fields in the restrictinfo.
 *
 *	  Currently, we support hashjoin for binary opclauses where
 *	  the operator is a hashjoinable operator.  The arguments can be
 *	  anything --- as long as there are no volatile functions in them.
 */
static void
check_hashjoinable(RestrictInfo *restrictinfo)
{
	Expr	   *clause = restrictinfo->clause;
	Oid			opno;

	if (!is_opclause(clause))
		return;
	if (length(((OpExpr *) clause)->args) != 2)
		return;

	opno = ((OpExpr *) clause)->opno;

	if (op_hashjoinable(opno) &&
		!contain_volatile_functions((Node *) clause))
		restrictinfo->hashjoinoperator = opno;
}
