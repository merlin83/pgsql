/*-------------------------------------------------------------------------
 *
 * paths.h
 *	  prototypes for various files in optimizer/path (were separate
 *	  header files)
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: paths.h,v 1.33 1999-07-27 06:23:11 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PATHS_H
#define PATHS_H

#include "nodes/relation.h"

/*
 * allpaths.c
 */
extern RelOptInfo *make_one_rel(Query *root, List *rels);

/*
 * indxpath.c
 *	  routines to generate index paths
 */
extern List *create_index_paths(Query *root, RelOptInfo *rel, List *indices,
				   List *restrictinfo_list,
				   List *joininfo_list);
extern List *expand_indexqual_conditions(List *indexquals);

/*
 * joinpath.c
 *	   routines to create join paths
 */
extern void update_rels_pathlist_for_joins(Query *root, List *joinrels);


/*
 * orindxpath.c
 */
extern List *create_or_index_paths(Query *root, RelOptInfo *rel, List *clauses);

/*
 * hashutils.c
 *	  routines to deal with hash keys and clauses
 */
extern List *group_clauses_by_hashop(List *restrictinfo_list,
						Relids inner_relids);

/*
 * joinutils.c
 *	  generic join method key/clause routines
 */
extern bool order_joinkeys_by_pathkeys(List *pathkeys,
				   List *joinkeys, List *joinclauses, int outer_or_inner,
						   List **matchedJoinKeysPtr,
						   List **matchedJoinClausesPtr);
extern List *make_pathkeys_from_joinkeys(List *joinkeys, List *tlist,
							int outer_or_inner);
extern Path *get_cheapest_path_for_joinkeys(List *joinkeys,
				   PathOrder *ordering, List *paths, int outer_or_inner);
extern List *new_join_pathkeys(List *outer_pathkeys,
				  List *join_rel_tlist, List *joinclauses);

/*
 * mergeutils.c
 *	  routines to deal with merge keys and clauses
 */
extern List *group_clauses_by_order(List *restrictinfo_list,
					   Relids inner_relids);
extern MergeInfo *match_order_mergeinfo(PathOrder *ordering,
					  List *mergeinfo_list);

/*
 * joinrels.c
 *	  routines to determine which relations to join
 */
extern List *make_rels_by_joins(Query *root, List *old_rels);
extern List *make_rels_by_clause_joins(Query *root, RelOptInfo *old_rel,
						  List *joininfo_list, Relids only_relids);
extern List *make_rels_by_clauseless_joins(RelOptInfo *old_rel,
							  List *inner_rels);
extern RelOptInfo *get_cheapest_complete_rel(List *join_rel_list);
extern bool nonoverlap_sets(List *s1, List *s2);
extern bool is_subset(List *s1, List *s2);

/*
 * prototypes for path/prune.c
 */
extern void merge_rels_with_same_relids(List *rel_list);
extern void rels_set_cheapest(List *rel_list);
extern List *del_rels_all_bushy_inactive(List *old_rels);

#endif	 /* PATHS_H */
