/*-------------------------------------------------------------------------
 *
 * heap.h--
 *	  prototypes for functions in lib/catalog/heap.c
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: heap.h,v 1.13 1998-08-06 05:13:01 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef HEAP_H
#define HEAP_H

#include <utils/rel.h>

extern Relation heap_create(char *relname, TupleDesc att);

extern Oid	heap_create_with_catalog(char *relname,
									 TupleDesc tupdesc, char relkind);

extern void heap_destroy_with_catalog(char relname[]);
extern void heap_destroy(Relation r);

extern void InitTempRelList(void);
extern void DestroyTempRels(void);

#endif							/* HEAP_H */
