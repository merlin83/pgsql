--
-- create.source
--
--

-- CLASS POPULATION
--	(any resemblance to real life is purely coincidental)
--

INSERT INTO tenk2 VALUES (tenk1.*);

SELECT * INTO TABLE onek2 FROM onek;


INSERT INTO fast_emp4000 VALUES (slow_emp4000.*);

SELECT *
   INTO TABLE Bprime
   FROM tenk1
   WHERE unique2 < 1000;

INSERT INTO hobbies_r (name, person)
   SELECT 'posthacking', p.name
   FROM person* p
   WHERE p.name = 'mike' or p.name = 'jeff';

INSERT INTO hobbies_r (name, person)
   SELECT 'basketball', p.name
   FROM person p
   WHERE p.name = 'joe' or p.name = 'sally';

INSERT INTO hobbies_r (name) VALUES ('skywalking');

INSERT INTO equipment_r (name, hobby) VALUES ('advil', 'posthacking');

INSERT INTO equipment_r (name, hobby) VALUES ('peet''s coffee', 'posthacking');

INSERT INTO equipment_r (name, hobby) VALUES ('hightops', 'basketball');

INSERT INTO equipment_r (name, hobby) VALUES ('guts', 'skywalking');

SELECT *
   INTO TABLE ramp
   FROM road
   WHERE name ~ '.*Ramp';

INSERT INTO ihighway 
   SELECT * 
   FROM road 
   WHERE name ~ 'I- .*';

INSERT INTO shighway 
   SELECT * 
   FROM road 
   WHERE name ~ 'State Hwy.*';

UPDATE shighway
   SET surface = 'asphalt';

INSERT INTO a_star (class, a) VALUES ('a', 1);

INSERT INTO a_star (class, a) VALUES ('a', 2);

INSERT INTO a_star (class) VALUES ('a');

INSERT INTO b_star (class, a, b) VALUES ('b', 3, 'mumble'::text);

INSERT INTO b_star (class, a) VALUES ('b', 4);

INSERT INTO b_star (class, b) VALUES ('b', 'bumble'::text);

INSERT INTO b_star (class) VALUES ('b');

INSERT INTO c_star (class, a, c) VALUES ('c', 5, 'hi mom'::char16);

INSERT INTO c_star (class, a) VALUES ('c', 6);

INSERT INTO c_star (class, c) VALUES ('c', 'hi paul'::char16);

INSERT INTO c_star (class) VALUES ('c');

INSERT INTO d_star (class, a, b, c, d)
   VALUES ('d', 7, 'grumble'::text, 'hi sunita'::char16, '0.0'::float8);

INSERT INTO d_star (class, a, b, c)
   VALUES ('d', 8, 'stumble'::text, 'hi koko'::char16);

INSERT INTO d_star (class, a, b, d)
   VALUES ('d', 9, 'rumble'::text, '1.1'::float8);

INSERT INTO d_star (class, a, c, d)
   VALUES ('d', 10, 'hi kristin'::char16, '10.01'::float8);

INSERT INTO d_star (class, b, c, d)
   VALUES ('d', 'crumble'::text, 'hi boris'::char16, '100.001'::float8);

INSERT INTO d_star (class, a, b)
   VALUES ('d', 11, 'fumble'::text);

INSERT INTO d_star (class, a, c)
   VALUES ('d', 12, 'hi avi'::char16);

INSERT INTO d_star (class, a, d)
   VALUES ('d', 13, '1000.0001'::float8);

INSERT INTO d_star (class, b, c)
   VALUES ('d', 'tumble'::text, 'hi andrew'::char16);

INSERT INTO d_star (class, b, d)
   VALUES ('d', 'humble'::text, '10000.00001'::float8);

INSERT INTO d_star (class, c, d)
   VALUES ('d', 'hi ginger'::char16, '100000.000001'::float8);

INSERT INTO d_star (class, a) VALUES ('d', 14);

INSERT INTO d_star (class, b) VALUES ('d', 'jumble'::text);

INSERT INTO d_star (class, c) VALUES ('d', 'hi jolly'::char16);

INSERT INTO d_star (class, d) VALUES ('d', '1000000.0000001'::float8);

INSERT INTO d_star (class) VALUES ('d');

INSERT INTO e_star (class, a, c, e)
   VALUES ('e', 15, 'hi carol'::char16, '-1'::int2);

INSERT INTO e_star (class, a, c)
   VALUES ('e', 16, 'hi bob'::char16);

INSERT INTO e_star (class, a, e)
   VALUES ('e', 17, '-2'::int2);

INSERT INTO e_star (class, c, e)
   VALUES ('e', 'hi michelle'::char16, '-3'::int2);

INSERT INTO e_star (class, a)
   VALUES ('e', 18);

INSERT INTO e_star (class, c)
   VALUES ('e', 'hi elisa'::char16);

INSERT INTO e_star (class, e)
   VALUES ('e', '-4'::int2);

INSERT INTO f_star (class, a, c, e, f)
   VALUES ('f', 19, 'hi claire'::char16, '-5'::int2, '(1,2,3,4)'::polygon);

INSERT INTO f_star (class, a, c, e)
   VALUES ('f', 20, 'hi mike'::char16, '-6'::int2);

INSERT INTO f_star (class, a, c, f)
   VALUES ('f', 21, 'hi marcel'::char16, '(11,22,33,44,55,66)'::polygon);

INSERT INTO f_star (class, a, e, f)
   VALUES ('f', 22, '-7'::int2, '(111,222,333,444,555,666,777,888)'::polygon);

INSERT INTO f_star (class, c, e, f)
   VALUES ('f', 'hi keith'::char16, '-8'::int2, 
	   '(1111,2222,3333,4444)'::polygon);

INSERT INTO f_star (class, a, c)
   VALUES ('f', 24, 'hi marc'::char16);

INSERT INTO f_star (class, a, e)
   VALUES ('f', 25, '-9'::int2);

INSERT INTO f_star (class, a, f)
   VALUES ('f', 26, '(11111,22222,33333,44444)'::polygon); 

INSERT INTO f_star (class, c, e)
   VALUES ('f', 'hi allison'::char16, '-10'::int2);

INSERT INTO f_star (class, c, f)
   VALUES ('f', 'hi jeff'::char16,
           '(111111,222222,333333,444444)'::polygon);

INSERT INTO f_star (class, e, f)
   VALUES ('f', '-11'::int2, '(1111111,2222222,3333333,4444444)'::polygon);

INSERT INTO f_star (class, a) VALUES ('f', 27);

INSERT INTO f_star (class, c) VALUES ('f', 'hi carl'::char16);

INSERT INTO f_star (class, e) VALUES ('f', '-12'::int2);

INSERT INTO f_star (class, f) 
   VALUES ('f', '(11111111,22222222,33333333,44444444)'::polygon);

INSERT INTO f_star (class) VALUES ('f');

--
-- ARRAYS
--

--
-- only this array as a 0-based 'e', the others are 1-based.
-- 'e' is also a large object.
--

INSERT INTO arrtest (a[5], b[2][1][2], c, d)
   VALUES ('{1,2,3,4,5}', '{{{},{1,2}}}', '{}', '{}');

UPDATE arrtest SET e[0] = '1.1';

UPDATE arrtest SET e[1] = '2.2';

INSERT INTO arrtest (a, b[2][2][1], c, d, e)
   VALUES ('{11,12,23}', '{{3,4},{4,5}}', '{"foobar"}', 
           '{{"elt1", "elt2"}}', '{"3.4", "6.7"}');

INSERT INTO arrtest (a, b[1][2][2], c, d[2][1])
   VALUES ('{}', '{3,4}', '{foo,bar}', '{bar,foo}');


--
-- for internal portal (cursor) tests
--
CREATE TABLE iportaltest (
	i		int4, 
	d		float4, 
	p		polygon
);

INSERT INTO iportaltest (i, d, p)
   VALUES (1, 3.567, '(3.0,4.0,1.0,2.0)'::polygon);

INSERT INTO iportaltest (i, d, p)
   VALUES (2, 89.05, '(4.0,3.0,2.0,1.0)'::polygon);

