/*-------------------------------------------------------------------------
 *
 *	  GB18030 <--> UTF-8
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/utils/mb/conversion_procs/utf8_and_gb18030/utf8_and_gb18030.c,v 1.9 2004/12/31 22:02:23 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "../../Unicode/gb18030_to_utf8.map"
#include "../../Unicode/utf8_to_gb18030.map"

PG_FUNCTION_INFO_V1(gb18030_to_utf8);
PG_FUNCTION_INFO_V1(utf8_to_gb18030);

extern Datum gb18030_to_utf8(PG_FUNCTION_ARGS);
extern Datum utf8_to_gb18030(PG_FUNCTION_ARGS);

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
gb18030_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	Assert(PG_GETARG_INT32(0) == PG_GB18030);
	Assert(PG_GETARG_INT32(1) == PG_UTF8);
	Assert(len >= 0);

	LocalToUtf(src, dest, LUmapGB18030,
		sizeof(LUmapGB18030) / sizeof(pg_local_to_utf), PG_GB18030, len);

	PG_RETURN_VOID();
}

Datum
utf8_to_gb18030(PG_FUNCTION_ARGS)
{
	unsigned char *src = PG_GETARG_CSTRING(2);
	unsigned char *dest = PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	Assert(PG_GETARG_INT32(0) == PG_UTF8);
	Assert(PG_GETARG_INT32(1) == PG_GB18030);
	Assert(len >= 0);

	UtfToLocal(src, dest, ULmapGB18030,
			   sizeof(ULmapGB18030) / sizeof(pg_utf_to_local), len);

	PG_RETURN_VOID();
}
