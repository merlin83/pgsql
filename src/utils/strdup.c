/*-------------------------------------------------------------------------
 *
 * strdup.c--
 *	  copies a null-terminated string.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/utils/Attic/strdup.c,v 1.5 1998-02-26 04:46:47 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <string.h>
#include <stdlib.h>
#include "strdup.h"

char *
strdup(char const * string)
{
	char	   *nstr;

	nstr = strcpy((char *) malloc(strlen(string) + 1), string);
	return nstr;
}
