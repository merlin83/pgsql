/*-------------------------------------------------------------------------
 *
 * memnodes.h
 *	  POSTGRES memory context node definitions.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: memnodes.h,v 1.18 2000-07-11 14:30:34 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef MEMNODES_H
#define MEMNODES_H

#include "nodes/nodes.h"

/*
 * MemoryContext
 *		A logical context in which memory allocations occur.
 *
 * MemoryContext itself is an abstract type that can have multiple
 * implementations, though for now we have only AllocSetContext.
 * The function pointers in MemoryContextMethods define one specific
 * implementation of MemoryContext --- they are a virtual function table
 * in C++ terms.
 *
 * Node types that are actual implementations of memory contexts must
 * begin with the same fields as MemoryContext.
 *
 * Note: for largely historical reasons, typedef MemoryContext is a pointer
 * to the context struct rather than the struct type itself.
 */

typedef struct MemoryContextMethods
{
	void	   *(*alloc) (MemoryContext context, Size size);
	/* call this free_p in case someone #define's free() */
	void		(*free_p) (MemoryContext context, void *pointer);
	void	   *(*realloc) (MemoryContext context, void *pointer, Size size);
	void		(*init) (MemoryContext context);
	void		(*reset) (MemoryContext context);
	void		(*delete) (MemoryContext context);
#ifdef MEMORY_CONTEXT_CHECKING	
	void		(*check) (MemoryContext context);
#endif
	void		(*stats) (MemoryContext context);
} MemoryContextMethods;


typedef struct MemoryContextData
{
	NodeTag		type;				/* identifies exact kind of context */
	MemoryContextMethods *methods;	/* virtual function table */
	MemoryContext parent;			/* NULL if no parent (toplevel context) */
	MemoryContext firstchild;		/* head of linked list of children */
	MemoryContext nextchild;		/* next child of same parent */
	char	   *name;				/* context name (just for debugging) */
} MemoryContextData;

/* utils/palloc.h contains typedef struct MemoryContextData *MemoryContext */


/*
 * AllocSetContext is our standard implementation of MemoryContext.
 */
typedef struct AllocBlockData *AllocBlock; /* internal to aset.c */
typedef struct AllocChunkData *AllocChunk;

typedef struct AllocSetContext
{
	MemoryContextData header;		/* Standard memory-context fields */
	/* Info about storage allocated in this context: */
	AllocBlock	blocks;				/* head of list of blocks in this set */
#define ALLOCSET_NUM_FREELISTS	8
	AllocChunk	freelist[ALLOCSET_NUM_FREELISTS]; /* free chunk lists */
	/* Allocation parameters for this context: */
	Size		initBlockSize;		/* initial block size */
	Size		maxBlockSize;		/* maximum block size */
	AllocBlock	keeper;				/* if not NULL, keep this block
									 * over resets */
} AllocSetContext;


/*
 * MemoryContextIsValid
 *		True iff memory context is valid.
 *
 * Add new context types to the set accepted by this macro.
 */
#define MemoryContextIsValid(context) \
	((context) != NULL && \
	 (IsA((context), AllocSetContext)))


#endif	 /* MEMNODES_H */
