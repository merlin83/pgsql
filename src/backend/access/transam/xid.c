/*-------------------------------------------------------------------------
 *
 * xid.c--
 *    POSTGRES transaction identifier code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/transam/Attic/xid.c,v 1.3 1996-11-03 22:58:26 scrappy Exp $
 *
 * OLD COMMENTS
 * XXX WARNING
 *	Much of this file will change when we change our representation
 *	of transaction ids -cim 3/23/90
 *
 * It is time to make the switch from 5 byte to 4 byte transaction ids
 * This file was totally reworked. -mer 5/22/92
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>

#include "postgres.h"
#include "utils/palloc.h"


extern TransactionId NullTransactionId;
extern TransactionId DisabledTransactionId;
extern TransactionId AmiTransactionId;
extern TransactionId FirstTransactionId;

/* ----------------------------------------------------------------
 * 	TransactionIdIsValid
 *
 *	Macro-ize me.
 * ----------------------------------------------------------------
 */
bool
TransactionIdIsValid(TransactionId transactionId)
{
    return ((bool) (transactionId != NullTransactionId) );
}

/* XXX char16 name for catalogs */
TransactionId
xidin(char *representation)
{
    return (atol(representation));
}

/* XXX char16 name for catalogs */
char*
xidout(TransactionId transactionId)
{
/*    return(TransactionIdFormString(transactionId)); */
    char 			*representation;
    
    /* maximum 32 bit unsigned integer representation takes 10 chars */
    representation = palloc(11);
    
    (void)sprintf(representation, "%u", transactionId);
    
    return (representation);

}

/* ----------------------------------------------------------------
 *	StoreInvalidTransactionId
 *
 *	Maybe do away with Pointer types in these routines.
 *      Macro-ize this one.
 * ----------------------------------------------------------------
 */
void
StoreInvalidTransactionId(TransactionId *destination)
{
    *destination = NullTransactionId;
}

/* ----------------------------------------------------------------
 *	TransactionIdStore
 *
 *      Macro-ize this one.
 * ----------------------------------------------------------------
 */
void
TransactionIdStore(TransactionId transactionId,
		   TransactionId *destination)
{
    *destination = transactionId;
}

/* ----------------------------------------------------------------
 *	TransactionIdEquals
 * ----------------------------------------------------------------
 */
bool
TransactionIdEquals(TransactionId id1, TransactionId id2)
{
    return ((bool) (id1 == id2));
}

/* ----------------------------------------------------------------
 *	TransactionIdIsLessThan
 * ----------------------------------------------------------------
 */
bool
TransactionIdIsLessThan(TransactionId id1, TransactionId id2)
{
    return ((bool)(id1 < id2));
}

/* ----------------------------------------------------------------
 *	xideq
 * ----------------------------------------------------------------
 */

/*
 *	xideq		- returns 1, iff xid1 == xid2
 *				  0  else;
 */
bool
xideq(TransactionId xid1, TransactionId xid2)
{
    return( (bool) (xid1 == xid2) );
}



/* ----------------------------------------------------------------
 *	TransactionIdIncrement
 * ----------------------------------------------------------------
 */
void
TransactionIdIncrement(TransactionId *transactionId)
{
    
    (*transactionId)++;
    if (*transactionId == DisabledTransactionId)
	elog(FATAL, "TransactionIdIncrement: exhausted XID's");
    return;
}

/* ----------------------------------------------------------------
 *	TransactionIdAdd
 * ----------------------------------------------------------------
 */
void
TransactionIdAdd(TransactionId *xid, int value)
{
    *xid += value;
    return;
}

