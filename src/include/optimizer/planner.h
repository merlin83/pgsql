/*-------------------------------------------------------------------------
 *
 * planner.h
 *	  prototypes for planner.c.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: planner.h,v 1.15 2000-03-21 05:11:51 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLANNER_H
#define PLANNER_H

/*
*/

#include "nodes/parsenodes.h"
#include "nodes/plannodes.h"

extern Plan *planner(Query *parse);
extern Plan *subquery_planner(Query *parse, double tuple_fraction);
extern Plan *union_planner(Query *parse, double tuple_fraction);
extern void pg_checkretval(Oid rettype, List *querytree_list);

#endif	 /* PLANNER_H */
