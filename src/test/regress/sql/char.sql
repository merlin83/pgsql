--  ****************** test built-in type char **************
--
-- all inputs are SILENTLY truncated at 1 character
--

-- fixed-length by value
-- internally passed by value if <= 4 bytes in storage
-- Not sure why this is a really useful test,
--  but this test has been here forever. - thomas 1997-11-30

SELECT 'c'::char = 'c'::char AS true;

--
-- Build a table for testing
--

CREATE TABLE CHAR_TBL(f1 char);

INSERT INTO CHAR_TBL (f1) VALUES ('a');

INSERT INTO CHAR_TBL (f1) VALUES ('A');

-- any of the following three input formats are acceptable 
INSERT INTO CHAR_TBL (f1) VALUES ('1');

INSERT INTO CHAR_TBL (f1) VALUES (2);

INSERT INTO CHAR_TBL (f1) VALUES ('3');

-- zero-length char 
INSERT INTO CHAR_TBL (f1) VALUES ('');

-- try char's of greater than 1 length 
INSERT INTO CHAR_TBL (f1) VALUES ('cd');


SELECT '' AS seven, CHAR_TBL.*;

SELECT '' AS six, c.*
   FROM CHAR_TBL c
   WHERE c.f1 <> 'a';

SELECT '' AS one, c.*
   FROM CHAR_TBL c
   WHERE c.f1 = 'a';

SELECT '' AS five, c.*
   FROM CHAR_TBL c
   WHERE c.f1 < 'a';

SELECT '' AS six, c.*
   FROM CHAR_TBL c
   WHERE c.f1 <= 'a';

SELECT '' AS one, c.*
   FROM CHAR_TBL c
   WHERE c.f1 > 'a';

SELECT '' AS two, c.*
   FROM CHAR_TBL c
   WHERE c.f1 >= 'a';

DROP TABLE CHAR_TBL;

--
-- Now test longer arrays of char
--

CREATE TABLE CHAR_TBL(f1 char(4));

INSERT INTO CHAR_TBL (f1) VALUES ('a');
INSERT INTO CHAR_TBL (f1) VALUES ('ab');
INSERT INTO CHAR_TBL (f1) VALUES ('abcd');
INSERT INTO CHAR_TBL (f1) VALUES ('abcde');

SELECT '' AS four, CHAR_TBL.*;

