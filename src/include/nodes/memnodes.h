/*-------------------------------------------------------------------------
 *
 * memnodes.h--
 *	  POSTGRES memory context node definitions.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: memnodes.h,v 1.5 1997-09-07 04:58:35 momjian Exp $
 *
 * XXX the typedefs in this file are different from the other ???nodes.h;
 *	  they are pointers to structures instead of the structures themselves.
 *	  If you're wondering, this is plain laziness. I don't want to touch
 *	  the memory context code which should be revamped altogether some day.
 *														- ay 10/94
 *-------------------------------------------------------------------------
 */
#ifndef MEMNODES_H
#define MEMNODES_H

#include <lib/fstack.h>
#include <utils/memutils.h>
#include <nodes/nodes.h>

/*
 * MemoryContext --
 *		A logical context in which memory allocations occur.
 *
 * The types of memory contexts can be thought of as members of the
 * following inheritance hierarchy with properties summarized below.
 *
 *						Node
 *						|
 *				MemoryContext___
 *				/				\
 *		GlobalMemory	PortalMemoryContext
 *						/				\
 *		PortalVariableMemory	PortalHeapMemory
 *
 *						Flushed at		Flushed at		Checkpoints
 *						Transaction		Portal
 *						Commit			Close
 *
 * GlobalMemory					n				n				n
 * PortalVariableMemory			n				y				n
 * PortalHeapMemory				y				y				y
 */

typedef struct MemoryContextMethodsData
{
	Pointer(*alloc) ();
	void			(*free_p) ();		/* need to use free as a #define,
										 * so can't use free */
					Pointer(*realloc) ();
	char		   *(*getName) ();
	void			(*dump) ();
}			   *MemoryContextMethods;

typedef struct MemoryContext
{
	NodeTag			type;
	MemoryContextMethods method;
}			   *MemoryContext;

/* think about doing this right some time but we'll have explicit fields
   for now -ay 10/94 */
typedef struct GlobalMemory
{
	NodeTag			type;
	MemoryContextMethods method;
	AllocSetData	setData;
	char		   *name;
	OrderedElemData elemData;
}			   *GlobalMemory;

typedef MemoryContext *PortalMemoryContext;

typedef struct PortalVariableMemory
{
	NodeTag			type;
	MemoryContextMethods method;
	AllocSetData	setData;
}			   *PortalVariableMemory;

typedef struct PortalHeapMemory
{
	NodeTag			type;
	MemoryContextMethods method;
	Pointer			block;
	FixedStackData	stackData;
}			   *PortalHeapMemory;

/*
 * MemoryContextIsValid --
 *		True iff memory context is valid.
 */
#define MemoryContextIsValid(context) \
	(IsA(context,MemoryContext) || IsA(context,GlobalMemory) || \
	 IsA(context,PortalVariableMemory) || IsA(context,PortalHeapMemory))

#endif							/* MEMNODES_H */
