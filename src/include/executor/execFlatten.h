/*-------------------------------------------------------------------------
 *
 * execFlatten.h--
 *    prototypes for execFlatten.c.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: execFlatten.h,v 1.2 1997-08-19 21:38:12 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef EXECFLATTEN_H
#define EXECFLATTEN_H

extern Datum ExecEvalIter(Iter *iterNode, ExprContext *econtext, bool *resultIsNull, bool *iterIsDone);

extern void ExecEvalFjoin(TargetEntry *tlist, ExprContext *econtext, bool *isNullVect, bool *fj_isDone);


#endif /* EXECFLATTEN_H */



