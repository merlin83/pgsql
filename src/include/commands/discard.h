/*-------------------------------------------------------------------------
 *
 * discard.h
 *	  prototypes for discard.c.
 *
 *
 * Copyright (c) 1996-2007, PostgreSQL Global Development Group
 *
 * $PostgreSQL: pgsql/src/include/commands/discard.h,v 1.3 2007/11/15 22:25:17 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef DISCARD_H
#define DISCARD_H

#include "nodes/parsenodes.h"

extern void DiscardCommand(DiscardStmt *stmt, bool isTopLevel);

#endif   /* DISCARD_H */