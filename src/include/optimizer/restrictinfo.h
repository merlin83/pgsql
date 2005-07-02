/*-------------------------------------------------------------------------
 *
 * restrictinfo.h
 *	  prototypes for restrictinfo.c.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/optimizer/restrictinfo.h,v 1.32 2005/07/02 23:00:42 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef RESTRICTINFO_H
#define RESTRICTINFO_H

#include "nodes/relation.h"


extern RestrictInfo *make_restrictinfo(Expr *clause,
									   bool is_pushed_down,
									   Relids required_relids);
extern List *make_restrictinfo_from_bitmapqual(Path *bitmapqual,
											   bool is_pushed_down);
extern bool restriction_is_or_clause(RestrictInfo *restrictinfo);
extern List *get_actual_clauses(List *restrictinfo_list);
extern void get_actual_join_clauses(List *restrictinfo_list,
						List **joinquals, List **otherquals);
extern List *remove_redundant_join_clauses(PlannerInfo *root,
							  List *restrictinfo_list,
							  bool isouterjoin);
extern List *select_nonredundant_join_clauses(PlannerInfo *root,
								 List *restrictinfo_list,
								 List *reference_list,
								 bool isouterjoin);

#endif   /* RESTRICTINFO_H */
