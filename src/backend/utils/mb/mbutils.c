/*
 * This file contains public functions for conversion between
 * client encoding and server internal encoding.
 * (currently mule internal code (mic) is used)
 * Tatsuo Ishii
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/mb/mbutils.c,v 1.36.2.1 2002-11-26 02:37:13 ishii Exp $
 */
#include "postgres.h"

#include "access/xact.h"
#include "miscadmin.h"
#include "mb/pg_wchar.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/syscache.h"
#include "catalog/namespace.h"

/*
 * We handle for actual FE and BE encoding setting encoding-identificator
 * and encoding-name too. It prevent searching and conversion from encoding
 * to encoding name in getdatabaseencoding() and other routines.
 */
static pg_enc2name *ClientEncoding = &pg_enc2name_tbl[PG_SQL_ASCII];
static pg_enc2name *DatabaseEncoding = &pg_enc2name_tbl[PG_SQL_ASCII];

/*
 * Caches for conversion function info. Note that these values are
 * allocated in TopMemoryContext so that they survive across
 * transactions. See SetClientEncoding() for more details.
 */
static FmgrInfo *ToServerConvProc = NULL;
static FmgrInfo *ToClientConvProc = NULL;

/* Internal functions */
static unsigned char *perform_default_encoding_conversion(unsigned char *src,
									int len, bool is_client_to_server);
static int cliplen(const unsigned char *str, int len, int limit);


/*
 * Set the client encoding and save fmgrinfo for the converion
 * function if necessary. if encoding conversion between client/server
 * encoding is not supported, returns -1
*/
int
SetClientEncoding(int encoding, bool doit)
{
	int			current_server_encoding;
	Oid			to_server_proc,
				to_client_proc;
	FmgrInfo   *to_server = NULL;
	FmgrInfo   *to_client = NULL;
	MemoryContext oldcontext;

	current_server_encoding = GetDatabaseEncoding();

	if (!PG_VALID_FE_ENCODING(encoding))
		return (-1);

	if (current_server_encoding == encoding ||
	(current_server_encoding == PG_SQL_ASCII || encoding == PG_SQL_ASCII))
	{
		ClientEncoding = &pg_enc2name_tbl[encoding];
		return 0;
	}

	/*
	 * XXX We cannot use FindDefaultConversionProc() while in bootstrap or
	 * initprocessing mode since namespace functions will not work.
	 */
	if (IsTransactionState())
	{
		to_server_proc = FindDefaultConversionProc(encoding, current_server_encoding);
		to_client_proc = FindDefaultConversionProc(current_server_encoding, encoding);

		if (!OidIsValid(to_server_proc) || !OidIsValid(to_client_proc))
			return -1;

		/*
		 * load the fmgr info into TopMemoryContext so that it survives
		 * outside transaction.
		 */
		oldcontext = MemoryContextSwitchTo(TopMemoryContext);
		to_server = palloc(sizeof(FmgrInfo));
		to_client = palloc(sizeof(FmgrInfo));
		fmgr_info(to_server_proc, to_server);
		fmgr_info(to_client_proc, to_client);
		MemoryContextSwitchTo(oldcontext);
	}

	if (!doit)
		return 0;

	if (IsTransactionState())
	{
		ClientEncoding = &pg_enc2name_tbl[encoding];

		if (ToServerConvProc != NULL)
		{
			if (ToServerConvProc->fn_extra)
				pfree(ToServerConvProc->fn_extra);
			pfree(ToServerConvProc);
		}
		ToServerConvProc = to_server;

		if (ToClientConvProc != NULL)
		{
			if (ToClientConvProc->fn_extra)
				pfree(ToClientConvProc->fn_extra);
			pfree(ToClientConvProc);
		}
		ToClientConvProc = to_client;
	}
	return 0;
}

/*
 * returns the current client encoding */
int
pg_get_client_encoding(void)
{
	Assert(ClientEncoding);
	return (ClientEncoding->encoding);
}

/*
 * returns the current client encoding name
 */
const char *
pg_get_client_encoding_name(void)
{
	Assert(ClientEncoding);
	return (ClientEncoding->name);
}

/*
 * Apply encoding conversion on src and return it. The encoding
 * conversion function is chosen from the pg_conversion system catalog
 * marked as "default". If it is not found in the schema search path,
 * it's taken from pg_catalog schema. If it even is not in the schema,
 * warn and returns src. We cannot raise an error, since it will cause
 * an infinit loop in error message sending.
 *
 * In the case of no coversion, src is returned.
 *
 * XXX We assume that storage for converted result is 4-to-1 growth in
 * the worst case. The rate for currently supported encoding pares are within 3
 * (SJIS JIS X0201 half width kanna -> UTF-8 is the worst case).
 * So "4" should be enough for the moment.
 */
unsigned char *
pg_do_encoding_conversion(unsigned char *src, int len,
						  int src_encoding, int dest_encoding)
{
	unsigned char *result;
	Oid			proc;

	if (!IsTransactionState())
		return src;

	if (src_encoding == dest_encoding)
		return src;

	if (src_encoding == PG_SQL_ASCII || dest_encoding == PG_SQL_ASCII)
		return src;

	if (len <= 0)
		return src;

	proc = FindDefaultConversionProc(src_encoding, dest_encoding);
	if (!OidIsValid(proc))
	{
		elog(LOG, "default conversion proc for %s to %s not found",
			 pg_encoding_to_char(src_encoding), pg_encoding_to_char(dest_encoding));
		return src;
	}

	/*
	 * XXX we shoud avoid throwing errors in OidFuctionCall. Otherwise we
	 * are going into inifinite loop!  So we have to make sure that the
	 * function exists before calling OidFunctionCall.
	 */
	if (!SearchSysCacheExists(PROCOID,
							  ObjectIdGetDatum(proc),
							  0, 0, 0))
	{
		elog(LOG, "default conversion proc %u for %s to %s not found in pg_proc",
			 proc,
			 pg_encoding_to_char(src_encoding), pg_encoding_to_char(dest_encoding));
		return src;
	}

	result = palloc(len * 4 + 1);

	OidFunctionCall5(proc,
					 Int32GetDatum(src_encoding),
					 Int32GetDatum(dest_encoding),
					 CStringGetDatum(src),
					 CStringGetDatum(result),
					 Int32GetDatum(len));
	return result;
}

/*
 * Convert string using encoding_nanme. We assume that string's
 * encoding is same as DB encoding.
 *
 * TEXT convert(TEXT string, NAME encoding_name) */
Datum
pg_convert(PG_FUNCTION_ARGS)
{
	Datum		string = PG_GETARG_DATUM(0);
	Datum		dest_encoding_name = PG_GETARG_DATUM(1);
	Datum		src_encoding_name = DirectFunctionCall1(
						namein, CStringGetDatum(DatabaseEncoding->name));
	Datum		result;

	result = DirectFunctionCall3(
			 pg_convert2, string, src_encoding_name, dest_encoding_name);

	/* free memory allocated by namein */
	pfree((void *) src_encoding_name);

	PG_RETURN_TEXT_P(result);
}

/*
 * Convert string using encoding_nanme.
 *
 * TEXT convert2(TEXT string, NAME src_encoding_name, NAME dest_encoding_name)
 */
Datum
pg_convert2(PG_FUNCTION_ARGS)
{
	text	   *string = PG_GETARG_TEXT_P(0);
	char	   *src_encoding_name = NameStr(*PG_GETARG_NAME(1));
	int			src_encoding = pg_char_to_encoding(src_encoding_name);
	char	   *dest_encoding_name = NameStr(*PG_GETARG_NAME(2));
	int			dest_encoding = pg_char_to_encoding(dest_encoding_name);
	unsigned char *result;
	text	   *retval;
	unsigned char *str;
	int			len;

	if (src_encoding < 0)
		elog(ERROR, "Invalid source encoding name %s", src_encoding_name);
	if (dest_encoding < 0)
		elog(ERROR, "Invalid destination encoding name %s", dest_encoding_name);

	/* make sure that source string is null terminated */
	len = VARSIZE(string) - VARHDRSZ;
	str = palloc(len + 1);
	memcpy(str, VARDATA(string), len);
	*(str + len) = '\0';

	result = pg_do_encoding_conversion(str, len, src_encoding, dest_encoding);
	if (result == NULL)
		elog(ERROR, "Encoding conversion failed");

	/*
	 * build text data type structre. we cannot use textin() here, since
	 * textin assumes that input string encoding is same as database
	 * encoding.
	 */
	len = strlen(result) + VARHDRSZ;
	retval = palloc(len);
	VARATT_SIZEP(retval) = len;
	memcpy(VARDATA(retval), result, len - VARHDRSZ);

	if (result != str)
		pfree(result);
	pfree(str);

	/* free memory if allocated by the toaster */
	PG_FREE_IF_COPY(string, 0);

	PG_RETURN_TEXT_P(retval);
}

/*
 * convert client encoding to server encoding.
 */
unsigned char *
pg_client_to_server(unsigned char *s, int len)
{
	Assert(DatabaseEncoding);
	Assert(ClientEncoding);

	if (ClientEncoding->encoding == DatabaseEncoding->encoding)
		return s;

	return perform_default_encoding_conversion(s, len, true);
}

/*
 * convert server encoding to client encoding.
 */
unsigned char *
pg_server_to_client(unsigned char *s, int len)
{
	Assert(DatabaseEncoding);
	Assert(ClientEncoding);

	if (ClientEncoding->encoding == DatabaseEncoding->encoding)
		return s;

	return perform_default_encoding_conversion(s, len, false);
}

/*
 *	Perform default encoding conversion using cached FmgrInfo. Since
 *	this function does not access database at all, it is safe to call
 *	outside transactions. Explicit setting client encoding required
 *	before calling this function. Otherwise no conversion is
 *	performed.
*/
static unsigned char *
perform_default_encoding_conversion(unsigned char *src, int len, bool is_client_to_server)
{
	unsigned char *result;
	int			src_encoding,
				dest_encoding;
	FmgrInfo   *flinfo;

	if (len <= 0)
		return src;

	if (is_client_to_server)
	{
		src_encoding = ClientEncoding->encoding;
		dest_encoding = DatabaseEncoding->encoding;
		flinfo = ToServerConvProc;
	}
	else
	{
		src_encoding = DatabaseEncoding->encoding;
		dest_encoding = ClientEncoding->encoding;
		flinfo = ToClientConvProc;
	}

	if (flinfo == NULL)
		return src;

	if (src_encoding == dest_encoding)
		return src;

	if (src_encoding == PG_SQL_ASCII || dest_encoding == PG_SQL_ASCII)
		return src;

	result = palloc(len * 4 + 1);

	FunctionCall5(flinfo,
				  Int32GetDatum(src_encoding),
				  Int32GetDatum(dest_encoding),
				  CStringGetDatum(src),
				  CStringGetDatum(result),
				  Int32GetDatum(len));
	return result;
}

/* convert a multibyte string to a wchar */
int
pg_mb2wchar(const unsigned char *from, pg_wchar *to)
{
	return (*pg_wchar_table[DatabaseEncoding->encoding].mb2wchar_with_len) (from, to, strlen(from));
}

/* convert a multibyte string to a wchar with a limited length */
int
pg_mb2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	return (*pg_wchar_table[DatabaseEncoding->encoding].mb2wchar_with_len) (from, to, len);
}

/* returns the byte length of a multibyte word */
int
pg_mblen(const unsigned char *mbstr)
{
	return ((*pg_wchar_table[DatabaseEncoding->encoding].mblen) (mbstr));
}

/* returns the length (counted as a wchar) of a multibyte string */
int
pg_mbstrlen(const unsigned char *mbstr)
{
	int			len = 0;

	/* optimization for single byte encoding */
	if (pg_database_encoding_max_length() == 1)
		return strlen((char *) mbstr);

	while (*mbstr)
	{
		mbstr += pg_mblen(mbstr);
		len++;
	}
	return (len);
}

/* returns the length (counted as a wchar) of a multibyte string
   (not necessarily  NULL terminated) */
int
pg_mbstrlen_with_len(const unsigned char *mbstr, int limit)
{
	int			len = 0;
	int			l;

	while (limit > 0 && *mbstr)
	{
		l = pg_mblen(mbstr);
		limit -= l;
		mbstr += l;
		len++;
	}
	return (len);
}

/*
 * returns the byte length of a multibyte string
 * (not necessarily  NULL terminated)
 * that is no longer than limit.
 * this function does not break multibyte word boundary.
 */
int
pg_mbcliplen(const unsigned char *mbstr, int len, int limit)
{
	int			clen = 0;
	int			l;

	/* optimization for single byte encoding */
	if (pg_database_encoding_max_length() == 1)
		return cliplen(mbstr, len, limit);

	while (len > 0 && *mbstr)
	{
		l = pg_mblen(mbstr);
		if ((clen + l) > limit)
			break;
		clen += l;
		if (clen == limit)
			break;
		len -= l;
		mbstr += l;
	}
	return (clen);
}

/*
 * Similar to pg_mbcliplen except the limit parameter specifies the
 * character length, not the byte length.  */
int
pg_mbcharcliplen(const unsigned char *mbstr, int len, int limit)
{
	int			clen = 0;
	int			nch = 0;
	int			l;

	/* optimization for single byte encoding */
	if (pg_database_encoding_max_length() == 1)
		return cliplen(mbstr, len, limit);

	while (len > 0 && *mbstr)
	{
		l = pg_mblen(mbstr);
		nch++;
		if (nch > limit)
			break;
		clen += l;
		len -= l;
		mbstr += l;
	}
	return (clen);
}

void
SetDatabaseEncoding(int encoding)
{
	if (!PG_VALID_BE_ENCODING(encoding))
		elog(ERROR, "SetDatabaseEncoding(): invalid database encoding");

	DatabaseEncoding = &pg_enc2name_tbl[encoding];
	Assert(DatabaseEncoding->encoding == encoding);
}

void
SetDefaultClientEncoding()
{
	ClientEncoding = &pg_enc2name_tbl[GetDatabaseEncoding()];
}

int
GetDatabaseEncoding(void)
{
	Assert(DatabaseEncoding);
	return (DatabaseEncoding->encoding);
}

const char *
GetDatabaseEncodingName(void)
{
	Assert(DatabaseEncoding);
	return (DatabaseEncoding->name);
}

Datum
getdatabaseencoding(PG_FUNCTION_ARGS)
{
	Assert(DatabaseEncoding);
	return DirectFunctionCall1(namein, CStringGetDatum(DatabaseEncoding->name));
}

Datum
pg_client_encoding(PG_FUNCTION_ARGS)
{
	Assert(ClientEncoding);
	return DirectFunctionCall1(namein, CStringGetDatum(ClientEncoding->name));
}

static int
cliplen(const unsigned char *str, int len, int limit)
{
	int			l = 0;
	const unsigned char *s;

	for (s = str; *s; s++, l++)
	{
		if (l >= len || l >= limit)
			return l;
	}
	return (s - str);
}
