/*-------------------------------------------------------------------------
 *
 * freespace.h
 *	  POSTGRES free space map for quickly finding free space in relations
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: freespace.h,v 1.1 2001-06-27 23:31:39 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef FREESPACE_H_
#define FREESPACE_H_

#include "storage/block.h"
#include "storage/relfilenode.h"
#include "storage/spin.h"


extern SPINLOCK FreeSpaceLock;

extern int	MaxFSMRelations;
extern int	MaxFSMPages;


/*
 * function prototypes
 */
extern void InitFreeSpaceMap(void);
extern int	FreeSpaceShmemSize(void);

extern BlockNumber GetPageWithFreeSpace(RelFileNode *rel, Size spaceNeeded);
extern void RecordFreeSpace(RelFileNode *rel, BlockNumber page,
							Size spaceAvail);
extern BlockNumber RecordAndGetPageWithFreeSpace(RelFileNode *rel,
												 BlockNumber oldPage,
												 Size oldSpaceAvail,
												 Size spaceNeeded);
extern void MultiRecordFreeSpace(RelFileNode *rel,
								 BlockNumber minPage,
								 BlockNumber maxPage,
								 int nPages,
								 BlockNumber *pages,
								 Size *spaceAvail);
extern void FreeSpaceMapForgetRel(RelFileNode *rel);

#ifdef FREESPACE_DEBUG
extern void DumpFreeSpace(void);
#endif

#endif	 /* FREESPACE_H */
