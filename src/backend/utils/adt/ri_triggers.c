/* ----------
 * ri_triggers.c
 *
 *	Generic trigger procedures for referential integrity constraint
 *	checks.
 *
 *	Note about memory management: the private hashtables kept here live
 *	across query and transaction boundaries, in fact they live as long as
 *	the backend does.  This works because the hashtable structures
 *	themselves are allocated by dynahash.c in its permanent DynaHashCxt,
 *	and the parse/plan node trees they point to are copied into
 *	TopMemoryContext using SPI_saveplan().	This is pretty ugly, since there
 *	is no way to free a no-longer-needed plan tree, but then again we don't
 *	yet have any bookkeeping that would allow us to detect that a plan isn't
 *	needed anymore.  Improve it someday.
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/ri_triggers.c,v 1.43.2.5 2004-10-13 22:22:22 tgl Exp $
 *
 * ----------
 */


/* ----------
 * Internal TODO:
 *
 *		Add MATCH PARTIAL logic.
 * ----------
 */

#include "postgres.h"

#include "catalog/pg_operator.h"
#include "commands/trigger.h"
#include "executor/spi_priv.h"
#include "optimizer/planmain.h"
#include "parser/parse_oper.h"
#include "rewrite/rewriteHandler.h"
#include "utils/lsyscache.h"
#include "miscadmin.h"


/* ----------
 * Local definitions
 * ----------
 */

#define RI_INIT_QUERYHASHSIZE			128
#define RI_INIT_OPREQHASHSIZE			128

#define RI_MATCH_TYPE_UNSPECIFIED		0
#define RI_MATCH_TYPE_FULL				1
#define RI_MATCH_TYPE_PARTIAL			2

#define RI_KEYS_ALL_NULL				0
#define RI_KEYS_SOME_NULL				1
#define RI_KEYS_NONE_NULL				2

/* queryno values must be distinct for the convenience of ri_PerformCheck */
#define RI_PLAN_CHECK_LOOKUPPK_NOCOLS	1
#define RI_PLAN_CHECK_LOOKUPPK			2
#define RI_PLAN_CASCADE_DEL_DODELETE	3
#define RI_PLAN_CASCADE_UPD_DOUPDATE	4
#define RI_PLAN_NOACTION_DEL_CHECKREF	5
#define RI_PLAN_NOACTION_UPD_CHECKREF	6
#define RI_PLAN_RESTRICT_DEL_CHECKREF	7
#define RI_PLAN_RESTRICT_UPD_CHECKREF	8
#define RI_PLAN_SETNULL_DEL_DOUPDATE	9
#define RI_PLAN_SETNULL_UPD_DOUPDATE	10
#define RI_PLAN_KEYEQUAL_UPD			11

#define MAX_QUOTED_NAME_LEN  (NAMEDATALEN*2+3)
#define MAX_QUOTED_REL_NAME_LEN  (MAX_QUOTED_NAME_LEN*2)


/* ----------
 * RI_QueryKey
 *
 *	The key identifying a prepared SPI plan in our private hashtable
 * ----------
 */
typedef struct RI_QueryKey
{
	int32		constr_type;
	Oid			constr_id;
	int32		constr_queryno;
	Oid			fk_relid;
	Oid			pk_relid;
	int32		nkeypairs;
	int16		keypair[RI_MAX_NUMKEYS][2];
} RI_QueryKey;


/* ----------
 * RI_QueryHashEntry
 * ----------
 */
typedef struct RI_QueryHashEntry
{
	RI_QueryKey key;
	void	   *plan;
} RI_QueryHashEntry;


typedef struct RI_OpreqHashEntry
{
	Oid			typeid;
	FmgrInfo	oprfmgrinfo;
} RI_OpreqHashEntry;



/* ----------
 * Local data
 * ----------
 */
static HTAB *ri_query_cache = (HTAB *) NULL;
static HTAB *ri_opreq_cache = (HTAB *) NULL;


/* ----------
 * Local function prototypes
 * ----------
 */
static void quoteOneName(char *buffer, const char *name);
static void quoteRelationName(char *buffer, Relation rel);
static int	ri_DetermineMatchType(char *str);
static int ri_NullCheck(Relation rel, HeapTuple tup,
			 RI_QueryKey *key, int pairidx);
static void ri_BuildQueryKeyFull(RI_QueryKey *key, Oid constr_id,
					 int32 constr_queryno,
					 Relation fk_rel, Relation pk_rel,
					 int argc, char **argv);
static void ri_BuildQueryKeyPkCheck(RI_QueryKey *key, Oid constr_id,
						int32 constr_queryno,
						Relation pk_rel,
						int argc, char **argv);
static bool ri_KeysEqual(Relation rel, HeapTuple oldtup, HeapTuple newtup,
			 RI_QueryKey *key, int pairidx);
static bool ri_AllKeysUnequal(Relation rel, HeapTuple oldtup, HeapTuple newtup,
				  RI_QueryKey *key, int pairidx);
static bool ri_OneKeyEqual(Relation rel, int column, HeapTuple oldtup,
			   HeapTuple newtup, RI_QueryKey *key, int pairidx);
static bool ri_AttributesEqual(Oid typeid, Datum oldvalue, Datum newvalue);
static bool ri_Check_Pk_Match(Relation pk_rel, HeapTuple old_row,
				  Oid tgoid, int match_type, int tgnargs, char **tgargs);

static void ri_InitHashTables(void);
static void *ri_FetchPreparedPlan(RI_QueryKey *key);
static void ri_HashPreparedPlan(RI_QueryKey *key, void *plan);
static void *ri_PlanCheck(char *querystr, int nargs, Oid *argtypes,
						  RI_QueryKey *qkey, Relation fk_rel, Relation pk_rel,
						  bool cache_plan);


/* ----------
 * RI_FKey_check -
 *
 *	Check foreign key existence (combined for INSERT and UPDATE).
 * ----------
 */
static Datum
RI_FKey_check(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	new_row;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		check_values[RI_MAX_NUMKEYS];
	char		check_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	int			match_type;
	Oid			save_uid;

	save_uid = GetUserId();

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_check() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_check() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event) &&
		!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_check() must be fired for INSERT or UPDATE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_check()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_check()",
			 RI_MAX_NUMKEYS);

	/*
	 * Get the relation descriptors of the FK and PK tables and the new
	 * tuple.
	 *
	 * pk_rel is opened in RowShareLock mode since that's what our eventual
	 * SELECT FOR UPDATE will get on it.
	 *
	 * Error check here is needed because of ancient pg_dump bug; see notes
	 * in CreateTrigger().
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	pk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowShareLock);
	fk_rel = trigdata->tg_relation;
	if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
	{
		old_row = trigdata->tg_trigtuple;
		new_row = trigdata->tg_newtuple;
	}
	else
	{
		old_row = NULL;
		new_row = trigdata->tg_trigtuple;
	}

	/*
	 * We should not even consider checking the row if it is no longer
	 * valid since it was either deleted (doesn't matter) or updated (in
	 * which case it'll be checked with its final values).
	 *
	 * Note: we need not SetBufferCommitInfoNeedsSave() here since the
	 * new tuple's commit state can't possibly change.
	 */
	if (new_row)
	{
		if (!HeapTupleSatisfiesItself(new_row->t_data))
		{
			heap_close(pk_rel, RowShareLock);
			return PointerGetDatum(NULL);
		}
	}

	/* ----------
	 * SQL3 11.9 <referential constraint definition>
	 *	Gereral rules 2) a):
	 *		If Rf and Rt are empty (no columns to compare given)
	 *		constraint is true if 0 < (SELECT COUNT(*) FROM T)
	 *
	 *	Note: The special case that no columns are given cannot
	 *		occur up to now in Postgres, it's just there for
	 *		future enhancements.
	 * ----------
	 */
	if (tgnargs == 4)
	{
		ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
							 RI_PLAN_CHECK_LOOKUPPK_NOCOLS,
							 fk_rel, pk_rel,
							 tgnargs, tgargs);

		if (SPI_connect() != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect() failed in RI_FKey_check()");

		if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
		{
			char		querystr[MAX_QUOTED_REL_NAME_LEN + 100];
			char		pkrelname[MAX_QUOTED_REL_NAME_LEN];

			/* ---------
			 * The query string built is
			 *	SELECT 1 FROM ONLY <pktable>
			 * ----------
			 */
			quoteRelationName(pkrelname, pk_rel);
			snprintf(querystr, sizeof(querystr), "SELECT 1 FROM ONLY %s x FOR UPDATE OF x",
					 pkrelname);

			/* Prepare and save the plan */
			qplan = ri_PlanCheck(querystr, 0, NULL,
								 &qkey, fk_rel, pk_rel, true);
		}

		/*
		 * Execute the plan
		 */
		SetUserId(RelationGetForm(pk_rel)->relowner);

		if (SPI_execp(qplan, check_values, check_nulls, 1) != SPI_OK_SELECT)
			elog(ERROR, "SPI_execp() failed in RI_FKey_check()");

		SetUserId(save_uid);

		if (SPI_processed == 0)
			elog(ERROR, "%s referential integrity violation - "
				 "no rows found in %s",
				 tgargs[RI_CONSTRAINT_NAME_ARGNO],
				 RelationGetRelationName(pk_rel));

		if (SPI_finish() != SPI_OK_FINISH)
			elog(WARNING, "SPI_finish() failed in RI_FKey_check()");

		heap_close(pk_rel, RowShareLock);

		return PointerGetDatum(NULL);

	}

	match_type = ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]);

	if (match_type == RI_MATCH_TYPE_PARTIAL)
	{
		elog(ERROR, "MATCH PARTIAL not yet supported");
		return PointerGetDatum(NULL);
	}

	ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
						 RI_PLAN_CHECK_LOOKUPPK, fk_rel, pk_rel,
						 tgnargs, tgargs);

	switch (ri_NullCheck(fk_rel, new_row, &qkey, RI_KEYPAIR_FK_IDX))
	{
		case RI_KEYS_ALL_NULL:

			/*
			 * No check - if NULLs are allowed at all is already checked
			 * by NOT NULL constraint.
			 *
			 * This is true for MATCH FULL, MATCH PARTIAL, and MATCH
			 * <unspecified>
			 */
			heap_close(pk_rel, RowShareLock);
			return PointerGetDatum(NULL);

		case RI_KEYS_SOME_NULL:

			/*
			 * This is the only case that differs between the three kinds
			 * of MATCH.
			 */
			switch (match_type)
			{
				case RI_MATCH_TYPE_FULL:

					/*
					 * Not allowed - MATCH FULL says either all or none of
					 * the attributes can be NULLs
					 */
					elog(ERROR, "%s referential integrity violation - "
						 "MATCH FULL doesn't allow mixing of NULL "
						 "and NON-NULL key values",
						 tgargs[RI_CONSTRAINT_NAME_ARGNO]);
					heap_close(pk_rel, RowShareLock);
					return PointerGetDatum(NULL);

				case RI_MATCH_TYPE_UNSPECIFIED:

					/*
					 * MATCH <unspecified> - if ANY column is null, we
					 * have a match.
					 */
					heap_close(pk_rel, RowShareLock);
					return PointerGetDatum(NULL);

				case RI_MATCH_TYPE_PARTIAL:

					/*
					 * MATCH PARTIAL - all non-null columns must match.
					 * (not implemented, can be done by modifying the
					 * query below to only include non-null columns, or by
					 * writing a special version here)
					 */
					elog(ERROR, "MATCH PARTIAL not yet implemented");
					heap_close(pk_rel, RowShareLock);
					return PointerGetDatum(NULL);
			}

		case RI_KEYS_NONE_NULL:

			/*
			 * Have a full qualified key - continue below for all three
			 * kinds of MATCH.
			 */
			break;
	}

	/*
	 * No need to check anything if old and new references are the
	 * same on UPDATE.
	 */
	if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
	{
		if (HeapTupleHeaderGetXmin(old_row->t_data) !=
				GetCurrentTransactionId() &&
				ri_KeysEqual(fk_rel, old_row, new_row, &qkey,
						 RI_KEYPAIR_FK_IDX))
		{
			heap_close(pk_rel, RowShareLock);
			return PointerGetDatum(NULL);
		}
	}

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(WARNING, "SPI_connect() failed in RI_FKey_check()");


	/*
	 * Fetch or prepare a saved plan for the real check
	 */
	if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
	{
		char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
							(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
		char		pkrelname[MAX_QUOTED_REL_NAME_LEN];
		char		attname[MAX_QUOTED_NAME_LEN];
		const char *querysep;
		Oid			queryoids[RI_MAX_NUMKEYS];

		/* ----------
		 * The query string built is
		 *	SELECT 1 FROM ONLY <pktable> WHERE pkatt1 = $1 [AND ...]
		 * The type id's for the $ parameters are those of the
		 * corresponding FK attributes. Thus, ri_PlanCheck could
		 * eventually fail if the parser cannot identify some way
		 * how to compare these two types by '='.
		 * ----------
		 */
		quoteRelationName(pkrelname, pk_rel);
		snprintf(querystr, sizeof(querystr), "SELECT 1 FROM ONLY %s x", pkrelname);
		querysep = "WHERE";
		for (i = 0; i < qkey.nkeypairs; i++)
		{
			quoteOneName(attname,
			 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_PK_IDX]);
			snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), " %s %s = $%d",
					 querysep, attname, i + 1);
			querysep = "AND";
			queryoids[i] = SPI_gettypeid(fk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_FK_IDX]);
		}
		strcat(querystr, " FOR UPDATE OF x");

		/* Prepare and save the plan */
		qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
							 &qkey, fk_rel, pk_rel, true);
	}

	/*
	 * We have a plan now. Build up the arguments for SPI_execp() from the
	 * key values in the new FK tuple.
	 */
	for (i = 0; i < qkey.nkeypairs; i++)
	{
		/*
		 * We can implement MATCH PARTIAL by excluding this column from
		 * the query if it is null.  Simple!  Unfortunately, the
		 * referential actions aren't so I've not bothered to do so for
		 * the moment.
		 */

		check_values[i] = SPI_getbinval(new_row,
										fk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_FK_IDX],
										&isnull);
		if (isnull)
			check_nulls[i] = 'n';
		else
			check_nulls[i] = ' ';
	}
	check_nulls[i] = '\0';

	/*
	 * Now check that foreign key exists in PK table
	 */

	SetUserId(RelationGetForm(pk_rel)->relowner);

	if (SPI_execp(qplan, check_values, check_nulls, 1) != SPI_OK_SELECT)
		elog(ERROR, "SPI_execp() failed in RI_FKey_check()");

	SetUserId(save_uid);

	if (SPI_processed == 0)
		elog(ERROR, "%s referential integrity violation - "
			 "key referenced from %s not found in %s",
			 tgargs[RI_CONSTRAINT_NAME_ARGNO],
			 RelationGetRelationName(fk_rel),
			 RelationGetRelationName(pk_rel));

	if (SPI_finish() != SPI_OK_FINISH)
		elog(WARNING, "SPI_finish() failed in RI_FKey_check()");

	heap_close(pk_rel, RowShareLock);

	return PointerGetDatum(NULL);

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #1 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_check_ins -
 *
 *	Check foreign key existence at insert event on FK table.
 * ----------
 */
Datum
RI_FKey_check_ins(PG_FUNCTION_ARGS)
{
	return RI_FKey_check(fcinfo);
}


/* ----------
 * RI_FKey_check_upd -
 *
 *	Check foreign key existence at update event on FK table.
 * ----------
 */
Datum
RI_FKey_check_upd(PG_FUNCTION_ARGS)
{
	return RI_FKey_check(fcinfo);
}


/* ----------
 * ri_Check_Pk_Match
 *
 *	Check for matching value of old pk row in current state for
 * noaction triggers. Returns false if no row was found and a fk row
 * could potentially be referencing this row, true otherwise.
 * ----------
 */
static bool
ri_Check_Pk_Match(Relation pk_rel, HeapTuple old_row, Oid tgoid, int match_type, int tgnargs, char **tgargs)
{
	void	   *qplan;
	RI_QueryKey qkey;
	bool		isnull;
	Datum		check_values[RI_MAX_NUMKEYS];
	char		check_nulls[RI_MAX_NUMKEYS + 1];
	int			i;
	Oid			save_uid;
	bool		result;

	save_uid = GetUserId();

	ri_BuildQueryKeyPkCheck(&qkey, tgoid,
							RI_PLAN_CHECK_LOOKUPPK, pk_rel,
							tgnargs, tgargs);

	switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
	{
		case RI_KEYS_ALL_NULL:

			/*
			 * No check - nothing could have been referencing this row
			 * anyway.
			 */
			return true;

		case RI_KEYS_SOME_NULL:

			/*
			 * This is the only case that differs between the three kinds
			 * of MATCH.
			 */
			switch (match_type)
			{
				case RI_MATCH_TYPE_FULL:
				case RI_MATCH_TYPE_UNSPECIFIED:

					/*
					 * MATCH <unspecified>/FULL  - if ANY column is null,
					 * we can't be matching to this row already.
					 */
					return true;

				case RI_MATCH_TYPE_PARTIAL:

					/*
					 * MATCH PARTIAL - all non-null columns must match.
					 * (not implemented, can be done by modifying the
					 * query below to only include non-null columns, or by
					 * writing a special version here)
					 */
					elog(ERROR, "MATCH PARTIAL not yet implemented");
					break;
			}

		case RI_KEYS_NONE_NULL:

			/*
			 * Have a full qualified key - continue below for all three
			 * kinds of MATCH.
			 */
			break;
	}

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(WARNING, "SPI_connect() failed in RI_FKey_check()");


	/*
	 * Fetch or prepare a saved plan for the real check
	 */
	if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
	{
		char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
							(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
		char		pkrelname[MAX_QUOTED_REL_NAME_LEN];
		char		attname[MAX_QUOTED_NAME_LEN];
		const char *querysep;
		Oid			queryoids[RI_MAX_NUMKEYS];

		/* ----------
		 * The query string built is
		 *	SELECT 1 FROM ONLY <pktable> WHERE pkatt1 = $1 [AND ...]
		 * The type id's for the $ parameters are those of the
		 * corresponding FK attributes. Thus, ri_PlanCheck could
		 * eventually fail if the parser cannot identify some way
		 * how to compare these two types by '='.
		 * ----------
		 */
		quoteRelationName(pkrelname, pk_rel);
		snprintf(querystr, sizeof(querystr), "SELECT 1 FROM ONLY %s x", pkrelname);
		querysep = "WHERE";
		for (i = 0; i < qkey.nkeypairs; i++)
		{
			quoteOneName(attname,
			 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_PK_IDX]);
			snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), " %s %s = $%d",
					 querysep, attname, i + 1);
			querysep = "AND";
			queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
		}
		strcat(querystr, " FOR UPDATE OF x");

		/* Prepare and save the plan */
		qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
							 &qkey, pk_rel, pk_rel, true);
	}

	/*
	 * We have a plan now. Build up the arguments for SPI_execp() from the
	 * key values in the new FK tuple.
	 */
	for (i = 0; i < qkey.nkeypairs; i++)
	{
		check_values[i] = SPI_getbinval(old_row,
										pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
										&isnull);
		if (isnull)
			check_nulls[i] = 'n';
		else
			check_nulls[i] = ' ';
	}
	check_nulls[i] = '\0';

	/*
	 * Now check that foreign key exists in PK table
	 */

	SetUserId(RelationGetForm(pk_rel)->relowner);

	if (SPI_execp(qplan, check_values, check_nulls, 1) != SPI_OK_SELECT)
		elog(ERROR, "SPI_execp() failed in ri_Check_Pk_Match()");

	SetUserId(save_uid);

	result = (SPI_processed != 0);

	if (SPI_finish() != SPI_OK_FINISH)
		elog(WARNING, "SPI_finish() failed in ri_Check_Pk_Match()");

	return result;
}


/* ----------
 * RI_FKey_noaction_del -
 *
 *	Give an error and roll back the current transaction if the
 *	delete has resulted in a violation of the given referential
 *	integrity constraint.
 * ----------
 */
Datum
RI_FKey_noaction_del(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		del_values[RI_MAX_NUMKEYS];
	char		del_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	int			match_type;
	Oid			save_uid;

	save_uid = GetUserId();

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_noaction_del() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_noaction_del() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_noaction_del() must be fired for DELETE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_noaction_del()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_noaction_del()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the old
	 * tuple.
	 *
	 * fk_rel is opened in RowShareLock mode since that's what our eventual
	 * SELECT FOR UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowShareLock);
	pk_rel = trigdata->tg_relation;
	old_row = trigdata->tg_trigtuple;

	match_type = ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]);
	if (ri_Check_Pk_Match(pk_rel, old_row, trigdata->tg_trigger->tgoid,
						  match_type, tgnargs, tgargs))
	{
		/*
		 * There's either another row, or no row could match this one.  In
		 * either case, we don't need to do the check.
		 */
		heap_close(fk_rel, RowShareLock);
		return PointerGetDatum(NULL);
	}

	switch (match_type)
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 6) a) iv):
			 *		MATCH <unspecified> or MATCH FULL
			 *			... ON DELETE CASCADE
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_NOACTION_DEL_CHECKREF,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No check - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowShareLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_noaction_del()");

			/*
			 * Fetch or prepare a saved plan for the restrict delete
			 * lookup if foreign references exist
			 */
			if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
							(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				Oid			queryoids[RI_MAX_NUMKEYS];

				/* ----------
				 * The query string built is
				 *	SELECT 1 FROM ONLY <fktable> WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "SELECT 1 FROM ONLY %s x", fkrelname);
				querysep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), " %s %s = $%d",
							 querysep, attname, i + 1);
					querysep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, " FOR UPDATE OF x");

				/* Prepare and save the plan */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, true);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the deleted PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				del_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					del_nulls[i] = 'n';
				else
					del_nulls[i] = ' ';
			}
			del_nulls[i] = '\0';

			/*
			 * Now check for existing references
			 */
			SetUserId(RelationGetForm(pk_rel)->relowner);

			if (SPI_execp(qplan, del_values, del_nulls, 1) != SPI_OK_SELECT)
				elog(ERROR, "SPI_execp() failed in RI_FKey_noaction_del()");

			SetUserId(save_uid);

			if (SPI_processed > 0)
				elog(ERROR, "%s referential integrity violation - "
					 "key in %s still referenced from %s",
					 tgargs[RI_CONSTRAINT_NAME_ARGNO],
					 RelationGetRelationName(pk_rel),
					 RelationGetRelationName(fk_rel));

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_noaction_del()");

			heap_close(fk_rel, RowShareLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL restrict delete.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #2 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_noaction_upd -
 *
 *	Give an error and roll back the current transaction if the
 *	update has resulted in a violation of the given referential
 *	integrity constraint.
 * ----------
 */
Datum
RI_FKey_noaction_upd(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	new_row;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		upd_values[RI_MAX_NUMKEYS];
	char		upd_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	int			match_type;
	Oid			save_uid;

	save_uid = GetUserId();

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_noaction_upd() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_noaction_upd() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_noaction_upd() must be fired for UPDATE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_noaction_upd()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_noaction_upd()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the new
	 * and old tuple.
	 *
	 * fk_rel is opened in RowShareLock mode since that's what our eventual
	 * SELECT FOR UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowShareLock);
	pk_rel = trigdata->tg_relation;
	new_row = trigdata->tg_newtuple;
	old_row = trigdata->tg_trigtuple;

	match_type = ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]);
	if (ri_Check_Pk_Match(pk_rel, old_row, trigdata->tg_trigger->tgoid,
						  match_type, tgnargs, tgargs))
	{
		/*
		 * There's either another row, or no row could match this one.  In
		 * either case, we don't need to do the check.
		 */
		heap_close(fk_rel, RowShareLock);
		return PointerGetDatum(NULL);
	}

	switch (match_type)
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 6) a) iv):
			 *		MATCH <unspecified> or MATCH FULL
			 *			... ON DELETE CASCADE
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_NOACTION_UPD_CHECKREF,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No check - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowShareLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			/*
			 * No need to check anything if old and new keys are equal
			 */
			if (ri_KeysEqual(pk_rel, old_row, new_row, &qkey,
							 RI_KEYPAIR_PK_IDX))
			{
				heap_close(fk_rel, RowShareLock);
				return PointerGetDatum(NULL);
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_noaction_upd()");

			/*
			 * Fetch or prepare a saved plan for the noaction update
			 * lookup if foreign references exist
			 */
			if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
							(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				Oid			queryoids[RI_MAX_NUMKEYS];

				/* ----------
				 * The query string built is
				 *	SELECT 1 FROM ONLY <fktable> WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "SELECT 1 FROM ONLY %s x", fkrelname);
				querysep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), " %s %s = $%d",
							 querysep, attname, i + 1);
					querysep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, " FOR UPDATE OF x");

				/* Prepare and save the plan */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, true);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the updated PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				upd_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[i] = 'n';
				else
					upd_nulls[i] = ' ';
			}
			upd_nulls[i] = '\0';

			/*
			 * Now check for existing references
			 */
			SetUserId(RelationGetForm(pk_rel)->relowner);

			if (SPI_execp(qplan, upd_values, upd_nulls, 1) != SPI_OK_SELECT)
				elog(ERROR, "SPI_execp() failed in RI_FKey_noaction_upd()");

			SetUserId(save_uid);

			if (SPI_processed > 0)
				elog(ERROR, "%s referential integrity violation - "
					 "key in %s still referenced from %s",
					 tgargs[RI_CONSTRAINT_NAME_ARGNO],
					 RelationGetRelationName(pk_rel),
					 RelationGetRelationName(fk_rel));

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_noaction_upd()");

			heap_close(fk_rel, RowShareLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL noaction update.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #3 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_cascade_del -
 *
 *	Cascaded delete foreign key references at delete event on PK table.
 * ----------
 */
Datum
RI_FKey_cascade_del(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		del_values[RI_MAX_NUMKEYS];
	char		del_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_cascade_del() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_cascade_del() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_cascade_del() must be fired for DELETE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_cascade_del()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_cascade_del()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the old
	 * tuple.
	 *
	 * fk_rel is opened in RowExclusiveLock mode since that's what our
	 * eventual DELETE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowExclusiveLock);
	pk_rel = trigdata->tg_relation;
	old_row = trigdata->tg_trigtuple;
	fk_owner = RelationGetForm(fk_rel)->relowner;

	switch (ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]))
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 6) a) i):
			 *		MATCH <unspecified> or MATCH FULL
			 *			... ON DELETE CASCADE
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_CASCADE_DEL_DODELETE,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No check - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowExclusiveLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_cascade_del()");

			/*
			 * Fetch or prepare a saved plan for the cascaded delete
			 */
			if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
							(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				Oid			queryoids[RI_MAX_NUMKEYS];

				/* ----------
				 * The query string built is
				 *	DELETE FROM ONLY <fktable> WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "DELETE FROM ONLY %s", fkrelname);
				querysep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), " %s %s = $%d",
							 querysep, attname, i + 1);
					querysep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}

				/* Prepare and save the plan */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, true);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the deleted PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				del_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					del_nulls[i] = 'n';
				else
					del_nulls[i] = ' ';
			}
			del_nulls[i] = '\0';

			/*
			 * Now delete constraint
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			if (SPI_execp(qplan, del_values, del_nulls, 0) != SPI_OK_DELETE)
				elog(ERROR, "SPI_execp() failed in RI_FKey_cascade_del()");

			SetUserId(save_uid);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_cascade_del()");

			heap_close(fk_rel, RowExclusiveLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL cascaded delete.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #4 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_cascade_upd -
 *
 *	Cascaded update/delete foreign key references at update event on PK table.
 * ----------
 */
Datum
RI_FKey_cascade_upd(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	new_row;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		upd_values[RI_MAX_NUMKEYS * 2];
	char		upd_nulls[RI_MAX_NUMKEYS * 2 + 1];
	bool		isnull;
	int			i;
	int			j;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_cascade_upd() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_cascade_upd() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_cascade_upd() must be fired for UPDATE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_cascade_upd()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_cascade_upd()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the new
	 * and old tuple.
	 *
	 * fk_rel is opened in RowExclusiveLock mode since that's what our
	 * eventual UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowExclusiveLock);
	pk_rel = trigdata->tg_relation;
	new_row = trigdata->tg_newtuple;
	old_row = trigdata->tg_trigtuple;
	fk_owner = RelationGetForm(fk_rel)->relowner;

	switch (ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]))
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 7) a) i):
			 *		MATCH <unspecified> or MATCH FULL
			 *			... ON UPDATE CASCADE
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_CASCADE_UPD_DOUPDATE,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No update - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowExclusiveLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			/*
			 * No need to do anything if old and new keys are equal
			 */
			if (ri_KeysEqual(pk_rel, old_row, new_row, &qkey,
							 RI_KEYPAIR_PK_IDX))
			{
				heap_close(fk_rel, RowExclusiveLock);
				return PointerGetDatum(NULL);
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_cascade_upd()");

			/*
			 * Fetch or prepare a saved plan for the cascaded update of
			 * foreign references
			 */
			if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
												 (MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS * 2];
				char		qualstr[(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				const char *qualsep;
				Oid			queryoids[RI_MAX_NUMKEYS * 2];

				/* ----------
				 * The query string built is
				 *	UPDATE ONLY <fktable> SET fkatt1 = $1 [, ...]
				 *			WHERE fkatt1 = $n [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "UPDATE ONLY %s SET", fkrelname);
				qualstr[0] = '\0';
				querysep = "";
				qualsep = "WHERE";
				for (i = 0, j = qkey.nkeypairs; i < qkey.nkeypairs; i++, j++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), "%s %s = $%d",
							 querysep, attname, i + 1);
					snprintf(qualstr + strlen(qualstr), sizeof(qualstr) - strlen(qualstr), " %s %s = $%d",
							 qualsep, attname, j + 1);
					querysep = ",";
					qualsep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
					queryoids[j] = queryoids[i];
				}
				strcat(querystr, qualstr);

				/* Prepare and save the plan */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs * 2, queryoids,
									 &qkey, fk_rel, pk_rel, true);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the updated PK tuple.
			 */
			for (i = 0, j = qkey.nkeypairs; i < qkey.nkeypairs; i++, j++)
			{
				upd_values[i] = SPI_getbinval(new_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[i] = 'n';
				else
					upd_nulls[i] = ' ';

				upd_values[j] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[j] = 'n';
				else
					upd_nulls[j] = ' ';
			}
			upd_nulls[j] = '\0';

			/*
			 * Now update the existing references
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			if (SPI_execp(qplan, upd_values, upd_nulls, 0) != SPI_OK_UPDATE)
				elog(ERROR, "SPI_execp() failed in RI_FKey_cascade_upd()");

			SetUserId(save_uid);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_cascade_upd()");

			heap_close(fk_rel, RowExclusiveLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL cascade update.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #5 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_restrict_del -
 *
 *	Restrict delete from PK table to rows unreferenced by foreign key.
 *
 *	SQL3 intends that this referential action occur BEFORE the
 *	update is performed, rather than after.  This appears to be
 *	the only difference between "NO ACTION" and "RESTRICT".
 *
 *	For now, however, we treat "RESTRICT" and "NO ACTION" as
 *	equivalent.
 * ----------
 */
Datum
RI_FKey_restrict_del(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		del_values[RI_MAX_NUMKEYS];
	char		del_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_restrict_del() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_restrict_del() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_restrict_del() must be fired for DELETE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_restrict_del()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_restrict_del()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the old
	 * tuple.
	 *
	 * fk_rel is opened in RowShareLock mode since that's what our eventual
	 * SELECT FOR UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowShareLock);
	pk_rel = trigdata->tg_relation;
	old_row = trigdata->tg_trigtuple;
	fk_owner = RelationGetForm(fk_rel)->relowner;

	switch (ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]))
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 6) a) iv):
			 *		MATCH <unspecified> or MATCH FULL
			 *			... ON DELETE CASCADE
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_RESTRICT_DEL_CHECKREF,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No check - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowShareLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_restrict_del()");

			/*
			 * Fetch or prepare a saved plan for the restrict delete
			 * lookup if foreign references exist
			 */
			if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
							(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				Oid			queryoids[RI_MAX_NUMKEYS];

				/* ----------
				 * The query string built is
				 *	SELECT 1 FROM ONLY <fktable> WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "SELECT 1 FROM ONLY %s x", fkrelname);
				querysep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), " %s %s = $%d",
							 querysep, attname, i + 1);
					querysep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, " FOR UPDATE OF x");

				/* Prepare and save the plan */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, true);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the deleted PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				del_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					del_nulls[i] = 'n';
				else
					del_nulls[i] = ' ';
			}
			del_nulls[i] = '\0';

			/*
			 * Now check for existing references
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			if (SPI_execp(qplan, del_values, del_nulls, 1) != SPI_OK_SELECT)
				elog(ERROR, "SPI_execp() failed in RI_FKey_restrict_del()");

			SetUserId(save_uid);

			if (SPI_processed > 0)
				elog(ERROR, "%s referential integrity violation - "
					 "key in %s still referenced from %s",
					 tgargs[RI_CONSTRAINT_NAME_ARGNO],
					 RelationGetRelationName(pk_rel),
					 RelationGetRelationName(fk_rel));

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_restrict_del()");

			heap_close(fk_rel, RowShareLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL restrict delete.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #6 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_restrict_upd -
 *
 *	Restrict update of PK to rows unreferenced by foreign key.
 *
 *	SQL3 intends that this referential action occur BEFORE the
 *	update is performed, rather than after.  This appears to be
 *	the only difference between "NO ACTION" and "RESTRICT".
 *
 *	For now, however, we treat "RESTRICT" and "NO ACTION" as
 *	equivalent.
 * ----------
 */
Datum
RI_FKey_restrict_upd(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	new_row;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		upd_values[RI_MAX_NUMKEYS];
	char		upd_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_restrict_upd() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_restrict_upd() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_restrict_upd() must be fired for UPDATE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_restrict_upd()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_restrict_upd()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the new
	 * and old tuple.
	 *
	 * fk_rel is opened in RowShareLock mode since that's what our eventual
	 * SELECT FOR UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowShareLock);
	pk_rel = trigdata->tg_relation;
	new_row = trigdata->tg_newtuple;
	old_row = trigdata->tg_trigtuple;
	fk_owner = RelationGetForm(fk_rel)->relowner;

	switch (ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]))
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 6) a) iv):
			 *		MATCH <unspecified> or MATCH FULL
			 *			... ON DELETE CASCADE
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_RESTRICT_UPD_CHECKREF,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No check - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowShareLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			/*
			 * No need to check anything if old and new keys are equal
			 */
			if (ri_KeysEqual(pk_rel, old_row, new_row, &qkey,
							 RI_KEYPAIR_PK_IDX))
			{
				heap_close(fk_rel, RowShareLock);
				return PointerGetDatum(NULL);
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_restrict_upd()");

			/*
			 * Fetch or prepare a saved plan for the restrict update
			 * lookup if foreign references exist
			 */
			if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
							(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				Oid			queryoids[RI_MAX_NUMKEYS];

				/* ----------
				 * The query string built is
				 *	SELECT 1 FROM ONLY <fktable> WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "SELECT 1 FROM ONLY %s x", fkrelname);
				querysep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), " %s %s = $%d",
							 querysep, attname, i + 1);
					querysep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, " FOR UPDATE OF x");

				/* Prepare and save the plan */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, true);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the updated PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				upd_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[i] = 'n';
				else
					upd_nulls[i] = ' ';
			}
			upd_nulls[i] = '\0';

			/*
			 * Now check for existing references
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			SetUserId(RelationGetForm(pk_rel)->relowner);

			if (SPI_execp(qplan, upd_values, upd_nulls, 1) != SPI_OK_SELECT)
				elog(ERROR, "SPI_execp() failed in RI_FKey_restrict_upd()");

			SetUserId(save_uid);

			if (SPI_processed > 0)
				elog(ERROR, "%s referential integrity violation - "
					 "key in %s still referenced from %s",
					 tgargs[RI_CONSTRAINT_NAME_ARGNO],
					 RelationGetRelationName(pk_rel),
					 RelationGetRelationName(fk_rel));

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_restrict_upd()");

			heap_close(fk_rel, RowShareLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL restrict update.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #7 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_setnull_del -
 *
 *	Set foreign key references to NULL values at delete event on PK table.
 * ----------
 */
Datum
RI_FKey_setnull_del(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		upd_values[RI_MAX_NUMKEYS];
	char		upd_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_setnull_del() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setnull_del() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setnull_del() must be fired for DELETE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_setnull_del()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_setnull_del()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the old
	 * tuple.
	 *
	 * fk_rel is opened in RowExclusiveLock mode since that's what our
	 * eventual UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowExclusiveLock);
	pk_rel = trigdata->tg_relation;
	old_row = trigdata->tg_trigtuple;
	fk_owner = RelationGetForm(fk_rel)->relowner;

	switch (ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]))
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 6) a) ii):
			 *		MATCH <UNSPECIFIED> or MATCH FULL
			 *			... ON DELETE SET NULL
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_SETNULL_DEL_DOUPDATE,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No update - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowExclusiveLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_setnull_del()");

			/*
			 * Fetch or prepare a saved plan for the set null delete
			 * operation
			 */
			if ((qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
												 (MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS * 2];
				char		qualstr[(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				const char *qualsep;
				Oid			queryoids[RI_MAX_NUMKEYS];

				/* ----------
				 * The query string built is
				 *	UPDATE ONLY <fktable> SET fkatt1 = NULL [, ...]
				 *			WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "UPDATE ONLY %s SET", fkrelname);
				qualstr[0] = '\0';
				querysep = "";
				qualsep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), "%s %s = NULL",
							 querysep, attname);
					snprintf(qualstr + strlen(qualstr), sizeof(qualstr) - strlen(qualstr), " %s %s = $%d",
							 qualsep, attname, i + 1);
					querysep = ",";
					qualsep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, qualstr);

				/* Prepare and save the plan */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, true);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the updated PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				upd_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[i] = 'n';
				else
					upd_nulls[i] = ' ';
			}
			upd_nulls[i] = '\0';

			/*
			 * Now update the existing references
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			if (SPI_execp(qplan, upd_values, upd_nulls, 0) != SPI_OK_UPDATE)
				elog(ERROR, "SPI_execp() failed in RI_FKey_setnull_del()");

			SetUserId(save_uid);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_setnull_del()");

			heap_close(fk_rel, RowExclusiveLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL set null delete.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #8 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_setnull_upd -
 *
 *	Set foreign key references to NULL at update event on PK table.
 * ----------
 */
Datum
RI_FKey_setnull_upd(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	new_row;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		upd_values[RI_MAX_NUMKEYS];
	char		upd_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	int			match_type;
	bool		use_cached_query;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_setnull_upd() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setnull_upd() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setnull_upd() must be fired for UPDATE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_setnull_upd()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_setnull_upd()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the old
	 * tuple.
	 *
	 * fk_rel is opened in RowExclusiveLock mode since that's what our
	 * eventual UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowExclusiveLock);
	pk_rel = trigdata->tg_relation;
	new_row = trigdata->tg_newtuple;
	old_row = trigdata->tg_trigtuple;
	match_type = ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]);
	fk_owner = RelationGetForm(fk_rel)->relowner;

	switch (match_type)
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 7) a) ii) 2):
			 *		MATCH FULL
			 *			... ON UPDATE SET NULL
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_SETNULL_UPD_DOUPDATE,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No update - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowExclusiveLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			/*
			 * No need to do anything if old and new keys are equal
			 */
			if (ri_KeysEqual(pk_rel, old_row, new_row, &qkey,
							 RI_KEYPAIR_PK_IDX))
			{
				heap_close(fk_rel, RowExclusiveLock);
				return PointerGetDatum(NULL);
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_setnull_upd()");

			/*
			 * "MATCH <unspecified>" only changes columns corresponding to
			 * the referenced columns that have changed in pk_rel.	This
			 * means the "SET attrn=NULL [, attrn=NULL]" string will be
			 * change as well.	In this case, we need to build a temporary
			 * plan rather than use our cached plan, unless the update
			 * happens to change all columns in the key.  Fortunately, for
			 * the most common case of a single-column foreign key, this
			 * will be true.
			 *
			 * In case you're wondering, the inequality check works because
			 * we know that the old key value has no NULLs (see above).
			 */

			use_cached_query = match_type == RI_MATCH_TYPE_FULL ||
				ri_AllKeysUnequal(pk_rel, old_row, new_row,
								  &qkey, RI_KEYPAIR_PK_IDX);

			/*
			 * Fetch or prepare a saved plan for the set null update
			 * operation if possible, or build a temporary plan if not.
			 */
			if (!use_cached_query ||
				(qplan = ri_FetchPreparedPlan(&qkey)) == NULL)
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
												 (MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS * 2];
				char		qualstr[(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				const char *qualsep;
				Oid			queryoids[RI_MAX_NUMKEYS];

				/* ----------
				 * The query string built is
				 *	UPDATE ONLY <fktable> SET fkatt1 = NULL [, ...]
				 *			WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "UPDATE ONLY %s SET", fkrelname);
				qualstr[0] = '\0';
				querysep = "";
				qualsep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);

					/*
					 * MATCH <unspecified> - only change columns
					 * corresponding to changed columns in pk_rel's key
					 */
					if (match_type == RI_MATCH_TYPE_FULL ||
					  !ri_OneKeyEqual(pk_rel, i, old_row, new_row, &qkey,
									  RI_KEYPAIR_PK_IDX))
					{
						snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), "%s %s = NULL",
								 querysep, attname);
						querysep = ",";
					}
					snprintf(qualstr + strlen(qualstr), sizeof(qualstr) - strlen(qualstr), " %s %s = $%d",
							 qualsep, attname, i + 1);
					qualsep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, qualstr);

				/*
				 * Prepare the plan.  Save it only if we're building the
				 * "standard" plan.
				 */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel,
									 use_cached_query);
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the updated PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				upd_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[i] = 'n';
				else
					upd_nulls[i] = ' ';
			}
			upd_nulls[i] = '\0';

			/*
			 * Now update the existing references
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			if (SPI_execp(qplan, upd_values, upd_nulls, 0) != SPI_OK_UPDATE)
				elog(ERROR, "SPI_execp() failed in RI_FKey_setnull_upd()");

			SetUserId(save_uid);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_setnull_upd()");

			heap_close(fk_rel, RowExclusiveLock);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL set null update.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #9 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_setdefault_del -
 *
 *	Set foreign key references to defaults at delete event on PK table.
 * ----------
 */
Datum
RI_FKey_setdefault_del(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		upd_values[RI_MAX_NUMKEYS];
	char		upd_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_setdefault_del() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setdefault_del() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setdefault_del() must be fired for DELETE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_setdefault_del()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_setdefault_del()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the old
	 * tuple.
	 *
	 * fk_rel is opened in RowExclusiveLock mode since that's what our
	 * eventual UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowExclusiveLock);
	pk_rel = trigdata->tg_relation;
	old_row = trigdata->tg_trigtuple;
	fk_owner = RelationGetForm(fk_rel)->relowner;

	switch (ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]))
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 6) a) iii):
			 *		MATCH <UNSPECIFIED> or MATCH FULL
			 *			... ON DELETE SET DEFAULT
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_SETNULL_DEL_DOUPDATE,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No update - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowExclusiveLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_setdefault_del()");

			/*
			 * Prepare a plan for the set default delete operation.
			 * Unfortunately we need to do it on every invocation because
			 * the default value could potentially change between calls.
			 */
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
												 (MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS * 2];
				char		qualstr[(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				const char *qualsep;
				Oid			queryoids[RI_MAX_NUMKEYS];
				Plan	   *spi_plan;
				int			i;
				List	   *l;

				/* ----------
				 * The query string built is
				 *	UPDATE ONLY <fktable> SET fkatt1 = NULL [, ...]
				 *			WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "UPDATE ONLY %s SET", fkrelname);
				qualstr[0] = '\0';
				querysep = "";
				qualsep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);
					snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), "%s %s = NULL",
							 querysep, attname);
					snprintf(qualstr + strlen(qualstr), sizeof(qualstr) - strlen(qualstr), " %s %s = $%d",
							 qualsep, attname, i + 1);
					querysep = ",";
					qualsep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, qualstr);

				/* Prepare the plan, don't save it */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, false);

				/*
				 * Scan the plan's targetlist and replace the NULLs by
				 * appropriate column defaults, if any (if not, they stay
				 * NULL).
				 *
				 * XXX  This is really ugly; it'd be better to use "UPDATE
				 * SET foo = DEFAULT", if we had it.
				 */
				spi_plan = (Plan *) lfirst(((_SPI_plan *) qplan)->ptlist);
				foreach(l, spi_plan->targetlist)
				{
					TargetEntry *tle = (TargetEntry *) lfirst(l);
					Node *dfl;

					/* Ignore any junk columns or Var=Var columns */
					if (tle->resdom->resjunk)
						continue;
					if (IsA(tle->expr, Var))
						continue;

					dfl = build_column_default(fk_rel, tle->resdom->resno);
					if (dfl)
					{
						fix_opids(dfl);
						tle->expr = dfl;
					}
				}
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the deleted PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				upd_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[i] = 'n';
				else
					upd_nulls[i] = ' ';
			}
			upd_nulls[i] = '\0';

			/*
			 * Now update the existing references
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			if (SPI_execp(qplan, upd_values, upd_nulls, 0) != SPI_OK_UPDATE)
				elog(ERROR, "SPI_execp() failed in RI_FKey_setdefault_del()");

			SetUserId(save_uid);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_setdefault_del()");

			heap_close(fk_rel, RowExclusiveLock);

			/*
			 * In the case we delete the row who's key is equal to the
			 * default values AND a referencing row in the foreign key
			 * table exists, we would just have updated it to the same
			 * values. We need to do another lookup now and in case a
			 * reference exists, abort the operation. That is already
			 * implemented in the NO ACTION trigger.
			 */
			RI_FKey_noaction_del(fcinfo);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL set null delete.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #10 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_setdefault_upd -
 *
 *	Set foreign key references to defaults at update event on PK table.
 * ----------
 */
Datum
RI_FKey_setdefault_upd(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	new_row;
	HeapTuple	old_row;
	RI_QueryKey qkey;
	void	   *qplan;
	Datum		upd_values[RI_MAX_NUMKEYS];
	char		upd_nulls[RI_MAX_NUMKEYS + 1];
	bool		isnull;
	int			i;
	int			match_type;
	Oid			save_uid;
	Oid			fk_owner;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "RI_FKey_setdefault_upd() not fired by trigger manager");
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setdefault_upd() must be fired AFTER ROW");
	if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		elog(ERROR, "RI_FKey_setdefault_upd() must be fired for UPDATE");

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_setdefault_upd()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_setdefault_upd()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return PointerGetDatum(NULL);

	/*
	 * Get the relation descriptors of the FK and PK tables and the old
	 * tuple.
	 *
	 * fk_rel is opened in RowExclusiveLock mode since that's what our
	 * eventual UPDATE will get on it.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, RowExclusiveLock);
	pk_rel = trigdata->tg_relation;
	new_row = trigdata->tg_newtuple;
	old_row = trigdata->tg_trigtuple;
	fk_owner = RelationGetForm(fk_rel)->relowner;

	match_type = ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]);

	switch (match_type)
	{
			/* ----------
			 * SQL3 11.9 <referential constraint definition>
			 *	Gereral rules 7) a) iii):
			 *		MATCH <UNSPECIFIED> or MATCH FULL
			 *			... ON UPDATE SET DEFAULT
			 * ----------
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_SETNULL_DEL_DOUPDATE,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			switch (ri_NullCheck(pk_rel, old_row, &qkey, RI_KEYPAIR_PK_IDX))
			{
				case RI_KEYS_ALL_NULL:
				case RI_KEYS_SOME_NULL:

					/*
					 * No update - MATCH FULL means there cannot be any
					 * reference to old key if it contains NULL
					 */
					heap_close(fk_rel, RowExclusiveLock);
					return PointerGetDatum(NULL);

				case RI_KEYS_NONE_NULL:

					/*
					 * Have a full qualified key - continue below
					 */
					break;
			}

			/*
			 * No need to do anything if old and new keys are equal
			 */
			if (ri_KeysEqual(pk_rel, old_row, new_row, &qkey,
							 RI_KEYPAIR_PK_IDX))
			{
				heap_close(fk_rel, RowExclusiveLock);
				return PointerGetDatum(NULL);
			}

			if (SPI_connect() != SPI_OK_CONNECT)
				elog(WARNING, "SPI_connect() failed in RI_FKey_setdefault_upd()");

			/*
			 * Prepare a plan for the set default delete operation.
			 * Unfortunately we need to do it on every invocation because
			 * the default value could potentially change between calls.
			 */
			{
				char		querystr[MAX_QUOTED_REL_NAME_LEN + 100 +
												 (MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS * 2];
				char		qualstr[(MAX_QUOTED_NAME_LEN + 32) * RI_MAX_NUMKEYS];
				char		fkrelname[MAX_QUOTED_REL_NAME_LEN];
				char		attname[MAX_QUOTED_NAME_LEN];
				const char *querysep;
				const char *qualsep;
				Oid			queryoids[RI_MAX_NUMKEYS];
				Plan	   *spi_plan;
				int			i;
				List	   *l;

				/* ----------
				 * The query string built is
				 *	UPDATE ONLY <fktable> SET fkatt1 = NULL [, ...]
				 *			WHERE fkatt1 = $1 [AND ...]
				 * The type id's for the $ parameters are those of the
				 * corresponding PK attributes. Thus, ri_PlanCheck could
				 * eventually fail if the parser cannot identify some way
				 * how to compare these two types by '='.
				 * ----------
				 */
				quoteRelationName(fkrelname, fk_rel);
				snprintf(querystr, sizeof(querystr), "UPDATE ONLY %s SET", fkrelname);
				qualstr[0] = '\0';
				querysep = "";
				qualsep = "WHERE";
				for (i = 0; i < qkey.nkeypairs; i++)
				{
					quoteOneName(attname,
								 tgargs[RI_FIRST_ATTNAME_ARGNO + i * 2 + RI_KEYPAIR_FK_IDX]);

					/*
					 * MATCH <unspecified> - only change columns
					 * corresponding to changed columns in pk_rel's key
					 */
					if (match_type == RI_MATCH_TYPE_FULL ||
						!ri_OneKeyEqual(pk_rel, i, old_row,
									  new_row, &qkey, RI_KEYPAIR_PK_IDX))
					{
						snprintf(querystr + strlen(querystr), sizeof(querystr) - strlen(querystr), "%s %s = NULL",
								 querysep, attname);
						querysep = ",";
					}
					snprintf(qualstr + strlen(qualstr), sizeof(qualstr) - strlen(qualstr), " %s %s = $%d",
							 qualsep, attname, i + 1);
					qualsep = "AND";
					queryoids[i] = SPI_gettypeid(pk_rel->rd_att,
									 qkey.keypair[i][RI_KEYPAIR_PK_IDX]);
				}
				strcat(querystr, qualstr);

				/* Prepare the plan, don't save it */
				qplan = ri_PlanCheck(querystr, qkey.nkeypairs, queryoids,
									 &qkey, fk_rel, pk_rel, false);

				/*
				 * Scan the plan's targetlist and replace the NULLs by
				 * appropriate column defaults, if any (if not, they stay
				 * NULL).
				 *
				 * XXX  This is really ugly; it'd be better to use "UPDATE
				 * SET foo = DEFAULT", if we had it.
				 */
				spi_plan = (Plan *) lfirst(((_SPI_plan *) qplan)->ptlist);
				foreach(l, spi_plan->targetlist)
				{
					TargetEntry *tle = (TargetEntry *) lfirst(l);
					Node *dfl;

					/* Ignore any junk columns or Var=Var columns */
					if (tle->resdom->resjunk)
						continue;
					if (IsA(tle->expr, Var))
						continue;

					dfl = build_column_default(fk_rel, tle->resdom->resno);
					if (dfl)
					{
						fix_opids(dfl);
						tle->expr = dfl;
					}
				}
			}

			/*
			 * We have a plan now. Build up the arguments for SPI_execp()
			 * from the key values in the deleted PK tuple.
			 */
			for (i = 0; i < qkey.nkeypairs; i++)
			{
				upd_values[i] = SPI_getbinval(old_row,
											  pk_rel->rd_att,
									  qkey.keypair[i][RI_KEYPAIR_PK_IDX],
											  &isnull);
				if (isnull)
					upd_nulls[i] = 'n';
				else
					upd_nulls[i] = ' ';
			}
			upd_nulls[i] = '\0';

			/*
			 * Now update the existing references
			 */
			save_uid = GetUserId();
			SetUserId(fk_owner);

			if (SPI_execp(qplan, upd_values, upd_nulls, 0) != SPI_OK_UPDATE)
				elog(ERROR, "SPI_execp() failed in RI_FKey_setdefault_upd()");

			SetUserId(save_uid);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(WARNING, "SPI_finish() failed in RI_FKey_setdefault_upd()");

			heap_close(fk_rel, RowExclusiveLock);

			/*
			 * In the case we updated the row who's key was equal to the
			 * default values AND a referencing row in the foreign key
			 * table exists, we would just have updated it to the same
			 * values. We need to do another lookup now and in case a
			 * reference exists, abort the operation. That is already
			 * implemented in the NO ACTION trigger.
			 */
			RI_FKey_noaction_upd(fcinfo);

			return PointerGetDatum(NULL);

			/*
			 * Handle MATCH PARTIAL set null delete.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			return PointerGetDatum(NULL);
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #11 in ri_triggers.c");
	return PointerGetDatum(NULL);
}


/* ----------
 * RI_FKey_keyequal_upd -
 *
 *	Check if we have a key change on update.
 *
 *	This is not a real trigger procedure. It is used by the deferred
 *	trigger queue manager to detect "triggered data change violation".
 * ----------
 */
bool
RI_FKey_keyequal_upd(TriggerData *trigdata)
{
	int			tgnargs;
	char	  **tgargs;
	Relation	fk_rel;
	Relation	pk_rel;
	HeapTuple	new_row;
	HeapTuple	old_row;
	RI_QueryKey qkey;

	/*
	 * Check for the correct # of call arguments
	 */
	tgnargs = trigdata->tg_trigger->tgnargs;
	tgargs = trigdata->tg_trigger->tgargs;
	if (tgnargs < 4 || (tgnargs % 2) != 0)
		elog(ERROR, "wrong # of arguments in call to RI_FKey_keyequal_upd()");
	if (tgnargs > RI_MAX_ARGUMENTS)
		elog(ERROR, "too many keys (%d max) in call to RI_FKey_keyequal_upd()",
			 RI_MAX_NUMKEYS);

	/*
	 * Nothing to do if no column names to compare given
	 */
	if (tgnargs == 4)
		return true;

	/*
	 * Get the relation descriptors of the FK and PK tables and the new
	 * and old tuple.
	 *
	 * Use minimal locking for fk_rel here.
	 */
	if (!OidIsValid(trigdata->tg_trigger->tgconstrrelid))
		elog(ERROR, "No target table given for trigger \"%s\" on \"%s\""
			 "\n\tRemove these RI triggers and do ALTER TABLE ADD CONSTRAINT",
			 trigdata->tg_trigger->tgname,
			 RelationGetRelationName(trigdata->tg_relation));

	fk_rel = heap_open(trigdata->tg_trigger->tgconstrrelid, AccessShareLock);
	pk_rel = trigdata->tg_relation;
	new_row = trigdata->tg_newtuple;
	old_row = trigdata->tg_trigtuple;

	switch (ri_DetermineMatchType(tgargs[RI_MATCH_TYPE_ARGNO]))
	{
			/*
			 * MATCH <UNSPECIFIED>
			 */
		case RI_MATCH_TYPE_UNSPECIFIED:
		case RI_MATCH_TYPE_FULL:
			ri_BuildQueryKeyFull(&qkey, trigdata->tg_trigger->tgoid,
								 RI_PLAN_KEYEQUAL_UPD,
								 fk_rel, pk_rel,
								 tgnargs, tgargs);

			heap_close(fk_rel, AccessShareLock);

			/*
			 * Return if key's are equal
			 */
			return ri_KeysEqual(pk_rel, old_row, new_row, &qkey,
								RI_KEYPAIR_PK_IDX);

			/*
			 * Handle MATCH PARTIAL set null delete.
			 */
		case RI_MATCH_TYPE_PARTIAL:
			elog(ERROR, "MATCH PARTIAL not yet supported");
			break;
	}

	/*
	 * Never reached
	 */
	elog(ERROR, "internal error #12 in ri_triggers.c");
	return false;
}





/* ----------
 * Local functions below
 * ----------
 */


/*
 * Prepare execution plan for a query to enforce an RI restriction
 *
 * If cache_plan is true, the plan is saved into our plan hashtable
 * so that we don't need to plan it again.
 */
static void *
ri_PlanCheck(char *querystr, int nargs, Oid *argtypes,
			 RI_QueryKey *qkey, Relation fk_rel, Relation pk_rel,
			 bool cache_plan)
{
	void	   *qplan;
	Relation	query_rel;
	Oid			save_uid;

	/*
	 * The query is always run against the FK table except
	 * when this is an update/insert trigger on the FK table itself -
	 * either RI_PLAN_CHECK_LOOKUPPK or RI_PLAN_CHECK_LOOKUPPK_NOCOLS
	 */
	if (qkey->constr_queryno == RI_PLAN_CHECK_LOOKUPPK ||
		qkey->constr_queryno == RI_PLAN_CHECK_LOOKUPPK_NOCOLS)
		query_rel = pk_rel;
	else
		query_rel = fk_rel;

	/* Switch to proper UID to perform check as */
	save_uid = GetUserId();
	SetUserId(RelationGetForm(query_rel)->relowner);

	/* Create the plan */
	qplan = SPI_prepare(querystr, nargs, argtypes);

	/* Restore UID */
	SetUserId(save_uid);

	/* Save the plan if requested */
	if (cache_plan)
	{
		qplan = SPI_saveplan(qplan);
		ri_HashPreparedPlan(qkey, qplan);
	}

	return qplan;
}

/*
 * quoteOneName --- safely quote a single SQL name
 *
 * buffer must be MAX_QUOTED_NAME_LEN long (includes room for \0)
 */
static void
quoteOneName(char *buffer, const char *name)
{
	/* Rather than trying to be smart, just always quote it. */
	*buffer++ = '"';
	while (*name)
	{
		if (*name == '"')
			*buffer++ = '"';
		*buffer++ = *name++;
	}
	*buffer++ = '"';
	*buffer = '\0';
}

/*
 * quoteRelationName --- safely quote a fully qualified relation name
 *
 * buffer must be MAX_QUOTED_REL_NAME_LEN long (includes room for \0)
 */
static void
quoteRelationName(char *buffer, Relation rel)
{
	quoteOneName(buffer, get_namespace_name(RelationGetNamespace(rel)));
	buffer += strlen(buffer);
	*buffer++ = '.';
	quoteOneName(buffer, RelationGetRelationName(rel));
}


/* ----------
 * ri_DetermineMatchType -
 *
 *	Convert the MATCH TYPE string into a switchable int
 * ----------
 */
static int
ri_DetermineMatchType(char *str)
{
	if (strcmp(str, "UNSPECIFIED") == 0)
		return RI_MATCH_TYPE_UNSPECIFIED;
	if (strcmp(str, "FULL") == 0)
		return RI_MATCH_TYPE_FULL;
	if (strcmp(str, "PARTIAL") == 0)
		return RI_MATCH_TYPE_PARTIAL;

	elog(ERROR, "unrecognized referential integrity MATCH type '%s'", str);
	return 0;
}


/* ----------
 * ri_BuildQueryKeyFull -
 *
 *	Build up a new hashtable key for a prepared SPI plan of a
 *	constraint trigger of MATCH FULL. The key consists of:
 *
 *		constr_type is FULL
 *		constr_id is the OID of the pg_trigger row that invoked us
 *		constr_queryno is an internal number of the query inside the proc
 *		fk_relid is the OID of referencing relation
 *		pk_relid is the OID of referenced relation
 *		nkeypairs is the number of keypairs
 *		following are the attribute number keypairs of the trigger invocation
 *
 *	At least for MATCH FULL this builds a unique key per plan.
 * ----------
 */
static void
ri_BuildQueryKeyFull(RI_QueryKey *key, Oid constr_id, int32 constr_queryno,
					 Relation fk_rel, Relation pk_rel,
					 int argc, char **argv)
{
	int			i;
	int			j;
	int			fno;

	/*
	 * Initialize the key and fill in type, oid's and number of keypairs
	 */
	memset((void *) key, 0, sizeof(RI_QueryKey));
	key->constr_type = RI_MATCH_TYPE_FULL;
	key->constr_id = constr_id;
	key->constr_queryno = constr_queryno;
	key->fk_relid = fk_rel->rd_id;
	key->pk_relid = pk_rel->rd_id;
	key->nkeypairs = (argc - RI_FIRST_ATTNAME_ARGNO) / 2;

	/*
	 * Lookup the attribute numbers of the arguments to the trigger call
	 * and fill in the keypairs.
	 */
	for (i = 0, j = RI_FIRST_ATTNAME_ARGNO; j < argc; i++, j += 2)
	{
		fno = SPI_fnumber(fk_rel->rd_att, argv[j]);
		if (fno == SPI_ERROR_NOATTRIBUTE)
			elog(ERROR, "constraint %s: table %s does not have an attribute %s",
				 argv[RI_CONSTRAINT_NAME_ARGNO],
				 RelationGetRelationName(fk_rel),
				 argv[j]);
		key->keypair[i][RI_KEYPAIR_FK_IDX] = fno;

		fno = SPI_fnumber(pk_rel->rd_att, argv[j + 1]);
		if (fno == SPI_ERROR_NOATTRIBUTE)
			elog(ERROR, "constraint %s: table %s does not have an attribute %s",
				 argv[RI_CONSTRAINT_NAME_ARGNO],
				 RelationGetRelationName(pk_rel),
				 argv[j + 1]);
		key->keypair[i][RI_KEYPAIR_PK_IDX] = fno;
	}
}

/* ----------
 * ri_BuildQueryKeyPkCheck -
 *
 *	Build up a new hashtable key for a prepared SPI plan of a
 *	check for PK rows in noaction triggers.
 *
 *		constr_type is FULL
 *		constr_id is the OID of the pg_trigger row that invoked us
 *		constr_queryno is an internal number of the query inside the proc
 *		pk_relid is the OID of referenced relation
 *		nkeypairs is the number of keypairs
 *		following are the attribute number keypairs of the trigger invocation
 *
 *	At least for MATCH FULL this builds a unique key per plan.
 * ----------
 */
static void
ri_BuildQueryKeyPkCheck(RI_QueryKey *key, Oid constr_id, int32 constr_queryno,
						Relation pk_rel,
						int argc, char **argv)
{
	int			i;
	int			j;
	int			fno;

	/*
	 * Initialize the key and fill in type, oid's and number of keypairs
	 */
	memset((void *) key, 0, sizeof(RI_QueryKey));
	key->constr_type = RI_MATCH_TYPE_FULL;
	key->constr_id = constr_id;
	key->constr_queryno = constr_queryno;
	key->fk_relid = 0;
	key->pk_relid = pk_rel->rd_id;
	key->nkeypairs = (argc - RI_FIRST_ATTNAME_ARGNO) / 2;

	/*
	 * Lookup the attribute numbers of the arguments to the trigger call
	 * and fill in the keypairs.
	 */
	for (i = 0, j = RI_FIRST_ATTNAME_ARGNO + RI_KEYPAIR_PK_IDX; j < argc; i++, j += 2)
	{
		fno = SPI_fnumber(pk_rel->rd_att, argv[j]);
		if (fno == SPI_ERROR_NOATTRIBUTE)
			elog(ERROR, "constraint %s: table %s does not have an attribute %s",
				 argv[RI_CONSTRAINT_NAME_ARGNO],
				 RelationGetRelationName(pk_rel),
				 argv[j + 1]);
		key->keypair[i][RI_KEYPAIR_PK_IDX] = fno;
		key->keypair[i][RI_KEYPAIR_FK_IDX] = 0;
	}
}


/* ----------
 * ri_NullCheck -
 *
 *	Determine the NULL state of all key values in a tuple
 *
 *	Returns one of RI_KEYS_ALL_NULL, RI_KEYS_NONE_NULL or RI_KEYS_SOME_NULL.
 * ----------
 */
static int
ri_NullCheck(Relation rel, HeapTuple tup, RI_QueryKey *key, int pairidx)
{
	int			i;
	bool		isnull;
	bool		allnull = true;
	bool		nonenull = true;

	for (i = 0; i < key->nkeypairs; i++)
	{
		isnull = false;
		SPI_getbinval(tup, rel->rd_att, key->keypair[i][pairidx], &isnull);
		if (isnull)
			nonenull = false;
		else
			allnull = false;
	}

	if (allnull)
		return RI_KEYS_ALL_NULL;

	if (nonenull)
		return RI_KEYS_NONE_NULL;

	return RI_KEYS_SOME_NULL;
}


/* ----------
 * ri_InitHashTables -
 *
 *	Initialize our internal hash tables for prepared
 *	query plans and equal operators.
 * ----------
 */
static void
ri_InitHashTables(void)
{
	HASHCTL		ctl;

	memset(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(RI_QueryKey);
	ctl.entrysize = sizeof(RI_QueryHashEntry);
	ctl.hash = tag_hash;
	ri_query_cache = hash_create("RI query cache", RI_INIT_QUERYHASHSIZE,
								 &ctl, HASH_ELEM | HASH_FUNCTION);

	ctl.keysize = sizeof(Oid);
	ctl.entrysize = sizeof(RI_OpreqHashEntry);
	ctl.hash = tag_hash;
	ri_opreq_cache = hash_create("RI OpReq cache", RI_INIT_OPREQHASHSIZE,
								 &ctl, HASH_ELEM | HASH_FUNCTION);
}


/* ----------
 * ri_FetchPreparedPlan -
 *
 *	Lookup for a query key in our private hash table of prepared
 *	and saved SPI execution plans. Return the plan if found or NULL.
 * ----------
 */
static void *
ri_FetchPreparedPlan(RI_QueryKey *key)
{
	RI_QueryHashEntry *entry;

	/*
	 * On the first call initialize the hashtable
	 */
	if (!ri_query_cache)
		ri_InitHashTables();

	/*
	 * Lookup for the key
	 */
	entry = (RI_QueryHashEntry *) hash_search(ri_query_cache,
											  (void *) key,
											  HASH_FIND, NULL);
	if (entry == NULL)
		return NULL;
	return entry->plan;
}


/* ----------
 * ri_HashPreparedPlan -
 *
 *	Add another plan to our private SPI query plan hashtable.
 * ----------
 */
static void
ri_HashPreparedPlan(RI_QueryKey *key, void *plan)
{
	RI_QueryHashEntry *entry;
	bool		found;

	/*
	 * On the first call initialize the hashtable
	 */
	if (!ri_query_cache)
		ri_InitHashTables();

	/*
	 * Add the new plan.
	 */
	entry = (RI_QueryHashEntry *) hash_search(ri_query_cache,
											  (void *) key,
											  HASH_ENTER, &found);
	if (entry == NULL)
		elog(ERROR, "out of memory for RI plan cache");
	entry->plan = plan;
}


/* ----------
 * ri_KeysEqual -
 *
 *	Check if all key values in OLD and NEW are equal.
 * ----------
 */
static bool
ri_KeysEqual(Relation rel, HeapTuple oldtup, HeapTuple newtup,
			 RI_QueryKey *key, int pairidx)
{
	int			i;
	Oid			typeid;
	Datum		oldvalue;
	Datum		newvalue;
	bool		isnull;

	for (i = 0; i < key->nkeypairs; i++)
	{
		/*
		 * Get one attributes oldvalue. If it is NULL - they're not equal.
		 */
		oldvalue = SPI_getbinval(oldtup, rel->rd_att,
								 key->keypair[i][pairidx], &isnull);
		if (isnull)
			return false;

		/*
		 * Get one attributes oldvalue. If it is NULL - they're not equal.
		 */
		newvalue = SPI_getbinval(newtup, rel->rd_att,
								 key->keypair[i][pairidx], &isnull);
		if (isnull)
			return false;

		/*
		 * Get the attributes type OID and call the '=' operator to
		 * compare the values.
		 */
		typeid = SPI_gettypeid(rel->rd_att, key->keypair[i][pairidx]);
		if (!ri_AttributesEqual(typeid, oldvalue, newvalue))
			return false;
	}

	return true;
}


/* ----------
 * ri_AllKeysUnequal -
 *
 *	Check if all key values in OLD and NEW are not equal.
 * ----------
 */
static bool
ri_AllKeysUnequal(Relation rel, HeapTuple oldtup, HeapTuple newtup,
				  RI_QueryKey *key, int pairidx)
{
	int			i;
	Oid			typeid;
	Datum		oldvalue;
	Datum		newvalue;
	bool		isnull;
	bool		keys_unequal;

	keys_unequal = true;
	for (i = 0; keys_unequal && i < key->nkeypairs; i++)
	{
		/*
		 * Get one attributes oldvalue. If it is NULL - they're not equal.
		 */
		oldvalue = SPI_getbinval(oldtup, rel->rd_att,
								 key->keypair[i][pairidx], &isnull);
		if (isnull)
			continue;

		/*
		 * Get one attributes oldvalue. If it is NULL - they're not equal.
		 */
		newvalue = SPI_getbinval(newtup, rel->rd_att,
								 key->keypair[i][pairidx], &isnull);
		if (isnull)
			continue;

		/*
		 * Get the attributes type OID and call the '=' operator to
		 * compare the values.
		 */
		typeid = SPI_gettypeid(rel->rd_att, key->keypair[i][pairidx]);
		if (!ri_AttributesEqual(typeid, oldvalue, newvalue))
			continue;
		keys_unequal = false;
	}

	return keys_unequal;
}


/* ----------
 * ri_OneKeyEqual -
 *
 *	Check if one key value in OLD and NEW is equal.
 *
 *	ri_KeysEqual could call this but would run a bit slower.  For
 *	now, let's duplicate the code.
 * ----------
 */
static bool
ri_OneKeyEqual(Relation rel, int column, HeapTuple oldtup, HeapTuple newtup,
			   RI_QueryKey *key, int pairidx)
{
	Oid			typeid;
	Datum		oldvalue;
	Datum		newvalue;
	bool		isnull;

	/*
	 * Get one attributes oldvalue. If it is NULL - they're not equal.
	 */
	oldvalue = SPI_getbinval(oldtup, rel->rd_att,
							 key->keypair[column][pairidx], &isnull);
	if (isnull)
		return false;

	/*
	 * Get one attributes oldvalue. If it is NULL - they're not equal.
	 */
	newvalue = SPI_getbinval(newtup, rel->rd_att,
							 key->keypair[column][pairidx], &isnull);
	if (isnull)
		return false;

	/*
	 * Get the attributes type OID and call the '=' operator to compare
	 * the values.
	 */
	typeid = SPI_gettypeid(rel->rd_att, key->keypair[column][pairidx]);
	if (!ri_AttributesEqual(typeid, oldvalue, newvalue))
		return false;

	return true;
}


/* ----------
 * ri_AttributesEqual -
 *
 *	Call the type specific '=' operator comparison function
 *	for two values.
 *
 *	NB: we have already checked that neither value is null.
 * ----------
 */
static bool
ri_AttributesEqual(Oid typeid, Datum oldvalue, Datum newvalue)
{
	RI_OpreqHashEntry *entry;
	bool		found;

	/*
	 * On the first call initialize the hashtable
	 */
	if (!ri_opreq_cache)
		ri_InitHashTables();

	/*
	 * Try to find the '=' operator for this type in our cache
	 */
	entry = (RI_OpreqHashEntry *) hash_search(ri_opreq_cache,
											  (void *) &typeid,
											  HASH_FIND, NULL);

	/*
	 * If not found, lookup the operator, then do the function manager
	 * lookup, and remember that info.
	 */
	if (!entry)
	{
		Oid			opr_proc;
		FmgrInfo	finfo;

		opr_proc = compatible_oper_funcid(makeList1(makeString("=")),
										  typeid, typeid, true);
		if (!OidIsValid(opr_proc))
			elog(ERROR,
			"ri_AttributesEqual(): cannot find '=' operator for type %u",
				 typeid);

		/*
		 * Since fmgr_info could fail, call it *before* creating the
		 * hashtable entry --- otherwise we could elog leaving an
		 * incomplete entry in the hashtable.  Also, because this will be
		 * a permanent table entry, we must make sure any subsidiary
		 * structures of the fmgr record are kept in TopMemoryContext.
		 */
		fmgr_info_cxt(opr_proc, &finfo, TopMemoryContext);

		entry = (RI_OpreqHashEntry *) hash_search(ri_opreq_cache,
												  (void *) &typeid,
												  HASH_ENTER, &found);
		if (entry == NULL)
			elog(ERROR, "out of memory for RI operator cache");

		entry->typeid = typeid;
		memcpy(&(entry->oprfmgrinfo), &finfo, sizeof(FmgrInfo));
	}

	/*
	 * Call the type specific '=' function
	 */
	return DatumGetBool(FunctionCall2(&(entry->oprfmgrinfo),
									  oldvalue, newvalue));
}
