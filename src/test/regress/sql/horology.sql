--
-- HOROLOGY
--

--
-- date, time arithmetic
--

SELECT date '1981-02-03' + time '04:05:06' AS "Date + Time";

SELECT date '1991-02-03' + time with time zone '04:05:06 PST' AS "Date + Time PST";

SELECT date '2001-02-03' + time with time zone '04:05:06 UTC' AS "Date + Time UTC";

SELECT date '1991-02-03' + interval '2 years' AS "Add Two Years";

SELECT date '2001-12-13' - interval '2 years' AS "Subtract Two Years";

SELECT date '1991-02-03' - time '04:05:06' AS "Subtract Time";

SELECT date '1991-02-03' - time with time zone '04:05:06 UTC' AS "Subtract Time UTC";

--
-- timestamp, interval arithmetic
--

SELECT timestamp '1996-03-01' - interval '1 second' AS "Feb 29";
SELECT timestamp '1999-03-01' - interval '1 second' AS "Feb 28";
SELECT timestamp '2000-03-01' - interval '1 second' AS "Feb 29";
SELECT timestamp '1999-12-01' + interval '1 month - 1 second' AS "Dec 31";

CREATE TABLE TEMP_TIMESTAMP (f1 timestamp);

-- get some candidate input values

INSERT INTO TEMP_TIMESTAMP (f1)
  SELECT d1 FROM TIMESTAMP_TBL
  WHERE d1 BETWEEN '13-jun-1957' AND '1-jan-1997'
   OR d1 BETWEEN '1-jan-1999' AND '1-jan-2010';

SELECT '' AS "15", f1 AS timestamp
  FROM TEMP_TIMESTAMP
  ORDER BY timestamp;

SELECT '' AS "150", d.f1 AS timestamp, t.f1 AS interval, d.f1 + t.f1 AS plus
  FROM TEMP_TIMESTAMP d, INTERVAL_TBL t
  ORDER BY plus, timestamp, interval;

SELECT '' AS "150", d.f1 AS timestamp, t.f1 AS interval, d.f1 - t.f1 AS minus
  FROM TEMP_TIMESTAMP d, INTERVAL_TBL t
  WHERE isfinite(d.f1)
  ORDER BY minus, timestamp, interval;

SELECT '' AS "15", d.f1 AS timestamp, timestamp '1980-01-06 00:00 GMT' AS gpstime_zero,
   d.f1 - timestamp '1980-01-06 00:00 GMT' AS difference
  FROM TEMP_TIMESTAMP d
  ORDER BY difference;

SELECT '' AS "225", d1.f1 AS timestamp1, d2.f1 AS timestamp2, d1.f1 - d2.f1 AS difference
  FROM TEMP_TIMESTAMP d1, TEMP_TIMESTAMP d2
  ORDER BY timestamp1, timestamp2, difference;

SELECT '' as "54", d1 as timestamp,
  date_part('year', d1) AS year, date_part('month', d1) AS month,
  date_part('day',d1) AS day, date_part('hour', d1) AS hour,
  date_part('minute', d1) AS minute, date_part('second', d1) AS second
  FROM TIMESTAMP_TBL
  WHERE isfinite(d1) and d1 >= '1-jan-1900 GMT'
  ORDER BY timestamp;

--
-- abstime, reltime arithmetic
--

SELECT '' AS ten, ABSTIME_TBL.f1 AS abstime, RELTIME_TBL.f1 AS reltime
   WHERE (ABSTIME_TBL.f1 + RELTIME_TBL.f1)
	< abstime 'Jan 14 14:00:00 1971'
   ORDER BY abstime, reltime;

-- these four queries should return the same answer
-- the "infinity" and "-infinity" tuples in ABSTIME_TBL cannot be added and
-- therefore, should not show up in the results.

SELECT '' AS three, ABSTIME_TBL.*
  WHERE  (ABSTIME_TBL.f1 + reltime '@ 3 year')         -- +3 years
	< abstime 'Jan 14 14:00:00 1977';

SELECT '' AS three, ABSTIME_TBL.*
   WHERE  (ABSTIME_TBL.f1 + reltime '@ 3 year ago')    -- -3 years
	< abstime 'Jan 14 14:00:00 1971';

SELECT '' AS three, ABSTIME_TBL.*
   WHERE  (ABSTIME_TBL.f1 - reltime '@ 3 year')        -- -(+3) years
	< abstime 'Jan 14 14:00:00 1971';

SELECT '' AS three, ABSTIME_TBL.*
   WHERE  (ABSTIME_TBL.f1 - reltime '@ 3 year ago')    -- -(-3) years
        < abstime 'Jan 14 14:00:00 1977';

--
-- Conversions
--

SELECT '' AS "15", f1 AS timestamp, date( f1) AS date
  FROM TEMP_TIMESTAMP
  WHERE f1 <> timestamp 'current'
  ORDER BY date, timestamp;

SELECT '' AS "15", f1 AS timestamp, abstime( f1) AS abstime
  FROM TEMP_TIMESTAMP
  ORDER BY abstime;

SELECT '' AS four, f1 AS abstime, date( f1) AS date
  FROM ABSTIME_TBL
  WHERE isfinite(f1) AND f1 <> abstime 'current'
  ORDER BY date, abstime;

SELECT '' AS five, d1 AS timestamp, abstime(d1) AS abstime
  FROM TIMESTAMP_TBL WHERE NOT isfinite(d1);

SELECT '' AS three, f1 as abstime, timestamp(f1) AS timestamp
  FROM ABSTIME_TBL WHERE NOT isfinite(f1);

SELECT '' AS ten, f1 AS interval, reltime( f1) AS reltime
  FROM INTERVAL_TBL;

SELECT '' AS six, f1 as reltime, interval( f1) AS interval
  FROM RELTIME_TBL;

DROP TABLE TEMP_TIMESTAMP;

--
-- Formats
--

SET DateStyle TO 'US,Postgres';

SHOW DateStyle;

SELECT '' AS "66", d1 AS us_postgres FROM TIMESTAMP_TBL;

SELECT '' AS eight, f1 AS us_postgres FROM ABSTIME_TBL;

SET DateStyle TO 'US,ISO';

SELECT '' AS "66", d1 AS us_iso FROM TIMESTAMP_TBL;

SELECT '' AS eight, f1 AS us_iso FROM ABSTIME_TBL;

SET DateStyle TO 'US,SQL';

SHOW DateStyle;

SELECT '' AS "66", d1 AS us_sql FROM TIMESTAMP_TBL;

SELECT '' AS eight, f1 AS us_sql FROM ABSTIME_TBL;

SET DateStyle TO 'European,Postgres';

SHOW DateStyle;

INSERT INTO TIMESTAMP_TBL VALUES('13/06/1957');

SELECT count(*) as one FROM TIMESTAMP_TBL WHERE d1 = 'Jun 13 1957';

SELECT '' AS "67", d1 AS european_postgres FROM TIMESTAMP_TBL;

SELECT '' AS eight, f1 AS european_postgres FROM ABSTIME_TBL;

SET DateStyle TO 'European,ISO';

SHOW DateStyle;

SELECT '' AS "67", d1 AS european_iso FROM TIMESTAMP_TBL;

SELECT '' AS eight, f1 AS european_iso FROM ABSTIME_TBL;

SET DateStyle TO 'European,SQL';

SHOW DateStyle;

SELECT '' AS "67", d1 AS european_sql FROM TIMESTAMP_TBL;

SELECT '' AS eight, f1 AS european_sql FROM ABSTIME_TBL;

RESET DateStyle;
