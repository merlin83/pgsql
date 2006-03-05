/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright (c) 2000-2006, PostgreSQL Global Development Group
 *
 * $PostgreSQL: pgsql/src/bin/psql/mbprint.c,v 1.21 2006/03/05 15:58:51 momjian Exp $
 */

#include "postgres_fe.h"
#ifndef PGSCRIPTS
#include "settings.h"
#endif
#include "mbprint.h"

#include "mb/pg_wchar.h"

static pg_wchar
utf2ucs(const unsigned char *c)
{
	/*
	 * one char version of pg_utf2wchar_with_len. no control here, c must
	 * point to a large enough string
	 */
	if ((*c & 0x80) == 0)
		return (pg_wchar) c[0];
	else if ((*c & 0xe0) == 0xc0)
	{
		return (pg_wchar) (((c[0] & 0x1f) << 6) |
						   (c[1] & 0x3f));
	}
	else if ((*c & 0xf0) == 0xe0)
	{
		return (pg_wchar) (((c[0] & 0x0f) << 12) |
						   ((c[1] & 0x3f) << 6) |
						   (c[2] & 0x3f));
	}
	else if ((*c & 0xf0) == 0xf0)
	{
		return (pg_wchar) (((c[0] & 0x07) << 18) |
						   ((c[1] & 0x3f) << 12) |
						   ((c[2] & 0x3f) << 6) |
						   (c[3] & 0x3f));
	}
	else
	{
		/* that is an invalid code on purpose */
		return 0xffffffff;
	}
}


/*
 * Unicode 3.1 compliant validation : for each category, it checks the
 * combination of each byte to make sure it maps to a valid range. It also
 * returns -1 for the following UCS values: ucs > 0x10ffff ucs & 0xfffe =
 * 0xfffe 0xfdd0 < ucs < 0xfdef ucs & 0xdb00 = 0xd800 (surrogates)
 */
static int
utf_charcheck(const unsigned char *c)
{
	if ((*c & 0x80) == 0)
		return 1;
	else if ((*c & 0xe0) == 0xc0)
	{
		/* two-byte char */
		if (((c[1] & 0xc0) == 0x80) && ((c[0] & 0x1f) > 0x01))
			return 2;
		return -1;
	}
	else if ((*c & 0xf0) == 0xe0)
	{
		/* three-byte char */
		if (((c[1] & 0xc0) == 0x80) &&
			(((c[0] & 0x0f) != 0x00) || ((c[1] & 0x20) == 0x20)) &&
			((c[2] & 0xc0) == 0x80))
		{
			int			z = c[0] & 0x0f;
			int			yx = ((c[1] & 0x3f) << 6) | (c[0] & 0x3f);
			int			lx = yx & 0x7f;

			/* check 0xfffe/0xffff, 0xfdd0..0xfedf range, surrogates */
			if (((z == 0x0f) &&
				 (((yx & 0xffe) == 0xffe) ||
			   (((yx & 0xf80) == 0xd80) && (lx >= 0x30) && (lx <= 0x4f)))) ||
				((z == 0x0d) && ((yx & 0xb00) == 0x800)))
				return -1;
			return 3;
		}
		return -1;
	}
	else if ((*c & 0xf8) == 0xf0)
	{
		int			u = ((c[0] & 0x07) << 2) | ((c[1] & 0x30) >> 4);

		/* four-byte char */
		if (((c[1] & 0xc0) == 0x80) &&
			(u > 0x00) && (u <= 0x10) &&
			((c[2] & 0xc0) == 0x80) && ((c[3] & 0xc0) == 0x80))
		{
			/* test for 0xzzzzfffe/0xzzzzfffff */
			if (((c[1] & 0x0f) == 0x0f) && ((c[2] & 0x3f) == 0x3f) &&
				((c[3] & 0x3e) == 0x3e))
				return -1;
			return 4;
		}
		return -1;
	}
	return -1;
}


static void
mb_utf_validate(unsigned char *pwcs)
{
	unsigned char *p = pwcs;

	while (*pwcs)
	{
		int			len;

		if ((len = utf_charcheck(pwcs)) > 0)
		{
			if (p != pwcs)
			{
				int			i;

				for (i = 0; i < len; i++)
					*p++ = *pwcs++;
			}
			else
			{
				pwcs += len;
				p += len;
			}
		}
		else
			/* we skip the char */
			pwcs++;
	}
	if (p != pwcs)
		*p = '\0';
}

/*
 * public functions : wcswidth and mbvalidate
 */

/*
 * pg_wcswidth is the dumb width function. It assumes that everything will
 * only appear on one line. OTOH it is easier to use if this applies to you.
 */
int
pg_wcswidth(const unsigned char *pwcs, size_t len, int encoding)
{
	int width = 0;

	while (len > 0)
	{
		int chlen, chwidth;

		chlen = PQmblen((const char*) pwcs, encoding);
		if (chlen > len)
			break;     /* Invalid string */
			
		chwidth = PQdsplen((const char *) pwcs, encoding);
		
		if (chwidth > 0)
			width += chwidth;
		pwcs += chlen;
	}
	return width;
}

/*
 * pg_wcssize takes the given string in the given encoding and returns three
 * values:
 *    result_width: Width in display character of longest line in string
 *    result_hieght: Number of lines in display output
 *    result_format_size: Number of bytes required to store formatted representation of string
 */
int
pg_wcssize(unsigned char *pwcs, size_t len, int encoding, int *result_width,
			int *result_height, int *result_format_size)
{
	int	w,
		chlen = 0,
		linewidth = 0;
	int width = 0;
	int height = 1;
	int format_size = 0;

	for (; *pwcs && len > 0; pwcs += chlen)
	{
		chlen = PQmblen((char *) pwcs, encoding);
		if (len < (size_t)chlen)
			break;
		w = PQdsplen((char *) pwcs, encoding);

		if (chlen == 1)   /* ASCII char */
		{
			if (*pwcs == '\n') /* Newline */
			{
				if (linewidth > width)
					width = linewidth;
				linewidth = 0;
				height += 1;
				format_size += 1;  /* For NUL char */
			}
			else if (*pwcs == '\r')   /* Linefeed */
			{
				linewidth += 2;
				format_size += 2;
			}
			else if (w <= 0)  /* Other control char */
			{
				linewidth += 4;
				format_size += 4;
			}
			else  /* Output itself */
			{
				linewidth++;
				format_size += 1;
			}
		}
		else if (w <= 0)  /* Non-ascii control char */
		{
			linewidth += 6;   /* \u0000 */
			format_size += 6;
		}
		else  /* All other chars */
		{
			linewidth += w;
			format_size += chlen;
		}
		len -= chlen;
	}
	if (linewidth > width)
		width = linewidth;
	format_size += 1;
	
	/* Set results */
	if (result_width)
		*result_width = width;
	if (result_height)
		*result_height = height;
	if (result_format_size)
		*result_format_size = format_size;
		
	return width;
}

void
pg_wcsformat(unsigned char *pwcs, size_t len, int encoding,
			 struct lineptr *lines, int count)
{
	int			w,
				chlen = 0;
	int linewidth = 0;
	unsigned char *ptr = lines->ptr;   /* Pointer to data area */

	for (; *pwcs && len > 0; pwcs += chlen)
	{
		chlen = PQmblen((char *) pwcs,encoding);
		if (len < (size_t)chlen)
			break;
		w = PQdsplen((char *) pwcs,encoding);

		if (chlen == 1)   /* single byte char char */
		{
			if (*pwcs == '\n') /* Newline */
			{
				*ptr++ = 0;   /* NULL char */
				lines->width = linewidth;
				linewidth = 0;
				lines++;
				count--;
				if (count == 0)
					exit(1);   /* Screwup */	
					
				lines->ptr = ptr;
			}
			else if (*pwcs == '\r')   /* Linefeed */
			{
				strcpy((char *) ptr, "\\r");
				linewidth += 2;
				ptr += 2;
			}
			else if (w <= 0)  /* Other control char */
			{
				sprintf((char *) ptr, "\\x%02X", *pwcs);
				linewidth += 4;
				ptr += 4;
			}
			else  /* Output itself */
			{
				linewidth++;
				*ptr++ = *pwcs;
			}
		}
		else if (w <= 0)  /* Non-ascii control char */
		{
			if (encoding == PG_UTF8)
				sprintf((char *) ptr, "\\u%04X", utf2ucs(pwcs));
			else
				/* This case cannot happen in the current
				 * code because only UTF-8 signals multibyte
				 * control characters. But we may need to
				 * support it at some stage */
				sprintf((char *) ptr, "\\u????");
				
			ptr += 6;
			linewidth += 6;
		}
		else  /* All other chars */
		{
			int i;
			for (i=0; i < chlen; i++)
				*ptr++ = pwcs[i];
			linewidth += w;
		}
		len -= chlen;
	}
	*ptr++ = 0;
	lines->width = linewidth;
	lines++;
	count--;
	if (count > 0)
		lines->ptr = NULL;
	return;
}

unsigned char *
mbvalidate(unsigned char *pwcs, int encoding)
{
	if (encoding == PG_UTF8)
		mb_utf_validate((unsigned char *) pwcs);
	else
	{
		/*
		 * other encodings needing validation should add their own routines
		 * here
		 */
	}

	return pwcs;
}
