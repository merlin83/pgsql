/*-------------------------------------------------------------------------
 *
 * paths.h
 *	  prototypes for various files in optimizer/path (were separate
 *	  header files)
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: paths.h,v 1.42 2000-02-07 04:41:04 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PATHS_H
#define PATHS_H

#include "nodes/relation.h"

/* default GEQO threshold (default value for geqo_rels) */
#define GEQO_RELS 11


/*
 * allpaths.c
 */
extern bool enable_geqo;
extern int	geqo_rels;

extern RelOptInfo *make_one_rel(Query *root);

/*
 * indxpath.c
 *	  routines to generate index paths
 */
extern List *create_index_paths(Query *root, RelOptInfo *rel, List *indices,
								List *restrictinfo_list,
								List *joininfo_list);
extern Oid indexable_operator(Expr *clause, Oid opclass, Oid relam,
							  bool indexkey_on_left);
extern List *extract_or_indexqual_conditions(RelOptInfo *rel,
											 IndexOptInfo *index,
											 Expr *orsubclause);
extern List *expand_indexqual_conditions(List *indexquals);

/*
 * orindxpath.c
 *	  additional routines for indexable OR clauses
 */
extern List *create_or_index_paths(Query *root, RelOptInfo *rel,
								   List *clauses);

/*
 * tidpath.h
 *	  routines to generate tid paths
 */
extern List *create_tidscan_paths(Query *root, RelOptInfo *rel);

/*
 * joinpath.c
 *	   routines to create join paths
 */
extern void add_paths_to_joinrel(Query *root, RelOptInfo *joinrel,
								 RelOptInfo *outerrel,
								 RelOptInfo *innerrel,
								 List *restrictlist);

/*
 * joinrels.c
 *	  routines to determine which relations to join
 */
extern void make_rels_by_joins(Query *root, int level);
extern RelOptInfo *make_rels_by_clause_joins(Query *root,
											 RelOptInfo *old_rel,
											 List *other_rels);
extern RelOptInfo *make_rels_by_clauseless_joins(Query *root,
												 RelOptInfo *old_rel,
												 List *other_rels);

/*
 * pathkeys.c
 *	  utilities for matching and building path keys
 */
typedef enum
{
	PATHKEYS_EQUAL,				/* pathkeys are identical */
	PATHKEYS_BETTER1,			/* pathkey 1 is a superset of pathkey 2 */
	PATHKEYS_BETTER2,			/* vice versa */
	PATHKEYS_DIFFERENT			/* neither pathkey includes the other */
} PathKeysComparison;

extern PathKeysComparison compare_pathkeys(List *keys1, List *keys2);
extern bool pathkeys_contained_in(List *keys1, List *keys2);
extern Path *get_cheapest_path_for_pathkeys(List *paths, List *pathkeys,
											bool indexpaths_only);
extern List *build_index_pathkeys(Query *root, RelOptInfo *rel,
								  IndexOptInfo *index);
extern List *build_join_pathkeys(List *outer_pathkeys,
								 List *join_rel_tlist, List *joinclauses);
extern bool commute_pathkeys(List *pathkeys);
extern List *make_pathkeys_for_sortclauses(List *sortclauses,
										   List *tlist);
extern List *find_mergeclauses_for_pathkeys(List *pathkeys,
											List *restrictinfos);
extern List *make_pathkeys_for_mergeclauses(List *mergeclauses,
											List *tlist);

#endif	 /* PATHS_H */
