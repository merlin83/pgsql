/*-------------------------------------------------------------------------
 *
 * fmgr.c--
 *	  Interface routines for the table-driven function manager.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/fmgr/fmgr.c,v 1.4 1997-09-07 04:53:30 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdarg.h>

#include "postgres.h"

/* these 2 files are generated by Gen_fmgrtab.sh; contain the declarations */
#include "fmgr.h"
#include "utils/fmgrtab.h"

#include "nodes/pg_list.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_language.h"
#include "utils/syscache.h"
#include "nodes/params.h"

#include "utils/elog.h"


char		   *
fmgr_c(func_ptr user_fn,
	   Oid func_id,
	   int n_arguments,
	   FmgrValues * values,
	   bool * isNull)
{
	char		   *returnValue = (char *) NULL;


	if (user_fn == (func_ptr) NULL)
	{

		/*
		 * a NULL func_ptr denotes untrusted function (in postgres 4.2).
		 * Untrusted functions have very limited use and is clumsy. We
		 * just get rid of it.
		 */
		elog(WARN, "internal error: untrusted function not supported.");
	}

	switch (n_arguments)
	{
	case 0:
		returnValue = (*user_fn) ();
		break;
	case 1:
		/* NullValue() uses isNull to check if args[0] is NULL */
		returnValue = (*user_fn) (values->data[0], isNull);
		break;
	case 2:
		returnValue = (*user_fn) (values->data[0], values->data[1]);
		break;
	case 3:
		returnValue = (*user_fn) (values->data[0], values->data[1],
								  values->data[2]);
		break;
	case 4:
		returnValue = (*user_fn) (values->data[0], values->data[1],
								  values->data[2], values->data[3]);
		break;
	case 5:
		returnValue = (*user_fn) (values->data[0], values->data[1],
								  values->data[2], values->data[3],
								  values->data[4]);
		break;
	case 6:
		returnValue = (*user_fn) (values->data[0], values->data[1],
								  values->data[2], values->data[3],
								  values->data[4], values->data[5]);
		break;
	case 7:
		returnValue = (*user_fn) (values->data[0], values->data[1],
								  values->data[2], values->data[3],
								  values->data[4], values->data[5],
								  values->data[6]);
		break;
	case 8:
		returnValue = (*user_fn) (values->data[0], values->data[1],
								  values->data[2], values->data[3],
								  values->data[4], values->data[5],
								  values->data[6], values->data[7]);
		break;
	case 9:

		/*
		 * XXX Note that functions with >8 arguments can only be called
		 * from inside the system, not from the user level, since the
		 * catalogs only store 8 argument types for user type-checking!
		 */
		returnValue = (*user_fn) (values->data[0], values->data[1],
								  values->data[2], values->data[3],
								  values->data[4], values->data[5],
								  values->data[6], values->data[7],
								  values->data[8]);
		break;
	default:
		elog(WARN, "fmgr_c: function %d: too many arguments (%d > %d)",
			 func_id, n_arguments, MAXFMGRARGS);
		break;
	}
	return (returnValue);
}

void
fmgr_info(Oid procedureId, func_ptr * function, int *nargs)
{
	func_ptr		user_fn = NULL;
	FmgrCall	   *fcp;
	HeapTuple		procedureTuple;
	FormData_pg_proc *procedureStruct;
	Oid				language;

	if (!(fcp = fmgr_isbuiltin(procedureId)))
	{
		procedureTuple = SearchSysCacheTuple(PROOID,
										   ObjectIdGetDatum(procedureId),
											 0, 0, 0);
		if (!HeapTupleIsValid(procedureTuple))
		{
			elog(WARN, "fmgr_info: function %d: cache lookup failed\n",
				 procedureId);
		}
		procedureStruct = (FormData_pg_proc *)
			GETSTRUCT(procedureTuple);
		if (!procedureStruct->proistrusted)
		{
			*function = (func_ptr) NULL;
			*nargs = procedureStruct->pronargs;
			return;
		}
		language = procedureStruct->prolang;
		switch (language)
		{
		case INTERNALlanguageId:
			user_fn = fmgr_lookupByName(procedureStruct->proname.data);
			if (!user_fn)
				elog(WARN, "fmgr_info: function %s: not in internal table",
					 procedureStruct->proname.data);
			break;
		case ClanguageId:
			user_fn = fmgr_dynamic(procedureId, nargs);
			break;
		case SQLlanguageId:
			user_fn = (func_ptr) NULL;
			*nargs = procedureStruct->pronargs;
			break;
		default:
			elog(WARN, "fmgr_info: function %d: unknown language %d",
				 procedureId, language);
		}
	}
	else
	{
		user_fn = fcp->func;
		*nargs = fcp->nargs;
	}
	*function = user_fn;
}

/*
 *		fmgr			- return the value of a function call
 *
 *		If the function is a system routine, it's compiled in, so call
 *		it directly.
 *
 *		Otherwise pass it to the the appropriate 'language' function caller.
 *
 *		Returns the return value of the invoked function if succesful,
 *		0 if unsuccessful.
 */
char		   *
fmgr(Oid procedureId,...)
{
	va_list			pvar;
	register		i;
	int				pronargs;
	FmgrValues		values;
	func_ptr		user_fn;
	bool			isNull = false;

	va_start(pvar, procedureId);

	fmgr_info(procedureId, &user_fn, &pronargs);

	if (pronargs > MAXFMGRARGS)
	{
		elog(WARN, "fmgr: function %d: too many arguments (%d > %d)",
			 procedureId, pronargs, MAXFMGRARGS);
	}
	for (i = 0; i < pronargs; ++i)
		values.data[i] = va_arg(pvar, char *);
	va_end(pvar);

	/* XXX see WAY_COOL_ORTHOGONAL_FUNCTIONS */
	return (fmgr_c(user_fn, procedureId, pronargs, &values,
				   &isNull));
}

/*
 * This is just a version of fmgr() in which the hacker can prepend a C
 * function pointer.  This routine is not normally called; generally,
 * if you have all of this information you're likely to just jump through
 * the pointer, but it's available for use with macros in fmgr.h if you
 * want this routine to do sanity-checking for you.
 *
 * func_ptr, func_id, n_arguments, args...
 */
#ifdef NOT_USED
char		   *
fmgr_ptr(func_ptr user_fn, Oid func_id,...)
{
	va_list			pvar;
	register		i;
	int				n_arguments;
	FmgrValues		values;
	bool			isNull = false;

	va_start(pvar, func_id);
	n_arguments = va_arg(pvar, int);
	if (n_arguments > MAXFMGRARGS)
	{
		elog(WARN, "fmgr_ptr: function %d: too many arguments (%d > %d)",
			 func_id, n_arguments, MAXFMGRARGS);
	}
	for (i = 0; i < n_arguments; ++i)
		values.data[i] = va_arg(pvar, char *);
	va_end(pvar);

	/* XXX see WAY_COOL_ORTHOGONAL_FUNCTIONS */
	return (fmgr_c(user_fn, func_id, n_arguments, &values,
				   &isNull));
}

#endif

/*
 * This routine is not well thought out.  When I get around to adding a
 * function pointer field to FuncIndexInfo, it will be replace by calls
 * to fmgr_c().
 */
char		   *
fmgr_array_args(Oid procedureId, int nargs, char *args[], bool * isNull)
{
	func_ptr		user_fn;
	int				true_arguments;

	fmgr_info(procedureId, &user_fn, &true_arguments);

	/* XXX see WAY_COOL_ORTHOGONAL_FUNCTIONS */
	return
		(fmgr_c(user_fn,
				procedureId,
				true_arguments,
				(FmgrValues *) args,
				isNull));
}
