/*-------------------------------------------------------------------------
 *
 * ipci.c--
 *    POSTGRES inter-process communication initialization code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/ipc/ipci.c,v 1.4 1996-11-08 05:58:33 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include "postgres.h"

#include "storage/ipc.h"
#include "storage/multilev.h"
#include "storage/sinval.h"
#include "storage/bufmgr.h"
#include "storage/proc.h"
#include "storage/smgr.h"
#include "storage/lock.h"
#include "miscadmin.h"		/* for DebugLvl */

/*
 * SystemPortAddressCreateMemoryKey --
 *	Returns a memory key given a port address.
 */
IPCKey
SystemPortAddressCreateIPCKey(SystemPortAddress address)
{
    Assert(address < 32768);	/* XXX */
    
    return (SystemPortAddressGetIPCKey(address));
}

/*
 * CreateSharedMemoryAndSemaphores --
 *	Creates and initializes shared memory and semaphores.
 */
/**************************************************
  
  CreateSharedMemoryAndSemaphores
  is called exactly *ONCE* by the postmaster.
  It is *NEVER* called by the postgres backend
  
  0) destroy any existing semaphores for both buffer
  and lock managers.
  1) create the appropriate *SHARED* memory segments
  for the two resource managers.
  
  **************************************************/

void
CreateSharedMemoryAndSemaphores(IPCKey key)
{
    int		size;
    
#ifdef HAS_TEST_AND_SET
    /* ---------------
     *  create shared memory for slocks
     * --------------
     */
    CreateAndInitSLockMemory(IPCKeyGetSLockSharedMemoryKey(key));
#endif
    /* ----------------
     *	kill and create the buffer manager buffer pool (and semaphore)
     * ----------------
     */
    CreateSpinlocks(IPCKeyGetSpinLockSemaphoreKey(key));
    size = BufferShmemSize() + LockShmemSize();
    
#ifdef MAIN_MEMORY
    size += MMShmemSize();
#endif /* MAIN_MEMORY */
    
    if (DebugLvl > 1) {
	fprintf(stderr, "binding ShmemCreate(key=%x, size=%d)\n",
		IPCKeyGetBufferMemoryKey(key), size);
    }
    ShmemCreate(IPCKeyGetBufferMemoryKey(key), size);
    ShmemBindingTabReset();
    InitShmem(key, size);
    InitBufferPool(key);
    
    /* ----------------
     *	do the lock table stuff
     * ----------------
     */
    InitLocks();
    InitMultiLevelLockm();
    if (InitMultiLevelLockm() == INVALID_TABLEID)
	elog(FATAL, "Couldn't create the lock table");

    /* ----------------
     *  do process table stuff
     * ----------------
     */
    InitProcGlobal(key);
    on_exitpg(ProcFreeAllSemaphores, 0);
    
    CreateSharedInvalidationState(key);
}


/*
 * AttachSharedMemoryAndSemaphores --
 *	Attachs existant shared memory and semaphores.
 */
void
AttachSharedMemoryAndSemaphores(IPCKey key)
{
    int size;
    
    /* ----------------
     *	create rather than attach if using private key
     * ----------------
     */
    if (key == PrivateIPCKey) {
	CreateSharedMemoryAndSemaphores(key);
	return;
    }
    
#ifdef HAS_TEST_AND_SET
    /* ----------------
     *  attach the slock shared memory
     * ----------------
     */
    AttachSLockMemory(IPCKeyGetSLockSharedMemoryKey(key));
#endif
    /* ----------------
     *	attach the buffer manager buffer pool (and semaphore)
     * ----------------
     */
    size = BufferShmemSize() + LockShmemSize();
    InitShmem(key, size);
    InitBufferPool(key);
    
    /* ----------------
     *	initialize lock table stuff
     * ----------------
     */
    InitLocks();
    if (InitMultiLevelLockm() == INVALID_TABLEID)
	elog(FATAL, "Couldn't attach to the lock table");
    
    AttachSharedInvalidationState(key);
}
