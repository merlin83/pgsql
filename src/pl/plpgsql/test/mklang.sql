--
-- PL/pgSQL language declaration
--
-- $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/pl/plpgsql/test/Attic/mklang.sql,v 1.1 1998-08-24 19:16:27 momjian Exp $
--

create function plpgsql_call_handler() returns opaque
	as '/usr/local/pgsql/lib/plpgsql.so'
	language 'C';

create trusted procedural language 'plpgsql'
	handler plpgsql_call_handler
	lancompiler 'PL/pgSQL';

