/*-------------------------------------------------------------------------
 *
 * functions.c
 *	  Routines to handle functions called from the executor
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/executor/functions.c,v 1.65 2003-05-08 18:16:36 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "executor/execdefs.h"
#include "executor/executor.h"
#include "executor/functions.h"
#include "tcop/pquery.h"
#include "tcop/tcopprot.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/syscache.h"


/*
 * We have an execution_state record for each query in a function.  Each
 * record contains a querytree and plantree for its query.  If the query
 * is currently in F_EXEC_RUN state then there's a QueryDesc too.
 */
typedef enum
{
	F_EXEC_START, F_EXEC_RUN, F_EXEC_DONE
} ExecStatus;

typedef struct local_es
{
	struct local_es *next;
	ExecStatus	status;
	Query	   *query;
	Plan	   *plan;
	QueryDesc  *qd;				/* null unless status == RUN */
} execution_state;

#define LAST_POSTQUEL_COMMAND(es) ((es)->next == (execution_state *) NULL)


/*
 * An SQLFunctionCache record is built during the first call,
 * and linked to from the fn_extra field of the FmgrInfo struct.
 */

typedef struct
{
	int			typlen;			/* length of the return type */
	bool		typbyval;		/* true if return type is pass by value */
	bool		returnsTuple;	/* true if return type is a tuple */
	bool		shutdown_reg;	/* true if registered shutdown callback */

	TupleTableSlot *funcSlot;	/* if one result we need to copy it before
								 * we end execution of the function and
								 * free stuff */

	ParamListInfo paramLI;		/* Param list representing current args */

	/* head of linked list of execution_state records */
	execution_state *func_state;
} SQLFunctionCache;

typedef SQLFunctionCache *SQLFunctionCachePtr;


/* non-export function prototypes */
static execution_state *init_execution_state(char *src,
					 Oid *argOidVect, int nargs);
static void init_sql_fcache(FmgrInfo *finfo);
static void postquel_start(execution_state *es, SQLFunctionCachePtr fcache);
static TupleTableSlot *postquel_getnext(execution_state *es);
static void postquel_end(execution_state *es);
static void postquel_sub_params(SQLFunctionCachePtr fcache,
								FunctionCallInfo fcinfo);
static Datum postquel_execute(execution_state *es,
				 FunctionCallInfo fcinfo,
				 SQLFunctionCachePtr fcache);
static void ShutdownSQLFunction(Datum arg);


static execution_state *
init_execution_state(char *src, Oid *argOidVect, int nargs)
{
	execution_state *firstes;
	execution_state *preves;
	List	   *queryTree_list,
			   *qtl_item;

	queryTree_list = pg_parse_and_rewrite(src, argOidVect, nargs);

	firstes = NULL;
	preves = NULL;

	foreach(qtl_item, queryTree_list)
	{
		Query	   *queryTree = lfirst(qtl_item);
		Plan	   *planTree;
		execution_state *newes;

		planTree = pg_plan_query(queryTree);

		newes = (execution_state *) palloc(sizeof(execution_state));
		if (preves)
			preves->next = newes;
		else
			firstes = newes;

		newes->next = NULL;
		newes->status = F_EXEC_START;
		newes->query = queryTree;
		newes->plan = planTree;
		newes->qd = NULL;

		preves = newes;
	}

	return firstes;
}


static void
init_sql_fcache(FmgrInfo *finfo)
{
	Oid			foid = finfo->fn_oid;
	HeapTuple	procedureTuple;
	HeapTuple	typeTuple;
	Form_pg_proc procedureStruct;
	Form_pg_type typeStruct;
	SQLFunctionCachePtr fcache;
	Oid		   *argOidVect;
	char	   *src;
	int			nargs;
	Datum		tmp;
	bool		isNull;

	/*
	 * get the procedure tuple corresponding to the given function Oid
	 */
	procedureTuple = SearchSysCache(PROCOID,
									ObjectIdGetDatum(foid),
									0, 0, 0);
	if (!HeapTupleIsValid(procedureTuple))
		elog(ERROR, "init_sql_fcache: Cache lookup failed for procedure %u",
			 foid);

	procedureStruct = (Form_pg_proc) GETSTRUCT(procedureTuple);

	/*
	 * get the return type from the procedure tuple
	 */
	typeTuple = SearchSysCache(TYPEOID,
						   ObjectIdGetDatum(procedureStruct->prorettype),
							   0, 0, 0);
	if (!HeapTupleIsValid(typeTuple))
		elog(ERROR, "init_sql_fcache: Cache lookup failed for type %u",
			 procedureStruct->prorettype);

	typeStruct = (Form_pg_type) GETSTRUCT(typeTuple);

	fcache = (SQLFunctionCachePtr) palloc0(sizeof(SQLFunctionCache));

	/*
	 * get the type length and by-value flag from the type tuple
	 */
	fcache->typlen = typeStruct->typlen;

	if (typeStruct->typtype != 'c' &&
		procedureStruct->prorettype != RECORDOID)
	{
		/* The return type is not a composite type, so just use byval */
		fcache->typbyval = typeStruct->typbyval;
		fcache->returnsTuple = false;
	}
	else
	{
		/*
		 * This is a hack.	We assume here that any function returning a
		 * tuple returns it by reference.  This needs to be fixed, since
		 * actually the mechanism isn't quite like return-by-reference.
		 */
		fcache->typbyval = false;
		fcache->returnsTuple = true;
	}

	/*
	 * If we are returning exactly one result then we have to copy tuples
	 * and by reference results because we have to end the execution
	 * before we return the results.  When you do this everything
	 * allocated by the executor (i.e. slots and tuples) is freed.
	 */
	if (!finfo->fn_retset && !fcache->typbyval)
		fcache->funcSlot = MakeTupleTableSlot();
	else
		fcache->funcSlot = NULL;

	/*
	 * Parse and plan the queries.  We need the argument info to pass
	 * to the parser.
	 */
	nargs = procedureStruct->pronargs;

	if (nargs > 0)
	{
		argOidVect = (Oid *) palloc(nargs * sizeof(Oid));
		memcpy(argOidVect,
			   procedureStruct->proargtypes,
			   nargs * sizeof(Oid));
	}
	else
		argOidVect = (Oid *) NULL;

	tmp = SysCacheGetAttr(PROCOID,
						  procedureTuple,
						  Anum_pg_proc_prosrc,
						  &isNull);
	if (isNull)
		elog(ERROR, "init_sql_fcache: null prosrc for procedure %u",
			 foid);
	src = DatumGetCString(DirectFunctionCall1(textout, tmp));

	fcache->func_state = init_execution_state(src, argOidVect, nargs);

	pfree(src);

	ReleaseSysCache(typeTuple);
	ReleaseSysCache(procedureTuple);

	finfo->fn_extra = (void *) fcache;
}


static void
postquel_start(execution_state *es, SQLFunctionCachePtr fcache)
{
	Assert(es->qd == NULL);
	es->qd = CreateQueryDesc(es->query, es->plan,
							 None_Receiver,
							 fcache->paramLI, false);

	/* Utility commands don't need Executor. */
	if (es->qd->operation != CMD_UTILITY)
		ExecutorStart(es->qd, false);

	es->status = F_EXEC_RUN;
}

static TupleTableSlot *
postquel_getnext(execution_state *es)
{
	long		count;

	if (es->qd->operation == CMD_UTILITY)
	{
		/*
		 * Process a utility command. (create, destroy...)	DZ - 30-8-1996
		 */
		ProcessUtility(es->qd->parsetree->utilityStmt, es->qd->dest, NULL);
		if (!LAST_POSTQUEL_COMMAND(es))
			CommandCounterIncrement();
		return (TupleTableSlot *) NULL;
	}

	/* If it's not the last command, just run it to completion */
	count = (LAST_POSTQUEL_COMMAND(es)) ? 1L : 0L;

	return ExecutorRun(es->qd, ForwardScanDirection, count);
}

static void
postquel_end(execution_state *es)
{
	/* Utility commands don't need Executor. */
	if (es->qd->operation != CMD_UTILITY)
		ExecutorEnd(es->qd);

	FreeQueryDesc(es->qd);

	es->qd = NULL;

	es->status = F_EXEC_DONE;
}

/* Build ParamListInfo array representing current arguments */
static void
postquel_sub_params(SQLFunctionCachePtr fcache,
					FunctionCallInfo fcinfo)
{
	ParamListInfo paramLI;
	int			nargs = fcinfo->nargs;

	if (nargs > 0)
	{
		int			i;

		paramLI = (ParamListInfo) palloc0((nargs + 1) * sizeof(ParamListInfoData));

		for (i = 0; i < nargs; i++)
		{
			paramLI[i].kind = PARAM_NUM;
			paramLI[i].id = i + 1;
			paramLI[i].value = fcinfo->arg[i];
			paramLI[i].isnull = fcinfo->argnull[i];
		}
		paramLI[nargs].kind = PARAM_INVALID;
	}
	else
		paramLI = (ParamListInfo) NULL;

	if (fcache->paramLI)
		pfree(fcache->paramLI);

	fcache->paramLI = paramLI;
}

static TupleTableSlot *
copy_function_result(SQLFunctionCachePtr fcache,
					 TupleTableSlot *resultSlot)
{
	TupleTableSlot *funcSlot;
	TupleDesc	resultTd;
	HeapTuple	resultTuple;
	HeapTuple	newTuple;

	Assert(!TupIsNull(resultSlot));
	resultTuple = resultSlot->val;

	funcSlot = fcache->funcSlot;

	if (funcSlot == NULL)
		return resultSlot;		/* no need to copy result */

	/*
	 * If first time through, we have to initialize the funcSlot's tuple
	 * descriptor.
	 */
	if (funcSlot->ttc_tupleDescriptor == NULL)
	{
		resultTd = CreateTupleDescCopy(resultSlot->ttc_tupleDescriptor);
		ExecSetSlotDescriptor(funcSlot, resultTd, true);
		ExecSetSlotDescriptorIsNew(funcSlot, true);
	}

	newTuple = heap_copytuple(resultTuple);

	return ExecStoreTuple(newTuple, funcSlot, InvalidBuffer, true);
}

static Datum
postquel_execute(execution_state *es,
				 FunctionCallInfo fcinfo,
				 SQLFunctionCachePtr fcache)
{
	TupleTableSlot *slot;
	Datum		value;

	if (es->status == F_EXEC_START)
		postquel_start(es, fcache);

	slot = postquel_getnext(es);

	if (TupIsNull(slot))
	{
		postquel_end(es);
		fcinfo->isnull = true;

		/*
		 * If this isn't the last command for the function we have to
		 * increment the command counter so that subsequent commands can
		 * see changes made by previous ones.
		 */
		if (!LAST_POSTQUEL_COMMAND(es))
			CommandCounterIncrement();
		return (Datum) NULL;
	}

	if (LAST_POSTQUEL_COMMAND(es))
	{
		TupleTableSlot *resSlot;

		/*
		 * Copy the result.  copy_function_result is smart enough to do
		 * nothing when no action is called for.  This helps reduce the
		 * logic and code redundancy here.
		 */
		resSlot = copy_function_result(fcache, slot);

		/*
		 * If we are supposed to return a tuple, we return the tuple slot
		 * pointer converted to Datum.	If we are supposed to return a
		 * simple value, then project out the first attribute of the
		 * result tuple (ie, take the first result column of the final
		 * SELECT).
		 */
		if (fcache->returnsTuple)
		{
			/*
			 * XXX do we need to remove junk attrs from the result tuple?
			 * Probably OK to leave them, as long as they are at the end.
			 */
			value = PointerGetDatum(resSlot);
			fcinfo->isnull = false;
		}
		else
		{
			value = heap_getattr(resSlot->val,
								 1,
								 resSlot->ttc_tupleDescriptor,
								 &(fcinfo->isnull));

			/*
			 * Note: if result type is pass-by-reference then we are
			 * returning a pointer into the tuple copied by
			 * copy_function_result.  This is OK.
			 */
		}

		/*
		 * If this is a single valued function we have to end the function
		 * execution now.
		 */
		if (!fcinfo->flinfo->fn_retset)
			postquel_end(es);

		return value;
	}

	/*
	 * If this isn't the last command for the function, we don't return
	 * any results, but we have to increment the command counter so that
	 * subsequent commands can see changes made by previous ones.
	 */
	CommandCounterIncrement();
	return (Datum) NULL;
}

Datum
fmgr_sql(PG_FUNCTION_ARGS)
{
	MemoryContext oldcontext;
	SQLFunctionCachePtr fcache;
	execution_state *es;
	Datum		result = 0;

	/*
	 * Switch to context in which the fcache lives.  This ensures that
	 * parsetrees, plans, etc, will have sufficient lifetime.  The
	 * sub-executor is responsible for deleting per-tuple information.
	 */
	oldcontext = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);

	/*
	 * Initialize fcache (build plans) if first time through.
	 */
	fcache = (SQLFunctionCachePtr) fcinfo->flinfo->fn_extra;
	if (fcache == NULL)
	{
		init_sql_fcache(fcinfo->flinfo);
		fcache = (SQLFunctionCachePtr) fcinfo->flinfo->fn_extra;
	}
	es = fcache->func_state;

	/*
	 * Convert params to appropriate format if starting a fresh execution.
	 * (If continuing execution, we can re-use prior params.)
	 */
	if (es && es->status == F_EXEC_START)
		postquel_sub_params(fcache, fcinfo);

	/*
	 * Find first unfinished query in function.
	 */
	while (es && es->status == F_EXEC_DONE)
		es = es->next;

	/*
	 * Execute each command in the function one after another until we're
	 * executing the final command and get a result or we run out of
	 * commands.
	 */
	while (es)
	{
		result = postquel_execute(es, fcinfo, fcache);
		if (es->status != F_EXEC_DONE)
			break;
		es = es->next;
	}

	/*
	 * If we've gone through every command in this function, we are done.
	 */
	if (es == (execution_state *) NULL)
	{
		/*
		 * Reset the execution states to start over again on next call.
		 */
		es = fcache->func_state;
		while (es)
		{
			es->status = F_EXEC_START;
			es = es->next;
		}

		/*
		 * Let caller know we're finished.
		 */
		if (fcinfo->flinfo->fn_retset)
		{
			ReturnSetInfo *rsi = (ReturnSetInfo *) fcinfo->resultinfo;

			if (rsi && IsA(rsi, ReturnSetInfo))
				rsi->isDone = ExprEndResult;
			else
				elog(ERROR, "Set-valued function called in context that cannot accept a set");
			fcinfo->isnull = true;
			result = (Datum) 0;

			/* Deregister shutdown callback, if we made one */
			if (fcache->shutdown_reg)
			{
				UnregisterExprContextCallback(rsi->econtext,
											  ShutdownSQLFunction,
											  PointerGetDatum(fcache));
				fcache->shutdown_reg = false;
			}
		}

		MemoryContextSwitchTo(oldcontext);

		return result;
	}

	/*
	 * If we got a result from a command within the function it has to be
	 * the final command.  All others shouldn't be returning anything.
	 */
	Assert(LAST_POSTQUEL_COMMAND(es));

	/*
	 * Let caller know we're not finished.
	 */
	if (fcinfo->flinfo->fn_retset)
	{
		ReturnSetInfo *rsi = (ReturnSetInfo *) fcinfo->resultinfo;

		if (rsi && IsA(rsi, ReturnSetInfo))
			rsi->isDone = ExprMultipleResult;
		else
			elog(ERROR, "Set-valued function called in context that cannot accept a set");

		/*
		 * Ensure we will get shut down cleanly if the exprcontext is not
		 * run to completion.
		 */
		if (!fcache->shutdown_reg)
		{
			RegisterExprContextCallback(rsi->econtext,
										ShutdownSQLFunction,
										PointerGetDatum(fcache));
			fcache->shutdown_reg = true;
		}
	}

	MemoryContextSwitchTo(oldcontext);

	return result;
}

/*
 * callback function in case a function-returning-set needs to be shut down
 * before it has been run to completion
 */
static void
ShutdownSQLFunction(Datum arg)
{
	SQLFunctionCachePtr fcache = (SQLFunctionCachePtr) DatumGetPointer(arg);
	execution_state *es = fcache->func_state;

	while (es != NULL)
	{
		/* Shut down anything still running */
		if (es->status == F_EXEC_RUN)
			postquel_end(es);
		/* Reset states to START in case we're called again */
		es->status = F_EXEC_START;
		es = es->next;
	}

	/* execUtils will deregister the callback... */
	fcache->shutdown_reg = false;
}
