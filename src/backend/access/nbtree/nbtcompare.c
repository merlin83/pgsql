/*-------------------------------------------------------------------------
 *
 * nbtcompare.c
 *	  Comparison functions for btree access method.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/nbtree/nbtcompare.c,v 1.27 1999-07-17 20:16:41 momjian Exp $
 *
 *	NOTES
 *		These functions are stored in pg_amproc.  For each operator class
 *		defined on btrees, they compute
 *
 *				compare(a, b):
 *						< 0 if a < b,
 *						= 0 if a == b,
 *						> 0 if a > b.
 *-------------------------------------------------------------------------
 */


#include "postgres.h"

#include "utils/builtins.h"

int32
btint2cmp(int16 a, int16 b)
{
	return (int32) (a - b);
}

int32
btint4cmp(int32 a, int32 b)
{
	return a - b;
}

int32
btint8cmp(int64 *a, int64 *b)
{
	if (*a > *b)
		return 1;
	else if (*a == *b)
		return 0;
	else
		return -1;
}

int32
btint24cmp(int16 a, int32 b)
{
	return ((int32) a) - b;
}

int32
btint42cmp(int32 a, int16 b)
{
	return a - ((int32) b);
}

int32
btfloat4cmp(float32 a, float32 b)
{
	if (*a > *b)
		return 1;
	else if (*a == *b)
		return 0;
	else
		return -1;
}

int32
btfloat8cmp(float64 a, float64 b)
{
	if (*a > *b)
		return 1;
	else if (*a == *b)
		return 0;
	else
		return -1;
}

int32
btoidcmp(Oid a, Oid b)
{
	if (a > b)
		return 1;
	else if (a == b)
		return 0;
	else
		return -1;
}

int32
btoid8cmp(Oid *a, Oid *b)
{
	int			i;

	for (i = 0; i < 8; i++)
		/* we use this because we need the int4gt, etc */
		if (!int4eq(a[i], b[i]))
		{
			if (int4gt(a[i], b[i]))
				return 1;
			else
				return -1;
		}
	return 0;
}


int32
btabstimecmp(AbsoluteTime a, AbsoluteTime b)
{
	if (AbsoluteTimeIsBefore(a, b))
		return -1;
	else if (AbsoluteTimeIsBefore(b, a))
		return 1;
	else
		return 0;
}

int32
btcharcmp(char a, char b)
{
	return (int32) ((uint8) a - (uint8) b);
}

int32
btnamecmp(NameData *a, NameData *b)
{
	return strncmp(a->data, b->data, NAMEDATALEN);
}

int32
bttextcmp(struct varlena * a, struct varlena * b)
{
	int			res;
	unsigned char *ap,
			   *bp;

#ifdef USE_LOCALE
	int			la = VARSIZE(a) - VARHDRSZ;
	int			lb = VARSIZE(b) - VARHDRSZ;

	ap = (unsigned char *) palloc(la + 1);
	bp = (unsigned char *) palloc(lb + 1);

	memcpy(ap, VARDATA(a), la);
	*(ap + la) = '\0';
	memcpy(bp, VARDATA(b), lb);
	*(bp + lb) = '\0';

	res = strcoll(ap, bp);

	pfree(ap);
	pfree(bp);

#else
	int			len = VARSIZE(a);

	/* len is the length of the shorter of the two strings */
	if (len > VARSIZE(b))
		len = VARSIZE(b);

	len -= VARHDRSZ;

	ap = (unsigned char *) VARDATA(a);
	bp = (unsigned char *) VARDATA(b);

	/*
	 * If the two strings differ in the first len bytes, or if they're the
	 * same in the first len bytes and they're both len bytes long, we're
	 * done.
	 */

	res = 0;
	if (len > 0)
	{
		do
		{
			res = (int) (*ap++ - *bp++);
			len--;
		} while (res == 0 && len != 0);
	}

#endif

	if (res != 0 || VARSIZE(a) == VARSIZE(b))
		return res;

	/*
	 * The two strings are the same in the first len bytes, and they are
	 * of different lengths.
	 */

	if (VARSIZE(a) < VARSIZE(b))
		return -1;
	else
		return 1;
}
