/*-------------------------------------------------------------------------
 *
 * assert.c
 *	  Assert code.
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/error/assert.c,v 1.21 2002-06-20 20:29:39 momjian Exp $
 *
 * NOTE
 *	  This should eventually work with elog(), dlog(), etc.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <stdio.h>
#include <unistd.h>

#include "utils/exc.h"

int
ExceptionalCondition(char *conditionName,
					 Exception *exceptionP,
					 char *detail,
					 char *fileName,
					 int lineNumber)
{
	ExcFileName = fileName;
	ExcLineNumber = lineNumber;

	if (!PointerIsValid(conditionName)
		|| !PointerIsValid(fileName)
		|| !PointerIsValid(exceptionP))
	{
		fprintf(stderr, "TRAP: ExceptionalCondition: bad arguments\n");

		ExcAbort(exceptionP,
				 (ExcDetail) detail,
				 (ExcData) NULL,
				 (ExcMessage) NULL);
	}
	else
	{
		fprintf(stderr, "TRAP: %s(\"%s:%s\", File: \"%s\", Line: %d)\n",
				exceptionP->message, conditionName,
				(detail == NULL ? "" : detail),
				fileName, lineNumber);
	}

#ifdef ABORT_ON_ASSERT
	abort();
#endif
#ifdef SLEEP_ON_ASSERT
	sleep(1000000);
#endif

	/*
	 * XXX Depending on the Exception and tracing conditions, you will XXX
	 * want to stop here immediately and maybe dump core. XXX This may be
	 * especially true for Assert(), etc.
	 */

	/* TraceDump();		dump the trace stack */

	/* XXX FIXME: detail is lost */
	ExcRaise(exceptionP, (ExcDetail) 0, (ExcData) NULL, conditionName);
	return 0;
}
