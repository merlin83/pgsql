/*-------------------------------------------------------------------------
 *
 * palloc.c
 *	  POSTGRES memory allocator code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/mmgr/Attic/palloc.c,v 1.11 1999-02-13 23:20:11 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include <string.h>

#include "postgres.h"

#include "utils/mcxt.h"
#include "utils/elog.h"
#include "utils/palloc.h"

#include "nodes/memnodes.h"


/* ----------------------------------------------------------------
 *		User library functions
 * ----------------------------------------------------------------
 */

/* ----------
 * palloc(), pfree() and repalloc() are now macros in palloc.h
 * ----------
 */

char *
pstrdup(char *string)
{
	char		*nstr;
	int			len;

	nstr = palloc(len = strlen(string) + 1);
	MemoryCopy(nstr, string, len);

	return nstr;
}

