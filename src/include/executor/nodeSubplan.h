/*-------------------------------------------------------------------------
 *
 * nodeSubplan.h
 *
 *
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeSubplan.h,v 1.17 2003-08-04 02:40:13 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODESUBPLAN_H
#define NODESUBPLAN_H

#include "nodes/execnodes.h"

extern void ExecInitSubPlan(SubPlanState * node, EState *estate);
extern Datum ExecSubPlan(SubPlanState * node,
			ExprContext *econtext,
			bool *isNull);
extern void ExecEndSubPlan(SubPlanState * node);
extern void ExecReScanSetParamPlan(SubPlanState * node, PlanState * parent);

extern void ExecSetParamPlan(SubPlanState * node, ExprContext *econtext);

#endif   /* NODESUBPLAN_H */
