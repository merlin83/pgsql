/*-------------------------------------------------------------------------
 *
 * nodeIndexscan.h--
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeIndexscan.h,v 1.4 1997-09-08 02:36:25 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEINDEXSCAN_H
#define NODEINDEXSCAN_H

extern TupleTableSlot *ExecIndexScan(IndexScan * node);

extern void ExecIndexReScan(IndexScan * node, ExprContext * exprCtxt, Plan * parent);

extern void ExecEndIndexScan(IndexScan * node);

extern void ExecIndexMarkPos(IndexScan * node);

extern void ExecIndexRestrPos(IndexScan * node);

extern void ExecUpdateIndexScanKeys(IndexScan * node, ExprContext * econtext);

extern bool ExecInitIndexScan(IndexScan * node, EState * estate, Plan * parent);

extern int	ExecCountSlotsIndexScan(IndexScan * node);

extern void ExecIndexReScan(IndexScan * node, ExprContext * exprCtxt, Plan * parent);

#endif							/* NODEINDEXSCAN_H */
