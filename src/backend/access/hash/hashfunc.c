/*-------------------------------------------------------------------------
 *
 * hashfunc.c
 *	  Comparison functions for hash access method.
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/hash/hashfunc.c,v 1.32 2002-03-06 20:49:38 momjian Exp $
 *
 * NOTES
 *	  These functions are stored in pg_amproc.	For each operator class
 *	  defined on hash tables, they compute the hash value of the argument.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/hash.h"


Datum
hashchar(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(~((uint32) PG_GETARG_CHAR(0)));
}

Datum
hashint2(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(~((uint32) PG_GETARG_INT16(0)));
}

Datum
hashint4(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(~PG_GETARG_UINT32(0));
}

Datum
hashint8(PG_FUNCTION_ARGS)
{
	/* we just use the low 32 bits... */
	PG_RETURN_UINT32(~((uint32) PG_GETARG_INT64(0)));
}

Datum
hashoid(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(~((uint32) PG_GETARG_OID(0)));
}

Datum
hashfloat4(PG_FUNCTION_ARGS)
{
	float4		key = PG_GETARG_FLOAT4(0);

	return hash_any((char *) &key, sizeof(key));
}

Datum
hashfloat8(PG_FUNCTION_ARGS)
{
	float8		key = PG_GETARG_FLOAT8(0);

	return hash_any((char *) &key, sizeof(key));
}

Datum
hashoidvector(PG_FUNCTION_ARGS)
{
	Oid		   *key = (Oid *) PG_GETARG_POINTER(0);

	return hash_any((char *) key, INDEX_MAX_KEYS * sizeof(Oid));
}

/*
 * Note: hashint2vector currently can't be used as a user hash table
 * hash function, because it has no pg_proc entry.	We only need it
 * for catcache indexing.
 */
Datum
hashint2vector(PG_FUNCTION_ARGS)
{
	int16	   *key = (int16 *) PG_GETARG_POINTER(0);

	return hash_any((char *) key, INDEX_MAX_KEYS * sizeof(int16));
}

Datum
hashname(PG_FUNCTION_ARGS)
{
	char	   *key = NameStr(*PG_GETARG_NAME(0));

	Assert(strlen(key) <= NAMEDATALEN);

	return hash_any(key, strlen(key));
}

/*
 * hashvarlena() can be used for any varlena datatype in which there are
 * no non-significant bits, ie, distinct bitpatterns never compare as equal.
 */
Datum
hashvarlena(PG_FUNCTION_ARGS)
{
	struct varlena *key = PG_GETARG_VARLENA_P(0);
	Datum		result;

	result = hash_any(VARDATA(key), VARSIZE(key) - VARHDRSZ);

	/* Avoid leaking memory for toasted inputs */
	PG_FREE_IF_COPY(key, 0);

	return result;
}

/* This hash function was written by Bob Jenkins
 * (bob_jenkins@burtleburtle.net), and superficially adapted
 * for PostgreSQL by Neil Conway. For more information on this
 * hash function, see http://burtleburtle.net/bob/hash/doobs.html
 */

/*
 * mix -- mix 3 32-bit values reversibly.
 * For every delta with one or two bits set, and the deltas of all three
 * high bits or all three low bits, whether the original value of a,b,c
 * is almost all zero or is uniformly distributed,
 * - If mix() is run forward or backward, at least 32 bits in a,b,c
 *   have at least 1/4 probability of changing.
 * - If mix() is run forward, every bit of c will change between 1/3 and
 *   2/3 of the time.  (Well, 22/100 and 78/100 for some 2-bit deltas.)
 */
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

/*
 * hash_any() -- hash a variable-length key into a 32-bit value
 *      k       : the key (the unaligned variable-length array of bytes)
 *      len     : the length of the key, counting by bytes
 * Returns a 32-bit value.  Every bit of the key affects every bit of
 * the return value.  Every 1-bit and 2-bit delta achieves avalanche.
 * About 6*len+35 instructions. The best hash table sizes are powers
 * of 2.  There is no need to do mod a prime (mod is sooo slow!).
 * If you need less than 32 bits, use a bitmask.
 */
Datum
hash_any(register const char *k, register int keylen)
{
   register Datum a,b,c,len;

   /* Set up the internal state */
   len = keylen;
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   /* Another arbitrary value. If the hash function is called
    * multiple times, this could be the previously generated
    * hash value; however, the interface currently doesn't allow
    * this. AFAIK this isn't a big deal.
    */
   c = 3923095;

   /* handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((Datum)k[1]<<8) +((Datum)k[2]<<16) +((Datum)k[3]<<24));
      b += (k[4] +((Datum)k[5]<<8) +((Datum)k[6]<<16) +((Datum)k[7]<<24));
      c += (k[8] +((Datum)k[9]<<8) +((Datum)k[10]<<16)+((Datum)k[11]<<24));
      mix(a,b,c);
      k += 12; len -= 12;
   }

   /* handle the last 11 bytes */
   c += keylen;
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((Datum)k[10]<<24);
   case 10: c+=((Datum)k[9]<<16);
   case 9 : c+=((Datum)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((Datum)k[7]<<24);
   case 7 : b+=((Datum)k[6]<<16);
   case 6 : b+=((Datum)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((Datum)k[3]<<24);
   case 3 : a+=((Datum)k[2]<<16);
   case 2 : a+=((Datum)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /* report the result */
   return c;
}
