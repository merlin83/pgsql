/*-------------------------------------------------------------------------
 *
 * assert.c
 *	  Assert code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/error/assert.c,v 1.14 1999-07-15 22:40:05 momjian Exp $
 *
 * NOTE
 *	  This should eventually work with elog(), dlog(), etc.
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <unistd.h>

#include "postgres.h"			/* where the declaration goes */

#include "utils/exc.h"
#include "utils/trace.h"

int
ExceptionalCondition(char *conditionName,
					 Exception *exceptionP,
					 char *detail,
					 char *fileName,
					 int lineNumber)
{
	extern char *ExcFileName;	/* XXX */
	extern Index ExcLineNumber; /* XXX */

	ExcFileName = fileName;
	ExcLineNumber = lineNumber;

	if (!PointerIsValid(conditionName)
		|| !PointerIsValid(fileName)
		|| !PointerIsValid(exceptionP))
	{
		EPRINTF("TRAP: ExceptionalCondition: bad arguments\n");

		ExcAbort(exceptionP,
				 (ExcDetail) detail,
				 (ExcData) NULL,
				 (ExcMessage) NULL);
	}
	else
	{
		EPRINTF("TRAP: %s(\"%s:%s\", File: \"%s\", Line: %d)\n",
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
