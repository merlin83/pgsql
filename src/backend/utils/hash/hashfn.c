/*-------------------------------------------------------------------------
 *
 * hashfn.c
 *		Hash functions for use in dynahash.c hashtables
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/hash/hashfn.c,v 1.16 2002-03-09 17:35:36 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/hash.h"
#include "utils/hsearch.h"


/*
 * string_hash: hash function for keys that are null-terminated strings.
 *
 * NOTE: since dynahash.c backs this up with a fixed-length memcmp(),
 * the key must actually be zero-padded to the specified maximum length
 * to work correctly.  However, if it is known that nothing after the
 * first zero byte is interesting, this is the right hash function to use.
 *
 * NOTE: this is the default hash function if none is specified.
 */
uint32
string_hash(void *key, int keysize)
{
	return DatumGetUInt32(hash_any((unsigned char *) key, strlen((char *) key)));
}

/*
 * tag_hash: hash function for fixed-size tag values
 */
uint32
tag_hash(void *key, int keysize)
{
	return DatumGetUInt32(hash_any((unsigned char *) key, keysize));
}
