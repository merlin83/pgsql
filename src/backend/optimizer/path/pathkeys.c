/*-------------------------------------------------------------------------
 *
 * joinutils.c
 *	  Utilities for matching and building join and path keys
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/path/pathkeys.c,v 1.9 1999-05-17 00:26:33 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "nodes/pg_list.h"
#include "nodes/relation.h"
#include "nodes/plannodes.h"

#include "optimizer/internal.h"
#include "optimizer/paths.h"
#include "optimizer/var.h"
#include "optimizer/keys.h"
#include "optimizer/tlist.h"
#include "optimizer/joininfo.h"
#include "optimizer/ordering.h"

static int match_pathkey_joinkeys(List *pathkey, List *joinkeys,
						int outer_or_inner);
static List *new_join_pathkey(List *pathkeys, List *join_rel_tlist,
							  List *joinclauses);


/*--------------------
 *	Explanation of Path.pathkeys
 *
 *	Path.pathkeys is a List of List of Var nodes that represent the sort
 *	order of the result generated by the Path.
 *
 *	In single/base relation RelOptInfo's, the Path's represent various ways
 *	of generating the relation and the resulting ordering of the tuples.
 *	Sequential scan Paths have NIL pathkeys, indicating no known ordering.
 *	Index scans have Path.pathkeys that represent the chosen index.
 *	A single-key index pathkeys would be { {tab1_indexkey1} }.  For a
 *	multi-key index pathkeys would be { {tab1_indexkey1}, {tab1_indexkey2} },
 *	indicating major sort by indexkey1 and minor sort by indexkey2.
 *
 *	Multi-relation RelOptInfo Path's are more complicated.  Mergejoins are
 *	only performed with equijoins ("=").  Because of this, the multi-relation
 *	path actually has more than one primary Var key.  For example, a
 *	mergejoin Path of "tab1.col1 = tab2.col1" would generate a pathkeys of
 *	{ {tab1.col1, tab2.col1} }, indicating that the major sort order of the
 *	Path can be taken to be *either* tab1.col1 or tab2.col1.
 *	They are equal, so they are both primary sort keys.  This allows future
 *	joins to use either Var as a pre-sorted key to prevent upper Mergejoins
 *	from having to re-sort the Path.  This is why pathkeys is a List of Lists.
 *
 *	Note that while the order of the top list is meaningful (primary vs.
 *	secondary sort key), the order of each sublist is arbitrary.
 *
 *	For multi-key sorts, if the outer is sorted by a multi-key index, the
 *	multi-key index remains after the join.  If the inner has a multi-key
 *	sort, only the primary key of the inner is added to the result.
 *	Mergejoins only join on the primary key.  Currently, non-primary keys
 *	in the pathkeys List are of limited value.
 *
 *	Although Hashjoins also work only with equijoin operators, it is *not*
 *	safe to consider the output of a Hashjoin to be sorted in any particular
 *	order --- not even the outer path's order.  This is true because the
 *	executor might have to split the join into multiple batches.
 *
 *	NestJoin does not perform sorting, and allows non-equijoins, so it does
 *	not allow useful pathkeys.  (But couldn't we use the outer path's order?)
 *
 *	-- bjm
 *--------------------
 */
 
/****************************************************************************
 *		KEY COMPARISONS
 ****************************************************************************/

/*
 * order_joinkeys_by_pathkeys
 *	  Attempts to match the keys of a path against the keys of join clauses.
 *	  This is done by looking for a matching join key in 'joinkeys' for
 *	  every path key in the list 'path.keys'. If there is a matching join key
 *	  (not necessarily unique) for every path key, then the list of
 *	  corresponding join keys and join clauses are returned in the order in
 *	  which the keys matched the path keys.
 *
 * 'pathkeys' is a list of path keys:
 *		( ( (var) (var) ... ) ( (var) ... ) )
 * 'joinkeys' is a list of join keys:
 *		( (outer inner) (outer inner) ... )
 * 'joinclauses' is a list of clauses corresponding to the join keys in
 *		'joinkeys'
 * 'outer_or_inner' is a flag that selects the desired pathkey of a join key
 *		in 'joinkeys'
 *
 * Returns the join keys and corresponding join clauses in a list if all
 * of the path keys were matched:
 *		(
 *		 ( (outerkey0 innerkey0) ... (outerkeyN or innerkeyN) )
 *		 ( clause0 ... clauseN )
 *		)
 * and nil otherwise.
 *
 * Returns a list of matched join keys and a list of matched join clauses
 * in pointers if valid order can be found.
 */
bool
order_joinkeys_by_pathkeys(List *pathkeys,
							List *joinkeys,
							List *joinclauses,
							int outer_or_inner,
							List **matchedJoinKeysPtr,
							List **matchedJoinClausesPtr)
{
	List	   *matched_joinkeys = NIL;
	List	   *matched_joinclauses = NIL;
	List	   *pathkey = NIL;
	List	   *i = NIL;
	int			matched_joinkey_index = -1;
	int			matched_keys = 0;
	/*
	 *	Reorder the joinkeys by picking out one that matches each pathkey,
	 *	and create a new joinkey/joinclause list in pathkey order
	 */
	foreach(i, pathkeys)
	{
		pathkey = lfirst(i);
		matched_joinkey_index = match_pathkey_joinkeys(pathkey, joinkeys,
													   outer_or_inner);

		if (matched_joinkey_index != -1)
		{
			matched_keys++;
			if (matchedJoinKeysPtr)
			{
				JoinKey	   *joinkey = nth(matched_joinkey_index, joinkeys);
				matched_joinkeys = lappend(matched_joinkeys, joinkey);
			}
			
			if (matchedJoinClausesPtr)
			{
				Expr	   *joinclause = nth(matched_joinkey_index,
											 joinclauses);
				Assert(joinclauses);
				matched_joinclauses = lappend(matched_joinclauses, joinclause);
			}
		}
		else
			/*	A pathkey could not be matched. */
			break;
	}

	/*
	 *	Did we fail to match all the joinkeys?
	 *	Extra pathkeys are no problem.
	 */
	if (matched_keys != length(joinkeys))
	{
			if (matchedJoinKeysPtr)
				*matchedJoinKeysPtr = NIL;
			if (matchedJoinClausesPtr)
				*matchedJoinClausesPtr = NIL;
			return false;
	}

	if (matchedJoinKeysPtr)
		*matchedJoinKeysPtr = matched_joinkeys;
	if (matchedJoinClausesPtr)
		*matchedJoinClausesPtr = matched_joinclauses;
	return true;
}


/*
 * match_pathkey_joinkeys
 *	  Returns the 0-based index into 'joinkeys' of the first joinkey whose
 *	  outer or inner pathkey matches any subkey of 'pathkey'.
 *
 *	All these keys are equivalent, so any of them can match.  See above.
 */
static int
match_pathkey_joinkeys(List *pathkey,
					   List *joinkeys,
					   int outer_or_inner)
{
	Var		   *key;
	int			pos;
	List	   *i, *x;
	JoinKey    *jk;

	foreach(i, pathkey)
	{
		key = (Var *) lfirst(i);
		pos = 0;
		foreach(x, joinkeys)
		{
			jk = (JoinKey *) lfirst(x);
			if (equal(key, extract_join_key(jk, outer_or_inner)))
				return pos;
			pos++;
		}
	}
	return -1;					/* no index found	*/
}


/*
 * get_cheapest_path_for_joinkeys
 *	  Attempts to find a path in 'paths' whose keys match a set of join
 *	  keys 'joinkeys'.	To match,
 *	  1. the path node ordering must equal 'ordering'.
 *	  2. each pathkey of a given path must match(i.e., be(equal) to) the
 *		 appropriate pathkey of the corresponding join key in 'joinkeys',
 *		 i.e., the Nth path key must match its pathkeys against the pathkey of
 *		 the Nth join key in 'joinkeys'.
 *
 * 'joinkeys' is the list of key pairs to which the path keys must be
 *		matched
 * 'ordering' is the ordering of the(outer) path to which 'joinkeys'
 *		must correspond
 * 'paths' is a list of(inner) paths which are to be matched against
 *		each join key in 'joinkeys'
 * 'outer_or_inner' is a flag that selects the desired pathkey of a join key
 *		in 'joinkeys'
 *
 *	Find the cheapest path that matches the join keys
 */
Path *
get_cheapest_path_for_joinkeys(List *joinkeys,
								 PathOrder *ordering,
								 List *paths,
								 int outer_or_inner)
{
	Path	   *matched_path = NULL;
	List	   *i;

	foreach(i, paths)
	{
		Path	   *path = (Path *) lfirst(i);
		int			better_sort;
		
		if (order_joinkeys_by_pathkeys(path->pathkeys, joinkeys, NIL,
									   outer_or_inner, NULL, NULL) &&
			pathorder_match(ordering, path->pathorder, &better_sort) &&
			better_sort == 0)
		{
			if (matched_path == NULL ||
				path->path_cost < matched_path->path_cost)
				matched_path = path;
		}
	}
	return matched_path;
}


/*
 * make_pathkeys_from_joinkeys
 *	  Builds a pathkey list for a path by pulling one of the pathkeys from
 *	  a list of join keys 'joinkeys' and then finding the var node in the
 *	  target list 'tlist' that corresponds to that pathkey.
 *
 * 'joinkeys' is a list of join key pairs
 * 'tlist' is a relation target list
 * 'outer_or_inner' is a flag that selects the desired pathkey of a join key
 *	in 'joinkeys'
 *
 * Returns a list of pathkeys: ((tlvar1)(tlvar2)...(tlvarN)).
 * It is a list of lists because of multi-key indexes.
 */
List *
make_pathkeys_from_joinkeys(List *joinkeys,
							  List *tlist,
							  int outer_or_inner)
{
	List	   *pathkeys = NIL;
	List	   *jk;

	foreach(jk, joinkeys)
	{
		JoinKey    *jkey = (JoinKey *) lfirst(jk);
		Var		   *key;
		List	   *p, *p2;
		bool		found = false;

		key = (Var *) extract_join_key(jkey, outer_or_inner);

		/* check to see if it is in the target list */
		if (matching_tlist_var(key, tlist))
		{
			/*
			 * Include it in the pathkeys list if we haven't already done so
			 */
			foreach(p, pathkeys)
			{
				List	   *pathkey = lfirst(p);
	
				foreach(p2, pathkey)
				{
					Var		   *pkey = lfirst(p2);
	
					if (equal(key, pkey))
					{
						found = true;
						break;
					}
				}
				if (found)
					break;
			}
			if (!found)
				pathkeys = lappend(pathkeys, lcons(key, NIL));
		}
	}
	return pathkeys;
}


/****************************************************************************
 *		NEW PATHKEY FORMATION
 ****************************************************************************/

/*
 * new_join_pathkeys
 *	  Find the path keys for a join relation by finding all vars in the list
 *	  of join clauses 'joinclauses' such that:
 *		(1) the var corresponding to the outer join relation is a
 *			key on the outer path
 *		(2) the var appears in the target list of the join relation
 *	  In other words, add to each outer path key the inner path keys that
 *	  are required for qualification.
 *
 * 'outer_pathkeys' is the list of the outer path's path keys
 * 'join_rel_tlist' is the target list of the join relation
 * 'joinclauses' is the list of restricting join clauses
 *
 * Returns the list of new path keys.
 *
 */
List *
new_join_pathkeys(List *outer_pathkeys,
				  List *join_rel_tlist,
				  List *joinclauses)
{
	List	   *final_pathkeys = NIL;
	List	   *i;

	foreach(i, outer_pathkeys)
	{
		List	   *outer_pathkey = lfirst(i);
		List	   *new_pathkey;

		new_pathkey = new_join_pathkey(outer_pathkey, join_rel_tlist,
									   joinclauses);
		if (new_pathkey != NIL)
			final_pathkeys = lappend(final_pathkeys, new_pathkey);
	}
	return final_pathkeys;
}

/*
 * new_join_pathkey
 *	  Generate an individual pathkey sublist, consisting of the outer vars
 *	  already mentioned in 'pathkey' plus any inner vars that are joined to
 *	  them (and thus can now also be considered path keys, per discussion
 *	  at the top of this file).
 *
 *	  Note that each returned pathkey is the var node found in
 *	  'join_rel_tlist' rather than the joinclause var node.
 *	  (Is this important?)  Also, we return a fully copied list
 *	  that does not share any subnodes with existing data structures.
 *	  (Is that important, either?)
 *
 * Returns a new pathkey (list of pathkey variables).
 *
 */
static List *
new_join_pathkey(List *pathkey,
				 List *join_rel_tlist,
				 List *joinclauses)
{
	List	   *new_pathkey = NIL;
	List	   *i,
			   *j;

	foreach(i, pathkey)
	{
		Var		   *key = (Var *) lfirst(i);
		Expr	   *tlist_key;

		Assert(key);
									
		tlist_key = matching_tlist_var(key, join_rel_tlist);
		if (tlist_key && !member(tlist_key, new_pathkey))
			new_pathkey = lcons(copyObject(tlist_key), new_pathkey);

		foreach(j, joinclauses)
		{
			Expr	   *joinclause = lfirst(j);
			Expr	   *tlist_other_var;

			tlist_other_var = matching_tlist_var(
								other_join_clause_var(key, joinclause),
								join_rel_tlist);
			if (tlist_other_var && !member(tlist_other_var, new_pathkey))
				new_pathkey = lcons(copyObject(tlist_other_var), new_pathkey);
		}
	}

	return new_pathkey;
}
