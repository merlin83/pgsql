--
-- CREATE ancillary data structures (i.e. indices)
--

--
-- BTREE
--
CREATE INDEX onek_unique1 ON onek USING btree(unique1 int4_ops);

CREATE INDEX onek_unique2 ON onek USING btree(unique2 int4_ops);

CREATE INDEX onek_hundred ON onek USING btree(hundred int4_ops);

CREATE INDEX onek_stringu1 ON onek USING btree(stringu1 char16_ops);

CREATE INDEX tenk1_unique1 ON tenk1 USING btree(unique1 int4_ops);

CREATE INDEX tenk1_unique2 ON tenk1 USING btree(unique2 int4_ops);

CREATE INDEX tenk1_hundred ON tenk1 USING btree(hundred int4_ops);

CREATE INDEX tenk2_unique1 ON tenk2 USING btree(unique1 int4_ops);

CREATE INDEX tenk2_unique2 ON tenk2 USING btree(unique2 int4_ops);

CREATE INDEX tenk2_hundred ON tenk2 USING btree(hundred int4_ops);

CREATE INDEX rix ON road USING btree (name text_ops);

CREATE INDEX iix ON ihighway USING btree (name text_ops);

CREATE INDEX six ON shighway USING btree (name text_ops);

--
-- BTREE ascending/descending cases
--
-- we load int4/text from pure descending data (each key is a new
-- low key) and c16/f8 from pure ascending data (each key is a new
-- high key).  we had a bug where new low keys would sometimes be
-- "lost".
--
CREATE INDEX bt_i4_index ON bt_i4_heap USING btree (seqno int4_ops);

CREATE INDEX bt_c16_index ON bt_c16_heap USING btree (seqno char16_ops);

CREATE INDEX bt_txt_index ON bt_txt_heap USING btree (seqno text_ops);

CREATE INDEX bt_f8_index ON bt_f8_heap USING btree (seqno float8_ops);

--
-- BTREE partial indices
-- partial indices are not supported in postgres95
--
--CREATE INDEX onek2_u1_prtl ON onek2 USING btree(unique1 int4_ops)
--	where onek2.unique1 < 20 or onek2.unique1 > 980;

--CREATE INDEX onek2_u2_prtl ON onek2 USING btree(unique2 int4_ops)
--	where onek2.stringu1 < 'B';

-- EXTEND INDEX onek2_u2_prtl where onek2.stringu1 < 'C';

-- EXTEND INDEX onek2_u2_prtl;

-- CREATE INDEX onek2_stu1_prtl ON onek2 USING btree(stringu1 char16_ops)
--	where onek2.stringu1 >= 'J' and onek2.stringu1 < 'K';

--
-- RTREE
-- 
-- rtrees use a quadratic page-splitting algorithm that takes a
-- really, really long time.  we don't test all rtree opclasses
-- in the regression test (we check them USING the sequoia 2000
-- benchmark).
--
CREATE INDEX rect2ind ON fast_emp4000 USING rtree (home_base bigbox_ops);


--
-- HASH
--
CREATE INDEX hash_i4_index ON hash_i4_heap USING hash (random int4_ops);

CREATE INDEX hash_c16_index ON hash_c16_heap USING hash (random char16_ops);

CREATE INDEX hash_txt_index ON hash_txt_heap USING hash (random text_ops);

CREATE INDEX hash_f8_index ON hash_f8_heap USING hash (random float8_ops);

-- CREATE INDEX hash_ovfl_index ON hash_ovfl_heap USING hash (x int4_ops);

