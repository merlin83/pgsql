/*-------------------------------------------------------------------------
 *
 * nodeAppend.h--
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeAppend.h,v 1.3 1997-09-07 04:57:51 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEAPPEND_H
#define NODEAPPEND_H

extern bool		ExecInitAppend(Append * node, EState * estate, Plan * parent);
extern int		ExecCountSlotsAppend(Append * node);
extern TupleTableSlot *ExecProcAppend(Append * node);
extern void		ExecEndAppend(Append * node);

#endif							/* NODEAPPEND_H */
