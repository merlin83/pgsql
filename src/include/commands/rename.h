/*-------------------------------------------------------------------------
 *
 * rename.h--
 *
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: rename.h,v 1.2 1997-09-07 04:57:30 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef RENAME_H
#define RENAME_H

extern void
renameatt(char *relname,
		  char *oldattname,
		  char *newattname,
		  char *userName, int recurse);

extern void
renamerel(char *oldrelname,
		  char *newrelname);

#endif							/* RENAME_H */
