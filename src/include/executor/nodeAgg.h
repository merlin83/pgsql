/*-------------------------------------------------------------------------
 *
 * nodeAgg.h
 *
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeAgg.h,v 1.18 2002-12-05 15:50:36 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEAGG_H
#define NODEAGG_H

#include "fmgr.h"
#include "nodes/execnodes.h"

extern int	ExecCountSlotsAgg(Agg *node);
extern AggState *ExecInitAgg(Agg *node, EState *estate);
extern TupleTableSlot *ExecAgg(AggState *node);
extern void ExecEndAgg(AggState *node);
extern void ExecReScanAgg(AggState *node, ExprContext *exprCtxt);

extern Datum aggregate_dummy(PG_FUNCTION_ARGS);

#endif   /* NODEAGG_H */
