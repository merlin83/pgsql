/*-------------------------------------------------------------------------
 *
 * plancat.h
 *	  prototypes for plancat.c.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/optimizer/plancat.h,v 1.35 2004/12/31 22:03:36 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLANCAT_H
#define PLANCAT_H

#include "nodes/relation.h"


extern void get_relation_info(Oid relationObjectId, RelOptInfo *rel);

extern List *build_physical_tlist(Query *root, RelOptInfo *rel);

extern List *find_inheritance_children(Oid inhparent);

extern bool has_subclass(Oid relationId);

extern bool has_unique_index(RelOptInfo *rel, AttrNumber attno);

extern Selectivity restriction_selectivity(Query *root,
						Oid operator,
						List *args,
						int varRelid);

extern Selectivity join_selectivity(Query *root,
				 Oid operator,
				 List *args,
				 JoinType jointype);

#endif   /* PLANCAT_H */
