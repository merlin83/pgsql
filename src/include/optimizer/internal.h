/*-------------------------------------------------------------------------
 *
 * internal.h
 *	  Definitions required throughout the query optimizer.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: internal.h,v 1.24 2000-01-11 03:59:31 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef INTERNAL_H
#define INTERNAL_H

#include "catalog/pg_index.h"

/*
 *		---------- SHARED MACROS
 *
 *		Macros common to modules for creating, accessing, and modifying
 *		query tree and query plan components.
 *		Shared with the executor.
 *
 */


/*
 *		System-dependent tuning constants
 *
 */
#define _CPU_PAGE_WEIGHT_  0.033  /* CPU-heap-to-page cost weighting factor */
#define _CPU_INDEX_PAGE_WEIGHT_ 0.017	/* CPU-index-to-page cost
										 * weighting factor */

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

/* GEQO switch according to number of relations in a query */
#define GEQO_RELS 11

#endif	 /* INTERNAL_H */
