/*-------------------------------------------------------------------------
 *
 * print.h
 *	  definitions for nodes/print.c
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: print.h,v 1.19 2002-09-04 20:31:44 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PRINT_H
#define PRINT_H

#include "nodes/parsenodes.h"
#include "nodes/plannodes.h"


#define nodeDisplay		pprint

extern void print(void *obj);
extern void pprint(void *obj);
extern void elog_node_display(int lev, const char *title,
				  void *obj, bool pretty);
extern char *format_node_dump(const char *dump);
extern char *pretty_format_node_dump(const char *dump);
extern void print_rt(List *rtable);
extern void print_expr(Node *expr, List *rtable);
extern void print_pathkeys(List *pathkeys, List *rtable);
extern void print_tl(List *tlist, List *rtable);
extern void print_slot(TupleTableSlot *slot);
extern void print_plan_recursive(Plan *p, Query *parsetree,
					 int indentLevel, char *label);
extern void print_plan(Plan *p, Query *parsetree);

#endif   /* PRINT_H */
