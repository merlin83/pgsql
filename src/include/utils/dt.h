/*-------------------------------------------------------------------------
 *
 * dt.h--
 *    Definitions for the date/time and other date/time support code.
 *    The support code is shared with other date data types,
 *     including abstime, reltime, date, and time.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: dt.h,v 1.1 1997-03-14 23:33:23 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef DT_H
#define DT_H

#include <time.h>

/*
 * DateTime represents absolute time.
 * TimeSpan represents delta time.
 * Note that Postgres uses "time interval" to mean a bounded interval,
 *  consisting of a beginning and ending time, not a time span.
 */

typedef double DateTime;

typedef struct {
    double      time;   /* all time units other than months and years */
    int4        month;  /* months and years */
} TimeSpan;


#define USE_NEW_TIME_CODE	1
#define USE_NEW_DATE		0
#define USE_NEW_TIME		0

/* ----------------------------------------------------------------
 *		time types + support macros
 *
 * String definitions for standard time quantities.
 *
 * These strings are the defaults used to form output time strings.
 * Other alternate forms are hardcoded into token tables in dt.c.
 * ----------------------------------------------------------------
 */
#define DAGO		"ago"
#define DCURRENT	"current"
#define EPOCH		"epoch"
#define INVALID		"invalid"
#define EARLY		"-infinity"
#define LATE		"infinity"
#define NOW		"now"
#define TODAY		"today"
#define TOMORROW	"tomorrow"
#define YESTERDAY	"yesterday"
#define ZULU		"zulu"

#define DMICROSEC	"usecond"
#define DMILLISEC	"msecond"
#define DSECOND		"second"
#define DMINUTE		"minute"
#define DHOUR		"hour"
#define DDAY		"day"
#define DWEEK		"week"
#define DMONTH		"month"
#define DYEAR		"year"
#define DDECADE		"decade"
#define DCENTURY	"century"
#define DMILLENIUM	"millenium"
#define DA_D		"ad"
#define DB_C		"bc"

/*
 * Fundamental time field definitions for parsing.
 *
 *  Meridian:  am, pm, or 24-hour style.
 *  Millenium: ad, bc
 */
#define AM	0
#define PM	1
#define HR24	2

#define AD	0
#define BC	1

/*
 * Fields for time decoding.
 * Can't have more of these than there are bits in an unsigned int
 *  since these are turned into bit masks during parsing and decoding.
 */
#define RESERV	0
#define MONTH	1
#define YEAR	2
#define DAY	3
#define TIME	4
#define TZ	5
#define DTZ	6
#define IGNORE	7
#define AMPM	8
#define HOUR	9
#define MINUTE	10
#define SECOND	11
#define DOY	12
#define DOW	13
#define UNITS	14
#define ADBC	15
/* these are only for relative dates */
#define ABS_BEFORE	14
#define ABS_AFTER	15
#define AGO	16

/*
 * Token field definitions for time parsing and decoding.
 * These need to fit into the datetkn table type.
 * At the moment, that means keep them within [-127,127].
 */
#define DTK_NUMBER	0
#define DTK_STRING	1

#define DTK_DATETIME	2
#define DTK_DATE	3
#define DTK_TIME	4
#define DTK_TZ		5

#define DTK_SPECIAL	6
#define DTK_INVALID	7
#define DTK_CURRENT	8
#define DTK_EARLY	9
#define DTK_LATE	10
#define DTK_EPOCH	11
#define DTK_NOW		12
#define DTK_YESTERDAY	13
#define DTK_TODAY	14
#define DTK_TOMORROW	15
#define DTK_ZULU	16

#define DTK_DELTA	32
#define DTK_SECOND	33
#define DTK_MINUTE	34
#define DTK_HOUR	35
#define DTK_DAY		36
#define DTK_WEEK	37
#define DTK_MONTH	38
#define DTK_YEAR	39
#define DTK_DECADE	40
#define DTK_CENTURY	41
#define DTK_MILLENIUM	42
#define DTK_MILLISEC	43
#define DTK_MICROSEC	44
#define DTK_AGO		45

/*
 * Bit mask definitions for time parsing.
 */
#define DTK_M(t)	(0x01 << t)

#define DTK_DATE_M	(DTK_M(YEAR) | DTK_M(MONTH) | DTK_M(DAY))
#define DTK_TIME_M	(DTK_M(HOUR) | DTK_M(MINUTE) | DTK_M(SECOND))

#define MAXDATELEN	47	/* maximum possible length of an input date string */
#define MAXDATEFIELDS	25	/* maximum possible number of fields in a date string */
#define TOKMAXLEN	10	/* only this many chars are stored in datetktbl */

/* keep this struct small; it gets used a lot */
typedef struct {
#if defined(aix)
    char *token;
#else
    char token[TOKMAXLEN];
#endif /* aix */
    char type;
    char value;		/* this may be unsigned, alas */
} datetkn;


extern int EuroDates;
extern void GetCurrentTime(struct tm *tm);

/*
 * dt.c prototypes 
 */
extern DateTime *datetime_in( char *str);
extern char *datetime_out( DateTime *dt);
extern TimeSpan *timespan_in(char *str);
extern char *timespan_out(TimeSpan *span);

extern TimeSpan *datetime_sub(DateTime *dt1, DateTime *dt2);
extern DateTime *datetime_add_span(DateTime *dt, TimeSpan *span);
extern DateTime *datetime_sub_span(DateTime *dt, TimeSpan *span);
extern TimeSpan *timespan_add(TimeSpan *span1, TimeSpan *span2);
extern TimeSpan *timespan_sub(TimeSpan *span1, TimeSpan *span2);

extern DateTime dt2local( DateTime dt, int timezone);

extern void j2date( int jd, int *year, int *month, int *day);
extern int date2j( int year, int month, int day);
extern int j2day( int jd);

extern double time2t(const int hour, const int min, const double sec);
extern void dt2time(DateTime dt, int *hour, int *min, double *sec);

/*
extern void GetCurrentTime(struct tm *tm);
*/
extern int ParseDateTime( char *timestr, char *lowstr,
  char *field[], int ftype[], int maxfields, int *numfields);

extern int DecodeDateTime( char *field[], int ftype[],
 int nf, int *dtype, struct tm *tm, double *fsec, int *tzp);
extern int DecodeDate(char *str, int fmask, int *tmask, struct tm *tm);
extern int DecodeNumber( int flen, char *field,
 int fmask, int *tmask, struct tm *tm, double *fsec);
extern int DecodeNumberField( int len, char *str,
 int fmask, int *tmask, struct tm *tm, double *fsec);
extern int DecodeTime(char *str,
 int fmask, int *tmask, struct tm *tm, double *fsec);
extern int DecodeTimeOnly( char *field[], int ftype[], int nf,
 int *dtype, struct tm *tm, double *fsec);
extern int DecodeTimezone( char *str, int *tzp);
extern int DecodeSpecial(int field, char *lowtoken, int *val);

extern int DecodeDateDelta( char *field[], int ftype[],
 int nf, int *dtype, struct tm *tm, double *fsec);
extern int DecodeUnits(int field, char *lowtoken, int *val);

extern int EncodeSpecialDateTime(DateTime *dt, char *str);
extern int EncodePostgresDate(struct tm *tm, double fsec, char *str);
extern int EncodePostgresSpan(struct tm *tm, double fsec, char *str);


extern datetkn *datebsearch(char *key, datetkn *base, unsigned int nel);

#endif /* DT_H */
