/*-------------------------------------------------------------------------
 *
 * fcache.c
 *	  Code for the 'function cache' used in Oper and Func nodes.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/cache/Attic/fcache.c,v 1.37 2000-08-24 03:29:07 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "utils/fcache.h"


/*-----------------------------------------------------------------
 *
 * Build a 'FunctionCache' struct given the PG_PROC oid.
 *
 *-----------------------------------------------------------------
 */
FunctionCachePtr
init_fcache(Oid foid, int nargs, MemoryContext fcacheCxt)
{
	MemoryContext oldcontext;
	FunctionCachePtr retval;

	/* Switch to a context long-lived enough for the fcache entry */
	oldcontext = MemoryContextSwitchTo(fcacheCxt);

	retval = (FunctionCachePtr) palloc(sizeof(FunctionCache));
	MemSet(retval, 0, sizeof(FunctionCache));

	/* Set up the primary fmgr lookup information */
	fmgr_info(foid, &(retval->func));

	/* Initialize unvarying fields of per-call info block */
	retval->fcinfo.flinfo = &(retval->func);
	retval->fcinfo.nargs = nargs;

	if (nargs > FUNC_MAX_ARGS)
		elog(ERROR, "init_fcache: too many arguments");

	/* If function returns set, prepare a resultinfo node for communication */
	if (retval->func.fn_retset)
	{
		retval->fcinfo.resultinfo = (Node *) &(retval->rsinfo);
		retval->rsinfo.type = T_ReturnSetInfo;
	}

	retval->argsValid = false;
	retval->hasSetArg = false;

	MemoryContextSwitchTo(oldcontext);

	return retval;
}
