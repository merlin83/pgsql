/*-------------------------------------------------------------------------
 *
 * lmgr.h
 *	  POSTGRES lock manager definitions.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: lmgr.h,v 1.20 1999-05-25 22:43:26 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef LMGR_H
#define LMGR_H

#include <storage/lock.h>
#include <utils/rel.h>
#include <catalog/catname.h>

#define AccessShareLock			1		/* SELECT */
#define RowShareLock			2		/* SELECT FOR UPDATE */
#define RowExclusiveLock		3		/* INSERT, UPDATE, DELETE */
#define ShareLock				4
#define ShareRowExclusiveLock	5
#define ExclusiveLock			6
#define AccessExclusiveLock		7

extern LOCKMETHOD LockTableId;


typedef struct LockRelId
{
	Oid			relId;			/* a relation identifier */
	Oid			dbId;			/* a database identifier */
} LockRelId;

typedef struct LockInfoData
{
	LockRelId	lockRelId;
} LockInfoData;

typedef LockInfoData *LockInfo;

#define LockInfoIsValid(lockinfo)	PointerIsValid(lockinfo)

extern LOCKMETHOD InitLockTable(void);
extern void RelationInitLockInfo(Relation relation);

extern void LockRelation(Relation relation, LOCKMODE lockmode);
extern void UnlockRelation(Relation relation, LOCKMODE lockmode);

/* this is for indices */
extern void LockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode);
extern void UnlockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode);

/* and this is for transactions */
extern void XactLockTableInsert(TransactionId xid);
extern void XactLockTableDelete(TransactionId xid);
extern void XactLockTableWait(TransactionId xid);

/* proc.c */
extern void InitProcGlobal(IPCKey key, int maxBackends);

#endif	 /* LMGR_H */
