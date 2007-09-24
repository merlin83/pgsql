--
-- create user defined conversion
--
CREATE USER conversion_test_user WITH NOCREATEDB NOCREATEUSER;
SET SESSION AUTHORIZATION conversion_test_user;
CREATE CONVERSION myconv FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
--
-- cannot make same name conversion in same schema
--
CREATE CONVERSION myconv FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
--
-- create default conversion with qualified name
--
CREATE DEFAULT CONVERSION public.mydef FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
--
-- cannot make default conversion with same shcema/for_encoding/to_encoding
--
CREATE DEFAULT CONVERSION public.mydef2 FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
-- test comments
COMMENT ON CONVERSION myconv_bad IS 'foo';
COMMENT ON CONVERSION myconv IS 'bar';
COMMENT ON CONVERSION myconv IS NULL;
--
-- drop user defined conversion
--
DROP CONVERSION myconv;
DROP CONVERSION mydef;
--
-- make sure all pre-defined conversions are fine.
-- SQL_ASCII --> MULE_INTERNAL
SELECT CONVERT('foo', 'SQL_ASCII', 'MULE_INTERNAL');
-- MULE_INTERNAL --> SQL_ASCII
SELECT CONVERT('foo', 'MULE_INTERNAL', 'SQL_ASCII');
-- KOI8R --> MULE_INTERNAL
SELECT CONVERT('foo', 'KOI8R', 'MULE_INTERNAL');
-- MULE_INTERNAL --> KOI8R
SELECT CONVERT('foo', 'MULE_INTERNAL', 'KOI8R');
-- ISO-8859-5 --> MULE_INTERNAL
SELECT CONVERT('foo', 'ISO-8859-5', 'MULE_INTERNAL');
-- MULE_INTERNAL --> ISO-8859-5
SELECT CONVERT('foo', 'MULE_INTERNAL', 'ISO-8859-5');
-- WIN1251 --> MULE_INTERNAL
SELECT CONVERT('foo', 'WIN1251', 'MULE_INTERNAL');
-- MULE_INTERNAL --> WIN1251
SELECT CONVERT('foo', 'MULE_INTERNAL', 'WIN1251');
-- WIN866 --> MULE_INTERNAL
SELECT CONVERT('foo', 'WIN866', 'MULE_INTERNAL');
-- MULE_INTERNAL --> WIN866
SELECT CONVERT('foo', 'MULE_INTERNAL', 'WIN866');
-- KOI8R --> WIN1251
SELECT CONVERT('foo', 'KOI8R', 'WIN1251');
-- WIN1251 --> KOI8R
SELECT CONVERT('foo', 'WIN1251', 'KOI8R');
-- KOI8R --> WIN866
SELECT CONVERT('foo', 'KOI8R', 'WIN866');
-- WIN866 --> KOI8R
SELECT CONVERT('foo', 'WIN866', 'KOI8R');
-- WIN866 --> WIN1251
SELECT CONVERT('foo', 'WIN866', 'WIN1251');
-- WIN1251 --> WIN866
SELECT CONVERT('foo', 'WIN1251', 'WIN866');
-- ISO-8859-5 --> KOI8R
SELECT CONVERT('foo', 'ISO-8859-5', 'KOI8R');
-- KOI8R --> ISO-8859-5
SELECT CONVERT('foo', 'KOI8R', 'ISO-8859-5');
-- ISO-8859-5 --> WIN1251
SELECT CONVERT('foo', 'ISO-8859-5', 'WIN1251');
-- WIN1251 --> ISO-8859-5
SELECT CONVERT('foo', 'WIN1251', 'ISO-8859-5');
-- ISO-8859-5 --> WIN866
SELECT CONVERT('foo', 'ISO-8859-5', 'WIN866');
-- WIN866 --> ISO-8859-5
SELECT CONVERT('foo', 'WIN866', 'ISO-8859-5');
-- EUC_CN --> MULE_INTERNAL
SELECT CONVERT('foo', 'EUC_CN', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_CN
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_CN');
-- EUC_JP --> SJIS
SELECT CONVERT('foo', 'EUC_JP', 'SJIS');
-- SJIS --> EUC_JP
SELECT CONVERT('foo', 'SJIS', 'EUC_JP');
-- EUC_JP --> MULE_INTERNAL
SELECT CONVERT('foo', 'EUC_JP', 'MULE_INTERNAL');
-- SJIS --> MULE_INTERNAL
SELECT CONVERT('foo', 'SJIS', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_JP
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_JP');
-- MULE_INTERNAL --> SJIS
SELECT CONVERT('foo', 'MULE_INTERNAL', 'SJIS');
-- EUC_KR --> MULE_INTERNAL
SELECT CONVERT('foo', 'EUC_KR', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_KR
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_KR');
-- EUC_TW --> BIG5
SELECT CONVERT('foo', 'EUC_TW', 'BIG5');
-- BIG5 --> EUC_TW
SELECT CONVERT('foo', 'BIG5', 'EUC_TW');
-- EUC_TW --> MULE_INTERNAL
SELECT CONVERT('foo', 'EUC_TW', 'MULE_INTERNAL');
-- BIG5 --> MULE_INTERNAL
SELECT CONVERT('foo', 'BIG5', 'MULE_INTERNAL');
-- MULE_INTERNAL --> EUC_TW
SELECT CONVERT('foo', 'MULE_INTERNAL', 'EUC_TW');
-- MULE_INTERNAL --> BIG5
SELECT CONVERT('foo', 'MULE_INTERNAL', 'BIG5');
-- LATIN2 --> MULE_INTERNAL
SELECT CONVERT('foo', 'LATIN2', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN2
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN2');
-- WIN1250 --> MULE_INTERNAL
SELECT CONVERT('foo', 'WIN1250', 'MULE_INTERNAL');
-- MULE_INTERNAL --> WIN1250
SELECT CONVERT('foo', 'MULE_INTERNAL', 'WIN1250');
-- LATIN2 --> WIN1250
SELECT CONVERT('foo', 'LATIN2', 'WIN1250');
-- WIN1250 --> LATIN2
SELECT CONVERT('foo', 'WIN1250', 'LATIN2');
-- LATIN1 --> MULE_INTERNAL
SELECT CONVERT('foo', 'LATIN1', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN1
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN1');
-- LATIN3 --> MULE_INTERNAL
SELECT CONVERT('foo', 'LATIN3', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN3
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN3');
-- LATIN4 --> MULE_INTERNAL
SELECT CONVERT('foo', 'LATIN4', 'MULE_INTERNAL');
-- MULE_INTERNAL --> LATIN4
SELECT CONVERT('foo', 'MULE_INTERNAL', 'LATIN4');
-- SQL_ASCII --> UTF8
SELECT CONVERT('foo', 'SQL_ASCII', 'UTF8');
-- UTF8 --> SQL_ASCII
SELECT CONVERT('foo', 'UTF8', 'SQL_ASCII');
-- BIG5 --> UTF8
SELECT CONVERT('foo', 'BIG5', 'UTF8');
-- UTF8 --> BIG5
SELECT CONVERT('foo', 'UTF8', 'BIG5');
-- UTF8 --> KOI8R
SELECT CONVERT('foo', 'UTF8', 'KOI8R');
-- KOI8R --> UTF8
SELECT CONVERT('foo', 'KOI8R', 'UTF8');
-- UTF8 --> WIN1251
SELECT CONVERT('foo', 'UTF8', 'WIN1251');
-- WIN1251 --> UTF8
SELECT CONVERT('foo', 'WIN1251', 'UTF8');
-- UTF8 --> WIN1252
SELECT CONVERT('foo', 'UTF8', 'WIN1252');
-- WIN1252 --> UTF8
SELECT CONVERT('foo', 'WIN1252', 'UTF8');
-- UTF8 --> WIN866
SELECT CONVERT('foo', 'UTF8', 'WIN866');
-- WIN866 --> UTF8
SELECT CONVERT('foo', 'WIN866', 'UTF8');
-- EUC_CN --> UTF8
SELECT CONVERT('foo', 'EUC_CN', 'UTF8');
-- UTF8 --> EUC_CN
SELECT CONVERT('foo', 'UTF8', 'EUC_CN');
-- EUC_JP --> UTF8
SELECT CONVERT('foo', 'EUC_JP', 'UTF8');
-- UTF8 --> EUC_JP
SELECT CONVERT('foo', 'UTF8', 'EUC_JP');
-- EUC_KR --> UTF8
SELECT CONVERT('foo', 'EUC_KR', 'UTF8');
-- UTF8 --> EUC_KR
SELECT CONVERT('foo', 'UTF8', 'EUC_KR');
-- EUC_TW --> UTF8
SELECT CONVERT('foo', 'EUC_TW', 'UTF8');
-- UTF8 --> EUC_TW
SELECT CONVERT('foo', 'UTF8', 'EUC_TW');
-- GB18030 --> UTF8
SELECT CONVERT('foo', 'GB18030', 'UTF8');
-- UTF8 --> GB18030
SELECT CONVERT('foo', 'UTF8', 'GB18030');
-- GBK --> UTF8
SELECT CONVERT('foo', 'GBK', 'UTF8');
-- UTF8 --> GBK
SELECT CONVERT('foo', 'UTF8', 'GBK');
-- UTF8 --> LATIN2
SELECT CONVERT('foo', 'UTF8', 'LATIN2');
-- LATIN2 --> UTF8
SELECT CONVERT('foo', 'LATIN2', 'UTF8');
-- UTF8 --> LATIN3
SELECT CONVERT('foo', 'UTF8', 'LATIN3');
-- LATIN3 --> UTF8
SELECT CONVERT('foo', 'LATIN3', 'UTF8');
-- UTF8 --> LATIN4
SELECT CONVERT('foo', 'UTF8', 'LATIN4');
-- LATIN4 --> UTF8
SELECT CONVERT('foo', 'LATIN4', 'UTF8');
-- UTF8 --> LATIN5
SELECT CONVERT('foo', 'UTF8', 'LATIN5');
-- LATIN5 --> UTF8
SELECT CONVERT('foo', 'LATIN5', 'UTF8');
-- UTF8 --> LATIN6
SELECT CONVERT('foo', 'UTF8', 'LATIN6');
-- LATIN6 --> UTF8
SELECT CONVERT('foo', 'LATIN6', 'UTF8');
-- UTF8 --> LATIN7
SELECT CONVERT('foo', 'UTF8', 'LATIN7');
-- LATIN7 --> UTF8
SELECT CONVERT('foo', 'LATIN7', 'UTF8');
-- UTF8 --> LATIN8
SELECT CONVERT('foo', 'UTF8', 'LATIN8');
-- LATIN8 --> UTF8
SELECT CONVERT('foo', 'LATIN8', 'UTF8');
-- UTF8 --> LATIN9
SELECT CONVERT('foo', 'UTF8', 'LATIN9');
-- LATIN9 --> UTF8
SELECT CONVERT('foo', 'LATIN9', 'UTF8');
-- UTF8 --> LATIN10
SELECT CONVERT('foo', 'UTF8', 'LATIN10');
-- LATIN10 --> UTF8
SELECT CONVERT('foo', 'LATIN10', 'UTF8');
-- UTF8 --> ISO-8859-5
SELECT CONVERT('foo', 'UTF8', 'ISO-8859-5');
-- ISO-8859-5 --> UTF8
SELECT CONVERT('foo', 'ISO-8859-5', 'UTF8');
-- UTF8 --> ISO-8859-6
SELECT CONVERT('foo', 'UTF8', 'ISO-8859-6');
-- ISO-8859-6 --> UTF8
SELECT CONVERT('foo', 'ISO-8859-6', 'UTF8');
-- UTF8 --> ISO-8859-7
SELECT CONVERT('foo', 'UTF8', 'ISO-8859-7');
-- ISO-8859-7 --> UTF8
SELECT CONVERT('foo', 'ISO-8859-7', 'UTF8');
-- UTF8 --> ISO-8859-8
SELECT CONVERT('foo', 'UTF8', 'ISO-8859-8');
-- ISO-8859-8 --> UTF8
SELECT CONVERT('foo', 'ISO-8859-8', 'UTF8');
-- LATIN1 --> UTF8
SELECT CONVERT('foo', 'LATIN1', 'UTF8');
-- UTF8 --> LATIN1
SELECT CONVERT('foo', 'UTF8', 'LATIN1');
-- JOHAB --> UTF8
SELECT CONVERT('foo', 'JOHAB', 'UTF8');
-- UTF8 --> JOHAB
SELECT CONVERT('foo', 'UTF8', 'JOHAB');
-- SJIS --> UTF8
SELECT CONVERT('foo', 'SJIS', 'UTF8');
-- UTF8 --> SJIS
SELECT CONVERT('foo', 'UTF8', 'SJIS');
-- WIN1258 --> UTF8
SELECT CONVERT('foo', 'WIN1258', 'UTF8');
-- UTF8 --> WIN1258
SELECT CONVERT('foo', 'UTF8', 'WIN1258');
-- UHC --> UTF8
SELECT CONVERT('foo', 'UHC', 'UTF8');
-- UTF8 --> UHC
SELECT CONVERT('foo', 'UTF8', 'UHC');
-- UTF8 --> WIN1250
SELECT CONVERT('foo', 'UTF8', 'WIN1250');
-- WIN1250 --> UTF8
SELECT CONVERT('foo', 'WIN1250', 'UTF8');
-- UTF8 --> WIN1256
SELECT CONVERT('foo', 'UTF8', 'WIN1256');
-- WIN1256 --> UTF8
SELECT CONVERT('foo', 'WIN1256', 'UTF8');
-- UTF8 --> WIN874
SELECT CONVERT('foo', 'UTF8', 'WIN874');
-- WIN874 --> UTF8
SELECT CONVERT('foo', 'WIN874', 'UTF8');
-- UTF8 --> WIN1253
SELECT CONVERT('foo', 'UTF8', 'WIN1253');
-- WIN1253 --> UTF8
SELECT CONVERT('foo', 'WIN1253', 'UTF8');
-- UTF8 --> WIN1254
SELECT CONVERT('foo', 'UTF8', 'WIN1254');
-- WIN1254 --> UTF8
SELECT CONVERT('foo', 'WIN1254', 'UTF8');
-- UTF8 --> WIN1255
SELECT CONVERT('foo', 'UTF8', 'WIN1255');
-- WIN1255 --> UTF8
SELECT CONVERT('foo', 'WIN1255', 'UTF8');
-- UTF8 --> WIN1257
SELECT CONVERT('foo', 'UTF8', 'WIN1257');
-- WIN1257 --> UTF8
SELECT CONVERT('foo', 'WIN1257', 'UTF8');
-- UTF8 --> EUC_JIS_2004
SELECT CONVERT('foo', 'UTF8', 'EUC_JIS_2004');
-- EUC_JIS_2004 --> UTF8
SELECT CONVERT('foo', 'EUC_JIS_2004', 'UTF8');
-- UTF8 --> SHIFT_JIS_2004
SELECT CONVERT('foo', 'UTF8', 'SHIFT_JIS_2004');
-- SHIFT_JIS_2004 --> UTF8
SELECT CONVERT('foo', 'SHIFT_JIS_2004', 'UTF8');
-- EUC_JIS_2004 --> SHIFT_JIS_2004
SELECT CONVERT('foo', 'EUC_JIS_2004', 'SHIFT_JIS_2004');
-- SHIFT_JIS_2004 --> EUC_JIS_2004
SELECT CONVERT('foo', 'SHIFT_JIS_2004', 'EUC_JIS_2004');
--
-- return to the super user
--
RESET SESSION AUTHORIZATION;
DROP USER conversion_test_user;
