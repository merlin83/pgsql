/*-------------------------------------------------------------------------
 *
 * execdesc.h--
 *	  plan and query descriptor accessor macros used by the executor
 *	  and related modules.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: execdesc.h,v 1.7 1998-01-24 22:48:50 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef EXECDESC_H
#define EXECDESC_H

#include <tcop/dest.h>
#include <nodes/plannodes.h>
#include <nodes/parsenodes.h>

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
extern QueryDesc * CreateQueryDesc(Query *parsetree, Plan *plantree,
				CommandDest dest);

#endif							/* EXECDESC_H  */
