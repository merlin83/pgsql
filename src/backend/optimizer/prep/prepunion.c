/*-------------------------------------------------------------------------
 *
 * prepunion.c
 *	  Routines to plan inheritance, union, and version queries
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/prep/prepunion.c,v 1.39 1999-08-21 03:49:05 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>

#include "postgres.h"

#include "optimizer/plancat.h"
#include "optimizer/planmain.h"
#include "optimizer/planner.h"
#include "optimizer/prep.h"
#include "parser/parse_clause.h"
#include "parser/parsetree.h"
#include "utils/lsyscache.h"

static List *plan_inherit_query(Relids relids, Index rt_index,
				   RangeTblEntry *rt_entry, Query *parse, List *tlist,
				   List **union_rtentriesPtr);
static RangeTblEntry *new_rangetable_entry(Oid new_relid,
					 RangeTblEntry *old_entry);
static Query *subst_rangetable(Query *root, Index index,
				 RangeTblEntry *new_entry);
static void fix_parsetree_attnums(Index rt_index, Oid old_relid,
					  Oid new_relid, Query *parsetree);
static Append *make_append(List *appendplans, List *unionrtables, Index rt_index,
			List *inheritrtable, List *tlist);


/*
 * plan_union_queries
 *
 *	  Plans the queries for a given UNION.
 *
 * Returns a list containing a list of plans and a list of rangetables
 */
Append *
plan_union_queries(Query *parse)
{
	List	   *union_plans = NIL,
			   *ulist,
			   *union_all_queries,
			   *union_rts,
			   *last_union = NIL,
			   *hold_sortClause = parse->sortClause;
	bool		union_all_found = false,
				union_found = false,
				last_union_all_flag = false;

	/*------------------------------------------------------------------
	 *
	 * Do we need to split up our unions because we have UNION and UNION
	 * ALL?
	 *
	 * We are checking for the case of: SELECT 1 UNION SELECT 2 UNION SELECT
	 * 3 UNION ALL SELECT 4 UNION ALL SELECT 5
	 *
	 * where we have to do a DISTINCT on the output of the first three
	 * queries, then add the rest.	If they have used UNION and UNION ALL,
	 * we grab all queries up to the last UNION query, make them their own
	 * UNION with the owner as the first query in the list.  Then, we take
	 * the remaining queries, which is UNION ALL, and add them to the list
	 * of union queries.
	 *
	 * So the above query becomes:
	 *
	 *	Append Node
	 *	{
	 *		Sort and Unique
	 *		{
	 *			Append Node
	 *			{
	 *				SELECT 1		This is really a sub-UNION.
	 *				unionClause		We run a DISTINCT on these.
	 *				{
	 *					SELECT 2
	 *					SELECT 3
	 *				}
	 *			}
	 *		}
	 *		SELECT 4
	 *		SELECT 5
	 *	}
	 *
	 *---------------------------------------------------------------------
	 */

	foreach(ulist, parse->unionClause)
	{
		Query	   *union_query = lfirst(ulist);

		if (union_query->unionall)
			union_all_found = true;
		else
		{
			union_found = true;
			last_union = ulist;
		}
		last_union_all_flag = union_query->unionall;
	}

	/* Is this a simple one */
	if (!union_all_found ||
		!union_found ||
	/* A trailing UNION negates the affect of earlier UNION ALLs */
		!last_union_all_flag)
	{
		List	   *hold_unionClause = parse->unionClause;

		/* we will do this later, so don't do it now */
		if (!union_all_found ||
			!last_union_all_flag)
		{
			parse->sortClause = NIL;
			parse->uniqueFlag = NULL;
		}

		parse->unionClause = NIL;		/* prevent recursion */
		union_plans = lcons(union_planner(parse), NIL);
		union_rts = lcons(parse->rtable, NIL);

		foreach(ulist, hold_unionClause)
		{
			Query	   *union_query = lfirst(ulist);

			union_plans = lappend(union_plans, union_planner(union_query));
			union_rts = lappend(union_rts, union_query->rtable);
		}
	}
	else
	{

		/*
		 * We have mixed unions and non-unions
		 *
		 * We need to restructure this to put the UNIONs on their own so we
		 * can do a DISTINCT.
		 */

		/* save off everthing past the last UNION */
		union_all_queries = lnext(last_union);

		/* clip off the list to remove the trailing UNION ALLs */
		lnext(last_union) = NIL;

		/*
		 * Recursion, but UNION only. The last one is a UNION, so it will
		 * not come here in recursion,
		 */
		union_plans = lcons(union_planner(parse), NIL);
		union_rts = lcons(parse->rtable, NIL);

		/* Append the remaining UNION ALLs */
		foreach(ulist, union_all_queries)
		{
			Query	   *union_all_query = lfirst(ulist);

			union_plans = lappend(union_plans, union_planner(union_all_query));
			union_rts = lappend(union_rts, union_all_query->rtable);
		}
	}

	/* We have already split UNION and UNION ALL and we made it consistent */
	if (!last_union_all_flag)
	{
		/* Need SELECT DISTINCT behavior to implement UNION.
		 * Set uniqueFlag properly, put back the held sortClause,
		 * and add any missing columns to the sort clause.
		 */
		parse->uniqueFlag = "*";
		parse->sortClause = addAllTargetsToSortList(hold_sortClause,
													parse->targetList);
	}
	else
	{
		/* needed so we don't take the flag from the first query */
		parse->uniqueFlag = NULL;
	}

	/* Make sure we don't try to apply the first query's grouping stuff
	 * to the Append node, either.  Basically we don't want union_planner
	 * to do anything when we return control, except add the top sort/unique
	 * nodes for DISTINCT processing if this wasn't UNION ALL, or the top
	 * sort node if it was UNION ALL with a user-provided sort clause.
	 */
	parse->groupClause = NULL;
	parse->havingQual = NULL;
	parse->hasAggs = false;

	return (make_append(union_plans,
						union_rts,
						0,
						NULL,
						parse->targetList));
}


/*
 * plan_inherit_queries
 *
 *	  Plans the queries for a given parent relation.
 *
 * Inputs:
 *	parse = parent parse tree
 *	tlist = target list for inheritance subqueries (not same as parent's!)
 *	rt_index = rangetable index for current inheritance item
 *
 * Returns an APPEND node that forms the result of performing the given
 * query for each member relation of the inheritance group.
 *
 * If grouping, aggregation, or sorting is specified in the parent plan,
 * the subplans should not do any of those steps --- we must do those
 * operations just once above the APPEND node.  The given tlist has been
 * modified appropriately to remove group/aggregate expressions, but the
 * Query node still has the relevant fields set.  We remove them in the
 * copies used for subplans (see plan_inherit_query).
 *
 * NOTE: this can be invoked recursively if more than one inheritance wildcard
 * is present.  At each level of recursion, the first wildcard remaining in
 * the rangetable is expanded.
 */
Append *
plan_inherit_queries(Query *parse, List *tlist, Index rt_index)
{
	List	   *rangetable = parse->rtable;
	RangeTblEntry *rt_entry = rt_fetch(rt_index, rangetable);
	List	   *inheritrtable = NIL;
	List	   *union_relids;
	List	   *union_plans;

	union_relids = find_all_inheritors(lconsi(rt_entry->relid,
											  NIL),
									   NIL);

	/*
	 * Remove the flag for this relation, since we're about to handle it
	 * (do it before recursing!). XXX destructive change to parent parse tree
	 */
	rt_entry->inh = false;

	union_plans = plan_inherit_query(union_relids, rt_index, rt_entry,
									 parse, tlist, &inheritrtable);

	return (make_append(union_plans,
						NULL,
						rt_index,
						inheritrtable,
						((Plan *) lfirst(union_plans))->targetlist));
}

/*
 * plan_inherit_query
 *	  Returns a list of plans for 'relids' and a list of range table entries
 *	  in union_rtentries.
 */
static List *
plan_inherit_query(Relids relids,
				   Index rt_index,
				   RangeTblEntry *rt_entry,
				   Query *root,
				   List *tlist,
				   List **union_rtentriesPtr)
{
	List	   *union_plans = NIL;
	List	   *union_rtentries = NIL;
	List	   *i;

	foreach(i, relids)
	{
		int			relid = lfirsti(i);
		RangeTblEntry *new_rt_entry = new_rangetable_entry(relid,
														   rt_entry);
		Query	   *new_root = subst_rangetable(root,
												rt_index,
												new_rt_entry);

		/*
		 * Insert the desired simplified tlist into the subquery
		 */
		new_root->targetList = copyObject(tlist);

		/*
		 * Clear the sorting and grouping qualifications in the subquery,
		 * so that sorting will only be done once after append
		 */
		new_root->uniqueFlag = NULL;
		new_root->sortClause = NULL;
		new_root->groupClause = NULL;
		new_root->havingQual = NULL;
		new_root->hasAggs = false; /* shouldn't be any left ... */

		/* Fix attribute numbers as necessary */
		fix_parsetree_attnums(rt_index,
							  rt_entry->relid,
							  relid,
							  new_root);

		union_plans = lappend(union_plans, union_planner(new_root));
		union_rtentries = lappend(union_rtentries, new_rt_entry);
	}

	*union_rtentriesPtr = union_rtentries;
	return union_plans;
}

/*
 * find_all_inheritors -
 *		Returns a list of relids corresponding to relations that inherit
 *		attributes from any relations listed in either of the argument relid
 *		lists.
 */
List *
find_all_inheritors(Relids unexamined_relids,
					Relids examined_relids)
{
	List	   *new_inheritors = NIL;
	List	   *new_examined_relids;
	List	   *new_unexamined_relids;
	List	   *rels;

	/*
	 * Find all relations which inherit from members of
	 * 'unexamined_relids' and store them in 'new_inheritors'.
	 */
	foreach(rels, unexamined_relids)
	{
		new_inheritors = LispUnioni(new_inheritors,
									find_inheritance_children(lfirsti(rels)));
	}

	new_examined_relids = LispUnioni(examined_relids, unexamined_relids);
	new_unexamined_relids = set_differencei(new_inheritors,
											new_examined_relids);

	if (new_unexamined_relids == NIL)
		return new_examined_relids;
	else
		return find_all_inheritors(new_unexamined_relids,
								   new_examined_relids);
}

/*
 * first_inherit_rt_entry -
 *		Given a rangetable, find the first rangetable entry that represents
 *		the appropriate special case.
 *
 *		Returns a rangetable index.,  Returns -1 if no matches
 */
int
first_inherit_rt_entry(List *rangetable)
{
	int			count = 0;
	List	   *temp;

	foreach(temp, rangetable)
	{
		RangeTblEntry *rt_entry = lfirst(temp);

		if (rt_entry->inh)
			return count + 1;
		count++;
	}

	return -1;
}

/*
 * new_rangetable_entry -
 *		Replaces the name and relid of 'old_entry' with the values for
 *		'new_relid'.
 *
 *		Returns a copy of 'old_entry' with the parameters substituted.
 */
static RangeTblEntry *
new_rangetable_entry(Oid new_relid, RangeTblEntry *old_entry)
{
	RangeTblEntry *new_entry = copyObject(old_entry);

	/* ??? someone tell me what the following is doing! - ay 11/94 */
	if (!strcmp(new_entry->refname, "*CURRENT*") ||
		!strcmp(new_entry->refname, "*NEW*"))
		new_entry->refname = get_rel_name(new_relid);
	else
		new_entry->relname = get_rel_name(new_relid);

	new_entry->relid = new_relid;
	return new_entry;
}

/*
 * subst_rangetable
 *	  Replaces the 'index'th rangetable entry in 'root' with 'new_entry'.
 *
 * Returns a new copy of 'root'.
 */
static Query *
subst_rangetable(Query *root, Index index, RangeTblEntry *new_entry)
{
	Query	   *new_root = copyObject(root);
	List	   *temp;
	int			i;

	for (temp = new_root->rtable, i = 1; i < index; temp = lnext(temp), i++)
		;
	lfirst(temp) = new_entry;

	return new_root;
}

/*
 * Adjust varnos for child tables.  This routine makes it possible for
 * child tables to have different column positions for the "same" attribute
 * as a parent, which helps ALTER TABLE ADD COLUMN.  Unfortunately this isn't
 * nearly enough to make it work transparently; there are other places where
 * things fall down if children and parents don't have the same column numbers
 * for inherited attributes.  It'd be better to rip this code out and fix
 * ALTER TABLE...
 */
static void
fix_parsetree_attnums_nodes(Index rt_index,
							Oid old_relid,
							Oid new_relid,
							Node *node)
{
	if (node == NULL)
		return;

	switch (nodeTag(node))
	{
		case T_TargetEntry:
			{
				TargetEntry *tle = (TargetEntry *) node;

				fix_parsetree_attnums_nodes(rt_index, old_relid, new_relid,
											tle->expr);
			}
			break;
		case T_Expr:
			{
				Expr	   *expr = (Expr *) node;

				fix_parsetree_attnums_nodes(rt_index, old_relid, new_relid,
											(Node *) expr->args);
			}
			break;
		case T_Var:
			{
				Var		   *var = (Var *) node;

				if (var->varno == rt_index && var->varattno != 0)
				{
					var->varattno = get_attnum(new_relid,
								 get_attname(old_relid, var->varattno));
				}
			}
			break;
		case T_List:
			{
				List	   *l;

				foreach(l, (List *) node)
				{
					fix_parsetree_attnums_nodes(rt_index, old_relid, new_relid,
												(Node *) lfirst(l));
				}
			}
			break;
		default:
			break;
	}
}

/*
 * fix_parsetree_attnums
 *	  Replaces attribute numbers from the relation represented by
 *	  'old_relid' in 'parsetree' with the attribute numbers from
 *	  'new_relid'.
 *
 * Returns the destructively_modified parsetree.
 *
 */
static void
fix_parsetree_attnums(Index rt_index,
					  Oid old_relid,
					  Oid new_relid,
					  Query *parsetree)
{
	if (old_relid == new_relid)
		return;

	fix_parsetree_attnums_nodes(rt_index, old_relid, new_relid,
								(Node *) parsetree->targetList);
	fix_parsetree_attnums_nodes(rt_index, old_relid, new_relid,
								parsetree->qual);
}

static Append *
make_append(List *appendplans,
			List *unionrtables,
			Index rt_index,
			List *inheritrtable,
			List *tlist)
{
	Append	   *node = makeNode(Append);
	List	   *subnode;

	node->appendplans = appendplans;
	node->unionrtables = unionrtables;
	node->inheritrelid = rt_index;
	node->inheritrtable = inheritrtable;
	node->plan.cost = 0.0;
	foreach(subnode, appendplans)
		node->plan.cost += ((Plan *) lfirst(subnode))->cost;
	node->plan.state = (EState *) NULL;
	node->plan.targetlist = tlist;
	node->plan.qual = NIL;
	node->plan.lefttree = (Plan *) NULL;
	node->plan.righttree = (Plan *) NULL;

	return node;
}
