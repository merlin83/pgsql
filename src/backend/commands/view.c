/*-------------------------------------------------------------------------
 *
 * view.c
 *	  use rewrite rules to construct views
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$Id: view.c,v 1.46 2000-09-12 04:15:56 momjian Exp $
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
	CreateStmt	createStmt;
	List	   *attrList,
			   *t;
	TargetEntry *entry;
	Resdom	   *res;
	char	   *resname;
	char	   *restypename;

	/*
	 * create a list with one entry per attribute of this relation. Each
	 * entry is a two element list. The first element is the name of the
	 * attribute (a string) and the second the name of the type (NOTE: a
	 * string, not a type id!).
	 */
	attrList = NIL;
	if (tlist != NIL)
	{
		foreach(t, tlist)
		{
			ColumnDef  *def = makeNode(ColumnDef);
			TypeName   *typename;

			/*
			 * find the names of the attribute & its type
			 */
			entry = lfirst(t);
			res = entry->resdom;
			resname = res->resname;
			restypename = typeidTypeName(res->restype);

			typename = makeNode(TypeName);

			typename->name = pstrdup(restypename);
			typename->typmod = res->restypmod;

			def->colname = pstrdup(resname);

			def->typename = typename;

			def->is_not_null = false;
			def->raw_default = NULL;
			def->cooked_default = NULL;

			attrList = lappend(attrList, def);
		}
	}
	else
		elog(ERROR, "attempted to define virtual relation with no attrs");

	/*
	 * now create the parametesr for keys/inheritance etc. All of them are
	 * nil...
	 */
	createStmt.relname = relname;
	createStmt.istemp = false;
	createStmt.tableElts = attrList;
/*	  createStmt.tableType = NULL;*/
	createStmt.inhRelnames = NIL;
	createStmt.constraints = NIL;

	/*
	 * finally create the relation...
	 */
	DefineRelation(&createStmt, RELKIND_RELATION);
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

	buf = palloc(strlen(viewName) + 5);
	snprintf(buf, strlen(viewName) + 5, "_RET%s", viewName);

#ifdef MULTIBYTE
	int len;
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
/*	  attr->refname = pstrdup(viewName);*/
	rule->rulename = pstrdup(rname);
	rule->whereClause = NULL;
	rule->event = CMD_SELECT;
	rule->object = attr;
	rule->instead = true;
	rule->actions = lcons(viewParse, NIL);

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
 * NOTE: these are destructive changes. It would be difficult to
 * make a complete copy of the parse tree and make the changes
 * in the copy.
 *---------------------------------------------------------------
 */
static void
UpdateRangeTableOfViewParse(char *viewName, Query *viewParse)
{
	List	   *old_rt;
	List	   *new_rt;
	RangeTblEntry *rt_entry1,
			   *rt_entry2;

	/*
	 * first offset all var nodes by 2
	 */
	OffsetVarNodes((Node *) viewParse->targetList, 2, 0);
	OffsetVarNodes(viewParse->qual, 2, 0);

	OffsetVarNodes(viewParse->havingQual, 2, 0);


	/*
	 * find the old range table...
	 */
	old_rt = viewParse->rtable;

	/*
	 * create the 2 new range table entries and form the new range
	 * table... OLD first, then NEW....
	 */
	rt_entry1 = addRangeTableEntry(NULL, (char *) viewName,
								   makeAttr("*OLD*", NULL),
								   FALSE, FALSE, FALSE);
	rt_entry2 = addRangeTableEntry(NULL, (char *) viewName,
								   makeAttr("*NEW*", NULL),
								   FALSE, FALSE, FALSE);
	new_rt = lcons(rt_entry2, old_rt);
	new_rt = lcons(rt_entry1, new_rt);

	/*
	 * Now the tricky part.... Update the range table in place... Be
	 * careful here, or hell breaks loooooooooooooOOOOOOOOOOOOOOOOOOSE!
	 */
	viewParse->rtable = new_rt;
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
	 * Create the "view" relation NOTE: if it already exists, the xaxt
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
