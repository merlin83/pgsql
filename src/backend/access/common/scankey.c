/*-------------------------------------------------------------------------
 *
 * scan.c
 *	  scan direction and key code
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/common/scankey.c,v 1.17 2000-01-26 05:55:53 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/skey.h"

/*
 * ScanKeyEntryIsLegal
 *		True iff the scan key entry is legal.
 */
#define ScanKeyEntryIsLegal(entry) \
( \
	AssertMacro(PointerIsValid(entry)), \
	AttributeNumberIsValid((entry)->sk_attno) \
)

/*
 * ScanKeyEntrySetIllegal
 *		Marks a scan key entry as illegal.
 */
void
ScanKeyEntrySetIllegal(ScanKey entry)
{

	Assert(PointerIsValid(entry));

	entry->sk_flags = 0;		/* just in case... */
	entry->sk_attno = InvalidAttrNumber;
	entry->sk_procedure = 0;	/* should be InvalidRegProcedure */
}

/*
 * ScanKeyEntryInitialize
 *		Initializes an scan key entry.
 *
 * Note:
 *		Assumes the scan key entry is valid.
 *		Assumes the intialized scan key entry will be legal.
 */
void
ScanKeyEntryInitialize(ScanKey entry,
					   bits16 flags,
					   AttrNumber attributeNumber,
					   RegProcedure procedure,
					   Datum argument)
{
	Assert(PointerIsValid(entry));

	entry->sk_flags = flags;
	entry->sk_attno = attributeNumber;
	entry->sk_procedure = procedure;
	entry->sk_argument = argument;
	fmgr_info(procedure, &entry->sk_func);
	entry->sk_nargs = entry->sk_func.fn_nargs;

	Assert(ScanKeyEntryIsLegal(entry));
}
