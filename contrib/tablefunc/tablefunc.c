/*
 * tablefunc
 *
 * Sample to demonstrate C functions which return setof scalar
 * and setof composite.
 * Joe Conway <mail@joeconway.com>
 *
 * Copyright 2002 by PostgreSQL Global Development Group
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written agreement
 * is hereby granted, provided that the above copyright notice and this
 * paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE AUTHOR OR DISTRIBUTORS HAVE BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE AUTHOR AND DISTRIBUTORS HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 */
#include "postgres.h"

#include <math.h>

#include "fmgr.h"
#include "funcapi.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"

#include "tablefunc.h"

static void validateConnectbyTupleDesc(TupleDesc tupdesc, bool show_branch);
static bool compatCrosstabTupleDescs(TupleDesc tupdesc1, TupleDesc tupdesc2);
static bool compatConnectbyTupleDescs(TupleDesc tupdesc1, TupleDesc tupdesc2);
static void get_normal_pair(float8 *x1, float8 *x2);
static TupleDesc make_crosstab_tupledesc(TupleDesc spi_tupdesc,
						int num_catagories);
static Tuplestorestate *connectby(char *relname,
		  char *key_fld,
		  char *parent_key_fld,
		  char *branch_delim,
		  char *start_with,
		  int max_depth,
		  bool show_branch,
		  MemoryContext per_query_ctx,
		  AttInMetadata *attinmeta);
static Tuplestorestate *build_tuplestore_recursively(char *key_fld,
							 char *parent_key_fld,
							 char *relname,
							 char *branch_delim,
							 char *start_with,
							 char *branch,
							 int level,
							 int max_depth,
							 bool show_branch,
							 MemoryContext per_query_ctx,
							 AttInMetadata *attinmeta,
							 Tuplestorestate *tupstore);
static char *quote_ident_cstr(char *rawstr);

typedef struct
{
	float8		mean;			/* mean of the distribution */
	float8		stddev;			/* stddev of the distribution */
	float8		carry_val;		/* hold second generated value */
	bool		use_carry;		/* use second generated value */
}	normal_rand_fctx;

typedef struct
{
	SPITupleTable *spi_tuptable;	/* sql results from user query */
	char	   *lastrowid;		/* rowid of the last tuple sent */
}	crosstab_fctx;

#define GET_TEXT(cstrp) DatumGetTextP(DirectFunctionCall1(textin, CStringGetDatum(cstrp)))
#define GET_STR(textp) DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(textp)))
#define xpfree(var_) \
	do { \
		if (var_ != NULL) \
		{ \
			pfree(var_); \
			var_ = NULL; \
		} \
	} while (0)

/* sign, 10 digits, '\0' */
#define INT32_STRLEN	12

/*
 * normal_rand - return requested number of random values
 * with a Gaussian (Normal) distribution.
 *
 * inputs are int numvals, float8 lower_bound, and float8 upper_bound
 * returns float8
 */
PG_FUNCTION_INFO_V1(normal_rand);
Datum
normal_rand(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	int			call_cntr;
	int			max_calls;
	normal_rand_fctx *fctx;
	float8		mean;
	float8		stddev;
	float8		carry_val;
	bool		use_carry;
	MemoryContext oldcontext;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL())
	{
		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/*
		 * switch to memory context appropriate for multiple function
		 * calls
		 */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* total number of tuples to be returned */
		funcctx->max_calls = PG_GETARG_UINT32(0);

		/* allocate memory for user context */
		fctx = (normal_rand_fctx *) palloc(sizeof(normal_rand_fctx));

		/*
		 * Use fctx to keep track of upper and lower bounds from call to
		 * call. It will also be used to carry over the spare value we get
		 * from the Box-Muller algorithm so that we only actually
		 * calculate a new value every other call.
		 */
		fctx->mean = PG_GETARG_FLOAT8(1);
		fctx->stddev = PG_GETARG_FLOAT8(2);
		fctx->carry_val = 0;
		fctx->use_carry = false;

		funcctx->user_fctx = fctx;

		/*
		 * we might actually get passed a negative number, but for this
		 * purpose it doesn't matter, just cast it as an unsigned value
		 */
		srandom(PG_GETARG_UINT32(3));

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;
	fctx = funcctx->user_fctx;
	mean = fctx->mean;
	stddev = fctx->stddev;
	carry_val = fctx->carry_val;
	use_carry = fctx->use_carry;

	if (call_cntr < max_calls)	/* do when there is more left to send */
	{
		float8		result;

		if (use_carry)
		{
			/*
			 * reset use_carry and use second value obtained on last pass
			 */
			fctx->use_carry = false;
			result = carry_val;
		}
		else
		{
			float8		normval_1;
			float8		normval_2;

			/* Get the next two normal values */
			get_normal_pair(&normval_1, &normval_2);

			/* use the first */
			result = mean + (stddev * normval_1);

			/* and save the second */
			fctx->carry_val = mean + (stddev * normval_2);
			fctx->use_carry = true;
		}

		/* send the result */
		SRF_RETURN_NEXT(funcctx, Float8GetDatum(result));
	}
	else
/* do when there is no more left */
		SRF_RETURN_DONE(funcctx);
}

/*
 * get_normal_pair()
 * Assigns normally distributed (Gaussian) values to a pair of provided
 * parameters, with mean 0, standard deviation 1.
 *
 * This routine implements Algorithm P (Polar method for normal deviates)
 * from Knuth's _The_Art_of_Computer_Programming_, Volume 2, 3rd ed., pages
 * 122-126. Knuth cites his source as "The polar method", G. E. P. Box, M. E.
 * Muller, and G. Marsaglia, _Annals_Math,_Stat._ 29 (1958), 610-611.
 *
 */
static void
get_normal_pair(float8 *x1, float8 *x2)
{
	float8		u1,
				u2,
				v1,
				v2,
				s;

	do
	{
		u1 = (float8) random() / (float8) MAX_RANDOM_VALUE;
		u2 = (float8) random() / (float8) MAX_RANDOM_VALUE;

		v1 = (2.0 * u1) - 1.0;
		v2 = (2.0 * u2) - 1.0;

		s = v1 * v1 + v2 * v2;
	} while (s >= 1.0);

	if (s == 0)
	{
		*x1 = 0;
		*x2 = 0;
	}
	else
	{
		s = sqrt((-2.0 * log(s)) / s);
		*x1 = v1 * s;
		*x2 = v2 * s;
	}
}

/*
 * crosstab - create a crosstab of rowids and values columns from a
 * SQL statement returning one rowid column, one category column,
 * and one value column.
 *
 * e.g. given sql which produces:
 *
 *			rowid	cat		value
 *			------+-------+-------
 *			row1	cat1	val1
 *			row1	cat2	val2
 *			row1	cat3	val3
 *			row1	cat4	val4
 *			row2	cat1	val5
 *			row2	cat2	val6
 *			row2	cat3	val7
 *			row2	cat4	val8
 *
 * crosstab returns:
 *					<===== values columns =====>
 *			rowid	cat1	cat2	cat3	cat4
 *			------+-------+-------+-------+-------
 *			row1	val1	val2	val3	val4
 *			row2	val5	val6	val7	val8
 *
 * NOTES:
 * 1. SQL result must be ordered by 1,2.
 * 2. The number of values columns depends on the tuple description
 *	  of the function's declared return type.
 * 2. Missing values (i.e. not enough adjacent rows of same rowid to
 *	  fill the number of result values columns) are filled in with nulls.
 * 3. Extra values (i.e. too many adjacent rows of same rowid to fill
 *	  the number of result values columns) are skipped.
 * 4. Rows with all nulls in the values columns are skipped.
 */
PG_FUNCTION_INFO_V1(crosstab);
Datum
crosstab(PG_FUNCTION_ARGS)
{
	FuncCallContext *funcctx;
	TupleDesc	ret_tupdesc;
	int			call_cntr;
	int			max_calls;
	TupleTableSlot *slot;
	AttInMetadata *attinmeta;
	SPITupleTable *spi_tuptable = NULL;
	TupleDesc	spi_tupdesc;
	char	   *lastrowid = NULL;
	crosstab_fctx *fctx;
	int			i;
	int			num_categories;
	MemoryContext oldcontext;

	/* stuff done only on the first call of the function */
	if (SRF_IS_FIRSTCALL())
	{
		char	   *sql = GET_STR(PG_GETARG_TEXT_P(0));
		Oid			funcid = fcinfo->flinfo->fn_oid;
		Oid			functypeid;
		char		functyptype;
		TupleDesc	tupdesc = NULL;
		int			ret;
		int			proc;

		/* create a function context for cross-call persistence */
		funcctx = SRF_FIRSTCALL_INIT();

		/*
		 * switch to memory context appropriate for multiple function
		 * calls
		 */
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* Connect to SPI manager */
		if ((ret = SPI_connect()) < 0)
			elog(ERROR, "crosstab: SPI_connect returned %d", ret);

		/* Retrieve the desired rows */
		ret = SPI_exec(sql, 0);
		proc = SPI_processed;

		/* Check for qualifying tuples */
		if ((ret == SPI_OK_SELECT) && (proc > 0))
		{
			spi_tuptable = SPI_tuptable;
			spi_tupdesc = spi_tuptable->tupdesc;

			/*
			 * The provided SQL query must always return three columns.
			 *
			 * 1. rowname	the label or identifier for each row in the final
			 * result 2. category  the label or identifier for each column
			 * in the final result 3. values	the value for each column
			 * in the final result
			 */
			if (spi_tupdesc->natts != 3)
				elog(ERROR, "crosstab: provided SQL must return 3 columns;"
					 " a rowid, a category, and a values column");
		}
		else
		{
			/* no qualifying tuples */
			SPI_finish();
			SRF_RETURN_DONE(funcctx);
		}

		/* SPI switches context on us, so reset it */
		MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* get the typeid that represents our return type */
		functypeid = get_func_rettype(funcid);

		/* check typtype to see if we have a predetermined return type */
		functyptype = get_typtype(functypeid);

		if (functyptype == 'c')
		{
			/* Build a tuple description for a functypeid tuple */
			tupdesc = TypeGetTupleDesc(functypeid, NIL);
		}
		else if (functyptype == 'p' && functypeid == RECORDOID)
		{
			if (fcinfo->nargs != 2)
				elog(ERROR, "Wrong number of arguments specified for function");
			else
			{
				int			num_catagories = PG_GETARG_INT32(1);

				tupdesc = make_crosstab_tupledesc(spi_tupdesc, num_catagories);
			}
		}
		else if (functyptype == 'b')
			elog(ERROR, "Invalid kind of return type specified for function");
		else
			elog(ERROR, "Unknown kind of return type specified for function");

		/*
		 * Check that return tupdesc is compatible with the one we got
		 * from ret_relname, at least based on number and type of
		 * attributes
		 */
		if (!compatCrosstabTupleDescs(tupdesc, spi_tupdesc))
			elog(ERROR, "crosstab: return and sql tuple descriptions are"
				 " incompatible");

		/* allocate a slot for a tuple with this tupdesc */
		slot = TupleDescGetSlot(tupdesc);

		/* assign slot to function context */
		funcctx->slot = slot;

		/*
		 * Generate attribute metadata needed later to produce tuples from
		 * raw C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		/* allocate memory for user context */
		fctx = (crosstab_fctx *) palloc(sizeof(crosstab_fctx));

		/*
		 * Save spi data for use across calls
		 */
		fctx->spi_tuptable = spi_tuptable;
		fctx->lastrowid = NULL;
		funcctx->user_fctx = fctx;

		/* total number of tuples to be returned */
		funcctx->max_calls = proc;

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();

	/*
	 * initialize per-call variables
	 */
	call_cntr = funcctx->call_cntr;
	max_calls = funcctx->max_calls;

	/* return slot for our tuple */
	slot = funcctx->slot;

	/* user context info */
	fctx = (crosstab_fctx *) funcctx->user_fctx;
	lastrowid = fctx->lastrowid;
	spi_tuptable = fctx->spi_tuptable;

	/* the sql tuple */
	spi_tupdesc = spi_tuptable->tupdesc;

	/* attribute return type and return tuple description */
	attinmeta = funcctx->attinmeta;
	ret_tupdesc = attinmeta->tupdesc;

	/* the return tuple always must have 1 rowid + num_categories columns */
	num_categories = ret_tupdesc->natts - 1;

	if (call_cntr < max_calls)	/* do when there is more left to send */
	{
		HeapTuple	tuple;
		Datum		result;
		char	  **values;
		bool		allnulls = true;

		while (true)
		{
			/* allocate space */
			values = (char **) palloc((1 + num_categories) * sizeof(char *));

			/* and make sure it's clear */
			memset(values, '\0', (1 + num_categories) * sizeof(char *));

			/*
			 * now loop through the sql results and assign each value in
			 * sequence to the next category
			 */
			for (i = 0; i < num_categories; i++)
			{
				HeapTuple	spi_tuple;
				char	   *rowid = NULL;

				/* see if we've gone too far already */
				if (call_cntr >= max_calls)
					break;

				/* get the next sql result tuple */
				spi_tuple = spi_tuptable->vals[call_cntr];

				/* get the rowid from the current sql result tuple */
				rowid = SPI_getvalue(spi_tuple, spi_tupdesc, 1);

				/*
				 * If this is the first pass through the values for this
				 * rowid set it, otherwise make sure it hasn't changed on
				 * us. Also check to see if the rowid is the same as that
				 * of the last tuple sent -- if so, skip this tuple
				 * entirely
				 */
				if (i == 0)
					values[0] = pstrdup(rowid);

				if ((rowid != NULL) && (strcmp(rowid, values[0]) == 0))
				{
					if ((lastrowid != NULL) && (strcmp(rowid, lastrowid) == 0))
						break;
					else if (allnulls == true)
						allnulls = false;

					/*
					 * Get the next category item value, which is alway
					 * attribute number three.
					 *
					 * Be careful to sssign the value to the array index
					 * based on which category we are presently
					 * processing.
					 */
					values[1 + i] = SPI_getvalue(spi_tuple, spi_tupdesc, 3);

					/*
					 * increment the counter since we consume a row for
					 * each category, but not for last pass because the
					 * API will do that for us
					 */
					if (i < (num_categories - 1))
						call_cntr = ++funcctx->call_cntr;
				}
				else
				{
					/*
					 * We'll fill in NULLs for the missing values, but we
					 * need to decrement the counter since this sql result
					 * row doesn't belong to the current output tuple.
					 */
					call_cntr = --funcctx->call_cntr;
					break;
				}

				if (rowid != NULL)
					xpfree(rowid);
			}

			xpfree(fctx->lastrowid);

			if (values[0] != NULL)
			{
				/*
				 * switch to memory context appropriate for multiple
				 * function calls
				 */
				oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

				lastrowid = fctx->lastrowid = pstrdup(values[0]);
				MemoryContextSwitchTo(oldcontext);
			}

			if (!allnulls)
			{
				/* build the tuple */
				tuple = BuildTupleFromCStrings(attinmeta, values);

				/* make the tuple into a datum */
				result = TupleGetDatum(slot, tuple);

				/* Clean up */
				for (i = 0; i < num_categories + 1; i++)
					if (values[i] != NULL)
						xpfree(values[i]);
				xpfree(values);

				SRF_RETURN_NEXT(funcctx, result);
			}
			else
			{
				/*
				 * Skipping this tuple entirely, but we need to advance
				 * the counter like the API would if we had returned one.
				 */
				call_cntr = ++funcctx->call_cntr;

				/* we'll start over at the top */
				xpfree(values);

				/* see if we've gone too far already */
				if (call_cntr >= max_calls)
				{
					/* release SPI related resources */
					SPI_finish();
					SRF_RETURN_DONE(funcctx);
				}
			}
		}
	}
	else
/* do when there is no more left */
	{
		/* release SPI related resources */
		SPI_finish();
		SRF_RETURN_DONE(funcctx);
	}
}

/*
 * connectby_text - produce a result set from a hierarchical (parent/child)
 * table.
 *
 * e.g. given table foo:
 *
 *			keyid	parent_keyid
 *			------+--------------
 *			row1	NULL
 *			row2	row1
 *			row3	row1
 *			row4	row2
 *			row5	row2
 *			row6	row4
 *			row7	row3
 *			row8	row6
 *			row9	row5
 *
 *
 * connectby(text relname, text keyid_fld, text parent_keyid_fld,
 *						text start_with, int max_depth [, text branch_delim])
 * connectby('foo', 'keyid', 'parent_keyid', 'row2', 0, '~') returns:
 *
 *		keyid	parent_id	level	 branch
 *		------+-----------+--------+-----------------------
 *		row2	NULL		  0		  row2
 *		row4	row2		  1		  row2~row4
 *		row6	row4		  2		  row2~row4~row6
 *		row8	row6		  3		  row2~row4~row6~row8
 *		row5	row2		  1		  row2~row5
 *		row9	row5		  2		  row2~row5~row9
 *
 */
PG_FUNCTION_INFO_V1(connectby_text);

#define CONNECTBY_NCOLS					4
#define CONNECTBY_NCOLS_NOBRANCH		3

Datum
connectby_text(PG_FUNCTION_ARGS)
{
	char	   *relname = GET_STR(PG_GETARG_TEXT_P(0));
	char	   *key_fld = GET_STR(PG_GETARG_TEXT_P(1));
	char	   *parent_key_fld = GET_STR(PG_GETARG_TEXT_P(2));
	char	   *start_with = GET_STR(PG_GETARG_TEXT_P(3));
	int			max_depth = PG_GETARG_INT32(4);
	char	   *branch_delim = NULL;
	bool		show_branch = false;
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	AttInMetadata *attinmeta;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	if (fcinfo->nargs == 6)
	{
		branch_delim = GET_STR(PG_GETARG_TEXT_P(5));
		show_branch = true;
	}

	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/* get the requested return tuple description */
	tupdesc = CreateTupleDescCopy(rsinfo->expectedDesc);

	/* does it meet our needs */
	validateConnectbyTupleDesc(tupdesc, show_branch);

	/* OK, use it then */
	attinmeta = TupleDescGetAttInMetadata(tupdesc);

	/* check to see if caller supports us returning a tuplestore */
	if (!rsinfo->allowedModes & SFRM_Materialize)
		elog(ERROR, "connectby requires Materialize mode, but it is not "
			 "allowed in this context");

	/* OK, go to work */
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = connectby(relname,
								  key_fld,
								  parent_key_fld,
								  branch_delim,
								  start_with,
								  max_depth,
								  show_branch,
								  per_query_ctx,
								  attinmeta);
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/*
	 * SFRM_Materialize mode expects us to return a NULL Datum. The actual
	 * tuples are in our tuplestore and passed back through
	 * rsinfo->setResult. rsinfo->setDesc is set to the tuple description
	 * that we actually used to build our tuples with, so the caller can
	 * verify we did what it was expecting.
	 */
	return (Datum) 0;
}

/*
 * connectby - does the real work for connectby_text()
 */
static Tuplestorestate *
connectby(char *relname,
		  char *key_fld,
		  char *parent_key_fld,
		  char *branch_delim,
		  char *start_with,
		  int max_depth,
		  bool show_branch,
		  MemoryContext per_query_ctx,
		  AttInMetadata *attinmeta)
{
	Tuplestorestate *tupstore = NULL;
	int			ret;
	MemoryContext oldcontext;

	/* Connect to SPI manager */
	if ((ret = SPI_connect()) < 0)
		elog(ERROR, "connectby: SPI_connect returned %d", ret);

	/* switch to longer term context to create the tuple store */
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/* initialize our tuplestore */
	tupstore = tuplestore_begin_heap(true, SortMem);

	MemoryContextSwitchTo(oldcontext);

	/* now go get the whole tree */
	tupstore = build_tuplestore_recursively(key_fld,
											parent_key_fld,
											relname,
											branch_delim,
											start_with,
											start_with, /* current_branch */
											0,	/* initial level is 0 */
											max_depth,
											show_branch,
											per_query_ctx,
											attinmeta,
											tupstore);

	SPI_finish();

	oldcontext = MemoryContextSwitchTo(per_query_ctx);
	tuplestore_donestoring(tupstore);
	MemoryContextSwitchTo(oldcontext);

	return tupstore;
}

static Tuplestorestate *
build_tuplestore_recursively(char *key_fld,
							 char *parent_key_fld,
							 char *relname,
							 char *branch_delim,
							 char *start_with,
							 char *branch,
							 int level,
							 int max_depth,
							 bool show_branch,
							 MemoryContext per_query_ctx,
							 AttInMetadata *attinmeta,
							 Tuplestorestate *tupstore)
{
	TupleDesc	tupdesc = attinmeta->tupdesc;
	MemoryContext oldcontext;
	StringInfo	sql = makeStringInfo();
	int			ret;
	int			proc;

	if (max_depth > 0 && level > max_depth)
		return tupstore;

	/* Build initial sql statement */
	appendStringInfo(sql, "SELECT %s, %s FROM %s WHERE %s = '%s' AND %s IS NOT NULL",
					 quote_ident_cstr(key_fld),
					 quote_ident_cstr(parent_key_fld),
					 quote_ident_cstr(relname),
					 quote_ident_cstr(parent_key_fld),
					 start_with,
					 quote_ident_cstr(key_fld));

	/* Retrieve the desired rows */
	ret = SPI_exec(sql->data, 0);
	proc = SPI_processed;

	/* Check for qualifying tuples */
	if ((ret == SPI_OK_SELECT) && (proc > 0))
	{
		HeapTuple	tuple;
		HeapTuple	spi_tuple;
		SPITupleTable *tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = tuptable->tupdesc;
		int			i;
		char	   *current_key;
		char	   *current_key_parent;
		char		current_level[INT32_STRLEN];
		char	   *current_branch;
		char	  **values;
		StringInfo	branchstr = NULL;

		/* start a new branch */
		branchstr = makeStringInfo();

		if (show_branch)
			values = (char **) palloc(CONNECTBY_NCOLS * sizeof(char *));
		else
			values = (char **) palloc(CONNECTBY_NCOLS_NOBRANCH * sizeof(char *));

		/* First time through, do a little setup */
		if (level == 0)
		{
			/*
			 * Check that return tupdesc is compatible with the one we got
			 * from the query, but only at level 0 -- no need to check
			 * more than once
			 */

			if (!compatConnectbyTupleDescs(tupdesc, spi_tupdesc))
				elog(ERROR, "connectby: return and sql tuple descriptions are "
					 "incompatible");

			/* root value is the one we initially start with */
			values[0] = start_with;

			/* root value has no parent */
			values[1] = NULL;

			/* root level is 0 */
			sprintf(current_level, "%d", level);
			values[2] = current_level;

			/* root branch is just starting root value */
			if (show_branch)
				values[3] = start_with;

			/* construct the tuple */
			tuple = BuildTupleFromCStrings(attinmeta, values);

			/* switch to long lived context while storing the tuple */
			oldcontext = MemoryContextSwitchTo(per_query_ctx);

			/* now store it */
			tuplestore_puttuple(tupstore, tuple);

			/* now reset the context */
			MemoryContextSwitchTo(oldcontext);

			/* increment level */
			level++;
		}

		for (i = 0; i < proc; i++)
		{
			/* initialize branch for this pass */
			appendStringInfo(branchstr, "%s", branch);

			/* get the next sql result tuple */
			spi_tuple = tuptable->vals[i];

			/* get the current key and parent */
			current_key = SPI_getvalue(spi_tuple, spi_tupdesc, 1);
			current_key_parent = pstrdup(SPI_getvalue(spi_tuple, spi_tupdesc, 2));

			/* check to see if this key is also an ancestor */
			if (strstr(branchstr->data, current_key))
				elog(ERROR, "infinite recursion detected");

			/* get the current level */
			sprintf(current_level, "%d", level);

			/* extend the branch */
			appendStringInfo(branchstr, "%s%s", branch_delim, current_key);
			current_branch = branchstr->data;

			/* build a tuple */
			values[0] = pstrdup(current_key);
			values[1] = current_key_parent;
			values[2] = current_level;
			if (show_branch)
				values[3] = current_branch;

			tuple = BuildTupleFromCStrings(attinmeta, values);

			xpfree(current_key);
			xpfree(current_key_parent);

			/* switch to long lived context while storing the tuple */
			oldcontext = MemoryContextSwitchTo(per_query_ctx);

			/* store the tuple for later use */
			tuplestore_puttuple(tupstore, tuple);

			/* now reset the context */
			MemoryContextSwitchTo(oldcontext);

			heap_freetuple(tuple);

			/* recurse using current_key_parent as the new start_with */
			tupstore = build_tuplestore_recursively(key_fld,
													parent_key_fld,
													relname,
													branch_delim,
													values[0],
													current_branch,
													level + 1,
													max_depth,
													show_branch,
													per_query_ctx,
													attinmeta,
													tupstore);

			/* reset branch for next pass */
			xpfree(branchstr->data);
			initStringInfo(branchstr);
		}
	}

	return tupstore;
}

/*
 * Check expected (query runtime) tupdesc suitable for Connectby
 */
static void
validateConnectbyTupleDesc(TupleDesc tupdesc, bool show_branch)
{
	/* are there the correct number of columns */
	if (show_branch)
	{
		if (tupdesc->natts != CONNECTBY_NCOLS)
			elog(ERROR, "Query-specified return tuple not valid for Connectby: "
				 "wrong number of columns");
	}
	else
	{
		if (tupdesc->natts != CONNECTBY_NCOLS_NOBRANCH)
			elog(ERROR, "Query-specified return tuple not valid for Connectby: "
				 "wrong number of columns");
	}

	/* check that the types of the first two columns match */
	if (tupdesc->attrs[0]->atttypid != tupdesc->attrs[1]->atttypid)
		elog(ERROR, "Query-specified return tuple not valid for Connectby: "
			 "first two columns must be the same type");

	/* check that the type of the third column is INT4 */
	if (tupdesc->attrs[2]->atttypid != INT4OID)
		elog(ERROR, "Query-specified return tuple not valid for Connectby: "
			 "third column must be type %s", format_type_be(INT4OID));

	/* check that the type of the forth column is TEXT if applicable */
	if (show_branch && tupdesc->attrs[3]->atttypid != TEXTOID)
		elog(ERROR, "Query-specified return tuple not valid for Connectby: "
			 "third column must be type %s", format_type_be(TEXTOID));

	/* OK, the tupdesc is valid for our purposes */
}

/*
 * Check if spi sql tupdesc and return tupdesc are compatible
 */
static bool
compatConnectbyTupleDescs(TupleDesc ret_tupdesc, TupleDesc sql_tupdesc)
{
	Oid			ret_atttypid;
	Oid			sql_atttypid;

	/* check the key_fld types match */
	ret_atttypid = ret_tupdesc->attrs[0]->atttypid;
	sql_atttypid = sql_tupdesc->attrs[0]->atttypid;
	if (ret_atttypid != sql_atttypid)
		elog(ERROR, "compatConnectbyTupleDescs: SQL key field datatype does "
			 "not match return key field datatype");

	/* check the parent_key_fld types match */
	ret_atttypid = ret_tupdesc->attrs[1]->atttypid;
	sql_atttypid = sql_tupdesc->attrs[1]->atttypid;
	if (ret_atttypid != sql_atttypid)
		elog(ERROR, "compatConnectbyTupleDescs: SQL parent key field datatype "
			 "does not match return parent key field datatype");

	/* OK, the two tupdescs are compatible for our purposes */
	return true;
}

/*
 * Check if two tupdescs match in type of attributes
 */
static bool
compatCrosstabTupleDescs(TupleDesc ret_tupdesc, TupleDesc sql_tupdesc)
{
	int			i;
	Form_pg_attribute ret_attr;
	Oid			ret_atttypid;
	Form_pg_attribute sql_attr;
	Oid			sql_atttypid;

	/* check the rowid types match */
	ret_atttypid = ret_tupdesc->attrs[0]->atttypid;
	sql_atttypid = sql_tupdesc->attrs[0]->atttypid;
	if (ret_atttypid != sql_atttypid)
		elog(ERROR, "compatCrosstabTupleDescs: SQL rowid datatype does not match"
			 " return rowid datatype");

	/*
	 * - attribute [1] of the sql tuple is the category; no need to check
	 * it - attribute [2] of the sql tuple should match attributes [1] to
	 * [natts] of the return tuple
	 */
	sql_attr = sql_tupdesc->attrs[2];
	for (i = 1; i < ret_tupdesc->natts; i++)
	{
		ret_attr = ret_tupdesc->attrs[i];

		if (ret_attr->atttypid != sql_attr->atttypid)
			return false;
	}

	/* OK, the two tupdescs are compatible for our purposes */
	return true;
}

static TupleDesc
make_crosstab_tupledesc(TupleDesc spi_tupdesc, int num_catagories)
{
	Form_pg_attribute sql_attr;
	Oid			sql_atttypid;
	TupleDesc	tupdesc;
	int			natts;
	AttrNumber	attnum;
	char		attname[NAMEDATALEN];
	int			i;

	/*
	 * We need to build a tuple description with one column for the
	 * rowname, and num_catagories columns for the values. Each must be of
	 * the same type as the corresponding spi result input column.
	 */
	natts = num_catagories + 1;
	tupdesc = CreateTemplateTupleDesc(natts, false);

	/* first the rowname column */
	attnum = 1;

	sql_attr = spi_tupdesc->attrs[0];
	sql_atttypid = sql_attr->atttypid;

	strcpy(attname, "rowname");

	TupleDescInitEntry(tupdesc, attnum, attname, sql_atttypid,
					   -1, 0, false);

	/* now the catagory values columns */
	sql_attr = spi_tupdesc->attrs[2];
	sql_atttypid = sql_attr->atttypid;

	for (i = 0; i < num_catagories; i++)
	{
		attnum++;

		sprintf(attname, "category_%d", i + 1);
		TupleDescInitEntry(tupdesc, attnum, attname, sql_atttypid,
						   -1, 0, false);
	}

	return tupdesc;
}

/*
 * Return a properly quoted identifier.
 * Uses quote_ident in quote.c
 */
static char *
quote_ident_cstr(char *rawstr)
{
	text	   *rawstr_text;
	text	   *result_text;
	char	   *result;

	rawstr_text = DatumGetTextP(DirectFunctionCall1(textin, CStringGetDatum(rawstr)));
	result_text = DatumGetTextP(DirectFunctionCall1(quote_ident, PointerGetDatum(rawstr_text)));
	result = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(result_text)));

	return result;
}
