/*-------------------------------------------------------------------------
 *
 * catalog_utils.h--
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parse_func.h,v 1.7 1998-02-05 04:08:44 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSER_FUNC_H
#define PARSER_FUNC_H

#include <nodes/nodes.h>
#include <nodes/pg_list.h>
#include <nodes/parsenodes.h>
#include <nodes/primnodes.h>
#include <parser/parse_func.h>
#include <parser/parse_node.h>

/*
 *	This structure is used to explore the inheritance hierarchy above
 *	nodes in the type tree in order to disambiguate among polymorphic
 *	functions.
 */
typedef struct _InhPaths
{
	int			nsupers;		/* number of superclasses */
	Oid			self;			/* this class */
	Oid		   *supervec;		/* vector of superclasses */
} InhPaths;

/*
 *	This structure holds a list of possible functions or operators that
 *	agree with the known name and argument types of the function/operator.
 */
typedef struct _CandidateList
{
	Oid		   *args;
	struct _CandidateList *next;
}		   *CandidateList;

extern Node *ParseNestedFuncOrColumn(ParseState *pstate, Attr *attr,
		int *curr_resno, int precedence);
extern Node *ParseFuncOrColumn(ParseState *pstate, char *funcname, List *fargs,
	int *curr_resno, int precedence);

extern void func_error(char *caller, char *funcname, int nargs, Oid *argtypes);

#endif							/* PARSE_FUNC_H */

