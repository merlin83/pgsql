/*-------------------------------------------------------------------------
 *
 * analyze.c
 *	  transform the parse tree into a query tree
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *	$Id: analyze.c,v 1.119 1999-09-18 19:07:12 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "parse.h"
#include "parser/analyze.h"
#include "parser/parse_agg.h"
#include "parser/parse_clause.h"
#include "parser/parse_relation.h"
#include "parser/parse_target.h"
#include "utils/builtins.h"

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

static void transformForUpdate(Query *qry, List *forUpdate);
void		CheckSelectForUpdate(Query *qry);

/* kluge to return extra info from transformCreateStmt() */
static List	   *extras_before;
static List	   *extras_after;


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
	ParseState *pstate;
	Query	   *parsetree;

	while (pl != NIL)
	{
		extras_before = extras_after = NIL;
		pstate = make_parsestate(parentParseState);

		parsetree = transformStmt(pstate, lfirst(pl));
		if (pstate->p_target_relation != NULL)
			heap_close(pstate->p_target_relation, AccessShareLock);
		pstate->p_target_relation = NULL;
		pstate->p_target_rangetblentry = NULL;

		while (extras_before != NIL)
		{
			result = lappend(result,
							 transformStmt(pstate, lfirst(extras_before)));
			if (pstate->p_target_relation != NULL)
				heap_close(pstate->p_target_relation, AccessShareLock);
			pstate->p_target_relation = NULL;
			pstate->p_target_rangetblentry = NULL;
			extras_before = lnext(extras_before);
		}

		result = lappend(result, parsetree);

		while (extras_after != NIL)
		{
			result = lappend(result,
							 transformStmt(pstate, lfirst(extras_after)));
			if (pstate->p_target_relation != NULL)
				heap_close(pstate->p_target_relation, AccessShareLock);
			pstate->p_target_relation = NULL;
			pstate->p_target_rangetblentry = NULL;
			extras_after = lnext(extras_after);
		}

		pfree(pstate);
		pl = lnext(pl);
	}

	return result;
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
				result = makeNode(Query);
				result->commandType = CMD_UTILITY;
				result->utilityStmt = (Node *) n;
			}
			break;

		case T_VacuumStmt:
			{
				MemoryContext oldcontext;

				/*
				 * make sure that this Query is allocated in TopMemory
				 * context because vacuum spans transactions and we don't
				 * want to lose the vacuum Query due to end-of-transaction
				 * free'ing
				 */
				oldcontext = MemoryContextSwitchTo(TopMemoryContext);
				result = makeNode(Query);
				result->commandType = CMD_UTILITY;
				result->utilityStmt = (Node *) parseTree;
				MemoryContextSwitchTo(oldcontext);
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

	qry->commandType = CMD_DELETE;

	/* set up a range table */
	makeRangeTable(pstate, NULL, NULL);
	setTargetTable(pstate, stmt->relname);

	qry->uniqueFlag = NULL;

	/* fix where clause */
	qry->qual = transformWhereClause(pstate, stmt->whereClause, NULL);
	qry->hasSubLinks = pstate->p_hasSubLinks;

	qry->rtable = pstate->p_rtable;
	qry->resultRelation = refnameRangeTablePosn(pstate, stmt->relname, NULL);

	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs)
		parseCheckAggregates(pstate, qry);

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
	Node	   *fromQual;
	List	   *icolumns;
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
	 * stand-alone SELECT.  (Indeed, Postgres up through 6.5 had bugs of
	 * just that nature...)
	 *----------
	 */

	/* set up a range table --- note INSERT target is not in it yet */
	makeRangeTable(pstate, stmt->fromClause, &fromQual);

	qry->uniqueFlag = stmt->unique;

	qry->targetList = transformTargetList(pstate, stmt->targetList);

	qry->qual = transformWhereClause(pstate, stmt->whereClause, fromQual);

	/* Initial processing of HAVING clause is just like WHERE clause.
	 * Additional work will be done in optimizer/plan/planner.c.
	 */
	qry->havingQual = transformWhereClause(pstate, stmt->havingClause, NULL);

	qry->groupClause = transformGroupClause(pstate,
											stmt->groupClause,
											qry->targetList);

	/* An InsertStmt has no sortClause, but we still call
	 * transformSortClause because it also handles uniqueFlag.
	 */
	qry->sortClause = transformSortClause(pstate,
										  NIL,
										  qry->targetList,
										  qry->uniqueFlag);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs || qry->groupClause)
		parseCheckAggregates(pstate, qry);

	/*
	 * If there is a havingQual but there are no aggregates, then there is
	 * something wrong with the query because HAVING must contain
	 * aggregates in its expressions! Otherwise the query could have been
	 * formulated using the WHERE clause.
	 */
	if (qry->havingQual && ! qry->hasAggs)
		elog(ERROR, "SELECT/HAVING requires aggregates to be valid");

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
	 * the SELECT part.)
	 */
	setTargetTable(pstate, stmt->relname);

	/* now the range table will not change */
	qry->rtable = pstate->p_rtable;
	qry->resultRelation = refnameRangeTablePosn(pstate, stmt->relname, NULL);

	/* Prepare to assign non-conflicting resnos to resjunk attributes */
	if (pstate->p_last_resno <= pstate->p_target_relation->rd_rel->relnatts)
		pstate->p_last_resno = pstate->p_target_relation->rd_rel->relnatts + 1;

	/* Validate stmt->cols list, or build default list if no list given */
	icolumns = makeTargetNames(pstate, stmt->cols);

	/* Prepare non-junk columns for assignment to target table */
	foreach(tl, qry->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tl);
		Resdom	   *resnode = tle->resdom;
		Ident	   *id;

		if (resnode->resjunk)
		{
			/* Resjunk nodes need no additional processing, but be sure they
			 * have names and resnos that do not match any target columns;
			 * else rewriter or planner might get confused.
			 */
			resnode->resname = "?resjunk?";
			resnode->resno = (AttrNumber) pstate->p_last_resno++;
			continue;
		}
		if (icolumns == NIL)
			elog(ERROR, "INSERT has more expressions than target columns");
		id = (Ident *) lfirst(icolumns);
		updateTargetListEntry(pstate, tle, id->name, id->indirection);
		icolumns = lnext(icolumns);
	}

	/*
	 * Add targetlist items to assign DEFAULT values to any columns that
	 * have defaults and were not assigned to by the user.
	 * XXX wouldn't it make more sense to do this further downstream,
	 * after the rule rewriter?
	 */
	rd_att = pstate->p_target_relation->rd_att;
	if (rd_att->constr && rd_att->constr->num_defval > 0)
	{
		Form_pg_attribute *att = rd_att->attrs;
		AttrDefault *defval = rd_att->constr->defval;
		int			ndef = rd_att->constr->num_defval;

		while (ndef-- > 0)
		{
			Form_pg_attribute thisatt = att[defval[ndef].adnum - 1];
			TargetEntry *te;

			foreach(tl, qry->targetList)
			{
				TargetEntry *tle = (TargetEntry *) lfirst(tl);
				Resdom	   *resnode = tle->resdom;

				if (resnode->resjunk)
					continue;	/* ignore resjunk nodes */
				if (namestrcmp(&(thisatt->attname), resnode->resname) == 0)
					break;
			}
			if (tl != NIL)		/* something given for this attr */
				continue;
			/*
			 * No user-supplied value, so add a targetentry with DEFAULT expr
			 * and correct data for the target column.
			 */
			te = makeTargetEntry(
				makeResdom(defval[ndef].adnum,
						   thisatt->atttypid,
						   thisatt->atttypmod,
						   pstrdup(nameout(&(thisatt->attname))),
						   0, 0, false),
				stringToNode(defval[ndef].adbin));
			qry->targetList = lappend(qry->targetList, te);
			/*
			 * Make sure the value is coerced to the target column type
			 * (might not be right type if it's not a constant!)
			 */
			updateTargetListEntry(pstate, te, te->resdom->resname, NIL);
		}
	}

	if (stmt->forUpdate != NULL)
		transformForUpdate(qry, stmt->forUpdate);

	return (Query *) qry;
}

/*
 *	makeObjectName()
 *
 *	Create a name for an implicitly created index, sequence, constraint, etc.
 *
 *	The parameters are: the original table name, the original field name, and
 *	a "type" string (such as "seq" or "pkey").  The field name and/or type
 *	can be NULL if not relevant.
 *
 *	The result is a palloc'd string.
 *
 *	The basic result we want is "name1_name2_type", omitting "_name2" or
 *	"_type" when those parameters are NULL.  However, we must generate
 *	a name with less than NAMEDATALEN characters!  So, we truncate one or
 *	both names if necessary to make a short-enough string.  The type part
 *	is never truncated (so it had better be reasonably short).
 *
 *	To reduce the probability of collisions, we might someday add more
 *	smarts to this routine, like including some "hash" characters computed
 *	from the truncated characters.  Currently it seems best to keep it simple,
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

	availchars = NAMEDATALEN-1 - overhead;

	/* If we must truncate,  preferentially truncate the longer name.
	 * This logic could be expressed without a loop, but it's simple and
	 * obvious as a loop.
	 */
	while (name1chars + name2chars > availchars)
	{
		if (name1chars > name2chars)
			name1chars--;
		else
			name2chars--;
	}

	/* Now construct the string using the chosen lengths */
	name = palloc(name1chars + name2chars + overhead + 1);
	strncpy(name, name1, name1chars);
	ndx = name1chars;
	if (name2)
	{
		name[ndx++] = '_';
		strncpy(name+ndx, name2, name2chars);
		ndx += name2chars;
	}
	if (typename)
	{
		name[ndx++] = '_';
		strcpy(name+ndx, typename);
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

	/* The type name for makeObjectName is label, or labelN if that's
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
			if (strcasecmp(iname, index->idxname) == 0)
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
	List	   *keys;
	Ident	   *key;
	List	   *blist = NIL;	/* "before list" of things to do before
								 * creating the table */
	List	   *ilist = NIL;	/* "index list" of things to do after
								 * creating the table */
	IndexStmt  *index,
			   *pkey = NULL;
	IndexElem  *iparam;

	q = makeNode(Query);
	q->commandType = CMD_UTILITY;

	elements = stmt->tableElts;
	constraints = stmt->constraints;
	columns = NIL;
	dlist = NIL;

	/*
	 * Run through each primary element in the table creation clause
	 */
	while (elements != NIL)
	{
		element = lfirst(elements);
		switch (nodeTag(element))
		{
			case T_ColumnDef:
				column = (ColumnDef *) element;
				columns = lappend(columns, column);

				/* Special case SERIAL type? */
				if (column->is_sequence)
				{
					char	   *sname;
					char	   *cstring;
					CreateSeqStmt *sequence;

					sname = makeObjectName(stmt->relname, column->colname,
										   "seq");
					constraint = makeNode(Constraint);
					constraint->contype = CONSTR_DEFAULT;
					constraint->name = sname;
					cstring = palloc(10 + strlen(constraint->name) + 3 + 1);
					strcpy(cstring, "nextval('\"");
					strcat(cstring, constraint->name);
					strcat(cstring, "\"')");
					constraint->def = cstring;
					constraint->keys = NULL;

					column->constraints = lappend(column->constraints, constraint);

					constraint = makeNode(Constraint);
					constraint->contype = CONSTR_UNIQUE;
					constraint->name = makeObjectName(stmt->relname,
													  column->colname,
													  "key");
					column->constraints = lappend(column->constraints, constraint);

					sequence = makeNode(CreateSeqStmt);
					sequence->seqname = pstrdup(sname);
					sequence->options = NIL;

					elog(NOTICE, "CREATE TABLE will create implicit sequence '%s' for SERIAL column '%s.%s'",
					  sequence->seqname, stmt->relname, column->colname);

					blist = lcons(sequence, NIL);
				}

				/* Check for column constraints, if any... */
				if (column->constraints != NIL)
				{
					clist = column->constraints;
					while (clist != NIL)
					{
						constraint = lfirst(clist);
						switch (constraint->contype)
						{
							case CONSTR_NULL:

								/*
								 * We should mark this explicitly, so we
								 * can tell if NULL and NOT NULL are both
								 * specified
								 */
								if (column->is_not_null)
									elog(ERROR, "CREATE TABLE/(NOT) NULL conflicting declaration"
										 " for '%s.%s'", stmt->relname, column->colname);
								column->is_not_null = FALSE;
								break;

							case CONSTR_NOTNULL:
								if (column->is_not_null)
									elog(ERROR, "CREATE TABLE/NOT NULL already specified"
										 " for '%s.%s'", stmt->relname, column->colname);
								column->is_not_null = TRUE;
								break;

							case CONSTR_DEFAULT:
								if (column->defval != NULL)
									elog(ERROR, "CREATE TABLE/DEFAULT multiple values specified"
										 " for '%s.%s'", stmt->relname, column->colname);
								column->defval = constraint->def;
								break;

							case CONSTR_PRIMARY:
								if (constraint->name == NULL)
									constraint->name = makeObjectName(stmt->relname, NULL, "pkey");
								if (constraint->keys == NIL)
									constraint->keys = lappend(constraint->keys, column);
								dlist = lappend(dlist, constraint);
								break;

							case CONSTR_UNIQUE:
								if (constraint->name == NULL)
									constraint->name = makeObjectName(stmt->relname, column->colname, "key");
								if (constraint->keys == NIL)
									constraint->keys = lappend(constraint->keys, column);
								dlist = lappend(dlist, constraint);
								break;

							case CONSTR_CHECK:
								constraints = lappend(constraints, constraint);
								if (constraint->name == NULL)
									constraint->name = makeObjectName(stmt->relname, column->colname, NULL);
								break;

							default:
								elog(ERROR, "parser: unrecognized constraint (internal error)", NULL);
								break;
						}
						clist = lnext(clist);
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

					case CONSTR_NOTNULL:
					case CONSTR_DEFAULT:
						elog(ERROR, "parser: illegal context for constraint (internal error)", NULL);
						break;
					default:
						elog(ERROR, "parser: unrecognized constraint (internal error)", NULL);
						break;
				}
				break;

			default:
				elog(ERROR, "parser: unrecognized node (internal error)", NULL);
		}

		elements = lnext(elements);
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
		Assert(nodeTag(constraint) == T_Constraint);
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

		keys = constraint->keys;
		while (keys != NIL)
		{
			key = lfirst(keys);
			columns = stmt->tableElts;
			column = NULL;
			while (columns != NIL)
			{
				column = lfirst(columns);
				if (strcasecmp(column->colname, key->name) == 0)
					break;
				else
					column = NULL;
				columns = lnext(columns);
			}
			if (column == NULL)
				elog(ERROR, "CREATE TABLE column '%s' in key does not exist", key->name);

			if (constraint->contype == CONSTR_PRIMARY)
				column->is_not_null = TRUE;
			iparam = makeNode(IndexElem);
			iparam->name = pstrdup(column->colname);
			iparam->args = NIL;
			iparam->class = NULL;
			iparam->typename = NULL;
			index->indexParams = lappend(index->indexParams, iparam);

			if (index->idxname == NULL)
				index->idxname = CreateIndexName(stmt->relname, iparam->name, "key", ilist);

			keys = lnext(keys);
		}

		if (index->idxname == NULL)	/* should not happen */
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
			List *pcols, *icols;
			int plen, ilen;
			int	keep = TRUE;

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
					IndexElem *pcol = lfirst(pcols);
					IndexElem *icol = lfirst(icols);
					char *pname = pcol->name;
					char *iname = icol->name;

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
	stmt->whereClause = transformWhereClause(pstate, stmt->whereClause, NULL);
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
	stmt->whereClause = transformWhereClause(pstate, stmt->whereClause, NULL);
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
	Query	   *action;
	List	   *actions;

	qry = makeNode(Query);
	qry->commandType = CMD_UTILITY;

	/*
	 * 'instead nothing' rules with a qualification need a query a
	 * rangetable so the rewrite handler can add the negated rule
	 * qualification to the original query. We create a query with the new
	 * command type CMD_NOTHING here that is treated special by the
	 * rewrite system.
	 */
	if (stmt->actions == NIL)
	{
		Query	   *nothing_qry = makeNode(Query);

		nothing_qry->commandType = CMD_NOTHING;

		addRangeTableEntry(pstate, stmt->object->relname, "*CURRENT*",
						   FALSE, FALSE);
		addRangeTableEntry(pstate, stmt->object->relname, "*NEW*",
						   FALSE, FALSE);

		nothing_qry->rtable = pstate->p_rtable;

		stmt->actions = lappend(NIL, nothing_qry);
	}

	actions = stmt->actions;

	/*
	 * transform each statment, like parse_analyze()
	 */
	while (actions != NIL)
	{

		/*
		 * NOTE: 'CURRENT' must always have a varno equal to 1 and 'NEW'
		 * equal to 2.
		 */
		addRangeTableEntry(pstate, stmt->object->relname, "*CURRENT*",
						   FALSE, FALSE);
		addRangeTableEntry(pstate, stmt->object->relname, "*NEW*",
						   FALSE, FALSE);

		pstate->p_last_resno = 1;
		pstate->p_is_rule = true;		/* for expand all */
		pstate->p_hasAggs = false;

		action = (Query *) lfirst(actions);
		if (action->commandType != CMD_NOTHING)
			lfirst(actions) = transformStmt(pstate, lfirst(actions));
		actions = lnext(actions);
	}

	/* take care of the where clause */
	stmt->whereClause = transformWhereClause(pstate, stmt->whereClause, NULL);
	qry->hasSubLinks = pstate->p_hasSubLinks;

	qry->utilityStmt = (Node *) stmt;
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
	Node	   *fromQual;

	qry->commandType = CMD_SELECT;

	/* set up a range table */
	makeRangeTable(pstate, stmt->fromClause, &fromQual);

	qry->uniqueFlag = stmt->unique;

	qry->into = stmt->into;
	qry->isTemp = stmt->istemp;
	qry->isPortal = FALSE;

	qry->targetList = transformTargetList(pstate, stmt->targetList);

	qry->qual = transformWhereClause(pstate, stmt->whereClause, fromQual);

	/* Initial processing of HAVING clause is just like WHERE clause.
	 * Additional work will be done in optimizer/plan/planner.c.
	 */
	qry->havingQual = transformWhereClause(pstate, stmt->havingClause, NULL);

	qry->groupClause = transformGroupClause(pstate,
											stmt->groupClause,
											qry->targetList);

	qry->sortClause = transformSortClause(pstate,
										  stmt->sortClause,
										  qry->targetList,
										  qry->uniqueFlag);

	qry->hasSubLinks = pstate->p_hasSubLinks;
	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs || qry->groupClause)
		parseCheckAggregates(pstate, qry);

	/*
	 * If there is a havingQual but there are no aggregates, then there is
	 * something wrong with the query because HAVING must contain
	 * aggregates in its expressions! Otherwise the query could have been
	 * formulated using the WHERE clause.
	 */
	if (qry->havingQual && ! qry->hasAggs)
		elog(ERROR, "SELECT/HAVING requires aggregates to be valid");

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
	List	   *origTargetList;
	List	   *tl;

	qry->commandType = CMD_UPDATE;
	pstate->p_is_update = true;

	/*
	 * the FROM clause is non-standard SQL syntax. We used to be able to
	 * do this with REPLACE in POSTQUEL so we keep the feature.
	 */
	makeRangeTable(pstate, stmt->fromClause, NULL);
	setTargetTable(pstate, stmt->relname);

	qry->targetList = transformTargetList(pstate, stmt->targetList);

	qry->qual = transformWhereClause(pstate, stmt->whereClause, NULL);

	qry->hasSubLinks = pstate->p_hasSubLinks;

	qry->rtable = pstate->p_rtable;
	qry->resultRelation = refnameRangeTablePosn(pstate, stmt->relname, NULL);

	qry->hasAggs = pstate->p_hasAggs;
	if (pstate->p_hasAggs)
		parseCheckAggregates(pstate, qry);

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
			/* Resjunk nodes need no additional processing, but be sure they
			 * have names and resnos that do not match any target columns;
			 * else rewriter or planner might get confused.
			 */
			resnode->resname = "?resjunk?";
			resnode->resno = (AttrNumber) pstate->p_last_resno++;
			continue;
		}
		if (origTargetList == NIL)
			elog(ERROR, "UPDATE target count mismatch --- internal error");
		origTarget = (ResTarget *) lfirst(origTargetList);
		updateTargetListEntry(pstate, tle,
							  origTarget->name, origTarget->indirection);
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
							expr->args = makeList(lexpr, rexpr, -1);
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
							expr->args = makeList(lexpr, rexpr, -1);
							result = (Node *) expr;
							break;
						}
					case NOT:
						{
							Expr	   *expr = makeNode(Expr);
							Node	   *rexpr = A_Expr_to_Expr(((A_Expr *) ptr)->rexpr, intersect_present);

							expr->typeOid = BOOLOID;
							expr->opType = NOT_EXPR;
							expr->args = makeList(rexpr, -1);
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
	if (qry->unionClause != NULL)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with UNION/INTERSECT/EXCEPT clause");
	if (qry->uniqueFlag != NULL)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with DISTINCT clause");
	if (qry->groupClause != NULL)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with GROUP BY clause");
	if (qry->hasAggs)
		elog(ERROR, "SELECT FOR UPDATE is not allowed with AGGREGATE");
}

static void
transformForUpdate(Query *qry, List *forUpdate)
{
	List	   *rowMark = NULL;
	RowMark    *newrm;
	List	   *l;
	Index		i;

	CheckSelectForUpdate(qry);

	if (lfirst(forUpdate) == NULL)		/* all tables */
	{
		i = 1;
		foreach(l, qry->rtable)
		{
			newrm = makeNode(RowMark);
			newrm->rti = i++;
			newrm->info = ROW_MARK_FOR_UPDATE | ROW_ACL_FOR_UPDATE;
			rowMark = lappend(rowMark, newrm);
		}
		qry->rowMark = nconc(qry->rowMark, rowMark);
		return;
	}

	foreach(l, forUpdate)
	{
		List	   *l2;
		List	   *l3;

		i = 1;
		foreach(l2, qry->rtable)
		{
			if (strcmp(((RangeTblEntry *) lfirst(l2))->refname, lfirst(l)) == 0)
			{
				foreach(l3, rowMark)
				{
					if (((RowMark *) lfirst(l3))->rti == i)		/* duplicate */
						break;
				}
				if (l3 == NULL)
				{
					newrm = makeNode(RowMark);
					newrm->rti = i;
					newrm->info = ROW_MARK_FOR_UPDATE | ROW_ACL_FOR_UPDATE;
					rowMark = lappend(rowMark, newrm);
				}
				break;
			}
			i++;
		}
		if (l2 == NULL)
			elog(ERROR, "FOR UPDATE: relation %s not found in FROM clause", lfirst(l));
	}

	qry->rowMark = rowMark;
	return;
}
