/*-------------------------------------------------------------------------
 *
 * analyze.h
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: analyze.h,v 1.21 2003-04-29 22:13:11 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef ANALYZE_H
#define ANALYZE_H

#include "parser/parse_node.h"


extern List *parse_analyze(Node *parseTree, Oid *paramTypes, int numParams);
extern List *parse_analyze_varparams(Node *parseTree, Oid **paramTypes,
									 int *numParams);
extern List *parse_sub_analyze(Node *parseTree, ParseState *parentParseState);
extern List *analyzeCreateSchemaStmt(CreateSchemaStmt *stmt);

extern void CheckSelectForUpdate(Query *qry);

/* This was exported to allow ADD CONSTRAINT to make use of it */
extern char *makeObjectName(char *name1, char *name2, char *typename);

#endif   /* ANALYZE_H */
