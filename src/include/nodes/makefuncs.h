/*-------------------------------------------------------------------------
 *
 * makefuncs.h
 *	  prototypes for the creator functions (for primitive nodes)
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: makefuncs.h,v 1.43 2002-12-12 15:49:40 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef MAKEFUNC_H
#define MAKEFUNC_H

#include "nodes/parsenodes.h"


extern A_Expr *makeA_Expr(int oper, List *name, Node *lexpr, Node *rexpr);

extern A_Expr *makeSimpleA_Expr(int oper, const char *name,
				 Node *lexpr, Node *rexpr);

extern Var *makeVar(Index varno,
		AttrNumber varattno,
		Oid vartype,
		int32 vartypmod,
		Index varlevelsup);

extern TargetEntry *makeTargetEntry(Resdom *resdom, Expr *expr);

extern Resdom *makeResdom(AttrNumber resno,
		   Oid restype,
		   int32 restypmod,
		   char *resname,
		   bool resjunk);

extern Const *makeConst(Oid consttype,
		  int constlen,
		  Datum constvalue,
		  bool constisnull,
		  bool constbyval);

extern Const *makeNullConst(Oid consttype);

extern Expr *makeBoolExpr(BoolExprType boolop, List *args);

extern Alias *makeAlias(const char *aliasname, List *colnames);

extern RelabelType *makeRelabelType(Expr *arg, Oid rtype, int32 rtypmod,
									CoercionForm rformat);

extern RangeVar *makeRangeVar(char *schemaname, char *relname);

extern TypeName *makeTypeName(char *typnam);

#endif   /* MAKEFUNC_H */
