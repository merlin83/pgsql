/*
 * misc conversion functions between pg_wchar and other encodings.
 * Tatsuo Ishii
 * $Id: utils.c,v 1.4 1998-07-18 18:34:08 momjian Exp $
 */
#include <regex/pg_wchar.h>

/*
 * conversion to pg_wchar is done by "table driven."
 * to add an encoding support, define mb2wchar_with_len(), mblen()
 * for the particular encoding. Note that if the encoding is only
 * supported in the client, you don't need to define 
 * mb2wchar_with_len() function (SJIS is the case).
 */
typedef struct {
  void	(*mb2wchar_with_len)();	/* convert a multi-byte string to a wchar */
  int	(*mblen)();		/* returns the length of a multi-byte word */
} pg_wchar_tbl;

static void pg_euc2wchar_with_len
(const unsigned char *from, pg_wchar *to, int len)
{
  while (*from && len > 0) {
    if (*from == SS2) {
      from++;
      len--;
      *to = 0xff & *from++;
      len--;
    } else if (*from == SS3) {
      from++;
      *to = *from++ << 8;
      *to |= 0x3f & *from++;
      len -= 3;
    } else if (*from & 0x80) {
      *to = *from++ << 8;
      *to |= *from++;
      len -= 2;
    } else {
      *to = *from++;
      len--;
    }
    to++;
  }
  *to = 0;
}

static int pg_euc_mblen(const unsigned char *s)
{
  int len;

  if (*s == SS2) {
    len = 2;
  } else if (*s == SS3) {
    len = 3;
  } else if (*s & 0x80) {
    len = 2;
  } else {
    len = 1;
  }
  return(len);
}

/*
 * EUC_JP
 */
static void pg_eucjp2wchar_with_len
(const unsigned char *from, pg_wchar *to, int len)
{
  pg_euc2wchar_with_len(from,to,len);
}

static int pg_eucjp_mblen(const unsigned char *s)
{
  return(pg_euc_mblen(s));
}

/*
 * EUC_KR
 */
static void pg_euckr2wchar_with_len
(const unsigned char *from, pg_wchar *to, int len)
{
  pg_euc2wchar_with_len(from,to,len);
}

static int pg_euckr_mblen(const unsigned char *s)
{
  return(pg_euc_mblen(s));
}

/*
 * EUC_CN
 */
static void pg_euccn2wchar_with_len
(const unsigned char *from, pg_wchar *to, int len)
{
  while (*from && len > 0) {
    if (*from == SS2) {
      from++;
      len--;
      *to = 0x3f00 & (*from++ << 8);
      *to = *from++;
      len -= 2;
    } else if (*from == SS3) {
      from++;
      *to = *from++ << 8;
      *to |= 0x3f & *from++;
      len -= 3;
    } else if (*from & 0x80) {
      *to = *from++ << 8;
      *to |= *from++;
      len -= 2;
    } else {
      *to = *from++;
      len--;
    }
    to++;
  }
  *to = 0;
}

static int pg_euccn_mblen(const unsigned char *s)
{
  int len;

  if (*s == SS2) {
    len = 3;
  } else if (*s == SS3) {
    len = 3;
  } else if (*s & 0x80) {
    len = 2;
  } else {
    len = 1;
  }
  return(len);
}

/*
 * EUC_TW
 */
static void pg_euctw2wchar_with_len
(const unsigned char *from, pg_wchar *to, int len)
{
  while (*from && len > 0) {
    if (*from == SS2) {
      from++;
      len--;
      *to = *from++ << 16;
      *to |= *from++ << 8;
      *to |= *from++;
      len -= 3;
    } else if (*from == SS3) {
      from++;
      *to = *from++ << 8;
      *to |= 0x3f & *from++;
      len -= 3;
    } else if (*from & 0x80) {
      *to = *from++ << 8;
      *to |= *from++;
      len -= 2;
    } else {
      *to = *from++;
      len--;
    }
    to++;
  }
  *to = 0;
}

static int pg_euctw_mblen(const unsigned char *s)
{
  int len;

  if (*s == SS2) {
    len = 4;
  } else if (*s == SS3) {
    len = 3;
  } else if (*s & 0x80) {
    len = 2;
  } else {
    len = 1;
  }
  return(len);
}

/*
 * convert UTF-8 to pg_wchar (UCS-2)
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static void pg_utf2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
  unsigned char c1,c2,c3;
  while (*from && len > 0) {
    if ((*from & 0x80) == 0) {
      *to = *from++;
      len--;
    } else if ((*from & 0xe0) == 0xc0) {
      c1 = *from++ & 0x1f;
      c2 = *from++ & 0x3f;
      len -= 2;
      *to = c1 << 6;
      *to |= c2;
    } else if ((*from & 0xe0) == 0xe0) {
      c1 = *from++ & 0x0f;
      c2 = *from++ & 0x3f;
      c3 = *from++ & 0x3f;
      len -= 3;
      *to = c1 << 12;
      *to |= c2 << 6;
      *to |= c3;
    }
    to++;
  }
  *to = 0;
}

static int pg_utf_mblen(const unsigned char *s)
{
  int len = 1;

  if ((*s & 0x80) == 0) {
    len = 1;
  } else if ((*s & 0xe0) == 0xc0) {
    len = 2;
  } else if ((*s & 0xe0) == 0xe0) {
    len = 3;
  }
  return(len);
}

/*
 * convert mule internal code to pg_wchar
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static void pg_mule2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
  while (*from && len > 0) {
    if (IS_LC1(*from)) {
      *to = *from++ << 16;
      *to |= *from++;
      len -= 2;
    } else if (IS_LCPRV1(*from)) {
      from++;
      *to = *from++ << 16;
      *to |= *from++;
      len -= 3;
    } else if (IS_LC2(*from)) {
      *to = *from++ << 16;
      *to |= *from++ << 8;
      *to |= *from++;
      len -= 3;
    } else if (IS_LCPRV2(*from)) {
      from++;
      *to = *from++ << 16;
      *to |= *from++ << 8;
      *to |= *from++;
      len -= 4;
    } else {	/* assume ASCII */
      *to = (unsigned char)*from++;
      len--;
    }
    to++;
  }
  *to = 0;
}

static int pg_mule_mblen(const unsigned char *s)
{
  int len;

  if (IS_LC1(*s)) {
    len = 2;
  } else if (IS_LCPRV1(*s)) {
    len = 3;
  } else if (IS_LC2(*s)) {
    len = 3;
  } else if (IS_LCPRV2(*s)) {
    len = 4;
  } else {	/* assume ASCII */
    len = 1;
  }
  return(len);
}

/*
 * ISO8859-1
 */
static void pg_latin12wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
  while (*from && len-- > 0) {
    *to++ = *from++;
  }
  *to = 0;
}

static int pg_latin1_mblen(const unsigned char *s)
{
  return(1);
}

/*
 * SJIS
 */
static int pg_sjis_mblen(const unsigned char *s)
{
  int len;

  if (*s >= 0xa1 && *s <= 0xdf) {	/* 1 byte kana? */
    len = 1;
  } else if (*s > 0x7f) {	/* kanji? */
    len = 2;
  } else {	/* should be ASCII */
    len = 1;
  }
  return(len);
}

static pg_wchar_tbl pg_wchar_table[] = {
  {pg_eucjp2wchar_with_len, pg_eucjp_mblen},
  {pg_euccn2wchar_with_len, pg_euccn_mblen},
  {pg_euckr2wchar_with_len, pg_euckr_mblen},
  {pg_euctw2wchar_with_len, pg_euctw_mblen},
  {pg_utf2wchar_with_len, pg_utf_mblen},
  {pg_mule2wchar_with_len, pg_mule_mblen},
  {pg_latin12wchar_with_len, pg_latin1_mblen},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, 0},
  {0, pg_sjis_mblen}
};

/*
 *########################################################################
 *
 * Public functions
 *
 *########################################################################
 */

/* convert a multi-byte string to a wchar */
void pg_mb2wchar(const unsigned char *from, pg_wchar *to)
{
  (*pg_wchar_table[MULTIBYTE].mb2wchar_with_len)(from,to,strlen(from));
}

/* convert a multi-byte string to a wchar with a limited length */
void pg_mb2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
  (*pg_wchar_table[MULTIBYTE].mb2wchar_with_len)(from,to,len);
}

/* returns the byte length of a multi-byte word */
int pg_mblen(const unsigned char *mbstr)
{
  return((*pg_wchar_table[MULTIBYTE].mblen)(mbstr));
}

/* returns the byte length of a multi-byte word for an encoding */
int pg_encoding_mblen(int encoding, const unsigned char *mbstr)
{
  return((*pg_wchar_table[encoding].mblen)(mbstr));
}

/* returns the byte length of a word for mule internal code */
int pg_mic_mblen(const unsigned char *mbstr)
{
  return(pg_mule_mblen(mbstr));
}

/* returns the length (counted as a wchar) of a multi-byte string */
int pg_mbstrlen(const unsigned char *mbstr)
{
  int len = 0;
  while (*mbstr) {
    mbstr += pg_mblen(mbstr);
    len++;
  }
  return(len);
}

/* returns the length (counted as a wchar) of a multi-byte string 
   (not necessarily  NULL terminated) */
int pg_mbstrlen_with_len(const unsigned char *mbstr, int limit)
{
  int len = 0;
  int l;
  while (*mbstr && limit > 0) {
    l = pg_mblen(mbstr);
    limit -= l;
    mbstr += l;
    len++;
  }
  return(len);
}
