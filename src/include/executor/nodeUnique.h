/*-------------------------------------------------------------------------
 *
 * nodeUnique.h
 *
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeUnique.h,v 1.14 2001-11-05 17:46:33 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEUNIQUE_H
#define NODEUNIQUE_H

#include "nodes/plannodes.h"

extern TupleTableSlot *ExecUnique(Unique *node);
extern bool ExecInitUnique(Unique *node, EState *estate, Plan *parent);
extern int	ExecCountSlotsUnique(Unique *node);
extern void ExecEndUnique(Unique *node);
extern void ExecReScanUnique(Unique *node, ExprContext *exprCtxt, Plan *parent);

#endif   /* NODEUNIQUE_H */
