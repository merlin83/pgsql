/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright 2000 by PostgreSQL Global Development Group
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/psql/common.h,v 1.26 2003-06-28 00:12:40 tgl Exp $
 */
#ifndef COMMON_H
#define COMMON_H

#include "postgres_fe.h"
#include <signal.h>
#include "pqsignal.h"
#include "libpq-fe.h"

extern char *xstrdup(const char *string);

extern bool setQFout(const char *fname);

extern void
psql_error(const char *fmt,...)
/* This lets gcc check the format string for consistency. */
__attribute__((format(printf, 1, 2)));

extern void NoticeProcessor(void *arg, const char *message);

extern char *simple_prompt(const char *prompt, int maxlen, bool echo);

extern volatile bool cancel_pressed;

extern void ResetCancelConn(void);

#ifndef WIN32
extern void handle_sigint(SIGNAL_ARGS);
#endif   /* not WIN32 */

extern PGresult *PSQLexec(const char *query, bool start_xact);

extern bool SendQuery(const char *query);

extern bool is_superuser(void);

/* sprompt.h */
extern char *simple_prompt(const char *prompt, int maxlen, bool echo);

/* Parse a numeric character code from the string pointed at by *buf, e.g.
 * one written as 0x0c (hexadecimal) or 015 (octal); advance *buf to the last
 * character of the numeric character code.
 */
extern char parse_char(char **buf);

/* Used for all Win32 popen/pclose calls */
#ifdef WIN32
#define popen(x,y) _popen(x,y)
#define pclose(x) _pclose(x)
#endif

#endif   /* COMMON_H */
