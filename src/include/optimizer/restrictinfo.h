/*-------------------------------------------------------------------------
 *
 * restrictinfo.h
 *	  prototypes for restrictinfo.c.
 *
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/optimizer/restrictinfo.h,v 1.20 2003/11/29 22:41:07 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef RESTRICTINFO_H
#define RESTRICTINFO_H

#include "nodes/relation.h"

extern bool restriction_is_or_clause(RestrictInfo *restrictinfo);
extern List *get_actual_clauses(List *restrictinfo_list);
extern void get_actual_join_clauses(List *restrictinfo_list,
						List **joinquals, List **otherquals);
extern List *remove_redundant_join_clauses(Query *root,
							  List *restrictinfo_list,
							  JoinType jointype);
extern List *select_nonredundant_join_clauses(Query *root,
								 List *restrictinfo_list,
								 List *reference_list,
								 JoinType jointype);

#endif   /* RESTRICTINFO_H */
