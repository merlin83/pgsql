/*-------------------------------------------------------------------------
 *
 * async.h
 *	  Asynchronous notification: NOTIFY, LISTEN, UNLISTEN
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: async.h,v 1.13 2000-01-26 05:58:00 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef ASYNC_H
#define ASYNC_H

/* notify-related SQL statements */
extern void Async_Notify(char *relname);
extern void Async_Listen(char *relname, int pid);
extern void Async_Unlisten(char *relname, int pid);

/* perform (or cancel) outbound notify processing at transaction commit */
extern void AtCommit_Notify(void);
extern void AtAbort_Notify(void);

/* signal handler for inbound notifies (SIGUSR2) */
extern void Async_NotifyHandler(SIGNAL_ARGS);

/*
 * enable/disable processing of inbound notifies directly from signal handler.
 * The enable routine first performs processing of any inbound notifies that
 * have occurred since the last disable.  These are meant to be called ONLY
 * from the appropriate places in PostgresMain().
 */
extern void EnableNotifyInterrupt(void);
extern void DisableNotifyInterrupt(void);

#endif	 /* ASYNC_H */
