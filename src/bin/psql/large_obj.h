/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright 2000 by PostgreSQL Global Development Team
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/psql/large_obj.h,v 1.6 2000-01-18 23:30:23 petere Exp $
 */
#ifndef LARGE_OBJ_H
#define LARGE_OBJ_H

#include <c.h>

bool		do_lo_export(const char *loid_arg, const char *filename_arg);
bool		do_lo_import(const char *filename_arg, const char *comment_arg);
bool		do_lo_unlink(const char *loid_arg);
bool		do_lo_list(void);

#endif	 /* LARGE_OBJ_H */
