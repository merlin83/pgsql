/*-------------------------------------------------------------------------
 *
 * nodeResult.h
 *
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/executor/nodeResult.h,v 1.21 2006/02/28 04:10:28 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODERESULT_H
#define NODERESULT_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsResult(Result *node);
extern ResultState *ExecInitResult(Result *node, EState *estate, int eflags);
extern TupleTableSlot *ExecResult(ResultState *node);
extern void ExecEndResult(ResultState *node);
extern void ExecReScanResult(ResultState *node, ExprContext *exprCtxt);

#endif   /* NODERESULT_H */
