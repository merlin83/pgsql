/*-------------------------------------------------------------------------
 *
 * fmgr.c
 *	  Interface routines for the table-driven function manager.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/fmgr/fmgr.c,v 1.35 2000-01-10 18:18:30 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_language.h"
#include "catalog/pg_proc.h"
#include "commands/trigger.h"
#include "utils/builtins.h"
#include "utils/fmgrtab.h"
#include "utils/syscache.h"


/*
 * Interface for PL functions
 *
 * XXX: use of global fmgr_pl_finfo variable is really ugly.  FIXME
 */

static char *
fmgr_pl(char *arg0,...)
{
	va_list		pvar;
	FmgrValues	values;
	int			n_arguments = fmgr_pl_finfo->fn_nargs;
	bool		isNull = false;
	int			i;

	memset(&values, 0, sizeof(values));

	if (n_arguments > 0)
	{
		values.data[0] = arg0;
		if (n_arguments > 1)
		{
			if (n_arguments > FUNC_MAX_ARGS)
				elog(ERROR, "fmgr_pl: function %u: too many arguments (%d > %d)",
					 fmgr_pl_finfo->fn_oid, n_arguments, FUNC_MAX_ARGS);
			va_start(pvar, arg0);
			for (i = 1; i < n_arguments; i++)
				values.data[i] = va_arg(pvar, char *);
			va_end(pvar);
		}
	}

	/* Call the PL handler */
	CurrentTriggerData = NULL;
	return (*(fmgr_pl_finfo->fn_plhandler)) (fmgr_pl_finfo,
											 &values,
											 &isNull);
}


/*
 * Interface for untrusted functions
 */

static char *
fmgr_untrusted(char *arg0,...)
{

	/*
	 * Currently these are unsupported.  Someday we might do something
	 * like forking a subprocess to execute 'em.
	 */
	elog(ERROR, "Untrusted functions not supported.");
	return NULL;				/* keep compiler happy */
}


/*
 * Interface for SQL-language functions
 */

static char *
fmgr_sql(char *arg0,...)
{

	/*
	 * XXX It'd be really nice to support SQL functions anywhere that
	 * builtins are supported.	What would we have to do?  What pitfalls
	 * are there?
	 */
	elog(ERROR, "SQL-language function not supported in this context.");
	return NULL;				/* keep compiler happy */
}


/*
 * fmgr_c is not really for C functions only; it can be called for functions
 * in any language.  Many parts of the system use this entry point if they
 * want to pass the arguments in an array rather than as explicit arguments.
 */

char *
fmgr_c(FmgrInfo *finfo,
	   FmgrValues *values,
	   bool *isNull)
{
	char	   *returnValue = (char *) NULL;
	int			n_arguments = finfo->fn_nargs;
	func_ptr	user_fn = fmgr_faddr(finfo);

	/*
	 * If finfo contains a PL handler for this function, call that
	 * instead.
	 */
	if (finfo->fn_plhandler != NULL)
		return (*(finfo->fn_plhandler)) (finfo, values, isNull);

	if (user_fn == (func_ptr) NULL)
		elog(ERROR, "Internal error: fmgr_c received NULL function pointer.");

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
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8]);
			break;
		case 10:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9]);
			break;
		case 11:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10]);
			break;
		case 12:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11]);
			break;
		case 13:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12]);
			break;
		case 14:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13]);
			break;
		case 15:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14]);
			break;
		case 16:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15]);
			break;
		case 17:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16]);
			break;
		case 18:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17]);
			break;
		case 19:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18]);
			break;
		case 20:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19]);
			break;
		case 21:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20]);
			break;
		case 22:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21]);
			break;
		case 23:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22]);
			break;
		case 24:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23]);
			break;
		case 25:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24]);
			break;
		case 26:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25]);
			break;
		case 27:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25],
									  values->data[26]);
			break;
		case 28:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25],
									  values->data[26], values->data[27]);
			break;
		case 29:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25],
									  values->data[26], values->data[27],
									  values->data[28]);
			break;
		case 30:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25],
									  values->data[26], values->data[27],
									  values->data[28], values->data[29]);
			break;
		case 31:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25],
									  values->data[26], values->data[27],
									  values->data[28], values->data[29],
									  values->data[30]);
			break;
		case 32:
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25],
									  values->data[26], values->data[27],
									  values->data[28], values->data[29],
									  values->data[30], values->data[31]);
			break;
		default:

			/*
			 * XXX Note that functions with >FUNC_MAX_ARGS arguments can only be
			 * called from inside the system, not from the user level,
			 * since the catalogs only store FUNC_MAX_ARGS argument types for user
			 * type-checking!
			 */
			returnValue = (*user_fn) (values->data[0], values->data[1],
									  values->data[2], values->data[3],
									  values->data[4], values->data[5],
									  values->data[6], values->data[7],
									  values->data[8], values->data[9],
									  values->data[10], values->data[11],
									  values->data[12], values->data[13],
									  values->data[14], values->data[15],
									  values->data[16], values->data[17],
									  values->data[18], values->data[19],
									  values->data[20], values->data[21],
									  values->data[22], values->data[23],
									  values->data[24], values->data[25],
									  values->data[26], values->data[27],
									  values->data[28], values->data[29],
									  values->data[30], values->data[31]);
			break;
		default:
			elog(ERROR, "fmgr_c: function %u: too many arguments (%d > %d)",
				 finfo->fn_oid, n_arguments, FUNC_MAX_ARGS);
			break;
	}
	return returnValue;
}

/*
 * Expand a regproc OID into an FmgrInfo cache struct.
 */

void
fmgr_info(Oid procedureId, FmgrInfo *finfo)
{
	FmgrCall   *fcp;
	HeapTuple	procedureTuple;
	FormData_pg_proc *procedureStruct;
	HeapTuple	languageTuple;
	Form_pg_language languageStruct;
	Oid			language;
	char	   *prosrc;

	finfo->fn_addr = NULL;
	finfo->fn_plhandler = NULL;
	finfo->fn_oid = procedureId;

	if ((fcp = fmgr_isbuiltin(procedureId)) != NULL)
	{

		/*
		 * Fast path for builtin functions: don't bother consulting
		 * pg_proc
		 */
		finfo->fn_addr = fcp->func;
		finfo->fn_nargs = fcp->nargs;
	}
	else
	{
		procedureTuple = SearchSysCacheTuple(PROCOID,
										   ObjectIdGetDatum(procedureId),
											 0, 0, 0);
		if (!HeapTupleIsValid(procedureTuple))
		{
			elog(ERROR, "fmgr_info: function %u: cache lookup failed",
				 procedureId);
		}
		procedureStruct = (FormData_pg_proc *) GETSTRUCT(procedureTuple);
		if (!procedureStruct->proistrusted)
		{
			finfo->fn_addr = (func_ptr) fmgr_untrusted;
			finfo->fn_nargs = procedureStruct->pronargs;
			return;
		}
		language = procedureStruct->prolang;
		switch (language)
		{
			case INTERNALlanguageId:

				/*
				 * For an ordinary builtin function, we should never get
				 * here because the isbuiltin() search above will have
				 * succeeded. However, if the user has done a CREATE
				 * FUNCTION to create an alias for a builtin function, we
				 * end up here.  In that case we have to look up the
				 * function by name.  The name of the internal function is
				 * stored in prosrc (it doesn't have to be the same as the
				 * name of the alias!)
				 */
				prosrc = textout(&(procedureStruct->prosrc));
				finfo->fn_addr = fmgr_lookupByName(prosrc);
				if (!finfo->fn_addr)
					elog(ERROR, "fmgr_info: function %s not in internal table",
						 prosrc);
				finfo->fn_nargs = procedureStruct->pronargs;
				pfree(prosrc);
				break;
			case ClanguageId:
				finfo->fn_addr = fmgr_dynamic(procedureId, &(finfo->fn_nargs));
				break;
			case SQLlanguageId:
				finfo->fn_addr = (func_ptr) fmgr_sql;
				finfo->fn_nargs = procedureStruct->pronargs;
				break;
			default:

				/*
				 * Might be a created procedural language Lookup the
				 * syscache for the language and check the lanispl flag If
				 * this is the case, we return a NULL function pointer and
				 * the number of arguments from the procedure.
				 */
				languageTuple = SearchSysCacheTuple(LANGOID,
							  ObjectIdGetDatum(procedureStruct->prolang),
													0, 0, 0);
				if (!HeapTupleIsValid(languageTuple))
				{
					elog(ERROR, "fmgr_info: %s %u",
						 "Cache lookup for language failed",
						 DatumGetObjectId(procedureStruct->prolang));
				}
				languageStruct = (Form_pg_language) GETSTRUCT(languageTuple);
				if (languageStruct->lanispl)
				{
					FmgrInfo	plfinfo;

					fmgr_info(languageStruct->lanplcallfoid, &plfinfo);
					finfo->fn_addr = (func_ptr) fmgr_pl;
					finfo->fn_plhandler = plfinfo.fn_addr;
					finfo->fn_nargs = procedureStruct->pronargs;
				}
				else
				{
					elog(ERROR, "fmgr_info: function %u: unknown language %d",
						 procedureId, language);
				}
				break;
		}
	}
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
char *
fmgr(Oid procedureId,...)
{
	va_list		pvar;
	int			i;
	int			pronargs;
	FmgrValues	values;
	FmgrInfo	finfo;
	bool		isNull = false;

	fmgr_info(procedureId, &finfo);
	pronargs = finfo.fn_nargs;

	if (pronargs > FUNC_MAX_ARGS)
		elog(ERROR, "fmgr: function %u: too many arguments (%d > %d)",
			 procedureId, pronargs, FUNC_MAX_ARGS);

	va_start(pvar, procedureId);
	for (i = 0; i < pronargs; ++i)
		values.data[i] = va_arg(pvar, char *);
	va_end(pvar);

	/* XXX see WAY_COOL_ORTHOGONAL_FUNCTIONS */
	return fmgr_c(&finfo, &values, &isNull);
}

/*
 * This is just a version of fmgr() in which the hacker can prepend a C
 * function pointer.  This routine is not normally called; generally,
 * if you have all of this information you're likely to just jump through
 * the pointer, but it's available for use with macros in fmgr.h if you
 * want this routine to do sanity-checking for you.
 *
 * funcinfo, n_arguments, args...
 */
#ifdef TRACE_FMGR_PTR

char *
fmgr_ptr(FmgrInfo *finfo,...)
{
	va_list		pvar;
	int			i;
	int			n_arguments;
	FmgrInfo	local_finfo;
	FmgrValues	values;
	bool		isNull = false;

	local_finfo->fn_addr = finfo->fn_addr;
	local_finfo->fn_plhandler = finfo->fn_plhandler;
	local_finfo->fn_oid = finfo->fn_oid;

	va_start(pvar, finfo);
	n_arguments = va_arg(pvar, int);
	local_finfo->fn_nargs = n_arguments;
	if (n_arguments > FUNC_MAX_ARGS)
	{
		elog(ERROR, "fmgr_ptr: function %u: too many arguments (%d > %d)",
			 func_id, n_arguments, FUNC_MAX_ARGS);
	}
	for (i = 0; i < n_arguments; ++i)
		values.data[i] = va_arg(pvar, char *);
	va_end(pvar);

	/* XXX see WAY_COOL_ORTHOGONAL_FUNCTIONS */
	return fmgr_c(&local_finfo, &values, &isNull);
}

#endif

/*
 * This routine is not well thought out.  When I get around to adding a
 * function pointer field to FuncIndexInfo, it will be replace by calls
 * to fmgr_c().
 */
char *
fmgr_array_args(Oid procedureId, int nargs, char *args[], bool *isNull)
{
	FmgrInfo	finfo;

	fmgr_info(procedureId, &finfo);
	finfo.fn_nargs = nargs;

	/* XXX see WAY_COOL_ORTHOGONAL_FUNCTIONS */
	return fmgr_c(&finfo,
				  (FmgrValues *) args,
				  isNull);
}
