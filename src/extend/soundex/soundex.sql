--------------- soundex.sql:

CREATE FUNCTION text_soundex(text) RETURNS text
   AS '/usr/local/postgres/postgres95/src/funcs/soundex.so' LANGUAGE 'c';

SELECT text_soundex('hello world!');

CREATE TABLE s (nm text)\g

insert into s values ('john')\g
insert into s values ('joan')\g
insert into s values ('wobbly')\g

select * from s
where text_soundex(nm) = text_soundex('john')\g

select nm from s a, s b
where text_soundex(a.nm) = text_soundex(b.nm)
and a.oid <> b.oid\g

CREATE FUNCTION text_sx_eq(text, text) RETURNS bool AS
'select text_soundex($1) = text_soundex($2)'
LANGUAGE 'sql'\g

CREATE FUNCTION text_sx_lt(text,text) RETURNS bool AS
'select text_soundex($1) < text_soundex($2)'
LANGUAGE 'sql'\g

CREATE FUNCTION text_sx_gt(text,text) RETURNS bool AS
'select text_soundex($1) > text_soundex($2)'
LANGUAGE 'sql';

CREATE FUNCTION text_sx_le(text,text) RETURNS bool AS
'select text_soundex($1) <= text_soundex($2)'
LANGUAGE 'sql';

CREATE FUNCTION text_sx_ge(text,text) RETURNS bool AS
'select text_soundex($1) >= text_soundex($2)'
LANGUAGE 'sql';

CREATE FUNCTION text_sx_ne(text,text) RETURNS bool AS
'select text_soundex($1) <> text_soundex($2)'
LANGUAGE 'sql';

DROP OPERATOR #= (text,text)\g

CREATE OPERATOR #= (leftarg=text, rightarg=text, procedure=text_sx_eq,
commutator=text_sx_eq)\g

SELECT *
FROM s
WHERE text_sx_eq(nm,'john')\g

SELECT *
from s
where s.nm #= 'john';

