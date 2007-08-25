/*-------------------------------------------------------------------------
 *
 * ts_locale.c
 *		locale compatiblility layer for tsearch
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/tsearch/ts_locale.c,v 1.2 2007/08/25 00:03:59 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "tsearch/ts_locale.h"
#include "tsearch/ts_public.h"

#ifdef TS_USE_WIDE

#ifdef WIN32

size_t
wchar2char(char *to, const wchar_t *from, size_t len)
{
	if (len == 0)
		return 0;

	if (GetDatabaseEncoding() == PG_UTF8)
	{
		int			r;

		r = WideCharToMultiByte(CP_UTF8, 0, from, -1, to, len,
								NULL, NULL);

		if (r == 0)
			ereport(ERROR,
					(errcode(ERRCODE_CHARACTER_NOT_IN_REPERTOIRE),
					 errmsg("UTF-16 to UTF-8 translation failed: %lu",
							GetLastError())));
		Assert(r <= len);

		return r;
	}

	return wcstombs(to, from, len);
}
#endif   /* WIN32 */

size_t
char2wchar(wchar_t *to, const char *from, size_t len)
{
	if (len == 0)
		return 0;

#ifdef WIN32
	if (GetDatabaseEncoding() == PG_UTF8)
	{
		int			r;

		r = MultiByteToWideChar(CP_UTF8, 0, from, len, to, len);

		if (!r)
		{
			pg_verifymbstr(from, len, false);
			ereport(ERROR,
					(errcode(ERRCODE_CHARACTER_NOT_IN_REPERTOIRE),
					 errmsg("invalid multibyte character for locale"),
					 errhint("The server's LC_CTYPE locale is probably incompatible with the database encoding.")));
		}

		Assert(r <= len);

		return r;
	}
	else
#endif   /* WIN32 */
	if (lc_ctype_is_c())
	{
		/*
		 * pg_mb2wchar_with_len always adds trailing '\0', so 'to' should be
		 * allocated with sufficient space
		 */
		return pg_mb2wchar_with_len(from, (pg_wchar *) to, len);
	}
	else
	{
		/*
		 * mbstowcs require ending '\0'
		 */
		char	   *str = pnstrdup(from, len);
		size_t		tolen;

		tolen = mbstowcs(to, str, len);
		pfree(str);

		return tolen;
	}
}

int
_t_isalpha(const char *ptr)
{
	wchar_t		character[2];

	if (lc_ctype_is_c())
		return isalpha(TOUCHAR(ptr));

	char2wchar(character, ptr, 1);

	return iswalpha((wint_t) *character);
}

int
_t_isprint(const char *ptr)
{
	wchar_t		character[2];

	if (lc_ctype_is_c())
		return isprint(TOUCHAR(ptr));

	char2wchar(character, ptr, 1);

	return iswprint((wint_t) *character);
}
#endif   /* TS_USE_WIDE */


/*
 * Read the next line from a tsearch data file (expected to be in UTF-8), and
 * convert it to database encoding if needed. The returned string is palloc'd.
 * NULL return means EOF.
 */
char *
t_readline(FILE *fp)
{
	int len;
	char *recoded;
	char buf[4096];		/* lines must not be longer than this */
	
	if (fgets(buf, sizeof(buf), fp) == NULL)
		return NULL;

	len = strlen(buf);

	/* Make sure the input is valid UTF-8 */
	(void) pg_verify_mbstr(PG_UTF8, buf, len, false);

	/* And convert */
	recoded = (char *) pg_do_encoding_conversion((unsigned char *) buf,
												 len,
												 PG_UTF8,
												 GetDatabaseEncoding());

	if (recoded == NULL)		/* should not happen */
		elog(ERROR, "encoding conversion failed");

	if (recoded == buf)
	{
		/*
		 * conversion didn't pstrdup, so we must.
		 * We can use the length of the original string, because
		 * no conversion was done.
		 */
		recoded = pnstrdup(recoded, len);
	}

	return recoded;
}

char *
lowerstr(char *str)
{
	return lowerstr_with_len(str, strlen(str));
}

/*
 * Returned string is palloc'd
 */
char *
lowerstr_with_len(char *str, int len)
{
	char	   *ptr = str;
	char	   *out;

	if (len == 0)
		return pstrdup("");

#ifdef TS_USE_WIDE

	/*
	 * Use wide char code only when max encoding length > 1 and ctype != C.
	 * Some operating systems fail with multi-byte encodings and a C locale.
	 * Also, for a C locale there is no need to process as multibyte. From
	 * backend/utils/adt/oracle_compat.c Teodor
	 */
	if (pg_database_encoding_max_length() > 1 && !lc_ctype_is_c())
	{
		wchar_t    *wstr,
				   *wptr;
		int			wlen;

		/*
		 * alloc number of wchar_t for worst case, len contains number of
		 * bytes <= number of characters and alloc 1 wchar_t for 0, because
		 * wchar2char(wcstombs in really) wants zero-terminated string
		 */
		wptr = wstr = (wchar_t *) palloc(sizeof(wchar_t) * (len + 1));

		/*
		 * str SHOULD be cstring, so wlen contains number of converted
		 * character
		 */
		wlen = char2wchar(wstr, str, len);
		if (wlen < 0)
			ereport(ERROR,
					(errcode(ERRCODE_CHARACTER_NOT_IN_REPERTOIRE),
			  errmsg("translation failed from server encoding to wchar_t")));

		Assert(wlen <= len);
		wstr[wlen] = 0;

		while (*wptr)
		{
			*wptr = towlower((wint_t) *wptr);
			wptr++;
		}

		/*
		 * Alloc result string for worst case + '\0'
		 */
		len = sizeof(char) * pg_database_encoding_max_length() *(wlen + 1);
		out = (char *) palloc(len);

		/*
		 * wlen now is number of bytes which is always >= number of characters
		 */
		wlen = wchar2char(out, wstr, len);
		pfree(wstr);

		if (wlen < 0)
			ereport(ERROR,
					(errcode(ERRCODE_CHARACTER_NOT_IN_REPERTOIRE),
					 errmsg("translation failed from wchar_t to server encoding %d", errno)));
		Assert(wlen <= len);
		out[wlen] = '\0';
	}
	else
#endif
	{
		char	   *outptr;

		outptr = out = (char *) palloc(sizeof(char) * (len + 1));
		while (*ptr && ptr - str < len)
		{
			*outptr++ = tolower(*(unsigned char *) ptr);
			ptr++;
		}
		*outptr = '\0';
	}

	return out;
}
