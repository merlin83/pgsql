/*-------------------------------------------------------------------------
 *
 * internal.c--
 *	  Definitions required throughout the query optimizer.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/Attic/internal.c,v 1.7 1998-02-26 04:33:17 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

/*
 *		---------- SHARED MACROS
 *
 *		Macros common to modules for creating, accessing, and modifying
 *		query tree and query plan components.
 *		Shared with the executor.
 *
 */
#include <sys/types.h>

#include "postgres.h"

#include "optimizer/internal.h"

#include "nodes/relation.h"
#include "nodes/plannodes.h"
#include "nodes/primnodes.h"
#include "utils/palloc.h"

#ifdef NOT_USED
/*****************************************************************************
 *
 *****************************************************************************/

/* the following should probably be moved elsewhere -ay */

TargetEntry *
MakeTLE(Resdom *resdom, Node *expr)
{
	TargetEntry *rt = makeNode(TargetEntry);

	rt->resdom = resdom;
	rt->expr = expr;
	return rt;
}

Var *
get_expr(TargetEntry *tle)
{
	Assert(tle != NULL);
	Assert(tle->expr != NULL);

	return ((Var *) tle->expr);
}

#endif							/* 0 */
