/*-------------------------------------------------------------------------
 *
 * superuser.c--
 *
 *    The superuser() function.  Determines if user has superuser privilege.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/misc/superuser.c,v 1.1 1996-11-02 02:06:47 bryanh Exp $
 *
 * DESCRIPTION
 *    See superuser().
 *-------------------------------------------------------------------------
 */

#include <c.h>
#include <postgres.h>
#include <access/htup.h>
#include <utils/syscache.h>
#include <catalog/pg_user.h>



bool
superuser(void) {
/*--------------------------------------------------------------------------
    The Postgres user running this command has Postgres superuser 
    privileges.
--------------------------------------------------------------------------*/
    extern char *UserName;  /* defined in global.c */

    HeapTuple utup;

    utup = SearchSysCacheTuple(USENAME, PointerGetDatum(UserName),
			       0,0,0);
    Assert(utup != NULL);
    return ((Form_pg_user)GETSTRUCT(utup))->usesuper;
}



