/*-------------------------------------------------------------------------
 *
 * hasht.c--
 *    hash table related functions that are not directly supported
 *    by the hashing packages under utils/hash.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/lib/Attic/hasht.c,v 1.1.1.1 1996-07-09 06:21:29 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "c.h"
#include "utils/memutils.h"
#include "utils/elog.h"
#include "utils/hsearch.h"
#include "lib/hasht.h"

/* -----------------------------------
 *	HashTableWalk
 *
 *	call function on every element in hashtable
 *	one extra argument, arg may be supplied
 * -----------------------------------
 */
void
HashTableWalk(HTAB *hashtable, HashtFunc function, int arg)
{
    long *hashent;
    long *data;
    int keysize;
    
    keysize = hashtable->hctl->keysize;
    (void)hash_seq((HTAB *)NULL);
    while ((hashent = hash_seq(hashtable)) != (long *) TRUE) {
	if (hashent == NULL)
	    elog(FATAL, "error in HashTableWalk.");
	/* 
	 * XXX the corresponding hash table insertion does NOT
	 * LONGALIGN -- make sure the keysize is ok
	 */
	data = (long *) LONGALIGN((char*) hashent + keysize);
	(*function)(data, arg);
    }
}
