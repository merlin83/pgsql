/*-------------------------------------------------------------------------
 *
 * rewriteSupport.c
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/rewrite/rewriteSupport.c,v 1.37 1999-09-18 19:07:20 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"


#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "rewrite/rewriteSupport.h"
#include "utils/catcache.h"
#include "utils/syscache.h"

/*
 * RuleIdGetActionInfo -
 *	   given a rule oid, look it up and return the rule-event-qual and
 *	   list of parsetrees for the rule (in parseTrees)
 */
#ifdef NOT_USED
static Node *
RuleIdGetActionInfo(Oid ruleoid, bool *instead_flag, Query **parseTrees)
{
	HeapTuple	ruletuple;
	char	   *ruleaction = NULL;
	bool		action_is_null = false;
	bool		instead_is_null = false;
	Relation	ruleRelation = NULL;
	TupleDesc	ruleTupdesc = NULL;
	Query	   *ruleparse = NULL;
	char	   *rule_evqual_string = NULL;
	Node	   *rule_evqual = NULL;

	ruleRelation = heap_openr(RewriteRelationName, AccessShareLock);
	ruleTupdesc = RelationGetDescr(ruleRelation);
	ruletuple = SearchSysCacheTuple(RULOID,
									ObjectIdGetDatum(ruleoid),
									0, 0, 0);
	if (ruletuple == NULL)
		elog(ERROR, "rule %u isn't in rewrite system relation", ruleoid);

	ruleaction = (char *) heap_getattr(ruletuple,
									   Anum_pg_rewrite_ev_action,
									   ruleTupdesc,
									   &action_is_null);
	rule_evqual_string = (char *) heap_getattr(ruletuple,
											   Anum_pg_rewrite_ev_qual,
										   ruleTupdesc, &action_is_null);
	*instead_flag = !!heap_getattr(ruletuple,
								   Anum_pg_rewrite_is_instead,
								   ruleTupdesc, &instead_is_null);

	if (action_is_null || instead_is_null)
		elog(ERROR, "internal error: rewrite rule not properly set up");

	ruleaction = textout((struct varlena *) ruleaction);
	rule_evqual_string = textout((struct varlena *) rule_evqual_string);

	ruleparse = (Query *) stringToNode(ruleaction);
	rule_evqual = (Node *) stringToNode(rule_evqual_string);

	heap_close(ruleRelation, AccessShareLock);

	*parseTrees = ruleparse;
	return rule_evqual;
}

#endif

int
IsDefinedRewriteRule(char *ruleName)
{
	HeapTuple	tuple;

	tuple = SearchSysCacheTuple(REWRITENAME,
								PointerGetDatum(ruleName),
								0, 0, 0);
	return HeapTupleIsValid(tuple);
}

static void
setRelhasrulesInRelation(Oid relationId, bool relhasrules)
{
	Relation	relationRelation;
	HeapTuple	tuple;
	Relation	idescs[Num_pg_class_indices];

	/*
	 * Lock a relation given its Oid. Go to the RelationRelation (i.e.
	 * pg_relation), find the appropriate tuple, and add the specified
	 * lock to it.
	 */
	relationRelation = heap_openr(RelationRelationName, RowExclusiveLock);
	tuple = SearchSysCacheTupleCopy(RELOID,
									ObjectIdGetDatum(relationId),
									0, 0, 0);
	Assert(HeapTupleIsValid(tuple));

	((Form_pg_class) GETSTRUCT(tuple))->relhasrules = relhasrules;
	heap_replace(relationRelation, &tuple->t_self, tuple, NULL);

	/* keep the catalog indices up to date */
	CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, idescs);
	CatalogIndexInsert(idescs, Num_pg_class_indices, relationRelation, tuple);
	CatalogCloseIndices(Num_pg_class_indices, idescs);

	pfree(tuple);
	heap_close(relationRelation, RowExclusiveLock);
}

void
prs2_addToRelation(Oid relid,
				   Oid ruleId,
				   CmdType event_type,
				   AttrNumber attno,
				   bool isInstead,
				   Node *qual,
				   List *actions)
{
	Relation	relation;
	RewriteRule *thisRule;
	RuleLock   *rulelock;
	MemoryContext oldcxt;

	/*
	 * create an in memory RewriteRule data structure which is cached by
	 * every Relation descriptor. (see utils/cache/relcache.c)
	 */
	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);
	thisRule = (RewriteRule *) palloc(sizeof(RewriteRule));
	if (qual != NULL)
		qual = copyObject(qual);
	if (actions != NIL)
		actions = copyObject(actions);
	MemoryContextSwitchTo(oldcxt);

	thisRule->ruleId = ruleId;
	thisRule->event = event_type;
	thisRule->attrno = attno;
	thisRule->qual = qual;
	thisRule->actions = actions;
	thisRule->isInstead = isInstead;

	relation = heap_open(relid, AccessShareLock);

	/*
	 * modify or create a RuleLock cached by Relation
	 */
	if (relation->rd_rules == NULL)
	{

		oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);
		rulelock = (RuleLock *) palloc(sizeof(RuleLock));
		rulelock->numLocks = 1;
		rulelock->rules = (RewriteRule **) palloc(sizeof(RewriteRule *));
		rulelock->rules[0] = thisRule;
		relation->rd_rules = rulelock;
		MemoryContextSwitchTo(oldcxt);

		/*
		 * the fact that relation->rd_rules is NULL means the relhasrules
		 * attribute of the tuple of this relation in pg_class is false.
		 * We need to set it to true.
		 */
		setRelhasrulesInRelation(relid, TRUE);
	}
	else
	{
		int			numlock;

		rulelock = relation->rd_rules;
		numlock = rulelock->numLocks;
		/* expand, for safety reasons */
		oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);
		rulelock->rules = (RewriteRule **) repalloc(rulelock->rules,
								  sizeof(RewriteRule *) * (numlock + 1));
		MemoryContextSwitchTo(oldcxt);
		rulelock->rules[numlock] = thisRule;
		rulelock->numLocks++;
	}

	heap_close(relation, AccessShareLock);
}

void
prs2_deleteFromRelation(Oid relid, Oid ruleId)
{
	RuleLock   *rulelock;
	Relation	relation;
	int			numlock;
	int			i;
	MemoryContext oldcxt;

	relation = heap_open(relid, AccessShareLock);
	rulelock = relation->rd_rules;
	Assert(rulelock != NULL);

	numlock = rulelock->numLocks;
	for (i = 0; i < numlock; i++)
	{
		if (rulelock->rules[i]->ruleId == ruleId)
			break;
	}
	Assert(i < numlock);
	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);
	pfree(rulelock->rules[i]);
	MemoryContextSwitchTo(oldcxt);
	if (numlock == 1)
	{
		relation->rd_rules = NULL;

		/*
		 * we don't have rules any more, flag the relhasrules attribute of
		 * the tuple of this relation in pg_class false.
		 */
		setRelhasrulesInRelation(relid, FALSE);
	}
	else
	{
		rulelock->rules[i] = rulelock->rules[numlock - 1];
		rulelock->rules[numlock - 1] = NULL;
		rulelock->numLocks--;
	}

	heap_close(relation, AccessShareLock);
}
