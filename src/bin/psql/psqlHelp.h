/*-------------------------------------------------------------------------
 *
 * psqlHelp.h
 *	  Help for query language syntax
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: psqlHelp.h,v 1.78 1999-10-26 03:48:58 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

struct _helpStruct
{
	char	   *cmd;			/* the command name */
	char	   *help;			/* the help associated with it */
	char	   *syntax;			/* the syntax associated with it */
};

static struct _helpStruct QL_HELP[] = {
	{"abort transaction",
		"abort the current transaction",
	"\
\tabort [transaction|work];"},
	{"alter table",
		"add/rename columns, rename tables",
	"\
\tALTER TABLE table_name [*] ADD COLUMN column_name type\n\
\tALTER TABLE table_name [*] RENAME [COLUMN] column_name1 TO column_name2\n\
\tALTER TABLE table_name1 RENAME TO table_name2"},
	{"alter user",
		"alter system information for a user",
	"\
\tALTER USER user_name\n\
\t[WITH PASSWORD password]\n\
\t[CREATEDB | NOCCREATEDB]\n\
\t[CREATEUSER | NOCREATEUSER]\n\
\t[IN GROUP group_1, ...groupN]\n\
\t[VALID UNTIL 'abstime'];"},
	{"begin work",
		"begin a new transaction",
	"\
\tBEGIN [WORK|TRANSACTION];"},
	{"close",
		"close an existing cursor (cursor)",
	"\
\tCLOSE cursorname;"},
	{"cluster",
		"create a clustered index (from an existing index)",
	"\
\tCLUSTER index_name ON relation_name"},
	{"comment",
		"add comment on object",
	"\
\tCOMMENT ON\n\
[\n\
  [ DATABASE | INDEX | RULE | SEQUENCE | TABLE | TYPE | VIEW ] <object_name> |\n\
  COLUMN <table_name>.<column_name>|\n\
  AGGREGATE <agg_name> <agg_type>|\n\
  FUNCTION <func_name> (arg1, arg2, ...)|\n\
  OPERATOR <op> (leftoperand_type rightoperand_type) |\n\
  TRIGGER <trigger_name> ON <table_name>\n\
] IS 'text'},
	{"commit work",
		"commit a transaction",
	"\
\tCOMMIT [WORK|TRANSACTION]"},
	{"copy",
		"copy data to and from a table",
	"\
\tCOPY [BINARY] table_name [WITH OIDS]\n\
\tTO|FROM filename|STDIN|STDOUT [USING DELIMITERS 'delim'];"},
	{"create",
		"Please be more specific:",
	"\
\tcreate aggregate\n\
\tcreate database\n\
\tcreate function\n\
\tcreate index\n\
\tcreate operator\n\
\tcreate rule\n\
\tcreate sequence\n\
\tcreate table\n\
\tcreate trigger\n\
\tcreate type\n\
\tcreate view"},
	{"create aggregate",
		"define an aggregate function",
	"\
\tCREATE AGGREGATE agg_name [AS] (BASETYPE = data_type, \n\
\t[SFUNC1 = sfunc_1, STYPE1 = sfunc1_return_type]\n\
\t[SFUNC2 = sfunc_2, STYPE2 = sfunc2_return_type]\n\
\t[,FINALFUNC = final-function]\n\
\t[,INITCOND1 = initial-cond1][,INITCOND2 = initial-cond2]);"},
	{"create database",
		"create a database",
	"\
\tCREATE DATABASE dbname [WITH LOCATION = 'dbpath']"},
	{"create function",
		"create a user-defined function",
	"\
\tCREATE FUNCTION function_name ([type1, ...typeN]) RETURNS return_type\n\
\t[WITH ( column_names )]\n\
\tAS 'sql_queries'|'builtin_function_name'|'procedural_commands'\n\
\tLANGUAGE 'sql'|'internal'|'procedural_language_name';\n\
\n\
OR\n\
\n\
\tCREATE FUNCTION function_name ([type1, ...typeN]) RETURNS return_type\n\
\t[WITH ( column_names )]\n\
\tAS 'object_filename' [, 'link_symbol']\n\
\tLANGUAGE 'C';"},
	{"create index",
		"construct an index",
	"\
\tCREATE [UNIQUE] INDEX indexname ON table_name [USING access_method]\n\
( column_name1 [type_class1], ...column_nameN |\n\
  funcname(column_name1, ...) [type_class] );"},
	{"create operator",
		"create a user-defined operator",
	"\
\tCREATE OPERATOR operator_name (\n\
\t[LEFTARG = type1][,RIGHTARG = type2]\n\
\t,PROCEDURE = func_name,\n\
\t[,COMMUTATOR = com_op][,NEGATOR = neg_op]\n\
\t[,RESTRICT = res_proc][,JOIN = join_proc][,HASHES]\n\
\t[,SORT1 = left_sort_op][,SORT2 = right_sort_op]);"},
	{"create rule",
		"define a new rule",
	"\
\tCREATE RULE rule_name AS ON\n\
\t{ SELECT | UPDATE | DELETE | INSERT }\n\
\tTO object_name [WHERE qual]\n\
\tDO [INSTEAD] [action|NOTHING|[actions]];"},
	{"create sequence",
		"create a new sequence number generator",
	"\
\tCREATE SEQUENCE sequence_name\n\
\t[INCREMENT number]\n\
\t[START number]\n\
\t[MINVALUE number]\n\
\t[MAXVALUE number]\n\
\t[CACHE number]\n\
\t[CYCLE];"},
	{"create table",
		"create a new table",
	"\
\tCREATE [TEMP] TABLE table_name\n\
\t(column_name1 type1 [DEFAULT expression] [NOT NULL], ...column_nameN\n\
\t[[CONSTRAINT name] CHECK condition1, ...conditionN] )\n\
\t[INHERITS (table_name1, ...table_nameN)\n\
;"},
	{"create trigger",
		"create a new trigger",
	"\
\tCREATE TRIGGER trigger_name AFTER|BEFORE event1 [OR event2 [OR event3] ]\n\
\tON table_name FOR EACH ROW|STATEMENT\n\
\tEXECUTE PROCEDURE func_name ([arguments])\n\
\n\
\teventX is one of INSERT, DELETE, UPDATE"},
	{"create type",
		"create a new user-defined base data type",
	"\
\tCREATE TYPE typename (\n\
\tINTERNALLENGTH = (number|VARIABLE),\n\
\t[EXTERNALLENGTH = (number|VARIABLE),]\n\
\tINPUT = input_function, OUTPUT = output_function\n\
\t[,ELEMENT = typename][,DELIMITER = character][,DEFAULT=\'<string>\']\n\
\t[,SEND = send_function][,RECEIVE = receive_function][,PASSEDBYVALUE]);"},
	{"create user",
		"create a new user",
	"\
\tCREATE USER user_name\n\
\t[WITH PASSWORD password]\n\
\t[CREATEDB | NOCREATEDB]\n\
\t[CREATEUSER | NOCREATEUSER]\n\
\t[IN GROUP group1, ...groupN]\n\
\t[VALID UNTIL 'abstime'];"},
	{"create view",
		"create a view",
	"\
\tCREATE VIEW view_name AS\n\
\tSELECT [DISTINCT [ON column_nameN]]\n\
\texpr1 [AS column_name1], ...exprN\n\
\t[FROM table_list]\n\
\t[WHERE qual]\n\
\t[GROUP BY group_list];"},
	{"declare",
		"set up a cursor",
	"\
\tDECLARE cursorname [BINARY] CURSOR FOR\n\
\tSELECT [DISTINCT [ON column_nameN]]\n\
\texpr1 [AS column_name1], ...exprN\n\
\t[FROM table_list]\n\
\t[WHERE qual]\n\
\t[GROUP BY group_list]\n\
\t[HAVING having_clause]\n\
\t[ORDER BY column_name1 [USING op1], ...column_nameN]\n\
\t[ { UNION [ALL] | INTERSECT | EXCEPT } SELECT ...];"},
	{"delete",
		"delete tuples",
	"\
\tDELETE FROM table_name [WHERE qual];"},
	{"drop",
		"Please be more specific:",
	"\
\tdrop aggregate\n\
\tdrop database\n\
\tdrop function\n\
\tdrop index\n\
\tdrop operator\n\
\tdrop rule\n\
\tdrop sequence\n\
\tdrop table\n\
\tdrop trigger\n\
\tdrop type\n\
\tdrop view"},
	{"drop aggregate",
		"remove an aggregate function",
	"\
\tDROP AGGREGATE agg_name agg_type|*;"},
	{"drop database",
		"remove a database",
	"\
\tDROP DATABASE dbname"},
	{"drop function",
		"remove a user-defined function",
	"\
\tDROP FUNCTION funcname ([type1, ...typeN]);"},
	{"drop index",
		"remove an existing index",
	"\
\tDROP INDEX indexname;"},
	{"drop operator",
		"remove a user-defined operator",
	"\
\tDROP OPERATOR operator_name ([ltype|NONE],[RTYPE|none]);"},
	{"drop rule",
		"remove a rule",
	"\
\tDROP RULE rulename;"},
	{"drop sequence",
		"remove a sequence number generator",
	"\
\tDROP SEQUENCE sequence_name[, ...sequence_nameN];"},
	{"drop table",
		"remove a table",
	"\
\tDROP TABLE table_name1, ...table_nameN;"},
	{"drop trigger",
		"remove a trigger",
	"\
\tDROP TRIGGER trigger_name ON table_name;"},
	{"drop type",
		"remove a user-defined base type",
	"\
\tDROP TYPE typename;"},
	{"drop user",
		"remove a user from the system",
	"\
\tDROP USER user_name;"},
	{"drop view",
		"remove a view",
	"\
\tDROP VIEW view_name"},
	{"end work",
		"end the current transaction",
	"\
\tEND [WORK|TRANSACTION];"},
	{"explain",
		"explain the query execution plan",
	"\
\tEXPLAIN [VERBOSE] query"},
	{"fetch",
		"retrieve tuples from a cursor",
	"\
\tFETCH [FORWARD|BACKWARD] [number|ALL] [IN cursorname];"},
	{"grant",
		"grant access control to a user or group",
	"\
\tGRANT privilege1, ...privilegeN ON rel1, ...relN TO \n\
{ PUBLIC | GROUP group | username }\n\
\t privilege is { ALL | SELECT | INSERT | UPDATE | DELETE | RULE }"},
	{"insert",
		"insert tuples",
	"\
\tINSERT INTO table_name [(column_name1, ...column_nameN)]\n\
\tVALUES (expr1,..exprN) |\n\
\tSELECT [DISTINCT [ON column_nameN]]\n\
\texpr1, ...exprN\n\
\t[FROM table_list]\n\
\t[WHERE qual]\n\
\t[GROUP BY group_list]\n\
\t[HAVING having_clause]\n\
\t[ { UNION [ALL] | INTERSECT | EXCEPT } SELECT ...];"},
	{"listen",
		"listen for notification on a condition name",
	"\
\tLISTEN name|\"non-name string\""},
	{"load",
		"dynamically load a module",
	"\
\tLOAD 'filename';"},
	{"lock",
		"exclusive lock a table inside a transaction",
	"\
\tLOCK [TABLE] table_name \n\
\t[IN [ROW|ACCESS] [SHARE|EXCLUSIVE] | [SHARE ROW EXCLUSIVE] MODE];"},
	{"move",
		"move an cursor position",
	"\
\tMOVE [FORWARD|BACKWARD] [number|ALL] [IN cursorname];"},
	{"notify",
		"signal all frontends listening on a condition name",
	"\
\tNOTIFY name|\"non-name string\""},
	{"reset",
		"set run-time environment back to default",
	"\
\tRESET DATESTYLE|COST_HEAP|COST_INDEX|GEQO|KSQO|PG_OPTIONS|\n\
TIMEZONE|XACTISOLEVEL|CLIENT_ENCODING|SERVER_ENCODING"},
	{"revoke",
		"revoke access control from a user or group",
	"\
\tREVOKE privilege1, ...privilegeN ON rel1, ...relN FROM \n\
{ PUBLIC | GROUP group | username }\n\
\t privilege is { ALL | SELECT | INSERT | UPDATE | DELETE | RULE }"},
	{"rollback work",
		"abort a transaction",
	"\
\tROLLBACK [WORK|TRANSACTION]"},
	{"select",
		"retrieve tuples",
	"\
\tSELECT [DISTINCT [ON column_nameN]] expr1 [AS column_name1], ...exprN\n\
\t[INTO [TEMP] [TABLE] table_name]\n\
\t[FROM table_list]\n\
\t[WHERE qual]\n\
\t[GROUP BY group_list]\n\
\t[HAVING having_clause]\n\
\t[ { UNION [ALL] | INTERSECT | EXCEPT } SELECT ...]\n\
\t[ORDER BY column_name1 [ASC|DESC] [USING op1], ...column_nameN ]\n\
\t[FOR UPDATE [OF table_name...]]\n\
\t[LIMIT count [OFFSET|, count]];"},
	{"set",
		"set run-time environment",
	"\
\tSET DATESTYLE TO 'ISO'|'SQL'|'Postgres'|'European'|'US'|'NonEuropean'\n\
\tSET COST_HEAP TO #\n\
\tSET COST_INDEX TO #\n\
\tSET GEQO TO 'ON[=#]'|'OFF'\n\
\tSET KSQO TO 'ON'|'OFF'\n\
\tSET PG_OPTIONS TO 'value'\n\
\tSET TIMEZONE TO 'value'\n\
\tSET TRANSACTION ISOLATION LEVEL 'SERIALIZABLE'|'READ COMMITTED'\n\
\tSET CLIENT_ENCODING|NAMES TO 'EUC_JP'|'SJIS'|'EUC_CN'|'EUC_KR'|'EUC_TW'|\n\
\t  'BIG5'|'MULE_INTERNAL'|'LATIN1'|'LATIN2'|'LATIN3'|'LATIN4'|'LATIN5'|\n\
\t  'KOI8|'WIN'|'ALT'|'WIN1250'\n\
\tSET SERVER_ENCODING TO 'EUC_JP'|'SJIS'|'EUC_CN'|'EUC_KR'|'EUC_TW'|\n\
\t  'BIG5'|'MULE_INTERNAL'|'LATIN1'|'LATIN2'|'LATIN3'|'LATIN4'|'LATIN5'|\n\
\t  'KOI8|'WIN'|'ALT'"},
	{"show",
		"show current run-time environment",
	"\
\tSHOW DATESTYLE|COST_HEAP|COST_INDEX|GEQO|KSQO|PG_OPTIONS|\n\
TIMEZONE|XACTISOLEVEL|CLIENT_ENCODING|SERVER_ENCODING"},
	{"unlisten",
		"stop listening for notification on a condition name",
	"\
\tUNLISTEN name|\"non-name string\"|\"*\""},
	{"truncate",
		"quickly removes all rows from a table",
	"\
\tTRUNCATE TABLE table_name"},
	{"update",
		"update tuples",
	"\
\tUPDATE table_name SET column_name1 = expr1, ...column_nameN = exprN\n\
\t[FROM table_list]\n\
\t[WHERE qual];"},
	{"vacuum",
		"vacuum the database, i.e. cleans out deleted records, updates statistics",
	"\
\tVACUUM [VERBOSE] [ANALYZE] [table]\n\
\tor\n\
\tVACUUM [VERBOSE]  ANALYZE  [table [(column_name1, ...column_nameN)]];"},
	{NULL, NULL, NULL}			/* important to keep a NULL terminator here!*/
};
