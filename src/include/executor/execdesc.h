/*-------------------------------------------------------------------------
 *
 * execdesc.h
 *	  plan and query descriptor accessor macros used by the executor
 *	  and related modules.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: execdesc.h,v 1.12 1999-07-16 17:07:32 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef EXECDESC_H
#define EXECDESC_H

#include "nodes/parsenodes.h"
#include "nodes/plannodes.h"
#include "tcop/dest.h"

/* ----------------
 *		query descriptor:
 *	a QueryDesc encapsulates everything that the executor
 *	needs to execute the query
 * ---------------------
 */
typedef struct QueryDesc
{
	CmdType		operation;		/* CMD_SELECT, CMD_UPDATE, etc. */
	Query	   *parsetree;
	Plan	   *plantree;
	CommandDest dest;			/* the destination output of the execution */
} QueryDesc;

/* in pquery.c */
extern QueryDesc *CreateQueryDesc(Query *parsetree, Plan *plantree,
				CommandDest dest);

#endif	 /* EXECDESC_H	*/
