
/* -----------------------------------------------------------------------
 * formatting.c
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/formatting.c,v 1.1 2000-01-25 23:53:51 momjian Exp $
 *
 *   TO_CHAR(); TO_DATETIME(); TO_DATE(); TO_NUMBER();  
 *
 *   The PostgreSQL routines for a DateTime/int/float/numeric formatting, 
 *   inspire with Oracle TO_CHAR() / TO_DATE() / TO_NUMBER() routines.  
 *
 *   1999 Karel Zak "Zakkr"
 *
 *
 *   Cache & Memory:
 *	Routines use (itself) internal cache for format pictures. If 
 *	new format arg is same as a last format string, routines not 
 *	call the format-parser. 
 *	
 *	The cache use static buffer and is persistent across transactions. If
 *	format-picture is bigger than cache buffer, parser is called always. 
 *
 *   NOTE for Number version:
 *	All in this version is implemented as keywords ( => not used
 * 	suffixes), because a format picture is for *one* item (number) 
 *	only. It not is as a datetime version, where each keyword (can) 
 *	has suffix.  
 *
 *   NOTE for DateTime version:
 *	In this modul is *not* used POSIX 'struct tm' type, but 
 *	PgSQL type, which has tm_mon based on one (*non* zero) and
 *	year *not* based on 1900, but is used full year number.  
 *	Modul support AC / BC years.	 
 *
 *  Supported types for to_char():
 *		
 *		DateTime, Numeric, int4, int8, float4, float8
 *
 *  Supported types for reverse conversion:
 *
 *		Datetime	- to_datetime()
 *		Date		- to_date()
 *		Numeric		- to_number()		
 *
 * -----------------------------------------------------------------------
 */
 
/* ----------
 * UnComment me for DEBUG
 * ----------
 */
/*** 
#define DEBUG_TO_FROM_CHAR	
#define DEBUG_elog_output	NOTICE
***/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>

#include "postgres.h"
#include "utils/builtins.h"
#include "utils/pg_locale.h"
#include "utils/formatting.h"

/* ----------
 * Routines type
 * ----------
 */
#define DCH_TYPE		1	/* DATE-TIME version 	*/		
#define NUM_TYPE		2	/* NUMBER version	*/

/* ----------
 * KeyWord Index (ascii from position 32 (' ') to 126 (~))
 * ----------
 */
#define KeyWord_INDEX_SIZE		('~' - ' ' + 1)
#define KeyWord_INDEX_FILTER(_c)	((_c) < ' ' || (_c) > '~' ? 0 : 1)

/* ----------
 * Maximal length of one node 
 * ----------
 */
#define DCH_MAX_ITEM_SIZ		9	/* some month name ? 	  */
#define NUM_MAX_ITEM_SIZ		16	/* roman number 	  */

/* ----------
 * Format picture cache limits
 * ----------
 */
#define NUM_CACHE_SIZE	64 
#define DCH_CACHE_SIZE	128 

/* ----------
 * More in float.c
 * ----------
 */
#define MAXFLOATWIDTH   64
#define MAXDOUBLEWIDTH  128

/* ----------
 * External (defined in PgSQL dt.c (datetime utils)) 
 * ----------
 */
extern  char		*months[],		/* month abbreviation   */
			*days[];		/* full days		*/

/* ----------
 * Format parser structs             
 * ----------
 */
typedef struct {
	char		*name;		/* suffix string		*/
	int		len,		/* suffix length		*/
			id,		/* used in node->suffix	*/
			type;		/* prefix / postfix 		*/
} KeySuffix;

typedef struct {
	char		*name;		/* keyword			*/
					/* action for keyword		*/
	int		len,		/* keyword length		*/	
			(*action)(),	
			id;		/* keyword id			*/			
} KeyWord;

typedef struct {
	int		type;		/* node type 			*/
	KeyWord		*key;		/* if node type is KEYWORD 	*/
	int		character,	/* if node type is CHAR 	*/
			suffix;		/* keyword suffix 		*/		
} FormatNode;

#define NODE_TYPE_END		1
#define	NODE_TYPE_ACTION	2
#define NODE_TYPE_CHAR		3

#define SUFFTYPE_PREFIX		1
#define SUFFTYPE_POSTFIX	2


/* ----------
 * Full months
 * ----------
 */
static char *months_full[]	= {
	"January", "February", "March", "April", "May", "June", "July", 
	"August", "September", "October", "November", "December", NULL        
};

/* ----------
 * AC / DC
 * ----------
 */
#define YEAR_ABS(_y)	(_y < 0 ? -(_y -1) : _y)
#define BC_STR		" BC"

/* ---------- 
 * Months in roman-numeral 
 * (Must be conversely for seq_search (in FROM_CHAR), because 
 *  'VIII' must be over 'V')   
 * ----------
 */
static char *rm_months[] = {
	"XII", "XI", "X", "IX", "VIII", "VII",
	"VI", "V", "IV", "III", "II", "I", NULL
};

/* ----------
 * Roman numbers
 * ----------
 */
static char *rm1[]   = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", NULL };
static char *rm10[]  = { "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC", NULL };
static char *rm100[] = { "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM", NULL };

/* ---------- 
 * Ordinal postfixes 
 * ----------
 */
static char *numTH[] = { "ST", "ND", "RD", "TH", NULL };
static char *numth[] = { "st", "nd", "rd", "th", NULL };

/* ---------- 
 * Flags & Options: 
 * ----------
 */
#define TO_CHAR		1
#define FROM_CHAR 	2

#define ONE_UPPER	1		/* Name	*/
#define ALL_UPPER	2		/* NAME */
#define ALL_LOWER	3		/* name */

#define FULL_SIZ	0

#define MAX_MON_LEN	3
#define MAX_DY_LEN	3

#define TH_UPPER	1
#define TH_LOWER	2


#ifdef DEBUG_TO_FROM_CHAR
	#define NOTICE_TM {\
		elog(DEBUG_elog_output, "TM:\nsec %d\nyear %d\nmin %d\nwday %d\nhour %d\nyday %d\nmday %d\nnisdst %d\nmon %d\n",\
			tm->tm_sec, tm->tm_year,\
			tm->tm_min, tm->tm_wday, tm->tm_hour, tm->tm_yday,\
			tm->tm_mday, tm->tm_isdst,tm->tm_mon);\
	}		
#endif

/* ----------
 * Flags for DCH version
 * ----------
 */
static int DCH_global_flag	= 0; 
 
#define DCH_F_FX	0x01 

#define IS_FX		(DCH_global_flag & DCH_F_FX)


/* ----------
 * Number description struct
 * ----------
 */
typedef struct {
	int	pre,			/* (count) numbers before decimal */
		pos,			/* (count) numbers after decimal  */
		lsign,			/* want locales sign		  */
		flag,			/* number parametrs		  */
		pre_lsign_num,		/* tmp value for lsign		  */	
		multi,			/* multiplier for 'V'		  */
		need_locale;		/* needs it locale		  */ 
} NUMDesc;

/* ----------
 * Flags for NUMBER version 
 * ----------
 */
#define	NUM_F_DECIMAL	0x01
#define NUM_F_LDECIMAL 	0x02
#define NUM_F_ZERO	0x04
#define NUM_F_BLANK	0x08
#define NUM_F_FILLMODE	0x10
#define NUM_F_LSIGN	0x20
#define NUM_F_BRACKET	0x40
#define NUM_F_MINUS	0x80
#define NUM_F_PLUS	0x100
#define NUM_F_ROMAN	0x200
#define NUM_F_MULTI	0x400

#define NUM_LSIGN_PRE	-1
#define NUM_LSIGN_POST	1
#define NUM_LSIGN_NONE	0

/* ----------
 * Tests
 * ----------
 */
#define	IS_DECIMAL(_f)	((_f)->flag & NUM_F_DECIMAL)
#define	IS_LDECIMAL(_f)	((_f)->flag & NUM_F_LDECIMAL)
#define	IS_ZERO(_f)	((_f)->flag & NUM_F_ZERO)
#define	IS_BLANK(_f)	((_f)->flag & NUM_F_BLANK)
#define	IS_FILLMODE(_f)	((_f)->flag & NUM_F_FILLMODE)
#define	IS_BRACKET(_f)	((_f)->flag & NUM_F_BRACKET)
#define IS_MINUS(_f)	((_f)->flag & NUM_F_MINUS)
#define IS_PLUS(_f)	((_f)->flag & NUM_F_PLUS)
#define IS_ROMAN(_f)	((_f)->flag & NUM_F_ROMAN)
#define IS_MULTI(_f)	((_f)->flag & NUM_F_MULTI)

/* ----------
 * Private global-modul definitions 
 * ----------
 */
static	struct tm 	_tm,	*tm  = &_tm;

/* ----------
 * Utils
 * ----------
 */
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif

/*****************************************************************************
 *			KeyWords definition & action 
 *****************************************************************************/

static int dch_global(int arg, char *inout, int suf, int flag, FormatNode *node); 
static int dch_time(int arg, char *inout, int suf, int flag, FormatNode *node);
static int dch_date(int arg, char *inout, int suf, int flag, FormatNode *node);

/* ---------- 
 * Suffixes: 
 * ----------
 */
#define	DCH_S_FM	0x01
#define	DCH_S_TH	0x02
#define	DCH_S_th	0x04
#define	DCH_S_SP	0x08

/* ---------- 
 * Suffix tests 
 * ----------
 */
#define S_THth(_s)	(((_s & DCH_S_TH) || (_s & DCH_S_th)) ? 1 : 0)
#define S_TH(_s)	((_s & DCH_S_TH) ? 1 : 0)
#define S_th(_s)	((_s & DCH_S_th) ? 1 : 0)
#define S_TH_TYPE(_s)	((_s & DCH_S_TH) ? TH_UPPER : TH_LOWER)

#define S_FM(_s)	((_s & DCH_S_FM) ? 1 : 0)
#define S_SP(_s)	((_s & DCH_S_SP) ? 1 : 0)

/* ----------
 * Suffixes definition for DATE-TIME TO/FROM CHAR
 * ----------
 */
static KeySuffix DCH_suff[] = {
	{	"FM",		2, DCH_S_FM,	SUFFTYPE_PREFIX	 },
	{	"TH",		2, DCH_S_TH,	SUFFTYPE_POSTFIX },		
	{	"th",		2, DCH_S_th,	SUFFTYPE_POSTFIX },		
	{	"SP",		2, DCH_S_SP,	SUFFTYPE_POSTFIX },
	/* last */
	{	NULL,		0, 0,		0		 }	
};

/* ----------
 * Format-pictures (KeyWord).
 * 
 * The KeyWord field; alphabetic sorted, *BUT* strings alike is sorted
 *		  complicated -to-> easy: 
 *
 *	(example: "DDD","DD","Day","D" )	
 *
 * (this specific sort needs the algorithm for sequential search for strings,
 * which not has exact end; -> How keyword is in "HH12blabla" ? - "HH" 
 * or "HH12"? You must first try "HH12", because "HH" is in string, but 
 * it is not good.   
 *
 * (!) 
 *    Position for the keyword is simular as position in the enum DCH/NUM_poz 
 * (!)
 *
 * For fast search is used the 'int index[256-32]' (first 32 ascii chars is 
 * skiped), in this index is DCH_ enums for each ASCII position or -1 if 
 * char is not used in the KeyWord. Search example for string "MM":
 * 	1)	see in index to index[77-32] (77 = 'M'), 
 *	2)	take keywords position from index[77-32]
 *	3)	run sequential search in keywords[] from position   
 *
 * ----------
 */

typedef enum { 
	DCH_CC,
	DCH_DAY,
	DCH_DDD,
	DCH_DD,
	DCH_DY,
	DCH_Day,
	DCH_Dy,
	DCH_D,
	DCH_FX,		/* global suffix */
	DCH_HH24,
	DCH_HH12,
	DCH_HH,
	DCH_J,
	DCH_MI,
	DCH_MM,
	DCH_MONTH,
	DCH_MON,
	DCH_Month,
	DCH_Mon,
	DCH_Q,
	DCH_RM,
	DCH_SSSS,
	DCH_SS,
	DCH_WW,
	DCH_W,
	DCH_Y_YYY,
	DCH_YYYY,
	DCH_YYY,
	DCH_YY,
	DCH_Y,
	DCH_day,
	DCH_dy,
	DCH_month,
	DCH_mon,
	/* last */
	_DCH_last_
} DCH_poz;

typedef enum {
	NUM_COMMA,
	NUM_DEC,
	NUM_0,
	NUM_9,
	NUM_B,
	NUM_C,
	NUM_D,
	NUM_E,
	NUM_FM,
	NUM_G,
	NUM_L,
	NUM_MI,
	NUM_PL,
	NUM_PR,
	NUM_RN,
	NUM_SG,
	NUM_SP,
	NUM_S,
	NUM_TH,
	NUM_V,
	NUM_rn,
	NUM_th,
	/* last */
	_NUM_last_
} NUM_poz;

/* ----------
 * KeyWords for DATE-TIME version
 * ----------
 */
static KeyWord DCH_keywords[] = {	
/*	keyword,	len, func.	type                 is in Index */
							
{	"CC",           2, dch_date,	DCH_CC		},	/*C*/
{	"DAY",          3, dch_date,	DCH_DAY		},	/*D*/
{	"DDD",          3, dch_date,	DCH_DDD		},
{	"DD",           2, dch_date,	DCH_DD		},
{	"DY",           2, dch_date,	DCH_DY		},
{	"Day",		3, dch_date,	DCH_Day		},
{	"Dy",           2, dch_date,	DCH_Dy		},
{	"D",            1, dch_date,	DCH_D		},	
{	"FX",		2, dch_global,	DCH_FX		},	/*F*/
{	"HH24",		4, dch_time,	DCH_HH24	},	/*H*/
{	"HH12",		4, dch_time,	DCH_HH12	},
{	"HH",		2, dch_time,	DCH_HH		},
{	"J",            1, dch_date,	DCH_J	 	},	/*J*/	
{	"MI",		2, dch_time,	DCH_MI		},
{	"MM",          	2, dch_date,	DCH_MM		},
{	"MONTH",        5, dch_date,	DCH_MONTH	},
{	"MON",          3, dch_date,	DCH_MON		},
{	"Month",        5, dch_date,	DCH_Month	},
{	"Mon",          3, dch_date,	DCH_Mon		},
{	"Q",            1, dch_date,	DCH_Q		},	/*Q*/	
{	"RM",           2, dch_date,	DCH_RM	 	},	/*R*/
{	"SSSS",		4, dch_time,	DCH_SSSS	},	/*S*/
{	"SS",		2, dch_time,	DCH_SS		},
{	"WW",           2, dch_date,	DCH_WW		},	/*W*/
{	"W",            1, dch_date,	DCH_W	 	},
{	"Y,YYY",        5, dch_date,	DCH_Y_YYY	},	/*Y*/
{	"YYYY",         4, dch_date,	DCH_YYYY	},
{	"YYY",          3, dch_date,	DCH_YYY		},
{	"YY",           2, dch_date,	DCH_YY		},
{	"Y",            1, dch_date,	DCH_Y	 	},
{	"day",		3, dch_date,	DCH_day		},	/*d*/
{	"dy",           2, dch_date,	DCH_dy		},	
{	"month",        5, dch_date,	DCH_month	},	/*m*/	
{	"mon",          3, dch_date,	DCH_mon		},

/* last */
{	NULL,		0, NULL,	0 		}};

/* ----------
 * KeyWords for NUMBER version
 * ----------
 */
static KeyWord NUM_keywords[] = {	
/*	keyword,	len, func.	type   	     	   is in Index */
{	",",	        1, NULL,	NUM_COMMA	},  /*,*/
{	".",	        1, NULL,	NUM_DEC		},  /*.*/
{	"0",	        1, NULL,	NUM_0		},  /*0*/
{	"9",	        1, NULL,	NUM_9		},  /*9*/	
{	"B",	        1, NULL,	NUM_B		},  /*B*/	
{	"C",	        1, NULL,	NUM_C		},  /*C*/
{	"D",	        1, NULL,	NUM_D		},  /*D*/
{	"E",	        1, NULL,	NUM_E		},  /*E*/	
{	"FM",	        2, NULL,	NUM_FM		},  /*F*/	
{	"G",	        1, NULL,	NUM_G		},  /*G*/	
{	"L",	        1, NULL,	NUM_L		},  /*L*/	
{	"MI",	        2, NULL,	NUM_MI		},  /*M*/	
{	"PL",        	2, NULL,	NUM_PL		},  /*P*/	
{	"PR",	        2, NULL,	NUM_PR		},  	
{	"RN",	        2, NULL,	NUM_RN		},  /*R*/
{	"SG",	        2, NULL,	NUM_SG		},  /*S*/
{	"SP",	        2, NULL,	NUM_SP   	},
{	"S",	        1, NULL,	NUM_S		},  
{	"TH",		2, NULL,	NUM_TH		},  /*T*/
{	"V",	        1, NULL,	NUM_V   	},  /*V*/			
{	"rn",	        2, NULL,	NUM_rn	 	},  /*r*/
{	"th",	        2, NULL,	NUM_th	 	},  /*t*/

/* last */	
{	NULL,		0, NULL,	0 		}};


/* ----------
 * KeyWords index for DATE-TIME version
 * ----------
 */
static int DCH_index[256 - 32] = {
/*
0	1	2	3	4	5	6	7	8	9
*/
	/*---- first 0..31 chars is skiped ----*/

		-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	DCH_CC,	DCH_DAY,-1,	
DCH_FX,	-1,	DCH_HH24,-1,	DCH_J,	-1,	-1,	DCH_MI,	-1,	-1,
-1,	DCH_Q,	DCH_RM,	DCH_SSSS,-1,	-1,	-1,	DCH_WW,	-1,	DCH_Y_YYY,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
DCH_day,-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	DCH_month,	
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1

	/*---- chars over 126 are skiped ----*/
};	

/* ----------
 * KeyWords index for NUMBER version
 * ----------
 */
static int NUM_index[256 - 32] = {
/*
0	1	2	3	4	5	6	7	8	9
*/
	/*---- first 0..31 chars are skiped ----*/

		-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	NUM_COMMA,-1,	NUM_DEC,-1,	NUM_0,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	NUM_9,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	NUM_B,	NUM_C,	NUM_D,	NUM_E,	
NUM_FM,	NUM_G,	-1,	-1,	-1,	-1,	NUM_L,	NUM_MI,	-1,	-1,
NUM_PL,-1,	NUM_RN,	NUM_SG,	NUM_TH,	-1,	NUM_V,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	
-1,	-1,	-1,	-1,	NUM_rn,	-1,	NUM_th,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,
-1,	-1,	-1,	-1,	-1,	-1

	/*---- chars over 126 are skiped ----*/
};

/* ----------
 * Number processor struct
 * ----------
 */
typedef struct NUMProc
{
	int	type;		/* FROM_CHAR (TO_NUMBER) or TO_CHAR */

	NUMDesc	*Num;		/* number description	*/	
	
	int	sign,		/* '-' or '+'		*/
		sign_wrote,	/* is sign wrote	*/
		count_num,	/* number of non-write digits	*/
		in_number,	/* is inside number	*/
		pre_number;	/* space before number	*/
		
	char	*number,	/* string with number	*/
		*number_p,	/* pointer to current number pozition */
		*inout,		/* in / out buffer	*/
		*inout_p,	/* pointer to current inout pozition */
		
		*L_negative_sign,	/* Locale */
		*L_positive_sign,
		*decimal,	
		*L_thousands_sep,
		*L_currency_symbol;
} NUMProc;


/* ----------
 * Functions
 * ----------
 */
static KeyWord *index_seq_search(char *str, KeyWord *kw, int *index);
static KeySuffix *suff_search(char *str, KeySuffix *suf, int type);
static void NUMDesc_prepare(NUMDesc *num, FormatNode *n);
static void parse_format(FormatNode *node, char *str, KeyWord *kw, 
		KeySuffix *suf, int *index, int ver, NUMDesc *Num);
static char *DCH_action(FormatNode *node, char *inout, int flag);

#ifdef DEBUG_TO_FROM_CHAR
	static void dump_index(KeyWord *k, int *index);
	static void dump_node(FormatNode *node, int max);
#endif

static char *get_th(char *num, int type);
static char *str_numth(char *dest, char *num, int type);
static int int4len(int4 num);
static char *str_toupper(char *buff);
static char *str_tolower(char *buff);
static int is_acdc(char *str, int *len);
static int seq_search(char *name, char **array, int type, int max, int *len);
static int dch_global(int arg, char *inout, int suf, int flag, FormatNode *node); 
static int dch_time(int arg, char *inout, int suf, int flag, FormatNode *node);
static int dch_date(int arg, char *inout, int suf, int flag, FormatNode *node);
static char *fill_str(char *str, int c, int max);
static FormatNode *NUM_cache( int len, char *CacheStr, FormatNode *CacheFormat, 
	   NUMDesc *CacheNum, NUMDesc *Num, char *pars_str, int *flag);
static char *int_to_roman(int number);
static void NUM_prepare_locale(NUMProc *Np);
static int NUM_numpart(NUMProc *Np, int id);
static char *NUM_processor (FormatNode *node, NUMDesc *Num, char *inout, char *number, 
					int plen, int sign, int type);


/* ----------
 * Fast sequential search, use index for data selection which
 * go to seq. cycle (it is very fast for non-wanted strings)
 * (can't be used binary search in format parsing)
 * ----------
 */
static KeyWord *
index_seq_search(char *str, KeyWord *kw, int *index) 
{
	int	poz;

	if (! KeyWord_INDEX_FILTER(*str))
		return (KeyWord *) NULL;

	if ( (poz =  *(index + (*str - ' '))) > -1) {
	
		KeyWord		*k = kw+poz;
	
		do {
			if (! strncmp(str, k->name, k->len))
				return k;
			k++;
			if (!k->name)
				return (KeyWord *) NULL;		  
		} while(*str == *k->name);
	}
	return (KeyWord *) NULL;
}

static KeySuffix *
suff_search(char *str, KeySuffix *suf, int type)
{
	KeySuffix	*s;
	
	for(s=suf; s->name != NULL; s++) {
		if (s->type != type)
			continue;
		
		if (!strncmp(str, s->name, s->len))
			return s;
	}
	return (KeySuffix *) NULL;
}

/* ----------
 * Prepare NUMDesc (number description struct) via FormatNode struct
 * ----------
 */
static void 
NUMDesc_prepare(NUMDesc *num, FormatNode *n)
{

	if (n->type != NODE_TYPE_ACTION)
		return;
	
	switch(n->key->id) {
		
		case NUM_9:
			if (IS_MULTI(num)) {
				++num->multi;
				break;
			}
			if (IS_DECIMAL(num))
				++num->pos;
			else 
				++num->pre;
			break;
		
		case NUM_0:
			if (num->pre == 0) 
				num->flag |= NUM_F_ZERO; 
			if (! IS_DECIMAL(num))
				++num->pre;
			else 
				++num->pos;
			break;	
		
		case NUM_B:
			if (num->pre == 0 && num->pos == 0 && (! IS_ZERO(num)))
				num->flag |= NUM_F_BLANK; 				
			break;		
		
		case NUM_D:
			num->flag |= NUM_F_LDECIMAL;
			num->need_locale = TRUE;
		case NUM_DEC:
			if (IS_DECIMAL(num))
				elog(ERROR, "to_char/number(): not unique decimal poit."); 
			if (IS_MULTI(num))
				elog(ERROR, "to_char/number(): can't use 'V' and decimal poin together."); 	
			num->flag |= NUM_F_DECIMAL;
			break;
		
		case NUM_FM:
			num->flag |= NUM_F_FILLMODE;
			break;
		
		case NUM_S:
			if (! IS_DECIMAL(num)) {
				num->lsign = NUM_LSIGN_PRE;
				num->pre_lsign_num = num->pre;
				num->need_locale = TRUE;			
				
			} else if (num->lsign == NUM_LSIGN_NONE) {
				num->lsign = NUM_LSIGN_POST;	
				num->need_locale = TRUE;
			}	
			break;
		
		case NUM_MI:
			num->flag |= NUM_F_MINUS;
			break;
		
		case NUM_PL:
			num->flag |= NUM_F_PLUS;
			break;		
		
		case NUM_SG:
			num->flag |= NUM_F_MINUS;
			num->flag |= NUM_F_PLUS;
			break;
		
		case NUM_PR:
			num->flag |= NUM_F_BRACKET;
			break;	
		case NUM_rn:
		case NUM_RN:
			num->flag |= NUM_F_ROMAN;
			break;
			
		case NUM_L:
		case NUM_G:
			num->need_locale = TRUE;	
			break;
		
		case NUM_V:
			if (IS_DECIMAL(num))
				elog(ERROR, "to_char/number(): can't use 'V' and decimal poin together."); 	
			num->flag |= NUM_F_MULTI;
			break;	
	}
	
	return;
}

/* ----------
 * Format parser, search small keywords and keyword's suffixes, and make 
 * format-node tree.
 *
 * for DATE-TIME & NUMBER version 
 * ----------
 */
static void 
parse_format(FormatNode *node, char *str, KeyWord *kw, 
		KeySuffix *suf, int *index, int ver, NUMDesc *Num)
{
	KeySuffix	*s;
	FormatNode	*n;
	int		node_set=0,
			suffix,
			last=0;

#ifdef DEBUG_TO_FROM_CHAR
	elog(DEBUG_elog_output, "to-from_char(): run parser.");
#endif

	n = node;

	while(*str) {
		suffix=0;
		
		/* ---------- 
		 * Prefix 
		 * ----------
		 */
		if (ver==DCH_TYPE && (s = suff_search(str, suf, SUFFTYPE_PREFIX)) != NULL) {
			suffix |= s->id;
			if (s->len)
				str += s->len;
		}
	
		/* ----------
		 * Keyword 
		 * ----------
		 */
		if (*str && (n->key = index_seq_search(str, kw, index)) != NULL) {
		
			n->type = NODE_TYPE_ACTION;
			n->suffix = 0;
			node_set= 1;
			if (n->key->len)
				str += n->key->len;
			
			/* ----------
			 * NUM version: Prepare global NUMDesc struct 
			 * ----------
			 */
			if (ver==NUM_TYPE)
				NUMDesc_prepare(Num, n);
			
			/* ----------
			 * Postfix
			 * ----------
			 */
			if (ver==DCH_TYPE && *str && (s = suff_search(str, suf, SUFFTYPE_POSTFIX)) != NULL) {
				suffix |= s->id;
				if (s->len)
					str += s->len;
			}
			
		} else if (*str) {
		
			/* ---------- 
			 * Special characters '\' and '"' 
			 * ----------
			 */
			if (*str == '"' && last != '\\') {
				
				int	x = 0;
			
				while(*(++str)) {
					if (*str == '"' && x != '\\') {
						str++;
						break;
					} else if (*str == '\\' && x != '\\') {
						x = '\\';
						continue;
					}
					n->type = NODE_TYPE_CHAR;
					n->character = *str;
					n->key = (KeyWord *) NULL;
					n->suffix = 0;
					++n;
					x = *str; 
				}
				node_set = 0;
				suffix = 0;
				last = 0;
				
			} else if (*str && *str == '\\' && last!='\\' && *(str+1) =='"') {
				last = *str;
				str++;
			
			} else if (*str) {
				n->type = NODE_TYPE_CHAR;
				n->character = *str;
				n->key = (KeyWord *) NULL;
				node_set = 1;
				last = 0;
				str++;
			}
			
		}
				
		/* end */	
		if (node_set) {
			if (n->type == NODE_TYPE_ACTION) 
				n->suffix = suffix;	
			++n;

			n->suffix = 0;
			node_set = 0;
		}
			
	}

	n->type = NODE_TYPE_END;
	n->suffix = 0;
	return;
}

/* ----------
 * Call keyword's function for each of (action) node in format-node tree 
 * ----------
 */
static char *
DCH_action(FormatNode *node, char *inout, int flag)
{
	FormatNode	*n;
	char		*s;
	
	
	/* ----------
	 * Zeroing global flags
	 * ----------
	 */
	DCH_global_flag = 0;
	
	for(n=node, s=inout; n->type != NODE_TYPE_END; n++) {
		if (n->type == NODE_TYPE_ACTION) {
			
			int 	len;
			
			/* ----------
			 * Call node action function 
			 * ----------
			 */
			len = n->key->action(n->key->id, s, n->suffix, flag, n); 	
			if (len > 0) 
				s += len;
			else 
				continue;	
				
		} else {
		
			/* ----------
			 * Remove to output char from input in TO_CHAR
			 * ----------
			 */
			if (flag == TO_CHAR) 
				*s = n->character;
			
			else {				
				/* ----------
				 * Skip blank space in FROM_CHAR's input 
				 * ----------
				 */
				if (isspace(n->character) && IS_FX == 0) {
					while(*s != '\0' && isspace(*(s+1)))
						++s;	
				}
			}
		}
		
		++s;	/* ! */
		
	}
	
	if (flag == TO_CHAR)
		*s = '\0';
	return inout;
}


/* ----------
 * DEBUG: Dump the FormatNode Tree (debug)
 * ----------
 */
#ifdef DEBUG_TO_FROM_CHAR

#define DUMP_THth(_suf) (S_TH(_suf) ? "TH" : (S_th(_suf) ? "th" : " "))
#define DUMP_FM(_suf)	(S_FM(_suf) ? "FM" : " ")

static void 
dump_node(FormatNode *node, int max)
{
	FormatNode	*n;
	int		a;
	
	elog(DEBUG_elog_output, "to_from-char(): DUMP FORMAT");
	
	for(a=0, n=node; a<=max; n++, a++) {
		if (n->type == NODE_TYPE_ACTION) 
			elog(DEBUG_elog_output, "%d:\t NODE_TYPE_ACTION\t(%s,%s)", 
					a, DUMP_THth(n->suffix), DUMP_FM(n->suffix));
		else if (n->type == NODE_TYPE_CHAR) 
			elog(DEBUG_elog_output, "%d:\t NODE_TYPE_CHAR '%c'", a, n->character);	
		else if (n->type == NODE_TYPE_END) {
			elog(DEBUG_elog_output, "%d:\t NODE_TYPE_END", a);		
			return;
		} else
			elog(DEBUG_elog_output, "%d:\t UnKnown NODE !!!", a);	
			
	}
}
#endif

/*****************************************************************************
 *			Private utils 
 *****************************************************************************/

/* ----------
 * Return ST/ND/RD/TH for simple (1..9) numbers 
 * type --> 0 upper, 1 lower
 * ----------	
 */	
static char *
get_th(char *num, int type)
{
	int	len = strlen(num),
		last;				
	
	last = *(num + (len-1));
	if (!isdigit((unsigned char) last))
		elog(ERROR, "get_th: '%s' is not number.", num);
	
	/* 11 || 12 */
	if (len == 2 && (last=='1' || last=='2') && *num == '1') 
		last=0;

	switch(last) {
		case '1':
			if (type==TH_UPPER) return numTH[0];
			return numth[0];
		case '2':
			if (type==TH_UPPER) return numTH[1];
			return numth[1];
		case '3':
			if (type==TH_UPPER) return numTH[2];
			return numth[2];		
		default:
			if (type==TH_UPPER) return numTH[3];
			return numth[3];
	}
	return NULL;
}

/* ----------
 * Convert string-number to ordinal string-number 
 * type --> 0 upper, 1 lower	
 * ----------
 */
static char *
str_numth(char *dest, char *num, int type)
{
	sprintf(dest, "%s%s", num, get_th(num, type));
	return dest; 
}

/* ----------
 * Return length of integer writed in string 
 * ----------
 */
static int 
int4len(int4 num)
{
 	char b[16];
 	return sprintf(b, "%d", num);
}

/* ----------
 * Convert string to upper-string
 * ----------
 */
static char *
str_toupper(char *buff)
{	
	char	*p_buff=buff;

	while (*p_buff) {
		*p_buff = toupper((unsigned char) *p_buff);
		++p_buff;
	}
	return buff;
}

/* ----------
 * Convert string to lower-string
 * ----------
 */
static char *
str_tolower(char *buff)
{	
	char	*p_buff=buff;

	while (*p_buff) {
		*p_buff = tolower((unsigned char) *p_buff);
		++p_buff;
	}
	return buff;
}

/* ----------
 * Check if in string is AC or BC (return: 0==none; -1==BC; 1==AC)  
 * ----------
 */
static int 
is_acdc(char *str, int *len)
{
	char	*p;
	
	for(p=str; *p != '\0'; p++) {
		if (isspace(*p))
			continue;
			
		if (*(p+1)) { 
			if (toupper(*p)=='B' && toupper(*(++p))=='C') {
			   	*len += (p - str) +1;
				return -1;   	
			} else if (toupper(*p)=='A' && toupper(*(++p))=='C') {
		   		*len += (p - str) +1;
				return 1;   	
		 	}
		} 
		return 0; 	
	}
	return 0;
} 
 
/* ----------
 * Sequential search with to upper/lower conversion
 * ----------
 */
static int 
seq_search(char *name, char **array, int type, int max, int *len)
{
	char	*p, *n, **a;
	int	last, i;
	
	*len = 0;
	
	if (!*name) 
		return -1;
	
        /* set first char */	
	if (type == ONE_UPPER || ALL_UPPER) 
		*name = toupper((unsigned char) *name);
	else if (type == ALL_LOWER)
		*name = tolower((unsigned char) *name);
		
	for(last=0, a=array; *a != NULL; a++) {
		
		/* comperate first chars */
		if (*name != **a)
			continue;
		
		for(i=1, p=*a+1, n=name+1; ; n++, p++, i++) {
			
			/* search fragment (max) only */
			if (max && i == max) {
				*len = i;
				return a - array;
			} 
			/* full size */
			if (*p=='\0') {
				*len = i;
				return a - array;
			}
			/* Not found in array 'a' */
			if (*n=='\0')
				break;
			
			/* 
			 * Convert (but convert new chars only)
			 */
			if (i > last) {
				if (type == ONE_UPPER || type == ALL_LOWER) 
					*n = tolower((unsigned char) *n);
				else if (type == ALL_UPPER)	
					*n = toupper((unsigned char) *n);
				last=i;
			}

#ifdef DEBUG_TO_FROM_CHAR			
			/* elog(DEBUG_elog_output, "N: %c, P: %c, A: %s (%s)", *n, *p, *a, name);*/
#endif			
			if (*n != *p)
				break; 
		}  	
	}
		
	return -1;		
}


#ifdef DEBUG_TO_FROM_CHAR
/* -----------
 * DEBUG: Call for debug and for index checking; (Show ASCII char 
 * and defined keyword for each used position  
 * ----------
 */	
static void 
dump_index(KeyWord *k, int *index)
{
	int	i, count=0, free_i=0;
	
	elog(DEBUG_elog_output, "TO-FROM_CHAR: Dump KeyWord Index:");
	
	for(i=0; i < KeyWord_INDEX_SIZE; i++) {
		if (index[i] != -1) {
			elog(DEBUG_elog_output, "\t%c: %s, ", i+32, k[ index[i] ].name);
			count++;
		} else {
			free_i++;	
			elog(DEBUG_elog_output, "\t(%d) %c %d", i, i+32, index[i]);
		}
	}	
	elog(DEBUG_elog_output, "\n\t\tUsed positions: %d,\n\t\tFree positions: %d",
		count, free_i);		
}
#endif

/* ----------
 * Skip TM / th in FROM_CHAR
 * ----------
 */
#define SKIP_THth(_suf)		(S_THth(_suf) ? 2 : 0)   


/* ----------
 * Global format opton for DCH version
 * ----------
 */
static int
dch_global(int arg, char *inout, int suf, int flag, FormatNode *node) 
{
	switch(arg) {
	
	case DCH_FX:
		DCH_global_flag	|= DCH_F_FX;
		break;
	}
	return 0;
}

/* ----------
 * Master function of TIME for: 
 *                    TO_CHAR   - write (inout) formated string
 *                    FROM_CHAR - scan (inout) string by course of FormatNode 
 * ----------
 */
static int 
dch_time(int arg, char *inout, int suf, int flag, FormatNode *node) 
{
	char	*p_inout = inout;

	switch(arg) {
	
	case DCH_HH:	
	case DCH_HH12:
		if (flag == TO_CHAR) {
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2, 
				tm->tm_hour==0 ? 12 :
				tm->tm_hour <13	? tm->tm_hour : tm->tm_hour-12);
			if (S_THth(suf)) 
				str_numth(p_inout, inout, 0);
			if (S_FM(suf) || S_THth(suf)) return strlen(p_inout)-1;
			else return 1;
		} else if (flag == FROM_CHAR) {
			if (S_FM(suf)) {
				sscanf(inout, "%d", &tm->tm_hour);
				return int4len((int4) tm->tm_hour)-1 + SKIP_THth(suf);
			} else {
				sscanf(inout, "%02d", &tm->tm_hour);
				return 1 + SKIP_THth(suf);
			}
				
		}
	case DCH_HH24:
		if (flag == TO_CHAR) {
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2, tm->tm_hour);
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_FM(suf) || S_THth(suf)) return strlen(p_inout)-1;
			else return 1;
		} else if (flag == FROM_CHAR) {
			if (S_FM(suf)) {
				sscanf(inout, "%d", &tm->tm_hour);
				return int4len((int4) tm->tm_hour)-1 + SKIP_THth(suf);
			} else {
				sscanf(inout, "%02d", &tm->tm_hour);
				return 1 + SKIP_THth(suf);
			}
		}
	case DCH_MI:	
		if (flag == TO_CHAR) {
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2, tm->tm_min);
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_FM(suf) || S_THth(suf)) return strlen(p_inout)-1;
			else return 1;
		} else if (flag == FROM_CHAR) {
			if (S_FM(suf)) {
				sscanf(inout, "%d", &tm->tm_min);
				return int4len((int4) tm->tm_min)-1 + SKIP_THth(suf);
			} else {
				sscanf(inout, "%02d", &tm->tm_min);
				return 1 + SKIP_THth(suf);
			}
		}
	case DCH_SS:	
		if (flag == TO_CHAR) {
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2, tm->tm_sec);
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_FM(suf) || S_THth(suf)) return strlen(p_inout)-1;
			else return 1;
		} else if (flag == FROM_CHAR) {
			if (S_FM(suf)) {
				sscanf(inout, "%d", &tm->tm_sec);
				return int4len((int4) tm->tm_sec)-1 + SKIP_THth(suf);
			} else {
				sscanf(inout, "%02d", &tm->tm_sec);
				return 1 + SKIP_THth(suf);
			}
		}
	case DCH_SSSS:	
		if (flag == TO_CHAR) {
			sprintf(inout, "%d", tm->tm_hour	* 3600	+
				    tm->tm_min	* 60	+
				    tm->tm_sec);
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			return strlen(p_inout)-1;		
		}  else if (flag == FROM_CHAR) 
			elog(ERROR, "to_datatime: SSSS is not supported");
	}	
	return 0;	
}

#define CHECK_SEQ_SEARCH(_l, _s) {					\
	if (_l <= 0) { 							\
		elog(ERROR, "to_datatime: bad value for %s", _s);		\
	}								\
}

/* ----------
 * Master of DATE for:
 *                    TO_CHAR   - write (inout) formated string
 *                    FROM_CHAR - scan (inout) string by course of FormatNode 
 * ----------
 */
static int 
dch_date(int arg, char *inout, int suf, int flag, FormatNode *node)
{
	char	buff[ DCH_CACHE_SIZE ], 		
		*p_inout;
	int	i, len;

	p_inout = inout;

	/* ----------
	 * In the FROM-char is not difference between "January" or "JANUARY" 
	 * or "january", all is before search convert to "first-upper".    
	 * This convention is used for MONTH, MON, DAY, DY
	 * ----------
	 */
	if (flag == FROM_CHAR) {
		if (arg == DCH_MONTH || arg == DCH_Month || arg == DCH_month) {
	
			tm->tm_mon = seq_search(inout, months_full, ONE_UPPER, FULL_SIZ, &len);
			CHECK_SEQ_SEARCH(len, "MONTH/Month/month");
			++tm->tm_mon;
			if (S_FM(suf))	return len-1;
			else 		return 8;

		} else if (arg == DCH_MON || arg == DCH_Mon || arg == DCH_mon) {
		
			tm->tm_mon = seq_search(inout, months, ONE_UPPER, MAX_MON_LEN, &len);
			CHECK_SEQ_SEARCH(len, "MON/Mon/mon");
			++tm->tm_mon;
			return 2;
		
		} else if (arg == DCH_DAY || arg == DCH_Day || arg == DCH_day) {
		
			tm->tm_wday =  seq_search(inout, days, ONE_UPPER, FULL_SIZ, &len);
			CHECK_SEQ_SEARCH(len, "DAY/Day/day");
			if (S_FM(suf))	return len-1;
			else 		return 8;
			
		} else if (arg == DCH_DY || arg == DCH_Dy || arg == DCH_dy) {
			
			tm->tm_wday =  seq_search(inout, days, ONE_UPPER, MAX_DY_LEN, &len);
			CHECK_SEQ_SEARCH(len, "DY/Dy/dy");
			return 2;
			
		} 
	} 
	
	switch(arg) {
	case DCH_MONTH:
		strcpy(inout, months_full[ tm->tm_mon - 1]);		
		sprintf(inout, "%*s", S_FM(suf) ? 0 : -9, str_toupper(inout));	
		if (S_FM(suf)) return strlen(p_inout)-1;
		else return 8;
	case DCH_Month:
		sprintf(inout, "%*s", S_FM(suf) ? 0 : -9, months_full[ tm->tm_mon -1 ]);		
	        if (S_FM(suf)) return strlen(p_inout)-1;
		else return 8;
	case DCH_month:
		sprintf(inout, "%*s", S_FM(suf) ? 0 : -9, months_full[ tm->tm_mon -1 ]);
		*inout = tolower(*inout);
	        if (S_FM(suf)) return strlen(p_inout)-1;
		else return 8;
	case DCH_MON:
		strcpy(inout, months[ tm->tm_mon -1 ]);		
		inout = str_toupper(inout);
		return 2;
	case DCH_Mon:
		strcpy(inout, months[ tm->tm_mon -1 ]);		
		return 2;     
	case DCH_mon:
		strcpy(inout, months[ tm->tm_mon -1 ]);		
		*inout = tolower(*inout);
		return 2;
	case DCH_MM:
		if (flag == TO_CHAR) {
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2, tm->tm_mon );
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_FM(suf) || S_THth(suf)) 
				return strlen(p_inout)-1;
			else return 1;
		} else if (flag == FROM_CHAR) {
			if (S_FM(suf)) {
				sscanf(inout, "%d", &tm->tm_mon);
				return int4len((int4) tm->tm_mon)-1 + SKIP_THth(suf);
			} else {
				sscanf(inout, "%02d", &tm->tm_mon);
				return 1 + SKIP_THth(suf);
			}		
		}	
	case DCH_DAY:
		strcpy(inout, days[ tm->tm_wday ]); 			        
		sprintf(inout, "%*s", S_FM(suf) ? 0 : -9, str_toupper(inout)); 
	        if (S_FM(suf)) return strlen(p_inout)-1;
		else return 8;	
	case DCH_Day:
		sprintf(inout, "%*s", S_FM(suf) ? 0 : -9, days[ tm->tm_wday]);			
	       	if (S_FM(suf)) return strlen(p_inout)-1;
		else return 8;			
	case DCH_day:
		sprintf(inout, "%*s", S_FM(suf) ? 0 : -9, days[ tm->tm_wday]);			
		*inout = tolower(*inout);
	       	if (S_FM(suf)) return strlen(p_inout)-1;
		else return 8;
	case DCH_DY:	        
		strcpy(inout, days[ tm->tm_wday]);
		inout = str_toupper(inout);		
		return 2;
	case DCH_Dy:
		strcpy(inout, days[ tm->tm_wday]);			
		return 2;			
	case DCH_dy:
		strcpy(inout, days[ tm->tm_wday]);			
		*inout = tolower(*inout);
		return 2;
	case DCH_DDD:
		if (flag == TO_CHAR) {
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 3, tm->tm_yday);
	        	if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_FM(suf) || S_THth(suf)) 
				return strlen(p_inout)-1;
			else return 2;
		} else if (flag == FROM_CHAR) {	
			if (S_FM(suf)) {
				sscanf(inout, "%d", &tm->tm_yday);
				return int4len((int4) tm->tm_yday)-1 + SKIP_THth(suf);
			} else {
				sscanf(inout, "%03d", &tm->tm_yday);
				return 2 + SKIP_THth(suf);
			}	
		}
	case DCH_DD:
		if (flag == TO_CHAR) {	
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2, tm->tm_mday);	
	        	if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_FM(suf) || S_THth(suf)) 
				return strlen(p_inout)-1;
			else return 1;
		} else if (flag == FROM_CHAR) {	
			if (S_FM(suf)) {
				sscanf(inout, "%d", &tm->tm_mday);
				return int4len((int4) tm->tm_mday)-1 + SKIP_THth(suf);
			} else {
				sscanf(inout, "%02d", &tm->tm_mday);
				return 1 + SKIP_THth(suf);
			}	
		}	
	case DCH_D:
		if (flag == TO_CHAR) {
			sprintf(inout, "%d", tm->tm_wday+1);
	        	if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_THth(suf)) 
				return 2;
	        	return 0;
	        } else if (flag == FROM_CHAR) {	
			sscanf(inout, "%1d", &tm->tm_wday);
			if(tm->tm_wday) --tm->tm_wday;
			return 0 + SKIP_THth(suf);
		} 
	case DCH_WW:
		if (flag == TO_CHAR) {
			sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2,  
				(tm->tm_yday - tm->tm_wday + 7) / 7);		
	        	if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_FM(suf) || S_THth(suf)) 
				return strlen(p_inout)-1;
			else return 1;
		}  else if (flag == FROM_CHAR)
			elog(ERROR, "to_datatime: WW is not supported");
	case DCH_Q:
		if (flag == TO_CHAR) {
			sprintf(inout, "%d", (tm->tm_mon-1)/3+1);		
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_THth(suf)) 
				return 2;
	        	return 0;
	        } else if (flag == FROM_CHAR)
			elog(ERROR, "to_datatime: Q is not supported");
	case DCH_CC:
		if (flag == TO_CHAR) {
			i = tm->tm_year/100 +1;
			if (i <= 99 && i >= -99)	
				sprintf(inout, "%0*d", S_FM(suf) ? 0 : 2, i);
			else
				sprintf(inout, "%d", i);			
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			return strlen(p_inout)-1;
		} else if (flag == FROM_CHAR)
			elog(ERROR, "to_datatime: CC is not supported");
	case DCH_Y_YYY:	
		if (flag == TO_CHAR) {
			i= YEAR_ABS(tm->tm_year) / 1000;
			sprintf(inout, "%d,%03d", i, YEAR_ABS(tm->tm_year) -(i*1000));
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (tm->tm_year < 0)
				strcat(inout, BC_STR);	
			return strlen(p_inout)-1;
		} else if (flag == FROM_CHAR) {
			int	cc, yy;
			sscanf(inout, "%d,%03d", &cc, &yy);
			tm->tm_year = (cc * 1000) + yy;
			
			if (!S_FM(suf) && tm->tm_year <= 9999 && tm->tm_year >= -9999)	
				len = 5; 
			else 					
				len = int4len((int4) tm->tm_year)+1;
			len +=  SKIP_THth(suf);	
			/* AC/BC */ 	
			if (is_acdc(inout+len, &len) < 0 && tm->tm_year > 0) 
				tm->tm_year = -(tm->tm_year);
			if (tm->tm_year < 0) 
				tm->tm_year = tm->tm_year+1; 
			return len-1;
		}	
	case DCH_YYYY:
		if (flag == TO_CHAR) {
			if (tm->tm_year <= 9999 && tm->tm_year >= -9998)
				sprintf(inout, "%0*d", S_FM(suf) ? 0 : 4,  YEAR_ABS(tm->tm_year));
			else
				sprintf(inout, "%d", YEAR_ABS(tm->tm_year));	
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (tm->tm_year < 0)
				strcat(inout, BC_STR);
			return strlen(p_inout)-1;
		} else if (flag == FROM_CHAR) {
			sscanf(inout, "%d", &tm->tm_year);
			if (!S_FM(suf) && tm->tm_year <= 9999 && tm->tm_year >= -9999)	
				len = 4;
			else 					
				len = int4len((int4) tm->tm_year);
			len +=  SKIP_THth(suf);
			/* AC/BC */ 	
			if (is_acdc(inout+len, &len) < 0 && tm->tm_year > 0) 
				tm->tm_year = -(tm->tm_year);
			if (tm->tm_year < 0) 
				tm->tm_year = tm->tm_year+1; 
			return len-1;
		}	
	case DCH_YYY:
		if (flag == TO_CHAR) {
			sprintf(buff, "%03d", YEAR_ABS(tm->tm_year));
			i=strlen(buff);
			strcpy(inout, buff+(i-3));
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_THth(suf)) return 4;
			return 2;
		} else if (flag == FROM_CHAR) {
			int	yy;
			sscanf(inout, "%03d", &yy);
			tm->tm_year = (tm->tm_year/1000)*1000 +yy;
			return 2 +  SKIP_THth(suf);
		}	
	case DCH_YY:
		if (flag == TO_CHAR) {
			sprintf(buff, "%02d", YEAR_ABS(tm->tm_year));
			i=strlen(buff);
			strcpy(inout, buff+(i-2));
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_THth(suf)) return 3;
			return 1;
		} else if (flag == FROM_CHAR) {
			int	yy;
			sscanf(inout, "%02d", &yy);
			tm->tm_year = (tm->tm_year/100)*100 +yy;
			return 1 +  SKIP_THth(suf);
		}	
	case DCH_Y:
		if (flag == TO_CHAR) {
			sprintf(buff, "%1d", YEAR_ABS(tm->tm_year));
			i=strlen(buff);
			strcpy(inout, buff+(i-1));
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_THth(suf)) return 2;
			return 0;
		} else if (flag == FROM_CHAR) {
			int	yy;
			sscanf(inout, "%1d", &yy);
			tm->tm_year = (tm->tm_year/10)*10 +yy;
			return 0 +  SKIP_THth(suf);
		}	
	case DCH_RM:
		if (flag == TO_CHAR) {
			sprintf(inout, "%*s", S_FM(suf) ? 0 : -4,   
				rm_months[ 12 - tm->tm_mon ]);
			if (S_FM(suf)) return strlen(p_inout)-1;
			else return 3;
		} else if (flag == FROM_CHAR) {
			tm->tm_mon = 11-seq_search(inout, rm_months, ALL_UPPER, FULL_SIZ, &len);
			CHECK_SEQ_SEARCH(len, "RM");
			++tm->tm_mon;
			if (S_FM(suf))	return len-1;
			else 		return 3;
		}	
	case DCH_W:
		if (flag == TO_CHAR) {
			sprintf(inout, "%d", (tm->tm_mday - tm->tm_wday +7) / 7 );
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			if (S_THth(suf)) return 2;
			return 0;
		} else if (flag == FROM_CHAR)
			elog(ERROR, "to_datatime: W is not supported");
	case DCH_J:
		if (flag == TO_CHAR) {
			sprintf(inout, "%d", date2j(tm->tm_year, tm->tm_mon, tm->tm_mday));		
			if (S_THth(suf)) 
				str_numth(p_inout, inout, S_TH_TYPE(suf));
			return strlen(p_inout)-1;
		} else if (flag == FROM_CHAR)
			elog(ERROR, "to_datatime: J is not supported");	
	}	
	return 0;	
}

/****************************************************************************
 *				Public routines
 ***************************************************************************/

/* -------------------
 * DATETIME to_char()
 * -------------------
 */
text *
datetime_to_char(DateTime *dt, text *fmt)
{
	static FormatNode	CacheFormat[ DCH_CACHE_SIZE +1];
	static char		CacheStr[ DCH_CACHE_SIZE +1];

	text 			*result;
	FormatNode		*format;
	int			flag=0;
	char			*str;
	double          	fsec;
	char       		*tzn;
	int			len=0, tz;

	if ((!PointerIsValid(dt)) || (!PointerIsValid(fmt)))
		return NULL;
	
	len 	= VARSIZE(fmt) - VARHDRSZ; 
	
	if (!len) 
		return textin("");

	tm->tm_sec	=0;	tm->tm_year	=0;
	tm->tm_min	=0;	tm->tm_wday	=0;
	tm->tm_hour	=0;	tm->tm_yday	=0;
	tm->tm_mday	=1;	tm->tm_isdst	=0;
	tm->tm_mon	=1;

	if (DATETIME_IS_EPOCH(*dt))
	{
		datetime2tm(SetDateTime(*dt), NULL, tm, &fsec, NULL);
	} else if (DATETIME_IS_CURRENT(*dt)) {
		datetime2tm(SetDateTime(*dt), &tz, tm, &fsec, &tzn);
	} else {
		if (datetime2tm(*dt, &tz, tm, &fsec, &tzn) != 0)
			elog(ERROR, "to_char: Unable to convert datetime to tm");
	}

	tm->tm_wday = (date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) + 1) % 7; 
	tm->tm_yday = date2j(tm->tm_year, tm->tm_mon, tm->tm_mday) - date2j(tm->tm_year, 1,1) +1;

	/* ----------
	 * Convert VARDATA() to string
	 * ----------
	 */
	str = (char *) palloc(len + 1);    
	memcpy(str, VARDATA(fmt), len);
	*(str + len) = '\0'; 

	/* ----------
	 * Allocate result
	 * ----------
	 */
	result	= (text *) palloc( (len * DCH_MAX_ITEM_SIZ) + 1 + VARHDRSZ);	

	/* ----------
	 * Allocate new memory if format picture is bigger than static cache 
	 * and not use cache (call parser always) - flag=1 show this variant
	 * ----------
         */
	if ( len > DCH_CACHE_SIZE ) {

		format = (FormatNode *) palloc((len + 1) * sizeof(FormatNode));
		flag  = 1;

		parse_format(format, str, DCH_keywords, 
         		     DCH_suff, DCH_index, DCH_TYPE, NULL);
	
		(format + len)->type = NODE_TYPE_END;		/* Paranoa? */	
				
	} else {

		/* ----------
		 * Use cache buffers
		 * ----------
		 */
#ifdef DEBUG_TO_FROM_CHAR
		elog(DEBUG_elog_output, "DCH_TO_CHAR() Len:            %d",  len); 
		elog(DEBUG_elog_output, "DCH_TO_CHAR() Cache - str:   >%s<", CacheStr);
		elog(DEBUG_elog_output, "DCH_TO_CHAR() Arg.str:       >%s<", str);
#endif
		flag = 0;

       		if (strcmp(CacheStr, str) != 0) {
	
			/* ----------
			 * Can't use the cache, must run parser and save a original 
			 * format-picture string to the cache. 
			 * ----------
        		 */		
			strncpy(CacheStr, str, DCH_CACHE_SIZE);
				
#ifdef DEBUG_TO_FROM_CHAR	 
			/* dump_node(CacheFormat, len); */
			/* dump_index(DCH_keywords, DCH_index); */
#endif         	
         		parse_format(CacheFormat, str, DCH_keywords, 
         		     DCH_suff, DCH_index, DCH_TYPE, NULL);
         		
         		(CacheFormat + len)->type = NODE_TYPE_END;	/* Paranoa? */	
        	}

        	format = CacheFormat;
	}
	
	DCH_action(format, VARDATA(result), TO_CHAR);

	if (flag)
		pfree(format);

	pfree(str);
	VARSIZE(result) = strlen(VARDATA(result)) + VARHDRSZ; 
	return result;
}


/* -------------------
 * TIMESTAMP to_char()
 * -------------------
 */
text *
timestamp_to_char(time_t dt, text *fmt)
{
	return datetime_to_char( timestamp_datetime(dt), fmt);
}

/* ---------------------
 * TO_DATETIME()
 *
 * Make DateTime from date_str which is formated at argument 'fmt'  	 
 * ( to_datetime is reverse to_char() )
 * ---------------------
 */
DateTime *
to_datetime(text *date_str, text *fmt)
{
	static FormatNode	CacheFormat[ DCH_CACHE_SIZE +1];
	static char		CacheStr[ DCH_CACHE_SIZE +1];

	FormatNode		*format;
	int			flag=0;
	DateTime		*result;
	char			*str;
	int			len=0,
				fsec=0,
				tz=0;

	if ((!PointerIsValid(date_str)) || (!PointerIsValid(fmt)))
		return NULL;
	
	tm->tm_sec	=0;	tm->tm_year	=0;
	tm->tm_min	=0;	tm->tm_wday	=0;
	tm->tm_hour	=0;	tm->tm_yday	=0;
	tm->tm_mday	=1;	tm->tm_isdst	=0;
	tm->tm_mon	=1;
	
	result = palloc(sizeof(DateTime));
	
	len 	= VARSIZE(fmt) - VARHDRSZ; 
	
	if (len) { 
		
		/* ----------
		 * Convert VARDATA() to string
		 * ----------
		 */
		str = (char *) palloc(len + 1);    
		memcpy(str, VARDATA(fmt), len);
		*(str + len) = '\0'; 

		/* ----------
		 * Allocate new memory if format picture is bigger than static cache 
		 * and not use cache (call parser always) - flag=1 show this variant
		 * ----------
        	 */
		if ( len > DCH_CACHE_SIZE ) {

			format = (FormatNode *) palloc((len + 1) * sizeof(FormatNode));
			flag  = 1;

			parse_format(format, str, DCH_keywords, 
         			     DCH_suff, DCH_index, DCH_TYPE, NULL);
	
			(format + len)->type = NODE_TYPE_END;		/* Paranoa? */	
					
		} else {
	
			/* ----------
			 * Use cache buffers
			 * ----------
			 */
#ifdef DEBUG_TO_FROM_CHAR
			elog(DEBUG_elog_output, "DCH_TO_CHAR() Len:            %d",  len); 
			elog(DEBUG_elog_output, "DCH_TO_CHAR() Cache - str:   >%s<", CacheStr);
			elog(DEBUG_elog_output, "DCH_TO_CHAR() Arg.str:       >%s<", str);
#endif
			flag = 0;

	       		if (strcmp(CacheStr, str) != 0) {
		
				/* ----------
				 * Can't use the cache, must run parser and save a original 
				 * format-picture string to the cache. 
				 * ----------
	        		 */		
				strncpy(CacheStr, str, DCH_CACHE_SIZE);

	         		parse_format(CacheFormat, str, DCH_keywords, 
        	 		     DCH_suff, DCH_index, DCH_TYPE, NULL);
         		
        	 		(CacheFormat + len)->type = NODE_TYPE_END;	/* Paranoa? */	
        		}

        		format = CacheFormat;
		}
	
		/* ----------
		 * Call action for each node in FormatNode tree 
		 * ----------
		 */	 
#ifdef DEBUG_TO_FROM_CHAR	 
		/* dump_node(format, len); */
#endif
		VARDATA(date_str)[ VARSIZE(date_str) - VARHDRSZ ] = '\0';
		DCH_action(format, VARDATA(date_str), FROM_CHAR);	

		pfree(str);
		
		if (flag)
			pfree(format);
	}

#ifdef DEBUG_TO_FROM_CHAR
	NOTICE_TM; 
#endif
	if (IS_VALID_UTIME(tm->tm_year, tm->tm_mon, tm->tm_mday)) {

#ifdef USE_POSIX_TIME
		tm->tm_isdst = -1;
		tm->tm_year -= 1900;
			tm->tm_mon -= 1;

#ifdef DEBUG_TO_FROM_CHAR
		elog(DEBUG_elog_output, "TO-FROM_CHAR: Call mktime()");
		NOTICE_TM;
#endif
		mktime(tm);
		tm->tm_year += 1900;
		tm->tm_mon += 1;

#if defined(HAVE_TM_ZONE)
		tz = -(tm->tm_gmtoff);	/* tm_gmtoff is Sun/DEC-ism */
#elif defined(HAVE_INT_TIMEZONE)

#ifdef __CYGWIN__
		tz = (tm->tm_isdst ? (_timezone - 3600) : _timezone);
#else
		tz = (tm->tm_isdst ? (timezone - 3600) : timezone);
#endif

#else
#error USE_POSIX_TIME is defined but neither HAVE_TM_ZONE or HAVE_INT_TIMEZONE are defined
#endif

#else		/* !USE_POSIX_TIME */
		tz = CTimeZone;
#endif
	} else {
		tm->tm_isdst = 0;
		tz = 0;
	}
#ifdef DEBUG_TO_FROM_CHAR
	NOTICE_TM; 
#endif
	if (tm2datetime(tm, fsec, &tz, result) != 0)
        	elog(ERROR, "to_datatime: can't convert 'tm' to datetime.");
	
	return result;
}

/* ----------
 * TO_DATE
 * 	Make Date from date_str which is formated at argument 'fmt'  	 
 * ----------
 */
DateADT  
to_date(text *date_str, text *fmt)
{
	return datetime_date( to_datetime(date_str, fmt) );
}

/* ----------
 * TO_TIMESTAMP 
 * 	Make timestamp from date_str which is formated at argument 'fmt'  	 
 * ----------
 */
time_t  
to_timestamp(text *date_str, text *fmt)
{
	return datetime_timestamp( to_datetime(date_str, fmt) );
}

/**********************************************************************
 *  the NUMBER version part
 *********************************************************************/


static char *
fill_str(char *str, int c, int max)
{
	memset(str, c, max);
	*(str+max+1) = '\0';	
	return str;	
}


/* ----------
 * Cache routine for NUM to_char version
 * ----------
 */
static FormatNode *
NUM_cache( int len, char *CacheStr, FormatNode *CacheFormat, 
	   NUMDesc *CacheNum, NUMDesc *Num, char *pars_str, int *flag)
{	
	FormatNode 	*format;
	char		*str;

	/* ----------
	 * Convert VARDATA() to string
	 * ----------
	 */
	str = (char *) palloc(len + 1);    
	memcpy(str, pars_str, len);
	*(str + len) = '\0'; 

	/* ----------
	 * Allocate new memory if format picture is bigger than static cache 
	 * and not use cache (call parser always) - flag=1 show this variant
	 * ----------
         */
	if ( len > NUM_CACHE_SIZE ) {

		format = (FormatNode *) palloc((len + 1) * sizeof(FormatNode));
		*flag  = 1;
		
		Num->flag 		= 0;	
		Num->lsign		= 0;	
		Num->pre		= 0;	
		Num->pos		= 0;
         	Num->pre_lsign_num	= 0;
		
		parse_format(format, str, NUM_keywords, 
         			NULL, NUM_index, NUM_TYPE, Num);
	
		(format + len)->type = NODE_TYPE_END;		/* Paranoa? */	
		pfree(str);
		return format;
		
	} else {

		/* ----------
		 * Use cache buffer
		 * ----------
		 */
#ifdef DEBUG_TO_FROM_CHAR
		elog(DEBUG_elog_output, "NUM_TO_CHAR() Len:            %d",  len); 
		elog(DEBUG_elog_output, "NUM_TO_CHAR() Cache - str:   >%s<", CacheStr);
		elog(DEBUG_elog_output, "NUM_TO_CHAR() Arg.str:       >%s<", str);
#endif
		*flag = 0;

       		if (strcmp(CacheStr, str) != 0) {
	
			/* ----------
			 * Can't use the cache, must run parser and save a original 
			 * format-picture string to the cache. 
			 * ----------
        		 */		
			strncpy(CacheStr, str, NUM_CACHE_SIZE);
         	
         		/* ----------                                  	
         		 * Set zeros to CacheNum struct   	
         		 * ----------
         		 */                                 	
			CacheNum->flag 		= 0;	
			CacheNum->lsign		= 0;	
			CacheNum->pre		= 0;	
			CacheNum->pos		= 0;
         		CacheNum->pre_lsign_num	= 0;
        		CacheNum->need_locale	= 0;
        		CacheNum->multi		= 0; 	
         	
#ifdef DEBUG_TO_FROM_CHAR	 
			/* dump_node(CacheFormat, len); */
			/* dump_index(NUM_keywords, NUM_index); */
#endif         	
         		parse_format(CacheFormat, str, NUM_keywords, 
         			NULL, NUM_index, NUM_TYPE, CacheNum);
         		
         		(CacheFormat + len)->type = NODE_TYPE_END;	/* Paranoa? */	
        	}
       		/* ----------
		 * Copy cache to used struct
		 * ----------
		 */
		Num->flag 		= CacheNum->flag;
		Num->lsign		= CacheNum->lsign;
		Num->pre		= CacheNum->pre;
		Num->pos		= CacheNum->pos;
		Num->pre_lsign_num 	= CacheNum->pre_lsign_num;	
		Num->need_locale	= CacheNum->need_locale; 
		Num->multi		= CacheNum->multi; 	

		pfree(str);
		return CacheFormat;
	}
}


static char *
int_to_roman(int number)
{
	int	len	= 0, 
		num	= 0, 
		set	= 0;		
	char	*p	= NULL,
		*result,
		numstr[5];	

	result = (char *) palloc( 16 );
	*result = '\0';
	
	if (number > 3999 || number < 1) {
		fill_str(result, '#', 15);
		return result;
	}
	len = sprintf(numstr, "%d", number);
	
	for(p=numstr; *p!='\0'; p++, --len) {
		num = *p - 49; 			/* 48 ascii + 1 */
		if (num < 0)
			continue;
		if (num == -1 && set==0)
			continue;
		set = 1;
			
		if (len > 3) { 
			while(num-- != -1)
				strcat(result, "M");
		} else {
			if (len==3)	
				strcat(result, rm100[num]);	
			else if (len==2)	
				strcat(result, rm10[num]);
			else if (len==1)	
				strcat(result, rm1[num]);
		}
	}
	return result;
}



/* ----------
 * Locale
 * ----------
 */
static void
NUM_prepare_locale(NUMProc *Np)
{

#ifdef USE_LOCALE

	if (Np->Num->need_locale) {

		struct lconv	*lconv;

		/* ----------
		 * Get locales
		 * ----------
		 */
		lconv = PGLC_localeconv();
			
		/* ---------- 	
		 * Positive / Negative number sign
		 * ----------
		 */
		if (lconv->negative_sign && *lconv->negative_sign)
			Np->L_negative_sign = lconv->negative_sign;
		else
			Np->L_negative_sign = "-";
				
		if (lconv->positive_sign && *lconv->positive_sign)
			Np->L_positive_sign = lconv->positive_sign;
		else	
			Np->L_positive_sign = "+";	
		
		/* ----------
		 * Number thousands separator
		 * ----------
		 */	
		if (lconv->thousands_sep && *lconv->thousands_sep)
			Np->L_thousands_sep = lconv->thousands_sep;
		else	
			Np->L_thousands_sep = ",";
		
		/* ----------
		 * Number decimal point
		 * ----------
		 */
		if (lconv->decimal_point && *lconv->decimal_point)
			Np->decimal = lconv->decimal_point;
		else	
			Np->decimal = ".";
		
		/* ----------
		 * Currency symbol
		 * ----------
		 */
		if (lconv->currency_symbol && *lconv->currency_symbol)	
			Np->L_currency_symbol = lconv->currency_symbol;
		else	
			Np->L_currency_symbol = " ";	

	
		if (!IS_LDECIMAL(Np->Num))
			Np->decimal = ".";	
	} else {			  

#endif
		/* ----------
		 * Default values
		 * ----------
		 */
		Np->L_negative_sign	= "-"; 
		Np->L_positive_sign	= "+";
		Np->decimal	  	= ".";	
		Np->L_thousands_sep	= ",";
		Np->L_currency_symbol 	= " ";

#ifdef USE_LOCALE
	}
#endif
}

/* ----------
 * SET SIGN macro - set 'S' or 'PR' befor number 	
 * ----------
 */
#define SET_SIGN(_np) {								\
	if ((_np)->sign_wrote==0) {							\
		if (IS_BRACKET((_np)->Num)) {					\
										\
			*(_np)->inout_p = '<';					\
			++(_np)->inout_p;					\
			(_np)->sign_wrote = 1;					\
										\
		} else if ((_np)->Num->lsign==NUM_LSIGN_PRE && (! IS_BRACKET((_np)->Num)) &&	\
			(! IS_MINUS((_np)->Num)) && (! IS_PLUS((_np)->Num))) {	\
										\
			if ((_np)->sign=='-') 					\
				strcpy((_np)->inout_p, (_np)->L_negative_sign);	\
			else 							\
				strcpy((_np)->inout_p, (_np)->L_positive_sign);	\
			(_np)->inout_p += strlen((_np)->inout_p);		\
			(_np)->sign_wrote = 1;					\
										\
		} else {							\
			if ((_np)->sign=='-' && (_np)->Num->lsign==NUM_LSIGN_NONE \
				&& (! IS_BRACKET((_np)->Num)) && 		\
				(! IS_MINUS((_np)->Num)) && (! IS_PLUS((_np)->Num))) {	\
										\
				*(_np)->inout_p = '-';					\
				++(_np)->inout_p;						\
				(_np)->sign_wrote=1;					\
										\
			} else if ((! IS_FILLMODE((_np)->Num)) && (_np)->Num->lsign==NUM_LSIGN_NONE && \
				(! IS_BRACKET((_np)->Num)) && (! IS_MINUS((_np)->Num)) && (! IS_PLUS((_np)->Num))) {	\
										\
				*(_np)->inout_p = ' ';					\
				++(_np)->inout_p;						\
				(_np)->sign_wrote=1;					\
			}							\
		}								\
	}									\
}

/* ----------
 * Create number
 *	return FALSE if any work over current NUM_[0|9|D|DEC] must be skip
 *	in NUM_processor()   
 * ----------
 */
static int
NUM_numpart(NUMProc *Np, int id) 
{	
	if (IS_ROMAN(Np->Num))
		return FALSE;
			
	/* Note: in this elog() output not set '\0' in inout
	elog(DEBUG_elog_output, "sign_w: %d, count: %d, plen %d, sn: %s, inout: '%s'", 
		Np->sign_wrote, Np->count_num, Np->pre_number, Np->number_p, Np->inout);
	*/

	/* ----------
	 * Number extraction for TO_NUMBER()
	 * ----------
	 */
	if (Np->type == FROM_CHAR) {
		if (id == NUM_9 || id == NUM_0) {
			if (!*(Np->number+1)) {
				if (*Np->inout_p == '-' || 
				     (IS_BRACKET(Np->Num) && 
				      *Np->inout_p == '<' )) {
				
					*Np->number = '-';
					Np->inout_p++;
				
				} else if (*Np->inout_p == '+') {
				
					*Np->number = '+';
					Np->inout_p++;
				
				} else if (*Np->inout_p == ' ' && (! IS_FILLMODE(Np->Num))) {
					Np->inout_p++;
				} 
			}
			if (isdigit((unsigned char) *Np->inout_p)) {
				*Np->number_p = *Np->inout_p;
				Np->number_p++;
			} 
		} else {
			if (id == NUM_DEC) {
				*Np->number_p = '.';
				Np->number_p++;
				
			} else if (id == NUM_D) {
				
				int x = strlen(Np->decimal);
				
				if (!strncmp(Np->inout_p, Np->decimal, x)) {
					Np->inout_p += x-1;
					*Np->number_p = '.';
					Np->number_p++;
				}	                    
			}
		}	
		return TRUE;		
	}

	/* ----------
	 * Add blank space (pre number)
	 * ----------
	 */
	if ((! IS_ZERO(Np->Num)) && (! IS_FILLMODE(Np->Num)) && Np->pre_number > 0) { 
		*Np->inout_p = ' ';
		--Np->pre_number;
		--Np->count_num;
		return TRUE;
	} 
	
	/* ----------
	 * Add zero (pre number)
	 * ----------
	 */
	if (IS_ZERO(Np->Num) && Np->pre_number > 0) {

		SET_SIGN(Np);
		*Np->inout_p='0';
		--Np->pre_number;
		--Np->count_num;
		return TRUE;
	}
	
	if (IS_FILLMODE(Np->Num) && Np->pre_number > 0) { 
		--Np->pre_number;
		return FALSE;
	}
	
	/* ----------
	 * Add sign or '<' or ' '
	 * ----------
	 */
	if (Np->number_p==Np->number && Np->sign_wrote==0) {
		SET_SIGN(Np);			
	}	
	
	/* ----------
	 * Add decimal point
	 * ----------
	 */
	if (*Np->number_p=='.') {
		strcpy(Np->inout_p, Np->decimal);
		Np->inout_p += strlen(Np->inout_p);
		if (*Np->number_p)
			++Np->number_p;
		return FALSE;	
	}
	
	if (*Np->number_p) { 
	
		/* ----------
		 * Copy number string to inout string
		 * ----------
		 */
		*Np->inout_p = *Np->number_p;
		++Np->number_p;	
		
		Np->in_number = 1;
		
		/* ----------
		 * Number end (set post 'S' or '>' 
		 * ----------
		 */
		if ((--Np->count_num)==0) {
		
			if (IS_BRACKET(Np->Num)) {
				++Np->inout_p;
				*Np->inout_p 	= '>';
				Np->sign_wrote	=1;
			}
		
			if (Np->Num->lsign==NUM_LSIGN_POST && Np->sign_wrote==0) {
				++Np->inout_p;
				if (Np->sign=='-')
					strcpy(Np->inout_p, Np->L_negative_sign);
				else 
					strcpy(Np->inout_p, Np->L_positive_sign);
				Np->inout_p += strlen(Np->inout_p)-1;	
				Np->sign_wrote = 1;
			}
		}
	} else	
		return FALSE;
	return TRUE;
}
		
/* ----------
 *  Master function for NUMBER version.
 *  type (variant):
 *	TO_CHAR 		
 *		-> create formatted string by course of FormatNode 
 *
 *	FROM_CHAR (TO_NUMBER) 	
 *		-> extract number string from formatted string (reverse TO_CHAR)
 *		
 *		- ! this version use non-string 'inout' VARDATA(), 
 *		  and 'plen' is used for VARSIZE()
 *		- value 'sign' is not used	
 *		- number is creating in Np->number and first position (*Np->number)
 *		  in this buffer is reserved for +/- sign	
 * ----------
 */
static char *
NUM_processor (FormatNode *node, NUMDesc *Num, char *inout, char *number, 
					int plen, int sign, int type)
{
	FormatNode	*n;
	NUMProc		_Np, *Np = &_Np;
	
	Np->Num 	= Num;
	Np->type	= type;
	Np->number	= number;
	
	if (type == TO_CHAR) {
		Np->pre_number 	= plen;
		Np->sign	= sign;

	} else if (type == FROM_CHAR) {	
		Np->pre_number 	= 0; 
		*Np->number	= ' ';	/* sign space */	
		*(Np->number+1)	= '\0';	
		Np->sign	= 0;
	}	
	
	Np->inout	= inout;
	
	Np->sign_wrote	= Np->count_num	= Np->in_number	= 0;
	
	/* ---------- 
	 * Roman corection 
	 * ----------
	 */
	if (IS_ROMAN(Np->Num)) {		
		if (Np->type==FROM_CHAR)
			elog(ERROR, "to_number: RN is not supported");	
			
		Np->Num->lsign = Np->Num->pre_lsign_num = Np->Num->pos = 
		Np->Num->pre = Np->pre_number = Np->sign = 0;
		
		if (IS_FILLMODE(Np->Num)){
			Np->Num->flag = 0;
			Np->Num->flag |= NUM_F_FILLMODE;
		} else {
			Np->Num->flag = 0;
		}	
		Np->Num->flag |= NUM_F_ROMAN; 
	}

	/* ---------- 
	 * Check Num struct
	 * ----------
	 */
	if (Np->sign=='+')
		Np->Num->flag &= ~NUM_F_BRACKET;
	
	if (Np->Num->lsign == NUM_LSIGN_PRE && Np->Num->pre == Np->Num->pre_lsign_num)
		Np->Num->lsign = NUM_LSIGN_POST;

	/* set counter (number of all digits) */ 
	Np->count_num = Np->Num->pos + Np->Num->pre;
	
	if (Np->type==TO_CHAR && IS_FILLMODE(Np->Num) && (! IS_ZERO(Np->Num)))
		Np->count_num -= Np->pre_number;

#ifdef DEBUG_TO_FROM_CHAR	
	elog(DEBUG_elog_output, "NUM: '%s', PRE: %d, POS: %d, PLEN: %d, LSIGN: %d, DECIMAL: %d, MULTI: %d",
			Np->number, Np->Num->pre, Np->Num->pos, 
			Np->pre_number, Np->Num->lsign,IS_DECIMAL(Np->Num), 
			Np->Num->multi); 
#endif
	/* ----------
	 * Locale
	 * ----------
	 */
	NUM_prepare_locale(Np);

	/* need for DEBUG: memset(Np->inout, '\0', Np->count_num );*/

	/* ----------
	 * Processor direct cycle
	 * ----------
	 */
	if (Np->type == FROM_CHAR) 
		 Np->number_p=Np->number+1;  /* first char is space for sign */
	else if (Np->type == TO_CHAR)
		Np->number_p=Np->number;
		
	for(n=node, Np->inout_p=Np->inout; n->type != NODE_TYPE_END; n++) {
		
		if (Np->type == FROM_CHAR) {
			/* ----------
			 * Check non-string inout end
			 * ----------
			 */
			if (Np->inout_p == Np->inout + plen) 
				break;
		}
		
		/* ----------
		 * Format pictures actions
		 * ----------
		 */
		if (n->type == NODE_TYPE_ACTION) {
			
			/* ----------
			 * Create/reading digit/zero/blank/sing 
			 * ----------
			 */
			switch( n->key->id ) {
			
			case NUM_9:
			case NUM_0:
			case NUM_DEC:
			case NUM_D:
			
				if (NUM_numpart(Np, n->key->id))
					break; 				
				else
					continue;
			
			case NUM_COMMA:
				if (Np->type == TO_CHAR) {
					if (!Np->in_number) {
						if (IS_FILLMODE(Np->Num))
							continue;
						else
							*Np->inout_p= ' ';
					} else
						*Np->inout_p = ',';
				
				} else if (Np->type == FROM_CHAR) {
					if (!Np->in_number) {
						if (IS_FILLMODE(Np->Num))
							continue;
					}
				}
				break;
				
			case NUM_G:
				if (Np->type == TO_CHAR) {
					if (!Np->in_number) {
						if (IS_FILLMODE(Np->Num)) 
							continue;
						else {
							int	x = strlen(Np->L_thousands_sep);
							memset(Np->inout_p,  ' ', x);
							Np->inout_p += x-1;
						}
					} else {
						strcpy(Np->inout_p, Np->L_thousands_sep);
						Np->inout_p += strlen(Np->inout_p)-1;
					}
					
				} else if (Np->type == FROM_CHAR) {		
					if (!Np->in_number) {
						if (IS_FILLMODE(Np->Num)) 
							continue;
					}
					Np->inout_p += strlen(Np->L_thousands_sep)-1;
				}
				break;	
				
			case NUM_L:
				if (Np->type == TO_CHAR) {
					strcpy(Np->inout_p, Np->L_currency_symbol);
					Np->inout_p += strlen(Np->inout_p)-1;
				
				} else if (Np->type == FROM_CHAR) {
					Np->inout_p += strlen(Np->L_currency_symbol)-1;
				}
				break;		
				
			case NUM_MI:
				if (Np->sign_wrote==1 || IS_BRACKET(Np->Num)) {
					if (Np->type == TO_CHAR)
						continue; 
					else
						break;
				}	
				if (Np->type == TO_CHAR) {
					if (Np->sign=='-')
						*Np->inout_p = '-';
					else	
						*Np->inout_p = ' ';
						
				} else if (Np->type == FROM_CHAR) {
					if (*Np->inout_p == '-')
						*Np->number = '-';
				}
				Np->sign_wrote=1;
				break;
				
			case NUM_PL:
				if (Np->sign_wrote==1 || IS_BRACKET(Np->Num)) {
					if (Np->type == TO_CHAR)
						continue; 
					else
						break;
				}
				if (Np->type == TO_CHAR) {
					if (Np->sign=='+')
						*Np->inout_p = '+';
					else
						*Np->inout_p = ' ';
						
				}  else if (Np->type == FROM_CHAR) {
					if (*Np->inout_p == '+')
						*Np->number = '+';
				}
				Np->sign_wrote=1;       	
				break;	
				
			case NUM_SG:
				if (Np->sign_wrote==1 || IS_BRACKET(Np->Num)) {
					if (Np->type == TO_CHAR)
						continue; 
					else
						break;
				}	
				if (Np->type == TO_CHAR) 
					*Np->inout_p = Np->sign;
				
				else if (Np->type == FROM_CHAR) {
					if (*Np->inout_p == '-')
						*Np->number = '-';
					else if (*Np->inout_p == '+')
						*Np->number = '+';	
				}
				Np->sign_wrote=1;
				break;	
				
			case NUM_RN:
				if (Np->type == FROM_CHAR) 
					elog(ERROR, "TO_NUMBER: internal error #1");
					
				if (IS_FILLMODE(Np->Num)) {
					strcpy(Np->inout_p, Np->number_p);	
					Np->inout_p += strlen(Np->inout_p) - 1;
				} else 
					Np->inout_p += sprintf(Np->inout_p, "%15s", Np->number_p) -1;
				break;	
				
			case NUM_rn:
				if (Np->type == FROM_CHAR) 
					elog(ERROR, "TO_NUMBER: internal error #2");
				
				if (IS_FILLMODE(Np->Num)) {
					strcpy(Np->inout_p, str_tolower(Np->number_p));	
					Np->inout_p += strlen(Np->inout_p) - 1;
				} else 
					Np->inout_p += sprintf(Np->inout_p, "%15s", str_tolower(Np->number_p)) -1;
				break;		
				
			case NUM_th:
				if (IS_ROMAN(Np->Num) || *Np->number=='#' || 
				    Np->sign=='-' || IS_DECIMAL(Np->Num))
						continue; 
					
				if (Np->type == TO_CHAR)
					strcpy(Np->inout_p, get_th(Np->number, TH_LOWER));
				Np->inout_p += 1;
				break;
				
			case NUM_TH:
				if (IS_ROMAN(Np->Num) || *Np->number=='#' || 
				    Np->sign=='-' || IS_DECIMAL(Np->Num))
					continue;			
					
				if (Np->type == TO_CHAR)	
					strcpy(Np->inout_p, get_th(Np->number, TH_UPPER));
				Np->inout_p += 1;
				break;	
			
			case NUM_S:
				/* ----------
				 * 'S' for TO_CHAR is in NUM_numpart()
				 * ----------
				 */
				if (Np->type == FROM_CHAR && Np->sign_wrote==FALSE) {
					
					int x = strlen(Np->L_negative_sign);
					if (!strncmp(Np->inout_p, Np->L_negative_sign, x)) {
						Np->inout_p += x-1;
						*Np->number = '-';
						break;
					}
					
					x = strlen(Np->L_positive_sign);
					if (!strncmp(Np->inout_p, Np->L_positive_sign, x)) {
						Np->inout_p += x-1;
						*Np->number = '+';
						break;
					}
				} else
					continue;
				break;
			default:
				continue;
				break;
			}

		} else {
			/* ----------
			 * Remove to output char from input in TO_CHAR
			 * ----------
			 */
			if (Np->type == TO_CHAR)
				*Np->inout_p = n->character;
		}
		Np->inout_p++;	
	}
	
	if (Np->type == TO_CHAR) {
		*Np->inout_p = '\0';
		return Np->inout;
	
	} else if (Np->type == FROM_CHAR) {
		*Np->number_p = '\0';
#ifdef DEBUG_TO_FROM_CHAR
	        elog(DEBUG_elog_output, "TO_NUMBER (number): '%s'", Np->number);
#endif
		return Np->number;
	} else
		return NULL;	
}

/* ----------
 * MACRO: Start part of NUM - for all NUM's to_char variants
 *	(sorry, but I hate copy same code - macro is better..)
 * ----------
 */
#define NUM_TOCHAR_prepare {							\
	if (!PointerIsValid(fmt))					\
		return NULL;						\
									\
	len = VARSIZE(fmt) - VARHDRSZ; 					\
									\
	if (!len) 							\
		return textin("");					\
									\
	result	= (text *) palloc( (len * NUM_MAX_ITEM_SIZ) + 1 + VARHDRSZ);	\
	format = NUM_cache(len, CacheStr, CacheFormat, &CacheNum, &Num, \
			VARDATA(fmt), &flag);				\
}

/* ----------
 * MACRO: Finish part of NUM
 * ----------
 */
#define NUM_TOCHAR_finish {							\
									\
	NUM_processor(format, &Num, VARDATA(result), 			\
		numstr, plen, sign, TO_CHAR);				\
	pfree(orgnum);							\
									\
	if (flag)							\
		pfree(format);						\
									\
	VARSIZE(result) = strlen(VARDATA(result)) + VARHDRSZ; 	 	\
}

/* -------------------
 * NUMERIC to_number()
 * -------------------
 */
Numeric 
numeric_to_number(text *value, text *fmt)
{
	static FormatNode	CacheFormat[ NUM_CACHE_SIZE +1];
	static char		CacheStr[ NUM_CACHE_SIZE +1];
	static NUMDesc		CacheNum;
	
	NUMDesc			Num;
	FormatNode		*format;
	char 			*numstr;
	int			flag=0;
	int			len=0;
	
	int			scale, precision;
	
	if ((!PointerIsValid(value)) || (!PointerIsValid(fmt)))
		return NULL;									

	len = VARSIZE(fmt) - VARHDRSZ; 					
									
	if (!len) 							
		return numeric_in(NULL, 0, 0);					
									
	format = NUM_cache(len, CacheStr, CacheFormat, &CacheNum, &Num, 
			VARDATA(fmt), &flag);
	
	numstr	= (char *) palloc( (len * NUM_MAX_ITEM_SIZ) + 1);	
	
	NUM_processor(format, &Num, VARDATA(value), numstr, 
			VARSIZE(value) - VARHDRSZ, 0, FROM_CHAR);
	
	scale = Num.pos;
	precision = MAX(0, Num.pre) + scale;
	
	return numeric_in(numstr, 0, ((precision << 16) | scale) + VARHDRSZ);						
}

/* ------------------
 * NUMERIC to_char()
 * ------------------
 */	
text *
numeric_to_char(Numeric value, text *fmt)
{
	static FormatNode	CacheFormat[ NUM_CACHE_SIZE +1];
	static char		CacheStr[ NUM_CACHE_SIZE +1];
	static NUMDesc		CacheNum;
	
	NUMDesc			Num;
	FormatNode		*format;
	text 			*result;
	int			flag=0;
	int			len=0, plen=0, sign=0;
	char			*numstr, *orgnum, *p;

	NUM_TOCHAR_prepare;

	/* ----------
	 * On DateType depend part (numeric)
	 * ----------
	 */
	if (IS_ROMAN(&Num)) {
	 	numstr = orgnum = int_to_roman( numeric_int4( numeric_round(value, 0)));
	 	
	} else { 
		Numeric val = value;
	
		if (IS_MULTI(&Num)) {
			val = numeric_mul(value, 
				numeric_power(int4_numeric(10), int4_numeric(Num.multi))); 
			Num.pre += Num.multi;
		}
		
		orgnum = numeric_out( numeric_round(val, Num.pos) );
		if (*orgnum == '-') { 					/* < 0 */
			sign = '-';
			numstr = orgnum+1; 
		} else {
			sign = '+';
			numstr = orgnum;
		}
		if ((p = strchr(numstr, '.')))
			len = p - numstr; 
		else
			len = strlen(numstr);
		
		if (Num.pre > len) 
			plen = Num.pre - len;
			
		else if (len > Num.pre) {
			fill_str(numstr, '#', Num.pre);
			*(numstr + Num.pre) = '.';
			fill_str(numstr + 1 + Num.pre, '#', Num.pos);
		} 
	}	
	/*dump_node(format, VARSIZE(fmt) - VARHDRSZ);*/
	NUM_TOCHAR_finish;
	return result;
}

/* ---------------
 * INT4 to_char()
 * ---------------
 */	
text *
int4_to_char(int32 value, text *fmt)
{
	static FormatNode	CacheFormat[ NUM_CACHE_SIZE +1];
	static char		CacheStr[ NUM_CACHE_SIZE +1];
	static NUMDesc		CacheNum;
	
	NUMDesc			Num;
	FormatNode		*format;
	text 			*result;
	int			flag=0;
	int			len=0, plen=0, sign=0;
	char			*numstr, *orgnum;

	NUM_TOCHAR_prepare;

	/* ----------
	 * On DateType depend part (int32)
	 * ----------
	 */
	 if (IS_ROMAN(&Num)) {
	 	numstr = orgnum = int_to_roman( value );
	 	
	 } else {
		if (IS_MULTI(&Num)) {
			orgnum = int4out(int4mul(value, (int32) pow( (double)10, (double) Num.multi)));
			Num.pre += Num.multi;
		} else
			orgnum = int4out(value);
		len    = strlen(orgnum);
	
		if (*orgnum == '-') {  					/* < 0 */
			sign = '-';
			--len; 	
		} else
			sign = '+';
		
		if (Num.pos) {
			int	i;
			
			numstr = palloc( len + 1 + Num.pos ); 
			strcpy(numstr, orgnum + (*orgnum == '-' ? 1 : 0));
			*(numstr + len) = '.';	
			
			for(i=len+1; i<=Num.pos+len+1; i++)
				*(numstr+i) = '0';
			*(numstr + Num.pos + len + 1)  = '\0';
			pfree(orgnum);
			orgnum = numstr;  	 
		} else
			numstr = orgnum + (*orgnum == '-' ? 1 : 0);
	
		if (Num.pre > len) 
			plen = Num.pre - len;		
		else if (len > Num.pre) {
			fill_str(numstr, '#', Num.pre);
			*(numstr + Num.pre) = '.';
			fill_str(numstr + 1 + Num.pre, '#', Num.pos);
		}
	}

	/*dump_node(format, len);*/
	
	NUM_TOCHAR_finish;
	return result;
}

/* ---------------
 * INT8 to_char()
 * ---------------
 */	
text *
int8_to_char(int64 *value, text *fmt)
{
	static FormatNode	CacheFormat[ NUM_CACHE_SIZE +1];
	static char		CacheStr[ NUM_CACHE_SIZE +1];
	static NUMDesc		CacheNum;
	
	NUMDesc			Num;
	FormatNode		*format;
	text 			*result;
	int			flag=0;
	int			len=0, plen=0, sign=0;
	char			*numstr, *orgnum;

	NUM_TOCHAR_prepare;

	/* ----------
	 * On DateType depend part (int32)
	 * ----------
	 */
	if (IS_ROMAN(&Num)) {
	 	numstr = orgnum = int_to_roman( int84( value ));
	 	
	} else { 
		if (IS_MULTI(&Num)) {
			double multi = pow( (double)10, (double) Num.multi);
			orgnum = int8out( int8mul(value, dtoi8( (float64) &multi )));
			Num.pre += Num.multi;
		} else
			orgnum = int8out(value);
		len    = strlen(orgnum);
		
		if (*orgnum == '-') {  					/* < 0 */
			sign = '-';
			--len; 	
		} else
			sign = '+';
		
		if (Num.pos) {
			int	i;
			
			numstr = palloc( len + 1 + Num.pos ); 
			strcpy(numstr, orgnum + (*orgnum == '-' ? 1 : 0));
			*(numstr + len) = '.';	
			
			for(i=len+1; i<=Num.pos+len+1; i++)
				*(numstr+i) = '0';
			*(numstr + Num.pos + len + 1)  = '\0';
			pfree(orgnum);
			orgnum = numstr;  	 
		} else 
			numstr = orgnum + (*orgnum == '-' ? 1 : 0);
	
		if (Num.pre > len) 
			plen = Num.pre - len;		
		else if (len > Num.pre) {
			fill_str(numstr, '#', Num.pre);
			*(numstr + Num.pre) = '.';
			fill_str(numstr + 1 + Num.pre, '#', Num.pos);
		}	
	}
	
	NUM_TOCHAR_finish;
	return result;
}

/* -----------------
 * FLOAT4 to_char()
 * -----------------
 */	
text *
float4_to_char(float32 value, text *fmt)
{
	static FormatNode	CacheFormat[ NUM_CACHE_SIZE +1];
	static char		CacheStr[ NUM_CACHE_SIZE +1];
	static NUMDesc		CacheNum;
	
	NUMDesc			Num;
	FormatNode		*format;
	text 			*result;
	int			flag=0;
	int			len=0, plen=0, sign=0;
	char			*numstr, *orgnum, *p;

	NUM_TOCHAR_prepare;

	if (IS_ROMAN(&Num)) {
	 	numstr = orgnum = int_to_roman( (int) rint( *value ));
	 	
	} else {
		float32	val = value;
	
		if (IS_MULTI(&Num)) {
			float multi = pow( (double) 10, (double) Num.multi);
			val = float4mul(value, (float32) &multi);
			Num.pre += Num.multi;
		} 
		
		orgnum = (char *) palloc(MAXFLOATWIDTH + 1);
		len = sprintf(orgnum, "%.0f",  fabs(*val));
		if (Num.pre > len)
			plen = Num.pre - len;
		if (len >= FLT_DIG)
		        Num.pos = 0;
		else if (Num.pos + len > FLT_DIG)
		        Num.pos = FLT_DIG - len;
		sprintf(orgnum, "%.*f", Num.pos, *val);
		
		if (*orgnum == '-') { 					/* < 0 */
			sign = '-';
			numstr = orgnum+1; 
		} else {
			sign = '+';
			numstr = orgnum;
		}
		if ((p = strchr(numstr, '.')))
			len = p - numstr; 
		else	
			len = strlen(numstr);
			
		if (Num.pre > len) 
			plen = Num.pre - len;
			
		else if (len > Num.pre) {
			fill_str(numstr, '#', Num.pre);
			*(numstr + Num.pre) = '.';
			fill_str(numstr + 1 + Num.pre, '#', Num.pos);
		} 
	}
	
	NUM_TOCHAR_finish;
	return result;
}

/* -----------------
 * FLOAT8 to_char()
 * -----------------
 */	
text *
float8_to_char(float64 value, text *fmt)
{
	static FormatNode	CacheFormat[ NUM_CACHE_SIZE +1];
	static char		CacheStr[ NUM_CACHE_SIZE +1];
	static NUMDesc		CacheNum;
	
	NUMDesc			Num;
	FormatNode		*format;
	text 			*result;
	int			flag=0;
	int			len=0, plen=0, sign=0;
	char			*numstr, *orgnum, *p;

	NUM_TOCHAR_prepare;

	if (IS_ROMAN(&Num)) {
	 	numstr = orgnum = int_to_roman( (int) rint( *value ));
	 	
	} else {
		float64	val = value;
	
		if (IS_MULTI(&Num)) {
			double multi = pow( (double) 10, (double) Num.multi);
			val = float8mul(value, (float64) &multi);
			Num.pre += Num.multi;
		} 
		orgnum = (char *) palloc(MAXDOUBLEWIDTH + 1);
		len = sprintf(orgnum, "%.0f",  fabs(*val));
		if (Num.pre > len)
			plen = Num.pre - len;
		if (len >= DBL_DIG)
		        Num.pos = 0;
		else if (Num.pos + len > DBL_DIG)
		        Num.pos = DBL_DIG - len;
		sprintf(orgnum, "%.*f", Num.pos, *val);
	
		if (*orgnum == '-') { 					/* < 0 */
			sign = '-';
			numstr = orgnum+1; 
		} else {
			sign = '+';
			numstr = orgnum;
		}
		if ((p = strchr(numstr, '.')))
			len = p - numstr; 
		else
			len = strlen(numstr);
			
		if (Num.pre > len) 
			plen = Num.pre - len;
			
		else if (len > Num.pre) {
			fill_str(numstr, '#', Num.pre);
			*(numstr + Num.pre) = '.';
			fill_str(numstr + 1 + Num.pre, '#', Num.pos);
		} 
	}
	
	NUM_TOCHAR_finish;
	return result;
}
