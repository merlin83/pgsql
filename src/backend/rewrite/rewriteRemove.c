/*-------------------------------------------------------------------------
 *
 * rewriteRemove.c
 *	  routines for removing rewrite rules
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/rewrite/rewriteRemove.c,v 1.26 1999-07-16 03:13:23 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include <string.h>

#include "postgres.h"


#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_rewrite.h"
#include "utils/syscache.h"

#include "rewrite/rewriteRemove.h"
#include "rewrite/rewriteSupport.h"

/*-----------------------------------------------------------------------
 * RewriteGetRuleEventRel
 *-----------------------------------------------------------------------
 */
char *
RewriteGetRuleEventRel(char *rulename)
{
	HeapTuple	htup;
	Oid			eventrel;

	htup = SearchSysCacheTuple(REWRITENAME,
							   PointerGetDatum(rulename),
							   0, 0, 0);
	if (!HeapTupleIsValid(htup))
		elog(ERROR, "Rule or view '%s' not found",
		  ((!strncmp(rulename, "_RET", 4)) ? (rulename + 4) : rulename));
	eventrel = ((Form_pg_rewrite) GETSTRUCT(htup))->ev_class;
	htup = SearchSysCacheTuple(RELOID,
							   PointerGetDatum(eventrel),
							   0, 0, 0);
	if (!HeapTupleIsValid(htup))
		elog(ERROR, "Class '%u' not found", eventrel);

	return ((Form_pg_class) GETSTRUCT(htup))->relname.data;
}

/* ----------------------------------------------------------------
 *
 * RemoveRewriteRule
 *
 * Delete a rule given its rulename.
 *
 * There are three steps.
 *	 1) Find the corresponding tuple in 'pg_rewrite' relation.
 *		Find the rule Id (i.e. the Oid of the tuple) and finally delete
 *		the tuple.
 *	 3) Delete the locks from the 'pg_class' relation.
 *
 *
 * ----------------------------------------------------------------
 */
void
RemoveRewriteRule(char *ruleName)
{
	Relation	RewriteRelation = NULL;
	HeapTuple	tuple = NULL;
	Oid			ruleId = (Oid) 0;
	Oid			eventRelationOid = (Oid) NULL;
	Datum		eventRelationOidDatum = (Datum) NULL;
	bool		isNull = false;

	/*
	 * Open the pg_rewrite relation.
	 */
	RewriteRelation = heap_openr(RewriteRelationName);

	/*
	 * Scan the RuleRelation ('pg_rewrite') until we find a tuple
	 */
	tuple = SearchSysCacheTupleCopy(REWRITENAME,
									PointerGetDatum(ruleName),
									0, 0, 0);

	/*
	 * complain if no rule with such name existed
	 */
	if (!HeapTupleIsValid(tuple))
	{
		heap_close(RewriteRelation);
		elog(ERROR, "Rule '%s' not found\n", ruleName);
	}

	/*
	 * Store the OID of the rule (i.e. the tuple's OID) and the event
	 * relation's OID
	 */
	ruleId = tuple->t_data->t_oid;
	eventRelationOidDatum = heap_getattr(tuple,
										 Anum_pg_rewrite_ev_class,
									   RelationGetDescr(RewriteRelation),
										 &isNull);
	if (isNull)
	{
		/* XXX strange!!! */
		pfree(tuple);
		elog(ERROR, "RemoveRewriteRule: internal error; null event target relation!");
	}
	eventRelationOid = DatumGetObjectId(eventRelationOidDatum);

	/*
	 * Now delete the relation level locks from the updated relation.
	 * (Make sure we do this before we remove the rule from pg_rewrite.
	 * Otherwise, heap_openr on eventRelationOid which reads pg_rwrite for
	 * the rules will fail.)
	 */
	prs2_deleteFromRelation(eventRelationOid, ruleId);

	/*
	 * Now delete the tuple...
	 */
	heap_delete(RewriteRelation, &tuple->t_self, NULL);

	pfree(tuple);
	heap_close(RewriteRelation);
}

/*
 * RelationRemoveRules -
 *	  removes all rules associated with the relation when the relation is
 *	  being removed.
 */
void
RelationRemoveRules(Oid relid)
{
	Relation	RewriteRelation = NULL;
	HeapScanDesc scanDesc = NULL;
	ScanKeyData scanKeyData;
	HeapTuple	tuple = NULL;

	/*
	 * Open the pg_rewrite relation.
	 */
	RewriteRelation = heap_openr(RewriteRelationName);

	/*
	 * Scan the RuleRelation ('pg_rewrite') for all the tuples that has
	 * the same ev_class as relid (the relation to be removed).
	 */
	ScanKeyEntryInitialize(&scanKeyData,
						   0,
						   Anum_pg_rewrite_ev_class,
						   F_OIDEQ,
						   ObjectIdGetDatum(relid));
	scanDesc = heap_beginscan(RewriteRelation,
							  0, SnapshotNow, 1, &scanKeyData);

	while (HeapTupleIsValid(tuple = heap_getnext(scanDesc, 0)))
		heap_delete(RewriteRelation, &tuple->t_self, NULL);

	heap_endscan(scanDesc);
	heap_close(RewriteRelation);
}
