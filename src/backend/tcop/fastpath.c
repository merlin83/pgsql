/*-------------------------------------------------------------------------
 *
 * fastpath.c
 *	  routines to handle function requests from the frontend
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/tcop/fastpath.c,v 1.32 2000-01-10 16:13:13 momjian Exp $
 *
 * NOTES
 *	  This cruft is the server side of PQfn.
 *
 *	  - jolly 07/11/95:
 *
 *	  no longer rely on return sizes provided by the frontend.	Always
 *	  use the true lengths for the catalogs.  Assume that the frontend
 *	  has allocated enough space to handle the result value returned.
 *
 *	  trust that the user knows what he is doing with the args.  If the
 *	  sys catalog says it is a varlena, assume that the user is only sending
 *	  down VARDATA and that the argsize is the VARSIZE.  If the arg is
 *	  fixed len, assume that the argsize given by the user is correct.
 *
 *	  if the function returns by value, then only send 4 bytes value
 *	  back to the frontend.  If the return returns by reference,
 *	  send down only the data portion and set the return size appropriately.
 *
 *	 OLD COMMENTS FOLLOW
 *
 *	  The VAR_LENGTH_{ARGS,RESULT} stuff is limited to MAX_STRING_LENGTH
 *	  (see src/backend/tmp/fastpath.h) for no obvious reason.  Since its
 *	  primary use (for us) is for Inversion path names, it should probably
 *	  be increased to 256 (MAXPATHLEN for Inversion, hidden in pg_type
 *	  as well as utils/adt/filename.c).
 *
 *	  Quoth PMA on 08/15/93:
 *
 *	  This code has been almost completely rewritten with an eye to
 *	  keeping it as compatible as possible with the previous (broken)
 *	  implementation.
 *
 *	  The previous implementation would assume (1) that any value of
 *	  length <= 4 bytes was passed-by-value, and that any other value
 *	  was a struct varlena (by-reference).	There was NO way to pass a
 *	  fixed-length by-reference argument (like name) or a struct
 *	  varlena of size <= 4 bytes.
 *
 *	  The new implementation checks the catalogs to determine whether
 *	  a value is by-value (type "0" is null-delimited character string,
 *	  as it is for, e.g., the parser).	The only other item obtained
 *	  from the catalogs is whether or not the value should be placed in
 *	  a struct varlena or not.	Otherwise, the size given by the
 *	  frontend is assumed to be correct (probably a bad decision, but
 *	  we do strange things in the name of compatibility).
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/xact.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "libpq/libpq.h"
#include "libpq/pqformat.h"
#include "tcop/fastpath.h"
#include "utils/builtins.h"
#include "utils/syscache.h"


/* ----------------
 *		SendFunctionResult
 * ----------------
 */
static void
SendFunctionResult(Oid fid,		/* function id */
				   char *retval,/* actual return value */
				   bool retbyval,
				   int retlen	/* the length according to the catalogs */
)
{
	StringInfoData buf;

	pq_beginmessage(&buf);
	pq_sendbyte(&buf, 'V');

	if (retlen != 0)
	{
		pq_sendbyte(&buf, 'G');
		if (retbyval)
		{						/* by-value */
			pq_sendint(&buf, retlen, 4);
			pq_sendint(&buf, (int) (Datum) retval, retlen);
		}
		else
		{						/* by-reference ... */
			if (retlen < 0)
			{					/* ... varlena */
				pq_sendint(&buf, VARSIZE(retval) - VARHDRSZ, VARHDRSZ);
				pq_sendbytes(&buf, VARDATA(retval), VARSIZE(retval) - VARHDRSZ);
			}
			else
			{					/* ... fixed */
				pq_sendint(&buf, retlen, 4);
				pq_sendbytes(&buf, retval, retlen);
			}
		}
	}

	pq_sendbyte(&buf, '0');
	pq_endmessage(&buf);
}

/*
 * This structure saves enough state so that one can avoid having to
 * do catalog lookups over and over again.	(Each RPC can require up
 * to MAXFMGRARGS+2 lookups, which is quite tedious.)
 *
 * The previous incarnation of this code just assumed that any argument
 * of size <= 4 was by value; this is not correct.	There is no cheap
 * way to determine function argument length etc.; one must simply pay
 * the price of catalog lookups.
 */
struct fp_info
{
	Oid			funcid;
	int			nargs;
	bool		argbyval[MAXFMGRARGS];
	int32		arglen[MAXFMGRARGS];	/* signed (for varlena) */
	bool		retbyval;
	int32		retlen;			/* signed (for varlena) */
	TransactionId xid;
	CommandId	cid;
};

/*
 * We implement one-back caching here.	If we need to do more, we can.
 * Most routines in tight loops (like PQfswrite -> F_LOWRITE) will do
 * the same thing repeatedly.
 */
static struct fp_info last_fp = {InvalidOid};

/*
 * valid_fp_info
 *
 * RETURNS:
 *		1 if the state in 'fip' is valid
 *		0 otherwise
 *
 * "valid" means:
 * The saved state was either uninitialized, for another function,
 * or from a previous command.	(Commands can do updates, which
 * may invalidate catalog entries for subsequent commands.	This
 * is overly pessimistic but since there is no smarter invalidation
 * scheme...).
 */
static int
valid_fp_info(Oid func_id, struct fp_info * fip)
{
	Assert(OidIsValid(func_id));
	Assert(fip != (struct fp_info *) NULL);

	return (OidIsValid(fip->funcid) &&
			oideq(func_id, fip->funcid) &&
			TransactionIdIsCurrentTransactionId(fip->xid) &&
			CommandIdIsCurrentCommandId(fip->cid));
}

/*
 * update_fp_info
 *
 * Performs catalog lookups to load a struct fp_info 'fip' for the
 * function 'func_id'.
 *
 * RETURNS:
 *		The correct information in 'fip'.  Sets 'fip->funcid' to
 *		InvalidOid if an exception occurs.
 */
static void
update_fp_info(Oid func_id, struct fp_info * fip)
{
	Oid		   *argtypes;		/* an oidvector */
	Oid			rettype;
	HeapTuple	func_htp,
				type_htp;
	Form_pg_type tp;
	Form_pg_proc pp;
	int			i;

	Assert(OidIsValid(func_id));
	Assert(fip != (struct fp_info *) NULL);

	/*
	 * Since the validity of this structure is determined by whether the
	 * funcid is OK, we clear the funcid here.	It must not be set to the
	 * correct value until we are about to return with a good struct
	 * fp_info, since we can be interrupted (i.e., with an elog(ERROR,
	 * ...)) at any time.
	 */
	MemSet((char *) fip, 0, (int) sizeof(struct fp_info));
	fip->funcid = InvalidOid;

	func_htp = SearchSysCacheTuple(PROCOID,
								   ObjectIdGetDatum(func_id),
								   0, 0, 0);
	if (!HeapTupleIsValid(func_htp))
	{
		elog(ERROR, "update_fp_info: cache lookup for function %u failed",
			 func_id);
	}
	pp = (Form_pg_proc) GETSTRUCT(func_htp);
	fip->nargs = pp->pronargs;
	rettype = pp->prorettype;
	argtypes = pp->proargtypes;

	for (i = 0; i < fip->nargs; ++i)
	{
		if (OidIsValid(argtypes[i]))
		{
			type_htp = SearchSysCacheTuple(TYPEOID,
										   ObjectIdGetDatum(argtypes[i]),
										   0, 0, 0);
			if (!HeapTupleIsValid(type_htp))
			{
				elog(ERROR, "update_fp_info: bad argument type %u for %u",
					 argtypes[i], func_id);
			}
			tp = (Form_pg_type) GETSTRUCT(type_htp);
			fip->argbyval[i] = tp->typbyval;
			fip->arglen[i] = tp->typlen;
		}						/* else it had better be VAR_LENGTH_ARG */
	}

	if (OidIsValid(rettype))
	{
		type_htp = SearchSysCacheTuple(TYPEOID,
									   ObjectIdGetDatum(rettype),
									   0, 0, 0);
		if (!HeapTupleIsValid(type_htp))
		{
			elog(ERROR, "update_fp_info: bad return type %u for %u",
				 rettype, func_id);
		}
		tp = (Form_pg_type) GETSTRUCT(type_htp);
		fip->retbyval = tp->typbyval;
		fip->retlen = tp->typlen;
	}							/* else it had better by VAR_LENGTH_RESULT */

	fip->xid = GetCurrentTransactionId();
	fip->cid = GetCurrentCommandId();

	/*
	 * This must be last!
	 */
	fip->funcid = func_id;
}


/*
 * HandleFunctionRequest
 *
 * Server side of PQfn (fastpath function calls from the frontend).
 * This corresponds to the libpq protocol symbol "F".
 *
 * RETURNS:
 *		0 if successful completion, EOF if frontend connection lost.
 *
 * Note: All ordinary errors result in elog(ERROR,...).  However,
 * if we lose the frontend connection there is no one to elog to,
 * and no use in proceeding...
 */
int
HandleFunctionRequest()
{
	Oid			fid;
	int			argsize;
	int			nargs;
	int			tmp;
	char	   *arg[8];
	char	   *retval;
	int			i;
	uint32		palloced;
	char	   *p;
	struct fp_info *fip;

	if (pq_getint(&tmp, 4))		/* function oid */
		return EOF;
	fid = (Oid) tmp;
	if (pq_getint(&nargs, 4))	/* # of arguments */
		return EOF;

	/*
	 * This is where the one-back caching is done. If you want to save
	 * more state, make this a loop around an array.
	 */
	fip = &last_fp;
	if (!valid_fp_info(fid, fip))
		update_fp_info(fid, fip);

	/*
	 * XXX FIXME: elog() here means we lose sync with the frontend,
	 * since we have not swallowed all of its input message.  What
	 * should happen is we absorb all of the input message per protocol
	 * syntax, and *then* do error checking and elog if appropriate.
	 */

	if (fip->nargs != nargs)
	{
		elog(ERROR, "HandleFunctionRequest: actual arguments (%d) != registered arguments (%d)",
			 nargs, fip->nargs);
	}

	/*
	 * Copy arguments into arg vector.	If we palloc() an argument, we
	 * need to remember, so that we pfree() it after the call.
	 */
	palloced = 0x0;
	for (i = 0; i < 8; ++i)
	{
		if (i >= nargs)
			arg[i] = (char *) NULL;
		else
		{
			if (pq_getint(&argsize, 4))
				return EOF;

			Assert(argsize > 0);
			if (fip->argbyval[i])
			{					/* by-value */
				Assert(argsize <= 4);
				if (pq_getint(&tmp, argsize))
					return EOF;
				arg[i] = (char *) tmp;
			}
			else
			{					/* by-reference ... */
				if (fip->arglen[i] < 0)
				{				/* ... varlena */
					if (!(p = palloc(argsize + VARHDRSZ + 1)))	/* Added +1 to solve
																 * memory leak - Peter
																 * 98 Jan 6 */
						elog(ERROR, "HandleFunctionRequest: palloc failed");
					VARSIZE(p) = argsize + VARHDRSZ;
					if (pq_getbytes(VARDATA(p), argsize))
						return EOF;
				}
				else
				{				/* ... fixed */
					/* XXX cross our fingers and trust "argsize" */
					if (!(p = palloc(argsize + 1)))
						elog(ERROR, "HandleFunctionRequest: palloc failed");
					if (pq_getbytes(p, argsize))
						return EOF;
				}
				palloced |= (1 << i);
				arg[i] = p;
			}
		}
	}

#ifndef NO_FASTPATH
	retval = fmgr(fid,
				  arg[0], arg[1], arg[2], arg[3],
				  arg[4], arg[5], arg[6], arg[7]);

#else
	retval = NULL;
#endif	 /* NO_FASTPATH */

	/* free palloc'ed arguments */
	for (i = 0; i < nargs; ++i)
	{
		if (palloced & (1 << i))
			pfree(arg[i]);
	}

	/*
	 * If this is an ordinary query (not a retrieve portal p ...), then we
	 * return the data to the user.  If the return value was palloc'ed,
	 * then it must also be freed.
	 */
#ifndef NO_FASTPATH
	SendFunctionResult(fid, retval, fip->retbyval, fip->retlen);
#else
	SendFunctionResult(fid, retval, fip->retbyval, 0);
#endif	 /* NO_FASTPATH */

	if (!fip->retbyval)
		pfree(retval);

	return 0;
}
