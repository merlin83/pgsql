/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright 2000 by PostgreSQL Global Development Group
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/psql/large_obj.h,v 1.10 2001-10-28 06:25:58 momjian Exp $
 */
#ifndef LARGE_OBJ_H
#define LARGE_OBJ_H

bool		do_lo_export(const char *loid_arg, const char *filename_arg);
bool		do_lo_import(const char *filename_arg, const char *comment_arg);
bool		do_lo_unlink(const char *loid_arg);
bool		do_lo_list(void);

#endif	 /* LARGE_OBJ_H */
