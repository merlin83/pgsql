/*-------------------------------------------------------------------------
 *
 * nodeHash.h
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodeHash.h,v 1.14 1999-07-15 15:21:11 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEHASH_H
#define NODEHASH_H

#include "nodes/plannodes.h"

extern TupleTableSlot *ExecHash(Hash *node);
extern bool ExecInitHash(Hash *node, EState *estate, Plan *parent);
extern int	ExecCountSlotsHash(Hash *node);
extern void ExecEndHash(Hash *node);
extern HashJoinTable ExecHashTableCreate(Hash *node);
extern void ExecHashTableDestroy(HashJoinTable hashtable);
extern void ExecHashTableInsert(HashJoinTable hashtable, ExprContext *econtext,
					Var *hashkey);
extern int ExecHashGetBucket(HashJoinTable hashtable, ExprContext *econtext,
				  Var *hashkey);
extern HeapTuple ExecScanHashBucket(HashJoinState *hjstate, List *hjclauses,
				   ExprContext *econtext);
extern void ExecHashTableReset(HashJoinTable hashtable, long ntuples);
extern void ExecReScanHash(Hash *node, ExprContext *exprCtxt, Plan *parent);

#endif	 /* NODEHASH_H */
