/*-------------------------------------------------------------------------
 *
 *	  EUC_JIS_2004 <--> UTF8
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/utils/mb/conversion_procs/utf8_and_euc_jis_2004/utf8_and_euc_jis_2004.c,v 1.4 2009/01/01 17:23:52 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "../../Unicode/euc_jis_2004_to_utf8.map"
#include "../../Unicode/utf8_to_euc_jis_2004.map"
#include "../../Unicode/euc_jis_2004_to_utf8_combined.map"
#include "../../Unicode/utf8_to_euc_jis_2004_combined.map"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(euc_jis_2004_to_utf8);
PG_FUNCTION_INFO_V1(utf8_to_euc_jis_2004);

extern Datum euc_jis_2004_to_utf8(PG_FUNCTION_ARGS);
extern Datum utf8_to_euc_jis_2004(PG_FUNCTION_ARGS);

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
euc_jis_2004_to_utf8(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	Assert(PG_GETARG_INT32(0) == PG_EUC_JIS_2004);
	Assert(PG_GETARG_INT32(1) == PG_UTF8);
	Assert(len >= 0);

	LocalToUtf(src, dest, LUmapEUC_JIS_2004, LUmapEUC_JIS_2004_combined,
			   sizeof(LUmapEUC_JIS_2004) / sizeof(pg_local_to_utf),
	   sizeof(LUmapEUC_JIS_2004_combined) / sizeof(pg_local_to_utf_combined),
			   PG_EUC_JIS_2004, len);

	PG_RETURN_VOID();
}

Datum
utf8_to_euc_jis_2004(PG_FUNCTION_ARGS)
{
	unsigned char *src = (unsigned char *) PG_GETARG_CSTRING(2);
	unsigned char *dest = (unsigned char *) PG_GETARG_CSTRING(3);
	int			len = PG_GETARG_INT32(4);

	Assert(PG_GETARG_INT32(0) == PG_UTF8);
	Assert(PG_GETARG_INT32(1) == PG_EUC_JIS_2004);
	Assert(len >= 0);

	UtfToLocal(src, dest, ULmapEUC_JIS_2004, ULmapEUC_JIS_2004_combined,
			   sizeof(ULmapEUC_JIS_2004) / sizeof(pg_utf_to_local),
	   sizeof(ULmapEUC_JIS_2004_combined) / sizeof(pg_utf_to_local_combined),
			   PG_EUC_JIS_2004, len);

	PG_RETURN_VOID();
}
