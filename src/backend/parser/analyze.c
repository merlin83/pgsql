/*-------------------------------------------------------------------------
 *
 * analyze.c
 *	  transform the parse tree into a query tree
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$Id: analyze.c,v 1.159 2000-09-29 18:21:36 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_index.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "parser/analyze.h"
#include "parser/parse.h"
#include "parser/parse_agg.h"
#include "parser/parse_clause.h"
#include "parser/parse_relation.h"
#include "parser/parse_target.h"
#include "parser/parse_type.h"
#include "rewrite/rewriteManip.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/relcache.h"
#include "utils/syscache.h"

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"
#endif

void		CheckSelectForUpdate(Query *qry);	/* no points for style... */

static Query *transformStmt(ParseState *pstate, Node *stmt);
static Query *transformDeleteStmt(ParseState *pstate, DeleteStmt *stmt);
static Query *transformInsertStmt(ParseState *pstate, InsertStmt *stmt);
static Query *transformIndexStmt(ParseState *pstate, IndexStmt *stmt);
static Query *transformExtendStmt(ParseState *pstate, ExtendStmt *stmt);
static Query *transformRuleStmt(ParseState *query, RuleStmt *stmt);
static Query *transformSelectStmt(ParseState *pstate, SelectStmt *stmt);
static Query *transformUpdateStmt(ParseState *pstate, UpdateStmt *stmt);
static Query *transformCursorStmt(ParseState *pstate, SelectStmt *stmt);
static Query *transformCreateStmt(ParseState *pstate, CreateStmt *stmt);
static Query *transformAlterTableStmt(ParseState *pstate, AlterTableStmt *stmt);

static void transformForUpdate(Query *qry, List *forUpdate);
static void transformFkeyGetPrimaryKey(FkConstraint *fkconstraint);
static void transformConstraintAttrs(List *constraintList);
static void transformColumnType(ParseState *pstate, ColumnDef *column);
static void transformFkeyCheckAttrs(FkConstraint *fkconstraint);

static void release_pstate_resources(ParseState *pstate);
static FromExpr *makeFromExpr(List *fromlist, Node *quals);

/* kluge to return extra info from transformCreateStmt() */
static List *extras_before;
static List *extras_after;


/*
 * parse_analyze -
 *	  analyze a list of parse trees and transform them if necessary.
 *
 * Returns a list of transformed parse trees. Optimizable statements are
 * all transformed to Query while the rest stays the same.
 *
 */
List *
parse_analyze(List *pl, ParseState *parentParseState)
{
	List	   *result = NIL;

	while (pl != NIL)
	{
		ParseState *pstate = make_parsestate(parentParseState);
		Query	   *parsetree;

		extras_before = extras_after = NIL;

		parsetree = transformStmt(pstate, lfirst(pl));
		release_pstate_resources(pstate);

		while (extras_before != NIL)
		{
			result = lappend(result,
							 transformStmt(pstate, lfirst(extras_before)));
			release_pstate_resources(pstate);
			extras_before = lnext(extras_before);
		}

		result = lappend(result, parsetree);

		while (extras_after != NIL)
		{
			result = lappend(result,
							 transformStmt(pstate, lfirst(extras_after)));
			release_pstate_resources(pstate);
			extras_after = lnext(extras_after);
		}

		pfree(pstate);
		pl = lnext(pl);
	}

	return result;
}

static void
release_pstate_resources(ParseState *pstate)
{
	if (pstate->p_target_relation != NULL)
		heap_close(pstate->p_target_relation, AccessShareLock);
	pstate->p_target_relation = NULL;
	pstate->p_target_rangetblentry = NULL;
}

/*
 * transformStmt -
 *	  transform a Parse tree. If it is an optimizable statement, turn it
 *	  into a Query tree.
 */
static Query *
transformStmt(ParseState *pstate, Node *parseTree)
{
	Query	   *result = NULL;

	switch (nodeTag(parseTree))
	{
			/*------------------------
			 *	Non-optimizable statements
			 *------------------------
			 */
		case T_CreateStmt:
			result = transformCreateStmt(pstate, (CreateStmt *) parseTree);
			break;

		case T_IndexStmt:
			result = transformIndexStmt(pstate, (IndexStmt *) parseTree);
			break;

		case T_ExtendStmt:
			result = transformExtendStmt(pstate, (ExtendStmt *) parseTree);
			break;

		case T_RuleStmt:
			result = transformRuleStmt(pstate, (RuleStmt *) parseTree);
			break;

		case T_ViewStmt:
			{
				ViewStmt   *n = (ViewStmt *) parseTree;

				n->query = (Query *) transformStmt(pstate, (Node *) n->query);

				/*
				 * If a list of column names was given, run through and
				 * insert these into the actual query tree. - thomas
				 * 2000-03-08
				 */
				if (n->aliases != NIL)
				{
					int			i;
					List	   *targetList = n->query->targetList;

					if (length(targetList) < length(n->aliases))
						elog(ERROR, "CREATE VIEW specifies %d columns"
							 " but only %d columns are present",
							 length(targetList), length(n->aliases));

					for (i = 0; i < length(n->aliases); i++)
					{
						Ident	   *id;
						TargetEntry *te;
						Resdom	   *rd;

						id = nth(i, n->aliases);
						Assert(IsA(id, Ident));
						te = nth(i, targetList);
						Assert(IsA(te, TargetEntry));
						rd = te->resdom;
						Assert(IsA(rd, Resdom));
						rd->resname = pstrdup(id->name);
					}
				}
				result = makeNode(Query);
				result->commandType = CMD_UTILITY;
				result->utilityStmt = (Node *) n;
			}
			break;

		case T_ExplainStmt:
			{
				ExplainStmt *n = (ExplainStmt *) parseTree;

				result = makeNode(Query);
				result->commandType = CMD_UTILITY;
				n->query = transformStmt(pstate, (Node *) n->query);
				result->utilityStmt = (Node *) parseTree;
			}
			break;

		case T_AlterTableStmt:
			result = transformAlterTableStmt(pstate, (AlterTableStmt *) parseTree);
			break;

		case T_SetSessionStmt:
			{
				List *l;
				/* Session is a list of SetVariable nodes
				 * so just run through the list.
				 */
				SetSessionStmt *stmt = (SetSessionStmt *) parseTree;

				l = stmt->args;
				/* First check for duplicate keywords (disallowed by SQL99) */
				while (l != NULL)
				{
					VariableSetStmt *v = (VariableSetStmt *) lfirst(l);
					List *ll = lnext(l);
					while (ll != NULL)
					{
						VariableSetStmt *vv = (VariableSetStmt *) lfirst(ll);
						if (strcmp(v->name, vv->name) == 0)
							elog(ERROR, "SET SESSION CHARACTERISTICS duplicated entry not allowed");
						ll = lnext(ll);
					}
					l = lnext(l);
				}

				l = stmt->args;
				result = transformStmt(pstate, lfirst(l));
				l = lnext(l);
				if (l != NULL)
					extras_after = lappend(extras_after, lfirst(l));
			}
			break;

			/*------------------------
			 *	Optimizable statements
			 *------------------------
			 */
		case T_InsertStmt:
			result = transformInsertStmt(pstate, (InsertStmt *) parseTree);
			break;

		case T_DeleteStmt:
			result = transformDeleteStmt(pstate, (DeleteStmt *) parseTree);
			break;

		case T_UpdateStmt:
			result = transformUpdateStmt(pstate, (UpdateStmt *) parseTree);
			break;

		case T_SelectStmt:
			if (!((SelectStmt *) parseTree)->portalname)
			{
				result = transformSelectStmt(pstate, (SelectStmt *) parseTree);
				result->limitOffset = ((SelectStmt *) parseTree)->limitOffset;
				result->limitCount = ((SelectStmt *) parseTree)->limitCount;
			}
			else
				result = transformCursorStmt(pstate, (SelectStmt *) parseTree);
			break;

		default:

			/*
			 * other statments don't require any transformation-- just
			 * return the original parsetree, yea!
			 */
			result = makeNode(Query);
			result->commandType = CMD_UTILITY;
			result->utilityStmt = (Node *) parseTree;
			break;
	}
	return result;
}

/*
 * transformDeleteStmt -
 *	  transforms a Delete Statement
 */
static Query *
transformDeleteStmt(ParseState *pstate, DeleteStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	Node	   *qual;

	qry->commandType = CMD_DELETE;

	/* set up a range table */
	makeRangeTable(pstate, NIL);
	setTargetTable(pstate, stmt->relname, stmt->inh, true);

	qry->distinctClause = NIL;

	/* fix where clause */
	qual = transformWhereClause(pstate, stmt->whereClause);

	/* done building the range table and jointree */
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);
	qry->resultRelation = refnameRangeTablePosn(pstate, stmt->relname, NULL);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs)
		parseCheckAggregates(pstate, qry, qual);

	return (Query *) qry;
}

/*
 * transformInsertStmt -
 *	  transform an Insert Statement
 */
static Query *
transformInsertStmt(ParseState *pstate, InsertStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	Node	   *qual;
	List	   *icolumns;
	List	   *attrnos;
	List	   *attnos;
	int			numuseratts;
	List	   *tl;
	TupleDesc	rd_att;

	qry->commandType = CMD_INSERT;
	pstate->p_is_insert = true;

	/*----------
	 * Initial processing steps are just like SELECT, which should not
	 * be surprising, since we may be handling an INSERT ... SELECT.
	 * It is important that we finish processing all the SELECT subclauses
	 * before we start doing any INSERT-specific processing; otherwise
	 * the behavior of SELECT within INSERT might be different from a
	 * stand-alone SELECT.	(Indeed, Postgres up through 6.5 had bugs of
	 * just that nature...)
	 *----------
	 */

	/* set up a range table --- note INSERT target is not in it yet */
	makeRangeTable(pstate, stmt->fromClause);

	qry->targetList = transformTargetList(pstate, stmt->targetList);

	qual = transformWhereClause(pstate, stmt->whereClause);

	/*
	 * Initial processing of HAVING clause is just like WHERE clause.
	 * Additional work will be done in optimizer/plan/planner.c.
	 */
	qry->havingQual = transformWhereClause(pstate, stmt->havingClause);

	qry->groupClause = transformGroupClause(pstate,
											stmt->groupClause,
											qry->targetList);

	/* An InsertStmt has no sortClause */
	qry->sortClause = NIL;

	qry->distinctClause = transformDistinctClause(pstate,
												  stmt->distinctClause,
												  qry->targetList,
												  &qry->sortClause);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs || qry->groupClause || qry->havingQual)
		parseCheckAggregates(pstate, qry, qual);

	/*
	 * The INSERT INTO ... SELECT ... could have a UNION in child, so
	 * unionClause may be false
	 */
	qry->unionall = stmt->unionall;

	/*
	 * Just hand through the unionClause and intersectClause. We will
	 * handle it in the function Except_Intersect_Rewrite()
	 */
	qry->unionClause = stmt->unionClause;
	qry->intersectClause = stmt->intersectClause;

	/*
	 * Now we are done with SELECT-like processing, and can get on with
	 * transforming the target list to match the INSERT target columns.
	 *
	 * In particular, it's time to add the INSERT target to the rangetable.
	 * (We didn't want it there until now since it shouldn't be visible in
	 * the SELECT part.)  Note that the INSERT target is NOT added to the
	 * joinlist, since we don't want to join over it.
	 */
	setTargetTable(pstate, stmt->relname, false, false);

	/* now the range table and jointree will not change */
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);
	qry->resultRelation = refnameRangeTablePosn(pstate, stmt->relname, NULL);

	/* Prepare to assign non-conflicting resnos to resjunk attributes */
	if (pstate->p_last_resno <= pstate->p_target_relation->rd_rel->relnatts)
		pstate->p_last_resno = pstate->p_target_relation->rd_rel->relnatts + 1;

	/* Validate stmt->cols list, or build default list if no list given */
	icolumns = checkInsertTargets(pstate, stmt->cols, &attrnos);

	/* Prepare non-junk columns for assignment to target table */
	numuseratts = 0;
	attnos = attrnos;
	foreach(tl, qry->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tl);
		Resdom	   *resnode = tle->resdom;
		Ident	   *id;

		if (resnode->resjunk)
		{

			/*
			 * Resjunk nodes need no additional processing, but be sure
			 * they have names and resnos that do not match any target
			 * columns; else rewriter or planner might get confused.
			 */
			resnode->resname = "?resjunk?";
			resnode->resno = (AttrNumber) pstate->p_last_resno++;
			continue;
		}
		if (icolumns == NIL || attnos == NIL)
			elog(ERROR, "INSERT has more expressions than target columns");
		id = (Ident *) lfirst(icolumns);
		updateTargetListEntry(pstate, tle, id->name, lfirsti(attnos),
							  id->indirection);
		numuseratts++;
		icolumns = lnext(icolumns);
		attnos = lnext(attnos);
	}

	/*
	 * It is possible that the targetlist has fewer entries than were in
	 * the columns list.  We do not consider this an error (perhaps we
	 * should, if the columns list was explictly given?).  We must
	 * truncate the attrnos list to only include the attrs actually
	 * provided, else we will fail to apply defaults for them below.
	 */
	if (icolumns != NIL)
		attrnos = ltruncate(numuseratts, attrnos);

	/*
	 * Add targetlist items to assign DEFAULT values to any columns that
	 * have defaults and were not assigned to by the user.
	 *
	 * XXX wouldn't it make more sense to do this further downstream, after
	 * the rule rewriter?
	 */
	rd_att = pstate->p_target_relation->rd_att;
	if (rd_att->constr && rd_att->constr->num_defval > 0)
	{
		Form_pg_attribute *att = rd_att->attrs;
		AttrDefault *defval = rd_att->constr->defval;
		int			ndef = rd_att->constr->num_defval;

		while (--ndef >= 0)
		{
			AttrNumber	attrno = defval[ndef].adnum;
			Form_pg_attribute thisatt = att[attrno - 1];
			TargetEntry *te;

			if (intMember((int) attrno, attrnos))
				continue;		/* there was a user-specified value */

			/*
			 * No user-supplied value, so add a targetentry with DEFAULT
			 * expr and correct data for the target column.
			 */
			te = makeTargetEntry(
								 makeResdom(attrno,
											thisatt->atttypid,
											thisatt->atttypmod,
									  pstrdup(NameStr(thisatt->attname)),
											false),
								 stringToNode(defval[ndef].adbin));
			qry->targetList = lappend(qry->targetList, te);

			/*
			 * Make sure the value is coerced to the target column type
			 * (might not be right type if it's not a constant!)
			 */
			updateTargetListEntry(pstate, te, te->resdom->resname, attrno,
								  NIL);
		}
	}

	if (stmt->forUpdate != NULL)
		transformForUpdate(qry, stmt->forUpdate);

	/* in case of subselects in default clauses... */
	qry->hasSubLinks = pstate->p_hasSubLinks;

	return (Query *) qry;
}

/*
 *	makeObjectName()
 *
 *	Create a name for an implicitly created index, sequence, constraint, etc.
 *
 *	The parameters are: the original table name, the original field name, and
 *	a "type" string (such as "seq" or "pkey").	The field name and/or type
 *	can be NULL if not relevant.
 *
 *	The result is a palloc'd string.
 *
 *	The basic result we want is "name1_name2_type", omitting "_name2" or
 *	"_type" when those parameters are NULL.  However, we must generate
 *	a name with less than NAMEDATALEN characters!  So, we truncate one or
 *	both names if necessary to make a short-enough string.	The type part
 *	is never truncated (so it had better be reasonably short).
 *
 *	To reduce the probability of collisions, we might someday add more
 *	smarts to this routine, like including some "hash" characters computed
 *	from the truncated characters.	Currently it seems best to keep it simple,
 *	so that the generated names are easily predictable by a person.
 */
static char *
makeObjectName(char *name1, char *name2, char *typename)
{
	char	   *name;
	int			overhead = 0;	/* chars needed for type and underscores */
	int			availchars;		/* chars available for name(s) */
	int			name1chars;		/* chars allocated to name1 */
	int			name2chars;		/* chars allocated to name2 */
	int			ndx;

	name1chars = strlen(name1);
	if (name2)
	{
		name2chars = strlen(name2);
		overhead++;				/* allow for separating underscore */
	}
	else
		name2chars = 0;
	if (typename)
		overhead += strlen(typename) + 1;

	availchars = NAMEDATALEN - 1 - overhead;

	/*
	 * If we must truncate,  preferentially truncate the longer name. This
	 * logic could be expressed without a loop, but it's simple and
	 * obvious as a loop.
	 */
	while (name1chars + name2chars > availchars)
	{
		if (name1chars > name2chars)
			name1chars--;
		else
			name2chars--;
	}

#ifdef MULTIBYTE
	if (name1)
		name1chars = pg_mbcliplen(name1, name1chars, name1chars);
	if (name2)
		name2chars = pg_mbcliplen(name2, name2chars, name2chars);
#endif

	/* Now construct the string using the chosen lengths */
	name = palloc(name1chars + name2chars + overhead + 1);
	strncpy(name, name1, name1chars);
	ndx = name1chars;
	if (name2)
	{
		name[ndx++] = '_';
		strncpy(name + ndx, name2, name2chars);
		ndx += name2chars;
	}
	if (typename)
	{
		name[ndx++] = '_';
		strcpy(name + ndx, typename);
	}
	else
		name[ndx] = '\0';

	return name;
}

static char *
CreateIndexName(char *table_name, char *column_name, char *label, List *indices)
{
	int			pass = 0;
	char	   *iname = NULL;
	List	   *ilist;
	char		typename[NAMEDATALEN];

	/*
	 * The type name for makeObjectName is label, or labelN if that's
	 * necessary to prevent collisions among multiple indexes for the same
	 * table.  Note there is no check for collisions with already-existing
	 * indexes; this ought to be rethought someday.
	 */
	strcpy(typename, label);

	for (;;)
	{
		iname = makeObjectName(table_name, column_name, typename);

		foreach(ilist, indices)
		{
			IndexStmt  *index = lfirst(ilist);

			if (strcmp(iname, index->idxname) == 0)
				break;
		}
		/* ran through entire list? then no name conflict found so done */
		if (ilist == NIL)
			break;

		/* the last one conflicted, so try a new name component */
		pfree(iname);
		sprintf(typename, "%s%d", label, ++pass);
	}

	return iname;
}

/*
 * transformCreateStmt -
 *	  transforms the "create table" statement
 *	  SQL92 allows constraints to be scattered all over, so thumb through
 *	   the columns and collect all constraints into one place.
 *	  If there are any implied indices (e.g. UNIQUE or PRIMARY KEY)
 *	   then expand those into multiple IndexStmt blocks.
 *	  - thomas 1997-12-02
 */
static Query *
transformCreateStmt(ParseState *pstate, CreateStmt *stmt)
{
	Query	   *q;
	List	   *elements;
	Node	   *element;
	List	   *columns;
	List	   *dlist;
	ColumnDef  *column;
	List	   *constraints,
			   *clist;
	Constraint *constraint;
	List	   *fkconstraints,	/* List of FOREIGN KEY constraints to */
			   *fkclist;		/* add finally */
	FkConstraint *fkconstraint;
	List	   *keys;
	Ident	   *key;
	List	   *blist = NIL;	/* "before list" of things to do before
								 * creating the table */
	List	   *ilist = NIL;	/* "index list" of things to do after
								 * creating the table */
	IndexStmt  *index,
			   *pkey = NULL;
	IndexElem  *iparam;
	bool		saw_nullable;

	q = makeNode(Query);
	q->commandType = CMD_UTILITY;

	fkconstraints = NIL;
	constraints = stmt->constraints;
	columns = NIL;
	dlist = NIL;

	/*
	 * Run through each primary element in the table creation clause
	 */
	foreach(elements, stmt->tableElts)
	{
		element = lfirst(elements);
		switch (nodeTag(element))
		{
			case T_ColumnDef:
				column = (ColumnDef *) element;
				columns = lappend(columns, column);

				transformColumnType(pstate, column);

				/* Special case SERIAL type? */
				if (column->is_sequence)
				{
					char	   *sname;
					char	   *qstring;
					A_Const    *snamenode;
					FuncCall   *funccallnode;
					CreateSeqStmt *sequence;

					/*
					 * Create appropriate constraints for SERIAL.  We do
					 * this in full, rather than shortcutting, so that we
					 * will detect any conflicting constraints the user
					 * wrote (like a different DEFAULT).
					 */
					sname = makeObjectName(stmt->relname, column->colname,
										   "seq");

					/*
					 * Create an expression tree representing the function
					 * call  nextval('"sequencename"')
					 */
					qstring = palloc(strlen(sname) + 2 + 1);
					sprintf(qstring, "\"%s\"", sname);
					snamenode = makeNode(A_Const);
					snamenode->val.type = T_String;
					snamenode->val.val.str = qstring;
					funccallnode = makeNode(FuncCall);
					funccallnode->funcname = "nextval";
					funccallnode->args = makeList1(snamenode);
					funccallnode->agg_star = false;
					funccallnode->agg_distinct = false;

					constraint = makeNode(Constraint);
					constraint->contype = CONSTR_DEFAULT;
					constraint->name = sname;
					constraint->raw_expr = (Node *) funccallnode;
					constraint->cooked_expr = NULL;
					constraint->keys = NIL;
					column->constraints = lappend(column->constraints,
												  constraint);

					constraint = makeNode(Constraint);
					constraint->contype = CONSTR_UNIQUE;
					constraint->name = makeObjectName(stmt->relname,
													  column->colname,
													  "key");
					column->constraints = lappend(column->constraints,
												  constraint);

					constraint = makeNode(Constraint);
					constraint->contype = CONSTR_NOTNULL;
					column->constraints = lappend(column->constraints,
												  constraint);

					sequence = makeNode(CreateSeqStmt);
					sequence->seqname = pstrdup(sname);
					sequence->options = NIL;

					elog(NOTICE, "CREATE TABLE will create implicit sequence '%s' for SERIAL column '%s.%s'",
					  sequence->seqname, stmt->relname, column->colname);

					blist = makeList1(sequence);
				}

				/* Process column constraints, if any... */
				transformConstraintAttrs(column->constraints);

				saw_nullable = false;

				foreach(clist, column->constraints)
				{
					constraint = lfirst(clist);

					/* ----------
					 * If this column constraint is a FOREIGN KEY
					 * constraint, then we fill in the current attributes
					 * name and throw it into the list of FK constraints
					 * to be processed later.
					 * ----------
					 */
					if (IsA(constraint, FkConstraint))
					{
						Ident	   *id = makeNode(Ident);

						id->name = column->colname;
						id->indirection = NIL;
						id->isRel = false;

						fkconstraint = (FkConstraint *) constraint;
						fkconstraint->fk_attrs = makeList1(id);

						fkconstraints = lappend(fkconstraints, constraint);
						continue;
					}

					switch (constraint->contype)
					{
						case CONSTR_NULL:
							if (saw_nullable && column->is_not_null)
								elog(ERROR, "CREATE TABLE/(NOT) NULL conflicting declaration"
									 " for '%s.%s'", stmt->relname, column->colname);
							column->is_not_null = FALSE;
							saw_nullable = true;
							break;

						case CONSTR_NOTNULL:
							if (saw_nullable && !column->is_not_null)
								elog(ERROR, "CREATE TABLE/(NOT) NULL conflicting declaration"
									 " for '%s.%s'", stmt->relname, column->colname);
							column->is_not_null = TRUE;
							saw_nullable = true;
							break;

						case CONSTR_DEFAULT:
							if (column->raw_default != NULL)
								elog(ERROR, "CREATE TABLE/DEFAULT multiple values specified"
									 " for '%s.%s'", stmt->relname, column->colname);
							column->raw_default = constraint->raw_expr;
							Assert(constraint->cooked_expr == NULL);
							break;

						case CONSTR_PRIMARY:
							if (constraint->name == NULL)
								constraint->name = makeObjectName(stmt->relname, NULL, "pkey");
							if (constraint->keys == NIL)
							{
								key = makeNode(Ident);
								key->name = pstrdup(column->colname);
								constraint->keys = makeList1(key);
							}
							dlist = lappend(dlist, constraint);
							break;

						case CONSTR_UNIQUE:
							if (constraint->name == NULL)
								constraint->name = makeObjectName(stmt->relname, column->colname, "key");
							if (constraint->keys == NIL)
							{
								key = makeNode(Ident);
								key->name = pstrdup(column->colname);
								constraint->keys = makeList1(key);
							}
							dlist = lappend(dlist, constraint);
							break;

						case CONSTR_CHECK:
							if (constraint->name == NULL)
								constraint->name = makeObjectName(stmt->relname, column->colname, NULL);
							constraints = lappend(constraints, constraint);
							break;

						case CONSTR_ATTR_DEFERRABLE:
						case CONSTR_ATTR_NOT_DEFERRABLE:
						case CONSTR_ATTR_DEFERRED:
						case CONSTR_ATTR_IMMEDIATE:
							/* transformConstraintAttrs took care of these */
							break;

						default:
							elog(ERROR, "parser: unrecognized constraint (internal error)");
							break;
					}
				}
				break;

			case T_Constraint:
				constraint = (Constraint *) element;
				switch (constraint->contype)
				{
					case CONSTR_PRIMARY:
						if (constraint->name == NULL)
							constraint->name = makeObjectName(stmt->relname, NULL, "pkey");
						dlist = lappend(dlist, constraint);
						break;

					case CONSTR_UNIQUE:
						dlist = lappend(dlist, constraint);
						break;

					case CONSTR_CHECK:
						constraints = lappend(constraints, constraint);
						break;

					case CONSTR_NULL:
					case CONSTR_NOTNULL:
					case CONSTR_DEFAULT:
					case CONSTR_ATTR_DEFERRABLE:
					case CONSTR_ATTR_NOT_DEFERRABLE:
					case CONSTR_ATTR_DEFERRED:
					case CONSTR_ATTR_IMMEDIATE:
						elog(ERROR, "parser: illegal context for constraint (internal error)");
						break;

					default:
						elog(ERROR, "parser: unrecognized constraint (internal error)");
						break;
				}
				break;

			case T_FkConstraint:
				/* ----------
				 * Table level FOREIGN KEY constraints are already complete.
				 * Just remember for later.
				 * ----------
				 */
				fkconstraints = lappend(fkconstraints, element);
				break;

			default:
				elog(ERROR, "parser: unrecognized node (internal error)");
		}
	}

	stmt->tableElts = columns;
	stmt->constraints = constraints;

/* Now run through the "deferred list" to complete the query transformation.
 * For PRIMARY KEYs, mark each column as NOT NULL and create an index.
 * For UNIQUE, create an index as for PRIMARY KEYS, but do not insist on NOT NULL.
 *
 * Note that this code does not currently look for all possible redundant cases
 *	and either ignore or stop with warning. The create might fail later when
 *	names for indices turn out to be duplicated, or a user might have specified
 *	extra useless indices which might hurt performance. - thomas 1997-12-08
 */
	while (dlist != NIL)
	{
		constraint = lfirst(dlist);
		Assert(IsA(constraint, Constraint));
		Assert((constraint->contype == CONSTR_PRIMARY)
			   || (constraint->contype == CONSTR_UNIQUE));

		index = makeNode(IndexStmt);

		index->unique = TRUE;
		index->primary = (constraint->contype == CONSTR_PRIMARY ? TRUE : FALSE);
		if (index->primary)
		{
			if (pkey != NULL)
				elog(ERROR, "CREATE TABLE/PRIMARY KEY multiple primary keys"
					 " for table '%s' are not allowed", stmt->relname);
			pkey = (IndexStmt *) index;
		}

		if (constraint->name != NULL)
			index->idxname = pstrdup(constraint->name);
		else if (constraint->contype == CONSTR_PRIMARY)
			index->idxname = makeObjectName(stmt->relname, NULL, "pkey");
		else
			index->idxname = NULL;

		index->relname = stmt->relname;
		index->accessMethod = "btree";
		index->indexParams = NIL;
		index->withClause = NIL;
		index->whereClause = NULL;

		foreach(keys, constraint->keys)
		{
			key = (Ident *) lfirst(keys);
			Assert(IsA(key, Ident));
			column = NULL;
			foreach(columns, stmt->tableElts)
			{
				column = lfirst(columns);
				Assert(IsA(column, ColumnDef));
				if (strcmp(column->colname, key->name) == 0)
					break;
			}
			if (columns == NIL) /* fell off end of list? */
				elog(ERROR, "CREATE TABLE: column '%s' named in key does not exist",
					 key->name);

			if (constraint->contype == CONSTR_PRIMARY)
				column->is_not_null = TRUE;
			iparam = makeNode(IndexElem);
			iparam->name = pstrdup(column->colname);
			iparam->args = NIL;
			iparam->class = NULL;
			index->indexParams = lappend(index->indexParams, iparam);

			if (index->idxname == NULL)
				index->idxname = CreateIndexName(stmt->relname, iparam->name, "key", ilist);
		}

		if (index->idxname == NULL)		/* should not happen */
			elog(ERROR, "CREATE TABLE: failed to make implicit index name");

		ilist = lappend(ilist, index);
		dlist = lnext(dlist);
	}

/* OK, now finally, if there is a primary key, then make sure that there aren't any redundant
 * unique indices defined on columns. This can arise if someone specifies UNIQUE explicitly
 * or if a SERIAL column was defined along with a table PRIMARY KEY constraint.
 * - thomas 1999-05-11
 */
	if (pkey != NULL)
	{
		dlist = ilist;
		ilist = NIL;
		while (dlist != NIL)
		{
			List	   *pcols,
					   *icols;
			int			plen,
						ilen;
			int			keep = TRUE;

			index = lfirst(dlist);
			pcols = pkey->indexParams;
			icols = index->indexParams;

			plen = length(pcols);
			ilen = length(icols);

			/* Not the same as the primary key? Then we should look... */
			if ((index != pkey) && (ilen == plen))
			{
				keep = FALSE;
				while ((pcols != NIL) && (icols != NIL))
				{
					IndexElem  *pcol = lfirst(pcols);
					IndexElem  *icol = lfirst(icols);
					char	   *pname = pcol->name;
					char	   *iname = icol->name;

					/* different names? then no match... */
					if (strcmp(iname, pname) != 0)
					{
						keep = TRUE;
						break;
					}
					pcols = lnext(pcols);
					icols = lnext(icols);
				}
			}

			if (keep)
				ilist = lappend(ilist, index);
			dlist = lnext(dlist);
		}
	}

	dlist = ilist;
	while (dlist != NIL)
	{
		index = lfirst(dlist);
		elog(NOTICE, "CREATE TABLE/%s will create implicit index '%s' for table '%s'",
			 (index->primary ? "PRIMARY KEY" : "UNIQUE"),
			 index->idxname, stmt->relname);
		dlist = lnext(dlist);
	}

	q->utilityStmt = (Node *) stmt;
	extras_before = blist;
	extras_after = ilist;

	/*
	 * Now process the FOREIGN KEY constraints and add appropriate queries
	 * to the extras_after statements list.
	 *
	 */
	if (fkconstraints != NIL)
	{
		CreateTrigStmt *fk_trigger;
		List	   *fk_attr;
		List	   *pk_attr;
		Ident	   *id;

		elog(NOTICE, "CREATE TABLE will create implicit trigger(s) for FOREIGN KEY check(s)");

		foreach(fkclist, fkconstraints)
		{
			fkconstraint = (FkConstraint *) lfirst(fkclist);

			/*
			 * If the constraint has no name, set it to <unnamed>
			 *
			 */
			if (fkconstraint->constr_name == NULL)
				fkconstraint->constr_name = "<unnamed>";

			/* 
			 * Check to see if the attributes mentioned by the constraint
			 * actually exist on this table.
			 */
			if (fkconstraint->fk_attrs!=NIL) {
				int found=0;
				List *cols;
				List *fkattrs;
				Ident *fkattr;
				ColumnDef *col;
				foreach(fkattrs, fkconstraint->fk_attrs) {
					found=0;
					fkattr=lfirst(fkattrs);
					foreach(cols, stmt->tableElts) {
						col=lfirst(cols);
						if (strcmp(col->colname, fkattr->name)==0) {
							found=1;
							break;
						}
					}
					if (!found)
						break;
				}
				if (!found)
					elog(ERROR, "columns referenced in foreign key constraint not found.");
			}

			/*
			 * If the attribute list for the referenced table was omitted,
			 * lookup for the definition of the primary key. If the
			 * referenced table is this table, use the definition we found
			 * above, rather than looking to the system tables.
			 *
			 */
			if (fkconstraint->fk_attrs != NIL && fkconstraint->pk_attrs == NIL)
			{
				if (strcmp(fkconstraint->pktable_name, stmt->relname) != 0)
					transformFkeyGetPrimaryKey(fkconstraint);
				else if (pkey != NULL)
				{
					List	   *pkey_attr = pkey->indexParams;
					List	   *attr;
					IndexElem  *ielem;
					Ident	   *pkattr;

					foreach(attr, pkey_attr)
					{
						ielem = lfirst(attr);
						pkattr = (Ident *) makeNode(Ident);
						pkattr->name = pstrdup(ielem->name);
						pkattr->indirection = NIL;
						pkattr->isRel = false;
						fkconstraint->pk_attrs = lappend(fkconstraint->pk_attrs, pkattr);
					}
				}
				else
				{
					elog(ERROR, "PRIMARY KEY for referenced table \"%s\" not found",
						 fkconstraint->pktable_name);
				}
			}
			else {
				if (strcmp(fkconstraint->pktable_name, stmt->relname)!=0)
					transformFkeyCheckAttrs(fkconstraint);
				else {
					/* Get a unique/pk constraint from above */
					List *index;
					int found=0;
					foreach(index, ilist)
					{
						IndexStmt *ind=lfirst(index);
						IndexElem *indparm;
						List *indparms;
						List *pkattrs;
						Ident *pkattr;
						if (ind->unique) {
							foreach(pkattrs, fkconstraint->pk_attrs) {
								found=0;
								pkattr=lfirst(pkattrs);
								foreach(indparms, ind->indexParams) {
									indparm=lfirst(indparms);
									if (strcmp(indparm->name, pkattr->name)==0) {
										found=1;
										break;
									}
								}
								if (!found)
									break;
							}
						}
						if (found)
							break;
					}
					if (!found)
						elog(ERROR, "UNIQUE constraint matching given keys for referenced table \"%s\" not found",
							 fkconstraint->pktable_name);
				}
			}
			/*
			 * Build a CREATE CONSTRAINT TRIGGER statement for the CHECK
			 * action.
			 *
			 */
			fk_trigger = (CreateTrigStmt *) makeNode(CreateTrigStmt);
			fk_trigger->trigname = fkconstraint->constr_name;
			fk_trigger->relname = stmt->relname;
			fk_trigger->funcname = "RI_FKey_check_ins";
			fk_trigger->before = false;
			fk_trigger->row = true;
			fk_trigger->actions[0] = 'i';
			fk_trigger->actions[1] = 'u';
			fk_trigger->actions[2] = '\0';
			fk_trigger->lang = NULL;
			fk_trigger->text = NULL;

			fk_trigger->attr = NIL;
			fk_trigger->when = NULL;
			fk_trigger->isconstraint = true;
			fk_trigger->deferrable = fkconstraint->deferrable;
			fk_trigger->initdeferred = fkconstraint->initdeferred;
			fk_trigger->constrrelname = fkconstraint->pktable_name;

			fk_trigger->args = NIL;
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->constr_name));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(stmt->relname));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->pktable_name));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->match_type));
			fk_attr = fkconstraint->fk_attrs;
			pk_attr = fkconstraint->pk_attrs;
			if (length(fk_attr) != length(pk_attr))
			{
				elog(NOTICE, "Illegal FOREIGN KEY definition REFERENCES \"%s\"",
					 fkconstraint->pktable_name);
				elog(ERROR, "number of key attributes in referenced table must be equal to foreign key");
			}
			while (fk_attr != NIL)
			{
				id = (Ident *) lfirst(fk_attr);
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(id->name));

				id = (Ident *) lfirst(pk_attr);
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(id->name));

				fk_attr = lnext(fk_attr);
				pk_attr = lnext(pk_attr);
			}

			extras_after = lappend(extras_after, (Node *) fk_trigger);

			/*
			 * Build a CREATE CONSTRAINT TRIGGER statement for the ON
			 * DELETE action fired on the PK table !!!
			 *
			 */
			fk_trigger = (CreateTrigStmt *) makeNode(CreateTrigStmt);
			fk_trigger->trigname = fkconstraint->constr_name;
			fk_trigger->relname = fkconstraint->pktable_name;
			fk_trigger->before = false;
			fk_trigger->row = true;
			fk_trigger->actions[0] = 'd';
			fk_trigger->actions[1] = '\0';
			fk_trigger->lang = NULL;
			fk_trigger->text = NULL;

			fk_trigger->attr = NIL;
			fk_trigger->when = NULL;
			fk_trigger->isconstraint = true;
			fk_trigger->deferrable = fkconstraint->deferrable;
			fk_trigger->initdeferred = fkconstraint->initdeferred;
			fk_trigger->constrrelname = stmt->relname;
			switch ((fkconstraint->actions & FKCONSTR_ON_DELETE_MASK)
					>> FKCONSTR_ON_DELETE_SHIFT)
			{
				case FKCONSTR_ON_KEY_NOACTION:
					fk_trigger->funcname = "RI_FKey_noaction_del";
					break;
				case FKCONSTR_ON_KEY_RESTRICT:
					fk_trigger->deferrable = false;
					fk_trigger->initdeferred = false;
					fk_trigger->funcname = "RI_FKey_restrict_del";
					break;
				case FKCONSTR_ON_KEY_CASCADE:
					fk_trigger->funcname = "RI_FKey_cascade_del";
					break;
				case FKCONSTR_ON_KEY_SETNULL:
					fk_trigger->funcname = "RI_FKey_setnull_del";
					break;
				case FKCONSTR_ON_KEY_SETDEFAULT:
					fk_trigger->funcname = "RI_FKey_setdefault_del";
					break;
				default:
					elog(ERROR, "Only one ON DELETE action can be specified for FOREIGN KEY constraint");
					break;
			}

			fk_trigger->args = NIL;
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->constr_name));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(stmt->relname));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->pktable_name));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->match_type));
			fk_attr = fkconstraint->fk_attrs;
			pk_attr = fkconstraint->pk_attrs;
			while (fk_attr != NIL)
			{
				id = (Ident *) lfirst(fk_attr);
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(id->name));

				id = (Ident *) lfirst(pk_attr);
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(id->name));

				fk_attr = lnext(fk_attr);
				pk_attr = lnext(pk_attr);
			}

			extras_after = lappend(extras_after, (Node *) fk_trigger);

			/*
			 * Build a CREATE CONSTRAINT TRIGGER statement for the ON
			 * UPDATE action fired on the PK table !!!
			 *
			 */
			fk_trigger = (CreateTrigStmt *) makeNode(CreateTrigStmt);
			fk_trigger->trigname = fkconstraint->constr_name;
			fk_trigger->relname = fkconstraint->pktable_name;
			fk_trigger->before = false;
			fk_trigger->row = true;
			fk_trigger->actions[0] = 'u';
			fk_trigger->actions[1] = '\0';
			fk_trigger->lang = NULL;
			fk_trigger->text = NULL;

			fk_trigger->attr = NIL;
			fk_trigger->when = NULL;
			fk_trigger->isconstraint = true;
			fk_trigger->deferrable = fkconstraint->deferrable;
			fk_trigger->initdeferred = fkconstraint->initdeferred;
			fk_trigger->constrrelname = stmt->relname;
			switch ((fkconstraint->actions & FKCONSTR_ON_UPDATE_MASK)
					>> FKCONSTR_ON_UPDATE_SHIFT)
			{
				case FKCONSTR_ON_KEY_NOACTION:
					fk_trigger->funcname = "RI_FKey_noaction_upd";
					break;
				case FKCONSTR_ON_KEY_RESTRICT:
					fk_trigger->deferrable = false;
					fk_trigger->initdeferred = false;
					fk_trigger->funcname = "RI_FKey_restrict_upd";
					break;
				case FKCONSTR_ON_KEY_CASCADE:
					fk_trigger->funcname = "RI_FKey_cascade_upd";
					break;
				case FKCONSTR_ON_KEY_SETNULL:
					fk_trigger->funcname = "RI_FKey_setnull_upd";
					break;
				case FKCONSTR_ON_KEY_SETDEFAULT:
					fk_trigger->funcname = "RI_FKey_setdefault_upd";
					break;
				default:
					elog(ERROR, "Only one ON UPDATE action can be specified for FOREIGN KEY constraint");
					break;
			}

			fk_trigger->args = NIL;
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->constr_name));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(stmt->relname));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->pktable_name));
			fk_trigger->args = lappend(fk_trigger->args,
									   makeString(fkconstraint->match_type));
			fk_attr = fkconstraint->fk_attrs;
			pk_attr = fkconstraint->pk_attrs;
			while (fk_attr != NIL)
			{
				id = (Ident *) lfirst(fk_attr);
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(id->name));

				id = (Ident *) lfirst(pk_attr);
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(id->name));

				fk_attr = lnext(fk_attr);
				pk_attr = lnext(pk_attr);
			}

			extras_after = lappend(extras_after, (Node *) fk_trigger);
		}
	}

	return q;
}	/* transformCreateStmt() */


/*
 * transformIndexStmt -
 *	  transforms the qualification of the index statement
 */
static Query *
transformIndexStmt(ParseState *pstate, IndexStmt *stmt)
{
	Query	   *qry;

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;

	/* take care of the where clause */
	stmt->whereClause = transformWhereClause(pstate, stmt->whereClause);

	qry->hasSubLinks = pstate->p_hasSubLinks;

	stmt->rangetable = pstate->p_rtable;

	qry->utilityStmt = (Node *) stmt;

	return qry;
}

/*
 * transformExtendStmt -
 *	  transform the qualifications of the Extend Index Statement
 *
 */
static Query *
transformExtendStmt(ParseState *pstate, ExtendStmt *stmt)
{
	Query	   *qry;

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;

	/* take care of the where clause */
	stmt->whereClause = transformWhereClause(pstate, stmt->whereClause);

	qry->hasSubLinks = pstate->p_hasSubLinks;

	stmt->rangetable = pstate->p_rtable;

	qry->utilityStmt = (Node *) stmt;
	return qry;
}

/*
 * transformRuleStmt -
 *	  transform a Create Rule Statement. The actions is a list of parse
 *	  trees which is transformed into a list of query trees.
 */
static Query *
transformRuleStmt(ParseState *pstate, RuleStmt *stmt)
{
	Query	   *qry;
	RangeTblEntry *oldrte;
	RangeTblEntry *newrte;

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;
	qry->utilityStmt = (Node *) stmt;

	/*
	 * NOTE: 'OLD' must always have a varno equal to 1 and 'NEW'
	 * equal to 2.  Set up their RTEs in the main pstate for use
	 * in parsing the rule qualification.
	 */
	Assert(pstate->p_rtable == NIL);
	oldrte = addRangeTableEntry(pstate, stmt->object->relname,
								makeAttr("*OLD*", NULL),
								false, true);
	newrte = addRangeTableEntry(pstate, stmt->object->relname,
								makeAttr("*NEW*", NULL),
								false, true);
	/* Must override addRangeTableEntry's default access-check flags */
	oldrte->checkForRead = false;
	newrte->checkForRead = false;
	/*
	 * They must be in the joinlist too for lookup purposes, but only add
	 * the one(s) that are relevant for the current kind of rule.  In an
	 * UPDATE rule, quals must refer to OLD.field or NEW.field to be
	 * unambiguous, but there's no need to be so picky for INSERT & DELETE.
	 * (Note we marked the RTEs "inFromCl = true" above to allow unqualified
	 * references to their fields.)
	 */
	switch (stmt->event)
	{
		case CMD_SELECT:
			addRTEtoJoinList(pstate, oldrte);
			break;
		case CMD_UPDATE:
			addRTEtoJoinList(pstate, oldrte);
			addRTEtoJoinList(pstate, newrte);
			break;
		case CMD_INSERT:
			addRTEtoJoinList(pstate, newrte);
			break;
		case CMD_DELETE:
			addRTEtoJoinList(pstate, oldrte);
			break;
		default:
			elog(ERROR, "transformRuleStmt: unexpected event type %d",
				 (int) stmt->event);
			break;
	}

	/* take care of the where clause */
	stmt->whereClause = transformWhereClause(pstate, stmt->whereClause);

	if (length(pstate->p_rtable) != 2) /* naughty, naughty... */
		elog(ERROR, "Rule WHERE condition may not contain references to other relations");

	/* save info about sublinks in where clause */
	qry->hasSubLinks = pstate->p_hasSubLinks;

	/*
	 * 'instead nothing' rules with a qualification need a query
	 * rangetable so the rewrite handler can add the negated rule
	 * qualification to the original query. We create a query with the new
	 * command type CMD_NOTHING here that is treated specially by the
	 * rewrite system.
	 */
	if (stmt->actions == NIL)
	{
		Query	   *nothing_qry = makeNode(Query);

		nothing_qry->commandType = CMD_NOTHING;
		nothing_qry->rtable = pstate->p_rtable;
		nothing_qry->jointree = makeFromExpr(NIL, NULL); /* no join wanted */

		stmt->actions = makeList1(nothing_qry);
	}
	else
	{
		List	   *actions;

		/*
		 * transform each statement, like parse_analyze()
		 */
		foreach(actions, stmt->actions)
		{
			ParseState *sub_pstate = make_parsestate(pstate->parentParseState);
			Query	   *sub_qry;
			bool		has_old,
						has_new;

			/*
			 * Set up OLD/NEW in the rtable for this statement.  The entries
			 * are marked not inFromCl because we don't want them to be
			 * referred to by unqualified field names nor "*" in the rule
			 * actions.  We don't need to add them to the joinlist for
			 * qualified-name lookup, either (see qualifiedNameToVar()).
			 */
			oldrte = addRangeTableEntry(sub_pstate, stmt->object->relname,
										makeAttr("*OLD*", NULL),
										false, false);
			newrte = addRangeTableEntry(sub_pstate, stmt->object->relname,
										makeAttr("*NEW*", NULL),
										false, false);
			oldrte->checkForRead = false;
			newrte->checkForRead = false;

			/* Transform the rule action statement */
			sub_qry = transformStmt(sub_pstate, lfirst(actions));

			/*
			 * Validate action's use of OLD/NEW, qual too
			 */
			has_old =
				rangeTableEntry_used((Node *) sub_qry, PRS2_OLD_VARNO, 0) ||
				rangeTableEntry_used(stmt->whereClause, PRS2_OLD_VARNO, 0);
			has_new =
				rangeTableEntry_used((Node *) sub_qry, PRS2_NEW_VARNO, 0) ||
				rangeTableEntry_used(stmt->whereClause, PRS2_NEW_VARNO, 0);

			switch (stmt->event)
			{
				case CMD_SELECT:
					if (has_old)
						elog(ERROR, "ON SELECT rule may not use OLD");
					if (has_new)
						elog(ERROR, "ON SELECT rule may not use NEW");
					break;
				case CMD_UPDATE:
					/* both are OK */
					break;
				case CMD_INSERT:
					if (has_old)
						elog(ERROR, "ON INSERT rule may not use OLD");
					break;
				case CMD_DELETE:
					if (has_new)
						elog(ERROR, "ON DELETE rule may not use NEW");
					break;
				default:
					elog(ERROR, "transformRuleStmt: unexpected event type %d",
						 (int) stmt->event);
					break;
			}

			/*
			 * For efficiency's sake, add OLD to the rule action's jointree
			 * only if it was actually referenced in the statement or qual.
			 * NEW is not really a relation and should never be added.
			 */
			if (has_old)
			{
				addRTEtoJoinList(sub_pstate, oldrte);
				sub_qry->jointree->fromlist = sub_pstate->p_joinlist;
			}

			lfirst(actions) = sub_qry;

			release_pstate_resources(sub_pstate);
			pfree(sub_pstate);
		}
	}

	return qry;
}


/*
 * transformSelectStmt -
 *	  transforms a Select Statement
 *
 */
static Query *
transformSelectStmt(ParseState *pstate, SelectStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	Node	   *qual;

	qry->commandType = CMD_SELECT;

	/* set up a range table */
	makeRangeTable(pstate, stmt->fromClause);

	qry->into = stmt->into;
	qry->isTemp = stmt->istemp;
	qry->isPortal = FALSE;

	qry->targetList = transformTargetList(pstate, stmt->targetList);

	qual = transformWhereClause(pstate, stmt->whereClause);

	/*
	 * Initial processing of HAVING clause is just like WHERE clause.
	 * Additional work will be done in optimizer/plan/planner.c.
	 */
	qry->havingQual = transformWhereClause(pstate, stmt->havingClause);

	qry->groupClause = transformGroupClause(pstate,
											stmt->groupClause,
											qry->targetList);

	qry->sortClause = transformSortClause(pstate,
										  stmt->sortClause,
										  qry->targetList);

	qry->distinctClause = transformDistinctClause(pstate,
												  stmt->distinctClause,
												  qry->targetList,
												  &qry->sortClause);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs || qry->groupClause || qry->havingQual)
		parseCheckAggregates(pstate, qry, qual);

	/*
	 * The INSERT INTO ... SELECT ... could have a UNION in child, so
	 * unionClause may be false
	 */
	qry->unionall = stmt->unionall;

	/*
	 * Just hand through the unionClause and intersectClause. We will
	 * handle it in the function Except_Intersect_Rewrite()
	 */
	qry->unionClause = stmt->unionClause;
	qry->intersectClause = stmt->intersectClause;

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

	if (stmt->forUpdate != NULL)
		transformForUpdate(qry, stmt->forUpdate);

	return (Query *) qry;
}

/*
 * transformUpdateStmt -
 *	  transforms an update statement
 *
 */
static Query *
transformUpdateStmt(ParseState *pstate, UpdateStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	Node	   *qual;
	List	   *origTargetList;
	List	   *tl;

	qry->commandType = CMD_UPDATE;
	pstate->p_is_update = true;

	/*
	 * the FROM clause is non-standard SQL syntax. We used to be able to
	 * do this with REPLACE in POSTQUEL so we keep the feature.
	 *
	 * Note: it's critical here that we process FROM before adding the
	 * target table to the rtable --- otherwise, if the target is also
	 * used in FROM, we'd fail to notice that it should be marked
	 * checkForRead as well as checkForWrite.  See setTargetTable().
	 */
	makeRangeTable(pstate, stmt->fromClause);
	setTargetTable(pstate, stmt->relname, stmt->inh, true);

	qry->targetList = transformTargetList(pstate, stmt->targetList);

	qual = transformWhereClause(pstate, stmt->whereClause);

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);
	qry->resultRelation = refnameRangeTablePosn(pstate, stmt->relname, NULL);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs)
		parseCheckAggregates(pstate, qry, qual);

	/*
	 * Now we are done with SELECT-like processing, and can get on with
	 * transforming the target list to match the UPDATE target columns.
	 */

	/* Prepare to assign non-conflicting resnos to resjunk attributes */
	if (pstate->p_last_resno <= pstate->p_target_relation->rd_rel->relnatts)
		pstate->p_last_resno = pstate->p_target_relation->rd_rel->relnatts + 1;

	/* Prepare non-junk columns for assignment to target table */
	origTargetList = stmt->targetList;
	foreach(tl, qry->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tl);
		Resdom	   *resnode = tle->resdom;
		ResTarget  *origTarget;

		if (resnode->resjunk)
		{

			/*
			 * Resjunk nodes need no additional processing, but be sure
			 * they have names and resnos that do not match any target
			 * columns; else rewriter or planner might get confused.
			 */
			resnode->resname = "?resjunk?";
			resnode->resno = (AttrNumber) pstate->p_last_resno++;
			continue;
		}
		if (origTargetList == NIL)
			elog(ERROR, "UPDATE target count mismatch --- internal error");
		origTarget = (ResTarget *) lfirst(origTargetList);
		updateTargetListEntry(pstate, tle, origTarget->name,
							  attnameAttNum(pstate->p_target_relation,
											origTarget->name),
							  origTarget->indirection);
		origTargetList = lnext(origTargetList);
	}
	if (origTargetList != NIL)
		elog(ERROR, "UPDATE target count mismatch --- internal error");

	return (Query *) qry;
}

/*
 * transformCursorStmt -
 *	  transform a Create Cursor Statement
 *
 */
static Query *
transformCursorStmt(ParseState *pstate, SelectStmt *stmt)
{
	Query	   *qry;

	qry = transformSelectStmt(pstate, stmt);

	qry->into = stmt->portalname;
	qry->isTemp = stmt->istemp;
	qry->isPortal = TRUE;
	qry->isBinary = stmt->binary;		/* internal portal */

	return qry;
}

/*
 * tranformAlterTableStmt -
 *	transform an Alter Table Statement
 *
 */
static Query *
transformAlterTableStmt(ParseState *pstate, AlterTableStmt *stmt)
{
	Query	   *qry;

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;

	/*
	 * The only subtypes that currently have special handling are 'A'dd
	 * column and Add 'C'onstraint.  In addition, right now only Foreign
	 * Key 'C'onstraints have a special transformation.
	 *
	 */
	switch (stmt->subtype)
	{
		case 'A':
			transformColumnType(pstate, (ColumnDef *) stmt->def);
			break;
		case 'C':
			if (stmt->def && IsA(stmt->def, FkConstraint))
			{
				CreateTrigStmt *fk_trigger;
				List	   *fk_attr;
				List	   *pk_attr;
				Ident	   *id;
				FkConstraint *fkconstraint;

				extras_after = NIL;
				elog(NOTICE, "ALTER TABLE ... ADD CONSTRAINT will create implicit trigger(s) for FOREIGN KEY check(s)");

				fkconstraint = (FkConstraint *) stmt->def;

				/*
				 * If the constraint has no name, set it to <unnamed>
				 *
				 */
				if (fkconstraint->constr_name == NULL)
					fkconstraint->constr_name = "<unnamed>";

				/*
				 * If the attribute list for the referenced table was
				 * omitted, lookup for the definition of the primary key
				 *
				 */
				if (fkconstraint->fk_attrs != NIL && fkconstraint->pk_attrs == NIL)
					transformFkeyGetPrimaryKey(fkconstraint);

				/*
				 * Build a CREATE CONSTRAINT TRIGGER statement for the
				 * CHECK action.
				 *
				 */
				fk_trigger = (CreateTrigStmt *) makeNode(CreateTrigStmt);
				fk_trigger->trigname = fkconstraint->constr_name;
				fk_trigger->relname = stmt->relname;
				fk_trigger->funcname = "RI_FKey_check_ins";
				fk_trigger->before = false;
				fk_trigger->row = true;
				fk_trigger->actions[0] = 'i';
				fk_trigger->actions[1] = 'u';
				fk_trigger->actions[2] = '\0';
				fk_trigger->lang = NULL;
				fk_trigger->text = NULL;

				fk_trigger->attr = NIL;
				fk_trigger->when = NULL;
				fk_trigger->isconstraint = true;
				fk_trigger->deferrable = fkconstraint->deferrable;
				fk_trigger->initdeferred = fkconstraint->initdeferred;
				fk_trigger->constrrelname = fkconstraint->pktable_name;

				fk_trigger->args = NIL;
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->constr_name));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(stmt->relname));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->pktable_name));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->match_type));
				fk_attr = fkconstraint->fk_attrs;
				pk_attr = fkconstraint->pk_attrs;
				if (length(fk_attr) != length(pk_attr))
				{
					elog(NOTICE, "Illegal FOREIGN KEY definition REFERENCES \"%s\"",
						 fkconstraint->pktable_name);
					elog(ERROR, "number of key attributes in referenced table must be equal to foreign key");
				}
				while (fk_attr != NIL)
				{
					id = (Ident *) lfirst(fk_attr);
					fk_trigger->args = lappend(fk_trigger->args,
											   makeString(id->name));

					id = (Ident *) lfirst(pk_attr);
					fk_trigger->args = lappend(fk_trigger->args,
											   makeString(id->name));

					fk_attr = lnext(fk_attr);
					pk_attr = lnext(pk_attr);
				}

				extras_after = lappend(extras_after, (Node *) fk_trigger);

				/*
				 * Build a CREATE CONSTRAINT TRIGGER statement for the ON
				 * DELETE action fired on the PK table !!!
				 *
				 */
				fk_trigger = (CreateTrigStmt *) makeNode(CreateTrigStmt);
				fk_trigger->trigname = fkconstraint->constr_name;
				fk_trigger->relname = fkconstraint->pktable_name;
				switch ((fkconstraint->actions & FKCONSTR_ON_DELETE_MASK)
						>> FKCONSTR_ON_DELETE_SHIFT)
				{
					case FKCONSTR_ON_KEY_NOACTION:
						fk_trigger->funcname = "RI_FKey_noaction_del";
						break;
					case FKCONSTR_ON_KEY_RESTRICT:
						fk_trigger->funcname = "RI_FKey_restrict_del";
						break;
					case FKCONSTR_ON_KEY_CASCADE:
						fk_trigger->funcname = "RI_FKey_cascade_del";
						break;
					case FKCONSTR_ON_KEY_SETNULL:
						fk_trigger->funcname = "RI_FKey_setnull_del";
						break;
					case FKCONSTR_ON_KEY_SETDEFAULT:
						fk_trigger->funcname = "RI_FKey_setdefault_del";
						break;
					default:
						elog(ERROR, "Only one ON DELETE action can be specified for FOREIGN KEY constraint");
						break;
				}
				fk_trigger->before = false;
				fk_trigger->row = true;
				fk_trigger->actions[0] = 'd';
				fk_trigger->actions[1] = '\0';
				fk_trigger->lang = NULL;
				fk_trigger->text = NULL;

				fk_trigger->attr = NIL;
				fk_trigger->when = NULL;
				fk_trigger->isconstraint = true;
				fk_trigger->deferrable = fkconstraint->deferrable;
				fk_trigger->initdeferred = fkconstraint->initdeferred;
				fk_trigger->constrrelname = stmt->relname;

				fk_trigger->args = NIL;
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->constr_name));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(stmt->relname));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->pktable_name));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->match_type));
				fk_attr = fkconstraint->fk_attrs;
				pk_attr = fkconstraint->pk_attrs;
				while (fk_attr != NIL)
				{
					id = (Ident *) lfirst(fk_attr);
					fk_trigger->args = lappend(fk_trigger->args,
											   makeString(id->name));

					id = (Ident *) lfirst(pk_attr);
					fk_trigger->args = lappend(fk_trigger->args,
											   makeString(id->name));

					fk_attr = lnext(fk_attr);
					pk_attr = lnext(pk_attr);
				}

				extras_after = lappend(extras_after, (Node *) fk_trigger);

				/*
				 * Build a CREATE CONSTRAINT TRIGGER statement for the ON
				 * UPDATE action fired on the PK table !!!
				 *
				 */
				fk_trigger = (CreateTrigStmt *) makeNode(CreateTrigStmt);
				fk_trigger->trigname = fkconstraint->constr_name;
				fk_trigger->relname = fkconstraint->pktable_name;
				switch ((fkconstraint->actions & FKCONSTR_ON_UPDATE_MASK)
						>> FKCONSTR_ON_UPDATE_SHIFT)
				{
					case FKCONSTR_ON_KEY_NOACTION:
						fk_trigger->funcname = "RI_FKey_noaction_upd";
						break;
					case FKCONSTR_ON_KEY_RESTRICT:
						fk_trigger->funcname = "RI_FKey_restrict_upd";
						break;
					case FKCONSTR_ON_KEY_CASCADE:
						fk_trigger->funcname = "RI_FKey_cascade_upd";
						break;
					case FKCONSTR_ON_KEY_SETNULL:
						fk_trigger->funcname = "RI_FKey_setnull_upd";
						break;
					case FKCONSTR_ON_KEY_SETDEFAULT:
						fk_trigger->funcname = "RI_FKey_setdefault_upd";
						break;
					default:
						elog(ERROR, "Only one ON UPDATE action can be specified for FOREIGN KEY constraint");
						break;
				}
				fk_trigger->before = false;
				fk_trigger->row = true;
				fk_trigger->actions[0] = 'u';
				fk_trigger->actions[1] = '\0';
				fk_trigger->lang = NULL;
				fk_trigger->text = NULL;

				fk_trigger->attr = NIL;
				fk_trigger->when = NULL;
				fk_trigger->isconstraint = true;
				fk_trigger->deferrable = fkconstraint->deferrable;
				fk_trigger->initdeferred = fkconstraint->initdeferred;
				fk_trigger->constrrelname = stmt->relname;

				fk_trigger->args = NIL;
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->constr_name));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(stmt->relname));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->pktable_name));
				fk_trigger->args = lappend(fk_trigger->args,
										   makeString(fkconstraint->match_type));
				fk_attr = fkconstraint->fk_attrs;
				pk_attr = fkconstraint->pk_attrs;
				while (fk_attr != NIL)
				{
					id = (Ident *) lfirst(fk_attr);
					fk_trigger->args = lappend(fk_trigger->args,
											   makeString(id->name));

					id = (Ident *) lfirst(pk_attr);
					fk_trigger->args = lappend(fk_trigger->args,
											   makeString(id->name));

					fk_attr = lnext(fk_attr);
					pk_attr = lnext(pk_attr);
				}

				extras_after = lappend(extras_after, (Node *) fk_trigger);
			}
			break;
		default:
			break;
	}
	qry->utilityStmt = (Node *) stmt;
	return qry;
}


/* This function steps through the tree
 * built up by the select_w_o_sort rule
 * and builds a list of all SelectStmt Nodes found
 * The built up list is handed back in **select_list.
 * If one of the SelectStmt Nodes has the 'unionall' flag
 * set to true *unionall_present hands back 'true' */
void
create_select_list(Node *ptr, List **select_list, bool *unionall_present)
{
	if (IsA(ptr, SelectStmt))
	{
		*select_list = lappend(*select_list, ptr);
		if (((SelectStmt *) ptr)->unionall == TRUE)
			*unionall_present = TRUE;
		return;
	}

	/* Recursively call for all arguments. A NOT expr has no lexpr! */
	if (((A_Expr *) ptr)->lexpr != NULL)
		create_select_list(((A_Expr *) ptr)->lexpr, select_list, unionall_present);
	create_select_list(((A_Expr *) ptr)->rexpr, select_list, unionall_present);
}

/* Changes the A_Expr Nodes to Expr Nodes and exchanges ANDs and ORs.
 * The reason for the exchange is easy: We implement INTERSECTs and EXCEPTs
 * by rewriting these queries to semantically equivalent queries that use
 * IN and NOT IN subselects. To be able to use all three operations
 * (UNIONs INTERSECTs and EXCEPTs) in one complex query we have to
 * translate the queries into Disjunctive Normal Form (DNF). Unfortunately
 * there is no function 'dnfify' but there is a function 'cnfify'
 * which produces DNF when we exchange ANDs and ORs before calling
 * 'cnfify' and exchange them back in the result.
 *
 * If an EXCEPT or INTERSECT is present *intersect_present
 * hands back 'true' */
Node *
A_Expr_to_Expr(Node *ptr, bool *intersect_present)
{
	Node	   *result = NULL;

	switch (nodeTag(ptr))
	{
		case T_A_Expr:
			{
				A_Expr	   *a = (A_Expr *) ptr;

				switch (a->oper)
				{
					case AND:
						{
							Expr	   *expr = makeNode(Expr);
							Node	   *lexpr = A_Expr_to_Expr(((A_Expr *) ptr)->lexpr, intersect_present);
							Node	   *rexpr = A_Expr_to_Expr(((A_Expr *) ptr)->rexpr, intersect_present);

							*intersect_present = TRUE;

							expr->typeOid = BOOLOID;
							expr->opType = OR_EXPR;
							expr->args = makeList2(lexpr, rexpr);
							result = (Node *) expr;
							break;
						}
					case OR:
						{
							Expr	   *expr = makeNode(Expr);
							Node	   *lexpr = A_Expr_to_Expr(((A_Expr *) ptr)->lexpr, intersect_present);
							Node	   *rexpr = A_Expr_to_Expr(((A_Expr *) ptr)->rexpr, intersect_present);

							expr->typeOid = BOOLOID;
							expr->opType = AND_EXPR;
							expr->args = makeList2(lexpr, rexpr);
							result = (Node *) expr;
							break;
						}
					case NOT:
						{
							Expr	   *expr = makeNode(Expr);
							Node	   *rexpr = A_Expr_to_Expr(((A_Expr *) ptr)->rexpr, intersect_present);

							expr->typeOid = BOOLOID;
							expr->opType = NOT_EXPR;
							expr->args = makeList1(rexpr);
							result = (Node *) expr;
							break;
						}
				}
				break;
			}
		default:
			result = ptr;
	}
	return result;
}

void
CheckSelectForUpdate(Query *qry)
{
	if (qry->unionClause || qry->intersectClause)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with UNION/INTERSECT/EXCEPT clause");
	if (qry->distinctClause != NIL)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with DISTINCT clause");
	if (qry->groupClause != NIL)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with GROUP BY clause");
	if (qry->hasAggs)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with AGGREGATE");
}

static void
transformForUpdate(Query *qry, List *forUpdate)
{
	List	   *rowMarks = NIL;
	List	   *l;
	List	   *rt;
	Index		i;

	CheckSelectForUpdate(qry);

	if (lfirst(forUpdate) == NULL)
	{
		/* all tables used in query */
		i = 0;
		foreach(rt, qry->rtable)
		{
			RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);

			++i;
			if (rte->subquery)
			{
				/* FOR UPDATE of subquery is propagated to subquery's rels */
				transformForUpdate(rte->subquery, makeList1(NULL));
			}
			else
			{
				rowMarks = lappendi(rowMarks, i);
				rte->checkForWrite = true;
			}
		}
	}
	else
	{
		/* just the named tables */
		foreach(l, forUpdate)
		{
			char	   *relname = lfirst(l);

			i = 0;
			foreach(rt, qry->rtable)
			{
				RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);

				++i;
				if (strcmp(rte->eref->relname, relname) == 0)
				{
					if (rte->subquery)
					{
						/* propagate to subquery */
						transformForUpdate(rte->subquery, makeList1(NULL));
					}
					else
					{
						if (!intMember(i, rowMarks)) /* avoid duplicates */
							rowMarks = lappendi(rowMarks, i);
						rte->checkForWrite = true;
					}
					break;
				}
			}
			if (rt == NIL)
				elog(ERROR, "FOR UPDATE: relation \"%s\" not found in FROM clause",
					 relname);
		}
	}

	qry->rowMarks = rowMarks;
}


/*
 * transformFkeyCheckAttrs -
 *
 *	Try to make sure that the attributes of a referenced table
 *      belong to a unique (or primary key) constraint.
 *
 */
static void 
transformFkeyCheckAttrs(FkConstraint *fkconstraint)
{
	Relation	pkrel;
	Form_pg_attribute *pkrel_attrs;
	List	   *indexoidlist,
			   *indexoidscan;
	Form_pg_index indexStruct = NULL;
	int			i;
	int found=0;

	/* ----------
	 * Open the referenced table and get the attributes list
	 * ----------
	 */
	pkrel = heap_openr(fkconstraint->pktable_name, AccessShareLock);
	if (pkrel == NULL)
		elog(ERROR, "referenced table \"%s\" not found",
			 fkconstraint->pktable_name);
	pkrel_attrs = pkrel->rd_att->attrs;

	/* ----------
	 * Get the list of index OIDs for the table from the relcache,
	 * and look up each one in the pg_index syscache for each unique
	 * one, and then compare the attributes we were given to those
         * defined.
	 * ----------
	 */
	indexoidlist = RelationGetIndexList(pkrel);

	foreach(indexoidscan, indexoidlist)
	{
		Oid		indexoid = lfirsti(indexoidscan);
		HeapTuple	indexTuple;
		List *attrl;
		indexTuple = SearchSysCacheTuple(INDEXRELID,
										 ObjectIdGetDatum(indexoid),
										 0, 0, 0);
		if (!HeapTupleIsValid(indexTuple))
			elog(ERROR, "transformFkeyGetPrimaryKey: index %u not found",
				 indexoid);
		indexStruct = (Form_pg_index) GETSTRUCT(indexTuple);

		if (indexStruct->indisunique) {
			/* go through the fkconstraint->pk_attrs list */
			foreach(attrl, fkconstraint->pk_attrs) {
				Ident *attr=lfirst(attrl);
				found=0;
				for (i = 0; i < INDEX_MAX_KEYS && indexStruct->indkey[i] != 0; i++)
				{
					int		pkattno = indexStruct->indkey[i];
					if (pkattno>0) {
						char *name = NameStr(pkrel_attrs[pkattno - 1]->attname);
						if (strcmp(name, attr->name)==0) {
							found=1;
							break;
						}
					}
				}
				if (!found)
					break;
			}
		}
		if (found)
			break;		
		indexStruct = NULL;
	}
	if (!found)
		elog(ERROR, "UNIQUE constraint matching given keys for referenced table \"%s\" not found",
			 fkconstraint->pktable_name);

	freeList(indexoidlist);
	heap_close(pkrel, AccessShareLock);
}


/*
 * transformFkeyGetPrimaryKey -
 *
 *	Try to find the primary key attributes of a referenced table if
 *	the column list in the REFERENCES specification was omitted.
 *
 */
static void
transformFkeyGetPrimaryKey(FkConstraint *fkconstraint)
{
	Relation	pkrel;
	Form_pg_attribute *pkrel_attrs;
	List	   *indexoidlist,
			   *indexoidscan;
	Form_pg_index indexStruct = NULL;
	int			i;

	/* ----------
	 * Open the referenced table and get the attributes list
	 * ----------
	 */
	pkrel = heap_openr(fkconstraint->pktable_name, AccessShareLock);
	if (pkrel == NULL)
		elog(ERROR, "referenced table \"%s\" not found",
			 fkconstraint->pktable_name);
	pkrel_attrs = pkrel->rd_att->attrs;

	/* ----------
	 * Get the list of index OIDs for the table from the relcache,
	 * and look up each one in the pg_index syscache until we find one
	 * marked primary key (hopefully there isn't more than one such).
	 * ----------
	 */
	indexoidlist = RelationGetIndexList(pkrel);

	foreach(indexoidscan, indexoidlist)
	{
		Oid		indexoid = lfirsti(indexoidscan);
		HeapTuple	indexTuple;

		indexTuple = SearchSysCacheTuple(INDEXRELID,
										 ObjectIdGetDatum(indexoid),
										 0, 0, 0);
		if (!HeapTupleIsValid(indexTuple))
			elog(ERROR, "transformFkeyGetPrimaryKey: index %u not found",
				 indexoid);
		indexStruct = (Form_pg_index) GETSTRUCT(indexTuple);
		if (indexStruct->indisprimary)
			break;
		indexStruct = NULL;
	}

	freeList(indexoidlist);

	/* ----------
	 * Check that we found it
	 * ----------
	 */
	if (indexStruct == NULL)
		elog(ERROR, "PRIMARY KEY for referenced table \"%s\" not found",
			 fkconstraint->pktable_name);

	/* ----------
	 * Now build the list of PK attributes from the indkey definition
	 * using the attribute names of the PK relation descriptor
	 * ----------
	 */
	for (i = 0; i < INDEX_MAX_KEYS && indexStruct->indkey[i] != 0; i++)
	{
		int			pkattno = indexStruct->indkey[i];
		Ident	   *pkattr = makeNode(Ident);

		pkattr->name = DatumGetCString(DirectFunctionCall1(nameout,
						NameGetDatum(&(pkrel_attrs[pkattno - 1]->attname))));
		pkattr->indirection = NIL;
		pkattr->isRel = false;

		fkconstraint->pk_attrs = lappend(fkconstraint->pk_attrs, pkattr);
	}

	heap_close(pkrel, AccessShareLock);
}

/*
 * Preprocess a list of column constraint clauses
 * to attach constraint attributes to their primary constraint nodes
 * and detect inconsistent/misplaced constraint attributes.
 *
 * NOTE: currently, attributes are only supported for FOREIGN KEY primary
 * constraints, but someday they ought to be supported for other constraints.
 */
static void
transformConstraintAttrs(List *constraintList)
{
	Node	   *lastprimarynode = NULL;
	bool		saw_deferrability = false;
	bool		saw_initially = false;
	List	   *clist;

	foreach(clist, constraintList)
	{
		Node	   *node = lfirst(clist);

		if (!IsA(node, Constraint))
		{
			lastprimarynode = node;
			/* reset flags for new primary node */
			saw_deferrability = false;
			saw_initially = false;
		}
		else
		{
			Constraint *con = (Constraint *) node;

			switch (con->contype)
			{
				case CONSTR_ATTR_DEFERRABLE:
					if (lastprimarynode == NULL ||
						!IsA(lastprimarynode, FkConstraint))
						elog(ERROR, "Misplaced DEFERRABLE clause");
					if (saw_deferrability)
						elog(ERROR, "Multiple DEFERRABLE/NOT DEFERRABLE clauses not allowed");
					saw_deferrability = true;
					((FkConstraint *) lastprimarynode)->deferrable = true;
					break;
				case CONSTR_ATTR_NOT_DEFERRABLE:
					if (lastprimarynode == NULL ||
						!IsA(lastprimarynode, FkConstraint))
						elog(ERROR, "Misplaced NOT DEFERRABLE clause");
					if (saw_deferrability)
						elog(ERROR, "Multiple DEFERRABLE/NOT DEFERRABLE clauses not allowed");
					saw_deferrability = true;
					((FkConstraint *) lastprimarynode)->deferrable = false;
					if (saw_initially &&
						((FkConstraint *) lastprimarynode)->initdeferred)
						elog(ERROR, "INITIALLY DEFERRED constraint must be DEFERRABLE");
					break;
				case CONSTR_ATTR_DEFERRED:
					if (lastprimarynode == NULL ||
						!IsA(lastprimarynode, FkConstraint))
						elog(ERROR, "Misplaced INITIALLY DEFERRED clause");
					if (saw_initially)
						elog(ERROR, "Multiple INITIALLY IMMEDIATE/DEFERRED clauses not allowed");
					saw_initially = true;
					((FkConstraint *) lastprimarynode)->initdeferred = true;

					/*
					 * If only INITIALLY DEFERRED appears, assume
					 * DEFERRABLE
					 */
					if (!saw_deferrability)
						((FkConstraint *) lastprimarynode)->deferrable = true;
					else if (!((FkConstraint *) lastprimarynode)->deferrable)
						elog(ERROR, "INITIALLY DEFERRED constraint must be DEFERRABLE");
					break;
				case CONSTR_ATTR_IMMEDIATE:
					if (lastprimarynode == NULL ||
						!IsA(lastprimarynode, FkConstraint))
						elog(ERROR, "Misplaced INITIALLY IMMEDIATE clause");
					if (saw_initially)
						elog(ERROR, "Multiple INITIALLY IMMEDIATE/DEFERRED clauses not allowed");
					saw_initially = true;
					((FkConstraint *) lastprimarynode)->initdeferred = false;
					break;
				default:
					/* Otherwise it's not an attribute */
					lastprimarynode = node;
					/* reset flags for new primary node */
					saw_deferrability = false;
					saw_initially = false;
					break;
			}
		}
	}
}

/* Build a FromExpr node */
static FromExpr *
makeFromExpr(List *fromlist, Node *quals)
{
	FromExpr *f = makeNode(FromExpr);

	f->fromlist = fromlist;
	f->quals = quals;
	return f;
}

/*
 * Special handling of type definition for a column
 */
static void
transformColumnType(ParseState *pstate, ColumnDef *column)
{

	/*
	 * If the column doesn't have an explicitly specified typmod, check to
	 * see if we want to insert a default length.
	 *
	 * Note that we deliberately do NOT look at array or set information
	 * here; "numeric[]" needs the same default typmod as "numeric".
	 */
	if (column->typename->typmod == -1)
	{
		switch (typeTypeId(typenameType(column->typename->name)))
		{
				case BPCHAROID:
				/* "char" -> "char(1)" */
				column->typename->typmod = VARHDRSZ + 1;
				break;
			case NUMERICOID:
				column->typename->typmod = VARHDRSZ +
					((NUMERIC_DEFAULT_PRECISION << 16) | NUMERIC_DEFAULT_SCALE);
				break;
		}
	}
}
