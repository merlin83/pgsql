/*-------------------------------------------------------------------------
 *
 * analyzejoins.c
 *	  Routines for simplifying joins after initial query analysis
 *
 * While we do a great deal of join simplification in prep/prepjointree.c,
 * certain optimizations cannot be performed at that stage for lack of
 * detailed information about the query.  The routines here are invoked
 * after initsplan.c has done its work, and can do additional join removal
 * and simplification steps based on the information extracted.  The penalty
 * is that we have to work harder to clean up after ourselves when we modify
 * the query, since the derived data structures have to be updated too.
 *
 * Portions Copyright (c) 1996-2010, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/optimizer/plan/analyzejoins.c,v 1.1 2010/03/28 22:59:32 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "optimizer/planmain.h"

/* local functions */
static bool join_is_removable(PlannerInfo *root, SpecialJoinInfo *sjinfo);
static void remove_rel_from_query(PlannerInfo *root, int relid);
static List *remove_rel_from_joinlist(List *joinlist, int relid, int *nremoved);


/*
 * remove_useless_joins
 *		Check for relations that don't actually need to be joined at all,
 *		and remove them from the query.
 *
 * We are passed the current joinlist and return the updated list.  Other
 * data structures that have to be updated are accessible via "root".
 */
List *
remove_useless_joins(PlannerInfo *root, List *joinlist)
{
	ListCell   *lc;

	/*
	 * We are only interested in relations that are left-joined to, so we
	 * can scan the join_info_list to find them easily.
	 */
restart:
	foreach(lc, root->join_info_list)
	{
		SpecialJoinInfo *sjinfo = (SpecialJoinInfo *) lfirst(lc);
		int		innerrelid;
		int		nremoved;

		/* Skip if not removable */
		if (!join_is_removable(root, sjinfo))
			continue;

		/*
		 * Currently, join_is_removable can only succeed when the sjinfo's
		 * righthand is a single baserel.  Remove that rel from the query and
		 * joinlist.
		 */
		innerrelid = bms_singleton_member(sjinfo->min_righthand);

		remove_rel_from_query(root, innerrelid);

		/* We verify that exactly one reference gets removed from joinlist */
		nremoved = 0;
		joinlist = remove_rel_from_joinlist(joinlist, innerrelid, &nremoved);
		if (nremoved != 1)
			elog(ERROR, "failed to find relation %d in joinlist", innerrelid);

		/*
		 * We can delete this SpecialJoinInfo from the list too, since it's no
		 * longer of interest.
		 */
		root->join_info_list = list_delete_ptr(root->join_info_list, sjinfo);

		/*
		 * Restart the scan.  This is necessary to ensure we find all
		 * removable joins independently of ordering of the join_info_list
		 * (note that removal of attr_needed bits may make a join appear
		 * removable that did not before).  Also, since we just deleted the
		 * current list cell, we'd have to have some kluge to continue the
		 * list scan anyway.
		 */
		goto restart;
	}

	return joinlist;
}

/*
 * clause_sides_match_join
 *	  Determine whether a join clause is of the right form to use in this join.
 *
 * We already know that the clause is a binary opclause referencing only the
 * rels in the current join.  The point here is to check whether it has the
 * form "outerrel_expr op innerrel_expr" or "innerrel_expr op outerrel_expr",
 * rather than mixing outer and inner vars on either side.	If it matches,
 * we set the transient flag outer_is_left to identify which side is which.
 */
static inline bool
clause_sides_match_join(RestrictInfo *rinfo, Relids outerrelids,
						Relids innerrelids)
{
	if (bms_is_subset(rinfo->left_relids, outerrelids) &&
		bms_is_subset(rinfo->right_relids, innerrelids))
	{
		/* lefthand side is outer */
		rinfo->outer_is_left = true;
		return true;
	}
	else if (bms_is_subset(rinfo->left_relids, innerrelids) &&
			 bms_is_subset(rinfo->right_relids, outerrelids))
	{
		/* righthand side is outer */
		rinfo->outer_is_left = false;
		return true;
	}
	return false;				/* no good for these input relations */
}

/*
 * join_is_removable
 *	  Check whether we need not perform this special join at all, because
 *	  it will just duplicate its left input.
 *
 * This is true for a left join for which the join condition cannot match
 * more than one inner-side row.  (There are other possibly interesting
 * cases, but we don't have the infrastructure to prove them.)  We also
 * have to check that the inner side doesn't generate any variables needed
 * above the join.
 */
static bool
join_is_removable(PlannerInfo *root, SpecialJoinInfo *sjinfo)
{
	int			innerrelid;
	RelOptInfo *innerrel;
	Relids		joinrelids;
	List	   *clause_list = NIL;
	ListCell   *l;
	int			attroff;

	/*
	 * Currently, we only know how to remove left joins to a baserel with
	 * unique indexes.	We can check most of these criteria pretty trivially
	 * to avoid doing useless extra work.  But checking whether any of the
	 * indexes are unique would require iterating over the indexlist, so for
	 * now we just make sure there are indexes of some sort or other.  If none
	 * of them are unique, join removal will still fail, just slightly later.
	 */
	if (sjinfo->jointype != JOIN_LEFT ||
		sjinfo->delay_upper_joins ||
		bms_membership(sjinfo->min_righthand) != BMS_SINGLETON)
		return false;

	innerrelid = bms_singleton_member(sjinfo->min_righthand);
	innerrel = find_base_rel(root, innerrelid);

	if (innerrel->reloptkind != RELOPT_BASEREL ||
		innerrel->rtekind != RTE_RELATION ||
		innerrel->indexlist == NIL)
		return false;

	/* Compute the relid set for the join we are considering */
	joinrelids = bms_union(sjinfo->min_lefthand, sjinfo->min_righthand);

	/*
	 * We can't remove the join if any inner-rel attributes are used above the
	 * join.
	 *
	 * Note that this test only detects use of inner-rel attributes in higher
	 * join conditions and the target list.  There might be such attributes in
	 * pushed-down conditions at this join, too.  We check that case below.
	 *
	 * As a micro-optimization, it seems better to start with max_attr and
	 * count down rather than starting with min_attr and counting up, on the
	 * theory that the system attributes are somewhat less likely to be wanted
	 * and should be tested last.
	 */
	for (attroff = innerrel->max_attr - innerrel->min_attr;
		 attroff >= 0;
		 attroff--)
	{
		if (!bms_is_subset(innerrel->attr_needed[attroff], joinrelids))
			return false;
	}

	/*
	 * Similarly check that the inner rel doesn't produce any PlaceHolderVars
	 * that will be used above the join.
	 */
	foreach(l, root->placeholder_list)
	{
		PlaceHolderInfo *phinfo = (PlaceHolderInfo *) lfirst(l);

		if (bms_is_subset(phinfo->ph_eval_at, innerrel->relids) &&
			!bms_is_subset(phinfo->ph_needed, joinrelids))
			return false;
	}

	/*
	 * Search for mergejoinable clauses that constrain the inner rel against
	 * either the outer rel or a pseudoconstant.  If an operator is
	 * mergejoinable then it behaves like equality for some btree opclass, so
	 * it's what we want.  The mergejoinability test also eliminates clauses
	 * containing volatile functions, which we couldn't depend on.
	 */
	foreach(l, innerrel->joininfo)
	{
		RestrictInfo *restrictinfo = (RestrictInfo *) lfirst(l);

		/* Ignore clauses not pertinent to this join */
		if (!bms_is_subset(restrictinfo->required_relids, joinrelids))
			continue;

		/*
		 * If we find a pushed-down clause, it must have come from above the
		 * outer join and it must contain references to the inner rel.	(If it
		 * had only outer-rel variables, it'd have been pushed down into the
		 * outer rel.)	Therefore, we can conclude that join removal is unsafe
		 * without any examination of the clause contents.
		 */
		if (restrictinfo->is_pushed_down)
			return false;

		/* Ignore if it's not a mergejoinable clause */
		if (!restrictinfo->can_join ||
			restrictinfo->mergeopfamilies == NIL)
			continue;			/* not mergejoinable */

		/*
		 * Check if clause has the form "outer op inner" or "inner op outer".
		 */
		if (!clause_sides_match_join(restrictinfo, sjinfo->min_lefthand,
									 innerrel->relids))
			continue;			/* no good for these input relations */

		/* OK, add to list */
		clause_list = lappend(clause_list, restrictinfo);
	}

	/* Now examine the rel's restriction clauses for var = const clauses */
	foreach(l, innerrel->baserestrictinfo)
	{
		RestrictInfo *restrictinfo = (RestrictInfo *) lfirst(l);

		/*
		 * Note: can_join won't be set for a restriction clause, but
		 * mergeopfamilies will be if it has a mergejoinable operator and
		 * doesn't contain volatile functions.
		 */
		if (restrictinfo->mergeopfamilies == NIL)
			continue;			/* not mergejoinable */

		/*
		 * The clause certainly doesn't refer to anything but the given rel.
		 * If either side is pseudoconstant then we can use it.
		 */
		if (bms_is_empty(restrictinfo->left_relids))
		{
			/* righthand side is inner */
			restrictinfo->outer_is_left = true;
		}
		else if (bms_is_empty(restrictinfo->right_relids))
		{
			/* lefthand side is inner */
			restrictinfo->outer_is_left = false;
		}
		else
			continue;

		/* OK, add to list */
		clause_list = lappend(clause_list, restrictinfo);
	}

	/* Now examine the indexes to see if we have a matching unique index */
	if (relation_has_unique_index_for(root, innerrel, clause_list))
		return true;

	/*
	 * Some day it would be nice to check for other methods of establishing
	 * distinctness.
	 */
	return false;
}


/*
 * Remove the target relid from the planner's data structures, having
 * determined that there is no need to include it in the query.
 *
 * We are not terribly thorough here.  We must make sure that the rel is
 * no longer treated as a baserel, and that attributes of other baserels
 * are no longer marked as being needed at joins involving this rel.
 * In particular, we don't bother removing join quals involving the rel from
 * the joininfo lists; they'll just get ignored, since we will never form a
 * join relation at which they could be evaluated.
 */
static void
remove_rel_from_query(PlannerInfo *root, int relid)
{
	RelOptInfo *rel = find_base_rel(root, relid);
	Index		rti;
	ListCell   *l;

	/*
	 * Mark the rel as "dead" to show it is no longer part of the join tree.
	 * (Removing it from the baserel array altogether seems too risky.)
	 */
	rel->reloptkind = RELOPT_DEADREL;

	/*
	 * Remove references to the rel from other baserels' attr_needed arrays.
	 */
	for (rti = 1; rti < root->simple_rel_array_size; rti++)
	{
		RelOptInfo *otherrel = root->simple_rel_array[rti];
		int			attroff;

		/* there may be empty slots corresponding to non-baserel RTEs */
		if (otherrel == NULL)
			continue;

		Assert(otherrel->relid == rti);		/* sanity check on array */

		/* no point in processing target rel itself */
		if (otherrel == rel)
			continue;

		for (attroff = otherrel->max_attr - otherrel->min_attr;
			 attroff >= 0;
			 attroff--)
		{
			otherrel->attr_needed[attroff] =
				bms_del_member(otherrel->attr_needed[attroff], relid);
		}
	}

	/*
	 * Likewise remove references from PlaceHolderVar data structures.
	 *
	 * Here we have a special case: if a PHV's eval_at set is just the target
	 * relid, we want to leave it that way instead of reducing it to the empty
	 * set.  An empty eval_at set would confuse later processing since it
	 * would match every possible eval placement.
	 */
	foreach(l, root->placeholder_list)
	{
		PlaceHolderInfo *phinfo = (PlaceHolderInfo *) lfirst(l);

		phinfo->ph_eval_at = bms_del_member(phinfo->ph_eval_at, relid);
		if (bms_is_empty(phinfo->ph_eval_at))		/* oops, belay that */
			phinfo->ph_eval_at = bms_add_member(phinfo->ph_eval_at, relid);

		phinfo->ph_needed = bms_del_member(phinfo->ph_needed, relid);
	}
}

/*
 * Remove any occurrences of the target relid from a joinlist structure.
 *
 * It's easiest to build a whole new list structure, so we handle it that
 * way.  Efficiency is not a big deal here.
 *
 * *nremoved is incremented by the number of occurrences removed (there
 * should be exactly one, but the caller checks that).
 */
static List *
remove_rel_from_joinlist(List *joinlist, int relid, int *nremoved)
{
	List	   *result = NIL;
	ListCell   *jl;

	foreach(jl, joinlist)
	{
		Node	   *jlnode = (Node *) lfirst(jl);

		if (IsA(jlnode, RangeTblRef))
		{
			int			varno = ((RangeTblRef *) jlnode)->rtindex;

			if (varno == relid)
				(*nremoved)++;
			else
				result = lappend(result, jlnode);
		}
		else if (IsA(jlnode, List))
		{
			/* Recurse to handle subproblem */
			List   *sublist;

			sublist = remove_rel_from_joinlist((List *) jlnode,
											   relid, nremoved);
			/* Avoid including empty sub-lists in the result */
			if (sublist)
				result = lappend(result, sublist);
		}
		else
		{
			elog(ERROR, "unrecognized joinlist node type: %d",
				 (int) nodeTag(jlnode));
		}
	}

	return result;
}
