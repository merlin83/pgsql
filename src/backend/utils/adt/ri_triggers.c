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
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/ri_triggers.c,v 1.49 2003-04-07 20:30:38 wieck Exp $
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

#define RI_TRIGTYPE_INSERT 1
#define RI_TRIGTYPE_UPDATE 2
#define RI_TRIGTYPE_INUP   3
#define RI_TRIGTYPE_DELETE 4


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
static bool ri_Check_Pk_Match(Relation pk_rel, Relation fk_rel,
							  HeapTuple old_row,
							  Oid tgoid, int match_type,
							  int tgnargs, char **tgargs);

static void ri_InitHashTables(void);
static void *ri_FetchPreparedPlan(RI_QueryKey *key);
static void ri_HashPreparedPlan(RI_QueryKey *key, void *plan);

static void ri_CheckTrigger(FunctionCallInfo fcinfo, const char *funcname,
							int tgkind);
static bool ri_PerformCheck(RI_QueryKey *qkey, void *qplan,
							Relation fk_rel, Relation pk_rel,
							HeapTuple old_tuple, HeapTuple new_tuple,
							int expect_OK, const char *constrname);
static void ri_ExtractValues(RI_QueryKey *qkey, int key_idx,
							 Relation rel, HeapTuple tuple,
							 Datum *vals, char *nulls);
static void ri_ReportViolation(RI_QueryKey *qkey, const char *constrname,
							   Relation pk_rel, Relation fk_rel,
							   HeapTuple violator, bool spi_err);


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
	int			i;
	int			match_type;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_check", RI_TRIGTYPE_INUP);

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

			/*
			 * Prepare, save and remember the new plan.
			 */
			qplan = SPI_prepare(querystr, 0, NULL);
			qplan = SPI_saveplan(qplan);
			ri_HashPreparedPlan(&qkey, qplan);
		}

		/*
		 * Execute the plan
		 */
		if (SPI_connect() != SPI_OK_CONNECT)
			elog(ERROR, "SPI_connect() failed in RI_FKey_check()");

		ri_PerformCheck(&qkey, qplan,
						fk_rel, pk_rel,
						NULL, NULL,
						SPI_OK_SELECT,
						tgargs[RI_CONSTRAINT_NAME_ARGNO]);

		if (SPI_finish() != SPI_OK_FINISH)
			elog(ERROR, "SPI_finish() failed in RI_FKey_check()");

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
		if (ri_KeysEqual(fk_rel, old_row, new_row, &qkey,
						 RI_KEYPAIR_FK_IDX))
		{
			heap_close(pk_rel, RowShareLock);
			return PointerGetDatum(NULL);
		}
	}

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect() failed in RI_FKey_check()");

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
		 * corresponding FK attributes. Thus, SPI_prepare could
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

		/*
		 * Prepare, save and remember the new plan.
		 */
		qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
		qplan = SPI_saveplan(qplan);
		ri_HashPreparedPlan(&qkey, qplan);
	}

	/*
	 * Now check that foreign key exists in PK table
	 */
	ri_PerformCheck(&qkey, qplan,
					fk_rel, pk_rel,
					NULL, new_row,
					SPI_OK_SELECT,
					tgargs[RI_CONSTRAINT_NAME_ARGNO]);

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish() failed in RI_FKey_check()");

	heap_close(pk_rel, RowShareLock);

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
ri_Check_Pk_Match(Relation pk_rel, Relation fk_rel,
				  HeapTuple old_row,
				  Oid tgoid, int match_type,
				  int tgnargs, char **tgargs)
{
	void	   *qplan;
	RI_QueryKey qkey;
	int			i;
	bool		result;

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
		elog(ERROR, "SPI_connect() failed in RI_FKey_check()");

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
		 * corresponding FK attributes. Thus, SPI_prepare could
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

		/*
		 * Prepare, save and remember the new plan.
		 */
		qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
		qplan = SPI_saveplan(qplan);
		ri_HashPreparedPlan(&qkey, qplan);
	}

	/*
	 * We have a plan now. Run it.
	 */
	result = ri_PerformCheck(&qkey, qplan,
							 fk_rel, pk_rel,
							 old_row, NULL,
							 SPI_OK_SELECT, NULL);

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish() failed in ri_Check_Pk_Match()");

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
	int			i;
	int			match_type;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_noaction_del", RI_TRIGTYPE_DELETE);

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
	if (ri_Check_Pk_Match(pk_rel, fk_rel,
						  old_row, trigdata->tg_trigger->tgoid,
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
				elog(ERROR, "SPI_connect() failed in RI_FKey_noaction_del()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare, save and remember the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
				qplan = SPI_saveplan(qplan);
				ri_HashPreparedPlan(&qkey, qplan);
			}

			/*
			 * We have a plan now. Run it to check for existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_SELECT,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_noaction_del()");

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
	int			i;
	int			match_type;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_noaction_upd", RI_TRIGTYPE_UPDATE);

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
	if (ri_Check_Pk_Match(pk_rel, fk_rel,
						  old_row, trigdata->tg_trigger->tgoid,
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
				elog(ERROR, "SPI_connect() failed in RI_FKey_noaction_upd()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare, save and remember the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
				qplan = SPI_saveplan(qplan);
				ri_HashPreparedPlan(&qkey, qplan);
			}

			/*
			 * We have a plan now. Run it to check for existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_SELECT,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_noaction_upd()");

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
	int			i;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_cascade_del", RI_TRIGTYPE_DELETE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_cascade_del()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare, save and remember the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
				qplan = SPI_saveplan(qplan);
				ri_HashPreparedPlan(&qkey, qplan);
			}

			/*
			 * We have a plan now. Build up the arguments
			 * from the key values in the deleted PK tuple and delete the
			 * referencing rows
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_DELETE,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_cascade_del()");

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
	int			i;
	int			j;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_cascade_upd", RI_TRIGTYPE_UPDATE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_cascade_upd()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare, save and remember the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs * 2, queryoids);
				qplan = SPI_saveplan(qplan);
				ri_HashPreparedPlan(&qkey, qplan);
			}

			/*
			 * We have a plan now. Run it to update the existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, new_row,
							SPI_OK_UPDATE,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_cascade_upd()");

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
	int			i;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_restrict_del", RI_TRIGTYPE_DELETE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_restrict_del()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare, save and remember the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
				qplan = SPI_saveplan(qplan);
				ri_HashPreparedPlan(&qkey, qplan);
			}

			/*
			 * We have a plan now. Run it to check for existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_SELECT,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_restrict_del()");

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
	int			i;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_restrict_upd", RI_TRIGTYPE_UPDATE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_restrict_upd()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare, save and remember the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
				qplan = SPI_saveplan(qplan);
				ri_HashPreparedPlan(&qkey, qplan);
			}

			/*
			 * We have a plan now. Run it to check for existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_SELECT,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_restrict_upd()");

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
	int			i;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_setnull_del", RI_TRIGTYPE_DELETE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_setnull_del()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare, save and remember the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);
				qplan = SPI_saveplan(qplan);
				ri_HashPreparedPlan(&qkey, qplan);
			}

			/*
			 * We have a plan now. Run it to check for existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_UPDATE,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_setnull_del()");

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
	int			i;
	int			match_type;
	bool		use_cached_query;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_setnull_upd", RI_TRIGTYPE_UPDATE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_setnull_upd()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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
				 * Prepare the new plan.
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);

				/*
				 * Save and remember the plan if we're building the
				 * "standard" plan.
				 */
				if (use_cached_query)
				{
					qplan = SPI_saveplan(qplan);
					ri_HashPreparedPlan(&qkey, qplan);
				}
			}

			/*
			 * We have a plan now. Run it to update the existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_UPDATE,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_setnull_upd()");

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

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_setdefault_del", RI_TRIGTYPE_DELETE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_setdefault_del()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare the plan
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);

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
						fix_opfuncids(dfl);
						tle->expr = (Expr *) dfl;
					}
				}
			}

			/*
			 * We have a plan now. Run it to update the existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_UPDATE,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_setdefault_del()");

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
	int			match_type;

	ReferentialIntegritySnapshotOverride = true;

	/*
	 * Check that this is a valid trigger call on the right time and
	 * event.
	 */
	ri_CheckTrigger(fcinfo, "RI_FKey_setdefault_upd", RI_TRIGTYPE_UPDATE);

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
				elog(ERROR, "SPI_connect() failed in RI_FKey_setdefault_upd()");

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
				 * corresponding PK attributes. Thus, SPI_prepare could
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

				/*
				 * Prepare the plan
				 */
				qplan = SPI_prepare(querystr, qkey.nkeypairs, queryoids);

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
						fix_opfuncids(dfl);
						tle->expr = (Expr *) dfl;
					}
				}
			}

			/*
			 * We have a plan now. Run it to update the existing references.
			 */
			ri_PerformCheck(&qkey, qplan,
							fk_rel, pk_rel,
							old_row, NULL,
							SPI_OK_UPDATE,
							tgargs[RI_CONSTRAINT_NAME_ARGNO]);

			if (SPI_finish() != SPI_OK_FINISH)
				elog(ERROR, "SPI_finish() failed in RI_FKey_setdefault_upd()");

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

/*
 * Check that RI trigger function was called in expected context
 */
static void
ri_CheckTrigger(FunctionCallInfo fcinfo, const char *funcname, int tgkind)
{
	TriggerData *trigdata = (TriggerData *) fcinfo->context;

	if (!CALLED_AS_TRIGGER(fcinfo))
		elog(ERROR, "%s() not fired by trigger manager", funcname);
	if (!TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
		!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		elog(ERROR, "%s() must be fired AFTER ROW", funcname);

	switch (tgkind)
	{
		case RI_TRIGTYPE_INSERT:
			if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
				elog(ERROR, "%s() must be fired for INSERT", funcname);
			break;
		case RI_TRIGTYPE_UPDATE:
			if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
				elog(ERROR, "%s() must be fired for UPDATE", funcname);
			break;
		case RI_TRIGTYPE_INUP:
			if (!TRIGGER_FIRED_BY_INSERT(trigdata->tg_event) &&
				!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
				elog(ERROR, "%s() must be fired for INSERT or UPDATE",
					 funcname);
			break;
		case RI_TRIGTYPE_DELETE:
			if (!TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
				elog(ERROR, "%s() must be fired for DELETE", funcname);
			break;
	}
}


/*
 * Perform a query to enforce an RI restriction
 */
static bool
ri_PerformCheck(RI_QueryKey *qkey, void *qplan,
				Relation fk_rel, Relation pk_rel,
				HeapTuple old_tuple, HeapTuple new_tuple,
				int expect_OK, const char *constrname)
{
	Relation	query_rel,
				source_rel;
	int			key_idx;
	int			limit;
	int			spi_result;
	AclId		save_uid;
	Datum		vals[RI_MAX_NUMKEYS * 2];
	char		nulls[RI_MAX_NUMKEYS * 2];

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

	/*
	 * The values for the query are taken from the table on which the trigger
	 * is called - it is normally the other one with respect to query_rel.
	 * An exception is ri_Check_Pk_Match(), which uses the PK table for both
	 * (the case when constrname == NULL)
	 */
	if (qkey->constr_queryno == RI_PLAN_CHECK_LOOKUPPK && constrname != NULL)
	{
		source_rel = fk_rel;
		key_idx = RI_KEYPAIR_FK_IDX;
	}
	else
	{
		source_rel = pk_rel;
		key_idx = RI_KEYPAIR_PK_IDX;
	}

	/* Extract the parameters to be passed into the query */
	if (new_tuple)
	{
		ri_ExtractValues(qkey, key_idx, source_rel, new_tuple,
						 vals, nulls);
		if (old_tuple)
			ri_ExtractValues(qkey, key_idx, source_rel, old_tuple,
							 vals + qkey->nkeypairs, nulls + qkey->nkeypairs);
	}
	else
	{
		ri_ExtractValues(qkey, key_idx, source_rel, old_tuple,
						 vals, nulls);
	}

	/* Switch to proper UID to perform check as */
	save_uid = GetUserId();
	SetUserId(RelationGetForm(query_rel)->relowner);

	/*
	 * If this is a select query (e.g., for a 'no action' or 'restrict'
	 * trigger), we only need to see if there is a single row in the table,
	 * matching the key.  Otherwise, limit = 0 - because we want the query to
	 * affect ALL the matching rows.
	 */
	limit = (expect_OK == SPI_OK_SELECT) ? 1 : 0;

	/* Run the plan */
	spi_result = SPI_execp(qplan, vals, nulls, limit);

	/* Restore UID */
	SetUserId(save_uid);

	/* Check result */
	if (spi_result < 0)
		elog(ERROR, "SPI_execp() failed in ri_PerformCheck()");

	if (expect_OK >= 0 && spi_result != expect_OK)
		ri_ReportViolation(qkey, constrname ? constrname : "",
						   pk_rel, fk_rel,
						   new_tuple ? new_tuple : old_tuple,
						   true);

	/* XXX wouldn't it be clearer to do this part at the caller? */
	if (constrname && expect_OK == SPI_OK_SELECT &&
		(SPI_processed==0) == (qkey->constr_queryno==RI_PLAN_CHECK_LOOKUPPK))
		ri_ReportViolation(qkey, constrname,
						   pk_rel, fk_rel,
						   new_tuple ? new_tuple : old_tuple,
						   false);

	return SPI_processed != 0;
}

/*
 * Extract fields from a tuple into Datum/nulls arrays
 */
static void
ri_ExtractValues(RI_QueryKey *qkey, int key_idx,
				 Relation rel, HeapTuple tuple,
				 Datum *vals, char *nulls)
{
	int			i;
	bool		isnull;

	for (i = 0; i < qkey->nkeypairs; i++)
	{
		vals[i] = SPI_getbinval(tuple, rel->rd_att,
								qkey->keypair[i][key_idx],
								&isnull);
		nulls[i] = isnull ? 'n' : ' ';
	}
}

/*
 * Produce an error report
 *
 * If the failed constraint was on insert/update to the FK table,
 * we want the key names and values extracted from there, and the error
 * message to look like 'key blah referenced from FK not found in PK'.
 * Otherwise, the attr names and values come from the PK table and the
 * message looks like 'key blah in PK still referenced from FK'.
 */
static void
ri_ReportViolation(RI_QueryKey *qkey, const char *constrname,
				   Relation pk_rel, Relation fk_rel,
				   HeapTuple violator, bool spi_err)
{
#define BUFLENGTH	512
	char		key_names[BUFLENGTH];
	char		key_values[BUFLENGTH];
	char	   *name_ptr = key_names;
	char	   *val_ptr = key_values;
	bool		onfk;
	Relation	rel,
				other_rel;
	int			idx,
				key_idx;

	/*
	 * rel is set to where the tuple description is coming from, and it also
	 * is the first relation mentioned in the message, other_rel is
	 * respectively the other relation.
	 */
	onfk = (qkey->constr_queryno == RI_PLAN_CHECK_LOOKUPPK);
	if (onfk)
	{
		rel = fk_rel;
		other_rel = pk_rel;
		key_idx = RI_KEYPAIR_FK_IDX;
	}
	else
	{
		rel = pk_rel;
		other_rel = fk_rel;
		key_idx = RI_KEYPAIR_PK_IDX;
	}

	/*
	 * Special case - if there are no keys at all, this is a 'no column'
	 * constraint - no need to try to extract the values, and the message
	 * in this case looks different.
	 */
	if (qkey->nkeypairs == 0)
	{
		if (spi_err)
			elog(ERROR, "%s referential action on %s from %s rewritten by rule",
				 constrname,
				 RelationGetRelationName(fk_rel),
				 RelationGetRelationName(pk_rel));
		else
			elog(ERROR, "%s referential integrity violation - no rows found in %s",
				 constrname, RelationGetRelationName(pk_rel));
	}

	/* Get printable versions of the keys involved */
	for (idx = 0; idx < qkey->nkeypairs; idx++)
	{
		int		fnum = qkey->keypair[idx][key_idx];
		char   *name,
			   *val;

		name = SPI_fname(rel->rd_att, fnum);
		val = SPI_getvalue(violator, rel->rd_att, fnum);
		if (!val)
			val = "null";

		/*
		 * Go to "..." if name or value doesn't fit in buffer.  We reserve
		 * 5 bytes to ensure we can add comma, "...", null.
		 */
		if (strlen(name) >= (key_names + BUFLENGTH - 5) - name_ptr ||
			strlen(val) >= (key_values + BUFLENGTH - 5) - val_ptr)
		{
			sprintf(name_ptr, "...");
			sprintf(val_ptr, "...");
			break;
		}

		name_ptr += sprintf(name_ptr, "%s%s", idx > 0 ? "," : "", name);
		val_ptr += sprintf(val_ptr, "%s%s", idx > 0 ? "," : "", val);
  }

  if (spi_err)
	  elog(ERROR, "%s referential action on %s from %s for (%s)=(%s) rewritten by rule",
		   constrname,
		   RelationGetRelationName(fk_rel),
		   RelationGetRelationName(pk_rel),
		   key_names, key_values);
  else if (onfk)
	  elog(ERROR, "%s referential integrity violation - key (%s)=(%s) referenced from %s not found in %s",
		   constrname, key_names, key_values,
		   RelationGetRelationName(rel),
		   RelationGetRelationName(other_rel));
  else
	  elog(ERROR, "%s referential integrity violation - key (%s)=(%s) in %s still referenced from %s",
		   constrname, key_names, key_values,
		   RelationGetRelationName(rel),
		   RelationGetRelationName(other_rel));
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

		opr_proc = equality_oper_funcid(typeid);

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
