/*
 * refint.c --	set of functions to define referential integrity
 *		constraints using general triggers.
 */

#include "executor/spi.h"		/* this is what you need to work with SPI */
#include "commands/trigger.h"	/* -"- and triggers */
#include <ctype.h>


extern Datum check_primary_key(PG_FUNCTION_ARGS);
extern Datum check_foreign_key(PG_FUNCTION_ARGS);


typedef struct
{
	char	   *ident;
	int			nplans;
	void	  **splan;
}			EPlan;

static EPlan *FPlans = NULL;
static int	nFPlans = 0;
static EPlan *PPlans = NULL;
static int	nPPlans = 0;

static EPlan *find_plan(char *ident, EPlan ** eplan, int *nplans);

/*
 * check_primary_key () -- check that key in tuple being inserted/updated
 *			 references existing tuple in "primary" table.
 * Though it's called without args You have to specify referenced
 * table/keys while creating trigger:  key field names in triggered table,
 * referenced table name, referenced key field names:
 * EXECUTE PROCEDURE
 * check_primary_key ('Fkey1', 'Fkey2', 'Ptable', 'Pkey1', 'Pkey2').
 */

PG_FUNCTION_INFO_V1(check_primary_key);

Datum
check_primary_key(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	Trigger    *trigger;		/* to get trigger name */
	int			nargs;			/* # of args specified in CREATE TRIGGER */
	char	  **args;			/* arguments: column names and table name */
	int			nkeys;			/* # of key columns (= nargs / 2) */
	Datum	   *kvals;			/* key values */
	char	   *relname;		/* referenced relation name */
	Relation	rel;			/* triggered relation */
	HeapTuple	tuple = NULL;	/* tuple to return */
	TupleDesc	tupdesc;		/* tuple description */
	EPlan	   *plan;			/* prepared plan */
	Oid		   *argtypes = NULL;/* key types to prepare execution plan */
	bool		isnull;			/* to know is some column NULL or not */
	char		ident[2 * NAMEDATALEN]; /* to identify myself */
	int			ret;
	int			i;

#ifdef	DEBUG_QUERY
	elog(NOTICE, "Check_primary_key Enter Function");
#endif

	/*
	 * Some checks first...
	 */

	/* Called by trigger manager ? */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "check_primary_key: not fired by trigger manager");

	/* Should be called for ROW trigger */
	if (TRIGGER_FIRED_FOR_STATEMENT(trigdata->tg_event))
		elog(ERROR, "check_primary_key: can't process STATEMENT events");

	/* If INSERTion then must check Tuple to being inserted */
	if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
		tuple = trigdata->tg_trigtuple;

	/* Not should be called for DELETE */
	else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		elog(ERROR, "check_primary_key: can't process DELETE events");

	/* If UPDATion the must check new Tuple, not old one */
	else
		tuple = trigdata->tg_newtuple;

	trigger = trigdata->tg_trigger;
	nargs = trigger->tgnargs;
	args = trigger->tgargs;

	if (nargs % 2 != 1)			/* odd number of arguments! */
		elog(ERROR, "check_primary_key: odd number of arguments should be specified");

	nkeys = nargs / 2;
	relname = args[nkeys];
	rel = trigdata->tg_relation;
	tupdesc = rel->rd_att;

	/* Connect to SPI manager */
	if ((ret = SPI_connect()) < 0)
		elog(ERROR, "check_primary_key: SPI_connect returned %d", ret);

	/*
	 * We use SPI plan preparation feature, so allocate space to place key
	 * values.
	 */
	kvals = (Datum *) palloc(nkeys * sizeof(Datum));

	/*
	 * Construct ident string as TriggerName $ TriggeredRelationId and try
	 * to find prepared execution plan.
	 */
	sprintf(ident, "%s$%u", trigger->tgname, rel->rd_id);
	plan = find_plan(ident, &PPlans, &nPPlans);

	/* if there is no plan then allocate argtypes for preparation */
	if (plan->nplans <= 0)
		argtypes = (Oid *) palloc(nkeys * sizeof(Oid));

	/* For each column in key ... */
	for (i = 0; i < nkeys; i++)
	{
		/* get index of column in tuple */
		int			fnumber = SPI_fnumber(tupdesc, args[i]);

		/* Bad guys may give us un-existing column in CREATE TRIGGER */
		if (fnumber < 0)
			elog(ERROR, "check_primary_key: there is no attribute %s in relation %s",
				 args[i], SPI_getrelname(rel));

		/* Well, get binary (in internal format) value of column */
		kvals[i] = SPI_getbinval(tuple, tupdesc, fnumber, &isnull);

		/*
		 * If it's NULL then nothing to do! DON'T FORGET call SPI_finish
		 * ()! DON'T FORGET return tuple! Executor inserts tuple you're
		 * returning! If you return NULL then nothing will be inserted!
		 */
		if (isnull)
		{
			SPI_finish();
			return PointerGetDatum(tuple);
		}

		if (plan->nplans <= 0)	/* Get typeId of column */
			argtypes[i] = SPI_gettypeid(tupdesc, fnumber);
	}

	/*
	 * If we have to prepare plan ...
	 */
	if (plan->nplans <= 0)
	{
		void	   *pplan;
		char		sql[8192];

		/*
		 * Construct query: SELECT 1 FROM _referenced_relation_ WHERE
		 * Pkey1 = $1 [AND Pkey2 = $2 [...]]
		 */
		sprintf(sql, "select 1 from %s where ", relname);
		for (i = 0; i < nkeys; i++)
		{
			sprintf(sql + strlen(sql), "%s = $%d %s",
			  args[i + nkeys + 1], i + 1, (i < nkeys - 1) ? "and " : "");
		}

		/* Prepare plan for query */
		pplan = SPI_prepare(sql, nkeys, argtypes);
		if (pplan == NULL)
			elog(ERROR, "check_primary_key: SPI_prepare returned %d", SPI_result);

		/*
		 * Remember that SPI_prepare places plan in current memory context
		 * - so, we have to save plan in Top memory context for latter
		 * use.
		 */
		pplan = SPI_saveplan(pplan);
		if (pplan == NULL)
			elog(ERROR, "check_primary_key: SPI_saveplan returned %d", SPI_result);
		plan->splan = (void **) malloc(sizeof(void *));
		*(plan->splan) = pplan;
		plan->nplans = 1;
	}

	/*
	 * Ok, execute prepared plan.
	 */
	ret = SPI_execp(*(plan->splan), kvals, NULL, 1);
	/* we have no NULLs - so we pass   ^^^^   here */

	if (ret < 0)
		elog(ERROR, "check_primary_key: SPI_execp returned %d", ret);

	/*
	 * If there are no tuples returned by SELECT then ...
	 */
	if (SPI_processed == 0)
		elog(ERROR, "%s: tuple references non-existing key in %s",
			 trigger->tgname, relname);

	SPI_finish();

	return PointerGetDatum(tuple);
}

/*
 * check_foreign_key () -- check that key in tuple being deleted/updated
 *			 is not referenced by tuples in "foreign" table(s).
 * Though it's called without args You have to specify (while creating trigger):
 * number of references, action to do if key referenced
 * ('restrict' | 'setnull' | 'cascade'), key field names in triggered
 * ("primary") table and referencing table(s)/keys:
 * EXECUTE PROCEDURE
 * check_foreign_key (2, 'restrict', 'Pkey1', 'Pkey2',
 * 'Ftable1', 'Fkey11', 'Fkey12', 'Ftable2', 'Fkey21', 'Fkey22').
 */

PG_FUNCTION_INFO_V1(check_foreign_key);

Datum
check_foreign_key(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	Trigger    *trigger;		/* to get trigger name */
	int			nargs;			/* # of args specified in CREATE TRIGGER */
	char	  **args;			/* arguments: as described above */
	char	  **args_temp;
	int			nrefs;			/* number of references (== # of plans) */
	char		action;			/* 'R'estrict | 'S'etnull | 'C'ascade */
	int			nkeys;			/* # of key columns */
	Datum	   *kvals;			/* key values */
	char	   *relname;		/* referencing relation name */
	Relation	rel;			/* triggered relation */
	HeapTuple	trigtuple = NULL;		/* tuple to being changed */
	HeapTuple	newtuple = NULL;/* tuple to return */
	TupleDesc	tupdesc;		/* tuple description */
	EPlan	   *plan;			/* prepared plan(s) */
	Oid		   *argtypes = NULL;/* key types to prepare execution plan */
	bool		isnull;			/* to know is some column NULL or not */
	bool		isequal = true; /* are keys in both tuples equal (in
								 * UPDATE) */
	char		ident[2 * NAMEDATALEN]; /* to identify myself */
	int			is_update = 0;
	int			ret;
	int			i,
				r;

#ifdef DEBUG_QUERY
	elog(NOTICE, "Check_foreign_key Enter Function");
#endif

	/*
	 * Some checks first...
	 */

	/* Called by trigger manager ? */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "check_foreign_key: not fired by trigger manager");

	/* Should be called for ROW trigger */
	if (TRIGGER_FIRED_FOR_STATEMENT(trigdata->tg_event))
		elog(ERROR, "check_foreign_key: can't process STATEMENT events");

	/* Not should be called for INSERT */
	if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
		elog(ERROR, "check_foreign_key: can't process INSERT events");

	/* Have to check tg_trigtuple - tuple being deleted */
	trigtuple = trigdata->tg_trigtuple;

	/*
	 * But if this is UPDATE then we have to return tg_newtuple. Also, if
	 * key in tg_newtuple is the same as in tg_trigtuple then nothing to
	 * do.
	 */
	is_update = 0;
	if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
	{
		newtuple = trigdata->tg_newtuple;
		is_update = 1;
	}
	trigger = trigdata->tg_trigger;
	nargs = trigger->tgnargs;
	args = trigger->tgargs;

	if (nargs < 5)				/* nrefs, action, key, Relation, key - at
								 * least */
		elog(ERROR, "check_foreign_key: too short %d (< 5) list of arguments", nargs);

	nrefs = pg_atoi(args[0], sizeof(int), 0);
	if (nrefs < 1)
		elog(ERROR, "check_foreign_key: %d (< 1) number of references specified", nrefs);
	action = tolower((unsigned char) *(args[1]));
	if (action != 'r' && action != 'c' && action != 's')
		elog(ERROR, "check_foreign_key: invalid action %s", args[1]);
	nargs -= 2;
	args += 2;
	nkeys = (nargs - nrefs) / (nrefs + 1);
	if (nkeys <= 0 || nargs != (nrefs + nkeys * (nrefs + 1)))
		elog(ERROR, "check_foreign_key: invalid number of arguments %d for %d references",
			 nargs + 2, nrefs);

	rel = trigdata->tg_relation;
	tupdesc = rel->rd_att;

	/* Connect to SPI manager */
	if ((ret = SPI_connect()) < 0)
		elog(ERROR, "check_foreign_key: SPI_connect returned %d", ret);

	/*
	 * We use SPI plan preparation feature, so allocate space to place key
	 * values.
	 */
	kvals = (Datum *) palloc(nkeys * sizeof(Datum));

	/*
	 * Construct ident string as TriggerName $ TriggeredRelationId and try
	 * to find prepared execution plan(s).
	 */
	sprintf(ident, "%s$%u", trigger->tgname, rel->rd_id);
	plan = find_plan(ident, &FPlans, &nFPlans);

	/* if there is no plan(s) then allocate argtypes for preparation */
	if (plan->nplans <= 0)
		argtypes = (Oid *) palloc(nkeys * sizeof(Oid));

	/*
	 * else - check that we have exactly nrefs plan(s) ready
	 */
	else if (plan->nplans != nrefs)
		elog(ERROR, "%s: check_foreign_key: # of plans changed in meantime",
			 trigger->tgname);

	/* For each column in key ... */
	for (i = 0; i < nkeys; i++)
	{
		/* get index of column in tuple */
		int			fnumber = SPI_fnumber(tupdesc, args[i]);

		/* Bad guys may give us un-existing column in CREATE TRIGGER */
		if (fnumber < 0)
			elog(ERROR, "check_foreign_key: there is no attribute %s in relation %s",
				 args[i], SPI_getrelname(rel));

		/* Well, get binary (in internal format) value of column */
		kvals[i] = SPI_getbinval(trigtuple, tupdesc, fnumber, &isnull);

		/*
		 * If it's NULL then nothing to do! DON'T FORGET call SPI_finish
		 * ()! DON'T FORGET return tuple! Executor inserts tuple you're
		 * returning! If you return NULL then nothing will be inserted!
		 */
		if (isnull)
		{
			SPI_finish();
			return PointerGetDatum((newtuple == NULL) ? trigtuple : newtuple);
		}

		/*
		 * If UPDATE then get column value from new tuple being inserted
		 * and compare is this the same as old one. For the moment we use
		 * string presentation of values...
		 */
		if (newtuple != NULL)
		{
			char	   *oldval = SPI_getvalue(trigtuple, tupdesc, fnumber);
			char	   *newval;

			/* this shouldn't happen! SPI_ERROR_NOOUTFUNC ? */
			if (oldval == NULL)
				elog(ERROR, "check_foreign_key: SPI_getvalue returned %d", SPI_result);
			newval = SPI_getvalue(newtuple, tupdesc, fnumber);
			if (newval == NULL || strcmp(oldval, newval) != 0)
				isequal = false;
		}

		if (plan->nplans <= 0)	/* Get typeId of column */
			argtypes[i] = SPI_gettypeid(tupdesc, fnumber);
	}
	args_temp = args;
	nargs -= nkeys;
	args += nkeys;

	/*
	 * If we have to prepare plans ...
	 */
	if (plan->nplans <= 0)
	{
		void	   *pplan;
		char		sql[8192];
		char	  **args2 = args;

		plan->splan = (void **) malloc(nrefs * sizeof(void *));

		for (r = 0; r < nrefs; r++)
		{
			relname = args2[0];

			/*---------
			 * For 'R'estrict action we construct SELECT query:
			 *
			 *  SELECT 1
			 *	FROM _referencing_relation_
			 *	WHERE Fkey1 = $1 [AND Fkey2 = $2 [...]]
			 *
			 *  to check is tuple referenced or not.
			 *---------
			 */
			if (action == 'r')

				sprintf(sql, "select 1 from %s where ", relname);

			/*---------
			 * For 'C'ascade action we construct DELETE query
			 *
			 *	DELETE
			 *	FROM _referencing_relation_
			 *	WHERE Fkey1 = $1 [AND Fkey2 = $2 [...]]
			 *
			 * to delete all referencing tuples.
			 *---------
			 */

			/*
			 * Max : Cascade with UPDATE query i create update query that
			 * updates new key values in referenced tables
			 */


			else if (action == 'c')
			{
				if (is_update == 1)
				{
					int			fn;
					char	   *nv;
					int			k;

					sprintf(sql, "update %s set ", relname);
					for (k = 1; k <= nkeys; k++)
					{
						int			is_char_type = 0;
						char	   *type;

						fn = SPI_fnumber(tupdesc, args_temp[k - 1]);
						nv = SPI_getvalue(newtuple, tupdesc, fn);
						type = SPI_gettype(tupdesc, fn);

						if ((strcmp(type, "text") && strcmp(type, "varchar") &&
						strcmp(type, "char") && strcmp(type, "bpchar") &&
							 strcmp(type, "date") && strcmp(type, "datetime")) == 0)
							is_char_type = 1;
#ifdef	DEBUG_QUERY
						elog(NOTICE, "Check_foreign_key Debug value %s type %s %d",
							 nv, type, is_char_type);
#endif

						/*
						 * is_char_type =1 i set ' ' for define a new
						 * value
						 */
						sprintf(sql + strlen(sql), " %s = %s%s%s %s ",
								args2[k], (is_char_type > 0) ? "'" : "",
								nv, (is_char_type > 0) ? "'" : "", (k < nkeys) ? ", " : "");
						is_char_type = 0;
					}
					strcat(sql, " where ");

				}
				else
/* DELETE */
					sprintf(sql, "delete from %s where ", relname);

			}

			/*
			 * For 'S'etnull action we construct UPDATE query - UPDATE
			 * _referencing_relation_ SET Fkey1 null [, Fkey2 null [...]]
			 * WHERE Fkey1 = $1 [AND Fkey2 = $2 [...]] - to set key
			 * columns in all referencing tuples to NULL.
			 */
			else if (action == 's')
			{
				sprintf(sql, "update %s set ", relname);
				for (i = 1; i <= nkeys; i++)
				{
					sprintf(sql + strlen(sql), "%s = null%s",
							args2[i], (i < nkeys) ? ", " : "");
				}
				strcat(sql, " where ");
			}

			/* Construct WHERE qual */
			for (i = 1; i <= nkeys; i++)
			{
				sprintf(sql + strlen(sql), "%s = $%d %s",
						args2[i], i, (i < nkeys) ? "and " : "");
			}

			/* Prepare plan for query */
			pplan = SPI_prepare(sql, nkeys, argtypes);
			if (pplan == NULL)
				elog(ERROR, "check_foreign_key: SPI_prepare returned %d", SPI_result);

			/*
			 * Remember that SPI_prepare places plan in current memory
			 * context - so, we have to save plan in Top memory context
			 * for latter use.
			 */
			pplan = SPI_saveplan(pplan);
			if (pplan == NULL)
				elog(ERROR, "check_foreign_key: SPI_saveplan returned %d", SPI_result);

			plan->splan[r] = pplan;

			args2 += nkeys + 1; /* to the next relation */
		}
		plan->nplans = nrefs;
#ifdef	DEBUG_QUERY
		elog(NOTICE, "Check_foreign_key Debug Query is :  %s ", sql);
#endif
	}

	/*
	 * If UPDATE and key is not changed ...
	 */
	if (newtuple != NULL && isequal)
	{
		SPI_finish();
		return PointerGetDatum(newtuple);
	}

	/*
	 * Ok, execute prepared plan(s).
	 */
	for (r = 0; r < nrefs; r++)
	{

		/*
		 * For 'R'estrict we may to execute plan for one tuple only, for
		 * other actions - for all tuples.
		 */
		int			tcount = (action == 'r') ? 1 : 0;

		relname = args[0];

		sprintf(ident, "%s$%u", trigger->tgname, rel->rd_id);
		plan = find_plan(ident, &FPlans, &nFPlans);
		ret = SPI_execp(plan->splan[r], kvals, NULL, tcount);
		/* we have no NULLs - so we pass   ^^^^  here */

		if (ret < 0)
			elog(ERROR, "check_foreign_key: SPI_execp returned %d", ret);

		/* If action is 'R'estrict ... */
		if (action == 'r')
		{
			/* If there is tuple returned by SELECT then ... */
			if (SPI_processed > 0)
				elog(ERROR, "%s: tuple referenced in %s",
					 trigger->tgname, relname);
		}
#ifdef REFINT_VERBOSE
		else
			elog(NOTICE, "%s: %d tuple(s) of %s are %s",
				 trigger->tgname, SPI_processed, relname,
				 (action == 'c') ? "deleted" : "setted to null");
#endif
		args += nkeys + 1;		/* to the next relation */
	}

	SPI_finish();

	return PointerGetDatum((newtuple == NULL) ? trigtuple : newtuple);
}

static EPlan *
find_plan(char *ident, EPlan ** eplan, int *nplans)
{
	EPlan	   *newp;
	int			i;

	if (*nplans > 0)
	{
		for (i = 0; i < *nplans; i++)
		{
			if (strcmp((*eplan)[i].ident, ident) == 0)
				break;
		}
		if (i != *nplans)
			return (*eplan + i);
		*eplan = (EPlan *) realloc(*eplan, (i + 1) * sizeof(EPlan));
		newp = *eplan + i;
	}
	else
	{
		newp = *eplan = (EPlan *) malloc(sizeof(EPlan));
		(*nplans) = i = 0;
	}

	newp->ident = (char *) malloc(strlen(ident) + 1);
	strcpy(newp->ident, ident);
	newp->nplans = 0;
	newp->splan = NULL;
	(*nplans)++;

	return (newp);
}
