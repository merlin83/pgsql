/*-------------------------------------------------------------------------
 *
 * elog.h
 *	  POSTGRES error logging definitions.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: elog.h,v 1.37 2002-05-17 01:19:19 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef ELOG_H
#define ELOG_H

/* Error level codes */
#define DEBUG5		10		/* Debugging messages, in categories
							 * of decreasing detail. */
#define DEBUG4		11
#define DEBUG3		12				
#define DEBUG2		13				
#define DEBUG1		14				
#define LOG			15		/* Server operational messages;
							 * sent only to server log by default. */
#define COMMERROR 	16		/* Client communication problems; same as
							 * LOG for server reporting, but never sent
							 * to client. */
#define INFO		17		/* Informative messages that are always sent to
							 * client;  is not affected by
							 * client_min_messages */
#define NOTICE		18		/* Helpful messages to users about query
							 * operation;  sent to client
							 * and server log by default. */
#define WARNING		19		/* Warnings */
#define ERROR		20		/* user error - abort transaction; return to known
							 * state */
#define FATAL		21		/* fatal error - abort process */
#define PANIC		22		/* take down the other backends with me */

/*#define DEBUG	DEBUG1*/		/* Backward compatibility with pre-7.3 */

/* Configurable parameters */
#ifdef HAVE_SYSLOG
extern int	Use_syslog;
#endif
extern bool Log_timestamp;
extern bool Log_pid;

extern char	   *server_min_messages_str;
extern char	   *client_min_messages_str;
extern const char server_min_messages_str_default[];
extern const char client_min_messages_str_default[];

extern void
elog(int lev, const char *fmt,...)
/* This extension allows gcc to check the format string for consistency with
   the supplied arguments. */
__attribute__((format(printf, 2, 3)));

extern int	DebugFileOpen(void);

extern const char *assign_server_min_messages(const char *newval,
											  bool doit, bool interactive);
extern const char *assign_client_min_messages(const char *newval,
											  bool doit, bool interactive);

#endif   /* ELOG_H */
