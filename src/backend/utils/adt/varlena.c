/*-------------------------------------------------------------------------
 *
 * varlena.c--
 *	  Functions for the variable-length built-in types.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/varlena.c,v 1.44.2.1 1998-12-14 00:13:56 thomas Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <ctype.h>
#include <string.h>

#include "postgres.h"
#include "utils/palloc.h"
#include "utils/builtins.h"		/* where function declarations go */

#include "mb/pg_wchar.h"

static int	text_cmp(text *arg1, text *arg2);


/*****************************************************************************
 *	 USER I/O ROUTINES														 *
 *****************************************************************************/


#define VAL(CH)			((CH) - '0')
#define DIG(VAL)		((VAL) + '0')

/*
 *		byteain			- converts from printable representation of byte array
 *
 *		Non-printable characters must be passed as '\nnn' (octal) and are
 *		converted to internal form.  '\' must be passed as '\\'.
 *		elog(ERROR, ...) if bad form.
 *
 *		BUGS:
 *				The input is scaned twice.
 *				The error checking of input is minimal.
 */
text *
byteain(char *inputText)
{
	char	   *tp;
	char	   *rp;
	int			byte;
	text	   *result;

	if (inputText == NULL)
		elog(ERROR, "Bad input string for type bytea");

	for (byte = 0, tp = inputText; *tp != '\0'; byte++)
		if (*tp++ == '\\')
		{
			if (*tp == '\\')
				tp++;
			else if (!isdigit(*tp++) ||
					 !isdigit(*tp++) ||
					 !isdigit(*tp++))
				elog(ERROR, "Bad input string for type bytea");
		}
	tp = inputText;
	byte += VARHDRSZ;
	result = (text *) palloc(byte);
	result->vl_len = byte;		/* varlena? */
	rp = result->vl_dat;
	while (*tp != '\0')
		if (*tp != '\\' || *++tp == '\\')
			*rp++ = *tp++;
		else
		{
			byte = VAL(*tp++);
			byte <<= 3;
			byte += VAL(*tp++);
			byte <<= 3;
			*rp++ = byte + VAL(*tp++);
		}
	return result;
}

/*
 *		byteaout		- converts to printable representation of byte array
 *
 *		Non-printable characters are inserted as '\nnn' (octal) and '\' as
 *		'\\'.
 *
 *		NULL vlena should be an error--returning string with NULL for now.
 */
char *
byteaout(text *vlena)
{
	char	   *result;

	char	   *vp;
	char	   *rp;
	int			val;			/* holds unprintable chars */
	int			i;
	int			len;

	if (vlena == NULL)
	{
		result = (char *) palloc(2);
		result[0] = '-';
		result[1] = '\0';
		return result;
	}
	vp = vlena->vl_dat;
	len = 1;					/* empty string has 1 char */
	for (i = vlena->vl_len - VARHDRSZ; i != 0; i--, vp++)
		if (*vp == '\\')
			len += 2;
		else if (isascii(*vp) && isprint(*vp))
			len++;
		else
			len += VARHDRSZ;
	rp = result = (char *) palloc(len);
	vp = vlena->vl_dat;
	for (i = vlena->vl_len - VARHDRSZ; i != 0; i--)
		if (*vp == '\\')
		{
			vp++;
			*rp++ = '\\';
			*rp++ = '\\';
		}
		else if (isascii(*vp) && isprint(*vp))
			*rp++ = *vp++;
		else
		{
			val = *vp++;
			*rp = '\\';
			rp += 3;
			*rp-- = DIG(val & 07);
			val >>= 3;
			*rp-- = DIG(val & 07);
			val >>= 3;
			*rp = DIG(val & 03);
			rp += 3;
		}
	*rp = '\0';
	return result;
}


/*
 *		textin			- converts "..." to internal representation
 */
text *
textin(char *inputText)
{
	text	   *result;
	int			len;

	if (inputText == NULL)
		return NULL;

	len = strlen(inputText) + VARHDRSZ;
	result = (text *) palloc(len);
	VARSIZE(result) = len;

	memmove(VARDATA(result), inputText, len - VARHDRSZ);

#ifdef CYR_RECODE
	convertstr(VARDATA(result), len - VARHDRSZ, 0);
#endif

	return result;
}

/*
 *		textout			- converts internal representation to "..."
 */
char *
textout(text *vlena)
{
	int			len;
	char	   *result;

	if (vlena == NULL)
	{
		result = (char *) palloc(2);
		result[0] = '-';
		result[1] = '\0';
		return result;
	}
	len = VARSIZE(vlena) - VARHDRSZ;
	result = (char *) palloc(len + 1);
	memmove(result, VARDATA(vlena), len);
	result[len] = '\0';

#ifdef CYR_RECODE
	convertstr(result, len, 1);
#endif

	return result;
}


/* ========== PUBLIC ROUTINES ========== */

/*
 * textlen -
 *	  returns the logical length of a text*
 *	   (which is less than the VARSIZE of the text*)
 */
int32
textlen(text *t)
{
#ifdef MULTIBYTE
	unsigned char *s;
	int			len,
				l,
				wl;

#endif

	if (!PointerIsValid(t))
		elog(ERROR, "Null input to textlen");

#ifdef MULTIBYTE
	len = 0;
	s = VARDATA(t);
	l = VARSIZE(t) - VARHDRSZ;
	while (l > 0)
	{
		wl = pg_mblen(s);
		l -= wl;
		s += wl;
		len++;
	}
	return (len);
#else
	return VARSIZE(t) - VARHDRSZ;
#endif

}	/* textlen() */

/*
 * textoctetlen -
 *	  returns the physical length of a text*
 *	   (which is less than the VARSIZE of the text*)
 */
int32
textoctetlen(text *t)
{
	if (!PointerIsValid(t))
		elog(ERROR, "Null input to textoctetlen");

	return VARSIZE(t) - VARHDRSZ;

}	/* textoctetlen() */

/*
 * textcat -
 *	  takes two text* and returns a text* that is the concatentation of
 *	  the two.
 *
 * Rewritten by Sapa, sapa@hq.icb.chel.su. 8-Jul-96.
 * Updated by Thomas, Thomas.Lockhart@jpl.nasa.gov 1997-07-10.
 * Allocate space for output in all cases.
 * XXX - thomas 1997-07-10
 */
text *
textcat(text *t1, text *t2)
{
	int			len1,
				len2,
				len;
	char	   *ptr;
	text	   *result;

	if (!PointerIsValid(t1) || !PointerIsValid(t2))
		return NULL;

	len1 = (VARSIZE(t1) - VARHDRSZ);
	if (len1 < 0)
		len1 = 0;
	while (len1 > 0 && VARDATA(t1)[len1 - 1] == '\0')
		len1--;

	len2 = (VARSIZE(t2) - VARHDRSZ);
	if (len2 < 0)
		len2 = 0;
	while (len2 > 0 && VARDATA(t2)[len2 - 1] == '\0')
		len2--;

	len = len1 + len2 + VARHDRSZ;
	result = palloc(len);

	/* Set size of result string... */
	VARSIZE(result) = len;

	/* Fill data field of result string... */
	ptr = VARDATA(result);
	if (len1 > 0)
		memcpy(ptr, VARDATA(t1), len1);
	if (len2 > 0)
		memcpy(ptr + len1, VARDATA(t2), len2);

	return result;
}	/* textcat() */

/*
 * text_substr()
 * Return a substring starting at the specified position.
 * - thomas 1997-12-31
 *
 * Input:
 *	- string
 *	- starting position (is one-based)
 *	- string length
 *
 * If the starting position is zero or less, then return from the start of the string
 *  adjusting the length to be consistant with the "negative start" per SQL92.
 * If the length is less than zero, return the remaining string.
 *
 * Note that the arguments operate on octet length,
 *	so not aware of multi-byte character sets.
 *
 * Added multi-byte support.
 * - Tatsuo Ishii 1998-4-21
 * Changed behavior if starting position is less than one to conform to SQL92 behavior.
 * Formerly returned the entire string; now returns a portion.
 * - Thomas Lockhart 1998-12-10
 */
text *
text_substr(text *string, int32 m, int32 n)
{
	text	   *ret;
	int			len;

#ifdef MULTIBYTE
	int			i;
	char	   *p;

#endif

	if (string == (text *) NULL)
		return string;

	len = VARSIZE(string) - VARHDRSZ;
#ifdef MULTIBYTE
	len = pg_mbstrlen_with_len(VARDATA(string), len);
#endif

	/* starting position after the end of the string? */
	if (m > len)
	{
		m = 1;
		n = 0;
	}
	/* starting position before the start of the string?
	 * then offset into the string per SQL92 spec... */
	else if (m < 1)
	{
		n += (m-1);
		m = 1;
	}

	/* m will now become a zero-based starting position */
	m--;
	if (((m + n) > len) || (n < 0))
		n = (len - m);

#ifdef MULTIBYTE
	p = VARDATA(string);
	for (i = 0; i < m; i++)
		p += pg_mblen(p);
	m = p - VARDATA(string);
	for (i = 0; i < n; i++)
		p += pg_mblen(p);
	n = p - (VARDATA(string) + m);
#endif
	ret = (text *) palloc(VARHDRSZ + n);
	VARSIZE(ret) = VARHDRSZ + n;

	memcpy(VARDATA(ret), VARDATA(string) + m, n);

	return ret;
}	/* text_substr() */

/*
 * textpos -
 *	  Return the position of the specified substring.
 *	  Implements the SQL92 POSITION() function.
 *	  Ref: A Guide To The SQL Standard, Date & Darwen, 1997
 * - thomas 1997-07-27
 *
 * Added multi-byte support.
 * - Tatsuo Ishii 1998-4-21
 */
int32
textpos(text *t1, text *t2)
{
	int			pos;
	int			px,
				p;
	int			len1,
				len2;
	pg_wchar   *p1,
			   *p2;

#ifdef MULTIBYTE
	pg_wchar   *ps1,
			   *ps2;

#endif

	if (!PointerIsValid(t1) || !PointerIsValid(t2))
		return 0;

	if (VARSIZE(t2) <= 0)
		return 1;

	len1 = (VARSIZE(t1) - VARHDRSZ);
	len2 = (VARSIZE(t2) - VARHDRSZ);
#ifdef MULTIBYTE
	ps1 = p1 = (pg_wchar *) palloc((len1 + 1) * sizeof(pg_wchar));
	(void) pg_mb2wchar_with_len((unsigned char *) VARDATA(t1), p1, len1);
	len1 = pg_wchar_strlen(p1);
	ps2 = p2 = (pg_wchar *) palloc((len2 + 1) * sizeof(pg_wchar));
	(void) pg_mb2wchar_with_len((unsigned char *) VARDATA(t2), p2, len2);
	len2 = pg_wchar_strlen(p2);
#else
	p1 = VARDATA(t1);
	p2 = VARDATA(t2);
#endif
	pos = 0;
	px = (len1 - len2);
	for (p = 0; p <= px; p++)
	{
#ifdef MULTIBYTE
		if ((*p2 == *p1) && (pg_wchar_strncmp(p1, p2, len2) == 0))
#else
		if ((*p2 == *p1) && (strncmp(p1, p2, len2) == 0))
#endif
		{
			pos = p + 1;
			break;
		};
		p1++;
	};
#ifdef MULTIBYTE
	pfree(ps1);
	pfree(ps2);
#endif
	return pos;
}	/* textpos() */

/*
 *		texteq			- returns 1 iff arguments are equal
 *		textne			- returns 1 iff arguments are not equal
 */
bool
texteq(text *arg1, text *arg2)
{
	int			len;
	char	   *a1p,
			   *a2p;

	if (arg1 == NULL || arg2 == NULL)
		return (bool) NULL;
	if ((len = arg1->vl_len) != arg2->vl_len)
		return (bool) 0;
	a1p = arg1->vl_dat;
	a2p = arg2->vl_dat;

	/*
	 * Varlenas are stored as the total size (data + size variable)
	 * followed by the data. Use VARHDRSZ instead of explicit sizeof() -
	 * thomas 1997-07-10
	 */
	len -= VARHDRSZ;
	while (len-- != 0)
		if (*a1p++ != *a2p++)
			return (bool) 0;
	return (bool) 1;
}	/* texteq() */

bool
textne(text *arg1, text *arg2)
{
	return (bool) !texteq(arg1, arg2);
}

/* varstr_cmp()
 * Comparison function for text strings with given lengths.
 * Includes locale support, but must copy strings to temporary memory
 *	to allow null-termination for inputs to strcoll().
 * Returns -1, 0 or 1
 */
int
varstr_cmp(char *arg1, int len1, char *arg2, int len2)
{
	int			result;
	char	   *a1p,
			   *a2p;

#ifdef USE_LOCALE
	a1p = (unsigned char *) palloc(len1 + 1);
	a2p = (unsigned char *) palloc(len2 + 1);

	memcpy(a1p, arg1, len1);
	*(a1p + len1) = '\0';
	memcpy(a2p, arg2, len2);
	*(a2p + len2) = '\0';

	result = strcoll(a1p, a2p);

	pfree(a1p);
	pfree(a2p);

#else

	a1p = arg1;
	a2p = arg2;

	result = strncmp(a1p, a2p, Min(len1, len2));
	if ((result == 0) && (len1 != len2))
		result = (len1 < len2) ? -1 : 1;
#endif

	return result;
}	/* varstr_cmp() */


/* text_cmp()
 * Comparison function for text strings.
 * Includes locale support, but must copy strings to temporary memory
 *	to allow null-termination for inputs to strcoll().
 * XXX HACK code for textlen() indicates that there can be embedded nulls
 *	but it appears that most routines (incl. this one) assume not! - tgl 97/04/07
 * Returns -1, 0 or 1
 */
static int
text_cmp(text *arg1, text *arg2)
{
	char	   *a1p,
			   *a2p;
	int			len1,
				len2;

	if (arg1 == NULL || arg2 == NULL)
		return (bool) FALSE;

	a1p = VARDATA(arg1);
	a2p = VARDATA(arg2);

	len1 = VARSIZE(arg1) - VARHDRSZ;
	len2 = VARSIZE(arg2) - VARHDRSZ;

	return varstr_cmp(a1p, len1, a2p, len2);
}	/* text_cmp() */

/* text_lt()
 * Comparison function for text strings.
 */
bool
text_lt(text *arg1, text *arg2)
{
	return (bool) (text_cmp(arg1, arg2) < 0);
}	/* text_lt() */

/* text_le()
 * Comparison function for text strings.
 */
bool
text_le(text *arg1, text *arg2)
{
	return (bool) (text_cmp(arg1, arg2) <= 0);
}	/* text_le() */

bool
text_gt(text *arg1, text *arg2)
{
	return (bool) !text_le(arg1, arg2);
}

bool
text_ge(text *arg1, text *arg2)
{
	return (bool) !text_lt(arg1, arg2);
}

/*-------------------------------------------------------------
 * byteaGetSize
 *
 * get the number of bytes contained in an instance of type 'bytea'
 *-------------------------------------------------------------
 */
int32
byteaGetSize(text *v)
{
	int			len;

	len = v->vl_len - sizeof(v->vl_len);

	return len;
}

/*-------------------------------------------------------------
 * byteaGetByte
 *
 * this routine treats "bytea" as an array of bytes.
 * It returns the Nth byte (a number between 0 and 255) or
 * it dies if the length of this array is less than n.
 *-------------------------------------------------------------
 */
int32
byteaGetByte(text *v, int32 n)
{
	int			len;
	int			byte;

	len = byteaGetSize(v);

	if (n >= len)
	{
		elog(ERROR, "byteaGetByte: index (=%d) out of range [0..%d]",
			 n, len - 1);
	}
#ifdef USE_LOCALE
	byte = (unsigned char) (v->vl_dat[n]);
#else
	byte = v->vl_dat[n];
#endif
	return (int32) byte;
}

/*-------------------------------------------------------------
 * byteaGetBit
 *
 * This routine treats a "bytea" type like an array of bits.
 * It returns the value of the Nth bit (0 or 1).
 * If 'n' is out of range, it dies!
 *
 *-------------------------------------------------------------
 */
int32
byteaGetBit(text *v, int32 n)
{
	int			byteNo,
				bitNo;
	int			byte;

	byteNo = n / 8;
	bitNo = n % 8;

	byte = byteaGetByte(v, byteNo);

	if (byte & (1 << bitNo))
		return (int32) 1;
	else
		return (int32) 0;
}

/*-------------------------------------------------------------
 * byteaSetByte
 *
 * Given an instance of type 'bytea' creates a new one with
 * the Nth byte set to the given value.
 *
 *-------------------------------------------------------------
 */
text *
byteaSetByte(text *v, int32 n, int32 newByte)
{
	int			len;
	text	   *res;

	len = byteaGetSize(v);

	if (n >= len)
	{
		elog(ERROR,
			 "byteaSetByte: index (=%d) out of range [0..%d]",
			 n, len - 1);
	}

	/*
	 * Make a copy of the original varlena.
	 */
	res = (text *) palloc(VARSIZE(v));
	if (res == NULL)
	{
		elog(ERROR, "byteaSetByte: Out of memory (%d bytes requested)",
			 VARSIZE(v));
	}
	memmove((char *) res, (char *) v, VARSIZE(v));

	/*
	 * Now set the byte.
	 */
	res->vl_dat[n] = newByte;

	return res;
}

/*-------------------------------------------------------------
 * byteaSetBit
 *
 * Given an instance of type 'bytea' creates a new one with
 * the Nth bit set to the given value.
 *
 *-------------------------------------------------------------
 */
text *
byteaSetBit(text *v, int32 n, int32 newBit)
{
	text	   *res;
	int			oldByte,
				newByte;
	int			byteNo,
				bitNo;

	/*
	 * sanity check!
	 */
	if (newBit != 0 && newBit != 1)
		elog(ERROR, "byteaSetByte: new bit must be 0 or 1");

	/*
	 * get the byte where the bit we want is stored.
	 */
	byteNo = n / 8;
	bitNo = n % 8;
	oldByte = byteaGetByte(v, byteNo);

	/*
	 * calculate the new value for that byte
	 */
	if (newBit == 0)
		newByte = oldByte & (~(1 << bitNo));
	else
		newByte = oldByte | (1 << bitNo);

	/*
	 * NOTE: 'byteaSetByte' creates a copy of 'v' & sets the byte.
	 */
	res = byteaSetByte(v, byteNo, newByte);

	return res;
}


/* text_name()
 * Converts a text() type to a NameData type.
 */
NameData   *
text_name(text *s)
{
	NameData   *result;
	int			len;

	if (s == NULL)
		return NULL;

	len = VARSIZE(s) - VARHDRSZ;
	if (len > NAMEDATALEN)
		len = NAMEDATALEN;

#ifdef STRINGDEBUG
	printf("text- convert string length %d (%d) ->%d\n",
		   VARSIZE(s) - VARHDRSZ, VARSIZE(s), len);
#endif

	result = palloc(NAMEDATALEN);
	StrNCpy(result->data, VARDATA(s), NAMEDATALEN);

	/* now null pad to full length... */
	while (len < NAMEDATALEN)
	{
		*(result->data + len) = '\0';
		len++;
	}

	return result;
}	/* text_name() */

/* name_text()
 * Converts a NameData type to a text type.
 */
text *
name_text(NameData *s)
{
	text	   *result;
	int			len;

	if (s == NULL)
		return NULL;

	len = strlen(s->data);

#ifdef STRINGDEBUG
	printf("text- convert string length %d (%d) ->%d\n",
		   VARSIZE(s) - VARHDRSZ, VARSIZE(s), len);
#endif

	result = palloc(VARHDRSZ + len);
	strncpy(VARDATA(result), s->data, len);
	VARSIZE(result) = len + VARHDRSZ;

	return result;
}	/* name_text() */
