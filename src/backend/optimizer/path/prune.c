/*-------------------------------------------------------------------------
 *
 * prune.c
 *	  Routines to prune redundant paths and relations
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/path/Attic/prune.c,v 1.45 2000-01-26 05:56:34 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"


#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"

static List *merge_rel_with_same_relids(RelOptInfo *rel, List *unmerged_rels);

/*
 * merge_rels_with_same_relids
 *	  Removes any redundant relation entries from a list of rel nodes
 *	  'rel_list', merging their pathlists into the first non-duplicate
 *	  relation entry for each value of relids.
 *
 * Returns the resulting list.
 */
void
merge_rels_with_same_relids(List *rel_list)
{
	List	   *i;

	/*
	 * rel_list can shorten while running as duplicate relations are
	 * deleted.  Obviously, the first relation can't be a duplicate,
	 * so the list head pointer won't change.
	 */
	foreach(i, rel_list)
	{
		lnext(i) = merge_rel_with_same_relids((RelOptInfo *) lfirst(i),
											  lnext(i));
	}
}

/*
 * merge_rel_with_same_relids
 *	  Prunes those relations from 'unmerged_rels' that are redundant with
 *	  'rel'.  A relation is redundant if it is built up of the same
 *	  relations as 'rel'.  Paths for the redundant relations are merged into
 *	  the pathlist of 'rel'.
 *
 * Returns a list of non-redundant relations, and sets the pathlist field
 * of 'rel' appropriately.
 *
 */
static List *
merge_rel_with_same_relids(RelOptInfo *rel, List *unmerged_rels)
{
	List	   *result = NIL;
	List	   *i;

	foreach(i, unmerged_rels)
	{
		RelOptInfo *unmerged_rel = (RelOptInfo *) lfirst(i);

		if (same(rel->relids, unmerged_rel->relids))
		{
			/*
			 * These rels are for the same set of base relations,
			 * so get the best of their pathlists.  We assume it's
			 * ok to reassign a path to the other RelOptInfo without
			 * doing more than changing its parent pointer (cf. pathnode.c).
			 */
			rel->pathlist = add_pathlist(rel,
										 rel->pathlist,
										 unmerged_rel->pathlist);
		}
		else
			result = lappend(result, unmerged_rel);
	}
	return result;
}

/*
 * rels_set_cheapest
 *	  For each relation entry in 'rel_list' (which should contain only join
 *	  relations), set pointers to the cheapest path and compute rel size.
 */
void
rels_set_cheapest(Query *root, List *rel_list)
{
	List	   *x;

	foreach(x, rel_list)
	{
		RelOptInfo	   *rel = (RelOptInfo *) lfirst(x);
		JoinPath	   *cheapest;

		cheapest = (JoinPath *) set_cheapest(rel, rel->pathlist);
		if (IsA_JoinPath(cheapest))
			set_joinrel_rows_width(root, rel, cheapest);
		else
			elog(ERROR, "rels_set_cheapest: non JoinPath found");
	}
}
