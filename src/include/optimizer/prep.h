/*-------------------------------------------------------------------------
 *
 * prep.h
 *	  prototypes for files in optimizer/prep/
 *
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/optimizer/prep.h,v 1.64 2009/01/01 17:24:00 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PREP_H
#define PREP_H

#include "nodes/plannodes.h"
#include "nodes/relation.h"


/*
 * prototypes for prepjointree.c
 */
extern void pull_up_sublinks(PlannerInfo *root);
extern void inline_set_returning_functions(PlannerInfo *root);
extern Node *pull_up_subqueries(PlannerInfo *root, Node *jtnode,
				   bool below_outer_join, bool append_rel_member);
extern void reduce_outer_joins(PlannerInfo *root);
extern Relids get_relids_in_jointree(Node *jtnode, bool include_joins);
extern Relids get_relids_for_join(PlannerInfo *root, int joinrelid);

/*
 * prototypes for prepqual.c
 */
extern Expr *canonicalize_qual(Expr *qual);

/*
 * prototypes for preptlist.c
 */
extern List *preprocess_targetlist(PlannerInfo *root, List *tlist);

/*
 * prototypes for prepunion.c
 */
extern Plan *plan_set_operations(PlannerInfo *root, double tuple_fraction,
					List **sortClauses);

extern List *find_all_inheritors(Oid parentrel);

extern void expand_inherited_tables(PlannerInfo *root);

extern Node *adjust_appendrel_attrs(Node *node, AppendRelInfo *appinfo);

#endif   /* PREP_H */
