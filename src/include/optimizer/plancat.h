/*-------------------------------------------------------------------------
 *
 * plancat.h
 *	  prototypes for plancat.c.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: plancat.h,v 1.16 2000-01-22 23:50:27 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLANCAT_H
#define PLANCAT_H

#include "nodes/relation.h"


extern void relation_info(Query *root, Index relid,
						  bool *hasindex, long *pages, double *tuples);

extern List *find_secondary_indexes(Query *root, Index relid);

extern List *find_inheritance_children(Oid inhparent);

extern Selectivity restriction_selectivity(Oid functionObjectId,
						Oid operatorObjectId,
						Oid relationObjectId,
						AttrNumber attributeNumber,
						Datum constValue,
						int constFlag);

extern Selectivity join_selectivity(Oid functionObjectId, Oid operatorObjectId,
				 Oid relationObjectId1, AttrNumber attributeNumber1,
				 Oid relationObjectId2, AttrNumber attributeNumber2);

#endif	 /* PLANCAT_H */
