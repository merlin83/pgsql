/*-------------------------------------------------------------------------
 *
 * enbl.c
 *	  POSTGRES module enable and disable support code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/init/Attic/enbl.c,v 1.7 1999-05-25 16:12:33 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "utils/module.h"		/* where the declarations go */

/*
 * BypassEnable
 *		False iff enable/disable processing is required given on and "*countP."
 *
 * Note:
 *		As a side-effect, *countP is modified.	It should be 0 initially.
 *
 * Exceptions:
 *		BadState if called with pointer to value 0 and false.
 *		BadArg if "countP" is invalid pointer.
 *		BadArg if on is invalid.
 */
bool
BypassEnable(int *enableCountInOutP, bool on)
{
	AssertArg(PointerIsValid(enableCountInOutP));
	AssertArg(BoolIsValid(on));

	if (on)
	{
		*enableCountInOutP += 1;
		return (bool) (*enableCountInOutP >= 2);
	}

	Assert(*enableCountInOutP >= 1);

	*enableCountInOutP -= 1;

	return (bool) (*enableCountInOutP >= 1);
}
