/*-------------------------------------------------------------------------
 *
 * superuser.c--
 *
 *	  The superuser() function.  Determines if user has superuser privilege.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/misc/superuser.c,v 1.5 1998-02-25 13:08:23 scrappy Exp $
 *
 * DESCRIPTION
 *	  See superuser().
 *-------------------------------------------------------------------------
 */

#include <postgres.h>
#include <utils/syscache.h>
#include <catalog/pg_shadow.h>

bool
superuser(void)
{
/*--------------------------------------------------------------------------
	The Postgres user running this command has Postgres superuser
	privileges.
--------------------------------------------------------------------------*/
	extern char *UserName;		/* defined in global.c */

	HeapTuple	utup;

	utup = SearchSysCacheTuple(USENAME, PointerGetDatum(UserName),
							   0, 0, 0);
	Assert(utup != NULL);
	return ((Form_pg_shadow) GETSTRUCT(utup))->usesuper;
}
