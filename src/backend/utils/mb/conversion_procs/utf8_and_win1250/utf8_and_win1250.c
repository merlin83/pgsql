/*-------------------------------------------------------------------------
 *
 *	  WIN1250 and UTF-8
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/mb/conversion_procs/utf8_and_win1250/Attic/utf8_and_win1250.c,v 1.6.4.2 2009-01-29 19:25:15 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "../../Unicode/utf8_to_win1250.map"
#include "../../Unicode/win1250_to_utf8.map"

PG_FUNCTION_INFO_V1(utf_to_win1250);
PG_FUNCTION_INFO_V1(win1250_to_utf);

extern Datum utf_to_win1250(PG_FUNCTION_ARGS);
extern Datum win1250_to_utf(PG_FUNCTION_ARGS);

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
utf_to_win1250(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_UTF8, PG_WIN1250);

	UtfToLocal(src, dest, ULmapWIN1250,
			   sizeof(ULmapWIN1250) / sizeof(pg_utf_to_local), PG_WIN1250, len);

	PG_RETURN_VOID();
}

Datum
win1250_to_utf(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_WIN1250, PG_UTF8);

	LocalToUtf(src, dest, LUmapWIN1250,
		sizeof(LUmapWIN1250) / sizeof(pg_local_to_utf), PG_WIN1250, len);

	PG_RETURN_VOID();
}
