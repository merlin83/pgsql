--
-- AGGREGATES
--
SELECT avg(four) AS avg_1 FROM onek;

SELECT avg(a) AS avg_32 FROM aggtest WHERE a < 100;

SELECT avg(b) AS avg_107_943 FROM aggtest;

SELECT avg(gpa) AS avg_3_4 FROM student;


SELECT sum(four) AS sum_1500 FROM onek;

SELECT sum(a) AS sum_198 FROM aggtest;

SELECT sum(b) AS avg_431_773 FROM aggtest;

SELECT sum(gpa) AS avg_6_8 FROM student;


SELECT max(four) AS max_3 FROM onek;

SELECT max(a) AS max_100 FROM aggtest;

SELECT max(aggtest.b) AS max_324_78 FROM aggtest;

SELECT max(student.gpa) AS max_3_7 FROM student;


SELECT count(four) AS cnt_1000 FROM onek;

SELECT count(DISTINCT four) AS cnt_4 FROM onek;

select ten, count(*), sum(four) from onek group by ten;

select ten, count(four), sum(DISTINCT four) from onek group by ten;


SELECT newavg(four) AS avg_1 FROM onek;

SELECT newsum(four) AS sum_1500 FROM onek;

SELECT newcnt(four) AS cnt_1000 FROM onek;

