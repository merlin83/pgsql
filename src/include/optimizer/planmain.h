/*-------------------------------------------------------------------------
 *
 * planmain.h
 *	  prototypes for various files in optimizer/plan
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: planmain.h,v 1.65 2003-01-15 19:35:47 tgl Exp $
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
extern void query_planner(Query *root, List *tlist, double tuple_fraction,
						  Path **cheapest_path, Path **sorted_path);

/*
 * prototypes for plan/createplan.c
 */
extern Plan *create_plan(Query *root, Path *best_path);
extern SubqueryScan *make_subqueryscan(List *qptlist, List *qpqual,
				  Index scanrelid, Plan *subplan);
extern Append *make_append(List *appendplans, bool isTarget, List *tlist);
extern Sort *make_sort(Query *root, List *tlist,
		  Plan *lefttree, int keycount);
extern Agg *make_agg(Query *root, List *tlist, List *qual,
					 AggStrategy aggstrategy,
					 int numGroupCols, AttrNumber *grpColIdx,
					 long numGroups, int numAggs,
					 Plan *lefttree);
extern Group *make_group(Query *root, List *tlist,
						 int numGroupCols, AttrNumber *grpColIdx,
						 double numGroups,
						 Plan *lefttree);
extern Material *make_material(List *tlist, Plan *lefttree);
extern Unique *make_unique(List *tlist, Plan *lefttree, List *distinctList);
extern Limit *make_limit(List *tlist, Plan *lefttree,
		   Node *limitOffset, Node *limitCount);
extern SetOp *make_setop(SetOpCmd cmd, List *tlist, Plan *lefttree,
		   List *distinctList, AttrNumber flagColIdx);
extern Result *make_result(List *tlist, Node *resconstantqual, Plan *subplan);

/*
 * prototypes for plan/initsplan.c
 */
extern void add_base_rels_to_query(Query *root, Node *jtnode);
extern void build_base_rel_tlists(Query *root, List *tlist);
extern Relids distribute_quals_to_rels(Query *root, Node *jtnode);
extern void process_implied_equality(Query *root, Node *item1, Node *item2,
						 Oid sortop1, Oid sortop2);
extern bool exprs_known_equal(Query *root, Node *item1, Node *item2);

/*
 * prototypes for plan/setrefs.c
 */
extern void set_plan_references(Plan *plan, List *rtable);
extern List *join_references(List *clauses, List *rtable,
				List *outer_tlist, List *inner_tlist,
				Index acceptable_rel);
extern void fix_opfuncids(Node *node);

#endif   /* PLANMAIN_H */
