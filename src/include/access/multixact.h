/*
 * multixact.h
 *
 * PostgreSQL multi-transaction-log manager
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/access/multixact.h,v 1.3 2005/06/08 15:50:28 tgl Exp $
 */
#ifndef MULTIXACT_H
#define MULTIXACT_H

#include "access/xlog.h"

#define InvalidMultiXactId	((MultiXactId) 0)
#define FirstMultiXactId	((MultiXactId) 1)

#define MultiXactIdIsValid(multi) ((multi) != InvalidMultiXactId)

/* ----------------
 *		multixact-related XLOG entries
 * ----------------
 */

#define XLOG_MULTIXACT_ZERO_OFF_PAGE	0x00
#define XLOG_MULTIXACT_ZERO_MEM_PAGE	0x10
#define XLOG_MULTIXACT_CREATE_ID		0x20

typedef struct xl_multixact_create
{
	MultiXactId		mid;		/* new MultiXact's ID */
	MultiXactOffset	moff;		/* its starting offset in members file */
	int32			nxids;		/* number of member XIDs */
	TransactionId	xids[1];	/* VARIABLE LENGTH ARRAY */
} xl_multixact_create;

#define MinSizeOfMultiXactCreate offsetof(xl_multixact_create, xids)


extern MultiXactId MultiXactIdCreate(TransactionId xid1, TransactionId xid2);
extern MultiXactId MultiXactIdExpand(MultiXactId multi, TransactionId xid);
extern bool MultiXactIdIsRunning(MultiXactId multi);
extern void MultiXactIdWait(MultiXactId multi);
extern void MultiXactIdSetOldestMember(void);

extern void AtEOXact_MultiXact(void);

extern int	MultiXactShmemSize(void);
extern void MultiXactShmemInit(void);
extern void BootStrapMultiXact(void);
extern void StartupMultiXact(void);
extern void ShutdownMultiXact(void);
extern void MultiXactGetCheckptMulti(bool is_shutdown,
									 MultiXactId *nextMulti,
									 MultiXactOffset *nextMultiOffset);
extern void CheckPointMultiXact(void);
extern void MultiXactSetNextMXact(MultiXactId nextMulti,
								  MultiXactOffset nextMultiOffset);
extern void MultiXactAdvanceNextMXact(MultiXactId minMulti,
									  MultiXactOffset minMultiOffset);

extern void multixact_redo(XLogRecPtr lsn, XLogRecord *record);
extern void multixact_desc(char *buf, uint8 xl_info, char *rec);

#endif   /* MULTIXACT_H */
