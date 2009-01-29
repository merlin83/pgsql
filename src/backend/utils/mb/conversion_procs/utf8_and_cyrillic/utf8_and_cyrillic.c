/*-------------------------------------------------------------------------
 *
 *	  UTF8 and Cyrillic
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/mb/conversion_procs/utf8_and_cyrillic/utf8_and_cyrillic.c,v 1.6.4.2 2009-01-29 19:25:13 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "../../Unicode/utf8_to_koi8r.map"
#include "../../Unicode/koi8r_to_utf8.map"
#include "../../Unicode/utf8_to_win1251.map"
#include "../../Unicode/win1251_to_utf8.map"
#include "../../Unicode/utf8_to_alt.map"
#include "../../Unicode/alt_to_utf8.map"

PG_FUNCTION_INFO_V1(utf8_to_koi8r);
PG_FUNCTION_INFO_V1(koi8r_to_utf8);
PG_FUNCTION_INFO_V1(utf8_to_win1251);
PG_FUNCTION_INFO_V1(win1251_to_utf8);
PG_FUNCTION_INFO_V1(utf8_to_alt);
PG_FUNCTION_INFO_V1(alt_to_utf8);

extern Datum utf8_to_koi8r(PG_FUNCTION_ARGS);
extern Datum koi8r_to_utf8(PG_FUNCTION_ARGS);
extern Datum utf8_to_win1251(PG_FUNCTION_ARGS);
extern Datum win1251_to_utf8(PG_FUNCTION_ARGS);
extern Datum utf8_to_alt(PG_FUNCTION_ARGS);
extern Datum alt_to_utf8(PG_FUNCTION_ARGS);

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
utf8_to_koi8r(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_UTF8, PG_KOI8R);

	UtfToLocal(src, dest, ULmap_KOI8R,
			   sizeof(ULmap_KOI8R) / sizeof(pg_utf_to_local), PG_KOI8R, len);

	PG_RETURN_VOID();
}

Datum
koi8r_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_KOI8R, PG_UTF8);

	LocalToUtf(src, dest, LUmapKOI8R,
			sizeof(LUmapKOI8R) / sizeof(pg_local_to_utf), PG_KOI8R, len);

	PG_RETURN_VOID();
}

Datum
utf8_to_win1251(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_UTF8, PG_WIN1251);

	UtfToLocal(src, dest, ULmap_WIN1251,
			   sizeof(ULmap_WIN1251) / sizeof(pg_utf_to_local), PG_WIN1251, len);

	PG_RETURN_VOID();
}

Datum
win1251_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_WIN1251, PG_UTF8);

	LocalToUtf(src, dest, LUmapWIN1251,
		sizeof(LUmapWIN1251) / sizeof(pg_local_to_utf), PG_WIN1251, len);

	PG_RETURN_VOID();
}

Datum
utf8_to_alt(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_UTF8, PG_ALT);

	UtfToLocal(src, dest, ULmap_ALT,
			   sizeof(ULmap_ALT) / sizeof(pg_utf_to_local), PG_ALT, len);

	PG_RETURN_VOID();
}

Datum
alt_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	CHECK_ENCODING_CONVERSION_ARGS(PG_ALT, PG_UTF8);

	LocalToUtf(src, dest, LUmapALT,
			   sizeof(LUmapALT) / sizeof(pg_local_to_utf), PG_ALT, len);

	PG_RETURN_VOID();
}
