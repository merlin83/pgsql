%{ /* -*-text-*- */

/*#define YYDEBUG 1*/
/*-------------------------------------------------------------------------
 *
 * gram.y
 *	  POSTGRES SQL YACC rules/actions
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/gram.y,v 2.104 1999-09-29 16:06:06 wieck Exp $
 *
 * HISTORY
 *	  AUTHOR			DATE			MAJOR EVENT
 *	  Andrew Yu			Sept, 1994		POSTQUEL to SQL conversion
 *	  Andrew Yu			Oct, 1994		lispy code conversion
 *
 * NOTES
 *	  CAPITALS are used to represent terminal symbols.
 *	  non-capitals are used to represent non-terminals.
 *	  SQL92-specific syntax is separated from plain SQL/Postgres syntax
 *	  to help isolate the non-extensible portions of the parser.
 *
 *	  if you use list, make sure the datum is a node so that the printing
 *	  routines work
 *
 * WARNING
 *	  sometimes we assign constants to makeStrings. Make sure we don't free
 *	  those.
 *
 *-------------------------------------------------------------------------
 */
#include <ctype.h>

#include "postgres.h"
#include "access/htup.h"
#include "access/xact.h"
#include "catalog/catname.h"
#include "catalog/pg_type.h"
#include "nodes/parsenodes.h"
#include "nodes/print.h"
#include "parser/analyze.h"
#include "parser/gramparse.h"
#include "parser/parse_type.h"
#include "storage/bufpage.h"
#include "storage/lmgr.h"
#include "utils/acl.h"
#include "utils/numeric.h"

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"
#endif

static char saved_relname[NAMEDATALEN];  /* need this for complex attributes */
static bool QueryIsRule = FALSE;
static Oid	*param_type_info;
static int	pfunc_num_args;
extern List *parsetree;


/*
 * If you need access to certain yacc-generated variables and find that
 * they're static by default, uncomment the next line.  (this is not a
 * problem, yet.)
 */
/*#define __YYSCLASS*/

static char *xlateSqlFunc(char *);
static char *xlateSqlType(char *);
static Node *makeA_Expr(int oper, char *opname, Node *lexpr, Node *rexpr);
static Node *makeRowExpr(char *opr, List *largs, List *rargs);
static void mapTargetColumns(List *source, List *target);
static List *makeConstantList( A_Const *node);
static char *FlattenStringList(List *list);
static char *fmtId(char *rawid);
static void param_type_init(Oid *typev, int nargs);
static Node *doNegate(Node *n);

Oid	param_type(int t); /* used in parse_expr.c */

/* old versions of flex define this as a macro */
#if defined(yywrap)
#undef yywrap
#endif /* yywrap */
%}


%union
{
	double				dval;
	int					ival;
	char				chr;
	char				*str;
	bool				boolean;
	bool*				pboolean;	/* for pg_shadow privileges */
	List				*list;
	Node				*node;
	Value				*value;

	Attr				*attr;

	TypeName			*typnam;
	DefElem				*defelt;
	ParamString			*param;
	SortGroupBy			*sortgroupby;
	JoinExpr			*joinexpr;
	IndexElem			*ielem;
	RangeVar			*range;
	RelExpr				*relexp;
	A_Indices			*aind;
	ResTarget			*target;
	ParamNo				*paramno;

	VersionStmt			*vstmt;
	DefineStmt			*dstmt;
	RuleStmt			*rstmt;
	InsertStmt			*astmt;
}

%type <node>	stmt,
		AddAttrStmt, ClosePortalStmt,
		CopyStmt, CreateStmt, CreateAsStmt, CreateSeqStmt, DefineStmt, DestroyStmt,
		TruncateStmt,
		ExtendStmt, FetchStmt,	GrantStmt, CreateTrigStmt, DropTrigStmt,
		CreatePLangStmt, DropPLangStmt,
		IndexStmt, ListenStmt, UnlistenStmt, LockStmt, OptimizableStmt,
		ProcedureStmt, RemoveAggrStmt, RemoveOperStmt,
		RemoveFuncStmt, RemoveStmt,
		RenameStmt, RevokeStmt, RuleStmt, TransactionStmt, ViewStmt, LoadStmt,
		CreatedbStmt, DestroydbStmt, VacuumStmt, CursorStmt, SubSelect,
		UpdateStmt, InsertStmt, select_clause, SelectStmt, NotifyStmt, DeleteStmt, 
		ClusterStmt, ExplainStmt, VariableSetStmt, VariableShowStmt, VariableResetStmt,
		CreateUserStmt, AlterUserStmt, DropUserStmt, RuleActionStmt,
		ConstraintsSetStmt,

%type <str>		opt_database1, opt_database2, location, encoding

%type <ival>	opt_lock, lock_type
%type <boolean>	opt_lmode

%type <pboolean> user_createdb_clause, user_createuser_clause
%type <str>		user_passwd_clause
%type <str>		user_valid_clause
%type <list>	user_group_list, user_group_clause

%type <boolean>	TriggerActionTime, TriggerForSpec, PLangTrusted

%type <ival>	OptConstrTrigDeferrable, OptConstrTrigInitdeferred
%type <str>		OptConstrFromTable

%type <str>		TriggerEvents, TriggerFuncArg

%type <str>		relation_name, copy_file_name, copy_delimiter, def_name,
		database_name, access_method_clause, access_method, attr_name,
		class, index_name, name, func_name, file_name, aggr_argtype

%type <str>		opt_id, opt_portal_name,
		all_Op, MathOp, opt_name, opt_unique,
		OptUseOp, opt_class, SpecialRuleRelation

%type <str>		opt_level
%type <str>		privileges, operation_commalist, grantee
%type <chr>		operation, TriggerOneEvent

%type <list>	stmtblock, stmtmulti,
		result, relation_name_list, OptTableElementList,
		OptInherit, definition,
		opt_with, func_args, func_args_list, func_as,
		oper_argtypes, RuleActionList, RuleActionBlock, RuleActionMulti,
		opt_column_list, columnList, opt_va_list, va_list,
		sort_clause, sortby_list, index_params, index_list, name_list,
		from_clause, from_expr, table_list, opt_array_bounds,
		expr_list, attrs, target_list, update_target_list,
		def_list, opt_indirection, group_clause, TriggerFuncArgs,
		opt_select_limit

%type <node>	func_return
%type <boolean>	set_opt

%type <boolean>	TriggerForOpt, TriggerForType, OptTemp, OptTempType, OptTempScope

%type <list>	for_update_clause, update_list
%type <boolean>	opt_union
%type <boolean>	opt_table
%type <boolean>	opt_trans

%type <list>	join_clause_with_union, join_clause, join_list, join_qual, using_list
%type <node>	join_expr, using_expr
%type <str>		join_outer
%type <ival>	join_type

%type <list>	extract_list, position_list
%type <list>	substr_list, substr_from, substr_for, trim_list
%type <list>	opt_interval

%type <boolean> opt_inh_star, opt_binary, opt_instead, opt_with_copy,
				index_opt_unique, opt_verbose, opt_analyze
%type <boolean> opt_cursor

%type <ival>	copy_dirn, def_type, opt_direction, remove_type,
				opt_column, event

%type <ival>	fetch_how_many

%type <node>	select_limit_value, select_offset_value

%type <list>	OptSeqList
%type <defelt>	OptSeqElem

%type <dstmt>	def_rest
%type <astmt>	insert_rest

%type <node>	OptTableElement, ConstraintElem
%type <node>	columnDef, alter_clause
%type <defelt>	def_elem
%type <node>	def_arg, columnElem, where_clause,
				a_expr, a_expr_or_null, b_expr, AexprConst,
				in_expr, having_clause
%type <list>	row_descriptor, row_list, c_list, c_expr, in_expr_nodes
%type <node>	row_expr
%type <str>		row_op
%type <node>	case_expr, case_arg, when_clause, case_default
%type <list>	when_clause_list
%type <ival>	sub_type
%type <list>	OptCreateAs, CreateAsList
%type <node>	CreateAsElement
%type <value>	NumericOnly, FloatOnly, IntegerOnly
%type <attr>	event_object, attr
%type <sortgroupby>		sortby
%type <ielem>	index_elem, func_index
%type <range>	table_expr
%type <relexp>	relation_expr
%type <target>	target_el, update_target_el
%type <paramno> ParamNo

%type <typnam>	Typename, opt_type, SimpleTypename,
				Generic, Numeric, Character, Datetime
%type <str>		generic, numeric, character, datetime
%type <str>		extract_arg
%type <str>		opt_charset, opt_collate
%type <str>		opt_float
%type <ival>	opt_numeric, opt_decimal
%type <boolean>	opt_varying, opt_timezone

%type <ival>	Iconst
%type <str>		Sconst
%type <str>		UserId, var_value, zone_value
%type <str>		ColId, ColLabel
%type <str>		TypeId

%type <node>	TableConstraint
%type <list>	constraint_list, constraint_expr
%type <list>	default_list, default_expr
%type <list>	ColPrimaryKey, ColQualList, ColQualifier
%type <node>	ColConstraint, ColConstraintElem
%type <list>	key_actions, key_action
%type <str>		key_match, key_reference

%type <list>	constraints_set_list
%type <list>	constraints_set_namelist
%type <boolean>	constraints_set_mode

/*
 * If you make any token changes, remember to:
 *		- use "yacc -d" and update parse.h
 *		- update the keyword table in parser/keywords.c
 */

/* Reserved word tokens
 * SQL92 syntax has many type-specific constructs.
 * So, go ahead and make these types reserved words,
 *  and call-out the syntax explicitly.
 * This gets annoying when trying to also retain Postgres' nice
 *  type-extensible features, but we don't really have a choice.
 * - thomas 1997-10-11
 */

/* Keywords (in SQL92 reserved words) */
%token	ABSOLUTE, ACTION, ADD, ALL, ALTER, AND, ANY, AS, ASC,
		BEGIN_TRANS, BETWEEN, BOTH, BY,
		CASCADE, CASE, CAST, CHAR, CHARACTER, CHECK, CLOSE,
		COALESCE, COLLATE, COLUMN, COMMIT,
		CONSTRAINT, CONSTRAINTS, CREATE, CROSS, CURRENT, CURRENT_DATE,
		CURRENT_TIME, CURRENT_TIMESTAMP, CURRENT_USER, CURSOR,
		DAY_P, DECIMAL, DECLARE, DEFAULT, DELETE, DESC, DISTINCT, DOUBLE, DROP,
		ELSE, END_TRANS, EXCEPT, EXECUTE, EXISTS, EXTRACT,
		FALSE_P, FETCH, FLOAT, FOR, FOREIGN, FROM, FULL,
		GLOBAL, GRANT, GROUP, HAVING, HOUR_P,
		IN, INNER_P, INSENSITIVE, INSERT, INTERSECT, INTERVAL, INTO, IS,
		ISOLATION, JOIN, KEY, LANGUAGE, LEADING, LEFT, LEVEL, LIKE, LOCAL,
		MATCH, MINUTE_P, MONTH_P, NAMES,
		NATIONAL, NATURAL, NCHAR, NEXT, NO, NOT, NULLIF, NULL_P, NUMERIC,
		OF, ON, ONLY, OPTION, OR, ORDER, OUTER_P,
		PARTIAL, POSITION, PRECISION, PRIMARY, PRIOR, PRIVILEGES, PROCEDURE, PUBLIC,
		READ, REFERENCES, RELATIVE, REVOKE, RIGHT, ROLLBACK,
		SCROLL, SECOND_P, SELECT, SET, SUBSTRING,
		TABLE, TEMP, TEMPORARY, THEN, TIME, TIMESTAMP, TIMEZONE_HOUR,
		TIMEZONE_MINUTE, TO, TRAILING, TRANSACTION, TRIM, TRUE_P,
		UNION, UNIQUE, UPDATE, USER, USING,
		VALUES, VARCHAR, VARYING, VIEW,
		WHEN, WHERE, WITH, WORK, YEAR_P, ZONE

/* Keywords (in SQL3 reserved words) */
%token	DEFERRABLE, DEFERRED,
		IMMEDIATE, INITIALLY,
		PENDANT,
		RESTRICT,
		TRIGGER

/* Keywords (in SQL92 non-reserved words) */
%token	COMMITTED, SERIALIZABLE, TYPE_P

/* Keywords for Postgres support (not in SQL92 reserved words)
 *
 * The CREATEDB and CREATEUSER tokens should go away
 * when some sort of pg_privileges relation is introduced.
 * - Todd A. Brandys 1998-01-01?
 */
%token	ABORT_TRANS, ACCESS, AFTER, AGGREGATE, ANALYZE,
		BACKWARD, BEFORE, BINARY, 
		CACHE, CLUSTER, COPY, CREATEDB, CREATEUSER, CYCLE,
		DATABASE, DELIMITERS, DO,
		EACH, ENCODING, EXCLUSIVE, EXPLAIN, EXTEND,
		FORWARD, FUNCTION, HANDLER,
		INCREMENT, INDEX, INHERITS, INSTEAD, ISNULL,
		LANCOMPILER, LIMIT, LISTEN, LOAD, LOCATION, LOCK_P,
		MAXVALUE, MINVALUE, MODE, MOVE,
		NEW, NOCREATEDB, NOCREATEUSER, NONE, NOTHING, NOTIFY, NOTNULL,
		OFFSET, OIDS, OPERATOR, PASSWORD, PROCEDURAL,
		RENAME, RESET, RETURNS, ROW, RULE,
		SEQUENCE, SERIAL, SETOF, SHARE, SHOW, START, STATEMENT, STDIN, STDOUT,
		TRUNCATE, TRUSTED, 
		UNLISTEN, UNTIL, VACUUM, VALID, VERBOSE, VERSION

/* Special keywords, not in the query language - see the "lex" file */
%token <str>	IDENT, SCONST, Op
%token <ival>	ICONST, PARAM
%token <dval>	FCONST

/* these are not real. they are here so that they get generated as #define's*/
%token			OP

/* precedence */
%left		OR
%left		AND
%right		NOT
%right		'='
%nonassoc	'<' '>'
%nonassoc	LIKE
%nonassoc	BETWEEN
%nonassoc	IN
%left		Op				/* multi-character ops and user-defined operators */
%nonassoc	NOTNULL
%nonassoc	ISNULL
%nonassoc	NULL_P
%nonassoc	IS
%left		'+' '-'
%left		'*' '/' '%'
%left		'^'
%left		'|'				/* this is the relation union op, not logical or */
/* Unary Operators */
%right		':'
%left		';'				/* end of statement or natural log */
%right		UMINUS
%left		'.'
%left		'[' ']'
%nonassoc	TYPECAST
%left		UNION INTERSECT EXCEPT
%%

stmtblock:  stmtmulti opt_semi
				{ parsetree = $1; }
		;

stmtmulti:  stmtmulti ';' stmt
				{ $$ = lappend($1, $3); }
		| stmt
				{ $$ = lcons($1,NIL); }
		;

opt_semi:	';'
		|	/*EMPTY*/
		;
		
stmt :	  AddAttrStmt
		| AlterUserStmt
		| ClosePortalStmt
		| CopyStmt
		| CreateStmt
		| CreateAsStmt
		| CreateSeqStmt
		| CreatePLangStmt
		| CreateTrigStmt
		| CreateUserStmt
		| ClusterStmt
		| DefineStmt
		| DestroyStmt		
		| TruncateStmt
		| DropPLangStmt
		| DropTrigStmt
		| DropUserStmt
		| ExtendStmt
		| ExplainStmt
		| FetchStmt
		| GrantStmt
		| IndexStmt
		| ListenStmt
		| UnlistenStmt
		| LockStmt
		| ProcedureStmt
		| RemoveAggrStmt
		| RemoveOperStmt
		| RemoveFuncStmt
		| RemoveStmt
		| RenameStmt
		| RevokeStmt
		| OptimizableStmt
		| RuleStmt
		| TransactionStmt
		| ViewStmt
		| LoadStmt
		| CreatedbStmt
		| DestroydbStmt
		| VacuumStmt
		| VariableSetStmt
		| VariableShowStmt
		| VariableResetStmt
		| ConstraintsSetStmt
		;

/*****************************************************************************
 *
 * Create a new Postgres DBMS user
 *
 *
 *****************************************************************************/

CreateUserStmt:  CREATE USER UserId user_passwd_clause user_createdb_clause
			user_createuser_clause user_group_clause user_valid_clause
				{
					CreateUserStmt *n = makeNode(CreateUserStmt);
					n->user = $3;
					n->password = $4;
					n->createdb = $5;
					n->createuser = $6;
					n->groupElts = $7;
					n->validUntil = $8;
					$$ = (Node *)n;
				}
		;

/*****************************************************************************
 *
 * Alter a postresql DBMS user
 *
 *
 *****************************************************************************/

AlterUserStmt:  ALTER USER UserId user_passwd_clause user_createdb_clause
			user_createuser_clause user_group_clause user_valid_clause
				{
					AlterUserStmt *n = makeNode(AlterUserStmt);
					n->user = $3;
					n->password = $4;
					n->createdb = $5;
					n->createuser = $6;
					n->groupElts = $7;
					n->validUntil = $8;
					$$ = (Node *)n;
				}
		;

/*****************************************************************************
 *
 * Drop a postresql DBMS user
 *
 *
 *****************************************************************************/

DropUserStmt:  DROP USER UserId
				{
					DropUserStmt *n = makeNode(DropUserStmt);
					n->user = $3;
					$$ = (Node *)n;
				}
		;

user_passwd_clause:  WITH PASSWORD UserId		{ $$ = $3; }
			| /*EMPTY*/							{ $$ = NULL; }
		;

user_createdb_clause:  CREATEDB
				{
					bool*  b;
					$$ = (b = (bool*)palloc(sizeof(bool)));
					*b = true;
				}
			| NOCREATEDB
				{
					bool*  b;
					$$ = (b = (bool*)palloc(sizeof(bool)));
					*b = false;
				}
			| /*EMPTY*/							{ $$ = NULL; }
		;

user_createuser_clause:  CREATEUSER
				{
					bool*  b;
					$$ = (b = (bool*)palloc(sizeof(bool)));
					*b = true;
				}
			| NOCREATEUSER
				{
					bool*  b;
					$$ = (b = (bool*)palloc(sizeof(bool)));
					*b = false;
				}
			| /*EMPTY*/							{ $$ = NULL; }
		;

user_group_list:  user_group_list ',' UserId
				{
					$$ = lcons((void*)makeString($3), $1);
				}
			| UserId
				{
					$$ = lcons((void*)makeString($1), NIL);
				}
		;

user_group_clause:  IN GROUP user_group_list	{ $$ = $3; }
			| /*EMPTY*/							{ $$ = NULL; }
		;

user_valid_clause:  VALID UNTIL SCONST			{ $$ = $3; }
			| /*EMPTY*/							{ $$ = NULL; }
		;

/*****************************************************************************
 *
 * Set PG internal variable
 *	  SET name TO 'var_value'
 * Include SQL92 syntax (thomas 1997-10-22):
 *    SET TIME ZONE 'var_value'
 *
 *****************************************************************************/

VariableSetStmt:  SET ColId TO var_value
				{
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name  = $2;
					n->value = $4;
					$$ = (Node *) n;
				}
		| SET ColId '=' var_value
				{
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name  = $2;
					n->value = $4;
					$$ = (Node *) n;
				}
		| SET TIME ZONE zone_value
				{
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name  = "timezone";
					n->value = $4;
					$$ = (Node *) n;
				}
		| SET TRANSACTION ISOLATION LEVEL opt_level
				{
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name  = "XactIsoLevel";
					n->value = $5;
					$$ = (Node *) n;
				}
		| SET NAMES encoding
				{
#ifdef MULTIBYTE
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name  = "client_encoding";
					n->value = $3;
					$$ = (Node *) n;
#else
					elog(ERROR, "SET NAMES is not supported");
#endif
				}
		;

opt_level:  READ COMMITTED					{ $$ = "committed"; }
		| SERIALIZABLE						{ $$ = "serializable"; }
		;

var_value:  Sconst			{ $$ = $1; }
		| DEFAULT			{ $$ = NULL; }
		;

zone_value:  Sconst			{ $$ = $1; }
		| DEFAULT			{ $$ = NULL; }
		| LOCAL				{ $$ = NULL; }
		;

VariableShowStmt:  SHOW ColId
				{
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name  = $2;
					$$ = (Node *) n;
				}
		| SHOW TIME ZONE
				{
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name  = "timezone";
					$$ = (Node *) n;
				}
		| SHOW TRANSACTION ISOLATION LEVEL
				{
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name  = "XactIsoLevel";
					$$ = (Node *) n;
				}
		;

VariableResetStmt:	RESET ColId
				{
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name  = $2;
					$$ = (Node *) n;
				}
		| RESET TIME ZONE
				{
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name  = "timezone";
					$$ = (Node *) n;
				}
		| RESET TRANSACTION ISOLATION LEVEL
				{
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name  = "XactIsoLevel";
					$$ = (Node *) n;
				}
		;


ConstraintsSetStmt:	SET CONSTRAINTS constraints_set_list constraints_set_mode
				{
					ConstraintsSetStmt *n = makeNode(ConstraintsSetStmt);
					n->constraints = $3;
					n->deferred    = $4;
					$$ = (Node *) n;
				}
		;


constraints_set_list:	ALL
				{
					$$ = NIL;
				}
		| constraints_set_namelist
				{
					$$ = $1;
				}
		;


constraints_set_namelist:	IDENT
				{
					$$ = lappend(NIL, $1);
				}
		| constraints_set_namelist ',' IDENT
				{
					$$ = lappend($1, $3);
				}
		;


constraints_set_mode:	DEFERRED
				{
					$$ = true;
				}
		| IMMEDIATE
				{
					$$ = false;
				}
		;


/*****************************************************************************
 *
 *		QUERY :
 *				addattr ( attr1 = type1 .. attrn = typen ) to <relname> [*]
 *
 *****************************************************************************/

AddAttrStmt:  ALTER TABLE relation_name opt_inh_star alter_clause
				{
					AddAttrStmt *n = makeNode(AddAttrStmt);
					n->relname = $3;
					n->inh = $4;
					n->colDef = $5;
					$$ = (Node *)n;
				}
		;

alter_clause:  ADD opt_column columnDef
				{
					$$ = $3;
				}
			| ADD '(' OptTableElementList ')'
				{
					Node *lp = lfirst($3);

					if (length($3) != 1)
						elog(ERROR,"ALTER TABLE/ADD() allows one column only");
					$$ = lp;
				}
			| DROP opt_column ColId
				{	elog(ERROR,"ALTER TABLE/DROP COLUMN not yet implemented"); }
			| ALTER opt_column ColId SET DEFAULT default_expr
				{	elog(ERROR,"ALTER TABLE/ALTER COLUMN/SET DEFAULT not yet implemented"); }
			| ALTER opt_column ColId DROP DEFAULT
				{	elog(ERROR,"ALTER TABLE/ALTER COLUMN/DROP DEFAULT not yet implemented"); }
			| ADD ConstraintElem
				{	elog(ERROR,"ALTER TABLE/ADD CONSTRAINT not yet implemented"); }
		;


/*****************************************************************************
 *
 *		QUERY :
 *				close <optname>
 *
 *****************************************************************************/

ClosePortalStmt:  CLOSE opt_id
				{
					ClosePortalStmt *n = makeNode(ClosePortalStmt);
					n->portalname = $2;
					$$ = (Node *)n;
				}
		;

opt_id:  ColId									{ $$ = $1; }
		| /*EMPTY*/								{ $$ = NULL; }
		;


/*****************************************************************************
 *
 *		QUERY :
 *				COPY [BINARY] <relname> FROM/TO
 *				[USING DELIMITERS <delimiter>]
 *
 *****************************************************************************/

CopyStmt:  COPY opt_binary relation_name opt_with_copy copy_dirn copy_file_name copy_delimiter
				{
					CopyStmt *n = makeNode(CopyStmt);
					n->binary = $2;
					n->relname = $3;
					n->oids = $4;
					n->direction = $5;
					n->filename = $6;
					n->delimiter = $7;
					$$ = (Node *)n;
				}
		;

copy_dirn:	TO
				{ $$ = TO; }
		| FROM
				{ $$ = FROM; }
		;

/*
 * copy_file_name NULL indicates stdio is used. Whether stdin or stdout is
 * used depends on the direction. (It really doesn't make sense to copy from
 * stdout. We silently correct the "typo".		 - AY 9/94
 */
copy_file_name:  Sconst							{ $$ = $1; }
		| STDIN									{ $$ = NULL; }
		| STDOUT								{ $$ = NULL; }
		;

opt_binary:  BINARY								{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

opt_with_copy:	WITH OIDS						{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

/*
 * the default copy delimiter is tab but the user can configure it
 */
copy_delimiter:  USING DELIMITERS Sconst		{ $$ = $3; }
		| /*EMPTY*/								{ $$ = "\t"; }
		;


/*****************************************************************************
 *
 *		QUERY :
 *				CREATE relname
 *
 *****************************************************************************/

CreateStmt:  CREATE OptTemp TABLE relation_name '(' OptTableElementList ')'
				OptInherit
				{
					CreateStmt *n = makeNode(CreateStmt);
					n->istemp = $2;
					n->relname = $4;
					n->tableElts = $6;
					n->inhRelnames = $8;
					n->constraints = NIL;
					$$ = (Node *)n;
				}
		;

OptTemp:  OptTempType						{ $$ = $1; }
			| OptTempScope OptTempType		{ $$ = $2; }
		;

OptTempType:  TEMP							{ $$ = TRUE; }
			| TEMPORARY						{ $$ = TRUE; }
			| /*EMPTY*/						{ $$ = FALSE; }
		;

OptTempScope:  GLOBAL
				{
					elog(ERROR, "GLOBAL TEMPORARY TABLE is not currently supported");
					$$ = TRUE;
				}
			| LOCAL
				{
					 $$ = FALSE;
				}
		;

OptTableElementList:  OptTableElementList ',' OptTableElement
				{
					if ($3 != NULL)
						$$ = lappend($1, $3);
					else
						$$ = $1;
				}
			| OptTableElement
				{
					if ($1 != NULL)
						$$ = lcons($1, NIL);
					else
						$$ = NULL;
				}
			| /*EMPTY*/							{ $$ = NULL; }
		;

OptTableElement:  columnDef						{ $$ = $1; }
			| TableConstraint					{ $$ = $1; }
		;

columnDef:  ColId Typename ColQualifier
				{
					ColumnDef *n = makeNode(ColumnDef);
					n->colname = $1;
					n->typename = $2;
					n->defval = NULL;
					n->is_not_null = FALSE;
					n->constraints = $3;
					$$ = (Node *)n;
				}
			| ColId SERIAL ColPrimaryKey
				{
					ColumnDef *n = makeNode(ColumnDef);
					n->colname = $1;
					n->typename = makeNode(TypeName);
					n->typename->name = xlateSqlType("integer");
					n->defval = NULL;
					n->is_not_null = TRUE;
					n->is_sequence = TRUE;
					n->constraints = $3;

					$$ = (Node *)n;
				}
		;

ColQualifier:  ColQualList						{ $$ = $1; }
			| /*EMPTY*/							{ $$ = NULL; }
		;

ColQualList:  ColQualList ColConstraint
				{
					if ($2 != NULL)
						$$ = lappend($1, $2);
					else
						$$ = $1;
				}
			| ColConstraint
				{
					if ($1 != NULL)
						$$ = lcons($1, NIL);
					else
						$$ = NULL;
				}
		;

ColPrimaryKey:  PRIMARY KEY
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_PRIMARY;
					n->name = NULL;
					n->def = NULL;
					n->keys = NULL;
					$$ = lcons((Node *)n, NIL);
				}
			| /*EMPTY*/							{ $$ = NULL; }
		;

ColConstraint:
		CONSTRAINT name ColConstraintElem
				{
						Constraint *n = (Constraint *)$3;
						if (n != NULL) n->name = fmtId($2);
						$$ = $3;
				}
		| ColConstraintElem
				{ $$ = $1; }
		;

/* DEFAULT NULL is already the default for Postgres.
 * Bue define it here and carry it forward into the system
 * to make it explicit.
 * - thomas 1998-09-13
 * WITH NULL and NULL are not SQL92-standard syntax elements,
 * so leave them out. Use DEFAULT NULL to explicitly indicate
 * that a column may have that value. WITH NULL leads to
 * shift/reduce conflicts with WITH TIME ZONE anyway.
 * - thomas 1999-01-08
 */
ColConstraintElem:  CHECK '(' constraint_expr ')'
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_CHECK;
					n->name = NULL;
					n->def = FlattenStringList($3);
					n->keys = NULL;
					$$ = (Node *)n;
				}
			| DEFAULT NULL_P
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_DEFAULT;
					n->name = NULL;
					n->def = NULL;
					n->keys = NULL;
					$$ = (Node *)n;
				}
			| DEFAULT default_expr
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_DEFAULT;
					n->name = NULL;
					n->def = FlattenStringList($2);
					n->keys = NULL;
					$$ = (Node *)n;
				}
			| NOT NULL_P
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_NOTNULL;
					n->name = NULL;
					n->def = NULL;
					n->keys = NULL;
					$$ = (Node *)n;
				}
			| UNIQUE
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_UNIQUE;
					n->name = NULL;
					n->def = NULL;
					n->keys = NULL;
					$$ = (Node *)n;
				}
			| PRIMARY KEY
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_PRIMARY;
					n->name = NULL;
					n->def = NULL;
					n->keys = NULL;
					$$ = (Node *)n;
				}
			| REFERENCES ColId opt_column_list key_match key_actions
				{
					elog(NOTICE,"CREATE TABLE/FOREIGN KEY clause ignored; not yet implemented");
					$$ = NULL;
				}
		;

default_list:  default_list ',' default_expr
				{
					$$ = lappend($1,makeString(","));
					$$ = nconc($$, $3);
				}
			| default_expr
				{
					$$ = $1;
				}
		;

/* The Postgres default column value is NULL.
 * Rather than carrying DEFAULT NULL forward as a clause,
 * let's just have it be a no-op.
			| NULL_P
				{	$$ = lcons( makeString("NULL"), NIL); }
 * - thomas 1998-09-13
 */
default_expr:  AexprConst
				{	$$ = makeConstantList((A_Const *) $1); }
			| '-' default_expr %prec UMINUS
				{	$$ = lcons( makeString( "-"), $2); }
			| default_expr '+' default_expr
				{	$$ = nconc( $1, lcons( makeString( "+"), $3)); }
			| default_expr '-' default_expr
				{	$$ = nconc( $1, lcons( makeString( "-"), $3)); }
			| default_expr '/' default_expr
				{	$$ = nconc( $1, lcons( makeString( "/"), $3)); }
			| default_expr '%' default_expr
				{	$$ = nconc( $1, lcons( makeString( "%"), $3)); }
			| default_expr '*' default_expr
				{	$$ = nconc( $1, lcons( makeString( "*"), $3)); }
			| default_expr '^' default_expr
				{	$$ = nconc( $1, lcons( makeString( "^"), $3)); }
			| default_expr '|' default_expr
				{	$$ = nconc( $1, lcons( makeString( "|"), $3)); }
			| default_expr '=' default_expr
				{	elog(ERROR,"boolean expressions not supported in DEFAULT"); }
			| default_expr '<' default_expr
				{	elog(ERROR,"boolean expressions not supported in DEFAULT"); }
			| default_expr '>' default_expr
				{	elog(ERROR,"boolean expressions not supported in DEFAULT"); }
			| ':' default_expr
				{	$$ = lcons( makeString( ":"), $2); }
			| ';' default_expr
				{	$$ = lcons( makeString( ";"), $2); }
			| '|' default_expr
				{	$$ = lcons( makeString( "|"), $2); }
			| default_expr TYPECAST Typename
				{
					$3->name = fmtId($3->name);
					$$ = nconc( lcons( makeString( "CAST"), $1), makeList( makeString("AS"), $3, -1));
				}
			| CAST '(' default_expr AS Typename ')'
				{
					$5->name = fmtId($5->name);
					$$ = nconc( lcons( makeString( "CAST"), $3), makeList( makeString("AS"), $5, -1));
				}
			| '(' default_expr ')'
				{	$$ = lappend( lcons( makeString( "("), $2), makeString( ")")); }
			| func_name '(' ')'
				{
					$$ = makeList( makeString($1), makeString("("), -1);
					$$ = lappend( $$, makeString(")"));
				}
			| func_name '(' default_list ')'
				{
					$$ = makeList( makeString($1), makeString("("), -1);
					$$ = nconc( $$, $3);
					$$ = lappend( $$, makeString(")"));
				}
			| default_expr Op default_expr
				{
					if (!strcmp("<=", $2) || !strcmp(">=", $2))
						elog(ERROR,"boolean expressions not supported in DEFAULT");
					$$ = nconc( $1, lcons( makeString( $2), $3));
				}
			| Op default_expr
				{	$$ = lcons( makeString( $1), $2); }
			| default_expr Op
				{	$$ = lappend( $1, makeString( $2)); }
			/* XXX - thomas 1997-10-07 v6.2 function-specific code to be changed */
			| CURRENT_DATE
				{	$$ = lcons( makeString( "date( 'current'::datetime + '0 sec')"), NIL); }
			| CURRENT_TIME
				{	$$ = lcons( makeString( "'now'::time"), NIL); }
			| CURRENT_TIME '(' Iconst ')'
				{
					if ($3 != 0)
						elog(NOTICE,"CURRENT_TIME(%d) precision not implemented; zero used instead",$3);
					$$ = lcons( makeString( "'now'::time"), NIL);
				}
			| CURRENT_TIMESTAMP
				{	$$ = lcons( makeString( "now()"), NIL); }
			| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					if ($3 != 0)
						elog(NOTICE,"CURRENT_TIMESTAMP(%d) precision not implemented; zero used instead",$3);
					$$ = lcons( makeString( "now()"), NIL);
				}
			| CURRENT_USER
				{	$$ = lcons( makeString( "CURRENT_USER"), NIL); }
			| USER
				{	$$ = lcons( makeString( "USER"), NIL); }
		;

/* ConstraintElem specifies constraint syntax which is not embedded into
 *  a column definition. ColConstraintElem specifies the embedded form.
 * - thomas 1997-12-03
 */
TableConstraint:  CONSTRAINT name ConstraintElem
				{
						Constraint *n = (Constraint *)$3;
						if (n != NULL) n->name = fmtId($2);
						$$ = $3;
				}
		| ConstraintElem
				{ $$ = $1; }
		;

ConstraintElem:  CHECK '(' constraint_expr ')'
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_CHECK;
					n->name = NULL;
					n->def = FlattenStringList($3);
					$$ = (Node *)n;
				}
		| UNIQUE '(' columnList ')'
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_UNIQUE;
					n->name = NULL;
					n->def = NULL;
					n->keys = $3;
					$$ = (Node *)n;
				}
		| PRIMARY KEY '(' columnList ')'
				{
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_PRIMARY;
					n->name = NULL;
					n->def = NULL;
					n->keys = $4;
					$$ = (Node *)n;
				}
		| FOREIGN KEY '(' columnList ')' REFERENCES ColId opt_column_list key_match key_actions
				{
					elog(NOTICE,"CREATE TABLE/FOREIGN KEY clause ignored; not yet implemented");
					$$ = NULL;
				}
		;

constraint_list:  constraint_list ',' constraint_expr
				{
					$$ = lappend($1,makeString(","));
					$$ = nconc($$, $3);
				}
			| constraint_expr
				{
					$$ = $1;
				}
		;

constraint_expr:  AexprConst
				{	$$ = makeConstantList((A_Const *) $1); }
			| NULL_P
				{	$$ = lcons( makeString("NULL"), NIL); }
			| ColId
				{
					$$ = lcons( makeString(fmtId($1)), NIL);
				}
			| '-' constraint_expr %prec UMINUS
				{	$$ = lcons( makeString( "-"), $2); }
			| constraint_expr '+' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "+"), $3)); }
			| constraint_expr '-' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "-"), $3)); }
			| constraint_expr '/' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "/"), $3)); }
			| constraint_expr '%' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "%"), $3)); }
			| constraint_expr '*' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "*"), $3)); }
			| constraint_expr '^' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "^"), $3)); }
			| constraint_expr '|' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "|"), $3)); }
			| constraint_expr '=' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "="), $3)); }
			| constraint_expr '<' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "<"), $3)); }
			| constraint_expr '>' constraint_expr
				{	$$ = nconc( $1, lcons( makeString( ">"), $3)); }
			| ':' constraint_expr
				{	$$ = lcons( makeString( ":"), $2); }
			| ';' constraint_expr
				{	$$ = lcons( makeString( ";"), $2); }
			| '|' constraint_expr
				{	$$ = lcons( makeString( "|"), $2); }
			| constraint_expr TYPECAST Typename
				{
					$3->name = fmtId($3->name);
					$$ = nconc( lcons( makeString( "CAST"), $1), makeList( makeString("AS"), $3, -1));
				}
			| CAST '(' constraint_expr AS Typename ')'
				{
					$5->name = fmtId($5->name);
					$$ = nconc( lcons( makeString( "CAST"), $3), makeList( makeString("AS"), $5, -1));
				}
			| '(' constraint_expr ')'
				{	$$ = lappend( lcons( makeString( "("), $2), makeString( ")")); }
			| func_name '(' ')'
				{
					$$ = makeList( makeString($1), makeString("("), -1);
					$$ = lappend( $$, makeString(")"));
				}
			| func_name '(' constraint_list ')'
				{
					$$ = makeList( makeString($1), makeString("("), -1);
					$$ = nconc( $$, $3);
					$$ = lappend( $$, makeString(")"));
				}
			| constraint_expr Op constraint_expr
				{	$$ = nconc( $1, lcons( makeString( $2), $3)); }
			| constraint_expr LIKE constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "LIKE"), $3)); }
			| constraint_expr NOT LIKE constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "NOT LIKE"), $4)); }
			| constraint_expr AND constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "AND"), $3)); }
			| constraint_expr OR constraint_expr
				{	$$ = nconc( $1, lcons( makeString( "OR"), $3)); }
			| NOT constraint_expr
				{	$$ = lcons( makeString( "NOT"), $2); }
			| Op constraint_expr
				{	$$ = lcons( makeString( $1), $2); }
			| constraint_expr Op
				{	$$ = lappend( $1, makeString( $2)); }
			| constraint_expr ISNULL
				{	$$ = lappend( $1, makeString( "IS NULL")); }
			| constraint_expr IS NULL_P
				{	$$ = lappend( $1, makeString( "IS NULL")); }
			| constraint_expr NOTNULL
				{	$$ = lappend( $1, makeString( "IS NOT NULL")); }
			| constraint_expr IS NOT NULL_P
				{	$$ = lappend( $1, makeString( "IS NOT NULL")); }
			| constraint_expr IS TRUE_P
				{	$$ = lappend( $1, makeString( "IS TRUE")); }
			| constraint_expr IS FALSE_P
				{	$$ = lappend( $1, makeString( "IS FALSE")); }
			| constraint_expr IS NOT TRUE_P
				{	$$ = lappend( $1, makeString( "IS NOT TRUE")); }
			| constraint_expr IS NOT FALSE_P
				{	$$ = lappend( $1, makeString( "IS NOT FALSE")); }
			| constraint_expr IN '(' c_list ')'
				{
					$$ = lappend( $1, makeString("IN"));
					$$ = lappend( $$, makeString("("));
					$$ = nconc( $$, $4);
					$$ = lappend( $$, makeString(")"));
				}
			| constraint_expr NOT IN '(' c_list ')'
				{
					$$ = lappend( $1, makeString("NOT IN"));
					$$ = lappend( $$, makeString("("));
					$$ = nconc( $$, $5);
					$$ = lappend( $$, makeString(")"));
				}
			| constraint_expr BETWEEN c_expr AND c_expr
				{
					$$ = lappend( $1, makeString("BETWEEN"));
					$$ = nconc( $$, $3);
					$$ = lappend( $$, makeString("AND"));
					$$ = nconc( $$, $5);
				}
			| constraint_expr NOT BETWEEN c_expr AND c_expr
				{
					$$ = lappend( $1, makeString("NOT BETWEEN"));
					$$ = nconc( $$, $4);
					$$ = lappend( $$, makeString("AND"));
					$$ = nconc( $$, $6);
				}
		;

c_list:  c_list ',' c_expr
				{
					$$ = lappend($1, makeString(","));
					$$ = nconc($$, $3);
				}
			| c_expr
				{
					$$ = $1;
				}
		;

c_expr:  AexprConst
				{	$$ = makeConstantList((A_Const *) $1); }
		;

key_match:  MATCH FULL					{ $$ = NULL; }
		| MATCH PARTIAL					{ $$ = NULL; }
		| /*EMPTY*/						{ $$ = NULL; }
		;

key_actions:  key_action key_action		{ $$ = NIL; }
		| key_action					{ $$ = NIL; }
		| /*EMPTY*/						{ $$ = NIL; }
		;

key_action:  ON DELETE key_reference	{ $$ = NIL; }
		| ON UPDATE key_reference		{ $$ = NIL; }
		;

key_reference:  NO ACTION				{ $$ = NULL; }
		| CASCADE						{ $$ = NULL; }
		| SET DEFAULT					{ $$ = NULL; }
		| SET NULL_P					{ $$ = NULL; }
		;

OptInherit:  INHERITS '(' relation_name_list ')'		{ $$ = $3; }
		| /*EMPTY*/										{ $$ = NIL; }
		;

CreateAsStmt:  CREATE OptTemp TABLE relation_name OptCreateAs AS SubSelect
				{
					SelectStmt *n = (SelectStmt *)$7;
					if ($5 != NIL)
						mapTargetColumns($5, n->targetList);
					n->istemp = $2;
					n->into = $4;
					$$ = (Node *)n;
				}
		;

OptCreateAs:  '(' CreateAsList ')'				{ $$ = $2; }
			| /*EMPTY*/							{ $$ = NULL; }
		;

CreateAsList:  CreateAsList ',' CreateAsElement	{ $$ = lappend($1, $3); }
			| CreateAsElement					{ $$ = lcons($1, NIL); }
		;

CreateAsElement:  ColId
				{
					ColumnDef *n = makeNode(ColumnDef);
					n->colname = $1;
					n->typename = NULL;
					n->defval = NULL;
					n->is_not_null = FALSE;
					n->constraints = NULL;
					$$ = (Node *)n;
				}
		;


/*****************************************************************************
 *
 *		QUERY :
 *				CREATE SEQUENCE seqname
 *
 *****************************************************************************/

CreateSeqStmt:  CREATE SEQUENCE relation_name OptSeqList
				{
					CreateSeqStmt *n = makeNode(CreateSeqStmt);
					n->seqname = $3;
					n->options = $4;
					$$ = (Node *)n;
				}
		;

OptSeqList:  OptSeqList OptSeqElem
				{ $$ = lappend($1, $2); }
			|	{ $$ = NIL; }
		;

OptSeqElem:  CACHE IntegerOnly
				{
					$$ = makeNode(DefElem);
					$$->defname = "cache";
					$$->arg = (Node *)$2;
				}
			| CYCLE
				{
					$$ = makeNode(DefElem);
					$$->defname = "cycle";
					$$->arg = (Node *)NULL;
				}
			| INCREMENT IntegerOnly
				{
					$$ = makeNode(DefElem);
					$$->defname = "increment";
					$$->arg = (Node *)$2;
				}
			| MAXVALUE IntegerOnly
				{
					$$ = makeNode(DefElem);
					$$->defname = "maxvalue";
					$$->arg = (Node *)$2;
				}
			| MINVALUE IntegerOnly
				{
					$$ = makeNode(DefElem);
					$$->defname = "minvalue";
					$$->arg = (Node *)$2;
				}
			| START IntegerOnly
				{
					$$ = makeNode(DefElem);
					$$->defname = "start";
					$$->arg = (Node *)$2;
				}
		;

NumericOnly:  FloatOnly					{ $$ = $1; }
			| IntegerOnly				{ $$ = $1; }

FloatOnly:  FCONST
				{
					$$ = makeFloat($1);
				}
			| '-' FCONST
				{
					$$ = makeFloat($2);
					$$->val.dval = - $$->val.dval;
				}
		;

IntegerOnly:  Iconst
				{
					$$ = makeInteger($1);
				}
			| '-' Iconst
				{
					$$ = makeInteger($2);
					$$->val.ival = - $$->val.ival;
				}
		;

/*****************************************************************************
 *
 *		QUERIES :
 *				CREATE PROCEDURAL LANGUAGE ...
 *				DROP PROCEDURAL LANGUAGE ...
 *
 *****************************************************************************/

CreatePLangStmt:  CREATE PLangTrusted PROCEDURAL LANGUAGE Sconst 
			HANDLER def_name LANCOMPILER Sconst
			{
				CreatePLangStmt *n = makeNode(CreatePLangStmt);
				n->plname = $5;
				n->plhandler = $7;
				n->plcompiler = $9;
				n->pltrusted = $2;
				$$ = (Node *)n;
			}
		;

PLangTrusted:		TRUSTED { $$ = TRUE; }
			|	{ $$ = FALSE; }

DropPLangStmt:  DROP PROCEDURAL LANGUAGE Sconst
			{
				DropPLangStmt *n = makeNode(DropPLangStmt);
				n->plname = $4;
				$$ = (Node *)n;
			}
		;

/*****************************************************************************
 *
 *		QUERIES :
 *				CREATE TRIGGER ...
 *				DROP TRIGGER ...
 *
 *****************************************************************************/

CreateTrigStmt:  CREATE TRIGGER name TriggerActionTime TriggerEvents ON
				relation_name TriggerForSpec EXECUTE PROCEDURE
				name '(' TriggerFuncArgs ')'
				{
					CreateTrigStmt *n = makeNode(CreateTrigStmt);
					n->trigname = $3;
					n->relname = $7;
					n->funcname = $11;
					n->args = $13;
					n->before = $4;
					n->row = $8;
					memcpy (n->actions, $5, 4);
					n->lang = NULL;		/* unused */
					n->text = NULL;		/* unused */
					n->attr = NULL;		/* unused */
					n->when = NULL;		/* unused */

					n->isconstraint  = false;
					n->deferrable    = false;
					n->initdeferred  = false;
					n->constrrelname = NULL;
					$$ = (Node *)n;
				}
		| CREATE CONSTRAINT TRIGGER name AFTER TriggerOneEvent ON
				relation_name OptConstrFromTable 
				OptConstrTrigDeferrable OptConstrTrigInitdeferred
				FOR EACH ROW EXECUTE PROCEDURE name '(' TriggerFuncArgs ')'
				{
					CreateTrigStmt *n = makeNode(CreateTrigStmt);
					n->trigname = $4;
					n->relname = $8;
					n->funcname = $17;
					n->args = $19;
					n->before = false;
					n->row = true;
					n->actions[0] = $6;
					n->actions[1] = '\0';
					n->lang = NULL;		/* unused */
					n->text = NULL;		/* unused */
					n->attr = NULL;		/* unused */
					n->when = NULL;		/* unused */

					/*
					 * Check that the DEFERRABLE and INITIALLY combination
					 * makes sense
					 */
					n->isconstraint  = true;
					if ($11 == 1)
					{
						if ($10 == 0)
							elog(ERROR, "INITIALLY DEFERRED constraint "
										"cannot be NOT DEFERRABLE");
						n->deferrable = true;
						n->initdeferred = true;
					} else {
						n->deferrable = ($10 == 1);
						n->initdeferred = false;
					}

					n->constrrelname = $9;
					$$ = (Node *)n;
				}
		;

TriggerActionTime:  BEFORE						{ $$ = TRUE; }
			| AFTER								{ $$ = FALSE; }
		;

TriggerEvents:	TriggerOneEvent
				{
					char *e = palloc (4);
					e[0] = $1; e[1] = 0; $$ = e;
				}
			| TriggerOneEvent OR TriggerOneEvent
				{
					char *e = palloc (4);
					e[0] = $1; e[1] = $3; e[2] = 0; $$ = e;
				}
			| TriggerOneEvent OR TriggerOneEvent OR TriggerOneEvent
				{
					char *e = palloc (4);
					e[0] = $1; e[1] = $3; e[2] = $5; e[3] = 0;
					$$ = e;
				}
		;

TriggerOneEvent:  INSERT					{ $$ = 'i'; }
			| DELETE						{ $$ = 'd'; }
			| UPDATE						{ $$ = 'u'; }
		;

TriggerForSpec:  FOR TriggerForOpt TriggerForType
				{
					$$ = $3;
				}
		;

TriggerForOpt:  EACH						{ $$ = TRUE; }
			| /*EMPTY*/						{ $$ = FALSE; }
		;

TriggerForType:  ROW						{ $$ = TRUE; }
			| STATEMENT						{ $$ = FALSE; }
		;

TriggerFuncArgs:  TriggerFuncArg
				{ $$ = lcons($1, NIL); }
			| TriggerFuncArgs ',' TriggerFuncArg
				{ $$ = lappend($1, $3); }
			| /*EMPTY*/
				{ $$ = NIL; }
		;

TriggerFuncArg:  ICONST
				{
					char *s = (char *) palloc (256);
					sprintf (s, "%d", $1);
					$$ = s;
				}
			| FCONST
				{
					char *s = (char *) palloc (256);
					sprintf (s, "%g", $1);
					$$ = s;
				}
			| Sconst						{  $$ = $1; }
			| IDENT							{  $$ = $1; }
		;

OptConstrFromTable:			/* Empty */
				{
					$$ = "";
				}
		| FROM relation_name
				{
					$$ = $2;
				}
		;

OptConstrTrigDeferrable:	/* Empty */
				{
					$$ = -1;
				}
		| DEFERRABLE
				{
					$$ = 1;
				}
		| NOT DEFERRABLE
				{
					$$ = 0;
				}
		;

OptConstrTrigInitdeferred:	/* Empty */
				{
					$$ = -1;
				}
		| INITIALLY DEFERRED
				{
					$$ = 1;
				}
		| INITIALLY IMMEDIATE
				{
					$$ = 0;
				}
		;

DropTrigStmt:  DROP TRIGGER name ON relation_name
				{
					DropTrigStmt *n = makeNode(DropTrigStmt);
					n->trigname = $3;
					n->relname = $5;
					$$ = (Node *) n;
				}
		;


/*****************************************************************************
 *
 *		QUERY :
 *				define (type,operator,aggregate)
 *
 *****************************************************************************/

DefineStmt:  CREATE def_type def_rest
				{
					$3->defType = $2;
					$$ = (Node *)$3;
				}
		;

def_rest:  def_name definition
				{
					$$ = makeNode(DefineStmt);
					$$->defname = $1;
					$$->definition = $2;
				}
		;

def_type:  OPERATOR							{ $$ = OPERATOR; }
		| TYPE_P							{ $$ = TYPE_P; }
		| AGGREGATE							{ $$ = AGGREGATE; }
		;

def_name:  PROCEDURE						{ $$ = "procedure"; }
		| JOIN								{ $$ = "join"; }
		| ColId								{ $$ = $1; }
		| MathOp							{ $$ = $1; }
		| Op								{ $$ = $1; }
		;

definition:  '(' def_list ')'				{ $$ = $2; }
		;

def_list:  def_elem							{ $$ = lcons($1, NIL); }
		| def_list ',' def_elem				{ $$ = lappend($1, $3); }
		;

def_elem:  def_name '=' def_arg
				{
					$$ = makeNode(DefElem);
					$$->defname = $1;
					$$->arg = (Node *)$3;
				}
		| def_name
				{
					$$ = makeNode(DefElem);
					$$->defname = $1;
					$$->arg = (Node *)NULL;
				}
		| DEFAULT '=' def_arg
				{
					$$ = makeNode(DefElem);
					$$->defname = "default";
					$$->arg = (Node *)$3;
				}
		;

def_arg:  ColId							{  $$ = (Node *)makeString($1); }
		| all_Op						{  $$ = (Node *)makeString($1); }
		| NumericOnly					{  $$ = (Node *)$1; }
		| Sconst						{  $$ = (Node *)makeString($1); }
		| SETOF ColId
				{
					TypeName *n = makeNode(TypeName);
					n->name = $2;
					n->setof = TRUE;
					n->arrayBounds = NULL;
					n->typmod = -1;
					$$ = (Node *)n;
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				destroy <relname1> [, <relname2> .. <relnameN> ]
 *
 *****************************************************************************/

DestroyStmt:  DROP TABLE relation_name_list
				{
					DestroyStmt *n = makeNode(DestroyStmt);
					n->relNames = $3;
					n->sequence = FALSE;
					$$ = (Node *)n;
				}
		| DROP SEQUENCE relation_name_list
				{
					DestroyStmt *n = makeNode(DestroyStmt);
					n->relNames = $3;
					n->sequence = TRUE;
					$$ = (Node *)n;
				}
		;

/*****************************************************************************
 *
 *		QUERY:
 *				truncate table relname
 *
 *****************************************************************************/

TruncateStmt:  TRUNCATE TABLE relation_name
				{
					TruncateStmt *n = makeNode(TruncateStmt);
					n->relName = $3;
					$$ = (Node *)n;
				}
			;

/*****************************************************************************
 *
 *		QUERY:
 *			fetch/move [forward | backward] [ # | all ] [ in <portalname> ]
 *			fetch [ forward | backward | absolute | relative ]
 *			      [ # | all | next | prior ] [ [ in | from ] <portalname> ]
 *
 *****************************************************************************/

FetchStmt:	FETCH opt_direction fetch_how_many opt_portal_name
				{
					FetchStmt *n = makeNode(FetchStmt);
					if ($2 == RELATIVE)
					{
						if ($3 == 0)
							elog(ERROR,"FETCH/RELATIVE at current position is not supported");
						$2 = FORWARD;
					}
					if ($3 < 0)
					{
						$3 = -$3;
						$2 = (($2 == FORWARD)? BACKWARD: FORWARD);
					}
					n->direction = $2;
					n->howMany = $3;
					n->portalname = $4;
					n->ismove = false;
					$$ = (Node *)n;
				}
		|	MOVE opt_direction fetch_how_many opt_portal_name
				{
					FetchStmt *n = makeNode(FetchStmt);
					if ($3 < 0)
					{
						$3 = -$3;
						$2 = (($2 == FORWARD)? BACKWARD: FORWARD);
					}
					n->direction = $2;
					n->howMany = $3;
					n->portalname = $4;
					n->ismove = TRUE;
					$$ = (Node *)n;
				}
		;

opt_direction:	FORWARD					{ $$ = FORWARD; }
		| BACKWARD						{ $$ = BACKWARD; }
		| RELATIVE						{ $$ = RELATIVE; }
		| ABSOLUTE
			{
				elog(NOTICE,"FETCH/ABSOLUTE not supported, using RELATIVE");
				$$ = RELATIVE;
			}
		| /*EMPTY*/						{ $$ = FORWARD; /* default */ }
		;

fetch_how_many:  Iconst					{ $$ = $1; }
		| '-' Iconst					{ $$ = - $2; }
		| ALL							{ $$ = 0; /* 0 means fetch all tuples*/ }
		| NEXT							{ $$ = 1; }
		| PRIOR							{ $$ = -1; }
		| /*EMPTY*/						{ $$ = 1; /*default*/ }
		;

opt_portal_name:  IN name				{ $$ = $2; }
		| FROM name						{ $$ = $2; }
		| /*EMPTY*/						{ $$ = NULL; }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				GRANT [privileges] ON [relation_name_list] TO [GROUP] grantee
 *
 *****************************************************************************/

GrantStmt:  GRANT privileges ON relation_name_list TO grantee opt_with_grant
				{
					$$ = (Node*)makeAclStmt($2,$4,$6,'+');
				}
		;

privileges:  ALL PRIVILEGES
				{
				 $$ = aclmakepriv("rwaR",0);
				}
		| ALL
				{
				 $$ = aclmakepriv("rwaR",0);
				}
		| operation_commalist
				{
				 $$ = $1;
				}
		;

operation_commalist:  operation
				{
						$$ = aclmakepriv("",$1);
				}
		| operation_commalist ',' operation
				{
						$$ = aclmakepriv($1,$3);
				}
		;

operation:  SELECT
				{
						$$ = ACL_MODE_RD_CHR;
				}
		| INSERT
				{
						$$ = ACL_MODE_AP_CHR;
				}
		| UPDATE
				{
						$$ = ACL_MODE_WR_CHR;
				}
		| DELETE
				{
						$$ = ACL_MODE_WR_CHR;
				}
		| RULE
				{
						$$ = ACL_MODE_RU_CHR;
				}
		;

grantee:  PUBLIC
				{
						$$ = aclmakeuser("A","");
				}
		| GROUP ColId
				{
						$$ = aclmakeuser("G",$2);
				}
		| ColId
				{
						$$ = aclmakeuser("U",$1);
				}
		;

opt_with_grant:  WITH GRANT OPTION
				{
					yyerror("WITH GRANT OPTION is not supported.  Only relation owners can set privileges");
				 }
		| /*EMPTY*/
		;


/*****************************************************************************
 *
 *		QUERY:
 *				REVOKE [privileges] ON [relation_name] FROM [user]
 *
 *****************************************************************************/

RevokeStmt:  REVOKE privileges ON relation_name_list FROM grantee
				{
					$$ = (Node*)makeAclStmt($2,$4,$6,'-');
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				create index <indexname> on <relname>
 *				  using <access> "(" (<col> with <op>)+ ")" [with
 *				  <target_list>]
 *
 *	[where <qual>] is not supported anymore
 *****************************************************************************/

IndexStmt:	CREATE index_opt_unique INDEX index_name ON relation_name
			access_method_clause '(' index_params ')' opt_with
				{
					/* should check that access_method is valid,
					   etc ... but doesn't */
					IndexStmt *n = makeNode(IndexStmt);
					n->unique = $2;
					n->idxname = $4;
					n->relname = $6;
					n->accessMethod = $7;
					n->indexParams = $9;
					n->withClause = $11;
					n->whereClause = NULL;
					$$ = (Node *)n;
				}
		;

index_opt_unique:  UNIQUE						{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

access_method_clause:  USING access_method		{ $$ = $2; }
		| /*EMPTY*/								{ $$ = "btree"; }
		;

index_params:  index_list						{ $$ = $1; }
		| func_index							{ $$ = lcons($1,NIL); }
		;

index_list:  index_list ',' index_elem			{ $$ = lappend($1, $3); }
		| index_elem							{ $$ = lcons($1, NIL); }
		;

func_index:  func_name '(' name_list ')' opt_type opt_class
				{
					$$ = makeNode(IndexElem);
					$$->name = $1;
					$$->args = $3;
					$$->class = $6;
					$$->typename = $5;
				}
		  ;

index_elem:  attr_name opt_type opt_class
				{
					$$ = makeNode(IndexElem);
					$$->name = $1;
					$$->args = NIL;
					$$->class = $3;
					$$->typename = $2;
				}
		;

opt_type:  ':' Typename							{ $$ = $2; }
		| FOR Typename							{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

/* opt_class "WITH class" conflicts with preceeding opt_type
 *  for Typename of "TIMESTAMP WITH TIME ZONE"
 * So, remove "WITH class" from the syntax. OK??
 * - thomas 1997-10-12
 *		| WITH class							{ $$ = $2; }
 */
opt_class:  class								{ $$ = $1; }
		| USING class							{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NULL; }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				extend index <indexname> [where <qual>]
 *
 *****************************************************************************/

ExtendStmt:  EXTEND INDEX index_name where_clause
				{
					ExtendStmt *n = makeNode(ExtendStmt);
					n->idxname = $3;
					n->whereClause = $4;
					$$ = (Node *)n;
				}
		;

/*****************************************************************************
 *
 *		QUERY:
 *				execute recipe <recipeName>
 *
 *****************************************************************************/

/* NOT USED
RecipeStmt:  EXECUTE RECIPE recipe_name
				{
					RecipeStmt *n;
					if (!IsTransactionBlock())
						elog(ERROR,"EXECUTE RECIPE may only be used in begin/end transaction blocks");

					n = makeNode(RecipeStmt);
					n->recipeName = $3;
					$$ = (Node *)n;
				}
		;
*/

/*****************************************************************************
 *
 *		QUERY:
 *				define function <fname>
 *					   (language = <lang>, returntype = <typename>
 *						[, arch_pct = <percentage | pre-defined>]
 *						[, disk_pct = <percentage | pre-defined>]
 *						[, byte_pct = <percentage | pre-defined>]
 *						[, perbyte_cpu = <int | pre-defined>]
 *						[, percall_cpu = <int | pre-defined>]
 *						[, iscachable])
 *						[arg is (<type-1> { , <type-n>})]
 *						as <filename or code in language as appropriate>
 *
 *****************************************************************************/

ProcedureStmt:	CREATE FUNCTION func_name func_args
			 RETURNS func_return opt_with AS func_as LANGUAGE Sconst
				{
					ProcedureStmt *n = makeNode(ProcedureStmt);
					n->funcname = $3;
					n->defArgs = $4;
					n->returnType = $6;
					n->withClause = $7;
					n->as = $9;
					n->language = $11;
					$$ = (Node *)n;
				};

opt_with:  WITH definition						{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NIL; }
		;

func_args:  '(' func_args_list ')'				{ $$ = $2; }
		| '(' ')'								{ $$ = NIL; }
		;

func_args_list:  TypeId
				{	$$ = lcons(makeString($1),NIL); }
		| func_args_list ',' TypeId
				{	$$ = lappend($1,makeString($3)); }
		;

func_as: Sconst	
				{   $$ = lcons(makeString($1),NIL); }
		| Sconst ',' Sconst
				{ 	$$ = lappend(lcons(makeString($1),NIL), makeString($3)); }
		;

func_return:  set_opt TypeId
				{
					TypeName *n = makeNode(TypeName);
					n->name = $2;
					n->setof = $1;
					n->arrayBounds = NULL;
					$$ = (Node *)n;
				}
		;

set_opt:  SETOF									{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

/*****************************************************************************
 *
 *		QUERY:
 *
 *		remove function <funcname>
 *				(REMOVE FUNCTION "funcname" (arg1, arg2, ...))
 *		remove aggregate <aggname>
 *				(REMOVE AGGREGATE "aggname" "aggtype")
 *		remove operator <opname>
 *				(REMOVE OPERATOR "opname" (leftoperand_typ rightoperand_typ))
 *		remove type <typename>
 *				(REMOVE TYPE "typename")
 *		remove rule <rulename>
 *				(REMOVE RULE "rulename")
 *
 *****************************************************************************/

RemoveStmt:  DROP remove_type name
				{
					RemoveStmt *n = makeNode(RemoveStmt);
					n->removeType = $2;
					n->name = $3;
					$$ = (Node *)n;
				}
		;

remove_type:  TYPE_P							{  $$ = TYPE_P; }
		| INDEX									{  $$ = INDEX; }
		| RULE									{  $$ = RULE; }
		| VIEW									{  $$ = VIEW; }
		;


RemoveAggrStmt:  DROP AGGREGATE name aggr_argtype
				{
						RemoveAggrStmt *n = makeNode(RemoveAggrStmt);
						n->aggname = $3;
						n->aggtype = $4;
						$$ = (Node *)n;
				}
		;

aggr_argtype:  name								{ $$ = $1; }
		| '*'									{ $$ = NULL; }
		;


RemoveFuncStmt:  DROP FUNCTION func_name func_args
				{
					RemoveFuncStmt *n = makeNode(RemoveFuncStmt);
					n->funcname = $3;
					n->args = $4;
					$$ = (Node *)n;
				}
		;


RemoveOperStmt:  DROP OPERATOR all_Op '(' oper_argtypes ')'
				{
					RemoveOperStmt *n = makeNode(RemoveOperStmt);
					n->opname = $3;
					n->args = $5;
					$$ = (Node *)n;
				}
		;

all_Op:  Op | MathOp;

MathOp:	'+'				{ $$ = "+"; }
		| '-'			{ $$ = "-"; }
		| '*'			{ $$ = "*"; }
		| '/'			{ $$ = "/"; }
		| '%'			{ $$ = "%"; }
		| '^'			{ $$ = "^"; }
		| '|'			{ $$ = "|"; }
		| '<'			{ $$ = "<"; }
		| '>'			{ $$ = ">"; }
		| '='			{ $$ = "="; }
		;

oper_argtypes:	name
				{
				   elog(ERROR,"parser: argument type missing (use NONE for unary operators)");
				}
		| name ',' name
				{ $$ = makeList(makeString($1), makeString($3), -1); }
		| NONE ',' name			/* left unary */
				{ $$ = makeList(NULL, makeString($3), -1); }
		| name ',' NONE			/* right unary */
				{ $$ = makeList(makeString($1), NULL, -1); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				rename <attrname1> in <relname> [*] to <attrname2>
 *				rename <relname1> to <relname2>
 *
 *****************************************************************************/

RenameStmt:  ALTER TABLE relation_name opt_inh_star
				  RENAME opt_column opt_name TO name
				{
					RenameStmt *n = makeNode(RenameStmt);
					n->relname = $3;
					n->inh = $4;
					n->column = $7;
					n->newname = $9;
					$$ = (Node *)n;
				}
		;

opt_name:  name							{ $$ = $1; }
		| /*EMPTY*/						{ $$ = NULL; }
		;

opt_column:  COLUMN						{ $$ = COLUMN; }
		| /*EMPTY*/						{ $$ = 0; }
		;


/*****************************************************************************
 *
 *		QUERY:	Define Rewrite Rule , Define Tuple Rule
 *				Define Rule <old rules >
 *
 *		only rewrite rule is supported -- ay 9/94
 *
 *****************************************************************************/

RuleStmt:  CREATE RULE name AS
		   { QueryIsRule=TRUE; }
		   ON event TO event_object where_clause
		   DO opt_instead RuleActionList
				{
					RuleStmt *n = makeNode(RuleStmt);
					n->rulename = $3;
					n->event = $7;
					n->object = $9;
					n->whereClause = $10;
					n->instead = $12;
					n->actions = $13;
					$$ = (Node *)n;
				}
		;

RuleActionList:  NOTHING				{ $$ = NIL; }
		| SelectStmt					{ $$ = lcons($1, NIL); }
		| RuleActionStmt				{ $$ = lcons($1, NIL); }
		| '[' RuleActionBlock ']'		{ $$ = $2; }
		| '(' RuleActionBlock ')'		{ $$ = $2; } 
		;

RuleActionBlock:  RuleActionMulti		{  $$ = $1; }
		| RuleActionStmt				{ $$ = lcons($1, NIL); }
		;

RuleActionMulti:  RuleActionMulti RuleActionStmt
				{  $$ = lappend($1, $2); }
		| RuleActionMulti RuleActionStmt ';'
				{  $$ = lappend($1, $2); }
		| RuleActionStmt ';'
				{ $$ = lcons($1, NIL); }
		;

RuleActionStmt:	InsertStmt
		| UpdateStmt
		| DeleteStmt
		| NotifyStmt
		;

event_object:  relation_name '.' attr_name
				{
					$$ = makeNode(Attr);
					$$->relname = $1;
					$$->paramNo = NULL;
					$$->attrs = lcons(makeString($3), NIL);
					$$->indirection = NIL;
				}
		| relation_name
				{
					$$ = makeNode(Attr);
					$$->relname = $1;
					$$->paramNo = NULL;
					$$->attrs = NIL;
					$$->indirection = NIL;
				}
		;

/* change me to select, update, etc. some day */
event:	SELECT							{ $$ = CMD_SELECT; }
		| UPDATE						{ $$ = CMD_UPDATE; }
		| DELETE						{ $$ = CMD_DELETE; }
		| INSERT						{ $$ = CMD_INSERT; }
		 ;

opt_instead:  INSTEAD					{ $$ = TRUE; }
		| /*EMPTY*/						{ $$ = FALSE; }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				NOTIFY <relation_name>	can appear both in rule bodies and
 *				as a query-level command
 *
 *****************************************************************************/

NotifyStmt:  NOTIFY relation_name
				{
					NotifyStmt *n = makeNode(NotifyStmt);
					n->relname = $2;
					$$ = (Node *)n;
				}
		;

ListenStmt:  LISTEN relation_name
				{
					ListenStmt *n = makeNode(ListenStmt);
					n->relname = $2;
					$$ = (Node *)n;
				}
;

UnlistenStmt:  UNLISTEN relation_name
				{
					UnlistenStmt *n = makeNode(UnlistenStmt);
					n->relname = $2;
					$$ = (Node *)n;
				}
		| UNLISTEN '*'
				{
					UnlistenStmt *n = makeNode(UnlistenStmt);
					n->relname = "*";
					$$ = (Node *)n;
				}
;


/*****************************************************************************
 *
 *		Transactions:
 *
 *		abort transaction
 *				(ABORT)
 *		begin transaction
 *				(BEGIN)
 *		end transaction
 *				(END)
 *
 *****************************************************************************/

TransactionStmt: ABORT_TRANS opt_trans
				{
					TransactionStmt *n = makeNode(TransactionStmt);
					n->command = ABORT_TRANS;
					$$ = (Node *)n;
				}
		| BEGIN_TRANS opt_trans
				{
					TransactionStmt *n = makeNode(TransactionStmt);
					n->command = BEGIN_TRANS;
					$$ = (Node *)n;
				}
		| COMMIT opt_trans
				{
					TransactionStmt *n = makeNode(TransactionStmt);
					n->command = END_TRANS;
					$$ = (Node *)n;
				}
		| END_TRANS opt_trans
				{
					TransactionStmt *n = makeNode(TransactionStmt);
					n->command = END_TRANS;
					$$ = (Node *)n;
				}
		| ROLLBACK opt_trans
				{
					TransactionStmt *n = makeNode(TransactionStmt);
					n->command = ABORT_TRANS;
					$$ = (Node *)n;
				}
		;

opt_trans: WORK									{ $$ = TRUE; }
		| TRANSACTION							{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = TRUE; }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				define view <viewname> '('target-list ')' [where <quals> ]
 *
 *****************************************************************************/

ViewStmt:  CREATE VIEW name AS SelectStmt
				{
					ViewStmt *n = makeNode(ViewStmt);
					n->viewname = $3;
					n->query = (Query *)$5;
					if (((SelectStmt *)n->query)->sortClause != NULL)
						elog(ERROR,"Order by and Distinct on views is not implemented.");
					if (((SelectStmt *)n->query)->unionClause != NULL)
						elog(ERROR,"Views on unions not implemented.");
					if (((SelectStmt *)n->query)->forUpdate != NULL)
						elog(ERROR, "SELECT FOR UPDATE is not allowed in CREATE VIEW");
					$$ = (Node *)n;
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				load "filename"
 *
 *****************************************************************************/

LoadStmt:  LOAD file_name
				{
					LoadStmt *n = makeNode(LoadStmt);
					n->filename = $2;
					$$ = (Node *)n;
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				createdb dbname
 *
 *****************************************************************************/

CreatedbStmt:  CREATE DATABASE database_name WITH opt_database1 opt_database2
				{
					CreatedbStmt *n = makeNode(CreatedbStmt);
					if ($5 == NULL && $6 == NULL) {
						elog(ERROR, "CREATE DATABASE WITH requires at least an option");
					}
					n->dbname = $3;
					n->dbpath = $5;
#ifdef MULTIBYTE
					if ($6 != NULL) {
						n->encoding = pg_char_to_encoding($6);
						if (n->encoding < 0) {
							elog(ERROR, "invalid encoding name %s", $6);
						}
					} else {
						n->encoding = GetTemplateEncoding();
					}
#else
					if ($6 != NULL)
						elog(ERROR, "WITH ENCODING is not supported");
					n->encoding = 0;
#endif
					$$ = (Node *)n;
				}
		| CREATE DATABASE database_name
				{
					CreatedbStmt *n = makeNode(CreatedbStmt);
					n->dbname = $3;
					n->dbpath = NULL;
#ifdef MULTIBYTE
					n->encoding = GetTemplateEncoding();
#else
					n->encoding = 0;
#endif
					$$ = (Node *)n;
				}
		;

opt_database1:  LOCATION '=' location			{ $$ = $3; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

opt_database2:  ENCODING '=' encoding			{ $$ = $3; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

location:  Sconst								{ $$ = $1; }
		| DEFAULT								{ $$ = NULL; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

encoding:  Sconst								{ $$ = $1; }
		| DEFAULT								{ $$ = NULL; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

/*****************************************************************************
 *
 *		QUERY:
 *				destroydb dbname
 *
 *****************************************************************************/

DestroydbStmt:	DROP DATABASE database_name
				{
					DestroydbStmt *n = makeNode(DestroydbStmt);
					n->dbname = $3;
					$$ = (Node *)n;
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				cluster <index_name> on <relation_name>
 *
 *****************************************************************************/

ClusterStmt:  CLUSTER index_name ON relation_name
				{
				   ClusterStmt *n = makeNode(ClusterStmt);
				   n->relname = $4;
				   n->indexname = $2;
				   $$ = (Node*)n;
				}
		;

/*****************************************************************************
 *
 *		QUERY:
 *				vacuum
 *
 *****************************************************************************/

VacuumStmt:  VACUUM opt_verbose opt_analyze
				{
					VacuumStmt *n = makeNode(VacuumStmt);
					n->verbose = $2;
					n->analyze = $3;
					n->vacrel = NULL;
					n->va_spec = NIL;
					$$ = (Node *)n;
				}
		| VACUUM opt_verbose opt_analyze relation_name opt_va_list
				{
					VacuumStmt *n = makeNode(VacuumStmt);
					n->verbose = $2;
					n->analyze = $3;
					n->vacrel = $4;
					n->va_spec = $5;
					if ( $5 != NIL && !$4 )
						elog(ERROR,"parser: syntax error at or near \"(\"");
					$$ = (Node *)n;
				}
		;

opt_verbose:  VERBOSE							{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

opt_analyze:  ANALYZE							{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

opt_va_list:  '(' va_list ')'					{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NIL; }
		;

va_list:  name
				{ $$=lcons($1,NIL); }
		| va_list ',' name
				{ $$=lappend($1,$3); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				EXPLAIN query
 *
 *****************************************************************************/

ExplainStmt:  EXPLAIN opt_verbose OptimizableStmt
				{
					ExplainStmt *n = makeNode(ExplainStmt);
					n->verbose = $2;
					n->query = (Query*)$3;
					$$ = (Node *)n;
				}
		;


/*****************************************************************************
 *																			 *
 *		Optimizable Stmts:													 *
 *																			 *
 *		one of the five queries processed by the planner					 *
 *																			 *
 *		[ultimately] produces query-trees as specified						 *
 *		in the query-spec document in ~postgres/ref							 *
 *																			 *
 *****************************************************************************/

OptimizableStmt:  SelectStmt
		| CursorStmt
		| UpdateStmt
		| InsertStmt
		| NotifyStmt
		| DeleteStmt					/* by default all are $$=$1 */
		;


/*****************************************************************************
 *
 *		QUERY:
 *				INSERT STATEMENTS
 *
 *****************************************************************************/

/* This rule used 'opt_column_list' between 'relation_name' and 'insert_rest'
 * originally. When the second rule of 'insert_rest' was changed to use the
 * new 'SelectStmt' rule (for INTERSECT and EXCEPT) it produced a shift/reduce
 * conflict. So I just changed the rules 'InsertStmt' and 'insert_rest' to
 * accept the same statements without any shift/reduce conflicts
 */
InsertStmt:  INSERT INTO relation_name insert_rest
				{
 					$4->relname = $3;
					$$ = (Node *) $4;
				}
		;

insert_rest:  VALUES '(' target_list ')'
				{
					$$ = makeNode(InsertStmt);
					$$->cols = NULL;
					$$->unique = NULL;
					$$->targetList = $3;
					$$->fromClause = NIL;
					$$->whereClause = NULL;
					$$->groupClause = NIL;
					$$->havingClause = NULL;
					$$->unionClause = NIL;
				}
		| DEFAULT VALUES
				{
					$$ = makeNode(InsertStmt);
					$$->unique = NULL;
					$$->targetList = NIL;
					$$->fromClause = NIL;
					$$->whereClause = NULL;
					$$->groupClause = NIL;
					$$->havingClause = NULL;
					$$->unionClause = NIL;
				 	$$->intersectClause = NIL;
				}
/* We want the full power of SelectStatements including INTERSECT and EXCEPT
 * for insertion.  However, we can't support sort or limit clauses.
 */
		| SelectStmt
				{
					SelectStmt *n = (SelectStmt *) $1;
					if (n->sortClause)
						elog(ERROR, "INSERT ... SELECT can't have ORDER BY");
					$$ = makeNode(InsertStmt);
					$$->cols = NIL;
					$$->unique = n->unique;
					$$->targetList = n->targetList;
					$$->fromClause = n->fromClause;
					$$->whereClause = n->whereClause;
					$$->groupClause = n->groupClause;
					$$->havingClause = n->havingClause;
					$$->unionClause = n->unionClause;
					$$->intersectClause = n->intersectClause;
					$$->unionall = n->unionall;
					$$->forUpdate = n->forUpdate;
				}
		| '(' columnList ')' VALUES '(' target_list ')'
				{
					$$ = makeNode(InsertStmt);
					$$->cols = $2;
					$$->unique = NULL;
					$$->targetList = $6;
					$$->fromClause = NIL;
					$$->whereClause = NULL;
					$$->groupClause = NIL;
					$$->havingClause = NULL;
					$$->unionClause = NIL; 
				 	$$->intersectClause = NIL;
				}
		| '(' columnList ')' SelectStmt
				{
					SelectStmt *n = (SelectStmt *) $4;
					if (n->sortClause)
						elog(ERROR, "INSERT ... SELECT can't have ORDER BY");
					$$ = makeNode(InsertStmt);
					$$->cols = $2;
					$$->unique = n->unique;
					$$->targetList = n->targetList;
					$$->fromClause = n->fromClause;
					$$->whereClause = n->whereClause;
					$$->groupClause = n->groupClause;
					$$->havingClause = n->havingClause;
					$$->unionClause = n->unionClause;
					$$->intersectClause = n->intersectClause;
					$$->unionall = n->unionall;
					$$->forUpdate = n->forUpdate;
				}
		;

opt_column_list:  '(' columnList ')'			{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NIL; }
		;

columnList:
		  columnList ',' columnElem
				{ $$ = lappend($1, $3); }
		| columnElem
				{ $$ = lcons($1, NIL); }
		;

columnElem:  ColId opt_indirection
				{
					Ident *id = makeNode(Ident);
					id->name = $1;
					id->indirection = $2;
					$$ = (Node *)id;
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				DELETE STATEMENTS
 *
 *****************************************************************************/

DeleteStmt:  DELETE FROM relation_name
			 where_clause
				{
					DeleteStmt *n = makeNode(DeleteStmt);
					n->relname = $3;
					n->whereClause = $4;
					$$ = (Node *)n;
				}
		;

LockStmt:	LOCK_P opt_table relation_name opt_lock
				{
					LockStmt *n = makeNode(LockStmt);

					n->relname = $3;
					n->mode = $4;
					$$ = (Node *)n;
				}
		;

opt_lock:  IN lock_type MODE		{ $$ = $2; }
		| /*EMPTY*/				{ $$ = AccessExclusiveLock; }
		;

lock_type:  SHARE ROW EXCLUSIVE	{ $$ = ShareRowExclusiveLock; }
		| ROW opt_lmode			{ $$ = ($2? RowShareLock: RowExclusiveLock); }
		| ACCESS opt_lmode		{ $$ = ($2? AccessShareLock: AccessExclusiveLock); }
		| opt_lmode				{ $$ = ($1? ShareLock: ExclusiveLock); }
		;

opt_lmode:	SHARE				{ $$ = TRUE; }
		| EXCLUSIVE				{ $$ = FALSE; }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				UpdateStmt (UPDATE)
 *
 *****************************************************************************/

UpdateStmt:  UPDATE relation_name
			  SET update_target_list
			  from_clause
			  where_clause
				{
					UpdateStmt *n = makeNode(UpdateStmt);
					n->relname = $2;
					n->targetList = $4;
					n->fromClause = $5;
					n->whereClause = $6;
					$$ = (Node *)n;
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				CURSOR STATEMENTS
 *
 *****************************************************************************/
CursorStmt:  DECLARE name opt_cursor CURSOR FOR SelectStmt
  				{
 					SelectStmt *n;
  
 					n= (SelectStmt *)$6;
  					/* from PORTAL name */
  					/*
  					 *	15 august 1991 -- since 3.0 postgres does locking
					 *	right, we discovered that portals were violating
					 *	locking protocol.  portal locks cannot span xacts.
					 *	as a short-term fix, we installed the check here.
					 *							-- mao
					 */
					if (!IsTransactionBlock())
						elog(ERROR,"Named portals may only be used in begin/end transaction blocks");

					n->portalname = $2;
					n->binary = $3;
					if (n->forUpdate != NULL)
							elog(ERROR,"DECLARE/UPDATE not supported;"
								 		" Cursors must be READ ONLY.");
					$$ = (Node *)n;
				}
		;

opt_cursor:  BINARY						{ $$ = TRUE; }
		| INSENSITIVE					{ $$ = FALSE; }
		| SCROLL						{ $$ = FALSE; }
		| INSENSITIVE SCROLL			{ $$ = FALSE; }
		| /*EMPTY*/						{ $$ = FALSE; }
		;

/*****************************************************************************
 *
 *		QUERY:
 *				SELECT STATEMENTS
 *
 *****************************************************************************/

/* A complete SELECT statement looks like this.  Note sort, for_update,
 * and limit clauses can only appear once, not in each subselect.
 * 
 * The rule returns a SelectStmt Node having the set operations attached to 
 * unionClause and intersectClause (NIL if no set operations were present)
 */

SelectStmt:	  select_clause sort_clause for_update_clause opt_select_limit
			{
				if IsA($1, SelectStmt)
				{
					/* There were no set operations, so just attach the
					 * one-time clauses.
					 */
					SelectStmt *n = (SelectStmt *) $1;
					n->sortClause = $2;
					n->forUpdate = $3;
					n->limitOffset = nth(0, $4);
					n->limitCount = nth(1, $4);
					$$ = (Node *) n;
                }
				else
				{
					/* There were set operations.  The root of the operator
					 * tree is delivered by $1, but we must hand back a
					 * SelectStmt node not an A_Expr Node.
					 * So we find the leftmost 'SelectStmt' in the operator
					 * tree $1 (which is the first Select Statement in the
					 * query), which will be the returned node.
					 * Then we attach the whole operator tree to that node's
					 * 'intersectClause', and a list of all 'SelectStmt' Nodes
					 * in the tree to its 'unionClause'. (NOTE that this means
					 * the top node has indirect recursive pointers to itself!
					 * This would cause trouble if we tried copyObject!!)
					 * The intersectClause and unionClause subtrees will be
					 * left untouched by the main parser, and will only be
					 * processed when control gets to the function
					 * Except_Intersect_Rewrite() (in rewriteHandler.c).
					 */
					Node *op = (Node *) $1;
					List *select_list = NIL;
					SelectStmt *first_select;
					bool	intersect_present = false,
							unionall_present = false;

					/* Take the operator tree as an argument and create a
					 * list of all SelectStmt Nodes found in the tree.
					 *
					 * If one of the SelectStmt Nodes has the 'unionall' flag
					 * set to true the 'unionall_present' flag is also set to
					 * true.
					 */
					create_select_list(op, &select_list, &unionall_present);

					/* Replace all the A_Expr Nodes in the operator tree by
					 * Expr Nodes.
					 *
					 * If an INTERSECT or an EXCEPT is present, the 
					 * 'intersect_present' flag is set to true
					 */
					op = A_Expr_to_Expr(op, &intersect_present);

					/* If both flags are set to true we have a UNION ALL
					 * statement mixed up with INTERSECT or EXCEPT 
					 * which can not be handled at the moment.
					 */
					if (intersect_present && unionall_present)
						elog(ERROR, "UNION ALL not allowed in mixed set operations");

					/* Get the leftmost SeletStmt Node (which automatically
					 * represents the first Select Statement of the query!)
					 */
					first_select = (SelectStmt *) lfirst(select_list);

					/* Attach the list of all SeletStmt Nodes to unionClause */
					first_select->unionClause = select_list;

					/* Attach the whole operator tree to intersectClause */
					first_select->intersectClause = (List *) op;

					/* finally attach the sort clause &etc */
					first_select->sortClause = $2;
					first_select->forUpdate = $3;
					first_select->limitOffset = nth(0, $4);
					first_select->limitCount = nth(1, $4);
					$$ = (Node *) first_select;
				}		
				if (((SelectStmt *)$$)->forUpdate != NULL && QueryIsRule)
					elog(ERROR, "SELECT FOR UPDATE is not allowed in RULES");
			}
		;

/* This rule parses Select statements that can appear within set operations,
 * including UNION, INTERSECT and EXCEPT.  '(' and ')' can be used to specify
 * the ordering of the set operations.  Without '(' and ')' we want the
 * operations to be left associative.
 *
 * Note that sort clauses cannot be included at this level --- a sort clause
 * can only appear at the end of the complete Select, and it will be handled
 * by the topmost SelectStmt rule.  Likewise FOR UPDATE and LIMIT.
 * 
 * The rule builds up an operator tree using A_Expr Nodes. AND Nodes represent
 * INTERSECTs, OR Nodes represent UNIONs, and AND NOT nodes represent EXCEPTs. 
 * The SelectStatements to be connected are the left and right arguments to
 * the A_Expr Nodes.
 * If no set operations appear in the query, the tree consists only of one
 * SelectStmt Node.
 */
select_clause: '(' select_clause ')'
			{
				$$ = $2; 
			}
		| SubSelect
			{
				$$ = $1; 
			}
		| select_clause EXCEPT select_clause
			{
				$$ = (Node *)makeA_Expr(AND,NULL,$1,
							makeA_Expr(NOT,NULL,NULL,$3));
			}
		| select_clause UNION opt_union select_clause
			{	
				if (IsA($4, SelectStmt))
				  {
				     SelectStmt *n = (SelectStmt *)$4;
				     n->unionall = $3;
					 /* NOTE: if UNION ALL appears with a parenthesized set
					  * operation to its right, the ALL is silently discarded.
					  * Should we generate an error instead?  I think it may
					  * be OK since ALL with UNION to its right is ignored
					  * anyway...
					  */
				  }
				$$ = (Node *)makeA_Expr(OR,NULL,$1,$4);
			}
		| select_clause INTERSECT select_clause
			{
				$$ = (Node *)makeA_Expr(AND,NULL,$1,$3);
			}
		; 

SubSelect:	SELECT opt_unique target_list
			 result from_clause where_clause
			 group_clause having_clause
				{
					SelectStmt *n = makeNode(SelectStmt);
					n->unique = $2;
					n->unionall = FALSE;
					n->targetList = $3;
					/* This is new: Subselects support the INTO clause
					 * which allows queries that are not part of the
					 * SQL92 standard and should not be formulated!
					 * We need it for INTERSECT and EXCEPT and I did not
					 * want to create a new rule 'SubSelect1' including the
					 * feature. If it makes troubles we will have to add 
					 * a new rule and change this to prevent INTOs in 
					 * Subselects again.
					 */
					n->istemp = (bool) ((Value *) lfirst($4))->val.ival;
					n->into = (char *) lnext($4);

					n->fromClause = $5;
					n->whereClause = $6;
					n->groupClause = $7;
					n->havingClause = $8;
					$$ = (Node *)n;
				}
		;

		/* easy way to return two values. Can someone improve this?  bjm */
result:  INTO OptTemp opt_table relation_name	{ $$ = lcons(makeInteger($2), (List *)$4); }
		| /*EMPTY*/								{ $$ = lcons(makeInteger(false), NIL); }
		;

opt_table:  TABLE								{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

opt_union:  ALL									{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

opt_unique:  DISTINCT							{ $$ = "*"; }
		| DISTINCT ON ColId						{ $$ = $3; }
		| ALL									{ $$ = NULL; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

sort_clause:  ORDER BY sortby_list				{ $$ = $3; }
		| /*EMPTY*/								{ $$ = NIL; }
		;

sortby_list:  sortby							{ $$ = lcons($1, NIL); }
		| sortby_list ',' sortby				{ $$ = lappend($1, $3); }
		;

sortby: a_expr OptUseOp
				{
					$$ = makeNode(SortGroupBy);
					$$->node = $1;
					$$->useOp = $2;
				}
		;

OptUseOp:  USING Op								{ $$ = $2; }
		| USING '<'								{ $$ = "<"; }
		| USING '>'								{ $$ = ">"; }
		| ASC									{ $$ = "<"; }
		| DESC									{ $$ = ">"; }
		| /*EMPTY*/								{ $$ = "<"; /*default*/ }
		;


opt_select_limit:	LIMIT select_limit_value ',' select_offset_value
			{ $$ = lappend(lappend(NIL, $4), $2); }
		| LIMIT select_limit_value OFFSET select_offset_value
			{ $$ = lappend(lappend(NIL, $4), $2); }
		| LIMIT select_limit_value
			{ $$ = lappend(lappend(NIL, NULL), $2); }
		| OFFSET select_offset_value LIMIT select_limit_value
			{ $$ = lappend(lappend(NIL, $2), $4); }
		| OFFSET select_offset_value
			{ $$ = lappend(lappend(NIL, $2), NULL); }
		| /* EMPTY */
			{ $$ = lappend(lappend(NIL, NULL), NULL); }
		;

select_limit_value:		Iconst
			{
				Const	*n = makeNode(Const);

				if ($1 < 1)
					elog(ERROR, "selection limit must be ALL or a positive integer value > 0");

				n->consttype	= INT4OID;
				n->constlen		= sizeof(int4);
				n->constvalue	= (Datum)$1;
				n->constisnull	= FALSE;
				n->constbyval	= TRUE;
				n->constisset	= FALSE;
				n->constiscast	= FALSE;
				$$ = (Node *)n;
			}
		| ALL
			{
				Const	*n = makeNode(Const);

				n->consttype	= INT4OID;
				n->constlen		= sizeof(int4);
				n->constvalue	= (Datum)0;
				n->constisnull	= FALSE;
				n->constbyval	= TRUE;
				n->constisset	= FALSE;
				n->constiscast	= FALSE;
				$$ = (Node *)n;
			}
		| PARAM
			{
				Param	*n = makeNode(Param);

				n->paramkind	= PARAM_NUM;
				n->paramid		= $1;
				n->paramtype	= INT4OID;
				$$ = (Node *)n;
			}
		;

select_offset_value:	Iconst
			{
				Const	*n = makeNode(Const);

				n->consttype	= INT4OID;
				n->constlen		= sizeof(int4);
				n->constvalue	= (Datum)$1;
				n->constisnull	= FALSE;
				n->constbyval	= TRUE;
				n->constisset	= FALSE;
				n->constiscast	= FALSE;
				$$ = (Node *)n;
			}
		| PARAM
			{
				Param	*n = makeNode(Param);

				n->paramkind	= PARAM_NUM;
				n->paramid		= $1;
				n->paramtype	= INT4OID;
				$$ = (Node *)n;
			}
		;
/*
 *	jimmy bell-style recursive queries aren't supported in the
 *	current system.
 *
 *	...however, recursive addattr and rename supported.  make special
 *	cases for these.
 */
opt_inh_star:  '*'								{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

relation_name_list:  name_list;

name_list:  name
				{	$$ = lcons(makeString($1),NIL); }
		| name_list ',' name
				{	$$ = lappend($1,makeString($3)); }
		;

group_clause:  GROUP BY expr_list				{ $$ = $3; }
		| /*EMPTY*/								{ $$ = NIL; }
		;

having_clause:  HAVING a_expr
				{
					$$ = $2;
				}
		| /*EMPTY*/								{ $$ = NULL; }
		;

for_update_clause:  FOR UPDATE update_list		{ $$ = $3; }
		| FOR READ ONLY							{ $$ = NULL; }
		| /* EMPTY */							{ $$ = NULL; }
		;

update_list:  OF va_list						{ $$ = $2; }
		| /* EMPTY */							{ $$ = lcons(NULL, NULL); }
		;

/*****************************************************************************
 *
 *	clauses common to all Optimizable Stmts:
 *		from_clause		-
 *		where_clause	-
 *
 *****************************************************************************/

from_clause:  FROM from_expr					{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NIL; }
		;

from_expr:  '(' join_clause_with_union ')'
				{ $$ = $2; }
		| join_clause
				{ $$ = $1; }
		| table_list
				{ $$ = $1; }
		;

table_list:  table_list ',' table_expr
				{ $$ = lappend($1, $3); }
		| table_expr
				{ $$ = lcons($1, NIL); }
		;

table_expr:  relation_expr AS ColLabel
				{
					$$ = makeNode(RangeVar);
					$$->relExpr = $1;
					$$->name = $3;
				}
		| relation_expr ColId
				{
					$$ = makeNode(RangeVar);
					$$->relExpr = $1;
					$$->name = $2;
				}
		| relation_expr
				{
					$$ = makeNode(RangeVar);
					$$->relExpr = $1;
					$$->name = NULL;
				}
		;

/* A UNION JOIN is the same as a FULL OUTER JOIN which *omits*
 * all result rows which would have matched on an INNER JOIN.
 * Let's reject this for now. - thomas 1999-01-08
 */
join_clause_with_union:  join_clause
				{	$$ = $1; }
		| table_expr UNION JOIN table_expr
				{	elog(ERROR,"UNION JOIN not yet implemented"); }
		;

join_clause:  table_expr join_list
				{
					Node *n = lfirst($2);

					/* JoinExpr came back? then it is a join of some sort...
					 */
					if (IsA(n, JoinExpr))
					{
						JoinExpr *j = (JoinExpr *)n;
						j->larg = $1;
						$$ = $2;
					}
					/* otherwise, it was a cross join,
					 * which we just represent as an inner join...
					 */
					else
						$$ = lcons($1, $2);
				}
		;

join_list:  join_list join_expr
				{
					$$ = lappend($1, $2);
				}
		| join_expr
				{
					$$ = lcons($1, NIL);
				}
		;

/* This is everything but the left side of a join.
 * Note that a CROSS JOIN is the same as an unqualified
 * inner join, so just pass back the right-side table.
 * A NATURAL JOIN implicitly matches column names between
 * tables, so we'll collect those during the later transformation.
 */
join_expr:  join_type JOIN table_expr join_qual
				{
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = $1;
					n->rarg = (Node *)$3;
					n->quals = $4;
					$$ = (Node *)n;
				}
		| NATURAL join_type JOIN table_expr
				{
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = $2;
					n->rarg = (Node *)$4;
					n->quals = NULL; /* figure out which columns later... */
					$$ = (Node *)n;
				}
		| CROSS JOIN table_expr
				{ $$ = (Node *)$3; }
		;

/* OUTER is just noise... */
join_type:  FULL join_outer
				{
					$$ = FULL;
					elog(NOTICE,"FULL OUTER JOIN not yet implemented");
				}
		| LEFT join_outer
				{
					$$ = LEFT;
					elog(NOTICE,"LEFT OUTER JOIN not yet implemented");
				}
		| RIGHT join_outer
				{
					$$ = RIGHT;
					elog(NOTICE,"RIGHT OUTER JOIN not yet implemented");
				}
		| OUTER_P
				{
					$$ = LEFT;
					elog(NOTICE,"OUTER JOIN not yet implemented");
				}
		| INNER_P
				{
					$$ = INNER_P;
				}
		| /*EMPTY*/
				{
					$$ = INNER_P;
				}
		;

join_outer:  OUTER_P							{ $$ = NULL; }
		| /*EMPTY*/								{ $$ = NULL;  /* no qualifiers */ }
		;

/* JOIN qualification clauses
 * Possibilities are:
 *  USING ( column list ) allows only unqualified column names,
 *                        which must match between tables.
 *  ON expr allows more general qualifications.
 * - thomas 1999-01-07
 */

join_qual:  USING '(' using_list ')'			{ $$ = $3; }
		| ON a_expr								{ $$ = lcons($2, NIL); }
		;

using_list:  using_list ',' using_expr			{ $$ = lappend($1, $3); }
		| using_expr							{ $$ = lcons($1, NIL); }
		;

using_expr:  ColId
				{
					/* could be a column name or a relation_name */
					Ident *n = makeNode(Ident);
					n->name = $1;
					n->indirection = NULL;
					$$ = (Node *)n;
				}
		;

where_clause:  WHERE a_expr						{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NULL;  /* no qualifiers */ }
		;

relation_expr:	relation_name
				{
					/* normal relations */
					$$ = makeNode(RelExpr);
					$$->relname = $1;
					$$->inh = FALSE;
				}
		| relation_name '*'				  %prec '='
				{
					/* inheritance query */
					$$ = makeNode(RelExpr);
					$$->relname = $1;
					$$->inh = TRUE;
				}

opt_array_bounds:	'[' ']' opt_array_bounds
				{  $$ = lcons(makeInteger(-1), $3); }
		| '[' Iconst ']' opt_array_bounds
				{  $$ = lcons(makeInteger($2), $4); }
		| /*EMPTY*/
				{  $$ = NIL; }
		;


/*****************************************************************************
 *
 *	Type syntax
 *		SQL92 introduces a large amount of type-specific syntax.
 *		Define individual clauses to handle these cases, and use
 *		 the generic case to handle regular type-extensible Postgres syntax.
 *		- thomas 1997-10-10
 *
 *****************************************************************************/

Typename:  SimpleTypename opt_array_bounds
				{
					$$ = $1;
					$$->arrayBounds = $2;

					/* Is this the name of a complex type? If so, implement
					 * it as a set.
					 */
					if (!strcmp(saved_relname, $$->name))
						/* This attr is the same type as the relation
						 * being defined. The classic example: create
						 * emp(name=text,mgr=emp)
						 */
						$$->setof = TRUE;
					else if (typeTypeRelid(typenameType($$->name)) != InvalidOid)
						 /* (Eventually add in here that the set can only
						  * contain one element.)
						  */
						$$->setof = TRUE;
					else
						$$->setof = FALSE;
				}
		| SETOF SimpleTypename
				{
					$$ = $2;
					$$->setof = TRUE;
				}
		;

SimpleTypename:  Generic
		| Numeric
		| Character
		| Datetime
		;

Generic:  generic
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType($1);
					$$->typmod = -1;
				}
		;

generic:  IDENT									{ $$ = $1; }
		| TYPE_P								{ $$ = "type"; }
		;

/* SQL92 numeric data types
 * Check FLOAT() precision limits assuming IEEE floating types.
 * Provide real DECIMAL() and NUMERIC() implementations now - Jan 1998-12-30
 * - thomas 1997-09-18
 */
Numeric:  FLOAT opt_float
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType($2);
					$$->typmod = -1;
				}
		| DOUBLE PRECISION
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType("float");
				}
		| DECIMAL opt_decimal
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType("numeric");
					$$->typmod = $2;
				}
		| NUMERIC opt_numeric
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType("numeric");
					$$->typmod = $2;
				}
		;

numeric:  FLOAT
				{	$$ = xlateSqlType("float8"); }
		| DOUBLE PRECISION
				{	$$ = xlateSqlType("float8"); }
		| DECIMAL
				{	$$ = xlateSqlType("numeric"); }
		| NUMERIC
				{	$$ = xlateSqlType("numeric"); }
		;

opt_float:  '(' Iconst ')'
				{
					if ($2 < 1)
						elog(ERROR,"precision for FLOAT must be at least 1");
					else if ($2 < 7)
						$$ = xlateSqlType("float4");
					else if ($2 < 16)
						$$ = xlateSqlType("float8");
					else
						elog(ERROR,"precision for FLOAT must be less than 16");
				}
		| /*EMPTY*/
				{
					$$ = xlateSqlType("float8");
				}
		;

opt_numeric:  '(' Iconst ',' Iconst ')'
				{
					if ($2 < 1 || $2 > NUMERIC_MAX_PRECISION)
						elog(ERROR,"NUMERIC precision %d must be beween 1 and %d",
									$2, NUMERIC_MAX_PRECISION);
					if ($4 < 0 || $4 > $2)
						elog(ERROR,"NUMERIC scale %d must be between 0 and precision %d",
									$4,$2);

					$$ = (($2 << 16) | $4) + VARHDRSZ;
				}
		| '(' Iconst ')'
				{
					if ($2 < 1 || $2 > NUMERIC_MAX_PRECISION)
						elog(ERROR,"NUMERIC precision %d must be beween 1 and %d",
									$2, NUMERIC_MAX_PRECISION);

					$$ = ($2 << 16) + VARHDRSZ;
				}
		| /*EMPTY*/
				{
					$$ = ((NUMERIC_DEFAULT_PRECISION << 16) | NUMERIC_DEFAULT_SCALE) + VARHDRSZ;
				}
		;

opt_decimal:  '(' Iconst ',' Iconst ')'
				{
					if ($2 < 1 || $2 > NUMERIC_MAX_PRECISION)
						elog(ERROR,"DECIMAL precision %d must be beween 1 and %d",
									$2, NUMERIC_MAX_PRECISION);
					if ($4 < 0 || $4 > $2)
						elog(ERROR,"DECIMAL scale %d must be between 0 and precision %d",
									$4,$2);

					$$ = (($2 << 16) | $4) + VARHDRSZ;
				}
		| '(' Iconst ')'
				{
					if ($2 < 1 || $2 > NUMERIC_MAX_PRECISION)
						elog(ERROR,"DECIMAL precision %d must be beween 1 and %d",
									$2, NUMERIC_MAX_PRECISION);

					$$ = ($2 << 16) + VARHDRSZ;
				}
		| /*EMPTY*/
				{
					$$ = ((NUMERIC_DEFAULT_PRECISION << 16) | NUMERIC_DEFAULT_SCALE) + VARHDRSZ;
				}
		;


/* SQL92 character data types
 * The following implements CHAR() and VARCHAR().
 */
Character:  character '(' Iconst ')'
				{
					$$ = makeNode(TypeName);
					if (strcasecmp($1, "char") == 0)
						$$->name = xlateSqlType("bpchar");
					else if (strcasecmp($1, "varchar") == 0)
						$$->name = xlateSqlType("varchar");
					else
						yyerror("internal parsing error; unrecognized character type");

					if ($3 < 1)
						elog(ERROR,"length for '%s' type must be at least 1",$1);
					else if ($3 > MaxAttrSize)
						elog(ERROR,"length for type '%s' cannot exceed %d",$1,
							MaxAttrSize);

					/* we actually implement this sort of like a varlen, so
					 * the first 4 bytes is the length. (the difference
					 * between this and "text" is that we blank-pad and
					 * truncate where necessary
					 */
					$$->typmod = VARHDRSZ + $3;
				}
		| character
				{
					$$ = makeNode(TypeName);
					/* Let's try to make all single-character types into bpchar(1)
					 * - thomas 1998-05-07
					 */
					if (strcasecmp($1, "char") == 0)
					{
						$$->name = xlateSqlType("bpchar");
						$$->typmod = VARHDRSZ + 1;
					}
					else
					{
						$$->name = xlateSqlType($1);
						$$->typmod = -1;
					}
				}
		;

character:  CHARACTER opt_varying opt_charset opt_collate
				{
					char *type, *c;
					if (($3 == NULL) || (strcasecmp($3, "sql_text") == 0)) {
						if ($2) type = xlateSqlType("varchar");
						else type = xlateSqlType("char");
					} else {
						if ($2) {
							c = palloc(strlen("var") + strlen($3) + 1);
							strcpy(c, "var");
							strcat(c, $3);
							type = xlateSqlType(c);
						} else {
							type = xlateSqlType($3);
						}
					};
					if ($4 != NULL)
						elog(NOTICE,"COLLATE %s not yet implemented; clause ignored",$4);
					$$ = type;
				}
		| CHAR opt_varying						{ $$ = xlateSqlType($2? "varchar": "char"); }
		| VARCHAR								{ $$ = xlateSqlType("varchar"); }
		| NATIONAL CHARACTER opt_varying		{ $$ = xlateSqlType($3? "varchar": "char"); }
		| NCHAR opt_varying						{ $$ = xlateSqlType($2? "varchar": "char"); }
		;

opt_varying:  VARYING							{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

opt_charset:  CHARACTER SET ColId				{ $$ = $3; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

opt_collate:  COLLATE ColId						{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

Datetime:  datetime
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType($1);
					$$->typmod = -1;
				}
		| TIMESTAMP opt_timezone
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType("timestamp");
					$$->timezone = $2;
					$$->typmod = -1;
				}
		| TIME
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType("time");
					$$->typmod = -1;
				}
		| INTERVAL opt_interval
				{
					$$ = makeNode(TypeName);
					$$->name = xlateSqlType("interval");
					$$->typmod = -1;
				}
		;

datetime:  YEAR_P								{ $$ = "year"; }
		| MONTH_P								{ $$ = "month"; }
		| DAY_P									{ $$ = "day"; }
		| HOUR_P								{ $$ = "hour"; }
		| MINUTE_P								{ $$ = "minute"; }
		| SECOND_P								{ $$ = "second"; }
		;

opt_timezone:  WITH TIME ZONE					{ $$ = TRUE; }
		| /*EMPTY*/								{ $$ = FALSE; }
		;

opt_interval:  datetime							{ $$ = lcons($1, NIL); }
		| YEAR_P TO MONTH_P						{ $$ = NIL; }
		| DAY_P TO HOUR_P						{ $$ = NIL; }
		| DAY_P TO MINUTE_P						{ $$ = NIL; }
		| DAY_P TO SECOND_P						{ $$ = NIL; }
		| HOUR_P TO MINUTE_P					{ $$ = NIL; }
		| HOUR_P TO SECOND_P					{ $$ = NIL; }
		| MINUTE_P TO SECOND_P					{ $$ = NIL; }
		| /*EMPTY*/								{ $$ = NIL; }
		;


/*****************************************************************************
 *
 *	expression grammar, still needs some cleanup
 *
 *****************************************************************************/

a_expr_or_null:  a_expr
				{ $$ = $1; }
		| NULL_P
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Null;
					$$ = (Node *)n;
				}
		;

/* Expressions using row descriptors
 * Define row_descriptor to allow yacc to break the reduce/reduce conflict
 *  with singleton expressions.
 * Eliminated lots of code by defining row_op and sub_type clauses.
 * However, can not consolidate EXPR_LINK case with others subselects
 *  due to shift/reduce conflict with the non-subselect clause (the parser
 *  would have to look ahead more than one token to resolve the conflict).
 * - thomas 1998-05-09
 */
row_expr: '(' row_descriptor ')' IN '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = $2;
					n->oper = lcons("=",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $6;
					$$ = (Node *)n;
				}
		| '(' row_descriptor ')' NOT IN '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = $2;
					n->oper = lcons("<>",NIL);
					n->useor = true;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $7;
					$$ = (Node *)n;
				}
		| '(' row_descriptor ')' row_op sub_type '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = $2;
					n->oper = lcons($4, NIL);
					if (strcmp($4,"<>") == 0)
						n->useor = true;
					else
						n->useor = false;
					n->subLinkType = $5;
					n->subselect = $7;
					$$ = (Node *)n;
				}
		| '(' row_descriptor ')' row_op '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = $2;
					n->oper = lcons($4, NIL);
					if (strcmp($4,"<>") == 0)
						n->useor = true;
					else
						n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $6;
					$$ = (Node *)n;
				}
		| '(' row_descriptor ')' row_op '(' row_descriptor ')'
				{
					$$ = makeRowExpr($4, $2, $6);
				}
		;

row_descriptor:  row_list ',' a_expr
				{
					$$ = lappend($1, $3);
				}
		;

row_list:  row_list ',' a_expr
				{
					$$ = lappend($1, $3);
				}
		| a_expr
				{
					$$ = lcons($1, NIL);
				}
		;

row_op:  Op									{ $$ = $1; }
		| '<'								{ $$ = "<"; }
		| '='								{ $$ = "="; }
		| '>'								{ $$ = ">"; }
		| '+'								{ $$ = "+"; }
		| '-'								{ $$ = "-"; }
		| '*'								{ $$ = "*"; }
		| '/'								{ $$ = "/"; }
		| '%'								{ $$ = "%"; }
		| '^'								{ $$ = "^"; }
		| '|'								{ $$ = "|"; }
		;

sub_type:  ANY								{ $$ = ANY_SUBLINK; }
		| ALL								{ $$ = ALL_SUBLINK; }
		;

/* General expressions
 * This is the heart of the expression syntax.
 * Note that the BETWEEN clause looks similar to a boolean expression
 *  and so we must define b_expr which is almost the same as a_expr
 *  but without the boolean expressions.
 * All operations/expressions are allowed in a BETWEEN clause
 *  if surrounded by parens.
 */
a_expr:  attr
				{	$$ = (Node *) $1;  }
		| row_expr
				{	$$ = $1;  }
		| AexprConst
				{	$$ = $1;  }
		| ColId opt_indirection
				{
					/* could be a column name or a relation_name */
					Ident *n = makeNode(Ident);
					n->name = $1;
					n->indirection = $2;
					$$ = (Node *)n;
				}
		| '-' a_expr %prec UMINUS
				{	$$ = doNegate($2); }
		| '%' a_expr
				{	$$ = makeA_Expr(OP, "%", NULL, $2); }
		| '^' a_expr
				{	$$ = makeA_Expr(OP, "^", NULL, $2); }
		| '|' a_expr
				{	$$ = makeA_Expr(OP, "|", NULL, $2); }
		| ':' a_expr
				{	$$ = makeA_Expr(OP, ":", NULL, $2); }
		| ';' a_expr
				{	$$ = makeA_Expr(OP, ";", NULL, $2); }
		| a_expr '%'
				{	$$ = makeA_Expr(OP, "%", $1, NULL); }
		| a_expr '^'
				{	$$ = makeA_Expr(OP, "^", $1, NULL); }
		| a_expr '|'
				{	$$ = makeA_Expr(OP, "|", $1, NULL); }
		| a_expr '+' a_expr
				{	$$ = makeA_Expr(OP, "+", $1, $3); }
		| a_expr '-' a_expr
				{	$$ = makeA_Expr(OP, "-", $1, $3); }
		| a_expr '*' a_expr
				{	$$ = makeA_Expr(OP, "*", $1, $3); }
		| a_expr '/' a_expr
				{	$$ = makeA_Expr(OP, "/", $1, $3); }
		| a_expr '%' a_expr
				{	$$ = makeA_Expr(OP, "%", $1, $3); }
		| a_expr '^' a_expr
				{	$$ = makeA_Expr(OP, "^", $1, $3); }
		| a_expr '|' a_expr
				{	$$ = makeA_Expr(OP, "|", $1, $3); }
		| a_expr '<' a_expr
				{	$$ = makeA_Expr(OP, "<", $1, $3); }
		| a_expr '>' a_expr
				{	$$ = makeA_Expr(OP, ">", $1, $3); }

  		| a_expr '=' NULL_P
  				{	$$ = makeA_Expr(ISNULL, NULL, $1, NULL); }
		/* We allow this for standards-broken SQL products, like MS stuff */
  		| NULL_P '=' a_expr
  				{	$$ = makeA_Expr(ISNULL, NULL, $3, NULL); }

		| a_expr '=' a_expr
				{	$$ = makeA_Expr(OP, "=", $1, $3); }
		| a_expr TYPECAST Typename
				{
					$$ = (Node *)$1;
					/* AexprConst can be either A_Const or ParamNo */
					if (nodeTag($1) == T_A_Const) {
						((A_Const *)$1)->typename = $3;
					} else if (nodeTag($1) == T_ParamNo) {
						((ParamNo *)$1)->typename = $3;
					/* otherwise, try to transform to a function call */
					} else {
						FuncCall *n = makeNode(FuncCall);
						n->funcname = $3->name;
						n->args = lcons($1,NIL);
						$$ = (Node *)n;
					}
				}
		| CAST '(' a_expr AS Typename ')'
				{
					$$ = (Node *)$3;
					/* AexprConst can be either A_Const or ParamNo */
					if (nodeTag($3) == T_A_Const) {
						((A_Const *)$3)->typename = $5;
					} else if (nodeTag($3) == T_ParamNo) {
						((ParamNo *)$3)->typename = $5;
					/* otherwise, try to transform to a function call */
					} else {
						FuncCall *n = makeNode(FuncCall);
						n->funcname = $5->name;
						n->args = lcons($3,NIL);
						$$ = (Node *)n;
					}
				}
		| '(' a_expr_or_null ')'
				{	$$ = $2; }
		| a_expr Op a_expr
				{	$$ = makeA_Expr(OP, $2, $1, $3);	}
		| a_expr LIKE a_expr
				{	$$ = makeA_Expr(OP, "~~", $1, $3); }
		| a_expr NOT LIKE a_expr
				{	$$ = makeA_Expr(OP, "!~~", $1, $4); }
		| Op a_expr
				{	$$ = makeA_Expr(OP, $1, NULL, $2); }
		| a_expr Op
				{	$$ = makeA_Expr(OP, $2, $1, NULL); }
		| func_name '(' '*' ')'
				{
					/* cheap hack for aggregate (eg. count) */
					FuncCall *n = makeNode(FuncCall);
					A_Const *star = makeNode(A_Const);

					star->val.type = T_String;
					star->val.val.str = "";
					n->funcname = $1;
					n->args = lcons(star, NIL);
					$$ = (Node *)n;
				}
		| func_name '(' ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = $1;
					n->args = NIL;
					$$ = (Node *)n;
				}
		| func_name '(' expr_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = $1;
					n->args = $3;
					$$ = (Node *)n;
				}
		| CURRENT_DATE
				{
					A_Const *n = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->val.type = T_String;
					n->val.val.str = "now";
					n->typename = t;

					t->name = xlateSqlType("date");
					t->setof = FALSE;
					t->typmod = -1;
 
					$$ = (Node *)n;
				}
		| CURRENT_TIME
				{
					A_Const *n = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->val.type = T_String;
					n->val.val.str = "now";
					n->typename = t;

					t->name = xlateSqlType("time");
					t->setof = FALSE;
					t->typmod = -1;

					$$ = (Node *)n;
				}
		| CURRENT_TIME '(' Iconst ')'
				{
					FuncCall *n = makeNode(FuncCall);
					A_Const *s = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->funcname = xlateSqlType("time");
					n->args = lcons(s, NIL);

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = t;

					t->name = xlateSqlType("time");
					t->setof = FALSE;
					t->typmod = -1;

					if ($3 != 0)
						elog(NOTICE,"CURRENT_TIME(%d) precision not implemented; zero used instead",$3);

					$$ = (Node *)n;
				}
		| CURRENT_TIMESTAMP
				{
					A_Const *n = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->val.type = T_String;
					n->val.val.str = "now";
					n->typename = t;

					t->name = xlateSqlType("timestamp");
					t->setof = FALSE;
					t->typmod = -1;

					$$ = (Node *)n;
				}
		| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					FuncCall *n = makeNode(FuncCall);
					A_Const *s = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->funcname = xlateSqlType("timestamp");
					n->args = lcons(s, NIL);

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = t;

					t->name = xlateSqlType("timestamp");
					t->setof = FALSE;
					t->typmod = -1;

					if ($3 != 0)
						elog(NOTICE,"CURRENT_TIMESTAMP(%d) precision not implemented; zero used instead",$3);

					$$ = (Node *)n;
				}
		| CURRENT_USER
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "getpgusername";
					n->args = NIL;
					$$ = (Node *)n;
				}
		| USER
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "getpgusername";
					n->args = NIL;
					$$ = (Node *)n;
				}
		| EXISTS '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = NIL;
					n->useor = false;
					n->oper = NIL;
					n->subLinkType = EXISTS_SUBLINK;
					n->subselect = $3;
					$$ = (Node *)n;
				}
		| EXTRACT '(' extract_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "date_part";
					n->args = $3;
					$$ = (Node *)n;
				}
		| POSITION '(' position_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "strpos";
					n->args = $3;
					$$ = (Node *)n;
				}
		| SUBSTRING '(' substr_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "substr";
					n->args = $3;
					$$ = (Node *)n;
				}
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "btrim";
					n->args = $4;
					$$ = (Node *)n;
				}
		| TRIM '(' LEADING trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "ltrim";
					n->args = $4;
					$$ = (Node *)n;
				}
		| TRIM '(' TRAILING trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "rtrim";
					n->args = $4;
					$$ = (Node *)n;
				}
		| TRIM '(' trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "btrim";
					n->args = $3;
					$$ = (Node *)n;
				}
		| a_expr ISNULL
				{	$$ = makeA_Expr(ISNULL, NULL, $1, NULL); }
		| a_expr IS NULL_P
				{	$$ = makeA_Expr(ISNULL, NULL, $1, NULL); }
		| a_expr NOTNULL
				{	$$ = makeA_Expr(NOTNULL, NULL, $1, NULL); }
		| a_expr IS NOT NULL_P
				{	$$ = makeA_Expr(NOTNULL, NULL, $1, NULL); }
		/* IS TRUE, IS FALSE, etc used to be function calls
		 *  but let's make them expressions to allow the optimizer
		 *  a chance to eliminate them if a_expr is a constant string.
		 * - thomas 1997-12-22
		 */
		| a_expr IS TRUE_P
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = "t";
					n->typename = makeNode(TypeName);
					n->typename->name = xlateSqlType("bool");
					n->typename->typmod = -1;
					$$ = makeA_Expr(OP, "=", $1,(Node *)n);
				}
		| a_expr IS NOT FALSE_P
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = "t";
					n->typename = makeNode(TypeName);
					n->typename->name = xlateSqlType("bool");
					n->typename->typmod = -1;
					$$ = makeA_Expr(OP, "=", $1,(Node *)n);
				}
		| a_expr IS FALSE_P
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = "f";
					n->typename = makeNode(TypeName);
					n->typename->name = xlateSqlType("bool");
					n->typename->typmod = -1;
					$$ = makeA_Expr(OP, "=", $1,(Node *)n);
				}
		| a_expr IS NOT TRUE_P
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = "f";
					n->typename = makeNode(TypeName);
					n->typename->name = xlateSqlType("bool");
					n->typename->typmod = -1;
					$$ = makeA_Expr(OP, "=", $1,(Node *)n);
				}
		| a_expr BETWEEN b_expr AND b_expr
				{
					$$ = makeA_Expr(AND, NULL,
						makeA_Expr(OP, ">=", $1, $3),
						makeA_Expr(OP, "<=", $1, $5));
				}
		| a_expr NOT BETWEEN b_expr AND b_expr
				{
					$$ = makeA_Expr(OR, NULL,
						makeA_Expr(OP, "<", $1, $4),
						makeA_Expr(OP, ">", $1, $6));
				}
		| a_expr IN '(' in_expr ')'
				{
					/* in_expr returns a SubLink or a list of a_exprs */
					if (IsA($4, SubLink))
					{
							SubLink *n = (SubLink *)$4;
							n->lefthand = lcons($1, NIL);
							n->oper = lcons("=",NIL);
							n->useor = false;
							n->subLinkType = ANY_SUBLINK;
							$$ = (Node *)n;
					}
					else
					{
						Node *n = NULL;
						List *l;
						foreach(l, (List *) $4)
						{
							Node *cmp = makeA_Expr(OP, "=", $1, lfirst(l));
							if (n == NULL)
								n = cmp;
							else
								n = makeA_Expr(OR, NULL, n, cmp);
						}
						$$ = n;
					}
				}
		| a_expr NOT IN '(' in_expr ')'
				{
					/* in_expr returns a SubLink or a list of a_exprs */
					if (IsA($5, SubLink))
					{
						SubLink *n = (SubLink *)$5;
						n->lefthand = lcons($1, NIL);
						n->oper = lcons("<>",NIL);
						n->useor = false;
						n->subLinkType = ALL_SUBLINK;
						$$ = (Node *)n;
					}
					else
					{
						Node *n = NULL;
						List *l;
						foreach(l, (List *) $5)
						{
							Node *cmp = makeA_Expr(OP, "<>", $1, lfirst(l));
							if (n == NULL)
								n = cmp;
							else
								n = makeA_Expr(AND, NULL, n, cmp);
						}
						$$ = n;
					}
				}
		| a_expr Op '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons($2,NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '+' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("+",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '-' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("-",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '*' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("*",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '/' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("/",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '%' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("%",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '^' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("^",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '|' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("|",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '<' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("<",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '>' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons(">",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr '=' '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("=",NIL);
					n->useor = false;
					n->subLinkType = EXPR_SUBLINK;
					n->subselect = $4;
					$$ = (Node *)n;
				}
		| a_expr Op ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons($2,NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '+' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("+",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '-' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("-",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '*' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("*",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '/' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("/",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '%' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("%",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '^' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("^",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '|' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("|",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '<' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("<",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '>' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons(">",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '=' ANY '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1,NIL);
					n->oper = lcons("=",NIL);
					n->useor = false;
					n->subLinkType = ANY_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr Op ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons($2,NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '+' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("+",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '-' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("-",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '*' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("*",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '/' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("/",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '%' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("%",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '^' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("^",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '|' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("|",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '<' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("<",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '>' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons(">",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr '=' ALL '(' SubSelect ')'
				{
					SubLink *n = makeNode(SubLink);
					n->lefthand = lcons($1, NULL);
					n->oper = lcons("=",NIL);
					n->useor = false;
					n->subLinkType = ALL_SUBLINK;
					n->subselect = $5;
					$$ = (Node *)n;
				}
		| a_expr AND a_expr
				{	$$ = makeA_Expr(AND, NULL, $1, $3); }
		| a_expr OR a_expr
				{	$$ = makeA_Expr(OR, NULL, $1, $3); }
		| NOT a_expr
				{	$$ = makeA_Expr(NOT, NULL, NULL, $2); }
		| case_expr
				{	$$ = $1; }
		;

/* Restricted expressions
 * b_expr is a subset of the complete expression syntax
 *  defined by a_expr. b_expr is used in BETWEEN clauses
 *  to eliminate parser ambiguities stemming from the AND keyword,
 *  and also in POSITION clauses where the IN keyword gives trouble.
 */
b_expr:  attr
				{	$$ = (Node *) $1;  }
		| AexprConst
				{	$$ = $1;  }
		| ColId opt_indirection
				{
					/* could be a column name or a relation_name */
					Ident *n = makeNode(Ident);
					n->name = $1;
					n->indirection = $2;
					$$ = (Node *)n;
				}
		| '-' b_expr %prec UMINUS
				{	$$ = doNegate($2); }
		| '%' b_expr
				{	$$ = makeA_Expr(OP, "%", NULL, $2); }
		| '^' b_expr
				{	$$ = makeA_Expr(OP, "^", NULL, $2); }
		| '|' b_expr
				{	$$ = makeA_Expr(OP, "|", NULL, $2); }
		| ':' b_expr
				{	$$ = makeA_Expr(OP, ":", NULL, $2); }
		| ';' b_expr
				{	$$ = makeA_Expr(OP, ";", NULL, $2); }
		| b_expr '%'
				{	$$ = makeA_Expr(OP, "%", $1, NULL); }
		| b_expr '^'
				{	$$ = makeA_Expr(OP, "^", $1, NULL); }
		| b_expr '|'
				{	$$ = makeA_Expr(OP, "|", $1, NULL); }
		| b_expr '+' b_expr
				{	$$ = makeA_Expr(OP, "+", $1, $3); }
		| b_expr '-' b_expr
				{	$$ = makeA_Expr(OP, "-", $1, $3); }
		| b_expr '*' b_expr
				{	$$ = makeA_Expr(OP, "*", $1, $3); }
		| b_expr '/' b_expr
				{	$$ = makeA_Expr(OP, "/", $1, $3); }
		| b_expr '%' b_expr
				{	$$ = makeA_Expr(OP, "%", $1, $3); }
		| b_expr '^' b_expr
				{	$$ = makeA_Expr(OP, "^", $1, $3); }
		| b_expr '|' b_expr
				{	$$ = makeA_Expr(OP, "|", $1, $3); }
		| b_expr TYPECAST Typename
				{
					$$ = (Node *)$1;
					/* AexprConst can be either A_Const or ParamNo */
					if (nodeTag($1) == T_A_Const) {
						((A_Const *)$1)->typename = $3;
					} else if (nodeTag($1) == T_ParamNo) {
						((ParamNo *)$1)->typename = $3;
					/* otherwise, try to transform to a function call */
					} else {
						FuncCall *n = makeNode(FuncCall);
						n->funcname = $3->name;
						n->args = lcons($1,NIL);
						$$ = (Node *)n;
					}
				}
		| CAST '(' b_expr AS Typename ')'
				{
					$$ = (Node *)$3;
					/* AexprConst can be either A_Const or ParamNo */
					if (nodeTag($3) == T_A_Const) {
						((A_Const *)$3)->typename = $5;
					} else if (nodeTag($3) == T_ParamNo) {
						((ParamNo *)$3)->typename = $5;
					/* otherwise, try to transform to a function call */
					} else {
						FuncCall *n = makeNode(FuncCall);
						n->funcname = $5->name;
						n->args = lcons($3,NIL);
						$$ = (Node *)n;
					}
				}
		| '(' a_expr ')'
				{	$$ = $2; }
		| b_expr Op b_expr
				{	$$ = makeA_Expr(OP, $2,$1,$3);	}
		| Op b_expr
				{	$$ = makeA_Expr(OP, $1, NULL, $2); }
		| b_expr Op
				{	$$ = makeA_Expr(OP, $2, $1, NULL); }
		| func_name '(' ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = $1;
					n->args = NIL;
					$$ = (Node *)n;
				}
		| func_name '(' expr_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = $1;
					n->args = $3;
					$$ = (Node *)n;
				}
		| CURRENT_DATE
				{
					A_Const *n = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->val.type = T_String;
					n->val.val.str = "now";
					n->typename = t;

					t->name = xlateSqlType("date");
					t->setof = FALSE;
					t->typmod = -1;

					$$ = (Node *)n;
				}
		| CURRENT_TIME
				{
					A_Const *n = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->val.type = T_String;
					n->val.val.str = "now";
					n->typename = t;

					t->name = xlateSqlType("time");
					t->setof = FALSE;
					t->typmod = -1;

					$$ = (Node *)n;
				}
		| CURRENT_TIME '(' Iconst ')'
				{
					FuncCall *n = makeNode(FuncCall);
					A_Const *s = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->funcname = xlateSqlType("time");
					n->args = lcons(s, NIL);

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = t;

					t->name = xlateSqlType("time");
					t->setof = FALSE;
					t->typmod = -1;

					if ($3 != 0)
						elog(NOTICE,"CURRENT_TIME(%d) precision not implemented; zero used instead",$3);

					$$ = (Node *)n;
				}
		| CURRENT_TIMESTAMP
				{
					A_Const *n = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->val.type = T_String;
					n->val.val.str = "now";
					n->typename = t;

					t->name = xlateSqlType("timestamp");
					t->setof = FALSE;
					t->typmod = -1;

					$$ = (Node *)n;
				}
		| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					FuncCall *n = makeNode(FuncCall);
					A_Const *s = makeNode(A_Const);
					TypeName *t = makeNode(TypeName);

					n->funcname = xlateSqlType("timestamp");
					n->args = lcons(s, NIL);

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = t;

					t->name = xlateSqlType("timestamp");
					t->setof = FALSE;
					t->typmod = -1;

					if ($3 != 0)
						elog(NOTICE,"CURRENT_TIMESTAMP(%d) precision not implemented; zero used instead",$3);

					$$ = (Node *)n;
				}
		| CURRENT_USER
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "getpgusername";
					n->args = NIL;
					$$ = (Node *)n;
				}
		| USER
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "getpgusername";
					n->args = NIL;
					$$ = (Node *)n;
				}
		| POSITION '(' position_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "strpos";
					n->args = $3;
					$$ = (Node *)n;
				}
		| SUBSTRING '(' substr_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "substr";
					n->args = $3;
					$$ = (Node *)n;
				}
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "btrim";
					n->args = $4;
					$$ = (Node *)n;
				}
		| TRIM '(' LEADING trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "ltrim";
					n->args = $4;
					$$ = (Node *)n;
				}
		| TRIM '(' TRAILING trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "rtrim";
					n->args = $4;
					$$ = (Node *)n;
				}
		| TRIM '(' trim_list ')'
				{
					FuncCall *n = makeNode(FuncCall);
					n->funcname = "btrim";
					n->args = $3;
					$$ = (Node *)n;
				}
		;

opt_indirection:  '[' a_expr ']' opt_indirection
				{
					A_Indices *ai = makeNode(A_Indices);
					ai->lidx = NULL;
					ai->uidx = $2;
					$$ = lcons(ai, $4);
				}
		| '[' a_expr ':' a_expr ']' opt_indirection
				{
					A_Indices *ai = makeNode(A_Indices);
					ai->lidx = $2;
					ai->uidx = $4;
					$$ = lcons(ai, $6);
				}
		| /*EMPTY*/
				{	$$ = NIL; }
		;

expr_list:  a_expr_or_null
				{ $$ = lcons($1, NIL); }
		| expr_list ',' a_expr_or_null
				{ $$ = lappend($1, $3); }
		| expr_list USING a_expr
				{ $$ = lappend($1, $3); }
		;

extract_list:  extract_arg FROM a_expr
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = $1;
					$$ = lappend(lcons((Node *)n,NIL), $3);
				}
		| /*EMPTY*/
				{	$$ = NIL; }
		;

extract_arg:  datetime						{ $$ = $1; }
		| TIMEZONE_HOUR						{ $$ = "tz_hour"; }
		| TIMEZONE_MINUTE					{ $$ = "tz_minute"; }
		;

/* position_list uses b_expr not a_expr to avoid conflict with general IN */

position_list:  b_expr IN b_expr
				{	$$ = makeList($3, $1, -1); }
		| /*EMPTY*/
				{	$$ = NIL; }
		;

substr_list:  expr_list substr_from substr_for
				{
					$$ = nconc(nconc($1,$2),$3);
				}
		| /*EMPTY*/
				{	$$ = NIL; }
		;

substr_from:  FROM expr_list
				{	$$ = $2; }
		| /*EMPTY*/
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Integer;
					n->val.val.ival = 1;
					$$ = lcons((Node *)n,NIL);
				}
		;

substr_for:  FOR expr_list
				{	$$ = $2; }
		| /*EMPTY*/
				{	$$ = NIL; }
		;

trim_list:  a_expr FROM expr_list
				{ $$ = lappend($3, $1); }
		| FROM expr_list
				{ $$ = $2; }
		| expr_list
				{ $$ = $1; }
		;

in_expr:  SubSelect
				{
					SubLink *n = makeNode(SubLink);
					n->subselect = $1;
					$$ = (Node *)n;
				}
		| in_expr_nodes
				{	$$ = (Node *)$1; }
		;

in_expr_nodes:  a_expr
				{	$$ = lcons($1, NIL); }
		| in_expr_nodes ',' a_expr
				{	$$ = lappend($1, $3); }
		;

/* Case clause
 * Define SQL92-style case clause.
 * Allow all four forms described in the standard:
 * - Full specification
 *  CASE WHEN a = b THEN c ... ELSE d END
 * - Implicit argument
 *  CASE a WHEN b THEN c ... ELSE d END
 * - Conditional NULL
 *  NULLIF(x,y)
 *  same as CASE WHEN x = y THEN NULL ELSE x END
 * - Conditional substitution from list, use first non-null argument
 *  COALESCE(a,b,...)
 * same as CASE WHEN a IS NOT NULL THEN a WHEN b IS NOT NULL THEN b ... END
 * - thomas 1998-11-09
 */
case_expr:  CASE case_arg when_clause_list case_default END_TRANS
				{
					CaseExpr *c = makeNode(CaseExpr);
					c->arg = $2;
					c->args = $3;
					c->defresult = $4;
					$$ = (Node *)c;
				}
		| NULLIF '(' a_expr ',' a_expr ')'
				{
					CaseExpr *c = makeNode(CaseExpr);
					CaseWhen *w = makeNode(CaseWhen);
/*
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Null;
					w->result = (Node *)n;
*/
					w->expr = makeA_Expr(OP, "=", $3, $5);
					c->args = lcons(w, NIL);
					c->defresult = $3;
					$$ = (Node *)c;
				}
		| COALESCE '(' expr_list ')'
				{
					CaseExpr *c = makeNode(CaseExpr);
					CaseWhen *w;
					List *l;
					foreach (l,$3)
					{
						w = makeNode(CaseWhen);
						w->expr = makeA_Expr(NOTNULL, NULL, lfirst(l), NULL);
						w->result = lfirst(l);
						c->args = lappend(c->args, w);
					}
					$$ = (Node *)c;
				}
		;

when_clause_list:  when_clause_list when_clause
				{ $$ = lappend($1, $2); }
		| when_clause
				{ $$ = lcons($1, NIL); }
		;

when_clause:  WHEN a_expr THEN a_expr_or_null
				{
					CaseWhen *w = makeNode(CaseWhen);
					w->expr = $2;
					w->result = $4;
					$$ = (Node *)w;
				}
		;

case_default:  ELSE a_expr_or_null				{ $$ = $2; }
		| /*EMPTY*/								{ $$ = NULL; }
		;

case_arg:  a_expr
				{	$$ = $1; }
		| /*EMPTY*/
				{	$$ = NULL; }
		;

attr:  relation_name '.' attrs opt_indirection
				{
					$$ = makeNode(Attr);
					$$->relname = $1;
					$$->paramNo = NULL;
					$$->attrs = $3;
					$$->indirection = $4;
				}
		| ParamNo '.' attrs opt_indirection
				{
					$$ = makeNode(Attr);
					$$->relname = NULL;
					$$->paramNo = $1;
					$$->attrs = $3;
					$$->indirection = $4;
				}
		;

attrs:	  attr_name
				{ $$ = lcons(makeString($1), NIL); }
		| attrs '.' attr_name
				{ $$ = lappend($1, makeString($3)); }
		| attrs '.' '*'
				{ $$ = lappend($1, makeString("*")); }
		;


/*****************************************************************************
 *
 *	target lists
 *
 *****************************************************************************/

/* Target lists as found in SELECT ... and INSERT VALUES ( ... ) */

target_list:  target_list ',' target_el
				{	$$ = lappend($1, $3);  }
		| target_el
				{	$$ = lcons($1, NIL);  }
		;

/* AS is not optional because shift/red conflict with unary ops */
target_el:  a_expr_or_null AS ColLabel
				{
					$$ = makeNode(ResTarget);
					$$->name = $3;
					$$->indirection = NULL;
					$$->val = (Node *)$1;
				}
		| a_expr_or_null
				{
					$$ = makeNode(ResTarget);
					$$->name = NULL;
					$$->indirection = NULL;
					$$->val = (Node *)$1;
				}
		| relation_name '.' '*'
				{
					Attr *att = makeNode(Attr);
					att->relname = $1;
					att->paramNo = NULL;
					att->attrs = lcons(makeString("*"), NIL);
					att->indirection = NIL;
					$$ = makeNode(ResTarget);
					$$->name = NULL;
					$$->indirection = NULL;
					$$->val = (Node *)att;
				}
		| '*'
				{
					Attr *att = makeNode(Attr);
					att->relname = "*";
					att->paramNo = NULL;
					att->attrs = NULL;
					att->indirection = NIL;
					$$ = makeNode(ResTarget);
					$$->name = NULL;
					$$->indirection = NULL;
					$$->val = (Node *)att;
				}
		;

/* Target list as found in UPDATE table SET ... */

update_target_list:  update_target_list ',' update_target_el
				{	$$ = lappend($1,$3);  }
		| update_target_el
				{	$$ = lcons($1, NIL);  }
		;

update_target_el:  ColId opt_indirection '=' a_expr_or_null
				{
					$$ = makeNode(ResTarget);
					$$->name = $1;
					$$->indirection = $2;
					$$->val = (Node *)$4;
				}
		;

/*****************************************************************************
 *
 *	Names and constants
 *
 *****************************************************************************/

relation_name:	SpecialRuleRelation
				{
					$$ = $1;
					StrNCpy(saved_relname, $1, NAMEDATALEN);
				}
		| ColId
				{
					/* disallow refs to variable system tables */
					if (strcmp(LogRelationName, $1) == 0
					   || strcmp(VariableRelationName, $1) == 0)
						elog(ERROR,"%s cannot be accessed by users",$1);
					else
						$$ = $1;
					StrNCpy(saved_relname, $1, NAMEDATALEN);
				}
		;

database_name:			ColId			{ $$ = $1; };
access_method:			IDENT			{ $$ = $1; };
attr_name:				ColId			{ $$ = $1; };
class:					IDENT			{ $$ = $1; };
index_name:				ColId			{ $$ = $1; };

/* Functions
 * Include date/time keywords as SQL92 extension.
 * Include TYPE as a SQL92 unreserved keyword. - thomas 1997-10-05
 */
name:					ColId			{ $$ = $1; };
func_name:				ColId			{ $$ = xlateSqlFunc($1); };

file_name:				Sconst			{ $$ = $1; };
/* NOT USED recipe_name:			IDENT			{ $$ = $1; };*/

/* Constants
 * Include TRUE/FALSE for SQL3 support. - thomas 1997-10-24
 */
AexprConst:  Iconst
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Integer;
					n->val.val.ival = $1;
					$$ = (Node *)n;
				}
		| FCONST
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Float;
					n->val.val.dval = $1;
					$$ = (Node *)n;
				}
		| Sconst
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = $1;
					$$ = (Node *)n;
				}
		/* this rule formerly used Typename, but that causes reduce conflicts
		 * with subscripted column names ...
		 */
		| SimpleTypename Sconst
				{
					A_Const *n = makeNode(A_Const);
					n->typename = $1;
					n->val.type = T_String;
					n->val.val.str = $2;
					$$ = (Node *)n;
				}
		| ParamNo
				{	$$ = (Node *)$1;  }
		| TRUE_P
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = "t";
					n->typename = makeNode(TypeName);
					n->typename->name = xlateSqlType("bool");
					n->typename->typmod = -1;
					$$ = (Node *)n;
				}
		| FALSE_P
				{
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = "f";
					n->typename = makeNode(TypeName);
					n->typename->name = xlateSqlType("bool");
					n->typename->typmod = -1;
					$$ = (Node *)n;
				}
		;

ParamNo:  PARAM opt_indirection
				{
					$$ = makeNode(ParamNo);
					$$->number = $1;
					$$->indirection = $2;
				}
		;

Iconst:  ICONST							{ $$ = $1; };
Sconst:  SCONST							{ $$ = $1; };
UserId:  IDENT							{ $$ = $1; };

/* Column and type identifier
 * Does not include explicit datetime types
 *  since these must be decoupled in Typename syntax.
 * Use ColId for most identifiers. - thomas 1997-10-21
 */
TypeId:  ColId
			{	$$ = xlateSqlType($1); }
		| numeric
			{	$$ = xlateSqlType($1); }
		| character
			{	$$ = xlateSqlType($1); }
		;
/* Column identifier
 * Include date/time keywords as SQL92 extension.
 * Include TYPE as a SQL92 unreserved keyword. - thomas 1997-10-05
 * Add other keywords. Note that as the syntax expands,
 *  some of these keywords will have to be removed from this
 *  list due to shift/reduce conflicts in yacc. If so, move
 *  down to the ColLabel entity. - thomas 1997-11-06
 */
ColId:  IDENT							{ $$ = $1; }
		| datetime						{ $$ = $1; }
		| ABSOLUTE						{ $$ = "absolute"; }
		| ACCESS						{ $$ = "access"; }
		| ACTION						{ $$ = "action"; }
		| AFTER							{ $$ = "after"; }
		| AGGREGATE						{ $$ = "aggregate"; }
		| BACKWARD						{ $$ = "backward"; }
		| BEFORE						{ $$ = "before"; }
		| CACHE							{ $$ = "cache"; }
		| COMMITTED						{ $$ = "committed"; }
		| CONSTRAINTS					{ $$ = "constraints"; }
		| CREATEDB						{ $$ = "createdb"; }
		| CREATEUSER					{ $$ = "createuser"; }
		| CYCLE							{ $$ = "cycle"; }
		| DATABASE						{ $$ = "database"; }
		| DEFERRABLE					{ $$ = "deferrable"; }
		| DEFERRED						{ $$ = "deferred"; }
		| DELIMITERS					{ $$ = "delimiters"; }
		| DOUBLE						{ $$ = "double"; }
		| EACH							{ $$ = "each"; }
		| ENCODING						{ $$ = "encoding"; }
		| EXCLUSIVE						{ $$ = "exclusive"; }
		| FORWARD						{ $$ = "forward"; }
		| FUNCTION						{ $$ = "function"; }
		| HANDLER						{ $$ = "handler"; }
		| IMMEDIATE						{ $$ = "immediate"; }
		| INCREMENT						{ $$ = "increment"; }
		| INDEX							{ $$ = "index"; }
		| INHERITS						{ $$ = "inherits"; }
		| INITIALLY						{ $$ = "initially"; }
		| INSENSITIVE					{ $$ = "insensitive"; }
		| INSTEAD						{ $$ = "instead"; }
		| ISNULL						{ $$ = "isnull"; }
		| ISOLATION						{ $$ = "isolation"; }
		| KEY							{ $$ = "key"; }
		| LANGUAGE						{ $$ = "language"; }
		| LANCOMPILER					{ $$ = "lancompiler"; }
		| LEVEL							{ $$ = "level"; }
		| LOCATION						{ $$ = "location"; }
		| MATCH							{ $$ = "match"; }
		| MAXVALUE						{ $$ = "maxvalue"; }
		| MINVALUE						{ $$ = "minvalue"; }
		| MODE							{ $$ = "mode"; }
		| NEXT							{ $$ = "next"; }
		| NOCREATEDB					{ $$ = "nocreatedb"; }
		| NOCREATEUSER					{ $$ = "nocreateuser"; }
		| NOTHING						{ $$ = "nothing"; }
		| NOTNULL						{ $$ = "notnull"; }
		| OF							{ $$ = "of"; }
		| OIDS							{ $$ = "oids"; }
		| ONLY							{ $$ = "only"; }
		| OPERATOR						{ $$ = "operator"; }
		| OPTION						{ $$ = "option"; }
		| PASSWORD						{ $$ = "password"; }
		| PENDANT						{ $$ = "pendant"; }
		| PRIOR							{ $$ = "prior"; }
		| PRIVILEGES					{ $$ = "privileges"; }
		| PROCEDURAL					{ $$ = "procedural"; }
		| READ							{ $$ = "read"; }
		| RELATIVE						{ $$ = "relative"; }
		| RENAME						{ $$ = "rename"; }
		| RESTRICT						{ $$ = "restrict"; }
		| RETURNS						{ $$ = "returns"; }
		| ROW							{ $$ = "row"; }
		| RULE							{ $$ = "rule"; }
		| SCROLL						{ $$ = "scroll"; }
		| SEQUENCE						{ $$ = "sequence"; }
		| SERIAL						{ $$ = "serial"; }
		| SERIALIZABLE					{ $$ = "serializable"; }
		| SHARE							{ $$ = "share"; }
		| START							{ $$ = "start"; }
		| STATEMENT						{ $$ = "statement"; }
		| STDIN							{ $$ = "stdin"; }
		| STDOUT						{ $$ = "stdout"; }
		| TIME							{ $$ = "time"; }
		| TIMESTAMP						{ $$ = "timestamp"; }
		| TIMEZONE_HOUR					{ $$ = "timezone_hour"; }
		| TIMEZONE_MINUTE				{ $$ = "timezone_minute"; }
		| TRIGGER						{ $$ = "trigger"; }
		| TRUSTED						{ $$ = "trusted"; }
		| TYPE_P						{ $$ = "type"; }
		| VALID							{ $$ = "valid"; }
		| VERSION						{ $$ = "version"; }
		| ZONE							{ $$ = "zone"; }
		;

/* Column label
 * Allowed labels in "AS" clauses.
 * Include TRUE/FALSE SQL3 reserved words for Postgres backward
 *  compatibility. Cannot allow this for column names since the
 *  syntax would not distinguish between the constant value and
 *  a column name. - thomas 1997-10-24
 * Add other keywords to this list. Note that they appear here
 *  rather than in ColId if there was a shift/reduce conflict
 *  when used as a full identifier. - thomas 1997-11-06
 */
ColLabel:  ColId						{ $$ = $1; }
		| ABORT_TRANS					{ $$ = "abort"; }
		| ANALYZE						{ $$ = "analyze"; }
		| BINARY						{ $$ = "binary"; }
		| CASE							{ $$ = "case"; }
		| CLUSTER						{ $$ = "cluster"; }
		| COALESCE						{ $$ = "coalesce"; }
		| CONSTRAINT					{ $$ = "constraint"; }
		| COPY							{ $$ = "copy"; }
		| CURRENT						{ $$ = "current"; }
		| DO							{ $$ = "do"; }
		| ELSE							{ $$ = "else"; }
		| END_TRANS						{ $$ = "end"; }
		| EXPLAIN						{ $$ = "explain"; }
		| EXTEND						{ $$ = "extend"; }
		| FALSE_P						{ $$ = "false"; }
		| FOREIGN						{ $$ = "foreign"; }
		| GLOBAL						{ $$ = "global"; }
		| GROUP							{ $$ = "group"; }
		| LISTEN						{ $$ = "listen"; }
		| LOAD							{ $$ = "load"; }
		| LOCAL							{ $$ = "local"; }
		| LOCK_P						{ $$ = "lock"; }
		| MOVE							{ $$ = "move"; }
		| NEW							{ $$ = "new"; }
		| NONE							{ $$ = "none"; }
		| NULLIF						{ $$ = "nullif"; }
		| ORDER							{ $$ = "order"; }
		| POSITION						{ $$ = "position"; }
		| PRECISION						{ $$ = "precision"; }
		| RESET							{ $$ = "reset"; }
		| SETOF							{ $$ = "setof"; }
		| SHOW							{ $$ = "show"; }
		| TABLE							{ $$ = "table"; }
		| THEN							{ $$ = "then"; }
		| TRANSACTION					{ $$ = "transaction"; }
		| TRUE_P						{ $$ = "true"; }
		| VACUUM						{ $$ = "vacuum"; }
		| VERBOSE						{ $$ = "verbose"; }
		| WHEN							{ $$ = "when"; }
		;

SpecialRuleRelation:  CURRENT
				{
					if (QueryIsRule)
						$$ = "*CURRENT*";
					else
						elog(ERROR,"CURRENT used in non-rule query");
				}
		| NEW
				{
					if (QueryIsRule)
						$$ = "*NEW*";
					else
						elog(ERROR,"NEW used in non-rule query");
				}
		;

%%

static Node *
makeA_Expr(int oper, char *opname, Node *lexpr, Node *rexpr)
{
	A_Expr *a = makeNode(A_Expr);
	a->oper = oper;
	a->opname = opname;
	a->lexpr = lexpr;
	a->rexpr = rexpr;
	return (Node *)a;
}

/* makeRowExpr()
 * Generate separate operator nodes for a single row descriptor expression.
 * Perhaps this should go deeper in the parser someday...
 * - thomas 1997-12-22
 */
static Node *
makeRowExpr(char *opr, List *largs, List *rargs)
{
	Node *expr = NULL;
	Node *larg, *rarg;

	if (length(largs) != length(rargs))
		elog(ERROR,"Unequal number of entries in row expression");

	if (lnext(largs) != NIL)
		expr = makeRowExpr(opr,lnext(largs),lnext(rargs));

	larg = lfirst(largs);
	rarg = lfirst(rargs);

	if ((strcmp(opr, "=") == 0)
	 || (strcmp(opr, "<") == 0)
	 || (strcmp(opr, "<=") == 0)
	 || (strcmp(opr, ">") == 0)
	 || (strcmp(opr, ">=") == 0))
	{
		if (expr == NULL)
			expr = makeA_Expr(OP, opr, larg, rarg);
		else
			expr = makeA_Expr(AND, NULL, expr, makeA_Expr(OP, opr, larg, rarg));
	}
	else if (strcmp(opr, "<>") == 0)
	{
		if (expr == NULL)
			expr = makeA_Expr(OP, opr, larg, rarg);
		else
			expr = makeA_Expr(OR, NULL, expr, makeA_Expr(OP, opr, larg, rarg));
	}
	else
	{
		elog(ERROR,"Operator '%s' not implemented for row expressions",opr);
	}

	return expr;
}

static void
mapTargetColumns(List *src, List *dst)
{
	ColumnDef *s;
	ResTarget *d;

	if (length(src) != length(dst))
		elog(ERROR,"CREATE TABLE/AS SELECT has mismatched column count");

	while ((src != NIL) && (dst != NIL))
	{
		s = (ColumnDef *)lfirst(src);
		d = (ResTarget *)lfirst(dst);

		d->name = s->colname;

		src = lnext(src);
		dst = lnext(dst);
	}

	return;
} /* mapTargetColumns() */


/* xlateSqlFunc()
 * Convert alternate type names to internal Postgres types.
 * Do not convert "float", since that is handled elsewhere
 *  for FLOAT(p) syntax.
 */
static char *
xlateSqlFunc(char *name)
{
	if (!strcasecmp(name,"character_length")
	 || !strcasecmp(name,"char_length"))
		return "length";
	else
		return name;
} /* xlateSqlFunc() */

/* xlateSqlType()
 * Convert alternate type names to internal Postgres types.
 */
static char *
xlateSqlType(char *name)
{
	if (!strcasecmp(name,"int")
	 || !strcasecmp(name,"integer"))
		return "int4";
	else if (!strcasecmp(name, "smallint"))
		return "int2";
	else if (!strcasecmp(name, "real")
	 || !strcasecmp(name, "float"))
		return "float8";
	else if (!strcasecmp(name, "interval"))
		return "timespan";
	else if (!strcasecmp(name, "boolean"))
		return "bool";
	else
		return name;
} /* xlateSqlType() */


void parser_init(Oid *typev, int nargs)
{
	QueryIsRule = FALSE;
	saved_relname[0]= '\0';

	param_type_init(typev, nargs);
}


/* FlattenStringList()
 * Traverse list of string nodes and convert to a single string.
 * Used for reconstructing string form of complex expressions.
 *
 * Allocate at least one byte for terminator.
 */
static char *
FlattenStringList(List *list)
{
	List *l;
	Value *v;
	char *s;
	char *sp;
	int nlist, len = 0;

	nlist = length(list);
	l = list;
	while(l != NIL) {
		v = (Value *)lfirst(l);
		sp = v->val.str;
		l = lnext(l);
		len += strlen(sp);
	};
	len += nlist;

	s = (char*) palloc(len+1);
	*s = '\0';

	l = list;
	while(l != NIL) {
		v = (Value *)lfirst(l);
		sp = v->val.str;
		l = lnext(l);
		strcat(s,sp);
		if (l != NIL) strcat(s," ");
	};
	*(s+len) = '\0';

	return s;
} /* FlattenStringList() */


/* makeConstantList()
 * Convert constant value node into string node.
 */
static List *
makeConstantList( A_Const *n)
{
	List *result = NIL;
	char *typval = NULL;
	char *defval = NULL;

	if (nodeTag(n) != T_A_Const) {
		elog(ERROR,"Cannot handle non-constant parameter");

	} else if (n->val.type == T_Float) {
		defval = (char*) palloc(20+1);
		sprintf( defval, "%g", n->val.val.dval);
		result = lcons( makeString(defval), NIL);

	} else if (n->val.type == T_Integer) {
		defval = (char*) palloc(20+1);
		sprintf( defval, "%ld", n->val.val.ival);
		result = lcons( makeString(defval), NIL);

	} else if (n->val.type == T_String) {
		defval = (char*) palloc(strlen( ((A_Const *) n)->val.val.str) + 3);
		strcpy( defval, "'");
		strcat( defval, ((A_Const *) n)->val.val.str);
		strcat( defval, "'");
		if (n->typename != NULL)
		{
			typval = (char*) palloc(strlen( n->typename->name) + 1);
			strcpy(typval, n->typename->name);
			result = lappend( lcons( makeString(typval), NIL), makeString(defval));
		}
		else
		{
			result = lcons( makeString(defval), NIL);
		}

	} else {
		elog(ERROR,"Internal error in makeConstantList(): cannot encode node");
	};

	return result;
} /* makeConstantList() */


/* fmtId()
 * Check input string for non-lowercase/non-numeric characters.
 * Returns either input string or input surrounded by double quotes.
 */
static char *
fmtId(char *rawid)
{
	static char *cp;

	for (cp = rawid; *cp != '\0'; cp++)
		if (! (islower(*cp) || isdigit(*cp) || (*cp == '_'))) break;

	if (*cp != '\0') {
		cp = palloc(strlen(rawid)+3);
		strcpy(cp,"\"");
		strcat(cp,rawid);
		strcat(cp,"\"");
	} else {
		cp = rawid;
	};

	return cp;
}

/*
 * param_type_init()
 *
 * keep enough information around fill out the type of param nodes
 * used in postquel functions
 */
static void
param_type_init(Oid *typev, int nargs)
{
	pfunc_num_args = nargs;
	param_type_info = typev;
}

Oid param_type(int t)
{
	if ((t > pfunc_num_args) || (t == 0))
		return InvalidOid;
	return param_type_info[t - 1];
}

/*
 *	The optimizer doesn't like '-' 4 for index use.  It only checks for
 *	Var '=' Const.  It wants an integer of -4, so we try to merge the
 *	minus into the constant.
 */
static Node *doNegate(Node *n)
{
	if (IsA(n, A_Const))
	{
		A_Const *con = (A_Const *)n;

		if (con->val.type == T_Integer)
		{
			con->val.val.ival = -con->val.val.ival;
			return n;
		}
		if (con->val.type == T_Float)
		{
			con->val.val.dval = -con->val.val.dval;
			return n;
		}
	}

	return makeA_Expr(OP, "-", NULL, n);
}
