/*-------------------------------------------------------------------------
 *
 * createplan.c
 *	  Routines to create the desired plan for processing a query
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/plan/createplan.c,v 1.70 1999-08-12 04:32:53 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>

#include "postgres.h"

#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/internal.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "optimizer/tlist.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


#define NONAME_SORT		1
#define NONAME_MATERIAL 2

static List *switch_outer(List *clauses);
static Oid *generate_merge_input_sortorder(List *pathkeys,
							   MergeOrder *mergeorder);
static Scan *create_scan_node(Path *best_path, List *tlist);
static Join *create_join_node(JoinPath *best_path, List *tlist);
static SeqScan *create_seqscan_node(Path *best_path, List *tlist,
					List *scan_clauses);
static IndexScan *create_indexscan_node(IndexPath *best_path, List *tlist,
					  List *scan_clauses);
static NestLoop *create_nestloop_node(NestPath *best_path, List *tlist,
					 List *clauses, Plan *outer_node, List *outer_tlist,
					 Plan *inner_node, List *inner_tlist);
static MergeJoin *create_mergejoin_node(MergePath *best_path, List *tlist,
					  List *clauses, Plan *outer_node, List *outer_tlist,
					  Plan *inner_node, List *inner_tlist);
static HashJoin *create_hashjoin_node(HashPath *best_path, List *tlist,
					 List *clauses, Plan *outer_node, List *outer_tlist,
					 Plan *inner_node, List *inner_tlist);
static List *fix_indxqual_references(List *indexquals, IndexPath *index_path);
static List *fix_indxqual_sublist(List *indexqual, IndexPath *index_path,
								  Form_pg_index index);
static Node *fix_indxqual_operand(Node *node, IndexPath *index_path,
								  Form_pg_index index);
static Noname *make_noname(List *tlist, List *pathkeys, Oid *operators,
			Plan *plan_node, int nonametype);
static IndexScan *make_indexscan(List *qptlist, List *qpqual, Index scanrelid,
			   List *indxid, List *indxqual, List *indxqualorig);
static NestLoop *make_nestloop(List *qptlist, List *qpqual, Plan *lefttree,
			  Plan *righttree);
static HashJoin *make_hashjoin(List *tlist, List *qpqual,
			  List *hashclauses, Plan *lefttree, Plan *righttree);
static Hash *make_hash(List *tlist, Var *hashkey, Plan *lefttree);
static MergeJoin *make_mergejoin(List *tlist, List *qpqual,
			   List *mergeclauses, Plan *righttree, Plan *lefttree);
static Material *make_material(List *tlist, Oid nonameid, Plan *lefttree,
			  int keycount);
static void copy_costsize(Plan *dest, Plan *src);

/*
 * create_plan
 *	  Creates the access plan for a query by tracing backwards through the
 *	  desired chain of pathnodes, starting at the node 'best_path'.  For
 *	  every pathnode found:
 *	  (1) Create a corresponding plan node containing appropriate id,
 *		  target list, and qualification information.
 *	  (2) Modify ALL clauses so that attributes are referenced using
 *		  relative values.
 *	  (3) Target lists are not modified, but will be in another routine.
 *
 *	  best_path is the best access path
 *
 *	  Returns the optimal(?) access plan.
 */
Plan *
create_plan(Path *best_path)
{
	List	   *tlist;
	Plan	   *plan_node = (Plan *) NULL;
	RelOptInfo *parent_rel;
	int			size;
	int			width;
	int			pages;
	int			tuples;

	parent_rel = best_path->parent;
	tlist = get_actual_tlist(parent_rel->targetlist);
	size = parent_rel->size;
	width = parent_rel->width;
	pages = parent_rel->pages;
	tuples = parent_rel->tuples;

	switch (best_path->pathtype)
	{
		case T_IndexScan:
		case T_SeqScan:
			plan_node = (Plan *) create_scan_node(best_path, tlist);
			break;
		case T_HashJoin:
		case T_MergeJoin:
		case T_NestLoop:
			plan_node = (Plan *) create_join_node((JoinPath *) best_path, tlist);
			break;
		default:
			/* do nothing */
			break;
	}

	plan_node->plan_size = size;
	plan_node->plan_width = width;
	if (pages == 0)
		pages = 1;
	plan_node->plan_tupperpage = tuples / pages;

#ifdef NOT_USED					/* fix xfunc */
	/* sort clauses by cost/(1-selectivity) -- JMH 2/26/92 */
	if (XfuncMode != XFUNC_OFF)
	{
		set_qpqual((Plan) plan_node,
				   lisp_qsort(get_qpqual((Plan) plan_node),
							  xfunc_clause_compare));
		if (XfuncMode != XFUNC_NOR)
			/* sort the disjuncts within each clause by cost -- JMH 3/4/92 */
			xfunc_disjunct_sort(plan_node->qpqual);
	}
#endif

	return plan_node;
}

/*
 * create_scan_node
 *	 Create a scan path for the parent relation of 'best_path'.
 *
 *	 tlist is the targetlist for the base relation scanned by 'best_path'
 *
 *	 Returns the scan node.
 */
static Scan *
create_scan_node(Path *best_path, List *tlist)
{

	Scan	   *node = NULL;
	List	   *scan_clauses;

	/*
	 * Extract the relevant restriction clauses from the parent relation;
	 * the executor must apply all these restrictions during the scan.
	 * Fix regproc ids in the restriction clauses.
	 */
	scan_clauses = fix_opids(get_actual_clauses(best_path->parent->restrictinfo));

	switch (best_path->pathtype)
	{
		case T_SeqScan:
			node = (Scan *) create_seqscan_node(best_path, tlist, scan_clauses);
			break;

		case T_IndexScan:
			node = (Scan *) create_indexscan_node((IndexPath *) best_path,
												  tlist,
												  scan_clauses);
			break;

		default:
			elog(ERROR, "create_scan_node: unknown node type",
				 best_path->pathtype);
			break;
	}

	return node;
}

/*
 * create_join_node
 *	  Create a join path for 'best_path' and(recursively) paths for its
 *	  inner and outer paths.
 *
 *	  'tlist' is the targetlist for the join relation corresponding to
 *		'best_path'
 *
 *	  Returns the join node.
 */
static Join *
create_join_node(JoinPath *best_path, List *tlist)
{
	Plan	   *outer_node;
	List	   *outer_tlist;
	Plan	   *inner_node;
	List	   *inner_tlist;
	List	   *clauses;
	Join	   *retval = NULL;

	outer_node = create_plan((Path *) best_path->outerjoinpath);
	outer_tlist = outer_node->targetlist;

	inner_node = create_plan((Path *) best_path->innerjoinpath);
	inner_tlist = inner_node->targetlist;

	clauses = get_actual_clauses(best_path->pathinfo);

	switch (best_path->path.pathtype)
	{
		case T_MergeJoin:
			retval = (Join *) create_mergejoin_node((MergePath *) best_path,
													tlist,
													clauses,
													outer_node,
													outer_tlist,
													inner_node,
													inner_tlist);
			break;
		case T_HashJoin:
			retval = (Join *) create_hashjoin_node((HashPath *) best_path,
												   tlist,
												   clauses,
												   outer_node,
												   outer_tlist,
												   inner_node,
												   inner_tlist);
			break;
		case T_NestLoop:
			retval = (Join *) create_nestloop_node((NestPath *) best_path,
												   tlist,
												   clauses,
												   outer_node,
												   outer_tlist,
												   inner_node,
												   inner_tlist);
			break;
		default:
			/* do nothing */
			elog(ERROR, "create_join_node: unknown node type",
				 best_path->path.pathtype);
	}

#ifdef NOT_USED
	/*
	 * * Expensive function pullups may have pulled local predicates *
	 * into this path node.  Put them in the qpqual of the plan node. *
	 * JMH, 6/15/92
	 */
	if (get_loc_restrictinfo(best_path) != NIL)
		set_qpqual((Plan) retval,
				   nconc(get_qpqual((Plan) retval),
						 fix_opids(get_actual_clauses
								   (get_loc_restrictinfo(best_path)))));
#endif

	return retval;
}

/*****************************************************************************
 *
 *	BASE-RELATION SCAN METHODS
 *
 *****************************************************************************/


/*
 * create_seqscan_node
 *	 Returns a seqscan node for the base relation scanned by 'best_path'
 *	 with restriction clauses 'scan_clauses' and targetlist 'tlist'.
 */
static SeqScan *
create_seqscan_node(Path *best_path, List *tlist, List *scan_clauses)
{
	SeqScan    *scan_node = (SeqScan *) NULL;
	Index		scan_relid = -1;
	List	   *temp;

	temp = best_path->parent->relids;
	/* there should be exactly one base rel involved... */
	Assert(length(temp) == 1);
	scan_relid = (Index) lfirsti(temp);

	scan_node = make_seqscan(tlist,
							 scan_clauses,
							 scan_relid,
							 (Plan *) NULL);

	scan_node->plan.cost = best_path->path_cost;

	return scan_node;
}

/*
 * create_indexscan_node
 *	  Returns a indexscan node for the base relation scanned by 'best_path'
 *	  with restriction clauses 'scan_clauses' and targetlist 'tlist'.
 *
 * The indexqual of the path contains a sublist of implicitly-ANDed qual
 * conditions for each scan of the index(es); if there is more than one
 * scan then the retrieved tuple sets are ORed together.  The indexqual
 * and indexid lists must have the same length, ie, the number of scans
 * that will occur.  Note it is possible for a qual condition sublist
 * to be empty --- then no index restrictions will be applied during that
 * scan.
 */
static IndexScan *
create_indexscan_node(IndexPath *best_path,
					  List *tlist,
					  List *scan_clauses)
{
	List	   *indxqual = best_path->indexqual;
	List	   *qpqual;
	List	   *fixed_indxqual;
	List	   *ixid;
	IndexScan  *scan_node;
	bool		lossy = false;

	/* there should be exactly one base rel involved... */
	Assert(length(best_path->path.parent->relids) == 1);

	/* check and see if any of the indices are lossy */
	foreach(ixid, best_path->indexid)
	{
		HeapTuple	indexTuple;
		Form_pg_index index;

		indexTuple = SearchSysCacheTuple(INDEXRELID,
										 ObjectIdGetDatum(lfirsti(ixid)),
										 0, 0, 0);
		if (!HeapTupleIsValid(indexTuple))
			elog(ERROR, "create_plan: index %u not found", lfirsti(ixid));
		index = (Form_pg_index) GETSTRUCT(indexTuple);
		if (index->indislossy)
		{
			lossy = true;
			break;
		}
	}

	/*
	 * The qpqual list must contain all restrictions not automatically
	 * handled by the index.  Note that for non-lossy indices, the
	 * predicates in the indxqual are checked fully by the index, while
	 * for lossy indices the indxqual predicates need to be double-checked
	 * after the index fetches the best-guess tuples.
	 *
	 * Since the indexquals were generated from the restriction clauses
	 * given by scan_clauses, there will normally be some duplications
	 * between the lists.  Get rid of the duplicates, then add back if lossy.
	 */
	if (length(indxqual) > 1)
	{
		/*
		 * Build an expression representation of the indexqual, expanding
		 * the implicit OR and AND semantics of the first- and second-level
		 * lists.  XXX Is it really necessary to do a deep copy here?
		 */
		List	   *orclauses = NIL;
		List	   *orclause;
		Expr	   *indxqual_expr;

		foreach(orclause, indxqual)
		{
			orclauses = lappend(orclauses,
								make_ands_explicit((List *) copyObject(lfirst(orclause))));
		}
		indxqual_expr = make_orclause(orclauses);

		/* this set_difference is almost certainly a waste of time... */
		qpqual = set_difference(scan_clauses,
								lcons(indxqual_expr, NIL));

		if (lossy)
			qpqual = lappend(qpqual, indxqual_expr);
	}
	else if (indxqual != NIL)
	{
		/* Here, we can simply treat the first sublist as an independent
		 * set of qual expressions, since there is no top-level OR behavior.
		 */
		qpqual = set_difference(scan_clauses, lfirst(indxqual));
		if (lossy)
			qpqual = nconc(qpqual, (List *) copyObject(lfirst(indxqual)));
	}
	else
		qpqual = NIL;

	/* The executor needs a copy with the indexkey on the left of each clause
	 * and with index attrs substituted for table ones.
	 */
	fixed_indxqual = fix_indxqual_references(indxqual, best_path);

	/*
	 * Fix opids in the completed indxquals.
	 * XXX this ought to only happen at final exit from the planner...
	 */
	indxqual = fix_opids(indxqual);
	fixed_indxqual = fix_opids(fixed_indxqual);

	scan_node = make_indexscan(tlist,
							   qpqual,
							   lfirsti(best_path->path.parent->relids),
							   best_path->indexid,
							   fixed_indxqual,
							   indxqual);

	scan_node->scan.plan.cost = best_path->path.path_cost;

	return scan_node;
}

/*****************************************************************************
 *
 *	JOIN METHODS
 *
 *****************************************************************************/

static NestLoop *
create_nestloop_node(NestPath *best_path,
					 List *tlist,
					 List *clauses,
					 Plan *outer_node,
					 List *outer_tlist,
					 Plan *inner_node,
					 List *inner_tlist)
{
	NestLoop   *join_node;

	if (IsA(inner_node, IndexScan))
	{
		/*
		 * An index is being used to reduce the number of tuples scanned
		 * in the inner relation.  If there are join clauses being used
		 * with the index, we must update their outer-rel var nodes to
		 * refer to the outer relation.
		 *
		 * We can also remove those join clauses from the list of clauses
		 * that have to be checked as qpquals at the join node, but only
		 * if there's just one indexscan in the inner path (otherwise,
		 * several different sets of clauses are being ORed together).
		 *
		 * Note: if the index is lossy, the same clauses may also be getting
		 * checked as qpquals in the indexscan.  We can still remove them
		 * from the nestloop's qpquals, but we gotta update the outer-rel
		 * vars in the indexscan's qpquals too...
		 */
		IndexScan  *innerscan = (IndexScan *) inner_node;
		List	   *indxqualorig = innerscan->indxqualorig;

		/* No work needed if indxqual refers only to its own relation... */
		if (NumRelids((Node *) indxqualorig) > 1)
		{
			/* Remove redundant tests from my clauses, if possible.
			 * Note we must compare against indxqualorig not the "fixed"
			 * indxqual (which has index attnos instead of relation attnos,
			 * and may have been commuted as well).
			 */
			if (length(indxqualorig) == 1) /* single indexscan? */
				clauses = set_difference(clauses, lfirst(indxqualorig));

			/* only refs to outer vars get changed in the inner indexqual */
			innerscan->indxqualorig = join_references(indxqualorig,
													  outer_tlist,
													  NIL);
			innerscan->indxqual = join_references(innerscan->indxqual,
												  outer_tlist,
												  NIL);
			/* fix the inner qpqual too, if it has join clauses */
			if (NumRelids((Node *) inner_node->qual) > 1)
				inner_node->qual = join_references(inner_node->qual,
												   outer_tlist,
												   NIL);
		}
	}
	else if (IsA_Join(inner_node))
	{
		/* Materialize the inner join for speed reasons */
		inner_node = (Plan *) make_noname(inner_tlist,
										  NIL,
										  NULL,
										  inner_node,
										  NONAME_MATERIAL);
	}

	join_node = make_nestloop(tlist,
							  join_references(clauses,
											  outer_tlist,
											  inner_tlist),
							  outer_node,
							  inner_node);

	join_node->join.cost = best_path->path.path_cost;

	return join_node;
}

static MergeJoin *
create_mergejoin_node(MergePath *best_path,
					  List *tlist,
					  List *clauses,
					  Plan *outer_node,
					  List *outer_tlist,
					  Plan *inner_node,
					  List *inner_tlist)
{
	List	   *qpqual,
			   *mergeclauses;
	MergeJoin  *join_node;

	/*
	 * Remove the mergeclauses from the list of join qual clauses,
	 * leaving the list of quals that must be checked as qpquals.
	 * Set those clauses to contain INNER/OUTER var references.
	 */
	qpqual = join_references(set_difference(clauses,
											best_path->path_mergeclauses),
							 outer_tlist,
							 inner_tlist);

	/*
	 * Now set the references in the mergeclauses and rearrange them so
	 * that the outer variable is always on the left.
	 */
	mergeclauses = switch_outer(join_references(best_path->path_mergeclauses,
												outer_tlist,
												inner_tlist));

	/*
	 * Create explicit sort paths for the outer and inner join paths if
	 * necessary.  The sort cost was already accounted for in the path.
	 */
	if (best_path->outersortkeys)
	{
		Oid		   *outer_order = generate_merge_input_sortorder(
												best_path->outersortkeys,
							 best_path->jpath.path.pathorder->ord.merge);

		outer_node = (Plan *) make_noname(outer_tlist,
										  best_path->outersortkeys,
										  outer_order,
										  outer_node,
										  NONAME_SORT);
	}

	if (best_path->innersortkeys)
	{
		Oid		   *inner_order = generate_merge_input_sortorder(
												best_path->innersortkeys,
							 best_path->jpath.path.pathorder->ord.merge);

		inner_node = (Plan *) make_noname(inner_tlist,
										  best_path->innersortkeys,
										  inner_order,
										  inner_node,
										  NONAME_SORT);
	}

	join_node = make_mergejoin(tlist,
							   qpqual,
							   mergeclauses,
							   inner_node,
							   outer_node);

	join_node->join.cost = best_path->jpath.path.path_cost;

	return join_node;
}

static HashJoin *
create_hashjoin_node(HashPath *best_path,
					 List *tlist,
					 List *clauses,
					 Plan *outer_node,
					 List *outer_tlist,
					 Plan *inner_node,
					 List *inner_tlist)
{
	List	   *qpqual;
	List	   *hashclauses;
	HashJoin   *join_node;
	Hash	   *hash_node;
	Var		   *innerhashkey;

	/*
	 * NOTE: there will always be exactly one hashclause in the list
	 * best_path->path_hashclauses (cf. hash_inner_and_outer()).
	 * We represent it as a list anyway for convenience with routines
	 * that want to work on lists of clauses.
	 */

	/*
	 * Remove the hashclauses from the list of join qual clauses,
	 * leaving the list of quals that must be checked as qpquals.
	 * Set those clauses to contain INNER/OUTER var references.
	 */
	qpqual = join_references(set_difference(clauses,
											best_path->path_hashclauses),
							 outer_tlist,
							 inner_tlist);

	/*
	 * Now set the references in the hashclauses and rearrange them so
	 * that the outer variable is always on the left.
	 */
	hashclauses = switch_outer(join_references(best_path->path_hashclauses,
											   outer_tlist,
											   inner_tlist));

	/* Now the righthand op of the sole hashclause is the inner hash key. */
	innerhashkey = get_rightop(lfirst(hashclauses));

	/*
	 * Build the hash node and hash join node.
	 */
	hash_node = make_hash(inner_tlist, innerhashkey, inner_node);
	join_node = make_hashjoin(tlist,
							  qpqual,
							  hashclauses,
							  outer_node,
							  (Plan *) hash_node);

	join_node->join.cost = best_path->jpath.path.path_cost;

	return join_node;
}


/*****************************************************************************
 *
 *	SUPPORTING ROUTINES
 *
 *****************************************************************************/

/*
 * fix_indxqual_references
 *	  Adjust indexqual clauses to refer to index attributes instead of the
 *	  attributes of the original relation.  Also, commute clauses if needed
 *	  to put the indexkey on the left.  (Someday the executor might not need
 *	  that, but for now it does.)
 *
 * This code used to be entirely bogus for multi-index scans.  Now it keeps
 * track of which index applies to each subgroup of index qual clauses...
 *
 * Returns a modified copy of the indexqual list --- the original is not
 * changed.
 */

static List *
fix_indxqual_references(List *indexquals, IndexPath *index_path)
{
	List	   *fixed_quals = NIL;
	List	   *indexids = index_path->indexid;
	List	   *i;

	foreach(i, indexquals)
	{
		List	   *indexqual = lfirst(i);
		Oid			indexid = lfirsti(indexids);
		HeapTuple	indexTuple;
		Form_pg_index index;

		indexTuple = SearchSysCacheTuple(INDEXRELID,
										 ObjectIdGetDatum(indexid),
										 0, 0, 0);
		if (!HeapTupleIsValid(indexTuple))
			elog(ERROR, "fix_indxqual_references: index %u not found",
				 indexid);
		index = (Form_pg_index) GETSTRUCT(indexTuple);

		fixed_quals = lappend(fixed_quals,
							  fix_indxqual_sublist(indexqual,
												   index_path,
												   index));

		indexids = lnext(indexids);
	}
	return fixed_quals;
}

/*
 * Fix the sublist of indexquals to be used in a particular scan.
 *
 * For each qual clause, commute if needed to put the indexkey operand on the
 * left, and then change its varno.  We do not need to change the other side
 * of the clause.
 */
static List *
fix_indxqual_sublist(List *indexqual, IndexPath *index_path,
					 Form_pg_index index)
{
	List	   *fixed_qual = NIL;
	List	   *i;

	foreach(i, indexqual)
	{
		Expr	   *clause = (Expr *) lfirst(i);
		int			relid;
		AttrNumber	attno;
		Datum		constval;
		int			flag;
		Expr	   *newclause;

		if (!is_opclause((Node *) clause) ||
			length(clause->args) != 2)
			elog(ERROR, "fix_indxqual_sublist: indexqual clause is not binary opclause");

		/* Which side is the indexkey on?
		 *
		 * get_relattval sets flag&SEL_RIGHT if the indexkey is on the LEFT.
		 */
		get_relattval((Node *) clause,
					  lfirsti(index_path->path.parent->relids),
					  &relid, &attno, &constval, &flag);

		/* Copy enough structure to allow commuting and replacing an operand
		 * without changing original clause.
		 */
		newclause = make_clause(clause->opType, clause->oper,
								listCopy(clause->args));

		/* If the indexkey is on the right, commute the clause. */
		if ((flag & SEL_RIGHT) == 0)
			CommuteClause(newclause);

		/* Now, change the indexkey operand as needed. */
		lfirst(newclause->args) = fix_indxqual_operand(lfirst(newclause->args),
													   index_path,
													   index);

		fixed_qual = lappend(fixed_qual, newclause);
	}
	return fixed_qual;
}

static Node *
fix_indxqual_operand(Node *node, IndexPath *index_path,
					 Form_pg_index index)
{
	if (IsA(node, Var))
	{
		if (((Var *) node)->varno == lfirsti(index_path->path.parent->relids))
		{
			int			varatt = ((Var *) node)->varattno;
			int			pos;

			for (pos = 0; pos < INDEX_MAX_KEYS; pos++)
			{
				if (index->indkey[pos] == varatt)
				{
					Node	   *newnode = copyObject(node);
					((Var *) newnode)->varattno = pos + 1;
					return newnode;
				}
			}
		}
		/*
		 * Oops, this Var isn't the indexkey!
		 */
		elog(ERROR, "fix_indxqual_operand: var is not index attribute");
	}

	/*
	 * Else, it must be a func expression representing a functional index.
	 *
	 * Currently, there is no need for us to do anything here for
	 * functional indexes.  If nodeIndexscan.c sees a func clause as the left
	 * or right-hand toplevel operand of an indexqual, it assumes that that is
	 * a reference to the functional index's value and makes the appropriate
	 * substitution.  (It would be cleaner to make the substitution here, I
	 * think --- suspect this issue if a join clause involving a function call
	 * misbehaves...)
	 */

	/* return the unmodified node */
	return node;
}

/*
 * switch_outer
 *	  Given a list of merge clauses, rearranges the elements within the
 *	  clauses so the outer join variable is on the left and the inner is on
 *	  the right.  The original list is not touched; a modified list
 *	  is returned.
 */
static List *
switch_outer(List *clauses)
{
	List	   *t_list = NIL;
	List	   *i;

	foreach(i, clauses)
	{
		Expr	   *clause = (Expr *) lfirst(i);
		Node	   *op;

		Assert(is_opclause((Node *) clause));
		op = (Node *) get_rightop(clause);
		Assert(op != (Node *) NULL);
		if (IsA(op, ArrayRef))	/* I think this test is dead code ... tgl */
			op = ((ArrayRef *) op)->refexpr;
		Assert(IsA(op, Var));
		if (var_is_outer((Var *) op))
		{
			/*
			 * Duplicate just enough of the structure to allow commuting
			 * the clause without changing the original list.  Could use
			 * copyObject, but a complete deep copy is overkill.
			 */
			Expr	   *temp;

			temp = make_clause(clause->opType, clause->oper,
							   listCopy(clause->args));
			/* Commute it --- note this modifies the temp node in-place. */
			CommuteClause(temp);
			t_list = lappend(t_list, temp);
		}
		else
			t_list = lappend(t_list, clause);
	}
	return t_list;
}

/*
 * generate_merge_input_sortorder
 *
 * Generate the list of sort ops needed to sort one of the input paths for
 * a merge.  We may have to use either left or right sortop for each item,
 * since the original mergejoin clause may or may not have been commuted
 * (compare switch_outer above).
 *
 * XXX This is largely a crock.  It works only because group_clauses_by_order
 * only groups together mergejoin clauses that have identical MergeOrder info,
 * which means we can safely use a single MergeOrder input to deal with all
 * the data.  There should be a more general data structure that allows coping
 * with groups of mergejoin clauses that have different join operators.
 */
static Oid *
generate_merge_input_sortorder(List *pathkeys, MergeOrder *mergeorder)
{
	int			listlength = length(pathkeys);
	Oid		   *result = (Oid *) palloc(sizeof(Oid) * (listlength + 1));
	Oid		   *nextsortop = result;
	List	   *p;

	foreach(p, pathkeys)
	{
		Var		   *pkey = (Var *) lfirst((List *) lfirst(p));

		Assert(IsA(pkey, Var));
		if (pkey->vartype == mergeorder->left_type)
			*nextsortop++ = mergeorder->left_operator;
		else if (pkey->vartype == mergeorder->right_type)
			*nextsortop++ = mergeorder->right_operator;
		else
			elog(ERROR,
			 "generate_merge_input_sortorder: can't handle data type %d",
				 pkey->vartype);
	}
	*nextsortop++ = InvalidOid;
	return result;
}

/*
 * set_noname_tlist_operators
 *	  Sets the key and keyop fields of resdom nodes in a target list.
 *
 *	  'tlist' is the target list
 *	  'pathkeys' is a list of N keys in the form((key1) (key2)...(keyn)),
 *				corresponding to vars in the target list that are to
 *				be sorted or hashed
 *	  'operators' is the corresponding list of N sort or hash operators
 *
 *	  Returns the modified-in-place target list.
 */
static List *
set_noname_tlist_operators(List *tlist, List *pathkeys, Oid *operators)
{
	int			keyno = 1;
	Node	   *pathkey;
	Resdom	   *resdom;
	List	   *i;

	foreach(i, pathkeys)
	{
		pathkey = lfirst((List *) lfirst(i));
		resdom = tlist_member((Var *) pathkey, tlist);
		if (resdom)
		{

			/*
			 * Order the resdom pathkey and replace the operator OID for
			 * each key with the regproc OID.
			 */
			resdom->reskey = keyno;
			resdom->reskeyop = get_opcode(operators[keyno - 1]);
		}
		keyno += 1;
	}
	return tlist;
}

/*
 * Copy cost and size info from a lower plan node to an inserted node.
 * This is not critical, since the decisions have already been made,
 * but it helps produce more reasonable-looking EXPLAIN output.
 */

static void
copy_costsize(Plan *dest, Plan *src)
{
	if (src)
	{
		dest->cost = src->cost;
		dest->plan_size = src->plan_size;
		dest->plan_width = src->plan_width;
	}
	else
	{
		dest->cost = 0;
		dest->plan_size = 0;
		dest->plan_width = 0;
	}
}


/*****************************************************************************
 *
 *
 *****************************************************************************/

/*
 * make_noname
 *	  Create plan nodes to sort or materialize relations into noname. The
 *	  result returned for a sort will look like (SEQSCAN(SORT(plan_node)))
 *	  or (SEQSCAN(MATERIAL(plan_node)))
 *
 *	  'tlist' is the target list of the scan to be sorted or hashed
 *	  'pathkeys' is the list of keys which the sort or hash will be done on
 *	  'operators' is the operators with which the sort or hash is to be done
 *		(a list of operator OIDs)
 *	  'plan_node' is the node which yields tuples for the sort
 *	  'nonametype' indicates which operation(sort or hash) to perform
 */
static Noname *
make_noname(List *tlist,
			List *pathkeys,
			Oid *operators,
			Plan *plan_node,
			int nonametype)
{
	List	   *noname_tlist;
	Noname	   *retval = NULL;

	/* Create a new target list for the noname, with keys set. */
	noname_tlist = set_noname_tlist_operators(new_unsorted_tlist(tlist),
											  pathkeys,
											  operators);
	switch (nonametype)
	{
		case NONAME_SORT:
			retval = (Noname *) make_seqscan(tlist,
											 NIL,
											 _NONAME_RELATION_ID_,
										 (Plan *) make_sort(noname_tlist,
													_NONAME_RELATION_ID_,
															plan_node,
													  length(pathkeys)));
			break;

		case NONAME_MATERIAL:
			retval = (Noname *) make_seqscan(tlist,
											 NIL,
											 _NONAME_RELATION_ID_,
									 (Plan *) make_material(noname_tlist,
													_NONAME_RELATION_ID_,
															plan_node,
													  length(pathkeys)));
			break;

		default:
			elog(ERROR, "make_noname: unknown noname type %d", nonametype);

	}
	return retval;
}


SeqScan    *
make_seqscan(List *qptlist,
			 List *qpqual,
			 Index scanrelid,
			 Plan *lefttree)
{
	SeqScan    *node = makeNode(SeqScan);
	Plan	   *plan = &node->plan;

	copy_costsize(plan, lefttree);
	plan->state = (EState *) NULL;
	plan->targetlist = qptlist;
	plan->qual = qpqual;
	plan->lefttree = lefttree;
	plan->righttree = NULL;
	node->scanrelid = scanrelid;
	node->scanstate = (CommonScanState *) NULL;

	return node;
}

static IndexScan *
make_indexscan(List *qptlist,
			   List *qpqual,
			   Index scanrelid,
			   List *indxid,
			   List *indxqual,
			   List *indxqualorig)
{
	IndexScan  *node = makeNode(IndexScan);
	Plan	   *plan = &node->scan.plan;

	copy_costsize(plan, NULL);
	plan->state = (EState *) NULL;
	plan->targetlist = qptlist;
	plan->qual = qpqual;
	plan->lefttree = NULL;
	plan->righttree = NULL;
	node->scan.scanrelid = scanrelid;
	node->indxid = indxid;
	node->indxqual = indxqual;
	node->indxqualorig = indxqualorig;
	node->indxorderdir = NoMovementScanDirection;
	node->scan.scanstate = (CommonScanState *) NULL;

	return node;
}


static NestLoop *
make_nestloop(List *qptlist,
			  List *qpqual,
			  Plan *lefttree,
			  Plan *righttree)
{
	NestLoop   *node = makeNode(NestLoop);
	Plan	   *plan = &node->join;

	/*
	 * this cost estimate is entirely bogus... hopefully it will be
	 * overwritten by caller.
	 */
	plan->cost = (lefttree ? lefttree->cost : 0) +
		(righttree ? righttree->cost : 0);
	plan->state = (EState *) NULL;
	plan->targetlist = qptlist;
	plan->qual = qpqual;
	plan->lefttree = lefttree;
	plan->righttree = righttree;
	node->nlstate = (NestLoopState *) NULL;

	return node;
}

static HashJoin *
make_hashjoin(List *tlist,
			  List *qpqual,
			  List *hashclauses,
			  Plan *lefttree,
			  Plan *righttree)
{
	HashJoin   *node = makeNode(HashJoin);
	Plan	   *plan = &node->join;

	/*
	 * this cost estimate is entirely bogus... hopefully it will be
	 * overwritten by caller.
	 */
	plan->cost = (lefttree ? lefttree->cost : 0) +
		(righttree ? righttree->cost : 0);
	plan->state = (EState *) NULL;
	plan->targetlist = tlist;
	plan->qual = qpqual;
	plan->lefttree = lefttree;
	plan->righttree = righttree;
	node->hashclauses = hashclauses;
	node->hashdone = false;

	return node;
}

static Hash *
make_hash(List *tlist, Var *hashkey, Plan *lefttree)
{
	Hash	   *node = makeNode(Hash);
	Plan	   *plan = &node->plan;

	copy_costsize(plan, lefttree);
	plan->state = (EState *) NULL;
	plan->targetlist = tlist;
	plan->qual = NULL;
	plan->lefttree = lefttree;
	plan->righttree = NULL;
	node->hashkey = hashkey;

	return node;
}

static MergeJoin *
make_mergejoin(List *tlist,
			   List *qpqual,
			   List *mergeclauses,
			   Plan *righttree,
			   Plan *lefttree)
{
	MergeJoin  *node = makeNode(MergeJoin);
	Plan	   *plan = &node->join;

	/*
	 * this cost estimate is entirely bogus... hopefully it will be
	 * overwritten by caller.
	 */
	plan->cost = (lefttree ? lefttree->cost : 0) +
		(righttree ? righttree->cost : 0);
	plan->state = (EState *) NULL;
	plan->targetlist = tlist;
	plan->qual = qpqual;
	plan->lefttree = lefttree;
	plan->righttree = righttree;
	node->mergeclauses = mergeclauses;

	return node;
}

Sort *
make_sort(List *tlist, Oid nonameid, Plan *lefttree, int keycount)
{
	Sort	   *node = makeNode(Sort);
	Plan	   *plan = &node->plan;

	copy_costsize(plan, lefttree);
	plan->cost += cost_sort(NULL, plan->plan_size, plan->plan_width);
	plan->state = (EState *) NULL;
	plan->targetlist = tlist;
	plan->qual = NIL;
	plan->lefttree = lefttree;
	plan->righttree = NULL;
	node->nonameid = nonameid;
	node->keycount = keycount;

	return node;
}

static Material *
make_material(List *tlist,
			  Oid nonameid,
			  Plan *lefttree,
			  int keycount)
{
	Material   *node = makeNode(Material);
	Plan	   *plan = &node->plan;

	copy_costsize(plan, lefttree);
	plan->state = (EState *) NULL;
	plan->targetlist = tlist;
	plan->qual = NIL;
	plan->lefttree = lefttree;
	plan->righttree = NULL;
	node->nonameid = nonameid;
	node->keycount = keycount;

	return node;
}

Agg *
make_agg(List *tlist, Plan *lefttree)
{
	Agg		   *node = makeNode(Agg);

	copy_costsize(&node->plan, lefttree);
	node->plan.state = (EState *) NULL;
	node->plan.qual = NULL;
	node->plan.targetlist = tlist;
	node->plan.lefttree = lefttree;
	node->plan.righttree = (Plan *) NULL;
	node->aggs = NIL;

	return node;
}

Group *
make_group(List *tlist,
		   bool tuplePerGroup,
		   int ngrp,
		   AttrNumber *grpColIdx,
		   Sort *lefttree)
{
	Group	   *node = makeNode(Group);

	copy_costsize(&node->plan, (Plan *) lefttree);
	node->plan.state = (EState *) NULL;
	node->plan.qual = NULL;
	node->plan.targetlist = tlist;
	node->plan.lefttree = (Plan *) lefttree;
	node->plan.righttree = (Plan *) NULL;
	node->tuplePerGroup = tuplePerGroup;
	node->numCols = ngrp;
	node->grpColIdx = grpColIdx;

	return node;
}

/*
 *	A unique node always has a SORT node in the lefttree.
 *
 *	the uniqueAttr argument must be a null-terminated string,
 * either the name of the attribute to select unique on
 * or "*"
 */

Unique *
make_unique(List *tlist, Plan *lefttree, char *uniqueAttr)
{
	Unique	   *node = makeNode(Unique);
	Plan	   *plan = &node->plan;

	copy_costsize(plan, lefttree);
	plan->state = (EState *) NULL;
	plan->targetlist = tlist;
	plan->qual = NIL;
	plan->lefttree = lefttree;
	plan->righttree = NULL;
	node->nonameid = _NONAME_RELATION_ID_;
	node->keycount = 0;
	if (strcmp(uniqueAttr, "*") == 0)
		node->uniqueAttr = NULL;
	else
		node->uniqueAttr = pstrdup(uniqueAttr);
	return node;
}

#ifdef NOT_USED
List *
generate_fjoin(List *tlist)
{
	List		tlistP;
	List		newTlist = NIL;
	List		fjoinList = NIL;
	int			nIters = 0;

	/*
	 * Break the target list into elements with Iter nodes, and those
	 * without them.
	 */
	foreach(tlistP, tlist)
	{
		List		tlistElem;

		tlistElem = lfirst(tlistP);
		if (IsA(lsecond(tlistElem), Iter))
		{
			nIters++;
			fjoinList = lappend(fjoinList, tlistElem);
		}
		else
			newTlist = lappend(newTlist, tlistElem);
	}

	/*
	 * if we have an Iter node then we need to flatten.
	 */
	if (nIters > 0)
	{
		List	   *inner;
		List	   *tempList;
		Fjoin	   *fjoinNode;
		DatumPtr	results = (DatumPtr) palloc(nIters * sizeof(Datum));
		BoolPtr		alwaysDone = (BoolPtr) palloc(nIters * sizeof(bool));

		inner = lfirst(fjoinList);
		fjoinList = lnext(fjoinList);
		fjoinNode = (Fjoin) MakeFjoin(false,
									  nIters,
									  inner,
									  results,
									  alwaysDone);
		tempList = lcons(fjoinNode, fjoinList);
		newTlist = lappend(newTlist, tempList);
	}
	return newTlist;
	return tlist;				/* do nothing for now - ay 10/94 */
}

#endif
