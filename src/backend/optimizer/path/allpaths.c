/*-------------------------------------------------------------------------
 *
 * allpaths.c
 *	  Routines to find possible search paths for processing a query
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/path/allpaths.c,v 1.63 2000-09-12 21:06:52 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "optimizer/cost.h"
#include "optimizer/geqo.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"


bool		enable_geqo = true;
int			geqo_rels = DEFAULT_GEQO_RELS;


static void set_base_rel_pathlist(Query *root);
static List *build_jointree_rels(Query *root);
static RelOptInfo *make_one_rel_by_joins(Query *root, int levels_needed,
										 List *initial_rels);

#ifdef OPTIMIZER_DEBUG
static void debug_print_rel(Query *root, RelOptInfo *rel);

#endif


/*
 * make_one_rel
 *	  Finds all possible access paths for executing a query, returning a
 *	  single rel that represents the join of all base rels in the query.
 */
RelOptInfo *
make_one_rel(Query *root)
{
	int			levels_needed;
	List	   *initial_rels;

	/*
	 * Count the number of top-level jointree nodes.  This is the depth
	 * of the dynamic-programming algorithm we must employ to consider
	 * all ways of joining the top-level nodes.  Currently, we build
	 * JoinExpr joins in exactly the order implied by the join expression,
	 * so no dynamic-programming search is needed within a JoinExpr.
	 */
	levels_needed = length(root->jointree);

	if (levels_needed <= 0)
		return NULL;			/* nothing to do? */

	/*
	 * Generate access paths for the base rels.
	 */
	set_base_rel_pathlist(root);

	/*
	 * Construct a list of rels corresponding to the toplevel jointree nodes.
	 * This may contain both base rels and rels constructed according to
	 * explicit JOIN directives.
	 */
	initial_rels = build_jointree_rels(root);

	if (levels_needed == 1)
	{
		/*
		 * Single jointree node, so we're done.
		 */
		return (RelOptInfo *) lfirst(initial_rels);
	}
	else
	{

		/*
		 * Generate join tree.
		 */
		return make_one_rel_by_joins(root, levels_needed, initial_rels);
	}
}

/*
 * set_base_rel_pathlist
 *	  Finds all paths available for scanning each base-relation entry.
 *	  Sequential scan and any available indices are considered.
 *	  Each useful path is attached to its relation's 'pathlist' field.
 */
static void
set_base_rel_pathlist(Query *root)
{
	List	   *rellist;

	foreach(rellist, root->base_rel_list)
	{
		RelOptInfo *rel = (RelOptInfo *) lfirst(rellist);
		List	   *indices = find_relation_indices(root, rel);

		/* Mark rel with estimated output rows, width, etc */
		set_baserel_size_estimates(root, rel);

		/*
		 * Generate paths and add them to the rel's pathlist.
		 *
		 * Note: add_path() will discard any paths that are dominated by
		 * another available path, keeping only those paths that are
		 * superior along at least one dimension of cost or sortedness.
		 */

		/* Consider sequential scan */
		add_path(rel, create_seqscan_path(rel));

		/* Consider TID scans */
		create_tidscan_paths(root, rel);

		/* Consider index paths for both simple and OR index clauses */
		create_index_paths(root, rel, indices,
						   rel->baserestrictinfo,
						   rel->joininfo);

		/*
		 * Note: create_or_index_paths depends on create_index_paths to
		 * have marked OR restriction clauses with relevant indices; this
		 * is why it doesn't need to be given the list of indices.
		 */
		create_or_index_paths(root, rel, rel->baserestrictinfo);

		/* Now find the cheapest of the paths for this rel */
		set_cheapest(rel);
	}
}

/*
 * build_jointree_rels
 *	  Construct a RelOptInfo for each item in the query's jointree.
 *
 * At present, we handle explicit joins in the FROM clause exactly as
 * specified, with no search for other join orders.  Only the cross-product
 * joins at the top level are involved in the dynamic-programming search.
 */
static List *
build_jointree_rels(Query *root)
{
	List	   *rels = NIL;
	List	   *jt;

	foreach(jt, root->jointree)
	{
		Node	   *jtnode = (Node *) lfirst(jt);

		rels = lappend(rels, make_rel_from_jointree(root, jtnode));
	}
	return rels;
}

/*
 * make_one_rel_by_joins
 *	  Find all possible joinpaths for a query by successively finding ways
 *	  to join component relations into join relations.
 *
 * 'levels_needed' is the number of iterations needed, ie, the number of
 *		independent jointree items in the query.  This is > 1.
 *
 * 'initial_rels' is a list of RelOptInfo nodes for each independent
 *		jointree item.  These are the components to be joined together.
 *
 * Returns the final level of join relations, i.e., the relation that is
 * the result of joining all the original relations together.
 */
static RelOptInfo *
make_one_rel_by_joins(Query *root, int levels_needed, List *initial_rels)
{
	List	  **joinitems;
	int			lev;
	RelOptInfo *rel;

	/*******************************************
	 * genetic query optimizer entry point	   *
	 *	  <utesch@aut.tu-freiberg.de>		   *
	 * rest will be skipped in case of GEQO    *
	 *******************************************/
	if (enable_geqo && levels_needed >= geqo_rels)
		return geqo(root);

	/*
	 * We employ a simple "dynamic programming" algorithm: we first find
	 * all ways to build joins of two jointree items, then all ways to
	 * build joins of three items (from two-item joins and single items),
	 * then four-item joins, and so on until we have considered all ways
	 * to join all the items into one rel.
	 *
	 * joinitems[j] is a list of all the j-item rels.  Initially we set
	 * joinitems[1] to represent all the single-jointree-item relations.
	 */
	joinitems = (List **) palloc((levels_needed+1) * sizeof(List *));
	MemSet(joinitems, 0, (levels_needed+1) * sizeof(List *));

	joinitems[1] = initial_rels;

	for (lev = 2; lev <= levels_needed; lev++)
	{
		List	   *x;

		/*
		 * Determine all possible pairs of relations to be joined at this
		 * level, and build paths for making each one from every available
		 * pair of lower-level relations.
		 */
		joinitems[lev] = make_rels_by_joins(root, lev, joinitems);

		/*
		 * Do cleanup work on each just-processed rel.
		 */
		foreach(x, joinitems[lev])
		{
			rel = (RelOptInfo *) lfirst(x);

#ifdef NOT_USED

			/*
			 * * for each expensive predicate in each path in each
			 * distinct rel, * consider doing pullup  -- JMH
			 */
			if (XfuncMode != XFUNC_NOPULL && XfuncMode != XFUNC_OFF)
				xfunc_trypullup(rel);
#endif

			/* Find and save the cheapest paths for this rel */
			set_cheapest(rel);

#ifdef OPTIMIZER_DEBUG
			debug_print_rel(root, rel);
#endif
		}
	}

	/*
	 * We should have a single rel at the final level,
	 * representing the join of all the base rels.
	 */
	Assert(length(joinitems[levels_needed]) == 1);
	rel = (RelOptInfo *) lfirst(joinitems[levels_needed]);
	Assert(length(rel->relids) == length(root->base_rel_list));

	return rel;
}

/*****************************************************************************
 *
 *****************************************************************************/

#ifdef OPTIMIZER_DEBUG

static void
print_joinclauses(Query *root, List *clauses)
{
	List	   *l;
	extern void print_expr(Node *expr, List *rtable);	/* in print.c */

	foreach(l, clauses)
	{
		RestrictInfo *c = lfirst(l);

		print_expr((Node *) c->clause, root->rtable);
		if (lnext(l))
			printf(" ");
	}
}

static void
print_path(Query *root, Path *path, int indent)
{
	char	   *ptype = NULL;
	JoinPath   *jp;
	bool		join = false;
	int			i;

	for (i = 0; i < indent; i++)
		printf("\t");

	switch (nodeTag(path))
	{
		case T_Path:
			ptype = "SeqScan";
			join = false;
			break;
		case T_IndexPath:
			ptype = "IdxScan";
			join = false;
			break;
		case T_NestPath:
			ptype = "Nestloop";
			join = true;
			break;
		case T_MergePath:
			ptype = "MergeJoin";
			join = true;
			break;
		case T_HashPath:
			ptype = "HashJoin";
			join = true;
			break;
		default:
			break;
	}
	if (join)
	{
		jp = (JoinPath *) path;

		printf("%s rows=%.0f cost=%.2f..%.2f\n",
			   ptype, path->parent->rows,
			   path->startup_cost, path->total_cost);

		if (path->pathkeys)
		{
			for (i = 0; i < indent; i++)
				printf("\t");
			printf("  pathkeys=");
			print_pathkeys(path->pathkeys, root->rtable);
		}

		switch (nodeTag(path))
		{
			case T_MergePath:
			case T_HashPath:
				for (i = 0; i < indent; i++)
					printf("\t");
				printf("  clauses=(");
				print_joinclauses(root, jp->joinrestrictinfo);
				printf(")\n");

				if (nodeTag(path) == T_MergePath)
				{
					MergePath  *mp = (MergePath *) path;

					if (mp->outersortkeys || mp->innersortkeys)
					{
						for (i = 0; i < indent; i++)
							printf("\t");
						printf("  sortouter=%d sortinner=%d\n",
							   ((mp->outersortkeys) ? 1 : 0),
							   ((mp->innersortkeys) ? 1 : 0));
					}
				}
				break;
			default:
				break;
		}
		print_path(root, jp->outerjoinpath, indent + 1);
		print_path(root, jp->innerjoinpath, indent + 1);
	}
	else
	{
		int			relid = lfirsti(path->parent->relids);

		printf("%s(%d) rows=%.0f cost=%.2f..%.2f\n",
			   ptype, relid, path->parent->rows,
			   path->startup_cost, path->total_cost);

		if (path->pathkeys)
		{
			for (i = 0; i < indent; i++)
				printf("\t");
			printf("  pathkeys=");
			print_pathkeys(path->pathkeys, root->rtable);
		}
	}
}

static void
debug_print_rel(Query *root, RelOptInfo *rel)
{
	List	   *l;

	printf("(");
	foreach(l, rel->relids)
		printf("%d ", lfirsti(l));
	printf("): rows=%.0f width=%d\n", rel->rows, rel->width);

	printf("\tpath list:\n");
	foreach(l, rel->pathlist)
		print_path(root, lfirst(l), 1);
	printf("\tcheapest startup path:\n");
	print_path(root, rel->cheapest_startup_path, 1);
	printf("\tcheapest total path:\n");
	print_path(root, rel->cheapest_total_path, 1);
}

#endif	 /* OPTIMIZER_DEBUG */
