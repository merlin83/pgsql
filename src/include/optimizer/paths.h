/*-------------------------------------------------------------------------
 *
 * paths.h
 *	  prototypes for various files in optimizer/paths (were separate
 *	  header files
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: paths.h,v 1.24 1999-02-19 05:18:06 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PATHS_H
#define PATHS_H

#include "nodes/nodes.h"
#include "nodes/parsenodes.h"
#include "nodes/relation.h"

/*
 * allpaths.h
 */
extern RelOptInfo *make_one_rel(Query *root, List *rels);

/*
 * indxpath.h
 *	  routines to generate index paths
 */
extern List *create_index_paths(Query *root, RelOptInfo *rel, List *indices,
				 List *restrictinfo_list,
				 List *joininfo_list);

/*
 * joinpath.h
 *	   routines to create join paths
 */
extern void update_rels_pathlist_for_joins(Query *root, List *joinrels);


/*
 * orindxpath.h
 */
extern List *create_or_index_paths(Query *root, RelOptInfo *rel, List *clauses);

/*
 * hashutils.h
 *	  routines to deal with hash keys and clauses
 */
extern List *group_clauses_by_hashop(List *restrictinfo_list,
						int inner_relid);

/*
 * joinutils.h
 *	  generic join method key/clause routines
 */
extern List *order_joinkeys_by_pathkeys(List *pathkeys,
					 List *joinkeys, List *joinclauses, int outer_or_inner,
						List **matchedJoinClausesPtr);
extern List *extract_path_keys(List *joinkeys, List *tlist,
				  int outer_or_inner);
extern Path *get_cheapest_path_for_joinkeys(List *joinkeys,
					PathOrder *ordering, List *paths, int outer_or_inner);
extern List *new_join_pathkeys(List *outer_pathkeys,
				  List *join_rel_tlist, List *joinclauses);

/*
 * mergeutils.h
 *	  routines to deal with merge keys and clauses
 */
extern List *group_clauses_by_order(List *restrictinfo_list,
					   int inner_relid);
extern MergeInfo *match_order_mergeinfo(PathOrder *ordering,
					  List *mergeinfo_list);

/*
 * joinrels.h
 *	  routines to determine which relations to join
 */
extern List *make_rels_by_joins(Query *root, List *old_rels);
extern List *make_rels_by_clause_joins(Query *root, RelOptInfo *old_rel,
								List *joininfo_list, Relids only_relids);
extern List *make_rels_by_clauseless_joins(RelOptInfo *old_rel,
												List *inner_rels);
extern RelOptInfo *make_join_rel(RelOptInfo *outer_rel, RelOptInfo *inner_rel, JoinInfo *joininfo);
extern List *new_join_tlist(List *tlist, int first_resdomno);
extern RelOptInfo *get_cheapest_complete_rel(List *join_rel_list);

/*
 * prototypes for path/prune.c
 */
extern void merge_rels_with_same_relids(List *rel_list);
extern void rels_set_cheapest(List *rel_list);
extern List *del_rels_all_bushy_inactive(List *old_rels);

#endif	 /* PATHS_H */
