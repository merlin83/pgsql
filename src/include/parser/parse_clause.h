/*-------------------------------------------------------------------------
 *
 * parse_clause.h
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parse_clause.h,v 1.10 1999-07-15 15:21:25 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_CLAUSE_H
#define PARSE_CLAUSE_H

#include <parser/parse_node.h>

extern void makeRangeTable(ParseState *pstate, char *relname, List *frmList, Node **qual);
extern Node *transformWhereClause(ParseState *pstate, Node *where, Node *using);
extern List *transformGroupClause(ParseState *pstate, List *grouplist,
					 List *targetlist);
extern List *transformSortClause(ParseState *pstate,
					List *orderlist, List *sortClause,
					List *targetlist, char *uniqueFlag);
#endif	 /* PARSE_CLAUSE_H */
