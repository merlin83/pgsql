/*-------------------------------------------------------------------------
 *
 * lwlock.h
 *	  Lightweight lock manager
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/storage/lwlock.h,v 1.19 2005/05/19 21:35:47 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef LWLOCK_H
#define LWLOCK_H

/*
 * We have a number of predefined LWLocks, plus a bunch of LWLocks that are
 * dynamically assigned (for shared buffers).  The LWLock structures live
 * in shared memory (since they contain shared data) and are identified by
 * values of this enumerated type.	We abuse the notion of an enum somewhat
 * by allowing values not listed in the enum declaration to be assigned.
 * The extra value MaxDynamicLWLock is there to keep the compiler from
 * deciding that the enum can be represented as char or short ...
 */
typedef enum LWLockId
{
	BufMappingLock,
	BufFreelistLock,
	LockMgrLock,
	OidGenLock,
	XidGenLock,
	ProcArrayLock,
	SInvalLock,
	FreeSpaceLock,
	MMCacheLock,
	WALInsertLock,
	WALWriteLock,
	ControlFileLock,
	CheckpointLock,
	CheckpointStartLock,
	CLogControlLock,
	SubtransControlLock,
	MultiXactGenLock,
	MultiXactOffsetControlLock,
	MultiXactMemberControlLock,
	RelCacheInitLock,
	BgWriterCommLock,

	NumFixedLWLocks,			/* must be last except for
								 * MaxDynamicLWLock */

	MaxDynamicLWLock = 1000000000
} LWLockId;


typedef enum LWLockMode
{
	LW_EXCLUSIVE,
	LW_SHARED
} LWLockMode;


#ifdef LOCK_DEBUG
extern bool Trace_lwlocks;
#endif

extern LWLockId LWLockAssign(void);
extern void LWLockAcquire(LWLockId lockid, LWLockMode mode);
extern bool LWLockConditionalAcquire(LWLockId lockid, LWLockMode mode);
extern void LWLockRelease(LWLockId lockid);
extern void LWLockReleaseAll(void);
extern bool LWLockHeldByMe(LWLockId lockid);

extern int	NumLWLocks(void);
extern int	LWLockShmemSize(void);
extern void CreateLWLocks(void);

#endif   /* LWLOCK_H */
