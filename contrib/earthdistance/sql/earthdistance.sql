--
--  Test earth distance functions
--

--
-- first, define the datatype.  Turn off echoing so that expected file
-- does not depend on contents of earthdistance.sql or cube.sql.
--
\set ECHO none
\i ../cube/cube.sql
\i earthdistance.sql
\set ECHO all

--
-- The radius of the Earth we are using.
--

select earth()::numeric(20,5);

--
-- Convert straight line distances to great circle distances.
--
select (pi()*earth())::numeric(20,5);
select sec_to_gc(0)::numeric(20,5);
select sec_to_gc(2*earth())::numeric(20,5);
select sec_to_gc(10*earth())::numeric(20,5);
select sec_to_gc(-earth())::numeric(20,5);
select sec_to_gc(1000)::numeric(20,5);
select sec_to_gc(10000)::numeric(20,5);
select sec_to_gc(100000)::numeric(20,5);
select sec_to_gc(1000000)::numeric(20,5);

--
-- Convert great circle distances to straight line distances.
--

select gc_to_sec(0)::numeric(20,5);
select gc_to_sec(sec_to_gc(2*earth()))::numeric(20,5);
select gc_to_sec(10*earth())::numeric(20,5);
select gc_to_sec(pi()*earth())::numeric(20,5);
select gc_to_sec(-1000)::numeric(20,5);
select gc_to_sec(1000)::numeric(20,5);
select gc_to_sec(10000)::numeric(20,5);
select gc_to_sec(100000)::numeric(20,5);
select gc_to_sec(1000000)::numeric(20,5);

--
-- Set coordinates using latitude and longitude.
-- Extract each coordinate separately so we can round them.
--

select cube_ll_coord(ll_to_earth(0,0),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(0,0),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(0,0),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(360,360),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(360,360),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(360,360),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(180,180),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(180,180),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(180,180),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(180,360),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(180,360),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(180,360),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(-180,-360),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(-180,-360),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(-180,-360),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(0,180),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(0,180),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(0,180),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(0,-180),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(0,-180),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(0,-180),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(90,0),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(90,0),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(90,0),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(90,180),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(90,180),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(90,180),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(-90,0),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(-90,0),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(-90,0),3)::numeric(20,5);
select cube_ll_coord(ll_to_earth(-90,180),1)::numeric(20,5),
 cube_ll_coord(ll_to_earth(-90,180),2)::numeric(20,5),
 cube_ll_coord(ll_to_earth(-90,180),3)::numeric(20,5);

--
-- Test getting the latitude of a location.
--

select latitude(ll_to_earth(0,0))::numeric(20,10);
select latitude(ll_to_earth(45,0))::numeric(20,10);
select latitude(ll_to_earth(90,0))::numeric(20,10);
select latitude(ll_to_earth(-45,0))::numeric(20,10);
select latitude(ll_to_earth(-90,0))::numeric(20,10);
select latitude(ll_to_earth(0,90))::numeric(20,10);
select latitude(ll_to_earth(45,90))::numeric(20,10);
select latitude(ll_to_earth(90,90))::numeric(20,10);
select latitude(ll_to_earth(-45,90))::numeric(20,10);
select latitude(ll_to_earth(-90,90))::numeric(20,10);
select latitude(ll_to_earth(0,180))::numeric(20,10);
select latitude(ll_to_earth(45,180))::numeric(20,10);
select latitude(ll_to_earth(90,180))::numeric(20,10);
select latitude(ll_to_earth(-45,180))::numeric(20,10);
select latitude(ll_to_earth(-90,180))::numeric(20,10);
select latitude(ll_to_earth(0,-90))::numeric(20,10);
select latitude(ll_to_earth(45,-90))::numeric(20,10);
select latitude(ll_to_earth(90,-90))::numeric(20,10);
select latitude(ll_to_earth(-45,-90))::numeric(20,10);
select latitude(ll_to_earth(-90,-90))::numeric(20,10);

--
-- Test getting the longitude of a location.
--

select longitude(ll_to_earth(0,0))::numeric(20,10);
select longitude(ll_to_earth(45,0))::numeric(20,10);
select longitude(ll_to_earth(90,0))::numeric(20,10);
select longitude(ll_to_earth(-45,0))::numeric(20,10);
select longitude(ll_to_earth(-90,0))::numeric(20,10);
select longitude(ll_to_earth(0,90))::numeric(20,10);
select longitude(ll_to_earth(45,90))::numeric(20,10);
select longitude(ll_to_earth(90,90))::numeric(20,10);
select longitude(ll_to_earth(-45,90))::numeric(20,10);
select longitude(ll_to_earth(-90,90))::numeric(20,10);
select longitude(ll_to_earth(0,180))::numeric(20,10);
select longitude(ll_to_earth(45,180))::numeric(20,10);
select longitude(ll_to_earth(90,180))::numeric(20,10);
select longitude(ll_to_earth(-45,180))::numeric(20,10);
select longitude(ll_to_earth(-90,180))::numeric(20,10);
select longitude(ll_to_earth(0,-90))::numeric(20,10);
select longitude(ll_to_earth(45,-90))::numeric(20,10);
select longitude(ll_to_earth(90,-90))::numeric(20,10);
select longitude(ll_to_earth(-45,-90))::numeric(20,10);
select longitude(ll_to_earth(-90,-90))::numeric(20,10);

--
-- For the distance tests the following is some real life data.
--
-- Chicago has a latitude of 41.8 and a longitude of 87.6.
-- Albuquerque has a latitude of 35.1 and a longitude of 106.7.
-- (Note that latitude and longitude are specified differently
-- in the cube based functions than for the point based functions.)
--

--
-- Test getting the distance between two points using earth_distance.
--

select earth_distance(ll_to_earth(0,0),ll_to_earth(0,0))::numeric(20,5);
select earth_distance(ll_to_earth(0,0),ll_to_earth(0,180))::numeric(20,5);
select earth_distance(ll_to_earth(0,0),ll_to_earth(90,0))::numeric(20,5);
select earth_distance(ll_to_earth(0,0),ll_to_earth(0,90))::numeric(20,5);
select earth_distance(ll_to_earth(0,0),ll_to_earth(0,1))::numeric(20,5);
select earth_distance(ll_to_earth(0,0),ll_to_earth(1,0))::numeric(20,5);
select earth_distance(ll_to_earth(30,0),ll_to_earth(30,1))::numeric(20,5);
select earth_distance(ll_to_earth(30,0),ll_to_earth(31,0))::numeric(20,5);
select earth_distance(ll_to_earth(60,0),ll_to_earth(60,1))::numeric(20,5);
select earth_distance(ll_to_earth(60,0),ll_to_earth(61,0))::numeric(20,5);
select earth_distance(ll_to_earth(41.8,87.6),ll_to_earth(35.1,106.7))::numeric(20,5);
select (earth_distance(ll_to_earth(41.8,87.6),ll_to_earth(35.1,106.7))*
      100./2.54/12./5280.)::numeric(20,5);

--
-- Test getting the distance between two points using geo_distance.
--

select geo_distance('(0,0)'::point,'(0,0)'::point)::numeric(20,5);
select geo_distance('(0,0)'::point,'(180,0)'::point)::numeric(20,5);
select geo_distance('(0,0)'::point,'(0,90)'::point)::numeric(20,5);
select geo_distance('(0,0)'::point,'(90,0)'::point)::numeric(20,5);
select geo_distance('(0,0)'::point,'(1,0)'::point)::numeric(20,5);
select geo_distance('(0,0)'::point,'(0,1)'::point)::numeric(20,5);
select geo_distance('(0,30)'::point,'(1,30)'::point)::numeric(20,5);
select geo_distance('(0,30)'::point,'(0,31)'::point)::numeric(20,5);
select geo_distance('(0,60)'::point,'(1,60)'::point)::numeric(20,5);
select geo_distance('(0,60)'::point,'(0,61)'::point)::numeric(20,5);
select geo_distance('(87.6,41.8)'::point,'(106.7,35.1)'::point)::numeric(20,5);
select (geo_distance('(87.6,41.8)'::point,'(106.7,35.1)'::point)*5280.*12.*2.54/100.)::numeric(20,5);

--
-- Test getting the distance between two points using the <@> operator.
--

select ('(0,0)'::point <@> '(0,0)'::point)::numeric(20,5);
select ('(0,0)'::point <@> '(180,0)'::point)::numeric(20,5);
select ('(0,0)'::point <@> '(0,90)'::point)::numeric(20,5);
select ('(0,0)'::point <@> '(90,0)'::point)::numeric(20,5);
select ('(0,0)'::point <@> '(1,0)'::point)::numeric(20,5);
select ('(0,0)'::point <@> '(0,1)'::point)::numeric(20,5);
select ('(0,30)'::point <@> '(1,30)'::point)::numeric(20,5);
select ('(0,30)'::point <@> '(0,31)'::point)::numeric(20,5);
select ('(0,60)'::point <@> '(1,60)'::point)::numeric(20,5);
select ('(0,60)'::point <@> '(0,61)'::point)::numeric(20,5);
select ('(87.6,41.8)'::point <@> '(106.7,35.1)'::point)::numeric(20,5);
select (('(87.6,41.8)'::point <@> '(106.7,35.1)'::point)*5280.*12.*2.54/100.)::numeric(20,5);

--
-- Test getting a bounding box around points.
--

select cube_ll_coord(earth_box(ll_to_earth(0,0),112000),1)::numeric(20,5),
       cube_ll_coord(earth_box(ll_to_earth(0,0),112000),2)::numeric(20,5),
       cube_ll_coord(earth_box(ll_to_earth(0,0),112000),3)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),112000),1)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),112000),2)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),112000),3)::numeric(20,5);
select cube_ll_coord(earth_box(ll_to_earth(0,0),pi()*earth()),1)::numeric(20,5),
       cube_ll_coord(earth_box(ll_to_earth(0,0),pi()*earth()),2)::numeric(20,5),
       cube_ll_coord(earth_box(ll_to_earth(0,0),pi()*earth()),3)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),pi()*earth()),1)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),pi()*earth()),2)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),pi()*earth()),3)::numeric(20,5);
select cube_ll_coord(earth_box(ll_to_earth(0,0),10*earth()),1)::numeric(20,5),
       cube_ll_coord(earth_box(ll_to_earth(0,0),10*earth()),2)::numeric(20,5),
       cube_ll_coord(earth_box(ll_to_earth(0,0),10*earth()),3)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),10*earth()),1)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),10*earth()),2)::numeric(20,5),
       cube_ur_coord(earth_box(ll_to_earth(0,0),10*earth()),3)::numeric(20,5);

--
-- Test for points that should be in bounding boxes.
--

select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,1))*1.00001) @
       ll_to_earth(0,1);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.1))*1.00001) @
       ll_to_earth(0,0.1);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.01))*1.00001) @
       ll_to_earth(0,0.01);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.001))*1.00001) @
       ll_to_earth(0,0.001);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.0001))*1.00001) @
       ll_to_earth(0,0.0001);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0.0001,0.0001))*1.00001) @
       ll_to_earth(0.0001,0.0001);
select earth_box(ll_to_earth(45,45),
       earth_distance(ll_to_earth(45,45),ll_to_earth(45.0001,45.0001))*1.00001) @
       ll_to_earth(45.0001,45.0001);
select earth_box(ll_to_earth(90,180),
       earth_distance(ll_to_earth(90,180),ll_to_earth(90.0001,180.0001))*1.00001) @
       ll_to_earth(90.0001,180.0001);

--
-- Test for points that shouldn't be in bounding boxes. Note that we need
-- to make points way outside, since some points close may be in the box
-- but further away than the distance we are testing.
--

select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,1))*.57735) @
       ll_to_earth(0,1);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.1))*.57735) @
       ll_to_earth(0,0.1);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.01))*.57735) @
       ll_to_earth(0,0.01);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.001))*.57735) @
       ll_to_earth(0,0.001);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0,0.0001))*.57735) @
       ll_to_earth(0,0.0001);
select earth_box(ll_to_earth(0,0),
       earth_distance(ll_to_earth(0,0),ll_to_earth(0.0001,0.0001))*.57735) @
       ll_to_earth(0.0001,0.0001);
select earth_box(ll_to_earth(45,45),
       earth_distance(ll_to_earth(45,45),ll_to_earth(45.0001,45.0001))*.57735) @
       ll_to_earth(45.0001,45.0001);
select earth_box(ll_to_earth(90,180),
       earth_distance(ll_to_earth(90,180),ll_to_earth(90.0001,180.0001))*.57735) @
       ll_to_earth(90.0001,180.0001);

--
-- Test the recommended constraints.
--

select is_point(ll_to_earth(0,0));
select cube_dim(ll_to_earth(0,0)) <= 3;
select abs(cube_distance(ll_to_earth(0,0), '(0)'::cube) / earth() - 1) <
       '10e-12'::float8;
select is_point(ll_to_earth(30,60));
select cube_dim(ll_to_earth(30,60)) <= 3;
select abs(cube_distance(ll_to_earth(30,60), '(0)'::cube) / earth() - 1) <
       '10e-12'::float8;
select is_point(ll_to_earth(60,90));
select cube_dim(ll_to_earth(60,90)) <= 3;
select abs(cube_distance(ll_to_earth(60,90), '(0)'::cube) / earth() - 1) <
       '10e-12'::float8;
select is_point(ll_to_earth(-30,-90));
select cube_dim(ll_to_earth(-30,-90)) <= 3;
select abs(cube_distance(ll_to_earth(-30,-90), '(0)'::cube) / earth() - 1) <
       '10e-12'::float8;
