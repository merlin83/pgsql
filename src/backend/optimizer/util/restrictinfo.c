/*-------------------------------------------------------------------------
 *
 * restrictinfo.c
 *	  RestrictInfo node manipulation routines.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/restrictinfo.c,v 1.8 1999-07-25 23:07:26 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"


#include "nodes/nodeFuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/internal.h"
#include "optimizer/restrictinfo.h"

/*
 * restriction_is_or_clause
 *
 * Returns t iff the restrictinfo node contains an 'or' clause.
 *
 */
bool
restriction_is_or_clause(RestrictInfo *restrictinfo)
{
	if (restrictinfo != NULL &&
		or_clause((Node *) restrictinfo->clause))
		return true;
	else
		return false;
}

/*
 * get_actual_clauses
 *
 * Returns a list containing the clauses from 'restrictinfo_list'.
 *
 */
List *
get_actual_clauses(List *restrictinfo_list)
{
	List	   *result = NIL;
	List	   *temp;

	foreach(temp, restrictinfo_list)
	{
		RestrictInfo *clause = (RestrictInfo *) lfirst(temp);

		result = lappend(result, clause->clause);
	}
	return result;
}
