/*-------------------------------------------------------------------------
 *
 * view.c
 *	  use rewrite rules to construct views
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$Id: view.c,v 1.50 2000-10-26 17:04:12 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/xact.h"
#include "catalog/heap.h"
#include "commands/creatinh.h"
#include "commands/view.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_relation.h"
#include "parser/parse_type.h"
#include "rewrite/rewriteDefine.h"
#include "rewrite/rewriteManip.h"
#include "rewrite/rewriteRemove.h"

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"
#endif

/*---------------------------------------------------------------------
 * DefineVirtualRelation
 *
 * Create the "view" relation.
 * `DefineRelation' does all the work, we just provide the correct
 * arguments!
 *
 * If the relation already exists, then 'DefineRelation' will abort
 * the xact...
 *---------------------------------------------------------------------
 */
static void
DefineVirtualRelation(char *relname, List *tlist)
{
	CreateStmt *createStmt = makeNode(CreateStmt);
	List	   *attrList,
			   *t;

	/*
	 * create a list of ColumnDef nodes based on the names and types of
	 * the (non-junk) targetlist items from the view's SELECT list.
	 */
	attrList = NIL;
	foreach(t, tlist)
	{
		TargetEntry *entry = lfirst(t);
		Resdom	   *res = entry->resdom;

		if (! res->resjunk)
		{
			char	   *resname = res->resname;
			char	   *restypename = typeidTypeName(res->restype);
			ColumnDef  *def = makeNode(ColumnDef);
			TypeName   *typename = makeNode(TypeName);

			def->colname = pstrdup(resname);

			typename->name = pstrdup(restypename);
			typename->typmod = res->restypmod;
			def->typename = typename;

			def->is_not_null = false;
			def->is_sequence = false;
			def->raw_default = NULL;
			def->cooked_default = NULL;
			def->constraints = NIL;

			attrList = lappend(attrList, def);
		}
	}

	if (attrList == NIL)
		elog(ERROR, "attempted to define virtual relation with no attrs");

	/*
	 * now create the parameters for keys/inheritance etc. All of them are
	 * nil...
	 */
	createStmt->relname = relname;
	createStmt->istemp = false;
	createStmt->tableElts = attrList;
	createStmt->inhRelnames = NIL;
	createStmt->constraints = NIL;

	/*
	 * finally create the relation...
	 */
	DefineRelation(createStmt, RELKIND_VIEW);
}

/*------------------------------------------------------------------
 * makeViewRetrieveRuleName
 *
 * Given a view name, returns the name for the 'on retrieve to "view"'
 * rule.
 *------------------------------------------------------------------
 */
char *
MakeRetrieveViewRuleName(char *viewName)
{
	char	   *buf;
#ifdef MULTIBYTE
	int			len;
#endif

	buf = palloc(strlen(viewName) + 5);
	snprintf(buf, strlen(viewName) + 5, "_RET%s", viewName);

#ifdef MULTIBYTE
	len = pg_mbcliplen(buf,strlen(buf),NAMEDATALEN-1);
	buf[len] = '\0';
#else
	buf[NAMEDATALEN-1] = '\0';
#endif

	return buf;
}

static RuleStmt *
FormViewRetrieveRule(char *viewName, Query *viewParse)
{
	RuleStmt   *rule;
	char	   *rname;
	Attr	   *attr;

	/*
	 * Create a RuleStmt that corresponds to the suitable rewrite rule
	 * args for DefineQueryRewrite();
	 */
	rule = makeNode(RuleStmt);
	rname = MakeRetrieveViewRuleName(viewName);

	attr = makeNode(Attr);
	attr->relname = pstrdup(viewName);
	rule->rulename = pstrdup(rname);
	rule->whereClause = NULL;
	rule->event = CMD_SELECT;
	rule->object = attr;
	rule->instead = true;
	rule->actions = makeList1(viewParse);

	return rule;
}

static void
DefineViewRules(char *viewName, Query *viewParse)
{
	RuleStmt   *retrieve_rule = NULL;

#ifdef NOTYET
	RuleStmt   *replace_rule = NULL;
	RuleStmt   *append_rule = NULL;
	RuleStmt   *delete_rule = NULL;

#endif

	retrieve_rule = FormViewRetrieveRule(viewName, viewParse);

#ifdef NOTYET

	replace_rule = FormViewReplaceRule(viewName, viewParse);
	append_rule = FormViewAppendRule(viewName, viewParse);
	delete_rule = FormViewDeleteRule(viewName, viewParse);

#endif

	DefineQueryRewrite(retrieve_rule);

#ifdef NOTYET
	DefineQueryRewrite(replace_rule);
	DefineQueryRewrite(append_rule);
	DefineQueryRewrite(delete_rule);
#endif

}

/*---------------------------------------------------------------
 * UpdateRangeTableOfViewParse
 *
 * Update the range table of the given parsetree.
 * This update consists of adding two new entries IN THE BEGINNING
 * of the range table (otherwise the rule system will die a slow,
 * horrible and painful death, and we do not want that now, do we?)
 * one for the OLD relation and one for the NEW one (both of
 * them refer in fact to the "view" relation).
 *
 * Of course we must also increase the 'varnos' of all the Var nodes
 * by 2...
 *
 * These extra RT entries are not actually used in the query, obviously.
 * We add them so that views look the same as ON SELECT rules ---
 * the rule rewriter assumes that ALL rules have OLD and NEW RTEs.
 *
 * NOTE: these are destructive changes. It would be difficult to
 * make a complete copy of the parse tree and make the changes
 * in the copy.
 *---------------------------------------------------------------
 */
static void
UpdateRangeTableOfViewParse(char *viewName, Query *viewParse)
{
	List	   *new_rt;
	RangeTblEntry *rt_entry1,
			   *rt_entry2;

	/*
	 * create the 2 new range table entries and form the new range
	 * table... OLD first, then NEW....
	 */
	rt_entry1 = addRangeTableEntry(NULL, viewName,
								   makeAttr("*OLD*", NULL),
								   false, false);
	rt_entry2 = addRangeTableEntry(NULL, viewName,
								   makeAttr("*NEW*", NULL),
								   false, false);
	/* Must override addRangeTableEntry's default access-check flags */
	rt_entry1->checkForRead = false;
	rt_entry2->checkForRead = false;

	new_rt = lcons(rt_entry1, lcons(rt_entry2, viewParse->rtable));

	/*
	 * Now the tricky part.... Update the range table in place... Be
	 * careful here, or hell breaks loooooooooooooOOOOOOOOOOOOOOOOOOSE!
	 */
	viewParse->rtable = new_rt;

	/*
	 * now offset all var nodes by 2, and jointree RT indexes too.
	 */
	OffsetVarNodes((Node *) viewParse, 2, 0);
}

/*-------------------------------------------------------------------
 * DefineView
 *
 *		- takes a "viewname", "parsetree" pair and then
 *		1)		construct the "virtual" relation
 *		2)		commit the command but NOT the transaction,
 *				so that the relation exists
 *				before the rules are defined.
 *		2)		define the "n" rules specified in the PRS2 paper
 *				over the "virtual" relation
 *-------------------------------------------------------------------
 */
void
DefineView(char *viewName, Query *viewParse)
{
	List	   *viewTlist;

	viewTlist = viewParse->targetList;

	/*
	 * Create the "view" relation NOTE: if it already exists, the xact
	 * will be aborted.
	 */
	DefineVirtualRelation(viewName, viewTlist);

	/*
	 * The relation we have just created is not visible to any other
	 * commands running with the same transaction & command id. So,
	 * increment the command id counter (but do NOT pfree any memory!!!!)
	 */
	CommandCounterIncrement();

	/*
	 * The range table of 'viewParse' does not contain entries for the
	 * "OLD" and "NEW" relations. So... add them! NOTE: we make the
	 * update in place! After this call 'viewParse' will never be what it
	 * used to be...
	 */
	UpdateRangeTableOfViewParse(viewName, viewParse);
	DefineViewRules(viewName, viewParse);
}

/*------------------------------------------------------------------
 * RemoveView
 *
 * Remove a view given its name
 *------------------------------------------------------------------
 */
void
RemoveView(char *viewName)
{
	/*
	 * We just have to drop the relation; the associated rules will
	 * be cleaned up automatically.
	 */
	heap_drop_with_catalog(viewName, allowSystemTableMods);
}
