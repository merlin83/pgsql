/*-------------------------------------------------------------------------
 *
 * nodeAgg.h--
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeAgg.h,v 1.3 1997-09-08 02:36:21 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEAGG_H
#define NODEAGG_H

extern TupleTableSlot *ExecAgg(Agg * node);
extern bool ExecInitAgg(Agg * node, EState * estate, Plan * parent);
extern int	ExecCountSlotsAgg(Agg * node);
extern void ExecEndAgg(Agg * node);

#endif							/* NODEAGG_H */
