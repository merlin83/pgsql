/*-------------------------------------------------------------------------
 *
 * psqlHelp.h--
 *    Help for query language syntax
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: psqlHelp.h,v 1.1.1.1 1996-07-09 06:22:15 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */

struct _helpStruct {
  char* cmd; /* the command name */
  char* help; /* the help associated with it */
  char* syntax; /* the syntax associated with it */
} ;
 
static struct _helpStruct QL_HELP[] = {
  { "abort",
      "abort the current transaction",
      "abort [transaction];"},
  { "abort transaction",
      "abort the current transaction",
      "abort [transaction];"},
  { "alter table",
      "add/rename attributes, rename tables",
      "alter table <relname> [*] add column <attr> <type>;\n\talter table <relname> [*] rename [column] <attr1> to <attr2>;\n\talter table <relname1> rename to <relname2>"},
  { "begin",
      "begin a new transaction",
      "begin [transaction|work];"},
  { "begin transaction",
      "begin a new transaction",
      "begin [transaction|work];"},
  { "begin work",
      "begin a new transaction",
      "begin [transaction|work];"},
  { "cluster",
      "create a clustered index (from an existing index)",
      "cluster <index_name> on <relation_name>"},
  { "close",
      "close an existing cursor (portal)",
      "close <portalname>;"},
  { "commit",
      "commit a transaction",
      "commit [work]"},
  { "commit work",
      "commit a transaction",
      "commit [work]"},
  { "copy",
      "copy data to and from a table",
      "copy [binary] [nonulls] <relname>\n\t{to|from} {<filename>|stdin|stdout} [using delimiters <delim>];"},
  { "create",   
      "Please more be specific:",
      "\tcreate aggregate\n\tcreate database\n\tcreate function\n\tcreate index\n\tcreate operator\n\tcreate rule\n\tcreate table\n\tcreate type\n\tcreate view"},
  { "create aggregate",
      "define an aggregate function",
      "create aggregate <agg_name> [as] (basetype = <data_type>, \n\t[sfunc1 = <sfunc_1>, stype1 = <sfunc1_return_type>]\n\t[sfunc2 = <sfunc_2>, stype2 = <sfunc2_return_type>]\n\t[,finalfunc = <final-function>]\n\t[,initcond1 = <initial-cond1>][,initcond2 = <initial-cond2>]);"},
  { "create database", 
    "create a database",
    "create database <dbname>"},
  { "create function",
      "create a user-defined function",
      "create function <function_name> ([<type1>,...<typeN>]) returns <return_type>\n\tas '<object_filename>'|'<sql-queries>'\n\tlanguage 'c'|'sql'|'internal';"},
  { "create index",
      "construct an index",
      "create index <indexname> on <relname> using <access_method> (<attr1>|<funcname>(<attr1>,...) <type_class1>);"},
  { "create operator",
      "create a user-defined operator",
      "create operator <operator_name> (\n\t[leftarg = <type1>][,rightarg = <type2>]\n\t,procedure = <func_name>,\n\t[,commutator = <com_op>][,negator = <neg_op>]\n\t[,restrict = <res_proc>][,hashes]\n\t[,join = <join_proc>][,sort = <sort_op1>...<sort_opN>]);"},
  { "create rule",
      "define a new rule",
      "create rule <rule_name> as on\n\t[select|update|delete|insert]\n\tto <object> [where <qual>]\n\tdo [instead] [<action>|nothing| [<actions>]];"},
  { "create table",
      "create a new table",
      "create table <relname> ( <attr1> <type1>,... <attrN> <typeN>)\n\t[inherits (<relname1>,...<relnameN>\n\tarchive=<archive_mode>\n\tstore=<smgr_name>\n\tarch_store=<smgr_name>];"},
  { "create type",
      "create a new user-defined base data type",
      "create type <typename> (\n\tinternallength = (<number> | variable),\n\t[externallength = (<number>|variable),]\n\tinput=<input_function>, output = <output_function>\n\t[,element = <typename>][,delimiter=<character>][,default=\'<string>\']\n\t[,send = <send_function>][,receive = <receive_function>][,passedbyvalue]);"},
  { "create view",
      "create a view",
      "create view <view_name> as select <expr1>[as <attr1>][,... <exprN>[as <attrN>]] [from <from_list>] [where <qual>];"},
  { "declare",
      "set up a cursor (portal)",
      "declare <portalname> [binary] cursor for\n\tselect [distinct]\n\t<expr1> [as <attr1>],...<exprN> [as <attrN>]\n\t[from <from_list>] [where <qual>]\n\t[order by <attr1> [using <op1>],... <attrN> [using <opN>]];"},
  { "delete",
      "delete tuples",
      "delete from <relname> [where <qual>];"},
  { "drop",   
      "Please more be specific:",
      "\tdrop aggregate\n\tdrop database\n\tdrop function\n\tdrop index\n\tdrop operator\n\tdrop rule\n\tdrop table\n\tdrop type\n\tdrop view"},
  { "drop aggregate",
      "remove an aggregate function",
      "drop aggregate <agg_name>;"},
  { "drop database",
     "remove a database",
     "drop database <dbname>"},
  { "drop function",
      "remove a user-defined function",
      "drop function <funcname> ([<type1>,....<typeN>]);"},
  { "drop index", 
      "remove an existing index",
      "drop index <indexname>;"},
  { "drop operator",
      "remove a user-defined operator",
      "drop operator <operator_name> ([<ltype>|none],[<rtype>|none]);"},
  { "drop rule",
      "remove a rule",
      "drop rule <rulename>;"},
  { "drop table", 
      "remove a table", 
      "drop table <relname>[,...<relnameN];"},
  { "drop type",
      "remove a user-defined base type",
      "drop type <typename>;"},
  { "drop view",
      "remove a view",
      "drop view <view_name>"},
  { "end",
      "end the current transaction",
      "end [transaction];"},
  { "end transaction",
      "end the current transaction",
      "end [transaction];"},
  { "explain",
      "explain the query execution plan",
      "explain [with {cost|full_plan}] <query>"},
  { "extend index",
      "extend a partial index",
      "extend index <indexname> [where <qual>];"},
  { "fetch",
      "retrieve tuples from a cursor (portal)",
      "fetch [forward|backward] [<number>|all] [in <portalname>];"},
  { "grant",
      "grant access control to a user or group",
      "grant <privilege[,privilege,...]> on <rel1>[,...<reln>] to \n[public | group <group> | <username>]\n\t privilege is {ALL | SELECT | INSERT | UPDATE | DELETE | RULE}"},
  { "insert",
      "insert tuples",
      "insert into <relname> [(<attr1>...<attrN>)]\n\t[values (<expr1>...<exprN>); |\n\tselect <expr1>,...<exprN> [from <from_clause>] [where <qual>];"},
  { "listen",
       "listen for notification on a relation",
       "listen <relname>"},
  { "load",
      "dynamically load a module",
      "load <filename>;"},
  { "notify",
      "signal all frontends and backends listening on a relation",
      "notify <relname>"},
  { "purge",
      "purge historical data",
      "purge <relname> [before <abstime>] [after <reltime>];"},
  { "revoke",
      "revoke access control from a user or group",
      "revoke <privilege[,privilege,...]> on <rel1>[,...<reln>] from \n[public | group <group> | <username>]\n\t privilege is {ALL | SELECT | INSERT | UPDATE | DELETE | RULE}"},
  { "rollback",
      "abort a transaction",
      "rollback [work]"},
  { "select",
      "retrieve tuples",
      "select [distinct on <attr>] <expr1> [as <attr1>], ... <exprN> [as <attrN>]\n\t[into table <relname>] [from <from_list>]\n\t[where <qual>]\n\t[order by <attr1>\n\t\t[using <op1>],..<attrN> [[using <opN>] | ASC | DESC]];" },
  { "update",
      "update tuples",
      "update <relname> set <attr1>=<expr1>,...<attrN>=<exprN> [from <from_clause>] [where <qual>];"},
  { "vacuum",
      "vacuum the database, i.e. cleans out deleted records, updates statistics",
      "vacuum;"},
  { NULL, NULL, NULL}  /* important to keep a NULL terminator here! */
};
