/*-------------------------------------------------------------------------
 *
 * shmem.c
 *	  create shared memory and initialize shared memory data structures.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/ipc/shmem.c,v 1.49 2000-02-26 05:25:55 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 * POSTGRES processes share one or more regions of shared memory.
 * The shared memory is created by a postmaster and is inherited
 * by each backends via fork().  The routines in this file are used for
 * allocating and binding to shared memory data structures.
 *
 * NOTES:
 *		(a) There are three kinds of shared memory data structures
 *	available to POSTGRES: fixed-size structures, queues and hash
 *	tables.  Fixed-size structures contain things like global variables
 *	for a module and should never be allocated after the process
 *	initialization phase.  Hash tables have a fixed maximum size, but
 *	their actual size can vary dynamically.  When entries are added
 *	to the table, more space is allocated.	Queues link data structures
 *	that have been allocated either as fixed size structures or as hash
 *	buckets.  Each shared data structure has a string name to identify
 *	it (assigned in the module that declares it).
 *
 *		(b) During initialization, each module looks for its
 *	shared data structures in a hash table called the "Shmem Index".
 *	If the data structure is not present, the caller can allocate
 *	a new one and initialize it.  If the data structure is present,
 *	the caller "attaches" to the structure by initializing a pointer
 *	in the local address space.
 *		The shmem index has two purposes: first, it gives us
 *	a simple model of how the world looks when a backend process
 *	initializes.  If something is present in the shmem index,
 *	it is initialized.	If it is not, it is uninitialized.	Second,
 *	the shmem index allows us to allocate shared memory on demand
 *	instead of trying to preallocate structures and hard-wire the
 *	sizes and locations in header files.  If you are using a lot
 *	of shared memory in a lot of different places (and changing
 *	things during development), this is important.
 *
 *		(c) memory allocation model: shared memory can never be
 *	freed, once allocated.	 Each hash table has its own free list,
 *	so hash buckets can be reused when an item is deleted.	However,
 *	if one hash table grows very large and then shrinks, its space
 *	cannot be redistributed to other tables.  We could build a simple
 *	hash bucket garbage collector if need be.  Right now, it seems
 *	unnecessary.
 *
 *		See InitSem() in sem.c for an example of how to use the
 *	shmem index.
 *
 */

#include "postgres.h"
#include "storage/proc.h"
#include "utils/tqual.h"

/* shared memory global variables */

unsigned long ShmemBase = 0;	/* start and end address of shared memory */
static unsigned long ShmemEnd = 0;
static unsigned long ShmemSize = 0;		/* current size (and default) */

extern VariableCache ShmemVariableCache;		/* varsup.c */

SPINLOCK	ShmemLock;			/* lock for shared memory allocation */

SPINLOCK	ShmemIndexLock;		/* lock for shmem index access */

static unsigned long *ShmemFreeStart = NULL;	/* pointer to the OFFSET
												 * of first free shared
												 * memory */
static unsigned long *ShmemIndexOffset = NULL;	/* start of the shmem
												 * index table (for
												 * bootstrap) */
static int	ShmemBootstrap = FALSE;		/* flag becomes true when shared
										 * mem is created by POSTMASTER */

static HTAB *ShmemIndex = NULL;

/* ---------------------
 * ShmemIndexReset() - Resets the shmem index to NULL....
 * useful when the postmaster destroys existing shared memory
 * and creates all new segments after a backend crash.
 * ----------------------
 */
void
ShmemIndexReset(void)
{
	ShmemIndex = (HTAB *) NULL;
}

/*
 *	CreateSharedRegion()
 *
 *	This routine is called once by the postmaster to
 *	initialize the shared buffer pool.	Assume there is
 *	only one postmaster so no synchronization is necessary
 *	until after this routine completes successfully.
 *
 * key is a unique identifier for the shmem region.
 * size is the size of the region.
 */
static IpcMemoryId ShmemId;

void
ShmemCreate(unsigned int key, unsigned int size)
{
	if (size)
		ShmemSize = size;
	/* create shared mem region */
	if ((ShmemId = IpcMemoryCreate(key, ShmemSize, IPCProtection))
		== IpcMemCreationFailed)
	{
		elog(FATAL, "ShmemCreate: cannot create region");
		exit(1);
	}

	/*
	 * ShmemBootstrap is true if shared memory has been created, but not
	 * yet initialized.  Only the postmaster/creator-of-all-things should
	 * have this flag set.
	 */
	ShmemBootstrap = TRUE;
}

/*
 *	InitShmem() -- map region into process address space
 *		and initialize shared data structures.
 *
 */
int
InitShmem(unsigned int key, unsigned int size)
{
	Pointer		sharedRegion;
	unsigned long currFreeSpace;

	HASHCTL		info;
	int			hash_flags;
	ShmemIndexEnt *result,
				item;
	bool		found;
	IpcMemoryId shmid;

	/* if zero key, use default memory size */
	if (size)
		ShmemSize = size;

	/* default key is 0 */

	/* attach to shared memory region (SysV or BSD OS specific) */
	if (ShmemBootstrap && key == PrivateIPCKey)
		/* if we are running backend alone */
		shmid = ShmemId;
	else
		shmid = IpcMemoryIdGet(IPCKeyGetBufferMemoryKey(key), ShmemSize);
	sharedRegion = IpcMemoryAttach(shmid);
	if (sharedRegion == NULL)
	{
		elog(FATAL, "AttachSharedRegion: couldn't attach to shmem\n");
		return FALSE;
	}

	/* get pointers to the dimensions of shared memory */
	ShmemBase = (unsigned long) sharedRegion;
	ShmemEnd = (unsigned long) sharedRegion + ShmemSize;
	currFreeSpace = 0;

	/* First long in shared memory is the count of available space */
	ShmemFreeStart = (unsigned long *) ShmemBase;
	/* next is a shmem pointer to the shmem index */
	ShmemIndexOffset = ShmemFreeStart + 1;
	/* next is ShmemVariableCache */
	ShmemVariableCache = (VariableCache) (ShmemIndexOffset + 1);

	currFreeSpace += sizeof(ShmemFreeStart) + sizeof(ShmemIndexOffset) +
		LONGALIGN(sizeof(VariableCacheData));

	/*
	 * bootstrap initialize spin locks so we can start to use the
	 * allocator and shmem index.
	 */
	InitSpinLocks();

	/*
	 * We have just allocated additional space for two spinlocks. Now
	 * setup the global free space count
	 */
	if (ShmemBootstrap)
	{
		*ShmemFreeStart = currFreeSpace;
		memset(ShmemVariableCache, 0, sizeof(*ShmemVariableCache));
	}

	/* if ShmemFreeStart is NULL, then the allocator won't work */
	Assert(*ShmemFreeStart);

	/* create OR attach to the shared memory shmem index */
	info.keysize = SHMEM_INDEX_KEYSIZE;
	info.datasize = SHMEM_INDEX_DATASIZE;
	hash_flags = HASH_ELEM;

	/* This will acquire the shmem index lock, but not release it. */
	ShmemIndex = ShmemInitHash("ShmemIndex",
							   SHMEM_INDEX_SIZE, SHMEM_INDEX_SIZE,
							   &info, hash_flags);

	if (!ShmemIndex)
	{
		elog(FATAL, "InitShmem: couldn't initialize Shmem Index");
		return FALSE;
	}

	/*
	 * Now, check the shmem index for an entry to the shmem index.	If
	 * there is an entry there, someone else created the table. Otherwise,
	 * we did and we have to initialize it.
	 */
	MemSet(item.key, 0, SHMEM_INDEX_KEYSIZE);
	strncpy(item.key, "ShmemIndex", SHMEM_INDEX_KEYSIZE);

	result = (ShmemIndexEnt *)
		hash_search(ShmemIndex, (char *) &item, HASH_ENTER, &found);


	if (!result)
	{
		elog(FATAL, "InitShmem: corrupted shmem index");
		return FALSE;
	}

	if (!found)
	{

		/*
		 * bootstrapping shmem: we have to initialize the shmem index now.
		 */

		Assert(ShmemBootstrap);
		result->location = MAKE_OFFSET(ShmemIndex->hctl);
		*ShmemIndexOffset = result->location;
		result->size = SHMEM_INDEX_SIZE;

		ShmemBootstrap = FALSE;

	}
	else
		Assert(!ShmemBootstrap);
	/* now release the lock acquired in ShmemHashInit */
	SpinRelease(ShmemIndexLock);

	Assert(result->location == MAKE_OFFSET(ShmemIndex->hctl));

	return TRUE;
}

/*
 * ShmemAlloc -- allocate word-aligned byte string from
 *		shared memory
 *
 * Assumes ShmemLock and ShmemFreeStart are initialized.
 * Returns: real pointer to memory or NULL if we are out
 *		of space.  Has to return a real pointer in order
 *		to be compatable with malloc().
 */
long *
ShmemAlloc(unsigned long size)
{
	unsigned long tmpFree;
	long	   *newSpace;

	/*
	 * ensure space is word aligned.
	 *
	 * Word-alignment is not good enough. We have to be more conservative:
	 * doubles need 8-byte alignment. (We probably only need this on RISC
	 * platforms but this is not a big waste of space.) - ay 12/94
	 */
	if (size % sizeof(double))
		size += sizeof(double) - (size % sizeof(double));

	Assert(*ShmemFreeStart);

	SpinAcquire(ShmemLock);

	tmpFree = *ShmemFreeStart + size;
	if (tmpFree <= ShmemSize)
	{
		newSpace = (long *) MAKE_PTR(*ShmemFreeStart);
		*ShmemFreeStart += size;
	}
	else
		newSpace = NULL;

	SpinRelease(ShmemLock);

	if (!newSpace)
		elog(NOTICE, "ShmemAlloc: out of memory ");
	return newSpace;
}

/*
 * ShmemIsValid -- test if an offset refers to valid shared memory
 *
 * Returns TRUE if the pointer is valid.
 */
int
ShmemIsValid(unsigned long addr)
{
	return (addr < ShmemEnd) && (addr >= ShmemBase);
}

/*
 * ShmemInitHash -- Create/Attach to and initialize
 *		shared memory hash table.
 *
 * Notes:
 *
 * assume caller is doing some kind of synchronization
 * so that two people dont try to create/initialize the
 * table at once.  Use SpinAlloc() to create a spinlock
 * for the structure before creating the structure itself.
 */
HTAB *
ShmemInitHash(char *name,		/* table string name for shmem index */
			  long init_size,	/* initial table size */
			  long max_size,	/* max size of the table */
			  HASHCTL *infoP,	/* info about key and bucket size */
			  int hash_flags)	/* info about infoP */
{
	bool		found;
	long	   *location;

	/*
	 * Hash tables allocated in shared memory have a fixed directory;
	 * it can't grow or other backends wouldn't be able to find it.
	 * So, make sure we make it big enough to start with.
	 *
	 * The segbase is for calculating pointer values. The shared memory
	 * allocator must be specified too.
	 */
	infoP->dsize = infoP->max_dsize = hash_select_dirsize(max_size);
	infoP->segbase = (long *) ShmemBase;
	infoP->alloc = ShmemAlloc;
	hash_flags |= HASH_SHARED_MEM | HASH_DIRSIZE;

	/* look it up in the shmem index */
	location = ShmemInitStruct(name,
						 sizeof(HHDR) + infoP->dsize * sizeof(SEG_OFFSET),
							   &found);

	/*
	 * shmem index is corrupted.	Let someone else give the error
	 * message since they have more information
	 */
	if (location == NULL)
		return 0;

	/*
	 * it already exists, attach to it rather than allocate and initialize
	 * new space
	 */
	if (found)
		hash_flags |= HASH_ATTACH;

	/* Now provide the header and directory pointers */
	infoP->hctl = (long *) location;
	infoP->dir = (long *) (((char *) location) + sizeof(HHDR));

	return hash_create(init_size, infoP, hash_flags);
}

/*
 * ShmemPIDLookup -- lookup process data structure using process id
 *
 * Returns: TRUE if no error.  locationPtr is initialized if PID is
 *		found in the shmem index.
 *
 * NOTES:
 *		only information about success or failure is the value of
 *		locationPtr.
 */
bool
ShmemPIDLookup(int pid, SHMEM_OFFSET *locationPtr)
{
	ShmemIndexEnt *result,
				item;
	bool		found;

	Assert(ShmemIndex);
	MemSet(item.key, 0, SHMEM_INDEX_KEYSIZE);
	sprintf(item.key, "PID %d", pid);

	SpinAcquire(ShmemIndexLock);
	result = (ShmemIndexEnt *)
		hash_search(ShmemIndex, (char *) &item, HASH_ENTER, &found);

	if (!result)
	{

		SpinRelease(ShmemIndexLock);
		elog(ERROR, "ShmemInitPID: ShmemIndex corrupted");
		return FALSE;

	}

	if (found)
		*locationPtr = result->location;
	else
		result->location = *locationPtr;

	SpinRelease(ShmemIndexLock);
	return TRUE;
}

/*
 * ShmemPIDDestroy -- destroy shmem index entry for process
 *		using process id
 *
 * Returns: offset of the process struct in shared memory or
 *		INVALID_OFFSET if not found.
 *
 * Side Effect: removes the entry from the shmem index
 */
SHMEM_OFFSET
ShmemPIDDestroy(int pid)
{
	ShmemIndexEnt *result,
				item;
	bool		found;
	SHMEM_OFFSET location = 0;

	Assert(ShmemIndex);

	MemSet(item.key, 0, SHMEM_INDEX_KEYSIZE);
	sprintf(item.key, "PID %d", pid);

	SpinAcquire(ShmemIndexLock);
	result = (ShmemIndexEnt *)
		hash_search(ShmemIndex, (char *) &item, HASH_REMOVE, &found);

	if (found)
		location = result->location;
	SpinRelease(ShmemIndexLock);

	if (!result)
	{

		elog(ERROR, "ShmemPIDDestroy: PID table corrupted");
		return INVALID_OFFSET;

	}

	if (found)
		return location;
	else
		return INVALID_OFFSET;
}

/*
 * ShmemInitStruct -- Create/attach to a structure in shared
 *		memory.
 *
 *	This is called during initialization to find or allocate
 *		a data structure in shared memory.	If no other processes
 *		have created the structure, this routine allocates space
 *		for it.  If it exists already, a pointer to the existing
 *		table is returned.
 *
 *	Returns: real pointer to the object.  FoundPtr is TRUE if
 *		the object is already in the shmem index (hence, already
 *		initialized).
 */
long *
ShmemInitStruct(char *name, unsigned long size, bool *foundPtr)
{
	ShmemIndexEnt *result,
				item;
	long	   *structPtr;

	strncpy(item.key, name, SHMEM_INDEX_KEYSIZE);
	item.location = BAD_LOCATION;

	SpinAcquire(ShmemIndexLock);

	if (!ShmemIndex)
	{
#ifdef USE_ASSERT_CHECKING
		char	   *strname = "ShmemIndex";

#endif

		/*
		 * If the shmem index doesnt exist, we fake it.
		 *
		 * If we are creating the first shmem index, then let shmemalloc()
		 * allocate the space for a new HTAB.  Otherwise, find the old one
		 * and return that.  Notice that the ShmemIndexLock is held until
		 * the shmem index has been completely initialized.
		 */
		Assert(!strcmp(name, strname));
		if (ShmemBootstrap)
		{
			/* in POSTMASTER/Single process */

			*foundPtr = FALSE;
			return (long *) ShmemAlloc(size);
		}
		else
		{
			Assert(ShmemIndexOffset);

			*foundPtr = TRUE;
			return (long *) MAKE_PTR(*ShmemIndexOffset);
		}


	}
	else
	{
		/* look it up in the shmem index */
		result = (ShmemIndexEnt *)
			hash_search(ShmemIndex, (char *) &item, HASH_ENTER, foundPtr);
	}

	if (!result)
	{
		SpinRelease(ShmemIndexLock);

		elog(ERROR, "ShmemInitStruct: Shmem Index corrupted");
		return NULL;

	}
	else if (*foundPtr)
	{

		/*
		 * Structure is in the shmem index so someone else has allocated
		 * it already.	The size better be the same as the size we are
		 * trying to initialize to or there is a name conflict (or worse).
		 */
		if (result->size != size)
		{
			SpinRelease(ShmemIndexLock);

			elog(NOTICE, "ShmemInitStruct: ShmemIndex entry size is wrong");
			/* let caller print its message too */
			return NULL;
		}
		structPtr = (long *) MAKE_PTR(result->location);
	}
	else
	{
		/* It isn't in the table yet. allocate and initialize it */
		structPtr = ShmemAlloc((long) size);
		if (!structPtr)
		{
			/* out of memory */
			Assert(ShmemIndex);
			hash_search(ShmemIndex, (char *) &item, HASH_REMOVE, foundPtr);
			SpinRelease(ShmemIndexLock);
			*foundPtr = FALSE;

			elog(NOTICE, "ShmemInitStruct: cannot allocate '%s'",
				 name);
			return NULL;
		}
		result->size = size;
		result->location = MAKE_OFFSET(structPtr);
	}
	Assert(ShmemIsValid((unsigned long) structPtr));

	SpinRelease(ShmemIndexLock);
	return structPtr;
}
