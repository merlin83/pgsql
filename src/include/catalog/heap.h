/*-------------------------------------------------------------------------
 *
 * heap.h
 *	  prototypes for functions in lib/catalog/heap.c
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: heap.h,v 1.30 2000-06-18 22:44:25 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef HEAP_H
#define HEAP_H

#include "utils/rel.h"

typedef struct RawColumnDefault
{
	AttrNumber	attnum;			/* attribute to attach default to */
	Node	   *raw_default;	/* default value (untransformed parse
								 * tree) */
} RawColumnDefault;

extern Oid	RelnameFindRelid(const char *relname);

extern Relation heap_create(char *relname, TupleDesc tupDesc,
							bool istemp, bool storage_create);
extern bool heap_storage_create(Relation rel);

extern Oid heap_create_with_catalog(char *relname, TupleDesc tupdesc,
						 char relkind, bool istemp);

extern void heap_drop_with_catalog(const char *relname);
extern void heap_truncate(char *relname);

extern void AddRelationRawConstraints(Relation rel,
						  List *rawColDefaults,
						  List *rawConstraints);

#endif	 /* HEAP_H */
