/*-------------------------------------------------------------------------
 *
 * nodeMergejoin.h
 *
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeMergejoin.h,v 1.16 2001-11-05 17:46:33 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEMERGEJOIN_H
#define NODEMERGEJOIN_H

#include "nodes/plannodes.h"

extern TupleTableSlot *ExecMergeJoin(MergeJoin *node);
extern bool ExecInitMergeJoin(MergeJoin *node, EState *estate, Plan *parent);
extern int	ExecCountSlotsMergeJoin(MergeJoin *node);
extern void ExecEndMergeJoin(MergeJoin *node);
extern void ExecReScanMergeJoin(MergeJoin *node, ExprContext *exprCtxt, Plan *parent);

#endif   /* NODEMERGEJOIN_H; */
