/*-------------------------------------------------------------------------
 *
 * nodeAgg.h
 *
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeAgg.h,v 1.11 2000-01-26 05:58:05 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEAGG_H
#define NODEAGG_H

#include "nodes/plannodes.h"

extern TupleTableSlot *ExecAgg(Agg *node);
extern bool ExecInitAgg(Agg *node, EState *estate, Plan *parent);
extern int	ExecCountSlotsAgg(Agg *node);
extern void ExecEndAgg(Agg *node);
extern void ExecReScanAgg(Agg *node, ExprContext *exprCtxt, Plan *parent);

#endif	 /* NODEAGG_H */
