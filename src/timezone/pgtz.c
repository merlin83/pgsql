/*-------------------------------------------------------------------------
 *
 * pgtz.c
 *	  Timezone Library Integration Functions
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/timezone/pgtz.c,v 1.21 2004/07/31 19:12:15 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#define NO_REDEFINE_TIMEFUNCS

#include "postgres.h"

#include <ctype.h>
#include <sys/stat.h>
#include <time.h>

#include "miscadmin.h"
#include "pgtime.h"
#include "pgtz.h"
#include "storage/fd.h"
#include "tzfile.h"
#include "utils/datetime.h"
#include "utils/elog.h"
#include "utils/guc.h"


static char tzdir[MAXPGPATH];
static int	done_tzdir = 0;

static const char *identify_system_timezone(void);


/*
 * Return full pathname of timezone data directory
 */
char *
pg_TZDIR(void)
{
	if (done_tzdir)
		return tzdir;

	get_share_path(my_exec_path, tzdir);
	strcat(tzdir, "/timezone");

	done_tzdir = 1;
	return tzdir;
}


/*
 * The following block of code attempts to determine which timezone in our
 * timezone database is the best match for the active system timezone.
 *
 * On most systems, we rely on trying to match the observable behavior of
 * the C library's localtime() function.  The database zone that matches
 * furthest into the past is the one to use.  Often there will be several
 * zones with identical rankings (since the zic database assigns multiple
 * names to many zones).  We break ties arbitrarily by preferring shorter,
 * then alphabetically earlier zone names.
 *
 * Win32's native knowledge about timezones appears to be too incomplete
 * and too different from the zic database for the above matching strategy
 * to be of any use. But there is just a limited number of timezones
 * available, so we can rely on a handmade mapping table instead.
 */

#ifndef WIN32

#define T_DAY	((time_t) (60*60*24))
#define T_WEEK  ((time_t) (60*60*24*7))
#define T_MONTH ((time_t) (60*60*24*31))

#define MAX_TEST_TIMES (52*100)	/* 100 years, or 1904..2004 */

struct tztry
{
	int			n_test_times;
	time_t		test_times[MAX_TEST_TIMES];
};

static void scan_available_timezones(char *tzdir, char *tzdirsub,
									 struct tztry *tt,
									 int *bestscore, char *bestzonename);


/*
 * Get GMT offset from a system struct tm
 */
static int
get_timezone_offset(struct tm *tm)
{
#if defined(HAVE_STRUCT_TM_TM_ZONE)
	return tm->tm_gmtoff;
#elif defined(HAVE_INT_TIMEZONE)
#ifdef HAVE_UNDERSCORE_TIMEZONE
	return -_timezone;
#else
	return -timezone;
#endif
#else
#error No way to determine TZ? Can this happen?
#endif
}

/*
 * Convenience subroutine to convert y/m/d to time_t (NOT pg_time_t)
 */
static time_t
build_time_t(int year, int month, int day)
{
	struct tm	tm;

	memset(&tm, 0, sizeof(tm));
	tm.tm_mday = day;
	tm.tm_mon = month - 1;
	tm.tm_year = year - 1900;

	return mktime(&tm);
}

/*
 * Does a system tm value match one we computed ourselves?
 */
static bool
compare_tm(struct tm *s, struct pg_tm *p)
{
	if (s->tm_sec != p->tm_sec ||
		s->tm_min != p->tm_min ||
		s->tm_hour != p->tm_hour ||
		s->tm_mday != p->tm_mday ||
		s->tm_mon != p->tm_mon ||
		s->tm_year != p->tm_year ||
		s->tm_wday != p->tm_wday ||
		s->tm_yday != p->tm_yday ||
		s->tm_isdst != p->tm_isdst)
		return false;
	return true;
}

/*
 * See how well a specific timezone setting matches the system behavior
 *
 * We score a timezone setting according to the number of test times it
 * matches.  (The test times are ordered later-to-earlier, but this routine
 * doesn't actually know that; it just scans until the first non-match.)
 *
 * We return -1 for a completely unusable setting; this is worse than the
 * score of zero for a setting that works but matches not even the first
 * test time.
 */
static int
score_timezone(const char *tzname, struct tztry *tt)
{
	int			i;
	pg_time_t	pgtt;
	struct tm	   *systm;
	struct pg_tm   *pgtm;
	char		cbuf[TZ_STRLEN_MAX + 1];

	if (!pg_tzset(tzname))
		return -1;				/* can't handle the TZ name at all */

	/* Reject if leap seconds involved */
	if (!tz_acceptable())
	{
		elog(DEBUG4, "Reject TZ \"%s\": uses leap seconds", tzname);
		return -1;
	}

	/* Check for match at all the test times */
	for (i = 0; i < tt->n_test_times; i++)
	{
		pgtt = (pg_time_t) (tt->test_times[i]);
		pgtm = pg_localtime(&pgtt);
		if (!pgtm)
			return -1;		/* probably shouldn't happen */
		systm = localtime(&(tt->test_times[i]));
		if (!systm)
		{
			elog(DEBUG4, "TZ \"%s\" scores %d: at %ld %04d-%02d-%02d %02d:%02d:%02d %s, system had no data",
				 tzname, i, (long) pgtt,
				 pgtm->tm_year + 1900, pgtm->tm_mon + 1, pgtm->tm_mday,
				 pgtm->tm_hour, pgtm->tm_min, pgtm->tm_sec,
				 pgtm->tm_isdst ? "dst" : "std");
			return i;
		}
		if (!compare_tm(systm, pgtm))
		{
			elog(DEBUG4, "TZ \"%s\" scores %d: at %ld %04d-%02d-%02d %02d:%02d:%02d %s versus %04d-%02d-%02d %02d:%02d:%02d %s",
				 tzname, i, (long) pgtt,
				 pgtm->tm_year + 1900, pgtm->tm_mon + 1, pgtm->tm_mday,
				 pgtm->tm_hour, pgtm->tm_min, pgtm->tm_sec,
				 pgtm->tm_isdst ? "dst" : "std",
				 systm->tm_year + 1900, systm->tm_mon + 1, systm->tm_mday,
				 systm->tm_hour, systm->tm_min, systm->tm_sec,
				 systm->tm_isdst ? "dst" : "std");
			return i;
		}
		if (systm->tm_isdst >= 0)
		{
			/* Check match of zone names, too */
			if (pgtm->tm_zone == NULL)
				return -1;		/* probably shouldn't happen */
			memset(cbuf, 0, sizeof(cbuf));
			strftime(cbuf, sizeof(cbuf) - 1, "%Z", systm); /* zone abbr */
			if (strcmp(cbuf, pgtm->tm_zone) != 0)
			{
				elog(DEBUG4, "TZ \"%s\" scores %d: at %ld \"%s\" versus \"%s\"",
					 tzname, i, (long) pgtt,
					 pgtm->tm_zone, cbuf);
				return i;
			}
		}
	}

	elog(DEBUG4, "TZ \"%s\" gets max score %d", tzname, i);
	return i;
}


/*
 * Try to identify a timezone name (in our terminology) that best matches the
 * observed behavior of the system timezone library.  We cannot assume that
 * the system TZ environment setting (if indeed there is one) matches our
 * terminology, so we ignore it and just look at what localtime() returns.
 */
static const char *
identify_system_timezone(void)
{
	static char resultbuf[TZ_STRLEN_MAX + 1];
	time_t		tnow;
	time_t		t;
	struct tztry tt;
	struct tm  *tm;
	int			bestscore;
	char		tmptzdir[MAXPGPATH];
	int			std_ofs;
	char		std_zone_name[TZ_STRLEN_MAX + 1],
				dst_zone_name[TZ_STRLEN_MAX + 1];
	char		cbuf[TZ_STRLEN_MAX + 1];

	/* Initialize OS timezone library */
	tzset();

	/*
	 * Set up the list of dates to be probed to see how well our timezone
	 * matches the system zone.  We first probe January and July of 2004;
	 * this serves to quickly eliminate the vast majority of the TZ database
	 * entries.  If those dates match, we probe every week from 2004 backwards
	 * to late 1904.  (Weekly resolution is good enough to identify DST
	 * transition rules, since everybody switches on Sundays.)  The further
	 * back the zone matches, the better we score it.  This may seem like
	 * a rather random way of doing things, but experience has shown that
	 * system-supplied timezone definitions are likely to have DST behavior
	 * that is right for the recent past and not so accurate further back.
	 * Scoring in this way allows us to recognize zones that have some
	 * commonality with the zic database, without insisting on exact match.
	 * (Note: we probe Thursdays, not Sundays, to avoid triggering
	 * DST-transition bugs in localtime itself.)
	 */
	tt.n_test_times = 0;
	tt.test_times[tt.n_test_times++] = build_time_t(2004, 1, 15);
	tt.test_times[tt.n_test_times++] = t = build_time_t(2004, 7, 15);
	while (tt.n_test_times < MAX_TEST_TIMES)
	{
		t -= T_WEEK;
		tt.test_times[tt.n_test_times++] = t;
	}

	/* Search for the best-matching timezone file */
	strcpy(tmptzdir, pg_TZDIR());
	bestscore = -1;
	resultbuf[0] = '\0';
	scan_available_timezones(tmptzdir, tmptzdir + strlen(tmptzdir) + 1,
							 &tt,
							 &bestscore, resultbuf);
	if (bestscore > 0)
		return resultbuf;

	/*
	 * Couldn't find a match in the database, so next we try constructed zone
	 * names (like "PST8PDT").
	 *
	 * First we need to determine the names of the local standard and daylight
	 * zones.  The idea here is to scan forward from today until we have
	 * seen both zones, if both are in use.
	 */
	memset(std_zone_name, 0, sizeof(std_zone_name));
	memset(dst_zone_name, 0, sizeof(dst_zone_name));
	std_ofs = 0;

	tnow = time(NULL);

	/*
	 * Round back to a GMT midnight so results don't depend on local time
	 * of day
	 */
	tnow -= (tnow % T_DAY);

	/*
	 * We have to look a little further ahead than one year, in case today
	 * is just past a DST boundary that falls earlier in the year than the
	 * next similar boundary.  Arbitrarily scan up to 14 months.
	 */
	for (t = tnow; t <= tnow + T_MONTH * 14; t += T_MONTH)
	{
		tm = localtime(&t);
		if (!tm)
			continue;
		if (tm->tm_isdst < 0)
			continue;
		if (tm->tm_isdst == 0 && std_zone_name[0] == '\0')
		{
			/* found STD zone */
			memset(cbuf, 0, sizeof(cbuf));
			strftime(cbuf, sizeof(cbuf) - 1, "%Z", tm); /* zone abbr */
			strcpy(std_zone_name, cbuf);
			std_ofs = get_timezone_offset(tm);
		}
		if (tm->tm_isdst > 0 && dst_zone_name[0] == '\0')
		{
			/* found DST zone */
			memset(cbuf, 0, sizeof(cbuf));
			strftime(cbuf, sizeof(cbuf) - 1, "%Z", tm); /* zone abbr */
			strcpy(dst_zone_name, cbuf);
		}
		/* Done if found both */
		if (std_zone_name[0] && dst_zone_name[0])
			break;
	}

	/* We should have found a STD zone name by now... */
	if (std_zone_name[0] == '\0')
	{
		ereport(LOG,
				(errmsg("unable to determine system timezone, defaulting to \"%s\"", "GMT"),
				 errhint("You can specify the correct timezone in postgresql.conf.")));
		return NULL;			/* go to GMT */
	}

	/* If we found DST then try STD<ofs>DST */
	if (dst_zone_name[0] != '\0')
	{
		snprintf(resultbuf, sizeof(resultbuf), "%s%d%s",
				 std_zone_name, -std_ofs / 3600, dst_zone_name);
		if (score_timezone(resultbuf, &tt) > 0)
			return resultbuf;
	}

	/* Try just the STD timezone (works for GMT at least) */
	strcpy(resultbuf, std_zone_name);
	if (score_timezone(resultbuf, &tt) > 0)
		return resultbuf;

	/* Try STD<ofs> */
	snprintf(resultbuf, sizeof(resultbuf), "%s%d",
			 std_zone_name, -std_ofs / 3600);
	if (score_timezone(resultbuf, &tt) > 0)
		return resultbuf;

	/*
	 * Did not find the timezone.  Fallback to use a GMT zone.  Note that the
	 * zic timezone database names the GMT-offset zones in POSIX style: plus
	 * is west of Greenwich.  It's unfortunate that this is opposite of SQL
	 * conventions.  Should we therefore change the names?  Probably not...
	 */
	snprintf(resultbuf, sizeof(resultbuf), "Etc/GMT%s%d",
			(-std_ofs > 0) ? "+" : "", -std_ofs / 3600);

	ereport(LOG,
			(errmsg("could not recognize system timezone, defaulting to \"%s\"",
					resultbuf),
			 errhint("You can specify the correct timezone in postgresql.conf.")));
	return resultbuf;
}

/*
 * Recursively scan the timezone database looking for the best match to
 * the system timezone behavior.
 *
 * tzdir points to a buffer of size MAXPGPATH.  On entry, it holds the
 * pathname of a directory containing TZ files.  We internally modify it
 * to hold pathnames of sub-directories and files, but must restore it
 * to its original contents before exit.
 *
 * tzdirsub points to the part of tzdir that represents the subfile name
 * (ie, tzdir + the original directory name length, plus one for the
 * first added '/').
 *
 * tt tells about the system timezone behavior we need to match.
 *
 * *bestscore and *bestzonename on entry hold the best score found so far
 * and the name of the best zone.  We overwrite them if we find a better
 * score.  bestzonename must be a buffer of length TZ_STRLEN_MAX + 1.
 */
static void
scan_available_timezones(char *tzdir, char *tzdirsub, struct tztry *tt,
						 int *bestscore, char *bestzonename)
{
	int			tzdir_orig_len = strlen(tzdir);
	DIR		   *dirdesc;

	dirdesc = AllocateDir(tzdir);
	if (!dirdesc)
	{
		ereport(LOG,
				(errcode_for_file_access(),
				 errmsg("could not open directory \"%s\": %m", tzdir)));
		return;
	}

	for (;;)
	{
		struct dirent *direntry;
		struct stat statbuf;

		errno = 0;
		direntry = readdir(dirdesc);
		if (!direntry)
		{
			if (errno)
				ereport(LOG,
						(errcode_for_file_access(),
						 errmsg("error reading directory: %m")));
			break;
		}

		/* Ignore . and .., plus any other "hidden" files */
		if (direntry->d_name[0] == '.')
			continue;

		snprintf(tzdir + tzdir_orig_len, MAXPGPATH - tzdir_orig_len,
				 "/%s", direntry->d_name);

		if (stat(tzdir, &statbuf) != 0)
		{
			ereport(LOG,
					(errcode_for_file_access(),
					 errmsg("could not stat \"%s\": %m", tzdir)));
			continue;
		}

		if (S_ISDIR(statbuf.st_mode))
		{
			/* Recurse into subdirectory */
			scan_available_timezones(tzdir, tzdirsub, tt,
									 bestscore, bestzonename);
		}
		else
		{
			/* Load and test this file */
			int score = score_timezone(tzdirsub, tt);

			if (score > *bestscore)
			{
				*bestscore = score;
				StrNCpy(bestzonename, tzdirsub, TZ_STRLEN_MAX + 1);
			}
			else if (score == *bestscore)
			{
				/* Consider how to break a tie */
				if (strlen(tzdirsub) < strlen(bestzonename) ||
					(strlen(tzdirsub) == strlen(bestzonename) &&
					 strcmp(tzdirsub, bestzonename) < 0))
					StrNCpy(bestzonename, tzdirsub, TZ_STRLEN_MAX + 1);
			}
		}
	}

	FreeDir(dirdesc);

	/* Restore tzdir */
	tzdir[tzdir_orig_len] = '\0';
}

#else /* WIN32 */

static const struct {
	const char *stdname;  /* Windows name of standard timezone */
	const char *dstname;  /* Windows name of daylight timezone */
	const char *pgtzname; /* Name of pgsql timezone to map to */
} win32_tzmap[] = {
	/*
	 * This list was built from the contents of the registry at
	 *  HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Time Zones
	 * on Windows XP Professional SP1
	 *
	 * The zones have been matched to zic timezones by looking at the cities
	 * listed in the win32 display name (in the comment here) in most cases.
	 */
	{"Afghanistan Standard Time", "Afghanistan Daylight Time",
	 "Asia/Kabul"},				/* (GMT+04:30) Kabul */
	{"Alaskan Standard Time", "Alaskan Daylight Time",
	 "US/Alaska"},				/* (GMT-09:00) Alaska */
	{"Arab Standard Time", "Arab Daylight Time",
	 "Asia/Kuwait"},			/* (GMT+03:00) Kuwait, Riyadh */
	{"Arabian Standard Time", "Arabian Daylight Time",
	 "Asia/Muscat"},			/* (GMT+04:00) Abu Dhabi, Muscat */
	{"Arabic Standard Time", "Arabic Daylight Time",
	 "Asia/Baghdad"},			/* (GMT+03:00) Baghdad */
	{"Atlantic Standard Time", "Atlantic Daylight Time",
	 "Canada/Atlantic"},		/* (GMT-04:00) Atlantic Time (Canada) */
	{"AUS Central Standard Time", "AUS Central Daylight Time",
	 "Australia/Darwin"},		/* (GMT+09:30) Darwin */
	{"AUS Eastern Standard Time", "AUS Eastern Daylight Time",
	 "Australia/Canberra"},		/* (GMT+10:00) Canberra, Melbourne, Sydney */
	{"Azores Standard Time", "Azores Daylight Time",
	 "Atlantic/Azores"},		/* (GMT-01:00) Azores */
	{"Canada Central Standard Time", "Canada Central Daylight Time",
	 "Canada/Saskatchewan"},	/* (GMT-06:00) Saskatchewan */
	{"Cape Verde Standard Time", "Cape Verde Daylight Time",
	 "Atlantic/Cape_Verde"},	/* (GMT-01:00) Cape Verde Is. */
	{"Caucasus Standard Time", "Caucasus Daylight Time",
	 "Asia/Baku"},				/* (GMT+04:00) Baku, Tbilisi, Yerevan */
	{"Cen. Australia Standard Time", "Cen. Australia Daylight Time",
	 "Australia/Adelaide"},		/* (GMT+09:30) Adelaide */
	{"Central America Standard Time", "Central America Daylight Time",
	 "CST6CDT"},				/* (GMT-06:00) Central America */
	{"Central Asia Standard Time", "Central Asia Daylight Time",
	 "Asia/Dhaka"},				/* (GMT+06:00) Astana, Dhaka */
	{"Central Europe Standard Time", "Central Europe Daylight Time",
	 "Europe/Belgrade"},		/* (GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague */
	{"Central European Standard Time", "Central European Daylight Time",
	 "Europe/Sarajevo"},		/* (GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb */
	{"Central Pacific Standard Time", "Central Pacific Daylight Time",
	 "Pacific/Noumea"},			/* (GMT+11:00) Magadan, Solomon Is., New Caledonia */
	{"Central Standard Time", "Central Daylight Time",
	 "US/Central"},				/* (GMT-06:00) Central Time (US & Canada) */
	{"China Standard Time", "China Daylight Time",
	 "Asia/Hong_Kong"},			/* (GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi */
	{"Dateline Standard Time", "Dateline Daylight Time",
	 "Etc/GMT+12"},				/* (GMT-12:00) International Date Line West */
	{"E. Africa Standard Time", "E. Africa Daylight Time",
	 "Africa/Nairobi"},			/* (GMT+03:00) Nairobi */
	{"E. Australia Standard Time", "E. Australia Daylight Time",
	 "Australia/Brisbane"},		/* (GMT+10:00) Brisbane */
	{"E. Europe Standard Time", "E. Europe Daylight Time",
	 "Europe/Bucharest"},		/* (GMT+02:00) Bucharest */
	{"E. South America Standard Time", "E. South America Daylight Time",
	 "America/Araguaina"},		/* (GMT-03:00) Brasilia */
	{"Eastern Standard Time", "Eastern Daylight Time",
	 "US/Eastern"},				/* (GMT-05:00) Eastern Time (US & Canada) */
	{"Egypt Standard Time", "Egypt Daylight Time",
	 "Africa/Cairo"},			/* (GMT+02:00) Cairo */
	{"Ekaterinburg Standard Time", "Ekaterinburg Daylight Time",
	 "Asia/Yekaterinburg"},		/* (GMT+05:00) Ekaterinburg */
	{"Fiji Standard Time", "Fiji Daylight Time",
	 "Pacific/Fiji"},			/* (GMT+12:00) Fiji, Kamchatka, Marshall Is. */
	{"FLE Standard Time", "FLE Daylight Time",
	 "Europe/Helsinki"},		/* (GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius */
	{"GMT Standard Time", "GMT Daylight Time",
	 "Europe/Dublin"},			/* (GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London */
	{"Greenland Standard Time", "Greenland Daylight Time",
	 "America/Godthab"},		/* (GMT-03:00) Greenland */
	{"Greenwich Standard Time", "Greenwich Daylight Time",
	 "Africa/Casablanca"},		/* (GMT) Casablanca, Monrovia */
	{"GTB Standard Time", "GTB Daylight Time",
	 "Europe/Athens"},			/* (GMT+02:00) Athens, Istanbul, Minsk */
	{"Hawaiian Standard Time", "Hawaiian Daylight Time",
	 "US/Hawaii"},				/* (GMT-10:00) Hawaii */
	{"India Standard Time", "India Daylight Time",
	 "Asia/Calcutta"},			/* (GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi */
	{"Iran Standard Time", "Iran Daylight Time",
	 "Asia/Tehran"},			/* (GMT+03:30) Tehran */
	{"Jerusalem Standard Time", "Jerusalem Daylight Time",
	 "Asia/Jerusalem"},			/* (GMT+02:00) Jerusalem */
	{"Korea Standard Time", "Korea Daylight Time",
	 "Asia/Seoul"},				/* (GMT+09:00) Seoul */
	{"Mexico Standard Time", "Mexico Daylight Time",
	 "America/Mexico_City"},	/* (GMT-06:00) Guadalajara, Mexico City, Monterrey */
	{"Mexico Standard Time", "Mexico Daylight Time",
	 "America/La_Paz"},			/* (GMT-07:00) Chihuahua, La Paz, Mazatlan */
	{"Mid-Atlantic Standard Time", "Mid-Atlantic Daylight Time",
	 "Atlantic/South_Georgia"}, /* (GMT-02:00) Mid-Atlantic */
	{"Mountain Standard Time", "Mountain Daylight Time",
	 "US/Mountain"},			/* (GMT-07:00) Mountain Time (US & Canada) */
	{"Myanmar Standard Time", "Myanmar Daylight Time",
	 "Asia/Rangoon"},			/* (GMT+06:30) Rangoon */
	{"N. Central Asia Standard Time", "N. Central Asia Daylight Time",
	 "Asia/Almaty"},			/* (GMT+06:00) Almaty, Novosibirsk */
	{"Nepal Standard Time", "Nepal Daylight Time",
	 "Asia/Katmandu"},			/* (GMT+05:45) Kathmandu */
	{"New Zealand Standard Time", "New Zealand Daylight Time",
	 "Pacific/Auckland"},		/* (GMT+12:00) Auckland, Wellington */
	{"Newfoundland Standard Time", "Newfoundland Daylight Time",
	 "Canada/Newfoundland"},	/* (GMT-03:30) Newfoundland */
	{"North Asia East Standard Time", "North Asia East Daylight Time",
	 "Asia/Irkutsk"},			/* (GMT+08:00) Irkutsk, Ulaan Bataar */
	{"North Asia Standard Time", "North Asia Daylight Time",
	 "Asia/Krasnoyarsk"},		/* (GMT+07:00) Krasnoyarsk */
	{"Pacific SA Standard Time", "Pacific SA Daylight Time",
	 "America/Santiago"},		/* (GMT-04:00) Santiago */
	{"Pacific Standard Time", "Pacific Daylight Time",
	 "US/Pacific"},				/* (GMT-08:00) Pacific Time (US & Canada); Tijuana */
	{"Romance Standard Time", "Romance Daylight Time",
	 "Europe/Brussels"},		/* (GMT+01:00) Brussels, Copenhagen, Madrid, Paris */
	{"Russian Standard Time", "Russian Daylight Time",
	 "Europe/Moscow"},			/* (GMT+03:00) Moscow, St. Petersburg, Volgograd */
	{"SA Eastern Standard Time", "SA Eastern Daylight Time",
	 "America/Buenos_Aires"},	/* (GMT-03:00) Buenos Aires, Georgetown */
	{"SA Pacific Standard Time", "SA Pacific Daylight Time",
	 "America/Bogota"},			/* (GMT-05:00) Bogota, Lima, Quito */
	{"SA Western Standard Time", "SA Western Daylight Time",
	 "America/Caracas"},		/* (GMT-04:00) Caracas, La Paz */
	{"Samoa Standard Time", "Samoa Daylight Time",
	 "Pacific/Midway"},			/* (GMT-11:00) Midway Island, Samoa */
	{"SE Asia Standard Time", "SE Asia Daylight Time",
	 "Asia/Bangkok"},			/* (GMT+07:00) Bangkok, Hanoi, Jakarta */
	{"Malay Peninsula Standard Time", "Malay Peninsula Daylight Time",
	 "Asia/Kuala_Lumpur"},		/* (GMT+08:00) Kuala Lumpur, Singapore */
	{"South Africa Standard Time", "South Africa Daylight Time",
	 "Africa/Harare"},			/* (GMT+02:00) Harare, Pretoria */
	{"Sri Lanka Standard Time", "Sri Lanka Daylight Time",
	 "Asia/Colombo"},			/* (GMT+06:00) Sri Jayawardenepura */
	{"Taipei Standard Time", "Taipei Daylight Time",
	 "Asia/Taipei"},			/* (GMT+08:00) Taipei */
	{"Tasmania Standard Time", "Tasmania Daylight Time",
	 "Australia/Hobart"},		/* (GMT+10:00) Hobart */
	{"Tokyo Standard Time", "Tokyo Daylight Time",
	 "Asia/Tokyo"},				/* (GMT+09:00) Osaka, Sapporo, Tokyo */
	{"Tonga Standard Time", "Tonga Daylight Time",
	 "Pacific/Tongatapu"},		/* (GMT+13:00) Nuku'alofa */
	{"US Eastern Standard Time", "US Eastern Daylight Time",
	 "US/Eastern"},				/* (GMT-05:00) Indiana (East) */
	{"US Mountain Standard Time", "US Mountain Daylight Time",
	 "US/Arizona"},				/* (GMT-07:00) Arizona */
	{"Vladivostok Standard Time", "Vladivostok Daylight Time",
	 "Asia/Vladivostok"},		/* (GMT+10:00) Vladivostok */
	{"W. Australia Standard Time", "W. Australia Daylight Time",
	 "Australia/Perth"},		/* (GMT+08:00) Perth */
/*	{"W. Central Africa Standard Time", "W. Central Africa Daylight Time",
	""}, Could not find a match for this one. Excluded for now. */  /* (GMT+01:00) West Central Africa */
	{"W. Europe Standard Time", "W. Europe Daylight Time",
	 "CET"},					/* (GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna */
	{"West Asia Standard Time", "West Asia Daylight Time",
	 "Asia/Karachi"},			/* (GMT+05:00) Islamabad, Karachi, Tashkent */
	{"West Pacific Standard Time", "West Pacific Daylight Time",
	 "Pacific/Guam"},			/* (GMT+10:00) Guam, Port Moresby */
	{"Yakutsk Standard Time", "Yakutsk Daylight Time",
	 "Asia/Yakutsk"},			/* (GMT+09:00) Yakutsk */
	{NULL, NULL, NULL}
};

static const char *
identify_system_timezone(void)
{
	int i;
	char tzname[128];
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	if (!tm)
	{
		ereport(WARNING,
				(errmsg_internal("could not determine current date/time: localtime failed")));
		return NULL;
	}

	memset(tzname, 0, sizeof(tzname));
	strftime(tzname, sizeof(tzname)-1, "%Z", tm);

	for (i=0; win32_tzmap[i].stdname != NULL; i++)
	{
		if (strcmp(tzname, win32_tzmap[i].stdname) == 0 ||
			strcmp(tzname, win32_tzmap[i].dstname) == 0)
		{
			elog(DEBUG4, "TZ \"%s\" matches Windows timezone \"%s\"",
				 win32_tzmap[i].pgtzname, tzname);
			return win32_tzmap[i].pgtzname;
		}
	}

	ereport(WARNING,
			(errmsg("could not find a match for Windows timezone \"%s\"",
					tzname)));
	return NULL;
}

#endif /* WIN32 */


/*
 * Check whether timezone is acceptable.
 *
 * What we are doing here is checking for leap-second-aware timekeeping.
 * We need to reject such TZ settings because they'll wreak havoc with our
 * date/time arithmetic.
 *
 * NB: this must NOT ereport(ERROR).  The caller must get control back so that
 * it can restore the old value of TZ if we don't like the new one.
 */
bool
tz_acceptable(void)
{
	struct pg_tm *tt;
	pg_time_t	time2000;

	/*
	 * To detect leap-second timekeeping, run pg_localtime for what should
	 * be GMT midnight, 2000-01-01.  Insist that the tm_sec value be zero;
	 * any other result has to be due to leap seconds.
	 */
	time2000 = (POSTGRES_EPOCH_JDATE - UNIX_EPOCH_JDATE) * 86400;
	tt = pg_localtime(&time2000);
	if (!tt || tt->tm_sec != 0)
		return false;

	return true;
}

/*
 * Identify a suitable default timezone setting based on the environment,
 * and make it active.
 *
 * We first look to the TZ environment variable.  If not found or not
 * recognized by our own code, we see if we can identify the timezone
 * from the behavior of the system timezone library.  When all else fails,
 * fall back to GMT.
 */
const char *
select_default_timezone(void)
{
	const char   *def_tz;

	def_tz = getenv("TZ");
	if (def_tz && pg_tzset(def_tz) && tz_acceptable())
		return def_tz;

	def_tz = identify_system_timezone();
	if (def_tz && pg_tzset(def_tz) && tz_acceptable())
		return def_tz;

	if (pg_tzset("GMT") && tz_acceptable())
		return "GMT";

	ereport(FATAL,
			(errmsg("could not select a suitable default timezone"),
			 errdetail("It appears that your GMT time zone uses leap seconds. PostgreSQL does not support leap seconds.")));
	return NULL;				/* keep compiler quiet */
}

/*
 * Initialize timezone library
 *
 * This is called after initial loading of postgresql.conf.  If no TimeZone
 * setting was found therein, we try to derive one from the environment.
 */
void
pg_timezone_initialize(void)
{
	/* Do we need to try to figure the timezone? */
	if (pg_strcasecmp(GetConfigOption("timezone"), "UNKNOWN") == 0)
	{
		const char *def_tz;

		/* Select setting */
		def_tz = select_default_timezone();
		/* Tell GUC about the value. Will redundantly call pg_tzset() */
		SetConfigOption("timezone", def_tz, PGC_POSTMASTER, PGC_S_ARGV);
	}
}
