/*-------------------------------------------------------------------------
 *
 * hio.h--
 *    POSTGRES heap access method input/output definitions.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: hio.h,v 1.1 1996-07-09 06:21:08 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef	HIO_H
#define HIO_H

#include "c.h"

#include "storage/block.h"
#include "access/htup.h"
#include "utils/rel.h"

extern void RelationPutHeapTuple(Relation relation, BlockNumber blockIndex,
				 HeapTuple tuple);
extern void RelationPutHeapTupleAtEnd(Relation relation, HeapTuple tuple);

#endif	/* HIO_H */
