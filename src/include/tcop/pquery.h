/*-------------------------------------------------------------------------
 *
 * pquery.h
 *	  prototypes for pquery.c.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pquery.h,v 1.12.2.1 1999-07-30 17:07:20 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQUERY_H
#define PQUERY_H

#include "executor/execdesc.h"

/* moved to execdesc.h
extern QueryDesc *CreateQueryDesc(Query *parsetree, Plan *plantree,
								  CommandDest dest);

*/
extern EState *CreateExecutorState(void);


extern void ProcessPortal(char *portalName, Query *parseTree,
			  Plan *plan, EState *state, TupleDesc attinfo,
			  CommandDest dest);

extern void
			ProcessQuery(Query *parsetree, Plan *plan, CommandDest dest);

#endif	 /* pqueryIncluded */
