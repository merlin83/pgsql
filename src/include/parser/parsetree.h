/*-------------------------------------------------------------------------
 *
 * parsetree.h
 *	  Routines to access various components and subcomponents of
 *	  parse trees.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parsetree.h,v 1.10 2000-06-12 19:40:51 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSETREE_H
#define PARSETREE_H				/* include once only */

#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"

/* ----------------
 *		need pg_list.h for definitions of CAR(), etc. macros
 * ----------------
 */

/* ----------------
 *		range table macros
 *
 *	parse tree:
 *		(root targetlist qual)
 *		 ^^^^
 *	parse root:
 *		(numlevels cmdtype resrel rangetable priority ruleinfo nestdotinfo)
 *								  ^^^^^^^^^^
 *	range table:
 *		(rtentry ...)
 *	rtentry:
 * ----------------
 */

#define rt_relname(rt_entry) \
	  ((!strcmp(((rt_entry)->ref->relname),"*OLD*") ||\
		!strcmp(((rt_entry)->ref->relname),"*NEW*")) ? ((rt_entry)->ref->relname) : \
		((char *)(rt_entry)->relname))

/*
 *		rt_fetch
 *		rt_store
 *
 *		Access and (destructively) replace rangetable entries.
 *
 */
#define rt_fetch(rangetable_index, rangetable) \
	((RangeTblEntry*)nth((rangetable_index)-1, rangetable))

#define rt_store(rangetable_index, rangetable, rt) \
	set_nth(rangetable, (rangetable_index)-1, rt)

/*
 *		getrelid
 *		getrelname
 *
 *		Given the range index of a relation, return the corresponding
 *		relation id or relation name.
 */
#define getrelid(rangeindex,rangetable) \
	((RangeTblEntry*)nth((rangeindex)-1, rangetable))->relid

#define getrelname(rangeindex, rangetable) \
	rt_relname((RangeTblEntry*)nth((rangeindex)-1, rangetable))

#endif	 /* PARSETREE_H */
