/*-------------------------------------------------------------------------
 *
 * superuser.c
 *
 *	  The superuser() function.  Determines if user has superuser privilege.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/misc/superuser.c,v 1.11 1999-11-22 17:56:35 momjian Exp $
 *
 * DESCRIPTION
 *	  See superuser().
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "catalog/pg_shadow.h"
#include "utils/syscache.h"

bool
superuser(void)
{
/*--------------------------------------------------------------------------
	The Postgres user running this command has Postgres superuser
	privileges.
--------------------------------------------------------------------------*/
	extern char *UserName;		/* defined in global.c */

	HeapTuple	utup;

	utup = SearchSysCacheTuple(USERNAME,
							   PointerGetDatum(UserName),
							   0, 0, 0);
	Assert(utup != NULL);
	return ((Form_pg_shadow) GETSTRUCT(utup))->usesuper;
}
