/*-------------------------------------------------------------------------
 *
 * execnodes.h
 *	  definitions for executor state nodes
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: print.h,v 1.11 1999-07-15 15:21:18 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PRINT_H
#define PRINT_H

#include "nodes/parsenodes.h"
#include "nodes/plannodes.h"

/*
 * nodes/{outfuncs.c,print.c}
 */
#define nodeDisplay		pprint

extern void print(void *obj);
extern void pprint(void *obj);
extern void print_rt(List *rtable);
extern void print_expr(Node *expr, List *rtable);
extern void print_pathkeys(List *pathkeys, List *rtable);
extern void print_tl(List *tlist, List *rtable);
extern void print_slot(TupleTableSlot *slot);
extern void print_plan_recursive(Plan *p, Query *parsetree,
					 int indentLevel, char *label);
extern void print_plan(Plan *p, Query *parsetree);

#endif	 /* PRINT_H */
