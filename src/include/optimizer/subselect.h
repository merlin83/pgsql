/*-------------------------------------------------------------------------
 *
 * subselect.h
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: subselect.h,v 1.18 2003-02-09 00:30:41 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef SUBSELECT_H
#define SUBSELECT_H

#include "nodes/plannodes.h"

extern Index PlannerQueryLevel; /* level of current query */
extern List *PlannerInitPlan;	/* init subplans for current query */
extern List *PlannerParamVar;	/* to get Var from Param->paramid */
extern int	PlannerPlanId;		/* to assign unique ID to subquery plans */

extern Node *convert_IN_to_join(Query *parse, SubLink *sublink);
extern Node *SS_replace_correlation_vars(Node *expr);
extern Node *SS_process_sublinks(Node *expr, bool isQual);
extern void SS_finalize_plan(Plan *plan, List *rtable);

#endif   /* SUBSELECT_H */
