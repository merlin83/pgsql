/*-------------------------------------------------------------------------
 *
 * internal.c--
 *    Definitions required throughout the query optimizer.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/Attic/internal.c,v 1.1.1.1 1996-07-09 06:21:38 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */

/*    
 *    	---------- SHARED MACROS
 *    
 *     	Macros common to modules for creating, accessing, and modifying
 *    	query tree and query plan components.
 *    	Shared with the executor.
 *    
 */


#include "optimizer/internal.h"

#include "nodes/relation.h"
#include "nodes/plannodes.h"
#include "nodes/primnodes.h"
#include "utils/elog.h"
#include "utils/palloc.h"

#if 0 
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
    Assert(tle!=NULL);
    Assert(tle->expr!=NULL);

    return ((Var *)tle->expr); 
}

#endif /* 0 */



