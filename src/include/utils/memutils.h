/*-------------------------------------------------------------------------
 *
 * memutils.h
 *	  This file contains declarations for memory allocation utility
 *	  functions.  These are functions that are not quite widely used
 *	  enough to justify going in utils/palloc.h, but are still part
 *	  of the API of the memory management subsystem.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: memutils.h,v 1.36 2000-06-28 03:33:33 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef MEMUTILS_H
#define MEMUTILS_H

#include "nodes/memnodes.h"


/*
 * MaxAllocSize
 *		Arbitrary limit on size of allocations.
 *
 * Note:
 *		There is no guarantee that allocations smaller than MaxAllocSize
 *		will succeed.  Allocation requests larger than MaxAllocSize will
 *		be summarily denied.
 *
 * XXX This should be defined in a file of tunable constants.
 */
#define MaxAllocSize	((Size) 0xfffffff)		/* 16G - 1 */

#define AllocSizeIsValid(size)	(0 < (size) && (size) <= MaxAllocSize)

/*
 * All chunks allocated by any memory context manager are required to be
 * preceded by a StandardChunkHeader at a spacing of STANDARDCHUNKHEADERSIZE.
 * A currently-allocated chunk must contain a backpointer to its owning
 * context as well as the allocated size of the chunk.  The backpointer is
 * used by pfree() and repalloc() to find the context to call.  The allocated
 * size is not absolutely essential, but it's expected to be needed by any
 * reasonable implementation.
 */
typedef struct StandardChunkHeader
{
	MemoryContext context;			/* owning context */
	Size size;						/* size of data space allocated in chunk */
} StandardChunkHeader;

#define STANDARDCHUNKHEADERSIZE  MAXALIGN(sizeof(StandardChunkHeader))


/*
 * Standard top-level memory contexts.
 *
 * Only TopMemoryContext and ErrorContext are initialized by
 * MemoryContextInit() itself.
 */
extern MemoryContext TopMemoryContext;
extern MemoryContext ErrorContext;
extern MemoryContext PostmasterContext;
extern MemoryContext CacheMemoryContext;
extern MemoryContext QueryContext;
extern MemoryContext TopTransactionContext;
extern MemoryContext TransactionCommandContext;


/*
 * Memory-context-type-independent functions in mcxt.c
 */
extern void MemoryContextInit(void);
extern void MemoryContextReset(MemoryContext context);
extern void MemoryContextDelete(MemoryContext context);
extern void MemoryContextResetChildren(MemoryContext context);
extern void MemoryContextDeleteChildren(MemoryContext context);
extern void MemoryContextResetAndDeleteChildren(MemoryContext context);
extern void MemoryContextStats(MemoryContext context);
extern bool MemoryContextContains(MemoryContext context, void *pointer);

/*
 * This routine handles the context-type-independent part of memory
 * context creation.  It's intended to be called from context-type-
 * specific creation routines, and noplace else.
 */
extern MemoryContext MemoryContextCreate(NodeTag tag, Size size,
										 MemoryContextMethods *methods,
										 MemoryContext parent,
										 const char *name);


/*
 * Memory-context-type-specific functions
 */

/* aset.c */
extern MemoryContext AllocSetContextCreate(MemoryContext parent,
										   const char *name,
										   Size minContextSize,
										   Size initBlockSize,
										   Size maxBlockSize);

/*
 * Recommended default alloc parameters, suitable for "ordinary" contexts
 * that might hold quite a lot of data.
 */
#define ALLOCSET_DEFAULT_MINSIZE   (8 * 1024)
#define ALLOCSET_DEFAULT_INITSIZE  (8 * 1024)
#define ALLOCSET_DEFAULT_MAXSIZE   (8 * 1024 * 1024)


#endif	 /* MEMUTILS_H */
