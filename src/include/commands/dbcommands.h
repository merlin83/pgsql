/*-------------------------------------------------------------------------
 *
 * dbcommands.h
 *
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: dbcommands.h,v 1.24 2002-06-20 20:29:49 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef DBCOMMANDS_H
#define DBCOMMANDS_H

#include <nodes/parsenodes.h>

extern void createdb(const CreatedbStmt *stmt);
extern void dropdb(const char *dbname);
extern void AlterDatabaseSet(AlterDatabaseSetStmt *stmt);

#endif   /* DBCOMMANDS_H */
