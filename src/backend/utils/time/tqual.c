/*-------------------------------------------------------------------------
 *
 * tqual.c
 *	  POSTGRES "time" qualification code.
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/time/tqual.c,v 1.40 2001-08-23 23:06:38 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "storage/sinval.h"
#include "utils/tqual.h"


static SnapshotData SnapshotDirtyData;
Snapshot	SnapshotDirty = &SnapshotDirtyData;

Snapshot	QuerySnapshot = NULL;
Snapshot	SerializableSnapshot = NULL;

bool		ReferentialIntegritySnapshotOverride = false;


/*
 * HeapTupleSatisfiesItself
 *		True iff heap tuple is valid for "itself."
 *		"{it}self" means valid as of everything that's happened
 *		in the current transaction, _including_ the current command.
 *
 * Note:
 *		Assumes heap tuple is valid.
 */
/*
 * The satisfaction of "itself" requires the following:
 *
 * ((Xmin == my-transaction &&				the row was updated by the current transaction, and
 *		(Xmax is null						it was not deleted
 *		 [|| Xmax != my-transaction)])			[or it was deleted by another transaction]
 * ||
 *
 * (Xmin is committed &&					the row was modified by a committed transaction, and
 *		(Xmax is null ||					the row has not been deleted, or
 *			(Xmax != my-transaction &&			the row was deleted by another transaction
 *			 Xmax is not committed)))			that has not been committed
 */
bool
HeapTupleSatisfiesItself(HeapTupleHeader tuple)
{

	if (!(tuple->t_infomask & HEAP_XMIN_COMMITTED))
	{
		if (tuple->t_infomask & HEAP_XMIN_INVALID)
			return false;

		if (tuple->t_infomask & HEAP_MOVED_OFF)
		{
			if (TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return false;
			}
		}
		else if (tuple->t_infomask & HEAP_MOVED_IN)
		{
			if (!TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return false;
			}
		}
		else if (TransactionIdIsCurrentTransactionId(tuple->t_xmin))
		{
			if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid */
				return true;
			if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
				return true;
			return false;
		}
		else if (!TransactionIdDidCommit(tuple->t_xmin))
		{
			if (TransactionIdDidAbort(tuple->t_xmin))
				tuple->t_infomask |= HEAP_XMIN_INVALID; /* aborted */
			return false;
		}
		tuple->t_infomask |= HEAP_XMIN_COMMITTED;
	}
	/* the tuple was inserted validly */

	if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid or aborted */
		return true;

	if (tuple->t_infomask & HEAP_XMAX_COMMITTED)
	{
		if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return true;
		return false;			/* updated by other */
	}

	if (TransactionIdIsCurrentTransactionId(tuple->t_xmax))
	{
		if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return true;
		return false;
	}

	if (!TransactionIdDidCommit(tuple->t_xmax))
	{
		if (TransactionIdDidAbort(tuple->t_xmax))
			tuple->t_infomask |= HEAP_XMAX_INVALID;		/* aborted */
		return true;
	}

	/* by here, deleting transaction has committed */
	tuple->t_infomask |= HEAP_XMAX_COMMITTED;

	if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
		return true;

	return false;
}

/*
 * HeapTupleSatisfiesNow
 *		True iff heap tuple is valid "now."
 *		"now" means valid including everything that's happened
 *		 in the current transaction _up to, but not including,_
 *		 the current command.
 *
 * Note:
 *		Assumes heap tuple is valid.
 */
/*
 * The satisfaction of "now" requires the following:
 *
 * ((Xmin == my-transaction &&				changed by the current transaction
 *	 Cmin != my-command &&					but not by this command, and
 *		(Xmax is null ||						the row has not been deleted, or
 *			(Xmax == my-transaction &&			it was deleted by the current transaction
 *			 Cmax != my-command)))				but not by this command,
 * ||										or
 *
 *	(Xmin is committed &&					the row was modified by a committed transaction, and
 *		(Xmax is null ||					the row has not been deleted, or
 *			(Xmax == my-transaction &&			the row is being deleted by this command, or
 *			 Cmax == my-command) ||
 *			(Xmax is not committed &&			the row was deleted by another transaction
 *			 Xmax != my-transaction))))			that has not been committed
 *
 *		mao says 17 march 1993:  the tests in this routine are correct;
 *		if you think they're not, you're wrong, and you should think
 *		about it again.  i know, it happened to me.  we don't need to
 *		check commit time against the start time of this transaction
 *		because 2ph locking protects us from doing the wrong thing.
 *		if you mess around here, you'll break serializability.  the only
 *		problem with this code is that it does the wrong thing for system
 *		catalog updates, because the catalogs aren't subject to 2ph, so
 *		the serializability guarantees we provide don't extend to xacts
 *		that do catalog accesses.  this is unfortunate, but not critical.
 */
bool
HeapTupleSatisfiesNow(HeapTupleHeader tuple)
{
	if (AMI_OVERRIDE)
		return true;

	if (!(tuple->t_infomask & HEAP_XMIN_COMMITTED))
	{
		if (tuple->t_infomask & HEAP_XMIN_INVALID)
			return false;

		if (tuple->t_infomask & HEAP_MOVED_OFF)
		{
			if (TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return false;
			}
		}
		else if (tuple->t_infomask & HEAP_MOVED_IN)
		{
			if (!TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return false;
			}
		}
		else if (TransactionIdIsCurrentTransactionId(tuple->t_xmin))
		{
			if (CommandIdGEScanCommandId(tuple->t_cmin))
				return false;	/* inserted after scan started */

			if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid */
				return true;

			Assert(TransactionIdIsCurrentTransactionId(tuple->t_xmax));

			if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
				return true;

			if (CommandIdGEScanCommandId(tuple->t_cmax))
				return true;	/* deleted after scan started */
			else
				return false;	/* deleted before scan started */
		}
		else if (!TransactionIdDidCommit(tuple->t_xmin))
		{
			if (TransactionIdDidAbort(tuple->t_xmin))
				tuple->t_infomask |= HEAP_XMIN_INVALID; /* aborted */
			return false;
		}
		tuple->t_infomask |= HEAP_XMIN_COMMITTED;
	}

	/* by here, the inserting transaction has committed */

	if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid or aborted */
		return true;

	if (tuple->t_infomask & HEAP_XMAX_COMMITTED)
	{
		if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return true;
		return false;
	}

	if (TransactionIdIsCurrentTransactionId(tuple->t_xmax))
	{
		if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return true;
		if (CommandIdGEScanCommandId(tuple->t_cmax))
			return true;		/* deleted after scan started */
		else
			return false;		/* deleted before scan started */
	}

	if (!TransactionIdDidCommit(tuple->t_xmax))
	{
		if (TransactionIdDidAbort(tuple->t_xmax))
			tuple->t_infomask |= HEAP_XMAX_INVALID;		/* aborted */
		return true;
	}

	/* xmax transaction committed */
	tuple->t_infomask |= HEAP_XMAX_COMMITTED;

	if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
		return true;

	return false;
}

int
HeapTupleSatisfiesUpdate(HeapTuple tuple)
{
	HeapTupleHeader th = tuple->t_data;

	if (AMI_OVERRIDE)
		return HeapTupleMayBeUpdated;

	if (!(th->t_infomask & HEAP_XMIN_COMMITTED))
	{
		if (th->t_infomask & HEAP_XMIN_INVALID) /* xid invalid or aborted */
			return HeapTupleInvisible;

		if (th->t_infomask & HEAP_MOVED_OFF)
		{
			if (TransactionIdDidCommit((TransactionId) th->t_cmin))
			{
				th->t_infomask |= HEAP_XMIN_INVALID;
				return HeapTupleInvisible;
			}
		}
		else if (th->t_infomask & HEAP_MOVED_IN)
		{
			if (!TransactionIdDidCommit((TransactionId) th->t_cmin))
			{
				th->t_infomask |= HEAP_XMIN_INVALID;
				return HeapTupleInvisible;
			}
		}
		else if (TransactionIdIsCurrentTransactionId(th->t_xmin))
		{
			if (CommandIdGEScanCommandId(th->t_cmin))
				return HeapTupleInvisible;		/* inserted after scan
												 * started */

			if (th->t_infomask & HEAP_XMAX_INVALID)		/* xid invalid */
				return HeapTupleMayBeUpdated;

			Assert(TransactionIdIsCurrentTransactionId(th->t_xmax));

			if (th->t_infomask & HEAP_MARKED_FOR_UPDATE)
				return HeapTupleMayBeUpdated;

			if (CommandIdGEScanCommandId(th->t_cmax))
				return HeapTupleSelfUpdated;	/* updated after scan
												 * started */
			else
				return HeapTupleInvisible;		/* updated before scan
												 * started */
		}
		else if (!TransactionIdDidCommit(th->t_xmin))
		{
			if (TransactionIdDidAbort(th->t_xmin))
				th->t_infomask |= HEAP_XMIN_INVALID;	/* aborted */
			return HeapTupleInvisible;
		}
		th->t_infomask |= HEAP_XMIN_COMMITTED;
	}

	/* by here, the inserting transaction has committed */

	if (th->t_infomask & HEAP_XMAX_INVALID)		/* xid invalid or aborted */
		return HeapTupleMayBeUpdated;

	if (th->t_infomask & HEAP_XMAX_COMMITTED)
	{
		if (th->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return HeapTupleMayBeUpdated;
		return HeapTupleUpdated;/* updated by other */
	}

	if (TransactionIdIsCurrentTransactionId(th->t_xmax))
	{
		if (th->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return HeapTupleMayBeUpdated;
		if (CommandIdGEScanCommandId(th->t_cmax))
			return HeapTupleSelfUpdated;		/* updated after scan
												 * started */
		else
			return HeapTupleInvisible;	/* updated before scan started */
	}

	if (!TransactionIdDidCommit(th->t_xmax))
	{
		if (TransactionIdDidAbort(th->t_xmax))
		{
			th->t_infomask |= HEAP_XMAX_INVALID;		/* aborted */
			return HeapTupleMayBeUpdated;
		}
		/* running xact */
		return HeapTupleBeingUpdated;	/* in updation by other */
	}

	/* xmax transaction committed */
	th->t_infomask |= HEAP_XMAX_COMMITTED;

	if (th->t_infomask & HEAP_MARKED_FOR_UPDATE)
		return HeapTupleMayBeUpdated;

	return HeapTupleUpdated;	/* updated by other */
}

bool
HeapTupleSatisfiesDirty(HeapTupleHeader tuple)
{
	SnapshotDirty->xmin = SnapshotDirty->xmax = InvalidTransactionId;
	ItemPointerSetInvalid(&(SnapshotDirty->tid));

	if (AMI_OVERRIDE)
		return true;

	if (!(tuple->t_infomask & HEAP_XMIN_COMMITTED))
	{
		if (tuple->t_infomask & HEAP_XMIN_INVALID)
			return false;

		if (tuple->t_infomask & HEAP_MOVED_OFF)
		{

			/*
			 * HeapTupleSatisfiesDirty is used by unique btree-s and so
			 * may be used while vacuuming.
			 */
			if (TransactionIdIsCurrentTransactionId((TransactionId) tuple->t_cmin))
				return false;
			if (TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return false;
			}
			tuple->t_infomask |= HEAP_XMIN_COMMITTED;
		}
		else if (tuple->t_infomask & HEAP_MOVED_IN)
		{
			if (!TransactionIdIsCurrentTransactionId((TransactionId) tuple->t_cmin))
			{
				if (TransactionIdDidCommit((TransactionId) tuple->t_cmin))
					tuple->t_infomask |= HEAP_XMIN_COMMITTED;
				else
				{
					tuple->t_infomask |= HEAP_XMIN_INVALID;
					return false;
				}
			}
		}
		else if (TransactionIdIsCurrentTransactionId(tuple->t_xmin))
		{
			if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid */
				return true;

			Assert(TransactionIdIsCurrentTransactionId(tuple->t_xmax));

			if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
				return true;

			return false;
		}
		else if (!TransactionIdDidCommit(tuple->t_xmin))
		{
			if (TransactionIdDidAbort(tuple->t_xmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID; /* aborted */
				return false;
			}
			SnapshotDirty->xmin = tuple->t_xmin;
			return true;		/* in insertion by other */
		}
		else
			tuple->t_infomask |= HEAP_XMIN_COMMITTED;
	}

	/* by here, the inserting transaction has committed */

	if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid or aborted */
		return true;

	if (tuple->t_infomask & HEAP_XMAX_COMMITTED)
	{
		if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return true;
		SnapshotDirty->tid = tuple->t_ctid;
		return false;			/* updated by other */
	}

	if (TransactionIdIsCurrentTransactionId(tuple->t_xmax))
	{
		if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
			return true;
		return false;
	}

	if (!TransactionIdDidCommit(tuple->t_xmax))
	{
		if (TransactionIdDidAbort(tuple->t_xmax))
		{
			tuple->t_infomask |= HEAP_XMAX_INVALID;		/* aborted */
			return true;
		}
		/* running xact */
		SnapshotDirty->xmax = tuple->t_xmax;
		return true;			/* in updation by other */
	}

	/* xmax transaction committed */
	tuple->t_infomask |= HEAP_XMAX_COMMITTED;

	if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
		return true;

	SnapshotDirty->tid = tuple->t_ctid;
	return false;				/* updated by other */
}

bool
HeapTupleSatisfiesSnapshot(HeapTupleHeader tuple, Snapshot snapshot)
{
	if (AMI_OVERRIDE)
		return true;

	if (ReferentialIntegritySnapshotOverride)
		return HeapTupleSatisfiesNow(tuple);

	if (!(tuple->t_infomask & HEAP_XMIN_COMMITTED))
	{
		if (tuple->t_infomask & HEAP_XMIN_INVALID)
			return false;

		if (tuple->t_infomask & HEAP_MOVED_OFF)
		{
			if (TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return false;
			}
		}
		else if (tuple->t_infomask & HEAP_MOVED_IN)
		{
			if (!TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return false;
			}
		}
		else if (TransactionIdIsCurrentTransactionId(tuple->t_xmin))
		{
			if (CommandIdGEScanCommandId(tuple->t_cmin))
				return false;	/* inserted after scan started */

			if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid */
				return true;

			Assert(TransactionIdIsCurrentTransactionId(tuple->t_xmax));

			if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
				return true;

			if (CommandIdGEScanCommandId(tuple->t_cmax))
				return true;	/* deleted after scan started */
			else
				return false;	/* deleted before scan started */
		}
		else if (!TransactionIdDidCommit(tuple->t_xmin))
		{
			if (TransactionIdDidAbort(tuple->t_xmin))
				tuple->t_infomask |= HEAP_XMIN_INVALID; /* aborted */
			return false;
		}
		tuple->t_infomask |= HEAP_XMIN_COMMITTED;
	}

	/*
	 * By here, the inserting transaction has committed - have to check
	 * when...
	 */

	if (TransactionIdFollowsOrEquals(tuple->t_xmin, snapshot->xmax))
		return false;
	if (TransactionIdFollowsOrEquals(tuple->t_xmin, snapshot->xmin))
	{
		uint32		i;

		for (i = 0; i < snapshot->xcnt; i++)
		{
			if (TransactionIdEquals(tuple->t_xmin, snapshot->xip[i]))
				return false;
		}
	}

	if (tuple->t_infomask & HEAP_XMAX_INVALID)	/* xid invalid or aborted */
		return true;

	if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
		return true;

	if (!(tuple->t_infomask & HEAP_XMAX_COMMITTED))
	{
		if (TransactionIdIsCurrentTransactionId(tuple->t_xmax))
		{
			if (CommandIdGEScanCommandId(tuple->t_cmax))
				return true;	/* deleted after scan started */
			else
				return false;	/* deleted before scan started */
		}

		if (!TransactionIdDidCommit(tuple->t_xmax))
		{
			if (TransactionIdDidAbort(tuple->t_xmax))
				tuple->t_infomask |= HEAP_XMAX_INVALID; /* aborted */
			return true;
		}

		/* xmax transaction committed */
		tuple->t_infomask |= HEAP_XMAX_COMMITTED;
	}

	if (TransactionIdFollowsOrEquals(tuple->t_xmax, snapshot->xmax))
		return true;
	if (TransactionIdFollowsOrEquals(tuple->t_xmax, snapshot->xmin))
	{
		uint32		i;

		for (i = 0; i < snapshot->xcnt; i++)
		{
			if (TransactionIdEquals(tuple->t_xmax, snapshot->xip[i]))
				return true;
		}
	}

	return false;
}


/*
 * HeapTupleSatisfiesVacuum - determine tuple status for VACUUM and related
 *		operations
 *
 * XmaxRecent is a cutoff XID (obtained from GetXmaxRecent()).  Tuples
 * deleted by XIDs >= XmaxRecent are deemed "recently dead"; they might
 * still be visible to some open transaction, so we can't remove them,
 * even if we see that the deleting transaction has committed.
 *
 * As with the other HeapTupleSatisfies routines, we may update the tuple's
 * "hint" status bits if we see that the inserting or deleting transaction
 * has now committed or aborted.  The caller is responsible for noticing any
 * change in t_infomask and scheduling a disk write if so.
 */
HTSV_Result
HeapTupleSatisfiesVacuum(HeapTupleHeader tuple, TransactionId XmaxRecent)
{
	/*
	 * Has inserting transaction committed?
	 *
	 * If the inserting transaction aborted, then the tuple was never visible
	 * to any other transaction, so we can delete it immediately.
	 *
	 * NOTE: must check TransactionIdIsInProgress (which looks in shared mem)
	 * before TransactionIdDidCommit/TransactionIdDidAbort (which look in
	 * pg_log).  Otherwise we have a race condition where we might decide
	 * that a just-committed transaction crashed, because none of the tests
	 * succeed.  xact.c is careful to record commit/abort in pg_log before
	 * it unsets MyProc->xid in shared memory.
	 */
	if (!(tuple->t_infomask & HEAP_XMIN_COMMITTED))
	{
		if (tuple->t_infomask & HEAP_XMIN_INVALID)
			return HEAPTUPLE_DEAD;
		else if (tuple->t_infomask & HEAP_MOVED_OFF)
		{
			if (TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return HEAPTUPLE_DEAD;
			}
			/* Assume we can only get here if previous VACUUM aborted, */
			/* ie, it couldn't still be in progress */
			tuple->t_infomask |= HEAP_XMIN_COMMITTED;
		}
		else if (tuple->t_infomask & HEAP_MOVED_IN)
		{
			if (!TransactionIdDidCommit((TransactionId) tuple->t_cmin))
			{
				/* Assume we can only get here if previous VACUUM aborted */
				tuple->t_infomask |= HEAP_XMIN_INVALID;
				return HEAPTUPLE_DEAD;
			}
			tuple->t_infomask |= HEAP_XMIN_COMMITTED;
		}
		else if (TransactionIdIsInProgress(tuple->t_xmin))
			return HEAPTUPLE_INSERT_IN_PROGRESS;
		else if (TransactionIdDidCommit(tuple->t_xmin))
			tuple->t_infomask |= HEAP_XMIN_COMMITTED;
		else if (TransactionIdDidAbort(tuple->t_xmin))
		{
			tuple->t_infomask |= HEAP_XMIN_INVALID;
			return HEAPTUPLE_DEAD;
		}
		else
		{
			/*
			 * Not in Progress, Not Committed, Not Aborted -
			 * so it's from crashed process. - vadim 11/26/96
			 */
			tuple->t_infomask |= HEAP_XMIN_INVALID;
			return HEAPTUPLE_DEAD;
		}
		/* Should only get here if we set XMIN_COMMITTED */
		Assert(tuple->t_infomask & HEAP_XMIN_COMMITTED);
	}

	/*
	 * Okay, the inserter committed, so it was good at some point.  Now
	 * what about the deleting transaction?
	 */
	if (tuple->t_infomask & HEAP_XMAX_INVALID)
		return HEAPTUPLE_LIVE;

	if (!(tuple->t_infomask & HEAP_XMAX_COMMITTED))
	{
		if (TransactionIdIsInProgress(tuple->t_xmax))
			return HEAPTUPLE_DELETE_IN_PROGRESS;
		else if (TransactionIdDidCommit(tuple->t_xmax))
			tuple->t_infomask |= HEAP_XMAX_COMMITTED;
		else if (TransactionIdDidAbort(tuple->t_xmax))
		{
			tuple->t_infomask |= HEAP_XMAX_INVALID;
			return HEAPTUPLE_LIVE;
		}
		else
		{
			/*
			 * Not in Progress, Not Committed, Not Aborted -
			 * so it's from crashed process. - vadim 06/02/97
			 */
			tuple->t_infomask |= HEAP_XMAX_INVALID;
			return HEAPTUPLE_LIVE;
		}
		/* Should only get here if we set XMAX_COMMITTED */
		Assert(tuple->t_infomask & HEAP_XMAX_COMMITTED);
	}

	/*
	 * Deleter committed, but check special cases.
	 */

	if (tuple->t_infomask & HEAP_MARKED_FOR_UPDATE)
	{
		/* "deleting" xact really only marked it for update */
		return HEAPTUPLE_LIVE;
	}

	if (TransactionIdEquals(tuple->t_xmin, tuple->t_xmax))
	{
		/* inserter also deleted it, so it was never visible to anyone else */
		return HEAPTUPLE_DEAD;
	}

	if (!TransactionIdPrecedes(tuple->t_xmax, XmaxRecent))
	{
		/* deleting xact is too recent, tuple could still be visible */
		return HEAPTUPLE_RECENTLY_DEAD;
	}

	/* Otherwise, it's dead and removable */
	return HEAPTUPLE_DEAD;
}


void
SetQuerySnapshot(void)
{
	/* Initialize snapshot overriding to false */
	ReferentialIntegritySnapshotOverride = false;

	/* 1st call in xaction */
	if (SerializableSnapshot == NULL)
	{
		SerializableSnapshot = GetSnapshotData(true);
		QuerySnapshot = SerializableSnapshot;
		Assert(QuerySnapshot != NULL);
		return;
	}

	if (QuerySnapshot != SerializableSnapshot)
	{
		free(QuerySnapshot->xip);
		free(QuerySnapshot);
	}

	if (XactIsoLevel == XACT_SERIALIZABLE)
		QuerySnapshot = SerializableSnapshot;
	else
		QuerySnapshot = GetSnapshotData(false);

	Assert(QuerySnapshot != NULL);
}

void
FreeXactSnapshot(void)
{
	if (QuerySnapshot != NULL && QuerySnapshot != SerializableSnapshot)
	{
		free(QuerySnapshot->xip);
		free(QuerySnapshot);
	}

	QuerySnapshot = NULL;

	if (SerializableSnapshot != NULL)
	{
		free(SerializableSnapshot->xip);
		free(SerializableSnapshot);
	}

	SerializableSnapshot = NULL;
}
