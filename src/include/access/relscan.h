/*-------------------------------------------------------------------------
 *
 * relscan.h
 *	  POSTGRES relation scan descriptor definitions.
 *
 *
 * Portions Copyright (c) 1996-2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/access/relscan.h,v 1.63 2008/04/12 23:14:21 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef RELSCAN_H
#define RELSCAN_H

#include "access/htup.h"
#include "access/skey.h"
#include "storage/bufpage.h"
#include "utils/snapshot.h"


typedef struct HeapScanDescData
{
	/* scan parameters */
	Relation	rs_rd;			/* heap relation descriptor */
	Snapshot	rs_snapshot;	/* snapshot to see */
	int			rs_nkeys;		/* number of scan keys */
	ScanKey		rs_key;			/* array of scan key descriptors */
	bool		rs_bitmapscan;	/* true if this is really a bitmap scan */
	bool		rs_pageatatime; /* verify visibility page-at-a-time? */
	bool		rs_allow_strat; /* allow or disallow use of access strategy */
	bool		rs_allow_sync;	/* allow or disallow use of syncscan */

	/* state set up at initscan time */
	BlockNumber rs_nblocks;		/* number of blocks to scan */
	BlockNumber rs_startblock;	/* block # to start at */
	BufferAccessStrategy rs_strategy;	/* access strategy for reads */
	bool		rs_syncscan;	/* report location to syncscan logic? */

	/* scan current state */
	bool		rs_inited;		/* false = scan not init'd yet */
	HeapTupleData rs_ctup;		/* current tuple in scan, if any */
	BlockNumber rs_cblock;		/* current block # in scan, if any */
	Buffer		rs_cbuf;		/* current buffer in scan, if any */
	/* NB: if rs_cbuf is not InvalidBuffer, we hold a pin on that buffer */
	ItemPointerData rs_mctid;	/* marked scan position, if any */

	/* these fields only used in page-at-a-time mode and for bitmap scans */
	int			rs_cindex;		/* current tuple's index in vistuples */
	int			rs_mindex;		/* marked tuple's saved index */
	int			rs_ntuples;		/* number of visible tuples on page */
	OffsetNumber rs_vistuples[MaxHeapTuplesPerPage];	/* their offsets */
} HeapScanDescData;

typedef HeapScanDescData *HeapScanDesc;

/*
 * We use the same IndexScanDescData structure for both amgettuple-based
 * and amgetbitmap-based index scans.  Some fields are only relevant in
 * amgettuple-based scans.
 */
typedef struct IndexScanDescData
{
	/* scan parameters */
	Relation	heapRelation;	/* heap relation descriptor, or NULL */
	Relation	indexRelation;	/* index relation descriptor */
	Snapshot	xs_snapshot;	/* snapshot to see */
	int			numberOfKeys;	/* number of scan keys */
	ScanKey		keyData;		/* array of scan key descriptors */

	/* signaling to index AM about killing index tuples */
	bool		kill_prior_tuple;		/* last-returned tuple is dead */
	bool		ignore_killed_tuples;	/* do not return killed entries */

	/* index access method's private state */
	void	   *opaque;			/* access-method-specific info */

	/* xs_ctup/xs_cbuf are valid after a successful index_getnext */
	HeapTupleData xs_ctup;		/* current heap tuple, if any */
	Buffer		xs_cbuf;		/* current heap buffer in scan, if any */
	/* NB: if xs_cbuf is not InvalidBuffer, we hold a pin on that buffer */

	/* state data for traversing HOT chains in index_getnext */
	TransactionId xs_prev_xmax; /* previous HOT chain member's XMAX, if any */
	OffsetNumber xs_next_hot;	/* next member of HOT chain, if any */
	bool		xs_hot_dead;	/* T if all members of HOT chain are dead */
} IndexScanDescData;

typedef IndexScanDescData *IndexScanDesc;


/*
 * HeapScanIsValid
 *		True iff the heap scan is valid.
 */
#define HeapScanIsValid(scan) PointerIsValid(scan)

/*
 * IndexScanIsValid
 *		True iff the index scan is valid.
 */
#define IndexScanIsValid(scan) PointerIsValid(scan)

#endif   /* RELSCAN_H */
