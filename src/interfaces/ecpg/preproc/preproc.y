/* $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/interfaces/ecpg/preproc/Attic/preproc.y,v 1.217 2003-05-14 14:37:35 meskes Exp $ */

/* Copyright comment */
%{
#include "postgres_fe.h"

#include "extern.h"

/*
 * Variables containing simple states.
 */
int struct_level = 0;
int braces_open; /* brace level counter */
char	errortext[128];
char	*connection = NULL;
char	*input_filename = NULL;

static int	QueryIsRule = 0, FoundInto = 0;
static int	initializer = 0;
static struct this_type actual_type[STRUCT_DEPTH];
static char *actual_storage[STRUCT_DEPTH];
static char *actual_startline[STRUCT_DEPTH];

/* temporarily store struct members while creating the data structure */
struct ECPGstruct_member *struct_member_list[STRUCT_DEPTH] = { NULL };

/* also store struct type so we can do a sizeof() later */
static char *ECPGstruct_sizeof = NULL;

struct ECPGtype ecpg_no_indicator = {ECPGt_NO_INDICATOR, 0L, NULL, {NULL}};
struct variable no_indicator = {"no_indicator", &ecpg_no_indicator, 0, NULL};

struct ECPGtype ecpg_query = {ECPGt_char_variable, 0L, NULL, {NULL}};

/*
 * Handle parsing errors and warnings
 */
void
mmerror(int error_code, enum errortype type, char * error)
{
	switch(type)
	{
		case ET_WARNING:
			fprintf(stderr, "%s:%d: WARNING: %s\n", input_filename, yylineno, error);
			break;
		case ET_ERROR:
			fprintf(stderr, "%s:%d: ERROR: %s\n", input_filename, yylineno, error);
			ret_value = error_code;
			break;
		case ET_FATAL:
			fprintf(stderr, "%s:%d: ERROR: %s\n", input_filename, yylineno, error);
			exit(error_code);
	}
}

/*
 * string concatenation
 */

static char *
cat2_str(char *str1, char *str2)
{
	char * res_str	= (char *)mm_alloc(strlen(str1) + strlen(str2) + 2);

	strcpy(res_str, str1);
	strcat(res_str, " ");
	strcat(res_str, str2);
	free(str1);
	free(str2);
	return(res_str);
}

static char *
cat_str(int count, ...)
{
	va_list		args;
	int			i;
	char		*res_str;

	va_start(args, count);

	res_str = va_arg(args, char *);

	/* now add all other strings */
	for (i = 1; i < count; i++)
		res_str = cat2_str(res_str, va_arg(args, char *));

	va_end(args);

	return(res_str);
}

char *
make_str(const char *str)
{
	char * res_str = (char *)mm_alloc(strlen(str) + 1);

	strcpy(res_str, str);
	return res_str;
}

static char *
make2_str(char *str1, char *str2)
{
	char * res_str	= (char *)mm_alloc(strlen(str1) + strlen(str2) + 1);

	strcpy(res_str, str1);
	strcat(res_str, str2);
	free(str1);
	free(str2);
	return(res_str);
}

static char *
make3_str(char *str1, char *str2, char *str3)
{
	char * res_str	= (char *)mm_alloc(strlen(str1) + strlen(str2) +strlen(str3) + 1);

	strcpy(res_str, str1);
	strcat(res_str, str2);
	strcat(res_str, str3);
	free(str1);
	free(str2);
	free(str3);
	return(res_str);
}

static char *
make_name(void)
{
	char * name = (char *)mm_alloc(yyleng + 1);

	strncpy(name, yytext, yyleng);
	name[yyleng] = '\0';
	return(name);
}

%}

%union {
	double	dval;
	char	*str;
	int     ival;
	struct	when		action;
	struct	index		index;
	int		tagname;
	struct	this_type	type;
	enum	ECPGttype	type_enum;
	enum	ECPGdtype	dtype_enum;
	struct	fetch_desc	descriptor;
}

/* special embedded SQL token */
%token	SQL_ALLOCATE SQL_AUTOCOMMIT SQL_BOOL SQL_BREAK
		SQL_CALL SQL_CARDINALITY SQL_CONNECT SQL_CONNECTION
		SQL_CONTINUE SQL_COUNT SQL_CURRENT SQL_DATA 
		SQL_DATETIME_INTERVAL_CODE
		SQL_DATETIME_INTERVAL_PRECISION
		SQL_DESCRIPTOR SQL_DISCONNECT SQL_ENUM SQL_FOUND
		SQL_FREE SQL_GO SQL_GOTO SQL_IDENTIFIED
		SQL_INDICATOR SQL_KEY_MEMBER SQL_LENGTH
		SQL_LONG SQL_NAME SQL_NULLABLE SQL_OCTET_LENGTH
		SQL_OPEN SQL_RELEASE SQL_REFERENCE
		SQL_RETURNED_LENGTH SQL_RETURNED_OCTET_LENGTH SQL_SCALE
		SQL_SECTION SQL_SHORT SQL_SIGNED SQL_SQL SQL_SQLERROR
		SQL_SQLPRINT SQL_SQLWARNING SQL_START SQL_STOP
		SQL_STRUCT SQL_UNSIGNED SQL_VALUE SQL_VAR SQL_WHENEVER

/* C token */
%token	S_ADD S_AND S_ANYTHING S_AUTO S_CONST S_DEC S_DIV
		S_DOTPOINT S_EQUAL S_EXTERN S_INC S_LSHIFT S_MEMPOINT
		S_MEMBER S_MOD S_MUL S_NEQUAL S_OR S_REGISTER S_RSHIFT
		S_STATIC S_SUB S_VOLATILE
		S_TYPEDEF

/* I need this and don't know where it is defined inside the backend */
%token	TYPECAST

/* ordinary key words in alphabetical order */
%token <keyword> ABORT_P ABSOLUTE ACCESS ACTION ADD AFTER
        AGGREGATE ALL ALTER ANALYSE ANALYZE AND ANY AS ASC
	ASSERTION ASSIGNMENT AT AUTHORIZATION

        BACKWARD BEFORE BEGIN_P BETWEEN BIGINT BINARY BIT
        BOOLEAN BOTH BY

        CACHE CALLED CASCADE CASE CAST CHAIN CHAR_P
	CHARACTER CHARACTERISTICS CHECK CHECKPOINT CLASS CLOSE
	CLUSTER COALESCE COLLATE COLUMN COMMENT COMMIT
	COMMITTED CONSTRAINT CONSTRAINTS CONVERSION_P CONVERT COPY
        CREATE CREATEDB CREATEUSER CROSS CURRENT_DATE CURRENT_TIME
        CURRENT_TIMESTAMP CURRENT_USER CURSOR CYCLE

        DATABASE DAY_P DEALLOCATE DEC DECIMAL DECLARE DEFAULT
	DEFERRABLE DEFERRED DEFINER DELETE_P DELIMITER DELIMITERS
	DESC DISTINCT DO DOMAIN_P DOUBLE DROP
        EACH ELSE ENCODING ENCRYPTED END_P ESCAPE EXCEPT EXCLUSIVE
        EXECUTE EXISTS EXPLAIN EXTERNAL EXTRACT

        FALSE_P FETCH FLOAT_P FOR FORCE FOREIGN FORWARD FREEZE FROM
        FULL FUNCTION

	GET GLOBAL GRANT GROUP_P
        HANDLER HAVING HOUR_P

	ILIKE IMMEDIATE IMMUTABLE IMPLICIT_P IN_P INCREMENT INDEX INHERITS
        INITIALLY INNER_P INOUT INPUT INSENSITIVE INSERT INSTEAD INT
        INTEGER INTERSECT INTERVAL INTO INVOKER IS ISNULL ISOLATION

        JOIN

        KEY

	LANCOMPILER LANGUAGE LEADING LEFT LEVEL LIKE LIMIT LISTEN
        LOAD LOCAL LOCATION LOCK_P

	MATCH MAXVALUE MINUTE_P MINVALUE MODE MONTH_P MOVE

	NAMES NATIONAL NATURAL NCHAR NEW NEXT NO NOCREATEDB
        NOCREATEUSER NONE NOT NOTHING NOTIFY NOTNULL NULL_P NULLIF
        NUMERIC

	OF OFF OFFSET OIDS OLD ON ONLY OPERATOR OPTION OR ORDER
        OUT_P OUTER_P OVERLAPS OVERLAY OWNER

	PARTIAL PASSWORD PATH_P PENDANT PLACING POSITION
	PRECISION PRESERVE PREPARE PRIMARY PRIOR PRIVILEGES PROCEDURAL PROCEDURE

	READ REAL RECHECK REFERENCES REINDEX RELATIVE RENAME REPLACE
	RESET RESTRICT RETURNS REVOKE RIGHT ROLLBACK ROW ROWS RULE

	SCHEMA SCROLL SECOND_P SECURITY SELECT SEQUENCE SERIALIZABLE
        SESSION SESSION_USER SET SETOF SHARE SHOW SIMILAR SIMPLE SMALLINT SOME
        STABLE START STATEMENT STATISTICS STDIN STDOUT STORAGE STRICT
        SUBSTRING SYSID

        TABLE TEMP TEMPLATE TEMPORARY THEN TIME TIMESTAMP TO TOAST
        TRAILING TRANSACTION TREAT TRIGGER TRIM TRUE_P TRUNCATE TRUSTED TYPE_P
        UNENCRYPTED UNION UNIQUE UNKNOWN UNLISTEN UNTIL UPDATE USAGE
        USER USING

        VACUUM VALID VALUE VALUES VARCHAR VARYING VERBOSE VERSION VIEW VOLATILE
	WHEN WHERE WITH WITHOUT WORK WRITE
        YEAR_P
        ZONE

/* The grammar thinks these are keywords, but they are not in the keywords.c
 * list and so can never be entered directly.  The filter in parser.c
 * creates these tokens when required.
 */
%token	UNIONJOIN

/* Special keywords, not in the query language - see the "lex" file */
%token <str>	IDENT SCONST Op CSTRING CVARIABLE CPP_LINE IP BCONST XCONST
%token <ival>	ICONST PARAM
%token <dval>	FCONST

/* precedence: lowest to highest */
%left		UNION EXCEPT
%left		INTERSECT
%left		OR
%left		AND
%right		NOT
%right		'='
%nonassoc	'<' '>'
%nonassoc	LIKE ILIKE SIMILAR
%nonassoc	ESCAPE
%nonassoc	OVERLAPS
%nonassoc	BETWEEN
%nonassoc	IN_P
%left		POSTFIXOP					/* dummy for postfix Op rules */
%left		Op OPERATOR				/* multi-character ops and user-defined operators */
%nonassoc	NOTNULL
%nonassoc	ISNULL
%nonassoc	IS NULL_P TRUE_P FALSE_P UNKNOWN
%left		'+' '-'
%left		'*' '/' '%'
%left		'^'
/* Unary Operators */
%left		AT ZONE
%right		UMINUS
%left		'[' ']'
%left		'(' ')'
%left		TYPECAST
%left		'.'
%left		JOIN UNIONJOIN CROSS LEFT FULL RIGHT INNER_P NATURAL

%type  <str>	Iconst Fconst Sconst TransactionStmt CreateStmt UserId
%type  <str>	CreateAsElement OptCreateAs CreateAsList CreateAsStmt
%type  <str>	comment_text ConstraintDeferrabilitySpec TableElementList
%type  <str>	key_match ColLabel SpecialRuleRelation ColId columnDef
%type  <str>	ColConstraint ColConstraintElem drop_type Bconst Iresult
%type  <str>	TableConstraint OptTableElementList Xconst opt_transaction 
%type  <str>	ConstraintElem key_actions ColQualList type_name
%type  <str>	target_list target_el update_target_list alias_clause
%type  <str>	update_target_el opt_id qualified_name database_name
%type  <str>	access_method attr_name index_name name func_name
%type  <str>	file_name AexprConst c_expr ConstTypename var_list
%type  <str>	a_expr b_expr TruncateStmt CommentStmt OnCommitOption opt_by
%type  <str>	opt_indirection expr_list extract_list extract_arg
%type  <str>	position_list substr_list substr_from alter_column_default
%type  <str>	trim_list in_expr substr_for attrs TableFuncElement
%type  <str>	Typename SimpleTypename Numeric opt_float opt_numeric
%type  <str>	opt_decimal Character character opt_varying opt_charset
%type  <str>	opt_collate opt_timezone opt_interval table_ref
%type  <str>	row_descriptor ConstDatetime AlterDomainStmt 
%type  <str>	SelectStmt into_clause OptTemp ConstraintAttributeSpec
%type  <str>	opt_table opt_all sort_clause sortby_list ConstraintAttr
%type  <str>	sortby OptUseOp qualified_name_list name_list ColId_or_Sconst
%type  <str>	group_clause having_clause from_clause opt_distinct
%type  <str>	join_outer where_clause relation_expr sub_type opt_arg
%type  <str>	opt_column_list insert_rest InsertStmt OptimizableStmt
%type  <str>	columnList DeleteStmt LockStmt UpdateStmt CursorStmt
%type  <str>	NotifyStmt columnElem UnlistenStmt TableElement rowdefinition
%type  <str>	copy_delimiter ListenStmt CopyStmt copy_file_name opt_binary
%type  <str>	FetchStmt direction fetch_how_many from_in CreateOpClassStmt
%type  <str>	ClosePortalStmt DropStmt VacuumStmt AnalyzeStmt opt_verbose
%type  <str>	opt_full func_arg OptWithOids opt_freeze opt_ecpg_into
%type  <str>	analyze_keyword opt_name_list ExplainStmt index_params
%type  <str>	index_list func_index index_elem opt_class access_method_clause
%type  <str>	index_opt_unique IndexStmt func_return ConstInterval
%type  <str>	func_args_list func_args opt_with def_arg overlay_placing
%type  <str>	def_elem def_list definition DefineStmt select_with_parens
%type  <str>	opt_instead event RuleActionList opt_using CreateAssertStmt
%type  <str>	RuleActionStmtOrEmpty RuleActionMulti func_as reindex_type
%type  <str>	RuleStmt opt_column opt_name oper_argtypes NumConst
%type  <str>	MathOp RemoveFuncStmt aggr_argtype for_update_clause
%type  <str>	RemoveAggrStmt opt_procedural select_no_parens CreateCastStmt
%type  <str>	RemoveOperStmt RenameStmt all_Op opt_Trusted opt_lancompiler
%type  <str>	VariableSetStmt var_value zone_value VariableShowStmt
%type  <str>	VariableResetStmt AlterTableStmt from_list overlay_list
%type  <str>	user_list OptUserList OptUserElem relation_name
%type  <str>	CreateUserStmt AlterUserStmt CreateSeqStmt OptSeqList
%type  <str>	OptSeqElem TriggerForSpec TriggerForOpt TriggerForType
%type  <str>	DropTrigStmt TriggerOneEvent TriggerEvents RuleActionStmt
%type  <str>	TriggerActionTime CreateTrigStmt DropPLangStmt DropCastStmt
%type  <str>	CreatePLangStmt TriggerFuncArgs TriggerFuncArg simple_select
%type  <str>	ViewStmt LoadStmt CreatedbStmt createdb_opt_item
%type  <str>	createdb_opt_list opt_encoding OptInherit opt_equal
%type  <str>	AlterUserSetStmt privilege_list privilege privilege_target
%type  <str>	opt_grant_grant_option opt_revoke_grant_option
%type  <str>	transaction_mode_list_or_empty transaction_mode_list
%type  <str>	function_with_argtypes_list function_with_argtypes
%type  <str>	DropdbStmt ClusterStmt grantee RevokeStmt Bit DropOpClassStmt
%type  <str>	GrantStmt privileges PosAllConst constraints_set_list
%type  <str>	opt_cursor ConstraintsSetStmt AllConst CreateDomainStmt
%type  <str>	case_expr when_clause_list case_default case_arg when_clause
%type  <str>	select_clause opt_select_limit select_limit_value opt_recheck
%type  <str>	ConstraintTimeSpec AlterDatabaseSetStmt DropAssertStmt
%type  <str>	select_offset_value ReindexStmt join_type opt_boolean
%type  <str>	join_qual update_list joined_table opclass_item
%type  <str>	opt_lock lock_type OptGroupList OptGroupElem
%type  <str>	OptConstrFromTable OptTempTableName StringConst
%type  <str>	constraints_set_mode comment_type opt_empty_parentheses
%type  <str>	CreateGroupStmt AlterGroupStmt DropGroupStmt key_delete
%type  <str>	opt_force key_update CreateSchemaStmt PosIntStringConst
%type  <str>	IntConst PosIntConst grantee_list func_type opt_or_replace
%type  <str>	select_limit opt_for_update_clause CheckPointStmt
%type  <str>	OptSchemaName OptSchemaEltList schema_stmt opt_drop_behavior
%type  <str>	handler_name any_name_list any_name opt_as insert_column_list
%type  <str>	columnref dotted_name function_name insert_target_el
%type  <str>	insert_target_list insert_column_item DropRuleStmt
%type  <str>	createfunc_opt_item set_rest var_list_or_default
%type  <str>	CreateFunctionStmt createfunc_opt_list func_table
%type  <str>	DropUserStmt copy_from copy_opt_list copy_opt_item
%type  <str>	opt_oids TableLikeClause key_action opt_definition
%type  <str>	cast_context row r_expr qual_Op qual_all_Op opt_default
%type  <str>	CreateConversionStmt any_operator opclass_item_list
%type  <str>	iso_level type_list CharacterWithLength ConstCharacter
%type  <str>	CharacterWithoutLength BitWithLength BitWithoutLength
%type  <str>	ConstBit GenericType TableFuncElementList opt_analyze
%type  <str>	opt_sort_clause transaction_access_mode

%type  <str>	ECPGWhenever ECPGConnect connection_target ECPGOpen
%type  <str>	indicator ECPGExecute ECPGPrepare opt_ecpg_using ecpg_into
%type  <str>	storage_declaration storage_clause opt_initializer c_anything
%type  <str>	variable_list variable c_thing c_term ECPGKeywords_vanames
%type  <str>	opt_pointer ECPGDisconnect dis_name storage_modifier
%type  <str>	ECPGRelease execstring server_name ECPGVarDeclaration
%type  <str>	connection_object opt_server opt_port c_stuff c_stuff_item
%type  <str>	user_name opt_user char_variable ora_user ident opt_reference
%type  <str>	var_type_declarations quoted_ident_stringvar ECPGKeywords_rest
%type  <str>	db_prefix server opt_options opt_connection_name c_list
%type  <str>	ECPGSetConnection ECPGTypedef c_args ECPGKeywords
%type  <str>	enum_type civar civarind ECPGCursorStmt ECPGDeallocate
%type  <str>	ECPGFree ECPGDeclare ECPGVar opt_at enum_definition
%type  <str>	struct_type s_struct vt_declarations variable_declarations
%type  <str>	var_declaration type_declaration single_vt_declaration
%type  <str>	s_union union_type ECPGSetAutocommit on_off
%type  <str>	ECPGAllocateDescr ECPGDeallocateDescr symbol opt_symbol
%type  <str>	ECPGGetDescriptorHeader ECPGColLabel single_var_declaration
%type  <str>	reserved_keyword unreserved_keyword ecpg_interval
%type  <str>	col_name_keyword func_name_keyword precision opt_scale
%type  <str>	ECPGTypeName variablelist ECPGColLabelCommon

%type  <descriptor> ECPGGetDescriptor

%type  <type_enum> simple_type signed_type unsigned_type

%type  <dtype_enum> descriptor_item desc_header_item

%type  <type>	var_type common_type single_vt_type

%type  <action> action

%type  <index>	opt_array_bounds 

%%
prog: statements;

statements: /*EMPTY*/
		| statements statement
		;

statement: ecpgstart opt_at stmt ';'	{ connection = NULL; }
		| ecpgstart stmt ';'
		| ecpgstart ECPGVarDeclaration
		{
			fprintf(yyout, "%s", $2);
                        free($2);
			output_line_number();
		}
		| ECPGDeclaration
		| c_thing		{ fprintf(yyout, "%s", $1); free($1); }
		| CPP_LINE		{ fprintf(yyout, "%s", $1); free($1); }
		| '{'			{ braces_open++; fputs("{", yyout); }
		| '}'			{ remove_variables(braces_open--); fputs("}", yyout); }
		;

opt_at: AT connection_target
		{
			connection = $2;
			/*
			 *	if we have a variable as connection
			 *	target, remove it from the variable
			 *	list or else it will be used twice
			 */
			if (argsinsert != NULL)
				argsinsert = NULL;
		};

stmt:  AlterDatabaseSetStmt { output_statement($1, 0, connection); }
		| AlterDomainStmt	{ output_statement($1, 0, connection); }
		| AlterGroupStmt	{ output_statement($1, 0, connection); }
		| AlterTableStmt	{ output_statement($1, 0, connection); }
		| AlterUserStmt		{ output_statement($1, 0, connection); }
		| AlterUserSetStmt	{ output_statement($1, 0, connection); }
		| ClosePortalStmt	{ output_statement($1, 0, connection); }
		| CommentStmt		{ output_statement($1, 0, connection); }
		| CopyStmt		{ output_statement($1, 0, connection); }
		| CreateStmt		{ output_statement($1, 0, connection); }
		| CreateAsStmt		{ output_statement($1, 0, connection); }
		| CreateCastStmt	{ output_statement($1, 0, connection); }
		| CreateDomainStmt	{ output_statement($1, 0, connection); }
		| CreateFunctionStmt	{ output_statement($1, 0, connection); }
		| CreateSchemaStmt	{ output_statement($1, 0, connection); }
		| CreateGroupStmt	{ output_statement($1, 0, connection); }
		| CreateSeqStmt		{ output_statement($1, 0, connection); }
		| CreatePLangStmt	{ output_statement($1, 0, connection); }
		| CreateAssertStmt	{ output_statement($1, 0, connection); }
		| CreateOpClassStmt	{ output_statement($1, 0, connection); }
		| CreateTrigStmt	{ output_statement($1, 0, connection); }
		| CreateUserStmt	{ output_statement($1, 0, connection); }
		| ClusterStmt		{ output_statement($1, 0, connection); }
		/*| DeallocateStmt	{ output_statement($1, 0, connection); }*/
		| DefineStmt		{ output_statement($1, 0, connection); }
		| DropStmt		{ output_statement($1, 0, connection); }
		| TruncateStmt		{ output_statement($1, 0, connection); }
		| DropCastStmt		{ output_statement($1, 0, connection); }
		| DropGroupStmt		{ output_statement($1, 0, connection); }
		| DropOpClassStmt	{ output_statement($1, 0, connection); }
		| DropPLangStmt		{ output_statement($1, 0, connection); }
		| DropAssertStmt	{ output_statement($1, 0, connection); }
		| DropTrigStmt		{ output_statement($1, 0, connection); }
		| DropRuleStmt		{ output_statement($1, 0, connection); }
		| DropUserStmt		{ output_statement($1, 0, connection); }
		| ExplainStmt		{ output_statement($1, 0, connection); }/*		| ExecuteStmt		{ output_statement($1, 0, connection); }*/
		| FetchStmt		{ output_statement($1, 1, connection); }
		| GrantStmt		{ output_statement($1, 0, connection); }
		| IndexStmt		{ output_statement($1, 0, connection); }
		| ListenStmt		{ output_statement($1, 0, connection); }
		| UnlistenStmt		{ output_statement($1, 0, connection); }
		| LockStmt		{ output_statement($1, 0, connection); }
		| NotifyStmt		{ output_statement($1, 0, connection); }
/*		| PrepareStmt		{ output_statement($1, 0, connection); }*/
		| ReindexStmt		{ output_statement($1, 0, connection); }
		| RemoveAggrStmt	{ output_statement($1, 0, connection); }
		| RemoveOperStmt	{ output_statement($1, 0, connection); }
		| RemoveFuncStmt	{ output_statement($1, 0, connection); }
		| RenameStmt		{ output_statement($1, 0, connection); }
		| RevokeStmt		{ output_statement($1, 0, connection); }
		| OptimizableStmt
		{
			if (strncmp($1, "/* " , sizeof("/* ")-1) == 0)
				output_simple_statement($1);
			else
				output_statement($1, 1, connection);
		}
		| RuleStmt		{ output_statement($1, 0, connection); }
		| TransactionStmt
		{
			fprintf(yyout, "{ ECPGtrans(__LINE__, %s, \"%s\");", connection ? connection : "NULL", $1);
			whenever_action(2);
			free($1);
		}
		| ViewStmt		{ output_statement($1, 0, connection); }
		| LoadStmt		{ output_statement($1, 0, connection); }
		| CreatedbStmt		{ output_statement($1, 0, connection); }
		| DropdbStmt		{ output_statement($1, 0, connection); }
		| VacuumStmt		{ output_statement($1, 0, connection); }
		| AnalyzeStmt		{ output_statement($1, 0, connection); }
		| VariableSetStmt	{ output_statement($1, 0, connection); }
		| VariableShowStmt	{ output_statement($1, 0, connection); }
		| VariableResetStmt	{ output_statement($1, 0, connection); }
		| ConstraintsSetStmt	{ output_statement($1, 0, connection); }
		| CheckPointStmt	{ output_statement($1, 0, connection); }
		| CreateConversionStmt	{ output_statement($1, 0, connection); }
		| ECPGAllocateDescr
		{
			fprintf(yyout,"ECPGallocate_desc(__LINE__, %s);",$1);
			whenever_action(0);
			free($1);
		}
		| ECPGConnect
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for connect statement.\n");

			if (compat == ECPG_COMPAT_INFORMIX)
				fprintf(yyout, "{ ECPGconnect_informix(__LINE__, %s, %d); ", $1, autocommit);
			else
				fprintf(yyout, "{ ECPGconnect(__LINE__, %s, %d); ", $1, autocommit);
			reset_variables();
			whenever_action(2);
			free($1);
		}
		| ECPGCursorStmt
		{
			output_simple_statement($1);
		}
		| ECPGDeallocate
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for connect statement.\n");

			fputc('{', yyout);
			fputs($1, yyout);
			whenever_action(2);
			free($1);
		}
		| ECPGDeallocateDescr
		{
			fprintf(yyout,"ECPGdeallocate_desc(__LINE__, %s);",$1);
			whenever_action(0);
			free($1);
		}
		| ECPGDeclare
		{
			output_simple_statement($1);
		}
		| ECPGDisconnect
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for disconnect statement.\n");

			fprintf(yyout, "{ ECPGdisconnect(__LINE__, %s);",
					$1 ? $1 : "\"CURRENT\"");
			whenever_action(2);
			free($1);
		}
		| ECPGExecute
		{
			output_statement($1, 0, connection);
		}
		| ECPGFree
		{
			fprintf(yyout, "{ ECPGdeallocate(__LINE__, \"%s\");", $1);

			whenever_action(2);
			free($1);
		}
		| ECPGGetDescriptor
		{
			lookup_descriptor($1.name, connection);
			output_get_descr($1.name, $1.str);
			free($1.name);
			free($1.str);
		}
		| ECPGGetDescriptorHeader
		{
			lookup_descriptor($1, connection);
			output_get_descr_header($1);
			free($1);
		}
		| ECPGOpen
		{
			struct cursor *ptr;
			struct arguments *p;

			for (ptr = cur; ptr != NULL; ptr=ptr->next)
			{
				if (strcmp(ptr->name, $1) == 0)
					break;
			}

			if (ptr == NULL)
			{
				snprintf(errortext, sizeof(errortext), "trying to open undeclared cursor %s\n", $1);
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			/* merge variables given in prepare statement with those given here */
			for (p = ptr->argsinsert; p; p = p->next)
				append_variable(&argsinsert, p->variable, p->indicator);

			for (p = ptr->argsresult; p; p = p->next)
				add_variable(&argsresult, p->variable, p->indicator);

			output_statement(mm_strdup(ptr->command), 0, ptr->connection ? mm_strdup(ptr->connection) : NULL);
		}
		| ECPGPrepare
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for set connection statement.\n");

			fprintf(yyout, "{ ECPGprepare(__LINE__, %s);", $1);
			whenever_action(2);
			free($1);
		}
		| ECPGRelease		{ /* output already done */ }
		| ECPGSetAutocommit
		{
			fprintf(yyout, "{ ECPGsetcommit(__LINE__, \"%s\", %s);", $1, connection ? connection : "NULL");
			whenever_action(2);
			free($1);
		}
		| ECPGSetConnection
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for set connection statement.\n");

			fprintf(yyout, "{ ECPGsetconn(__LINE__, %s);", $1);
			whenever_action(2);
			free($1);
		}
		| ECPGTypedef
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for typedef statement.\n");

			fprintf(yyout, "%s", $1);
			free($1);
			output_line_number();
		}
		| ECPGVar
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for var statement.\n");

			output_simple_statement($1);
		}
		| ECPGWhenever
		{
			if (connection)
				mmerror(PARSE_ERROR, ET_ERROR, "no at option for whenever statement.\n");

			output_simple_statement($1);
		}
		;


/*
 * We start with a lot of stuff that's very similar to the backend's parsing
 */

/*****************************************************************************
 *
 * Create a new Postgres DBMS user
 *
 *
 *****************************************************************************/

CreateUserStmt: CREATE USER UserId opt_with OptUserList
			{ $$ = cat_str(4, make_str("create user"), $3, make_str("with"), $5); }
		;

opt_with:  WITH 		{ $$ = make_str("with"); }
		| /*EMPTY*/	{ $$ = EMPTY; }
		;


/*****************************************************************************
 *
 * Alter a postgresql DBMS user
 *
 *
 *****************************************************************************/

AlterUserStmt: ALTER USER UserId OptUserList
			{ $$ = cat_str(3, make_str("alter user"), $3, $4); }
		| ALTER USER UserId WITH OptUserList
			{ $$ = cat_str(4, make_str("alter user"), $3, make_str("with"), $5); }
		;

AlterUserSetStmt: ALTER USER UserId SET set_rest
			{ $$ = cat_str(4, make_str("alter user"), $3, make_str("set"), $5); }
		| ALTER USER UserId VariableResetStmt
			{ $$ = cat_str(3, make_str("alter user"), $3, $4); }
		;

/*****************************************************************************
 *
 * Drop a postgresql DBMS user
 *
 *
 *****************************************************************************/
DropUserStmt:  DROP USER user_list
			{ $$ = cat2_str(make_str("drop user"), $3);}
		;
/*
 * Options for CREATE USER and ALTER USER
 */

OptUserList: OptUserList OptUserElem	{ $$ = cat2_str($1, $2); }
		| /* EMPTY */					{ $$ = EMPTY; }
		;

OptUserElem:  PASSWORD Sconst
		{ $$ = cat2_str(make_str("password"), $2); }
		| SYSID PosIntConst
			{ $$ = cat2_str(make_str("sysid"), $2); }
		| CREATEDB
			{ $$ = make_str("createdb"); }
		| NOCREATEDB
			{ $$ = make_str("nocreatedb"); }
		| CREATEUSER
			{ $$ = make_str("createuser"); }
		| NOCREATEUSER
			{ $$ = make_str("nocreateuser"); }
		| IN_P GROUP_P user_list
			{ $$ = cat2_str(make_str("in group"), $3); }
		| VALID UNTIL Sconst
			{ $$ = cat2_str(make_str("valid until"), $3); }
		;

user_list:	user_list ',' UserId
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		| UserId
			{ $$ = $1; }
		;

/*****************************************************************************
 *
 * Create a postgresql group
 *
 *
 ****************************************************************************/
CreateGroupStmt:  CREATE GROUP_P UserId OptGroupList
			{ $$ = cat_str(3, make_str("create group"), $3, $4); }
		| CREATE GROUP_P UserId WITH OptGroupList
			{ $$ = cat_str(4, make_str("create group"), $3, make_str("with"), $5); }
		;

/*
 * Options for CREATE GROUP
 */
OptGroupList: OptGroupList OptGroupElem		{ $$ = cat2_str($1, $2); }
		| /* EMPTY */						{ $$ = EMPTY; }
		;

OptGroupElem:  USER user_list
			{ $$ = cat2_str(make_str("user"), $2); }
		| SYSID PosIntConst
			{ $$ = cat2_str(make_str("sysid"), $2); }
		;


/*****************************************************************************
 *
 * Alter a postgresql group
 *
 *
 *****************************************************************************/
AlterGroupStmt: ALTER GROUP_P UserId ADD USER user_list
			{ $$ = cat_str(4, make_str("alter group"), $3, make_str("add user"), $6); }
		| ALTER GROUP_P UserId DROP USER user_list
			{ $$ = cat_str(4, make_str("alter group"), $3, make_str("drop user"), $6); }
		;

/*****************************************************************************
 *
 * Drop a postgresql group
 *
 *
 *****************************************************************************/
DropGroupStmt: DROP GROUP_P UserId
			{ $$ = cat2_str(make_str("drop group"), $3); }
		;

/*****************************************************************************
 *
 * Manipulate a schema
 *
 *
 *****************************************************************************/

CreateSchemaStmt:  CREATE SCHEMA UserId OptSchemaName AUTHORIZATION UserId OptSchemaEltList
			{ $$ = cat_str(6, make_str("create scheme"), $3, $4, make_str("authorization"), $6, $7); }
		| CREATE SCHEMA ColId OptSchemaEltList
			{ $$ = cat_str(3, make_str("create scheme"), $3, $4); }
		;

OptSchemaName: ColId		{ $$ = $1; }
               | /* EMPTY */   { $$ = EMPTY; }
	       ;

OptSchemaEltList: OptSchemaEltList schema_stmt         { $$ = cat2_str($1, $2); }
		| /* EMPTY */   { $$ = EMPTY; }
		;

/*
 *     schema_stmt are the ones that can show up inside a CREATE SCHEMA
 *     statement (in addition to by themselves).
 */
schema_stmt: CreateStmt		{ $$ = $1; }
               | GrantStmt	{ $$ = $1; }
               | ViewStmt	{ $$ = $1; }
               ;



/*****************************************************************************
 *
 * Set PG internal variable
 *	  SET name TO 'var_value'
 * Include SQL92 syntax (thomas 1997-10-22):
 *	  SET TIME ZONE 'var_value'
 *
 *****************************************************************************/
VariableSetStmt:  SET set_rest
			{ $$ = cat2_str(make_str("set"), $2 ); }
		| SET LOCAL set_rest
			{ $$ = cat2_str(make_str("set local"), $3 ); }
		| SET SESSION set_rest
			{ $$ = cat2_str(make_str("set session"), $3 ); }
		;

set_rest:	ColId TO var_list_or_default
			{ $$ = cat_str(3, $1, make_str("to"), $3); }
		| ColId "=" var_list_or_default
                        { $$ = cat_str(3, $1, make_str("="), $3); }
		| TIME ZONE zone_value
			{ $$ = cat2_str(make_str("time zone"), $3); }
		| TRANSACTION transaction_mode_list
			{ $$ = cat2_str(make_str("transaction"), $2); }
		| SESSION CHARACTERISTICS AS TRANSACTION transaction_mode_list
			{ $$ = cat2_str(make_str("session characteristics as transaction"), $5); }
		| NAMES opt_encoding
			{ $$ = cat2_str(make_str("names"), $2); }
		| SESSION AUTHORIZATION ColId_or_Sconst
			{ $$ = cat2_str(make_str("session authorization"), $3); }
		| SESSION AUTHORIZATION DEFAULT
			{ $$ = make_str("session authorization default"); }
		;

var_list_or_default:  var_list
			{ $$ = $1; }
		| DEFAULT
			{ $$ = make_str("default"); }
		;

var_list:  var_value
			{ $$ = $1; }
		| var_list ',' var_value
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

iso_level:	READ COMMITTED	{ $$ = make_str("read committed"); }
		| SERIALIZABLE		{ $$ = make_str("serializable"); }
		;

var_value:	opt_boolean		{ $$ = $1; }
		| AllConst		{ $$ = $1; }
		| ColId			{ $$ = $1; }
		;

opt_boolean:  TRUE_P		{ $$ = make_str("true"); }
		| FALSE_P			{ $$ = make_str("false"); }
		| ON				{ $$ = make_str("on"); }
		| OFF				{ $$ = make_str("off"); }
		;
/* Timezone values can be:
 * - a string such as 'pst8pdt'
 * - a column identifier such as "pst8pdt"
 * - an integer or floating point number
 * - a time interval per SQL99
 * ConstInterval and ColId give shift/reduce errors,
 * so use IDENT and reject anything which is a reserved word.
 */
zone_value:  AllConst		{ $$ = $1; }
		| ident		{ $$ = $1; }
		| ConstInterval StringConst opt_interval
			{ $$ = cat_str(3, $1, $2, $3); }
		| ConstInterval '(' PosIntConst ')' StringConst opt_interval
			{ $$ = cat_str(6, $1, make_str("("), $3, make_str(")"), $5, $6); }
		| DEFAULT
			{ $$ = make_str("default"); }
		| LOCAL
			{ $$ = make_str("local"); }
		;

opt_encoding:	StringConst		{ $$ = $1; }
		| DEFAULT				{ $$ = make_str("default"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

ColId_or_Sconst: ColId			{ $$ = $1; }
		| StringConst			{ $$ = $1; }
		;

VariableShowStmt:  SHOW ColId
			{ $$ = cat2_str(make_str("show"), $2); }
		| SHOW TIME ZONE
			{ $$ = make_str("show time zone"); }
		| SHOW TRANSACTION ISOLATION LEVEL
			{ $$ = make_str("show transaction isolation level"); }
		| SHOW SESSION AUTHORIZATION
			{ $$ = make_str("show session authorization"); }
		| SHOW ALL
			{ $$ = make_str("show all"); }
		;

VariableResetStmt:	RESET ColId
			{ $$ = cat2_str(make_str("reset"), $2); }
		| RESET TIME ZONE
			{ $$ = make_str("reset time zone"); }
		| RESET TRANSACTION ISOLATION LEVEL
			{ $$ = make_str("reset transaction isolation level"); }
		| RESET SESSION AUTHORIZATION
			{ $$ = make_str("reset session authorization"); }
		| RESET ALL
			{ $$ = make_str("reset all"); }
		;

ConstraintsSetStmt:    SET CONSTRAINTS constraints_set_list constraints_set_mode
			{ $$ = cat_str(3, make_str("set constraints"), $3, $4); }
		;

constraints_set_list:  ALL
			{ $$ = make_str("all"); }
		| name_list
			{ $$ = $1; }
		;

constraints_set_mode:  DEFERRED		{ $$ = make_str("deferred"); }
		| IMMEDIATE		{ $$ = make_str("immediate"); }
		;

/*
 * Checkpoint statement
 */
CheckPointStmt: CHECKPOINT	   { $$= make_str("checkpoint"); }
		;


/*****************************************************************************
 *
 *	ALTER TABLE variations
 *
 *****************************************************************************/

AlterTableStmt:
/* ALTER TABLE <relation> ADD [COLUMN] <coldef> */
		ALTER TABLE relation_expr ADD opt_column columnDef
			{ $$ = cat_str(5, make_str("alter table"), $3, make_str("add"), $5, $6); }
/* ALTER TABLE <relation> ALTER [COLUMN] <colname> {SET DEFAULT <expr>|DROP DEFAULT} */
		| ALTER TABLE relation_expr ALTER opt_column ColId alter_column_default
			{ $$ = cat_str(6, make_str("alter table"), $3, make_str("alter"), $5, $6, $7); }
/* ALTER TABLE <relation> ALTER [COLUMN] <colname> DROP NOT NULL */
		| ALTER TABLE relation_expr ALTER opt_column ColId DROP NOT NULL_P
			{ $$ = cat_str(6, make_str("alter table"), $3, make_str("alter"), $5, $6, make_str("drop not null")); }
/* ALTER TABLE <relation> ALTER [COLUMN] <colname> SET NOT NULL */
		| ALTER TABLE relation_expr ALTER opt_column ColId SET NOT NULL_P
			{ $$ = cat_str(6, make_str("alter table"), $3, make_str("alter"), $5, $6, make_str("set not null")); }
/* ALTER TABLE <relation> ALTER [COLUMN] <colname> SET STATISTICS <IntegerOnly> */
		| ALTER TABLE relation_expr ALTER opt_column ColId SET STATISTICS PosIntConst
			{ $$ = cat_str(7, make_str("alter table"), $3, make_str("alter"), $5, $6, make_str("set statistics"), $9); }
/* ALTER TABLE <relation> ALTER [COLUMN] <colname> SET STORAGE <storagemode> */
		| ALTER TABLE relation_expr ALTER opt_column ColId SET STORAGE ColId
			{ $$ = cat_str(7, make_str("alter table"), $3, make_str("alter"), $5, $6, make_str("set storage"), $9); }
/* ALTER TABLE <relation> DROP [COLUMN] <colname> {RESTRICT|CASCADE} */
		| ALTER TABLE relation_expr DROP opt_column ColId opt_drop_behavior
			{ $$ = cat_str(6, make_str("alter table"), $3, make_str("drop"), $5, $6, $7); }
/* ALTER TABLE <relation> ADD CONSTRAINT ... */
		| ALTER TABLE relation_expr ADD TableConstraint
			{ $$ = cat_str(4, make_str("alter table"), $3, make_str("add"), $5); }
/* ALTER TABLE <relation> DROP CONSTRAINT ... */
		| ALTER TABLE relation_expr DROP CONSTRAINT name opt_drop_behavior
			{ $$ = cat_str(5, make_str("alter table"), $3, make_str("drop constraint"), $6, $7); }
/* ALTER TABLE <relation> SET WITHOUT OIDS  */
		| ALTER TABLE relation_expr SET WITHOUT OIDS
			{ $$ = cat_str(3, make_str("alter table"), $3, make_str("set without oids")); }
 /* ALTER TABLE <name> CREATE TOAST TABLE */
		| ALTER TABLE qualified_name CREATE TOAST TABLE
			{ $$ = cat_str(3, make_str("alter table"), $3, make_str("create toast table")); }
/* ALTER TABLE <name> OWNER TO UserId */
		| ALTER TABLE qualified_name OWNER TO UserId
			{ $$ = cat_str(4, make_str("alter table"), $3, make_str("owner to"), $6); }
		;

alter_column_default:
		SET DEFAULT a_expr		{ $$ = cat2_str(make_str("set default"), $3); }
		| DROP DEFAULT			{ $$ = make_str("drop default"); }
		;

opt_drop_behavior: CASCADE 			{ $$ = make_str("cascade"); }
		| RESTRICT 			{ $$ = make_str("restrict"); }
		| /* EMPTY */ 			{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *		QUERY :
 *				close <optname>
 *
 *****************************************************************************/

ClosePortalStmt:  CLOSE opt_id	{ $$ = cat2_str(make_str("close"), $2); }
		;

opt_id:  ColId					{ $$ = $1; }
		| /*EMPTY*/ 			{ $$ = NULL; }
		;

/*****************************************************************************
 *
 *		QUERY :
 *				COPY [BINARY] <relname> FROM/TO
 *				[USING DELIMITERS <delimiter>]
 *
 *****************************************************************************/

CopyStmt:  COPY opt_binary qualified_name opt_oids copy_from
		copy_file_name copy_delimiter opt_with copy_opt_list
			{ $$ = cat_str(9, make_str("copy"), $2, $3, $4, $5, $6, $7, $8, $9); }
		;

copy_from:	TO					{ $$ = make_str("to"); }
		| FROM					{ $$ = make_str("from"); }
		;

/*
 * copy_file_name NULL indicates stdio is used. Whether stdin or stdout is
 * used depends on the direction. (It really doesn't make sense to copy from
 * stdout. We silently correct the "typo".		 - AY 9/94
 */
copy_file_name:  StringConst	{ $$ = $1; }
		| STDIN					{ $$ = make_str("stdin"); }
		| STDOUT				{ $$ = make_str("stdout"); }
		;

copy_opt_list: copy_opt_list copy_opt_item	{ $$ = cat2_str($1, $2); }
		| /* EMPTY */ 			{ $$ = EMPTY; }
		;

copy_opt_item:	BINARY		{ $$ = make_str("binary"); }
		| OIDS		{ $$ = make_str("oids"); }
		| DELIMITER opt_as StringConst
			{ $$ = cat_str(3, make_str("delimiter"), $2, $3); }
		| NULL_P opt_as StringConst
			{ $$ = cat_str(3, make_str("null"), $2, $3); }
		;

opt_binary:	BINARY		{ $$ = make_str("binary"); }
		| /* EMPTY */	{ $$ = EMPTY; }
		;

opt_oids:	WITH OIDS	{ $$ = make_str("with oids"); }
		| /* EMPTY */   { $$ = EMPTY; }
		;


/*
 * the default copy delimiter is tab but the user can configure it
 */
copy_delimiter:  opt_using DELIMITERS StringConst
			{ $$ = cat_str(3, $1, make_str("delimiters"), $3); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

opt_using:	USING		{ $$ = make_str("using"); }
		| /* EMPTY */	{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *		QUERY :
 *				CREATE TABLE relname
 *
 *****************************************************************************/

CreateStmt:  CREATE OptTemp TABLE qualified_name '(' OptTableElementList ')'
				OptInherit OptWithOids OnCommitOption
			{ $$ = cat_str(10, make_str("create"), $2, make_str("table"), $4, make_str("("), $6, make_str(")"), $8, $9, $10); }
		| CREATE OptTemp TABLE qualified_name OF qualified_name
			'(' OptTableElementList ')' OptWithOids OnCommitOption
			{ $$ = cat_str(11, make_str("create"), $2, make_str("table"), $4, make_str("of"), $6, make_str("("), $8, make_str(")"), $10, $11); }
		;

/*
 * Redundancy here is needed to avoid shift/reduce conflicts,
 * since TEMP is not a reserved word.  See also OptTempTableName.
 */

OptTemp: TEMPORARY			{ $$ = make_str("temporary"); }
		| TEMP				{ $$ = make_str("temp"); }
		| LOCAL TEMPORARY	{ $$ = make_str("local temporary"); }
		| LOCAL TEMP		{ $$ = make_str("local temp"); }
		| GLOBAL TEMPORARY
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported CREATE TABLE / GLOBAL TEMPORARY will be passed to backend");
			$$ = make_str("global temporary");
		}
		| GLOBAL TEMP
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported CREATE TABLE / GLOBAL TEMP will be passed to backend");
			$$ = make_str("global temp");
		}
		| /*EMPTY*/			{ $$ = EMPTY; }
		;


OptTableElementList:  TableElementList
			{ $$ = $1; }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;
TableElementList: TableElement
			{ $$ = $1; }
		| TableElementList ',' TableElement
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

TableElement:  columnDef		{ $$ = $1; }
		| TableLikeClause	{ $$ = $1; }
		| TableConstraint	{ $$ = $1; }
		;

columnDef:	ColId Typename ColQualList opt_collate
		{
			if (strlen($4) > 0)
			{
				snprintf(errortext, sizeof(errortext), "Currently unsupported CREATE TABLE / COLLATE %s will be passed to backend", $4);
				mmerror(PARSE_ERROR, ET_WARNING, errortext);
			}
			$$ = cat_str(4, $1, $2, $3, $4);
		}
		;

ColQualList:  ColQualList ColConstraint { $$ = cat2_str($1,$2); }
		| /*EMPTY*/						{ $$ = EMPTY; }
		;

ColConstraint:	CONSTRAINT name ColConstraintElem
			{ $$ = cat_str(3, make_str("constraint"), $2, $3); }
		| ColConstraintElem		{ $$ = $1; }
		| ConstraintAttr		{ $$ = $1; }
		;

/* DEFAULT NULL is already the default for Postgres.
 * But define it here and carry it forward into the system
 * to make it explicit.
 * - thomas 1998-09-13
 *
 * WITH NULL and NULL are not SQL92-standard syntax elements,
 * so leave them out. Use DEFAULT NULL to explicitly indicate
 * that a column may have that value. WITH NULL leads to
 * shift/reduce conflicts with WITH TIME ZONE anyway.
 * - thomas 1999-01-08
 */
ColConstraintElem:	NOT NULL_P
			{ $$ = make_str("not null"); }
		| NULL_P
			{ $$ = make_str("null"); }
		| UNIQUE
			{ $$ = make_str("unique"); }
		| PRIMARY KEY
			{ $$ = make_str("primary key"); }
		| CHECK '(' a_expr ')'
			{ $$ = cat_str(3, make_str("check ("), $3, make_str(")")); }
		| DEFAULT b_expr
			{ $$ = cat2_str(make_str("default"), $2); }
		|  REFERENCES qualified_name opt_column_list key_match key_actions
			{ $$ = cat_str(5, make_str("references"), $2, $3, $4, $5); }
		;

/*
 * ConstraintAttr represents constraint attributes, which we parse as if
 * they were independent constraint clauses, in order to avoid shift/reduce
 * conflicts (since NOT might start either an independent NOT NULL clause
 * or an attribute).  analyze.c is responsible for attaching the attribute
 * information to the preceding "real" constraint node, and for complaining
 * if attribute clauses appear in the wrong place or wrong combinations.
 *
 * See also ConstraintAttributeSpec, which can be used in places where
 * there is no parsing conflict.
 */
ConstraintAttr: DEFERRABLE		{ $$ = make_str("deferrable"); }
		| NOT DEFERRABLE		{ $$ = make_str("not deferrable"); }
		| INITIALLY DEFERRED	{ $$ = make_str("initially deferred"); }
		| INITIALLY IMMEDIATE	{ $$ = make_str("initially immediate"); }
		;

TableLikeClause:  LIKE any_name
	                {
				mmerror(PARSE_ERROR, ET_ERROR, "LIKE in table definitions not yet supported");
				$$ = cat2_str(make_str("like"), $2);
			}
		;

/* ConstraintElem specifies constraint syntax which is not embedded into
 *	a column definition. ColConstraintElem specifies the embedded form.
 * - thomas 1997-12-03
 */
TableConstraint:  CONSTRAINT name ConstraintElem
			{ $$ = cat_str(3, make_str("constraint"), $2, $3); }
		| ConstraintElem
			{ $$ = $1; }
		;

ConstraintElem:  CHECK '(' a_expr ')'
			{ $$ = cat_str(3, make_str("check("), $3, make_str(")")); }
		| UNIQUE '(' columnList ')'
			{ $$ = cat_str(3, make_str("unique("), $3, make_str(")")); }
		| PRIMARY KEY '(' columnList ')'
			{ $$ = cat_str(3, make_str("primary key("), $4, make_str(")")); }
		| FOREIGN KEY '(' columnList ')' REFERENCES qualified_name opt_column_list
			key_match key_actions ConstraintAttributeSpec
			{ $$ = cat_str(8, make_str("foreign key("), $4, make_str(") references"), $7, $8, $9, $10, $11); }
		;

opt_column_list:  '(' columnList ')'	{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
               | /*EMPTY*/		{ $$ = EMPTY; }
	       ;

columnList:  columnList ',' columnElem
                               { $$ = cat_str(3, $1, make_str(","), $3); }
               | columnElem
                               { $$ = $1; }
               ;

columnElem:  ColId	{ $$ = $1; }
		;

key_match:	MATCH FULL
			{ $$ = make_str("match full"); }
		| MATCH PARTIAL
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported FOREIGN KEY/MATCH PARTIAL will be passed to backend");
			$$ = make_str("match partial");
		}
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

key_actions:  key_delete			{ $$ = $1; }
		| key_update				{ $$ = $1; }
		| key_delete key_update 	{ $$ = cat2_str($1, $2); }
		| key_update key_delete 	{ $$ = cat2_str($1, $2); }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;

key_delete: ON DELETE_P key_action
			{ $$ = cat2_str(make_str("on delete"), $3); }
		;

key_update: ON UPDATE key_action
			{ $$ = cat2_str(make_str("on update"), $3); }
		;

key_action:	NO ACTION			{ $$ = make_str("no action"); }
		| RESTRICT					{ $$ = make_str("restrict"); }
		| CASCADE					{ $$ = make_str("cascade"); }
		| SET DEFAULT				{ $$ = make_str("set default"); }
		| SET NULL_P				{ $$ = make_str("set null"); }
		;

OptInherit:  INHERITS '(' qualified_name_list ')'
			{ $$ = cat_str(3, make_str("inherits ("), $3, make_str(")")); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

OptWithOids:  WITH OIDS				{ $$ = make_str("with oids"); }
		| WITHOUT OIDS				{ $$ = make_str("without oids"); }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;

OnCommitOption:   ON COMMIT DROP		{ $$ = make_str("on commit drop"); }
		| ON COMMIT DELETE_P ROWS	{ $$ = make_str("on commit delete rows"); }
		| ON COMMIT PRESERVE ROWS	{ $$ = make_str("on commit preserve rows"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;


/*
 * Note: CREATE TABLE ... AS SELECT ... is just another spelling for
 * SELECT ... INTO.
 */

CreateAsStmt:  CREATE OptTemp TABLE qualified_name OptCreateAs AS
		{ FoundInto = 0; }
		SelectStmt
		{
			if (FoundInto == 1)
				mmerror(PARSE_ERROR, ET_ERROR, "CREATE TABLE / AS SELECT may not specify INTO");

			$$ = cat_str(7, make_str("create"), $2, make_str("table"), $4, $5, make_str("as"), $8);
		}
		;

OptCreateAs:  '(' CreateAsList ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

CreateAsList:  CreateAsList ',' CreateAsElement
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		| CreateAsElement
			{ $$ = $1; }
		;

CreateAsElement:  ColId { $$ = $1; }
		;

/*****************************************************************************
 *
 *		QUERY :
 *				CREATE SEQUENCE seqname
 *
 *****************************************************************************/

CreateSeqStmt:	CREATE OptTemp SEQUENCE qualified_name OptSeqList
			{ $$ = cat_str(4, make_str("create sequence"), $2, $4, $5); }
		;

OptSeqList:  OptSeqList OptSeqElem	{ $$ = cat2_str($1, $2); }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;

OptSeqElem:  CACHE NumConst
			{ $$ = cat2_str(make_str("cache"), $2); }
		| CYCLE
			{ $$ = make_str("cycle"); }
		| NO CYCLE
			{ $$ = make_str("no cycle"); }
		| INCREMENT opt_by NumConst
			{ $$ = cat_str(3, make_str("increment"), $2, $3); }
		| MAXVALUE NumConst
			{ $$ = cat2_str(make_str("maxvalue"), $2); }
		| MINVALUE NumConst
			{ $$ = cat2_str(make_str("minvalue"), $2); }
		| NO MAXVALUE
			{ $$ = make_str("no maxvalue"); }
		| NO MINVALUE
			{ $$ = make_str("no minvalue"); }
		| START opt_with NumConst
			{ $$ = cat_str(3, make_str("start"), $2, $3); }
		;

opt_by:		BY	{ $$ = make_str("by"); }
		| /*EMPTY*/	{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *		QUERIES :
 *				CREATE PROCEDURAL LANGUAGE ...
 *				DROP PROCEDURAL LANGUAGE ...
 *
 *****************************************************************************/

CreatePLangStmt:  CREATE opt_Trusted opt_procedural LANGUAGE ColId_or_Sconst
			HANDLER handler_name opt_lancompiler
			{ $$ = cat_str(8, make_str("create"), $2, $3, make_str("language"), $5, make_str("handler"), $7, $8); }
		;

opt_Trusted:	TRUSTED { $$ = make_str("trusted"); }
		| /*EMPTY*/		{ $$ = EMPTY; }
		;

/* This ought to be just func_name, but that causes reduce/reduce conflicts
 * (CREATE LANGUAGE is the only place where func_name isn't followed by '(').
 * Work around by using name and dotted_name separately.
 */
handler_name: name
                               { $$ = $1; }
	| dotted_name
                               { $$ = $1; /* XXX changing soon */ }
               ;

opt_lancompiler: LANCOMPILER StringConst
			{ $$ = cat2_str(make_str("lancompiler"), $2); }
		| /*EMPTY*/
			{ $$ = ""; }
		;

DropPLangStmt:	DROP opt_procedural LANGUAGE StringConst opt_drop_behavior
			{ $$ = cat_str(5, make_str("drop"), $2, make_str("language"), $4, $5); }
		;

opt_procedural: PROCEDURAL	{ $$ = make_str("prcedural"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *		QUERIES :
 *				CREATE TRIGGER ...
 *				DROP TRIGGER ...
 *
 *****************************************************************************/

CreateTrigStmt:  CREATE TRIGGER name TriggerActionTime TriggerEvents ON
				qualified_name TriggerForSpec
				EXECUTE PROCEDURE
				name '(' TriggerFuncArgs ')'
			{ $$ = cat_str(12, make_str("create trigger"), $3, $4, $5, make_str("on"), $7, $8, make_str("execute procedure"), $11, make_str("("), $13, make_str(")")); }
		|	CREATE CONSTRAINT TRIGGER name AFTER TriggerEvents ON
				qualified_name OptConstrFromTable
				ConstraintAttributeSpec
				FOR EACH ROW EXECUTE PROCEDURE
				func_name '(' TriggerFuncArgs ')'
			{ $$ = cat_str(13, make_str("create constraint trigger"), $4, make_str("after"), $6, make_str("on"), $8, $9, $10, make_str("for each row execute procedure"), $16, make_str("("), $18, make_str(")")); }
		;

TriggerActionTime:	BEFORE		{ $$ = make_str("before"); }
		| AFTER					{ $$ = make_str("after"); }
		;

TriggerEvents:	TriggerOneEvent
			{ $$ = $1; }
		| TriggerOneEvent OR TriggerOneEvent
			{ $$ = cat_str(3, $1, make_str("or"), $3); }
		| TriggerOneEvent OR TriggerOneEvent OR TriggerOneEvent
			{ $$ = cat_str(5, $1, make_str("or"), $3, make_str("or"), $5); }
		;

TriggerOneEvent:  INSERT	{ $$ = make_str("insert"); }
		| DELETE_P			{ $$ = make_str("delete"); }
		| UPDATE			{ $$ = make_str("update"); }
		;

TriggerForSpec:  FOR TriggerForOpt TriggerForType
			{ $$ = cat_str(3, make_str("for"), $2, $3); }
		| /* EMPTY */
			{ $$ = EMPTY; }
		;

TriggerForOpt:	EACH		{ $$ = make_str("each"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

TriggerForType:  ROW		{ $$ = make_str("row"); }
		| STATEMENT			{ $$ = make_str("statement"); }
		;

TriggerFuncArgs:  TriggerFuncArg
			{ $$ = $1; }
		| TriggerFuncArgs ',' TriggerFuncArg
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

TriggerFuncArg:  PosAllConst 	{ $$ = $1; }
		| ColId		{ $$ = $1; }
		;

OptConstrFromTable: /* Empty */		{ $$ = EMPTY; }
		| FROM qualified_name	{ $$ = cat2_str(make_str("from"), $2); }
		;

ConstraintAttributeSpec: ConstraintDeferrabilitySpec	{ $$ = $1; }
		| ConstraintDeferrabilitySpec ConstraintTimeSpec
		{
			if (strcmp($1, "deferrable") != 0 && strcmp($2, "initially deferrable") == 0 )
				mmerror(PARSE_ERROR, ET_ERROR, "INITIALLY DEFERRED constraint must be DEFERRABLE");

			$$ = cat2_str($1, $2);
		}
		| ConstraintTimeSpec		{ $$ = $1; }
		| ConstraintTimeSpec ConstraintDeferrabilitySpec
		{
			if (strcmp($2, "deferrable") != 0 && strcmp($1, "initially deferrable") == 0 )
				mmerror(PARSE_ERROR, ET_ERROR, "INITIALLY DEFERRED constraint must be DEFERRABLE");

			$$ = cat2_str($1, $2);
		}
		;

ConstraintDeferrabilitySpec: NOT DEFERRABLE
			{ $$ = make_str("not deferrable"); }
		| DEFERRABLE
			{ $$ = make_str("deferrable"); }
		;

ConstraintTimeSpec: INITIALLY IMMEDIATE
			{ $$ = make_str("initially immediate"); }
		| INITIALLY DEFERRED
			{ $$ = make_str("initially deferred"); }
		;

DropTrigStmt:  DROP TRIGGER name ON qualified_name opt_drop_behavior
			{ $$ = cat_str(5, make_str("drop trigger"), $3, make_str("on"), $5, $6); }
		;

/*****************************************************************************
 *
 *             QUERIES :
 *                             CREATE ASSERTION ...
 *                             DROP ASSERTION ...
 *
 *****************************************************************************/
CreateAssertStmt:  CREATE ASSERTION name
                       CHECK '(' a_expr ')' ConstraintAttributeSpec
		        {
				mmerror(PARSE_ERROR, ET_ERROR, "CREATE ASSERTION is not yet supported");
		       		$$ = cat_str(6, make_str("create assertion"), $3, make_str("check ("), $6, make_str(")"), $8);
			}
		;

DropAssertStmt:  DROP ASSERTION name
	{
		mmerror(PARSE_ERROR, ET_ERROR, "DROP ASSERTION is not yet supported");
		$$ = cat2_str(make_str("drop assertion"), $3);
	}
	;


/*****************************************************************************
 *
 *		QUERY :
 *				define (type,operator,aggregate)
 *
 *****************************************************************************/

DefineStmt:  CREATE AGGREGATE func_name definition
			{ $$ = cat_str(3, make_str("create aggregate"), $3, $4); }
		| CREATE OPERATOR all_Op definition
			{ $$ = cat_str(3, make_str("create operator"), $3, $4); }
		| CREATE TYPE_P any_name definition
			{ $$ = cat_str(3, make_str("create type"), $3, $4); }
		| CREATE TYPE_P any_name AS rowdefinition
			{ $$ = cat_str(4, make_str("create type"), $3, make_str("as"), $5); }
		;

rowdefinition: '(' TableFuncElementList ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")"));}
		;

definition:  '(' def_list ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		;

def_list:  def_elem					{ $$ = $1; }
		| def_list ',' def_elem		{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

def_elem:  ColLabel '=' def_arg		{ $$ = cat_str(3, $1, make_str("="), $3); }
		| ColLabel					{ $$ = $1; }
		;

/* Note: any simple identifier will be returned as a type name! */
def_arg:  func_return				{  $$ = $1; }
		| qual_all_Op					{  $$ = $1; }
		| AllConst					{  $$ = $1; }
		;

CreateOpClassStmt:      CREATE OPERATOR CLASS any_name opt_default FOR TYPE_P Typename
                        USING access_method AS opclass_item_list
				{
					$$ = cat_str(9, make_str("create operator class"), $4, $5, make_str("for type"), $8, make_str("using"), $10, make_str("as"), $12);
				}
				;

opclass_item_list:	opclass_item		{ $$ = $1; }
			| opclass_item_list ',' opclass_item	{ $$ = cat_str(3, $1, make_str(","), $3); }
			;

opclass_item:	OPERATOR PosIntConst any_operator opt_recheck
			{ $$ = cat_str(4, make_str("operator"), $2, $3, $4); }
		| OPERATOR PosIntConst any_operator '(' oper_argtypes ')' opt_recheck
			{ $$ =  cat_str(7, make_str("operator"), $2, $3, make_str("("), $5, make_str(")"), $7); }
		| FUNCTION PosIntConst func_name func_args
			{ $$ = cat_str(4, make_str("function"), $2, $3, $4); }
		| STORAGE Typename
			{ $$ = cat2_str(make_str("storage"), $2); }
		;

opt_default:   DEFAULT	{ $$ = make_str("default"); }
	|  /*EMPTY*/    { $$ = EMPTY; }
	;

opt_recheck:   RECHECK	{ $$ = make_str("recheck"); }
	|  /*EMPTY*/    { $$ = EMPTY; }
	;

DropOpClassStmt: DROP OPERATOR CLASS any_name USING access_method opt_drop_behavior
			{ $$ = cat_str(5,make_str("drop operator class"), $4, make_str("using"), $6, $7); }
		;

/*****************************************************************************
 *
 *		QUERY:
 *
 *			   DROP itemtype itemname [, itemname ...]
 *
 *****************************************************************************/

DropStmt:  DROP drop_type any_name_list opt_drop_behavior
			{ $$ = cat_str(4, make_str("drop"), $2, $3, $4); }
		;

drop_type:	TABLE		{ $$ = make_str("table"); }
		| SEQUENCE		{ $$ = make_str("sequence"); }
		| VIEW			{ $$ = make_str("view"); }
		| INDEX			{ $$ = make_str("index"); }
		| TYPE_P		{ $$ = make_str("type"); }
		| DOMAIN_P		{ $$ = make_str("domain"); }
		| CONVERSION_P		{ $$ = make_str("conversion"); }
		| SCHEMA		{ $$ = make_str("schema"); }
		;

any_name_list:  any_name
                       { $$ = $1; }
               | any_name_list ',' any_name
                       { $$ = cat_str(3, $1, make_str(","), $3); }
               ;

any_name: ColId
                       { $$ = $1; }
               | dotted_name
                       { $$ = $1; }
                ;
/*****************************************************************************
 *
 *			   QUERY:
 *							   truncate table relname
 *
 *****************************************************************************/
TruncateStmt:  TRUNCATE opt_table qualified_name
			{ $$ = cat_str(3, make_str("truncate table"), $2, $3); }
		;

/*****************************************************************************
 *
 *		QUERY:
 *					   fetch/move [forward | backward] [ # | all ] [ in <portalname> ]
 *					   fetch [ forward | backward | absolute | relative ]
 *							 [ # | all | next | prior ] [ [ in | from ] <portalname> ]
 *
 *****************************************************************************/

FetchStmt: FETCH direction fetch_how_many from_in name ecpg_into
			{ $$ = cat_str(5, make_str("fetch"), $2, $3, $4, $5); }
		| FETCH fetch_how_many from_in name ecpg_into
			{ $$ = cat_str(4, make_str("fetch"), $2, $3, $4); }
		| FETCH direction from_in name ecpg_into
			{ $$ = cat_str(4, make_str("fetch"), $2, $3, $4); }
		| FETCH from_in name ecpg_into
			{ $$ = cat_str(3, make_str("fetch"), $2, $3); }
		| FETCH name ecpg_into
			{ $$ = cat2_str(make_str("fetch"), $2); }
		| MOVE direction fetch_how_many from_in name
			{ $$ = cat_str(5, make_str("move"), $2, $3, $4, $5); }
		| MOVE fetch_how_many from_in name
			{ $$ = cat_str(4, make_str("move"), $2, $3, $4); }
		| MOVE direction from_in name
			{ $$ = cat_str(4, make_str("move"), $2, $3, $4); }
		| MOVE from_in name
			{ $$ = cat_str(3, make_str("move"), $2, $3); }
		| MOVE name
			{ $$ = cat2_str(make_str("move"), $2); }
		;

direction:	FORWARD		{ $$ = make_str("forward"); }
		| BACKWARD		{ $$ = make_str("backward"); }
		| RELATIVE		{ $$ = make_str("relative"); }
		| ABSOLUTE
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported FETCH/ABSOLUTE will be passed to backend, backend will use RELATIVE");
			$$ = make_str("absolute");
		}
		;

fetch_how_many: IntConst	{ $$ = $1; }
		| ALL		{ $$ = make_str("all"); }
		| NEXT		{ $$ = make_str("next"); }
		| PRIOR		{ $$ = make_str("prior"); }
		;

from_in: IN_P				{ $$ = make_str("in"); }
		| FROM				{ $$ = make_str("from"); }
		;

/*****************************************************************************
 *
 *	The COMMENT ON statement can take different forms based upon the type of
 *	the object associated with the comment. The form of the statement is:
 *
 *	COMMENT ON [ [ DATABASE | DOMAIN | INDEX |  SEQUENCE | TABLE | TYPE | VIEW ]
 *				 <objname> | AGGREGATE <aggname> (<aggtype>) | FUNCTION
 *				<funcname> (arg1, arg2, ...) | OPERATOR <op>
 *				(leftoperand_typ rightoperand_typ) | TRIGGER <triggername> ON
 *				<relname> | RULE <rulename> ON <relname> ] IS 'text'
 *
 *****************************************************************************/
CommentStmt:   COMMENT ON comment_type name IS comment_text
			{ $$ = cat_str(5, make_str("comment on"), $3, $4, make_str("is"), $6); }
		| COMMENT ON AGGREGATE func_name '(' aggr_argtype ')' IS comment_text
			{ $$ = cat_str(6, make_str("comment on aggregate"), $4, make_str("("), $6, make_str(") is"), $9); }
		| COMMENT ON FUNCTION func_name func_args IS comment_text
			{ $$ = cat_str(5, make_str("comment on function"), $4, $5, make_str("is"), $7); }
		| COMMENT ON OPERATOR all_Op '(' oper_argtypes ')' IS comment_text
			{ $$ = cat_str(6, make_str("comment on operator"), $4, make_str("("), $6, make_str(") is"), $9); }
		| COMMENT ON TRIGGER name ON any_name IS comment_text
			{ $$ = cat_str(6, make_str("comment on trigger"), $4, make_str("on"), $6, make_str("is"), $8); }
		| COMMENT ON RULE name ON any_name IS comment_text
			{ $$ = cat_str(6, make_str("comment on rule"), $4, make_str("on"), $6, make_str("is"), $8); }
		| COMMENT ON RULE name IS comment_text
			{ $$ = cat_str(4, make_str("comment on rule"), $4, make_str("is"), $6); }
		;

comment_type:  COLUMN		{ $$ = make_str("column"); }
		| DATABASE	{ $$ = make_str("database"); }
		| SCHEMA	{ $$ = make_str("schema"); }
		| INDEX		{ $$ = make_str("idnex"); }
		| SEQUENCE	{ $$ = make_str("sequence"); }
		| TABLE		{ $$ = make_str("table"); }
		| DOMAIN_P	{ $$ = make_str("domain"); }
		| TYPE_P	{ $$ = make_str("type"); }
		| VIEW		{ $$ = make_str("view"); }
		;

comment_text:	StringConst { $$ = $1; }
		| NULL_P			{ $$ = make_str("null"); }
	   	;

/*****************************************************************************
 *
 *		QUERY:
 * GRANT and REVOKE statements
 *
 *****************************************************************************/

GrantStmt:	GRANT privileges ON privilege_target TO grantee_list opt_grant_grant_option
			{ $$ = cat_str(7, make_str("grant"), $2, make_str("on"), $4, make_str("to"), $6, $7); }
		;

RevokeStmt:  REVOKE opt_revoke_grant_option privileges ON privilege_target FROM grantee_list opt_drop_behavior
			{
			  $$ = cat_str(9, make_str("revoke"), $2, $3, make_str("on"), $5, make_str("from"), $7, $8);
			}
			  
		;

privileges:  ALL PRIVILEGES		{ $$ = make_str("all privileges"); }
		| ALL					{ $$ = make_str("all"); }
		| privilege_list		{ $$ = $1; }
		;

privilege_list:  privilege
			{ $$ = $1; }
		| privilege_list ',' privilege
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

privilege:	SELECT			{ $$ = make_str("select"); }
		| INSERT			{ $$ = make_str("insert"); }
		| UPDATE			{ $$ = make_str("update"); }
		| DELETE_P			{ $$ = make_str("delete"); }
		| RULE				{ $$ = make_str("rule"); }
		| REFERENCES		{ $$ = make_str("references"); }
		| TRIGGER			{ $$ = make_str("trigger"); }
		| EXECUTE			{ $$ = make_str("execute"); }
		| USAGE				{ $$ = make_str("usage"); }
		| CREATE			{ $$ = make_str("create"); }
		| TEMPORARY			{ $$ = make_str("temporary"); }
		| TEMP				{ $$ = make_str("temp"); }
		;

privilege_target: qualified_name_list
			{ $$ = $1; }
		| TABLE qualified_name_list
			{ $$ = cat2_str(make_str("table"), $2); }
		| FUNCTION function_with_argtypes_list
			{ $$ = cat2_str(make_str("function"), $2); }
		| DATABASE name_list
			{ $$ = cat2_str(make_str("database"), $2); }
		| LANGUAGE name_list
			{ $$ = cat2_str(make_str("language") , $2); }
		| SCHEMA name_list
			{ $$ = cat2_str(make_str("schema") , $2); }
		;

grantee_list: grantee
			{ $$ = $1; }
		| grantee_list ',' grantee
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

grantee:  ColId			{ $$ = $1; }
		| GROUP_P ColId		{ $$ = cat2_str(make_str("group"), $2); }
		;

opt_grant_grant_option:  WITH GRANT OPTION
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported GRANT/WITH GRANT OPTION will be passed to backend");
			$$ = make_str("with grant option");
		}
		| /*EMPTY*/ 	{ $$ = EMPTY; }
		;

opt_revoke_grant_option: GRANT OPTION FOR
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported REVOKE/GRANT OPTION FOR will be passed to backend");
			$$ = make_str("with grant option");
		}
		| /*EMPTY*/ 	{ $$ = EMPTY; }
		;

function_with_argtypes_list: function_with_argtypes
			{ $$ = $1; }
		| function_with_argtypes_list ',' function_with_argtypes
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

function_with_argtypes: func_name func_args { $$ = cat2_str($1, $2); };

/*****************************************************************************
 *
 *		QUERY:
 *				create index <indexname> on <relname>
 *				  [ using <access> ] "(" (<col> with <op>)+ ")"
 *				  [ where <predicate> ]
 *
 *****************************************************************************/

IndexStmt:	CREATE index_opt_unique INDEX index_name ON qualified_name
				access_method_clause '(' index_params ')' where_clause
			{ $$ = cat_str(11, make_str("create"), $2, make_str("index"), $4, make_str("on"), $6, $7, make_str("("), $9, make_str(")"), $11); }
		;

index_opt_unique:  UNIQUE	{ $$ = make_str("unique"); }
		| /*EMPTY*/ 		{ $$ = EMPTY; }
		;

access_method_clause:  USING access_method
			{ $$ = cat2_str(make_str("using"), $2); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

index_params:  index_list	{ $$ = $1; }
		| func_index		{ $$ = $1; }
		;

index_list:  index_list ',' index_elem
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		| index_elem
			{ $$ = $1; }
		;

func_index:  func_name '(' name_list ')' opt_class
			{ $$ = cat_str(5, $1, make_str("("), $3, ")", $5); }
		;

index_elem:  attr_name opt_class
			{ $$ = cat2_str($1, $2); }
		;

opt_class:	any_name 	{ $$ = $1; }
		| USING any_name	{ $$ = cat2_str(make_str("using"), $2); }
		| /*EMPTY*/		{ $$ = EMPTY; }
		;

CreateFunctionStmt:	CREATE opt_or_replace FUNCTION func_name func_args
					RETURNS func_return createfunc_opt_list opt_definition
			{ $$ = cat_str(8, make_str("create"), $2, make_str("function"), $4, $5, make_str("returns"), $7, $8); }
		;

opt_or_replace:  OR REPLACE		{ $$ = make_str("or replace"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

func_args:	'(' func_args_list ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| '(' ')'
			{ $$ = make_str("()"); }
		;

func_args_list:  func_arg
			{ $$ = $1; }
		| func_args_list ',' func_arg
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

func_arg:  opt_arg func_type
		{
			/* We can catch over-specified arguments here if we want to,
			 * but for now better to silently swallow typmod, etc.
			 * - thomas 2000-03-22
			 */
			$$ = cat2_str($1, $2);
		}
		| func_type		{ $$ = $1; }
		;

opt_arg:  IN_P	{ $$ = make_str("in"); }
		| OUT_P
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported CREATE FUNCTION/OUT will be passed to backend");

			$$ = make_str("out");
		}
		| INOUT
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported CREATE FUNCTION/INOUT will be passed to backend");

			$$ = make_str("inout");
		}
		;

func_as: StringConst
			{ $$ = $1; }
		| StringConst ',' StringConst
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

func_return:  func_type
		{
			/* We can catch over-specified arguments here if we want to,
			 * but for now better to silently swallow typmod, etc.
			 * - thomas 2000-03-22
			 */
			$$ = $1;
		}
		;

func_type:	Typename
			{ $$ = $1; }
		| type_name attrs '%' TYPE_P
			{ $$ = cat_str(3, $1, $2, make_str("% type")); }
		;


createfunc_opt_list: createfunc_opt_item
			{ $$ = $1; }
		| createfunc_opt_list createfunc_opt_item
			{ $$ = cat2_str($1, $2); }
		;

createfunc_opt_item: AS func_as
				{ $$ = cat2_str(make_str("as"), $2); }
		| LANGUAGE ColId_or_Sconst
				{ $$ = cat2_str(make_str("language"), $2); }
		| IMMUTABLE
				{ $$ = make_str("immutable"); }
		| STABLE
				{ $$ = make_str("stable"); }
		| VOLATILE
				{ $$ = make_str("volatile"); }
		| CALLED ON NULL_P INPUT
				{ $$ = make_str("called on null input"); }
		| RETURNS NULL_P ON NULL_P INPUT
				{ $$ = make_str("returns null on null input"); }
		| STRICT
				{ $$ = make_str("strict"); }
		| EXTERNAL SECURITY DEFINER
				{ $$ = make_str("external security definer"); }
		| EXTERNAL SECURITY INVOKER
				{ $$ = make_str("external security invoker"); }
		| SECURITY DEFINER
				{ $$ = make_str("security definer"); }
		| SECURITY INVOKER
				{ $$ = make_str("security invoker"); }
		;

opt_definition: WITH definition	{ $$ = cat2_str(make_str("with"), $2); }
                | /*EMPTY*/     { $$ = EMPTY; }
	        ;

/*****************************************************************************
 *
 *		QUERY:
 *
 *			   DROP FUNCTION funcname (arg1, arg2, ...)
 *			   DROP AGGREGATE aggname (aggtype)
 *			   DROP OPERATOR opname (leftoperand_typ rightoperand_typ)
 *
 *****************************************************************************/

RemoveFuncStmt:  DROP FUNCTION func_name func_args opt_drop_behavior
			{ $$ = cat_str(4, make_str("drop function"), $3, $4, $5); }
		;

RemoveAggrStmt:  DROP AGGREGATE func_name '(' aggr_argtype ')' opt_drop_behavior
			{ $$ = cat_str(6, make_str("drop aggregate"), $3, make_str("("), $5, make_str(")"), $7); }
		;

aggr_argtype:  Typename		{ $$ = $1; }
		| '*'				{ $$ = make_str("*"); }
		;


RemoveOperStmt:  DROP OPERATOR all_Op '(' oper_argtypes ')' opt_drop_behavior
			{ $$ = cat_str(6, make_str("drop operator"), $3, make_str("("), $5, make_str(")"), $7); }
		;

oper_argtypes:	Typename
			{ mmerror(PARSE_ERROR, ET_ERROR, "parser: argument type missing (use NONE for unary operators)"); }
		| Typename ',' Typename
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		| NONE ',' Typename			/* left unary */
			{ $$ = cat2_str(make_str("none,"), $3); }
		| Typename ',' NONE			/* right unary */
			{ $$ = cat2_str($1, make_str(", none")); }
		;

any_operator:
                        all_Op
				{ $$ = $1; }
			| ColId '.' any_operator
				{ $$ = cat_str(3, $1, make_str("."), $3); }
			;

CreateCastStmt:		CREATE CAST '(' Typename AS Typename ')'
				WITH FUNCTION function_with_argtypes cast_context
			{ $$ = cat_str(6, make_str("create cast ("), $4, make_str("as"), $6, make_str(") with function"), $10); }
			| CREATE CAST '(' Typename AS Typename ')'
				WITHOUT FUNCTION cast_context
			{ $$ = cat_str(6, make_str("create cast ("), $4, make_str("as"), $6, make_str(") without function"), $10); }
		;

cast_context: AS ASSIGNMENT   { $$ = make_str("as assignment"); }
		| /*EMPTY*/	{ $$ = EMPTY; }
		;


DropCastStmt: DROP CAST '(' Typename AS Typename ')' opt_drop_behavior
			{ $$ = cat_str(6, make_str("drop cast ("), $4, make_str("as"), $6, make_str(")"), $8); }
		;

/*****************************************************************************
 *
 *				QUERY:
 *
 *				REINDEX type <typename> [FORCE] [ALL]
 *
 *****************************************************************************/
ReindexStmt:  REINDEX reindex_type qualified_name opt_force
			{ $$ = cat_str(4, make_str("reindex"), $2, $3, $4); }
		| REINDEX DATABASE name opt_force
			{ $$ = cat_str(3, make_str("reindex database"), $3, $4); }
		;

reindex_type:	INDEX		{ $$ = make_str("index"); }
		| TABLE		{ $$ = make_str("table"); }
		;

opt_force: FORCE			{ $$ = make_str("force"); }
		| /* EMPTY */		{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *		QUERY:
 *				rename <attrname1> in <relname> [*] to <attrname2>
 *				rename <relname1> to <relname2>
 *
 *****************************************************************************/

RenameStmt:  ALTER TABLE relation_expr RENAME opt_column opt_name TO name
			{ $$ = cat_str(7, make_str("alter table"), $3, make_str("rename"), $5, $6, make_str("to"), $8); }
	| ALTER TRIGGER name ON relation_expr RENAME TO name
			{ $$ = cat_str(6, make_str("alter trigger"), $3, make_str("on"), $5, make_str("rename to"), $8); }
		;

opt_name:  name				{ $$ = $1; }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

opt_column:  COLUMN			{ $$ = make_str("column"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;


/*****************************************************************************
 *
 *		QUERY:	Define Rewrite Rule
 *
 *****************************************************************************/

RuleStmt:  CREATE opt_or_replace RULE name AS
		   { QueryIsRule=1; }
		   ON event TO qualified_name where_clause
		   DO opt_instead RuleActionList
		{
			QueryIsRule=0;
			$$ = cat_str(12, make_str("create"), $2, make_str("rule"), $4, make_str("as on"), $8, make_str("to"), $10, $11, make_str("do"), $13, $14);
		}
		;

RuleActionList:  NOTHING				{ $$ = make_str("nothing"); }
		| RuleActionStmt				{ $$ = $1; }
		| '(' RuleActionMulti ')'		{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
				;

/* the thrashing around here is to discard "empty" statements... */
RuleActionMulti:  RuleActionMulti ';' RuleActionStmtOrEmpty
			{  $$ = cat_str(3, $1, make_str(";"), $3); }
		| RuleActionStmtOrEmpty
			{ $$ = cat2_str($1, make_str(";")); }
		;

RuleActionStmt:   SelectStmt
		| InsertStmt
		| UpdateStmt
		| DeleteStmt
		| NotifyStmt
		;

RuleActionStmtOrEmpty: RuleActionStmt	{ $$ = $1; }
	   | /*EMPTY*/						{ $$ = EMPTY; }
	   ;

/* change me to select, update, etc. some day */
event:	SELECT				{ $$ = make_str("select"); }
		| UPDATE			{ $$ = make_str("update"); }
		| DELETE_P			{ $$ = make_str("delete"); }
		| INSERT			{ $$ = make_str("insert"); }
		 ;

opt_instead:  INSTEAD		{ $$ = make_str("instead"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

DropRuleStmt:  DROP RULE name ON qualified_name opt_drop_behavior
		{ $$ = cat_str(5, make_str("drop rule"), $3, make_str("on"), $5, $6);}
		;

/*****************************************************************************
 *
 *		QUERY:
 *				NOTIFY <qualified_name>	can appear both in rule bodies and
 *				as a query-level command
 *
 *****************************************************************************/

NotifyStmt:  NOTIFY qualified_name
		{ $$ = cat2_str(make_str("notify"), $2); }
		;

ListenStmt:  LISTEN qualified_name
		{ $$ = cat2_str(make_str("listen"), $2); }
		;

UnlistenStmt:  UNLISTEN qualified_name
			{ $$ = cat2_str(make_str("unlisten"), $2); }
		| UNLISTEN '*'
			{ $$ = make_str("unlisten *"); }
		;


/*****************************************************************************
 *
 *				Transactions:
 *
 *	  BEGIN / COMMIT / ROLLBACK
 *		(also older versions END / ABORT)
 *
 *****************************************************************************/
TransactionStmt:  ABORT_P opt_transaction		{ $$ = make_str("rollback"); }
		| BEGIN_P opt_transaction		{ $$ = make_str("begin transaction"); }
		| START TRANSACTION transaction_mode_list_or_empty	{ $$ = cat2_str(make_str("start transaction"), $3); }
		| COMMIT opt_transaction		{ $$ = make_str("commit"); }
		| END_P opt_transaction		{ $$ = make_str("commit"); }
		| ROLLBACK opt_transaction		{ $$ = make_str("rollback"); }
		;

opt_transaction: WORK			{ $$ = EMPTY; }
		| TRANSACTION	{ $$ = EMPTY; }
		| /*EMPTY*/		{ $$ = EMPTY; }
		;

transaction_mode_list:
	ISOLATION LEVEL iso_level
	{ $$ = cat2_str(make_str("isolation level"), $3); }
	| transaction_access_mode
	{ $$ = $1; }
	| ISOLATION LEVEL iso_level transaction_access_mode
	{ $$ = cat_str(3, make_str("isolation level"), $3, $4); }
	| transaction_access_mode ISOLATION LEVEL iso_level
	{ $$ = cat_str(3, $1, make_str("isolation level"), $4); }
	;
	 
transaction_mode_list_or_empty:
	transaction_mode_list	{ $$ = $1; }
	| /* EMPTY */		{ $$ = EMPTY; }
	;

transaction_access_mode:
	READ ONLY	{ $$ = make_str("read only"); }
	| READ WRITE 	{ $$ = make_str("read write"); }
	;
	
/*****************************************************************************
 *
 *		QUERY:
 *				define view <viewname> '('target-list ')' [where <quals> ]
 *
 *****************************************************************************/

ViewStmt:  CREATE opt_or_replace VIEW qualified_name opt_column_list AS SelectStmt
			{ $$ = cat_str(7, make_str("create"), $2, make_str("view"), $4, $5, make_str("as"), $7); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				load make_str("filename")
 *
 *****************************************************************************/

LoadStmt:  LOAD file_name
			{ $$ = cat2_str(make_str("load"), $2); }
		;


/*****************************************************************************
 *
 *		CREATE DATABASE
 *
 *
 *****************************************************************************/

CreatedbStmt:  CREATE DATABASE database_name WITH createdb_opt_list
			{ $$ = cat_str(4, make_str("create database"), $3, make_str("with"), $5); }
		| CREATE DATABASE database_name
			{ $$ = cat2_str(make_str("create database"), $3); }
		;

createdb_opt_list:	createdb_opt_item
			{ $$ = $1; }
		| createdb_opt_list createdb_opt_item
			{ $$ = cat2_str($1, $2); }
		;

createdb_opt_item:	LOCATION opt_equal StringConst
			{ $$ = cat_str(3,make_str("location"), $2, $3); }
		| LOCATION opt_equal DEFAULT
			{ $$ = cat_str(3, make_str("location"), $2, make_str("default")); }
		| TEMPLATE opt_equal name
			{ $$ = cat_str(3, make_str("template"), $2, $3); }
		| TEMPLATE opt_equal DEFAULT
			{ $$ = cat_str(3, make_str("template"), $2, make_str("default")); }
		| ENCODING opt_equal PosIntStringConst
			{ $$ = cat_str(3, make_str("encoding"), $2, $3); }
		| ENCODING opt_equal DEFAULT
			{ $$ = cat_str(3, make_str("encoding"), $2, make_str("default")); }
		| OWNER opt_equal name
			{ $$ = cat_str(3, make_str("owner"), $2, $3); }
		| OWNER opt_equal DEFAULT
			{ $$ = cat_str(3, make_str("owner"), $2, make_str("default")); }
		;

opt_equal: '='					{ $$ = make_str("="); }
		| /* EMPTY */			{ $$ = EMPTY; }
		;


/*****************************************************************************
 *
 *				ALTER DATABASE
 *
 *
 *****************************************************************************/

AlterDatabaseSetStmt: ALTER DATABASE database_name SET set_rest
			{ $$ = cat_str(4, make_str("alter database"), $3, make_str("set"), $5); }
		| ALTER DATABASE database_name VariableResetStmt
			{ $$ = cat_str(3, make_str("alter database"), $3, $4); }
		;

/*****************************************************************************
 *
 *		DROP DATABASE
 *
 *
 *****************************************************************************/

DropdbStmt: DROP DATABASE database_name
			{ $$ = cat2_str(make_str("drop database"), $3); }
		;


/*****************************************************************************
 *
 * Manipulate a domain
 *
 *****************************************************************************/

CreateDomainStmt:  CREATE DOMAIN_P any_name opt_as Typename ColQualList opt_collate
			{
				$$ = cat_str(6, make_str("create domain"), $3, $4, $5, $6, $7);
 			}
		;

AlterDomainStmt:
	ALTER DOMAIN_P any_name alter_column_default
		{ $$ = cat_str(3, make_str("alter domain"), $3, $4); }
	| ALTER DOMAIN_P any_name DROP NOT NULL_P
		{ $$ = cat_str(3, make_str("alter domain"), $3, make_str("drop not null")); }
	| ALTER DOMAIN_P any_name SET NOT NULL_P
		{ $$ = cat_str(3, make_str("alter domain"), $3, make_str("set not null")); }
	| ALTER DOMAIN_P any_name ADD TableConstraint
		{ $$ = cat_str(4, make_str("alter domain"), $3, make_str("add"), $5); }
	| ALTER DOMAIN_P any_name DROP CONSTRAINT name opt_drop_behavior
		{ $$ = cat_str(5, make_str("alter domain"), $3, make_str("drop constraint"), $6, $7); }
	| ALTER DOMAIN_P any_name OWNER TO UserId
		{ $$ = cat_str(4, make_str("alter domain"), $3, make_str("owner to"), $6); }
	;
	
opt_as:	AS	{$$ = make_str("as"); }
	| /* EMPTY */	{$$ = EMPTY; }
	;

CreateConversionStmt:
                       CREATE opt_default CONVERSION_P any_name FOR StringConst
                       TO StringConst FROM any_name
		       { $$ = cat_str(10, make_str("create"), $2, make_str("conversion"), $4, make_str("for"), $6, make_str("to"), $8, make_str("from"), $10); }
		       ;

/*****************************************************************************
 *
 *		QUERY:
 *			       cluster <index_name> on <qualified_name>
 *                             cluster <qualified_name>
 *                             cluster
 *
 *****************************************************************************/

ClusterStmt:  CLUSTER index_name ON qualified_name
			{ $$ = cat_str(4, make_str("cluster"), $2, make_str("on"), $4); }
	| CLUSTER qualified_name
			{ $$ = cat2_str(make_str("cluster"), $2); }
	| CLUSTER
			{ $$ = make_str("cluster"); }
	;


/*****************************************************************************
 *
 *		QUERY:
 *				vacuum
 *				analyze
 *
 *****************************************************************************/

VacuumStmt:  VACUUM opt_full opt_freeze opt_verbose
			{ $$ = cat_str(4, make_str("vacuum"), $2, $3, $4); }
		| VACUUM opt_full opt_freeze opt_verbose qualified_name
			{ $$ = cat_str(5, make_str("vacuum"), $2, $3, $4, $5); }
		| VACUUM opt_full opt_freeze opt_verbose AnalyzeStmt
			{ $$ = cat_str(5, make_str("vacuum"), $2, $3, $4, $5); }
		;

AnalyzeStmt:  analyze_keyword opt_verbose
			{ $$ = cat_str(2, $1, $2); }
		| analyze_keyword opt_verbose qualified_name opt_name_list
			{ $$ = cat_str(4, $1, $2, $3, $4); }
		;

analyze_keyword:  ANALYZE		{ $$ = make_str("analyze"); }
		| ANALYSE				{ $$ = make_str("analyse"); }
		;

opt_verbose:  VERBOSE			{ $$ = make_str("verbose"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_full:  FULL					{ $$ = make_str("full"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_freeze:  FREEZE				{ $$ = make_str("freeze"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_name_list:	'(' name_list ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				EXPLAIN query
 *
 *****************************************************************************/

ExplainStmt:  EXPLAIN opt_analyze opt_verbose OptimizableStmt
			{ $$ = cat_str(4, make_str("explain"), $2, $3, $4); }
/*		| EXPLAIN opt_analyze opt_verbose ExecuteStmt
			{ $$ = cat_str(4, make_str("explain"), $2, $3, $4); }*/
		;

opt_analyze:
	analyze_keyword                 { $$ = $1; }
	| /* EMPTY */			{ $$ = EMPTY; }
	;

/*

conflicts with ecpg

PrepareStmt: PREPARE name prep_type_clause AS OptimizableStmt
		{ $$ = cat_str(5, make_str("prepare"), $2, $3, make_str("as"), $5); }
		;

prep_type_clause: '(' prep_type_list ')'	{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
			| /* EMPTY * /		{ $$ = EMPTY; }
			;

prep_type_list: Typename		{ $$ = $1; }
	| prep_type_list ',' Typename	{ $$ = cat_str(3, $1, make_str(","), $3); }
	;

ExecuteStmt: EXECUTE name execute_param_clause into_clause
		{ $$ = cat_str(4, make_str("execute"), $2, $3, $4); }
		;

execute_param_clause: '(' expr_list ')'	{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
			| /* EMPTY * /		{ $$ = EMPTY; }
			;

DeallocateStmt: DEALLOCATE name		{ $$ = cat2_str(make_str("deallocate"), $2); }
	| DEALLOCATE PREPARE name	{ $$ = cat2_str(make_str("deallocate prepare"), $3); }
	;
*/

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
		| DeleteStmt
		;


/*****************************************************************************
 *
 *		QUERY:
 *				INSERT STATEMENTS
 *
 *****************************************************************************/

InsertStmt:  INSERT INTO qualified_name insert_rest
			{ $$ = cat_str(3, make_str("insert into"), $3, $4); }
		;

insert_rest:  VALUES '(' insert_target_list ')'
			{ $$ = cat_str(3, make_str("values("), $3, make_str(")")); }
		| DEFAULT VALUES
			{ $$ = make_str("default values"); }
		| SelectStmt
			{ $$ = $1; }
		| '(' insert_column_list ')' VALUES '(' insert_target_list ')'
			{ $$ = cat_str(5, make_str("("), $2, make_str(") values ("), $6, make_str(")")); }
		| '(' insert_column_list ')' SelectStmt
			{ $$ = cat_str(4, make_str("("), $2, make_str(")"), $4); }
		;

insert_column_list: insert_column_list ',' insert_column_item
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		| insert_column_item
			{ $$ = $1; }
		;

insert_column_item:  ColId opt_indirection
			{ $$ = cat2_str($1, $2); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				DELETE STATEMENTS
 *
 *****************************************************************************/

DeleteStmt:  DELETE_P FROM relation_expr where_clause
			{ $$ = cat_str(3, make_str("delete from"), $3, $4); }
		;

LockStmt:  LOCK_P opt_table qualified_name_list opt_lock
			{ $$ = cat_str(4, make_str("lock"), $2, $3, $4); }
		;

opt_lock:  IN_P lock_type MODE
			{ $$ = cat_str(3, make_str("in"), $2, make_str("mode")); }
		| /*EMPTY*/
			{ $$ = EMPTY;}
		;

lock_type:	ACCESS SHARE		{ $$ = make_str("access share"); }
		| ROW SHARE				{ $$ = make_str("access share"); }
		| ROW EXCLUSIVE			{ $$ = make_str("row exclusive"); }
		| SHARE UPDATE EXCLUSIVE { $$ = make_str("share update exclusive"); }
		| SHARE					{ $$ = make_str("share"); }
		| SHARE ROW EXCLUSIVE	{ $$ = make_str("share row exclusive"); }
		| EXCLUSIVE				{ $$ = make_str("exclusive"); }
		| ACCESS EXCLUSIVE		{ $$ = make_str("access exclusive"); }
		;

/*****************************************************************************
 *
 *		QUERY:
 *				UpdateStmt (UPDATE)
 *
 *****************************************************************************/

UpdateStmt:  UPDATE relation_expr
				SET update_target_list
				from_clause
				where_clause
			{$$ = cat_str(6, make_str("update"), $2, make_str("set"), $4, $5, $6); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				CURSOR STATEMENTS
 *
 *****************************************************************************/
CursorStmt:  DECLARE name opt_cursor CURSOR FOR SelectStmt
		{
			struct cursor *ptr, *this;

			for (ptr = cur; ptr != NULL; ptr = ptr->next)
			{
				if (strcmp($2, ptr->name) == 0)
				{
						/* re-definition is a bug */
					snprintf(errortext, sizeof(errortext), "cursor %s already defined", $2);
					mmerror(PARSE_ERROR, ET_ERROR, errortext);
				}
			}

			this = (struct cursor *) mm_alloc(sizeof(struct cursor));

			/* initial definition */
			this->next = cur;
			this->name = $2;
			this->connection = connection;
			this->command =  cat_str(5, make_str("declare"), mm_strdup($2), $3, make_str("cursor for"), $6);
			this->argsinsert = argsinsert;
			this->argsresult = argsresult;
			argsinsert = argsresult = NULL;

			cur = this;

			$$ = cat_str(3, make_str("/*"), mm_strdup(this->command), make_str("*/"));
		}
		;

opt_cursor:  BINARY				{ $$ = make_str("binary"); }
	   | INSENSITIVE			{ $$ = make_str("insensitive"); }
	   | SCROLL					{ $$ = make_str("scroll"); }
	   | INSENSITIVE SCROLL		{ $$ = make_str("insensitive scroll"); }
	   | /*EMPTY*/				{ $$ = EMPTY; }
	   ;

/*****************************************************************************
 *
 *		QUERY:
 *				SELECT STATEMENTS
 *
 *****************************************************************************/

SelectStmt: select_no_parens		%prec UMINUS
			{ $$ = $1; }
		|	select_with_parens		%prec UMINUS
			{ $$ = $1; }
		;

select_with_parens: '(' select_no_parens ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| '(' select_with_parens ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		;

select_no_parens:	   simple_select
			{ $$ = $1; }
		| select_clause sort_clause
			{ $$ = cat2_str($1, $2); }
		| select_clause opt_sort_clause for_update_clause opt_select_limit
			{ $$ = cat_str(4, $1, $2, $3, $4); }
		| select_clause opt_sort_clause select_limit opt_for_update_clause
			{ $$ = cat_str(4, $1, $2, $3, $4); }
		;

select_clause: simple_select		{ $$ = $1; }
		| select_with_parens		{ $$ = $1; }
		;

simple_select:	SELECT opt_distinct target_list
					into_clause from_clause where_clause
					group_clause having_clause
			{ $$ = cat_str(8, make_str("select"), $2, $3, $4, $5, $6, $7, $8); }
		| select_clause UNION opt_all select_clause
			{ $$ = cat_str(4, $1, make_str("union"), $3, $4); }
		| select_clause INTERSECT opt_all select_clause
			{ $$ = cat_str(4, $1, make_str("intersect"), $3, $4); }
		| select_clause EXCEPT opt_all select_clause
			{ $$ = cat_str(4, $1, make_str("except"), $3, $4); }
		;

into_clause:  INTO OptTempTableName
		{
			FoundInto = 1;
			$$= cat2_str(make_str("into"), $2);
		}
		| ecpg_into			{ $$ = EMPTY; }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

/*
 * Redundancy here is needed to avoid shift/reduce conflicts,
 * since TEMP is not a reserved word.  See also OptTemp.
 *
 * The result is a cons cell (not a true list!) containing
 * a boolean and a table name.
 */
OptTempTableName:  TEMPORARY opt_table qualified_name
			{ $$ = cat_str(3, make_str("temporary"), $2, $3); }
		| TEMP opt_table qualified_name
			{ $$ = cat_str(3, make_str("temp"), $2, $3); }
		| LOCAL TEMPORARY opt_table qualified_name
			{ $$ = cat_str(3, make_str("local temporary"), $3, $4); }
		| LOCAL TEMP opt_table qualified_name
			{ $$ = cat_str(3, make_str("local temp"), $3, $4); }
		| GLOBAL TEMPORARY opt_table qualified_name
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported CREATE TABLE / GLOBAL TEMPORARY will be passed to backend");
			$$ = cat_str(3, make_str("global temporary"), $3, $4);
		}
		| GLOBAL TEMP opt_table qualified_name
		{
			mmerror(PARSE_ERROR, ET_WARNING, "Currently unsupported CREATE TABLE / GLOBAL TEMP will be passed to backend");
			$$ = cat_str(3, make_str("global temp"), $3, $4);
		}
		| TABLE qualified_name
			{ $$ = cat2_str(make_str("table"), $2); }
		| qualified_name
			{ $$ = $1; }
		;

opt_table:	TABLE				{ $$ = make_str("table"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_all:  ALL					{ $$ = make_str("all"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_distinct:  DISTINCT
			{ $$ = make_str("distinct"); }
		| DISTINCT ON '(' expr_list ')'
			{ $$ = cat_str(3, make_str("distinct on ("), $4, make_str(")")); }
		| ALL
			{ $$ = make_str("all"); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

opt_sort_clause:	sort_clause	{ $$ = $1; }
		| /* EMPTY */		{ $$ = EMPTY; }
		;

sort_clause:  ORDER BY sortby_list
			{ $$ = cat2_str(make_str("order by"), $3); }
		;

sortby_list:  sortby					{ $$ = $1; }
		| sortby_list ',' sortby		{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

sortby: a_expr OptUseOp
			{ $$ = cat2_str($1, $2); }
		;

OptUseOp:  USING all_Op		{ $$ = cat2_str(make_str("using"), $2); }
		| ASC				{ $$ = make_str("asc"); }
		| DESC				{ $$ = make_str("desc"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

select_limit:	LIMIT select_limit_value OFFSET select_offset_value
		   { $$ = cat_str(4, make_str("limit"), $2, make_str("offset"), $4); }
		| OFFSET select_offset_value LIMIT select_limit_value
		   { $$ = cat_str(4, make_str("offset"), $2, make_str("limit"), $4); }
		| LIMIT select_limit_value
		   { $$ = cat2_str(make_str("limit"), $2); }
		| OFFSET select_offset_value
		   { $$ = cat2_str(make_str("offset"), $2); }
		| LIMIT select_limit_value ',' select_offset_value
		   { mmerror(PARSE_ERROR, ET_WARNING, "No longer supported LIMIT #,# syntax passed to backend."); }
		;

opt_select_limit:	select_limit	{ $$ = $1; }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;

select_limit_value: PosIntConst
		{
			if (atoi($1) < 0)
				mmerror(PARSE_ERROR, ET_ERROR, "LIMIT must not be negative");
			$$ = $1;
		}
		| ALL	{ $$ = make_str("all"); }
		| PARAM { $$ = make_name(); }
		;

select_offset_value:	PosIntConst
		{
			if (atoi($1) < 0)
				mmerror(PARSE_ERROR, ET_ERROR, "OFFSET must not be negative");
			$$ = $1;
		}
		| PARAM { $$ = make_name(); }
		;

/*
 *	jimmy bell-style recursive queries aren't supported in the
 *	current system.
 *
 *	...however, recursive addattr and rename supported.  make special
 *	cases for these.
 */
group_clause:  GROUP_P BY expr_list
			{ $$ = cat2_str(make_str("group by"), $3); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

having_clause:	HAVING a_expr
			{ $$ = cat2_str(make_str("having"), $2); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

for_update_clause:	FOR UPDATE update_list
			{ $$ = make_str("for update"); }
		| FOR READ ONLY
			{ $$ = make_str("for read only"); }
		;

opt_for_update_clause: for_update_clause	{ $$ = $1; }
		| /* EMPTY */						{ $$ = EMPTY; }
		;

update_list:  OF name_list		{ $$ = cat2_str(make_str("of"), $2); }
		| /* EMPTY */			{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *	clauses common to all Optimizable Stmts:
 *		from_clause - allow list of both JOIN expressions and table names
 *		where_clause	- qualifications for joins or restrictions
 *
 *****************************************************************************/

from_clause:	FROM from_list		{ $$ = cat2_str(make_str("from"), $2); }
		| /* EMPTY */				{ $$ = EMPTY; }
		;

from_list:	from_list ',' table_ref { $$ = cat_str(3, $1, make_str(","), $3); }
		| table_ref					{ $$ = $1; }
		;

/*
 * table_ref is where an alias clause can be attached.	Note we cannot make
 * alias_clause have an empty production because that causes parse conflicts
 * between table_ref := '(' joined_table ')' alias_clause
 * and joined_table := '(' joined_table ')'.  So, we must have the
 * redundant-looking productions here instead.
 */
table_ref:	relation_expr
			{ $$ = $1; }
		| relation_expr alias_clause
			{ $$= cat2_str($1, $2); }
		| func_table
			{ $$ = $1; }
		| func_table alias_clause
		        { $$= cat2_str($1, $2); }
		| func_table AS '(' TableFuncElementList ')'
			{ $$=cat_str(4, $1, make_str("as ("), $4, make_str(")")); }
		| func_table AS ColId '(' TableFuncElementList ')'
			{ $$=cat_str(6, $1, make_str("as"), $3, make_str("("), $5, make_str(")")); }
		| func_table ColId '(' TableFuncElementList ')'
			{ $$=cat_str(5, $1, $2, make_str("("), $4, make_str(")")); }
		| select_with_parens
			{mmerror(PARSE_ERROR, ET_ERROR, "sub-SELECT in FROM must have an alias");}
		| select_with_parens alias_clause
			{ $$=cat2_str($1, $2); }
		| joined_table
			{ $$ = $1; }
		| '(' joined_table ')' alias_clause
			{ $$=cat_str(4, make_str("("), $2, make_str(")"), $4); }
		;

/*
 * It may seem silly to separate joined_table from table_ref, but there is
 * method in SQL92's madness: if you don't do it this way you get reduce-
 * reduce conflicts, because it's not clear to the parser generator whether
 * to expect alias_clause after ')' or not.  For the same reason we must
 * treat 'JOIN' and 'join_type JOIN' separately, rather than allowing
 * join_type to expand to empty; if we try it, the parser generator can't
 * figure out when to reduce an empty join_type right after table_ref.
 *
 * Note that a CROSS JOIN is the same as an unqualified
 * INNER JOIN, and an INNER JOIN/ON has the same shape
 * but a qualification expression to limit membership.
 * A NATURAL JOIN implicitly matches column names between
 * tables and the shape is determined by which columns are
 * in common. We'll collect columns during the later transformations.
 */

joined_table:  '(' joined_table ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| table_ref CROSS JOIN table_ref
			{ $$ = cat_str(3, $1, make_str("cross join"), $4); }
		| table_ref UNIONJOIN table_ref
			{ $$ = cat_str(3, $1, make_str("unionjoin"), $3); }
		| table_ref join_type JOIN table_ref join_qual
			{ $$ = cat_str(5, $1, $2, make_str("join"), $4, $5); }
		| table_ref JOIN table_ref join_qual
			{ $$ = cat_str(4, $1, make_str("join"), $3, $4); }
		| table_ref NATURAL join_type JOIN table_ref
			{ $$ = cat_str(5, $1, make_str("natural"), $3, make_str("join"), $5); }
		| table_ref NATURAL JOIN table_ref
			{ $$ = cat_str(3, $1, make_str("natural join"), $4); }
		;

alias_clause:  AS ColId '(' name_list ')'
			{ $$ = cat_str(5, make_str("as"), $2, make_str("("), $4, make_str(")")); }
		| AS ColId
			{ $$ = cat2_str(make_str("as"), $2); }
		| ColId '(' name_list ')'
			{ $$ = cat_str(4, $1, make_str("("), $3, make_str(")")); }
		| ColId
			{ $$ = $1; }
		;

join_type:	FULL join_outer		{ $$ = cat2_str(make_str("full"), $2); }
		| LEFT join_outer		{ $$ = cat2_str(make_str("left"), $2); }
		| RIGHT join_outer		{ $$ = cat2_str(make_str("right"), $2); }
		| INNER_P				{ $$ = make_str("inner"); }
		;

/* OUTER is just noise... */
join_outer:  OUTER_P			{ $$ = make_str("outer"); }
		| /*EMPTY*/				{ $$ = EMPTY;  /* no qualifiers */ }
		;

/* JOIN qualification clauses
 * Possibilities are:
 *	USING ( column list ) allows only unqualified column names,
 *						  which must match between tables.
 *	ON expr allows more general qualifications.
 */

join_qual:	USING '(' name_list ')'
			{ $$ = cat_str(3, make_str("using ("), $3, make_str(")")); }
		| ON a_expr
			{ $$ = cat2_str(make_str("on"), $2); }
		;

relation_expr:	qualified_name
			{ /* normal relations */ $$ = $1; }
		| qualified_name '*'
			{ /* inheritance query */ $$ = cat2_str($1, make_str("*")); }
		| ONLY qualified_name
			{ /* inheritance query */ $$ = cat2_str(make_str("only "), $2); }
		| ONLY '(' qualified_name ')'
			{ /* inheritance query */ $$ = cat_str(3, make_str("only ("), $3, make_str(")")); }
		;

func_table:  func_name '(' ')'
		{ $$ = cat2_str($1, make_str("()")); }
	| func_name '(' expr_list ')'
		{ $$ = cat_str(4, $1, make_str("("), $3, make_str(")")); }
	;

where_clause:  WHERE a_expr		{ $$ = cat2_str(make_str("where"), $2); }
		| /*EMPTY*/				{ $$ = EMPTY;  /* no qualifiers */ }
		;

TableFuncElementList: TableFuncElement
                        { $$ = $1; }
		| TableFuncElementList ',' TableFuncElement
			{ $$ = cat_str(3, $1, ',', $3); }
		;

TableFuncElement:	ColId Typename	{ $$ = cat2_str($1, $2); }
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
			{ $$ = cat2_str($1, $2.str); }
		| SETOF SimpleTypename opt_array_bounds
			{ $$ = cat_str(3, make_str("setof"), $2, $3); }
		;


opt_array_bounds:  '[' ']' opt_array_bounds
		{
			$$.index1 = make_str("0");
			$$.index2 = $3.index1;
			$$.str = cat2_str(make_str("[]"), $3.str);
		}
		| '[' Iresult ']' opt_array_bounds
		{
			$$.index1 = strdup($2);
			$$.index2 = $4.index1;
			$$.str = cat_str(4, make_str("["), $2, make_str("]"), $4.str);
		}
		| /* EMPTY */
		{
			$$.index1 = make_str("-1");
			$$.index2 = make_str("-1");
			$$.str= EMPTY;
		}
		;

Iresult:	PosIntConst		{ $$ = $1; }
		| '(' Iresult ')'	{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| Iresult '+' Iresult	{ $$ = cat_str(3, $1, make_str("+"), $3); }
		| Iresult '-' Iresult	{ $$ = cat_str(3, $1, make_str("-"), $3); }
		| Iresult '*' Iresult	{ $$ = cat_str(3, $1, make_str("*"), $3); }
		| Iresult '/' Iresult	{ $$ = cat_str(3, $1, make_str("/"), $3); }
		| Iresult '%' Iresult	{ $$ = cat_str(3, $1, make_str("%"), $3); }
		| Sconst		{ $$ = $1; }
		| ColId			{ $$ = $1; }
		;

SimpleTypename:  GenericType		{ $$ = $1; }
		| ConstDatetime		{ $$ = $1; }
		| Numeric		{ $$ = $1; }
		| Bit			{ $$ = $1; }
		| Character		{ $$ = $1; }
		| ConstInterval opt_interval
			{ $$ = cat2_str($1, $2); }
		| ConstInterval '(' PosIntConst ')' opt_interval
			{ $$ = cat_str(5, $1, make_str("("), $3, make_str(")"), $5); }
		| type_name attrs
			{ $$ = cat2_str($1, $2);}
		;

ConstTypename:	GenericType		{ $$ = $1; }
		| ConstDatetime		{ $$ = $1; }
		| Numeric		{ $$ = $1; }
		| ConstBit		{ $$ = $1; }
		| ConstCharacter	{ $$ = $1; }
		;

GenericType:  type_name			{ $$ = $1; }
		;

/* SQL92 numeric data types
 * Check FLOAT() precision limits assuming IEEE floating types.
 * Provide real DECIMAL() and NUMERIC() implementations now - Jan 1998-12-30
 * - thomas 1997-09-18
 */
Numeric:  INT
			{ $$ = make_str("int"); }
		| INTEGER
			{ $$ = make_str("integer"); }
		| SMALLINT
			{ $$ = make_str("smallint"); }
		| BIGINT
			{ $$ = make_str("bigint"); }
		| REAL
			{ $$ = make_str("real"); }
		| FLOAT_P opt_float
			{ $$ = cat2_str(make_str("float"), $2); }
		| DOUBLE PRECISION
			{ $$ = make_str("double precision"); }
		| DECIMAL opt_decimal
			{ $$ = cat2_str(make_str("decimal"), $2); }
		| DEC opt_decimal
			{ $$ = cat2_str(make_str("dec"), $2); }
		| NUMERIC opt_numeric
			{ $$ = cat2_str(make_str("numeric"), $2); }
		| BOOLEAN
			{ $$ = make_str("boolean"); }
		;

opt_float:	'(' PosIntConst ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

opt_numeric:  '(' PosIntConst ',' PosIntConst ')'
			{ $$ = cat_str(5, make_str("("), $2, make_str(","), $4, make_str(")")); }
		| '(' PosIntConst ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

opt_decimal:  '(' PosIntConst ',' PosIntConst ')'
			{ $$ = cat_str(5, make_str("("), $2, make_str(","), $4, make_str(")")); }
		| '(' PosIntConst ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

/*
 * SQL92 bit-field data types
 * The following implements BIT() and BIT VARYING().
 */

Bit:	BitWithLength		{ $$ = $1; }
	| BitWithoutLength	{ $$ = $1; }
	;

ConstBit:	BitWithLength	{ $$ = $1; }
		| BitWithoutLength      { $$ = $1; }
		;

BitWithLength:  BIT opt_varying '(' PosIntConst ')'
			{ $$ = cat_str(5, make_str("bit"), $2, make_str("("), $4, make_str(")")); }
		;

BitWithoutLength: BIT opt_varying
			{ $$ = cat2_str(make_str("bit"), $2); }
		;

/*
 * SQL92 character data types
 * The following implements CHAR() and VARCHAR().
 *								- ay 6/95
 */
Character:	CharacterWithLength		{ $$ = $1; }
		| CharacterWithoutLength	{ $$ = $1; }
		;

ConstCharacter:	CharacterWithLength	{ $$ = $1; }
		| CharacterWithoutLength      { $$ = $1; }
		;

CharacterWithLength: character '(' PosIntConst ')' opt_charset
			{ $$ = cat_str(5, $1, make_str("("), $3, make_str(")"), $5); }
		;

CharacterWithoutLength: character opt_charset
			{ $$ = cat2_str($1, $2); }
		;

character:	CHARACTER opt_varying
			{ $$ = cat2_str(make_str("character"), $2); }
		| CHAR_P opt_varying
			{ $$ = cat2_str(make_str("char"), $2); }
		| VARCHAR
			{ $$ = make_str("varchar"); }
		| NATIONAL CHARACTER opt_varying
			{ $$ = cat2_str(make_str("national character"), $3); }
		| NATIONAL CHAR_P opt_varying
			{ $$ = cat2_str(make_str("national char"), $3); }
		| NCHAR opt_varying
			{ $$ = cat2_str(make_str("nchar"), $2); }
		;

opt_varying:  VARYING
			{ $$ = make_str("varying"); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

opt_charset:  CHARACTER SET ColId
			{ $$ = cat2_str(make_str("character set"), $3); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

opt_collate:  COLLATE ColId
			{ $$ = cat2_str(make_str("collate"), $2); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

ConstDatetime:	TIMESTAMP '(' PosIntConst ')' opt_timezone
			{ $$ = cat_str(4, make_str("timestamp("), $3, make_str(")"), $5); }
		| TIMESTAMP opt_timezone
			{ $$ = cat2_str(make_str("timestamp"), $2); }
		| TIME '(' PosIntConst ')' opt_timezone
			{ $$ = cat_str(4, make_str("time("), $3, make_str(")"), $5); }
		| TIME opt_timezone
			{ $$ = cat2_str(make_str("time"), $2); }
		;

ConstInterval:	INTERVAL
			{ $$ = make_str("interval"); }
		;

opt_timezone:  WITH TIME ZONE
			{ $$ = make_str("with time zone"); }
		| WITHOUT TIME ZONE
			{ $$ = make_str("without time zone"); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

opt_interval:  YEAR_P			{ $$ = make_str("year"); }
		| MONTH_P				{ $$ = make_str("month"); }
		| DAY_P					{ $$ = make_str("day"); }
		| HOUR_P				{ $$ = make_str("hour"); }
		| MINUTE_P				{ $$ = make_str("minute"); }
		| SECOND_P				{ $$ = make_str("second"); }
		| YEAR_P TO MONTH_P		{ $$ = make_str("year to month"); }
		| DAY_P TO HOUR_P		{ $$ = make_str("day to hour"); }
		| DAY_P TO MINUTE_P		{ $$ = make_str("day to minute"); }
		| DAY_P TO SECOND_P		{ $$ = make_str("day to second"); }
		| HOUR_P TO MINUTE_P	{ $$ = make_str("hour to minute"); }
		| MINUTE_P TO SECOND_P	{ $$ = make_str("minute to second"); }
		| HOUR_P TO SECOND_P	{ $$ = make_str("hour to second"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;


/*****************************************************************************
 *
 *	expression grammar
 *
 *****************************************************************************/

/* Expressions using row descriptors
 * Define row_descriptor to allow yacc to break the reduce/reduce conflict
 *	with singleton expressions.
 */
r_expr: row IN_P select_with_parens
			{ $$ = cat_str(3, $1, make_str("in"), $3); }
		| row NOT IN_P select_with_parens
			{ $$ = cat_str(3, $1, make_str("not in"), $4); }
		| row qual_all_Op sub_type select_with_parens %prec Op
			{ $$ = cat_str(4, $1, $2, $3, $4); }
		| row qual_all_Op select_with_parens %prec Op
			{ $$ = cat_str(3, $1, $2, $3); }
		| row qual_all_Op row %prec Op
			{ $$ = cat_str(3, $1, $2, $3); }
		| row IS NULL_P
			{ $$ = cat2_str($1, make_str("is null")); }
		| row IS NOT NULL_P
		        { $$ = cat2_str($1, make_str("is not null")); }
		| row OVERLAPS row
			{ $$ = cat_str(3, $1, make_str("overlaps"), $3); }
		| row IS DISTINCT FROM row %prec IS
		        { $$ = cat_str(3, $1, make_str("is distinct from"), $5); }
		;

row: ROW '(' row_descriptor ')'
		{ $$ = cat_str(3, make_str("row ("), $3, make_str(")")); }
	| ROW '(' a_expr ')'
	        { $$ = cat_str(3, make_str("row ("), $3, make_str(")")); }
	| ROW '(' ')'
		{ $$ = make_str("row()"); }
	| '(' row_descriptor ')'
                { $$ = cat_str(3, make_str("("), $2, make_str(")")); }
	;

row_descriptor:  expr_list ',' a_expr
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

sub_type:  ANY					{ $$ = make_str("ANY"); }
		| SOME					{ $$ = make_str("SOME"); }
		| ALL					{ $$ = make_str("ALL"); }
			  ;

all_Op:  Op 				{ $$ = $1; }
	| MathOp			{ $$ = $1; }
	;

MathOp: '+'				{ $$ = make_str("+"); }
		| '-'			{ $$ = make_str("-"); }
		| '*'			{ $$ = make_str("*"); }
		| '%'			{ $$ = make_str("%"); }
		| '^'			{ $$ = make_str("^"); }
		| '/'			{ $$ = make_str("/"); }
		| '<'			{ $$ = make_str("<"); }
		| '>'			{ $$ = make_str(">"); }
		| '='			{ $$ = make_str("="); }
		;

qual_Op:  Op 				{ $$ = $1; }
		| OPERATOR '(' any_operator ')'	{ $$ = cat_str(3, make_str("operator ("), $3, make_str(")")); }
		;

qual_all_Op:  all_Op 				{ $$ = $1; }
		| OPERATOR '(' any_operator ')'	{ $$ = cat_str(3, make_str("operator ("), $3, make_str(")")); }
		;

/* General expressions
 * This is the heart of the expression syntax.
 *
 * We have two expression types: a_expr is the unrestricted kind, and
 * b_expr is a subset that must be used in some places to avoid shift/reduce
 * conflicts.  For example, we can't do BETWEEN as "BETWEEN a_expr AND a_expr"
 * because that use of AND conflicts with AND as a boolean operator.  So,
 * b_expr is used in BETWEEN and we remove boolean keywords from b_expr.
 *
 * Note that '(' a_expr ')' is a b_expr, so an unrestricted expression can
 * always be used by surrounding it with parens.
 *
 * c_expr is all the productions that are common to a_expr and b_expr;
 * it's factored out just to eliminate redundant coding.
 */

a_expr:  c_expr
			{ $$ = $1; }
		| a_expr TYPECAST Typename
			{ $$ = cat_str(3, $1, make_str("::"), $3); }
		| a_expr AT TIME ZONE c_expr
			{ $$ = cat_str(3, $1, make_str("at time zone"), $5); }
		/*
		 * These operators must be called out explicitly in order to make use
		 * of yacc/bison's automatic operator-precedence handling.  All other
		 * operator names are handled by the generic productions using "Op",
		 * below; and all those operators will have the same precedence.
		 *
		 * If you add more explicitly-known operators, be sure to add them
		 * also to b_expr and to the MathOp list above.
		 */
		| '+' a_expr %prec UMINUS
			{ $$ = cat2_str(make_str("+"), $2); }
		| '-' a_expr %prec UMINUS
			{ $$ = cat2_str(make_str("-"), $2); }
		| '%' a_expr
			{ $$ = cat2_str(make_str("%"), $2); }
		| '^' a_expr
			{ $$ = cat2_str(make_str("^"), $2); }
		| a_expr '%'
			{ $$ = cat2_str($1, make_str("%")); }
		| a_expr '^'
			{ $$ = cat2_str($1, make_str("^")); }
		| a_expr '+' a_expr
			{ $$ = cat_str(3, $1, make_str("+"), $3); }
		| a_expr '-' a_expr
			{ $$ = cat_str(3, $1, make_str("-"), $3); }
		| a_expr '*' a_expr
			{ $$ = cat_str(3, $1, make_str("*"), $3); }
		| a_expr '/' a_expr
			{ $$ = cat_str(3, $1, make_str("/"), $3); }
		| a_expr '%' a_expr
			{ $$ = cat_str(3, $1, make_str("%"), $3); }
		| a_expr '^' a_expr
			{ $$ = cat_str(3, $1, make_str("^"), $3); }
		| a_expr '<' a_expr
			{ $$ = cat_str(3, $1, make_str("<"), $3); }
		| a_expr '>' a_expr
			{ $$ = cat_str(3, $1, make_str(">"), $3); }
		| a_expr '=' a_expr
			{ $$ = cat_str(3, $1, make_str("="), $3); }
		| a_expr qual_Op a_expr		%prec Op
			{ $$ = cat_str(3, $1, $2, $3); }
		| qual_Op a_expr		%prec Op
			{ $$ = cat2_str($1, $2); }
		| a_expr qual_Op		%prec POSTFIXOP
			{ $$ = cat2_str($1, $2); }
		| a_expr AND a_expr
			{ $$ = cat_str(3, $1, make_str("and"), $3); }
		| a_expr OR a_expr
			{ $$ = cat_str(3, $1, make_str("or"), $3); }
		| NOT a_expr
			{ $$ = cat2_str(make_str("not"), $2); }
		| a_expr LIKE a_expr
			{ $$ = cat_str(3, $1, make_str("like"), $3); }
		| a_expr LIKE a_expr ESCAPE a_expr
			{ $$ = cat_str(5, $1, make_str("like"), $3, make_str("escape"), $5); }
		| a_expr NOT LIKE a_expr
			{ $$ = cat_str(3, $1, make_str("not like"), $4); }
		| a_expr NOT LIKE a_expr ESCAPE a_expr
			{ $$ = cat_str(5, $1, make_str("not like"), $4, make_str("escape"), $6); }
		| a_expr ILIKE a_expr
			{ $$ = cat_str(3, $1, make_str("ilike"), $3); }
		| a_expr ILIKE a_expr ESCAPE a_expr
			{ $$ = cat_str(5, $1, make_str("ilike"), $3, make_str("escape"), $5); }
		| a_expr NOT ILIKE a_expr
			{ $$ = cat_str(3, $1, make_str("not ilike"), $4); }
		| a_expr NOT ILIKE a_expr ESCAPE a_expr
			{ $$ = cat_str(5, $1, make_str("not ilike"), $4, make_str("escape"), $6); }
		| a_expr SIMILAR TO a_expr	%prec SIMILAR
			{ $$ = cat_str(3, $1, make_str("similar to"), $4); }
		| a_expr SIMILAR TO a_expr ESCAPE a_expr
			{ $$ = cat_str(5, $1, make_str("similar to"), $4, make_str("escape"), $6); }
		| a_expr NOT SIMILAR TO a_expr	%prec SIMILAR
			{ $$ = cat_str(3, $1, make_str("not similar to"), $5); }
		| a_expr NOT SIMILAR TO a_expr ESCAPE a_expr
			{ $$ = cat_str(5, $1, make_str("not similar to"), $5, make_str("escape"), $7); }
		| a_expr ISNULL
			{ $$ = cat2_str($1, make_str("isnull")); }
		| a_expr IS NULL_P
			{ $$ = cat2_str($1, make_str("is null")); }
		| a_expr NOTNULL
			{ $$ = cat2_str($1, make_str("notnull")); }
		| a_expr IS NOT NULL_P
			{ $$ = cat2_str($1, make_str("is not null")); }
		/* IS TRUE, IS FALSE, etc used to be function calls
		 *	but let's make them expressions to allow the optimizer
		 *	a chance to eliminate them if a_expr is a constant string.
		 * - thomas 1997-12-22
		 *
		 *	Created BooleanTest Node type, and changed handling
		 *	for NULL inputs
		 * - jec 2001-06-18
		 */
		| a_expr IS TRUE_P
			{ $$ = cat2_str($1, make_str("is true")); }
		| a_expr IS NOT TRUE_P
			{ $$ = cat2_str($1, make_str("is not true")); }
		| a_expr IS FALSE_P
			{ $$ = cat2_str($1, make_str("is false")); }
		| a_expr IS NOT FALSE_P
			{ $$ = cat2_str($1, make_str("is not false")); }
		| a_expr IS UNKNOWN
			{ $$ = cat2_str($1, make_str("is unknown")); }
		| a_expr IS NOT UNKNOWN
			{ $$ = cat2_str($1, make_str("is not unknown")); }
		| a_expr IS DISTINCT FROM a_expr %prec IS
			{ $$ = cat_str(3, $1, make_str("is distinct from"), $5); }
		| a_expr IS OF '(' type_list ')' %prec IS
			{ $$ = cat_str(4, $1, make_str("is of ("), $5, make_str(")")); }
		| a_expr IS NOT OF '(' type_list ')' %prec IS
			{ $$ = cat_str(4, $1, make_str("is not of ("), $6, make_str(")")); }
		| a_expr BETWEEN b_expr AND b_expr	%prec BETWEEN
			{ $$ = cat_str(5, $1, make_str("between"), $3, make_str("and"), $5); }
		| a_expr NOT BETWEEN b_expr AND b_expr	%prec BETWEEN
			{ $$ = cat_str(5, $1, make_str("not between"), $4, make_str("and"), $6); }
		| a_expr IN_P in_expr
			{ $$ = cat_str(3, $1, make_str("in"), $3); }
		| a_expr NOT IN_P in_expr
			{ $$ = cat_str(3, $1, make_str("not in"), $4); }
		| a_expr qual_all_Op sub_type select_with_parens %prec Op
			{ $$ = cat_str(4, $1, $2, $3, $4); }
		| UNIQUE select_with_parens %prec Op
			{ $$ = cat2_str(make_str("unique"), $2); }
		| r_expr
			{ $$ = $1; }
		;

/* Restricted expressions
 *
 * b_expr is a subset of the complete expression syntax
 *
 * Presently, AND, NOT, IS and IN are the a_expr keywords that would
 * cause trouble in the places where b_expr is used.  For simplicity, we
 * just eliminate all the boolean-keyword-operator productions from b_expr.
 */
b_expr:  c_expr
			{ $$ = $1; }
		| b_expr TYPECAST Typename
			{ $$ = cat_str(3, $1, make_str("::"), $3); }
		| '-' b_expr %prec UMINUS
			{ $$ = cat2_str(make_str("-"), $2); }
		| '%' b_expr
			{ $$ = cat2_str(make_str("%"), $2); }
		| '^' b_expr
			{ $$ = cat2_str(make_str("^"), $2); }
		| b_expr '%'
			{ $$ = cat2_str($1, make_str("%")); }
		| b_expr '^'
			{ $$ = cat2_str($1, make_str("^")); }
		| b_expr '+' b_expr
			{ $$ = cat_str(3, $1, make_str("+"), $3); }
		| b_expr '-' b_expr
			{ $$ = cat_str(3, $1, make_str("-"), $3); }
		| b_expr '*' b_expr
			{ $$ = cat_str(3, $1, make_str("*"), $3); }
		| b_expr '/' b_expr
			{ $$ = cat_str(3, $1, make_str("/"), $3); }
		| b_expr '%' b_expr
			{ $$ = cat_str(3, $1, make_str("%"), $3); }
		| b_expr '^' b_expr
			{ $$ = cat_str(3, $1, make_str("^"), $3); }
		| b_expr '<' b_expr
			{ $$ = cat_str(3, $1, make_str("<"), $3); }
		| b_expr '>' b_expr
			{ $$ = cat_str(3, $1, make_str(">"), $3); }
		| b_expr '=' b_expr
			{ $$ = cat_str(3, $1, make_str("="), $3); }
		| b_expr Op b_expr
			{ $$ = cat_str(3, $1, $2, $3); }
		| qual_Op b_expr		%prec Op
			{ $$ = cat2_str($1, $2); }
		| b_expr qual_Op		%prec POSTFIXOP
			{ $$ = cat2_str($1, $2); }
		| b_expr IS DISTINCT FROM b_expr %prec IS
			{ $$ = cat_str(3, $1, make_str("is distinct from"), $5); }
		| b_expr IS OF '(' b_expr ')' %prec IS
			{ $$ = cat_str(4, $1, make_str("is of ("), $5, make_str(")")); }
		| b_expr IS NOT OF '(' b_expr ')' %prec IS
			{ $$ = cat_str(4, $1, make_str("is not of ("), $6, make_str(")")); }
		;

/*
 * Productions that can be used in both a_expr and b_expr.
 *
 * Note: productions that refer recursively to a_expr or b_expr mostly
 * cannot appear here.	However, it's OK to refer to a_exprs that occur
 * inside parentheses, such as function arguments; that cannot introduce
 * ambiguity to the b_expr syntax.
 */
c_expr: columnref
			{ $$ = $1;	}
		| AexprConst
			{ $$ = $1;	}
		| PARAM attrs opt_indirection
			{ $$ = cat_str(3, make_str("param"), $2, $3); }
		| '(' a_expr ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| '(' a_expr ')' attrs opt_indirection
			{ $$ = cat_str(5, make_str("("), $2, make_str(")"), $4, $5); }
		| case_expr
			{ $$ = $1; }
		| func_name '(' ')'
			{ $$ = cat2_str($1, make_str("()"));	}
		| func_name '(' expr_list ')'
			{ $$ = cat_str(4, $1, make_str("("), $3, make_str(")"));	}
		| func_name '(' ALL expr_list ')'
			{ $$ = cat_str(4, $1, make_str("( all"), $4, make_str(")"));	}
		| func_name '(' DISTINCT expr_list ')'
			{ $$ = cat_str(4, $1, make_str("( distinct"), $4, make_str(")"));  }
		| func_name '(' '*' ')'
			{ $$ = cat2_str($1, make_str("(*)")); }
		| CURRENT_DATE
			{ $$ = make_str("current_date"); }
		| CURRENT_TIME opt_empty_parentheses
			{ $$ = cat2_str(make_str("current_time"), $2); }
		| CURRENT_TIME '(' PosIntConst ')'
			{ $$ = make_str("current_time"); }
		| CURRENT_TIMESTAMP opt_empty_parentheses
			{ $$ = cat2_str(make_str("current_timestamp"), $2); }
		| CURRENT_TIMESTAMP '(' PosIntConst ')'
			{ $$ = make_str("current_timestamp"); }
		| CURRENT_USER opt_empty_parentheses
			{ $$ = cat2_str(make_str("current_user"), $2); }
		| SESSION_USER opt_empty_parentheses
			{ $$ = cat2_str(make_str("session_user"), $2); }
		| USER opt_empty_parentheses
			{ $$ = cat2_str(make_str("user"), $2); }
		| CAST '(' a_expr AS Typename ')'
			{ $$ = cat_str(5, make_str("cast("), $3, make_str("as"), $5, make_str(")")); }
		| EXTRACT '(' extract_list ')'
			{ $$ = cat_str(3, make_str("extract("), $3, make_str(")")); }
		| OVERLAY '(' overlay_list ')'
			{ $$ = cat_str(3, make_str("overlay("), $3, make_str(")")); }
		| POSITION '(' position_list ')'
			{ $$ = cat_str(3, make_str("position("), $3, make_str(")")); }
		| SUBSTRING '(' substr_list ')'
			{ $$ = cat_str(3, make_str("substring("), $3, make_str(")")); }
		| TREAT '(' a_expr AS Typename ')'
			{ $$ = cat_str(5, make_str("treat("), $3, make_str("as"), $5, make_str(")")); }
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
			{ $$ = cat_str(3, make_str("trim(both"), $4, make_str(")")); }
		| TRIM '(' LEADING trim_list ')'
			{ $$ = cat_str(3, make_str("trim(leading"), $4, make_str(")")); }
		| TRIM '(' TRAILING trim_list ')'
			{ $$ = cat_str(3, make_str("trim(trailing"), $4, make_str(")")); }
		| TRIM '(' trim_list ')'
			{ $$ = cat_str(3, make_str("trim("), $3, make_str(")")); }
		| CONVERT '(' a_expr USING any_name ')'
		 	{ $$ = cat_str(5, make_str("convert("), $3, make_str("using"), $5, make_str(")"));}
		| CONVERT '(' expr_list ')'
			{ $$ = cat_str(3, make_str("convert("), $3, make_str(")")); }
		| select_with_parens	%prec UMINUS
			{ $$ = $1; }
		| EXISTS select_with_parens
			{ $$ = cat2_str(make_str("exists"), $2); }
		| VALUE
			{ $$ = make_str("value"); }
		;
/*
 * This used to use ecpg_expr, but since there is no shift/reduce conflict
 * anymore, we can remove ecpg_expr. - MM
 */
opt_indirection:  '[' a_expr ']' opt_indirection
			{ $$ = cat_str(4, make_str("["), $2, make_str("]"), $4); }
		| '[' a_expr ':' a_expr ']' opt_indirection
			{ $$ = cat_str(6, make_str("["), $2, make_str(":"), $4, make_str("]"), $6); }
		| /* EMPTY */
			{ $$ = EMPTY; }
		;

expr_list:	a_expr
			{ $$ = $1; }
		| expr_list ',' a_expr
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

extract_list:  extract_arg FROM a_expr
			{ $$ = cat_str(3, $1, make_str("from"), $3); }
		| /* EMPTY */
			{ $$ = EMPTY; }
		;

type_list:	type_list ',' Typename
			{ $$ = cat_str(3, $1, ',', $3); }
		| Typename
			{ $$ = $1; }
		;

/* Allow delimited string SCONST in extract_arg as an SQL extension.
 * - thomas 2001-04-12
 */

extract_arg:  ident				{ $$ = $1; }
		| YEAR_P				{ $$ = make_str("year"); }
		| MONTH_P				{ $$ = make_str("month"); }
		| DAY_P					{ $$ = make_str("day"); }
		| HOUR_P				{ $$ = make_str("hour"); }
		| MINUTE_P				{ $$ = make_str("minute"); }
		| SECOND_P				{ $$ = make_str("second"); }
		| StringConst			{ $$ = $1; }
		;

overlay_list:
		a_expr overlay_placing substr_from substr_for
			{ $$ = cat_str(4, $1, 42, $3, $4); }
		| a_expr overlay_placing substr_from
			{ $$ = cat_str(3, $1, $2, $3); }
		;

overlay_placing:
                        PLACING a_expr		{ $$ = cat2_str(make_str("placing"), $2); }
			;

/* position_list uses b_expr not a_expr to avoid conflict with general IN */
position_list:	b_expr IN_P b_expr
			{ $$ = cat_str(3, $1, make_str("in"), $3); }
		| /* EMPTY */
			{ $$ = EMPTY; }
		;

substr_list:  a_expr substr_from substr_for
			{ $$ = cat_str(3, $1, $2, $3); }
		| a_expr substr_for substr_from
			{ $$ = cat_str(3, $1, $2, $3); }
		| a_expr substr_from
			{ $$ = cat2_str($1, $2); }
		| a_expr substr_for
			{ $$ = cat2_str($1, $2); }
		| expr_list
			{ $$ = $1; }
		| /* EMPTY */
			{ $$ = EMPTY; }
		;

substr_from:  FROM a_expr
			{ $$ = cat2_str(make_str("from"), $2); }
		;

substr_for:  FOR a_expr
			{ $$ = cat2_str(make_str("for"), $2); }
		;

trim_list:	a_expr FROM expr_list
			{ $$ = cat_str(3, $1, make_str("from"), $3); }
		| FROM expr_list
			{ $$ = cat2_str(make_str("from"), $2); }
		| expr_list
			{ $$ = $1; }
		;

in_expr:  select_with_parens
			{ $$ = $1; }
		| '(' expr_list ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		;

/* Case clause
 * Define SQL92-style case clause.
 * Allow all four forms described in the standard:
 * - Full specification
 *	CASE WHEN a = b THEN c ... ELSE d END
 * - Implicit argument
 *	CASE a WHEN b THEN c ... ELSE d END
 * - Conditional NULL
 *	NULLIF(x,y)
 *	same as CASE WHEN x = y THEN NULL ELSE x END
 * - Conditional substitution from list, use first non-null argument
 *	COALESCE(a,b,...)
 * same as CASE WHEN a IS NOT NULL THEN a WHEN b IS NOT NULL THEN b ... END
 * - thomas 1998-11-09
 */
case_expr:	CASE case_arg when_clause_list case_default END_P
			{ $$ = cat_str(5, make_str("case"), $2, $3, $4, make_str("end")); }
		| NULLIF '(' a_expr ',' a_expr ')'
			{ $$ = cat_str(5, make_str("nullif("), $3, make_str(","), $5, make_str(")")); }
		| COALESCE '(' expr_list ')'
			{ $$ = cat_str(3, make_str("coalesce("), $3, make_str(")")); }
		;

when_clause_list:  when_clause_list when_clause
			{ $$ = cat2_str($1, $2); }
		| when_clause
			{ $$ = $1; }
		;

when_clause:  WHEN a_expr THEN a_expr
			{ $$ = cat_str(4, make_str("when"), $2, make_str("then"), $4); }
		;

case_default:  ELSE a_expr
			{ $$ = cat2_str(make_str("else"), $2); }
		| /*EMPTY*/
			{ $$ = EMPTY; }
		;

case_arg:  a_expr			{ $$ = $1; }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

columnref: relation_name opt_indirection
		{ $$ = cat2_str($1, $2); }
	| dotted_name opt_indirection
		{ $$ = cat2_str($1, $2); }
	;

dotted_name: relation_name attrs
			{ $$ = cat2_str($1, $2); }
		;

attrs: '.' attr_name
			{ $$ = cat2_str(make_str("."), $2); }
		| '.' '*'
			{ $$ = make_str(".*"); }
		| '.' attr_name attrs
			{ $$ = cat_str(3, make_str("."), $2, $3); }
		;

opt_empty_parentheses: '(' ')'	{ $$ = make_str("()"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;


/*****************************************************************************
 *
 *	target lists
 *
 *****************************************************************************/

/* Target lists as found in SELECT ... and INSERT VALUES ( ... ) */
target_list:  target_list ',' target_el
			{ $$ = cat_str(3, $1, make_str(","), $3);  }
		| target_el
			{ $$ = $1;	}
		;

/* AS is not optional because shift/red conflict with unary ops */
target_el:	a_expr AS ColLabel
			{ $$ = cat_str(3, $1, make_str("as"), $3); }
		| a_expr
			{ $$ = $1; }
		| '*'
			{ $$ = make_str("*"); }
		;

/* Target list as found in UPDATE table SET ... */
update_target_list:  update_target_list ',' update_target_el
			{ $$ = cat_str(3, $1, make_str(","),$3);	}
		| update_target_el
			{ $$ = $1;	}
		| '*'
			{ $$ = make_str("*"); }
		;

update_target_el:  ColId opt_indirection '=' a_expr
			{ $$ = cat_str(4, $1, $2, make_str("="), $4); }
		;

insert_target_list:  insert_target_list ',' insert_target_el
				{	$$ = cat_str(3, $1, make_str(","), $3);  }
		| insert_target_el
				{	$$ = $1;  }
		;

insert_target_el:  target_el	{	$$ = $1;  }
		| DEFAULT	{	$$ = make_str("default"); }
		;


/*****************************************************************************
 *
 *	   Names and constants
 *
 *****************************************************************************/

relation_name:  SpecialRuleRelation	{ $$ = $1; }
		| ColId	                { $$ = $1; }
		;

qualified_name_list:  qualified_name
				{ $$ = $1; }
		| qualified_name_list ',' qualified_name
				{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

qualified_name: relation_name
		{ $$ = $1; }
		| dotted_name
		{ $$ = $1; }
		;

name_list:  name
			{ $$ = $1; }
		| name_list ',' name
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;


name:					ColId			{ $$ = $1; };
database_name:			ColId			{ $$ = $1; };
access_method:			ColId			{ $$ = $1; };
attr_name:				ColId			{ $$ = $1; };
index_name:				ColId			{ $$ = $1; };

file_name:				StringConst		{ $$ = $1; };

/* func_name will soon return a List ... but not yet */
/*
func_name: function_name
			{ $$ = makeList1(makeString($1)); }
		| dotted_name
			{ $$ = $1; }
		;
*/
func_name: function_name
			{ $$ = $1; }
		| dotted_name
			{ $$ = $1; }
		;


/* Constants
 * Include TRUE/FALSE for SQL3 support. - thomas 1997-10-24
 */
AexprConst:  PosAllConst
			{ $$ = $1; }
		| ConstTypename StringConst
			{ $$ = cat2_str($1, $2); }
		| ConstInterval StringConst opt_interval
			{ $$ = cat_str(3, $1, $2, $3); }
		| ConstInterval  '(' PosIntConst ')' StringConst opt_interval
			{ $$ = cat_str(6, $1, make_str("("), $3, make_str(")"), $5, $6); }
		| PARAM opt_indirection
			{ $$ = cat2_str(make_str("param"), $2); }
		| TRUE_P
			{ $$ = make_str("true"); }
		| FALSE_P
			{ $$ = make_str("false"); }
		| NULL_P
			{ $$ = make_str("null"); }
		| civarind
			{ $$ = make_str("?"); }
		;

Iconst:  ICONST				{ $$ = make_name();};
Fconst:  FCONST				{ $$ = make_name();};
Bconst:  BCONST				{ $$ = make_name();};
Xconst:  XCONST				{ $$ = make_name();};
Sconst:  SCONST
		{
			$$ = (char *)mm_alloc(strlen($1) + 3);
			$$[0]='\'';
					strcpy($$+1, $1);
			$$[strlen($1)+2]='\0';
			$$[strlen($1)+1]='\'';
			free($1);
		}
		;

PosIntConst:	Iconst		{ $$ = $1; }
		| civar				{ $$ = make_str("?"); }
		;

IntConst:	PosIntConst		{ $$ = $1; }
		| '-' PosIntConst	{ $$ = cat2_str(make_str("-"), $2); }
		;

StringConst:	Sconst		{ $$ = $1; }
		| civar				{ $$ = make_str("?"); }
		;

PosIntStringConst:	Iconst	{ $$ = $1; }
		| Sconst			{ $$ = $1; }
		| civar				{ $$ = make_str("?"); }
		;

NumConst:	Fconst			{ $$ = $1; }
		| Iconst			{ $$ = $1; }
		| '-' Fconst		{ $$ = cat2_str(make_str("-"), $2); }
		| '-' Iconst		{ $$ = cat2_str(make_str("-"), $2); }
		| civar				{ $$ = make_str("?"); }
		;

AllConst:	Sconst			{ $$ = $1; }
		| NumConst			{ $$ = $1; }
		;

PosAllConst:	Sconst		{ $$ = $1; }
		| Fconst	{ $$ = $1; }
		| Iconst	{ $$ = $1; }
		| Bconst	{ $$ = $1; }
		| Xconst	{ $$ = $1; }
		| civar		{ $$ = make_str("?"); }
		;

UserId:  ColId				{ $$ = $1;};

SpecialRuleRelation:  OLD
		{
			if (!QueryIsRule)
				mmerror(PARSE_ERROR, ET_ERROR, "OLD used in non-rule query");

			$$ = make_str("old");
		}
		| NEW
		{
			if (!QueryIsRule)
				mmerror(PARSE_ERROR, ET_ERROR, "NEW used in non-rule query");

			$$ = make_str("new");
		}
		;

/*
 * and now special embedded SQL stuff
 */

/*
 * the exec sql connect statement: connect to the given database
 */
ECPGConnect: SQL_CONNECT TO connection_target opt_connection_name opt_user
			{ $$ = cat_str(5, $3, make_str(","), $5, make_str(","), $4); }
		| SQL_CONNECT TO DEFAULT
			{ $$ = make_str("NULL,NULL,NULL,\"DEFAULT\""); }
		  /* also allow ORACLE syntax */
		| SQL_CONNECT ora_user
			{ $$ = cat_str(3, make_str("NULL,"), $2, make_str(",NULL")); }
		| DATABASE connection_target
			{ $$ = cat2_str($2, make_str(",NULL,NULL,NULL")); }
		;

connection_target: database_name opt_server opt_port
		{
			/* old style: dbname[@server][:port] */
			if (strlen($2) > 0 && *($2) != '@')
			{
				snprintf(errortext, sizeof(errortext),
						 "Expected '@', found '%s'", $2);
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			$$ = make3_str(make_str("\""), make3_str($1, $2, $3), make_str("\""));
		}
		|  db_prefix ':' server opt_port '/' database_name opt_options
		{
			/* new style: <tcp|unix>:postgresql://server[:port][/dbname] */
			if (strncmp($1, "unix:postgresql", strlen("unix:postgresql")) != 0 && strncmp($1, "tcp:postgresql", strlen("tcp:postgresql")) != 0)
			{
				snprintf(errortext, sizeof(errortext), "only protocols 'tcp' and 'unix' and database type 'postgresql' are supported");
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			if (strncmp($3, "//", strlen("//")) != 0)
			{
				snprintf(errortext, sizeof(errortext), "Expected '://', found '%s'", $3);
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			if (strncmp($1, "unix", strlen("unix")) == 0 &&
				strncmp($3 + strlen("//"), "localhost", strlen("localhost")) != 0 &&
				strncmp($3 + strlen("//"), "127.0.0.1", strlen("127.0.0.1")) != 0)
			{
				snprintf(errortext, sizeof(errortext), "unix domain sockets only work on 'localhost' but not on '%9.9s'", $3 + strlen("//"));
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			$$ = make3_str(make3_str(make_str("\""), $1, make_str(":")), $3, make3_str(make3_str($4, make_str("/"), $6),	$7, make_str("\"")));
		}
		| StringConst
		{
			if ($1[0] == '\"')
				$$ = $1;
			else if (strcmp($1, "?") == 0) /* variable */
			{
				enum ECPGttype type = argsinsert->variable->type->type;

				/* if array see what's inside */
				if (type == ECPGt_array)
					type = argsinsert->variable->type->u.element->type;

				/* handle varchars */
				if (type == ECPGt_varchar)
					$$ = make2_str(mm_strdup(argsinsert->variable->name), make_str(".arr"));
				else
					$$ = mm_strdup(argsinsert->variable->name);
			}
			else
				$$ = make3_str(make_str("\""), $1, make_str("\""));
		}
		;

db_prefix: ident CVARIABLE
		{
			if (strcmp($2, "postgresql") != 0 && strcmp($2, "postgres") != 0)
			{
				snprintf(errortext, sizeof(errortext), "Expected 'postgresql', found '%s'", $2);
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			if (strcmp($1, "tcp") != 0 && strcmp($1, "unix") != 0)
			{
				snprintf(errortext, sizeof(errortext), "Illegal connection type %s", $1);
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			$$ = make3_str($1, make_str(":"), $2);
		}
		;

server: Op server_name
		{
			if (strcmp($1, "@") != 0 && strcmp($1, "//") != 0)
			{
				snprintf(errortext, sizeof(errortext), "Expected '@' or '://', found '%s'", $1);
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			$$ = make2_str($1, $2);
		}
		;

opt_server: server			{ $$ = $1; }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

server_name: ColId					{ $$ = $1; }
		| ColId '.' server_name		{ $$ = make3_str($1, make_str("."), $3); }
		| IP						{ $$ = make_name(); }
		;

opt_port: ':' PosIntConst	{ $$ = make2_str(make_str(":"), $2); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

opt_connection_name: AS connection_target { $$ = $2; }
		| /*EMPTY*/			{ $$ = make_str("NULL"); }
		;

opt_user: USER ora_user		{ $$ = $2; }
		| /*EMPTY*/			{ $$ = make_str("NULL,NULL"); }
		;

ora_user: user_name
			{ $$ = cat2_str($1, make_str(", NULL")); }
		| user_name '/' user_name
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		| user_name SQL_IDENTIFIED BY user_name
			{ $$ = cat_str(3, $1, make_str(","), $4); }
		| user_name USING user_name
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

user_name: UserId
		{
			if ($1[0] == '\"')
				$$ = $1;
			else
				$$ = make3_str(make_str("\""), $1, make_str("\""));
		}
		| StringConst
		{
			if ($1[0] == '\"')
				$$ = $1;
			else if (strcmp($1, "?") == 0) /* variable */
			{
				enum ECPGttype type = argsinsert->variable->type->type;

				/* if array see what's inside */
				if (type == ECPGt_array)
					type = argsinsert->variable->type->u.element->type;

				/* handle varchars */
				if (type == ECPGt_varchar)
					$$ = make2_str(mm_strdup(argsinsert->variable->name), make_str(".arr"));
				else
					$$ = mm_strdup(argsinsert->variable->name);
			}
			else
				$$ = make3_str(make_str("\""), $1, make_str("\""));
		}
		;

char_variable: CVARIABLE
		{
			/* check if we have a char variable */
			struct variable *p = find_variable($1);
			enum ECPGttype type = p->type->type;

			/* if array see what's inside */
			if (type == ECPGt_array)
				type = p->type->u.element->type;

			switch (type)
			{
				case ECPGt_char:
				case ECPGt_unsigned_char:
					$$ = $1;
					break;
				case ECPGt_varchar:
					$$ = make2_str($1, make_str(".arr"));
					break;
				default:
					mmerror(PARSE_ERROR, ET_ERROR, "invalid datatype");
					break;
			}
		}
		;

opt_options: Op ColId
		{
			if (strlen($1) == 0)
				mmerror(PARSE_ERROR, ET_ERROR, "incomplete statement");

			if (strcmp($1, "?") != 0)
			{
				snprintf(errortext, sizeof(errortext), "unrecognised token '%s'", $1);
				mmerror(PARSE_ERROR, ET_ERROR, errortext);
			}

			$$ = make2_str(make_str("?"), $2);
		}
		| /*EMPTY*/ 	{ $$ = EMPTY; }
		;

/*
 * Declare a prepared cursor. The syntax is different from the standard
 * declare statement, so we create a new rule.
 */
ECPGCursorStmt:  DECLARE name opt_cursor CURSOR FOR ident
		{
			struct cursor *ptr, *this;
			struct variable *thisquery = (struct variable *)mm_alloc(sizeof(struct variable));

			for (ptr = cur; ptr != NULL; ptr = ptr->next)
			{
				if (strcmp($2, ptr->name) == 0)
				{
						/* re-definition is a bug */
					snprintf(errortext, sizeof(errortext), "cursor %s already defined", $2);
					mmerror(PARSE_ERROR, ET_ERROR, errortext);
				}
			}

			this = (struct cursor *) mm_alloc(sizeof(struct cursor));

			/* initial definition */
			this->next = cur;
			this->name = $2;
			this->connection = connection;
			this->command =  cat_str(4, make_str("declare"), mm_strdup($2), $3, make_str("cursor for ?"));
			this->argsresult = NULL;

			thisquery->type = &ecpg_query;
			thisquery->brace_level = 0;
			thisquery->next = NULL;
			thisquery->name = (char *) mm_alloc(sizeof("ECPGprepared_statement(\"\")") + strlen($6));
			sprintf(thisquery->name, "ECPGprepared_statement(\"%s\")", $6);

			this->argsinsert = NULL;
			add_variable(&(this->argsinsert), thisquery, &no_indicator);

			cur = this;

			$$ = cat_str(3, make_str("/*"), mm_strdup(this->command), make_str("*/"));
		}
		;

/*
 * the exec sql deallocate prepare command to deallocate a previously
 * prepared statement
 */
ECPGDeallocate: DEALLOCATE PREPARE ident
			{ $$ = cat_str(3, make_str("ECPGdeallocate(__LINE__, \""), $3, make_str("\");")); }
		| DEALLOCATE ident
			{ $$ = cat_str(2, make_str("ECPGdeallocate(__LINE__, \""), $2, make_str("\");")); }
		;

/* 
 * variable decalartion outside exec sql declare block
 */
ECPGVarDeclaration: single_vt_declaration;

single_vt_declaration: type_declaration		{ $$ = $1; }
		| single_var_declaration	{ $$ = $1; }
		;
	
single_var_declaration: storage_declaration
		single_vt_type
		{
			actual_type[struct_level].type_enum = $2.type_enum;
			actual_type[struct_level].type_dimension = $2.type_dimension;
			actual_type[struct_level].type_index = $2.type_index;
			actual_type[struct_level].type_sizeof = $2.type_sizeof;

			/* we do not need the string "varchar" for output */
			/* so replace it with an empty string */
			/* if ($2.type_enum == ECPGt_varchar)
			{
				free($2.type_str);
				$2.type_str=EMPTY;
			}*/
		}
		variable_list ';'
		{
			$$ = cat_str(5, actual_startline[struct_level], $1, $2.type_str, $4, make_str(";\n"));
		}
		;

precision:	NumConst	{ $$ = $1; };

opt_scale:	',' NumConst	{ $$ = $2; }
		| /* EMPTY */	{ $$ = EMPTY; }
		;

ecpg_interval:	opt_interval	{ $$ = $1; }
		| YEAR_P TO MINUTE_P	{ $$ = make_str("year to minute"); }
		| DAY_P TO DAY_P	{ $$ = make_str("day to day"); }
		| MONTH_P TO MONTH_P	{ $$ = make_str("month to month"); }
		;

single_vt_type: common_type
		| ECPGColLabelCommon ecpg_interval
		{
			if (strlen($2) != 0 && strcmp ($1, "datetime") != 0 && strcmp ($1, "interval") != 0)
				mmerror (PARSE_ERROR, ET_ERROR, "Interval specification not allowed here ");
			
			/*
			 * Check for type names that the SQL grammar treats as
			 * unreserved keywords
			 */
			if (strcmp($1, "varchar") == 0)
			{
				$$.type_enum = ECPGt_varchar;
				$$.type_str = EMPTY;
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "float") == 0)
			{
				$$.type_enum = ECPGt_float;
				$$.type_str = make_str("float");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "double") == 0)
			{
				$$.type_enum = ECPGt_double;
				$$.type_str = make_str("double");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "numeric") == 0)
			{
				$$.type_enum = ECPGt_numeric;
				$$.type_str = EMPTY;
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "decimal") == 0)
			{
				$$.type_enum = ECPGt_numeric;
				$$.type_str = EMPTY;
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "date") == 0)
			{
				$$.type_enum = ECPGt_date;
				$$.type_str = make_str("Date");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "timestamp") == 0)
			{
				$$.type_enum = ECPGt_timestamp;
				$$.type_str = make_str("Timestamp");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "datetime") == 0)
			{
				$$.type_enum = ECPGt_timestamp;
				$$.type_str = make_str("Timestamp");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "interval") == 0)
			{
				$$.type_enum = ECPGt_interval;
				$$.type_str = EMPTY;
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else
			{
				/* this is for typedef'ed types */
				struct typedefs *this = get_typedef($1);

				$$.type_str = (this->type->type_enum == ECPGt_varchar) ? EMPTY : mm_strdup(this->name);
				$$.type_enum = this->type->type_enum;
				$$.type_dimension = this->type->type_dimension;
				$$.type_index = this->type->type_index;
				$$.type_sizeof = this->type->type_sizeof;
				struct_member_list[struct_level] = ECPGstruct_member_dup(this->struct_member_list);
			}
		}
		;

/*
 * variable declaration inside exec sql declare block
 */
ECPGDeclaration: sql_startdeclare
		{ fputs("/* exec sql begin declare section */", yyout); }
		var_type_declarations sql_enddeclare
		{
			fprintf(yyout, "%s/* exec sql end declare section */", $3);
			free($3);
			output_line_number();
		}
		;

sql_startdeclare: ecpgstart BEGIN_P DECLARE SQL_SECTION ';' {};

sql_enddeclare: ecpgstart END_P DECLARE SQL_SECTION ';' {};

var_type_declarations:	/*EMPTY*/			{ $$ = EMPTY; }
		| vt_declarations			{ $$ = $1; }
		;

vt_declarations:  var_declaration			{ $$ = $1; }
		| type_declaration			{ $$ = $1; }
		| vt_declarations var_declaration	{ $$ = cat2_str($1, $2); }
		| vt_declarations type_declaration	{ $$ = cat2_str($1, $2); }
		;

variable_declarations:	var_declaration                         { $$ = $1; }
		| variable_declarations var_declaration         { $$ = cat2_str($1, $2); }
		;

type_declaration: S_TYPEDEF
	{
		/* reset this variable so we see if there was */
		/* an initializer specified */
		initializer = 0;
	}
	var_type opt_pointer ECPGColLabelCommon opt_array_bounds ';'
	{
		/* add entry to list */
		struct typedefs *ptr, *this;
		char * dimension = $6.index1;
		char * length = $6.index2;

		if (($3.type_enum == ECPGt_struct ||
		     $3.type_enum == ECPGt_union) &&
		    initializer == 1)
		{
			mmerror(PARSE_ERROR, ET_ERROR, "Initializer not allowed in typedef command");

		}
		else
		{
			for (ptr = types; ptr != NULL; ptr = ptr->next)
			{
				if (strcmp($5, ptr->name) == 0)
				{
			        	/* re-definition is a bug */
					snprintf(errortext, sizeof(errortext), "Type %s already defined", $5);
					mmerror(PARSE_ERROR, ET_ERROR, errortext);
				}
			}

			adjust_array($3.type_enum, &dimension, &length, $3.type_dimension, $3.type_index, *$4?1:0);

			this = (struct typedefs *) mm_alloc(sizeof(struct typedefs));

	        	/* initial definition */
		        this->next = types;
	        	this->name = $5;
			this->type = (struct this_type *) mm_alloc(sizeof(struct this_type));
			this->type->type_enum = $3.type_enum;
			this->type->type_str = mm_strdup($5);
			this->type->type_dimension = dimension; /* dimension of array */
			this->type->type_index = length;    /* lenght of string */
			this->type->type_sizeof = ECPGstruct_sizeof;
			this->struct_member_list = ($3.type_enum == ECPGt_struct || $3.type_enum == ECPGt_union) ?
				struct_member_list[struct_level] : NULL;

			if ($3.type_enum != ECPGt_varchar &&
			    $3.type_enum != ECPGt_char &&
	        	    $3.type_enum != ECPGt_unsigned_char &&
			    atoi(this->type->type_index) >= 0)
				mmerror(PARSE_ERROR, ET_ERROR, "No multi-dimensional array support for simple data types");

			types = this;
		}

		fprintf(yyout, "typedef %s %s %s %s;\n", $3.type_str, *$4?"*":"", $5, $6.str);
		output_line_number();
		$$ = make_str("");
	};

var_declaration: storage_declaration
		var_type
		{
			actual_type[struct_level].type_enum = $2.type_enum;
			actual_type[struct_level].type_dimension = $2.type_dimension;
			actual_type[struct_level].type_index = $2.type_index;
			actual_type[struct_level].type_sizeof = $2.type_sizeof;

			/* we do not need the string "varchar" for output */
			/* so replace it with an empty string */
			/* if ($2.type_enum == ECPGt_varchar)
			{
				free($2.type_str);
				$2.type_str=EMPTY;
			}*/
		}
		variable_list ';'
		{
			$$ = cat_str(5, actual_startline[struct_level], $1, $2.type_str, $4, make_str(";\n"));
		}
		;

storage_declaration: storage_clause storage_modifier
		{
			actual_storage[struct_level] = cat2_str(mm_strdup($1), mm_strdup($2));
			actual_startline[struct_level] = hashline_number();
		}
		;

storage_clause : S_EXTERN		{ $$ = make_str("extern"); }
		| S_STATIC				{ $$ = make_str("static"); }
		| S_REGISTER			{ $$ = make_str("register"); }
		| S_AUTO				{ $$ = make_str("auto"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

storage_modifier : S_CONST		{ $$ = make_str("const"); }
		| S_VOLATILE			{ $$ = make_str("volatile"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

common_type: simple_type
		{
			$$.type_enum = $1;
			$$.type_str = mm_strdup(ECPGtype_name($1));
			$$.type_dimension = make_str("-1");
			$$.type_index = make_str("-1");
			$$.type_sizeof = NULL;
		}
		| struct_type
		{
			$$.type_enum = ECPGt_struct;
			$$.type_str = $1;
			$$.type_dimension = make_str("-1");
			$$.type_index = make_str("-1");
			$$.type_sizeof = ECPGstruct_sizeof;
		}
		| union_type
		{
			$$.type_enum = ECPGt_union;
			$$.type_str = $1;
			$$.type_dimension = make_str("-1");
			$$.type_index = make_str("-1");
			$$.type_sizeof = NULL;
		}
		| enum_type
		{
			$$.type_str = $1;
			$$.type_enum = ECPGt_int;
			$$.type_dimension = make_str("-1");
			$$.type_index = make_str("-1");
			$$.type_sizeof = NULL;
		}
		| ECPGColLabelCommon '(' precision opt_scale ')'
		{
			if (strcmp($1, "numeric") != 0 && strcmp($1, "decimal") != 0)
				mmerror(PARSE_ERROR, ET_ERROR, "Only numeric/decimal have precision/scale argument");
			
			$$.type_enum = ECPGt_numeric;
			$$.type_str = EMPTY;
			$$.type_dimension = make_str("-1");
			$$.type_index = make_str("-1");
			$$.type_sizeof = NULL;
		}
		;

var_type:	common_type
		| ECPGColLabel ecpg_interval
		{
			if (strlen($2) != 0 && strcmp ($1, "datetime") != 0 && strcmp ($1, "interval") != 0)
				mmerror (PARSE_ERROR, ET_ERROR, "Interval specification not allowed here ");
			
			/*
			 * Check for type names that the SQL grammar treats as
			 * unreserved keywords
			 */
			if (strcmp($1, "varchar") == 0)
			{
				$$.type_enum = ECPGt_varchar;
				$$.type_str = EMPTY; /*make_str("varchar");*/
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "float") == 0)
			{
				$$.type_enum = ECPGt_float;
				$$.type_str = make_str("float");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "double") == 0)
			{
				$$.type_enum = ECPGt_double;
				$$.type_str = make_str("double");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "numeric") == 0)
			{
				$$.type_enum = ECPGt_numeric;
				$$.type_str = EMPTY;
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "decimal") == 0)
			{
				$$.type_enum = ECPGt_numeric;
				$$.type_str = EMPTY;
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "date") == 0)
			{
				$$.type_enum = ECPGt_date;
				$$.type_str = make_str("Date");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "timestamp") == 0)
			{
				$$.type_enum = ECPGt_timestamp;
				$$.type_str = make_str("Timestamp");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "interval") == 0)
			{
				$$.type_enum = ECPGt_interval;
				$$.type_str = EMPTY;
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else if (strcmp($1, "datetime") == 0)
			{
				$$.type_enum = ECPGt_timestamp;
				$$.type_str = make_str("Timestamp");
				$$.type_dimension = make_str("-1");
				$$.type_index = make_str("-1");
				$$.type_sizeof = NULL;
			}
			else
			{
				/* this is for typedef'ed types */
				struct typedefs *this = get_typedef($1);

				$$.type_str = (this->type->type_enum == ECPGt_varchar) ? EMPTY : mm_strdup(this->name);
				$$.type_enum = this->type->type_enum;
				$$.type_dimension = this->type->type_dimension;
				$$.type_index = this->type->type_index;
				$$.type_sizeof = this->type->type_sizeof;
				struct_member_list[struct_level] = ECPGstruct_member_dup(this->struct_member_list);
			}
		}
		;

enum_type: SQL_ENUM opt_symbol enum_definition
			{ $$ = cat_str(3, make_str("enum"), $2, $3); }
		|  SQL_ENUM symbol
			{ $$ = cat2_str(make_str("enum"), $2); }
		;

enum_definition: '{' c_list '}'
			{ $$ = cat_str(3, make_str("{"), $2, make_str("}")); };

struct_type: s_struct '{' variable_declarations '}'
		{
			ECPGfree_struct_member(struct_member_list[struct_level]);
			struct_member_list[struct_level] = NULL;
			free(actual_storage[struct_level--]);
			$$ = cat_str(4, $1, make_str("{"), $3, make_str("}"));
		}
		;

union_type: s_union '{' variable_declarations '}'
		{
			ECPGfree_struct_member(struct_member_list[struct_level]);
			struct_member_list[struct_level] = NULL;
			free(actual_storage[struct_level--]);
			$$ = cat_str(4, $1, make_str("{"), $3, make_str("}"));
		}
		;

s_struct: SQL_STRUCT opt_symbol
		{
			struct_member_list[struct_level++] = NULL;
			$$ = cat2_str(make_str("struct"), $2);
			ECPGstruct_sizeof = cat_str(3, make_str("sizeof("), strdup($$), make_str(")"));
			if (struct_level >= STRUCT_DEPTH)
				 mmerror(PARSE_ERROR, ET_ERROR, "Too many levels in nested structure definition");
		}
		;

s_union: UNION opt_symbol
		{
			struct_member_list[struct_level++] = NULL;
			if (struct_level >= STRUCT_DEPTH)
				 mmerror(PARSE_ERROR, ET_ERROR, "Too many levels in nested structure definition");

			$$ = cat2_str(make_str("union"), $2);
		}
		;

simple_type: unsigned_type					{ $$=$1; }
		|	opt_signed signed_type			{ $$=$2; }
		;

unsigned_type: SQL_UNSIGNED SQL_SHORT		{ $$ = ECPGt_unsigned_short; }
		| SQL_UNSIGNED SQL_SHORT INT	{ $$ = ECPGt_unsigned_short; }
		| SQL_UNSIGNED						{ $$ = ECPGt_unsigned_int; }
		| SQL_UNSIGNED INT				{ $$ = ECPGt_unsigned_int; }
		| SQL_UNSIGNED SQL_LONG				{ $$ = ECPGt_unsigned_long; }
		| SQL_UNSIGNED SQL_LONG INT		{ $$ = ECPGt_unsigned_long; }
		| SQL_UNSIGNED SQL_LONG SQL_LONG
		{
#ifdef HAVE_LONG_LONG_INT_64
			$$ = ECPGt_unsigned_long_long;
#else
			$$ = ECPGt_unsigned_long;
#endif
		}
		| SQL_UNSIGNED SQL_LONG SQL_LONG INT
		{
#ifdef HAVE_LONG_LONG_INT_64
			$$ = ECPGt_unsigned_long_long;
#else
			$$ = ECPGt_unsigned_long;
#endif
		}
		| SQL_UNSIGNED CHAR_P			{ $$ = ECPGt_unsigned_char; }
		;

signed_type: SQL_SHORT				{ $$ = ECPGt_short; }
		| SQL_SHORT INT			{ $$ = ECPGt_short; }
		| INT					{ $$ = ECPGt_int; }
		| SQL_LONG					{ $$ = ECPGt_long; }
		| SQL_LONG INT			{ $$ = ECPGt_long; }
		| SQL_LONG SQL_LONG
		{
#ifdef HAVE_LONG_LONG_INT_64
			$$ = ECPGt_long_long;
#else
			$$ = ECPGt_long;
#endif
		}
		| SQL_LONG SQL_LONG INT
		{
#ifdef HAVE_LONG_LONG_INT_64
			$$ = ECPGt_long_long;
#else
			$$ = ECPGt_long;
#endif
		}
		| SQL_BOOL					{ $$ = ECPGt_bool; }
		| CHAR_P					{ $$ = ECPGt_char; }
		;

opt_signed: SQL_SIGNED
		|	/* EMPTY */
		;

variable_list: variable
			{ $$ = $1; }
		| variable_list ',' variable
			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

variable: opt_pointer ECPGColLabelCommon opt_array_bounds opt_initializer
		{
			struct ECPGtype * type;
			char *dimension = $3.index1; /* dimension of array */
			char *length = $3.index2;    /* lenght of string */
			char dim[14L], ascii_len[12];

			adjust_array(actual_type[struct_level].type_enum, &dimension, &length, actual_type[struct_level].type_dimension, actual_type[struct_level].type_index, strlen($1));

			switch (actual_type[struct_level].type_enum)
			{
				case ECPGt_struct:
				case ECPGt_union:
					if (atoi(dimension) < 0)
						type = ECPGmake_struct_type(struct_member_list[struct_level], actual_type[struct_level].type_enum, actual_type[struct_level].type_sizeof);
					else
						type = ECPGmake_array_type(ECPGmake_struct_type(struct_member_list[struct_level], actual_type[struct_level].type_enum, actual_type[struct_level].type_sizeof), dimension);

					$$ = cat_str(4, $1, mm_strdup($2), $3.str, $4);
					break;

				case ECPGt_varchar:
					if (atoi(dimension) < 0)
						type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
					else
						type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

					if (strcmp(dimension, "0") == 0 || abs(atoi(dimension)) == 1)
							*dim = '\0';
					else	
							sprintf(dim, "[%s]", dimension);
					if (strcmp(length, "0") == 0)
						mmerror(PARSE_ERROR, ET_ERROR, "pointer to varchar are not implemented");

					if (strcmp(dimension, "0") == 0)
						$$ = cat_str(7, mm_strdup(actual_storage[struct_level]), make2_str(make_str(" struct varchar_"), mm_strdup($2)), make_str(" { int len; char arr["), mm_strdup(length), make_str("]; } *"), mm_strdup($2), $4);
					else
					   $$ = cat_str(8, mm_strdup(actual_storage[struct_level]), make2_str(make_str(" struct varchar_"), mm_strdup($2)), make_str(" { int len; char arr["), mm_strdup(length), make_str("]; } "), mm_strdup($2), mm_strdup(dim), $4);
					break;

				case ECPGt_char:
				case ECPGt_unsigned_char:
					if (atoi(dimension) == -1)
						type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
					else
						type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

					$$ = cat_str(4, $1, mm_strdup($2), $3.str, $4);
					break;

				case ECPGt_numeric:
					if (atoi(dimension) < 0)
                                                type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
                                        else
                                                type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

					if (atoi(dimension) < 0)
						$$ = cat_str(4, mm_strdup(actual_storage[struct_level]), make_str("Numeric"), mm_strdup($2), $4);
					else
						$$ = cat_str(5, mm_strdup(actual_storage[struct_level]), make_str("Numeric"), mm_strdup($2), mm_strdup(dim), $4);
					break;
				
				case ECPGt_interval:
					if (atoi(dimension) < 0)
                                                type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
                                        else
                                                type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

					if (atoi(dimension) < 0)
						$$ = cat_str(4, mm_strdup(actual_storage[struct_level]), make_str("Interval"), mm_strdup($2), $4);
					else
						$$ = cat_str(5, mm_strdup(actual_storage[struct_level]), make_str("Interval"), mm_strdup($2), mm_strdup(dim), $4);
					break;
					
				default:
					if (atoi(dimension) < 0)
						type = ECPGmake_simple_type(actual_type[struct_level].type_enum, make_str("1"));
					else
						type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, make_str("1")), dimension);

					$$ = cat_str(4, $1, mm_strdup($2), $3.str, $4);
					break;
			}

			if (struct_level == 0)
				new_variable($2, type, braces_open);
			else
				ECPGmake_struct_member($2, type, &(struct_member_list[struct_level - 1]));

			free($2);
		}
		;

opt_initializer: /*EMPTY*/
			{ $$ = EMPTY; }
		| '=' c_term
		{
			initializer = 1;
			$$ = cat2_str(make_str("="), $2);
		}
		;

opt_pointer: /*EMPTY*/				{ $$ = EMPTY; }
		| '*'						{ $$ = make_str("*"); }
		| '*' '*'					{ $$ = make_str("**"); }
		;

/*
 * As long as the prepare statement is not supported by the backend, we will
 * try to simulate it here so we get dynamic SQL
 */
ECPGDeclare: DECLARE STATEMENT ident
		{
			/* this is only supported for compatibility */
			$$ = cat_str(3, make_str("/* declare statement"), $3, make_str("*/"));
		}
		;
/*
 * the exec sql disconnect statement: disconnect from the given database
 */
ECPGDisconnect: SQL_DISCONNECT dis_name { $$ = $2; }
		;

dis_name: connection_object				{ $$ = $1; }
		| SQL_CURRENT						{ $$ = make_str("\"CURRENT\""); }
		| ALL							{ $$ = make_str("\"ALL\""); }
		| /*EMPTY*/						{ $$ = make_str("\"CURRENT\""); }
		;

connection_object: connection_target	{ $$ = $1; }
		| DEFAULT						{ $$ = make_str("\"DEFAULT\""); }
		;

/*
 * execute a given string as sql command
 */
ECPGExecute : EXECUTE IMMEDIATE execstring
		{
			struct variable *thisquery = (struct variable *)mm_alloc(sizeof(struct variable));

			thisquery->type = &ecpg_query;
			thisquery->brace_level = 0;
			thisquery->next = NULL;
			thisquery->name = $3;

			add_variable(&argsinsert, thisquery, &no_indicator);

			$$ = make_str("?");
		}
		| EXECUTE name
		{
			struct variable *thisquery = (struct variable *)mm_alloc(sizeof(struct variable));

			thisquery->type = &ecpg_query;
			thisquery->brace_level = 0;
			thisquery->next = NULL;
			thisquery->name = (char *) mm_alloc(sizeof("ECPGprepared_statement(\"\")") + strlen($2));
			sprintf(thisquery->name, "ECPGprepared_statement(\"%s\")", $2);

			add_variable(&argsinsert, thisquery, &no_indicator);
		}
		opt_ecpg_using opt_ecpg_into
		{
			$$ = make_str("?");
		}
		;

execstring: char_variable
			{ $$ = $1; }
		|	CSTRING
			{ $$ = make3_str(make_str("\""), $1, make_str("\"")); }
		;

/*
 * the exec sql free command to deallocate a previously
 * prepared statement
 */
ECPGFree:	SQL_FREE name	{ $$ = $2; };

/*
 * open is an open cursor, at the moment this has to be removed
 */
ECPGOpen: SQL_OPEN name opt_ecpg_using { $$ = $2; };

opt_ecpg_using: /*EMPTY*/		{ $$ = EMPTY; }
		| USING variablelist
		{
			/* mmerror ("open cursor with variables not implemented yet"); */
			$$ = EMPTY;
		}
		;

opt_sql: /*EMPTY*/ | SQL_SQL;

ecpg_into: INTO into_list
		{
			$$ = EMPTY;
		}
		| INTO opt_sql SQL_DESCRIPTOR quoted_ident_stringvar
		{
			add_variable(&argsresult, descriptor_variable($4,0), &no_indicator);
			$$ = EMPTY;
		}
		;

opt_ecpg_into: /*EMPTY*/			{ $$ = EMPTY; }
		| ecpg_into					{ $$ = $1; }
		;

variable: civarind | civar
		;
variablelist: variable | variable ',' variablelist;

/*
 * As long as the prepare statement is not supported by the backend, we will
 * try to simulate it here so we get dynamic SQL
 */
ECPGPrepare: PREPARE name FROM execstring
			{ $$ = cat2_str(make3_str(make_str("\""), $2, make_str("\",")), $4); }
		;

/*
 * dynamic SQL: descriptor based access
 *	written by Christof Petig <christof.petig@wtal.de>
 */

/*
 * deallocate a descriptor
 */
ECPGDeallocateDescr:	DEALLOCATE SQL_DESCRIPTOR quoted_ident_stringvar
		{
			drop_descriptor($3,connection);
			$$ = $3;
		}
		;

/*
 * allocate a descriptor
 */
ECPGAllocateDescr:	SQL_ALLOCATE SQL_DESCRIPTOR quoted_ident_stringvar
		{
			add_descriptor($3,connection);
			$$ = $3;
		};

/*
 * read from descriptor
 */

ECPGGetDescHeaderItem: CVARIABLE '=' desc_header_item
			{ push_assignment($1, $3); }
		;

desc_header_item:	SQL_COUNT			{ $$ = ECPGd_count; }
		;

ECPGGetDescItem: CVARIABLE '=' descriptor_item	{ push_assignment($1, $3); };

descriptor_item:	SQL_CARDINALITY		{ $$ = ECPGd_cardinality; }
		| SQL_DATA						{ $$ = ECPGd_data; }
		| SQL_DATETIME_INTERVAL_CODE	{ $$ = ECPGd_di_code; }
		| SQL_DATETIME_INTERVAL_PRECISION { $$ = ECPGd_di_precision; }
		| SQL_INDICATOR					{ $$ = ECPGd_indicator; }
		| SQL_KEY_MEMBER				{ $$ = ECPGd_key_member; }
		| SQL_LENGTH					{ $$ = ECPGd_length; }
		| SQL_NAME						{ $$ = ECPGd_name; }
		| SQL_NULLABLE					{ $$ = ECPGd_nullable; }
		| SQL_OCTET_LENGTH				{ $$ = ECPGd_octet; }
		| PRECISION						{ $$ = ECPGd_precision; }
		| SQL_RETURNED_LENGTH			{ $$ = ECPGd_length; }
		| SQL_RETURNED_OCTET_LENGTH		{ $$ = ECPGd_ret_octet; }
		| SQL_SCALE						{ $$ = ECPGd_scale; }
		| TYPE_P						{ $$ = ECPGd_type; }
		;

ECPGGetDescHeaderItems: ECPGGetDescHeaderItem
		| ECPGGetDescHeaderItems ',' ECPGGetDescHeaderItem
		;

ECPGGetDescItems: ECPGGetDescItem
		| ECPGGetDescItems ',' ECPGGetDescItem
		;

ECPGGetDescriptorHeader:	GET SQL_DESCRIPTOR quoted_ident_stringvar
								ECPGGetDescHeaderItems
			{  $$ = $3; }
		;

ECPGGetDescriptor:	GET SQL_DESCRIPTOR quoted_ident_stringvar SQL_VALUE CVARIABLE ECPGGetDescItems
			{  $$.str = $5; $$.name = $3; }
		|	GET SQL_DESCRIPTOR quoted_ident_stringvar SQL_VALUE Iconst ECPGGetDescItems
			{  $$.str = $5; $$.name = $3; }
		;

/*
 * for compatibility with ORACLE we will also allow the keyword RELEASE
 * after a transaction statement to disconnect from the database.
 */

ECPGRelease: TransactionStmt SQL_RELEASE
		{
			if (strcmp($1, "begin") == 0)
							mmerror(PARSE_ERROR, ET_ERROR, "RELEASE does not make sense when beginning a transaction");

			fprintf(yyout, "ECPGtrans(__LINE__, %s, \"%s\");",
					connection ? connection : "NULL", $1);
			whenever_action(0);
			fprintf(yyout, "ECPGdisconnect(__LINE__, %s);",
					connection ? connection : "\"CURRENT\"");
			whenever_action(0);
			free($1);
		}
		;

/*
 * set/reset the automatic transaction mode, this needs a differnet handling
 * as the other set commands
 */
ECPGSetAutocommit:	SET SQL_AUTOCOMMIT '=' on_off	{ $$ = $4; }
		|  SET SQL_AUTOCOMMIT TO on_off   { $$ = $4; }
		;

on_off: ON				{ $$ = make_str("on"); }
		| OFF			{ $$ = make_str("off"); }
		;

/*
 * set the actual connection, this needs a differnet handling as the other
 * set commands
 */
ECPGSetConnection:	SET SQL_CONNECTION TO connection_object { $$ = $4; }
		| SET SQL_CONNECTION '=' connection_object { $$ = $4; }
		| SET SQL_CONNECTION  connection_object { $$ = $3; }
		;

/*
 * define a new type for embedded SQL
 */
ECPGTypedef: TYPE_P
		{
			/* reset this variable so we see if there was */
			/* an initializer specified */
			initializer = 0;
		}
		ColLabel IS var_type opt_array_bounds opt_reference
		{
			/* add entry to list */
			struct typedefs *ptr, *this;
			char *dimension = $6.index1;
			char *length = $6.index2;

			if (($5.type_enum == ECPGt_struct ||
				 $5.type_enum == ECPGt_union) &&
				initializer == 1)
				mmerror(PARSE_ERROR, ET_ERROR, "Initializer not allowed in EXEC SQL TYPE command");
			else
			{
				for (ptr = types; ptr != NULL; ptr = ptr->next)
				{
					if (strcmp($3, ptr->name) == 0)
					{
						/* re-definition is a bug */
						snprintf(errortext, sizeof(errortext), "Type %s already defined", $3);
						mmerror(PARSE_ERROR, ET_ERROR, errortext);
					}
				}

				adjust_array($5.type_enum, &dimension, &length, $5.type_dimension, $5.type_index, *$7?1:0);

				this = (struct typedefs *) mm_alloc(sizeof(struct typedefs));

				/* initial definition */
				this->next = types;
				this->name = $3;
				this->type = (struct this_type *) mm_alloc(sizeof(struct this_type));
				this->type->type_enum = $5.type_enum;
				this->type->type_str = mm_strdup($3);
				this->type->type_dimension = dimension; /* dimension of array */
				this->type->type_index = length;	/* lenght of string */
				this->type->type_sizeof = ECPGstruct_sizeof;
				this->struct_member_list = ($5.type_enum == ECPGt_struct || $5.type_enum == ECPGt_union) ?
					struct_member_list[struct_level] : NULL;

				if ($5.type_enum != ECPGt_varchar &&
					$5.type_enum != ECPGt_char &&
					$5.type_enum != ECPGt_unsigned_char &&
					atoi(this->type->type_index) >= 0)
					mmerror(PARSE_ERROR, ET_ERROR, "No multi-dimensional array support for simple data types");

				types = this;
			}

			if (auto_create_c == false)
				$$ = cat_str(7, make_str("/* exec sql type"), mm_strdup($3), make_str("is"), mm_strdup($5.type_str), mm_strdup($6.str), $7, make_str("*/"));
			else
				$$ = cat_str(6, make_str("typedef "), mm_strdup($5.type_str), *$7?make_str("*"):make_str(""), mm_strdup($6.str), mm_strdup($3), make_str(";"));
		}
		;

opt_reference: SQL_REFERENCE 		{ $$ = make_str("reference"); }
		| /*EMPTY*/		 			{ $$ = EMPTY; }
		;

/*
 * define the type of one variable for embedded SQL
 */
ECPGVar: SQL_VAR
		{
			/* reset this variable so we see if there was */
			/* an initializer specified */
			initializer = 0;
		}
		ColLabel IS var_type opt_array_bounds opt_reference
		{
			struct variable *p = find_variable($3);
			char *dimension = $6.index1;
			char *length = $6.index2;
			struct ECPGtype * type;

			if (($5.type_enum == ECPGt_struct ||
				 $5.type_enum == ECPGt_union) &&
				initializer == 1)
				mmerror(PARSE_ERROR, ET_ERROR, "Initializer not allowed in EXEC SQL VAR command");
			else
			{
				adjust_array($5.type_enum, &dimension, &length, $5.type_dimension, $5.type_index, *$7?1:0);

				switch ($5.type_enum)
				{
					case ECPGt_struct:
					case ECPGt_union:
						if (atoi(dimension) < 0)
							type = ECPGmake_struct_type(struct_member_list[struct_level], $5.type_enum, $5.type_sizeof);
						else
							type = ECPGmake_array_type(ECPGmake_struct_type(struct_member_list[struct_level], $5.type_enum,$5.type_sizeof), dimension);
						break;

					case ECPGt_varchar:
						if (atoi(dimension) == -1)
							type = ECPGmake_simple_type($5.type_enum, length);
						else
							type = ECPGmake_array_type(ECPGmake_simple_type($5.type_enum, length), dimension);
						break;

					case ECPGt_char:
					case ECPGt_unsigned_char:
						if (atoi(dimension) == -1)
							type = ECPGmake_simple_type($5.type_enum, length);
						else
							type = ECPGmake_array_type(ECPGmake_simple_type($5.type_enum, length), dimension);
						break;

					default:
						if (atoi(length) >= 0)
							mmerror(PARSE_ERROR, ET_ERROR, "No multi-dimensional array support for simple data types");

						if (atoi(dimension) < 0)
							type = ECPGmake_simple_type($5.type_enum, make_str("1"));
						else
							type = ECPGmake_array_type(ECPGmake_simple_type($5.type_enum, make_str("1")), dimension);
						break;
				}

				ECPGfree_type(p->type);
				p->type = type;
			}

			$$ = cat_str(7, make_str("/* exec sql var"), mm_strdup($3), make_str("is"), mm_strdup($5.type_str), mm_strdup($6.str), $7, make_str("*/"));
		}
		;

/*
 * whenever statement: decide what to do in case of error/no data found
 * according to SQL standards we lack: SQLSTATE, CONSTRAINT and SQLEXCEPTION
 */
ECPGWhenever: SQL_WHENEVER SQL_SQLERROR action
		{
			when_error.code = $<action>3.code;
			when_error.command = $<action>3.command;
			$$ = cat_str(3, make_str("/* exec sql whenever sqlerror "), $3.str, make_str("; */\n"));
		}
		| SQL_WHENEVER NOT SQL_FOUND action
		{
			when_nf.code = $<action>4.code;
			when_nf.command = $<action>4.command;
			$$ = cat_str(3, make_str("/* exec sql whenever not found "), $4.str, make_str("; */\n"));
		}
		| SQL_WHENEVER SQL_SQLWARNING action
		{
			when_warn.code = $<action>3.code;
			when_warn.command = $<action>3.command;
			$$ = cat_str(3, make_str("/* exec sql whenever sql_warning "), $3.str, make_str("; */\n"));
		}
		;

action : SQL_CONTINUE
		{
			$<action>$.code = W_NOTHING;
			$<action>$.command = NULL;
			$<action>$.str = make_str("continue");
		}
		| SQL_SQLPRINT
		{
			$<action>$.code = W_SQLPRINT;
			$<action>$.command = NULL;
			$<action>$.str = make_str("sqlprint");
		}
		| SQL_STOP
		{
			$<action>$.code = W_STOP;
			$<action>$.command = NULL;
			$<action>$.str = make_str("stop");
		}
		| SQL_GOTO name
		{
			$<action>$.code = W_GOTO;
			$<action>$.command = strdup($2);
			$<action>$.str = cat2_str(make_str("goto "), $2);
		}
		| SQL_GO TO name
		{
			$<action>$.code = W_GOTO;
			$<action>$.command = strdup($3);
			$<action>$.str = cat2_str(make_str("goto "), $3);
		}
		| DO name '(' c_args ')'
		{
			$<action>$.code = W_DO;
			$<action>$.command = cat_str(4, $2, make_str("("), $4, make_str(")"));
			$<action>$.str = cat2_str(make_str("do"), mm_strdup($<action>$.command));
		}
		| DO SQL_BREAK
		{
			$<action>$.code = W_BREAK;
			$<action>$.command = NULL;
			$<action>$.str = make_str("break");
		}
		| SQL_CALL name '(' c_args ')'
		{
			$<action>$.code = W_DO;
			$<action>$.command = cat_str(4, $2, make_str("("), $4, make_str(")"));
			$<action>$.str = cat2_str(make_str("call"), mm_strdup($<action>$.command));
		}
		| SQL_CALL name 
		{
			$<action>$.code = W_DO;
			$<action>$.command = cat_str(3, $2, make_str("("), make_str(")"));
			$<action>$.str = cat2_str(make_str("call"), mm_strdup($<action>$.command));
		}
		;

/* some other stuff for ecpg */

/* additional unreserved keywords */
ECPGKeywords: ECPGKeywords_vanames	{ $$ = $1; }
		| ECPGKeywords_rest 	{ $$ = $1; }
		;

ECPGKeywords_vanames:  SQL_BREAK		{ $$ = make_str("break"); }
		| SQL_CALL				{ $$ = make_str("call"); }
		| SQL_CARDINALITY		{ $$ = make_str("cardinality"); }
		| SQL_CONTINUE			{ $$ = make_str("continue"); }
		| SQL_COUNT				{ $$ = make_str("count"); }
		| SQL_DATA				{ $$ = make_str("data"); }
		| SQL_DATETIME_INTERVAL_CODE	{ $$ = make_str("datetime_interval_code"); }
		| SQL_DATETIME_INTERVAL_PRECISION	{ $$ = make_str("datetime_interval_precision"); }
		| SQL_FOUND				{ $$ = make_str("found"); }
		| SQL_GO				{ $$ = make_str("go"); }
		| SQL_GOTO				{ $$ = make_str("goto"); }
		| SQL_IDENTIFIED		{ $$ = make_str("identified"); }
		| SQL_INDICATOR			{ $$ = make_str("indicator"); }
		| SQL_KEY_MEMBER		{ $$ = make_str("key_member"); }
		| SQL_LENGTH			{ $$ = make_str("length"); }
		| SQL_NAME				{ $$ = make_str("name"); }
		| SQL_NULLABLE			{ $$ = make_str("nullable"); }
		| SQL_OCTET_LENGTH		{ $$ = make_str("octet_length"); }
		| SQL_RELEASE			{ $$ = make_str("release"); }
		| SQL_RETURNED_LENGTH		{ $$ = make_str("returned_length"); }
		| SQL_RETURNED_OCTET_LENGTH { $$ = make_str("returned_octet_length"); }
		| SQL_SCALE				{ $$ = make_str("scale"); }
		| SQL_SECTION			{ $$ = make_str("section"); }
		| SQL_SQLERROR			{ $$ = make_str("sqlerror"); }
		| SQL_SQLPRINT			{ $$ = make_str("sqlprint"); }
		| SQL_SQLWARNING		{ $$ = make_str("sqlwarning"); }
		| SQL_STOP				{ $$ = make_str("stop"); }
		;
		
ECPGKeywords_rest:  SQL_CONNECT			{ $$ = make_str("connect"); }
		| SQL_DISCONNECT		{ $$ = make_str("disconnect"); }
		| SQL_OPEN				{ $$ = make_str("open"); }
		| SQL_VAR				{ $$ = make_str("var"); }
		| SQL_WHENEVER			{ $$ = make_str("whenever"); }
		;

/* additional keywords that can be SQL type names (but not ECPGColLabels) */
ECPGTypeName:  SQL_BOOL				{ $$ = make_str("bool"); }
		| SQL_LONG			{ $$ = make_str("long"); }
		| SQL_SHORT			{ $$ = make_str("short"); }
		| SQL_STRUCT			{ $$ = make_str("struct"); }
		| SQL_SIGNED			{ $$ = make_str("signed"); }
		| SQL_UNSIGNED			{ $$ = make_str("unsigned"); }
		;

opt_symbol: symbol				{ $$ = $1; }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

symbol: ColLabel				{ $$ = $1; }
		;

/*
 * Name classification hierarchy.
 *
 * IDENT is the lexeme returned by the lexer for identifiers that match
 * no known keyword.  In most cases, we can accept certain keywords as
 * names, not only IDENTs.	We prefer to accept as many such keywords
 * as possible to minimize the impact of "reserved words" on programmers.
 * So, we divide names into several possible classes.  The classification
 * is chosen in part to make keywords acceptable as names wherever possible.
 */

/* Column identifier --- names that can be column, table, etc names.
 */
ColId:	ident							{ $$ = $1; }
		| unreserved_keyword			{ $$ = $1; }
		| col_name_keyword				{ $$ = $1; }
		| ECPGKeywords					{ $$ = $1; }
		| CHAR_P						{ $$ = make_str("char"); }
		;

/* Type identifier --- names that can be type names.
 */
type_name:	ident						{ $$ = $1; }
		| unreserved_keyword			{ $$ = $1; }
		| ECPGKeywords					{ $$ = $1; }
		| ECPGTypeName					{ $$ = $1; }
		;

/* Function identifier --- names that can be function names.
 */
function_name:	ident						{ $$ = $1; }
		| unreserved_keyword			{ $$ = $1; }
		| func_name_keyword				{ $$ = $1; }
		| ECPGKeywords					{ $$ = $1; }
		;

/* Column label --- allowed labels in "AS" clauses.
 * This presently includes *all* Postgres keywords.
 */
ColLabel:  ECPGColLabel					{ $$ = $1; }
		| ECPGTypeName					{ $$ = $1; }
		| CHAR_P						{ $$ = make_str("char"); }
		| INT							{ $$ = make_str("int"); }
		| UNION							{ $$ = make_str("union"); }
		;

ECPGColLabelCommon:  ident                              { $$ = $1; }
                | col_name_keyword                      { $$ = $1; }
                | func_name_keyword                     { $$ = $1; }
		| ECPGKeywords_vanames			{ $$ = $1; }
                ;
		
ECPGColLabel:  ECPGColLabelCommon			{ $$ = $1; }
		| unreserved_keyword			{ $$ = $1; }
		| reserved_keyword			{ $$ = $1; }
		| ECPGKeywords_rest			{ $$ = $1; }
		;

/*
 * Keyword classification lists.  Generally, every keyword present in
 * the Postgres grammar should appear in exactly one of these lists.
 *
 * Put a new keyword into the first list that it can go into without causing
 * shift or reduce conflicts.  The earlier lists define "less reserved"
 * categories of keywords.
 */

/* "Unreserved" keywords --- available for use as any kind of name.
 */
unreserved_keyword:
		  ABORT_P					{ $$ = make_str("abort"); }
		| ABSOLUTE						{ $$ = make_str("absolute"); }
		| ACCESS						{ $$ = make_str("access"); }
		| ACTION						{ $$ = make_str("action"); }
		| ADD							{ $$ = make_str("add"); }
		| AFTER							{ $$ = make_str("after"); }
		| AGGREGATE						{ $$ = make_str("aggregate"); }
		| ALTER							{ $$ = make_str("alter"); }
		| ASSERTION						{ $$ = make_str("assertion"); }
		| ASSIGNMENT						{ $$ = make_str("assignment"); }
		| AT							{ $$ = make_str("at"); }
		| BACKWARD						{ $$ = make_str("backward"); }
		| BEFORE						{ $$ = make_str("before"); }
		| BEGIN_P					{ $$ = make_str("begin"); }
		| BY							{ $$ = make_str("by"); }
		| CACHE							{ $$ = make_str("cache"); }
		| CASCADE						{ $$ = make_str("cascade"); }
		| CHAIN							{ $$ = make_str("chain"); }
		| CHARACTERISTICS				{ $$ = make_str("characteristics"); }
		| CHECKPOINT					{ $$ = make_str("checkpoint"); }
		| CLASS							{ $$ = make_str("class"); }
		| CLOSE							{ $$ = make_str("close"); }
		| CLUSTER						{ $$ = make_str("cluster"); }
		| COMMENT						{ $$ = make_str("comment"); }
		| COMMIT						{ $$ = make_str("commit"); }
		| COMMITTED						{ $$ = make_str("committed"); }
		| CONSTRAINTS						{ $$ = make_str("constraints"); }
		| CONVERSION_P						{ $$ = make_str("conversion"); }
		| COPY							{ $$ = make_str("copy"); }
		| CREATEDB						{ $$ = make_str("createdb"); }
		| CREATEUSER					{ $$ = make_str("createuser"); }
		| CURSOR						{ $$ = make_str("cursor"); }
		| CYCLE							{ $$ = make_str("cycle"); }
		| DATABASE						{ $$ = make_str("database"); }
		| DAY_P							{ $$ = make_str("day"); }
		| DEALLOCATE						{ $$ = make_str("deallocate"); }
		| DECLARE						{ $$ = make_str("declare"); }
		| DEFERRED						{ $$ = make_str("deferred"); }
		| DELETE_P						{ $$ = make_str("delete"); }
		| DELIMITER					{ $$ = make_str("delimiter"); }
		| DELIMITERS					{ $$ = make_str("delimiters"); }
		| DOMAIN_P					{ $$ = make_str("domain"); }
		| DOUBLE						{ $$ = make_str("double"); }
		| DROP							{ $$ = make_str("drop"); }
		| EACH							{ $$ = make_str("each"); }
		| ENCODING						{ $$ = make_str("encoding"); }
		| ENCRYPTED						{ $$ = make_str("encrypted"); }
		| ESCAPE						{ $$ = make_str("escape"); }
		| EXCLUSIVE						{ $$ = make_str("exclusive"); }
		| EXECUTE						{ $$ = make_str("execute"); }
		| EXPLAIN						{ $$ = make_str("explain"); }
		| FETCH							{ $$ = make_str("fetch"); }
		| FORCE							{ $$ = make_str("force"); }
		| FORWARD						{ $$ = make_str("forward"); }
		| FUNCTION						{ $$ = make_str("function"); }
		| GLOBAL						{ $$ = make_str("global"); }
		| HANDLER						{ $$ = make_str("handler"); }
		| HOUR_P						{ $$ = make_str("hour"); }
		| IMMEDIATE						{ $$ = make_str("immediate"); }
		| IMMUTABLE						{ $$ = make_str("immutable"); }
		| IMPLICIT_P						{ $$ = make_str("implicit"); }
		| INCREMENT						{ $$ = make_str("increment"); }
		| INDEX							{ $$ = make_str("index"); }
		| INHERITS						{ $$ = make_str("inherits"); }
		| INOUT							{ $$ = make_str("inout"); }
		| INSENSITIVE					{ $$ = make_str("insensitive"); }
		| INSERT						{ $$ = make_str("insert"); }
		| INSTEAD						{ $$ = make_str("instead"); }
		| ISOLATION						{ $$ = make_str("isolation"); }
		| KEY							{ $$ = make_str("key"); }
		| LANCOMPILER						{ $$ = make_str("lancompiler"); }
		| LANGUAGE						{ $$ = make_str("language"); }
		| LEVEL							{ $$ = make_str("level"); }
		| LISTEN						{ $$ = make_str("listen"); }
		| LOAD							{ $$ = make_str("load"); }
		| LOCAL							{ $$ = make_str("local"); }
		| LOCATION						{ $$ = make_str("location"); }
		| LOCK_P						{ $$ = make_str("lock"); }
		| MATCH							{ $$ = make_str("match"); }
		| MAXVALUE						{ $$ = make_str("maxvalue"); }
		| MINUTE_P						{ $$ = make_str("minute"); }
		| MINVALUE						{ $$ = make_str("minvalue"); }
		| MODE							{ $$ = make_str("mode"); }
		| MONTH_P						{ $$ = make_str("month"); }
		| MOVE							{ $$ = make_str("move"); }
		| NAMES							{ $$ = make_str("names"); }
		| NATIONAL						{ $$ = make_str("national"); }
		| NEXT							{ $$ = make_str("next"); }
		| NO							{ $$ = make_str("no"); }
		| NOCREATEDB					{ $$ = make_str("nocreatedb"); }
		| NOCREATEUSER					{ $$ = make_str("nocreateuser"); }
		| NOTHING						{ $$ = make_str("nothing"); }
		| NOTIFY						{ $$ = make_str("notify"); }
		| OF							{ $$ = make_str("of"); }
		| OIDS							{ $$ = make_str("oids"); }
		| OPERATOR						{ $$ = make_str("operator"); }
		| OPTION						{ $$ = make_str("option"); }
		| OUT_P							{ $$ = make_str("out"); }
		| OWNER							{ $$ = make_str("owner"); }
		| PARTIAL						{ $$ = make_str("partial"); }
		| PASSWORD						{ $$ = make_str("password"); }
		| PATH_P						{ $$ = make_str("path"); }
		| PENDANT						{ $$ = make_str("pendant"); }
		| PRECISION						{ $$ = make_str("precision"); }
		| PREPARE						{ $$ = make_str("prepare"); }
		| PRESERVE						{ $$ = make_str("preserver"); }
		| PRIOR							{ $$ = make_str("prior"); }
		| PRIVILEGES					{ $$ = make_str("privileges"); }
		| PROCEDURAL					{ $$ = make_str("procedural"); }
		| PROCEDURE						{ $$ = make_str("procedure"); }
		| READ							{ $$ = make_str("read"); }
		| RECHECK						{ $$ = make_str("recheck"); }
		| REINDEX						{ $$ = make_str("reindex"); }
		| RELATIVE						{ $$ = make_str("relative"); }
		| RENAME						{ $$ = make_str("rename"); }
		| REPLACE						{ $$ = make_str("replace"); }
		| RESET							{ $$ = make_str("reset"); }
		| RESTRICT						{ $$ = make_str("restrict"); }
		| RETURNS						{ $$ = make_str("returns"); }
		| REVOKE						{ $$ = make_str("revoke"); }
		| ROLLBACK						{ $$ = make_str("rollback"); }
		| ROWS							{ $$ = make_str("rows"); }
		| RULE							{ $$ = make_str("rule"); }
		| SCHEMA						{ $$ = make_str("schema"); }
		| SCROLL						{ $$ = make_str("scroll"); }
		| SECOND_P						{ $$ = make_str("second"); }
		| SESSION						{ $$ = make_str("session"); }
		| SEQUENCE						{ $$ = make_str("sequence"); }
		| SERIALIZABLE					{ $$ = make_str("serializable"); }
		| SET							{ $$ = make_str("set"); }
		| SHARE							{ $$ = make_str("share"); }
		| SHOW							{ $$ = make_str("show"); }
		| SIMPLE						{ $$ = make_str("simple"); }
		| STABLE						{ $$ = make_str("stable"); }
		| START							{ $$ = make_str("start"); }
		| STATEMENT						{ $$ = make_str("statement"); }
		| STATISTICS					{ $$ = make_str("statistics"); }
		| STDIN							{ $$ = make_str("stdin"); }
		| STDOUT						{ $$ = make_str("stdout"); }
		| STORAGE						{ $$ = make_str("storage"); }
		| SYSID							{ $$ = make_str("sysid"); }
		| TEMP							{ $$ = make_str("temp"); }
		| TEMPLATE						{ $$ = make_str("template"); }
		| TEMPORARY						{ $$ = make_str("temporary"); }
		| TOAST							{ $$ = make_str("toast"); }
		| TRANSACTION					{ $$ = make_str("transaction"); }
		| TRIGGER						{ $$ = make_str("trigger"); }
		| TRUNCATE						{ $$ = make_str("truncate"); }
		| TRUSTED						{ $$ = make_str("trusted"); }
		| TYPE_P						{ $$ = make_str("type"); }
		| UNENCRYPTED					{ $$ = make_str("unencrypted"); }
		| UNKNOWN						{ $$ = make_str("unknown"); }
		| UNLISTEN						{ $$ = make_str("unlisten"); }
		| UNTIL							{ $$ = make_str("until"); }
		| UPDATE						{ $$ = make_str("update"); }
		| USAGE							{ $$ = make_str("usage"); }
		| VACUUM						{ $$ = make_str("vacuum"); }
		| VALID							{ $$ = make_str("valid"); }
		| VALUES						{ $$ = make_str("values"); }
		| VARYING						{ $$ = make_str("varying"); }
		| VERSION						{ $$ = make_str("version"); }
		| VIEW							{ $$ = make_str("view"); }
		| WITH							{ $$ = make_str("with"); }
		| WITHOUT						{ $$ = make_str("without"); }
		| WRITE  						{ $$ = make_str("write"); }
		| WORK							{ $$ = make_str("work"); }
		| YEAR_P						{ $$ = make_str("year"); }
		| ZONE							{ $$ = make_str("zone"); }
		;

/* Column identifier --- keywords that can be column, table, etc names.
 *
 * Many of these keywords will in fact be recognized as type or function
 * names too; but they have special productions for the purpose, and so
 * can't be treated as "generic" type or function names.
 *
 * The type names appearing here are not usable as function names
 * because they can be followed by '(' in typename productions, which
 * looks too much like a function call for an LR(1) parser.
 */
col_name_keyword:
		BIGINT			{ $$ = make_str("bigint");}
		| BIT			{ $$ = make_str("bit"); }
/* CHAR must be excluded from ECPGColLabel because of conflict with UNSIGNED
		| CHAR_P		{ $$ = make_str("char"); }
 */
		| CHARACTER		{ $$ = make_str("character"); }
		| COALESCE		{ $$ = make_str("coalesce"); }
		| CONVERT		{ $$ = make_str("convert"); }
		| DEC			{ $$ = make_str("dec"); }
		| DECIMAL		{ $$ = make_str("decimal"); }
		| EXISTS		{ $$ = make_str("exists"); }
		| EXTRACT		{ $$ = make_str("extract"); }
		| FLOAT_P		{ $$ = make_str("float"); }
/* INT must be excluded from ECPGColLabel because of conflict
		| INT			{ $$ = make_str("int"); }
 */
		| INTEGER 		{ $$ = make_str("integer"); }
		| INTERVAL		{ $$ = make_str("interval"); }
		| NCHAR			{ $$ = make_str("nchar"); }
		| NONE			{ $$ = make_str("none"); }
		| NULLIF		{ $$ = make_str("nullif"); }
		| NUMERIC		{ $$ = make_str("numeric"); }
		| POSITION		{ $$ = make_str("position"); }
		| REAL			{ $$ = make_str("real"); }
		| ROW 			{ $$ = make_str("row"); }
		| SETOF			{ $$ = make_str("setof"); }
		| SMALLINT		{ $$ = make_str("smallint"); }
		| SUBSTRING		{ $$ = make_str("substring"); }
		| TIME			{ $$ = make_str("time"); }
		| TIMESTAMP		{ $$ = make_str("timestamp"); }
		| TREAT    		{ $$ = make_str("treat"); }
		| TRIM			{ $$ = make_str("trim"); }
		| VARCHAR		{ $$ = make_str("varchar"); }
		;

/* Function identifier --- keywords that can be function names.
 *
 * Most of these are keywords that are used as operators in expressions;
 * in general such keywords can't be column names because they would be
 * ambiguous with variables, but they are unambiguous as function identifiers.
 *
 * Do not include POSITION, SUBSTRING, etc here since they have explicit
 * productions in a_expr to support the goofy SQL9x argument syntax.
 *	- thomas 2000-11-28
 */
func_name_keyword:
		  AUTHORIZATION					{ $$ = make_str("authorization"); }
		| BETWEEN						{ $$ = make_str("between"); }
		| BINARY						{ $$ = make_str("binary"); }
		| CROSS							{ $$ = make_str("cross"); }
		| FREEZE						{ $$ = make_str("freeze"); }
		| FULL							{ $$ = make_str("full"); }
		| ILIKE							{ $$ = make_str("ilike"); }
		| IN_P							{ $$ = make_str("in"); }
		| INNER_P						{ $$ = make_str("inner"); }
		| IS							{ $$ = make_str("is"); }
		| ISNULL						{ $$ = make_str("isnull"); }
		| JOIN							{ $$ = make_str("join"); }
		| LEFT							{ $$ = make_str("left"); }
		| LIKE							{ $$ = make_str("like"); }
		| NATURAL						{ $$ = make_str("natural"); }
		| NOTNULL						{ $$ = make_str("notnull"); }
		| OUTER_P						{ $$ = make_str("outer"); }
		| OVERLAPS						{ $$ = make_str("overlaps"); }
		| RIGHT							{ $$ = make_str("right"); }
		| SIMILAR						{ $$ = make_str("similar"); }
		| VERBOSE						{ $$ = make_str("verbose"); }
		;

/* Reserved keyword --- these keywords are usable only as a ColLabel.
 *
 * Keywords appear here if they could not be distinguished from variable,
 * type, or function names in some contexts.  Don't put things here unless
 * forced to.
 */
reserved_keyword:
		  ALL							{ $$ = make_str("all"); }
		| ANALYSE						{ $$ = make_str("analyse"); } /* British */
		| ANALYZE						{ $$ = make_str("analyze"); }
		| AND							{ $$ = make_str("and"); }
		| ANY							{ $$ = make_str("any"); }
		| AS							{ $$ = make_str("as"); }
		| ASC							{ $$ = make_str("asc"); }
		| BOTH							{ $$ = make_str("both"); }
		| CASE							{ $$ = make_str("case"); }
		| CAST							{ $$ = make_str("cast"); }
		| CHECK							{ $$ = make_str("check"); }
		| COLLATE						{ $$ = make_str("collate"); }
		| COLUMN						{ $$ = make_str("column"); }
		| CONSTRAINT					{ $$ = make_str("constraint"); }
		| CREATE						{ $$ = make_str("create"); }
		| CURRENT_DATE					{ $$ = make_str("current_date"); }
		| CURRENT_TIME					{ $$ = make_str("current_time"); }
		| CURRENT_TIMESTAMP				{ $$ = make_str("current_timestamp"); }
		| CURRENT_USER					{ $$ = make_str("current_user"); }
		| DEFAULT						{ $$ = make_str("default"); }
		| DEFERRABLE					{ $$ = make_str("deferrable"); }
		| DESC							{ $$ = make_str("desc"); }
		| DISTINCT						{ $$ = make_str("distinct"); }
		| DO							{ $$ = make_str("do"); }
		| ELSE							{ $$ = make_str("else"); }
		| END_P						{ $$ = make_str("end"); }
		| EXCEPT						{ $$ = make_str("except"); }
		| FALSE_P						{ $$ = make_str("false"); }
		| FOR							{ $$ = make_str("for"); }
		| FOREIGN						{ $$ = make_str("foreign"); }
		| FROM							{ $$ = make_str("from"); }
		| GRANT							{ $$ = make_str("grant"); }
		| GROUP_P						{ $$ = make_str("group"); }
		| HAVING						{ $$ = make_str("having"); }
		| INITIALLY						{ $$ = make_str("initially"); }
		| INTERSECT						{ $$ = make_str("intersect"); }
		| INTO							{ $$ = make_str("into"); }
		| LEADING						{ $$ = make_str("leading"); }
		| LIMIT							{ $$ = make_str("limit"); }
		| NEW							{ $$ = make_str("new"); }
		| NOT							{ $$ = make_str("not"); }
		| NULL_P						{ $$ = make_str("null"); }
		| OFF							{ $$ = make_str("off"); }
		| OFFSET						{ $$ = make_str("offset"); }
		| OLD							{ $$ = make_str("old"); }
		| ON							{ $$ = make_str("on"); }
		| ONLY							{ $$ = make_str("only"); }
		| OR							{ $$ = make_str("or"); }
		| ORDER							{ $$ = make_str("order"); }
		| PRIMARY						{ $$ = make_str("primary"); }
		| REFERENCES					{ $$ = make_str("references"); }
		| SELECT						{ $$ = make_str("select"); }
		| SESSION_USER					{ $$ = make_str("session_user"); }
		| SOME							{ $$ = make_str("some"); }
		| TABLE							{ $$ = make_str("table"); }
		| THEN							{ $$ = make_str("then"); }
		| TO							{ $$ = make_str("to"); }
		| TRAILING						{ $$ = make_str("trailing"); }
		| TRUE_P						{ $$ = make_str("true"); }
/* UNION must be excluded from ECPGColLabel because of conflict with s_union
		| UNION							{ $$ = make_str("union"); }
 */
		| UNIQUE						{ $$ = make_str("unique"); }
		| USER							{ $$ = make_str("user"); }
		| USING							{ $$ = make_str("using"); }
		| WHEN							{ $$ = make_str("when"); }
		| WHERE							{ $$ = make_str("where"); }
		;


into_list : coutputvariable | into_list ',' coutputvariable
		;

ecpgstart: SQL_START	{ reset_variables(); }
		;

c_args: /*EMPTY*/		{ $$ = EMPTY; }
		| c_list		{ $$ = $1; }
		;

coutputvariable: CVARIABLE indicator
			{ add_variable(&argsresult, find_variable($1), find_variable($2)); }
		| CVARIABLE
			{ add_variable(&argsresult, find_variable($1), &no_indicator); }
		;


civarind: CVARIABLE indicator
		{
			if ($2 != NULL && (find_variable($2))->type->type == ECPGt_array)
				mmerror(PARSE_ERROR, ET_ERROR, "arrays of indicators are not allowed on input");

			add_variable(&argsinsert, find_variable($1), ($2 == NULL) ? &no_indicator : find_variable($2));
		}
		;

civar: CVARIABLE
		{
			add_variable(&argsinsert, find_variable($1), &no_indicator);
			$$ = $1;
		}
		;

indicator: CVARIABLE				{ check_indicator((find_variable($1))->type); $$ = $1; }
		| SQL_INDICATOR CVARIABLE	{ check_indicator((find_variable($2))->type); $$ = $2; }
		| SQL_INDICATOR name		{ check_indicator((find_variable($2))->type); $$ = $2; }
		;

ident: IDENT						{ $$ = $1; }
		| CSTRING					{ $$ = make3_str(make_str("\""), $1, make_str("\"")); }
		;

quoted_ident_stringvar: name
			{ $$ = make3_str(make_str("\""), $1, make_str("\"")); }
		| char_variable
			{ $$ = make3_str(make_str("("), $1, make_str(")")); }
		;

/*
 * C stuff
 */

c_stuff_item: c_anything			{ $$ = $1; }
		| '(' ')'			{ $$ = make_str("()"); }
		| '(' c_stuff ')'
			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		;

c_stuff: c_stuff_item	{ $$ = $1; }
		| c_stuff c_stuff_item
			{ $$ = cat2_str($1, $2); }
		;

c_list: c_term				{ $$ = $1; }
		| c_list ',' c_term	{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

c_term:  c_stuff			{ $$ = $1; }
		| '{' c_list '}'	{ $$ = cat_str(3, make_str("{"), $2, make_str("}")); }
		;

c_thing:	c_anything	{ $$ = $1; }
		|	'('	{ $$ = make_str("("); }
		|	')'	{ $$ = make_str(")"); }
		|	','	{ $$ = make_str(","); }
		|	';'	{ $$ = make_str(";"); }
		;

c_anything:  IDENT					{ $$ = $1; }
		| CSTRING					{ $$ = make3_str(make_str("\""), $1, make_str("\"")); }
		| PosIntConst				{ $$ = $1; }
		| Fconst					{ $$ = $1; }
		| Sconst					{ $$ = $1; }
		| '*'						{ $$ = make_str("*"); }
		| '+'						{ $$ = make_str("+"); }
		| '-'						{ $$ = make_str("-"); }
		| '/'						{ $$ = make_str("/"); }
		| '%'						{ $$ = make_str("%"); }
		| NULL_P					{ $$ = make_str("NULL"); }
		| S_ADD						{ $$ = make_str("+="); }
		| S_AND						{ $$ = make_str("&&"); }
		| S_ANYTHING				{ $$ = make_name(); }
		| S_AUTO					{ $$ = make_str("auto"); }
		| S_CONST					{ $$ = make_str("const"); }
		| S_DEC						{ $$ = make_str("--"); }
		| S_DIV						{ $$ = make_str("/="); }
		| S_DOTPOINT				{ $$ = make_str(".*"); }
		| S_EQUAL					{ $$ = make_str("=="); }
		| S_EXTERN					{ $$ = make_str("extern"); }
		| S_INC						{ $$ = make_str("++"); }
		| S_LSHIFT					{ $$ = make_str("<<"); }
		| S_MEMBER					{ $$ = make_str("->"); }
		| S_MEMPOINT				{ $$ = make_str("->*"); }
		| S_MOD						{ $$ = make_str("%="); }
		| S_MUL						{ $$ = make_str("*="); }
		| S_NEQUAL					{ $$ = make_str("!="); }
		| S_OR						{ $$ = make_str("||"); }
		| S_REGISTER				{ $$ = make_str("register"); }
		| S_RSHIFT					{ $$ = make_str(">>"); }
		| S_STATIC					{ $$ = make_str("static"); }
		| S_SUB						{ $$ = make_str("-="); }
		| S_TYPEDEF				{ $$ = make_str("typedef"); }
		| SQL_BOOL					{ $$ = make_str("bool"); }
		| SQL_ENUM					{ $$ = make_str("enum"); }
		| INT					{ $$ = make_str("int"); }
		| SQL_LONG					{ $$ = make_str("long"); }
		| SQL_SHORT					{ $$ = make_str("short"); }
		| SQL_SIGNED				{ $$ = make_str("signed"); }
		| SQL_STRUCT				{ $$ = make_str("struct"); }
		| SQL_UNSIGNED				{ $$ = make_str("unsigned"); }
		| CHAR_P					{ $$ = make_str("char"); }
		| DOUBLE					{ $$ = make_str("double"); }
		| FLOAT_P					{ $$ = make_str("float"); }
		| UNION						{ $$ = make_str("union"); }
		| VARCHAR					{ $$ = make_str("varchar"); }
		| '['						{ $$ = make_str("["); }
		| ']'						{ $$ = make_str("]"); }
		| '='						{ $$ = make_str("="); }
		;

%%

void yyerror( char * error)
{
	char buf[1024];

	snprintf(buf,sizeof buf,"%s at or near \"%s\"", error, token_start ? token_start : yytext);
	buf[sizeof(buf)-1]=0;
	mmerror(PARSE_ERROR, ET_ERROR, buf);
}

#include "pgc.c"
