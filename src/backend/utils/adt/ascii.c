/* -----------------------------------------------------------------------
 * ascii.c
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/ascii.c,v 1.13.2.1 2003-04-02 21:08:07 tgl Exp $
 *
 *	 Portions Copyright (c) 1999-2000, PostgreSQL Global Development Group
 *
 *
 *	 TO_ASCII()
 *
 *	 The PostgreSQL routine for string to ascii conversion.
 *
 * -----------------------------------------------------------------------
 */

#include "postgres.h"
#include "utils/builtins.h"
#include "mb/pg_wchar.h"
#include "utils/ascii.h"

static text *encode_to_ascii(text *data, int enc);

/* ----------
 * to_ascii
 * ----------
 */
char *
pg_to_ascii(unsigned char *src, unsigned char *src_end, unsigned char *desc, int enc)
{
	unsigned char *x;
	unsigned char *ascii;
	int			range;

	/*
	 * relevant start for an encoding
	 */
#define RANGE_128	128
#define RANGE_160	160


	if (enc == PG_LATIN1)
	{
		/*
		 * ISO-8859-1 <range: 160 -- 255>
		 */
		ascii = "  cL Y  \"Ca  -R     'u .,      ?AAAAAAACEEEEIIII NOOOOOxOUUUUYTBaaaaaaaceeeeiiii nooooo/ouuuuyty";
		range = RANGE_160;
	}
	else if (enc == PG_LATIN2)
	{
		/*
		 * ISO-8859-2 <range: 160 -- 255>
		 */
		ascii = " A L LS \"SSTZ-ZZ a,l'ls ,sstz\"zzRAAAALCCCEEEEIIDDNNOOOOxRUUUUYTBraaaalccceeeeiiddnnoooo/ruuuuyt.";
		range = RANGE_160;
	}
	else if (enc == PG_WIN1250)
	{
		/*
		 * Window CP1250 <range: 128 -- 255>
		 */
		ascii = "  ' \"    %S<STZZ `'\"\".--  s>stzz   L A  \"CS  -RZ  ,l'u .,as L\"lzRAAAALCCCEEEEIIDDNNOOOOxRUUUUYTBraaaalccceeeeiiddnnoooo/ruuuuyt ";
		range = RANGE_128;
	}
	else
	{
		elog(ERROR, "pg_to_ascii(): unsupported encoding from %s",
			 pg_encoding_to_char(enc));
		return NULL;			/* keep compiler quiet */
	}

	/*
	 * Encode
	 */
	for (x = src; x < src_end; x++)
	{
		if (*x < 128)
			*desc++ = *x;
		else if (*x < range)
			*desc++ = ' ';		/* bogus 128 to 'range' */
		else
			*desc++ = ascii[*x - range];
	}

	return desc;
}

/* ----------
 * encode text
 * ----------
 */
static text *
encode_to_ascii(text *data, int enc)
{
	pg_to_ascii(
				(unsigned char *) VARDATA(data),		/* src */
				VARDATA(data) + VARSIZE(data),	/* src end */
				(unsigned char *) VARDATA(data),		/* desc */
				enc);			/* encoding */

	return data;
}

/* ----------
 * convert to ASCII - enc is set as 'name' arg.
 * ----------
 */
Datum
to_ascii_encname(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P
		(
		 encode_to_ascii
		 (
		  PG_GETARG_TEXT_P_COPY(0),
		  pg_char_to_encoding(NameStr(*PG_GETARG_NAME(1)))
		  )
		);
}

/* ----------
 * convert to ASCII - enc is set as int4
 * ----------
 */
Datum
to_ascii_enc(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P
		(
		 encode_to_ascii
		 (
		  PG_GETARG_TEXT_P_COPY(0),
		  PG_GETARG_INT32(1)
		  )
		);
}

/* ----------
 * convert to ASCII - current enc is DatabaseEncoding
 * ----------
 */
Datum
to_ascii_default(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P
		(
		 encode_to_ascii
		 (
		  PG_GETARG_TEXT_P_COPY(0),
		  GetDatabaseEncoding()
		  )
		);
}
