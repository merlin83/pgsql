/*-------------------------------------------------------------------------
 *
 * hasht.h--
 *    hash table related functions that are not directly supported
 *    under utils/hash.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: hasht.h,v 1.1.1.1 1996-07-09 06:21:29 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef	HASHT_H
#define HASHT_H

#include "utils/hsearch.h"

typedef void (*HashtFunc)();

extern void HashTableWalk(HTAB *hashtable, HashtFunc function, int arg);

#endif	/* HASHT_H */
