/*-------------------------------------------------------------------------
 *
 * elog.h
 *	  POSTGRES error logging definitions.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: elog.h,v 1.20 2000-12-06 17:25:45 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef ELOG_H
#define ELOG_H

#define NOTICE	0				/* random info - no special action */
#define ERROR	(-1)			/* user error - return to known state */
#define FATAL	1				/* fatal error - abort process */
#define REALLYFATAL 2			/* take down the other backends with me */
#define STOP	REALLYFATAL
#define DEBUG	(-2)			/* debug message */
#define LOG		DEBUG
#define NOIND	(-3)			/* debug message, don't indent as far */

#ifdef ENABLE_SYSLOG
extern int Use_syslog;
#endif

/*
 * If StopIfError > 0 signal handlers mustn't do
 * elog(ERROR|FATAL), instead remember what action is
 * required with QueryCancel & ExitAfterAbort.
 */
extern uint32 StopIfError;		/* duplicates access/xlog.h */
extern bool QueryCancel;		/* duplicates miscadmin.h */
extern bool	ExitAfterAbort;

#define	START_CRIT_CODE		(StopIfError++)

#define END_CRIT_CODE	\
	do { \
		if (!StopIfError) \
			elog(STOP, "Not in critical section"); \
		StopIfError--; \
		if (!StopIfError && QueryCancel) \
		{ \
			if (ExitAfterAbort) \
				elog(FATAL, "The system is shutting down"); \
			else \
				elog(ERROR, "Query was cancelled."); \
		} \
	} while(0)

extern bool Log_timestamp;
extern bool Log_pid;

#ifndef __GNUC__
extern void elog(int lev, const char *fmt,...);

#else
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
extern void elog(int lev, const char *fmt,...) __attribute__((format(printf, 2, 3)));

#endif

#ifndef PG_STANDALONE
extern int	DebugFileOpen(void);

#endif

#endif	 /* ELOG_H */
