/*-------------------------------------------------------------------------
 *
 * rtscan.h
 *	  routines defined in access/rtree/rtscan.c
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: rtscan.h,v 1.11 2001-10-25 05:49:55 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef RTSCAN_H
#define RTSCAN_H

#include "storage/block.h"
#include "storage/off.h"
#include "utils/rel.h"

void		rtadjscans(Relation r, int op, BlockNumber blkno, OffsetNumber offnum);
#endif	 /* RTSCAN_H */
