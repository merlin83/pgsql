/*-------------------------------------------------------------------------
 *
 * joinpath.c
 *	  Routines to find all possible paths for processing a set of joins
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/path/joinpath.c,v 1.32 1999-02-22 05:26:20 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>
#include <math.h>

#include "postgres.h"

#include "storage/buf_internals.h"

#include "nodes/pg_list.h"
#include "nodes/relation.h"
#include "nodes/plannodes.h"

#include "optimizer/internal.h"
#include "optimizer/paths.h"
#include "optimizer/pathnode.h"
#include "optimizer/keys.h"
#include "optimizer/cost.h"		/* for _enable_{hashjoin,
								 * _enable_mergejoin} */

static Path *best_innerjoin(List *join_paths, List *outer_relid);
static List *sort_inner_and_outer(RelOptInfo *joinrel, RelOptInfo *outerrel, RelOptInfo *innerrel,
					 List *mergeinfo_list);
static List *match_unsorted_outer(RelOptInfo *joinrel, RelOptInfo *outerrel, RelOptInfo *innerrel,
		List *outerpath_list, Path *cheapest_inner, Path *best_innerjoin,
					 List *mergeinfo_list);
static List *match_unsorted_inner(RelOptInfo *joinrel, RelOptInfo *outerrel, RelOptInfo *innerrel,
					 List *innerpath_list, List *mergeinfo_list);
static bool EnoughMemoryForHashjoin(RelOptInfo *hashrel);
static List *hash_inner_and_outer(RelOptInfo *joinrel, RelOptInfo *outerrel, RelOptInfo *innerrel,
					 List *hashinfo_list);

/*
 * update_rels_pathlist_for_joins
 *	  Creates all possible ways to process joins for each of the join
 *	  relations in the list 'joinrels.'  Each unique path will be included
 *	  in the join relation's 'pathlist' field.
 *
 *	  In postgres, n-way joins are handled left-only(permuting clauseless
 *	  joins doesn't usually win much).
 *
 *	  if BushyPlanFlag is true, bushy tree plans will be generated
 *
 * 'joinrels' is the list of relation entries to be joined
 *
 * Modifies the pathlist field of the appropriate rel node to contain
 * the unique join paths.
 * If bushy trees are considered, may modify the relid field of the
 * join rel nodes to flatten the lists.
 *
 * It does a destructive modification.
 */
void
update_rels_pathlist_for_joins(Query *root, List *joinrels)
{
	List	   *mergeinfo_list = NIL;
	List	   *hashinfo_list = NIL;
	List	   *j;

	foreach(j, joinrels)
	{
		RelOptInfo *joinrel = (RelOptInfo *) lfirst(j);
		Relids		innerrelids;
		Relids		outerrelids;
		RelOptInfo *innerrel;
		RelOptInfo *outerrel;
		Path	   *bestinnerjoin;
		List	   *pathlist = NIL;

		/* flatten out relids later in this function */
		outerrelids = lfirst(joinrel->relids);
		innerrelids = lsecond(joinrel->relids);

		/*
		 * base relation id is an integer and join relation relid is a
		 * list of integers.
		 */
		innerrel = (length(innerrelids) == 1) ?
			get_base_rel(root, lfirsti(innerrelids)) :
			get_join_rel(root, innerrelids);
		outerrel = (length(outerrelids) == 1) ?
			get_base_rel(root, lfirsti(outerrelids)) :
			get_join_rel(root, outerrelids);

		bestinnerjoin = best_innerjoin(innerrel->innerjoin, outerrel->relids);

		if (_enable_mergejoin_)
			mergeinfo_list = group_clauses_by_order(joinrel->restrictinfo,
									   lfirsti(innerrel->relids));

		if (_enable_hashjoin_)
			hashinfo_list = group_clauses_by_hashop(joinrel->restrictinfo,
										lfirsti(innerrel->relids));

		/* need to flatten the relids list */
		joinrel->relids = nconc(listCopy(outerrelids),
								listCopy(innerrelids));

		/*
		 * 1. Consider mergejoin paths where both relations must be
		 * explicitly sorted.
		 */
		pathlist = sort_inner_and_outer(joinrel, outerrel,
										innerrel, mergeinfo_list);

		/*
		 * 2. Consider paths where the outer relation need not be
		 * explicitly sorted. This may include either nestloops and
		 * mergejoins where the outer path is already ordered.
		 */
		pathlist = add_pathlist(joinrel, pathlist,
						 match_unsorted_outer(joinrel,
											  outerrel,
											  innerrel,
											  outerrel->pathlist,
											  innerrel->cheapestpath,
											  bestinnerjoin,
											  mergeinfo_list));

		/*
		 * 3. Consider paths where the inner relation need not be
		 * explicitly sorted.  This may include nestloops and mergejoins
		 * the actual nestloop nodes were constructed in
		 * (match_unsorted_outer).
		 */
		pathlist = add_pathlist(joinrel, pathlist,
						 match_unsorted_inner(joinrel, outerrel,
											  innerrel,
											  innerrel->pathlist,
											  mergeinfo_list));

		/*
		 * 4. Consider paths where both outer and inner relations must be
		 * hashed before being joined.
		 */
		pathlist = add_pathlist(joinrel, pathlist,
						 hash_inner_and_outer(joinrel, outerrel,
											  innerrel, hashinfo_list));

		joinrel->pathlist = pathlist;
	}
}

/*
 * best_innerjoin
 *	  Find the cheapest index path that has already been identified by
 *	  (indexable_joinclauses) as being a possible inner path for the given
 *	  outer relation in a nestloop join.
 *
 * 'join_paths' is a list of join nodes
 * 'outer_relid' is the relid of the outer join relation
 *
 * Returns the pathnode of the selected path.
 */
static Path *
best_innerjoin(List *join_paths, Relids outer_relids)
{
	Path	   *cheapest = (Path *) NULL;
	List	   *join_path;

	foreach(join_path, join_paths)
	{
		Path	   *path = (Path *) lfirst(join_path);

		if (intMember(lfirsti(path->joinid), outer_relids)
			&& ((cheapest == NULL ||
				 path_is_cheaper((Path *) lfirst(join_path), cheapest))))
		{
			cheapest = (Path *) lfirst(join_path);
		}
	}
	return cheapest;
}

/*
 * sort_inner_and_outer
 *	  Create mergejoin join paths by explicitly sorting both the outer and
 *	  inner join relations on each available merge ordering.
 *
 * 'joinrel' is the join relation
 * 'outerrel' is the outer join relation
 * 'innerrel' is the inner join relation
 * 'mergeinfo_list' is a list of nodes containing info on(mergejoinable)
 *				clauses for joining the relations
 *
 * Returns a list of mergejoin paths.
 */
static List *
sort_inner_and_outer(RelOptInfo *joinrel,
					 RelOptInfo *outerrel,
					 RelOptInfo *innerrel,
					 List *mergeinfo_list)
{
	List	   *ms_list = NIL;
	MergeInfo  *xmergeinfo = (MergeInfo *) NULL;
	MergePath  *temp_node = (MergePath *) NULL;
	List	   *i;
	List	   *outerkeys = NIL;
	List	   *innerkeys = NIL;
	List	   *merge_pathkeys = NIL;

	foreach(i, mergeinfo_list)
	{
		xmergeinfo = (MergeInfo *) lfirst(i);

		outerkeys = make_pathkeys_from_joinkeys(xmergeinfo->jmethod.jmkeys,
							  outerrel->targetlist,
							  OUTER);

		innerkeys = make_pathkeys_from_joinkeys(xmergeinfo->jmethod.jmkeys,
							  innerrel->targetlist,
							  INNER);

		merge_pathkeys = new_join_pathkeys(outerkeys, joinrel->targetlist,
							  xmergeinfo->jmethod.clauses);

		temp_node = create_mergejoin_path(joinrel,
								  outerrel->size,
								  innerrel->size,
								  outerrel->width,
								  innerrel->width,
								  (Path *) outerrel->cheapestpath,
								  (Path *) innerrel->cheapestpath,
								  merge_pathkeys,
								  xmergeinfo->m_ordering,
								  xmergeinfo->jmethod.clauses,
								  outerkeys,
								  innerkeys);

		ms_list = lappend(ms_list, temp_node);
	}
	return ms_list;
}

/*
 * match_unsorted_outer
 *	  Creates possible join paths for processing a single join relation
 *	  'joinrel' by employing either iterative substitution or
 *	  mergejoining on each of its possible outer paths(assuming that the
 *	  outer relation need not be explicitly sorted).
 *
 *	  1. The inner path is the cheapest available inner path.
 *	  2. Mergejoin wherever possible.  Mergejoin are considered if there
 *		 are mergejoinable join clauses between the outer and inner join
 *		 relations such that the outer path is keyed on the variables
 *		 appearing in the clauses.	The corresponding inner merge path is
 *		 either a path whose keys match those of the outer path(if such a
 *		 path is available) or an explicit sort on the appropriate inner
 *		 join keys, whichever is cheaper.
 *
 * 'joinrel' is the join relation
 * 'outerrel' is the outer join relation
 * 'innerrel' is the inner join relation
 * 'outerpath_list' is the list of possible outer paths
 * 'cheapest_inner' is the cheapest inner path
 * 'best_innerjoin' is the best inner index path(if any)
 * 'mergeinfo_list' is a list of nodes containing info on mergejoinable
 *		clauses
 *
 * Returns a list of possible join path nodes.
 */
static List *
match_unsorted_outer(RelOptInfo *joinrel,
					 RelOptInfo *outerrel,
					 RelOptInfo *innerrel,
					 List *outerpath_list,
					 Path *cheapest_inner,
					 Path *best_innerjoin,
					 List *mergeinfo_list)
{
	Path	   *outerpath = (Path *) NULL;
	List	   *jp_list = NIL;
	List	   *temp_node = NIL;
	List	   *merge_pathkeys = NIL;
	Path	   *nestinnerpath = (Path *) NULL;
	List	   *paths = NIL;
	List	   *i = NIL;
	PathOrder  *outerpath_ordering = NULL;

	foreach(i, outerpath_list)
	{
		List	   *clauses = NIL;
		List	   *matchedJoinKeys = NIL;
		List	   *matchedJoinClauses = NIL;
		MergeInfo  *xmergeinfo = NULL;

		outerpath = (Path *) lfirst(i);

		outerpath_ordering = outerpath->pathorder;

		if (outerpath_ordering)
			xmergeinfo = match_order_mergeinfo(outerpath_ordering,
												mergeinfo_list);

		if (xmergeinfo)
			clauses = xmergeinfo->jmethod.clauses;

		if (clauses)
		{
			List	   *jmkeys = xmergeinfo->jmethod.jmkeys;

			order_joinkeys_by_pathkeys(outerpath->pathkeys,
										jmkeys,
										clauses,
										OUTER,
										&matchedJoinKeys,
										&matchedJoinClauses);
 			merge_pathkeys = new_join_pathkeys(outerpath->pathkeys,
											  joinrel->targetlist, clauses);
		}
		else
			merge_pathkeys = outerpath->pathkeys;

		if (best_innerjoin &&
			path_is_cheaper(best_innerjoin, cheapest_inner))
			nestinnerpath = best_innerjoin;
		else
			nestinnerpath = cheapest_inner;

		paths = lcons(create_nestloop_path(joinrel,
										   outerrel,
										   outerpath,
										   nestinnerpath,
										   merge_pathkeys),
					  NIL);

		if (clauses && matchedJoinKeys)
		{
			bool		path_is_cheaper_than_sort;
			List	   *varkeys = NIL;
			Path	   *mergeinnerpath = get_cheapest_path_for_joinkeys(
														 matchedJoinKeys,
														 outerpath_ordering,
														 innerrel->pathlist,
														 INNER);

			/* Should we use the mergeinner, or sort the cheapest inner? */
			path_is_cheaper_than_sort = (bool) (mergeinnerpath &&
				(mergeinnerpath->path_cost <
				 (cheapest_inner->path_cost +
				  cost_sort(matchedJoinKeys, innerrel->size,
							innerrel->width, false))));
			if (!path_is_cheaper_than_sort)
			{
				varkeys = make_pathkeys_from_joinkeys(matchedJoinKeys,
									  innerrel->targetlist,
									  INNER);
			}


			/*
			 * Keep track of the cost of the outer path used with this
			 * ordered inner path for later processing in
			 * (match_unsorted_inner), since it isn't a sort and thus
			 * wouldn't otherwise be considered.
			 */
			if (path_is_cheaper_than_sort)
				mergeinnerpath->outerjoincost = outerpath->path_cost;
			else
				mergeinnerpath = cheapest_inner;

			temp_node = lcons(create_mergejoin_path(joinrel,
											outerrel->size,
											innerrel->size,
											outerrel->width,
											innerrel->width,
											outerpath,
											mergeinnerpath,
											merge_pathkeys,
											xmergeinfo->m_ordering,
											matchedJoinClauses,
											NIL,
											varkeys),
									  paths);
		}
		else
			temp_node = paths;
		jp_list = nconc(jp_list, temp_node);
	}
	return jp_list;
}

/*
 * match_unsorted_inner 
 *	  Find the cheapest ordered join path for a given(ordered, unsorted)
 *	  inner join path.
 *
 *	  Scans through each path available on an inner join relation and tries
 *	  matching its ordering keys against those of mergejoin clauses.
 *	  If 1. an appropriately_ordered inner path and matching mergeclause are
 *			found, and
 *		 2. sorting the cheapest outer path is cheaper than using an ordered
 *			  but unsorted outer path(as was considered in
 *			(match_unsorted_outer)), then this merge path is considered.
 *
 * 'joinrel' is the join result relation
 * 'outerrel' is the outer join relation
 * 'innerrel' is the inner join relation
 * 'innerpath_list' is the list of possible inner join paths
 * 'mergeinfo_list' is a list of nodes containing info on mergejoinable
 *				clauses
 *
 * Returns a list of possible merge paths.
 */
static List *
match_unsorted_inner(RelOptInfo *joinrel,
					 RelOptInfo *outerrel,
					 RelOptInfo *innerrel,
					 List *innerpath_list,
					 List *mergeinfo_list)
{
	Path	   *innerpath = (Path *) NULL;
	List	   *mp_list = NIL;
	PathOrder  *innerpath_ordering = NULL;
	Cost		temp1 = 0.0;
	bool		temp2 = false;
	List	   *i = NIL;

	foreach(i, innerpath_list)
	{
		MergeInfo  *xmergeinfo = (MergeInfo *) NULL;
		List	   *clauses = NIL;
		List	   *matchedJoinKeys = NIL;
		List	   *matchedJoinClauses = NIL;

		innerpath = (Path *) lfirst(i);

		innerpath_ordering = innerpath->pathorder;

		if (innerpath_ordering)
		{
			xmergeinfo = match_order_mergeinfo(innerpath_ordering,
									  mergeinfo_list);
		}

		if (xmergeinfo)
			clauses = ((JoinMethod *) xmergeinfo)->clauses;

		if (clauses)
		{
			List	   *jmkeys = xmergeinfo->jmethod.jmkeys;

			order_joinkeys_by_pathkeys(innerpath->pathkeys,
										jmkeys,
										clauses,
										INNER,
										&matchedJoinKeys,
										&matchedJoinClauses);
		}

		/*
		 * (match_unsorted_outer) if it is applicable. 'OuterJoinCost was
		 * set above in
		 */
		if (clauses && matchedJoinKeys)
		{
			temp1 = outerrel->cheapestpath->path_cost +
				cost_sort(matchedJoinKeys, outerrel->size, outerrel->width,
						  false);

			temp2 = (bool) (FLOAT_IS_ZERO(innerpath->outerjoincost)
							|| (innerpath->outerjoincost > temp1));

			if (temp2)
			{
				List	   *outerkeys = make_pathkeys_from_joinkeys(matchedJoinKeys,
								  outerrel->targetlist,
								  OUTER);
				List	   *merge_pathkeys = new_join_pathkeys(outerkeys,
								  joinrel->targetlist,
								  clauses);

				mp_list = lappend(mp_list,
								  create_mergejoin_path(joinrel,
												outerrel->size,
												innerrel->size,
												outerrel->width,
												innerrel->width,
										 (Path *) outerrel->cheapestpath,
												innerpath,
												merge_pathkeys,
												xmergeinfo->m_ordering,
												matchedJoinClauses,
												outerkeys,
												NIL));
			}
		}
	}
	return mp_list;

}

static bool
EnoughMemoryForHashjoin(RelOptInfo *hashrel)
{
	int			ntuples;
	int			tupsize;
	int			pages;

	ntuples = hashrel->size;
	if (ntuples == 0)
		ntuples = 1000;
	tupsize = hashrel->width + sizeof(HeapTupleData);
	pages = page_size(ntuples, tupsize);

	/*
	 * if amount of buffer space below hashjoin threshold, return false
	 */
	if (ceil(sqrt((double) pages)) > NBuffers)
		return false;
	return true;
}

/*
 * hash_inner_and_outer--				XXX HASH
 *	  Create hashjoin join paths by explicitly hashing both the outer and
 *	  inner join relations on each available hash op.
 *
 * 'joinrel' is the join relation
 * 'outerrel' is the outer join relation
 * 'innerrel' is the inner join relation
 * 'hashinfo_list' is a list of nodes containing info on(hashjoinable)
 *		clauses for joining the relations
 *
 * Returns a list of hashjoin paths.
 */
static List *
hash_inner_and_outer(RelOptInfo *joinrel,
					 RelOptInfo *outerrel,
					 RelOptInfo *innerrel,
					 List *hashinfo_list)
{
	HashInfo   *xhashinfo = (HashInfo *) NULL;
	List	   *hjoin_list = NIL;
	HashPath   *temp_node = (HashPath *) NULL;
	List	   *i = NIL;
	List	   *outerkeys = NIL;
	List	   *innerkeys = NIL;
	List	   *hash_pathkeys = NIL;

	foreach(i, hashinfo_list)
	{
		xhashinfo = (HashInfo *) lfirst(i);
		outerkeys = make_pathkeys_from_joinkeys(
							  ((JoinMethod *) xhashinfo)->jmkeys,
							  outerrel->targetlist,
							  OUTER);
		innerkeys = make_pathkeys_from_joinkeys(
							  ((JoinMethod *) xhashinfo)->jmkeys,
							  innerrel->targetlist,
							  INNER);
		hash_pathkeys = new_join_pathkeys(outerkeys,
							  joinrel->targetlist,
							  ((JoinMethod *) xhashinfo)->clauses);

		if (EnoughMemoryForHashjoin(innerrel))
		{
			temp_node = create_hashjoin_path(joinrel,
											 outerrel->size,
											 innerrel->size,
											 outerrel->width,
											 innerrel->width,
										 	(Path *) outerrel->cheapestpath,
										 	(Path *) innerrel->cheapestpath,
											 hash_pathkeys,
											 xhashinfo->hashop,
									 	((JoinMethod *) xhashinfo)->clauses,
											 outerkeys,
											 innerkeys);
			hjoin_list = lappend(hjoin_list, temp_node);
		}
	}
	return hjoin_list;
}
