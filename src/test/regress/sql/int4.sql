--
-- INT4
-- WARNING: int4 operators never check for over/underflow!
-- Some of these answers are consequently numerically incorrect.
--

CREATE TABLE INT4_TBL(f1 int4);

INSERT INTO INT4_TBL(f1) VALUES ('0');

INSERT INTO INT4_TBL(f1) VALUES ('123456');

INSERT INTO INT4_TBL(f1) VALUES ('-123456');

INSERT INTO INT4_TBL(f1) VALUES ('34.5');

-- largest and smallest values 
INSERT INTO INT4_TBL(f1) VALUES ('2147483647');

INSERT INTO INT4_TBL(f1) VALUES ('-2147483647');

-- bad input values -- should give warnings 
INSERT INTO INT4_TBL(f1) VALUES ('1000000000000');

INSERT INTO INT4_TBL(f1) VALUES ('asdf');


SELECT '' AS five, INT4_TBL.*;

SELECT '' AS four, i.* FROM INT4_TBL i WHERE i.f1 <> int2 '0';

SELECT '' AS four, i.* FROM INT4_TBL i WHERE i.f1 <> int4 '0';

SELECT '' AS one, i.* FROM INT4_TBL i WHERE i.f1 = int2 '0';

SELECT '' AS one, i.* FROM INT4_TBL i WHERE i.f1 = int4 '0';

SELECT '' AS two, i.* FROM INT4_TBL i WHERE i.f1 < int2 '0';

SELECT '' AS two, i.* FROM INT4_TBL i WHERE i.f1 < int4 '0';

SELECT '' AS three, i.* FROM INT4_TBL i WHERE i.f1 <= int2 '0';

SELECT '' AS three, i.* FROM INT4_TBL i WHERE i.f1 <= int4 '0';

SELECT '' AS two, i.* FROM INT4_TBL i WHERE i.f1 > int2 '0';

SELECT '' AS two, i.* FROM INT4_TBL i WHERE i.f1 > int4 '0';

SELECT '' AS three, i.* FROM INT4_TBL i WHERE i.f1 >= int2 '0';

SELECT '' AS three, i.* FROM INT4_TBL i WHERE i.f1 >= int4 '0';

-- positive odds 
SELECT '' AS one, i.* FROM INT4_TBL i WHERE (i.f1 % int2 '2') = int2 '1';

-- any evens 
SELECT '' AS three, i.* FROM INT4_TBL i WHERE (i.f1 % int4 '2') = int2 '0';

SELECT '' AS five, i.f1, i.f1 * int2 '2' AS x FROM INT4_TBL i;

SELECT '' AS five, i.f1, i.f1 * int4 '2' AS x FROM INT4_TBL i;

SELECT '' AS five, i.f1, i.f1 + int2 '2' AS x FROM INT4_TBL i;

SELECT '' AS five, i.f1, i.f1 + int4 '2' AS x FROM INT4_TBL i;

SELECT '' AS five, i.f1, i.f1 - int2 '2' AS x FROM INT4_TBL i;

SELECT '' AS five, i.f1, i.f1 - int4 '2' AS x FROM INT4_TBL i;

SELECT '' AS five, i.f1, i.f1 / int2 '2' AS x FROM INT4_TBL i;

SELECT '' AS five, i.f1, i.f1 / int4 '2' AS x FROM INT4_TBL i;

--
-- more complex expressions
--

-- variations on unary minus parsing
SELECT -2+3 AS one;

SELECT 4-2 AS two;

SELECT 2- -1 AS three;

SELECT 2 - -2 AS four;

SELECT int2 '2' * int2 '2' = int2 '16' / int2 '4' AS true;

SELECT int4 '2' * int2 '2' = int2 '16' / int4 '4' AS true;

SELECT int2 '2' * int4 '2' = int4 '16' / int2 '4' AS true;

SELECT int4 '1000' < int4 '999' AS false;

SELECT 4! AS twenty_four;

SELECT !!3 AS six;

SELECT 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 AS ten;

SELECT 2 + 2 / 2 AS three;

SELECT (2 + 2) / 2 AS two;

SELECT dsqrt(float8 '64') AS eight;

SELECT |/float8 '64' AS eight;

SELECT ||/float8 '27' AS three;

