/*-------------------------------------------------------------------------
 *
 * orindxpath.c
 *	  Routines to find index paths that match a set of 'or' clauses
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/path/orindxpath.c,v 1.27 1999-07-15 22:39:24 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"


#include "nodes/nodeFuncs.h"

#include "optimizer/internal.h"
#include "optimizer/clauses.h"
#include "optimizer/restrictinfo.h"
#include "optimizer/paths.h"
#include "optimizer/cost.h"
#include "optimizer/plancat.h"

#include "parser/parsetree.h"


static void best_or_subclause_indices(Query *root, RelOptInfo *rel, List *subclauses,
				List *indices, List **indexids, Cost *cost, Cost *selec);
static void best_or_subclause_index(Query *root, RelOptInfo *rel, Expr *subclause,
				   List *indices, int *indexid, Cost *cost, Cost *selec);


/*
 * create_or_index_paths
 *	  Creates index paths for indices that match 'or' clauses.
 *
 * 'rel' is the relation entry for which the paths are to be defined on
 * 'clauses' is the list of available restriction clause nodes
 *
 * Returns a list of these index path nodes.
 *
 */
List *
create_or_index_paths(Query *root,
					  RelOptInfo *rel, List *clauses)
{
	List	   *t_list = NIL;
	List	   *clist;

	foreach(clist, clauses)
	{
		RestrictInfo *clausenode = (RestrictInfo *) (lfirst(clist));

		/*
		 * Check to see if this clause is an 'or' clause, and, if so,
		 * whether or not each of the subclauses within the 'or' clause
		 * has been matched by an index (the 'Index field was set in
		 * (match_or)  if no index matches a given subclause, one of the
		 * lists of index nodes returned by (get_index) will be 'nil').
		 */
		if (valid_or_clause(clausenode) &&
			clausenode->indexids)
		{
			List	   *temp = NIL;
			List	   *index_list = NIL;
			bool		index_flag = true;

			index_list = clausenode->indexids;
			foreach(temp, index_list)
			{
				if (!lfirst(temp))
				{
					index_flag = false;
					break;
				}
			}
			/* do they all have indexes? */
			if (index_flag)
			{					/* used to be a lisp every function */
				IndexPath  *pathnode = makeNode(IndexPath);
				List	   *indexids = NIL;
				Cost		cost;
				Cost		selec;

				best_or_subclause_indices(root,
										  rel,
										  clausenode->clause->args,
										  clausenode->indexids,
										  &indexids,
										  &cost,
										  &selec);

				pathnode->path.pathtype = T_IndexScan;
				pathnode->path.parent = rel;
				pathnode->path.pathorder = makeNode(PathOrder);
				pathnode->path.pathorder->ordtype = SORTOP_ORDER;

				/*
				 * This is an IndexScan, but it does index lookups based
				 * on the order of the fields specified in the WHERE
				 * clause, not in any order, so the sortop is NULL.
				 */
				pathnode->path.pathorder->ord.sortop = NULL;
				pathnode->path.pathkeys = NIL;

				pathnode->indexqual = lcons(clausenode, NIL);
				pathnode->indexid = indexids;
				pathnode->path.path_cost = cost;

				/*
				 * copy restrictinfo list into path for expensive function
				 * processing	 -- JMH, 7/7/92
				 */
				pathnode->path.loc_restrictinfo = set_difference(copyObject((Node *) rel->restrictinfo),
												 lcons(clausenode, NIL));

#ifdef NOT_USED					/* fix xfunc */
				/* add in cost for expensive functions!  -- JMH, 7/7/92 */
				if (XfuncMode != XFUNC_OFF)
					((Path *) pathnode)->path_cost += xfunc_get_path_cost((Path) pathnode);
#endif
				clausenode->selectivity = (Cost) selec;
				t_list = lappend(t_list, pathnode);
			}
		}
	}

	return t_list;
}

/*
 * best_or_subclause_indices
 *	  Determines the best index to be used in conjunction with each subclause
 *	  of an 'or' clause and the cost of scanning a relation using these
 *	  indices.	The cost is the sum of the individual index costs.
 *
 * 'rel' is the node of the relation on which the index is defined
 * 'subclauses' are the subclauses of the 'or' clause
 * 'indices' are those index nodes that matched subclauses of the 'or'
 *		clause
 * 'examined_indexids' is a list of those index ids to be used with
 *		subclauses that have already been examined
 * 'subcost' is the cost of using the indices in 'examined_indexids'
 * 'selec' is a list of all subclauses that have already been examined
 *
 * Returns a list of the indexids, cost, and selectivities of each
 * subclause, e.g., ((i1 i2 i3) cost (s1 s2 s3)), where 'i' is an OID,
 * 'cost' is a flonum, and 's' is a flonum.
 */
static void
best_or_subclause_indices(Query *root,
						  RelOptInfo *rel,
						  List *subclauses,
						  List *indices,
						  List **indexids,		/* return value */
						  Cost *cost,	/* return value */
						  Cost *selec)	/* return value */
{
	List	   *slist;

	*selec = (Cost) 0.0;
	*cost = (Cost) 0.0;

	foreach(slist, subclauses)
	{
		int			best_indexid;
		Cost		best_cost;
		Cost		best_selec;

		best_or_subclause_index(root, rel, lfirst(slist), lfirst(indices),
								&best_indexid, &best_cost, &best_selec);

		*indexids = lappendi(*indexids, best_indexid);
		*cost += best_cost;
		*selec += best_selec;
		if (*selec > (Cost) 1.0)
			*selec = (Cost) 1.0;

		indices = lnext(indices);
	}

	return;
}

/*
 * best_or_subclause_index
 *	  Determines which is the best index to be used with a subclause of
 *	  an 'or' clause by estimating the cost of using each index and selecting
 *	  the least expensive.
 *
 * 'rel' is the node of the relation on which the index is defined
 * 'subclause' is the subclause
 * 'indices' is a list of index nodes that match the subclause
 *
 * Returns a list (index_id index_subcost index_selectivity)
 * (a fixnum, a fixnum, and a flonum respectively).
 *
 */
static void
best_or_subclause_index(Query *root,
						RelOptInfo *rel,
						Expr *subclause,
						List *indices,
						int *retIndexid,		/* return value */
						Cost *retCost,	/* return value */
						Cost *retSelec) /* return value */
{
	List	   *ilist;
	bool		first_run = true;

	/* if we don't match anything, return zeros */
	*retIndexid = 0;
	*retCost = 0.0;
	*retSelec = 0.0;

	foreach(ilist, indices)
	{
		RelOptInfo *index = (RelOptInfo *) lfirst(ilist);

		Datum		value;
		int			flag = 0;
		Cost		subcost;
		AttrNumber	attno = (get_leftop(subclause))->varattno;
		Oid			opno = ((Oper *) subclause->oper)->opno;
		bool		constant_on_right = non_null((Expr *) get_rightop(subclause));
		float		npages,
					selec;

		if (constant_on_right)
			value = ((Const *) get_rightop(subclause))->constvalue;
		else
			value = NameGetDatum("");
		if (constant_on_right)
			flag = (_SELEC_IS_CONSTANT_ || _SELEC_CONSTANT_RIGHT_);
		else
			flag = _SELEC_CONSTANT_RIGHT_;

		index_selectivity(lfirsti(index->relids),
						  index->classlist,
						  lconsi(opno, NIL),
						  getrelid(lfirsti(rel->relids),
								   root->rtable),
						  lconsi(attno, NIL),
						  lconsi(value, NIL),
						  lconsi(flag, NIL),
						  1,
						  &npages,
						  &selec);

		subcost = cost_index((Oid) lfirsti(index->relids),
							 (int) npages,
							 (Cost) selec,
							 rel->pages,
							 rel->tuples,
							 index->pages,
							 index->tuples,
							 false);

		if (first_run || subcost < *retCost)
		{
			*retIndexid = lfirsti(index->relids);
			*retCost = subcost;
			*retSelec = selec;
			first_run = false;
		}
	}

	return;
}
