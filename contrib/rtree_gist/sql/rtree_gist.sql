--
-- first, define the datatype.  Turn off echoing so that expected file
-- does not depend on contents of seg.sql.
--
\set ECHO none
\i rtree_gist.sql
\set ECHO all

create table boxtmp (b box);

\copy boxtmp from 'data/test_box.data'

select count(*) from boxtmp where b && '(1000,1000,0,0)'::box;

create index bix on boxtmp using rtree (b);

select count(*) from boxtmp where b && '(1000,1000,0,0)'::box;

drop index bix;

create index bix on boxtmp using gist (b);

select count(*) from boxtmp where b && '(1000,1000,0,0)'::box;

create table polytmp (p polygon);

\copy polytmp from 'data/test_box.data'

create index pix on polytmp using rtree (p);

select count(*) from polytmp where p && '(1000,1000),(0,0)'::polygon;

drop index pix;

create index pix on polytmp using gist (p);

select count(*) from polytmp where p && '(1000,1000),(0,0)'::polygon;

