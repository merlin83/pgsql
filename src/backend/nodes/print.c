/*-------------------------------------------------------------------------
 *
 * print.c
 *	  various print routines (used mostly for debugging)
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/nodes/print.c,v 1.69 2004/06/06 00:41:26 tgl Exp $
 *
 * HISTORY
 *	  AUTHOR			DATE			MAJOR EVENT
 *	  Andrew Yu			Oct 26, 1994	file creation
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/printtup.h"
#include "lib/stringinfo.h"
#include "nodes/print.h"
#include "optimizer/clauses.h"
#include "parser/parsetree.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

static char *plannode_type(Plan *p);

/*
 * print
 *	  print contents of Node to stdout
 */
void
print(void *obj)
{
	char	   *s;
	char	   *f;

	s = nodeToString(obj);
	f = format_node_dump(s);
	pfree(s);
	printf("%s\n", f);
	fflush(stdout);
	pfree(f);
}

/*
 * pprint
 *	  pretty-print contents of Node to stdout
 */
void
pprint(void *obj)
{
	char	   *s;
	char	   *f;

	s = nodeToString(obj);
	f = pretty_format_node_dump(s);
	pfree(s);
	printf("%s\n", f);
	fflush(stdout);
	pfree(f);
}

/*
 * elog_node_display
 *	  send pretty-printed contents of Node to postmaster log
 */
void
elog_node_display(int lev, const char *title, void *obj, bool pretty)
{
	char	   *s;
	char	   *f;

	s = nodeToString(obj);
	if (pretty)
		f = pretty_format_node_dump(s);
	else
		f = format_node_dump(s);
	pfree(s);
	ereport(lev,
			(errmsg_internal("%s:", title),
			 errdetail("%s", f)));
	pfree(f);
}

/*
 * Format a nodeToString output for display on a terminal.
 *
 * The result is a palloc'd string.
 *
 * This version just tries to break at whitespace.
 */
char *
format_node_dump(const char *dump)
{
#define LINELEN		78
	char		line[LINELEN + 1];
	StringInfoData str;
	int			i;
	int			j;
	int			k;

	initStringInfo(&str);
	i = 0;
	for (;;)
	{
		for (j = 0; j < LINELEN && dump[i] != '\0'; i++, j++)
			line[j] = dump[i];
		if (dump[i] == '\0')
			break;
		if (dump[i] == ' ')
		{
			/* ok to break at adjacent space */
			i++;
		}
		else
		{
			for (k = j - 1; k > 0; k--)
				if (line[k] == ' ')
					break;
			if (k > 0)
			{
				/* back up; will reprint all after space */
				i -= (j - k - 1);
				j = k;
			}
		}
		line[j] = '\0';
		appendStringInfo(&str, "%s\n", line);
	}
	if (j > 0)
	{
		line[j] = '\0';
		appendStringInfo(&str, "%s\n", line);
	}
	return str.data;
#undef LINELEN
}

/*
 * Format a nodeToString output for display on a terminal.
 *
 * The result is a palloc'd string.
 *
 * This version tries to indent intelligently.
 */
char *
pretty_format_node_dump(const char *dump)
{
#define INDENTSTOP	3
#define MAXINDENT	60
#define LINELEN		78
	char		line[LINELEN + 1];
	StringInfoData str;
	int			indentLev;
	int			indentDist;
	int			i;
	int			j;

	initStringInfo(&str);
	indentLev = 0;				/* logical indent level */
	indentDist = 0;				/* physical indent distance */
	i = 0;
	for (;;)
	{
		for (j = 0; j < indentDist; j++)
			line[j] = ' ';
		for (; j < LINELEN && dump[i] != '\0'; i++, j++)
		{
			line[j] = dump[i];
			switch (line[j])
			{
				case '}':
					if (j != indentDist)
					{
						/* print data before the } */
						line[j] = '\0';
						appendStringInfo(&str, "%s\n", line);
					}
					/* print the } at indentDist */
					line[indentDist] = '}';
					line[indentDist + 1] = '\0';
					appendStringInfo(&str, "%s\n", line);
					/* outdent */
					if (indentLev > 0)
					{
						indentLev--;
						indentDist = Min(indentLev * INDENTSTOP, MAXINDENT);
					}
					j = indentDist - 1;
					/* j will equal indentDist on next loop iteration */
					/* suppress whitespace just after } */
					while (dump[i+1] == ' ')
						i++;
					break;
				case ')':
					/* force line break after ), unless another ) follows */
					if (dump[i+1] != ')')
					{
						line[j + 1] = '\0';
						appendStringInfo(&str, "%s\n", line);
						j = indentDist - 1;
						while (dump[i+1] == ' ')
							i++;
					}
					break;
				case '{':
					/* force line break before { */
					if (j != indentDist)
					{
						line[j] = '\0';
						appendStringInfo(&str, "%s\n", line);
					}
					/* indent */
					indentLev++;
					indentDist = Min(indentLev * INDENTSTOP, MAXINDENT);
					for (j = 0; j < indentDist; j++)
						line[j] = ' ';
					line[j] = dump[i];
					break;
				case ':':
					/* force line break before : */
					if (j != indentDist)
					{
						line[j] = '\0';
						appendStringInfo(&str, "%s\n", line);
					}
					j = indentDist;
					line[j] = dump[i];
					break;
			}
		}
		line[j] = '\0';
		if (dump[i] == '\0')
			break;
		appendStringInfo(&str, "%s\n", line);
	}
	if (j > 0)
		appendStringInfo(&str, "%s\n", line);
	return str.data;
#undef INDENTSTOP
#undef MAXINDENT
#undef LINELEN
}

/*
 * print_rt
 *	  print contents of range table
 */
void
print_rt(List *rtable)
{
	ListCell   *l;
	int			i = 1;

	printf("resno\trefname  \trelid\tinFromCl\n");
	printf("-----\t---------\t-----\t--------\n");
	foreach(l, rtable)
	{
		RangeTblEntry *rte = lfirst(l);

		switch (rte->rtekind)
		{
			case RTE_RELATION:
				printf("%d\t%s\t%u",
					   i, rte->eref->aliasname, rte->relid);
				break;
			case RTE_SUBQUERY:
				printf("%d\t%s\t[subquery]",
					   i, rte->eref->aliasname);
				break;
			case RTE_FUNCTION:
				printf("%d\t%s\t[rangefunction]",
					   i, rte->eref->aliasname);
				break;
			case RTE_JOIN:
				printf("%d\t%s\t[join]",
					   i, rte->eref->aliasname);
				break;
			case RTE_SPECIAL:
				printf("%d\t%s\t[special]",
					   i, rte->eref->aliasname);
				break;
			default:
				printf("%d\t%s\t[unknown rtekind]",
					   i, rte->eref->aliasname);
		}

		printf("\t%s\t%s\n",
			   (rte->inh ? "inh" : ""),
			   (rte->inFromCl ? "inFromCl" : ""));
		i++;
	}
}


/*
 * print_expr
 *	  print an expression
 */
void
print_expr(Node *expr, List *rtable)
{
	if (expr == NULL)
	{
		printf("<>");
		return;
	}

	if (IsA(expr, Var))
	{
		Var		   *var = (Var *) expr;
		char	   *relname,
				   *attname;

		switch (var->varno)
		{
			case INNER:
				relname = "INNER";
				attname = "?";
				break;
			case OUTER:
				relname = "OUTER";
				attname = "?";
				break;
			default:
				{
					RangeTblEntry *rte;

					Assert(var->varno > 0 &&
						   (int) var->varno <= list_length(rtable));
					rte = rt_fetch(var->varno, rtable);
					relname = rte->eref->aliasname;
					attname = get_rte_attribute_name(rte, var->varattno);
				}
				break;
		}
		printf("%s.%s", relname, attname);
	}
	else if (IsA(expr, Const))
	{
		Const	   *c = (Const *) expr;
		Oid			typoutput;
		Oid			typioparam;
		bool		typIsVarlena;
		char	   *outputstr;

		if (c->constisnull)
		{
			printf("NULL");
			return;
		}

		getTypeOutputInfo(c->consttype,
						  &typoutput, &typioparam, &typIsVarlena);

		outputstr = DatumGetCString(OidFunctionCall3(typoutput,
													 c->constvalue,
											   ObjectIdGetDatum(typioparam),
													 Int32GetDatum(-1)));
		printf("%s", outputstr);
		pfree(outputstr);
	}
	else if (IsA(expr, OpExpr))
	{
		OpExpr	   *e = (OpExpr *) expr;
		char	   *opname;

		opname = get_opname(e->opno);
		if (list_length(e->args) > 1)
		{
			print_expr(get_leftop((Expr *) e), rtable);
			printf(" %s ", ((opname != NULL) ? opname : "(invalid operator)"));
			print_expr(get_rightop((Expr *) e), rtable);
		}
		else
		{
			/* we print prefix and postfix ops the same... */
			printf("%s ", ((opname != NULL) ? opname : "(invalid operator)"));
			print_expr(get_leftop((Expr *) e), rtable);
		}
	}
	else if (IsA(expr, FuncExpr))
	{
		FuncExpr   *e = (FuncExpr *) expr;
		char	   *funcname;
		ListCell   *l;

		funcname = get_func_name(e->funcid);
		printf("%s(", ((funcname != NULL) ? funcname : "(invalid function)"));
		foreach(l, e->args)
		{
			print_expr(lfirst(l), rtable);
			if (lnext(l))
				printf(",");
		}
		printf(")");
	}
	else
		printf("unknown expr");
}

/*
 * print_pathkeys -
 *	  pathkeys list of list of PathKeyItems
 */
void
print_pathkeys(List *pathkeys, List *rtable)
{
	ListCell   *i;

	printf("(");
	foreach(i, pathkeys)
	{
		List	   *pathkey = (List *) lfirst(i);
		ListCell   *k;

		printf("(");
		foreach(k, pathkey)
		{
			PathKeyItem *item = (PathKeyItem *) lfirst(k);

			print_expr(item->key, rtable);
			if (lnext(k))
				printf(", ");
		}
		printf(")");
		if (lnext(i))
			printf(", ");
	}
	printf(")\n");
}

/*
 * print_tl
 *	  print targetlist in a more legible way.
 */
void
print_tl(List *tlist, List *rtable)
{
	ListCell   *tl;

	printf("(\n");
	foreach(tl, tlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(tl);

		printf("\t%d %s\t", tle->resdom->resno,
			   tle->resdom->resname ? tle->resdom->resname : "<null>");
		if (tle->resdom->ressortgroupref != 0)
			printf("(%u):\t", tle->resdom->ressortgroupref);
		else
			printf("    :\t");
		print_expr((Node *) tle->expr, rtable);
		printf("\n");
	}
	printf(")\n");
}

/*
 * print_slot
 *	  print out the tuple with the given TupleTableSlot
 */
void
print_slot(TupleTableSlot *slot)
{
	if (!slot->val)
	{
		printf("tuple is null.\n");
		return;
	}
	if (!slot->ttc_tupleDescriptor)
	{
		printf("no tuple descriptor.\n");
		return;
	}

	debugtup(slot->val, slot->ttc_tupleDescriptor, NULL);
}

static char *
plannode_type(Plan *p)
{
	switch (nodeTag(p))
	{
		case T_Plan:
			return "PLAN";
		case T_Result:
			return "RESULT";
		case T_Append:
			return "APPEND";
		case T_Scan:
			return "SCAN";
		case T_SeqScan:
			return "SEQSCAN";
		case T_IndexScan:
			return "INDEXSCAN";
		case T_TidScan:
			return "TIDSCAN";
		case T_SubqueryScan:
			return "SUBQUERYSCAN";
		case T_FunctionScan:
			return "FUNCTIONSCAN";
		case T_Join:
			return "JOIN";
		case T_NestLoop:
			return "NESTLOOP";
		case T_MergeJoin:
			return "MERGEJOIN";
		case T_HashJoin:
			return "HASHJOIN";
		case T_Material:
			return "MATERIAL";
		case T_Sort:
			return "SORT";
		case T_Agg:
			return "AGG";
		case T_Unique:
			return "UNIQUE";
		case T_SetOp:
			return "SETOP";
		case T_Limit:
			return "LIMIT";
		case T_Hash:
			return "HASH";
		case T_Group:
			return "GROUP";
		default:
			return "UNKNOWN";
	}
}

/*
 * Recursively prints a simple text description of the plan tree
 */
void
print_plan_recursive(Plan *p, Query *parsetree, int indentLevel, char *label)
{
	int			i;
	char		extraInfo[NAMEDATALEN + 100];

	if (!p)
		return;
	for (i = 0; i < indentLevel; i++)
		printf(" ");
	printf("%s%s :c=%.2f..%.2f :r=%.0f :w=%d ", label, plannode_type(p),
		   p->startup_cost, p->total_cost,
		   p->plan_rows, p->plan_width);
	if (IsA(p, Scan) ||
		IsA(p, SeqScan))
	{
		RangeTblEntry *rte;

		rte = rt_fetch(((Scan *) p)->scanrelid, parsetree->rtable);
		StrNCpy(extraInfo, rte->eref->aliasname, NAMEDATALEN);
	}
	else if (IsA(p, IndexScan))
	{
		RangeTblEntry *rte;

		rte = rt_fetch(((IndexScan *) p)->scan.scanrelid, parsetree->rtable);
		StrNCpy(extraInfo, rte->eref->aliasname, NAMEDATALEN);
	}
	else if (IsA(p, FunctionScan))
	{
		RangeTblEntry *rte;

		rte = rt_fetch(((FunctionScan *) p)->scan.scanrelid, parsetree->rtable);
		StrNCpy(extraInfo, rte->eref->aliasname, NAMEDATALEN);
	}
	else
		extraInfo[0] = '\0';
	if (extraInfo[0] != '\0')
		printf(" ( %s )\n", extraInfo);
	else
		printf("\n");
	print_plan_recursive(p->lefttree, parsetree, indentLevel + 3, "l: ");
	print_plan_recursive(p->righttree, parsetree, indentLevel + 3, "r: ");

	if (IsA(p, Append))
	{
		ListCell   *l;
		int			whichplan = 0;
		Append	   *appendplan = (Append *) p;

		foreach(l, appendplan->appendplans)
		{
			Plan	   *subnode = (Plan *) lfirst(l);

			/*
			 * I don't think we need to fiddle with the range table here,
			 * bjm
			 */
			print_plan_recursive(subnode, parsetree, indentLevel + 3, "a: ");

			whichplan++;
		}
	}
}

/* print_plan
  prints just the plan node types */

void
print_plan(Plan *p, Query *parsetree)
{
	print_plan_recursive(p, parsetree, 0, "");
}
