 /*-------------------------------------------------------------------------
 *
 * htup.h
 *	  POSTGRES heap tuple definitions.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: htup.h,v 1.31 2000-07-02 22:01:00 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef HTUP_H
#define HTUP_H

#include "storage/bufpage.h"

#define MinHeapTupleBitmapSize	32		/* 8 * 4 */

/* check these, they are likely to be more severely limited by t_hoff */

#define MaxHeapAttributeNumber	1600	/* 8 * 200 */

/*
 * to avoid wasting space, the attributes should be layed out in such a
 * way to reduce structure padding.
 */
typedef struct HeapTupleHeaderData
{
	Oid			t_oid;			/* OID of this tuple -- 4 bytes */

	CommandId	t_cmin;			/* insert CID stamp -- 4 bytes each */
	CommandId	t_cmax;			/* delete CommandId stamp */

	TransactionId t_xmin;		/* insert XID stamp -- 4 bytes each */
	TransactionId t_xmax;		/* delete XID stamp */

	ItemPointerData t_ctid;		/* current TID of this or newer tuple */

	int16		t_natts;		/* number of attributes */

	uint16		t_infomask;		/* various infos */

	uint8		t_hoff;			/* sizeof tuple header */

								/* ^ - 31 bytes - ^ */

	bits8		t_bits[MinHeapTupleBitmapSize / 8];
	/* bit map of domains */

	/* MORE DATA FOLLOWS AT END OF STRUCT */
} HeapTupleHeaderData;

typedef HeapTupleHeaderData *HeapTupleHeader;


#ifdef XLOG

/* XLOG stuff */

/*
 * XLOG allows to store some information in high 4 bits of log
 * record xl_info field
 */
#define	XLOG_HEAP_INSERT	0x00
#define	XLOG_HEAP_DELETE	0x10
#define	XLOG_HEAP_UPDATE	0x20
#define	XLOG_HEAP_MOVE		0x30

/*
 * All what we need to find changed tuple (14 bytes)
 */
typedef struct xl_heaptid
{
	Oid					dbId;		/* database */
	Oid					relId;		/* relation */
	ItemPointerData		tid;		/* changed tuple id */
} xl_heaptid;

/* This is what we need to know about delete - ALIGN(14) = 16 bytes */
typedef struct xl_heap_delete
{
	xl_heaptid			dtid;		/* deleted tuple id */
} xl_heap_delete;

/* This is what we need to know about insert - 22 + data */
typedef struct xl_heap_insert
{
	xl_heaptid			itid;		/* inserted tuple id */
	/* something from tuple header */
	int16				t_natts;
	Oid					t_oid;
	uint8				t_hoff;
	uint8				mask;		/* low 8 bits of t_infomask */
	/* TUPLE DATA FOLLOWS AT END OF STRUCT */
} xl_heap_insert;

/* This is what we need to know about update - 28 + data */
typedef struct xl_heap_update
{
	xl_heaptid			dtid;		/* deleted tuple id */
	ItemPointerData		itid;		/* new inserted tuple id */
	/* something from header of new tuple version */
	int16				t_natts;
	uint8				t_hoff;
	uint8				mask;		/* low 8 bits of t_infomask */
	/* NEW TUPLE DATA FOLLOWS AT END OF STRUCT */
} xl_heap_update;

/* This is what we need to know about tuple move - ALIGN(20) = 24 bytes */
typedef struct xl_heap_move
{
	xl_heaptid			ftid;		/* moved from */
	ItemPointerData		ttid;		/* moved to */
} xl_heap_move;

/* end of XLOG stuff */

#endif	/* XLOG */

#define MinTupleSize	(MAXALIGN(sizeof (PageHeaderData)) + \
						 MAXALIGN(sizeof(HeapTupleHeaderData)) + \
						 MAXALIGN(sizeof(char)))

#define MaxTupleSize	(BLCKSZ - MinTupleSize)

#define MaxAttrSize		(MaxTupleSize - MAXALIGN(sizeof(HeapTupleHeaderData)))

#define SelfItemPointerAttributeNumber			(-1)
#define ObjectIdAttributeNumber					(-2)
#define MinTransactionIdAttributeNumber			(-3)
#define MinCommandIdAttributeNumber				(-4)
#define MaxTransactionIdAttributeNumber			(-5)
#define MaxCommandIdAttributeNumber				(-6)
#define TableOidAttributeNumber			        (-7)
#define FirstLowInvalidHeapAttributeNumber		(-8)

/* If you make any changes above, the order off offsets in this must change */
extern long heap_sysoffset[];

/*
 * This new HeapTuple for version >= 6.5 and this is why it was changed:
 *
 * 1. t_len moved off on-disk tuple data - ItemIdData is used to get len;
 * 2. t_ctid above is not self tuple TID now - it may point to
 *	  updated version of tuple (required by MVCC);
 * 3. someday someone let tuple to cross block boundaries -
 *	  he have to add something below...
 *
 * Change for 7.0:
 *	  Up to now t_data could be NULL, the memory location directly following
 *	  HeapTupleData or pointing into a buffer. Now, it could also point to
 *	  a separate allocation that was done in the t_datamcxt memory context.
 */
typedef struct HeapTupleData
{
	uint32		t_len;			/* length of *t_data */
	ItemPointerData t_self;		/* SelfItemPointer */
	Oid tableOid;                    /* */
	MemoryContext t_datamcxt;	/* */
	HeapTupleHeader t_data;		/* */
} HeapTupleData;

typedef HeapTupleData *HeapTuple;

#define HEAPTUPLESIZE	MAXALIGN(sizeof(HeapTupleData))


/* ----------------
 *		support macros
 * ----------------
 */
#define GETSTRUCT(TUP) (((char *)((HeapTuple)(TUP))->t_data) + \
						((HeapTuple)(TUP))->t_data->t_hoff)


/*
 * BITMAPLEN(NATTS) -
 *		Computes minimum size of bitmap given number of domains.
 */
#define BITMAPLEN(NATTS) \
		((((((int)(NATTS) - 1) >> 3) + 4 - (MinHeapTupleBitmapSize >> 3)) \
		  & ~03) + (MinHeapTupleBitmapSize >> 3))

/*
 * HeapTupleIsValid
 *		True iff the heap tuple is valid.
 */
#define HeapTupleIsValid(tuple) PointerIsValid(tuple)

/*
 * information stored in t_infomask:
 */
#define HEAP_HASNULL			0x0001	/* has null attribute(s) */
#define HEAP_HASVARLENA			0x0002	/* has variable length
										 * attribute(s) */
#define HEAP_HASEXTERNAL		0x0004	/* has external stored */
 /* attribute(s) */
#define HEAP_HASCOMPRESSED		0x0008	/* has compressed stored */
 /* attribute(s) */
#define HEAP_HASEXTENDED		0x000C	/* the two above combined */
#define HEAP_XMIN_COMMITTED		0x0100	/* t_xmin committed */
#define HEAP_XMIN_INVALID		0x0200	/* t_xmin invalid/aborted */
#define HEAP_XMAX_COMMITTED		0x0400	/* t_xmax committed */
#define HEAP_XMAX_INVALID		0x0800	/* t_xmax invalid/aborted */
#define HEAP_MARKED_FOR_UPDATE	0x1000	/* marked for UPDATE */
#define HEAP_UPDATED			0x2000	/* this is UPDATEd version of row */
#define HEAP_MOVED_OFF			0x4000	/* removed or moved to another
										 * place by vacuum */
#define HEAP_MOVED_IN			0x8000	/* moved from another place by
										 * vacuum */

#define HEAP_XACT_MASK			0xFF00	/* */

#define HeapTupleNoNulls(tuple) \
		(!(((HeapTuple) (tuple))->t_data->t_infomask & HEAP_HASNULL))

#define HeapTupleAllFixed(tuple) \
		(!(((HeapTuple) (tuple))->t_data->t_infomask & HEAP_HASVARLENA))

#define HeapTupleHasExternal(tuple) \
		((((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASEXTERNAL) != 0)

#define HeapTupleHasCompressed(tuple) \
		((((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASCOMPRESSED) != 0)

#define HeapTupleHasExtended(tuple) \
		((((HeapTuple)(tuple))->t_data->t_infomask & HEAP_HASEXTENDED) != 0)

#endif	 /* HTUP_H */
