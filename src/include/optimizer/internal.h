/*-------------------------------------------------------------------------
 *
 * internal.h
 *	  Definitions required throughout the query optimizer.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: internal.h,v 1.27 2000-06-15 03:32:51 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef INTERNAL_H
#define INTERNAL_H

/*
 *		---------- SHARED MACROS
 *
 *		Macros common to modules for creating, accessing, and modifying
 *		query tree and query plan components.
 *		Shared with the executor.
 *
 */


/*
 *		Size estimates
 *
 */

/*	   The cost of sequentially scanning a materialized temporary relation
 */
#define _NONAME_SCAN_COST_		10

/*	   The number of pages and tuples in a materialized relation
 */
#define _NONAME_RELATION_PAGES_			1
#define _NONAME_RELATION_TUPLES_	10

/*	   The length of a variable-length field in bytes (stupid estimate...)
 */
#define _DEFAULT_ATTRIBUTE_WIDTH_ 12

/*
 *		Flags and identifiers
 *
 */

/*	   Identifier for (sort) temp relations   */
/* used to be -1 */
#define _NONAME_RELATION_ID_	 InvalidOid

#endif	 /* INTERNAL_H */
