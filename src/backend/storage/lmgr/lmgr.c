/*-------------------------------------------------------------------------
 *
 * lmgr.c
 *	  POSTGRES lock manager code
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/lmgr/lmgr.c,v 1.55 2003-02-19 04:02:53 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/transam.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "miscadmin.h"
#include "storage/lmgr.h"
#include "utils/inval.h"


static LOCKMASK LockConflicts[] = {
	0,

	/* AccessShareLock */
	(1 << AccessExclusiveLock),

	/* RowShareLock */
	(1 << ExclusiveLock) | (1 << AccessExclusiveLock),

	/* RowExclusiveLock */
	(1 << ShareLock) | (1 << ShareRowExclusiveLock) |
	(1 << ExclusiveLock) | (1 << AccessExclusiveLock),

	/* ShareUpdateExclusiveLock */
	(1 << ShareUpdateExclusiveLock) |
	(1 << ShareLock) | (1 << ShareRowExclusiveLock) |
	(1 << ExclusiveLock) | (1 << AccessExclusiveLock),

	/* ShareLock */
	(1 << RowExclusiveLock) | (1 << ShareUpdateExclusiveLock) |
	(1 << ShareRowExclusiveLock) |
	(1 << ExclusiveLock) | (1 << AccessExclusiveLock),

	/* ShareRowExclusiveLock */
	(1 << RowExclusiveLock) | (1 << ShareUpdateExclusiveLock) |
	(1 << ShareLock) | (1 << ShareRowExclusiveLock) |
	(1 << ExclusiveLock) | (1 << AccessExclusiveLock),

	/* ExclusiveLock */
	(1 << RowShareLock) |
	(1 << RowExclusiveLock) | (1 << ShareUpdateExclusiveLock) |
	(1 << ShareLock) | (1 << ShareRowExclusiveLock) |
	(1 << ExclusiveLock) | (1 << AccessExclusiveLock),

	/* AccessExclusiveLock */
	(1 << AccessShareLock) | (1 << RowShareLock) |
	(1 << RowExclusiveLock) | (1 << ShareUpdateExclusiveLock) |
	(1 << ShareLock) | (1 << ShareRowExclusiveLock) |
	(1 << ExclusiveLock) | (1 << AccessExclusiveLock)

};

LOCKMETHOD	LockTableId = (LOCKMETHOD) NULL;
LOCKMETHOD	LongTermTableId = (LOCKMETHOD) NULL;

/*
 * Create the lock table described by LockConflicts
 */
LOCKMETHOD
InitLockTable(int maxBackends)
{
	int			lockmethod;

	lockmethod = LockMethodTableInit("LockTable",
									 LockConflicts, MAX_LOCKMODES - 1,
									 maxBackends);
	LockTableId = lockmethod;

	if (!(LockTableId))
		elog(ERROR, "InitLockTable: couldn't initialize lock table");

#ifdef USER_LOCKS

	/*
	 * Allocate another tableId for long-term locks
	 */
	LongTermTableId = LockMethodTableRename(LockTableId);
	if (!(LongTermTableId))
		elog(ERROR, "InitLockTable: couldn't rename long-term lock table");
#endif

	return LockTableId;
}

/*
 * RelationInitLockInfo
 *		Initializes the lock information in a relation descriptor.
 *
 *		relcache.c must call this during creation of any reldesc.
 */
void
RelationInitLockInfo(Relation relation)
{
	Assert(RelationIsValid(relation));
	Assert(OidIsValid(RelationGetRelid(relation)));

	relation->rd_lockInfo.lockRelId.relId = RelationGetRelid(relation);

	if (relation->rd_rel->relisshared)
		relation->rd_lockInfo.lockRelId.dbId = InvalidOid;
	else
		relation->rd_lockInfo.lockRelId.dbId = MyDatabaseId;
}

/*
 *		LockRelation
 */
void
LockRelation(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = relation->rd_lockInfo.lockRelId.relId;
	tag.classId = RelOid_pg_class;
	tag.dbId = relation->rd_lockInfo.lockRelId.dbId;
	tag.objsubId.blkno = InvalidBlockNumber;

	if (!LockAcquire(LockTableId, &tag, GetCurrentTransactionId(),
					 lockmode, false))
		elog(ERROR, "LockRelation: LockAcquire failed");

	/*
	 * Check to see if the relcache entry has been invalidated while we
	 * were waiting to lock it.  If so, rebuild it, or elog() trying.
	 * Increment the refcount to ensure that RelationFlushRelation will
	 * rebuild it and not just delete it.
	 */
	RelationIncrementReferenceCount(relation);
	AcceptInvalidationMessages();
	RelationDecrementReferenceCount(relation);
}

/*
 *		ConditionalLockRelation
 *
 * As above, but only lock if we can get the lock without blocking.
 * Returns TRUE iff the lock was acquired.
 *
 * NOTE: we do not currently need conditional versions of the other
 * LockXXX routines in this file, but they could easily be added if needed.
 */
bool
ConditionalLockRelation(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = relation->rd_lockInfo.lockRelId.relId;
	tag.classId = RelOid_pg_class;
	tag.dbId = relation->rd_lockInfo.lockRelId.dbId;
	tag.objsubId.blkno = InvalidBlockNumber;

	if (!LockAcquire(LockTableId, &tag, GetCurrentTransactionId(),
					 lockmode, true))
		return false;

	/*
	 * Check to see if the relcache entry has been invalidated while we
	 * were waiting to lock it.  If so, rebuild it, or elog() trying.
	 * Increment the refcount to ensure that RelationFlushRelation will
	 * rebuild it and not just delete it.
	 */
	RelationIncrementReferenceCount(relation);
	AcceptInvalidationMessages();
	RelationDecrementReferenceCount(relation);

	return true;
}

/*
 *		UnlockRelation
 */
void
UnlockRelation(Relation relation, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = relation->rd_lockInfo.lockRelId.relId;
	tag.classId = RelOid_pg_class;
	tag.dbId = relation->rd_lockInfo.lockRelId.dbId;
	tag.objsubId.blkno = InvalidBlockNumber;

	LockRelease(LockTableId, &tag, GetCurrentTransactionId(), lockmode);
}

/*
 *		LockRelationForSession
 *
 * This routine grabs a session-level lock on the target relation.	The
 * session lock persists across transaction boundaries.  It will be removed
 * when UnlockRelationForSession() is called, or if an elog(ERROR) occurs,
 * or if the backend exits.
 *
 * Note that one should also grab a transaction-level lock on the rel
 * in any transaction that actually uses the rel, to ensure that the
 * relcache entry is up to date.
 */
void
LockRelationForSession(LockRelId *relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = relid->relId;
	tag.classId = RelOid_pg_class;
	tag.dbId = relid->dbId;
	tag.objsubId.blkno = InvalidBlockNumber;

	if (!LockAcquire(LockTableId, &tag, InvalidTransactionId,
					 lockmode, false))
		elog(ERROR, "LockRelationForSession: LockAcquire failed");
}

/*
 *		UnlockRelationForSession
 */
void
UnlockRelationForSession(LockRelId *relid, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = relid->relId;
	tag.classId = RelOid_pg_class;
	tag.dbId = relid->dbId;
	tag.objsubId.blkno = InvalidBlockNumber;

	LockRelease(LockTableId, &tag, InvalidTransactionId, lockmode);
}

/*
 *		LockPage
 *
 * Obtain a page-level lock.  This is currently used by some index access
 * methods to lock index pages.  For heap relations, it is used only with
 * blkno == 0 to signify locking the relation for extension.
 */
void
LockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = relation->rd_lockInfo.lockRelId.relId;
	tag.classId = RelOid_pg_class;
	tag.dbId = relation->rd_lockInfo.lockRelId.dbId;
	tag.objsubId.blkno = blkno;

	if (!LockAcquire(LockTableId, &tag, GetCurrentTransactionId(),
					 lockmode, false))
		elog(ERROR, "LockPage: LockAcquire failed");
}

/*
 *		UnlockPage
 */
void
UnlockPage(Relation relation, BlockNumber blkno, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = relation->rd_lockInfo.lockRelId.relId;
	tag.classId = RelOid_pg_class;
	tag.dbId = relation->rd_lockInfo.lockRelId.dbId;
	tag.objsubId.blkno = blkno;

	LockRelease(LockTableId, &tag, GetCurrentTransactionId(), lockmode);
}

/*
 *		XactLockTableInsert
 *
 * Insert a lock showing that the given transaction ID is running ---
 * this is done during xact startup.  The lock can then be used to wait
 * for the transaction to finish.
 *
 * We need no corresponding unlock function, since the lock will always
 * be released implicitly at transaction commit/abort, never any other way.
 */
void
XactLockTableInsert(TransactionId xid)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = InvalidOid;
	tag.classId = XactLockTableId;
	tag.dbId = InvalidOid;		/* xids are globally unique */
	tag.objsubId.xid = xid;

	if (!LockAcquire(LockTableId, &tag, xid,
					 ExclusiveLock, false))
		elog(ERROR, "XactLockTableInsert: LockAcquire failed");
}

/*
 *		XactLockTableWait
 *
 * Wait for the specified transaction to commit or abort.
 */
void
XactLockTableWait(TransactionId xid)
{
	LOCKTAG		tag;
	TransactionId myxid = GetCurrentTransactionId();

	Assert(!TransactionIdEquals(xid, myxid));

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = InvalidOid;
	tag.classId = XactLockTableId;
	tag.dbId = InvalidOid;
	tag.objsubId.xid = xid;

	if (!LockAcquire(LockTableId, &tag, myxid,
					 ShareLock, false))
		elog(ERROR, "XactLockTableWait: LockAcquire failed");

	LockRelease(LockTableId, &tag, myxid, ShareLock);

	/*
	 * Transaction was committed/aborted/crashed - we have to update
	 * pg_clog if transaction is still marked as running.
	 */
	if (!TransactionIdDidCommit(xid) && !TransactionIdDidAbort(xid))
		TransactionIdAbort(xid);
}

/*
 * LockObject
 *
 * Lock an arbitrary database object.  A standard relation lock would lock the
 * classId of RelOid_pg_class and objId of the relations OID within the pg_class
 * table.  LockObject allows classId to be specified by the caller, thus allowing
 * locks on any row in any system table.
 *
 * If classId is NOT a system table (protected from removal), an additional lock
 * should be held on the relation to prevent it from being dropped.
 */
void
LockObject(Oid objId, Oid classId, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = objId;
	tag.classId = classId;
	tag.dbId = MyDatabaseId;
	tag.objsubId.blkno = InvalidBlockNumber;

	/* Only two reasonable lock types */
	Assert(lockmode == AccessShareLock || lockmode == AccessExclusiveLock);

	if (!LockAcquire(LockTableId, &tag, GetCurrentTransactionId(),
					 lockmode, false))
		elog(ERROR, "LockObject: LockAcquire failed");
}

/*
 * UnlockObject
 */
void
UnlockObject(Oid objId, Oid classId, LOCKMODE lockmode)
{
	LOCKTAG		tag;

	/* NoLock is a no-op */
	if (lockmode == NoLock)
		return;

	MemSet(&tag, 0, sizeof(tag));
	tag.objId = objId;
	tag.classId = classId;
	tag.dbId = MyDatabaseId;
	tag.objsubId.blkno = InvalidBlockNumber;

	/* Only two reasonable lock types */
	Assert(lockmode == AccessShareLock
		  || lockmode == AccessExclusiveLock);

	LockRelease(LockTableId, &tag, GetCurrentTransactionId(), lockmode);
}

