/*-------------------------------------------------------------------------
 *
 * rewriteDefine.c
 *	  routines for defining a rewrite rule
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/rewrite/rewriteDefine.c,v 1.76 2002-08-02 18:15:07 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/pg_rewrite.h"
#include "commands/view.h"
#include "miscadmin.h"
#include "optimizer/clauses.h"
#include "parser/parse_relation.h"
#include "rewrite/rewriteDefine.h"
#include "rewrite/rewriteManip.h"
#include "rewrite/rewriteSupport.h"
#include "storage/smgr.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/syscache.h"


static void setRuleCheckAsUser(Query *qry, Oid userid);
static bool setRuleCheckAsUser_walker(Node *node, Oid *context);


/*
 * InsertRule -
 *	  takes the arguments and inserts them as a row into the system
 *	  relation "pg_rewrite"
 */
static Oid
InsertRule(char *rulname,
		   int evtype,
		   Oid eventrel_oid,
		   AttrNumber evslot_index,
		   bool evinstead,
		   Node *event_qual,
		   List *action)
{
	char	   *evqual = nodeToString(event_qual);
	char	   *actiontree = nodeToString((Node *) action);
	int			i;
	Datum		values[Natts_pg_rewrite];
	char		nulls[Natts_pg_rewrite];
	NameData	rname;
	Relation	pg_rewrite_desc;
	TupleDesc	tupDesc;
	HeapTuple	tup;
	Oid			rewriteObjectId;
	ObjectAddress	myself,
					referenced;

	if (IsDefinedRewriteRule(eventrel_oid, rulname))
		elog(ERROR, "Attempt to insert rule \"%s\" failed: already exists",
			 rulname);

	/*
	 * Set up *nulls and *values arrays
	 */
	MemSet(nulls, ' ', sizeof(nulls));

	i = 0;
	namestrcpy(&rname, rulname);
	values[i++] = NameGetDatum(&rname);				/* rulename */
	values[i++] = ObjectIdGetDatum(eventrel_oid);	/* ev_class */
	values[i++] = Int16GetDatum(evslot_index);		/* ev_attr */
	values[i++] = CharGetDatum(evtype + '0');		/* ev_type */
	values[i++] = BoolGetDatum(evinstead);			/* is_instead */
	values[i++] = DirectFunctionCall1(textin, CStringGetDatum(evqual));	/* ev_qual */
	values[i++] = DirectFunctionCall1(textin, CStringGetDatum(actiontree));	/* ev_action */

	/*
	 * create a new pg_rewrite tuple
	 */
	pg_rewrite_desc = heap_openr(RewriteRelationName, RowExclusiveLock);

	tupDesc = pg_rewrite_desc->rd_att;

	tup = heap_formtuple(tupDesc,
						 values,
						 nulls);

	rewriteObjectId = simple_heap_insert(pg_rewrite_desc, tup);

	if (RelationGetForm(pg_rewrite_desc)->relhasindex)
	{
		Relation	idescs[Num_pg_rewrite_indices];

		CatalogOpenIndices(Num_pg_rewrite_indices, Name_pg_rewrite_indices,
						   idescs);
		CatalogIndexInsert(idescs, Num_pg_rewrite_indices, pg_rewrite_desc,
						   tup);
		CatalogCloseIndices(Num_pg_rewrite_indices, idescs);
	}

	heap_freetuple(tup);

	/*
	 * Install dependency on rule's relation to ensure it will go away
	 * on relation deletion.  If the rule is ON SELECT, make the dependency
	 * implicit --- this prevents deleting a view's SELECT rule.  Other
	 * kinds of rules can be AUTO.
	 */
	myself.classId = RelationGetRelid(pg_rewrite_desc);
	myself.objectId = rewriteObjectId;
	myself.objectSubId = 0;

	referenced.classId = RelOid_pg_class;
	referenced.objectId = eventrel_oid;
	referenced.objectSubId = 0;

	recordDependencyOn(&myself, &referenced,
		(evtype == CMD_SELECT) ? DEPENDENCY_INTERNAL : DEPENDENCY_AUTO);

	/*
	 * Also install dependencies on objects referenced in action and qual.
	 */
	recordDependencyOnExpr(&myself, (Node *) action, NIL,
						   DEPENDENCY_NORMAL);
	if (event_qual != NULL)
	{
		/* Find query containing OLD/NEW rtable entries */
		Query  *qry = (Query *) lfirst(action);

		qry = getInsertSelectQuery(qry, NULL);
		recordDependencyOnExpr(&myself, event_qual, qry->rtable,
							   DEPENDENCY_NORMAL);
	}

	heap_close(pg_rewrite_desc, RowExclusiveLock);

	return rewriteObjectId;
}

void
DefineQueryRewrite(RuleStmt *stmt)
{
	RangeVar   *event_obj = stmt->relation;
	Node	   *event_qual = stmt->whereClause;
	CmdType		event_type = stmt->event;
	bool		is_instead = stmt->instead;
	List	   *action = stmt->actions;
	Relation	event_relation;
	Oid			ev_relid;
	Oid			ruleId;
	int			event_attno;
	Oid			event_attype;
	List	   *l;
	Query	   *query;
	AclResult	aclresult;
	bool		RelisBecomingView = false;

	/*
	 * If we are installing an ON SELECT rule, we had better grab
	 * AccessExclusiveLock to ensure no SELECTs are currently running on
	 * the event relation.	For other types of rules, it might be
	 * sufficient to grab ShareLock to lock out insert/update/delete
	 * actions.  But for now, let's just grab AccessExclusiveLock all the
	 * time.
	 */
	event_relation = heap_openrv(event_obj, AccessExclusiveLock);
	ev_relid = RelationGetRelid(event_relation);

	/*
	 * Check user has permission to apply rules to this relation.
	 */
	aclresult = pg_class_aclcheck(ev_relid, GetUserId(), ACL_RULE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, RelationGetRelationName(event_relation));

	/*
	 * No rule actions that modify OLD or NEW
	 */
	foreach(l, action)
	{
		query = (Query *) lfirst(l);
		if (query->resultRelation == 0)
			continue;
		/* Don't be fooled by INSERT/SELECT */
		if (query != getInsertSelectQuery(query, NULL))
			continue;
		if (query->resultRelation == PRS2_OLD_VARNO)
			elog(ERROR, "rule actions on OLD currently not supported"
				 "\n\tuse views or triggers instead");
		if (query->resultRelation == PRS2_NEW_VARNO)
			elog(ERROR, "rule actions on NEW currently not supported"
				 "\n\tuse triggers instead");
	}

	/*
	 * Rules ON SELECT are restricted to view definitions
	 */
	if (event_type == CMD_SELECT)
	{
		List	   *tllist;
		int			i;

		/*
		 * So there cannot be INSTEAD NOTHING, ...
		 */
		if (length(action) == 0)
		{
			elog(ERROR, "instead nothing rules on select currently not supported"
				 "\n\tuse views instead");
		}

		/*
		 * ... there cannot be multiple actions, ...
		 */
		if (length(action) > 1)
			elog(ERROR, "multiple action rules on select currently not supported");

		/*
		 * ... the one action must be a SELECT, ...
		 */
		query = (Query *) lfirst(action);
		if (!is_instead || query->commandType != CMD_SELECT)
			elog(ERROR, "only instead-select rules currently supported on select");

		/*
		 * ... there can be no rule qual, ...
		 */
		if (event_qual != NULL)
			elog(ERROR, "event qualifications not supported for rules on select");

		/*
		 * ... the targetlist of the SELECT action must exactly match the
		 * event relation, ...
		 */
		i = 0;
		foreach(tllist, query->targetList)
		{
			TargetEntry *tle = (TargetEntry *) lfirst(tllist);
			Resdom	   *resdom = tle->resdom;
			Form_pg_attribute attr;
			char	   *attname;

			if (resdom->resjunk)
				continue;
			i++;
			if (i > event_relation->rd_att->natts)
				elog(ERROR, "select rule's target list has too many entries");

			attr = event_relation->rd_att->attrs[i - 1];
			attname = NameStr(attr->attname);

			/*
			 * Disallow dropped columns in the relation.  This won't happen
			 * in the cases we actually care about (namely creating a view
			 * via CREATE TABLE then CREATE RULE).  Trying to cope with it
			 * is much more trouble than it's worth, because we'd have to
			 * modify the rule to insert dummy NULLs at the right positions.
			 */
			if (attr->attisdropped)
				elog(ERROR, "cannot convert relation containing dropped columns to view");

			if (strcmp(resdom->resname, attname) != 0)
				elog(ERROR, "select rule's target entry %d has different column name from %s", i, attname);

			if (attr->atttypid != resdom->restype)
				elog(ERROR, "select rule's target entry %d has different type from attribute %s", i, attname);

			/*
			 * Allow typmods to be different only if one of them is -1,
			 * ie, "unspecified".  This is necessary for cases like
			 * "numeric", where the table will have a filled-in default
			 * length but the select rule's expression will probably have
			 * typmod = -1.
			 */
			if (attr->atttypmod != resdom->restypmod &&
				attr->atttypmod != -1 && resdom->restypmod != -1)
				elog(ERROR, "select rule's target entry %d has different size from attribute %s", i, attname);
		}

		if (i != event_relation->rd_att->natts)
			elog(ERROR, "select rule's target list has too few entries");

		/*
		 * ... there must not be another ON SELECT rule already ...
		 */
		if (event_relation->rd_rules != NULL)
		{
			for (i = 0; i < event_relation->rd_rules->numLocks; i++)
			{
				RewriteRule *rule;

				rule = event_relation->rd_rules->rules[i];
				if (rule->event == CMD_SELECT)
					elog(ERROR, "\"%s\" is already a view",
						 RelationGetRelationName(event_relation));
			}
		}

		/*
		 * ... and finally the rule must be named _RETURN.
		 */
		if (strcmp(stmt->rulename, ViewSelectRuleName) != 0)
		{
			/*
			 * In versions before 7.3, the expected name was _RETviewname.
			 * For backwards compatibility with old pg_dump output, accept
			 * that and silently change it to _RETURN.  Since this is just
			 * a quick backwards-compatibility hack, limit the number of
			 * characters checked to a few less than NAMEDATALEN; this
			 * saves having to worry about where a multibyte character might
			 * have gotten truncated.
			 */
			if (strncmp(stmt->rulename, "_RET", 4) != 0 ||
				strncmp(stmt->rulename + 4, event_obj->relname,
						NAMEDATALEN - 4 - 4) != 0)
				elog(ERROR, "view rule for \"%s\" must be named \"%s\"",
					 event_obj->relname, ViewSelectRuleName);
			stmt->rulename = pstrdup(ViewSelectRuleName);
		}

		/*
		 * Are we converting a relation to a view?
		 *
		 * If so, check that the relation is empty because the storage for
		 * the relation is going to be deleted.
		 */
		if (event_relation->rd_rel->relkind != RELKIND_VIEW)
		{
			HeapScanDesc scanDesc;

			scanDesc = heap_beginscan(event_relation, SnapshotNow, 0, NULL);
			if (heap_getnext(scanDesc, ForwardScanDirection) != NULL)
				elog(ERROR, "Relation \"%s\" is not empty. Cannot convert it to view",
					 event_obj->relname);
			heap_endscan(scanDesc);

			RelisBecomingView = true;
		}
	}

	/*
	 * This rule is allowed - prepare to install it.
	 */
	event_attno = -1;
	event_attype = InvalidOid;

	/*
	 * We want the rule's table references to be checked as though by the
	 * rule owner, not the user referencing the rule.  Therefore, scan
	 * through the rule's rtables and set the checkAsUser field on all
	 * rtable entries.
	 */
	foreach(l, action)
	{
		query = (Query *) lfirst(l);
		setRuleCheckAsUser(query, GetUserId());
	}

	/* discard rule if it's null action and not INSTEAD; it's a no-op */
	if (action != NIL || is_instead)
	{
		ruleId = InsertRule(stmt->rulename,
							event_type,
							ev_relid,
							event_attno,
							is_instead,
							event_qual,
							action);

		/*
		 * Set pg_class 'relhasrules' field TRUE for event relation. If
		 * appropriate, also modify the 'relkind' field to show that the
		 * relation is now a view.
		 *
		 * Important side effect: an SI notice is broadcast to force all
		 * backends (including me!) to update relcache entries with the
		 * new rule.
		 */
		SetRelationRuleStatus(ev_relid, true, RelisBecomingView);
	}

	/*
	 * IF the relation is becoming a view, delete the storage files
	 * associated with it.	NB: we had better have AccessExclusiveLock to
	 * do this ...
	 */
	if (RelisBecomingView)
		smgrunlink(DEFAULT_SMGR, event_relation);

	/* Close rel, but keep lock till commit... */
	heap_close(event_relation, NoLock);
}

/*
 * setRuleCheckAsUser
 *		Recursively scan a query and set the checkAsUser field to the
 *		given userid in all rtable entries.
 *
 * Note: for a view (ON SELECT rule), the checkAsUser field of the *OLD*
 * RTE entry will be overridden when the view rule is expanded, and the
 * checkAsUser field of the *NEW* entry is irrelevant because that entry's
 * checkFor bits will never be set.  However, for other types of rules it's
 * important to set these fields to match the rule owner.  So we just set
 * them always.
 */
static void
setRuleCheckAsUser(Query *qry, Oid userid)
{
	List	   *l;

	/* Set all the RTEs in this query node */
	foreach(l, qry->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(l);

		if (rte->rtekind == RTE_SUBQUERY)
		{
			/* Recurse into subquery in FROM */
			setRuleCheckAsUser(rte->subquery, userid);
		}
		else
			rte->checkAsUser = userid;
	}

	/* If there are sublinks, search for them and process their RTEs */
	if (qry->hasSubLinks)
		query_tree_walker(qry, setRuleCheckAsUser_walker, (void *) &userid,
						  false /* already did the ones in rtable */ );
}

/*
 * Expression-tree walker to find sublink queries
 */
static bool
setRuleCheckAsUser_walker(Node *node, Oid *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Query))
	{
		Query	   *qry = (Query *) node;

		setRuleCheckAsUser(qry, *context);
		return false;
	}
	return expression_tree_walker(node, setRuleCheckAsUser_walker,
								  (void *) context);
}


/*
 * Rename an existing rewrite rule.
 *
 * This is unused code at the moment.
 */
void
RenameRewriteRule(Oid owningRel, const char *oldName,
				  const char *newName)
{
	Relation	pg_rewrite_desc;
	HeapTuple	ruletup;

	pg_rewrite_desc = heap_openr(RewriteRelationName, RowExclusiveLock);

	ruletup = SearchSysCacheCopy(RULERELNAME,
								 ObjectIdGetDatum(owningRel),
								 PointerGetDatum(oldName),
								 0, 0);
	if (!HeapTupleIsValid(ruletup))
		elog(ERROR, "RenameRewriteRule: rule \"%s\" does not exist", oldName);

	/* should not already exist */
	if (IsDefinedRewriteRule(owningRel, newName))
		elog(ERROR, "Attempt to rename rule \"%s\" failed: \"%s\" already exists",
			 oldName, newName);

	namestrcpy(&(((Form_pg_rewrite) GETSTRUCT(ruletup))->rulename), newName);

	simple_heap_update(pg_rewrite_desc, &ruletup->t_self, ruletup);

	/* keep system catalog indices current */
	if (RelationGetForm(pg_rewrite_desc)->relhasindex)
	{
		Relation	idescs[Num_pg_rewrite_indices];

		CatalogOpenIndices(Num_pg_rewrite_indices, Name_pg_rewrite_indices,
						   idescs);
		CatalogIndexInsert(idescs, Num_pg_rewrite_indices, pg_rewrite_desc,
						   ruletup);
		CatalogCloseIndices(Num_pg_rewrite_indices, idescs);
	}

	heap_freetuple(ruletup);
	heap_close(pg_rewrite_desc, RowExclusiveLock);
}
