/**********************************************************************
 * pl_exec.c		- Executor for the PL/pgSQL
 *			  procedural language
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/pl/plpgsql/src/pl_exec.c,v 1.30 2000-09-05 09:02:18 wieck Exp $
 *
 *	  This software is copyrighted by Jan Wieck - Hamburg.
 *
 *	  The author hereby grants permission  to  use,  copy,	modify,
 *	  distribute,  and	license this software and its documentation
 *	  for any purpose, provided that existing copyright notices are
 *	  retained	in	all  copies  and  that	this notice is included
 *	  verbatim in any distributions. No written agreement, license,
 *	  or  royalty  fee	is required for any of the authorized uses.
 *	  Modifications to this software may be  copyrighted  by  their
 *	  author  and  need  not  follow  the licensing terms described
 *	  here, provided that the new terms are  clearly  indicated  on
 *	  the first page of each file where they apply.
 *
 *	  IN NO EVENT SHALL THE AUTHOR OR DISTRIBUTORS BE LIABLE TO ANY
 *	  PARTY  FOR  DIRECT,	INDIRECT,	SPECIAL,   INCIDENTAL,	 OR
 *	  CONSEQUENTIAL   DAMAGES  ARISING	OUT  OF  THE  USE  OF  THIS
 *	  SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF, EVEN
 *	  IF  THE  AUTHOR  HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 *	  DAMAGE.
 *
 *	  THE  AUTHOR  AND	DISTRIBUTORS  SPECIFICALLY	 DISCLAIM	ANY
 *	  WARRANTIES,  INCLUDING,  BUT	NOT  LIMITED  TO,  THE	IMPLIED
 *	  WARRANTIES  OF  MERCHANTABILITY,	FITNESS  FOR  A  PARTICULAR
 *	  PURPOSE,	AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON
 *	  AN "AS IS" BASIS, AND THE AUTHOR	AND  DISTRIBUTORS  HAVE  NO
 *	  OBLIGATION   TO	PROVIDE   MAINTENANCE,	 SUPPORT,  UPDATES,
 *	  ENHANCEMENTS, OR MODIFICATIONS.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>

#include "plpgsql.h"
#include "pl.tab.h"

#include "access/heapam.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/trigger.h"
#include "executor/spi.h"
#include "executor/spi_priv.h"
#include "fmgr.h"
#include "parser/parse_expr.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


static PLpgSQL_function *error_info_func = NULL;
static PLpgSQL_stmt *error_info_stmt = NULL;
static char *error_info_text = NULL;


/************************************************************
 * Local function forward declarations
 ************************************************************/
static PLpgSQL_var *copy_var(PLpgSQL_var * var);
static PLpgSQL_rec *copy_rec(PLpgSQL_rec * rec);

static int exec_stmt_block(PLpgSQL_execstate * estate,
				PLpgSQL_stmt_block * block);
static int exec_stmts(PLpgSQL_execstate * estate,
		   PLpgSQL_stmts * stmts);
static int exec_stmt(PLpgSQL_execstate * estate,
		  PLpgSQL_stmt * stmt);
static int exec_stmt_assign(PLpgSQL_execstate * estate,
				 PLpgSQL_stmt_assign * stmt);
static int exec_stmt_getdiag(PLpgSQL_execstate * estate,
                                 PLpgSQL_stmt_getdiag * stmt);
static int exec_stmt_if(PLpgSQL_execstate * estate,
			 PLpgSQL_stmt_if * stmt);
static int exec_stmt_loop(PLpgSQL_execstate * estate,
			   PLpgSQL_stmt_loop * stmt);
static int exec_stmt_while(PLpgSQL_execstate * estate,
				PLpgSQL_stmt_while * stmt);
static int exec_stmt_fori(PLpgSQL_execstate * estate,
			   PLpgSQL_stmt_fori * stmt);
static int exec_stmt_fors(PLpgSQL_execstate * estate,
			   PLpgSQL_stmt_fors * stmt);
static int exec_stmt_select(PLpgSQL_execstate * estate,
				 PLpgSQL_stmt_select * stmt);
static int exec_stmt_exit(PLpgSQL_execstate * estate,
			   PLpgSQL_stmt_exit * stmt);
static int exec_stmt_return(PLpgSQL_execstate * estate,
				 PLpgSQL_stmt_return * stmt);
static int exec_stmt_raise(PLpgSQL_execstate * estate,
				PLpgSQL_stmt_raise * stmt);
static int exec_stmt_execsql(PLpgSQL_execstate * estate,
				  PLpgSQL_stmt_execsql * stmt);
static int exec_stmt_dynexecute(PLpgSQL_execstate * estate,
				  PLpgSQL_stmt_dynexecute * stmt);
static int exec_stmt_dynfors(PLpgSQL_execstate * estate,
			   PLpgSQL_stmt_dynfors * stmt);

static void exec_prepare_plan(PLpgSQL_execstate * estate,
				  PLpgSQL_expr * expr);
static bool exec_simple_check_node(Node *node);
static void exec_simple_check_plan(PLpgSQL_expr * expr);
static void exec_eval_clear_fcache(Node *node);
static Datum exec_eval_simple_expr(PLpgSQL_execstate * estate,
					  PLpgSQL_expr * expr,
					  bool *isNull,
					  Oid *rettype);

static void exec_assign_expr(PLpgSQL_execstate * estate,
				 PLpgSQL_datum * target,
				 PLpgSQL_expr * expr);
static void exec_assign_value(PLpgSQL_execstate * estate,
				  PLpgSQL_datum * target,
				  Datum value, Oid valtype, bool *isNull);
static Datum exec_eval_expr(PLpgSQL_execstate * estate,
			   PLpgSQL_expr * expr,
			   bool *isNull,
			   Oid *rettype);
static int exec_run_select(PLpgSQL_execstate * estate,
				PLpgSQL_expr * expr, int maxtuples);
static void exec_move_row(PLpgSQL_execstate * estate,
			  PLpgSQL_rec * rec,
			  PLpgSQL_row * row,
			  HeapTuple tup, TupleDesc tupdesc);
static Datum exec_cast_value(Datum value, Oid valtype,
				Oid reqtype,
				FmgrInfo *reqinput,
				Oid reqtypelem,
				int32 reqtypmod,
				bool *isnull);
static void exec_set_found(PLpgSQL_execstate * estate, bool state);


/* ----------
 * plpgsql_exec_function	Called by the call handler for
 *				function execution.
 * ----------
 */
Datum
plpgsql_exec_function(PLpgSQL_function * func, FunctionCallInfo fcinfo)
{
	PLpgSQL_execstate estate;
	int			i;
	sigjmp_buf	save_restart;
	PLpgSQL_function *save_efunc;
	PLpgSQL_stmt *save_estmt;
	char	   *save_etext;

	/* ----------
	 * Setup debug error info and catch elog()
	 * ----------
	 */
	save_efunc = error_info_func;
	save_estmt = error_info_stmt;
	save_etext = error_info_text;

	error_info_func = func;
	error_info_stmt = NULL;
	error_info_text = "while initialization of execution state";

	memcpy(&save_restart, &Warn_restart, sizeof(save_restart));
	if (sigsetjmp(Warn_restart, 1) != 0)
	{
		memcpy(&Warn_restart, &save_restart, sizeof(Warn_restart));

		/* ----------
		 * If we are the first of cascaded error catchings,
		 * print where this happened
		 * ----------
		 */
		if (error_info_func != NULL)
		{
			elog(DEBUG, "Last error occured while executing PL/pgSQL function %s",
				 error_info_func->fn_name);
			if (error_info_stmt != NULL)
			{
				char	   *stmttype;

				switch (error_info_stmt->cmd_type)
				{
					case PLPGSQL_STMT_BLOCK:
						stmttype = "blocks variable initialization";
						break;
					case PLPGSQL_STMT_ASSIGN:
						stmttype = "assignment";
						break;
					case PLPGSQL_STMT_GETDIAG:
						stmttype = "get diagnostics";
						break;
					case PLPGSQL_STMT_IF:
						stmttype = "if";
						break;
					case PLPGSQL_STMT_LOOP:
						stmttype = "loop";
						break;
					case PLPGSQL_STMT_WHILE:
						stmttype = "while";
						break;
					case PLPGSQL_STMT_FORI:
						stmttype = "for with integer loopvar";
						break;
					case PLPGSQL_STMT_FORS:
						stmttype = "for over select rows";
						break;
					case PLPGSQL_STMT_SELECT:
						stmttype = "select into variables";
						break;
					case PLPGSQL_STMT_EXIT:
						stmttype = "exit";
						break;
					case PLPGSQL_STMT_RETURN:
						stmttype = "return";
						break;
					case PLPGSQL_STMT_RAISE:
						stmttype = "raise";
						break;
					case PLPGSQL_STMT_EXECSQL:
						stmttype = "SQL statement";
						break;
					case PLPGSQL_STMT_DYNEXECUTE:
						stmttype = "execute statement";
						break;
					case PLPGSQL_STMT_DYNFORS:
						stmttype = "for over execute statement";
						break;
					default:
						stmttype = "unknown";
						break;
				}
				elog(DEBUG, "line %d at %s", error_info_stmt->lineno,
					 stmttype);
			}
			else
			{
				if (error_info_text != NULL)
					elog(DEBUG, "%s", error_info_text);
				else
					elog(DEBUG, "no more error information available");
			}

			error_info_func = NULL;
			error_info_stmt = NULL;
			error_info_text = NULL;
		}

		siglongjmp(Warn_restart, 1);
	}


	/* ----------
	 * Setup the execution state
	 * ----------
	 */
	estate.retval = 0;
	estate.retisnull = false;
	estate.rettype = InvalidOid;
	estate.retistuple = func->fn_retistuple;
	estate.retisset = func->fn_retset;
	estate.exitlabel = NULL;

	estate.found_varno = func->found_varno;
	estate.ndatums = func->ndatums;
	estate.datums = palloc(sizeof(PLpgSQL_datum *) * estate.ndatums);

	/* ----------
	 * Make local execution copies of all the datums
	 * ----------
	 */
	for (i = 0; i < func->ndatums; i++)
	{
		switch (func->datums[i]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				estate.datums[i] = (PLpgSQL_datum *)
					copy_var((PLpgSQL_var *) (func->datums[i]));
				break;

			case PLPGSQL_DTYPE_REC:
				estate.datums[i] = (PLpgSQL_datum *)
					copy_rec((PLpgSQL_rec *) (func->datums[i]));
				break;

			case PLPGSQL_DTYPE_ROW:
			case PLPGSQL_DTYPE_RECFIELD:
				estate.datums[i] = func->datums[i];
				break;

			default:
				elog(ERROR, "unknown dtype %d in plpgsql_exec_function()",
					 func->datums[i]->dtype);
		}
	}

	/* ----------
	 * Put the actual call argument values into the variables
	 * ----------
	 */
	error_info_text = "while putting call arguments to local variables";
	for (i = 0; i < func->fn_nargs; i++)
	{
		int			n = func->fn_argvarnos[i];

		switch (estate.datums[n]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) estate.datums[n];

					var->value = fcinfo->arg[i];
					var->isnull = fcinfo->argnull[i];
					var->shouldfree = false;
				}
				break;

			case PLPGSQL_DTYPE_ROW:
				{
					PLpgSQL_row *row = (PLpgSQL_row *) estate.datums[n];
					TupleTableSlot *slot = (TupleTableSlot *) fcinfo->arg[i];
					HeapTuple	tup;
					TupleDesc	tupdesc;

					Assert(slot != NULL && ! fcinfo->argnull[i]);
					tup = slot->val;
					tupdesc = slot->ttc_tupleDescriptor;
					exec_move_row(&estate, NULL, row, tup, tupdesc);
				}
				break;

			default:
				elog(ERROR, "unknown dtype %d in plpgsql_exec_function()",
					 func->datums[i]->dtype);
		}
	}

	/* ----------
	 * Initialize the other variables to NULL values for now.
	 * The default values are set when the blocks are entered.
	 * ----------
	 */
	error_info_text = "while initializing local variables to NULL";
	for (i = estate.found_varno; i < estate.ndatums; i++)
	{
		switch (estate.datums[i]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) estate.datums[i];

					var->value = 0;
					var->isnull = true;
					var->shouldfree = false;
				}
				break;

			case PLPGSQL_DTYPE_ROW:
			case PLPGSQL_DTYPE_REC:
			case PLPGSQL_DTYPE_RECFIELD:
				break;

			default:
				elog(ERROR, "unknown dtype %d in plpgsql_exec_function()",
					 func->datums[i]->dtype);
		}
	}

	/* ----------
	 * Set the magic variable FOUND to false
	 * ----------
	 */
	exec_set_found(&estate, false);

	/* ----------
	 * Now call the toplevel block of statements
	 * ----------
	 */
	error_info_text = NULL;
	error_info_stmt = (PLpgSQL_stmt *) (func->action);
	if (exec_stmt_block(&estate, func->action) != PLPGSQL_RC_RETURN)
	{
		error_info_stmt = NULL;
		error_info_text = "at END of toplevel PL block";
		elog(ERROR, "control reaches end of function without RETURN");
	}

	/* ----------
	 * We got a return value - process it
	 * ----------
	 */
	error_info_stmt = NULL;
	error_info_text = "while casting return value to functions return type";

	fcinfo->isnull = estate.retisnull;

	if (!estate.retistuple)
	{
		estate.retval = exec_cast_value(estate.retval, estate.rettype,
										func->fn_rettype,
										&(func->fn_retinput),
										func->fn_rettypelem,
										-1,
										&fcinfo->isnull);

		/* ----------
		 * If the functions return type isn't by value,
		 * copy the value into upper executor memory context.
		 * ----------
		 */
		if (!fcinfo->isnull && !func->fn_retbyval)
		{
			int			len;
			Datum		tmp;

			if (func->fn_rettyplen < 0)
				len = VARSIZE(estate.retval);
			else
				len = func->fn_rettyplen;

			tmp = (Datum) SPI_palloc(len);
			memcpy((void *) tmp, (void *) estate.retval, len);
			estate.retval = tmp;
		}
	}

	/* ----------
	 * Restore the previous error info and elog() jump target
	 * ----------
	 */
	error_info_func = save_efunc;
	error_info_stmt = save_estmt;
	error_info_text = save_etext;
	memcpy(&Warn_restart, &save_restart, sizeof(Warn_restart));

	/* ----------
	 * Return the functions result
	 * ----------
	 */
	return estate.retval;
}


/* ----------
 * plpgsql_exec_trigger		Called by the call handler for
 *				trigger execution.
 * ----------
 */
HeapTuple
plpgsql_exec_trigger(PLpgSQL_function * func,
					 TriggerData *trigdata)
{
	PLpgSQL_execstate estate;
	int			i;
	sigjmp_buf	save_restart;
	PLpgSQL_function *save_efunc;
	PLpgSQL_stmt *save_estmt;
	char	   *save_etext;
	PLpgSQL_rec *rec_new;
	PLpgSQL_rec *rec_old;
	PLpgSQL_var *var;
	HeapTuple	rettup;

	/* ----------
	 * Setup debug error info and catch elog()
	 * ----------
	 */
	save_efunc = error_info_func;
	save_estmt = error_info_stmt;
	save_etext = error_info_text;

	error_info_func = func;
	error_info_stmt = NULL;
	error_info_text = "while initialization of execution state";

	memcpy(&save_restart, &Warn_restart, sizeof(save_restart));
	if (sigsetjmp(Warn_restart, 1) != 0)
	{
		memcpy(&Warn_restart, &save_restart, sizeof(Warn_restart));

		/* ----------
		 * If we are the first of cascaded error catchings,
		 * print where this happened
		 * ----------
		 */
		if (error_info_func != NULL)
		{
			elog(DEBUG, "Last error occured while executing PL/pgSQL function %s",
				 error_info_func->fn_name);
			if (error_info_stmt != NULL)
			{
				char	   *stmttype;

				switch (error_info_stmt->cmd_type)
				{
					case PLPGSQL_STMT_BLOCK:
						stmttype = "blocks variable initialization";
						break;
					case PLPGSQL_STMT_ASSIGN:
						stmttype = "assignment";
						break;
                                        case PLPGSQL_STMT_GETDIAG:
                                                stmttype = "get diagnostics";
                                                break;
					case PLPGSQL_STMT_IF:
						stmttype = "if";
						break;
					case PLPGSQL_STMT_LOOP:
						stmttype = "loop";
						break;
					case PLPGSQL_STMT_WHILE:
						stmttype = "while";
						break;
					case PLPGSQL_STMT_FORI:
						stmttype = "for with integer loopvar";
						break;
					case PLPGSQL_STMT_FORS:
						stmttype = "for over select rows";
						break;
					case PLPGSQL_STMT_SELECT:
						stmttype = "select into variables";
						break;
					case PLPGSQL_STMT_EXIT:
						stmttype = "exit";
						break;
					case PLPGSQL_STMT_RETURN:
						stmttype = "return";
						break;
					case PLPGSQL_STMT_RAISE:
						stmttype = "raise";
						break;
					case PLPGSQL_STMT_EXECSQL:
						stmttype = "SQL statement";
						break;
					case PLPGSQL_STMT_DYNEXECUTE:
						stmttype = "execute statement";
						break;
					case PLPGSQL_STMT_DYNFORS:
						stmttype = "for over execute statement";
						break;
					default:
						stmttype = "unknown";
						break;
				}
				elog(DEBUG, "line %d at %s", error_info_stmt->lineno,
					 stmttype);
			}
			else
			{
				if (error_info_text != NULL)
					elog(DEBUG, "%s", error_info_text);
				else
					elog(DEBUG, "no more error information available");
			}

			error_info_func = NULL;
			error_info_stmt = NULL;
			error_info_text = NULL;
		}

		siglongjmp(Warn_restart, 1);
	}


	/* ----------
	 * Setup the execution state
	 * ----------
	 */
	estate.retval = 0;
	estate.retisnull = false;
	estate.rettype = InvalidOid;
	estate.retistuple = func->fn_retistuple;
	estate.retisset = func->fn_retset;
	estate.exitlabel = NULL;

	estate.found_varno = func->found_varno;
	estate.ndatums = func->ndatums;
	estate.datums = palloc(sizeof(PLpgSQL_datum *) * estate.ndatums);

	/* ----------
	 * Make local execution copies of all the datums
	 * ----------
	 */
	for (i = 0; i < func->ndatums; i++)
	{
		switch (func->datums[i]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				estate.datums[i] = (PLpgSQL_datum *)
					copy_var((PLpgSQL_var *) (func->datums[i]));
				break;

			case PLPGSQL_DTYPE_REC:
				estate.datums[i] = (PLpgSQL_datum *)
					copy_rec((PLpgSQL_rec *) (func->datums[i]));
				break;

			case PLPGSQL_DTYPE_ROW:
			case PLPGSQL_DTYPE_RECFIELD:
			case PLPGSQL_DTYPE_TRIGARG:
				estate.datums[i] = func->datums[i];
				break;

			default:
				elog(ERROR, "unknown dtype %d in plpgsql_exec_function()",
					 func->datums[i]->dtype);
		}
	}

	/* ----------
	 * Put the trig and new tuples into the records
	 * and set the tg_op variable
	 * ----------
	 */
	rec_new = (PLpgSQL_rec *) (estate.datums[func->new_varno]);
	rec_old = (PLpgSQL_rec *) (estate.datums[func->old_varno]);
	var = (PLpgSQL_var *) (estate.datums[func->tg_op_varno]);
	var->isnull = false;

	if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
	{
		rec_new->tup = trigdata->tg_trigtuple;
		rec_new->tupdesc = trigdata->tg_relation->rd_att;
		rec_old->tup = NULL;
		rec_old->tupdesc = NULL;
		var->value = DirectFunctionCall1(textin, CStringGetDatum("INSERT"));
	}
	else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
	{
		rec_new->tup = trigdata->tg_newtuple;
		rec_new->tupdesc = trigdata->tg_relation->rd_att;
		rec_old->tup = trigdata->tg_trigtuple;
		rec_old->tupdesc = trigdata->tg_relation->rd_att;
		var->value = DirectFunctionCall1(textin, CStringGetDatum("UPDATE"));
	}
	else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
	{
		rec_new->tup = NULL;
		rec_new->tupdesc = NULL;
		rec_old->tup = trigdata->tg_trigtuple;
		rec_old->tupdesc = trigdata->tg_relation->rd_att;
		var->value = DirectFunctionCall1(textin, CStringGetDatum("DELETE"));
	}
	else
	{
		rec_new->tup = NULL;
		rec_new->tupdesc = NULL;
		var->value = DirectFunctionCall1(textin, CStringGetDatum("UNKNOWN"));
	}

	/* ----------
	 * Fill all the other special tg_ variables
	 * ----------
	 */
	var = (PLpgSQL_var *) (estate.datums[func->tg_name_varno]);
	var->isnull = false;
	var->value = DirectFunctionCall1(namein,
							CStringGetDatum(trigdata->tg_trigger->tgname));

	var = (PLpgSQL_var *) (estate.datums[func->tg_when_varno]);
	var->isnull = false;
	if (TRIGGER_FIRED_BEFORE(trigdata->tg_event))
		var->value = DirectFunctionCall1(textin, CStringGetDatum("BEFORE"));
	else if (TRIGGER_FIRED_AFTER(trigdata->tg_event))
		var->value = DirectFunctionCall1(textin, CStringGetDatum("AFTER"));
	else
		var->value = DirectFunctionCall1(textin, CStringGetDatum("UNKNOWN"));

	var = (PLpgSQL_var *) (estate.datums[func->tg_level_varno]);
	var->isnull = false;
	if (TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		var->value = DirectFunctionCall1(textin, CStringGetDatum("ROW"));
	else if (TRIGGER_FIRED_FOR_STATEMENT(trigdata->tg_event))
		var->value = DirectFunctionCall1(textin, CStringGetDatum("STATEMENT"));
	else
		var->value = DirectFunctionCall1(textin, CStringGetDatum("UNKNOWN"));

	var = (PLpgSQL_var *) (estate.datums[func->tg_relid_varno]);
	var->isnull = false;
	var->value = (Datum) (trigdata->tg_relation->rd_id);

	var = (PLpgSQL_var *) (estate.datums[func->tg_relname_varno]);
	var->isnull = false;
	var->value = DirectFunctionCall1(namein,
			CStringGetDatum(RelationGetRelationName(trigdata->tg_relation)));

	var = (PLpgSQL_var *) (estate.datums[func->tg_nargs_varno]);
	var->isnull = false;
	var->value = (Datum) (trigdata->tg_trigger->tgnargs);

	/* ----------
	 * Put the actual call argument values into the special
	 * execution state variables
	 * ----------
	 */
	error_info_text = "while putting call arguments to local variables";
	estate.trig_nargs = trigdata->tg_trigger->tgnargs;
	if (estate.trig_nargs == 0)
		estate.trig_argv = NULL;
	else
	{
		estate.trig_argv = palloc(sizeof(Datum) * estate.trig_nargs);
		for (i = 0; i < trigdata->tg_trigger->tgnargs; i++)
			estate.trig_argv[i] = DirectFunctionCall1(textin,
							CStringGetDatum(trigdata->tg_trigger->tgargs[i]));
	}

	/* ----------
	 * Initialize the other variables to NULL values for now.
	 * The default values are set when the blocks are entered.
	 * ----------
	 */
	error_info_text = "while initializing local variables to NULL";
	for (i = estate.found_varno; i < estate.ndatums; i++)
	{
		switch (estate.datums[i]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) estate.datums[i];

					var->value = 0;
					var->isnull = true;
					var->shouldfree = false;
				}
				break;

			case PLPGSQL_DTYPE_ROW:
			case PLPGSQL_DTYPE_REC:
			case PLPGSQL_DTYPE_RECFIELD:
			case PLPGSQL_DTYPE_TRIGARG:
				break;

			default:
				elog(ERROR, "unknown dtype %d in plpgsql_exec_trigger()",
					 func->datums[i]->dtype);
		}
	}

	/* ----------
	 * Set the magic variable FOUND to false
	 * ----------
	 */
	exec_set_found(&estate, false);

	/* ----------
	 * Now call the toplevel block of statements
	 * ----------
	 */
	error_info_text = NULL;
	error_info_stmt = (PLpgSQL_stmt *) (func->action);
	if (exec_stmt_block(&estate, func->action) != PLPGSQL_RC_RETURN)
	{
		error_info_stmt = NULL;
		error_info_text = "at END of toplevel PL block";
		elog(ERROR, "control reaches end of trigger procedure without RETURN");
	}

	/* ----------
	 * Check that the returned tuple structure has the same attributes,
	 * the relation that fired the trigger has.
	 *
	 * XXX This way it is possible, that the trigger returns a tuple
	 *	   where attributes don't have the correct atttypmod's length.
	 *	   It's up to the trigger's programmer to ensure that this
	 *	   doesn't happen. Jan
	 * ----------
	 */
	if (estate.retisnull)
		rettup = NULL;
	else
	{
		TupleDesc	td1 = trigdata->tg_relation->rd_att;
		TupleDesc	td2 = estate.rettupdesc;
		int			i;

		if (td1->natts != td2->natts)
			elog(ERROR, "returned tuple structure doesn't match table of trigger event");
		for (i = 1; i <= td1->natts; i++)
		{
			if (SPI_gettypeid(td1, i) != SPI_gettypeid(td2, i))
				elog(ERROR, "returned tuple structure doesn't match table of trigger event");
		}

		rettup = SPI_copytuple((HeapTuple) (estate.retval));
	}

	/* ----------
	 * Restore the previous error info and elog() jump target
	 * ----------
	 */
	error_info_func = save_efunc;
	error_info_stmt = save_estmt;
	error_info_text = save_etext;
	memcpy(&Warn_restart, &save_restart, sizeof(Warn_restart));

	/* ----------
	 * Return the triggers result
	 * ----------
	 */
	return rettup;
}


/* ----------
 * Support functions for copying local execution variables
 * ----------
 */
static PLpgSQL_var *
copy_var(PLpgSQL_var * var)
{
	PLpgSQL_var *new = palloc(sizeof(PLpgSQL_var));

	memcpy(new, var, sizeof(PLpgSQL_var));

	return new;
}


static PLpgSQL_rec *
copy_rec(PLpgSQL_rec * rec)
{
	PLpgSQL_rec *new = palloc(sizeof(PLpgSQL_rec));

	memcpy(new, rec, sizeof(PLpgSQL_rec));

	return new;
}


/* ----------
 * exec_stmt_block			Execute a block of statements
 * ----------
 */
static int
exec_stmt_block(PLpgSQL_execstate * estate, PLpgSQL_stmt_block * block)
{
	int			rc;
	int			i;
	int			n;

	/* ----------
	 * First initialize all variables declared in this block
	 * ----------
	 */
	for (i = 0; i < block->n_initvars; i++)
	{
		n = block->initvarnos[i];

		switch (estate->datums[n]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) (estate->datums[n]);

					if (!var->isconst || var->isnull)
					{
						if (var->default_val == NULL)
						{
							var->value = (Datum) 0;
							var->isnull = true;
							if (var->notnull)
								elog(ERROR, "variable '%s' declared NOT NULL cannot default to NULL", var->refname);
						}
						else
						{
							exec_assign_expr(estate, (PLpgSQL_datum *) var,
											 var->default_val);
						}
					}
				}
				break;

			case PLPGSQL_DTYPE_REC:
				{
					PLpgSQL_rec *rec = (PLpgSQL_rec *) (estate->datums[n]);

					rec->tup = NULL;
					rec->tupdesc = NULL;
				}
				break;

			case PLPGSQL_DTYPE_RECFIELD:
				break;

			default:
				elog(ERROR, "unknown dtype %d in exec_stmt_block()", estate->datums[n]->dtype);
		}

	}

	/* ----------
	 * Execute the statements in the block's body
	 * ----------
	 */
	rc = exec_stmts(estate, block->body);

	/* ----------
	 * Handle the return code.
	 * ----------
	 */
	switch (rc)
	{
		case PLPGSQL_RC_OK:
			return PLPGSQL_RC_OK;

		case PLPGSQL_RC_EXIT:
			if (estate->exitlabel == NULL)
				return PLPGSQL_RC_OK;
			if (block->label == NULL)
				return PLPGSQL_RC_EXIT;
			if (strcmp(block->label, estate->exitlabel))
				return PLPGSQL_RC_EXIT;
			estate->exitlabel = NULL;
			return PLPGSQL_RC_OK;

		case PLPGSQL_RC_RETURN:
			return PLPGSQL_RC_RETURN;

		default:
			elog(ERROR, "unknown rc %d from exec_stmt()", rc);
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmts			Iterate over a list of statements
 *				as long as their return code is OK
 * ----------
 */
static int
exec_stmts(PLpgSQL_execstate * estate, PLpgSQL_stmts * stmts)
{
	int			rc;
	int			i;

	for (i = 0; i < stmts->stmts_used; i++)
	{
		rc = exec_stmt(estate, (PLpgSQL_stmt *) (stmts->stmts[i]));
		if (rc != PLPGSQL_RC_OK)
			return rc;
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt			Distribute one statement to the statements
 *				type specific execution function.
 * ----------
 */
static int
exec_stmt(PLpgSQL_execstate * estate, PLpgSQL_stmt * stmt)
{
	PLpgSQL_stmt *save_estmt;
	int			rc = -1;

	save_estmt = error_info_stmt;
	error_info_stmt = stmt;

	switch (stmt->cmd_type)
	{
		case PLPGSQL_STMT_BLOCK:
			rc = exec_stmt_block(estate, (PLpgSQL_stmt_block *) stmt);
			break;

		case PLPGSQL_STMT_ASSIGN:
			rc = exec_stmt_assign(estate, (PLpgSQL_stmt_assign *) stmt);
			break;

		case PLPGSQL_STMT_GETDIAG:
			rc = exec_stmt_getdiag(estate, (PLpgSQL_stmt_getdiag *) stmt);
			break;

		case PLPGSQL_STMT_IF:
			rc = exec_stmt_if(estate, (PLpgSQL_stmt_if *) stmt);
			break;

		case PLPGSQL_STMT_LOOP:
			rc = exec_stmt_loop(estate, (PLpgSQL_stmt_loop *) stmt);
			break;

		case PLPGSQL_STMT_WHILE:
			rc = exec_stmt_while(estate, (PLpgSQL_stmt_while *) stmt);
			break;

		case PLPGSQL_STMT_FORI:
			rc = exec_stmt_fori(estate, (PLpgSQL_stmt_fori *) stmt);
			break;

		case PLPGSQL_STMT_FORS:
			rc = exec_stmt_fors(estate, (PLpgSQL_stmt_fors *) stmt);
			break;

		case PLPGSQL_STMT_SELECT:
			rc = exec_stmt_select(estate, (PLpgSQL_stmt_select *) stmt);
			break;

		case PLPGSQL_STMT_EXIT:
			rc = exec_stmt_exit(estate, (PLpgSQL_stmt_exit *) stmt);
			break;

		case PLPGSQL_STMT_RETURN:
			rc = exec_stmt_return(estate, (PLpgSQL_stmt_return *) stmt);
			break;

		case PLPGSQL_STMT_RAISE:
			rc = exec_stmt_raise(estate, (PLpgSQL_stmt_raise *) stmt);
			break;

		case PLPGSQL_STMT_EXECSQL:
			rc = exec_stmt_execsql(estate, (PLpgSQL_stmt_execsql *) stmt);
			break;

		case PLPGSQL_STMT_DYNEXECUTE:
			rc = exec_stmt_dynexecute(estate, (PLpgSQL_stmt_dynexecute *) stmt);
			break;

		case PLPGSQL_STMT_DYNFORS:
			rc = exec_stmt_dynfors(estate, (PLpgSQL_stmt_dynfors *) stmt);
			break;

		default:
			error_info_stmt = save_estmt;
			elog(ERROR, "unknown cmdtype %d in exec_stmt",
				 stmt->cmd_type);
	}

	error_info_stmt = save_estmt;

	return rc;
}


/* ----------
 * exec_stmt_assign			Evaluate an expression and
 *					put the result into a variable.
 * ----------
 */
static int
exec_stmt_assign(PLpgSQL_execstate * estate, PLpgSQL_stmt_assign * stmt)
{
	if (stmt->varno < 0)
		exec_assign_expr(estate, NULL, stmt->expr);
	else
		exec_assign_expr(estate, estate->datums[stmt->varno], stmt->expr);

	return PLPGSQL_RC_OK;
}

/* ----------
 * exec_stmt_getdiag                    Put internal PG information into
 *                                      specified variables.
 * ----------
 */
static int
exec_stmt_getdiag(PLpgSQL_execstate * estate, PLpgSQL_stmt_getdiag * stmt)
{
	int		i;
	PLpgSQL_datum 	*var;
	HeapTuple	typeTup;
        bool            isnull = false;

	for ( i=0 ; i < stmt->nitems ; i++) 
	{
		if ((stmt->targets[i] <= 0))
			break;
	
		var = (estate->datums[stmt->targets[i]]);

		if (var == NULL)
			break;

		switch (stmt->items[i])
		{
			case PLPGSQL_GETDIAG_PROCESSED: 

				exec_assign_value(estate, var, (Datum)SPI_processed, INT4OID, &isnull);
        			break;

			case PLPGSQL_GETDIAG_RESULT:

				exec_assign_value(estate, var, (Datum)SPI_result, INT4OID, &isnull);
                                break;

			default:
			
				elog(ERROR, "unknown attribute request %d in get_diagnostic", stmt->items[i]);
		};
	};
	
        return PLPGSQL_RC_OK;
}

/* ----------
 * exec_stmt_if				Evaluate a bool expression and
 *					execute the true or false body
 *					conditionally.
 * ----------
 */
static int
exec_stmt_if(PLpgSQL_execstate * estate, PLpgSQL_stmt_if * stmt)
{
	Datum		value;
	Oid			valtype;
	bool		isnull = false;

	value = exec_eval_expr(estate, stmt->cond, &isnull, &valtype);

	if (value)
	{
		if (stmt->true_body != NULL)
			return exec_stmts(estate, stmt->true_body);
	}
	else
	{
		if (stmt->false_body != NULL)
			return exec_stmts(estate, stmt->false_body);
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_loop			Loop over statements until
 *					an exit occurs.
 * ----------
 */
static int
exec_stmt_loop(PLpgSQL_execstate * estate, PLpgSQL_stmt_loop * stmt)
{
	int			rc;

	for (;;)
	{
		rc = exec_stmts(estate, stmt->body);

		switch (rc)
		{
			case PLPGSQL_RC_OK:
				break;

			case PLPGSQL_RC_EXIT:
				if (estate->exitlabel == NULL)
					return PLPGSQL_RC_OK;
				if (stmt->label == NULL)
					return PLPGSQL_RC_EXIT;
				if (strcmp(stmt->label, estate->exitlabel))
					return PLPGSQL_RC_EXIT;
				estate->exitlabel = NULL;
				return PLPGSQL_RC_OK;

			case PLPGSQL_RC_RETURN:
				return PLPGSQL_RC_RETURN;

			default:
				elog(ERROR, "unknown rc %d from exec_stmts()", rc);
		}
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_while			Loop over statements as long
 *					as an expression evaluates to
 *					true or an exit occurs.
 * ----------
 */
static int
exec_stmt_while(PLpgSQL_execstate * estate, PLpgSQL_stmt_while * stmt)
{
	Datum		value;
	Oid			valtype;
	bool		isnull = false;
	int			rc;

	for (;;)
	{
		value = exec_eval_expr(estate, stmt->cond, &isnull, &valtype);
		if (!value)
			break;

		rc = exec_stmts(estate, stmt->body);

		switch (rc)
		{
			case PLPGSQL_RC_OK:
				break;

			case PLPGSQL_RC_EXIT:
				if (estate->exitlabel == NULL)
					return PLPGSQL_RC_OK;
				if (stmt->label == NULL)
					return PLPGSQL_RC_EXIT;
				if (strcmp(stmt->label, estate->exitlabel))
					return PLPGSQL_RC_EXIT;
				estate->exitlabel = NULL;
				return PLPGSQL_RC_OK;

			case PLPGSQL_RC_RETURN:
				return PLPGSQL_RC_RETURN;

			default:
				elog(ERROR, "unknown rc %d from exec_stmts()", rc);
		}
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_fori			Iterate an integer variable
 *					from a lower to an upper value.
 *					Loop can be left with exit.
 * ----------
 */
static int
exec_stmt_fori(PLpgSQL_execstate * estate, PLpgSQL_stmt_fori * stmt)
{
	PLpgSQL_var *var;
	Datum		value;
	Oid			valtype;
	bool		isnull = false;
	int			rc;

	/* ----------
	 * Get the value of the lower bound into the loop var
	 * ----------
	 */
	value = exec_eval_expr(estate, stmt->lower, &isnull, &valtype);
	var = (PLpgSQL_var *) (estate->datums[stmt->var->varno]);

	value = exec_cast_value(value, valtype, var->datatype->typoid,
							&(var->datatype->typinput),
							var->datatype->typelem,
							var->datatype->atttypmod, &isnull);
	if (isnull)
		elog(ERROR, "lower bound of FOR loop cannot be NULL");
	var->value = value;
	var->isnull = false;

	/* ----------
	 * Get the value of the upper bound
	 * ----------
	 */
	value = exec_eval_expr(estate, stmt->upper, &isnull, &valtype);
	value = exec_cast_value(value, valtype, var->datatype->typoid,
							&(var->datatype->typinput),
							var->datatype->typelem,
							var->datatype->atttypmod, &isnull);
	if (isnull)
		elog(ERROR, "upper bound of FOR loop cannot be NULL");

	/* ----------
	 * Now do the loop
	 * ----------
	 */
	exec_set_found(estate, false);
	for (;;)
	{
		/* ----------
		 * Check bounds
		 * ----------
		 */
		if (stmt->reverse)
		{
			if ((int4) (var->value) < (int4) value)
				break;
		}
		else
		{
			if ((int4) (var->value) > (int4) value)
				break;
		}
		exec_set_found(estate, true);

		/* ----------
		 * Execute the statements
		 * ----------
		 */
		rc = exec_stmts(estate, stmt->body);

		/* ----------
		 * Check returncode
		 * ----------
		 */
		switch (rc)
		{
			case PLPGSQL_RC_OK:
				break;

			case PLPGSQL_RC_EXIT:
				if (estate->exitlabel == NULL)
					return PLPGSQL_RC_OK;
				if (stmt->label == NULL)
					return PLPGSQL_RC_EXIT;
				if (strcmp(stmt->label, estate->exitlabel))
					return PLPGSQL_RC_EXIT;
				estate->exitlabel = NULL;
				return PLPGSQL_RC_OK;

			case PLPGSQL_RC_RETURN:
				return PLPGSQL_RC_RETURN;

			default:
				elog(ERROR, "unknown rc %d from exec_stmts()", rc);
		}

		/* ----------
		 * Increase/decrease loop var
		 * ----------
		 */
		if (stmt->reverse)
			var->value--;
		else
			var->value++;
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_fors			Execute a query, assign each
 *					tuple to a record or row and
 *					execute a group of statements
 *					for it.
 * ----------
 */
static int
exec_stmt_fors(PLpgSQL_execstate * estate, PLpgSQL_stmt_fors * stmt)
{
	PLpgSQL_rec *rec = NULL;
	PLpgSQL_row *row = NULL;
	SPITupleTable *tuptab;
	int			rc;
	int			i;
	int			n;

	/* ----------
	 * Initialize the global found variable to false
	 * ----------
	 */
	exec_set_found(estate, false);

	/* ----------
	 * Determine if we assign to a record or a row
	 * ----------
	 */
	if (stmt->rec != NULL)
		rec = (PLpgSQL_rec *) (estate->datums[stmt->rec->recno]);
	else
	{
		if (stmt->row != NULL)
			row = (PLpgSQL_row *) (estate->datums[stmt->row->rowno]);
		else
			elog(ERROR, "unsupported target in exec_stmt_fors()");
	}

	/* ----------
	 * Run the query
	 * ----------
	 */
	exec_run_select(estate, stmt->query, 0);
	n = SPI_processed;

	/* ----------
	 * If the query didn't return any row, set the target
	 * to NULL and return.
	 * ----------
	 */
	if (n == 0)
	{
		exec_move_row(estate, rec, row, NULL, NULL);
		return PLPGSQL_RC_OK;
	}

	/* ----------
	 * There are tuples, so set found to true
	 * ----------
	 */
	exec_set_found(estate, true);

	/* ----------
	 * Now do the loop
	 * ----------
	 */
	tuptab = SPI_tuptable;
	SPI_tuptable = NULL;

	for (i = 0; i < n; i++)
	{
		/* ----------
		 * Assign the tuple to the target
		 * ----------
		 */
		exec_move_row(estate, rec, row, tuptab->vals[i], tuptab->tupdesc);

		/* ----------
		 * Execute the statements
		 * ----------
		 */
		rc = exec_stmts(estate, stmt->body);

		/* ----------
		 * Check returncode
		 * ----------
		 */
		switch (rc)
		{
			case PLPGSQL_RC_OK:
				break;

			case PLPGSQL_RC_EXIT:
				if (estate->exitlabel == NULL)
					return PLPGSQL_RC_OK;
				if (stmt->label == NULL)
					return PLPGSQL_RC_EXIT;
				if (strcmp(stmt->label, estate->exitlabel))
					return PLPGSQL_RC_EXIT;
				estate->exitlabel = NULL;
				return PLPGSQL_RC_OK;

			case PLPGSQL_RC_RETURN:
				return PLPGSQL_RC_RETURN;

			default:
				elog(ERROR, "unknown rc %d from exec_stmts()", rc);
		}
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_select			Run a query and assign the first
 *					row to a record or rowtype.
 *					 ----------
 */
static int
exec_stmt_select(PLpgSQL_execstate * estate, PLpgSQL_stmt_select * stmt)
{
	PLpgSQL_rec *rec = NULL;
	PLpgSQL_row *row = NULL;
	SPITupleTable *tuptab;
	int			n;

	/* ----------
	 * Initialize the global found variable to false
	 * ----------
	 */
	exec_set_found(estate, false);

	/* ----------
	 * Determine if we assign to a record or a row
	 * ----------
	 */
	if (stmt->rec != NULL)
		rec = (PLpgSQL_rec *) (estate->datums[stmt->rec->recno]);
	else
	{
		if (stmt->row != NULL)
			row = (PLpgSQL_row *) (estate->datums[stmt->row->rowno]);
		else
			elog(ERROR, "unsupported target in exec_stmt_select()");
	}

	/* ----------
	 * Run the query
	 * ----------
	 */
	exec_run_select(estate, stmt->query, 1);
	n = SPI_processed;

	/* ----------
	 * If the query didn't return any row, set the target
	 * to NULL and return.
	 * ----------
	 */
	if (n == 0)
	{
		exec_move_row(estate, rec, row, NULL, NULL);
		return PLPGSQL_RC_OK;
	}

	/* ----------
	 * Put the result into the target and set found to true
	 * ----------
	 */
	tuptab = SPI_tuptable;
	SPI_tuptable = NULL;

	exec_move_row(estate, rec, row, tuptab->vals[0], tuptab->tupdesc);

	exec_set_found(estate, true);

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_exit			Start exiting loop(s) or blocks
 * ----------
 */
static int
exec_stmt_exit(PLpgSQL_execstate * estate, PLpgSQL_stmt_exit * stmt)
{
	Datum		value;
	Oid			valtype;
	bool		isnull = false;

	/* ----------
	 * If the exit has a condition, check that it's true
	 * ----------
	 */
	if (stmt->cond != NULL)
	{
		value = exec_eval_expr(estate, stmt->cond, &isnull, &valtype);
		if (!value)
			return PLPGSQL_RC_OK;
	}

	estate->exitlabel = stmt->label;
	return PLPGSQL_RC_EXIT;
}


/* ----------
 * exec_stmt_return			Evaluate an expression and start
 *					returning from the function.
 * ----------
 */
static int
exec_stmt_return(PLpgSQL_execstate * estate, PLpgSQL_stmt_return * stmt)
{
	if (estate->retistuple)
	{
		if (stmt->retrecno >= 0)
		{
			PLpgSQL_rec *rec = (PLpgSQL_rec *) (estate->datums[stmt->retrecno]);

			estate->retval = (Datum) (rec->tup);
			estate->rettupdesc = rec->tupdesc;
			estate->retisnull = !HeapTupleIsValid(rec->tup);

			return PLPGSQL_RC_RETURN;
		}

		if (stmt->expr == NULL)
		{
			estate->retval = (Datum) 0;
			estate->rettupdesc = NULL;
			estate->retisnull = true;
		}
		else
		{
			exec_run_select(estate, stmt->expr, 1);
			estate->retval = (Datum) SPI_copytuple(SPI_tuptable->vals[0]);
			estate->rettupdesc = SPI_tuptable->tupdesc;
			estate->retisnull = false;
		}
		return PLPGSQL_RC_RETURN;
	}

	estate->retval = exec_eval_expr(estate, stmt->expr,
									&(estate->retisnull),
									&(estate->rettype));

	return PLPGSQL_RC_RETURN;
}


/* ----------
 * exec_stmt_raise			Build a message and throw it with
 *					elog()
 * ----------
 */
static int
exec_stmt_raise(PLpgSQL_execstate * estate, PLpgSQL_stmt_raise * stmt)
{
	HeapTuple	typetup;
	Form_pg_type typeStruct;
	FmgrInfo	finfo_output;
	char	   *extval;
	int			pidx = 0;
	char		c[2] = {0, 0};
	char	   *cp;
	PLpgSQL_dstring ds;
	PLpgSQL_var *var;
	PLpgSQL_rec *rec;
	PLpgSQL_recfield *recfield;
	int			fno;

	plpgsql_dstring_init(&ds);

	for (cp = stmt->message; *cp; cp++)
	{
		/* ----------
		 * Occurences of a single % are replaced by the next
		 * arguments external representation. Double %'s are
		 * left as is so elog() will also don't touch them.
		 * ----------
		 */
		if ((c[0] = *cp) == '%')
		{
			cp++;
			if (*cp == '%')
			{
				plpgsql_dstring_append(&ds, c);
				plpgsql_dstring_append(&ds, c);
				continue;
			}
			cp--;
			if (pidx >= stmt->nparams)
			{
				plpgsql_dstring_append(&ds, c);
				plpgsql_dstring_append(&ds, c);
				continue;
			}
			switch (estate->datums[stmt->params[pidx]]->dtype)
			{
				case PLPGSQL_DTYPE_VAR:
					var = (PLpgSQL_var *)
						(estate->datums[stmt->params[pidx]]);
					if (var->isnull)
						extval = "<NULL>";
					else
					{
						typetup = SearchSysCacheTuple(TYPEOID,
						ObjectIdGetDatum(var->datatype->typoid), 0, 0, 0);
						if (!HeapTupleIsValid(typetup))
							elog(ERROR, "cache lookup for type %u failed (1)", var->datatype->typoid);
						typeStruct = (Form_pg_type) GETSTRUCT(typetup);

						fmgr_info(typeStruct->typoutput, &finfo_output);
						extval = DatumGetCString(FunctionCall3(&finfo_output,
									var->value,
									ObjectIdGetDatum(typeStruct->typelem),
									Int32GetDatum(var->datatype->atttypmod)));
					}
					plpgsql_dstring_append(&ds, extval);
					break;

				case PLPGSQL_DTYPE_RECFIELD:
					recfield = (PLpgSQL_recfield *)
						(estate->datums[stmt->params[pidx]]);
					rec = (PLpgSQL_rec *)
						(estate->datums[recfield->recno]);
					if (!HeapTupleIsValid(rec->tup))
						extval = "<NULL>";
					else
					{
						fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
						if (fno == SPI_ERROR_NOATTRIBUTE)
							elog(ERROR, "record %s has no field %s", rec->refname, recfield->fieldname);
						extval = SPI_getvalue(rec->tup, rec->tupdesc, fno);
						if (extval == NULL)
							extval = "<NULL>";
					}
					plpgsql_dstring_append(&ds, extval);
					break;

				case PLPGSQL_DTYPE_TRIGARG:
					{
						PLpgSQL_trigarg *trigarg;
						int			value;
						Oid			valtype;
						bool		valisnull = false;

						trigarg = (PLpgSQL_trigarg *)
							(estate->datums[stmt->params[pidx]]);
						value = (int) exec_eval_expr(estate, trigarg->argnum,
												   &valisnull, &valtype);
						if (valisnull)
							extval = "<INDEX_IS_NULL>";
						else
						{
							if (value < 0 || value >= estate->trig_nargs)
								extval = "<OUT_OF_RANGE>";
							else
								extval = DatumGetCString(DirectFunctionCall1(textout,
													estate->trig_argv[value]));
						}
						plpgsql_dstring_append(&ds, extval);
					}
					break;

				default:
					c[0] = '?';
					plpgsql_dstring_append(&ds, c);
					break;
			}
			pidx++;
			continue;
		}

		/* ----------
		 * Occurences of single ' are removed. double ' are reduced
		 * to single ones.
		 * ----------
		 */
		if (*cp == '\'')
		{
			cp++;
			if (*cp == '\'')
				plpgsql_dstring_append(&ds, c);
			else
				cp--;
			continue;
		}
		plpgsql_dstring_append(&ds, c);
	}

	/* ----------
	 * Now suppress debug info and throw the elog()
	 * ----------
	 */
	if (stmt->elog_level == ERROR)
	{
		error_info_func = NULL;
		error_info_stmt = NULL;
		error_info_text = NULL;
	}
	elog(stmt->elog_level, "%s", plpgsql_dstring_get(&ds));
	plpgsql_dstring_free(&ds);

	return PLPGSQL_RC_OK;
}


/* ----------
 * Generate a prepared plan
 * ----------
 */
static void
exec_prepare_plan(PLpgSQL_execstate * estate,
				  PLpgSQL_expr * expr)
{
	PLpgSQL_var *var;
	PLpgSQL_rec *rec;
	PLpgSQL_recfield *recfield;
	int			i;
	int			fno;
	void	   *plan;
	Oid		   *argtypes;

	/* ----------
	 * Setup the argtypes array
	 * ----------
	 */
	argtypes = malloc(sizeof(Oid *) * (expr->nparams + 1));

	for (i = 0; i < expr->nparams; i++)
	{
		switch (estate->datums[expr->params[i]]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				var = (PLpgSQL_var *) (estate->datums[expr->params[i]]);
				argtypes[i] = var->datatype->typoid;
				break;

			case PLPGSQL_DTYPE_RECFIELD:
				recfield = (PLpgSQL_recfield *) (estate->datums[expr->params[i]]);
				rec = (PLpgSQL_rec *) (estate->datums[recfield->recno]);

				if (!HeapTupleIsValid(rec->tup))
					elog(ERROR, "record %s is unassigned yet", rec->refname);
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno == SPI_ERROR_NOATTRIBUTE)
					elog(ERROR, "record %s has no field %s", rec->refname, recfield->fieldname);
				argtypes[i] = SPI_gettypeid(rec->tupdesc, fno);
				break;

			case PLPGSQL_DTYPE_TRIGARG:
				argtypes[i] = (Oid) TEXTOID;
				break;

			default:
				elog(ERROR, "unknown parameter dtype %d in exec_run_select()",
					 estate->datums[expr->params[i]]->dtype);
		}
	}

	/* ----------
	 * Generate and save the plan
	 * ----------
	 */
	plan = SPI_prepare(expr->query, expr->nparams, argtypes);
	if (plan == NULL)
		elog(ERROR, "SPI_prepare() failed on \"%s\"", expr->query);
	expr->plan = SPI_saveplan(plan);
	expr->plan_argtypes = argtypes;
	expr->plan_simple_expr = NULL;
	exec_simple_check_plan(expr);
}


/* ----------
 * exec_stmt_execsql			Execute an SQL statement not
 *					returning any data.
 * ----------
 */
static int
exec_stmt_execsql(PLpgSQL_execstate * estate,
				  PLpgSQL_stmt_execsql * stmt)
{
	PLpgSQL_var *var;
	PLpgSQL_rec *rec;
	PLpgSQL_recfield *recfield;
	PLpgSQL_trigarg *trigarg;
	int			tgargno;
	Oid			tgargoid;
	int			fno;
	int			i;
	Datum	   *values;
	char	   *nulls;
	int			rc;
	PLpgSQL_expr *expr = stmt->sqlstmt;
	bool		isnull;

	/* ----------
	 * On the first call for this expression generate the plan
	 * ----------
	 */
	if (expr->plan == NULL)
		exec_prepare_plan(estate, expr);

	/* ----------
	 * Now build up the values and nulls arguments for SPI_execp()
	 * ----------
	 */
	values = palloc(sizeof(Datum) * (expr->nparams + 1));
	nulls = palloc(expr->nparams + 1);

	for (i = 0; i < expr->nparams; i++)
	{
		switch (estate->datums[expr->params[i]]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				var = (PLpgSQL_var *) (estate->datums[expr->params[i]]);
				values[i] = var->value;
				if (var->isnull)
					nulls[i] = 'n';
				else
					nulls[i] = ' ';
				break;

			case PLPGSQL_DTYPE_RECFIELD:
				recfield = (PLpgSQL_recfield *) (estate->datums[expr->params[i]]);
				rec = (PLpgSQL_rec *) (estate->datums[recfield->recno]);

				if (!HeapTupleIsValid(rec->tup))
					elog(ERROR, "record %s is unassigned yet", rec->refname);
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno == SPI_ERROR_NOATTRIBUTE)
					elog(ERROR, "record %s has no field %s", rec->refname, recfield->fieldname);

				if (expr->plan_argtypes[i] != SPI_gettypeid(rec->tupdesc, fno))
					elog(ERROR, "type of %s.%s doesn't match that when preparing the plan", rec->refname, recfield->fieldname);

				values[i] = SPI_getbinval(rec->tup, rec->tupdesc, fno, &isnull);
				if (isnull)
					nulls[i] = 'n';
				else
					nulls[i] = ' ';
				break;

			case PLPGSQL_DTYPE_TRIGARG:
				trigarg = (PLpgSQL_trigarg *) (estate->datums[expr->params[i]]);
				tgargno = (int) exec_eval_expr(estate, trigarg->argnum,
											   &isnull, &tgargoid);
				if (isnull || tgargno < 0 || tgargno >= estate->trig_nargs)
				{
					values[i] = 0;
					nulls[i] = 'n';
				}
				else
				{
					values[i] = estate->trig_argv[tgargno];
					nulls[i] = ' ';
				}
				break;

			default:
				elog(ERROR, "unknown parameter dtype %d in exec_stmt_execsql()", estate->datums[expr->params[i]]->dtype);
		}
	}
	nulls[i] = '\0';

	/* ----------
	 * Execute the plan
	 * ----------
	 */
	rc = SPI_execp(expr->plan, values, nulls, 0);
	switch (rc)
	{
		case SPI_OK_UTILITY:
		case SPI_OK_SELINTO:
		case SPI_OK_INSERT:
		case SPI_OK_DELETE:
		case SPI_OK_UPDATE:
			break;

		case SPI_OK_SELECT:
			elog(ERROR, "unexpected SELECT query in exec_stmt_execsql()");

		default:
			elog(ERROR, "error executing query \"%s\"",
				 expr->query);
	}
	pfree(values);
	pfree(nulls);

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_dynexecute			Execute a dynamic SQL query not
 *					returning any data.
 * ----------
 */
static int
exec_stmt_dynexecute(PLpgSQL_execstate * estate,
				  PLpgSQL_stmt_dynexecute * stmt)
{
	Datum		query;
	bool		isnull = false;
	Oid			restype;
	char	   *querystr;
	HeapTuple	typetup;
	Form_pg_type typeStruct;
	FmgrInfo	finfo_output;

	/* ----------
	 * First we evaluate the string expression after the
	 * EXECUTE keyword. It's result is the querystring we have
	 * to execute.
	 * ----------
	 */
	query = exec_eval_expr(estate, stmt->query, &isnull, &restype);
	if (isnull)
		elog(ERROR, "cannot EXECUTE NULL-query");

	/* ----------
	 * Get the C-String representation.
	 * ----------
	 */
	typetup = SearchSysCacheTuple(TYPEOID,
		ObjectIdGetDatum(restype), 0, 0, 0);
	if (!HeapTupleIsValid(typetup))
		elog(ERROR, "cache lookup for type %u failed (1)", restype);
	typeStruct = (Form_pg_type) GETSTRUCT(typetup);

	fmgr_info(typeStruct->typoutput, &finfo_output);
	querystr = DatumGetCString(FunctionCall3(&finfo_output,
				query,
				ObjectIdGetDatum(typeStruct->typelem),
				Int32GetDatum(-1)));

	if(!typeStruct->typbyval)
		pfree((void *)query);

	/* ----------
	 * Call SPI_exec() without preparing a saved plan. 
	 * The returncode can be any OK except for OK_SELECT.
	 * ----------
	 */
	switch(SPI_exec(querystr, 0))
	{
		case SPI_OK_UTILITY:
		case SPI_OK_SELINTO:
		case SPI_OK_INSERT:
		case SPI_OK_UPDATE:
		case SPI_OK_DELETE:
			break;

		case SPI_OK_SELECT:
			elog(ERROR, "unexpected SELECT operation in EXECUTE of query '%s'",
						querystr);
			break;

		default:
			elog(ERROR, "unexpected error in EXECUTE for query '%s'",
						querystr);
			break;
	}

	pfree(querystr);
	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_dynfors			Execute a dynamic query, assign each
 *					tuple to a record or row and
 *					execute a group of statements
 *					for it.
 * ----------
 */
static int
exec_stmt_dynfors(PLpgSQL_execstate * estate, PLpgSQL_stmt_dynfors * stmt)
{
	Datum		query;
	bool		isnull = false;
	Oid			restype;
	char	   *querystr;
	PLpgSQL_rec *rec = NULL;
	PLpgSQL_row *row = NULL;
	SPITupleTable *tuptab;
	int			rc;
	int			i;
	int			n;
	HeapTuple	typetup;
	Form_pg_type typeStruct;
	FmgrInfo	finfo_output;

	/* ----------
	 * Initialize the global found variable to false
	 * ----------
	 */
	exec_set_found(estate, false);

	/* ----------
	 * Determine if we assign to a record or a row
	 * ----------
	 */
	if (stmt->rec != NULL)
		rec = (PLpgSQL_rec *) (estate->datums[stmt->rec->recno]);
	else
	{
		if (stmt->row != NULL)
			row = (PLpgSQL_row *) (estate->datums[stmt->row->rowno]);
		else
			elog(ERROR, "unsupported target in exec_stmt_fors()");
	}

	/* ----------
	 * Evaluate the string expression after the
	 * EXECUTE keyword. It's result is the querystring we have
	 * to execute.
	 * ----------
	 */
	query = exec_eval_expr(estate, stmt->query, &isnull, &restype);
	if (isnull)
		elog(ERROR, "cannot EXECUTE NULL-query");

	/* ----------
	 * Get the C-String representation.
	 * ----------
	 */
	typetup = SearchSysCacheTuple(TYPEOID,
		ObjectIdGetDatum(restype), 0, 0, 0);
	if (!HeapTupleIsValid(typetup))
		elog(ERROR, "cache lookup for type %u failed (1)", restype);
	typeStruct = (Form_pg_type) GETSTRUCT(typetup);

	fmgr_info(typeStruct->typoutput, &finfo_output);
	querystr = DatumGetCString(FunctionCall3(&finfo_output,
				query,
				ObjectIdGetDatum(typeStruct->typelem),
				Int32GetDatum(-1)));

	if(!typeStruct->typbyval)
		pfree((void *)query);

	/* ----------
	 * Run the query
	 * ----------
	 */
	if (SPI_exec(querystr, 0) != SPI_OK_SELECT)
		elog(ERROR, "FOR ... EXECUTE query '%s' was no SELECT", querystr);
	pfree(querystr);

	n = SPI_processed;

	/* ----------
	 * If the query didn't return any row, set the target
	 * to NULL and return.
	 * ----------
	 */
	if (n == 0)
	{
		exec_move_row(estate, rec, row, NULL, NULL);
		return PLPGSQL_RC_OK;
	}

	/* ----------
	 * There are tuples, so set found to true
	 * ----------
	 */
	exec_set_found(estate, true);

	/* ----------
	 * Now do the loop
	 * ----------
	 */
	tuptab = SPI_tuptable;
	SPI_tuptable = NULL;

	for (i = 0; i < n; i++)
	{
		/* ----------
		 * Assign the tuple to the target
		 * ----------
		 */
		exec_move_row(estate, rec, row, tuptab->vals[i], tuptab->tupdesc);

		/* ----------
		 * Execute the statements
		 * ----------
		 */
		rc = exec_stmts(estate, stmt->body);

		/* ----------
		 * Check returncode
		 * ----------
		 */
		switch (rc)
		{
			case PLPGSQL_RC_OK:
				break;

			case PLPGSQL_RC_EXIT:
				if (estate->exitlabel == NULL)
					return PLPGSQL_RC_OK;
				if (stmt->label == NULL)
					return PLPGSQL_RC_EXIT;
				if (strcmp(stmt->label, estate->exitlabel))
					return PLPGSQL_RC_EXIT;
				estate->exitlabel = NULL;
				return PLPGSQL_RC_OK;

			case PLPGSQL_RC_RETURN:
				return PLPGSQL_RC_RETURN;

			default:
				elog(ERROR, "unknown rc %d from exec_stmts()", rc);
		}
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_assign_expr			Put an expressions result into
 *					a variable.
 * ----------
 */
static void
exec_assign_expr(PLpgSQL_execstate * estate, PLpgSQL_datum * target,
				 PLpgSQL_expr * expr)
{
	Datum		value;
	Oid			valtype;
	bool		isnull = false;

	value = exec_eval_expr(estate, expr, &isnull, &valtype);
	if (target != NULL)
		exec_assign_value(estate, target, value, valtype, &isnull);
}


/* ----------
 * exec_assign_value			Put a value into a target field
 * ----------
 */
static void
exec_assign_value(PLpgSQL_execstate * estate,
				  PLpgSQL_datum * target,
				  Datum value, Oid valtype, bool *isNull)
{
	PLpgSQL_var *var;
	PLpgSQL_rec *rec;
	PLpgSQL_recfield *recfield;
	int			fno;
	int			i;
	int			natts;
	Datum	   *values;
	char	   *nulls;
	bool		attisnull;
	Oid			atttype;
	int32		atttypmod;
	HeapTuple	typetup;
	Form_pg_type typeStruct;
	FmgrInfo	finfo_input;

	switch (target->dtype)
	{
		case PLPGSQL_DTYPE_VAR:
			/* ----------
			 * Target field is a variable - that's easy
			 * ----------
			 */
			var = (PLpgSQL_var *) target;
			var->value = exec_cast_value(value, valtype, var->datatype->typoid,
										 &(var->datatype->typinput),
										 var->datatype->typelem,
										 var->datatype->atttypmod,
										 isNull);

			if (isNull && var->notnull)
				elog(ERROR, "NULL assignment to variable '%s' declared NOT NULL", var->refname);

			var->isnull = *isNull;
			break;

		case PLPGSQL_DTYPE_RECFIELD:
			/* ----------
			 * Target field is a record
			 * ----------
			 */
			recfield = (PLpgSQL_recfield *) target;
			rec = (PLpgSQL_rec *) (estate->datums[recfield->recno]);

			/* ----------
			 * Check that there is already a tuple in the record.
			 * We need that because records don't have any predefined
			 * field structure.
			 * ----------
			 */
			if (!HeapTupleIsValid(rec->tup))
				elog(ERROR, "record %s is unassigned yet - don't know its tuple structure", rec->refname);

			/* ----------
			 * Get the number of the records field to change and the
			 * number of attributes in the tuple.
			 * ----------
			 */
			fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
			if (fno == SPI_ERROR_NOATTRIBUTE)
				elog(ERROR, "record %s has no field %s", rec->refname, recfield->fieldname);
			fno--;
			natts = rec->tupdesc->natts;

			/* ----------
			 * We loop over the attributes of the rec's current tuple
			 * and collect the values in a Datum array along with the
			 * nulls information.
			 * ----------
			 */
			values = palloc(sizeof(Datum) * natts);
			nulls = palloc(natts + 1);

			for (i = 0; i < natts; i++)
			{
				/* ----------
				 * If this isn't the field we assign to, just use the
				 * value that's already in the tuple.
				 * ----------
				 */
				if (i != fno)
				{
					values[i] = SPI_getbinval(rec->tup, rec->tupdesc,
											  i + 1, &attisnull);
					if (attisnull)
						nulls[i] = 'n';
					else
						nulls[i] = ' ';
					continue;
				}

				/* ----------
				 * This is the field to change. Get its type
				 * and cast the value we insert to that type.
				 * ----------
				 */
				atttype = SPI_gettypeid(rec->tupdesc, i + 1);
				atttypmod = rec->tupdesc->attrs[i]->atttypmod;
				typetup = SearchSysCacheTuple(TYPEOID,
									 ObjectIdGetDatum(atttype), 0, 0, 0);
				if (!HeapTupleIsValid(typetup))
					elog(ERROR, "cache lookup for type %u failed", atttype);
				typeStruct = (Form_pg_type) GETSTRUCT(typetup);
				fmgr_info(typeStruct->typinput, &finfo_input);

				attisnull = *isNull;
				values[i] = exec_cast_value(value, valtype,
											atttype, &finfo_input,
											typeStruct->typelem,
											atttypmod, &attisnull);
				if (attisnull)
					nulls[i] = 'n';
				else
					nulls[i] = ' ';
			}

			/* ----------
			 * Now call heap_formtuple() to create a new tuple
			 * that replaces the old one in the record.
			 * ----------
			 */
			nulls[i] = '\0';
			rec->tup = heap_formtuple(rec->tupdesc, values, nulls);
			pfree(values);
			pfree(nulls);

			break;

		default:
			elog(ERROR, "unknown dtype %d in exec_assign_value()",
				 target->dtype);
	}
}


/* ----------
 * exec_eval_expr			Evaluate an expression and return
 *					the result Datum.
 * ----------
 */
static Datum
exec_eval_expr(PLpgSQL_execstate * estate,
			   PLpgSQL_expr * expr,
			   bool *isNull,
			   Oid *rettype)
{
	int			rc;

	/* ----------
	 * If not already done create a plan for this expression
	 * ----------
	 */
	if (expr->plan == NULL)
		exec_prepare_plan(estate, expr);

	/* ----------
	 * If this is a simple expression, bypass SPI and use the
	 * executor directly
	 * ----------
	 */
	if (expr->plan_simple_expr != NULL)
		return exec_eval_simple_expr(estate, expr, isNull, rettype);

	rc = exec_run_select(estate, expr, 2);
	if (rc != SPI_OK_SELECT)
		elog(ERROR, "query \"%s\" didn't return data", expr->query);

	/* ----------
	 * If there are no rows selected, the result is NULL.
	 * ----------
	 */
	if (SPI_processed == 0)
	{
		*isNull = true;
		return (Datum) 0;
	}

	/* ----------
	 * Check that the expression returned one single Datum
	 * ----------
	 */
	if (SPI_processed > 1)
		elog(ERROR, "query \"%s\" didn't return a single value", expr->query);
	if (SPI_tuptable->tupdesc->natts != 1)
		elog(ERROR, "query \"%s\" didn't return a single value", expr->query);

	/* ----------
	 * Return the result and its type
	 * ----------
	 */
	*rettype = SPI_gettypeid(SPI_tuptable->tupdesc, 1);
	return SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, isNull);
}


/* ----------
 * exec_run_select			Execute a select query
 * ----------
 */
static int
exec_run_select(PLpgSQL_execstate * estate,
				PLpgSQL_expr * expr, int maxtuples)
{
	PLpgSQL_var *var;
	PLpgSQL_rec *rec;
	PLpgSQL_recfield *recfield;
	PLpgSQL_trigarg *trigarg;
	int			tgargno;
	Oid			tgargoid;
	int			i;
	Datum	   *values;
	char	   *nulls;
	int			rc;
	int			fno;
	bool		isnull;

	/* ----------
	 * On the first call for this expression generate the plan
	 * ----------
	 */
	if (expr->plan == NULL)
		exec_prepare_plan(estate, expr);

	/* ----------
	 * Now build up the values and nulls arguments for SPI_execp()
	 * ----------
	 */
	values = palloc(sizeof(Datum) * (expr->nparams + 1));
	nulls = palloc(expr->nparams + 1);

	for (i = 0; i < expr->nparams; i++)
	{
		switch (estate->datums[expr->params[i]]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				var = (PLpgSQL_var *) (estate->datums[expr->params[i]]);
				values[i] = var->value;
				if (var->isnull)
					nulls[i] = 'n';
				else
					nulls[i] = ' ';
				break;

			case PLPGSQL_DTYPE_RECFIELD:
				recfield = (PLpgSQL_recfield *) (estate->datums[expr->params[i]]);
				rec = (PLpgSQL_rec *) (estate->datums[recfield->recno]);

				if (!HeapTupleIsValid(rec->tup))
					elog(ERROR, "record %s is unassigned yet", rec->refname);
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno == SPI_ERROR_NOATTRIBUTE)
					elog(ERROR, "record %s has no field %s", rec->refname, recfield->fieldname);

				if (expr->plan_argtypes[i] != SPI_gettypeid(rec->tupdesc, fno))
					elog(ERROR, "type of %s.%s doesn't match that when preparing the plan", rec->refname, recfield->fieldname);

				values[i] = SPI_getbinval(rec->tup, rec->tupdesc, fno, &isnull);
				if (isnull)
					nulls[i] = 'n';
				else
					nulls[i] = ' ';
				break;

			case PLPGSQL_DTYPE_TRIGARG:
				trigarg = (PLpgSQL_trigarg *) (estate->datums[expr->params[i]]);
				tgargno = (int) exec_eval_expr(estate, trigarg->argnum,
											   &isnull, &tgargoid);
				if (isnull || tgargno < 0 || tgargno >= estate->trig_nargs)
				{
					values[i] = 0;
					nulls[i] = 'n';
				}
				else
				{
					values[i] = estate->trig_argv[tgargno];
					nulls[i] = ' ';
				}
				break;

			default:
				elog(ERROR, "unknown parameter dtype %d in exec_eval_expr()",
					 estate->datums[expr->params[i]]->dtype);
		}
	}
	nulls[i] = '\0';

	/* ----------
	 * Execute the query
	 * ----------
	 */
	rc = SPI_execp(expr->plan, values, nulls, maxtuples);
	if (rc != SPI_OK_SELECT)
		elog(ERROR, "query \"%s\" isn't a SELECT", expr->query);
	pfree(values);
	pfree(nulls);

	return rc;
}


/* ----------
 * exec_eval_simple_expr -		Evaluate a simple expression returning
 *								a Datum by directly calling ExecEvalExpr().
 * ----------
 */
static Datum
exec_eval_simple_expr(PLpgSQL_execstate * estate,
					  PLpgSQL_expr * expr,
					  bool *isNull,
					  Oid *rettype)
{
	Datum		retval;
	PLpgSQL_var *var;
	PLpgSQL_rec *rec;
	PLpgSQL_recfield *recfield;
	PLpgSQL_trigarg *trigarg;
	int			tgargno;
	Oid			tgargoid;
	int			fno;
	int			i;
	bool		isnull;
	ExprContext *econtext;
	ParamListInfo paramLI;

	/* ----------
	 * Create a simple expression context to hold the arguments
	 * ----------
	 */
	econtext = MakeExprContext(NULL, TransactionCommandContext);
	paramLI = (ParamListInfo) palloc((expr->nparams + 1) *
									 sizeof(ParamListInfoData));
	econtext->ecxt_param_list_info = paramLI;

	/* ----------
	 * Put the parameter values into the parameter list info of
	 * the expression context.
	 * ----------
	 */
	for (i = 0; i < expr->nparams; i++, paramLI++)
	{
		paramLI->kind = PARAM_NUM;
		paramLI->id = i + 1;

		switch (estate->datums[expr->params[i]]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				var = (PLpgSQL_var *) (estate->datums[expr->params[i]]);
				paramLI->isnull = var->isnull;
				paramLI->value = var->value;
				break;

			case PLPGSQL_DTYPE_RECFIELD:
				recfield = (PLpgSQL_recfield *) (estate->datums[expr->params[i]]);
				rec = (PLpgSQL_rec *) (estate->datums[recfield->recno]);

				if (!HeapTupleIsValid(rec->tup))
					elog(ERROR, "record %s is unassigned yet", rec->refname);
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno == SPI_ERROR_NOATTRIBUTE)
					elog(ERROR, "record %s has no field %s", rec->refname, recfield->fieldname);

				if (expr->plan_argtypes[i] != SPI_gettypeid(rec->tupdesc, fno))
					elog(ERROR, "type of %s.%s doesn't match that when preparing the plan", rec->refname, recfield->fieldname);

				paramLI->value = SPI_getbinval(rec->tup, rec->tupdesc, fno, &isnull);
				paramLI->isnull = isnull;
				break;

			case PLPGSQL_DTYPE_TRIGARG:
				trigarg = (PLpgSQL_trigarg *) (estate->datums[expr->params[i]]);
				tgargno = (int) exec_eval_expr(estate, trigarg->argnum,
											   &isnull, &tgargoid);
				if (isnull || tgargno < 0 || tgargno >= estate->trig_nargs)
				{
					paramLI->value = 0;
					paramLI->isnull = TRUE;
				}
				else
				{
					paramLI->value = estate->trig_argv[tgargno];
					paramLI->isnull = FALSE;
				}
				break;

			default:
				elog(ERROR, "unknown parameter dtype %d in exec_eval_simple_expr()", estate->datums[expr->params[i]]->dtype);
		}
	}
	paramLI->kind = PARAM_INVALID;

	/* ----------
	 * Initialize things
	 * ----------
	 */
	*rettype = expr->plan_simple_type;

	/* ----------
	 * Clear the function cache
	 * ----------
	 */
	exec_eval_clear_fcache(expr->plan_simple_expr);

	/* ----------
	 * Now call the executor to evaluate the expression
	 * ----------
	 */
	SPI_push();
	retval = ExecEvalExprSwitchContext(expr->plan_simple_expr,
									   econtext,
									   isNull,
									   NULL);
	SPI_pop();

	/*
	 * Copy the result out of the expression-evaluation memory context,
	 * so that we can free the expression context.
	 */
	if (! *isNull)
		retval = datumCopy(retval,
						   get_typbyval(*rettype),
						   get_typlen(*rettype));

	FreeExprContext(econtext);

	/* ----------
	 * That's it.
	 * ----------
	 */
	return retval;
}


/* ----------
 * exec_move_row			Move one tuples values into a
 *					record or row
 * ----------
 */
static void
exec_move_row(PLpgSQL_execstate * estate,
			  PLpgSQL_rec * rec,
			  PLpgSQL_row * row,
			  HeapTuple tup, TupleDesc tupdesc)
{
	PLpgSQL_var *var;
	int			i;
	Datum		value;
	Oid			valtype;
	bool		isnull;

	/* ----------
	 * Record is simple - just put the tuple and its descriptor
	 * into the record
	 * ----------
	 */
	if (rec != NULL)
	{
		if (HeapTupleIsValid(tup))
		{
			rec->tup = tup;
			rec->tupdesc = tupdesc;
		}
		else
		{
			rec->tup = NULL;
			rec->tupdesc = NULL;
		}

		return;
	}


	/* ----------
	 * Row is a bit more complicated in that we assign the single
	 * attributes of the query to the variables the row points to.
	 * ----------
	 */
	if (row != NULL)
	{
		if (HeapTupleIsValid(tup))
		{
			if (row->nfields != tup->t_data->t_natts)
			{
				elog(ERROR, "query didn't return correct # of attributes for %s",
					 row->refname);
			}

			for (i = 0; i < row->nfields; i++)
			{
				var = (PLpgSQL_var *) (estate->datums[row->varnos[i]]);

				valtype = SPI_gettypeid(tupdesc, i + 1);
				value = SPI_getbinval(tup, tupdesc, i + 1, &isnull);
				exec_assign_value(estate, estate->datums[row->varnos[i]],
								  value, valtype, &isnull);

			}
		}
		else
		{
			for (i = 0; i < row->nfields; i++)
			{
				bool		nullval = true;

				exec_assign_value(estate, estate->datums[row->varnos[i]],
								  (Datum) 0, 0, &nullval);
			}
		}

		return;
	}

	elog(ERROR, "unsupported target in exec_move_row()");
}


/* ----------
 * exec_cast_value			Cast a value if required
 * ----------
 */
static Datum
exec_cast_value(Datum value, Oid valtype,
				Oid reqtype,
				FmgrInfo *reqinput,
				Oid reqtypelem,
				int32 reqtypmod,
				bool *isnull)
{
	if (!*isnull)
	{
		/* ----------
		 * If the type of the queries return value isn't
		 * that of the variable, convert it.
		 * ----------
		 */
		if (valtype != reqtype || reqtypmod != -1)
		{
			HeapTuple	typetup;
			Form_pg_type typeStruct;
			FmgrInfo	finfo_output;
			char	   *extval;

			typetup = SearchSysCacheTuple(TYPEOID,
									 ObjectIdGetDatum(valtype), 0, 0, 0);
			if (!HeapTupleIsValid(typetup))
				elog(ERROR, "cache lookup for type %u failed", valtype);
			typeStruct = (Form_pg_type) GETSTRUCT(typetup);

			fmgr_info(typeStruct->typoutput, &finfo_output);
			extval = DatumGetCString(FunctionCall3(&finfo_output,
									 value,
									 ObjectIdGetDatum(typeStruct->typelem),
									 Int32GetDatum(-1)));
			value = FunctionCall3(reqinput,
								  CStringGetDatum(extval),
								  ObjectIdGetDatum(reqtypelem),
								  Int32GetDatum(reqtypmod));
			pfree(extval);
		}
	}

	return value;
}


/* ----------
 * exec_simple_check_node -		Recursively check if an expression
 *								is made only of simple things we can
 *								hand out directly to ExecEvalExpr()
 *								instead of calling SPI.
 * ----------
 */
static bool
exec_simple_check_node(Node *node)
{
	switch (nodeTag(node))
	{
			case T_Expr:
			{
				Expr	   *expr = (Expr *) node;
				List	   *l;

				switch (expr->opType)
				{
					case OP_EXPR:
					case FUNC_EXPR:
					case OR_EXPR:
					case AND_EXPR:
					case NOT_EXPR:
						break;

					default:
						return FALSE;
				}

				foreach(l, expr->args)
				{
					if (!exec_simple_check_node(lfirst(l)))
						return FALSE;
				}

				return TRUE;
			}

		case T_Param:
			return TRUE;

		case T_Const:
			return TRUE;

		case T_RelabelType:
			return exec_simple_check_node(((RelabelType *) node)->arg);

		default:
			return FALSE;
	}
}


/* ----------
 * exec_simple_check_plan -		Check if a plan is simple enough to
 *								be evaluated by ExecEvalExpr() instead
 *								of SPI.
 * ----------
 */
static void
exec_simple_check_plan(PLpgSQL_expr * expr)
{
	_SPI_plan  *spi_plan = (_SPI_plan *) expr->plan;
	Plan	   *plan;
	TargetEntry *tle;

	expr->plan_simple_expr = NULL;

	/* ----------
	 * 1. We can only evaluate queries that resulted in one single
	 *	  execution plan
	 * ----------
	 */
	if (length(spi_plan->ptlist) != 1)
		return;

	plan = (Plan *) lfirst(spi_plan->ptlist);

	/* ----------
	 * 2. It must be a RESULT plan --> no scan's required
	 * ----------
	 */
	if (plan == NULL)			/* utility statement produces this */
		return;

	if (!IsA(plan, Result))
		return;

	/* ----------
	 * 3. Can't have any subplan or qual clause, either
	 * ----------
	 */
	if (plan->lefttree != NULL ||
		plan->righttree != NULL ||
		plan->initPlan != NULL ||
		plan->subPlan != NULL ||
		plan->qual != NULL ||
		((Result *) plan)->resconstantqual != NULL)
		return;

	/* ----------
	 * 4. The plan must have a single attribute as result
	 * ----------
	 */
	if (length(plan->targetlist) != 1)
		return;

	tle = (TargetEntry *) lfirst(plan->targetlist);

	/* ----------
	 * 5. Check that all the nodes in the expression are one of
	 *	  Expr, Param or Const.
	 * ----------
	 */
	if (!exec_simple_check_node(tle->expr))
		return;

	/* ----------
	 * Yes - this is a simple expression. Remember the expression
	 * and the return type
	 * ----------
	 */
	expr->plan_simple_expr = tle->expr;
	expr->plan_simple_type = exprType(tle->expr);
}


/* ----------
 * exec_eval_clear_fcache -		The function cache is palloc()'d by
 *								the executor, and contains call specific
 *								data based on the arguments. This has
 *								to be recalculated.
 * ----------
 */
static void
exec_eval_clear_fcache(Node *node)
{
	Expr	   *expr;
	List	   *l;

	if (nodeTag(node) != T_Expr)
		return;

	expr = (Expr *) node;

	switch (expr->opType)
	{
		case OP_EXPR:
			((Oper *) (expr->oper))->op_fcache = NULL;
			break;

		case FUNC_EXPR:
			((Func *) (expr->oper))->func_fcache = NULL;
			break;

		default:
			break;
	}

	foreach(l, expr->args)
		exec_eval_clear_fcache(lfirst(l));
}


/* ----------
 * exec_set_found			Set the global found variable
 *					to true/false
 * ----------
 */
static void
exec_set_found(PLpgSQL_execstate * estate, bool state)
{
	PLpgSQL_var *var;

	var = (PLpgSQL_var *) (estate->datums[estate->found_varno]);
	var->value = (Datum) state;
	var->isnull = false;
}
