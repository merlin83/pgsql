/*-------------------------------------------------------------------------
 *
 * superuser.c
 *
 *	  The superuser() function.  Determines if user has superuser privilege.
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/misc/superuser.c,v 1.17 2001-01-24 19:43:16 momjian Exp $
 *
 * DESCRIPTION
 *	  See superuser().
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "catalog/pg_shadow.h"
#include "utils/syscache.h"
#include "miscadmin.h"

bool
superuser(void)
{
/*--------------------------------------------------------------------------
	The Postgres user running this command has Postgres superuser
	privileges.
--------------------------------------------------------------------------*/
	HeapTuple	utup;
	bool		result;

	utup = SearchSysCache(SHADOWSYSID,
						  ObjectIdGetDatum(GetUserId()),
						  0, 0, 0);
	if (HeapTupleIsValid(utup))
	{
		result = ((Form_pg_shadow) GETSTRUCT(utup))->usesuper;
		ReleaseSysCache(utup);
		return result;
	}
	return false;
}
