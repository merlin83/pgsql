/*-------------------------------------------------------------------------
 *
 * ipc.h
 *	  System V IPC Emulation
 *
 * Copyright (c) 1999, repas AEG Automation GmbH
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/port/qnx4/Attic/ipc.h,v 1.2 2000-04-12 17:15:30 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#ifndef _SYS_IPC_H
#define _SYS_IPC_H

/* Common IPC definitions. */
/* Mode bits. */
#define IPC_CREAT	0001000		/* create entry if key doesn't exist */
#define IPC_EXCL	0002000		/* fail if key exists */
#define IPC_NOWAIT	0004000		/* error if request must wait */

/* Keys. */
#define IPC_PRIVATE (key_t)0	/* private key */

/* Control Commands. */
#define IPC_RMID	0			/* remove identifier */

#endif	 /* _SYS_IPC_H */
