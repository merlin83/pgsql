/*-------------------------------------------------------------------------
 *
 * ipc.c
 *	  POSTGRES inter-process communication definitions.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/ipc/ipc.c,v 1.47 2000-05-16 20:48:48 momjian Exp $
 *
 * NOTES
 *
 *	  Currently, semaphores are used (my understanding anyway) in two
 *	  different ways:
 *		1. as mutexes on machines that don't have test-and-set (eg.
 *		   mips R3000).
 *		2. for putting processes to sleep when waiting on a lock
 *		   and waking them up when the lock is free.
 *	  The number of semaphores in (1) is fixed and those are shared
 *	  among all backends. In (2), there is 1 semaphore per process and those
 *	  are not shared with anyone else.
 *														  -ay 4/95
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>

#include "postgres.h"

#include "storage/ipc.h"
#include "storage/s_lock.h"
/* In Ultrix, sem.h and shm.h must be included AFTER ipc.h */
#include <sys/sem.h>
#include <sys/shm.h>
#include "utils/memutils.h"
#include "libpq/libpq.h"
#include "utils/trace.h"

#if defined(solaris_sparc)
#include <sys/ipc.h>
#endif

/*
 * This flag is set during proc_exit() to change elog()'s behavior,
 * so that an elog() from an on_proc_exit routine cannot get us out
 * of the exit procedure.  We do NOT want to go back to the idle loop...
 */
bool		proc_exit_inprogress = false;

static int	UsePrivateMemory = 0;

static void IpcMemoryDetach(int status, char *shmaddr);
static void IpcConfigTip(void);

/* ----------------------------------------------------------------
 *						exit() handling stuff
 * ----------------------------------------------------------------
 */

#define MAX_ON_EXITS 20

static struct ONEXIT
{
	void		(*function) ();
	caddr_t		arg;
}			on_proc_exit_list[MAX_ON_EXITS], on_shmem_exit_list[MAX_ON_EXITS];

static int	on_proc_exit_index,
			on_shmem_exit_index;

typedef struct _PrivateMemStruct
{
	int			id;
	char	   *memptr;
} PrivateMem;

static PrivateMem IpcPrivateMem[16];


static int
PrivateMemoryCreate(IpcMemoryKey memKey,
					uint32 size)
{
	static int	memid = 0;

	UsePrivateMemory = 1;

	IpcPrivateMem[memid].id = memid;
	IpcPrivateMem[memid].memptr = malloc(size);
	if (IpcPrivateMem[memid].memptr == NULL)
		elog(ERROR, "PrivateMemoryCreate: not enough memory to malloc");
	MemSet(IpcPrivateMem[memid].memptr, 0, size);		/* XXX PURIFY */

	return memid++;
}

static char *
PrivateMemoryAttach(IpcMemoryId memid)
{
	return IpcPrivateMem[memid].memptr;
}


/* ----------------------------------------------------------------
 *		proc_exit
 *
 *		this function calls all the callbacks registered
 *		for it (to free resources) and then calls exit.
 *		This should be the only function to call exit().
 *		-cim 2/6/90
 * ----------------------------------------------------------------
 */
void
proc_exit(int code)
{

	/*
	 * Once we set this flag, we are committed to exit.  Any elog() will
	 * NOT send control back to the main loop, but right back here.
	 */
	proc_exit_inprogress = true;

	TPRINTF(TRACE_VERBOSE, "proc_exit(%d)", code);

	/* do our shared memory exits first */
	shmem_exit(code);

	/* ----------------
	 *	call all the callbacks registered before calling exit().
	 *
	 *	Note that since we decrement on_proc_exit_index each time,
	 *	if a callback calls elog(ERROR) or elog(FATAL) then it won't
	 *	be invoked again when control comes back here (nor will the
	 *	previously-completed callbacks).  So, an infinite loop
	 *	should not be possible.
	 * ----------------
	 */
	while (--on_proc_exit_index >= 0)
		(*on_proc_exit_list[on_proc_exit_index].function) (code,
							  on_proc_exit_list[on_proc_exit_index].arg);

	TPRINTF(TRACE_VERBOSE, "exit(%d)", code);
	exit(code);
}

/* ------------------
 * Run all of the on_shmem_exit routines but don't exit in the end.
 * This is used by the postmaster to re-initialize shared memory and
 * semaphores after a backend dies horribly
 * ------------------
 */
void
shmem_exit(int code)
{
	TPRINTF(TRACE_VERBOSE, "shmem_exit(%d)", code);

	/* ----------------
	 *	call all the registered callbacks.
	 *
	 *	As with proc_exit(), we remove each callback from the list
	 *	before calling it, to avoid infinite loop in case of error.
	 * ----------------
	 */
	while (--on_shmem_exit_index >= 0)
		(*on_shmem_exit_list[on_shmem_exit_index].function) (code,
							on_shmem_exit_list[on_shmem_exit_index].arg);

	on_shmem_exit_index = 0;
}

/* ----------------------------------------------------------------
 *		on_proc_exit
 *
 *		this function adds a callback function to the list of
 *		functions invoked by proc_exit().	-cim 2/6/90
 * ----------------------------------------------------------------
 */
int
			on_proc_exit(void (*function) (), caddr_t arg)
{
	if (on_proc_exit_index >= MAX_ON_EXITS)
		return -1;

	on_proc_exit_list[on_proc_exit_index].function = function;
	on_proc_exit_list[on_proc_exit_index].arg = arg;

	++on_proc_exit_index;

	return 0;
}

/* ----------------------------------------------------------------
 *		on_shmem_exit
 *
 *		this function adds a callback function to the list of
 *		functions invoked by shmem_exit().	-cim 2/6/90
 * ----------------------------------------------------------------
 */
int
			on_shmem_exit(void (*function) (), caddr_t arg)
{
	if (on_shmem_exit_index >= MAX_ON_EXITS)
		return -1;

	on_shmem_exit_list[on_shmem_exit_index].function = function;
	on_shmem_exit_list[on_shmem_exit_index].arg = arg;

	++on_shmem_exit_index;

	return 0;
}

/* ----------------------------------------------------------------
 *		on_exit_reset
 *
 *		this function clears all proc_exit() registered functions.
 * ----------------------------------------------------------------
 */
void
on_exit_reset(void)
{
	on_shmem_exit_index = 0;
	on_proc_exit_index = 0;
}

/****************************************************************************/
/*	 IPCPrivateSemaphoreKill(status, semId)									*/
/*																			*/
/****************************************************************************/
static void
IPCPrivateSemaphoreKill(int status,
						int semId)		/* caddr_t */
{
	union semun semun;
	semun.val = 0;		/* unused */

	semctl(semId, 0, IPC_RMID, semun);
}


/****************************************************************************/
/*	 IPCPrivateMemoryKill(status, shmId)									*/
/*																			*/
/****************************************************************************/
static void
IPCPrivateMemoryKill(int status,
					 int shmId) /* caddr_t */
{
	if (UsePrivateMemory)
	{
		/* free ( IpcPrivateMem[shmId].memptr ); */
	}
	else
	{
		if (shmctl(shmId, IPC_RMID, (struct shmid_ds *) NULL) < 0)
		{
			elog(NOTICE, "IPCPrivateMemoryKill: shmctl(%d, %d, 0) failed: %m",
				 shmId, IPC_RMID);
		}
	}
}

/*
 * Note:
 * XXX	This should be split into two different calls.	One should
 * XXX	be used to create a semaphore set.	The other to "attach" a
 * XXX	existing set.  It should be an error for the semaphore set
 * XXX	to to already exist or for it not to, respectively.
 *
 *		Currently, the semaphore sets are "attached" and an error
 *		is detected only when a later shared memory attach fails.
 */

IpcSemaphoreId
IpcSemaphoreCreate(IpcSemaphoreKey semKey,
				   int semNum,
				   int permission,
				   int semStartValue,
				   int removeOnExit)
{
	int			i;
	int			errStatus;
	int			semId;
	u_short		array[IPC_NMAXSEM];
	union semun semun;

	/* check arguments	*/
	if (semNum > IPC_NMAXSEM || semNum <= 0)
		return (-1);

	semId = semget(semKey, 0, 0);

	if (semId == -1)
	{
#ifdef DEBUG_IPC
		EPRINTF("calling semget with %d, %d , %d\n",
				semKey,
				semNum,
				IPC_CREAT | permission);
#endif
		semId = semget(semKey, semNum, IPC_CREAT | permission);

		if (semId < 0)
		{
			EPRINTF("IpcSemaphoreCreate: semget failed (%s) "
					"key=%d, num=%d, permission=%o",
					strerror(errno), semKey, semNum, permission);
			IpcConfigTip();
			return (-1);
		}
		for (i = 0; i < semNum; i++)
			array[i] = semStartValue;
		semun.array = array;
		errStatus = semctl(semId, 0, SETALL, semun);
		if (errStatus == -1)
		{
			EPRINTF("IpcSemaphoreCreate: semctl failed (%s) id=%d",
					strerror(errno), semId);
			semctl(semId, 0, IPC_RMID, semun);
			IpcConfigTip();
			return (-1);
		}

		if (removeOnExit)
			on_shmem_exit(IPCPrivateSemaphoreKill, (caddr_t) semId);
	}

#ifdef DEBUG_IPC
	EPRINTF("\nIpcSemaphoreCreate, returns %d\n", semId);
	fflush(stdout);
	fflush(stderr);
#endif
	return semId;
}


/****************************************************************************/
/*	 IpcSemaphoreSet()			- sets the initial value of the semaphore	*/
/*																			*/
/*		note: the xxx_return variables are only used for debugging.			*/
/****************************************************************************/
#ifdef NOT_USED
static int	IpcSemaphoreSet_return;

void
IpcSemaphoreSet(int semId, int semno, int value)
{
	int			errStatus;
	union semun semun;

	semun.val = value;
	errStatus = semctl(semId, semno, SETVAL, semun);
	IpcSemaphoreSet_return = errStatus;

	if (errStatus == -1)
	{
		EPRINTF("IpcSemaphoreSet: semctl failed (%s) id=%d",
				strerror(errno), semId);
	}
}

#endif

/****************************************************************************/
/*	 IpcSemaphoreKill(key)		- removes a semaphore						*/
/*																			*/
/****************************************************************************/
void
IpcSemaphoreKill(IpcSemaphoreKey key)
{
	int			semId;
	union semun semun;
	semun.val = 0;		/* unused */

	/* kill semaphore if existent */

	semId = semget(key, 0, 0);
	if (semId != -1)
		semctl(semId, 0, IPC_RMID, semun);
}

/****************************************************************************/
/*	 IpcSemaphoreLock(semId, sem, lock) - locks a semaphore					*/
/*																			*/
/*		note: the xxx_return variables are only used for debugging.			*/
/****************************************************************************/
static int	IpcSemaphoreLock_return;

void
IpcSemaphoreLock(IpcSemaphoreId semId, int sem, int lock)
{
	extern int	errno;
	int			errStatus;
	struct sembuf sops;

	sops.sem_op = lock;
	sops.sem_flg = 0;
	sops.sem_num = sem;

	/* ----------------
	 *	Note: if errStatus is -1 and errno == EINTR then it means we
	 *		  returned from the operation prematurely because we were
	 *		  sent a signal.  So we try and lock the semaphore again.
	 *		  I am not certain this is correct, but the semantics aren't
	 *		  clear it fixes problems with parallel abort synchronization,
	 *		  namely that after processing an abort signal, the semaphore
	 *		  call returns with -1 (and errno == EINTR) before it should.
	 *		  -cim 3/28/90
	 * ----------------
	 */
	do
	{
		errStatus = semop(semId, &sops, 1);
	} while (errStatus == -1 && errno == EINTR);

	IpcSemaphoreLock_return = errStatus;

	if (errStatus == -1)
	{
		EPRINTF("IpcSemaphoreLock: semop failed (%s) id=%d",
				strerror(errno), semId);
		proc_exit(255);
	}
}

/****************************************************************************/
/*	 IpcSemaphoreUnlock(semId, sem, lock)		- unlocks a semaphore		*/
/*																			*/
/*		note: the xxx_return variables are only used for debugging.			*/
/****************************************************************************/
static int	IpcSemaphoreUnlock_return;

void
IpcSemaphoreUnlock(IpcSemaphoreId semId, int sem, int lock)
{
	extern int	errno;
	int			errStatus;
	struct sembuf sops;

	sops.sem_op = -lock;
	sops.sem_flg = 0;
	sops.sem_num = sem;


	/* ----------------
	 *	Note: if errStatus is -1 and errno == EINTR then it means we
	 *		  returned from the operation prematurely because we were
	 *		  sent a signal.  So we try and lock the semaphore again.
	 *		  I am not certain this is correct, but the semantics aren't
	 *		  clear it fixes problems with parallel abort synchronization,
	 *		  namely that after processing an abort signal, the semaphore
	 *		  call returns with -1 (and errno == EINTR) before it should.
	 *		  -cim 3/28/90
	 * ----------------
	 */
	do
	{
		errStatus = semop(semId, &sops, 1);
	} while (errStatus == -1 && errno == EINTR);

	IpcSemaphoreUnlock_return = errStatus;

	if (errStatus == -1)
	{
		EPRINTF("IpcSemaphoreUnlock: semop failed (%s) id=%d",
				strerror(errno), semId);
		proc_exit(255);
	}
}

int
IpcSemaphoreGetCount(IpcSemaphoreId semId, int sem)
{
	int			semncnt;
	union semun dummy;			/* for Solaris */
	dummy.val = 0;		/* unused */

	semncnt = semctl(semId, sem, GETNCNT, dummy);
	return semncnt;
}

int
IpcSemaphoreGetValue(IpcSemaphoreId semId, int sem)
{
	int			semval;
	union semun dummy;			/* for Solaris */
	dummy.val = 0;		/* unused */

	semval = semctl(semId, sem, GETVAL, dummy);
	return semval;
}

/****************************************************************************/
/*	 IpcMemoryCreate(memKey)												*/
/*																			*/
/*	  - returns the memory identifier, if creation succeeds					*/
/*		returns IpcMemCreationFailed, if failure							*/
/****************************************************************************/

IpcMemoryId
IpcMemoryCreate(IpcMemoryKey memKey, uint32 size, int permission)
{
	IpcMemoryId shmid;

	if (memKey == PrivateIPCKey)
	{
		/* private */
		shmid = PrivateMemoryCreate(memKey, size);
	}
	else
		shmid = shmget(memKey, size, IPC_CREAT | permission);

	if (shmid < 0)
	{
		EPRINTF("IpcMemoryCreate: shmget failed (%s) "
				"key=%d, size=%d, permission=%o",
				strerror(errno), memKey, size, permission);
		IpcConfigTip();
		return IpcMemCreationFailed;
	}

	/* if (memKey == PrivateIPCKey) */
	on_shmem_exit(IPCPrivateMemoryKill, (caddr_t) shmid);

	return shmid;
}

/****************************************************************************/
/*	IpcMemoryIdGet(memKey, size)	returns the shared memory Id			*/
/*									or IpcMemIdGetFailed					*/
/****************************************************************************/
IpcMemoryId
IpcMemoryIdGet(IpcMemoryKey memKey, uint32 size)
{
	IpcMemoryId shmid;

	shmid = shmget(memKey, size, 0);

	if (shmid < 0)
	{
		EPRINTF("IpcMemoryIdGet: shmget failed (%s) "
				"key=%d, size=%d, permission=%o",
				strerror(errno), memKey, size, 0);
		return IpcMemIdGetFailed;
	}

	return shmid;
}

/****************************************************************************/
/*	IpcMemoryDetach(status, shmaddr)	removes a shared memory segment		*/
/*										from a backend address space		*/
/*	(only called by backends running under the postmaster)					*/
/****************************************************************************/
static void
IpcMemoryDetach(int status, char *shmaddr)
{
	if (shmdt(shmaddr) < 0)
		elog(NOTICE, "IpcMemoryDetach: shmdt(0x%p): %m", shmaddr);
}

/****************************************************************************/
/*	IpcMemoryAttach(memId)	  returns the adress of shared memory			*/
/*							  or IpcMemAttachFailed							*/
/*																			*/
/* CALL IT:  addr = (struct <MemoryStructure> *) IpcMemoryAttach(memId);	*/
/*																			*/
/****************************************************************************/
char *
IpcMemoryAttach(IpcMemoryId memId)
{
	char	   *memAddress;

	if (UsePrivateMemory)
		memAddress = (char *) PrivateMemoryAttach(memId);
	else
		memAddress = (char *) shmat(memId, 0, 0);

	/* if ( *memAddress == -1) { XXX ??? */
	if (memAddress == (char *) -1)
	{
		EPRINTF("IpcMemoryAttach: shmat failed (%s) id=%d",
				strerror(errno), memId);
		return IpcMemAttachFailed;
	}

	if (!UsePrivateMemory)
		on_shmem_exit(IpcMemoryDetach, (caddr_t) memAddress);

	return (char *) memAddress;
}


/****************************************************************************/
/*	IpcMemoryKill(memKey)				removes a shared memory segment		*/
/*	(only called by the postmaster and standalone backends)					*/
/****************************************************************************/
void
IpcMemoryKill(IpcMemoryKey memKey)
{
	IpcMemoryId shmid;

	if (!UsePrivateMemory && (shmid = shmget(memKey, 0, 0)) >= 0)
	{
		if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) NULL) < 0)
		{
			elog(NOTICE, "IpcMemoryKill: shmctl(%d, %d, 0) failed: %m",
				 shmid, IPC_RMID);
		}
	}
}

#ifdef HAS_TEST_AND_SET
/* ------------------
 *	use hardware locks to replace semaphores for sequent machines
 *	to avoid costs of swapping processes and to provide unlimited
 *	supply of locks.
 * ------------------
 */

/* used in spin.c */
SLock	   *SLockArray = NULL;

static SLock **FreeSLockPP;
static int *UnusedSLockIP;
static slock_t *SLockMemoryLock;
static IpcMemoryId SLockMemoryId = -1;

struct ipcdummy
{								/* to get alignment/size right */
	SLock	   *free;
	int			unused;
	slock_t		memlock;
	SLock		slocks[MAX_SPINS + 1];
};

#define SLOCKMEMORYSIZE		sizeof(struct ipcdummy)

void
CreateAndInitSLockMemory(IPCKey key)
{
	int			id;
	SLock	   *slckP;

	SLockMemoryId = IpcMemoryCreate(key,
									SLOCKMEMORYSIZE,
									0700);
	AttachSLockMemory(key);
	*FreeSLockPP = NULL;
	*UnusedSLockIP = (int) FIRSTFREELOCKID;
	for (id = 0; id < (int) FIRSTFREELOCKID; id++)
	{
		slckP = &(SLockArray[id]);
		S_INIT_LOCK(&(slckP->locklock));
		slckP->flag = NOLOCK;
		slckP->nshlocks = 0;
		S_INIT_LOCK(&(slckP->shlock));
		S_INIT_LOCK(&(slckP->exlock));
		S_INIT_LOCK(&(slckP->comlock));
		slckP->next = NULL;
	}
	return;
}

void
AttachSLockMemory(IPCKey key)
{
	struct ipcdummy *slockM;

	if (SLockMemoryId == -1)
		SLockMemoryId = IpcMemoryIdGet(key, SLOCKMEMORYSIZE);
	if (SLockMemoryId == -1)
		elog(FATAL, "SLockMemory not in shared memory");
	slockM = (struct ipcdummy *) IpcMemoryAttach(SLockMemoryId);
	if (slockM == IpcMemAttachFailed)
		elog(FATAL, "AttachSLockMemory: could not attach segment");
	FreeSLockPP = (SLock **) &(slockM->free);
	UnusedSLockIP = (int *) &(slockM->unused);
	SLockMemoryLock = (slock_t *) &(slockM->memlock);
	S_INIT_LOCK(SLockMemoryLock);
	SLockArray = (SLock *) &(slockM->slocks[0]);
	return;
}

#ifdef NOT_USED
bool
LockIsFree(int lockid)
{
	return SLockArray[lockid].flag == NOLOCK;
}

#endif

#endif	 /* HAS_TEST_AND_SET */

static void
IpcConfigTip(void)
{
	fprintf(stderr, "This type of error is usually caused by an improper\n");
	fprintf(stderr, "shared memory or System V IPC semaphore configuration.\n");
	fprintf(stderr, "For more information, see the FAQ and platform-specific\n");
	fprintf(stderr, "FAQ's in the source directory pgsql/doc or on our\n");
	fprintf(stderr, "web site at http://www.postgresql.org.\n");
}
