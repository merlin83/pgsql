--
-- TIMESTAMPTZ
--

CREATE TABLE TIMESTAMPTZ_TBL (d1 timestamp(2) with time zone);

-- Test shorthand input values
-- We can't just "select" the results since they aren't constants; test for
-- equality instead.  We can do that by running the test inside a transaction
-- block, within which the value of 'now' shouldn't change.  We also check
-- that 'now' *does* change over a reasonable interval such as 100 msec.
-- NOTE: it is possible for this part of the test to fail if the transaction
-- block is entered exactly at local midnight; then 'now' and 'today' have
-- the same values and the counts will come out different.

INSERT INTO TIMESTAMPTZ_TBL VALUES ('now');
SELECT pg_sleep(0.1);

BEGIN;

INSERT INTO TIMESTAMPTZ_TBL VALUES ('now');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('today');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('yesterday');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('tomorrow');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('tomorrow EST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('tomorrow zulu');

SELECT count(*) AS One FROM TIMESTAMPTZ_TBL WHERE d1 = timestamp with time zone 'today';
SELECT count(*) AS One FROM TIMESTAMPTZ_TBL WHERE d1 = timestamp with time zone 'tomorrow';
SELECT count(*) AS One FROM TIMESTAMPTZ_TBL WHERE d1 = timestamp with time zone 'yesterday';
SELECT count(*) AS One FROM TIMESTAMPTZ_TBL WHERE d1 = timestamp(2) with time zone 'now';

COMMIT;

DELETE FROM TIMESTAMPTZ_TBL;

-- verify uniform transaction time within transaction block
BEGIN;
INSERT INTO TIMESTAMPTZ_TBL VALUES ('now');
SELECT pg_sleep(0.1);
INSERT INTO TIMESTAMPTZ_TBL VALUES ('now');
SELECT pg_sleep(0.1);
SELECT count(*) AS two FROM TIMESTAMPTZ_TBL WHERE d1 = timestamp(2) with time zone 'now';
COMMIT;

DELETE FROM TIMESTAMPTZ_TBL;

-- Special values
INSERT INTO TIMESTAMPTZ_TBL VALUES ('-infinity');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('infinity');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('epoch');
-- Obsolete special values
INSERT INTO TIMESTAMPTZ_TBL VALUES ('invalid');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('current');

-- Postgres v6.0 standard output format
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mon Feb 10 17:32:01 1997 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Invalid Abstime');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Undefined Abstime');

-- Variations on Postgres v6.1 standard output format
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mon Feb 10 17:32:01.000001 1997 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mon Feb 10 17:32:01.999999 1997 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mon Feb 10 17:32:01.4 1997 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mon Feb 10 17:32:01.5 1997 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mon Feb 10 17:32:01.6 1997 PST');

-- ISO 8601 format
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-01-02');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-01-02 03:04:05');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-02-10 17:32:01-08');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-02-10 17:32:01-0800');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-02-10 17:32:01 -08:00');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('19970210 173201 -0800');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-06-10 17:32:01 -07:00');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('2001-09-22T18:19:20');

-- POSIX format (note that the timezone abbrev is just decoration here)
INSERT INTO TIMESTAMPTZ_TBL VALUES ('2000-03-15 08:14:01 GMT+8');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('2000-03-15 13:14:02 GMT-1');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('2000-03-15 12:14:03 GMT-2');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('2000-03-15 03:14:04 PST+8');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('2000-03-15 02:14:05 MST+7:00');

-- Variations for acceptable input formats
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 10 17:32:01 1997 -0800');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 10 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 10 5:32PM 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997/02/10 17:32:01-0800');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-02-10 17:32:01 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb-10-1997 17:32:01 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('02-10-1997 17:32:01 PST');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('19970210 173201 PST');
set datestyle to ymd;
INSERT INTO TIMESTAMPTZ_TBL VALUES ('97FEB10 5:32:01PM UTC');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('97/02/10 17:32:01 UTC');
reset datestyle;
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997.041 17:32:01 UTC');

-- timestamps at different timezones
INSERT INTO TIMESTAMPTZ_TBL VALUES ('19970210 173201 America/New_York');
SELECT '19970210 173201' AT TIME ZONE 'America/New_York';
INSERT INTO TIMESTAMPTZ_TBL VALUES ('19970710 173201 America/New_York');
SELECT '19970710 173201' AT TIME ZONE 'America/New_York';
INSERT INTO TIMESTAMPTZ_TBL VALUES ('19970710 173201 America/Does_not_exist');
SELECT '19970710 173201' AT TIME ZONE 'America/Does_not_exist';

-- Daylight saving time for timestamps beyond 32-bit time_t range.
SELECT '20500710 173201 Europe/Helsinki'::timestamptz; -- DST
SELECT '20500110 173201 Europe/Helsinki'::timestamptz; -- non-DST

SELECT '205000-07-10 17:32:01 Europe/Helsinki'::timestamptz; -- DST
SELECT '205000-01-10 17:32:01 Europe/Helsinki'::timestamptz; -- non-DST

-- Check date conversion and date arithmetic
INSERT INTO TIMESTAMPTZ_TBL VALUES ('1997-06-10 18:32:01 PDT');

INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 10 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 11 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 12 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 13 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 14 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 15 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 1997');

INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 0097 BC');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 0097');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 0597');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 1097');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 1697');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 1797');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 1897');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 2097');

INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 28 17:32:01 1996');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 29 17:32:01 1996');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mar 01 17:32:01 1996');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Dec 30 17:32:01 1996');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Dec 31 17:32:01 1996');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Jan 01 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 28 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 29 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Mar 01 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Dec 30 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Dec 31 17:32:01 1997');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Dec 31 17:32:01 1999');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Jan 01 17:32:01 2000');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Dec 31 17:32:01 2000');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Jan 01 17:32:01 2001');

-- Currently unsupported syntax and ranges
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 -0097');
INSERT INTO TIMESTAMPTZ_TBL VALUES ('Feb 16 17:32:01 5097 BC');

-- Alternative field order that we've historically supported (sort of)
-- with regular and POSIXy timezone specs
SELECT 'Wed Jul 11 10:51:14 America/New_York 2001'::timestamptz;
SELECT 'Wed Jul 11 10:51:14 GMT-4 2001'::timestamptz;
SELECT 'Wed Jul 11 10:51:14 GMT+4 2001'::timestamptz;
SELECT 'Wed Jul 11 10:51:14 PST-03:00 2001'::timestamptz;
SELECT 'Wed Jul 11 10:51:14 PST+03:00 2001'::timestamptz;

SELECT '' AS "64", d1 FROM TIMESTAMPTZ_TBL; 

-- Demonstrate functions and operators
SELECT '' AS "48", d1 FROM TIMESTAMPTZ_TBL
   WHERE d1 > timestamp with time zone '1997-01-02';

SELECT '' AS "15", d1 FROM TIMESTAMPTZ_TBL
   WHERE d1 < timestamp with time zone '1997-01-02';

SELECT '' AS one, d1 FROM TIMESTAMPTZ_TBL
   WHERE d1 = timestamp with time zone '1997-01-02';

SELECT '' AS "63", d1 FROM TIMESTAMPTZ_TBL
   WHERE d1 != timestamp with time zone '1997-01-02';

SELECT '' AS "16", d1 FROM TIMESTAMPTZ_TBL
   WHERE d1 <= timestamp with time zone '1997-01-02';

SELECT '' AS "49", d1 FROM TIMESTAMPTZ_TBL
   WHERE d1 >= timestamp with time zone '1997-01-02';

SELECT '' AS "54", d1 - timestamp with time zone '1997-01-02' AS diff
   FROM TIMESTAMPTZ_TBL WHERE d1 BETWEEN '1902-01-01' AND '2038-01-01';

SELECT '' AS date_trunc_week, date_trunc( 'week', timestamp with time zone '2004-02-29 15:44:17.71393' ) AS week_trunc;

-- Test casting within a BETWEEN qualifier
SELECT '' AS "54", d1 - timestamp with time zone '1997-01-02' AS diff
  FROM TIMESTAMPTZ_TBL
  WHERE d1 BETWEEN timestamp with time zone '1902-01-01' AND timestamp with time zone '2038-01-01';

SELECT '' AS "54", d1 as timestamptz,
   date_part( 'year', d1) AS year, date_part( 'month', d1) AS month,
   date_part( 'day', d1) AS day, date_part( 'hour', d1) AS hour,
   date_part( 'minute', d1) AS minute, date_part( 'second', d1) AS second
   FROM TIMESTAMPTZ_TBL WHERE d1 BETWEEN '1902-01-01' AND '2038-01-01';

SELECT '' AS "54", d1 as timestamptz,
   date_part( 'quarter', d1) AS quarter, date_part( 'msec', d1) AS msec,
   date_part( 'usec', d1) AS usec
   FROM TIMESTAMPTZ_TBL WHERE d1 BETWEEN '1902-01-01' AND '2038-01-01';

SELECT '' AS "54", d1 as timestamptz,
   date_part( 'isoyear', d1) AS isoyear, date_part( 'week', d1) AS week,
   date_part( 'dow', d1) AS dow
   FROM TIMESTAMPTZ_TBL WHERE d1 BETWEEN '1902-01-01' AND '2038-01-01';

-- TO_CHAR()
SELECT '' AS to_char_1, to_char(d1, 'DAY Day day DY Dy dy MONTH Month month RM MON Mon mon') 
   FROM TIMESTAMPTZ_TBL;
	
SELECT '' AS to_char_2, to_char(d1, 'FMDAY FMDay FMday FMMONTH FMMonth FMmonth FMRM')
   FROM TIMESTAMPTZ_TBL;	

SELECT '' AS to_char_3, to_char(d1, 'Y,YYY YYYY YYY YY Y CC Q MM WW DDD DD D J')
   FROM TIMESTAMPTZ_TBL;
	
SELECT '' AS to_char_4, to_char(d1, 'FMY,YYY FMYYYY FMYYY FMYY FMY FMCC FMQ FMMM FMWW FMDDD FMDD FMD FMJ') 
   FROM TIMESTAMPTZ_TBL;	
	
SELECT '' AS to_char_5, to_char(d1, 'HH HH12 HH24 MI SS SSSS') 
   FROM TIMESTAMPTZ_TBL;

SELECT '' AS to_char_6, to_char(d1, E'"HH:MI:SS is" HH:MI:SS "\\"text between quote marks\\""') 
   FROM TIMESTAMPTZ_TBL;		
		
SELECT '' AS to_char_7, to_char(d1, 'HH24--text--MI--text--SS')
   FROM TIMESTAMPTZ_TBL;		

SELECT '' AS to_char_8, to_char(d1, 'YYYYTH YYYYth Jth') 
   FROM TIMESTAMPTZ_TBL;
  
SELECT '' AS to_char_9, to_char(d1, 'YYYY A.D. YYYY a.d. YYYY bc HH:MI:SS P.M. HH:MI:SS p.m. HH:MI:SS pm') 
   FROM TIMESTAMPTZ_TBL;   

SELECT '' AS to_char_10, to_char(d1, 'IYYY IYY IY I IW IDDD ID')
   FROM TIMESTAMPTZ_TBL;

SELECT '' AS to_char_11, to_char(d1, 'FMIYYY FMIYY FMIY FMI FMIW FMIDDD FMID')
   FROM TIMESTAMPTZ_TBL;

-- TO_TIMESTAMP()
SELECT '' AS to_timestamp_1, to_timestamp('0097/Feb/16 --> 08:14:30', 'YYYY/Mon/DD --> HH:MI:SS');
	
SELECT '' AS to_timestamp_2, to_timestamp('97/2/16 8:14:30', 'FMYYYY/FMMM/FMDD FMHH:FMMI:FMSS');

SELECT '' AS to_timestamp_3, to_timestamp('1985 January 12', 'YYYY FMMonth DD');

SELECT '' AS to_timestamp_4, to_timestamp('My birthday-> Year: 1976, Month: May, Day: 16',
										  '"My birthday-> Year" YYYY, "Month:" FMMonth, "Day:" DD');

SELECT '' AS to_timestamp_5, to_timestamp('1,582nd VIII 21', 'Y,YYYth FMRM DD');

SELECT '' AS to_timestamp_6, to_timestamp('15 "text between quote marks" 98 54 45', 
										  E'HH "\\text between quote marks\\"" YY MI SS');
    
SELECT '' AS to_timestamp_7, to_timestamp('05121445482000', 'MMDDHHMISSYYYY');    

SELECT '' AS to_timestamp_8, to_timestamp('2000January09Sunday', 'YYYYFMMonthDDFMDay');

SELECT '' AS to_timestamp_9, to_timestamp('97/Feb/16', 'YYMonDD');

SELECT '' AS to_timestamp_10, to_timestamp('19971116', 'YYYYMMDD');

SELECT '' AS to_timestamp_11, to_timestamp('20000-1116', 'YYYY-MMDD');

SELECT '' AS to_timestamp_12, to_timestamp('9-1116', 'Y-MMDD');

SELECT '' AS to_timestamp_13, to_timestamp('95-1116', 'YY-MMDD');

SELECT '' AS to_timestamp_14, to_timestamp('995-1116', 'YYY-MMDD');

SELECT '' AS to_timestamp_15, to_timestamp('2005426', 'YYYYWWD');

SELECT '' AS to_timestamp_16, to_timestamp('2005300', 'YYYYDDD');

SELECT '' AS to_timestamp_17, to_timestamp('2005527', 'IYYYIWID');

SELECT '' AS to_timestamp_18, to_timestamp('005527', 'IYYIWID');

SELECT '' AS to_timestamp_19, to_timestamp('05527', 'IYIWID');

SELECT '' AS to_timestamp_20, to_timestamp('5527', 'IIWID');

SELECT '' AS to_timestamp_21, to_timestamp('2005364', 'IYYYIDDD');

SET DateStyle TO DEFAULT;
