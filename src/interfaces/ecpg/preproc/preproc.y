/* Copyright comment */
%{
#include <stdarg.h>

#include "postgres.h"
#include "access/htup.h"
#include "catalog/catname.h"
#include "utils/numeric.h"
#include "utils/memutils.h"
#include "storage/bufpage.h"

#include "extern.h"

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"
#endif

#define STRUCT_DEPTH 128
#define EMPTY make_str("")

/*
 * Variables containing simple states.
 */
int	struct_level = 0;
char	errortext[128];
static char	*connection = NULL;
static int      QueryIsRule = 0, ForUpdateNotAllowed = 0, FoundInto = 0;
static struct this_type actual_type[STRUCT_DEPTH];
static char     *actual_storage[STRUCT_DEPTH];

/* temporarily store struct members while creating the data structure */
struct ECPGstruct_member *struct_member_list[STRUCT_DEPTH] = { NULL };

struct ECPGtype ecpg_no_indicator = {ECPGt_NO_INDICATOR, 0L, {NULL}};
struct variable no_indicator = {"no_indicator", &ecpg_no_indicator, 0, NULL};

struct ECPGtype ecpg_query = {ECPGt_char_variable, 0L, {NULL}};

enum errortype {ET_WARN, ET_ERROR, ET_FATAL};

/*
 * Handle parsing errors and warnings
 */
static void
mmerror(enum errortype type, char * error)
{

    switch(type)
    {
	case ET_WARN: 
		fprintf(stderr, "%s:%d: WARNING: %s\n", input_filename, yylineno, error); 
		break;
	case ET_ERROR:
		fprintf(stderr, "%s:%d: ERROR: %s\n", input_filename, yylineno, error);
		ret_value = PARSE_ERROR;
		break;
	case ET_FATAL:
		fprintf(stderr, "%s:%d: ERROR: %s\n", input_filename, yylineno, error);
		exit(PARSE_ERROR);
    }
}

/*
 * Handle the filename and line numbering.
 */
char * input_filename = NULL;

void
output_line_number()
{
    if (input_filename)
       fprintf(yyout, "\n#line %d \"%s\"\n", yylineno + 1, input_filename);
}

static void
output_simple_statement(char *cmd)
{
	fputs(cmd, yyout);
	output_line_number();
        free(cmd);
}

/*
 * store the whenever action here
 */
static struct when when_error, when_nf, when_warn;

static void
print_action(struct when *w)
{
	switch (w->code)
	{
		case W_SQLPRINT: fprintf(yyout, "sqlprint();");
                                 break;
		case W_GOTO:	 fprintf(yyout, "goto %s;", w->command);
				 break;
		case W_DO:	 fprintf(yyout, "%s;", w->command);
				 break;
		case W_STOP:	 fprintf(yyout, "exit (1);");
				 break;
		case W_BREAK:	 fprintf(yyout, "break;");
				 break;
		default:	 fprintf(yyout, "{/* %d not implemented yet */}", w->code);
				 break;
	}
}

static void
whenever_action(int mode)
{
	if ((mode&1) ==  1 && when_nf.code != W_NOTHING)
	{
		output_line_number();
		fprintf(yyout, "\nif (sqlca.sqlcode == ECPG_NOT_FOUND) ");
		print_action(&when_nf);
	}
	if (when_warn.code != W_NOTHING)
        {
		output_line_number();
                fprintf(yyout, "\nif (sqlca.sqlwarn[0] == 'W') ");
		print_action(&when_warn);
        }
	if (when_error.code != W_NOTHING)
        {
		output_line_number();
                fprintf(yyout, "\nif (sqlca.sqlcode < 0) ");
		print_action(&when_error);
        }
	if ((mode&2) == 2)
		fputc('}', yyout);
	output_line_number();
}

/*
 * Handling of variables.
 */

/*
 * brace level counter
 */
int braces_open;

static struct variable * allvariables = NULL;

static struct variable *
new_variable(const char * name, struct ECPGtype * type)
{
    struct variable * p = (struct variable*) mm_alloc(sizeof(struct variable));

    p->name = mm_strdup(name);
    p->type = type;
    p->brace_level = braces_open;

    p->next = allvariables;
    allvariables = p;

    return(p);
}

static struct variable * find_variable(char * name);

static struct variable *
find_struct_member(char *name, char *str, struct ECPGstruct_member *members)
{
    char *next = strchr(++str, '.'), c = '\0';

    if (next != NULL)
    {
	c = *next;
	*next = '\0';
    }

    for (; members; members = members->next)
    {
        if (strcmp(members->name, str) == 0)
	{
		if (c == '\0')
		{
			/* found the end */
			switch (members->typ->typ)
			{
			   case ECPGt_array:
				return(new_variable(name, ECPGmake_array_type(members->typ->u.element, members->typ->size)));
			   case ECPGt_struct:
			   case ECPGt_union:
				return(new_variable(name, ECPGmake_struct_type(members->typ->u.members, members->typ->typ)));
			   default:
				return(new_variable(name, ECPGmake_simple_type(members->typ->typ, members->typ->size)));
			}
		}
		else
		{
			*next = c;
			if (c == '-')
			{
				next++;
				return(find_struct_member(name, next, members->typ->u.element->u.members));
			}
			else return(find_struct_member(name, next, members->typ->u.members));
		}
	}
    }

    return(NULL);
}

static struct variable *
find_struct(char * name, char *next)
{
    struct variable * p;
    char c = *next;

    /* first get the mother structure entry */
    *next = '\0';
    p = find_variable(name);

    if (c == '-')
    {
	if (p->type->typ != ECPGt_struct && p->type->typ != ECPGt_union)
        {
                sprintf(errortext, "variable %s is not a pointer", name);
                mmerror(ET_FATAL, errortext);
        }

	if (p->type->u.element->typ  != ECPGt_struct && p->type->u.element->typ != ECPGt_union)
        {
                sprintf(errortext, "variable %s is not a pointer to a structure or a union", name);
                mmerror(ET_FATAL, errortext);
        }

	/* restore the name, we will need it later on */
	*next = c;
	next++;

	return find_struct_member(name, next, p->type->u.element->u.members);
    }
    else
    {
	if (p->type->typ != ECPGt_struct && p->type->typ != ECPGt_union)
	{
		sprintf(errortext, "variable %s is neither a structure nor a union", name);
		mmerror(ET_FATAL, errortext);
	}

	/* restore the name, we will need it later on */
	*next = c;

	return find_struct_member(name, next, p->type->u.members);
    }
}

static struct variable *
find_simple(char * name)
{
    struct variable * p;

    for (p = allvariables; p; p = p->next)
    {
        if (strcmp(p->name, name) == 0)
	    return p;
    }

    return(NULL);
}

/* Note that this function will end the program in case of an unknown */
/* variable */
static struct variable *
find_variable(char * name)
{
    char * next;
    struct variable * p;

    if ((next = strchr(name, '.')) != NULL)
	p = find_struct(name, next);
    else if ((next = strstr(name, "->")) != NULL)
	p = find_struct(name, next);
    else
	p = find_simple(name);

    if (p == NULL)
    {
	sprintf(errortext, "The variable %s is not declared", name);
	mmerror(ET_FATAL, errortext);
    }

    return(p);
}

static void
remove_variables(int brace_level)
{
    struct variable * p, *prev;

    for (p = prev = allvariables; p; p = p ? p->next : NULL)
    {
	if (p->brace_level >= brace_level)
	{
	    /* remove it */
	    if (p == allvariables)
		prev = allvariables = p->next;
	    else
		prev->next = p->next;

	    ECPGfree_type(p->type);
	    free(p->name);
	    free(p);
	    p = prev;
	}
	else
	    prev = p;
    }
}


/*
 * Here are the variables that need to be handled on every request.
 * These are of two kinds: input and output.
 * I will make two lists for them.
 */

struct arguments * argsinsert = NULL;
struct arguments * argsresult = NULL;

static void
reset_variables(void)
{
    argsinsert = NULL;
    argsresult = NULL;
}


/* Add a variable to a request. */
static void
add_variable(struct arguments ** list, struct variable * var, struct variable * ind)
{
    struct arguments * p = (struct arguments *)mm_alloc(sizeof(struct arguments));
    p->variable = var;
    p->indicator = ind;
    p->next = *list;
    *list = p;
}


/* Dump out a list of all the variable on this list.
   This is a recursive function that works from the end of the list and
   deletes the list as we go on.
 */
static void
dump_variables(struct arguments * list, int mode)
{
    if (list == NULL)
    {
        return;
    }

    /* The list is build up from the beginning so lets first dump the
       end of the list:
     */

    dump_variables(list->next, mode);

    /* Then the current element and its indicator */
    ECPGdump_a_type(yyout, list->variable->name, list->variable->type,
	(list->indicator->type->typ != ECPGt_NO_INDICATOR) ? list->indicator->name : NULL,
	(list->indicator->type->typ != ECPGt_NO_INDICATOR) ? list->indicator->type : NULL, NULL, NULL);

    /* Then release the list element. */
    if (mode != 0)
    	free(list);
}

static void
check_indicator(struct ECPGtype *var)
{
	/* make sure this is a valid indicator variable */
	switch (var->typ)
	{
		struct ECPGstruct_member *p;

		case ECPGt_short:
		case ECPGt_int:
		case ECPGt_long:
		case ECPGt_unsigned_short:
		case ECPGt_unsigned_int:
		case ECPGt_unsigned_long:
			break;

		case ECPGt_struct:
		case ECPGt_union:
			for (p = var->u.members; p; p = p->next)
				check_indicator(p->typ);
			break;

		case ECPGt_array:
			check_indicator(var->u.element);
			break;
		default: 
			mmerror(ET_ERROR, "indicator variable must be integer type");
			break;
	}
}

static char *
cat2_str(char *str1, char *str2)
{ 
	char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + 2);

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
	int 		i; 
	char		*res_str;

	va_start(args, count);

	res_str = va_arg(args, char *);

	/* now add all other strings */
	for (i = 1; i < count; i++)
		res_str = cat2_str(res_str, va_arg(args, char *));

	va_end(args);

	return(res_str);
}

static char *
make_str(const char *str)
{
        char * res_str = (char *)mm_alloc(strlen(str) + 1);

	strcpy(res_str, str);
	return res_str;
}

static char *
make2_str(char *str1, char *str2)
{ 
	char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + 1);

	strcpy(res_str, str1);
	strcat(res_str, str2);
	free(str1);
	free(str2);
	return(res_str);
}

static char *
make3_str(char *str1, char *str2, char *str3)
{ 
	char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) +strlen(str3) + 1);

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

static void
output_statement(char * stmt, int mode)
{
	int i, j=strlen(stmt);

	fprintf(yyout, "{ ECPGdo(__LINE__, %s, \"", connection ? connection : "NULL");

	/* do this char by char as we have to filter '\"' */
	for (i = 0;i < j; i++) {
		if (stmt[i] != '\"')
			fputc(stmt[i], yyout);
		else
			fputs("\\\"", yyout);
	}

	fputs("\", ", yyout);

	/* dump variables to C file*/
	dump_variables(argsinsert, 1);
	fputs("ECPGt_EOIT, ", yyout);
	dump_variables(argsresult, 1);
	fputs("ECPGt_EORT);", yyout);
	mode |= 2;
	whenever_action(mode);
	free(stmt);
	if (connection != NULL)
		free(connection);
}

static struct typedefs *
get_typedef(char *name)
{
	struct typedefs *this;

	for (this = types; this && strcmp(this->name, name); this = this->next);
	if (!this)
	{
		sprintf(errortext, "invalid datatype '%s'", name);
		mmerror(ET_FATAL, errortext);
	}

	return(this);
}

static void
adjust_array(enum ECPGttype type_enum, int *dimension, int *length, int type_dimension, int type_index, bool pointer)
{
	if (type_index >= 0) 
	{
		if (*length >= 0)
                      	mmerror(ET_FATAL, "No multi-dimensional array support");

		*length = type_index;
	}
		       
	if (type_dimension >= 0)
	{
		if (*dimension >= 0 && *length >= 0)
			mmerror(ET_FATAL, "No multi-dimensional array support");

		if (*dimension >= 0)
			*length = *dimension;

		*dimension = type_dimension;
	}

	if (*length >= 0 && *dimension >= 0 && pointer)
		mmerror(ET_FATAL, "No multi-dimensional array support");

	switch (type_enum)
	{
	   case ECPGt_struct:
	   case ECPGt_union:
	        /* pointer has to get dimension 0 */
                if (pointer)
	        {
		    *length = *dimension;
                    *dimension = 0;
	        }

                if (*length >= 0)
                   mmerror(ET_FATAL, "No multi-dimensional array support for structures");

                break;
           case ECPGt_varchar:
	        /* pointer has to get dimension 0 */
                if (pointer)
                    *dimension = 0;

                /* one index is the string length */
                if (*length < 0)
                {
                   *length = *dimension;
                   *dimension = -1;
                }

                break;
           case ECPGt_char:
           case ECPGt_unsigned_char:
	        /* pointer has to get length 0 */
                if (pointer)
                    *length=0;

                /* one index is the string length */
                if (*length < 0)
                {
                   *length = (*dimension < 0) ? 1 : *dimension;
                   *dimension = -1;
                }

                break;
           default:
 	        /* a pointer has dimension = 0 */
                if (pointer) {
                    *length = *dimension;
		    *dimension = 0;
	        }

                if (*length >= 0)
                   mmerror(ET_FATAL, "No multi-dimensional array support for simple data types");

                break;
	}
}

%}

%union {
	double                  dval;
        int                     ival;
	char *                  str;
	struct when             action;
	struct index		index;
	int			tagname;
	struct this_type	type;
	enum ECPGttype		type_enum;
}

/* special embedded SQL token */
%token		SQL_AT SQL_AUTOCOMMIT SQL_BOOL SQL_BREAK 
%token		SQL_CALL SQL_CONNECT SQL_CONNECTION SQL_CONTINUE
%token		SQL_DEALLOCATE SQL_DISCONNECT SQL_ENUM 
%token		SQL_FOUND SQL_FREE SQL_GO SQL_GOTO
%token		SQL_IDENTIFIED SQL_INDICATOR SQL_INT SQL_LONG
%token		SQL_OFF SQL_OPEN SQL_PREPARE SQL_RELEASE SQL_REFERENCE
%token		SQL_SECTION SQL_SHORT SQL_SIGNED SQL_SQLERROR SQL_SQLPRINT
%token		SQL_SQLWARNING SQL_START SQL_STOP SQL_STRUCT SQL_UNSIGNED
%token		SQL_VAR SQL_WHENEVER

/* C token */
%token		S_ANYTHING S_AUTO S_BOOL S_CHAR S_CONST S_DOUBLE S_ENUM S_EXTERN
%token		S_FLOAT S_INT S
%token		S_LONG S_REGISTER S_SHORT S_SIGNED S_STATIC S_STRUCT
%token		S_UNION S_UNSIGNED S_VARCHAR

/* I need this and don't know where it is defined inside the backend */
%token		TYPECAST

/* Keywords (in SQL92 reserved words) */
%token  ABSOLUTE, ACTION, ADD, ALL, ALTER, AND, ANY, AS, ASC,
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
%token DEFERRABLE, DEFERRED,
               IMMEDIATE, INITIALLY,
               PENDANT,
               RESTRICT,
               TRIGGER

/* Keywords (in SQL92 non-reserved words) */
%token  COMMITTED, SERIALIZABLE, TYPE_P

/* Keywords for Postgres support (not in SQL92 reserved words)
 *
 * The CREATEDB and CREATEUSER tokens should go away
 * when some sort of pg_privileges relation is introduced.
 * - Todd A. Brandys 1998-01-01?
 */
%token  ABORT_TRANS, ACCESS, AFTER, AGGREGATE, ANALYZE,
		BACKWARD, BEFORE, BINARY,
		CACHE, CLUSTER, COMMENT, COPY, CREATEDB, CREATEUSER, CYCLE,
                DATABASE, DELIMITERS, DO,
		EACH, ENCODING, EXCLUSIVE, EXPLAIN, EXTEND,
                FORWARD, FUNCTION, HANDLER,
                INCREMENT, INDEX, INHERITS, INSTEAD, ISNULL,
                LANCOMPILER, LIMIT, LISTEN, UNLISTEN, LOAD, LOCATION, LOCK_P,
		MAXVALUE, MINVALUE, MODE, MOVE,
                NEW,  NOCREATEDB, NOCREATEUSER, NONE, NOTHING, NOTIFY, NOTNULL,
		OFFSET, OIDS, OPERATOR, PASSWORD, PROCEDURAL,
                RENAME, RESET, RETURNS, ROW, RULE,
                SEQUENCE, SERIAL, SETOF, SHARE, SHOW, START, STATEMENT, STDIN, STDOUT, SYSID
		TRUNCATE, TRUSTED,
                UNLISTEN, UNTIL, VACUUM, VALID, VERBOSE, VERSION

/* Special keywords, not in the query language - see the "lex" file */
%token <str>    IDENT SCONST Op CSTRING CVARIABLE CPP_LINE
%token <ival>   ICONST PARAM
%token <dval>   FCONST

/* these are not real. they are here so that they get generated as #define's*/
%token                  OP

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
%left		TYPECAST
%left		UNION INTERSECT EXCEPT

%type  <str>	Iconst Fconst Sconst TransactionStmt CreateStmt UserId
%type  <str>	CreateAsElement OptCreateAs CreateAsList CreateAsStmt
%type  <str>	OptInherit key_reference key_action comment_text
%type  <str>    key_match ColLabel SpecialRuleRelation ColId columnDef
%type  <str>    ColConstraint ColConstraintElem NumericOnly FloatOnly
%type  <str>    OptTableElementList OptTableElement TableConstraint
%type  <str>    ConstraintElem key_actions ColPrimaryKey
%type  <str>    target_list target_el update_target_list ColConstraintList
%type  <str>    update_target_el opt_id relation_name database_name
%type  <str>    access_method attr_name class index_name name func_name
%type  <str>    file_name AexprConst ParamNo TypeId com_expr
%type  <str>	in_expr_nodes a_expr b_expr TruncateStmt CommentStmt
%type  <str> 	opt_indirection expr_list extract_list extract_arg
%type  <str>	position_list substr_list substr_from
%type  <str>	trim_list in_expr substr_for attr attrs
%type  <str>	Typename SimpleTypename Generic Numeric generic opt_float opt_numeric
%type  <str> 	opt_decimal Character character opt_varying opt_charset
%type  <str>	opt_collate Datetime datetime opt_timezone opt_interval
%type  <str>	numeric a_expr_or_null row_expr row_descriptor row_list
%type  <str>	SelectStmt SubSelect result OptTemp OptTempType OptTempScope
%type  <str>	opt_table opt_all opt_unique sort_clause sortby_list
%type  <str>	sortby OptUseOp opt_inh_star relation_name_list name_list
%type  <str>	group_clause having_clause from_clause 
%type  <str>	table_list join_outer where_clause relation_expr sub_type
%type  <str>	opt_column_list insert_rest InsertStmt OptimizableStmt
%type  <str>    columnList DeleteStmt LockStmt UpdateStmt CursorStmt
%type  <str>    NotifyStmt columnElem copy_dirn UnlistenStmt copy_null
%type  <str>    copy_delimiter ListenStmt CopyStmt copy_file_name opt_binary
%type  <str>    opt_with_copy FetchStmt opt_direction fetch_how_many opt_portal_name
%type  <str>    ClosePortalStmt DropStmt VacuumStmt opt_verbose
%type  <str>    opt_analyze opt_va_list va_list ExplainStmt index_params
%type  <str>    index_list func_index index_elem opt_type opt_class access_method_clause
%type  <str>    index_opt_unique IndexStmt set_opt func_return def_rest
%type  <str>    func_args_list func_args opt_with ProcedureStmt def_arg
%type  <str>    def_elem def_list definition def_name def_type DefineStmt
%type  <str>    opt_instead event event_object RuleActionList opt_using
%type  <str>	RuleActionStmtOrEmpty RuleActionMulti join_list func_as
%type  <str>    RuleStmt opt_column opt_name oper_argtypes sysid_clause
%type  <str>    MathOp RemoveFuncStmt aggr_argtype for_update_clause
%type  <str>    RemoveAggrStmt remove_type RemoveStmt ExtendStmt
%type  <str>    RemoveOperStmt RenameStmt all_Op user_valid_clause
%type  <str>    VariableSetStmt var_value zone_value VariableShowStmt
%type  <str>    VariableResetStmt AddAttrStmt alter_clause DropUserStmt
%type  <str>    user_passwd_clause user_createdb_clause opt_trans
%type  <str>    user_createuser_clause user_group_list user_group_clause
%type  <str>    CreateUserStmt AlterUserStmt CreateSeqStmt OptSeqList
%type  <str>    OptSeqElem TriggerForSpec TriggerForOpt TriggerForType
%type  <str>	DropTrigStmt TriggerOneEvent TriggerEvents RuleActionStmt
%type  <str>    TriggerActionTime CreateTrigStmt DropPLangStmt PLangTrusted
%type  <str>    CreatePLangStmt IntegerOnly TriggerFuncArgs TriggerFuncArg
%type  <str>    ViewStmt LoadStmt CreatedbStmt opt_database1 opt_database2 location
%type  <str>    DropdbStmt ClusterStmt grantee RevokeStmt encoding
%type  <str>	GrantStmt privileges operation_commalist operation
%type  <str>	opt_cursor opt_lmode ConstraintsSetStmt comment_tg
%type  <str>	case_expr when_clause_list case_default case_arg when_clause
%type  <str>    select_clause opt_select_limit select_limit_value
%type  <str>    select_offset_value table_list using_expr join_expr
%type  <str>	using_list from_expr table_expr join_clause join_type
%type  <str>	join_qual update_list join_clause join_clause_with_union
%type  <str>	opt_level opt_lock lock_type,
%type  <str>    OptConstrFromTable comment_op ConstraintAttributeSpec
%type  <str>    constraints_set_list constraints_set_namelist comment_fn
%type  <str>	constraints_set_mode comment_type comment_cl comment_ag
%type  <str>	ConstraintDeferrabilitySpec ConstraintTimeSpec 

%type  <str>	ECPGWhenever ECPGConnect connection_target ECPGOpen
%type  <str>	indicator ECPGExecute ECPGPrepare ecpg_using
%type  <str>    storage_clause opt_initializer c_anything blockstart
%type  <str>    blockend variable_list variable c_thing c_term
%type  <str>	opt_pointer cvariable ECPGDisconnect dis_name
%type  <str>	stmt symbol opt_symbol ECPGRelease execstring server_name
%type  <str>	connection_object opt_server opt_port c_stuff opt_reference
%type  <str>    user_name opt_user char_variable ora_user ident
%type  <str>    db_prefix server opt_options opt_connection_name c_list
%type  <str>	ECPGSetConnection cpp_line s_enum ECPGTypedef c_args
%type  <str>	enum_type civariableonly ECPGCursorStmt ECPGDeallocate
%type  <str>	ECPGFree ECPGDeclare ECPGVar sql_variable_declarations
%type  <str>	sql_declaration sql_variable_list sql_variable opt_at
%type  <str>    struct_type s_struct declaration variable_declarations
%type  <str>    s_struct s_union union_type ECPGSetAutocommit on_off

%type  <type_enum> simple_type varchar_type

%type  <type>	type ctype

%type  <action> action

%type  <index>	opt_array_bounds opt_type_array_bounds

%type  <ival>	Iresult
%%
prog: statements;

statements: /* empty */
	| statements statement

statement: ecpgstart opt_at stmt ';'	{ connection = NULL; }
	| ecpgstart stmt ';'
	| ECPGDeclaration
	| c_thing 			{ fprintf(yyout, "%s", $1); free($1); }
	| cpp_line			{ fprintf(yyout, "%s", $1); free($1); }
	| blockstart			{ fputs($1, yyout); free($1); }
	| blockend			{ fputs($1, yyout); free($1); }

opt_at:	SQL_AT connection_target	{ connection = $2; }

stmt:  AddAttrStmt			{ output_statement($1, 0); }
		| AlterUserStmt		{ output_statement($1, 0); }
		| ClosePortalStmt	{ output_statement($1, 0); }
		| CommentStmt		{ output_statement($1, 0); }
		| CopyStmt		{ output_statement($1, 0); }
		| CreateStmt		{ output_statement($1, 0); }
		| CreateAsStmt		{ output_statement($1, 0); }
		| CreateSeqStmt		{ output_statement($1, 0); }
		| CreatePLangStmt	{ output_statement($1, 0); }
		| CreateTrigStmt	{ output_statement($1, 0); }
		| CreateUserStmt	{ output_statement($1, 0); }
  		| ClusterStmt		{ output_statement($1, 0); }
		| DefineStmt 		{ output_statement($1, 0); }
		| DropStmt		{ output_statement($1, 0); }
		| TruncateStmt		{ output_statement($1, 0); }
		| DropPLangStmt		{ output_statement($1, 0); }
		| DropTrigStmt		{ output_statement($1, 0); }
		| DropUserStmt		{ output_statement($1, 0); }
		| ExtendStmt 		{ output_statement($1, 0); }
		| ExplainStmt		{ output_statement($1, 0); }
		| FetchStmt		{ output_statement($1, 1); }
		| GrantStmt		{ output_statement($1, 0); }
		| IndexStmt		{ output_statement($1, 0); }
		| ListenStmt		{ output_statement($1, 0); }
		| UnlistenStmt		{ output_statement($1, 0); }
		| LockStmt		{ output_statement($1, 0); }
		| ProcedureStmt		{ output_statement($1, 0); }
		| RemoveAggrStmt	{ output_statement($1, 0); }
		| RemoveOperStmt	{ output_statement($1, 0); }
		| RemoveFuncStmt	{ output_statement($1, 0); }
		| RemoveStmt		{ output_statement($1, 0); }
		| RenameStmt		{ output_statement($1, 0); }
		| RevokeStmt		{ output_statement($1, 0); }
                | OptimizableStmt	{
						if (strncmp($1, "/* " , sizeof("/* ")-1) == 0)
							output_simple_statement($1);
						else
							output_statement($1, 1);
					}
		| RuleStmt		{ output_statement($1, 0); }
		| TransactionStmt	{
						fprintf(yyout, "{ ECPGtrans(__LINE__, %s, \"%s\");", connection ? connection : "NULL", $1);
						whenever_action(2);
						free($1);
					}
		| ViewStmt		{ output_statement($1, 0); }
		| LoadStmt		{ output_statement($1, 0); }
		| CreatedbStmt		{ output_statement($1, 0); }
		| DropdbStmt		{ output_statement($1, 0); }
		| VacuumStmt		{ output_statement($1, 0); }
		| VariableSetStmt	{ output_statement($1, 0); }
		| VariableShowStmt	{ output_statement($1, 0); }
		| VariableResetStmt	{ output_statement($1, 0); }
		| ConstraintsSetStmt	{ output_statement($1, 0); }
		| ECPGConnect		{
						if (connection)
							mmerror(ET_ERROR, "no at option for connect statement.\n");

						fprintf(yyout, "{ ECPGconnect(__LINE__, %s, %d);", $1, autocommit);
						whenever_action(2);
						free($1);
					} 
		| ECPGCursorStmt	{
						output_simple_statement($1);
					}
		| ECPGDeallocate	{
						if (connection)
							mmerror(ET_ERROR, "no at option for connect statement.\n");

						fputc('{', yyout);
						fputs($1, yyout);
						whenever_action(2);
						free($1);
					}
		| ECPGDeclare		{
						output_simple_statement($1);
					}
		| ECPGDisconnect	{
						if (connection)
							mmerror(ET_ERROR, "no at option for disconnect statement.\n");

						fprintf(yyout, "{ ECPGdisconnect(__LINE__, \"%s\");", $1); 
						whenever_action(2);
						free($1);
					} 
		| ECPGExecute		{
						output_statement($1, 0);
					}
		| ECPGFree		{
						fprintf(yyout, "{ ECPGdeallocate(__LINE__, %s, \"%s\");", connection ? connection : "NULL", $1); 
						whenever_action(2);
						free($1);
					}
		| ECPGOpen		{	
						struct cursor *ptr;
						 
						for (ptr = cur; ptr != NULL; ptr=ptr->next)
						{
					               if (strcmp(ptr->name, $1) == 0)
						       		break;
						}
						
						if (ptr == NULL)
						{
							sprintf(errortext, "trying to open undeclared cursor %s\n", $1);
							mmerror(ET_ERROR, errortext);
						}
                  
						fprintf(yyout, "{ ECPGdo(__LINE__, %s, \"%s\",", ptr->connection ? ptr->connection : "NULL", ptr->command);
						/* dump variables to C file*/
						dump_variables(ptr->argsinsert, 0);
						dump_variables(argsinsert, 0);
						fputs("ECPGt_EOIT, ", yyout);
						dump_variables(ptr->argsresult, 0);
						fputs("ECPGt_EORT);", yyout);
						whenever_action(2);
						free($1);
					}
		| ECPGPrepare		{
						if (connection)
							mmerror(ET_ERROR, "no at option for set connection statement.\n");

						fprintf(yyout, "{ ECPGprepare(__LINE__, %s);", $1); 
						whenever_action(2);
						free($1);
					}
		| ECPGRelease		{ /* output already done */ }
		| ECPGSetAutocommit     {
						fprintf(yyout, "{ ECPGsetcommit(__LINE__, \"%s\", %s);", $1, connection ? connection : "NULL");
						whenever_action(2);
                                       		free($1);
					}
		| ECPGSetConnection     {
						if (connection)
							mmerror(ET_ERROR, "no at option for set connection statement.\n");

						fprintf(yyout, "{ ECPGsetconn(__LINE__, %s);", $1);
						whenever_action(2);
                                       		free($1);
					}
		| ECPGTypedef		{
						if (connection)
							mmerror(ET_ERROR, "no at option for typedef statement.\n");

						output_simple_statement($1);
					}
		| ECPGVar		{
						if (connection)
							mmerror(ET_ERROR, "no at option for var statement.\n");

						output_simple_statement($1);
					}
		| ECPGWhenever		{
						if (connection)
							mmerror(ET_ERROR, "no at option for whenever statement.\n");

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

CreateUserStmt: CREATE USER UserId
		user_createdb_clause user_createuser_clause user_group_clause
		user_valid_clause
				{
					$$ = cat_str(6, make_str("create user"), $3, $4, $5, $6, $7);
				}
		| CREATE USER UserId WITH sysid_clause user_passwd_clause
		user_createdb_clause user_createuser_clause user_group_clause
		user_valid_clause
		{
					$$ = cat_str(9, make_str("create user"), $3, make_str("with"), $5, $6, $7, $8, $9, $10);
		}
		;

/*****************************************************************************
 *
 * Alter a postresql DBMS user
 *
 *
 *****************************************************************************/

AlterUserStmt:  ALTER USER UserId user_createdb_clause
		user_createuser_clause user_group_clause user_valid_clause
				{
					$$ = cat_str(6, make_str("alter user"), $3, $4, $5, $6, $7);
				}
		|ALTER USER UserId WITH sysid_clause user_passwd_clause
		user_createdb_clause user_createuser_clause user_group_clause
		user_valid_clause
				{
					$$ = cat_str(9, make_str("alter user"), $3, make_str("with"), $5, $6, $7, $8, $9, $10);
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
					$$ = cat2_str(make_str("drop user"), $3);
				}
		;

user_passwd_clause:  PASSWORD UserId	{ $$ = cat2_str(make_str("password") , $2); }
			| /*EMPTY*/	{ $$ = EMPTY; }
		;

sysid_clause:	SYSID Iconst		{ $$ = cat2_str(make_str("sysid"), $2); }
			| /*EMPTY*/     { $$ = EMPTY; }
                ;

user_createdb_clause:  CREATEDB
				{
					$$ = make_str("createdb");
				}
			| NOCREATEDB
				{
					$$ = make_str("nocreatedb");
				}
			| /*EMPTY*/		{ $$ = EMPTY; }
		;

user_createuser_clause:  CREATEUSER
				{
					$$ = make_str("createuser");
				}
			| NOCREATEUSER
				{
					$$ = make_str("nocreateuser");
				}
			| /*EMPTY*/		{ $$ = NULL; }
		;

user_group_list:  user_group_list ',' UserId
				{
					$$ = cat_str(3, $1, make_str(","), $3);
				}
			| UserId
				{
					$$ = $1;
				}
		;

user_group_clause:  IN GROUP user_group_list
			{
				/* the backend doesn't actually process this,
                                 * so an warning message is probably fairer */
				mmerror(ET_WARN, "IN GROUP is not implemented");

				$$ = cat2_str(make_str("in group"), $3); 
			}
			| /*EMPTY*/		{ $$ = EMPTY; }
		;

user_valid_clause:  VALID UNTIL Sconst			{ $$ = cat2_str(make_str("valid until"), $3); }
			| /*EMPTY*/			{ $$ = EMPTY; }
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
					$$ = cat_str(4, make_str("set"), $2, make_str("to"), $4);
				}
		| SET ColId '=' var_value
				{
					$$ = cat_str(4, make_str("set"), $2, make_str("="), $4);
				}
		| SET TIME ZONE zone_value
				{
					$$ = cat2_str(make_str("set time zone"), $4);
				}
		| SET TRANSACTION ISOLATION LEVEL opt_level
				{
					$$ = cat2_str(make_str("set transaction isolation level"), $5);
				}
		| SET NAMES encoding
                                {
#ifdef MULTIBYTE
					$$ = cat2_str(make_str("set names"), $3);
#else
                                        mmerror(ET_ERROR, "SET NAMES is not supported");
#endif
                                }
                ;

opt_level:  READ COMMITTED      { $$ = make_str("read committed"); }
               | SERIALIZABLE   { $$ = make_str("serializable"); }
               ;


var_value:  Sconst			{ $$ = $1; }
		| DEFAULT			{ $$ = make_str("default"); }
		;

zone_value:  Sconst			{ $$ = $1; }
		| DEFAULT			{ $$ = make_str("default"); }
		| LOCAL				{ $$ = make_str("local"); }
		;

VariableShowStmt:  SHOW ColId
				{
					$$ = cat2_str(make_str("show"), $2);
				}
		| SHOW TIME ZONE
				{
					$$ = make_str("show time zone");
				}
		| SHOW TRANSACTION ISOLATION LEVEL
				{
					$$ = make_str("show transaction isolation level");
				}
		;

VariableResetStmt:	RESET ColId
				{
					$$ = cat2_str(make_str("reset"), $2);
				}
		| RESET TIME ZONE
				{
					$$ = make_str("reset time zone");
				}
		| RESET TRANSACTION ISOLATION LEVEL
				{
					$$ = make_str("reset transaction isolation level");
				}
		;

ConstraintsSetStmt:    SET CONSTRAINTS constraints_set_list constraints_set_mode
                               {
					$$ = cat_str(3, make_str("set constraints"), $3, $4);
                               }
               ;

constraints_set_list:  ALL
                               {
                                       $$ = make_str("all");
                               }
               | constraints_set_namelist
                               {
                                       $$ = $1;
                               }
               ;


constraints_set_namelist:      IDENT
                               {
                                       $$ = $1;
                               }
               | constraints_set_namelist ',' IDENT
                               {
                                       $$ = cat_str(3, $1, make_str(","), $3);
                               }
               ;

constraints_set_mode:  DEFERRED
                               {
                                       $$ = make_str("deferred");
                               }
               | IMMEDIATE
                               {
                                       $$ = make_str("immediate");
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
					$$ = cat_str(4, make_str("alter table"), $3, $4, $5);
				}
		;

alter_clause:  ADD opt_column columnDef
				{
					$$ = cat_str(3, make_str("add"), $2, $3);
				}
			| ADD '(' OptTableElementList ')'
				{
					$$ = cat_str(2, make_str("add("), $3, make_str(")"));
				}
			| DROP opt_column ColId
				{	mmerror(ET_ERROR, "ALTER TABLE/DROP COLUMN not yet implemented"); }
			| ALTER opt_column ColId SET DEFAULT a_expr
				{	mmerror(ET_ERROR, "ALTER TABLE/ALTER COLUMN/SET DEFAULT not yet implemented"); }
			| ALTER opt_column ColId DROP DEFAULT
				{	mmerror(ET_ERROR, "ALTER TABLE/ALTER COLUMN/DROP DEFAULT not yet implemented"); }
			| ADD ConstraintElem
				{	mmerror(ET_ERROR, "ALTER TABLE/ADD CONSTRAINT not yet implemented"); }
		;

/*****************************************************************************
 *
 *		QUERY :
 *				close <optname>
 *
 *****************************************************************************/

ClosePortalStmt:  CLOSE opt_id
				{
					$$ = cat2_str(make_str("close"), $2);
				}
		;

opt_id:  ColId		{ $$ = $1; }
	| /*EMPTY*/	{ $$ = NULL; }
	;

/*****************************************************************************
 *
 *		QUERY :
 *				COPY [BINARY] <relname> FROM/TO
 *				[USING DELIMITERS <delimiter>]
 *
 *****************************************************************************/

CopyStmt:  COPY opt_binary relation_name opt_with_copy copy_dirn copy_file_name copy_delimiter copy_null
				{
					$$ = cat_str(8, make_str("copy"), $2, $3, $4, $5, $6, $7, $8);
				}
		;

copy_dirn:	TO
				{ $$ = make_str("to"); }
		| FROM
				{ $$ = make_str("from"); }
		;

/*
 * copy_file_name NULL indicates stdio is used. Whether stdin or stdout is
 * used depends on the direction. (It really doesn't make sense to copy from
 * stdout. We silently correct the "typo".		 - AY 9/94
 */
copy_file_name:  Sconst					{ $$ = $1; }
		| STDIN					{ $$ = make_str("stdin"); }
		| STDOUT				{ $$ = make_str("stdout"); }
		;

opt_binary:  BINARY					{ $$ = make_str("binary"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_with_copy:	WITH OIDS				{ $$ = make_str("with oids"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

/*
 * the default copy delimiter is tab but the user can configure it
 */
copy_delimiter:  opt_using DELIMITERS Sconst		{ $$ = cat_str(3, $1, make_str("delimiters"), $3); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_using:	USING		{ $$ = make_str("using"); }
		| /* EMPTY */	{ $$ = EMPTY; }
		;

copy_null:	WITH NULL_P AS Sconst	{ $$ = cat2_str(make_str("with null as"), $4); }
		| /* EMPTY */   { $$ = EMPTY; }
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
					$$ = cat_str(8, make_str("create"), $2, make_str("table"), $4, make_str("("), $6, make_str(")"), $8);
				}
		;

OptTemp:  OptTempType                           { $$ = $1; }
                | OptTempScope OptTempType	{ $$ = cat2_str($1,$2); }
                ;

OptTempType:	  TEMP		{ $$ = make_str("temp"); }
		| TEMPORARY	{ $$ = make_str("temporary"); }
		| /* EMPTY */	{ $$ = EMPTY; }
		;

OptTempScope:  GLOBAL
               {
                    mmerror(ET_ERROR, "GLOBAL TEMPORARY TABLE is not currently supported");
                    $$ = make_str("global");
               }
             | LOCAL { $$ = make_str("local"); }
             ;


OptTableElementList:  OptTableElementList ',' OptTableElement
				{
					$$ = cat_str(3, $1, make_str(","), $3);
				}
			| OptTableElement
				{
					$$ = $1;
				}
			| /*EMPTY*/	{ $$ = EMPTY; }
		;

OptTableElement:  columnDef		{ $$ = $1; }
			| TableConstraint	{ $$ = $1; }
		;

columnDef:  ColId Typename ColConstraintList
				{
					$$ = cat_str(3, $1, $2, $3);
				}
	| ColId SERIAL ColPrimaryKey
		{
			$$ = cat_str(3, $1, make_str(" serial "), $3);
		}
		;

ColConstraintList:  ColConstraintList ColConstraint	{ $$ = cat2_str($1,$2); }
			| /* EMPTY */		{ $$ = EMPTY; }
		;

ColPrimaryKey:  PRIMARY KEY
                {
			$$ = make_str("primary key");
                }
              | /*EMPTY*/
		{
			$$ = EMPTY;
		}
                ;

ColConstraint:
		CONSTRAINT name ColConstraintElem
				{
					$$ = cat_str(3, make_str("constraint"), $2, $3);
				}
		| ColConstraintElem
				{ $$ = $1; }
		;

/*
 * DEFAULT NULL is already the default for Postgres.
 * Bue define it here and carry it forward into the system
 * to make it explicit.
 * - thomas 1998-09-13
 *
 * WITH NULL and NULL are not SQL92-standard syntax elements,
 * so leave them out. Use DEFAULT NULL to explicitly indicate
 * that a column may have that value. WITH NULL leads to
 * shift/reduce conflicts with WITH TIME ZONE anyway.
 * - thomas 1999-01-08
 *
 * DEFAULT expression must be b_expr not a_expr to prevent shift/reduce
 * conflict on NOT (since NOT might start a subsequent NOT NULL constraint,
 * or be part of a_expr NOT LIKE or similar constructs).
 */
ColConstraintElem:  CHECK '(' a_expr ')'
				{
					$$ = cat_str(3, make_str("check("), $3, make_str(")"));
				}
			| DEFAULT NULL_P
				{
					$$ = make_str("default null");
				}
			| DEFAULT b_expr
				{
					$$ = cat2_str(make_str("default"), $2);
				}
			| NOT NULL_P
				{
					$$ = make_str("not null");
				}
			| UNIQUE
				{
					$$ = make_str("unique");
				}
			| PRIMARY KEY
				{
					$$ = make_str("primary key");
				}
			| REFERENCES ColId opt_column_list key_match key_actions
				{
					$$ = cat_str(5, make_str("references"), $2, $3, $4, $5);
				}
		;

/* ConstraintElem specifies constraint syntax which is not embedded into
 *  a column definition. ColConstraintElem specifies the embedded form.
 * - thomas 1997-12-03
 */
TableConstraint:  CONSTRAINT name ConstraintElem
				{
						$$ = cat_str(3, make_str("constraint"), $2, $3);
				}
		| ConstraintElem
				{ $$ = $1; }
		;

ConstraintElem:  CHECK '(' a_expr ')'
				{
					$$ = cat_str(3, make_str("check("), $3, make_str(")"));
				}
		| UNIQUE '(' columnList ')'
				{
					$$ = cat_str(3, make_str("unique("), $3, make_str(")"));
				}
		| PRIMARY KEY '(' columnList ')'
				{
					$$ = cat_str(3, make_str("primary key("), $4, make_str(")"));
				}
		| FOREIGN KEY '(' columnList ')' REFERENCES ColId opt_column_list key_match key_actions
				{
					$$ = cat_str(7, make_str("foreign key("), $4, make_str(") references"), $7, $8, $9, $10);
				}
		;

key_match:  MATCH FULL
		{
			 $$ = make_str("match full");
		}
		| MATCH PARTIAL		
		{
			mmerror(ET_WARN, "FOREIGN KEY match type PARTIAL not implemented yet");
			$$ = make_str("match partial");
		}
		| /*EMPTY*/
		{
			mmerror(ET_WARN, "FOREIGN KEY match type UNSPECIFIED not implemented yet");
			$$ = EMPTY;
		}
		;

key_actions:  key_action key_action	{ $$ = cat2_str($1, $2); }
		| key_action		{ $$ = $1; }
		| /*EMPTY*/		{ $$ = EMPTY; }
		;

key_action:  ON DELETE key_reference	{ $$ = cat2_str(make_str("on delete"), $3); }
		| ON UPDATE key_reference		{ $$ = cat2_str(make_str("on update"), $3); }
		;

key_reference:  NO ACTION	{ $$ = make_str("no action"); }
		| RESTRICT	{ $$ = make_str("restrict"); }
		| CASCADE	{ $$ = make_str("cascade"); }
		| SET DEFAULT	{ $$ = make_str("set default"); }
		| SET NULL_P	{ $$ = make_str("set null"); }
		;

OptInherit:  INHERITS '(' relation_name_list ')' { $$ = cat_str(3, make_str("inherits ("), $3, make_str(")")); }
		| /*EMPTY*/ { $$ = EMPTY; }
		;

/*
 * Note: CREATE TABLE ... AS SELECT ... is just another spelling for
 * SELECT ... INTO.
 */

CreateAsStmt:  CREATE OptTemp TABLE relation_name OptCreateAs AS SelectStmt
		{
			if (FoundInto == 1)
				mmerror(ET_ERROR, "CREATE TABLE/AS SELECT may not specify INTO");

			$$ = cat_str(7, make_str("create"), $2, make_str("table"), $4, $5, make_str("as"), $7); 
		}
		;

OptCreateAs:  '(' CreateAsList ')' { $$ = cat_str(3, make_str("("), $2, make_str(")")); }
			| /*EMPTY*/ { $$ = EMPTY; }	
		;

CreateAsList:  CreateAsList ',' CreateAsElement	{ $$ = cat_str(3, $1, make_str(","), $3); }
			| CreateAsElement	{ $$ = $1; }
		;

CreateAsElement:  ColId { $$ = $1; }
		;

/*****************************************************************************
 *
 *		QUERY :
 *				CREATE SEQUENCE seqname
 *
 *****************************************************************************/

CreateSeqStmt:  CREATE SEQUENCE relation_name OptSeqList
				{
					$$ = cat_str(3, make_str("create sequence"), $3, $4);
				}
		;

OptSeqList:  OptSeqList OptSeqElem
				{ $$ = cat2_str($1, $2); }
			|	{ $$ = EMPTY; }
		;

OptSeqElem:  CACHE IntegerOnly
				{
					$$ = cat2_str(make_str("cache"), $2);
				}
			| CYCLE
				{
					$$ = make_str("cycle");
				}
			| INCREMENT IntegerOnly
				{
					$$ = cat2_str(make_str("increment"), $2);
				}
			| MAXVALUE IntegerOnly
				{
					$$ = cat2_str(make_str("maxvalue"), $2);
				}
			| MINVALUE IntegerOnly
				{
					$$ = cat2_str(make_str("minvalue"), $2);
				}
			| START IntegerOnly
				{
					$$ = cat2_str(make_str("start"), $2);
				}
		;

NumericOnly:  FloatOnly         { $$ = $1; }
		| IntegerOnly   { $$ = $1; }

FloatOnly:  Fconst
                               {
                                       $$ = $1;
                               }
                       | '-' Fconst
                               {
                                       $$ = cat2_str(make_str("-"), $2);
                               }
               ;


IntegerOnly:  Iconst
				{
					$$ = $1;
				}
			| '-' Iconst
				{
					$$ = cat2_str(make_str("-"), $2);
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
				$$ = cat_str(8, make_str("create"), $2, make_str("precedural language"), $5, make_str("handler"), $7, make_str("langcompiler"), $9);
			}
		;

PLangTrusted:		TRUSTED { $$ = make_str("trusted"); }
			|	{ $$ = EMPTY; }

DropPLangStmt:  DROP PROCEDURAL LANGUAGE Sconst
			{
				$$ = cat2_str(make_str("drop procedural language"), $4);
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
					$$ = cat_str(12, make_str("create trigger"), $3, $4, $5, make_str("on"), $7, $8, make_str("execute procedure"), $11, make_str("("), $13, make_str(")"));
				}
	|	CREATE CONSTRAINT TRIGGER name AFTER TriggerOneEvent ON
                                relation_name OptConstrFromTable
				ConstraintAttributeSpec
                                FOR EACH ROW EXECUTE PROCEDURE
				name '(' TriggerFuncArgs ')'
				{
					$$ = cat_str(13, make_str("create constraint trigger"), $4, make_str("after"), $6, make_str("on"), $8, $9, $10, make_str("for each row execute procedure"), $16, make_str("("), $18, make_str(")"));
				}
		;

TriggerActionTime:  BEFORE				{ $$ = make_str("before"); }
			| AFTER				{ $$ = make_str("after"); }
		;

TriggerEvents:	TriggerOneEvent
				{
					$$ = $1;
				}
			| TriggerOneEvent OR TriggerOneEvent
				{
					$$ = cat_str(3, $1, make_str("or"), $3);
				}
			| TriggerOneEvent OR TriggerOneEvent OR TriggerOneEvent
				{
					$$ = cat_str(5, $1, make_str("or"), $3, make_str("or"), $5);
				}
		;

TriggerOneEvent:  INSERT				{ $$ = make_str("insert"); }
			| DELETE			{ $$ = make_str("delete"); }
			| UPDATE			{ $$ = make_str("update"); }
		;

TriggerForSpec:  FOR TriggerForOpt TriggerForType
				{
					$$ = cat_str(3, make_str("for"), $2, $3);
				}
		;

TriggerForOpt:  EACH					{ $$ = make_str("each"); }
			| /*EMPTY*/			{ $$ = EMPTY; }
		;

TriggerForType:  ROW					{ $$ = make_str("row"); }
			| STATEMENT			{ $$ = make_str("statement"); }
		;

TriggerFuncArgs:  TriggerFuncArg
				{ $$ = $1; }
			| TriggerFuncArgs ',' TriggerFuncArg
				{ $$ = cat_str(3, $1, make_str(","), $3); }
			| /*EMPTY*/
				{ $$ = EMPTY; }
		;

TriggerFuncArg:  Iconst
				{
					$$ = $1;
				}
			| Fconst
				{
					$$ = $1;
				}
			| Sconst	{  $$ = $1; }
			| ident		{  $$ = $1; }
		;

OptConstrFromTable:                     /* Empty */
                                {
                                        $$ = EMPTY;
                                }
                | FROM relation_name
                                {
                                        $$ = cat2_str(make_str("from"), $2);
                                }
                ;

ConstraintAttributeSpec: /* Empty */
		{	$$ = EMPTY; }
	| ConstraintDeferrabilitySpec
                { 	$$ = $1; }
	| ConstraintDeferrabilitySpec ConstraintTimeSpec
		{
			if (strcmp($1, "deferrable") != 0 && strcmp($2, "initially deferrable") == 0 )
				mmerror(ET_ERROR, "INITIALLY DEFERRED constraint must be DEFERRABLE");

                	$$ = cat2_str($1, $2);
		}
	| ConstraintTimeSpec
		{ 	$$ = $1; }
	| ConstraintTimeSpec ConstraintDeferrabilitySpec
		{
			if (strcmp($2, "deferrable") != 0 && strcmp($1, "initially deferrable") == 0 )
				mmerror(ET_ERROR, "INITIALLY DEFERRED constraint must be DEFERRABLE");

                	$$ = cat2_str($1, $2);
		}
	;

ConstraintDeferrabilitySpec: NOT DEFERRABLE
                                {
                                        $$ = make_str("not deferrable");
                                }
                | DEFERRABLE
                                {
                                        $$ = make_str("deferrable");
                                }
                ;

ConstraintTimeSpec: INITIALLY IMMEDIATE
                                {
                                        $$ = make_str("initially immediate");
                                }
                | INITIALLY DEFERRED
                                {
                                        $$ = make_str("initially deferrable");
                                }
                ;

DropTrigStmt:  DROP TRIGGER name ON relation_name
				{
					$$ = cat_str(4, make_str("drop trigger"), $3, make_str("on"), $5);
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
					$$ = cat_str(3, make_str("create"), $2, $3);
				}
		;

def_rest:  def_name definition
				{
					$$ = cat2_str($1, $2);
				}
		;

def_type:  OPERATOR		{ $$ = make_str("operator"); }
		| TYPE_P	{ $$ = make_str("type"); }
		| AGGREGATE	{ $$ = make_str("aggregate"); }
		;

def_name:  PROCEDURE		{ $$ = make_str("procedure"); }
		| JOIN		{ $$ = make_str("join"); }
		| ColId		{ $$ = $1; }
		| all_Op	{ $$ = $1; }
		;

definition:  '(' def_list ')'				{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		;

def_list:  def_elem					{ $$ = $1; }
		| def_list ',' def_elem			{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

def_elem:  def_name '=' def_arg	{
					$$ = cat_str(3, $1, make_str("="), $3);
				}
		| def_name
				{
					$$ = $1;
				}
		| DEFAULT '=' def_arg
				{
					$$ = cat2_str(make_str("default ="), $3);
				}
		;

def_arg:  ColId			{  $$ = $1; }
		| all_Op	{  $$ = $1; }
		| NumericOnly	{  $$ = $1; }
		| Sconst	{  $$ = $1; }
		| SETOF ColId
				{
					$$ = cat2_str(make_str("setof"), $2);
				}
		;

/*****************************************************************************
 *
 *		QUERY:
 *				drop <relname1> [, <relname2> .. <relnameN> ]
 *
 *****************************************************************************/

DropStmt:  DROP TABLE relation_name_list
				{
					$$ = cat2_str(make_str("drop table"), $3);
				}
		| DROP SEQUENCE relation_name_list
				{
					$$ = cat2_str(make_str("drop sequence"), $3);
				}
		;

/*****************************************************************************
 *
 *             QUERY:
 *                             truncate table relname
 *
 *****************************************************************************/
TruncateStmt:  TRUNCATE TABLE relation_name
                               {
					$$ = cat2_str(make_str("drop table"), $3);
                               }
                       ;

/*****************************************************************************
 *
 *		QUERY:
 *                     fetch/move [forward | backward] [ # | all ] [ in <portalname> ]
 *                     fetch [ forward | backward | absolute | relative ]
 *                           [ # | all | next | prior ] [ [ in | from ] <portalname> ]
 *
 *****************************************************************************/

FetchStmt:	FETCH opt_direction fetch_how_many opt_portal_name INTO into_list
				{
					if (strcmp($2, "relative") == 0 && atol($3) == 0L)
						mmerror(ET_ERROR, "FETCH/RELATIVE at current position is not supported");

					$$ = cat_str(4, make_str("fetch"), $2, $3, $4);
				}
		|	MOVE opt_direction fetch_how_many opt_portal_name
				{
					$$ = cat_str(4, make_str("fetch"), $2, $3, $4);
				}
		;

opt_direction:	FORWARD		{ $$ = make_str("forward"); }
		| BACKWARD	{ $$ = make_str("backward"); }
		| RELATIVE      { $$ = make_str("relative"); }
                | ABSOLUTE
 				{
					mmerror(ET_WARN, "FETCH/ABSOLUTE not supported, backend will use RELATIVE");
					$$ = make_str("absolute");
				}
		| /*EMPTY*/	{ $$ = EMPTY; /* default */ }
		;

fetch_how_many:   Iconst        { $$ = $1; }
		| '-' Iconst    { $$ = cat2_str(make_str("-"), $2); }
		| ALL		{ $$ = make_str("all"); }
		| NEXT		{ $$ = make_str("next"); }
		| PRIOR		{ $$ = make_str("prior"); }
		| /*EMPTY*/	{ $$ = EMPTY; /*default*/ }
		;

opt_portal_name:  IN name		{ $$ = cat2_str(make_str("in"), $2); }
		| FROM name		{ $$ = cat2_str(make_str("from"), $2); }
/*		| name			{ $$ = cat2_str(make_str("in"), $1); */
		| /*EMPTY*/		{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *  The COMMENT ON statement can take different forms based upon the type of
 *  the object associated with the comment. The form of the statement is:
 *
 *  COMMENT ON [ [ DATABASE | INDEX | RULE | SEQUENCE | TABLE | TYPE | VIEW ]
 *               <objname> | AGGREGATE <aggname> <aggtype> | FUNCTION
 *              <funcname> (arg1, arg2, ...) | OPERATOR <op>
 *              (leftoperand_typ rightoperand_typ) | TRIGGER <triggername> ON
 *              <relname> ] IS 'text'
 *
 *****************************************************************************/
CommentStmt:   COMMENT ON comment_type name IS comment_text
                        {
                                $$ = cat_str(5, make_str("comment on"), $3, $4, make_str("is"), $6);
                        }
                | COMMENT ON comment_cl relation_name '.' attr_name IS comment_text
                        { 
                                $$ = cat_str(7, make_str("comment on"), $3, $4, make_str("."), $6, make_str("is"), $8);
			}
                | COMMENT ON comment_ag name aggr_argtype IS comment_text
                        {
                                $$ = cat_str(6, make_str("comment on"), $3, $4, $5, make_str("is"), $7);
			}
		| COMMENT ON comment_fn func_name func_args IS comment_text
			{
                                $$ = cat_str(6, make_str("comment on"), $3, $4, $5, make_str("is"), $7);
			}
		| COMMENT ON comment_op all_Op '(' oper_argtypes ')' IS comment_text
			{
				$$ = cat_str(7, make_str("comment on"), $3, $4, make_str("("), $6, make_str(") is"), $9);
			}
		| COMMENT ON comment_tg name ON relation_name IS comment_text
                        {
                                $$ = cat_str(7, make_str("comment on"), $3, $4, make_str("on"), $6, make_str("is"), $8);
			}
			;

comment_type:  DATABASE 	{ $$ = make_str("database"); }
                | INDEX		{ $$ = make_str("idnex"); }
                | RULE		{ $$ = make_str("rule"); }
                | SEQUENCE	{ $$ = make_str("sequence"); }
                | TABLE		{ $$ = make_str("table"); }
                | TYPE_P	{ $$ = make_str("type"); }
                | VIEW		{ $$ = make_str("view"); }
		;

comment_cl:    COLUMN		{ $$ = make_str("column"); }

comment_ag:    AGGREGATE	{ $$ = make_str("aggregate"); }

comment_fn:    FUNCTION		{ $$ = make_str("function"); }

comment_op:    OPERATOR		{ $$ = make_str("operator"); }

comment_tg:    TRIGGER		{ $$ = make_str("trigger"); }

comment_text:    Sconst		{ $$ = $1; }
               | NULL_P		{ $$ = make_str("null"); }
               ;

/*****************************************************************************
 *
 *		QUERY:
 *				GRANT [privileges] ON [relation_name_list] TO [GROUP] grantee
 *
 *****************************************************************************/

GrantStmt:  GRANT privileges ON relation_name_list TO grantee opt_with_grant
				{
					$$ = cat_str(7, make_str("grant"), $2, make_str("on"), $4, make_str("to"), $6);
				}
		;

privileges:  ALL PRIVILEGES
				{
				 $$ = make_str("all privileges");
				}
		| ALL
				{
				 $$ = make_str("all");
				}
		| operation_commalist
				{
				 $$ = $1;
				}
		;

operation_commalist:  operation
				{
						$$ = $1;
				}
		| operation_commalist ',' operation
				{
						$$ = cat_str(3, $1, make_str(","), $3);
				}
		;

operation:  SELECT
				{
						$$ = make_str("select");
				}
		| INSERT
				{
						$$ = make_str("insert");
				}
		| UPDATE
				{
						$$ = make_str("update");
				}
		| DELETE
				{
						$$ = make_str("delete");
				}
		| RULE
				{
						$$ = make_str("rule");
				}
		;

grantee:  PUBLIC
				{
						$$ = make_str("public");
				}
		| GROUP ColId
				{
						$$ = cat2_str(make_str("group"), $2);
				}
		| ColId
				{
						$$ = $1;
				}
		;

opt_with_grant:  WITH GRANT OPTION
				{
					mmerror(ET_ERROR, "WITH GRANT OPTION is not supported.  Only relation owners can set privileges");
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
					$$ = cat_str(7, make_str("revoke"), $2, make_str("on"), $4, make_str("from"), $6);
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
					$$ = cat_str(11, make_str("create"), $2, make_str("index"), $4, make_str("on"), $6, $7, make_str("("), $9, make_str(")"), $11);
				}
		;

index_opt_unique:  UNIQUE	{ $$ = make_str("unique"); }
		| /*EMPTY*/	{ $$ = EMPTY; }
		;

access_method_clause:  USING access_method	{ $$ = cat2_str(make_str("using"), $2); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

index_params:  index_list			{ $$ = $1; }
		| func_index			{ $$ = $1; }
		;

index_list:  index_list ',' index_elem		{ $$ = cat_str(3, $1, make_str(","), $3); }
		| index_elem			{ $$ = $1; }
		;

func_index:  func_name '(' name_list ')' opt_type opt_class
				{
					$$ = cat_str(6, $1, make_str("("), $3, ")", $5, $6);
				}
		  ;

index_elem:  attr_name opt_type opt_class
				{
					$$ = cat_str(3, $1, $2, $3);
				}
		;

opt_type:  ':' Typename		{ $$ = cat2_str(make_str(":"), $2); }
		| FOR Typename	{ $$ = cat2_str(make_str("for"), $2); }
		| /*EMPTY*/	{ $$ = EMPTY; }
		;

/* opt_class "WITH class" conflicts with preceeding opt_type
 *  for Typename of "TIMESTAMP WITH TIME ZONE"
 * So, remove "WITH class" from the syntax. OK??
 * - thomas 1997-10-12
 *		| WITH class							{ $$ = $2; }
 */
opt_class:  class				{ $$ = $1; }
		| USING class			{ $$ = cat2_str(make_str("using"), $2); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

/*****************************************************************************
 *
 *		QUERY:
 *				extend index <indexname> [where <qual>]
 *
 *****************************************************************************/

ExtendStmt:  EXTEND INDEX index_name where_clause
				{
					$$ = cat_str(3, make_str("extend index"), $3, $4);
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
					$$ = cat2_str(make_str("execute recipe"), $3);
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
					$$ = cat_str(10, make_str("create function"), $3, $4, make_str("returns"), $6, $7, make_str("as"), $9, make_str("language"), $11);
				}

opt_with:  WITH definition			{ $$ = cat2_str(make_str("with"), $2); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

func_args:  '(' func_args_list ')'		{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| '(' ')'			{ $$ = make_str("()"); }
		;

func_args_list:  TypeId				{ $$ = $1; }
		| func_args_list ',' TypeId
				{	$$ = cat_str(3, $1, make_str(","), $3); }
		;

func_as: Sconst				{ $$ = $1; }
		| Sconst ',' Sconst	{ $$ = cat_str(3, $1, make_str(","), $3); }

func_return:  set_opt TypeId
				{
					$$ = cat2_str($1, $2);
				}
		;

set_opt:  SETOF					{ $$ = make_str("setof"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
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
					$$ = cat_str(3, make_str("drop"), $2, $3);
				}
		;

remove_type:  TYPE_P		{  $$ = make_str("type"); }
		| INDEX		{  $$ = make_str("index"); }
		| RULE		{  $$ = make_str("rule"); }
		| VIEW		{  $$ = make_str("view"); }
		;


RemoveAggrStmt:  DROP AGGREGATE name aggr_argtype
				{
						$$ = cat_str(3, make_str("drop aggregate"), $3, $4);
				}
		;

aggr_argtype:  name			{ $$ = $1; }
		| '*'			{ $$ = make_str("*"); }
		;


RemoveFuncStmt:  DROP FUNCTION func_name func_args
				{
						$$ = cat_str(3, make_str("drop function"), $3, $4);
				}
		;


RemoveOperStmt:  DROP OPERATOR all_Op '(' oper_argtypes ')'
				{
					$$ = cat_str(5, make_str("drop operator"), $3, make_str("("), $5, make_str(")"));
				}
		;

oper_argtypes:	name
				{
				   mmerror(ET_ERROR, "parser: argument type missing (use NONE for unary operators)");
				}
		| name ',' name
				{ $$ = cat_str(3, $1, make_str(","), $3); }
		| NONE ',' name			/* left unary */
				{ $$ = cat2_str(make_str("none,"), $3); }
		| name ',' NONE			/* right unary */
				{ $$ = cat2_str($1, make_str(", none")); }
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
					$$ = cat_str(8, make_str("alter table"), $3, $4, make_str("rename"), $6, $7, make_str("to"), $9);
				}
		;

opt_name:  name							{ $$ = $1; }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;

opt_column:  COLUMN					{ $$ = make_str("colmunn"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
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
		   { QueryIsRule=1; }
		   ON event TO event_object where_clause
		   DO opt_instead RuleActionList
				{
					$$ = cat_str(10, make_str("create rule"), $3, make_str("as on"), $7, make_str("to"), $9, $10, make_str("do"), $12, $13);
				}
		;

RuleActionList:  NOTHING                               { $$ = make_str("nothing"); }
               | SelectStmt                            { $$ = $1; }
               | RuleActionStmt                        { $$ = $1; }
               | '[' RuleActionMulti ']'               { $$ = cat_str(3, make_str("["), $2, make_str("]")); }
               | '(' RuleActionMulti ')'               { $$ = cat_str(3, make_str("("), $2, make_str(")")); }
                ;

/* the thrashing around here is to discard "empty" statements... */
RuleActionMulti:  RuleActionMulti ';' RuleActionStmtOrEmpty
                                {  $$ = cat_str(3, $1, make_str(";"), $3); }
		| RuleActionStmtOrEmpty
				{ $$ = cat2_str($1, make_str(";")); }
		;

RuleActionStmt:        InsertStmt
                | UpdateStmt
                | DeleteStmt
		| NotifyStmt
                ;
RuleActionStmtOrEmpty: RuleActionStmt	{ $$ = $1; }
               |       /*EMPTY*/        { $$ = EMPTY; }
               ;

event_object:  relation_name '.' attr_name
				{
					$$ = make3_str($1, make_str("."), $3);
				}
		| relation_name
				{
					$$ = $1;
				}
		;

/* change me to select, update, etc. some day */
event:	SELECT					{ $$ = make_str("select"); }
		| UPDATE			{ $$ = make_str("update"); }
		| DELETE			{ $$ = make_str("delete"); }
		| INSERT			{ $$ = make_str("insert"); }
		 ;

opt_instead:  INSTEAD					{ $$ = make_str("instead"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
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
					$$ = cat2_str(make_str("notify"), $2);
				}
		;

ListenStmt:  LISTEN relation_name
				{
					$$ = cat2_str(make_str("listen"), $2);
                                }
;

UnlistenStmt:  UNLISTEN relation_name
				{
					$$ = cat2_str(make_str("unlisten"), $2);
                                }
		| UNLISTEN '*'
				{
					$$ = make_str("unlisten *");
                                }
;

/*****************************************************************************
 *
 *              Transactions:
 *
 *              abort transaction
 *                              (ABORT)
 *              begin transaction
 *                              (BEGIN)
 *              end transaction  
 *                              (END)
 *
 *****************************************************************************/
TransactionStmt:  ABORT_TRANS opt_trans	{ $$ = make_str("rollback"); }
	| BEGIN_TRANS opt_trans		{ $$ = make_str("begin transaction"); }
	| COMMIT opt_trans		{ $$ = make_str("commit"); }
	| END_TRANS opt_trans			{ $$ = make_str("commit"); }
	| ROLLBACK opt_trans			{ $$ = make_str("rollback"); }

opt_trans: WORK 	{ $$ = ""; }
	| TRANSACTION	{ $$ = ""; }
	| /*EMPTY*/	{ $$ = ""; }
                ;

/*****************************************************************************
 *
 *		QUERY:
 *				define view <viewname> '('target-list ')' [where <quals> ]
 *
 *****************************************************************************/

ViewStmt:  CREATE VIEW name AS SelectStmt
				{
					$$ = cat_str(4, make_str("create view"), $3, make_str("as"), $5);
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				load make_str("filename")
 *
 *****************************************************************************/

LoadStmt:  LOAD file_name
				{
					$$ = cat2_str(make_str("load"), $2);
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
					if (strlen($5) == 0 || strlen($6) == 0) 
						mmerror(ET_ERROR, "CREATE DATABASE WITH requires at least an option");
#ifndef MULTIBYTE
					if (strlen($6) != 0)
						mmerror(ET_ERROR, "WITH ENCODING is not supported");
#endif
					$$ = cat_str(5, make_str("create database"), $3, make_str("with"), $5, $6);
				}
		| CREATE DATABASE database_name
        			{
					$$ = cat2_str(make_str("create database"), $3);
				}
		;

opt_database1:  LOCATION '=' location	{ $$ = cat2_str(make_str("location ="), $3); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

opt_database2:  ENCODING '=' encoding   { $$ = cat2_str(make_str("encoding ="), $3); }
                | /*EMPTY*/	        { $$ = NULL; }
                ;

location:  Sconst				{ $$ = $1; }
		| DEFAULT			{ $$ = make_str("default"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

encoding:  Sconst		{ $$ = $1; }
		| DEFAULT	{ $$ = make_str("default"); }
		| /*EMPTY*/	{ $$ = EMPTY; }
               ;

/*****************************************************************************
 *
 *		QUERY:
 *				dropdb dbname
 *
 *****************************************************************************/

DropdbStmt:	DROP DATABASE database_name
				{
					$$ = cat2_str(make_str("drop database"), $3);
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
				   $$ = cat_str(4, make_str("cluster"), $2, make_str("on"), $4);
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
					$$ = cat_str(3, make_str("vacuum"), $2, $3);
				}
		| VACUUM opt_verbose opt_analyze relation_name opt_va_list
				{
					if ( strlen($5) > 0 && strlen($4) == 0 )
						mmerror(ET_ERROR, "parser: syntax error at or near \"(\"");
					$$ = cat_str(5, make_str("vacuum"), $2, $3, $4, $5);
				}
		;

opt_verbose:  VERBOSE					{ $$ = make_str("verbose"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_analyze:  ANALYZE					{ $$ = make_str("analyse"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_va_list:  '(' va_list ')'				{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

va_list:  name
				{ $$=$1; }
		| va_list ',' name
				{ $$=cat_str(3, $1, make_str(","), $3); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				EXPLAIN query
 *
 *****************************************************************************/

ExplainStmt:  EXPLAIN opt_verbose OptimizableStmt
				{
					$$ = cat_str(3, make_str("explain"), $2, $3);
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
		| DeleteStmt
		;


/*****************************************************************************
 *
 *		QUERY:
 *				INSERT STATEMENTS
 *
 *****************************************************************************/

/* This rule used 'opt_column_list' between 'relation_name' and 'insert_rest'
 * originally. When the second rule of 'insert_rest' was changed to use
 * the new 'SelectStmt' rule (for INTERSECT and EXCEPT) it produced a shift/red uce
 * conflict. So I just changed the rules 'InsertStmt' and 'insert_rest' to accept
 * the same statements without any shift/reduce conflicts */
InsertStmt:  INSERT INTO relation_name insert_rest
				{
					$$ = cat_str(3, make_str("insert into"), $3, $4);
				}
		;

insert_rest:  VALUES '(' target_list ')'
				{
					$$ = cat_str(3, make_str("values("), $3, make_str(")"));
				}
		| DEFAULT VALUES
				{
					$$ = make_str("default values");
				}
		| SelectStmt
				{
					$$ = $1;
				}
		| '(' columnList ')' VALUES '(' target_list ')'
				{
					$$ = cat_str(5, make_str("("), $2, make_str(") values ("), $6, make_str(")"));
				}
		| '(' columnList ')' SelectStmt
				{
					$$ = cat_str(4, make_str("("), $2, make_str(")"), $4);
				}
		;

opt_column_list:  '(' columnList ')'			{ $$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

columnList:
		  columnList ',' columnElem
				{ $$ = cat_str(3, $1, make_str(","), $3); }
		| columnElem
				{ $$ = $1; }
		;

columnElem:  ColId opt_indirection
				{
					$$ = cat2_str($1, $2);
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
					$$ = cat_str(3, make_str("delete from"), $3, $4);
				}
		;

LockStmt:  LOCK_P opt_table relation_name opt_lock
				{
					$$ = cat_str(4, make_str("lock"), $2, $3, $4);
				}
		;

opt_lock:  IN lock_type MODE            { $$ = cat_str(3, make_str("in"), $2, make_str("mode")); }
                | /*EMPTY*/             { $$ = EMPTY;}
                ;

lock_type:  SHARE ROW EXCLUSIVE 	{ $$ = make_str("share row exclusive"); }
                | ROW opt_lmode         { $$ = cat2_str(make_str("row"), $2);}
                | ACCESS opt_lmode      { $$ = cat2_str(make_str("access"), $2);}
                | opt_lmode             { $$ = $1; }
                ;

opt_lmode:      SHARE                           { $$ = make_str("share"); }
                | EXCLUSIVE                     { $$ = make_str("exclusive"); }
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
					$$ = cat_str(6, make_str("update"), $2, make_str("set"), $4, $5, $6);
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				CURSOR STATEMENTS
 *
 *****************************************************************************/
CursorStmt:  DECLARE name opt_cursor CURSOR FOR
		{ ForUpdateNotAllowed = 1; }
	     SelectStmt
				{
					struct cursor *ptr, *this;
	
					for (ptr = cur; ptr != NULL; ptr = ptr->next)
					{
						if (strcmp($2, ptr->name) == 0)
						{
						        /* re-definition is a bug */
							sprintf(errortext, "cursor %s already defined", $2);
							mmerror(ET_ERROR, errortext);
				                }
        				}
                        
        				this = (struct cursor *) mm_alloc(sizeof(struct cursor));

			        	/* initial definition */
				        this->next = cur;
				        this->name = $2;
					this->connection = connection;
				        this->command =  cat_str(5, make_str("declare"), mm_strdup($2), $3, make_str("cursor for"), $7);
					this->argsinsert = argsinsert;
					this->argsresult = argsresult;
					argsinsert = argsresult = NULL;
											
			        	cur = this;
					
					$$ = cat_str(3, make_str("/*"), mm_strdup(this->command), make_str("*/"));
				}
		;

opt_cursor:  BINARY             	{ $$ = make_str("binary"); }
               | INSENSITIVE		{ $$ = make_str("insensitive"); }
               | SCROLL        		{ $$ = make_str("scroll"); }
               | INSENSITIVE SCROLL	{ $$ = make_str("insensitive scroll"); }
               | /*EMPTY*/      	{ $$ = EMPTY; }
               ;

/*****************************************************************************
 *
 *		QUERY:
 *				SELECT STATEMENTS
 *
 *****************************************************************************/

/* The new 'SelectStmt' rule adapted for the optional use of INTERSECT EXCEPT a nd UNION
 * accepts the use of '(' and ')' to select an order of set operations.
 * The rule returns a SelectStmt Node having the set operations attached to
 * unionClause and intersectClause (NIL if no set operations were present)
 */

SelectStmt:      select_clause sort_clause for_update_clause opt_select_limit
				{
					if (strlen($3) > 0 && ForUpdateNotAllowed != 0)
							mmerror(ET_ERROR, "FOR UPDATE is not allowed in this context");

					ForUpdateNotAllowed = 0;
					$$ = cat_str(4, $1, $2, $3, $4);
				}

/* This rule parses Select statements including UNION INTERSECT and EXCEPT.
 * '(' and ')' can be used to specify the order of the operations 
 * (UNION EXCEPT INTERSECT). Without the use of '(' and ')' we want the
 * operations to be left associative.
 *
 *  The sort_clause is not handled here!
 */
select_clause: '(' select_clause ')'
                        {
                                $$ = cat_str(3, make_str("("), $2, make_str(")")); 
                        }
                | SubSelect
                        {
				FoundInto = 0;
                                $$ = $1; 
                        }
                | select_clause EXCEPT select_clause
			{
				$$ = cat_str(3, $1, make_str("except"), $3);
				ForUpdateNotAllowed = 1;
			}
		| select_clause UNION opt_all select_clause
			{
				$$ = cat_str(4, $1, make_str("union"), $3, $4);
				ForUpdateNotAllowed = 1;
			}
		| select_clause INTERSECT opt_all select_clause
			{
				$$ = cat_str(3, $1, make_str("intersect"), $3);
				ForUpdateNotAllowed = 1;
			}
		;

SubSelect:     SELECT opt_unique target_list
                         result from_clause where_clause
                         group_clause having_clause
				{
					if (strlen($7) > 0 || strlen($8) > 0)
						ForUpdateNotAllowed = 1;
					$$ = cat_str(8, make_str("select"), $2, $3, $4, $5, $6, $7, $8);
				}
		;

result:  INTO OptTemp opt_table relation_name		{ FoundInto = 1;
							  $$= cat_str(4, make_str("into"), $2, $3, $4);
							}
		| INTO into_list			{ $$ = EMPTY; }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_table:  TABLE					{ $$ = make_str("table"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_all:  ALL						{ $$ = make_str("all"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_unique:  DISTINCT					{ $$ = make_str("distinct"); }
		| DISTINCT ON ColId			{ $$ = cat2_str(make_str("distinct on"), $3); }
		| ALL					{ $$ = make_str("all"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

sort_clause:  ORDER BY sortby_list			{ $$ = cat2_str(make_str("order by"), $3); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

sortby_list:  sortby					{ $$ = $1; }
		| sortby_list ',' sortby		{ $$ = cat_str(3, $1, make_str(","), $3); }
		;

sortby: a_expr OptUseOp
				{
					 $$ = cat2_str($1, $2);
                                }
		;

OptUseOp:  USING all_Op				{ $$ = cat2_str(make_str("using"), $2); }
		| ASC				{ $$ = make_str("asc"); }
		| DESC				{ $$ = make_str("desc"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

opt_select_limit:      LIMIT select_limit_value ',' select_offset_value
                       { $$ = cat_str(4, make_str("limit"), $2, make_str(","), $4); }
               | LIMIT select_limit_value OFFSET select_offset_value
                       { $$ = cat_str(4, make_str("limit"), $2, make_str("offset"), $4); }
               | LIMIT select_limit_value
                       { $$ = cat2_str(make_str("limit"), $2); }
               | OFFSET select_offset_value LIMIT select_limit_value
                       { $$ = cat_str(4, make_str("offset"), $2, make_str("limit"), $4); }
               | OFFSET select_offset_value
                       { $$ = cat2_str(make_str("offset"), $2); }
               | /* EMPTY */
                       { $$ = EMPTY; }
               ;

select_limit_value:	Iconst	{ $$ = $1; }
	          	| ALL	{ $$ = make_str("all"); }
			| PARAM { $$ = make_name(); }
               ;

select_offset_value:  	Iconst	{ $$ = $1; }
			| PARAM { $$ = make_name(); }
               ;

/*
 *	jimmy bell-style recursive queries aren't supported in the
 *	current system.
 *
 *	...however, recursive addattr and rename supported.  make special
 *	cases for these.
 */
opt_inh_star:  '*'					{ $$ = make_str("*"); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

relation_name_list:  name_list { $$ = $1; };

name_list:  name
				{	$$ = $1; }
		| name_list ',' name
				{	$$ = cat_str(3, $1, make_str(","), $3); }
		;

group_clause:  GROUP BY expr_list			{ $$ = cat2_str(make_str("group by"), $3); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

having_clause:  HAVING a_expr
				{
					$$ = cat2_str(make_str("having"), $2);
				}
		| /*EMPTY*/		{ $$ = EMPTY; }
		;

for_update_clause:  FOR UPDATE update_list
		{
                	$$ = make_str("for update"); 
		}
		| FOR READ ONLY
		{
			$$ = make_str("for read only");
		}
		| /* EMPTY */
                {
                        $$ = EMPTY;
                }
                ;
update_list:  OF va_list
              {
			$$ = cat2_str(make_str("of"), $2);
	      }
              | /* EMPTY */
              {
                        $$ = EMPTY;
              }
              ;

/*****************************************************************************
 *
 *	clauses common to all Optimizable Stmts:
 *		from_clause		-
 *		where_clause	-
 *
 *****************************************************************************/

from_clause:  FROM from_expr
		{
			$$ = cat2_str(make_str("from"), $2);
		}
		| /* EMPTY */
		{
			$$ = EMPTY;
		}


from_expr:  '(' join_clause_with_union ')'
                                { $$ = cat_str(3, make_str("("), $2, make_str(")")); }
                | join_clause
                                { $$ = $1; }
                | table_list
                                { $$ = $1; }
                ;

table_list:  table_list ',' table_expr
                                { $$ = cat_str(3, $1, make_str(","), $3); }
                | table_expr
                                { $$ = $1; }
                ;

table_expr:  relation_expr AS ColLabel
                                {
                                        $$ = cat_str(3, $1, make_str("as"), $3);
                                }
                | relation_expr ColId
                                {
                                        $$ = cat2_str($1, $2);
                                }
                | relation_expr
                                {
                                        $$ = $1;
                                }
                ;

/* A UNION JOIN is the same as a FULL OUTER JOIN which *omits*
 * all result rows which would have matched on an INNER JOIN.
 * Let's reject this for now. - thomas 1999-01-08
 */
join_clause_with_union:  join_clause
                                {       $$ = $1; }
                | table_expr UNION JOIN table_expr
                                {       mmerror(ET_ERROR, "UNION JOIN not yet implemented"); }
                ;

join_clause:  table_expr join_list
                                {
					$$ = cat2_str($1, $2);
                                }
                ;

join_list:  join_list join_expr
                                {
                                        $$ = cat2_str($1, $2);
                                }
                | join_expr
                                {
                                        $$ = $1;
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
					$$ = cat_str(4, $1, make_str("join"), $3, $4);
                                }
                | NATURAL join_type JOIN table_expr
                                {
					$$ = cat_str(4, make_str("natural"), $2, make_str("join"), $4);
                                }
                | CROSS JOIN table_expr
                                { 	$$ = cat2_str(make_str("cross join"), $3); }
                ;

/* OUTER is just noise... */
join_type:  FULL join_outer
                                {
                                        $$ = cat2_str(make_str("full"), $2);
					mmerror(ET_WARN, "FULL OUTER JOIN not yet implemented");
                                }
                | LEFT join_outer
                                {
                                        $$ = cat2_str(make_str("left"), $2);
					mmerror(ET_WARN, "LEFT OUTER JOIN not yet implemented");
                                }
                | RIGHT join_outer
                                {
                                        $$ = cat2_str(make_str("right"), $2);
					mmerror(ET_WARN, "RIGHT OUTER JOIN not yet implemented");
                                }
                | OUTER_P
                                {
                                        $$ = make_str("outer");
					mmerror(ET_WARN, "OUTER JOIN not yet implemented");
                                }
                | INNER_P
                                {
                                        $$ = make_str("inner");
				}
                | /* EMPTY */
                                {
                                        $$ = EMPTY;
				}


join_outer:  OUTER_P				{ $$ = make_str("outer"); }
		| /*EMPTY*/			{ $$ = EMPTY;  /* no qualifiers */ }
		;

/* JOIN qualification clauses
 * Possibilities are:
 *  USING ( column list ) allows only unqualified column names,
 *                        which must match between tables.
 *  ON expr allows more general qualifications.
 * - thomas 1999-01-07
 */

join_qual:  USING '(' using_list ')'                   { $$ = cat_str(3, make_str("using ("), $3, make_str(")")); }
               | ON a_expr			       { $$ = cat2_str(make_str("on"), $2); }
                ;

using_list:  using_list ',' using_expr                  { $$ = cat_str(3, $1, make_str(","), $3); }
               | using_expr				{ $$ = $1; }
               ;

using_expr:  ColId
				{
					$$ = $1;
				}
		;

where_clause:  WHERE a_expr			{ $$ = cat2_str(make_str("where"), $2); }
		| /*EMPTY*/				{ $$ = EMPTY;  /* no qualifiers */ }
		;

relation_expr:	relation_name
				{
					/* normal relations */
					$$ = $1;
				}
		| relation_name '*'				  %prec '='
				{
					/* inheritance query */
					$$ = cat2_str($1, make_str("*"));
				}

opt_array_bounds:  '[' ']' opt_array_bounds
			{
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make_str("[]"), $3.str);
                        }
		| '[' Iresult ']' opt_array_bounds
			{
			    char *txt = mm_alloc(20L);

			    sprintf (txt, "%d", $2);
                            $$.index1 = $2;
                            $$.index2 = $4.index1;
                            $$.str = cat_str(4, make_str("["), txt, make_str("]"), $4.str);
                        }
		| /* EMPTY */
			{
                            $$.index1 = -1;
                            $$.index2 = -1;
                            $$.str= EMPTY;
                        }
		;

Iresult:	Iconst			{ $$ = atol($1); }
	|	'(' Iresult ')'		{ $$ = $2; }
	|	Iresult '+' Iresult	{ $$ = $1 + $3; }
	|	Iresult '-' Iresult	{ $$ = $1 - $3; }
	|	Iresult '*' Iresult	{ $$ = $1 * $3; }
	|	Iresult '/' Iresult	{ $$ = $1 / $3; }
	|	Iresult '%' Iresult	{ $$ = $1 % $3; }
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
					$$ = cat2_str($1, $2.str);
				}
		| SETOF SimpleTypename
				{
					$$ = cat2_str(make_str("setof"), $2);
				}
		;

SimpleTypename:  Generic	{ $$ = $1; }
		| Datetime	{ $$ = $1; }
		| Numeric	{ $$ = $1; }
		| Character	{ $$ = $1; }
		;

Generic:  generic
				{
					$$ = $1;
				}
		;

generic:  ident					{ $$ = $1; }
		| TYPE_P			{ $$ = make_str("type"); }
		| SQL_AT			{ $$ = make_str("at"); }
		| SQL_AUTOCOMMIT		{ $$ = make_str("autocommit"); }
		| SQL_BOOL			{ $$ = make_str("bool"); }
		| SQL_BREAK			{ $$ = make_str("break"); }
		| SQL_CALL			{ $$ = make_str("call"); }
		| SQL_CONNECT			{ $$ = make_str("connect"); }
		| SQL_CONNECTION		{ $$ = make_str("connection"); }
		| SQL_CONTINUE			{ $$ = make_str("continue"); }
		| SQL_DEALLOCATE		{ $$ = make_str("deallocate"); }
		| SQL_DISCONNECT		{ $$ = make_str("disconnect"); }
		| SQL_FOUND			{ $$ = make_str("found"); }
		| SQL_GO			{ $$ = make_str("go"); }
		| SQL_GOTO			{ $$ = make_str("goto"); }
		| SQL_IDENTIFIED		{ $$ = make_str("identified"); }
		| SQL_INDICATOR			{ $$ = make_str("indicator"); }
		| SQL_INT			{ $$ = make_str("int"); }
		| SQL_LONG			{ $$ = make_str("long"); }
		| SQL_OFF			{ $$ = make_str("off"); }
		| SQL_OPEN			{ $$ = make_str("open"); }
		| SQL_PREPARE			{ $$ = make_str("prepare"); }
		| SQL_RELEASE			{ $$ = make_str("release"); }
		| SQL_SECTION			{ $$ = make_str("section"); }
		| SQL_SHORT			{ $$ = make_str("short"); }
		| SQL_SIGNED			{ $$ = make_str("signed"); }
		| SQL_SQLERROR			{ $$ = make_str("sqlerror"); }
		| SQL_SQLPRINT			{ $$ = make_str("sqlprint"); }
		| SQL_SQLWARNING		{ $$ = make_str("sqlwarning"); }
		| SQL_STOP			{ $$ = make_str("stop"); }
		| SQL_STRUCT			{ $$ = make_str("struct"); }
		| SQL_UNSIGNED			{ $$ = make_str("unsigned"); }
		| SQL_VAR			{ $$ = make_str("var"); }
		| SQL_WHENEVER			{ $$ = make_str("whenever"); }
		;

/* SQL92 numeric data types
 * Check FLOAT() precision limits assuming IEEE floating types.
 * Provide real DECIMAL() and NUMERIC() implementations now - Jan 1998-12-30
 * - thomas 1997-09-18
 */
Numeric:  FLOAT opt_float
				{
					$$ = cat2_str(make_str("float"), $2);
				}
		| DOUBLE PRECISION
				{
					$$ = make_str("double precision");
				}
		| DECIMAL opt_decimal
				{
					$$ = cat2_str(make_str("decimal"), $2);
				}
		| NUMERIC opt_numeric
				{
					$$ = cat2_str(make_str("numeric"), $2);
				}
		;

numeric:  FLOAT
				{	$$ = make_str("float"); }
		| DOUBLE PRECISION
				{	$$ = make_str("double precision"); }
		| DECIMAL
				{	$$ = make_str("decimal"); }
		| NUMERIC
				{	$$ = make_str("numeric"); }
		;

opt_float:  '(' Iconst ')'
				{
					if (atol($2) < 1)
						mmerror(ET_ERROR, "precision for FLOAT must be at least 1");
					else if (atol($2) >= 16)
						mmerror(ET_ERROR, "precision for FLOAT must be less than 16");
					$$ = cat_str(3, make_str("("), $2, make_str(")"));
				}
		| /*EMPTY*/
				{
					$$ = EMPTY;
				}
		;

opt_numeric:  '(' Iconst ',' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						mmerror(ET_ERROR, errortext);
					}
					if (atol($4) < 0 || atol($4) > atol($2)) {
						sprintf(errortext, "NUMERIC scale %s must be between 0 and precision %s", $4, $2);
						mmerror(ET_ERROR, errortext);
					}
					$$ = cat_str(5, make_str("("), $2, make_str(","), $4, make_str(")"));
				}
		| '(' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						mmerror(ET_ERROR, errortext);
					}
					$$ = cat_str(3, make_str("("), $2, make_str(")"));
				}
		| /*EMPTY*/
				{
					$$ = EMPTY;
				}
		;

opt_decimal:  '(' Iconst ',' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						mmerror(ET_ERROR, errortext);
					}
					if (atol($4) < 0 || atol($4) > atol($2)) {
						sprintf(errortext, "NUMERIC scale %s must be between 0 and precision %s", $4, $2);
						mmerror(ET_ERROR, errortext);
					}
					$$ = cat_str(5, make_str("("), $2, make_str(","), $4, make_str(")"));
				}
		| '(' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						mmerror(ET_ERROR, errortext);
					}
					$$ = cat_str(3, make_str("("), $2, make_str(")"));
				}
		| /*EMPTY*/
				{
					$$ = EMPTY;
				}
		;

/* SQL92 character data types
 * The following implements CHAR() and VARCHAR().
 *								- ay 6/95
 */
Character:  character '(' Iconst ')'
				{
					if (strncasecmp($1, "char", strlen("char")) && strncasecmp($1, "varchar", strlen("varchar")))
						mmerror(ET_ERROR, "internal parsing error; unrecognized character type");
					if (atol($3) < 1)
					{
						sprintf(errortext, "length for '%s' type must be at least 1",$1);
						mmerror(ET_ERROR, errortext);
					}
					else if (atol($3) > MaxAttrSize)
					{
						sprintf(errortext, "length for type '%s' cannot exceed %ld", $1, MaxAttrSize);
						mmerror(ET_ERROR, errortext);
					}

					$$ = cat_str(4, $1, make_str("("), $3, make_str(")"));
				}
		| character
				{
					$$ = $1;
				}
		;

character:  CHARACTER opt_varying opt_charset opt_collate
				{
					if (strlen($4) > 0)
					{
						sprintf(errortext, "COLLATE %s not yet implemented", $4);
						mmerror(ET_WARN, errortext);
					}

					$$ = cat_str(4, make_str("character"), $2, $3, $4);
				}
		| CHAR opt_varying	{ $$ = cat2_str(make_str("char"), $2); }
		| VARCHAR		{ $$ = make_str("varchar"); }
		| NATIONAL CHARACTER opt_varying { $$ = cat2_str(make_str("national character"), $3); }
		| NCHAR opt_varying		{ $$ = cat2_str(make_str("nchar"), $2); }
		;

opt_varying:  VARYING			{ $$ = make_str("varying"); }
		| /*EMPTY*/			{ $$ = EMPTY; }
		;

opt_charset:  CHARACTER SET ColId	{ $$ = cat2_str(make_str("character set"), $3); }
		| /*EMPTY*/				{ $$ = EMPTY; }
		;

opt_collate:  COLLATE ColId		{ $$ = cat2_str(make_str("collate"), $2); }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;

Datetime:  datetime
				{
					$$ = $1;
				}
		| TIMESTAMP opt_timezone
				{
					$$ = cat2_str(make_str("timestamp"), $2);
				}
		| TIME
				{
					$$ = make_str("time");
				}
		| INTERVAL opt_interval
				{
					$$ = cat2_str(make_str("interval"), $2);
				}
		;

datetime:  YEAR_P								{ $$ = make_str("year"); }
		| MONTH_P								{ $$ = make_str("month"); }
		| DAY_P									{ $$ = make_str("day"); }
		| HOUR_P								{ $$ = make_str("hour"); }
		| MINUTE_P								{ $$ = make_str("minute"); }
		| SECOND_P								{ $$ = make_str("second"); }
		;

opt_timezone:  WITH TIME ZONE				{ $$ = make_str("with time zone"); }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;

opt_interval:  datetime					{ $$ = $1; }
		| YEAR_P TO MONTH_P			{ $$ = make_str("year to #month"); }
		| DAY_P TO HOUR_P			{ $$ = make_str("day to hour"); }
		| DAY_P TO MINUTE_P			{ $$ = make_str("day to minute"); }
		| DAY_P TO SECOND_P			{ $$ = make_str("day to second"); }
		| HOUR_P TO MINUTE_P			{ $$ = make_str("hour to minute"); }
		| MINUTE_P TO SECOND_P			{ $$ = make_str("minute to second"); }
		| HOUR_P TO SECOND_P			{ $$ = make_str("hour to second"); }
		| /*EMPTY*/					{ $$ = EMPTY; }
		;


/*****************************************************************************
 *
 *	expression grammar
 *
 *****************************************************************************/

a_expr_or_null:  a_expr
				{ $$ = $1; }
		| NULL_P
				{
					$$ = make_str("null");
				}
		;

/* Expressions using row descriptors
 * Define row_descriptor to allow yacc to break the reduce/reduce conflict
 *  with singleton expressions.
 */
row_expr: '(' row_descriptor ')' IN '(' SubSelect ')'
				{
					$$ = cat_str(5, make_str("("), $2, make_str(") in ("), $6, make_str(")"));
				}
		| '(' row_descriptor ')' NOT IN '(' SubSelect ')'
				{
					$$ = cat_str(5, make_str("("), $2, make_str(") not in ("), $7, make_str(")"));
				}
		| '(' row_descriptor ')' all_Op sub_type  '(' SubSelect ')'
				{
					$$ = cat_str(8, make_str("("), $2, make_str(")"), $4, $5, make_str("("), $7, make_str(")"));
				}
		| '(' row_descriptor ')' all_Op '(' SubSelect ')'
				{
					$$ = cat_str(7, make_str("("), $2, make_str(")"), $4, make_str("("), $6, make_str(")"));
				}
		| '(' row_descriptor ')' all_Op '(' row_descriptor ')'
				{
					$$ = cat_str(7, make_str("("), $2, make_str(")"), $4, make_str("("), $6, make_str(")"));
				}
		;

row_descriptor:  row_list ',' a_expr
				{
					$$ = cat_str(3, $1, make_str(","), $3);
				}
		;

sub_type:  ANY                  { $$ = make_str("ANY"); }
         | ALL                  { $$ = make_str("ALL"); }
              ;


row_list:  row_list ',' a_expr
				{
					$$ = cat_str(3, $1, make_str(","), $3);
				}
		| a_expr
				{
					$$ = $1;
				}
		;

all_Op:  Op | MathOp;

MathOp:	'+'				{ $$ = make_str("+"); }
		| '-'			{ $$ = make_str("-"); }
		| '*'			{ $$ = make_str("*"); }
		| '%'			{ $$ = make_str("%"); }
                | '^'                   { $$ = make_str("^"); }
                | '|'                   { $$ = make_str("|"); }
		| '/'			{ $$ = make_str("/"); }
		| '<'			{ $$ = make_str("<"); }
		| '>'			{ $$ = make_str(">"); }
		| '='			{ $$ = make_str("="); }
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
 * com_expr is all the productions that are common to a_expr and b_expr;
 * it's factored out just to eliminate redundant coding.
 */

a_expr:  com_expr
				{	$$ = $1; }
		| a_expr TYPECAST Typename
				{	$$ = cat_str(3, $1, make_str("::"), $3); }
		 /*
                 * Can't collapse this into prior rule by using a_expr_or_null;
                 * that creates reduce/reduce conflicts.  Grumble.
                 */
                | NULL_P TYPECAST Typename
                                {
					$$ = cat2_str(make_str("null::"), $3);
                                }
		/*
                 * These operators must be called out explicitly in order to make use
                 * of yacc/bison's automatic operator-precedence handling.  All other
                 * operator names are handled by the generic productions using "Op",
                 * below; and all those operators will have the same precedence.
                 *
                 * If you add more explicitly-known operators, be sure to add them
                 * also to b_expr and to the MathOp list above.
                 */
		| '-' a_expr %prec UMINUS
				{	$$ = cat2_str(make_str("-"), $2); }
		| '%' a_expr
				{       $$ = cat2_str(make_str("%"), $2); }
		| '^' a_expr
				{       $$ = cat2_str(make_str("^"), $2); }
		| '|' a_expr
				{       $$ = cat2_str(make_str("|"), $2); }
/* not possible in embedded sql		| ':' a_expr	
				{       $$ = cat2_str(make_str(":"), $2); }
*/
		| ';' a_expr
				{       $$ = cat2_str(make_str(";"), $2); }
		| a_expr '%'
				{       $$ = cat2_str($1, make_str("%")); }
		| a_expr '^'
				{       $$ = cat2_str($1, make_str("^")); }
		| a_expr '|'
				{       $$ = cat2_str($1, make_str("|")); }
		| a_expr '+' a_expr
				{	$$ = cat_str(3, $1, make_str("+"), $3); }
		| a_expr '-' a_expr
				{	$$ = cat_str(3, $1, make_str("-"), $3); }
		| a_expr '*' a_expr
				{	$$ = cat_str(3, $1, make_str("*"), $3); }
		| a_expr '/' a_expr
				{	$$ = cat_str(3, $1, make_str("/"), $3); }
		| a_expr '%' a_expr
				{	$$ = cat_str(3, $1, make_str("%"), $3); }
		| a_expr '^' a_expr
				{	$$ = cat_str(3, $1, make_str("^"), $3); }
		| a_expr '|' a_expr
				{	$$ = cat_str(3, $1, make_str("|"), $3); }
		| a_expr '<' a_expr
				{	$$ = cat_str(3, $1, make_str("<"), $3); }
		| a_expr '>' a_expr
				{	$$ = cat_str(3, $1, make_str(">"), $3); }
		| a_expr '=' NULL_P
                                {       $$ = cat2_str($1, make_str("= NULL")); }
		/* We allow this for standards-broken SQL products, like MS stuff */
		| NULL_P '=' a_expr
                                {       $$ = cat2_str(make_str("= NULL"), $3); }
		| a_expr '=' a_expr
				{	$$ = cat_str(3, $1, make_str("="), $3); }
		| a_expr Op a_expr
				{	$$ = cat_str(3, $1, $2, $3); }
		| Op a_expr
				{	$$ = cat2_str($1, $2); }
		| a_expr Op
				{	$$ = cat2_str($1, $2); }
		| a_expr AND a_expr
				{	$$ = cat_str(3, $1, make_str("and"), $3); }
		| a_expr OR a_expr
				{	$$ = cat_str(3, $1, make_str("or"), $3); }
		| NOT a_expr
				{	$$ = cat2_str(make_str("not"), $2); }
		| a_expr LIKE a_expr
				{	$$ = cat_str(3, $1, make_str("like"), $3); }
		| a_expr NOT LIKE a_expr
				{	$$ = cat_str(3, $1, make_str("not like"), $4); }
		| a_expr ISNULL
				{	$$ = cat2_str($1, make_str("isnull")); }
		| a_expr IS NULL_P
				{	$$ = cat2_str($1, make_str("is null")); }
		| a_expr NOTNULL
				{	$$ = cat2_str($1, make_str("notnull")); }
		| a_expr IS NOT NULL_P
				{	$$ = cat2_str($1, make_str("is not null")); }
		/* IS TRUE, IS FALSE, etc used to be function calls
		 *  but let's make them expressions to allow the optimizer
		 *  a chance to eliminate them if a_expr is a constant string.
		 * - thomas 1997-12-22
		 */
		| a_expr IS TRUE_P
				{	$$ = cat2_str($1, make_str("is true")); }
		| a_expr IS NOT FALSE_P
				{	$$ = cat2_str($1, make_str("is not false")); }
		| a_expr IS FALSE_P
				{	$$ = cat2_str($1, make_str("is false")); }
		| a_expr IS NOT TRUE_P
				{	$$ = cat2_str($1, make_str("is not true")); }
		| a_expr BETWEEN b_expr AND b_expr
				{
					$$ = cat_str(5, $1, make_str("between"), $3, make_str("and"), $5); 
				}
		| a_expr NOT BETWEEN b_expr AND b_expr
				{
					$$ = cat_str(5, $1, make_str("not between"), $4, make_str("and"), $6); 
				}
		| a_expr IN '(' in_expr ')'
				{
					$$ = cat_str(4, $1, make_str(" in ("), $4, make_str(")")); 
				}
		| a_expr NOT IN '(' in_expr ')'
				{
					$$ = cat_str(4, $1, make_str(" not in ("), $5, make_str(")")); 
				}
		| a_expr all_Op sub_type '(' SubSelect ')'
				{
					$$ = cat_str(6, $1, $2, $3, make_str("("), $5, make_str(")")); 
				}
		| row_expr
				{       $$ = $1; }
		| cinputvariable
			        { $$ = make_str("?"); }
		;

/* Restricted expressions
 *
 * b_expr is a subset of the complete expression syntax
 *
 * Presently, AND, NOT, IS, IN, and NULL are the a_expr keywords that would
 * cause trouble in the places where b_expr is used.  For simplicity, we
 * just eliminate all the boolean-keyword-operator productions from b_expr.
 */
b_expr:  com_expr
				{
					$$ = $1;
				}
		| b_expr TYPECAST Typename
				{
					$$ = cat_str(3, $1, make_str("::"), $3);
				}
                | NULL_P TYPECAST Typename
                                {
					$$ = cat2_str(make_str("null::"), $3);
                                }
		| '-' b_expr %prec UMINUS
				{	$$ = cat2_str(make_str("-"), $2); }
		| '%' b_expr
				{       $$ = cat2_str(make_str("%"), $2); }
		| '^' b_expr
				{       $$ = cat2_str(make_str("^"), $2); }
		| '|' b_expr
				{       $$ = cat2_str(make_str("|"), $2); }
/* not possible in embedded sql		| ':' b_expr	
				{       $$ = cat2_str(make_str(":"), $2); }
*/
		| ';' b_expr
				{       $$ = cat2_str(make_str(";"), $2); }
		| b_expr '%'
				{       $$ = cat2_str($1, make_str("%")); }
		| b_expr '^'
				{       $$ = cat2_str($1, make_str("^")); }
		| b_expr '|'
				{       $$ = cat2_str($1, make_str("|")); }
		| b_expr '+' b_expr
				{	$$ = cat_str(3, $1, make_str("+"), $3); }
		| b_expr '-' b_expr
				{	$$ = cat_str(3, $1, make_str("-"), $3); }
		| b_expr '*' b_expr
				{	$$ = cat_str(3, $1, make_str("*"), $3); }
		| b_expr '/' b_expr
				{	$$ = cat_str(3, $1, make_str("/"), $3); }
		| b_expr '%' b_expr
				{	$$ = cat_str(3, $1, make_str("%"), $3); }
		| b_expr '^' b_expr
				{	$$ = cat_str(3, $1, make_str("^"), $3); }
		| b_expr '|' b_expr
				{	$$ = cat_str(3, $1, make_str("|"), $3); }
		| b_expr '<' b_expr
				{	$$ = cat_str(3, $1, make_str("<"), $3); }
		| b_expr '>' b_expr
				{	$$ = cat_str(3, $1, make_str(">"), $3); }
		| b_expr '=' b_expr
				{	$$ = cat_str(3, $1, make_str("="), $3); }
		| b_expr Op b_expr
				{	$$ = cat_str(3, $1, $2, $3); }
		| Op b_expr
				{	$$ = cat2_str($1, $2); }
		| b_expr Op
				{	$$ = cat2_str($1, $2); }
		| civariableonly
			        { 	$$ = $1; }
		;

/*
 * Productions that can be used in both a_expr and b_expr.
 *
 * Note: productions that refer recursively to a_expr or b_expr mostly
 * cannot appear here.  However, it's OK to refer to a_exprs that occur
 * inside parentheses, such as function arguments; that cannot introduce
 * ambiguity to the b_expr syntax.
 */
com_expr:  attr
				{	$$ = $1;  }
		| ColId opt_indirection
				{	$$ = cat2_str($1, $2);	}
		| AexprConst
				{	$$ = $1;  }
		| '(' a_expr_or_null ')'
				{	$$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| CAST '(' a_expr_or_null AS Typename ')'
				{ 	$$ = cat_str(5, make_str("cast("), $3, make_str("as"), $5, make_str(")")); }
		| case_expr
				{       $$ = $1; }
		| func_name '(' ')'
				{	$$ = cat2_str($1, make_str("()"));  }
		| func_name '(' expr_list ')'
				{	$$ = cat_str(4, $1, make_str("("), $3, make_str(")"));  }
		| func_name '(' DISTINCT expr_list ')'
				{	$$ = cat_str(4, $1, make_str("( distinct"), $4, make_str(")"));  }
		| func_name '(' '*' ')'
				{	$$ = cat2_str($1, make_str("(*)")); }
		| CURRENT_DATE
				{	$$ = make_str("current_date"); }
		| CURRENT_TIME
				{	$$ = make_str("current_time"); }
		| CURRENT_TIME '(' Iconst ')'
				{
					if (atol($3) != 0)
					{
						sprintf(errortext, "CURRENT_TIME(%s) precision not implemented; backend will use zero instead", $3);
						mmerror(ET_WARN, errortext);
					}

					$$ = make_str("current_time");
				}
		| CURRENT_TIMESTAMP
				{	$$ = make_str("current_timestamp"); }
		| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					if (atol($3) != 0)
					{
						sprintf(errortext, "CURRENT_TIMESTAMP(%s) precision not implemented; backend will use zero instead", $3);
						mmerror(ET_WARN, errortext);
					}

					$$ = make_str("current_timestamp");
				}
		| CURRENT_USER
				{	$$ = make_str("current_user"); }
		| USER
				{       $$ = make_str("user"); }
		| EXTRACT '(' extract_list ')'
				{	$$ = cat_str(3, make_str("extract("), $3, make_str(")")); }
		| POSITION '(' position_list ')'
				{	$$ = cat_str(3, make_str("position("), $3, make_str(")")); }
		| SUBSTRING '(' substr_list ')'
				{	$$ = cat_str(3, make_str("substring("), $3, make_str(")")); }
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
				{	$$ = cat_str(3, make_str("trim(both"), $4, make_str(")")); }
		| TRIM '(' LEADING trim_list ')'
				{	$$ = cat_str(3, make_str("trim(leading"), $4, make_str(")")); }
		| TRIM '(' TRAILING trim_list ')'
				{	$$ = cat_str(3, make_str("trim(trailing"), $4, make_str(")")); }
		| TRIM '(' trim_list ')'
				{	$$ = cat_str(3, make_str("trim("), $3, make_str(")")); }
		| '(' SubSelect ')'
				{	$$ = cat_str(3, make_str("("), $2, make_str(")")); }
		| EXISTS '(' SubSelect ')'
				{	$$ = cat_str(3, make_str("exists("), $3, make_str(")")); }
		;
/* 
 * This used to use ecpg_expr, but since there is no shift/reduce conflict
 * anymore, we can remove ecpg_expr. - MM
 */
opt_indirection:  '[' a_expr ']' opt_indirection
				{
					$$ = cat_str(4, make_str("["), $2, make_str("]"), $4);
				}
		| '[' a_expr ':' a_expr ']' opt_indirection
				{
					$$ = cat_str(6, make_str("["), $2, make_str(":"), $4, make_str("]"), $6);
				}
		| /* EMPTY */
				{	$$ = EMPTY; }
		;

expr_list:  a_expr_or_null
				{ $$ = $1; }
		| expr_list ',' a_expr_or_null
				{ $$ = cat_str(3, $1, make_str(","), $3); }
		| expr_list USING a_expr
				{ $$ = cat_str(3, $1, make_str("using"), $3); }
		;

extract_list:  extract_arg FROM a_expr
				{
					$$ = cat_str(3, $1, make_str("from"), $3);
				}
		| /* EMPTY */
				{	$$ = EMPTY; }
		| cinputvariable
			        { $$ = make_str("?"); }
		;

extract_arg:  datetime		{ $$ = $1; }
	| TIMEZONE_HOUR 	{ $$ = make_str("timezone_hour"); }	
	| TIMEZONE_MINUTE 	{ $$ = make_str("timezone_minute"); }	
		;

/* position_list uses b_expr not a_expr to avoid conflict with general IN */
position_list:  b_expr IN b_expr
				{	$$ = cat_str(3, $1, make_str("in"), $3); }
		| /* EMPTY */
				{	$$ = EMPTY; }
		;

substr_list:  expr_list substr_from substr_for
				{
					$$ = cat_str(3, $1, $2, $3);
				}
		| /* EMPTY */
				{	$$ = EMPTY; }
		;

substr_from:  FROM expr_list
				{	$$ = cat2_str(make_str("from"), $2); }
		| /* EMPTY */
				{
					$$ = EMPTY;
				}
		;

substr_for:  FOR expr_list
				{	$$ = cat2_str(make_str("for"), $2); }
		| /* EMPTY */
				{	$$ = EMPTY; }
		;

trim_list:  a_expr FROM expr_list
				{ $$ = cat_str(3, $1, make_str("from"), $3); }
		| FROM expr_list
				{ $$ = cat2_str(make_str("from"), $2); }
		| expr_list
				{ $$ = $1; }
		;

in_expr:  SubSelect
				{
					$$ = $1;
				}
		| in_expr_nodes
				{	$$ = $1; }
		;

in_expr_nodes:  a_expr
				{	$$ = $1; }
		| in_expr_nodes ',' a_expr
				{	$$ = cat_str(3, $1, make_str(","), $3);}
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
                                { $$ = cat_str(5, make_str("case"), $2, $3, $4, make_str("end")); }
                | NULLIF '(' a_expr ',' a_expr ')'
                                {
					$$ = cat_str(5, make_str("nullif("), $3, make_str(","), $5, make_str(")"));

					mmerror(ET_WARN, "NULLIF() not yet fully implemented");
                                }
                | COALESCE '(' expr_list ')'
                                {
					$$ = cat_str(3, make_str("coalesce("), $3, make_str(")"));
				}
		;

when_clause_list:  when_clause_list when_clause
                               { $$ = cat2_str($1, $2); }
               | when_clause
                               { $$ = $1; }
               ;

when_clause:  WHEN a_expr THEN a_expr_or_null
                               {
					$$ = cat_str(4, make_str("when"), $2, make_str("then"), $4);
                               }
               ;

case_default:  ELSE a_expr_or_null	{ $$ = cat2_str(make_str("else"), $2); }
               | /*EMPTY*/        	{ $$ = EMPTY; }
               ;

case_arg:  a_expr              {
                                       $$ = $1;
                               }
               | /*EMPTY*/
                               {       $$ = EMPTY; }
               ;

attr:  relation_name '.' attrs opt_indirection
				{
					$$ = cat_str(4, $1, make_str("."), $3, $4);
				}
		| ParamNo '.' attrs opt_indirection
				{
					$$ = cat_str(4, $1, make_str("."), $3, $4);
				}
		;

attrs:	  attr_name
				{ $$ = $1; }
		| attrs '.' attr_name
				{ $$ = cat_str(3, $1, make_str("."), $3); }
		| attrs '.' '*'
				{ $$ = make2_str($1, make_str(".*")); }
		;


/*****************************************************************************
 *
 *	target lists
 *
 *****************************************************************************/

/* Target lists as found in SELECT ... and INSERT VALUES ( ... ) */
target_list:  target_list ',' target_el
				{	$$ = cat_str(3, $1, make_str(","), $3);  }
		| target_el
				{	$$ = $1;  }
		;

/* AS is not optional because shift/red conflict with unary ops */
target_el:  a_expr_or_null AS ColLabel
				{
					$$ = cat_str(3, $1, make_str("as"), $3);
				}
		| a_expr_or_null
				{
					$$ = $1;
				}
		| relation_name '.' '*'
				{
					$$ = make2_str($1, make_str(".*"));
				}
		| '*'
				{
					$$ = make_str("*");
				}
		;

/* Target list as found in UPDATE table SET ... */
update_target_list:  update_target_list ',' update_target_el
				{	$$ = cat_str(3, $1, make_str(","),$3);  }
		| update_target_el
				{	$$ = $1;  }
		| '*'		{ $$ = make_str("*"); }
		;

update_target_el:  ColId opt_indirection '=' a_expr_or_null
				{
					$$ = cat_str(4, $1, $2, make_str("="), $4);
				}
		;

/*****************************************************************************
 *
 *     Names and constants
 *
 *****************************************************************************/

relation_name:	SpecialRuleRelation
				{
					$$ = $1;
				}
		| ColId
				{
					/* disallow refs to variable system tables */
					if (strcmp(LogRelationName, $1) == 0
					   || strcmp(VariableRelationName, $1) == 0) {
						sprintf(errortext, make_str("%s cannot be accessed by users"),$1);
						mmerror(ET_ERROR, errortext);
					}
					else
						$$ = $1;
				}
		;

database_name:			ColId			{ $$ = $1; };
access_method:			ident			{ $$ = $1; };
attr_name:				ColId			{ $$ = $1; };
class:					ident			{ $$ = $1; };
index_name:				ColId			{ $$ = $1; };

/* Functions
 * Include date/time keywords as SQL92 extension.
 * Include TYPE as a SQL92 unreserved keyword. - thomas 1997-10-05
 */
name:					ColId			{ $$ = $1; };
func_name:				ColId			{ $$ = $1; };

file_name:				Sconst			{ $$ = $1; };
/* NOT USED recipe_name:			ident			{ $$ = $1; };*/

/* Constants
 * Include TRUE/FALSE for SQL3 support. - thomas 1997-10-24
 */
AexprConst:  Iconst
				{
					$$ = $1;
				}
		| Fconst
				{
					$$ = $1;
				}
		| Sconst
				{
					$$ = $1;
				}
		 /* this rule formerly used Typename, but that causes reduce conf licts
                  * with subscripted column names ...
                  */
		| SimpleTypename Sconst
				{
					$$ = cat2_str($1, $2);
				}
		| ParamNo
				{	$$ = $1;  }
		| TRUE_P
				{
					$$ = make_str("true");
				}
		| FALSE_P
				{
					$$ = make_str("false");
				}
		;

ParamNo:  PARAM opt_indirection
				{
					$$ = cat2_str(make_name(), $2);
				}
		;

Iconst:  ICONST                                 { $$ = make_name();};
Fconst:  FCONST                                 { $$ = make_name();};
Sconst:  SCONST                                 {
							$$ = (char *)mm_alloc(strlen($1) + 3);
							$$[0]='\'';
				     		        strcpy($$+1, $1);
							$$[strlen($1)+2]='\0';
							$$[strlen($1)+1]='\'';
							free($1);
						}
UserId:  ident                                  { $$ = $1;};

/* Column and type identifier
 * Does not include explicit datetime types
 *  since these must be decoupled in Typename syntax.
 * Use ColId for most identifiers. - thomas 1997-10-21
 */
TypeId:  ColId
			{	$$ = $1; }
		| numeric
			{	$$ = $1; }
		| character
			{	$$ = $1; }
		;
/* Column identifier
 * Include date/time keywords as SQL92 extension.
 * Include TYPE as a SQL92 unreserved keyword. - thomas 1997-10-05
 * Add other keywords. Note that as the syntax expands,
 *  some of these keywords will have to be removed from this
 *  list due to shift/reduce conflicts in yacc. If so, move
 *  down to the ColLabel entity. - thomas 1997-11-06
 */
ColId:  ident					{ $$ = $1; }
		| datetime			{ $$ = $1; }
		| ABSOLUTE			{ $$ = make_str("absolute"); }
		| ACCESS			{ $$ = make_str("access"); }
		| ACTION			{ $$ = make_str("action"); }
		| AFTER				{ $$ = make_str("after"); }
		| AGGREGATE			{ $$ = make_str("aggregate"); }
		| BACKWARD			{ $$ = make_str("backward"); }
		| BEFORE			{ $$ = make_str("before"); }
		| CACHE				{ $$ = make_str("cache"); }
		| COMMENT			{ $$ = make_str("comment"); } 
		| COMMITTED			{ $$ = make_str("committed"); }
		| CONSTRAINTS			{ $$ = make_str("constraints"); }
		| CREATEDB			{ $$ = make_str("createdb"); }
		| CREATEUSER			{ $$ = make_str("createuser"); }
		| CYCLE				{ $$ = make_str("cycle"); }
		| DATABASE			{ $$ = make_str("database"); }
		| DEFERRABLE			{ $$ = make_str("deferrable"); }
		| DEFERRED			{ $$ = make_str("deferred"); }
		| DELIMITERS			{ $$ = make_str("delimiters"); }
		| DOUBLE			{ $$ = make_str("double"); }
		| EACH				{ $$ = make_str("each"); }
		| ENCODING			{ $$ = make_str("encoding"); }
		| EXCLUSIVE			{ $$ = make_str("exclusive"); }
		| FORWARD			{ $$ = make_str("forward"); }
		| FUNCTION			{ $$ = make_str("function"); }
		| HANDLER			{ $$ = make_str("handler"); }
		| IMMEDIATE			{ $$ = make_str("immediate"); }
		| INCREMENT			{ $$ = make_str("increment"); }
		| INDEX				{ $$ = make_str("index"); }
		| INHERITS			{ $$ = make_str("inherits"); }
		| INITIALLY			{ $$ = make_str("initially"); }
		| INSENSITIVE			{ $$ = make_str("insensitive"); }
		| INSTEAD			{ $$ = make_str("instead"); }
		| ISNULL			{ $$ = make_str("isnull"); }
		| ISOLATION			{ $$ = make_str("isolation"); }
		| KEY				{ $$ = make_str("key"); }
		| LANGUAGE			{ $$ = make_str("language"); }
		| LANCOMPILER			{ $$ = make_str("lancompiler"); }
		| LEVEL				{ $$ = make_str("level"); }
		| LOCATION			{ $$ = make_str("location"); }
		| MATCH				{ $$ = make_str("match"); }
		| MAXVALUE			{ $$ = make_str("maxvalue"); }
		| MINVALUE			{ $$ = make_str("minvalue"); }
		| MODE				{ $$ = make_str("mode"); }
		| NEXT				{ $$ = make_str("next"); }
		| NOCREATEDB			{ $$ = make_str("nocreatedb"); }
		| NOCREATEUSER			{ $$ = make_str("nocreateuser"); }
		| NOTHING			{ $$ = make_str("nothing"); }
		| NOTNULL			{ $$ = make_str("notnull"); }
		| OF				{ $$ = make_str("of"); }
		| OIDS				{ $$ = make_str("oids"); }
		| ONLY				{ $$ = make_str("only"); }
		| OPERATOR			{ $$ = make_str("operator"); }
		| OPTION			{ $$ = make_str("option"); }
		| PASSWORD			{ $$ = make_str("password"); }
		| PENDANT			{ $$ = make_str("pendant"); }
		| PRIOR				{ $$ = make_str("prior"); }
		| PRIVILEGES			{ $$ = make_str("privileges"); }
		| PROCEDURAL			{ $$ = make_str("procedural"); }
		| READ				{ $$ = make_str("read"); }
		| RELATIVE			{ $$ = make_str("relative"); }
		| RENAME			{ $$ = make_str("rename"); }
		| RESTRICT			{ $$ = make_str("restrict"); }
		| RETURNS			{ $$ = make_str("returns"); }
		| ROW				{ $$ = make_str("row"); }
		| RULE				{ $$ = make_str("rule"); }
		| SCROLL			{ $$ = make_str("scroll"); }
		| SEQUENCE                      { $$ = make_str("sequence"); }
		| SERIAL			{ $$ = make_str("serial"); }
		| SERIALIZABLE			{ $$ = make_str("serializable"); }
		| SHARE				{ $$ = make_str("share"); }
		| START				{ $$ = make_str("start"); }
		| STATEMENT			{ $$ = make_str("statement"); }
		| STDIN                         { $$ = make_str("stdin"); }
		| STDOUT                        { $$ = make_str("stdout"); }
		| SYSID                         { $$ = make_str("sysid"); }
		| TIME				{ $$ = make_str("time"); }
		| TIMESTAMP			{ $$ = make_str("timestamp"); }
		| TIMEZONE_HOUR                 { $$ = make_str("timezone_hour"); }
                | TIMEZONE_MINUTE               { $$ = make_str("timezone_minute"); }
		| TRIGGER			{ $$ = make_str("trigger"); }
		| TRUNCATE			{ $$ = make_str("truncate"); }
		| TRUSTED			{ $$ = make_str("trusted"); }
		| TYPE_P			{ $$ = make_str("type"); }
		| VALID				{ $$ = make_str("valid"); }
		| VERSION			{ $$ = make_str("version"); }
		| ZONE				{ $$ = make_str("zone"); }
		| SQL_AT			{ $$ = make_str("at"); }
		| SQL_BOOL			{ $$ = make_str("bool"); }
		| SQL_BREAK			{ $$ = make_str("break"); }
		| SQL_CALL			{ $$ = make_str("call"); }
		| SQL_CONNECT			{ $$ = make_str("connect"); }
		| SQL_CONTINUE			{ $$ = make_str("continue"); }
		| SQL_DEALLOCATE		{ $$ = make_str("deallocate"); }
		| SQL_DISCONNECT		{ $$ = make_str("disconnect"); }
		| SQL_FOUND			{ $$ = make_str("found"); }
		| SQL_GO			{ $$ = make_str("go"); }
		| SQL_GOTO			{ $$ = make_str("goto"); }
		| SQL_IDENTIFIED		{ $$ = make_str("identified"); }
		| SQL_INDICATOR			{ $$ = make_str("indicator"); }
		| SQL_INT			{ $$ = make_str("int"); }
		| SQL_LONG			{ $$ = make_str("long"); }
		| SQL_OFF			{ $$ = make_str("off"); }
		| SQL_OPEN			{ $$ = make_str("open"); }
		| SQL_PREPARE			{ $$ = make_str("prepare"); }
		| SQL_RELEASE			{ $$ = make_str("release"); }
		| SQL_SECTION			{ $$ = make_str("section"); }
		| SQL_SHORT			{ $$ = make_str("short"); }
		| SQL_SIGNED			{ $$ = make_str("signed"); }
		| SQL_SQLERROR			{ $$ = make_str("sqlerror"); }
		| SQL_SQLPRINT			{ $$ = make_str("sqlprint"); }
		| SQL_SQLWARNING		{ $$ = make_str("sqlwarning"); }
		| SQL_STOP			{ $$ = make_str("stop"); }
		| SQL_STRUCT			{ $$ = make_str("struct"); }
		| SQL_UNSIGNED			{ $$ = make_str("unsigned"); }
		| SQL_VAR			{ $$ = make_str("var"); }
		| SQL_WHENEVER			{ $$ = make_str("whenever"); }
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
		| ABORT_TRANS                                   { $$ = make_str("abort"); }
		| ANALYZE                                       { $$ = make_str("analyze"); }
		| BINARY                                        { $$ = make_str("binary"); }
		| CASE                                        { $$ = make_str("case"); }
		| CLUSTER						{ $$ = make_str("cluster"); }
		| COALESCE                                        { $$ = make_str("coalesce"); }
		| CONSTRAINT					{ $$ = make_str("constraint"); }
		| COPY							{ $$ = make_str("copy"); }
		| CURRENT							{ $$ = make_str("current"); }
		| DO							{ $$ = make_str("do"); }
		| ELSE                                        { $$ = make_str("else"); }
		| END_TRANS                                        { $$ = make_str("end"); }
		| EXPLAIN							{ $$ = make_str("explain"); }
		| EXTEND							{ $$ = make_str("extend"); }
		| FALSE_P							{ $$ = make_str("false"); }
		| FOREIGN						{ $$ = make_str("foreign"); }
		| GROUP							{ $$ = make_str("group"); }
		| LISTEN							{ $$ = make_str("listen"); }
		| LOAD							{ $$ = make_str("load"); }
		| LOCK_P							{ $$ = make_str("lock"); }
		| MOVE							{ $$ = make_str("move"); }
		| NEW							{ $$ = make_str("new"); }
		| NONE							{ $$ = make_str("none"); }
		| NULLIF                                        { $$ = make_str("nullif"); }
		| ORDER							{ $$ = make_str("order"); }
		| POSITION						{ $$ = make_str("position"); }
		| PRECISION						{ $$ = make_str("precision"); }
		| RESET							{ $$ = make_str("reset"); }
		| SETOF							{ $$ = make_str("setof"); }
		| SHOW							{ $$ = make_str("show"); }
		| TABLE							{ $$ = make_str("table"); }
		| THEN                                        { $$ = make_str("then"); }
		| TRANSACTION					{ $$ = make_str("transaction"); }
		| TRUE_P						{ $$ = make_str("true"); }
		| VACUUM					{ $$ = make_str("vacuum"); }
		| VERBOSE						{ $$ = make_str("verbose"); }
		| WHEN                                        { $$ = make_str("when"); }
		;

SpecialRuleRelation:  CURRENT
				{
					if (QueryIsRule)
						$$ = make_str("current");
					else
						mmerror(ET_ERROR, "CURRENT used in non-rule query");
				}
		| NEW
				{
					if (QueryIsRule)
						$$ = make_str("new");
					else
						mmerror(ET_ERROR, "NEW used in non-rule query");
				}
		;

/*
 * and now special embedded SQL stuff
 */

/*
 * the exec sql connect statement: connect to the given database 
 */
ECPGConnect: SQL_CONNECT TO connection_target opt_connection_name opt_user
		{
			$$ = cat_str(5, $3, make_str(","), $5, make_str(","), $4);
                }
	| SQL_CONNECT TO DEFAULT
        	{
                	$$ = make_str("NULL,NULL,NULL,\"DEFAULT\"");
                }
      /* also allow ORACLE syntax */
        | SQL_CONNECT ora_user
                {
		       $$ = cat_str(3, make_str("NULL,"), $2, make_str(",NULL"));
		}

connection_target: database_name opt_server opt_port
                {
		  /* old style: dbname[@server][:port] */
		  if (strlen($2) > 0 && *($2) != '@')
		  {
		    sprintf(errortext, "parse error at or near '%s'", $2);
		    mmerror(ET_ERROR, errortext);
		  }

		  $$ = make3_str(make_str("\""), make3_str($1, $2, $3), make_str("\""));
		}
        |  db_prefix server opt_port '/' database_name opt_options
                {
		  /* new style: <tcp|unix>:postgresql://server[:port][/dbname] */
                  if (strncmp($2, "://", strlen("://")) != 0)
		  {
		    sprintf(errortext, "parse error at or near '%s'", $2);
		    mmerror(ET_ERROR, errortext);
		  }

		  if (strncmp($1, "unix", strlen("unix")) == 0 && strncmp($2 + strlen("://"), "localhost", strlen("localhost")) != 0)
		  {
		    sprintf(errortext, "unix domain sockets only work on 'localhost' but not on '%9.9s'", $2);
                    mmerror(ET_ERROR, errortext);
		  }

		  if (strncmp($1, "unix", strlen("unix")) != 0 && strncmp($1, "tcp", strlen("tcp")) != 0)
		  {
		    sprintf(errortext, "only protocols 'tcp' and 'unix' are supported");
                    mmerror(ET_ERROR, errortext);
		  }
	
		  $$ = make2_str(make3_str(make_str("\""), $1, $2), make3_str(make3_str($3, make_str("/"), $5),  $6, make_str("\"")));
		}
	| char_variable
                {
		  $$ = $1;
		}
	| Sconst
		{
		  $$ = mm_strdup($1);
		  $$[0] = '\"';
		  $$[strlen($$) - 1] = '\"';
		  free($1);
		}

db_prefix: ident cvariable
                {
		  if (strcmp($2, "postgresql") != 0 && strcmp($2, "postgres") != 0)
		  {
		    sprintf(errortext, "parse error at or near '%s'", $2);
		    mmerror(ET_ERROR, errortext);	
		  }

		  if (strcmp($1, "tcp") != 0 && strcmp($1, "unix") != 0)
		  {
		    sprintf(errortext, "Illegal connection type %s", $1);
		    mmerror(ET_ERROR, errortext);
		  }

		  $$ = make3_str( $1, make_str(":"), $2);
		}
        
server: Op server_name
                {
		  if (strcmp($1, "@") != 0 && strcmp($1, "://") != 0)
		  {
		    sprintf(errortext, "parse error at or near '%s'", $1);
		    mmerror(ET_ERROR, errortext);
		  }

		  $$ = make2_str($1, $2);
	        }

opt_server: server { $$ = $1; }
        | /* empty */ { $$ = EMPTY; }

server_name: ColId   { $$ = $1; }
        | ColId '.' server_name { 	$$ = make3_str($1, make_str("."), $3); }

opt_port: ':' Iconst { $$ = make2_str(make_str(":"), $2); }
        | /* empty */ { $$ = EMPTY; }

opt_connection_name: AS connection_target { $$ = $2; }
        | /* empty */ { $$ = make_str("NULL"); }

opt_user: USER ora_user { $$ = $2; }
          | /* empty */ { $$ = make_str("NULL,NULL"); }

ora_user: user_name
		{
                        $$ = cat2_str($1, make_str(", NULL"));
	        }
	| user_name '/' user_name
		{
        		$$ = cat_str(3, $1, make_str(","), $3);
                }
        | user_name SQL_IDENTIFIED BY user_name
                {
        		$$ = cat_str(3, $1, make_str(","), $4);
                }
        | user_name USING user_name
                {
        		$$ = cat_str(3, $1, make_str(","), $3);
                }

user_name: UserId       { if ($1[0] == '\"')
				$$ = $1;
			  else
				$$ = make3_str(make_str("\""), $1, make_str("\""));
			}
        | char_variable { $$ = $1; }
        | SCONST        { $$ = make3_str(make_str("\""), $1, make_str("\"")); }

char_variable: cvariable
		{ /* check if we have a char variable */
			struct variable *p = find_variable($1);
			enum ECPGttype typ = p->type->typ;

			/* if array see what's inside */
			if (typ == ECPGt_array)
				typ = p->type->u.element->typ;

                        switch (typ)
                        {
                            case ECPGt_char:
                            case ECPGt_unsigned_char:
                                $$ = $1;
                                break;
                            case ECPGt_varchar:
                                $$ = make2_str($1, make_str(".arr"));
                                break;
                            default:
                                mmerror(ET_ERROR, "invalid datatype");
                                break;
                        }
		}

opt_options: Op ColId
		{
			if (strlen($1) == 0)
				mmerror(ET_ERROR, "parse error");
				
			if (strcmp($1, "?") != 0)
			{
				sprintf(errortext, "parse error at or near %s", $1);
				mmerror(ET_ERROR, errortext);
			}
			
			$$ = make2_str(make_str("?"), $2);
		}
	| /* empty */ { $$ = EMPTY; }

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
							sprintf(errortext, "cursor %s already defined", $2);
							mmerror(ET_ERROR, errortext);
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
ECPGDeallocate:	SQL_DEALLOCATE SQL_PREPARE ident	{ $$ = cat_str(3, make_str("ECPGdeallocate(__LINE__, \""), $3, make_str("\");")); }

/*
 * variable declaration inside the exec sql declare block
 */
ECPGDeclaration: sql_startdeclare
	{
		fputs("/* exec sql begin declare section */", yyout);
	        output_line_number();
	}
	variable_declarations sql_enddeclare
	{
		fprintf(yyout, "%s/* exec sql end declare section */", $3);
		free($3);
		output_line_number();
	}

sql_startdeclare : ecpgstart BEGIN_TRANS DECLARE SQL_SECTION ';' {}

sql_enddeclare: ecpgstart END_TRANS DECLARE SQL_SECTION ';' {}

variable_declarations: /* empty */
	{
		$$ = EMPTY;
	}
	| declaration variable_declarations
	{
		$$ = cat2_str($1, $2);
	}

declaration: storage_clause
	{
		actual_storage[struct_level] = mm_strdup($1);
	}
	type
	{
		actual_type[struct_level].type_enum = $3.type_enum;
		actual_type[struct_level].type_dimension = $3.type_dimension;
		actual_type[struct_level].type_index = $3.type_index;
	}
	variable_list ';'
	{
 		$$ = cat_str(4, $1, $3.type_str, $5, make_str(";\n"));
	}

storage_clause : S_EXTERN	{ $$ = make_str("extern"); }
       | S_STATIC		{ $$ = make_str("static"); }
       | S_SIGNED		{ $$ = make_str("signed"); }
       | S_CONST		{ $$ = make_str("const"); }
       | S_REGISTER		{ $$ = make_str("register"); }
       | S_AUTO			{ $$ = make_str("auto"); }
       | /* empty */		{ $$ = EMPTY; }

type: simple_type
		{
			$$.type_enum = $1;
			$$.type_str = mm_strdup(ECPGtype_name($1));
			$$.type_dimension = -1;
  			$$.type_index = -1;
		}
	| varchar_type
		{
			$$.type_enum = ECPGt_varchar;
			$$.type_str = EMPTY;
			$$.type_dimension = -1;
  			$$.type_index = -1;
		}
	| struct_type
		{
			$$.type_enum = ECPGt_struct;
			$$.type_str = $1;
			$$.type_dimension = -1;
  			$$.type_index = -1;
		}
	| union_type
		{
			$$.type_enum = ECPGt_union;
			$$.type_str = $1;
			$$.type_dimension = -1;
  			$$.type_index = -1;
		}
	| enum_type
		{
			$$.type_str = $1;
			$$.type_enum = ECPGt_int;
			$$.type_dimension = -1;
  			$$.type_index = -1;
		}
	| symbol
		{
			/* this is for typedef'ed types */
			struct typedefs *this = get_typedef($1);

			$$.type_str = (this->type->type_enum == ECPGt_varchar) ? EMPTY : mm_strdup(this->name);
                        $$.type_enum = this->type->type_enum;
			$$.type_dimension = this->type->type_dimension;
  			$$.type_index = this->type->type_index;
			struct_member_list[struct_level] = ECPGstruct_member_dup(this->struct_member_list);
		}

enum_type: s_enum '{' c_list '}'
	{
		$$ = cat_str(4, $1, make_str("{"), $3, make_str("}"));
	}
	
s_enum: S_ENUM opt_symbol	{ $$ = cat2_str(make_str("enum"), $2); }

struct_type: s_struct '{' variable_declarations '}'
	{
	    ECPGfree_struct_member(struct_member_list[struct_level]);
	    free(actual_storage[struct_level--]);
	    $$ = cat_str(4, $1, make_str("{"), $3, make_str("}"));
	}

union_type: s_union '{' variable_declarations '}'
	{
	    ECPGfree_struct_member(struct_member_list[struct_level]);
	    free(actual_storage[struct_level--]);
	    $$ = cat_str(4, $1, make_str("{"), $3, make_str("}"));
	}

s_struct : S_STRUCT opt_symbol
        {
            struct_member_list[struct_level++] = NULL;
            if (struct_level >= STRUCT_DEPTH)
                 mmerror(ET_ERROR, "Too many levels in nested structure definition");
	    $$ = cat2_str(make_str("struct"), $2);
	}

s_union : S_UNION opt_symbol
        {
            struct_member_list[struct_level++] = NULL;
            if (struct_level >= STRUCT_DEPTH)
                 mmerror(ET_ERROR, "Too many levels in nested structure definition");
	    $$ = cat2_str(make_str("union"), $2);
	}

opt_symbol: /* empty */ 	{ $$ = EMPTY; }
	| symbol		{ $$ = $1; }

simple_type: S_SHORT		{ $$ = ECPGt_short; }
           | S_UNSIGNED S_SHORT { $$ = ECPGt_unsigned_short; }
	   | S_INT 		{ $$ = ECPGt_int; }
           | S_UNSIGNED S_INT	{ $$ = ECPGt_unsigned_int; }
	   | S_LONG		{ $$ = ECPGt_long; }
           | S_UNSIGNED S_LONG	{ $$ = ECPGt_unsigned_long; }
           | S_FLOAT		{ $$ = ECPGt_float; }
           | S_DOUBLE		{ $$ = ECPGt_double; }
	   | S_BOOL		{ $$ = ECPGt_bool; };
	   | S_CHAR		{ $$ = ECPGt_char; }
           | S_UNSIGNED S_CHAR	{ $$ = ECPGt_unsigned_char; }

varchar_type:  S_VARCHAR		{ $$ = ECPGt_varchar; }

variable_list: variable 
	{
		$$ = $1;
	}
	| variable_list ',' variable
	{
		$$ = cat_str(3, $1, make_str(","), $3);
	}

variable: opt_pointer symbol opt_array_bounds opt_initializer
		{
			struct ECPGtype * type;
                        int dimension = $3.index1; /* dimension of array */
                        int length = $3.index2;    /* lenght of string */
                        char dim[14L], ascii_len[12];

			adjust_array(actual_type[struct_level].type_enum, &dimension, &length, actual_type[struct_level].type_dimension, actual_type[struct_level].type_index, strlen($1));

			switch (actual_type[struct_level].type_enum)
			{
			   case ECPGt_struct:
			   case ECPGt_union:
                               if (dimension < 0)
                                   type = ECPGmake_struct_type(struct_member_list[struct_level], actual_type[struct_level].type_enum);
                               else
                                   type = ECPGmake_array_type(ECPGmake_struct_type(struct_member_list[struct_level], actual_type[struct_level].type_enum), dimension); 

                               $$ = cat_str(4, $1, mm_strdup($2), $3.str, $4);
                               break;
                           case ECPGt_varchar:
                               if (dimension == -1)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

                               switch(dimension)
                               {
                                  case 0:
				  case -1:
                                  case 1:
                                      *dim = '\0';
                                      break;
                                  default:
                                      sprintf(dim, "[%d]", dimension);
                                      break;
                               }
			       sprintf(ascii_len, "%d", length);

                               if (length == 0)
				   mmerror(ET_ERROR, "pointer to varchar are not implemented");

			       if (dimension == 0)
				   $$ = cat_str(7, mm_strdup(actual_storage[struct_level]), make2_str(make_str(" struct varchar_"), mm_strdup($2)), make_str(" { int len; char arr["), mm_strdup(ascii_len), make_str("]; } *"), mm_strdup($2), $4);
			       else
                                   $$ = cat_str(8, mm_strdup(actual_storage[struct_level]), make2_str(make_str(" struct varchar_"), mm_strdup($2)), make_str(" { int len; char arr["), mm_strdup(ascii_len), make_str("]; } "), mm_strdup($2), mm_strdup(dim), $4);

                               break;
                           case ECPGt_char:
                           case ECPGt_unsigned_char:
                               if (dimension == -1)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

			       $$ = cat_str(4, $1, mm_strdup($2), $3.str, $4);
                               break;
                           default:
                               if (dimension < 0)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, 1);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, 1), dimension);

			       $$ = cat_str(4, $1, mm_strdup($2), $3.str, $4);
                               break;
			}

			if (struct_level == 0)
				new_variable($2, type);
			else
				ECPGmake_struct_member($2, type, &(struct_member_list[struct_level - 1]));

			free($2);
		}

opt_initializer: /* empty */		{ $$ = EMPTY; }
	| '=' c_term			{ $$ = cat2_str(make_str("="), $2); }

opt_pointer: /* empty */	{ $$ = EMPTY; }
	| '*'			{ $$ = make_str("*"); }

/*
 * As long as the prepare statement is not supported by the backend, we will
 * try to simulate it here so we get dynamic SQL 
 */
ECPGDeclare: DECLARE STATEMENT ident
	{
		/* this is only supported for compatibility */
		$$ = cat_str(3, make_str("/* declare statement"), $3, make_str("*/"));
	}
/*
 * the exec sql disconnect statement: disconnect from the given database 
 */
ECPGDisconnect: SQL_DISCONNECT dis_name { $$ = $2; }

dis_name: connection_object	{ $$ = $1; }
	| CURRENT	{ $$ = make_str("CURRENT"); }
	| ALL		{ $$ = make_str("ALL"); }
	| /* empty */	{ $$ = make_str("CURRENT"); }

connection_object: connection_target { $$ = $1; }
	| DEFAULT	{ $$ = make_str("DEFAULT"); }

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
	| EXECUTE ident 
	{
		struct variable *thisquery = (struct variable *)mm_alloc(sizeof(struct variable));

		thisquery->type = &ecpg_query;
		thisquery->brace_level = 0;
		thisquery->next = NULL;
		thisquery->name = (char *) mm_alloc(sizeof("ECPGprepared_statement(\"\")") + strlen($2));
		sprintf(thisquery->name, "ECPGprepared_statement(\"%s\")", $2);

		add_variable(&argsinsert, thisquery, &no_indicator); 
	} ecpg_using
	{
		$$ = make_str("?");
	}

execstring: char_variable |
	CSTRING	 { $$ = make3_str(make_str("\""), $1, make_str("\"")); };

/*
 * the exec sql free command to deallocate a previously
 * prepared statement
 */
ECPGFree:	SQL_FREE ident	{ $$ = $2; }

/*
 * open is an open cursor, at the moment this has to be removed
 */
ECPGOpen: SQL_OPEN name ecpg_using {
		$$ = $2;
};

ecpg_using: /* empty */		{ $$ = EMPTY; }
	| USING variablelist	{
					/* mmerror ("open cursor with variables not implemented yet"); */
					$$ = EMPTY;
				}

variablelist: cinputvariable | cinputvariable ',' variablelist

/*
 * As long as the prepare statement is not supported by the backend, we will
 * try to simulate it here so we get dynamic SQL 
 */
ECPGPrepare: SQL_PREPARE ident FROM execstring
	{
		$$ = cat2_str(make3_str(make_str("\""), $2, make_str("\",")), $4);
	}

/*
 * for compatibility with ORACLE we will also allow the keyword RELEASE
 * after a transaction statement to disconnect from the database.
 */

ECPGRelease: TransactionStmt SQL_RELEASE
	{
		if (strcmp($1, "begin") == 0)
                        mmerror(ET_ERROR, "RELEASE does not make sense when beginning a transaction");

		fprintf(yyout, "ECPGtrans(__LINE__, %s, \"%s\");",
				connection ? connection : "NULL", $1);
		whenever_action(0);
		fprintf(yyout, "ECPGdisconnect(__LINE__, \"\");"); 
		whenever_action(0);
		free($1);
	}

/* 
 * set/reset the automatic transaction mode, this needs a differnet handling
 * as the other set commands
 */
ECPGSetAutocommit:  SET SQL_AUTOCOMMIT to_equal on_off
           		{
				$$ = $4;
                        }

on_off:	ON		{ $$ = make_str("on"); }
	| SQL_OFF	{ $$ = make_str("off"); }

to_equal:	TO | '=';

/* 
 * set the actual connection, this needs a differnet handling as the other
 * set commands
 */
ECPGSetConnection:  SET SQL_CONNECTION to_equal connection_object
           		{
				$$ = $4;
                        }

/*
 * define a new type for embedded SQL
 */
ECPGTypedef: TYPE_P symbol IS ctype opt_type_array_bounds opt_reference
	{
		/* add entry to list */
		struct typedefs *ptr, *this;
		int dimension = $5.index1;
		int length = $5.index2;

		for (ptr = types; ptr != NULL; ptr = ptr->next)
		{
			if (strcmp($2, ptr->name) == 0)
			{
			        /* re-definition is a bug */
				sprintf(errortext, "type %s already defined", $2);
				mmerror(ET_ERROR, errortext);
	                }
		}

		adjust_array($4.type_enum, &dimension, &length, $4.type_dimension, $4.type_index, strlen($6));

        	this = (struct typedefs *) mm_alloc(sizeof(struct typedefs));

        	/* initial definition */
	        this->next = types;
	        this->name = $2;
		this->type = (struct this_type *) mm_alloc(sizeof(struct this_type));
		this->type->type_enum = $4.type_enum;
		this->type->type_str = mm_strdup($2);
		this->type->type_dimension = dimension; /* dimension of array */
		this->type->type_index = length;    /* lenght of string */
		this->struct_member_list = struct_member_list[struct_level];

		if ($4.type_enum != ECPGt_varchar &&
		    $4.type_enum != ECPGt_char &&
	            $4.type_enum != ECPGt_unsigned_char &&
		    this->type->type_index >= 0)
                            mmerror(ET_ERROR, "No multi-dimensional array support for simple data types");

        	types = this;

		$$ = cat_str(7, make_str("/* exec sql type"), mm_strdup($2), make_str("is"), mm_strdup($4.type_str), mm_strdup($5.str), $6, make_str("*/"));
	}

opt_type_array_bounds:  '[' ']' opt_type_array_bounds
			{
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make_str("[]"), $3.str);
                        }
		| '(' ')' opt_type_array_bounds
			{
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make_str("[]"), $3.str);
                        }
		| '[' Iresult ']' opt_type_array_bounds
			{
			    char *txt = mm_alloc(20L);

			    sprintf (txt, "%d", $2);
                            $$.index1 = $2;
                            $$.index2 = $4.index1;
                            $$.str = cat_str(4, make_str("["), txt, make_str("]"), $4.str);
                        }
		| '(' Iresult ')' opt_type_array_bounds
			{
			    char *txt = mm_alloc(20L);

			    sprintf (txt, "%d", $2);
                            $$.index1 = $2;
                            $$.index2 = $4.index1;
                            $$.str = cat_str(4, make_str("["), txt, make_str("]"), $4.str);
                        }
		| /* EMPTY */
			{
                            $$.index1 = -1;
                            $$.index2 = -1;
                            $$.str= EMPTY;
                        }
		;

opt_reference: SQL_REFERENCE { $$ = make_str("reference"); }
	| /* empty */ 	     { $$ = EMPTY; }

ctype: CHAR
	{
		$$.type_str = make_str("char");
                $$.type_enum = ECPGt_char;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| VARCHAR
	{
		$$.type_str = make_str("varchar");
                $$.type_enum = ECPGt_varchar;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| FLOAT
	{
		$$.type_str = make_str("float");
                $$.type_enum = ECPGt_float;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| DOUBLE
	{
		$$.type_str = make_str("double");
                $$.type_enum = ECPGt_double;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| opt_signed SQL_INT
	{
		$$.type_str = make_str("int");
       	        $$.type_enum = ECPGt_int;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_ENUM
	{
		$$.type_str = make_str("int");
       	        $$.type_enum = ECPGt_int;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| opt_signed SQL_SHORT
	{
		$$.type_str = make_str("short");
       	        $$.type_enum = ECPGt_short;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| opt_signed SQL_LONG
	{
		$$.type_str = make_str("long");
       	        $$.type_enum = ECPGt_long;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_BOOL
	{
		$$.type_str = make_str("bool");
       	        $$.type_enum = ECPGt_bool;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_UNSIGNED SQL_INT
	{
		$$.type_str = make_str("unsigned int");
       	        $$.type_enum = ECPGt_unsigned_int;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_UNSIGNED SQL_SHORT
	{
		$$.type_str = make_str("unsigned short");
       	        $$.type_enum = ECPGt_unsigned_short;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_UNSIGNED SQL_LONG
	{
		$$.type_str = make_str("unsigned long");
       	        $$.type_enum = ECPGt_unsigned_long;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_STRUCT
	{
		struct_member_list[struct_level++] = NULL;
		if (struct_level >= STRUCT_DEPTH)
        		mmerror(ET_ERROR, "Too many levels in nested structure definition");
	} '{' sql_variable_declarations '}'
	{
		ECPGfree_struct_member(struct_member_list[struct_level--]);
		$$.type_str = cat_str(3, make_str("struct {"), $4, make_str("}"));
		$$.type_enum = ECPGt_struct;
                $$.type_index = -1;
                $$.type_dimension = -1;
	}
	| UNION
	{
		struct_member_list[struct_level++] = NULL;
		if (struct_level >= STRUCT_DEPTH)
        		mmerror(ET_ERROR, "Too many levels in nested structure definition");
	} '{' sql_variable_declarations '}'
	{
		ECPGfree_struct_member(struct_member_list[struct_level--]);
		$$.type_str = cat_str(3, make_str("union {"), $4, make_str("}"));
		$$.type_enum = ECPGt_union;
                $$.type_index = -1;
                $$.type_dimension = -1;
	}
	| symbol
	{
		struct typedefs *this = get_typedef($1);

		$$.type_str = mm_strdup($1);
		$$.type_enum = this->type->type_enum;
		$$.type_dimension = this->type->type_dimension;
		$$.type_index = this->type->type_index;
		struct_member_list[struct_level] = this->struct_member_list;
	}

opt_signed: SQL_SIGNED | /* empty */

sql_variable_declarations: /* empty */
	{
		$$ = EMPTY;
	}
	| sql_declaration sql_variable_declarations
	{
		$$ = cat2_str($1, $2);
	}
	;

sql_declaration: ctype
	{
		actual_type[struct_level].type_enum = $1.type_enum;
		actual_type[struct_level].type_dimension = $1.type_dimension;
		actual_type[struct_level].type_index = $1.type_index;
	}
	sql_variable_list ';'
	{
		$$ = cat_str(3, $1.type_str, $3, make_str(";"));
	}

sql_variable_list: sql_variable 
	{
		$$ = $1;
	}
	| sql_variable_list ',' sql_variable
	{
		$$ = cat_str(3, $1, make_str(","), $3);
	}

sql_variable: opt_pointer symbol opt_array_bounds
		{
			int dimension = $3.index1;
			int length = $3.index2;
			struct ECPGtype * type;
                        char dim[14L];

			adjust_array(actual_type[struct_level].type_enum, &dimension, &length, actual_type[struct_level].type_dimension, actual_type[struct_level].type_index, strlen($1));

			switch (actual_type[struct_level].type_enum)
			{
			   case ECPGt_struct:
			   case ECPGt_union:
                               if (dimension < 0)
                                   type = ECPGmake_struct_type(struct_member_list[struct_level], actual_type[struct_level].type_enum);
                               else
                                   type = ECPGmake_array_type(ECPGmake_struct_type(struct_member_list[struct_level], actual_type[struct_level].type_enum), dimension); 

                               break;
                           case ECPGt_varchar:
                               if (dimension == -1)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

                               switch(dimension)
                               {
                                  case 0:
                                      strcpy(dim, "[]");
                                      break;
				  case -1:
                                  case 1:
                                      *dim = '\0';
                                      break;
                                  default:
                                      sprintf(dim, "[%d]", dimension);
                                      break;
                                }

                               break;
                           case ECPGt_char:
                           case ECPGt_unsigned_char:
                               if (dimension == -1)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

                               break;
                           default:
			       if (length >= 0)
                	            mmerror(ET_ERROR, "No multi-dimensional array support for simple data types");

                               if (dimension < 0)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, 1);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, 1), dimension);

                               break;
			}

			if (struct_level == 0)
				new_variable($2, type);
			else
				ECPGmake_struct_member($2, type, &(struct_member_list[struct_level - 1]));

			$$ = cat_str(3, $1, $2, $3.str);
		}

/*
 * define the type of one variable for embedded SQL
 */
ECPGVar: SQL_VAR symbol IS ctype opt_type_array_bounds opt_reference
	{
		struct variable *p = find_variable($2);
		int dimension = $5.index1;
		int length = $5.index2;
		struct ECPGtype * type;

		adjust_array($4.type_enum, &dimension, &length, $4.type_dimension, $4.type_index, strlen($6));

		switch ($4.type_enum)
		{
		   case ECPGt_struct:
		   case ECPGt_union:
                        if (dimension < 0)
                            type = ECPGmake_struct_type(struct_member_list[struct_level], $4.type_enum);
                        else
                            type = ECPGmake_array_type(ECPGmake_struct_type(struct_member_list[struct_level], $4.type_enum), dimension); 
                        break;
                   case ECPGt_varchar:
                        if (dimension == -1)
                            type = ECPGmake_simple_type($4.type_enum, length);
                        else
                            type = ECPGmake_array_type(ECPGmake_simple_type($4.type_enum, length), dimension);

			break;
                   case ECPGt_char:
                   case ECPGt_unsigned_char:
                        if (dimension == -1)
                            type = ECPGmake_simple_type($4.type_enum, length);
                        else
                            type = ECPGmake_array_type(ECPGmake_simple_type($4.type_enum, length), dimension);

			break;
		   default:
			if (length >= 0)
                	    mmerror(ET_ERROR, "No multi-dimensional array support for simple data types");

                        if (dimension < 0)
                            type = ECPGmake_simple_type($4.type_enum, 1);
                        else
                            type = ECPGmake_array_type(ECPGmake_simple_type($4.type_enum, 1), dimension);

			break;
		}	

		ECPGfree_type(p->type);
		p->type = type;

		$$ = cat_str(7, make_str("/* exec sql var"), mm_strdup($2), make_str("is"), mm_strdup($4.type_str), mm_strdup($5.str), $6, make_str("*/"));
	}

/*
 * whenever statement: decide what to do in case of error/no data found
 * according to SQL standards we lack: SQLSTATE, CONSTRAINT and SQLEXCEPTION
 */
ECPGWhenever: SQL_WHENEVER SQL_SQLERROR action {
	when_error.code = $<action>3.code;
	when_error.command = $<action>3.command;
	$$ = cat_str(3, make_str("/* exec sql whenever sqlerror "), $3.str, make_str("; */\n"));
}
	| SQL_WHENEVER NOT SQL_FOUND action {
	when_nf.code = $<action>4.code;
	when_nf.command = $<action>4.command;
	$$ = cat_str(3, make_str("/* exec sql whenever not found "), $4.str, make_str("; */\n"));
}
	| SQL_WHENEVER SQL_SQLWARNING action {
	when_warn.code = $<action>3.code;
	when_warn.command = $<action>3.command;
	$$ = cat_str(3, make_str("/* exec sql whenever sql_warning "), $3.str, make_str("; */\n"));
}

action : SQL_CONTINUE {
	$<action>$.code = W_NOTHING;
	$<action>$.command = NULL;
	$<action>$.str = make_str("continue");
}
       | SQL_SQLPRINT {
	$<action>$.code = W_SQLPRINT;
	$<action>$.command = NULL;
	$<action>$.str = make_str("sqlprint");
}
       | SQL_STOP {
	$<action>$.code = W_STOP;
	$<action>$.command = NULL;
	$<action>$.str = make_str("stop");
}
       | SQL_GOTO name {
        $<action>$.code = W_GOTO;
        $<action>$.command = strdup($2);
	$<action>$.str = cat2_str(make_str("goto "), $2);
}
       | SQL_GO TO name {
        $<action>$.code = W_GOTO;
        $<action>$.command = strdup($3);
	$<action>$.str = cat2_str(make_str("goto "), $3);
}
       | DO name '(' c_args ')' {
	$<action>$.code = W_DO;
	$<action>$.command = cat_str(4, $2, make_str("("), $4, make_str(")"));
	$<action>$.str = cat2_str(make_str("do"), mm_strdup($<action>$.command));
}
       | DO SQL_BREAK {
        $<action>$.code = W_BREAK;
        $<action>$.command = NULL;
        $<action>$.str = make_str("break");
}
       | SQL_CALL name '(' c_args ')' {
	$<action>$.code = W_DO;
	$<action>$.command = cat_str(4, $2, make_str("("), $4, make_str(")"));
	$<action>$.str = cat2_str(make_str("call"), mm_strdup($<action>$.command));
}

/* some other stuff for ecpg */
/*
 * no longer used
ecpg_expr:  com_expr 
				{	$$ = $1; }
		| a_expr TYPECAST Typename
				{	$$ = cat_str(3, $1, make_str("::"), $3); }
		| '-' ecpg_expr %prec UMINUS
				{	$$ = cat2_str(make_str("-"), $2); }
		| '%' ecpg_expr
				{       $$ = cat2_str(make_str("%"), $2); }
		| '^' ecpg_expr
				{       $$ = cat2_str(make_str("^"), $2); }
		| '|' ecpg_expr
				{       $$ = cat2_str(make_str("|"), $2); }
		| ';' a_expr
				{       $$ = cat2_str(make_str(";"), $2); }
		| a_expr '%'
				{       $$ = cat2_str($1, make_str("%")); }
		| a_expr '^'
				{       $$ = cat2_str($1, make_str("^")); }
		| a_expr '|'
				{       $$ = cat2_str($1, make_str("|")); }
		| a_expr '+' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("+"), $3); }
		| a_expr '-' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("-"), $3); }
		| a_expr '*' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("*"), $3); }
		| a_expr '/' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("/"), $3); }
		| a_expr '%' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("%"), $3); }
		| a_expr '^' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("^"), $3); }
		| a_expr '|' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("|"), $3); }
		| a_expr '<' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("<"), $3); }
		| a_expr '>' ecpg_expr
				{	$$ = cat_str(3, $1, make_str(">"), $3); }
		| a_expr '=' NULL_P
                                {       $$ = cat2_str($1, make_str("= NULL")); }
		| NULL_P '=' ecpg_expr
                                {       $$ = cat2_str(make_str("= NULL"), $3); }
		| a_expr '=' ecpg_expr
				{	$$ = cat_str(3, $1, make_str("="), $3); }
		| a_expr Op ecpg_expr
				{	$$ = cat_str(3, $1, make_str("="), $3); }
		| Op ecpg_expr
				{	$$ = cat2_str($1, $2); }
		| a_expr Op
				{	$$ = cat2_str($1, $2); }
		| a_expr AND ecpg_expr
				{	$$ = cat_str(3, $1, make_str("and"), $3); }
		| a_expr OR ecpg_expr
				{	$$ = cat_str(3, $1, make_str("or"), $3); }
		| NOT ecpg_expr
				{	$$ = cat2_str(make_str("not"), $2); }
		| a_expr LIKE ecpg_expr
				{	$$ = cat_str(3, $1, make_str("like"), $3); }
		| a_expr NOT LIKE ecpg_expr
				{	$$ = cat_str(3, $1, make_str("not like"), $4); }
		| a_expr ISNULL
				{	$$ = cat2_str($1, make_str("isnull")); }
		| a_expr IS NULL_P
				{	$$ = cat2_str($1, make_str("is null")); }
		| a_expr NOTNULL
				{	$$ = cat2_str($1, make_str("notnull")); }
		| a_expr IS NOT NULL_P
				{	$$ = cat2_str($1, make_str("is not null")); }
		| a_expr IS TRUE_P
				{	$$ = cat2_str($1, make_str("is true")); }
		| a_expr IS NOT FALSE_P
				{	$$ = cat2_str($1, make_str("is not false")); }
		| a_expr IS FALSE_P
				{	$$ = cat2_str($1, make_str("is false")); }
		| a_expr IS NOT TRUE_P
				{	$$ = cat2_str($1, make_str("is not true")); }
		| a_expr BETWEEN b_expr AND b_expr
				{
					$$ = cat_str(5, $1, make_str("between"), $3, make_str("and"), $5); 
				}
		| a_expr NOT BETWEEN b_expr AND b_expr
				{
					$$ = cat_str(5, $1, make_str("not between"), $4, make_str("and"), $6); 
				}
		| a_expr IN '(' in_expr ')'
				{
					$$ = cat_str(4, $1, make_str(" in ("), $4, make_str(")")); 
				}
		| a_expr NOT IN '(' in_expr ')'
				{
					$$ = cat_str(4, $1, make_str(" not in ("), $5, make_str(")")); 
				}
		| a_expr all_Op sub_type '(' SubSelect ')'
				{
					$$ = cat_str(4, $1, $2, $3, cat_str(3, make_str("("), $5, make_str(")"))); 
				}
		| row_expr
				{       $$ = $1; }
		| civariableonly
			        { 	$$ = $1; }
		;
*/

into_list : coutputvariable | into_list ',' coutputvariable;

ecpgstart: SQL_START { reset_variables();}

c_args: /* empty */		{ $$ = EMPTY; }
	| c_list		{ $$ = $1; }

coutputvariable : cvariable indicator {
		add_variable(&argsresult, find_variable($1), ($2 == NULL) ? &no_indicator : find_variable($2)); 
}

cinputvariable : cvariable indicator {
		add_variable(&argsinsert, find_variable($1), ($2 == NULL) ? &no_indicator : find_variable($2)); 
}

civariableonly : cvariable {
		add_variable(&argsinsert, find_variable($1), &no_indicator); 
		$$ = make_str("?");
}

cvariable: CVARIABLE			{ $$ = $1; }

indicator: /* empty */			{ $$ = NULL; }
	| cvariable		 	{ check_indicator((find_variable($1))->type); $$ = $1; }
	| SQL_INDICATOR cvariable 	{ check_indicator((find_variable($2))->type); $$ = $2; }
	| SQL_INDICATOR name		{ check_indicator((find_variable($2))->type); $$ = $2; }

ident: IDENT	{ $$ = $1; }
	| CSTRING	{ $$ = make3_str(make_str("\""), $1, make_str("\"")); };

/*
 * C stuff
 */

symbol: IDENT	{ $$ = $1; }

cpp_line: CPP_LINE	{ $$ = $1; }

c_stuff: c_anything 	{ $$ = $1; }
	| c_stuff c_anything
			{
				$$ = cat2_str($1, $2);
			}
	| c_stuff '(' c_stuff ')'
			{
				$$ = cat_str(4, $1, make_str("("), $3, make_str(")"));
			}

c_list: c_term			{ $$ = $1; }
	| c_term ',' c_list	{ $$ = cat_str(3, $1, make_str(","), $3); }

c_term:  c_stuff 		{ $$ = $1; }
	| '{' c_list '}'	{ $$ = cat_str(3, make_str("{"), $2, make_str("}")); }

c_thing:	c_anything	{ $$ = $1; }
	|	'('		{ $$ = make_str("("); }
	|	')'		{ $$ = make_str(")"); }
	|	','		{ $$ = make_str(","); }
	|	';'		{ $$ = make_str(";"); }

c_anything:  IDENT 	{ $$ = $1; }
	| CSTRING	{ $$ = make3_str(make_str("\""), $1, make_str("\"")); }
	| Iconst	{ $$ = $1; }
	| Fconst	{ $$ = $1; }
	| '*'		{ $$ = make_str("*"); }
	| '+'		{ $$ = make_str("+"); }
	| '-'		{ $$ = make_str("-"); }
	| '/'		{ $$ = make_str("/"); }
	| '%'		{ $$ = make_str("%"); }
	| S_AUTO	{ $$ = make_str("auto"); }
	| S_BOOL	{ $$ = make_str("bool"); }
	| S_CHAR	{ $$ = make_str("char"); }
	| S_CONST	{ $$ = make_str("const"); }
	| S_DOUBLE	{ $$ = make_str("double"); }
	| S_ENUM	{ $$ = make_str("enum"); }
	| S_EXTERN	{ $$ = make_str("extern"); }
	| S_FLOAT	{ $$ = make_str("float"); }
        | S_INT		{ $$ = make_str("int"); }
	| S_LONG	{ $$ = make_str("long"); }
	| S_REGISTER	{ $$ = make_str("register"); }
	| S_SHORT	{ $$ = make_str("short"); }
	| S_SIGNED	{ $$ = make_str("signed"); }
	| S_STATIC	{ $$ = make_str("static"); }
        | S_STRUCT	{ $$ = make_str("struct"); }
        | S_UNION	{ $$ = make_str("union"); }
	| S_UNSIGNED	{ $$ = make_str("unsigned"); }
	| S_VARCHAR	{ $$ = make_str("varchar"); }
	| S_ANYTHING	{ $$ = make_name(); }
        | '['		{ $$ = make_str("["); }
	| ']'		{ $$ = make_str("]"); }
/*        | '('		{ $$ = make_str("("); }
	| ')'		{ $$ = make_str(")"); }*/
	| '='		{ $$ = make_str("="); }

blockstart : '{' {
    braces_open++;
    $$ = make_str("{");
}

blockend : '}' {
    remove_variables(braces_open--);
    $$ = make_str("}");
}

%%

void yyerror(char * error)
{
	mmerror(ET_ERROR, error);
}
