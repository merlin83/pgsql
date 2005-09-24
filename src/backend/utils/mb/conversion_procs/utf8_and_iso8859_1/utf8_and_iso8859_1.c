/*-------------------------------------------------------------------------
 *
 *	  ISO8859_1 <--> UTF8
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/utils/mb/conversion_procs/utf8_and_iso8859_1/utf8_and_iso8859_1.c,v 1.12 2005/09/24 17:53:24 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"

PG_FUNCTION_INFO_V1(iso8859_1_to_utf8);
PG_FUNCTION_INFO_V1(utf8_to_iso8859_1);

extern Datum iso8859_1_to_utf8(PG_FUNCTION_ARGS);
extern Datum utf8_to_iso8859_1(PG_FUNCTION_ARGS);

/* ----------
 * conv_proc(
 *		INTEGER,	-- source encoding id
 *		INTEGER,	-- destination encoding id
 *		CSTRING,	-- source string (null terminated C string)
 *		CSTRING,	-- destination string (null terminated C string)
 *		INTEGER		-- source string length
 * ) returns VOID;
 * ----------
 */

Datum
iso8859_1_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);
	unsigned short c;

	Assert(PG_GETARG_INT32(0) == PG_LATIN1);
	Assert(PG_GETARG_INT32(1) == PG_UTF8);
	Assert(len >= 0);

	while (len-- > 0 && (c = *src++))
	{
		if (c < 0x80)
			*dest++ = c;
		else
		{
			*dest++ = (c >> 6) | 0xc0;
			*dest++ = (c & 0x003f) | 0x80;
		}
	}
	*dest = '\0';

	PG_RETURN_VOID();
}

Datum
utf8_to_iso8859_1(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);
	unsigned short c,
				c1,
				c2;

	Assert(PG_GETARG_INT32(0) == PG_UTF8);
	Assert(PG_GETARG_INT32(1) == PG_LATIN1);
	Assert(len >= 0);

	while (len >= 0 && (c = *src++))
	{
		if ((c & 0xe0) == 0xc0)
		{
			c1 = c & 0x1f;
			c2 = *src++ & 0x3f;
			*dest = c1 << 6;
			*dest++ |= c2;
			len -= 2;
		}
		else if ((c & 0xe0) == 0xe0)
			elog(ERROR, "could not convert UTF8 character 0x%04x to ISO8859-1",
				 c);
		else
		{
			*dest++ = c;
			len--;
		}
	}
	*dest = '\0';

	PG_RETURN_VOID();
}
