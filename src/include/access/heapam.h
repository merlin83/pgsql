/*-------------------------------------------------------------------------
 *
 * heapam.h
 *	  POSTGRES heap access method definitions.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: heapam.h,v 1.55 2000-07-02 22:01:00 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef HEAPAM_H
#define HEAPAM_H

#include <time.h>
#include "access/htup.h"
#include "access/relscan.h"
#include "access/tupmacs.h"
#include "storage/block.h"
#include "storage/lmgr.h"
#include "utils/rel.h"
#include "utils/tqual.h"

/* ----------------------------------------------------------------
 *				heap access method statistics
 * ----------------------------------------------------------------
 */

typedef struct HeapAccessStatisticsData
{
	time_t		init_global_timestamp;	/* time global statistics started */
	time_t		local_reset_timestamp;	/* last time local reset was done */
	time_t		last_request_timestamp; /* last time stats were requested */

	int			global_open;
	int			global_openr;
	int			global_close;
	int			global_beginscan;
	int			global_rescan;
	int			global_endscan;
	int			global_getnext;
	int			global_fetch;
	int			global_insert;
	int			global_delete;
	int			global_replace;
	int			global_mark4update;
	int			global_markpos;
	int			global_restrpos;
	int			global_BufferGetRelation;
	int			global_RelationIdGetRelation;
	int			global_RelationIdGetRelation_Buf;
	int			global_RelationNameGetRelation;
	int			global_getreldesc;
	int			global_heapgettup;
	int			global_RelationPutHeapTuple;
	int			global_RelationPutLongHeapTuple;

	int			local_open;
	int			local_openr;
	int			local_close;
	int			local_beginscan;
	int			local_rescan;
	int			local_endscan;
	int			local_getnext;
	int			local_fetch;
	int			local_insert;
	int			local_delete;
	int			local_replace;
	int			local_mark4update;
	int			local_markpos;
	int			local_restrpos;
	int			local_BufferGetRelation;
	int			local_RelationIdGetRelation;
	int			local_RelationIdGetRelation_Buf;
	int			local_RelationNameGetRelation;
	int			local_getreldesc;
	int			local_heapgettup;
	int			local_RelationPutHeapTuple;
	int			local_RelationPutLongHeapTuple;
} HeapAccessStatisticsData;

typedef HeapAccessStatisticsData *HeapAccessStatistics;

#define IncrHeapAccessStat(x) \
	(heap_access_stats == NULL ? 0 : (heap_access_stats->x)++)

/* ----------------
 *		fastgetattr
 *
 *		This gets called many times, so we macro the cacheable and NULL
 *		lookups, and call noncachegetattr() for the rest.
 *
 * ----------------
 */

extern Datum nocachegetattr(HeapTuple tup, int attnum,
			   TupleDesc att, bool *isnull);

#if !defined(DISABLE_COMPLEX_MACRO)

#define fastgetattr(tup, attnum, tupleDesc, isnull)					\
(																	\
	AssertMacro((attnum) > 0),										\
	((isnull) ? (*(isnull) = false) : (dummyret)NULL),				\
	HeapTupleNoNulls(tup) ?											\
	(																\
		((tupleDesc)->attrs[(attnum)-1]->attcacheoff != -1 ||		\
		 (attnum) == 1) ?											\
		(															\
			(Datum)fetchatt(&((tupleDesc)->attrs[(attnum)-1]),		\
				(char *) (tup)->t_data + (tup)->t_data->t_hoff +	\
				(													\
					((attnum) != 1) ?								\
						(tupleDesc)->attrs[(attnum)-1]->attcacheoff	\
					:												\
						0											\
				)													\
			)														\
		)															\
		:															\
			nocachegetattr((tup), (attnum), (tupleDesc), (isnull))	\
	)																\
	:																\
	(																\
		att_isnull((attnum)-1, (tup)->t_data->t_bits) ?				\
		(															\
			((isnull) ? (*(isnull) = true) : (dummyret)NULL),		\
			(Datum)NULL												\
		)															\
		:															\
		(															\
			nocachegetattr((tup), (attnum), (tupleDesc), (isnull))	\
		)															\
	)																\
)

#else /* defined(DISABLE_COMPLEX_MACRO) */

extern Datum
fastgetattr(HeapTuple tup, int attnum, TupleDesc tupleDesc,
			bool *isnull);

#endif /* defined(DISABLE_COMPLEX_MACRO) */


/* ----------------
 *		heap_getattr
 *
 *		Find a particular field in a row represented as a heap tuple.
 *		We return a pointer into that heap tuple, which points to the
 *		first byte of the value of the field in question.
 *
 *		If the field in question has a NULL value, we return a null
 *		pointer and return <*isnull> == true.  Otherwise, we return
 *		<*isnull> == false.
 *
 *		<tup> is the pointer to the heap tuple.  <attnum> is the attribute
 *		number of the column (field) caller wants.	<tupleDesc> is a
 *		pointer to the structure describing the row and all its fields.
 *
 *		Because this macro is often called with constants, it generates
 *		compiler warnings about 'left-hand comma expression has no effect.
 *
 * ----------------
 */
#define heap_getattr(tup, attnum, tupleDesc, isnull) \
( \
	AssertMacro((tup) != NULL && \
		(attnum) > FirstLowInvalidHeapAttributeNumber && \
		(attnum) != 0), \
	((attnum) > (int) (tup)->t_data->t_natts) ? \
	( \
		((isnull) ? (*(isnull) = true) : (dummyret)NULL), \
		(Datum)NULL \
	) \
	: \
	( \
		((attnum) > 0) ? \
		( \
			fastgetattr((tup), (attnum), (tupleDesc), (isnull)) \
		) \
		: \
		( \
			((isnull) ? (*(isnull) = false) : (dummyret)NULL), \
			((attnum) == SelfItemPointerAttributeNumber) ? \
			( \
				(Datum)((char *)&((tup)->t_self)) \
			) \
			: \
			(((attnum) == TableOidAttributeNumber) ? \
			( \
				(Datum)((tup)->tableOid) \
			) \
            : \
			( \
				(Datum)*(unsigned int *) \
					((char *)(tup)->t_data + heap_sysoffset[-(attnum)-1]) \
			)) \
		) \
	) \
)

extern HeapAccessStatistics heap_access_stats;	/* in stats.c */

/* ----------------
 *		function prototypes for heap access method
 *
 * heap_create, heap_create_with_catalog, and heap_drop_with_catalog
 * are declared in catalog/heap.h
 * ----------------
 */

/* heapam.c */

extern Relation heap_open(Oid relationId, LOCKMODE lockmode);
extern Relation heap_openr(const char *relationName, LOCKMODE lockmode);
extern void heap_close(Relation relation, LOCKMODE lockmode);
extern HeapScanDesc heap_beginscan(Relation relation, int atend,
			   Snapshot snapshot, unsigned nkeys, ScanKey key);
extern void heap_rescan(HeapScanDesc scan, bool scanFromEnd, ScanKey key);
extern void heap_endscan(HeapScanDesc scan);
extern HeapTuple heap_getnext(HeapScanDesc scandesc, int backw);
extern void heap_fetch(Relation relation, Snapshot snapshot, HeapTuple tup, Buffer *userbuf);
extern ItemPointer heap_get_latest_tid(Relation relation, Snapshot snapshot, ItemPointer tid);
extern Oid	heap_insert(Relation relation, HeapTuple tup);
extern int	heap_delete(Relation relation, ItemPointer tid, ItemPointer ctid);
extern int heap_update(Relation relation, ItemPointer otid, HeapTuple tup,
			ItemPointer ctid);
extern int	heap_mark4update(Relation relation, HeapTuple tup, Buffer *userbuf);
extern void heap_markpos(HeapScanDesc scan);
extern void heap_restrpos(HeapScanDesc scan);

/* in common/heaptuple.c */
extern Size ComputeDataSize(TupleDesc tupleDesc, Datum *value, char *nulls);
extern void DataFill(char *data, TupleDesc tupleDesc,
		 Datum *value, char *nulls, uint16 *infomask,
		 bits8 *bit);
extern int	heap_attisnull(HeapTuple tup, int attnum);
extern int	heap_sysattrlen(AttrNumber attno);
extern bool heap_sysattrbyval(AttrNumber attno);
extern Datum nocachegetattr(HeapTuple tup, int attnum,
			   TupleDesc att, bool *isnull);
extern HeapTuple heap_copytuple(HeapTuple tuple);
extern void heap_copytuple_with_tuple(HeapTuple src, HeapTuple dest);
extern HeapTuple heap_formtuple(TupleDesc tupleDescriptor,
			   Datum *value, char *nulls);
extern HeapTuple heap_modifytuple(HeapTuple tuple,
		Relation relation, Datum *replValue, char *replNull, char *repl);
extern void heap_freetuple(HeapTuple tuple);
HeapTuple	heap_addheader(uint32 natts, int structlen, char *structure);

/* in common/heap/stats.c */
extern void PrintHeapAccessStatistics(HeapAccessStatistics stats);
extern void initam(void);

#endif	 /* HEAPAM_H */
