/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright 2000 by PostgreSQL Global Development Group
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/psql/common.h,v 1.14 2001-10-03 21:58:28 tgl Exp $
 */
#ifndef COMMON_H
#define COMMON_H

#include "postgres_fe.h"
#include <signal.h>
#include "pqsignal.h"
#include "libpq-fe.h"

extern char	   *xstrdup(const char *string);

extern bool		setQFout(const char *fname);

extern void		psql_error(const char *fmt, ...)
/* This lets gcc check the format string for consistency. */
__attribute__((format(printf, 1, 2)));

extern void		NoticeProcessor(void *arg, const char *message);

extern char	   *simple_prompt(const char *prompt, int maxlen, bool echo);

extern volatile bool cancel_pressed;
extern PGconn *cancelConn;

#ifndef WIN32
extern void		handle_sigint(SIGNAL_ARGS);

#endif	 /* not WIN32 */

extern PGresult   *PSQLexec(const char *query);

extern bool		SendQuery(const char *query);

#endif	 /* COMMON_H */
