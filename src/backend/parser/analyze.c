/*-------------------------------------------------------------------------
 *
 * analyze.c
 *	  transform the parse tree into a query tree
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/analyze.c,v 1.253.2.3 2007-06-20 18:21:51 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/namespace.h"
#include "catalog/pg_index.h"
#include "catalog/pg_type.h"
#include "commands/prepare.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/planmain.h"
#include "parser/analyze.h"
#include "parser/gramparse.h"
#include "parser/parsetree.h"
#include "parser/parse_agg.h"
#include "parser/parse_clause.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_oper.h"
#include "parser/parse_relation.h"
#include "parser/parse_target.h"
#include "parser/parse_type.h"
#include "parser/parse_expr.h"
#include "rewrite/rewriteManip.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/relcache.h"
#include "utils/syscache.h"
#include "mb/pg_wchar.h"


/* State shared by transformCreateSchemaStmt and its subroutines */
typedef struct
{
	const char *stmtType;		/* "CREATE SCHEMA" or "ALTER SCHEMA" */
	char	   *schemaname;		/* name of schema */
	char	   *authid;			/* owner of schema */
	List	   *tables;			/* CREATE TABLE items */
	List	   *views;			/* CREATE VIEW items */
	List	   *grants;			/* GRANT items */
	List	   *fwconstraints;	/* Forward referencing FOREIGN KEY
								 * constraints */
	List	   *alters;			/* Generated ALTER items (from the above) */
	List	   *ixconstraints;	/* index-creating constraints */
	List	   *blist;			/* "before list" of things to do before
								 * creating the schema */
	List	   *alist;			/* "after list" of things to do after
								 * creating the schema */
} CreateSchemaStmtContext;

/* State shared by transformCreateStmt and its subroutines */
typedef struct
{
	const char *stmtType;		/* "CREATE TABLE" or "ALTER TABLE" */
	RangeVar   *relation;		/* relation to create */
	List	   *inhRelations;	/* relations to inherit from */
	bool		hasoids;		/* does relation have an OID column? */
	Oid			relOid;			/* OID of table, if ALTER TABLE case */
	List	   *columns;		/* ColumnDef items */
	List	   *ckconstraints;	/* CHECK constraints */
	List	   *fkconstraints;	/* FOREIGN KEY constraints */
	List	   *ixconstraints;	/* index-creating constraints */
	List	   *blist;			/* "before list" of things to do before
								 * creating the table */
	List	   *alist;			/* "after list" of things to do after
								 * creating the table */
	IndexStmt  *pkey;			/* PRIMARY KEY index, if any */
} CreateStmtContext;


static Query *transformStmt(ParseState *pstate, Node *stmt,
			  List **extras_before, List **extras_after);
static Query *transformDeleteStmt(ParseState *pstate, DeleteStmt *stmt);
static Query *transformInsertStmt(ParseState *pstate, InsertStmt *stmt,
					List **extras_before, List **extras_after);
static Query *transformIndexStmt(ParseState *pstate, IndexStmt *stmt);
static Query *transformRuleStmt(ParseState *query, RuleStmt *stmt,
				  List **extras_before, List **extras_after);
static Query *transformSelectStmt(ParseState *pstate, SelectStmt *stmt);
static Query *transformSetOperationStmt(ParseState *pstate, SelectStmt *stmt);
static Node *transformSetOperationTree(ParseState *pstate, SelectStmt *stmt);
static Query *transformUpdateStmt(ParseState *pstate, UpdateStmt *stmt);
static Query *transformPrepareStmt(ParseState *pstate, PrepareStmt *stmt);
static Query *transformExecuteStmt(ParseState *pstate, ExecuteStmt *stmt);
static Query *transformCreateStmt(ParseState *pstate, CreateStmt *stmt,
					List **extras_before, List **extras_after);
static Query *transformAlterTableStmt(ParseState *pstate, AlterTableStmt *stmt,
						List **extras_before, List **extras_after);
static void transformColumnDefinition(ParseState *pstate,
						  CreateStmtContext *cxt,
						  ColumnDef *column);
static void transformTableConstraint(ParseState *pstate,
						 CreateStmtContext *cxt,
						 Constraint *constraint);
static void transformIndexConstraints(ParseState *pstate,
						  CreateStmtContext *cxt);
static void transformFKConstraints(ParseState *pstate,
								   CreateStmtContext *cxt,
								   bool isAddConstraint);
static void applyColumnNames(List *dst, List *src);
static List *getSetColTypes(ParseState *pstate, Node *node);
static void transformForUpdate(Query *qry, List *forUpdate);
static void transformConstraintAttrs(List *constraintList);
static void transformColumnType(ParseState *pstate, ColumnDef *column);
static bool relationHasPrimaryKey(Oid relationOid);
static void release_pstate_resources(ParseState *pstate);
static FromExpr *makeFromExpr(List *fromlist, Node *quals);



/*
 * parse_analyze -
 *	  analyze a raw parse tree and transform it to Query form.
 *
 * The result is a List of Query nodes (we need a list since some commands
 * produce multiple Queries).  Optimizable statements require considerable
 * transformation, while many utility-type statements are simply hung off
 * a dummy CMD_UTILITY Query node.
 */
List *
parse_analyze(Node *parseTree, ParseState *parentParseState)
{
	List	   *result = NIL;
	ParseState *pstate = make_parsestate(parentParseState);

	/* Lists to return extra commands from transformation */
	List	   *extras_before = NIL;
	List	   *extras_after = NIL;
	Query	   *query;
	List	   *listscan;

	query = transformStmt(pstate, parseTree, &extras_before, &extras_after);
	release_pstate_resources(pstate);

	while (extras_before != NIL)
	{
		result = nconc(result, parse_analyze(lfirst(extras_before), pstate));
		extras_before = lnext(extras_before);
	}

	result = lappend(result, query);

	while (extras_after != NIL)
	{
		result = nconc(result, parse_analyze(lfirst(extras_after), pstate));
		extras_after = lnext(extras_after);
	}

	/*
	 * Make sure that only the original query is marked original. We have
	 * to do this explicitly since recursive calls of parse_analyze will
	 * have marked some of the added-on queries as "original".
	 */
	foreach(listscan, result)
	{
		Query	   *q = lfirst(listscan);

		q->querySource = (q == query ? QSRC_ORIGINAL : QSRC_PARSER);
	}

	pfree(pstate);

	return result;
}

static void
release_pstate_resources(ParseState *pstate)
{
	if (pstate->p_target_relation != NULL)
		heap_close(pstate->p_target_relation, NoLock);
	pstate->p_target_relation = NULL;
	pstate->p_target_rangetblentry = NULL;
}

/*
 * transformStmt -
 *	  transform a Parse tree into a Query tree.
 */
static Query *
transformStmt(ParseState *pstate, Node *parseTree,
			  List **extras_before, List **extras_after)
{
	Query	   *result = NULL;

	switch (nodeTag(parseTree))
	{
			/*
			 * Non-optimizable statements
			 */
		case T_CreateStmt:
			result = transformCreateStmt(pstate, (CreateStmt *) parseTree,
										 extras_before, extras_after);
			break;

		case T_IndexStmt:
			result = transformIndexStmt(pstate, (IndexStmt *) parseTree);
			break;

		case T_RuleStmt:
			result = transformRuleStmt(pstate, (RuleStmt *) parseTree,
									   extras_before, extras_after);
			break;

		case T_ViewStmt:
			{
				ViewStmt   *n = (ViewStmt *) parseTree;

				n->query = transformStmt(pstate, (Node *) n->query,
										 extras_before, extras_after);

				/*
				 * If a list of column names was given, run through and
				 * insert these into the actual query tree. - thomas
				 * 2000-03-08
				 *
				 * Outer loop is over targetlist to make it easier to skip
				 * junk targetlist entries.
				 */
				if (n->aliases != NIL)
				{
					List	   *aliaslist = n->aliases;
					List	   *targetList;

					foreach(targetList, n->query->targetList)
					{
						TargetEntry *te = (TargetEntry *) lfirst(targetList);
						Resdom	   *rd;

						Assert(IsA(te, TargetEntry));
						rd = te->resdom;
						Assert(IsA(rd, Resdom));
						/* junk columns don't get aliases */
						if (rd->resjunk)
							continue;
						rd->resname = pstrdup(strVal(lfirst(aliaslist)));
						aliaslist = lnext(aliaslist);
						if (aliaslist == NIL)
							break;		/* done assigning aliases */
					}

					if (aliaslist != NIL)
						elog(ERROR, "CREATE VIEW specifies more column names than columns");
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
				n->query = transformStmt(pstate, (Node *) n->query,
										 extras_before, extras_after);
				result->utilityStmt = (Node *) parseTree;
			}
			break;

		case T_AlterTableStmt:
			result = transformAlterTableStmt(pstate, (AlterTableStmt *) parseTree,
											 extras_before, extras_after);
			break;

		case T_PrepareStmt:
			result = transformPrepareStmt(pstate, (PrepareStmt *) parseTree);
			break;

		case T_ExecuteStmt:
			result = transformExecuteStmt(pstate, (ExecuteStmt *) parseTree);
			break;

			/*
			 * Optimizable statements
			 */
		case T_InsertStmt:
			result = transformInsertStmt(pstate, (InsertStmt *) parseTree,
										 extras_before, extras_after);
			break;

		case T_DeleteStmt:
			result = transformDeleteStmt(pstate, (DeleteStmt *) parseTree);
			break;

		case T_UpdateStmt:
			result = transformUpdateStmt(pstate, (UpdateStmt *) parseTree);
			break;

		case T_SelectStmt:
			if (((SelectStmt *) parseTree)->op == SETOP_NONE)
				result = transformSelectStmt(pstate,
											 (SelectStmt *) parseTree);
			else
				result = transformSetOperationStmt(pstate,
											   (SelectStmt *) parseTree);
			break;

		default:

			/*
			 * other statements don't require any transformation-- just
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

	/* set up range table with just the result rel */
	qry->resultRelation = setTargetTable(pstate, stmt->relation,
							  interpretInhOption(stmt->relation->inhOpt),
										 true);

	qry->distinctClause = NIL;

	/* fix where clause */
	qual = transformWhereClause(pstate, stmt->whereClause);

	/* done building the range table and jointree */
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs)
		parseCheckAggregates(pstate, qry, qual);

	return qry;
}

/*
 * transformInsertStmt -
 *	  transform an Insert Statement
 */
static Query *
transformInsertStmt(ParseState *pstate, InsertStmt *stmt,
					List **extras_before, List **extras_after)
{
	Query	   *qry = makeNode(Query);
	List	   *sub_rtable;
	List	   *sub_namespace;
	List	   *icolumns;
	List	   *attrnos;
	List	   *attnos;
	List	   *tl;

	qry->commandType = CMD_INSERT;
	pstate->p_is_insert = true;

	/*
	 * If a non-nil rangetable/namespace was passed in, and we are doing
	 * INSERT/SELECT, arrange to pass the rangetable/namespace down to the
	 * SELECT.	This can only happen if we are inside a CREATE RULE, and
	 * in that case we want the rule's OLD and NEW rtable entries to
	 * appear as part of the SELECT's rtable, not as outer references for
	 * it.	(Kluge!)  The SELECT's joinlist is not affected however. We
	 * must do this before adding the target table to the INSERT's rtable.
	 */
	if (stmt->selectStmt)
	{
		sub_rtable = pstate->p_rtable;
		pstate->p_rtable = NIL;
		sub_namespace = pstate->p_namespace;
		pstate->p_namespace = NIL;
	}
	else
	{
		sub_rtable = NIL;		/* not used, but keep compiler quiet */
		sub_namespace = NIL;
	}

	/*
	 * Must get write lock on INSERT target table before scanning SELECT,
	 * else we will grab the wrong kind of initial lock if the target
	 * table is also mentioned in the SELECT part.	Note that the target
	 * table is not added to the joinlist or namespace.
	 */
	qry->resultRelation = setTargetTable(pstate, stmt->relation,
										 false, false);

	/*
	 * Is it INSERT ... SELECT or INSERT ... VALUES?
	 */
	if (stmt->selectStmt)
	{
		ParseState *sub_pstate = make_parsestate(pstate->parentParseState);
		Query	   *selectQuery;
		RangeTblEntry *rte;
		RangeTblRef *rtr;

		/*
		 * Process the source SELECT.
		 *
		 * It is important that this be handled just like a standalone
		 * SELECT; otherwise the behavior of SELECT within INSERT might be
		 * different from a stand-alone SELECT. (Indeed, Postgres up
		 * through 6.5 had bugs of just that nature...)
		 */
		sub_pstate->p_rtable = sub_rtable;
		sub_pstate->p_namespace = sub_namespace;

		/*
		 * Note: we are not expecting that extras_before and extras_after
		 * are going to be used by the transformation of the SELECT
		 * statement.
		 */
		selectQuery = transformStmt(sub_pstate, stmt->selectStmt,
									extras_before, extras_after);

		release_pstate_resources(sub_pstate);
		pfree(sub_pstate);

		Assert(IsA(selectQuery, Query));
		Assert(selectQuery->commandType == CMD_SELECT);
		if (selectQuery->into || selectQuery->isPortal)
			elog(ERROR, "INSERT ... SELECT may not specify INTO");

		/*
		 * Make the source be a subquery in the INSERT's rangetable, and
		 * add it to the INSERT's joinlist.
		 */
		rte = addRangeTableEntryForSubquery(pstate,
											selectQuery,
											makeAlias("*SELECT*", NIL),
											true);
		rtr = makeNode(RangeTblRef);
		/* assume new rte is at end */
		rtr->rtindex = length(pstate->p_rtable);
		Assert(rte == rt_fetch(rtr->rtindex, pstate->p_rtable));
		pstate->p_joinlist = lappend(pstate->p_joinlist, rtr);

		/*
		 * Generate a targetlist for the INSERT that selects all the
		 * non-resjunk columns from the subquery.  (We need this to be
		 * separate from the subquery's tlist because we may add columns,
		 * insert datatype coercions, etc.)
		 *
		 * HACK: constants in the INSERT's targetlist are copied up as-is
		 * rather than being referenced as subquery outputs.  This is
		 * mainly to ensure that when we try to coerce them to the target
		 * column's datatype, the right things happen for UNKNOWN
		 * constants. Otherwise this fails: INSERT INTO foo SELECT 'bar',
		 * ... FROM baz
		 */
		qry->targetList = NIL;
		foreach(tl, selectQuery->targetList)
		{
			TargetEntry *tle = (TargetEntry *) lfirst(tl);
			Resdom	   *resnode = tle->resdom;
			Node	   *expr;

			if (resnode->resjunk)
				continue;
			if (tle->expr && IsA(tle->expr, Const))
				expr = tle->expr;
			else
				expr = (Node *) makeVar(rtr->rtindex,
										resnode->resno,
										resnode->restype,
										resnode->restypmod,
										0);
			resnode = copyObject(resnode);
			resnode->resno = (AttrNumber) pstate->p_last_resno++;
			qry->targetList = lappend(qry->targetList,
									  makeTargetEntry(resnode, expr));
		}
	}
	else
	{
		/*
		 * For INSERT ... VALUES, transform the given list of values to
		 * form a targetlist for the INSERT.
		 */
		qry->targetList = transformTargetList(pstate, stmt->targetList);
	}

	/*
	 * Now we are done with SELECT-like processing, and can get on with
	 * transforming the target list to match the INSERT target columns.
	 */

	/* Prepare to assign non-conflicting resnos to resjunk attributes */
	if (pstate->p_last_resno <= pstate->p_target_relation->rd_rel->relnatts)
		pstate->p_last_resno = pstate->p_target_relation->rd_rel->relnatts + 1;

	/* Validate stmt->cols list, or build default list if no list given */
	icolumns = checkInsertTargets(pstate, stmt->cols, &attrnos);

	/*
	 * Prepare columns for assignment to target table.
	 */
	attnos = attrnos;
	foreach(tl, qry->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tl);
		ResTarget  *col;

		if (icolumns == NIL || attnos == NIL)
			elog(ERROR, "INSERT has more expressions than target columns");
		col = (ResTarget *) lfirst(icolumns);

		/*
		 * When the value is to be set to the column default we can simply
		 * drop it now and handle it later on using methods for missing
		 * columns.
		 */
		if (!IsA(tle, InsertDefault))
		{
			Assert(IsA(col, ResTarget));
			Assert(!tle->resdom->resjunk);
			updateTargetListEntry(pstate, tle, col->name, lfirsti(attnos),
								  col->indirection);
		}
		else
		{
			icolumns = lremove(icolumns, icolumns);
			attnos = lremove(attnos, attnos);
			qry->targetList = lremove(tle, qry->targetList);
		}

		icolumns = lnext(icolumns);
		attnos = lnext(attnos);
	}

	/*
	 * Ensure that the targetlist has the same number of  entries that
	 * were present in the columns list.  Don't do the check for select
	 * statements.
	 */
	if (stmt->cols != NIL && (icolumns != NIL || attnos != NIL))
		elog(ERROR, "INSERT has more target columns than expressions");

	/* done building the range table and jointree */
	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, NULL);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs)
		parseCheckAggregates(pstate, qry, NULL);

	return qry;
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
char *
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

	if (name1)
		name1chars = pg_mbcliplen(name1, name1chars, name1chars);
	if (name2)
		name2chars = pg_mbcliplen(name2, name2chars, name2chars);

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
CreateIndexName(char *table_name, char *column_name,
				char *label, List *indices)
{
	int			pass = 0;
	char	   *iname = NULL;
	List	   *ilist;
	char		typename[NAMEDATALEN];

	/*
	 * The type name for makeObjectName is label, or labelN if that's
	 * necessary to prevent collisions among multiple indexes for the same
	 * table.  Note there is no check for collisions with already-existing
	 * indexes, only among the indexes we're about to create now; this
	 * ought to be improved someday.
	 */
	strncpy(typename, label, sizeof(typename));

	for (;;)
	{
		iname = makeObjectName(table_name, column_name, typename);

		foreach(ilist, indices)
		{
			IndexStmt  *index = lfirst(ilist);

			if (index->idxname != NULL &&
				strcmp(iname, index->idxname) == 0)
				break;
		}
		/* ran through entire list? then no name conflict found so done */
		if (ilist == NIL)
			break;

		/* found a conflict, so try a new name component */
		pfree(iname);
		snprintf(typename, sizeof(typename), "%s%d", label, ++pass);
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
transformCreateStmt(ParseState *pstate, CreateStmt *stmt,
					List **extras_before, List **extras_after)
{
	CreateStmtContext cxt;
	Query	   *q;
	List	   *elements;

	cxt.stmtType = "CREATE TABLE";
	cxt.relation = stmt->relation;
	cxt.inhRelations = stmt->inhRelations;
	cxt.hasoids = stmt->hasoids;
	cxt.relOid = InvalidOid;
	cxt.columns = NIL;
	cxt.ckconstraints = NIL;
	cxt.fkconstraints = NIL;
	cxt.ixconstraints = NIL;
	cxt.blist = NIL;
	cxt.alist = NIL;
	cxt.pkey = NULL;

	/*
	 * Run through each primary element in the table creation clause.
	 * Separate column defs from constraints, and do preliminary analysis.
	 */
	foreach(elements, stmt->tableElts)
	{
		Node	   *element = lfirst(elements);

		switch (nodeTag(element))
		{
			case T_ColumnDef:
				transformColumnDefinition(pstate, &cxt,
										  (ColumnDef *) element);
				break;

			case T_Constraint:
				transformTableConstraint(pstate, &cxt,
										 (Constraint *) element);
				break;

			case T_FkConstraint:
				/* No pre-transformation needed */
				cxt.fkconstraints = lappend(cxt.fkconstraints, element);
				break;

			default:
				elog(ERROR, "parser: unrecognized node (internal error)");
		}
	}

	Assert(stmt->constraints == NIL);

	/*
	 * Postprocess constraints that give rise to index definitions.
	 */
	transformIndexConstraints(pstate, &cxt);

	/*
	 * Postprocess foreign-key constraints.
	 */
	transformFKConstraints(pstate, &cxt, false);

	/*
	 * Output results.
	 */
	q = makeNode(Query);
	q->commandType = CMD_UTILITY;
	q->utilityStmt = (Node *) stmt;
	stmt->tableElts = cxt.columns;
	stmt->constraints = cxt.ckconstraints;
	*extras_before = nconc(*extras_before, cxt.blist);
	*extras_after = nconc(cxt.alist, *extras_after);

	return q;
}

static void
transformColumnDefinition(ParseState *pstate, CreateStmtContext *cxt,
						  ColumnDef *column)
{
	bool		is_serial;
	bool		saw_nullable;
	bool		saw_default;
	Constraint *constraint;
	List	   *clist;

	cxt->columns = lappend(cxt->columns, column);

	/* Check for SERIAL pseudo-types */
	is_serial = false;
	if (length(column->typename->names) == 1)
	{
		char	   *typname = strVal(lfirst(column->typename->names));

		if (strcmp(typname, "serial") == 0 ||
			strcmp(typname, "serial4") == 0)
		{
			is_serial = true;
			column->typename->names = NIL;
			column->typename->typeid = INT4OID;
		}
		else if (strcmp(typname, "bigserial") == 0 ||
				 strcmp(typname, "serial8") == 0)
		{
			is_serial = true;
			column->typename->names = NIL;
			column->typename->typeid = INT8OID;
		}
	}

	/* Do necessary work on the column type declaration */
	transformColumnType(pstate, column);

	/* Special actions for SERIAL pseudo-types */
	if (is_serial)
	{
		char	   *sname;
		char	   *snamespace;
		char	   *qstring;
		A_Const    *snamenode;
		FuncCall   *funccallnode;
		CreateSeqStmt *seqstmt;

		/*
		 * Determine name and namespace to use for the sequence.
		 */
		sname = makeObjectName(cxt->relation->relname, column->colname, "seq");
		snamespace = get_namespace_name(RangeVarGetCreationNamespace(cxt->relation));

		elog(NOTICE, "%s will create implicit sequence '%s' for SERIAL column '%s.%s'",
		  cxt->stmtType, sname, cxt->relation->relname, column->colname);

		/*
		 * Build a CREATE SEQUENCE command to create the sequence object,
		 * and add it to the list of things to be done before this
		 * CREATE/ALTER TABLE.
		 */
		seqstmt = makeNode(CreateSeqStmt);
		seqstmt->sequence = makeRangeVar(snamespace, sname);
		seqstmt->options = NIL;

		cxt->blist = lappend(cxt->blist, seqstmt);

		/*
		 * Mark the ColumnDef so that during execution, an appropriate
		 * dependency will be added from the sequence to the column.
		 */
		column->support = makeRangeVar(snamespace, sname);

		/*
		 * Create appropriate constraints for SERIAL.  We do this in full,
		 * rather than shortcutting, so that we will detect any
		 * conflicting constraints the user wrote (like a different
		 * DEFAULT).
		 *
		 * Create an expression tree representing the function call
		 * nextval('"sequencename"')
		 */
		qstring = quote_qualified_identifier(snamespace, sname);
		snamenode = makeNode(A_Const);
		snamenode->val.type = T_String;
		snamenode->val.val.str = qstring;
		funccallnode = makeNode(FuncCall);
		funccallnode->funcname = SystemFuncName("nextval");
		funccallnode->args = makeList1(snamenode);
		funccallnode->agg_star = false;
		funccallnode->agg_distinct = false;

		constraint = makeNode(Constraint);
		constraint->contype = CONSTR_DEFAULT;
		constraint->name = sname;
		constraint->raw_expr = (Node *) funccallnode;
		constraint->cooked_expr = NULL;
		constraint->keys = NIL;
		column->constraints = lappend(column->constraints, constraint);

		constraint = makeNode(Constraint);
		constraint->contype = CONSTR_NOTNULL;
		column->constraints = lappend(column->constraints, constraint);
	}

	/* Process column constraints, if any... */
	transformConstraintAttrs(column->constraints);

	saw_nullable = false;
	saw_default = false;

	foreach(clist, column->constraints)
	{
		constraint = lfirst(clist);

		/*
		 * If this column constraint is a FOREIGN KEY constraint, then we
		 * fill in the current attribute's name and throw it into the list
		 * of FK constraints to be processed later.
		 */
		if (IsA(constraint, FkConstraint))
		{
			FkConstraint *fkconstraint = (FkConstraint *) constraint;

			fkconstraint->fk_attrs = makeList1(makeString(column->colname));
			cxt->fkconstraints = lappend(cxt->fkconstraints, fkconstraint);
			continue;
		}

		Assert(IsA(constraint, Constraint));

		switch (constraint->contype)
		{
			case CONSTR_NULL:
				if (saw_nullable && column->is_not_null)
					elog(ERROR, "%s/(NOT) NULL conflicting declaration for '%s.%s'",
						 cxt->stmtType, cxt->relation->relname, column->colname);
				column->is_not_null = FALSE;
				saw_nullable = true;
				break;

			case CONSTR_NOTNULL:
				if (saw_nullable && !column->is_not_null)
					elog(ERROR, "%s/(NOT) NULL conflicting declaration for '%s.%s'",
						 cxt->stmtType, cxt->relation->relname, column->colname);
				column->is_not_null = TRUE;
				saw_nullable = true;
				break;

			case CONSTR_DEFAULT:
				if (saw_default)
					elog(ERROR, "%s/DEFAULT multiple values specified for '%s.%s'",
						 cxt->stmtType, cxt->relation->relname, column->colname);
				/* Note: DEFAULT NULL maps to constraint->raw_expr == NULL */
				column->raw_default = constraint->raw_expr;
				Assert(constraint->cooked_expr == NULL);
				saw_default = true;
				break;

			case CONSTR_PRIMARY:
				if (constraint->name == NULL)
					constraint->name = makeObjectName(cxt->relation->relname,
													  NULL,
													  "pkey");
				if (constraint->keys == NIL)
					constraint->keys = makeList1(makeString(column->colname));
				cxt->ixconstraints = lappend(cxt->ixconstraints, constraint);
				break;

			case CONSTR_UNIQUE:
				if (constraint->name == NULL)
					constraint->name = makeObjectName(cxt->relation->relname,
													  column->colname,
													  "key");
				if (constraint->keys == NIL)
					constraint->keys = makeList1(makeString(column->colname));
				cxt->ixconstraints = lappend(cxt->ixconstraints, constraint);
				break;

			case CONSTR_CHECK:
				if (constraint->name == NULL)
					constraint->name = makeObjectName(cxt->relation->relname,
													  column->colname,
													  NULL);
				cxt->ckconstraints = lappend(cxt->ckconstraints, constraint);
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
}

static void
transformTableConstraint(ParseState *pstate, CreateStmtContext *cxt,
						 Constraint *constraint)
{
	switch (constraint->contype)
	{
		case CONSTR_PRIMARY:
			if (constraint->name == NULL)
				constraint->name = makeObjectName(cxt->relation->relname,
												  NULL,
												  "pkey");
			cxt->ixconstraints = lappend(cxt->ixconstraints, constraint);
			break;

		case CONSTR_UNIQUE:
			cxt->ixconstraints = lappend(cxt->ixconstraints, constraint);
			break;

		case CONSTR_CHECK:
			cxt->ckconstraints = lappend(cxt->ckconstraints, constraint);
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
}

static void
transformIndexConstraints(ParseState *pstate, CreateStmtContext *cxt)
{
	List	   *listptr;
	List	   *keys;
	IndexStmt  *index;
	IndexElem  *iparam;
	ColumnDef  *column;
	List	   *columns;
	List	   *indexlist = NIL;

	/*
	 * Run through the constraints that need to generate an index. For
	 * PRIMARY KEY, mark each column as NOT NULL and create an index. For
	 * UNIQUE, create an index as for PRIMARY KEY, but do not insist on
	 * NOT NULL.
	 */
	foreach(listptr, cxt->ixconstraints)
	{
		Constraint *constraint = lfirst(listptr);

		Assert(IsA(constraint, Constraint));
		Assert((constraint->contype == CONSTR_PRIMARY)
			   || (constraint->contype == CONSTR_UNIQUE));

		index = makeNode(IndexStmt);

		index->unique = true;
		index->primary = (constraint->contype == CONSTR_PRIMARY);
		if (index->primary)
		{
			/* In ALTER TABLE case, a primary index might already exist */
			if (cxt->pkey != NULL ||
				(OidIsValid(cxt->relOid) &&
				 relationHasPrimaryKey(cxt->relOid)))
				elog(ERROR, "%s / PRIMARY KEY multiple primary keys"
					 " for table '%s' are not allowed",
					 cxt->stmtType, cxt->relation->relname);
			cxt->pkey = index;
		}
		index->isconstraint = true;

		if (constraint->name != NULL)
			index->idxname = pstrdup(constraint->name);
		else if (constraint->contype == CONSTR_PRIMARY)
			index->idxname = makeObjectName(cxt->relation->relname, NULL, "pkey");
		else
			index->idxname = NULL;		/* will set it later */

		index->relation = cxt->relation;
		index->accessMethod = DEFAULT_INDEX_TYPE;
		index->indexParams = NIL;
		index->whereClause = NULL;

		/*
		 * Make sure referenced keys exist.  If we are making a PRIMARY
		 * KEY index, also make sure they are NOT NULL.
		 */
		foreach(keys, constraint->keys)
		{
			char	   *key = strVal(lfirst(keys));
			bool		found = false;

			column = NULL;
			foreach(columns, cxt->columns)
			{
				column = lfirst(columns);
				Assert(IsA(column, ColumnDef));
				if (strcmp(column->colname, key) == 0)
				{
					found = true;
					break;
				}
			}
			if (found)
			{
				/* found column in the new table; force it to be NOT NULL */
				if (constraint->contype == CONSTR_PRIMARY)
					column->is_not_null = TRUE;
			}
			else if (SystemAttributeByName(key, cxt->hasoids) != NULL)
			{
				/*
				 * column will be a system column in the new table, so
				 * accept it.  System columns can't ever be null, so no
				 * need to worry about PRIMARY/NOT NULL constraint.
				 */
				found = true;
			}
			else if (cxt->inhRelations)
			{
				/* try inherited tables */
				List	   *inher;

				foreach(inher, cxt->inhRelations)
				{
					RangeVar   *inh = lfirst(inher);
					Relation	rel;
					int			count;

					Assert(IsA(inh, RangeVar));
					rel = heap_openrv(inh, AccessShareLock);
					if (rel->rd_rel->relkind != RELKIND_RELATION)
						elog(ERROR, "inherited table \"%s\" is not a relation",
							 inh->relname);
					for (count = 0; count < rel->rd_att->natts; count++)
					{
						Form_pg_attribute inhattr = rel->rd_att->attrs[count];
						char	   *inhname = NameStr(inhattr->attname);

						if (inhattr->attisdropped)
							continue;
						if (strcmp(key, inhname) == 0)
						{
							found = true;

							/*
							 * If the column is inherited, we currently
							 * have no easy way to force it to be NOT
							 * NULL. Only way I can see to fix this would
							 * be to convert the inherited-column info to
							 * ColumnDef nodes before we reach this point,
							 * and then create the table from those nodes
							 * rather than referencing the parent tables
							 * later.  That would likely be cleaner, but
							 * too much work to contemplate right now.
							 * Instead, raise an error if the inherited
							 * column won't be NOT NULL. (Would a WARNING
							 * be more reasonable?)
							 */
							if (constraint->contype == CONSTR_PRIMARY &&
								!inhattr->attnotnull)
								elog(ERROR, "inherited attribute \"%s\" cannot be a PRIMARY KEY because it is not marked NOT NULL",
									 inhname);
							break;
						}
					}
					heap_close(rel, NoLock);
					if (found)
						break;
				}
			}
			else if (OidIsValid(cxt->relOid))
			{
				/* ALTER TABLE case: does column already exist? */
				HeapTuple	atttuple;

				atttuple = SearchSysCacheAttName(cxt->relOid, key);
				if (HeapTupleIsValid(atttuple))
				{
					found = true;

					/*
					 * We require pre-existing column to be already marked
					 * NOT NULL.
					 */
					if (constraint->contype == CONSTR_PRIMARY &&
						!((Form_pg_attribute) GETSTRUCT(atttuple))->attnotnull)
						elog(ERROR, "Existing attribute \"%s\" cannot be a PRIMARY KEY because it is not marked NOT NULL",
							 key);
					ReleaseSysCache(atttuple);
				}
			}

			if (!found)
				elog(ERROR, "%s: column \"%s\" named in key does not exist",
					 cxt->stmtType, key);

			/* Check for PRIMARY KEY(foo, foo) */
			foreach(columns, index->indexParams)
			{
				iparam = (IndexElem *) lfirst(columns);
				if (iparam->name && strcmp(key, iparam->name) == 0)
					elog(ERROR, "%s: column \"%s\" appears twice in %s constraint",
						 cxt->stmtType, key,
						 index->primary ? "PRIMARY KEY" : "UNIQUE");
			}

			/* OK, add it to the index definition */
			iparam = makeNode(IndexElem);
			iparam->name = pstrdup(key);
			iparam->funcname = NIL;
			iparam->args = NIL;
			iparam->opclass = NIL;
			index->indexParams = lappend(index->indexParams, iparam);
		}

		indexlist = lappend(indexlist, index);
	}

	/*
	 * Scan the index list and remove any redundant index specifications.
	 * This can happen if, for instance, the user writes UNIQUE PRIMARY
	 * KEY. A strict reading of SQL92 would suggest raising an error
	 * instead, but that strikes me as too anal-retentive. - tgl
	 * 2001-02-14
	 *
	 * XXX in ALTER TABLE case, it'd be nice to look for duplicate
	 * pre-existing indexes, too.
	 */
	cxt->alist = NIL;
	if (cxt->pkey != NULL)
	{
		/* Make sure we keep the PKEY index in preference to others... */
		cxt->alist = makeList1(cxt->pkey);
	}
	while (indexlist != NIL)
	{
		index = lfirst(indexlist);

		/* if it's pkey, it's already in cxt->alist */
		if (index != cxt->pkey)
		{
			bool		keep = true;
			List	   *priorlist;

			foreach(priorlist, cxt->alist)
			{
				IndexStmt  *priorindex = lfirst(priorlist);

				if (equal(index->indexParams, priorindex->indexParams))
				{
					/*
					 * If the prior index is as yet unnamed, and this one
					 * is named, then transfer the name to the prior
					 * index. This ensures that if we have named and
					 * unnamed constraints, we'll use (at least one of)
					 * the names for the index.
					 */
					if (priorindex->idxname == NULL)
						priorindex->idxname = index->idxname;
					keep = false;
					break;
				}
			}

			if (keep)
				cxt->alist = lappend(cxt->alist, index);
		}

		indexlist = lnext(indexlist);
	}

	/*
	 * Finally, select unique names for all not-previously-named indices,
	 * and display NOTICE messages.
	 *
	 * XXX in ALTER TABLE case, we fail to consider name collisions against
	 * pre-existing indexes.
	 */
	foreach(indexlist, cxt->alist)
	{
		index = lfirst(indexlist);

		if (index->idxname == NULL && index->indexParams != NIL)
		{
			iparam = lfirst(index->indexParams);
			index->idxname = CreateIndexName(cxt->relation->relname,
											 iparam->name ? iparam->name :
										 strVal(llast(iparam->funcname)),
											 "key", cxt->alist);
		}
		if (index->idxname == NULL)		/* should not happen */
			elog(ERROR, "%s: failed to make implicit index name",
				 cxt->stmtType);

		elog(NOTICE, "%s / %s%s will create implicit index '%s' for table '%s'",
			 cxt->stmtType,
			 (strcmp(cxt->stmtType, "ALTER TABLE") == 0) ? "ADD " : "",
			 (index->primary ? "PRIMARY KEY" : "UNIQUE"),
			 index->idxname, cxt->relation->relname);
	}
}

static void
transformFKConstraints(ParseState *pstate, CreateStmtContext *cxt,
					   bool isAddConstraint)
{
	if (cxt->fkconstraints == NIL)
		return;

	elog(NOTICE, "%s will create implicit trigger(s) for FOREIGN KEY check(s)",
		 cxt->stmtType);

	/*
	 * For ALTER TABLE ADD CONSTRAINT, nothing to do.  For CREATE TABLE or
	 * ALTER TABLE ADD COLUMN, gin up an ALTER TABLE ADD CONSTRAINT command
	 * to execute after the basic command is complete.
	 *
	 * Note: the ADD CONSTRAINT command must also execute after any index
	 * creation commands.  Thus, this should run after
	 * transformIndexConstraints, so that the CREATE INDEX commands are
	 * already in cxt->alist.
	 */
	if (!isAddConstraint)
	{
		AlterTableStmt *alterstmt = makeNode(AlterTableStmt);
		List	   *fkclist;

		alterstmt->subtype = 'c'; /* preprocessed add constraint */
		alterstmt->relation = cxt->relation;
		alterstmt->name = NULL;
		alterstmt->def = (Node *) cxt->fkconstraints;

		/* Don't need to scan the table contents in this case */
		foreach(fkclist, cxt->fkconstraints)
		{
			FkConstraint *fkconstraint = (FkConstraint *) lfirst(fkclist);

			fkconstraint->skip_validation = true;
		}

		cxt->alist = lappend(cxt->alist, (Node *) alterstmt);
	}
}

/*
 * transformIndexStmt -
 *	  transforms the qualification of the index statement
 */
static Query *
transformIndexStmt(ParseState *pstate, IndexStmt *stmt)
{
	Query	   *qry;
	RangeTblEntry *rte;

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;

	/* take care of the where clause */
	if (stmt->whereClause)
	{
		/*
		 * Put the parent table into the rtable so that the WHERE clause
		 * can refer to its fields without qualification.  Note that this
		 * only works if the parent table already exists --- so we can't
		 * easily support predicates on indexes created implicitly by
		 * CREATE TABLE. Fortunately, that's not necessary.
		 */
		rte = addRangeTableEntry(pstate, stmt->relation, NULL, false, true);

		/* no to join list, yes to namespace */
		addRTEtoQuery(pstate, rte, false, true);

		stmt->whereClause = transformWhereClause(pstate, stmt->whereClause);
	}

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
transformRuleStmt(ParseState *pstate, RuleStmt *stmt,
				  List **extras_before, List **extras_after)
{
	Query	   *qry;
	RangeTblEntry *oldrte;
	RangeTblEntry *newrte;

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;
	qry->utilityStmt = (Node *) stmt;

	/*
	 * To avoid deadlock, make sure the first thing we do is grab
	 * AccessExclusiveLock on the target relation.	This will be needed by
	 * DefineQueryRewrite(), and we don't want to grab a lesser lock
	 * beforehand.	We don't need to hold a refcount on the relcache
	 * entry, however.
	 */
	heap_close(heap_openrv(stmt->relation, AccessExclusiveLock),
			   NoLock);

	/*
	 * NOTE: 'OLD' must always have a varno equal to 1 and 'NEW' equal to
	 * 2.  Set up their RTEs in the main pstate for use in parsing the
	 * rule qualification.
	 */
	Assert(pstate->p_rtable == NIL);
	oldrte = addRangeTableEntry(pstate, stmt->relation,
								makeAlias("*OLD*", NIL),
								false, true);
	newrte = addRangeTableEntry(pstate, stmt->relation,
								makeAlias("*NEW*", NIL),
								false, true);
	/* Must override addRangeTableEntry's default access-check flags */
	oldrte->checkForRead = false;
	newrte->checkForRead = false;

	/*
	 * They must be in the namespace too for lookup purposes, but only add
	 * the one(s) that are relevant for the current kind of rule.  In an
	 * UPDATE rule, quals must refer to OLD.field or NEW.field to be
	 * unambiguous, but there's no need to be so picky for INSERT &
	 * DELETE. (Note we marked the RTEs "inFromCl = true" above to allow
	 * unqualified references to their fields.)  We do not add them to the
	 * joinlist.
	 */
	switch (stmt->event)
	{
		case CMD_SELECT:
			addRTEtoQuery(pstate, oldrte, false, true);
			break;
		case CMD_UPDATE:
			addRTEtoQuery(pstate, oldrte, false, true);
			addRTEtoQuery(pstate, newrte, false, true);
			break;
		case CMD_INSERT:
			addRTEtoQuery(pstate, newrte, false, true);
			break;
		case CMD_DELETE:
			addRTEtoQuery(pstate, oldrte, false, true);
			break;
		default:
			elog(ERROR, "transformRuleStmt: unexpected event type %d",
				 (int) stmt->event);
			break;
	}

	/* take care of the where clause */
	stmt->whereClause = transformWhereClause(pstate, stmt->whereClause);

	if (length(pstate->p_rtable) != 2)	/* naughty, naughty... */
		elog(ERROR, "Rule WHERE condition may not contain references to other relations");

	/* aggregates not allowed (but subselects are okay) */
	if (contain_agg_clause(stmt->whereClause))
		elog(ERROR, "Rule WHERE condition may not contain aggregate functions");

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
		nothing_qry->jointree = makeFromExpr(NIL, NULL);		/* no join wanted */

		stmt->actions = makeList1(nothing_qry);
	}
	else
	{
		List	   *oldactions;
		List	   *newactions = NIL;

		/*
		 * transform each statement, like parse_analyze()
		 */
		foreach(oldactions, stmt->actions)
		{
			Node	   *action = (Node *) lfirst(oldactions);
			ParseState *sub_pstate = make_parsestate(pstate->parentParseState);
			Query	   *sub_qry,
					   *top_subqry;
			bool		has_old,
						has_new;

			/*
			 * Set up OLD/NEW in the rtable for this statement.  The
			 * entries are marked not inFromCl because we don't want them
			 * to be referred to by unqualified field names nor "*" in the
			 * rule actions.  We must add them to the namespace, however,
			 * or they won't be accessible at all.  We decide later
			 * whether to put them in the joinlist.
			 */
			oldrte = addRangeTableEntry(sub_pstate, stmt->relation,
										makeAlias("*OLD*", NIL),
										false, false);
			newrte = addRangeTableEntry(sub_pstate, stmt->relation,
										makeAlias("*NEW*", NIL),
										false, false);
			oldrte->checkForRead = false;
			newrte->checkForRead = false;
			addRTEtoQuery(sub_pstate, oldrte, false, true);
			addRTEtoQuery(sub_pstate, newrte, false, true);

			/* Transform the rule action statement */
			top_subqry = transformStmt(sub_pstate, action,
									   extras_before, extras_after);

			/*
			 * We cannot support utility-statement actions (eg NOTIFY)
			 * with nonempty rule WHERE conditions, because there's no way
			 * to make the utility action execute conditionally.
			 */
			if (top_subqry->commandType == CMD_UTILITY &&
				stmt->whereClause != NULL)
				elog(ERROR, "Rules with WHERE conditions may only have SELECT, INSERT, UPDATE, or DELETE actions");

			/*
			 * If the action is INSERT...SELECT, OLD/NEW have been pushed
			 * down into the SELECT, and that's what we need to look at.
			 * (Ugly kluge ... try to fix this when we redesign
			 * querytrees.)
			 */
			sub_qry = getInsertSelectQuery(top_subqry, NULL);

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
			 * For efficiency's sake, add OLD to the rule action's
			 * jointree only if it was actually referenced in the
			 * statement or qual.
			 *
			 * For INSERT, NEW is not really a relation (only a reference to
			 * the to-be-inserted tuple) and should never be added to the
			 * jointree.
			 *
			 * For UPDATE, we treat NEW as being another kind of reference to
			 * OLD, because it represents references to *transformed*
			 * tuples of the existing relation.  It would be wrong to
			 * enter NEW separately in the jointree, since that would
			 * cause a double join of the updated relation.  It's also
			 * wrong to fail to make a jointree entry if only NEW and not
			 * OLD is mentioned.
			 */
			if (has_old || (has_new && stmt->event == CMD_UPDATE))
			{
				/* hack so we can use addRTEtoQuery() */
				sub_pstate->p_rtable = sub_qry->rtable;
				sub_pstate->p_joinlist = sub_qry->jointree->fromlist;
				addRTEtoQuery(sub_pstate, oldrte, true, false);
				sub_qry->jointree->fromlist = sub_pstate->p_joinlist;
			}

			newactions = lappend(newactions, top_subqry);

			release_pstate_resources(sub_pstate);
			pfree(sub_pstate);
		}

		stmt->actions = newactions;
	}

	return qry;
}


/*
 * transformSelectStmt -
 *	  transforms a Select Statement
 *
 * Note: this is also used for DECLARE CURSOR statements.
 */
static Query *
transformSelectStmt(ParseState *pstate, SelectStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	Node	   *qual;

	qry->commandType = CMD_SELECT;

	if (stmt->portalname)
	{
		/* DECLARE CURSOR */
		if (stmt->into)
			elog(ERROR, "DECLARE CURSOR must not specify INTO");
		if (stmt->forUpdate)
			elog(ERROR, "DECLARE/UPDATE is not supported"
				 "\n\tCursors must be READ ONLY");
		qry->into = makeNode(RangeVar);
		qry->into->relname = stmt->portalname;
		qry->isPortal = TRUE;
		qry->isBinary = stmt->binary;	/* internal portal */
	}
	else
	{
		/* SELECT */
		qry->into = stmt->into;
		qry->isPortal = FALSE;
		qry->isBinary = FALSE;
	}

	/* make FOR UPDATE clause available to addRangeTableEntry */
	pstate->p_forUpdate = stmt->forUpdate;

	/* process the FROM clause */
	transformFromClause(pstate, stmt->fromClause);

	/* transform targetlist */
	qry->targetList = transformTargetList(pstate, stmt->targetList);

	if (stmt->intoColNames)
		applyColumnNames(qry->targetList, stmt->intoColNames);

	/* transform WHERE */
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

	qry->limitOffset = stmt->limitOffset;
	qry->limitCount = stmt->limitCount;

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs || qry->groupClause || qry->havingQual)
		parseCheckAggregates(pstate, qry, qual);

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

	if (stmt->forUpdate != NIL)
		transformForUpdate(qry, stmt->forUpdate);

	return qry;
}

/*
 * transformSetOperationsStmt -
 *	  transforms a set-operations tree
 *
 * A set-operation tree is just a SELECT, but with UNION/INTERSECT/EXCEPT
 * structure to it.  We must transform each leaf SELECT and build up a top-
 * level Query that contains the leaf SELECTs as subqueries in its rangetable.
 * The tree of set operations is converted into the setOperations field of
 * the top-level Query.
 */
static Query *
transformSetOperationStmt(ParseState *pstate, SelectStmt *stmt)
{
	Query	   *qry = makeNode(Query);
	SelectStmt *leftmostSelect;
	int			leftmostRTI;
	Query	   *leftmostQuery;
	SetOperationStmt *sostmt;
	RangeVar   *into;
	List	   *intoColNames;
	char	   *portalname;
	bool		binary;
	List	   *sortClause;
	Node	   *limitOffset;
	Node	   *limitCount;
	List	   *forUpdate;
	Node	   *node;
	List	   *lefttl,
			   *dtlist,
			   *targetvars,
			   *targetnames,
			   *sv_namespace,
			   *sv_rtable;
	RangeTblEntry *jrte;
	RangeTblRef *jrtr;
	int			tllen;

	qry->commandType = CMD_SELECT;

	/*
	 * Find leftmost leaf SelectStmt; extract the one-time-only items from
	 * it and from the top-level node.
	 */
	leftmostSelect = stmt->larg;
	while (leftmostSelect && leftmostSelect->op != SETOP_NONE)
		leftmostSelect = leftmostSelect->larg;
	Assert(leftmostSelect && IsA(leftmostSelect, SelectStmt) &&
		   leftmostSelect->larg == NULL);
	into = leftmostSelect->into;
	intoColNames = leftmostSelect->intoColNames;
	portalname = stmt->portalname;
	binary = stmt->binary;

	/* clear them to prevent complaints in transformSetOperationTree() */
	leftmostSelect->into = NULL;
	leftmostSelect->intoColNames = NIL;
	stmt->portalname = NULL;
	stmt->binary = false;

	/*
	 * These are not one-time, exactly, but we want to process them here
	 * and not let transformSetOperationTree() see them --- else it'll
	 * just recurse right back here!
	 */
	sortClause = stmt->sortClause;
	limitOffset = stmt->limitOffset;
	limitCount = stmt->limitCount;
	forUpdate = stmt->forUpdate;

	stmt->sortClause = NIL;
	stmt->limitOffset = NULL;
	stmt->limitCount = NULL;
	stmt->forUpdate = NIL;

	/* We don't support forUpdate with set ops at the moment. */
	if (forUpdate)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with UNION/INTERSECT/EXCEPT");

	/*
	 * Recursively transform the components of the tree.
	 */
	sostmt = (SetOperationStmt *) transformSetOperationTree(pstate, stmt);
	Assert(sostmt && IsA(sostmt, SetOperationStmt));
	qry->setOperations = (Node *) sostmt;

	/*
	 * Re-find leftmost SELECT (now it's a sub-query in rangetable)
	 */
	node = sostmt->larg;
	while (node && IsA(node, SetOperationStmt))
		node = ((SetOperationStmt *) node)->larg;
	Assert(node && IsA(node, RangeTblRef));
	leftmostRTI = ((RangeTblRef *) node)->rtindex;
	leftmostQuery = rt_fetch(leftmostRTI, pstate->p_rtable)->subquery;
	Assert(leftmostQuery != NULL);

	/*
	 * Generate dummy targetlist for outer query using column names of
	 * leftmost select and common datatypes of topmost set operation. Also
	 * make lists of the dummy vars and their names for use in parsing
	 * ORDER BY.
	 *
	 * Note: we use leftmostRTI as the varno of the dummy variables.
	 * It shouldn't matter too much which RT index they have, as long
	 * as they have one that corresponds to a real RT entry; else funny
	 * things may happen when the tree is mashed by rule rewriting.
	 */
	qry->targetList = NIL;
	targetvars = NIL;
	targetnames = NIL;
	lefttl = leftmostQuery->targetList;
	foreach(dtlist, sostmt->colTypes)
	{
		Oid			colType = (Oid) lfirsti(dtlist);
		Resdom	   *leftResdom = ((TargetEntry *) lfirst(lefttl))->resdom;
		char	   *colName = pstrdup(leftResdom->resname);
		Resdom	   *resdom;
		Node	   *expr;

		resdom = makeResdom((AttrNumber) pstate->p_last_resno++,
							colType,
							-1,
							colName,
							false);
		expr = (Node *) makeVar(leftmostRTI,
								leftResdom->resno,
								colType,
								-1,
								0);
		qry->targetList = lappend(qry->targetList,
								  makeTargetEntry(resdom, expr));
		targetvars = lappend(targetvars, expr);
		targetnames = lappend(targetnames, makeString(colName));
		lefttl = lnext(lefttl);
	}

	/*
	 * Insert one-time items into top-level query
	 *
	 * This needs to agree with transformSelectStmt!
	 */
	if (portalname)
	{
		/* DECLARE CURSOR */
		if (into)
			elog(ERROR, "DECLARE CURSOR must not specify INTO");
		if (forUpdate)
			elog(ERROR, "DECLARE/UPDATE is not supported"
				 "\n\tCursors must be READ ONLY");
		qry->into = makeNode(RangeVar);
		qry->into->relname = portalname;
		qry->isPortal = TRUE;
		qry->isBinary = binary; /* internal portal */
	}
	else
	{
		/* SELECT */
		qry->into = into;
		qry->isPortal = FALSE;
		qry->isBinary = FALSE;
	}

	/*
	 * Any column names from CREATE TABLE AS need to be attached to both
	 * the top level and the leftmost subquery.  We do not do this earlier
	 * because we do *not* want the targetnames list to be affected.
	 */
	if (intoColNames)
	{
		applyColumnNames(qry->targetList, intoColNames);
		applyColumnNames(leftmostQuery->targetList, intoColNames);
	}

	/*
	 * As a first step towards supporting sort clauses that are
	 * expressions using the output columns, generate a namespace entry
	 * that makes the output columns visible.  A Join RTE node is handy
	 * for this, since we can easily control the Vars generated upon
	 * matches.
	 *
	 * Note: we don't yet do anything useful with such cases, but at least
	 * "ORDER BY upper(foo)" will draw the right error message rather than
	 * "foo not found".
	 */
	jrte = addRangeTableEntryForJoin(NULL,
									 targetnames,
									 JOIN_INNER,
									 targetvars,
									 NULL,
									 true);
	jrtr = makeNode(RangeTblRef);
	jrtr->rtindex = 1;

	sv_rtable = pstate->p_rtable;
	pstate->p_rtable = makeList1(jrte);

	sv_namespace = pstate->p_namespace;
	pstate->p_namespace = makeList1(jrtr);

	/*
	 * For now, we don't support resjunk sort clauses on the output of a
	 * setOperation tree --- you can only use the SQL92-spec options of
	 * selecting an output column by name or number.  Enforce by checking
	 * that transformSortClause doesn't add any items to tlist.
	 */
	tllen = length(qry->targetList);

	qry->sortClause = transformSortClause(pstate,
										  sortClause,
										  qry->targetList);

	pstate->p_namespace = sv_namespace;
	pstate->p_rtable = sv_rtable;

	if (tllen != length(qry->targetList))
		elog(ERROR, "ORDER BY on a UNION/INTERSECT/EXCEPT result must be on one of the result columns");

	qry->limitOffset = limitOffset;
	qry->limitCount = limitCount;

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs || qry->groupClause || qry->havingQual)
		parseCheckAggregates(pstate, qry, NULL);

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, NULL);

	if (forUpdate != NIL)
		transformForUpdate(qry, forUpdate);

	return qry;
}

/*
 * transformSetOperationTree
 *		Recursively transform leaves and internal nodes of a set-op tree
 */
static Node *
transformSetOperationTree(ParseState *pstate, SelectStmt *stmt)
{
	bool		isLeaf;

	Assert(stmt && IsA(stmt, SelectStmt));

	/*
	 * Validity-check both leaf and internal SELECTs for disallowed ops.
	 */
	if (stmt->into)
		elog(ERROR, "INTO is only allowed on first SELECT of UNION/INTERSECT/EXCEPT");
	if (stmt->portalname)		/* should not happen */
		elog(ERROR, "Portal may not appear in UNION/INTERSECT/EXCEPT");
	/* We don't support forUpdate with set ops at the moment. */
	if (stmt->forUpdate)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with UNION/INTERSECT/EXCEPT");

	/*
	 * If an internal node of a set-op tree has ORDER BY, UPDATE, or LIMIT
	 * clauses attached, we need to treat it like a leaf node to generate
	 * an independent sub-Query tree.  Otherwise, it can be represented by
	 * a SetOperationStmt node underneath the parent Query.
	 */
	if (stmt->op == SETOP_NONE)
	{
		Assert(stmt->larg == NULL && stmt->rarg == NULL);
		isLeaf = true;
	}
	else
	{
		Assert(stmt->larg != NULL && stmt->rarg != NULL);
		if (stmt->sortClause || stmt->limitOffset || stmt->limitCount ||
			stmt->forUpdate)
			isLeaf = true;
		else
			isLeaf = false;
	}

	if (isLeaf)
	{
		/* Process leaf SELECT */
		List	   *selectList;
		Query	   *selectQuery;
		char		selectName[32];
		RangeTblEntry *rte;
		RangeTblRef *rtr;

		/*
		 * Transform SelectStmt into a Query.
		 *
		 * Note: previously transformed sub-queries don't affect the parsing
		 * of this sub-query, because they are not in the toplevel
		 * pstate's namespace list.
		 */
		selectList = parse_analyze((Node *) stmt, pstate);

		Assert(length(selectList) == 1);
		selectQuery = (Query *) lfirst(selectList);

		/*
		 * Make the leaf query be a subquery in the top-level rangetable.
		 */
		snprintf(selectName, sizeof(selectName), "*SELECT* %d", length(pstate->p_rtable) + 1);
		rte = addRangeTableEntryForSubquery(pstate,
											selectQuery,
											makeAlias(selectName, NIL),
											false);

		/*
		 * Return a RangeTblRef to replace the SelectStmt in the set-op
		 * tree.
		 */
		rtr = makeNode(RangeTblRef);
		/* assume new rte is at end */
		rtr->rtindex = length(pstate->p_rtable);
		Assert(rte == rt_fetch(rtr->rtindex, pstate->p_rtable));
		return (Node *) rtr;
	}
	else
	{
		/* Process an internal node (set operation node) */
		SetOperationStmt *op = makeNode(SetOperationStmt);
		List	   *lcoltypes;
		List	   *rcoltypes;
		const char *context;

		context = (stmt->op == SETOP_UNION ? "UNION" :
				   (stmt->op == SETOP_INTERSECT ? "INTERSECT" :
					"EXCEPT"));

		op->op = stmt->op;
		op->all = stmt->all;

		/*
		 * Recursively transform the child nodes.
		 */
		op->larg = transformSetOperationTree(pstate, stmt->larg);
		op->rarg = transformSetOperationTree(pstate, stmt->rarg);

		/*
		 * Verify that the two children have the same number of non-junk
		 * columns, and determine the types of the merged output columns.
		 */
		lcoltypes = getSetColTypes(pstate, op->larg);
		rcoltypes = getSetColTypes(pstate, op->rarg);
		if (length(lcoltypes) != length(rcoltypes))
			elog(ERROR, "Each %s query must have the same number of columns",
				 context);
		op->colTypes = NIL;
		while (lcoltypes != NIL)
		{
			Oid			lcoltype = (Oid) lfirsti(lcoltypes);
			Oid			rcoltype = (Oid) lfirsti(rcoltypes);
			Oid			rescoltype;

			rescoltype = select_common_type(makeListi2(lcoltype, rcoltype),
											context);
			op->colTypes = lappendi(op->colTypes, rescoltype);
			lcoltypes = lnext(lcoltypes);
			rcoltypes = lnext(rcoltypes);
		}

		return (Node *) op;
	}
}

/*
 * getSetColTypes
 *		Get output column types of an (already transformed) set-op node
 */
static List *
getSetColTypes(ParseState *pstate, Node *node)
{
	if (IsA(node, RangeTblRef))
	{
		RangeTblRef *rtr = (RangeTblRef *) node;
		RangeTblEntry *rte = rt_fetch(rtr->rtindex, pstate->p_rtable);
		Query	   *selectQuery = rte->subquery;
		List	   *result = NIL;
		List	   *tl;

		Assert(selectQuery != NULL);
		/* Get types of non-junk columns */
		foreach(tl, selectQuery->targetList)
		{
			TargetEntry *tle = (TargetEntry *) lfirst(tl);
			Resdom	   *resnode = tle->resdom;

			if (resnode->resjunk)
				continue;
			result = lappendi(result, resnode->restype);
		}
		return result;
	}
	else if (IsA(node, SetOperationStmt))
	{
		SetOperationStmt *op = (SetOperationStmt *) node;

		/* Result already computed during transformation of node */
		Assert(op->colTypes != NIL);
		return op->colTypes;
	}
	else
	{
		elog(ERROR, "getSetColTypes: unexpected node %d",
			 (int) nodeTag(node));
		return NIL;				/* keep compiler quiet */
	}
}

/* Attach column names from a ColumnDef list to a TargetEntry list */
static void
applyColumnNames(List *dst, List *src)
{
	if (length(src) > length(dst))
		elog(ERROR, "CREATE TABLE AS specifies too many column names");

	while (src != NIL && dst != NIL)
	{
		TargetEntry *d = (TargetEntry *) lfirst(dst);
		ColumnDef  *s = (ColumnDef *) lfirst(src);

		Assert(d->resdom && !d->resdom->resjunk);

		d->resdom->resname = pstrdup(s->colname);

		dst = lnext(dst);
		src = lnext(src);
	}
}


/*
 * transformUpdateStmt -
 *	  transforms an update statement
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

	qry->resultRelation = setTargetTable(pstate, stmt->relation,
							  interpretInhOption(stmt->relation->inhOpt),
										 true);

	/*
	 * the FROM clause is non-standard SQL syntax. We used to be able to
	 * do this with REPLACE in POSTQUEL so we keep the feature.
	 */
	transformFromClause(pstate, stmt->fromClause);

	qry->targetList = transformTargetList(pstate, stmt->targetList);

	qual = transformWhereClause(pstate, stmt->whereClause);

	qry->rtable = pstate->p_rtable;
	qry->jointree = makeFromExpr(pstate->p_joinlist, qual);

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
											origTarget->name, true),
							  origTarget->indirection);
		origTargetList = lnext(origTargetList);
	}
	if (origTargetList != NIL)
		elog(ERROR, "UPDATE target count mismatch --- internal error");

	return qry;
}

/*
 * tranformAlterTableStmt -
 *	transform an Alter Table Statement
 */
static Query *
transformAlterTableStmt(ParseState *pstate, AlterTableStmt *stmt,
						List **extras_before, List **extras_after)
{
	Relation	rel;
	CreateStmtContext cxt;
	Query	   *qry;

	/*
	 * The only subtypes that currently require parse transformation
	 * handling are 'A'dd column and Add 'C'onstraint.	These largely
	 * re-use code from CREATE TABLE.
	 *
	 * If we need to do any parse transformation, get exclusive lock on
	 * the relation to make sure it won't change before we execute the
	 * command.
	 */
	switch (stmt->subtype)
	{
		case 'A':
			rel = heap_openrv(stmt->relation, AccessExclusiveLock);

			cxt.stmtType = "ALTER TABLE";
			cxt.relation = stmt->relation;
			cxt.inhRelations = NIL;
			cxt.relOid = RelationGetRelid(rel);
			cxt.hasoids = SearchSysCacheExists(ATTNUM,
											ObjectIdGetDatum(cxt.relOid),
								  Int16GetDatum(ObjectIdAttributeNumber),
											   0, 0);
			cxt.columns = NIL;
			cxt.ckconstraints = NIL;
			cxt.fkconstraints = NIL;
			cxt.ixconstraints = NIL;
			cxt.blist = NIL;
			cxt.alist = NIL;
			cxt.pkey = NULL;

			Assert(IsA(stmt->def, ColumnDef));
			transformColumnDefinition(pstate, &cxt,
									  (ColumnDef *) stmt->def);

			transformIndexConstraints(pstate, &cxt);
			transformFKConstraints(pstate, &cxt, false);

			((ColumnDef *) stmt->def)->constraints = cxt.ckconstraints;
			*extras_before = nconc(*extras_before, cxt.blist);
			*extras_after = nconc(cxt.alist, *extras_after);

			heap_close(rel, NoLock); /* close rel, keep lock */
			break;

		case 'C':
			rel = heap_openrv(stmt->relation, AccessExclusiveLock);

			cxt.stmtType = "ALTER TABLE";
			cxt.relation = stmt->relation;
			cxt.inhRelations = NIL;
			cxt.relOid = RelationGetRelid(rel);
			cxt.hasoids = SearchSysCacheExists(ATTNUM,
											ObjectIdGetDatum(cxt.relOid),
								  Int16GetDatum(ObjectIdAttributeNumber),
											   0, 0);
			cxt.columns = NIL;
			cxt.ckconstraints = NIL;
			cxt.fkconstraints = NIL;
			cxt.ixconstraints = NIL;
			cxt.blist = NIL;
			cxt.alist = NIL;
			cxt.pkey = NULL;

			if (IsA(stmt->def, Constraint))
				transformTableConstraint(pstate, &cxt,
										 (Constraint *) stmt->def);
			else if (IsA(stmt->def, FkConstraint))
				cxt.fkconstraints = lappend(cxt.fkconstraints, stmt->def);
			else
				elog(ERROR, "Unexpected node type in ALTER TABLE ADD CONSTRAINT");

			transformIndexConstraints(pstate, &cxt);
			transformFKConstraints(pstate, &cxt, true);

			Assert(cxt.columns == NIL);
			/* fkconstraints should be put into my own stmt in this case */
			stmt->def = (Node *) nconc(cxt.ckconstraints, cxt.fkconstraints);
			*extras_before = nconc(*extras_before, cxt.blist);
			*extras_after = nconc(cxt.alist, *extras_after);

			heap_close(rel, NoLock); /* close rel, keep lock */
			break;

		case 'c':

			/*
			 * Already-transformed ADD CONSTRAINT, so just make it look
			 * like the standard case.
			 */
			stmt->subtype = 'C';
			break;

		default:
			break;
	}

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;
	qry->utilityStmt = (Node *) stmt;

	return qry;
}

static Query *
transformPrepareStmt(ParseState *pstate, PrepareStmt *stmt)
{
	Query	   *result = makeNode(Query);
	List	   *extras_before = NIL,
			   *extras_after = NIL;
	List	   *argtype_oids = NIL;		/* argtype OIDs in a list */
	Oid		   *argtoids = NULL;	/* as an array for parser_param_set */
	int			nargs;

	result->commandType = CMD_UTILITY;
	result->utilityStmt = (Node *) stmt;

	/* Transform list of TypeNames to list (and array) of type OIDs */
	nargs = length(stmt->argtypes);

	if (nargs)
	{
		List	   *l;
		int			i = 0;

		argtoids = (Oid *) palloc(nargs * sizeof(Oid));

		foreach(l, stmt->argtypes)
		{
			TypeName   *tn = lfirst(l);
			Oid			toid = typenameTypeId(tn);

			argtype_oids = lappendi(argtype_oids, toid);
			argtoids[i++] = toid;
		}
	}

	stmt->argtype_oids = argtype_oids;

	/*
	 * We need to adjust the parameters expected by the rest of the
	 * system, so that $1, ... $n are parsed properly.
	 *
	 * This is somewhat of a hack; however, the main parser interface only
	 * allows parameters to be specified when working with a raw query
	 * string, which is not helpful here.
	 */
	parser_param_set(argtoids, nargs);

	stmt->query = transformStmt(pstate, (Node *) stmt->query,
								&extras_before, &extras_after);

	/* Shouldn't get any extras, since grammar only allows OptimizableStmt */
	if (extras_before || extras_after)
		elog(ERROR, "transformPrepareStmt: internal error");

	/* Remove links to our local parameters */
	parser_param_set(NULL, 0);

	return result;
}

static Query *
transformExecuteStmt(ParseState *pstate, ExecuteStmt *stmt)
{
	Query	   *result = makeNode(Query);
	List	   *paramtypes;

	result->commandType = CMD_UTILITY;
	result->utilityStmt = (Node *) stmt;

	paramtypes = FetchQueryParams(stmt->name);

	if (stmt->params || paramtypes)
	{
		int			nparams = length(stmt->params);
		int			nexpected = length(paramtypes);
		List	   *l;
		int			i = 1;

		if (nparams != nexpected)
			elog(ERROR, "Wrong number of parameters, expected %d but got %d",
				 nexpected, nparams);

		foreach(l, stmt->params)
		{
			Node	   *expr = lfirst(l);
			Oid			expected_type_id,
						given_type_id;

			expr = transformExpr(pstate, expr);

			/* Cannot contain subselects or aggregates */
			if (contain_subplans(expr))
				elog(ERROR, "Cannot use subselects in EXECUTE parameters");
			if (contain_agg_clause(expr))
				elog(ERROR, "Cannot use aggregates in EXECUTE parameters");

			given_type_id = exprType(expr);
			expected_type_id = (Oid) lfirsti(paramtypes);

			expr = coerce_to_target_type(expr, given_type_id,
										 expected_type_id, -1,
										 COERCION_ASSIGNMENT,
										 COERCE_IMPLICIT_CAST);

			if (expr == NULL)
				elog(ERROR, "Parameter $%d of type %s cannot be coerced into the expected type %s"
					 "\n\tYou will need to rewrite or cast the expression",
					 i,
					 format_type_be(given_type_id),
					 format_type_be(expected_type_id));

			fix_opids(expr);

			lfirst(l) = expr;

			paramtypes = lnext(paramtypes);
			i++;
		}
	}

	return result;
}

/* exported so planner can check again after rewriting, query pullup, etc */
void
CheckSelectForUpdate(Query *qry)
{
	if (qry->setOperations)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with UNION/INTERSECT/EXCEPT");
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
	List	   *rowMarks = qry->rowMarks;
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
			if (rte->rtekind == RTE_SUBQUERY)
			{
				/* FOR UPDATE of subquery is propagated to subquery's rels */
				transformForUpdate(rte->subquery, makeList1(NULL));
			}
			else
			{
				if (!intMember(i, rowMarks))	/* avoid duplicates */
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
			char	   *relname = strVal(lfirst(l));

			i = 0;
			foreach(rt, qry->rtable)
			{
				RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);

				++i;
				if (strcmp(rte->eref->aliasname, relname) == 0)
				{
					if (rte->rtekind == RTE_SUBQUERY)
					{
						/* propagate to subquery */
						transformForUpdate(rte->subquery, makeList1(NULL));
					}
					else
					{
						if (!intMember(i, rowMarks))	/* avoid duplicates */
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
 * relationHasPrimaryKey -
 *
 *	See whether an existing relation has a primary key.
 */
static bool
relationHasPrimaryKey(Oid relationOid)
{
	bool		result = false;
	Relation	rel;
	List	   *indexoidlist,
			   *indexoidscan;

	rel = heap_open(relationOid, AccessShareLock);

	/*
	 * Get the list of index OIDs for the table from the relcache, and
	 * look up each one in the pg_index syscache until we find one marked
	 * primary key (hopefully there isn't more than one such).
	 */
	indexoidlist = RelationGetIndexList(rel);

	foreach(indexoidscan, indexoidlist)
	{
		Oid			indexoid = lfirsti(indexoidscan);
		HeapTuple	indexTuple;

		indexTuple = SearchSysCache(INDEXRELID,
									ObjectIdGetDatum(indexoid),
									0, 0, 0);
		if (!HeapTupleIsValid(indexTuple))
			elog(ERROR, "relationHasPrimaryKey: index %u not found",
				 indexoid);
		result = ((Form_pg_index) GETSTRUCT(indexTuple))->indisprimary;
		ReleaseSysCache(indexTuple);
		if (result)
			break;
	}

	freeList(indexoidlist);

	heap_close(rel, AccessShareLock);

	return result;
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
	FromExpr   *f = makeNode(FromExpr);

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
	TypeName   *typename = column->typename;
	Type		ctype = typenameType(typename);

	/*
	 * Is this the name of a complex type? If so, implement it as a set.
	 *
	 * XXX this is a hangover from ancient Berkeley code that probably
	 * doesn't work anymore anyway.
	 */
	if (typeTypeRelid(ctype) != InvalidOid)
	{
		/*
		 * (Eventually add in here that the set can only contain one
		 * element.)
		 */
		typename->setof = true;
	}

	ReleaseSysCache(ctype);
}

/*
 * analyzeCreateSchemaStmt -
 *	  analyzes the "create schema" statement
 *
 * Split the schema element list into individual commands and place
 * them in the result list in an order such that there are no
 * forward references (e.g. GRANT to a table created later in the list).
 *
 * SQL92 also allows constraints to make forward references, so thumb through
 * the table columns and move forward references to a posterior alter-table
 * command.
 *
 * The result is a list of parse nodes that still need to be analyzed ---
 * but we can't analyze the later commands until we've executed the earlier
 * ones, because of possible inter-object references.
 *
 * Note: Called from commands/command.c
 */
List *
analyzeCreateSchemaStmt(CreateSchemaStmt *stmt)
{
	CreateSchemaStmtContext cxt;
	List	   *result;
	List	   *elements;

	cxt.stmtType = "CREATE SCHEMA";
	cxt.schemaname = stmt->schemaname;
	cxt.authid = stmt->authid;
	cxt.tables = NIL;
	cxt.views = NIL;
	cxt.grants = NIL;
	cxt.fwconstraints = NIL;
	cxt.alters = NIL;
	cxt.blist = NIL;
	cxt.alist = NIL;

	/*
	 * Run through each schema element in the schema element list.
	 * Separate statements by type, and do preliminary analysis.
	 */
	foreach(elements, stmt->schemaElts)
	{
		Node	   *element = lfirst(elements);

		switch (nodeTag(element))
		{
			case T_CreateStmt:
				{
					CreateStmt *elp = (CreateStmt *) element;

					if (elp->relation->schemaname == NULL)
						elp->relation->schemaname = cxt.schemaname;
					else if (strcmp(cxt.schemaname, elp->relation->schemaname) != 0)
						elog(ERROR, "New table specifies a schema (%s)"
							 " different from the one being created (%s)",
							 elp->relation->schemaname, cxt.schemaname);

					/*
					 * XXX todo: deal with constraints
					 */

					cxt.tables = lappend(cxt.tables, element);
				}
				break;

			case T_ViewStmt:
				{
					ViewStmt   *elp = (ViewStmt *) element;

					if (elp->view->schemaname == NULL)
						elp->view->schemaname = cxt.schemaname;
					else if (strcmp(cxt.schemaname, elp->view->schemaname) != 0)
						elog(ERROR, "New view specifies a schema (%s)"
							 " different from the one being created (%s)",
							 elp->view->schemaname, cxt.schemaname);

					/*
					 * XXX todo: deal with references between views
					 */

					cxt.views = lappend(cxt.views, element);
				}
				break;

			case T_GrantStmt:
				cxt.grants = lappend(cxt.grants, element);
				break;

			default:
				elog(ERROR, "parser: unsupported schema node (internal error)");
		}
	}

	result = NIL;
	result = nconc(result, cxt.tables);
	result = nconc(result, cxt.views);
	result = nconc(result, cxt.grants);

	return result;
}
