/*-------------------------------------------------------------------------
 *
 * readfuncs.h--
 *	  header file for read.c and readfuncs.c. These functions are internal
 *	  to the stringToNode interface and should not be used by anyone else.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: readfuncs.h,v 1.5 1997-11-26 01:13:09 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef READFUNCS_H
#define READFUNCS_H

#include "nodes/nodes.h"

/*
 * prototypes for functions in read.c (the lisp token parser)
 */
extern char *lsptok(char *string, int *length);
extern void *nodeRead(bool read_car_only);

/*
 * prototypes for functions in readfuncs.c
 */
extern Node *parsePlanString(void);

#endif							/* READFUNCS_H */
