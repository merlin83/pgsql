/*-------------------------------------------------------------------------
 *
 * snapshot.h
 *	  POSTGRES snapshot definition
 *
 * Portions Copyright (c) 1996-2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/utils/snapshot.h,v 1.2 2008/03/26 21:10:39 alvherre Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "access/htup.h"
#include "storage/buf.h"


typedef struct SnapshotData *Snapshot;

#define InvalidSnapshot		((Snapshot) NULL)

/*
 * We use SnapshotData structures to represent both "regular" (MVCC)
 * snapshots and "special" snapshots that have non-MVCC semantics.
 * The specific semantics of a snapshot are encoded by the "satisfies"
 * function.
 */
typedef bool (*SnapshotSatisfiesFunc) (HeapTupleHeader tuple,
									   Snapshot snapshot, Buffer buffer);

typedef struct SnapshotData
{
	SnapshotSatisfiesFunc satisfies;	/* tuple test function */

	/*
	 * The remaining fields are used only for MVCC snapshots, and are normally
	 * just zeroes in special snapshots.  (But xmin and xmax are used
	 * specially by HeapTupleSatisfiesDirty.)
	 *
	 * An MVCC snapshot can never see the effects of XIDs >= xmax. It can see
	 * the effects of all older XIDs except those listed in the snapshot. xmin
	 * is stored as an optimization to avoid needing to search the XID arrays
	 * for most tuples.
	 */
	TransactionId xmin;			/* all XID < xmin are visible to me */
	TransactionId xmax;			/* all XID >= xmax are invisible to me */
	uint32		xcnt;			/* # of xact ids in xip[] */
	TransactionId *xip;			/* array of xact IDs in progress */
	/* note: all ids in xip[] satisfy xmin <= xip[i] < xmax */
	int32		subxcnt;		/* # of xact ids in subxip[], -1 if overflow */
	TransactionId *subxip;		/* array of subxact IDs in progress */

	/*
	 * note: all ids in subxip[] are >= xmin, but we don't bother filtering
	 * out any that are >= xmax
	 */
	CommandId	curcid;			/* in my xact, CID < curcid are visible */
} SnapshotData;

/*
 * Result codes for HeapTupleSatisfiesUpdate.  This should really be in
 * tqual.h, but we want to avoid including that file elsewhere.
 */
typedef enum
{
	HeapTupleMayBeUpdated,
	HeapTupleInvisible,
	HeapTupleSelfUpdated,
	HeapTupleUpdated,
	HeapTupleBeingUpdated
} HTSU_Result;

#endif /* SNAPSHOT_H */
