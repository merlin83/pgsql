/*-------------------------------------------------------------------------
 *
 * proc.c
 *	  routines to manage per-process shared memory data structure
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/lmgr/proc.c,v 1.95 2001-01-22 22:30:06 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *	Each postgres backend gets one of these.  We'll use it to
 *	clean up after the process should the process suddenly die.
 *
 *
 * Interface (a):
 *		ProcSleep(), ProcWakeup(), ProcWakeupNext(),
 *		ProcQueueAlloc() -- create a shm queue for sleeping processes
 *		ProcQueueInit() -- create a queue without allocing memory
 *
 * Locking and waiting for buffers can cause the backend to be
 * put to sleep.  Whoever releases the lock, etc. wakes the
 * process up again (and gives it an error code so it knows
 * whether it was awoken on an error condition).
 *
 * Interface (b):
 *
 * ProcReleaseLocks -- frees the locks associated with current transaction
 *
 * ProcKill -- destroys the shared memory state (and locks)
 *		associated with the process.
 *
 * 5/15/91 -- removed the buffer pool based lock chain in favor
 *		of a shared memory lock chain.	The write-protection is
 *		more expensive if the lock chain is in the buffer pool.
 *		The only reason I kept the lock chain in the buffer pool
 *		in the first place was to allow the lock table to grow larger
 *		than available shared memory and that isn't going to work
 *		without a lot of unimplemented support anyway.
 *
 * 4/7/95 -- instead of allocating a set of 1 semaphore per process, we
 *		allocate a semaphore from a set of PROC_NSEMS_PER_SET semaphores
 *		shared among backends (we keep a few sets of semaphores around).
 *		This is so that we can support more backends. (system-wide semaphore
 *		sets run out pretty fast.)				  -ay 4/95
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/lmgr/proc.c,v 1.95 2001-01-22 22:30:06 tgl Exp $
 */
#include "postgres.h"

#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#if defined(solaris_sparc) || defined(__CYGWIN__)
#include <sys/ipc.h>
#include <sys/sem.h>
#endif

#include "miscadmin.h"

#if defined(__darwin__)
#include "port/darwin/sem.h"
#endif

/* In Ultrix and QNX, sem.h must be included after ipc.h */
#ifdef HAVE_SYS_SEM_H
#include <sys/sem.h>
#endif

#include "access/xact.h"
#include "storage/proc.h"


int DeadlockTimeout = 1000;

/* --------------------
 * Spin lock for manipulating the shared process data structure:
 * ProcGlobal.... Adding an extra spin lock seemed like the smallest
 * hack to get around reading and updating this structure in shared
 * memory. -mer 17 July 1991
 * --------------------
 */
SPINLOCK	ProcStructLock;

static PROC_HDR *ProcGlobal = NULL;

PROC	   *MyProc = NULL;

static bool waitingForLock = false;

static void ProcKill(void);
static void ProcGetNewSemIdAndNum(IpcSemaphoreId *semId, int *semNum);
static void ProcFreeSem(IpcSemaphoreId semId, int semNum);
static void ZeroProcSemaphore(PROC *proc);
static void ProcFreeAllSemaphores(void);


/*
 * InitProcGlobal -
 *	  initializes the global process table. We put it here so that
 *	  the postmaster can do this initialization. (ProcFreeAllSemaphores needs
 *	  to read this table on exiting the postmaster. If we have the first
 *	  backend do this, starting up and killing the postmaster without
 *	  starting any backends will be a problem.)
 *
 *	  We also allocate all the per-process semaphores we will need to support
 *	  the requested number of backends.  We used to allocate semaphores
 *	  only when backends were actually started up, but that is bad because
 *	  it lets Postgres fail under load --- a lot of Unix systems are
 *	  (mis)configured with small limits on the number of semaphores, and
 *	  running out when trying to start another backend is a common failure.
 *	  So, now we grab enough semaphores to support the desired max number
 *	  of backends immediately at initialization --- if the sysadmin has set
 *	  MaxBackends higher than his kernel will support, he'll find out sooner
 *	  rather than later.
 */
void
InitProcGlobal(int maxBackends)
{
	bool		found = false;

	/* attach to the free list */
	ProcGlobal = (PROC_HDR *)
		ShmemInitStruct("Proc Header", sizeof(PROC_HDR), &found);

	/* --------------------
	 * We're the first - initialize.
	 * XXX if found should ever be true, it is a sign of impending doom ...
	 * ought to complain if so?
	 * --------------------
	 */
	if (!found)
	{
		int			i;

		ProcGlobal->freeProcs = INVALID_OFFSET;
		for (i = 0; i < PROC_SEM_MAP_ENTRIES; i++)
		{
			ProcGlobal->procSemIds[i] = -1;
			ProcGlobal->freeSemMap[i] = 0;
		}

		/*
		 * Arrange to delete semas on exit --- set this up now so that we
		 * will clean up if pre-allocation fails.  We use our own freeproc,
		 * rather than IpcSemaphoreCreate's removeOnExit option, because
		 * we don't want to fill up the on_shmem_exit list with a separate
		 * entry for each semaphore set.
		 */
		on_shmem_exit(ProcFreeAllSemaphores, 0);

		/*
		 * Pre-create the semaphores for the first maxBackends processes.
		 */
		Assert(maxBackends > 0 && maxBackends <= MAXBACKENDS);

		for (i = 0; i < ((maxBackends-1)/PROC_NSEMS_PER_SET+1); i++)
		{
			IpcSemaphoreId		semId;

			semId = IpcSemaphoreCreate(PROC_NSEMS_PER_SET,
									   IPCProtection,
									   1,
									   false);
			ProcGlobal->procSemIds[i] = semId;
		}
	}
}

/* ------------------------
 * InitProc -- create a per-process data structure for this process
 * used by the lock manager on semaphore queues.
 * ------------------------
 */
void
InitProcess(void)
{
	bool		found = false;
	unsigned long location,
				myOffset;

	SpinAcquire(ProcStructLock);

	/* attach to the ProcGlobal structure */
	ProcGlobal = (PROC_HDR *)
		ShmemInitStruct("Proc Header", sizeof(PROC_HDR), &found);
	if (!found)
	{
		/* this should not happen. InitProcGlobal() is called before this. */
		elog(STOP, "InitProcess: Proc Header uninitialized");
	}

	if (MyProc != NULL)
	{
		SpinRelease(ProcStructLock);
		elog(ERROR, "ProcInit: you already exist");
	}

	/* try to get a proc struct from the free list first */

	myOffset = ProcGlobal->freeProcs;

	if (myOffset != INVALID_OFFSET)
	{
		MyProc = (PROC *) MAKE_PTR(myOffset);
		ProcGlobal->freeProcs = MyProc->links.next;
	}
	else
	{

		/*
		 * have to allocate one.  We can't use the normal shmem index
		 * table mechanism because the proc structure is stored by PID
		 * instead of by a global name (need to look it up by PID when we
		 * cleanup dead processes).
		 */

		MyProc = (PROC *) ShmemAlloc(sizeof(PROC));
		if (!MyProc)
		{
			SpinRelease(ProcStructLock);
			elog(FATAL, "cannot create new proc: out of memory");
		}
	}

	/*
	 * zero out the spin lock counts and set the sLocks field for
	 * ProcStructLock to 1 as we have acquired this spinlock above but
	 * didn't record it since we didn't have MyProc until now.
	 */
	MemSet(MyProc->sLocks, 0, sizeof(MyProc->sLocks));
	MyProc->sLocks[ProcStructLock] = 1;

	/*
	 * Set up a wait-semaphore for the proc.
	 */
	if (IsUnderPostmaster)
	{
		ProcGetNewSemIdAndNum(&MyProc->sem.semId, &MyProc->sem.semNum);
		/*
		 * we might be reusing a semaphore that belongs to a dead backend.
		 * So be careful and reinitialize its value here.
		 */
		ZeroProcSemaphore(MyProc);
	}
	else
	{
		MyProc->sem.semId = -1;
		MyProc->sem.semNum = -1;
	}

	SHMQueueElemInit(&(MyProc->links));
	MyProc->errType = NO_ERROR;
	MyProc->pid = MyProcPid;
	MyProc->databaseId = MyDatabaseId;
	MyProc->xid = InvalidTransactionId;
	MyProc->xmin = InvalidTransactionId;
	MyProc->waitLock = NULL;
	MyProc->waitHolder = NULL;
	SHMQueueInit(&(MyProc->procHolders));

	/* ----------------------
	 * Release the lock.
	 * ----------------------
	 */
	SpinRelease(ProcStructLock);

	/* -------------------------
	 * Install ourselves in the shmem index table.	The name to
	 * use is determined by the OS-assigned process id.  That
	 * allows the cleanup process to find us after any untimely
	 * exit.
	 * -------------------------
	 */
	location = MAKE_OFFSET(MyProc);
	if ((!ShmemPIDLookup(MyProcPid, &location)) ||
		(location != MAKE_OFFSET(MyProc)))
		elog(STOP, "InitProcess: ShmemPID table broken");

	on_shmem_exit(ProcKill, 0);
}

/*
 * Initialize the proc's wait-semaphore to count zero.
 */
static void
ZeroProcSemaphore(PROC *proc)
{
	union semun		semun;

	semun.val = 0;
	if (semctl(proc->sem.semId, proc->sem.semNum, SETVAL, semun) < 0)
	{
		fprintf(stderr, "ZeroProcSemaphore: semctl(id=%d,SETVAL) failed: %s\n",
				proc->sem.semId, strerror(errno));
		proc_exit(255);
	}
}

/*
 * Remove a proc from the wait-queue it is on
 * (caller must know it is on one).
 * Locktable lock must be held by caller.
 *
 * NB: this does not remove the process' holder object, nor the lock object,
 * even though their counts might now have gone to zero.  That will happen
 * during a subsequent LockReleaseAll call, which we expect will happen
 * during transaction cleanup.  (Removal of a proc from its wait queue by
 * this routine can only happen if we are aborting the transaction.)
 */
static void
RemoveFromWaitQueue(PROC *proc)
{
	LOCK   *waitLock = proc->waitLock;
	LOCKMODE lockmode = proc->waitLockMode;

	/* Make sure proc is waiting */
	Assert(proc->links.next != INVALID_OFFSET);
	Assert(waitLock);
	Assert(waitLock->waitProcs.size > 0);

	/* Remove proc from lock's wait queue */
	SHMQueueDelete(&(proc->links));
	waitLock->waitProcs.size--;

	/* Undo increments of request counts by waiting process */
	Assert(waitLock->nRequested > 0);
	Assert(waitLock->nRequested > proc->waitLock->nGranted);
	waitLock->nRequested--;
	Assert(waitLock->requested[lockmode] > 0);
	waitLock->requested[lockmode]--;
	/* don't forget to clear waitMask bit if appropriate */
	if (waitLock->granted[lockmode] == waitLock->requested[lockmode])
		waitLock->waitMask &= ~(1 << lockmode);

	/* Clean up the proc's own state */
	proc->waitLock = NULL;
	proc->waitHolder = NULL;

	/* See if any other waiters for the lock can be woken up now */
	ProcLockWakeup(LOCK_LOCKMETHOD(*waitLock), waitLock);
}

/*
 * Cancel any pending wait for lock, when aborting a transaction.
 *
 * Returns true if we had been waiting for a lock, else false.
 *
 * (Normally, this would only happen if we accept a cancel/die
 * interrupt while waiting; but an elog(ERROR) while waiting is
 * within the realm of possibility, too.)
 */
bool
LockWaitCancel(void)
{
	/* Nothing to do if we weren't waiting for a lock */
	if (!waitingForLock)
		return false;

	waitingForLock = false;

	/* Turn off the deadlock timer, if it's still running (see ProcSleep) */
#ifndef __BEOS__
	{
		struct itimerval timeval,
						 dummy;

		MemSet(&timeval, 0, sizeof(struct itimerval));
		setitimer(ITIMER_REAL, &timeval, &dummy);
	}
#else
	/* BeOS doesn't have setitimer, but has set_alarm */
    set_alarm(B_INFINITE_TIMEOUT, B_PERIODIC_ALARM);
#endif /* __BEOS__ */

	/* Unlink myself from the wait queue, if on it (might not be anymore!) */
	LockLockTable();
	if (MyProc->links.next != INVALID_OFFSET)
		RemoveFromWaitQueue(MyProc);
	UnlockLockTable();

	/*
	 * Reset the proc wait semaphore to zero.  This is necessary in the
	 * scenario where someone else granted us the lock we wanted before we
	 * were able to remove ourselves from the wait-list.  The semaphore will
	 * have been bumped to 1 by the would-be grantor, and since we are no
	 * longer going to wait on the sema, we have to force it back to zero.
	 * Otherwise, our next attempt to wait for a lock will fall through
	 * prematurely.
	 */
	ZeroProcSemaphore(MyProc);

	/*
	 * Return true even if we were kicked off the lock before we were
	 * able to remove ourselves.
	 */
	return true;
}


/*
 * ProcReleaseLocks() -- release locks associated with current transaction
 *			at transaction commit or abort
 *
 * At commit, we release only locks tagged with the current transaction's XID,
 * leaving those marked with XID 0 (ie, session locks) undisturbed.  At abort,
 * we release all locks including XID 0, because we need to clean up after
 * a failure.  This logic will need extension if we ever support nested
 * transactions.
 *
 * Note that user locks are not released in either case.
 */
void
ProcReleaseLocks(bool isCommit)
{
	if (!MyProc)
		return;
	/* If waiting, get off wait queue (should only be needed after error) */
	LockWaitCancel();
	/* Release locks */
	LockReleaseAll(DEFAULT_LOCKMETHOD, MyProc,
				   !isCommit, GetCurrentTransactionId());
}

/*
 * ProcRemove -
 *	  called by the postmaster to clean up the global tables after a
 *	  backend exits.  This also frees up the proc's wait semaphore.
 */
bool
ProcRemove(int pid)
{
	SHMEM_OFFSET location;
	PROC	   *proc;

	location = ShmemPIDDestroy(pid);
	if (location == INVALID_OFFSET)
		return FALSE;
	proc = (PROC *) MAKE_PTR(location);

	SpinAcquire(ProcStructLock);

	ProcFreeSem(proc->sem.semId, proc->sem.semNum);

	/* Add PROC struct to freelist so space can be recycled in future */
	proc->links.next = ProcGlobal->freeProcs;
	ProcGlobal->freeProcs = MAKE_OFFSET(proc);

	SpinRelease(ProcStructLock);

	return TRUE;
}

/*
 * ProcKill() -- Destroy the per-proc data structure for
 *		this process. Release any of its held spin locks.
 *
 * This is done inside the backend process before it exits.
 * ProcRemove, above, will be done by the postmaster afterwards.
 */
static void
ProcKill(void)
{
	Assert(MyProc);

	/* Release any spinlocks I am holding */
	ProcReleaseSpins(MyProc);

	/* Get off any wait queue I might be on */
	LockWaitCancel();

	/* Remove from the standard lock table */
	LockReleaseAll(DEFAULT_LOCKMETHOD, MyProc, true, InvalidTransactionId);

#ifdef USER_LOCKS
	/* Remove from the user lock table */
	LockReleaseAll(USER_LOCKMETHOD, MyProc, true, InvalidTransactionId);
#endif

	MyProc = NULL;
}

/*
 * ProcQueue package: routines for putting processes to sleep
 *		and  waking them up
 */

/*
 * ProcQueueAlloc -- alloc/attach to a shared memory process queue
 *
 * Returns: a pointer to the queue or NULL
 * Side Effects: Initializes the queue if we allocated one
 */
#ifdef NOT_USED
PROC_QUEUE *
ProcQueueAlloc(char *name)
{
	bool		found;
	PROC_QUEUE *queue = (PROC_QUEUE *)
		ShmemInitStruct(name, sizeof(PROC_QUEUE), &found);

	if (!queue)
		return NULL;
	if (!found)
		ProcQueueInit(queue);
	return queue;
}

#endif

/*
 * ProcQueueInit -- initialize a shared memory process queue
 */
void
ProcQueueInit(PROC_QUEUE *queue)
{
	SHMQueueInit(&(queue->links));
	queue->size = 0;
}


/*
 * ProcSleep -- put a process to sleep
 *
 * P() on the semaphore should put us to sleep.  The process
 * semaphore is normally zero, so when we try to acquire it, we sleep.
 *
 * Locktable's spinlock must be held at entry, and will be held
 * at exit.
 *
 * Result is NO_ERROR if we acquired the lock, STATUS_ERROR if not (deadlock).
 *
 * ASSUME: that no one will fiddle with the queue until after
 *		we release the spin lock.
 *
 * NOTES: The process queue is now a priority queue for locking.
 */
int
ProcSleep(LOCKMETHODCTL *lockctl,
		  LOCKMODE lockmode,
		  LOCK *lock,
		  HOLDER *holder)
{
	PROC_QUEUE *waitQueue = &(lock->waitProcs);
	SPINLOCK	spinlock = lockctl->masterLock;
	int			myMask = (1 << lockmode);
	int			waitMask = lock->waitMask;
	PROC	   *proc;
	int			i;
	int			aheadGranted[MAX_LOCKMODES];
	bool		selfConflict = (lockctl->conflictTab[lockmode] & myMask),
				prevSame = false;
#ifndef __BEOS__
	struct itimerval timeval,
				dummy;
#else
    bigtime_t time_interval;
#endif

	proc = (PROC *) MAKE_PTR(waitQueue->links.next);

	/* if we don't conflict with any waiter - be first in queue */
	if (!(lockctl->conflictTab[lockmode] & waitMask))
		goto ins;

	/* otherwise, determine where we should go into the queue */
	for (i = 1; i < MAX_LOCKMODES; i++)
		aheadGranted[i] = lock->granted[i];
	(aheadGranted[lockmode])++;

	for (i = 0; i < waitQueue->size; i++)
	{
		LOCKMODE	procWaitMode = proc->waitLockMode;

		/* must I wait for him ? */
		if (lockctl->conflictTab[lockmode] & proc->heldLocks)
		{
			/* is he waiting for me ? */
			if (lockctl->conflictTab[procWaitMode] & MyProc->heldLocks)
			{
				/* Yes, report deadlock failure */
				MyProc->errType = STATUS_ERROR;
				return STATUS_ERROR;
			}
			/* I must go after him in queue - so continue loop */
		}
		/* if he waits for me, go before him in queue */
		else if (lockctl->conflictTab[procWaitMode] & MyProc->heldLocks)
			break;
		/* if conflicting locks requested */
		else if (lockctl->conflictTab[procWaitMode] & myMask)
		{

			/*
			 * If I request non self-conflicting lock and there are others
			 * requesting the same lock just before this guy - stop here.
			 */
			if (!selfConflict && prevSame)
				break;
		}

		/*
		 * Last attempt to not move any further to the back of the queue:
		 * if we don't conflict with remaining waiters, stop here.
		 */
		else if (!(lockctl->conflictTab[lockmode] & waitMask))
			break;

		/* Move past this guy, and update state accordingly */
		prevSame = (procWaitMode == lockmode);
		(aheadGranted[procWaitMode])++;
		if (aheadGranted[procWaitMode] == lock->requested[procWaitMode])
			waitMask &= ~(1 << procWaitMode);
		proc = (PROC *) MAKE_PTR(proc->links.next);
	}

ins:;
	/* -------------------
	 * Insert self into queue, ahead of the given proc (or at tail of queue).
	 * -------------------
	 */
	SHMQueueInsertBefore(&(proc->links), &(MyProc->links));
	waitQueue->size++;

	lock->waitMask |= myMask;

	/* Set up wait information in PROC object, too */
	MyProc->waitLock = lock;
	MyProc->waitHolder = holder;
	MyProc->waitLockMode = lockmode;
	/* We assume the caller set up MyProc->heldLocks */

	MyProc->errType = NO_ERROR;		/* initialize result for success */

	/* mark that we are waiting for a lock */
	waitingForLock = true;

	/* -------------------
	 * Release the locktable's spin lock.
	 *
	 * NOTE: this may also cause us to exit critical-section state,
	 * possibly allowing a cancel/die interrupt to be accepted.
	 * This is OK because we have recorded the fact that we are waiting for
	 * a lock, and so LockWaitCancel will clean up if cancel/die happens.
	 * -------------------
	 */
	SpinRelease(spinlock);

	/* --------------
	 * Set timer so we can wake up after awhile and check for a deadlock.
	 * If a deadlock is detected, the handler releases the process's
	 * semaphore and sets MyProc->errType = STATUS_ERROR, allowing us to
	 * know that we must report failure rather than success.
	 *
	 * By delaying the check until we've waited for a bit, we can avoid
	 * running the rather expensive deadlock-check code in most cases.
	 *
	 * Need to zero out struct to set the interval and the micro seconds fields
	 * to 0.
	 * --------------
	 */
#ifndef __BEOS__
	MemSet(&timeval, 0, sizeof(struct itimerval));
	timeval.it_value.tv_sec = DeadlockTimeout / 1000;
	timeval.it_value.tv_usec = (DeadlockTimeout % 1000) * 1000;
	if (setitimer(ITIMER_REAL, &timeval, &dummy))
		elog(FATAL, "ProcSleep: Unable to set timer for process wakeup");
#else
    time_interval = DeadlockTimeout * 1000000; /* usecs */
	if (set_alarm(time_interval, B_ONE_SHOT_RELATIVE_ALARM) < 0)
		elog(FATAL, "ProcSleep: Unable to set timer for process wakeup");
#endif

	/* --------------
	 * If someone wakes us between SpinRelease and IpcSemaphoreLock,
	 * IpcSemaphoreLock will not block.  The wakeup is "saved" by
	 * the semaphore implementation.  Note also that if HandleDeadLock
	 * is invoked but does not detect a deadlock, IpcSemaphoreLock()
	 * will continue to wait.  There used to be a loop here, but it
	 * was useless code...
	 *
	 * We pass interruptOK = true, which eliminates a window in which
	 * cancel/die interrupts would be held off undesirably.  This is a
	 * promise that we don't mind losing control to a cancel/die interrupt
	 * here.  We don't, because we have no state-change work to do after
	 * being granted the lock (the grantor did it all).
	 * --------------
	 */
	IpcSemaphoreLock(MyProc->sem.semId, MyProc->sem.semNum, true);

	/* ---------------
	 * Disable the timer, if it's still running
	 * ---------------
	 */
#ifndef __BEOS__
	MemSet(&timeval, 0, sizeof(struct itimerval));
	if (setitimer(ITIMER_REAL, &timeval, &dummy))
		elog(FATAL, "ProcSleep: Unable to disable timer for process wakeup");
#else
    if (set_alarm(B_INFINITE_TIMEOUT, B_PERIODIC_ALARM) < 0)
		elog(FATAL, "ProcSleep: Unable to disable timer for process wakeup");
#endif

	/*
	 * Now there is nothing for LockWaitCancel to do.
	 */
	waitingForLock = false;

	/* ----------------
	 * Re-acquire the locktable's spin lock.
	 *
	 * We could accept a cancel/die interrupt here.  That's OK because
	 * the lock is now registered as being held by this process.
	 * ----------------
	 */
	SpinAcquire(spinlock);

	/*
	 * We don't have to do anything else, because the awaker did all the
	 * necessary update of the lock table and MyProc.
	 */
	return MyProc->errType;
}


/*
 * ProcWakeup -- wake up a process by releasing its private semaphore.
 *
 *	 Also remove the process from the wait queue and set its links invalid.
 *	 RETURN: the next process in the wait queue.
 */
PROC *
ProcWakeup(PROC *proc, int errType)
{
	PROC	   *retProc;

	/* assume that spinlock has been acquired */

	/* Proc should be sleeping ... */
	if (proc->links.prev == INVALID_OFFSET ||
		proc->links.next == INVALID_OFFSET)
		return (PROC *) NULL;

	/* Save next process before we zap the list link */
	retProc = (PROC *) MAKE_PTR(proc->links.next);

	/* Remove process from wait queue */
	SHMQueueDelete(&(proc->links));
	(proc->waitLock->waitProcs.size)--;

	/* Clean up process' state and pass it the ok/fail signal */
	proc->waitLock = NULL;
	proc->waitHolder = NULL;
	proc->errType = errType;

	/* And awaken it */
	IpcSemaphoreUnlock(proc->sem.semId, proc->sem.semNum);

	return retProc;
}

/*
 * ProcLockWakeup -- routine for waking up processes when a lock is
 *		released.
 */
int
ProcLockWakeup(LOCKMETHOD lockmethod, LOCK *lock)
{
	PROC_QUEUE *queue = &(lock->waitProcs);
	PROC	   *proc;
	int			awoken = 0;
	LOCKMODE	last_lockmode = 0;
	int			queue_size = queue->size;

	Assert(queue_size >= 0);

	if (!queue_size)
		return STATUS_NOT_FOUND;

	proc = (PROC *) MAKE_PTR(queue->links.next);

	while (queue_size-- > 0)
	{
		if (proc->waitLockMode == last_lockmode)
		{
			/*
			 * This proc will conflict as the previous one did, don't even
			 * try.
			 */
			goto nextProc;
		}

		/*
		 * Does this proc conflict with locks held by others ?
		 */
		if (LockResolveConflicts(lockmethod,
								 proc->waitLockMode,
								 lock,
								 proc->waitHolder,
								 proc,
								 NULL) != STATUS_OK)
		{
			/* Yes.  Quit if we already awoke at least one process. */
			if (awoken != 0)
				break;
			/* Otherwise, see if any later waiters can be awoken. */
			last_lockmode = proc->waitLockMode;
			goto nextProc;
		}

		/*
		 * OK to wake up this sleeping process.
		 */
		GrantLock(lock, proc->waitHolder, proc->waitLockMode);
		proc = ProcWakeup(proc, NO_ERROR);
		awoken++;

		/*
		 * ProcWakeup removes proc from the lock's waiting process queue
		 * and returns the next proc in chain; don't use proc's next-link,
		 * because it's been cleared.
		 */
		continue;

nextProc:
		proc = (PROC *) MAKE_PTR(proc->links.next);
	}

	Assert(queue->size >= 0);

	if (awoken)
		return STATUS_OK;
	else
	{
		/* Something is still blocking us.	May have deadlocked. */
#ifdef LOCK_DEBUG
		if (lock->tag.lockmethod == USER_LOCKMETHOD ? Trace_userlocks : Trace_locks)
		{
			elog(DEBUG, "ProcLockWakeup: lock(%lx) can't wake up any process",
				 MAKE_OFFSET(lock));
			if (Debug_deadlocks)
				DumpAllLocks();
		}
#endif
		return STATUS_NOT_FOUND;
	}
}

/* --------------------
 * We only get to this routine if we got SIGALRM after DeadlockTimeout
 * while waiting for a lock to be released by some other process.  Look
 * to see if there's a deadlock; if not, just return and continue waiting.
 * If we have a real deadlock, remove ourselves from the lock's wait queue
 * and signal an error to ProcSleep.
 * --------------------
 */
void
HandleDeadLock(SIGNAL_ARGS)
{
	int			save_errno = errno;

	/*
	 * Acquire locktable lock.  Note that the SIGALRM interrupt had better
	 * not be enabled anywhere that this process itself holds the locktable
	 * lock, else this will wait forever.  Also note that this calls
	 * SpinAcquire which creates a critical section, so that this routine
	 * cannot be interrupted by cancel/die interrupts.
	 */
	LockLockTable();

	/* ---------------------
	 * Check to see if we've been awoken by anyone in the interim.
	 *
	 * If we have we can return and resume our transaction -- happy day.
	 * Before we are awoken the process releasing the lock grants it to
	 * us so we know that we don't have to wait anymore.
	 *
	 * We check by looking to see if we've been unlinked from the wait queue.
	 * This is quicker than checking our semaphore's state, since no kernel
	 * call is needed, and it is safe because we hold the locktable lock.
	 * ---------------------
	 */
	if (MyProc->links.prev == INVALID_OFFSET ||
		MyProc->links.next == INVALID_OFFSET)
	{
		UnlockLockTable();
		errno = save_errno;
		return;
	}

#ifdef LOCK_DEBUG
    if (Debug_deadlocks)
        DumpAllLocks();
#endif

	if (!DeadLockCheck(MyProc, MyProc->waitLock))
	{
		/* No deadlock, so keep waiting */
		UnlockLockTable();
		errno = save_errno;
		return;
	}

	/* ------------------------
	 * Oops.  We have a deadlock.
	 *
	 * Get this process out of wait state.
	 * ------------------------
	 */
	RemoveFromWaitQueue(MyProc);

	/* -------------
	 * Set MyProc->errType to STATUS_ERROR so that ProcSleep will
	 * report an error after we return from this signal handler.
	 * -------------
	 */
	MyProc->errType = STATUS_ERROR;

	/* ------------------
	 * Unlock my semaphore so that the interrupted ProcSleep() call can finish.
	 * ------------------
	 */
	IpcSemaphoreUnlock(MyProc->sem.semId, MyProc->sem.semNum);

	/* ------------------
	 * We're done here.  Transaction abort caused by the error that ProcSleep
	 * will raise will cause any other locks we hold to be released, thus
	 * allowing other processes to wake up; we don't need to do that here.
	 * NOTE: an exception is that releasing locks we hold doesn't consider
	 * the possibility of waiters that were blocked behind us on the lock
	 * we just failed to get, and might now be wakable because we're not
	 * in front of them anymore.  However, RemoveFromWaitQueue took care of
	 * waking up any such processes.
	 * ------------------
	 */
	UnlockLockTable();
	errno = save_errno;
}

void
ProcReleaseSpins(PROC *proc)
{
	int			i;

	if (!proc)
		proc = MyProc;

	if (!proc)
		return;
	for (i = 0; i < (int) MAX_SPINS; i++)
	{
		if (proc->sLocks[i])
		{
			Assert(proc->sLocks[i] == 1);
			SpinRelease(i);
		}
	}
	AbortBufferIO();
}

/*****************************************************************************
 *
 *****************************************************************************/

/*
 * ProcGetNewSemIdAndNum -
 *	  scan the free semaphore bitmap and allocate a single semaphore from
 *	  a semaphore set.
 */
static void
ProcGetNewSemIdAndNum(IpcSemaphoreId *semId, int *semNum)
{
	int			i;
	IpcSemaphoreId *procSemIds = ProcGlobal->procSemIds;
	int32	   *freeSemMap = ProcGlobal->freeSemMap;
	int32		fullmask = (1 << PROC_NSEMS_PER_SET) - 1;

	/*
	 * we hold ProcStructLock when entering this routine. We scan through
	 * the bitmap to look for a free semaphore.
	 */

	for (i = 0; i < PROC_SEM_MAP_ENTRIES; i++)
	{
		int			mask = 1;
		int			j;

		if (freeSemMap[i] == fullmask)
			continue;			/* this set is fully allocated */
		if (procSemIds[i] < 0)
			continue;			/* this set hasn't been initialized */

		for (j = 0; j < PROC_NSEMS_PER_SET; j++)
		{
			if ((freeSemMap[i] & mask) == 0)
			{

				/*
				 * a free semaphore found. Mark it as allocated.
				 */
				freeSemMap[i] |= mask;

				*semId = procSemIds[i];
				*semNum = j;
				return;
			}
			mask <<= 1;
		}
	}

	/* if we reach here, all the semaphores are in use. */
	elog(ERROR, "ProcGetNewSemIdAndNum: cannot allocate a free semaphore");
}

/*
 * ProcFreeSem -
 *	  free up our semaphore in the semaphore set.
 */
static void
ProcFreeSem(IpcSemaphoreId semId, int semNum)
{
	int32		mask;
	int			i;

	mask = ~(1 << semNum);

	for (i = 0; i < PROC_SEM_MAP_ENTRIES; i++)
	{
		if (ProcGlobal->procSemIds[i] == semId)
		{
			ProcGlobal->freeSemMap[i] &= mask;
			return;
		}
	}
	fprintf(stderr, "ProcFreeSem: no ProcGlobal entry for semId %d\n", semId);
}

/*
 * ProcFreeAllSemaphores -
 *	  called at shmem_exit time, ie when exiting the postmaster or
 *	  destroying shared state for a failed set of backends.
 *	  Free up all the semaphores allocated to the lmgrs of the backends.
 */
static void
ProcFreeAllSemaphores(void)
{
	int			i;

	for (i = 0; i < PROC_SEM_MAP_ENTRIES; i++)
	{
		if (ProcGlobal->procSemIds[i] >= 0)
			IpcSemaphoreKill(ProcGlobal->procSemIds[i]);
	}
}
