/*-------------------------------------------------------------------------
 *
 * transam.c
 *	  postgres transaction log/time interface routines
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/transam/transam.c,v 1.39 2001-01-24 19:42:51 momjian Exp $
 *
 * NOTES
 *	  This file contains the high level access-method interface to the
 *	  transaction system.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/transam.h"
#include "catalog/catname.h"
#include "miscadmin.h"

static int	RecoveryCheckingEnabled(void);
static void TransRecover(Relation logRelation);
static bool TransactionLogTest(TransactionId transactionId, XidStatus status);
static void TransactionLogUpdate(TransactionId transactionId,
					 XidStatus status);

/* ----------------
 *	  global variables holding pointers to relations used
 *	  by the transaction system.  These are initialized by
 *	  InitializeTransactionLog().
 * ----------------
 */

Relation	LogRelation = (Relation) NULL;
Relation	VariableRelation = (Relation) NULL;

/* ----------------
 *		global variables holding cached transaction id's and statuses.
 * ----------------
 */
TransactionId cachedTestXid;
XidStatus	cachedTestXidStatus;

/* ----------------
 *		transaction system constants
 * ----------------
 */
/* ----------------------------------------------------------------
 *		transaction system constants
 *
 *		read the comments for GetNewTransactionId in order to
 *		understand the initial values for AmiTransactionId and
 *		FirstTransactionId. -cim 3/23/90
 * ----------------------------------------------------------------
 */
TransactionId NullTransactionId = (TransactionId) 0;

TransactionId AmiTransactionId = (TransactionId) 512;

TransactionId FirstTransactionId = (TransactionId) 514;

/* ----------------
 *		transaction recovery state variables
 *
 *		When the transaction system is initialized, we may
 *		need to do recovery checking.  This decision is decided
 *		by the postmaster or the user by supplying the backend
 *		with a special flag.  In general, we want to do recovery
 *		checking whenever we are running without a postmaster
 *		or when the number of backends running under the postmaster
 *		goes from zero to one. -cim 3/21/90
 * ----------------
 */
int			RecoveryCheckingEnableState = 0;

/* ----------------
 *		recovery checking accessors
 * ----------------
 */
static int
RecoveryCheckingEnabled(void)
{
	return RecoveryCheckingEnableState;
}

#ifdef NOT_USED
static void
SetRecoveryCheckingEnabled(bool state)
{
	RecoveryCheckingEnableState = (state == true);
}

#endif

/* ----------------------------------------------------------------
 *		postgres log access method interface
 *
 *		TransactionLogTest
 *		TransactionLogUpdate
 *		========
 *		   these functions do work for the interface
 *		   functions - they search/retrieve and append/update
 *		   information in the log and time relations.
 * ----------------------------------------------------------------
 */

/* --------------------------------
 *		TransactionLogTest
 * --------------------------------
 */

static bool						/* true/false: does transaction id have
								 * specified status? */
TransactionLogTest(TransactionId transactionId, /* transaction id to test */
				   XidStatus status)	/* transaction status */
{
	BlockNumber blockNumber;
	XidStatus	xidstatus;		/* recorded status of xid */
	bool		fail = false;	/* success/failure */

	/* ----------------
	 *	during initialization consider all transactions
	 *	as having been committed
	 * ----------------
	 */
	if (!RelationIsValid(LogRelation))
		return (bool) (status == XID_COMMIT);

	/* ----------------
	 *	 before going to the buffer manager, check our single
	 *	 item cache to see if we didn't just check the transaction
	 *	 status a moment ago.
	 * ----------------
	 */
	if (TransactionIdEquals(transactionId, cachedTestXid))
		return (bool)
			(status == cachedTestXidStatus);

	/* ----------------
	 *	compute the item pointer corresponding to the
	 *	page containing our transaction id.  We save the item in
	 *	our cache to speed up things if we happen to ask for the
	 *	same xid's status more than once.
	 * ----------------
	 */
	TransComputeBlockNumber(LogRelation, transactionId, &blockNumber);
	xidstatus = TransBlockNumberGetXidStatus(LogRelation,
											 blockNumber,
											 transactionId,
											 &fail);

	if (!fail)
	{

		/*
		 * DO NOT cache status for transactions in unknown state !!!
		 */
		if (xidstatus == XID_COMMIT || xidstatus == XID_ABORT)
		{
			TransactionIdStore(transactionId, &cachedTestXid);
			cachedTestXidStatus = xidstatus;
		}
		return (bool) (status == xidstatus);
	}

	/* ----------------
	 *	  here the block didn't contain the information we wanted
	 * ----------------
	 */
	elog(ERROR, "TransactionLogTest: failed to get xidstatus");

	/*
	 * so lint is happy...
	 */
	return false;
}

/* --------------------------------
 *		TransactionLogUpdate
 * --------------------------------
 */
static void
TransactionLogUpdate(TransactionId transactionId,		/* trans id to update */
					 XidStatus status)	/* new trans status */
{
	BlockNumber blockNumber;
	bool		fail = false;	/* success/failure */

	/* ----------------
	 *	during initialization we don't record any updates.
	 * ----------------
	 */
	if (!RelationIsValid(LogRelation))
		return;

	/* ----------------
	 *	update the log relation
	 * ----------------
	 */
	TransComputeBlockNumber(LogRelation, transactionId, &blockNumber);
	TransBlockNumberSetXidStatus(LogRelation,
								 blockNumber,
								 transactionId,
								 status,
								 &fail);

	/*
	 * update (invalidate) our single item TransactionLogTest cache.
	 *
	 * if (status != XID_COMMIT)
	 *
	 * What's the hell ?! Why != XID_COMMIT ?!
	 */
	TransactionIdStore(transactionId, &cachedTestXid);
	cachedTestXidStatus = status;

}

/* ----------------------------------------------------------------
 *					 transaction recovery code
 * ----------------------------------------------------------------
 */

/* --------------------------------
 *		TransRecover
 *
 *		preform transaction recovery checking.
 *
 *		Note: this should only be preformed if no other backends
 *			  are running.	This is known by the postmaster and
 *			  conveyed by the postmaster passing a "do recovery checking"
 *			  flag to the backend.
 *
 *		here we get the last recorded transaction from the log,
 *		get the "last" and "next" transactions from the variable relation
 *		and then preform some integrity tests:
 *
 *		1) No transaction may exist higher then the "next" available
 *		   transaction recorded in the variable relation.  If this is the
 *		   case then it means either the log or the variable relation
 *		   has become corrupted.
 *
 *		2) The last committed transaction may not be higher then the
 *		   next available transaction for the same reason.
 *
 *		3) The last recorded transaction may not be lower then the
 *		   last committed transaction.	(the reverse is ok - it means
 *		   that some transactions have aborted since the last commit)
 *
 *		Here is what the proper situation looks like.  The line
 *		represents the data stored in the log.	'c' indicates the
 *		transaction was recorded as committed, 'a' indicates an
 *		abortted transaction and '.' represents information not
 *		recorded.  These may correspond to in progress transactions.
 *
 *			 c	c  a  c  .	.  a  .  .	.  .  .  .	.  .  .  .
 *					  |					|
 *					 last			   next
 *
 *		Since "next" is only incremented by GetNewTransactionId() which
 *		is called when transactions are started.  Hence if there
 *		are commits or aborts after "next", then it means we committed
 *		or aborted BEFORE we started the transaction.  This is the
 *		rational behind constraint (1).
 *
 *		Likewise, "last" should never greater then "next" for essentially
 *		the same reason - it would imply we committed before we started.
 *		This is the reasoning for (2).
 *
 *		(3) implies we may never have a situation such as:
 *
 *			 c	c  a  c  .	.  a  c  .	.  .  .  .	.  .  .  .
 *					  |					|
 *					 last			   next
 *
 *		where there is a 'c' greater then "last".
 *
 *		Recovery checking is more difficult in the case where
 *		several backends are executing concurrently because the
 *		transactions may be executing in the other backends.
 *		So, we only do recovery stuff when the backend is explicitly
 *		passed a flag on the command line.
 * --------------------------------
 */
static void
TransRecover(Relation logRelation)
{
#ifdef NOT_USED
	/* ----------------
	 *	  first get the last recorded transaction in the log.
	 * ----------------
	 */
	TransGetLastRecordedTransaction(logRelation, logLastXid, &fail);
	if (fail == true)
		elog(ERROR, "TransRecover: failed TransGetLastRecordedTransaction");

	/* ----------------
	 *	  next get the "last" and "next" variables
	 * ----------------
	 */
	VariableRelationGetLastXid(&varLastXid);
	VariableRelationGetNextXid(&varNextXid);

	/* ----------------
	 *	  intregity test (1)
	 * ----------------
	 */
	if (TransactionIdIsLessThan(varNextXid, logLastXid))
		elog(ERROR, "TransRecover: varNextXid < logLastXid");

	/* ----------------
	 *	  intregity test (2)
	 * ----------------
	 */

	/* ----------------
	 *	  intregity test (3)
	 * ----------------
	 */

	/* ----------------
	 *	here we have a valid "
	 *
	 *			**** RESUME HERE ****
	 * ----------------
	 */
	varNextXid = TransactionIdDup(varLastXid);
	TransactionIdIncrement(&varNextXid);

	VarPut(var, VAR_PUT_LASTXID, varLastXid);
	VarPut(var, VAR_PUT_NEXTXID, varNextXid);
#endif
}

/* ----------------------------------------------------------------
 *						Interface functions
 *
 *		InitializeTransactionLog
 *		========
 *		   this function (called near cinit) initializes
 *		   the transaction log, time and variable relations.
 *
 *		TransactionId DidCommit
 *		TransactionId DidAbort
 *		TransactionId IsInProgress
 *		========
 *		   these functions test the transaction status of
 *		   a specified transaction id.
 *
 *		TransactionId Commit
 *		TransactionId Abort
 *		TransactionId SetInProgress
 *		========
 *		   these functions set the transaction status
 *		   of the specified xid. TransactionIdCommit() also
 *		   records the current time in the time relation
 *		   and updates the variable relation counter.
 *
 * ----------------------------------------------------------------
 */

/*
 * InitializeTransactionLog
 *		Initializes transaction logging.
 */
void
InitializeTransactionLog(void)
{
	Relation	logRelation;
	MemoryContext oldContext;

	/* ----------------
	 *	  don't do anything during bootstrapping
	 * ----------------
	 */
	if (AMI_OVERRIDE)
		return;

	/* ----------------
	 *	 disable the transaction system so the access methods
	 *	 don't interfere during initialization.
	 * ----------------
	 */
	OverrideTransactionSystem(true);

	/* ----------------
	 *	make sure allocations occur within the top memory context
	 *	so that our log management structures are protected from
	 *	garbage collection at the end of every transaction.
	 * ----------------
	 */
	oldContext = MemoryContextSwitchTo(TopMemoryContext);

	/* ----------------
	 *	 first open the log and time relations
	 *	 (these are created by amiint so they are guaranteed to exist)
	 * ----------------
	 */
	logRelation = heap_openr(LogRelationName, NoLock);
	VariableRelation = heap_openr(VariableRelationName, NoLock);

	/* ----------------
	 *	 XXX TransactionLogUpdate requires that LogRelation
	 *	 is valid so we temporarily set it so we can initialize
	 *	 things properly. This could be done cleaner.
	 * ----------------
	 */
	LogRelation = logRelation;

	/* ----------------
	 *	 if we have a virgin database, we initialize the log
	 *	 relation by committing the AmiTransactionId (id 512) and we
	 *	 initialize the variable relation by setting the next available
	 *	 transaction id to FirstTransactionId (id 514).  OID initialization
	 *	 happens as a side effect of bootstrapping in varsup.c.
	 * ----------------
	 */
	SpinAcquire(OidGenLockId);
	if (!TransactionIdDidCommit(AmiTransactionId))
	{
		TransactionLogUpdate(AmiTransactionId, XID_COMMIT);
		TransactionIdStore(AmiTransactionId, &cachedTestXid);
		cachedTestXidStatus = XID_COMMIT;
		Assert(!IsUnderPostmaster && 
				ShmemVariableCache->nextXid <= FirstTransactionId);
		ShmemVariableCache->nextXid = FirstTransactionId;
	}
	else if (RecoveryCheckingEnabled())
	{
		/* ----------------
		 *		if we have a pre-initialized database and if the
		 *		perform recovery checking flag was passed then we
		 *		do our database integrity checking.
		 * ----------------
		 */
		TransRecover(logRelation);
	}
	LogRelation = (Relation) NULL;
	SpinRelease(OidGenLockId);

	/* ----------------
	 *	now re-enable the transaction system
	 * ----------------
	 */
	OverrideTransactionSystem(false);

	/* ----------------
	 *	instantiate the global variables
	 * ----------------
	 */
	LogRelation = logRelation;

	/* ----------------
	 *	restore the memory context to the previous context
	 *	before we return from initialization.
	 * ----------------
	 */
	MemoryContextSwitchTo(oldContext);
}

/* --------------------------------
 *		TransactionId DidCommit
 *		TransactionId DidAbort
 *		TransactionId IsInProgress
 * --------------------------------
 */

/*
 * TransactionIdDidCommit
 *		True iff transaction associated with the identifier did commit.
 *
 * Note:
 *		Assumes transaction identifier is valid.
 */
bool							/* true if given transaction committed */
TransactionIdDidCommit(TransactionId transactionId)
{
	if (AMI_OVERRIDE)
		return true;

	return TransactionLogTest(transactionId, XID_COMMIT);
}

/*
 * TransactionIdDidAborted
 *		True iff transaction associated with the identifier did abort.
 *
 * Note:
 *		Assumes transaction identifier is valid.
 *		XXX Is this unneeded?
 */
bool							/* true if given transaction aborted */
TransactionIdDidAbort(TransactionId transactionId)
{
	if (AMI_OVERRIDE)
		return false;

	return TransactionLogTest(transactionId, XID_ABORT);
}

/*
 * Now this func in shmem.c and gives quality answer by scanning
 * PROC structures of all running backend. - vadim 11/26/96
 *
 * Old comments:
 * true if given transaction neither committed nor aborted

bool
TransactionIdIsInProgress(TransactionId transactionId)
{
	if (AMI_OVERRIDE)
		return false;

	return TransactionLogTest(transactionId, XID_INPROGRESS);
}
 */

/* --------------------------------
 *		TransactionId Commit
 *		TransactionId Abort
 *		TransactionId SetInProgress
 * --------------------------------
 */

/*
 * TransactionIdCommit
 *		Commits the transaction associated with the identifier.
 *
 * Note:
 *		Assumes transaction identifier is valid.
 */
void
TransactionIdCommit(TransactionId transactionId)
{
	if (AMI_OVERRIDE)
		return;

	TransactionLogUpdate(transactionId, XID_COMMIT);
}

/*
 * TransactionIdAbort
 *		Aborts the transaction associated with the identifier.
 *
 * Note:
 *		Assumes transaction identifier is valid.
 */
void
TransactionIdAbort(TransactionId transactionId)
{
	if (AMI_OVERRIDE)
		return;

	TransactionLogUpdate(transactionId, XID_ABORT);
}
