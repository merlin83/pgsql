/*-------------------------------------------------------------------------
 *
 * heap.h
 *	  prototypes for functions in backend/catalog/heap.c
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: heap.h,v 1.51 2002-06-20 20:29:43 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef HEAP_H
#define HEAP_H

#include "catalog/pg_attribute.h"
#include "nodes/parsenodes.h"
#include "parser/parse_node.h"
#include "utils/rel.h"


typedef struct RawColumnDefault
{
	AttrNumber	attnum;			/* attribute to attach default to */
	Node	   *raw_default;	/* default value (untransformed parse
								 * tree) */
} RawColumnDefault;

extern Relation heap_create(const char *relname,
			Oid relnamespace,
			TupleDesc tupDesc,
			bool shared_relation,
			bool storage_create,
			bool allow_system_table_mods);

extern void heap_storage_create(Relation rel);

extern Oid heap_create_with_catalog(const char *relname,
						 Oid relnamespace,
						 TupleDesc tupdesc,
						 char relkind,
						 bool shared_relation,
						 bool relhasoids,
						 bool allow_system_table_mods);

extern void heap_drop_with_catalog(Oid rid, bool allow_system_table_mods);

extern void heap_truncate(Oid rid);

extern void AddRelationRawConstraints(Relation rel,
						  List *rawColDefaults,
						  List *rawConstraints);

extern Node *cookDefault(ParseState *pstate,
						 Node *raw_default,
						 Oid atttypid,
						 int32 atttypmod,
						 char *attname);

extern int	RemoveCheckConstraint(Relation rel, const char *constrName, bool inh);

extern Form_pg_attribute SystemAttributeDefinition(AttrNumber attno,
						  bool relhasoids);

extern Form_pg_attribute SystemAttributeByName(const char *attname,
					  bool relhasoids);

#endif   /* HEAP_H */
