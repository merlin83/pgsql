/*-------------------------------------------------------------------------
 *
 * nodeSeqscan.h
 *
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/executor/nodeSeqscan.h,v 1.23 2006/02/28 04:10:28 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODESEQSCAN_H
#define NODESEQSCAN_H

#include "nodes/execnodes.h"

extern int	ExecCountSlotsSeqScan(SeqScan *node);
extern SeqScanState *ExecInitSeqScan(SeqScan *node, EState *estate, int eflags);
extern TupleTableSlot *ExecSeqScan(SeqScanState *node);
extern void ExecEndSeqScan(SeqScanState *node);
extern void ExecSeqMarkPos(SeqScanState *node);
extern void ExecSeqRestrPos(SeqScanState *node);
extern void ExecSeqReScan(SeqScanState *node, ExprContext *exprCtxt);

#endif   /* NODESEQSCAN_H */
