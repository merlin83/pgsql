/*-------------------------------------------------------------------------
 *
 * restrictinfo.c
 *	  RestrictInfo node manipulation routines.
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/restrictinfo.c,v 1.15 2002-11-24 21:52:14 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "optimizer/clauses.h"
#include "optimizer/paths.h"
#include "optimizer/restrictinfo.h"


/*
 * restriction_is_or_clause
 *
 * Returns t iff the restrictinfo node contains an 'or' clause.
 */
bool
restriction_is_or_clause(RestrictInfo *restrictinfo)
{
	if (restrictinfo != NULL &&
		or_clause((Node *) restrictinfo->clause))
		return true;
	else
		return false;
}

/*
 * get_actual_clauses
 *
 * Returns a list containing the bare clauses from 'restrictinfo_list'.
 */
List *
get_actual_clauses(List *restrictinfo_list)
{
	List	   *result = NIL;
	List	   *temp;

	foreach(temp, restrictinfo_list)
	{
		RestrictInfo *clause = (RestrictInfo *) lfirst(temp);

		result = lappend(result, clause->clause);
	}
	return result;
}

/*
 * get_actual_join_clauses
 *
 * Extract clauses from 'restrictinfo_list', separating those that
 * syntactically match the join level from those that were pushed down.
 */
void
get_actual_join_clauses(List *restrictinfo_list,
						List **joinquals, List **otherquals)
{
	List	   *temp;

	*joinquals = NIL;
	*otherquals = NIL;

	foreach(temp, restrictinfo_list)
	{
		RestrictInfo *clause = (RestrictInfo *) lfirst(temp);

		if (clause->ispusheddown)
			*otherquals = lappend(*otherquals, clause->clause);
		else
			*joinquals = lappend(*joinquals, clause->clause);
	}
}

/*
 * remove_redundant_join_clauses
 *
 * Given a list of RestrictInfo clauses that are to be applied in a join,
 * remove any duplicate or redundant clauses.
 *
 * We must eliminate duplicates when forming the restrictlist for a joinrel,
 * since we will see many of the same clauses arriving from both input
 * relations. Also, if a clause is a mergejoinable clause, it's possible that
 * it is redundant with previous clauses (see optimizer/README for
 * discussion). We detect that case and omit the redundant clause from the
 * result list.
 *
 * We can detect redundant mergejoinable clauses very cheaply by using their
 * left and right pathkeys, which uniquely identify the sets of equijoined
 * variables in question.  All the members of a pathkey set that are in the
 * left relation have already been forced to be equal; likewise for those in
 * the right relation.  So, we need to have only one clause that checks
 * equality between any set member on the left and any member on the right;
 * by transitivity, all the rest are then equal.
 *
 * Weird special case: if we have two clauses that seem redundant
 * except one is pushed down into an outer join and the other isn't,
 * then they're not really redundant, because one constrains the
 * joined rows after addition of null fill rows, and the other doesn't.
 *
 * The result is a fresh List, but it points to the same member nodes
 * as were in the input.
 */
List *
remove_redundant_join_clauses(Query *root, List *restrictinfo_list,
							  JoinType jointype)
{
	List	   *result = NIL;
	List	   *item;

	foreach(item, restrictinfo_list)
	{
		RestrictInfo *rinfo = (RestrictInfo *) lfirst(item);

		/* eliminate duplicates */
		if (member(rinfo, result))
			continue;

		/* check for redundant merge clauses */
		if (rinfo->mergejoinoperator != InvalidOid)
		{
			bool		redundant = false;
			List	   *olditem;

			cache_mergeclause_pathkeys(root, rinfo);

			foreach(olditem, result)
			{
				RestrictInfo *oldrinfo = (RestrictInfo *) lfirst(olditem);

				if (oldrinfo->mergejoinoperator != InvalidOid &&
					rinfo->left_pathkey == oldrinfo->left_pathkey &&
					rinfo->right_pathkey == oldrinfo->right_pathkey &&
					(rinfo->ispusheddown == oldrinfo->ispusheddown ||
					 !IS_OUTER_JOIN(jointype)))
				{
					redundant = true;
					break;
				}
			}

			if (redundant)
				continue;
		}

		/* otherwise, add it to result list */
		result = lappend(result, rinfo);
	}

	return result;
}
