/*-------------------------------------------------------------------------
 *
 * dynloader.c
 *	  dynamic loader for HP-UX using the shared library mechanism
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/port/dynloader/hpux.c,v 1.13 2000-04-26 23:35:34 tgl Exp $
 *
 *	NOTES
 *		all functions are defined here -- it's impossible to trace the
 *		shl_* routines from the bundled HP-UX debugger.
 *
 *-------------------------------------------------------------------------
 */
/* System includes */
#include <a.out.h>

#include "postgres.h"

#include "dl.h"
#include "dynloader.h"
#include "fmgr.h"
#include "utils/dynamic_loader.h"

void *
pg_dlopen(char *filename)
{
	/*
	 * Use BIND_IMMEDIATE so that undefined symbols cause a failure return
	 * from shl_load(), rather than an abort() later on when we attempt to
	 * call the library!
	 */
	shl_t		handle = shl_load(filename, BIND_IMMEDIATE | BIND_VERBOSE, 0);

	return (void *) handle;
}

func_ptr
pg_dlsym(void *handle, char *funcname)
{
	func_ptr	f;

	if (shl_findsym((shl_t *) & handle, funcname, TYPE_PROCEDURE, &f) == -1)
		f = (func_ptr) NULL;
	return f;
}

void
pg_dlclose(void *handle)
{
	shl_unload((shl_t) handle);
}

char *
pg_dlerror()
{
	static char errmsg[] = "shl_load failed";

	if (errno)
		return strerror(errno);

	return errmsg;
}
