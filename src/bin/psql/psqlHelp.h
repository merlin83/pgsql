/*-------------------------------------------------------------------------
 *
 * psqlHelp.h--
 *	  Help for query language syntax
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: psqlHelp.h,v 1.43 1998-06-15 18:39:49 momjian Exp $
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
	{"abort",
		"abort the current transaction",
	"abort [transaction];"},
	{"abort transaction",
		"abort the current transaction",
	"abort [transaction];"},
	{"alter table",
		"add/rename attributes, rename tables",
	"\talter table <class_name> [*] add column <attr> <type>\n\
\talter table <class_name> [*] rename [column] <attr1> to <attr2>\n\
\talter table <class_name1> rename to <class_name2>"},
	{"alter user",
		"alter system information for a user",
	"alter user <user_name>\n\
\t[with password <password>]\n\
\t[createdb | noccreatedb]\n\
\t[createuser | nocreateuser]\n\
\t[in group <group_1>, ..., <group_n>]\n\
\t[valid until '<abstime>'];"},
	{"begin",
		"begin a new transaction",
	"begin [transaction|work];"},
	{"begin transaction",
		"begin a new transaction",
	"begin [transaction|work];"},
	{"begin work",
		"begin a new transaction",
	"begin [transaction|work];"},
	{"cluster",
		"create a clustered index (from an existing index)",
	"cluster <index_name> on <relation_name>"},
	{"close",
		"close an existing cursor (cursor)",
	"close <cursorname>;"},
	{"commit",
		"commit a transaction",
	"commit [work]"},
	{"commit work",
		"commit a transaction",
	"commit [work]"},
	{"copy",
		"copy data to and from a table",
	"copy [binary] <class_name> [with oids]\n\
\t{to|from} {<filename>|stdin|stdout} [using delimiters <delim>];"},
	{"create",
		"Please be more specific:",
	"\tcreate aggregate\n\
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
	"create aggregate <agg_name> [as] (basetype = <data_type>, \n\
\t[sfunc1 = <sfunc_1>, stype1 = <sfunc1_return_type>]\n\
\t[sfunc2 = <sfunc_2>, stype2 = <sfunc2_return_type>]\n\
\t[,finalfunc = <final-function>]\n\
\t[,initcond1 = <initial-cond1>][,initcond2 = <initial-cond2>]);"},
	{"create database",
		"create a database",
	"create database <dbname> [with location = '<dbpath>']"},
	{"create function",
		"create a user-defined function",
	"create function <function_name> ([<type1>,...<typeN>]) returns <return_type>\n\
\tas '<object_filename>'|'<sql-queries>'\n\
\tlanguage 'c'|'sql'|'internal';"},
	{"create index",
		"construct an index",
	"create [unique] index <indexname> on <class_name> [using <access_method>]\n\
( <attr1> [<type_class1>] [,...] | <funcname>(<attr1>,...) [<type_class>] );"},
	{"create operator",
		"create a user-defined operator",
	"create operator <operator_name> (\n\
\t[leftarg = <type1>][,rightarg = <type2>]\n\
\t,procedure = <func_name>,\n\
\t[,commutator = <com_op>][,negator = <neg_op>]\n\
\t[,restrict = <res_proc>][,hashes]\n\
\t[,join = <join_proc>][,sort = <sort_op1>...<sort_opN>]);"},
	{"create rule",
		"define a new rule",
	"create rule <rule_name> as on\n\
\t[select|update|delete|insert]\n\
\tto <object> [where <qual>]\n\
\tdo [instead] [<action>|nothing| [<actions>]];"},
	{"create sequence",
		"create a new sequence number generator",
	"create sequence <sequence_name>\n\
\t[increment <NUMBER>]\n\
\t[start <NUMBER>]\n\
\t[minvalue <NUMBER>]\n\
\t[maxvalue <NUMBER>]\n\
\t[cache <NUMBER>]\n\
\t[cycle];"},
	{"create table",
		"create a new table",
	"create table <class_name>\n\
\t(<attr1> <type1> [default <expression>] [not null] [,...])\n\
\t[inherits (<class_name1>,...<class_nameN>)\n\
\t[[constraint <name>] check <condition> [,...] ]\n\
;"},
	{"create trigger",
		"create a new trigger",
	"create trigger <trigger_name> after|before event1 [or event2 [or event3] ]\n\
\ton <class_name> for each row|statement\n\
\texecute procedure <func_name> ([arguments])\n\
\n\
\teventX is one of INSERT, DELETE, UPDATE"},
	{"create type",
		"create a new user-defined base data type",
	"create type <typename> (\n\
\tinternallength = (<number> | variable),\n\
\t[externallength = (<number>|variable),]\n\
\tinput=<input_function>, output = <output_function>\n\
\t[,element = <typename>][,delimiter=<character>][,default=\'<string>\']\n\
\t[,send = <send_function>][,receive = <receive_function>][,passedbyvalue]);"},
	{"create user",
		"create a new user",
	"create user <user_name>\n\
\t[with password <password>]\n\
\t[createdb | nocreatedb]\n\
\t[createuser | nocreateuser]\n\
\t[in group <group_1>, ..., <group_n>]\n\
\t[valid until '<abstime>'];"},
	{"create view",
		"create a view",
	"create view <view_name> as\n\
\tselect\n\
\t<expr1>[as <attr1>][,... <exprN>[as <attrN>]]\n\
\t[from <from_list>]\n\
\t[where <qual>]\n\
\t[group by <group_list>];"},
	{"declare",
		"set up a cursor",
	"declare <cursorname> [binary] cursor for\n\
\tselect [distinct]\n\
\t<expr1> [as <attr1>],...<exprN> [as <attrN>]\n\
\t[from <from_list>]\n\
\t[where <qual>]\n\
\t[group by <group_list>]\n\
\t[having <having_clause>]\n\
\t[order by <attr1> [using <op1>],... <attrN> [using <opN>]]\n\
\t[union [all] select ...];"},
	{"delete",
		"delete tuples",
	"delete from <class_name> [where <qual>];"},
	{"drop",
		"Please be more specific:",
	"\tdrop aggregate\n\
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
	"drop aggregate <agg_name> <agg_type>|*;"},
	{"drop database",
		"remove a database",
	"drop database <dbname>"},
	{"drop function",
		"remove a user-defined function",
	"drop function <funcname> ([<type1>,....<typeN>]);"},
	{"drop index",
		"remove an existing index",
	"drop index <indexname>;"},
	{"drop operator",
		"remove a user-defined operator",
	"drop operator <operator_name> ([<ltype>|none],[<rtype>|none]);"},
	{"drop rule",
		"remove a rule",
	"drop rule <rulename>;"},
	{"drop sequence",
		"remove a sequence number generator",
	"drop sequence <sequence_name>[,...<sequence_nameN];"},
	{"drop table",
		"remove a table",
	"drop table <class_name>[,...<class_nameN];"},
	{"drop trigger",
		"remove a trigger",
	"drop trigger <trigger_name> on <class_name>;"},
	{"drop type",
		"remove a user-defined base type",
	"drop type <typename>;"},
	{"drop user",
		"remove a user from the system",
	"drop user <user_name>;"},
	{"drop view",
		"remove a view",
	"drop view <view_name>"},
	{"end",
		"end the current transaction",
	"end [transaction];"},
	{"end transaction",
		"end the current transaction",
	"end [transaction];"},
	{"explain",
		"explain the query execution plan",
	"explain [verbose] <query>"},
	{"fetch",
		"retrieve tuples from a cursor",
	"fetch [forward|backward] [<number>|all] [in <cursorname>];"},
	{"grant",
		"grant access control to a user or group",
	"grant <privilege[,privilege,...]> on <rel1>[,...<reln>] to \n\
[public | group <group> | <username>]\n\
\t privilege is {ALL | SELECT | INSERT | UPDATE | DELETE | RULE}"},
	{"insert",
		"insert tuples",
	"insert into <class_name> [(<attr1>...<attrN>)]\n\
\tvalues (<expr1>...<exprN>) |\n\
\tselect [distinct]\n\
\t<expr1>,...<exprN>\n\
\t[from <from_clause>]\n\
\t[where <qual>]\n\
\t[group by <group_list>]\n\
\t[having <having_clause>]\n\
\t[union [all] select ...];"},
	{"listen",
		"listen for notification on a relation",
	"listen <class_name>"},
	{"load",
		"dynamically load a module",
	"load <filename>;"},
	{"lock",
		"exclusive lock a table inside a transaction",
	"lock [table] <class_name>;"},
	{"move",
		"move an cursor position",
	"move [forward|backward] [<number>|all] [in <cursorname>];"},
	{"notify",
		"signal all frontends and backends listening on a relation",
	"notify <class_name>"},
	{"reset",
		"set run-time environment back to default",
	"reset {DateStyle | GEQO | R_PLANS}"},
	{"revoke",
		"revoke access control from a user or group",
	"revoke <privilege[,privilege,...]> on <rel1>[,...<reln>] from \n\
[public | group <group> | <username>]\n\
\t privilege is {ALL | SELECT | INSERT | UPDATE | DELETE | RULE}"},
	{"rollback",
		"abort a transaction",
	"rollback [transaction|work]"},
	{"select",
		"retrieve tuples",
	"select [distinct on <attr>] <expr1> [as <attr1>], ... <exprN> [as <attrN>]\n\
\t[into [table] <class_name>]\n\
\t[from <from_list>]\n\
\t[where <qual>]\n\
\t[group by <group_list>]\n\
\t[having <having_clause>]\n\
\t[order by <attr1> [ASC | DESC] [using <op1>], ... <attrN> ]\n\
\t[union [all] select ...];"},
	{"set",
		"set run-time environment",
	"set DateStyle to {'ISO' | 'SQL' | 'Postgres' | 'European' | 'US' | 'NonEuropean'}\n\
set GEQO to {'ON[=#]' | 'OFF'}\n\
set R_PLANS to {'ON' | 'OFF'}"},
	{"show",
		"show current run-time environment",
	"show {DateStyle | GEQO | R_PLANS}"},
	{"update",
		"update tuples",
	"update <class_name> set <attr1>=<expr1>,...<attrN>=<exprN> [from <from_clause>] [where <qual>];"},
	{"vacuum",
		"vacuum the database, i.e. cleans out deleted records, updates statistics",
	"\
vacuum [verbose] [analyze] [table]\n\
\tor\n\
vacuum [verbose]  analyze  [table [(attr1, ... attrN)]];"},
	{NULL, NULL, NULL}			/* important to keep a NULL terminator
								 * here! */
};
