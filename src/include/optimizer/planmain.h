/*-------------------------------------------------------------------------
 *
 * planmain.h
 *	  prototypes for various files in optimizer/plan
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: planmain.h,v 1.37 2000-01-27 18:11:45 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLANMAIN_H
#define PLANMAIN_H

#include "nodes/plannodes.h"
#include "nodes/relation.h"

/*
 * prototypes for plan/planmain.c
 */
extern Plan *query_planner(Query *root, List *tlist, List *qual);

/*
 * prototypes for plan/createplan.c
 */
extern Plan *create_plan(Query *root, Path *best_path);
extern SeqScan *make_seqscan(List *qptlist, List *qpqual, Index scanrelid);
extern Sort *make_sort(List *tlist, Oid nonameid, Plan *lefttree,
		  int keycount);
extern Agg *make_agg(List *tlist, Plan *lefttree);
extern Group *make_group(List *tlist, bool tuplePerGroup, int ngrp,
		   AttrNumber *grpColIdx, Plan *lefttree);
extern Noname *make_noname(List *tlist, List *pathkeys, Plan *subplan);
extern Unique *make_unique(List *tlist, Plan *lefttree, List *distinctList);
extern Result *make_result(List *tlist, Node *resconstantqual, Plan *subplan);

/*
 * prototypes for plan/initsplan.c
 */
extern void make_var_only_tlist(Query *root, List *tlist);
extern void add_restrict_and_join_to_rels(Query *root, List *clauses);
extern void add_missing_rels_to_query(Query *root);

/*
 * prototypes for plan/setrefs.c
 */
extern void set_plan_references(Plan *plan);
extern List *join_references(List *clauses, List *outer_tlist,
							 List *inner_tlist, Index acceptable_rel);
extern void fix_opids(Node *node);

/*
 * prep/prepkeyset.c
 */
extern void transformKeySetQuery(Query *origNode);

#endif	 /* PLANMAIN_H */
