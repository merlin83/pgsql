/*-------------------------------------------------------------------------
 *
 * sinvaladt.h
 *	  POSTGRES shared cache invalidation segment definitions.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: sinvaladt.h,v 1.15 1999-07-15 23:04:15 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef SINVALADT_H
#define SINVALADT_H

#include "storage/itemptr.h"
#include "storage/ipc.h"

/*
 * The structure of the shared cache invaidation segment
 *
 */
/*
A------------- Header info --------------
	criticalSectionSemaphoreId
	generalSemaphoreId
	startEntrySection	(offset a)
	endEntrySection		(offset a + b)
	startFreeSpace		(offset relative to B)
	startEntryChain		(offset relatiev to B)
	endEntryChain		(offset relative to B)
	numEntries
	maxNumEntries
	maxBackends
	procState[maxBackends] --> limit
								resetState (bool)
a								tag (POSTID)
B------------- Start entry section -------
	SISegEntry	--> entryData --> ... (see	SharedInvalidData!)
					isfree	(bool)
					next  (offset to next entry in chain )
b	  .... (dynamically growing down)
C----------------End shared segment -------

*/

/* Parameters (configurable)  *******************************************/
#define MAXNUMMESSAGES 4000		/* maximum number of messages in seg */


#define InvalidOffset	1000000000		/* a invalid offset  (End of
										 * chain) */

typedef struct ProcState
{
	int			limit;			/* the number of read messages			*/
	bool		resetState;		/* true, if backend has to reset its state */
	int			tag;			/* special tag, recieved from the
								 * postmaster */
} ProcState;


typedef struct SISeg
{
	IpcSemaphoreId criticalSectionSemaphoreId;	/* semaphore id		*/
	IpcSemaphoreId generalSemaphoreId;	/* semaphore id		*/
	Offset		startEntrySection;		/* (offset a)					*/
	Offset		endEntrySection;/* (offset a + b)				*/
	Offset		startFreeSpace; /* (offset relative to B)		*/
	Offset		startEntryChain;/* (offset relative to B)		*/
	Offset		endEntryChain;	/* (offset relative to B)		*/
	int			numEntries;
	int			maxNumEntries;
	int			maxBackends;	/* size of procState array */
	/*
	 * We declare procState as 1 entry because C wants a fixed-size array,
	 * but actually it is maxBackends entries long.
	 */
	ProcState	procState[1];	/* reflects the invalidation state */
	/*
	 * The entry section begins after the end of the procState array.
	 * Everything there is controlled by offsets.
	 */
} SISeg;

typedef struct SharedInvalidData
{
	int			cacheId;		/* XXX */
	Index		hashIndex;
	ItemPointerData pointerData;
} SharedInvalidData;

typedef SharedInvalidData *SharedInvalid;


typedef struct SISegEntry
{
	SharedInvalidData entryData;/* the message data */
	bool		isfree;			/* entry free? */
	Offset		next;			/* offset to next entry */
} SISegEntry;

typedef struct SISegOffsets
{
	Offset		startSegment;	/* always 0 (for now) */
	Offset		offsetToFirstEntry;		/* A + a = B */
	Offset		offsetToEndOfSegment;	/* A + a + b */
} SISegOffsets;


/****************************************************************************/
/* synchronization of the shared buffer access								*/
/*	  access to the buffer is synchronized by the lock manager !!			*/
/****************************************************************************/

#define SI_LockStartValue  255
#define SI_SharedLock	  (-1)
#define SI_ExclusiveLock  (-255)

extern SISeg *shmInvalBuffer;

/*
 * prototypes for functions in sinvaladt.c
 */
extern int	SIBackendInit(SISeg *segInOutP);
extern int	SISegmentInit(bool killExistingSegment, IPCKey key,
						  int maxBackends);

extern bool SISetDataEntry(SISeg *segP, SharedInvalidData *data);
extern void SISetProcStateInvalid(SISeg *segP);
extern bool SIDelDataEntry(SISeg *segP);
extern void SIReadEntryData(SISeg *segP, int backendId,
				void (*invalFunction) (), void (*resetFunction) ());
extern void SIDelExpiredDataEntries(SISeg *segP);

#endif	 /* SINVALADT_H */
