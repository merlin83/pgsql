/*-------------------------------------------------------------------------
 *
 * index.h
 *	  prototypes for index.c.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: index.h,v 1.21 2000-01-26 05:57:56 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef INDEX_H
#define INDEX_H

#include "access/itup.h"
#include "nodes/execnodes.h"

extern Form_pg_am AccessMethodObjectIdGetForm(Oid accessMethodObjectId);

extern void UpdateIndexPredicate(Oid indexoid, Node *oldPred, Node *predicate);

extern void InitIndexStrategy(int numatts,
				  Relation indexRelation,
				  Oid accessMethodObjectId);

extern void index_create(char *heapRelationName,
			 char *indexRelationName,
			 FuncIndexInfo *funcInfo,
			 List *attributeList,
			 Oid accessMethodObjectId,
			 int numatts,
			 AttrNumber *attNums,
			 Oid *classObjectId,
			 uint16 parameterCount,
			 Datum *parameter,
			 Node *predicate,
			 bool islossy,
			 bool unique,
			 bool primary);

extern void index_drop(Oid indexId);

extern void FormIndexDatum(int numberOfAttributes,
			   AttrNumber *attributeNumber, HeapTuple heapTuple,
			   TupleDesc heapDescriptor, Datum *datum,
			   char *nullv, FuncIndexInfoPtr fInfo);

extern void UpdateStats(Oid relid, long reltuples, bool hasindex);

extern void FillDummyExprContext(ExprContext *econtext, TupleTableSlot *slot,
					 TupleDesc tupdesc, Buffer buffer);

extern void index_build(Relation heapRelation, Relation indexRelation,
			int numberOfAttributes, AttrNumber *attributeNumber,
		uint16 parameterCount, Datum *parameter, FuncIndexInfo *funcInfo,
			PredInfo *predInfo);

extern bool IndexIsUnique(Oid indexId);
extern bool IndexIsUniqueNoCache(Oid indexId);

#endif	 /* INDEX_H */
