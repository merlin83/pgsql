/*-------------------------------------------------------------------------
 *
 * pg_shmem.h
 *	  Platform-independent API for shared memory support.
 *
 * Every port is expected to support shared memory with approximately
 * SysV-ish semantics; in particular, a memory block is not anonymous
 * but has an ID, and we must be able to tell whether there are any
 * remaining processes attached to a block of a specified ID.
 *
 * To simplify life for the SysV implementation, the ID is assumed to
 * consist of two unsigned long values (these are key and ID in SysV
 * terms).  Other platforms may ignore the second value if they need
 * only one ID number.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pg_shmem.h,v 1.1 2002-05-05 00:03:29 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_SHMEM_H
#define PG_SHMEM_H

#include <sys/types.h>


typedef struct PGShmemHeader	/* standard header for all Postgres shmem */
{
	int32		magic;			/* magic # to identify Postgres segments */
#define PGShmemMagic  679834892
	pid_t		creatorPID;		/* PID of creating process */
	uint32		totalsize;		/* total size of segment */
	uint32		freeoffset;		/* offset to first free space */
} PGShmemHeader;


extern PGShmemHeader *PGSharedMemoryCreate(uint32 size, bool makePrivate,
										   int port);
extern bool PGSharedMemoryIsInUse(unsigned long id1, unsigned long id2);

#endif   /* PG_SHMEM_H */
