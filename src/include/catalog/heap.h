/*-------------------------------------------------------------------------
 *
 * heap.h
 *	  prototypes for functions in lib/catalog/heap.c
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: heap.h,v 1.25 1999-11-04 08:01:04 inoue Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef HEAP_H
#define HEAP_H

#include "utils/rel.h"

typedef struct RawColumnDefault
{
	AttrNumber	attnum;			/* attribute to attach default to */
	Node	   *raw_default;	/* default value (untransformed parse tree) */
} RawColumnDefault;

extern Oid RelnameFindRelid(char *relname);
extern Relation heap_create(char *relname, TupleDesc att,
			    bool isnoname, bool istemp, bool storage_create);
extern bool heap_storage_create(Relation rel);

extern Oid heap_create_with_catalog(char *relname, TupleDesc tupdesc, 
				    char relkind, bool istemp);

extern void heap_destroy_with_catalog(char *relname);
extern void heap_truncate(char *relname);
extern void heap_destroy(Relation rel);

extern void AddRelationRawConstraints(Relation rel,
				      List *rawColDefaults,
				      List *rawConstraints);

extern void InitNoNameRelList(void);
extern void DestroyNoNameRels(void);

#endif	 /* HEAP_H */
