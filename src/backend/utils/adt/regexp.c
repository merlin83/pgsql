/*-------------------------------------------------------------------------
 *
 * regexp.c--
 *    regular expression handling code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/regexp.c,v 1.3 1996-11-06 06:49:58 scrappy Exp $
 *
 *      Alistair Crooks added the code for the regex caching
 *	agc - cached the regular expressions used - there's a good chance
 *	that we'll get a hit, so this saves a compile step for every
 *	attempted match. I haven't actually measured the speed improvement,
 *	but it `looks' a lot quicker visually when watching regression
 *	test output.
 *
 *	agc - incorporated Keith Bostic's Berkeley regex code into
 *	the tree for all ports. To distinguish this regex code from any that
 *	is existent on a platform, I've prepended the string "pg95_" to
 *	the functions regcomp, regerror, regexec and regfree.
 *	Fixed a bug that was originally a typo by me, where `i' was used
 *	instead of `oldest' when compiling regular expressions - benign
 *	results mostly, although occasionally it bit you...
 *
 *
 *-------------------------------------------------------------------------
 */
#include <string.h>
#include "postgres.h"		/* postgres system include file */
#include "utils/palloc.h"
#include "utils/builtins.h"	/* where the function declarations go */

#if defined(DISABLE_XOPEN_NLS)
#undef _XOPEN_SOURCE
#endif /* DISABLE_XOPEN_NLS */

#ifndef WIN32

#include <sys/types.h>
#include <regex.h>

#endif /* WIN32 why is this necessary? */

/* this is the number of cached regular expressions held. */
#ifndef MAX_CACHED_RES
#define MAX_CACHED_RES	32
#endif

/* this structure describes a cached regular expression */
struct cached_re_str {
        struct varlena  *cre_text;      /* pattern as a text* */
	char		*cre_s;		/* pattern as null-terminated string */
	int             cre_type;       /* compiled-type: extended,icase etc */
	regex_t		cre_re;		/* the compiled regular expression */
	unsigned long	cre_lru;	/* lru tag */
};

static int			rec = 0;	        /* # of cached re's */
static struct cached_re_str	rev[MAX_CACHED_RES];	/* cached re's */
static unsigned long		lru;			/* system lru tag */

/* attempt to compile `re' as an re, then match it against text */
/* cflags - flag to regcomp indicates case sensitivity */
static int
RE_compile_and_execute(struct varlena *text_re, char *text, int cflags)
{
	int	oldest;
	int	n;
	int	i;
	char    *re;
	int regcomp_result;

  	re = textout(text_re);
  	/* find a previously compiled regular expression */
  	for (i = 0 ; i < rec ; i++) {
	    if (rev[i].cre_s) {
		if (strcmp(rev[i].cre_s, re) == 0) {
		    if (rev[i].cre_type == cflags) {
			rev[i].cre_lru = ++lru;
			pfree(re);
			return(pg95_regexec(&rev[i].cre_re,
					    text, 0,
					    (regmatch_t *) NULL, 0) == 0);
		    }
		}
	    }
  	}
 


	/* we didn't find it - make room in the cache for it */
	if (rec == MAX_CACHED_RES) {
		/* cache is full - find the oldest entry */
		for (oldest = 0, i = 1 ; i < rec ; i++) {
			if (rev[i].cre_lru < rev[oldest].cre_lru) {
				oldest = i;
			}
		}
	} else {
		oldest = rec++;
	}

	/* if there was an old re, then de-allocate the space it used */
	if (rev[oldest].cre_s != (char *) NULL) {
		for (lru = i = 0 ; i < rec ; i++) {
			rev[i].cre_lru =
				(rev[i].cre_lru - rev[oldest].cre_lru) / 2;
			if (rev[i].cre_lru > lru) {
				lru = rev[i].cre_lru;
			}
 		}
		pg95_regfree(&rev[oldest].cre_re);
		/* use malloc/free for the cre_s field because the storage
		   has to persist across transactions */
		free(rev[oldest].cre_s); 
	}

	/* compile the re */
	regcomp_result = pg95_regcomp(&rev[oldest].cre_re, re, cflags);
	if ( regcomp_result == 0) {
		n = strlen(re);
		/* use malloc/free for the cre_s field because the storage
		   has to persist across transactions */
		rev[oldest].cre_s = (char *) malloc(n + 1);
		(void) memmove(rev[oldest].cre_s, re, n);
		rev[oldest].cre_s[n] = 0;
	        rev[oldest].cre_text = text_re;
		rev[oldest].cre_lru = ++lru;
 		rev[oldest].cre_type = cflags;
		pfree(re);
 		/* agc - fixed an old typo here */
 		return(pg95_regexec(&rev[oldest].cre_re, text, 0,
  						(regmatch_t *) NULL, 0) == 0);
	} else {
	    char errMsg[1000];
	    /* re didn't compile */
	    rev[oldest].cre_s = (char *) NULL;
	    pg95_regerror(regcomp_result, &rev[oldest].cre_re, errMsg,
			  sizeof(errMsg));
	    elog(WARN,"regcomp failed with error %s",errMsg);
	}

	/* not reached */
	return(0);
}



/*
 *  interface routines called by the function manager
 */

/*
   fixedlen_regexeq:

   a generic fixed length regexp routine
         s      - the string to match against (not necessarily null-terminated)
	 p      - the pattern
	 charlen   - the length of the string
*/
static bool 
fixedlen_regexeq(char *s, struct varlena* p, int charlen, int cflags)
{
    char *sterm;
    int result;
    
    if (!s || !p)
	return FALSE;
    
    /* be sure sterm is null-terminated */
    sterm = (char *) palloc(charlen + 1);
    memset(sterm, 0, charlen + 1);
    strncpy(sterm, s, charlen);
    
    result = RE_compile_and_execute(p, sterm, cflags);

    pfree(sterm);
    
    return ((bool) result);

}


/*
 *  routines that use the regexp stuff
 */
bool 
char2regexeq(uint16 arg1, struct varlena *p)
{
    char *s = (char *) &arg1;
    return (fixedlen_regexeq(s, p, 2, REG_EXTENDED));
}

bool 
char2regexne(uint16 arg1, struct varlena *p)
{
    return (!char2regexeq(arg1, p));
}

bool 
char4regexeq(uint32 arg1, struct varlena *p)
{
    char *s = (char *) &arg1;
    return (fixedlen_regexeq(s, p, 4, REG_EXTENDED));
}

bool 
char4regexne(uint32 arg1, struct varlena *p)
{
    return (!char4regexeq(arg1, p));
}

bool 
char8regexeq(char *s, struct varlena *p)
{
    return (fixedlen_regexeq(s, p, 8, REG_EXTENDED));
}

bool 
char8regexne(char *s, struct varlena *p)
{
    return (!char8regexeq(s, p));
}

bool 
char16regexeq(char *s, struct varlena *p)
{
    return (fixedlen_regexeq(s, p, 16, REG_EXTENDED));
}

bool 
char16regexne(char *s, struct varlena *p)
{
    return (!char16regexeq(s, p));
}

bool 
nameregexeq(NameData *n, struct varlena *p)
{
    if (!n) return FALSE;
    return (fixedlen_regexeq(n->data, p, NAMEDATALEN, REG_EXTENDED));
}
bool 
nameregexne(NameData *s, struct varlena *p)
{
    return (!nameregexeq(s, p));
}

bool 
textregexeq(struct varlena *s, struct varlena *p)
{
    if (!s) return (FALSE);
    return (fixedlen_regexeq(VARDATA(s), p, VARSIZE(s) - VARHDRSZ, REG_EXTENDED));
}

bool 
textregexne(struct varlena *s, struct varlena *p)
{
    return (!textregexeq(s, p));
}


/*
*  routines that use the regexp stuff, but ignore the case.
 *  for this, we use the REG_ICASE flag to pg95_regcomp
 */
bool 
char2icregexeq(uint16 arg1, struct varlena *p)
{
    char *s = (char *) &arg1;
    return (fixedlen_regexeq(s, p, 2, REG_ICASE | REG_EXTENDED));
}

  
bool 
char2icregexne(uint16 arg1, struct varlena *p)
{
    return (!char2icregexeq(arg1, p));
}

bool 
char4icregexeq(uint32 arg1, struct varlena *p)
{
    char *s = (char *) &arg1;
    return (fixedlen_regexeq(s, p, 4, REG_ICASE | REG_EXTENDED ));
}

bool 
char4icregexne(uint32 arg1, struct varlena *p)
{
    return (!char4icregexeq(arg1, p));
}
 
bool 
char8icregexeq(char *s, struct varlena *p)
{
    return (fixedlen_regexeq(s, p, 8, REG_ICASE | REG_EXTENDED));
}

bool 
char8icregexne(char *s, struct varlena *p)
{
    return (!char8icregexeq(s, p));
}

bool 
char16icregexeq(char *s, struct varlena *p)
{
    return (fixedlen_regexeq(s, p, 16, REG_ICASE | REG_EXTENDED));
}

bool 
char16icregexne(char *s, struct varlena *p)
{
    return (!char16icregexeq(s, p));
}

bool 
texticregexeq(struct varlena *s, struct varlena *p)
{
    if (!s) return FALSE;
    return (fixedlen_regexeq(VARDATA(s), p, VARSIZE(s) - VARHDRSZ, 
			     REG_ICASE | REG_EXTENDED));
}

bool 
texticregexne(struct varlena *s, struct varlena *p)
{
    return (!texticregexeq(s, p));
}

bool 
nameicregexeq(NameData *n, struct varlena *p)
{
    if (!n) return FALSE;
    return (fixedlen_regexeq(n->data, p, NAMEDATALEN, 
			     REG_ICASE | REG_EXTENDED));
}
bool 
nameicregexne(NameData *s, struct varlena *p)
{
    return (!nameicregexeq(s, p));
}

