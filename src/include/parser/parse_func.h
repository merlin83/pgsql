/*-------------------------------------------------------------------------
 *
 * parse_func.h
 *
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parse_func.h,v 1.38 2002-04-09 20:35:55 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSER_FUNC_H
#define PARSER_FUNC_H

#include "parser/parse_node.h"

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
}	*CandidateList;

/* Result codes for func_get_detail */
typedef enum
{
	FUNCDETAIL_NOTFOUND,		/* no suitable interpretation */
	FUNCDETAIL_NORMAL,			/* found a matching function */
	FUNCDETAIL_COERCION			/* it's a type coercion request */
} FuncDetailCode;


extern Node *ParseFuncOrColumn(ParseState *pstate,
				  List *funcname, List *fargs,
				  bool agg_star, bool agg_distinct, bool is_column);

extern FuncDetailCode func_get_detail(List *funcname, List *fargs,
				int nargs, Oid *argtypes,
				Oid *funcid, Oid *rettype,
				bool *retset, Oid **true_typeids);

extern bool typeInheritsFrom(Oid subclassTypeId, Oid superclassTypeId);

extern void func_error(const char *caller, List *funcname,
					   int nargs, const Oid *argtypes,
					   const char *msg);

extern Oid	LookupFuncName(List *funcname, int nargs, const Oid *argtypes);
extern Oid	LookupFuncNameTypeNames(List *funcname, List *argtypes,
									bool opaqueOK, const char *caller);

#endif   /* PARSE_FUNC_H */
