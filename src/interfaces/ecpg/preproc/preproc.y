/* Copyright comment */
%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "catalog/catname.h"
#include "utils/numeric.h"

#include "extern.h"

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"
#endif

#define STRUCT_DEPTH 128

/*
 * Variables containing simple states.
 */
int	struct_level = 0;
char	errortext[128];
static char	*connection = NULL;
static int      QueryIsRule = 0, ForUpdateNotAllowed = 0;
static struct this_type actual_type[STRUCT_DEPTH];
static char     *actual_storage[STRUCT_DEPTH];

/* temporarily store struct members while creating the data structure */
struct ECPGstruct_member *struct_member_list[STRUCT_DEPTH] = { NULL };

struct ECPGtype ecpg_no_indicator = {ECPGt_NO_INDICATOR, 0L, {NULL}};
struct variable no_indicator = {"no_indicator", &ecpg_no_indicator, 0, NULL};

struct ECPGtype ecpg_query = {ECPGt_char_variable, 0L, {NULL}};

/*
 * Handle the filename and line numbering.
 */
char * input_filename = NULL;

static void
output_line_number()
{
    if (input_filename)
       fprintf(yyout, "\n#line %d \"%s\"\n", yylineno, input_filename);
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
	if (mode == 1 && when_nf.code != W_NOTHING)
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
                yyerror (errortext);
        }

	if (p->type->u.element->typ  != ECPGt_struct && p->type->u.element->typ != ECPGt_union)
        {
                sprintf(errortext, "variable %s is not a pointer to a structure or a union", name);
                yyerror (errortext);
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
		yyerror (errortext);
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
	yyerror(errortext);
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
			yyerror ("indicator variable must be integer type");
			break;
	}
}

static char *
make1_str(const char *str)
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
make3_str(char *str1, char *str2, char * str3)
{    
        char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + strlen(str3) + 1);
     
        strcpy(res_str, str1);
        strcat(res_str, str2);
	strcat(res_str, str3);
	free(str1);
	free(str2);
	free(str3);
        return(res_str);
}    

static char *
cat3_str(char *str1, char *str2, char * str3)
{    
        char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + strlen(str3) + 3);
     
        strcpy(res_str, str1);
	strcat(res_str, " ");
        strcat(res_str, str2);
	strcat(res_str, " ");
	strcat(res_str, str3);
	free(str1);
	free(str2);
	free(str3);
        return(res_str);
}    

static char *
make4_str(char *str1, char *str2, char *str3, char *str4)
{    
        char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + 1);
     
        strcpy(res_str, str1);
        strcat(res_str, str2);
	strcat(res_str, str3);
	strcat(res_str, str4);
	free(str1);
	free(str2);
	free(str3);
	free(str4);
        return(res_str);
}

static char *
cat4_str(char *str1, char *str2, char *str3, char *str4)
{    
        char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + 4);
     
        strcpy(res_str, str1);
	strcat(res_str, " ");
        strcat(res_str, str2);
	strcat(res_str, " ");
	strcat(res_str, str3);
	strcat(res_str, " ");
	strcat(res_str, str4);
	free(str1);
	free(str2);
	free(str3);
	free(str4);
        return(res_str);
}

static char *
make5_str(char *str1, char *str2, char *str3, char *str4, char *str5)
{    
        char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + strlen(str5) + 1);
     
        strcpy(res_str, str1);
        strcat(res_str, str2);
	strcat(res_str, str3);
	strcat(res_str, str4);
	strcat(res_str, str5);
	free(str1);
	free(str2);
	free(str3);
	free(str4);
	free(str5);
        return(res_str);
}    

static char *
cat5_str(char *str1, char *str2, char *str3, char *str4, char *str5)
{    
        char * res_str  = (char *)mm_alloc(strlen(str1) + strlen(str2) + strlen(str3) + strlen(str4) + strlen(str5) + 5);
     
        strcpy(res_str, str1);
	strcat(res_str, " ");
        strcat(res_str, str2);
	strcat(res_str, " ");
	strcat(res_str, str3);
	strcat(res_str, " ");
	strcat(res_str, str4);
	strcat(res_str, " ");
	strcat(res_str, str5);
	free(str1);
	free(str2);
	free(str3);
	free(str4);
	free(str5);
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

	fprintf(yyout, "ECPGdo(__LINE__, %s, \"", connection ? connection : "NULL");

	/* do this char by char as we have to filter '\"' */
	for (i = 0;i < j; i++)
		if (stmt[i] != '\"')
			fputc(stmt[i], yyout);
	fputs("\", ", yyout);

	/* dump variables to C file*/
	dump_variables(argsinsert, 1);
	fputs("ECPGt_EOIT, ", yyout);
	dump_variables(argsresult, 1);
	fputs("ECPGt_EORT);", yyout);
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
		yyerror(errortext);
	}

	return(this);
}

static void
adjust_array(enum ECPGttype type_enum, int *dimension, int *length, int type_dimension, int type_index, bool pointer)
{
	if (type_index >= 0) 
	{
		if (*length >= 0)
                      	yyerror("No multi-dimensional array support");

		*length = type_index;
	}
		       
	if (type_dimension >= 0)
	{
		if (*dimension >= 0 && *length >= 0)
			yyerror("No multi-dimensional array support");

		if (*dimension >= 0)
			*length = *dimension;

		*dimension = type_dimension;
	}

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
                   yyerror("No multi-dimensional array support for structures");

                break;
           case ECPGt_varchar:
	        /* pointer has to get length 0 */
                if (pointer)
                    *length=0;

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
                   yyerror("No multi-dimensional array support for simple data types");

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
%token		SQL_AT SQL_BOOL SQL_BREAK 
%token		SQL_CALL SQL_CONNECT SQL_CONNECTION SQL_CONTINUE
%token		SQL_DEALLOCATE SQL_DISCONNECT SQL_ENUM 
%token		SQL_FOUND SQL_FREE SQL_GO SQL_GOTO
%token		SQL_IDENTIFIED SQL_IMMEDIATE SQL_INDICATOR SQL_INT SQL_LONG
%token		SQL_OPEN SQL_PREPARE SQL_RELEASE SQL_REFERENCE
%token		SQL_SECTION SQL_SEMI SQL_SHORT SQL_SIGNED SQL_SQLERROR SQL_SQLPRINT
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
                CONSTRAINT, CREATE, CROSS, CURRENT, CURRENT_DATE, CURRENT_TIME, 
                CURRENT_TIMESTAMP, CURRENT_USER, CURSOR,
                DAY_P, DECIMAL, DECLARE, DEFAULT, DELETE, DESC, DISTINCT, DOUBLE, DROP,
                ELSE, END_TRANS, EXCEPT, EXECUTE, EXISTS, EXTRACT,
                FALSE_P, FETCH, FLOAT, FOR, FOREIGN, FROM, FULL,
                GRANT, GROUP, HAVING, HOUR_P,
                IN, INNER_P, INSENSITIVE, INSERT, INTERSECT, INTERVAL, INTO, IS,
                ISOLATION, JOIN, KEY, LANGUAGE, LEADING, LEFT, LEVEL, LIKE, LOCAL,
                MATCH, MINUTE_P, MONTH_P, NAMES,
                NATIONAL, NATURAL, NCHAR, NEXT, NO, NOT, NULLIF, NULL_P, NUMERIC,
                OF, ON, ONLY, OPTION, OR, ORDER, OUTER_P,
                PARTIAL, POSITION, PRECISION, PRIMARY, PRIOR, PRIVILEGES, PROCEDURE, PUBLIC,
                READ, REFERENCES, RELATIVE, REVOKE, RIGHT, ROLLBACK,
                SCROLL, SECOND_P, SELECT, SET, SUBSTRING,
                TABLE, TEMP, THEN, TIME, TIMESTAMP, TIMEZONE_HOUR, TIMEZONE_MINUTE,
		TO, TRAILING, TRANSACTION, TRIM, TRUE_P,
                UNION, UNIQUE, UPDATE, USER, USING,
                VALUES, VARCHAR, VARYING, VIEW,
                WHEN, WHERE, WITH, WORK, YEAR_P, ZONE

/* Keywords (in SQL3 reserved words) */
%token  TRIGGER

/* Keywords (in SQL92 non-reserved words) */
%token  TYPE_P

/* Keywords for Postgres support (not in SQL92 reserved words)
 *
 * The CREATEDB and CREATEUSER tokens should go away
 * when some sort of pg_privileges relation is introduced.
 * - Todd A. Brandys 1998-01-01?
 */
%token  ABORT_TRANS, AFTER, AGGREGATE, ANALYZE, BACKWARD, BEFORE, BINARY,
		CACHE, CLUSTER, COPY, CREATEDB, CREATEUSER, CYCLE,
                DATABASE, DELIMITERS, DO, EACH, ENCODING, EXPLAIN, EXTEND,
                FORWARD, FUNCTION, HANDLER,
                INCREMENT, INDEX, INHERITS, INSTEAD, ISNULL,
                LANCOMPILER, LIMIT, LISTEN, UNLISTEN, LOAD, LOCATION, LOCK_P, MAXVALUE, MINVALUE, MOVE,
                NEW,  NOCREATEDB, NOCREATEUSER, NONE, NOTHING, NOTIFY, NOTNULL,
		OFFSET, OIDS, OPERATOR, PASSWORD, PROCEDURAL,
                RECIPE, RENAME, RESET, RETURNS, ROW, RULE,
                SERIAL, SEQUENCE, SETOF, SHOW, START, STATEMENT, STDIN, STDOUT, TRUSTED,
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
%nonassoc	Op				/* multi-character ops and user-defined operators */
%nonassoc	NOTNULL
%nonassoc	ISNULL
%nonassoc	IS
%left		'+' '-'
%left		'*' '/'
%left		'|'				/* this is the relation union op, not logical or */
/* Unary Operators */
%right		':'
%left		';'				/* end of statement or natural log */
%right		UMINUS
%left		'.'
%left		'[' ']'
%nonassoc	TYPECAST
%left		UNION INTERSECT EXCEPT

%type  <str>	Iconst Fconst Sconst TransactionStmt CreateStmt UserId
%type  <str>	CreateAsElement OptCreateAs CreateAsList CreateAsStmt
%type  <str>	OptInherit key_reference key_action
%type  <str>    key_match constraint_expr ColLabel SpecialRuleRelation
%type  <str> 	ColId default_expr ColQualifier columnDef ColQualList
%type  <str>    ColConstraint ColConstraintElem default_list NumericOnly FloatOnly
%type  <str>    OptTableElementList OptTableElement TableConstraint
%type  <str>    ConstraintElem key_actions constraint_list ColPrimaryKey
%type  <str>    res_target_list res_target_el res_target_list2
%type  <str>    res_target_el2 opt_id relation_name database_name
%type  <str>    access_method attr_name class index_name name func_name
%type  <str>    file_name recipe_name AexprConst ParamNo TypeId
%type  <str>	in_expr_nodes not_in_expr_nodes a_expr b_expr
%type  <str> 	opt_indirection expr_list extract_list extract_arg
%type  <str>	position_list position_expr substr_list substr_from
%type  <str>	trim_list in_expr substr_for not_in_expr attr attrs
%type  <str>	Typename Array Generic Numeric generic opt_float opt_numeric
%type  <str> 	opt_decimal Character character opt_varying opt_charset
%type  <str>	opt_collate Datetime datetime opt_timezone opt_interval
%type  <str>	numeric a_expr_or_null row_expr row_descriptor row_list
%type  <str>	SelectStmt SubSelect result OptTemp
%type  <str>	opt_table opt_union opt_unique sort_clause sortby_list
%type  <str>	sortby OptUseOp opt_inh_star relation_name_list name_list
%type  <str>	group_clause having_clause from_clause c_list 
%type  <str>	table_list join_outer where_clause relation_expr row_op sub_type
%type  <str>	opt_column_list insert_rest InsertStmt OptimizableStmt
%type  <str>    columnList DeleteStmt LockStmt UpdateStmt CursorStmt
%type  <str>    NotifyStmt columnElem copy_dirn c_expr UnlistenStmt
%type  <str>    copy_delimiter ListenStmt CopyStmt copy_file_name opt_binary
%type  <str>    opt_with_copy FetchStmt opt_direction fetch_how_many opt_portal_name
%type  <str>    ClosePortalStmt DestroyStmt VacuumStmt opt_verbose
%type  <str>    opt_analyze opt_va_list va_list ExplainStmt index_params
%type  <str>    index_list func_index index_elem opt_type opt_class access_method_clause
%type  <str>    index_opt_unique IndexStmt set_opt func_return def_rest
%type  <str>    func_args_list func_args opt_with ProcedureStmt def_arg
%type  <str>    def_elem def_list definition def_name def_type DefineStmt
%type  <str>    opt_instead event event_object RuleActionList,
%type  <str>	RuleActionBlock RuleActionMulti join_list
%type  <str>    RuleStmt opt_column opt_name oper_argtypes
%type  <str>    MathOp RemoveFuncStmt aggr_argtype for_update_clause
%type  <str>    RemoveAggrStmt remove_type RemoveStmt ExtendStmt RecipeStmt
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
%type  <str>    DestroydbStmt ClusterStmt grantee RevokeStmt encoding
%type  <str>	GrantStmt privileges operation_commalist operation
%type  <str>	cursor_clause opt_cursor opt_readonly opt_of opt_lmode
%type  <str>	case_expr when_clause_list case_default case_arg when_clause
%type  <str>    select_clause opt_select_limit select_limit_value,
%type  <str>    select_offset_value table_list using_expr join_expr
%type  <str>	using_list from_expr table_expr join_clause join_type
%type  <str>	join_qual update_list join_clause join_clause_with_union

%type  <str>	ECPGWhenever ECPGConnect connection_target ECPGOpen opt_using
%type  <str>	indicator ECPGExecute ecpg_expr dotext ECPGPrepare
%type  <str>    storage_clause opt_initializer vartext c_anything blockstart
%type  <str>    blockend variable_list variable var_anything do_anything
%type  <str>	opt_pointer cvariable ECPGDisconnect dis_name
%type  <str>	stmt symbol opt_symbol ECPGRelease execstring server_name
%type  <str>	connection_object opt_server opt_port c_thing opt_reference
%type  <str>    user_name opt_user char_variable ora_user ident
%type  <str>    db_prefix server opt_options opt_connection_name
%type  <str>	ECPGSetConnection c_line cpp_line s_enum ECPGTypedef
%type  <str>	enum_type civariableonly ECPGCursorStmt ECPGDeallocate
%type  <str>	ECPGFree ECPGDeclare ECPGVar sql_variable_declarations
%type  <str>	sql_declaration sql_variable_list sql_variable opt_at
%type  <str>    struct_type s_struct declaration variable_declarations
%type  <str>    s_struct s_union union_type

%type  <type_enum> simple_type varchar_type

%type  <type>	type ctype

%type  <action> action

%type  <index>	opt_array_bounds nest_array_bounds opt_type_array_bounds
%type  <index>  nest_type_array_bounds

%%
prog: statements;

statements: /* empty */
	| statements statement

statement: ecpgstart opt_at stmt SQL_SEMI { connection = NULL; }
	| ecpgstart stmt SQL_SEMI
	| ECPGDeclaration
	| c_thing 			{ fprintf(yyout, "%s", $1); free($1); }
	| cpp_line			{ fprintf(yyout, "%s", $1); free($1); }
	| blockstart			{ fputs($1, yyout); free($1); }
	| blockend			{ fputs($1, yyout); free($1); }

opt_at:	SQL_AT connection_target	{ connection = $2; }

stmt:  AddAttrStmt			{ output_statement($1, 0); }
		| AlterUserStmt		{ output_statement($1, 0); }
		| ClosePortalStmt	{ output_statement($1, 0); }
		| CopyStmt		{ output_statement($1, 0); }
		| CreateStmt		{ output_statement($1, 0); }
		| CreateAsStmt		{ output_statement($1, 0); }
		| CreateSeqStmt		{ output_statement($1, 0); }
		| CreatePLangStmt	{ output_statement($1, 0); }
		| CreateTrigStmt	{ output_statement($1, 0); }
		| CreateUserStmt	{ output_statement($1, 0); }
  		| ClusterStmt		{ output_statement($1, 0); }
		| DefineStmt 		{ output_statement($1, 0); }
		| DestroyStmt		{ output_statement($1, 0); }
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
 		| RecipeStmt		{ output_statement($1, 0); }
		| RemoveAggrStmt	{ output_statement($1, 0); }
		| RemoveOperStmt	{ output_statement($1, 0); }
		| RemoveFuncStmt	{ output_statement($1, 0); }
		| RemoveStmt		{ output_statement($1, 0); }
		| RenameStmt		{ output_statement($1, 0); }
		| RevokeStmt		{ output_statement($1, 0); }
                | OptimizableStmt	{
						if (strncmp($1, "/* " , sizeof("/* ")-1) == 0)
						{
							fputs($1, yyout);
							free($1);
						}
						else
							output_statement($1, 1);
					}
		| RuleStmt		{ output_statement($1, 0); }
		| TransactionStmt	{
						fprintf(yyout, "ECPGtrans(__LINE__, %s, \"%s\");", connection ? connection : "NULL", $1);
						whenever_action(0);
						free($1);
					}
		| ViewStmt		{ output_statement($1, 0); }
		| LoadStmt		{ output_statement($1, 0); }
		| CreatedbStmt		{ output_statement($1, 0); }
		| DestroydbStmt		{ output_statement($1, 0); }
		| VacuumStmt		{ output_statement($1, 0); }
		| VariableSetStmt	{ output_statement($1, 0); }
		| VariableShowStmt	{ output_statement($1, 0); }
		| VariableResetStmt	{ output_statement($1, 0); }
		| ECPGConnect		{
						if (connection)
							yyerror("no at option for connect statement.\n");

						fprintf(yyout, "no_auto_trans = %d;\n", no_auto_trans);
						fprintf(yyout, "ECPGconnect(__LINE__, %s);", $1);
						whenever_action(0);
						free($1);
					} 
		| ECPGCursorStmt	{
						fputs($1, yyout);
                                                free($1); 
					}
		| ECPGDeallocate	{
						if (connection)
							yyerror("no at option for connect statement.\n");

						fputs($1, yyout);
						whenever_action(0);
						free($1);
					}
		| ECPGDeclare		{
						fputs($1, yyout);
						free($1);
					}
		| ECPGDisconnect	{
						if (connection)
							yyerror("no at option for disconnect statement.\n");

						fprintf(yyout, "ECPGdisconnect(__LINE__, \"%s\");", $1); 
						whenever_action(0);
						free($1);
					} 
		| ECPGExecute		{
						output_statement($1, 0);
					}
		| ECPGFree		{
						fprintf(yyout, "ECPGdeallocate(__LINE__, %s, \"%s\");", connection ? connection : "NULL", $1); 
						whenever_action(0);
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
							yyerror(errortext);
						}
                  
						fprintf(yyout, "ECPGdo(__LINE__, %s, \"%s\",", ptr->connection ? ptr->connection : "NULL", ptr->command);
						/* dump variables to C file*/
						dump_variables(ptr->argsinsert, 0);
						dump_variables(argsinsert, 0);
						fputs("ECPGt_EOIT, ", yyout);
						dump_variables(ptr->argsresult, 0);
						fputs("ECPGt_EORT);", yyout);
						whenever_action(0);
						free($1);
					}
		| ECPGPrepare		{
						if (connection)
							yyerror("no at option for set connection statement.\n");

						fprintf(yyout, "ECPGprepare(__LINE__, %s);", $1); 
						whenever_action(0);
						free($1);
					}
		| ECPGRelease		{ /* output already done */ }
		| ECPGSetConnection     {
						if (connection)
							yyerror("no at option for set connection statement.\n");

						fprintf(yyout, "ECPGsetconn(__LINE__, %s);", $1);
						whenever_action(0);
                                       		free($1);
					}
		| ECPGTypedef		{
						if (connection)
							yyerror("no at option for typedef statement.\n");

						fputs($1, yyout);
                                                free($1);
					}
		| ECPGVar		{
						if (connection)
							yyerror("no at option for var statement.\n");

						fputs($1, yyout);
                                                free($1);
					}
		| ECPGWhenever		{
						if (connection)
							yyerror("no at option for whenever statement.\n");

						fputs($1, yyout);
						output_line_number();
						free($1);
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

CreateUserStmt:  CREATE USER UserId user_passwd_clause user_createdb_clause
			user_createuser_clause user_group_clause user_valid_clause
				{
					$$ = cat3_str(cat5_str(make1_str("create user"), $3, $4, $5, $6), $7, $8);
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
					$$ = cat3_str(cat5_str(make1_str("alter user"), $3, $4, $5, $6), $7, $8);
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
					$$ = cat2_str(make1_str("drop user"), $3);
				}
		;

user_passwd_clause:  WITH PASSWORD UserId	{ $$ = cat2_str(make1_str("with password") , $3); }
			| /*EMPTY*/		{ $$ = make1_str(""); }
		;

user_createdb_clause:  CREATEDB
				{
					$$ = make1_str("createdb");
				}
			| NOCREATEDB
				{
					$$ = make1_str("nocreatedb");
				}
			| /*EMPTY*/		{ $$ = make1_str(""); }
		;

user_createuser_clause:  CREATEUSER
				{
					$$ = make1_str("createuser");
				}
			| NOCREATEUSER
				{
					$$ = make1_str("nocreateuser");
				}
			| /*EMPTY*/		{ $$ = NULL; }
		;

user_group_list:  user_group_list ',' UserId
				{
					$$ = cat3_str($1, make1_str(","), $3);
				}
			| UserId
				{
					$$ = $1;
				}
		;

user_group_clause:  IN GROUP user_group_list	{ $$ = cat2_str(make1_str("in group"), $3); }
			| /*EMPTY*/		{ $$ = make1_str(""); }
		;

user_valid_clause:  VALID UNTIL Sconst			{ $$ = cat2_str(make1_str("valid until"), $3);; }
			| /*EMPTY*/			{ $$ = make1_str(""); }
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
					$$ = cat4_str(make1_str("set"), $2, make1_str("to"), $4);
				}
		| SET ColId '=' var_value
				{
					$$ = cat4_str(make1_str("set"), $2, make1_str("="), $4);
				}
		| SET TIME ZONE zone_value
				{
					$$ = cat2_str(make1_str("set time zone"), $4);
				}
		| SET TRANSACTION ISOLATION LEVEL READ ColId
				{
					if (strcasecmp($6, "COMMITTED"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $6);
						yyerror(errortext);
					}

					$$ = cat2_str(make1_str("set transaction isolation level read"), $6);
				}
		| SET TRANSACTION ISOLATION LEVEL ColId
				{
					if (strcasecmp($5, "SERIALIZABLE"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $5);
                                                yyerror(errortext);
					}

					$$ = cat2_str(make1_str("set transaction isolation level read"), $5);
				}
		| SET NAMES encoding
                                {
#ifdef MB
					$$ = cat2_str(make1_str("set names"), $3);
#else
                                        yyerror("SET NAMES is not supported");
#endif
                                }
                ;

var_value:  Sconst			{ $$ = $1; }
		| DEFAULT			{ $$ = make1_str("default"); }
		;

zone_value:  Sconst			{ $$ = $1; }
		| DEFAULT			{ $$ = make1_str("default"); }
		| LOCAL				{ $$ = make1_str("local"); }
		;

VariableShowStmt:  SHOW ColId
				{
					$$ = cat2_str(make1_str("show"), $2);
				}
		| SHOW TIME ZONE
				{
					$$ = make1_str("show time zone");
				}
		| SHOW TRANSACTION ISOLATION LEVEL
				{
					$$ = make1_str("show transaction isolation level");
				}
		;

VariableResetStmt:	RESET ColId
				{
					$$ = cat2_str(make1_str("reset"), $2);
				}
		| RESET TIME ZONE
				{
					$$ = make1_str("reset time zone");
				}
		| RESET TRANSACTION ISOLATION LEVEL
				{
					$$ = make1_str("reset transaction isolation level");
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
					$$ = cat4_str(make1_str("alter table"), $3, $4, $5);
				}
		;

alter_clause:  ADD opt_column columnDef
				{
					$$ = cat3_str(make1_str("add"), $2, $3);
				}
			| ADD '(' OptTableElementList ')'
				{
					$$ = make3_str(make1_str("add("), $3, make1_str(")"));
				}
			| DROP opt_column ColId
				{	yyerror("ALTER TABLE/DROP COLUMN not yet implemented"); }
			| ALTER opt_column ColId SET DEFAULT default_expr
				{	yyerror("ALTER TABLE/ALTER COLUMN/SET DEFAULT not yet implemented"); }
			| ALTER opt_column ColId DROP DEFAULT
				{	yyerror("ALTER TABLE/ALTER COLUMN/DROP DEFAULT not yet implemented"); }
			| ADD ConstraintElem
				{	yyerror("ALTER TABLE/ADD CONSTRAINT not yet implemented"); }
		;

/*****************************************************************************
 *
 *		QUERY :
 *				close <optname>
 *
 *****************************************************************************/

ClosePortalStmt:  CLOSE opt_id
				{
					$$ = cat2_str(make1_str("close"), $2);
				}
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
					$$ = cat3_str(cat5_str(make1_str("copy"), $2, $3, $4, $5), $6, $7);
				}
		;

copy_dirn:	TO
				{ $$ = make1_str("to"); }
		| FROM
				{ $$ = make1_str("from"); }
		;

/*
 * copy_file_name NULL indicates stdio is used. Whether stdin or stdout is
 * used depends on the direction. (It really doesn't make sense to copy from
 * stdout. We silently correct the "typo".		 - AY 9/94
 */
copy_file_name:  Sconst					{ $$ = $1; }
		| STDIN					{ $$ = make1_str("stdin"); }
		| STDOUT				{ $$ = make1_str("stdout"); }
		;

opt_binary:  BINARY					{ $$ = make1_str("binary"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

opt_with_copy:	WITH OIDS				{ $$ = make1_str("with oids"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

/*
 * the default copy delimiter is tab but the user can configure it
 */
copy_delimiter:  USING DELIMITERS Sconst		{ $$ = cat2_str(make1_str("using delimiters"), $3); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
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
					$$ = cat3_str(cat4_str(make1_str("create"), $2, make1_str("table"), $4), make3_str(make1_str("("), $6, make1_str(")")), $8);
				}
		;

OptTemp:	  TEMP		{ $$ = make1_str("temp"); }
		| /* EMPTY */	{ $$ = make1_str(""); }
		;

OptTableElementList:  OptTableElementList ',' OptTableElement
				{
					$$ = cat3_str($1, make1_str(","), $3);
				}
			| OptTableElement
				{
					$$ = $1;
				}
			| /*EMPTY*/	{ $$ = make1_str(""); }
		;

OptTableElement:  columnDef		{ $$ = $1; }
			| TableConstraint	{ $$ = $1; }
		;

columnDef:  ColId Typename ColQualifier
				{
					$$ = cat3_str($1, $2, $3);
				}
	| ColId SERIAL ColPrimaryKey
		{
			$$ = make3_str($1, make1_str(" serial "), $3);
		}
		;

ColQualifier:  ColQualList	{ $$ = $1; }
			| /*EMPTY*/	{ $$ = make1_str(""); }
		;

ColQualList:  ColQualList ColConstraint	{ $$ = cat2_str($1,$2); }
			| ColConstraint		{ $$ = $1; }
		;

ColPrimaryKey:  PRIMARY KEY
                {
			$$ = make1_str("primary key");
                }
              | /*EMPTY*/
		{
			$$ = make1_str("");
		}
                ;

ColConstraint:
		CONSTRAINT name ColConstraintElem
				{
					$$ = cat3_str(make1_str("constraint"), $2, $3);
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
					$$ = make3_str(make1_str("check("), $3, make1_str(")"));
				}
			| DEFAULT NULL_P
				{
					$$ = make1_str("default null");
				}
			| DEFAULT default_expr
				{
					$$ = cat2_str(make1_str("default"), $2);
				}
			| NOT NULL_P
				{
					$$ = make1_str("not null");
				}
			| UNIQUE
				{
					$$ = make1_str("unique");
				}
			| PRIMARY KEY
				{
					$$ = make1_str("primary key");
				}
			| REFERENCES ColId opt_column_list key_match key_actions
				{
					fprintf(stderr, "CREATE TABLE/FOREIGN KEY clause ignored; not yet implemented");
					$$ = make1_str("");
				}
		;

default_list:  default_list ',' default_expr
				{
					$$ = cat3_str($1, make1_str(","), $3);
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
				{	$$ = make1_str("null"); }
 * - thomas 1998-09-13
 */

default_expr:  AexprConst
				{	$$ = $1; }
			| '-' default_expr %prec UMINUS
				{	$$ = cat2_str(make1_str("-"), $2); }
			| default_expr '+' default_expr
				{	$$ = cat3_str($1, make1_str("+"), $3); }
			| default_expr '-' default_expr
				{	$$ = cat3_str($1, make1_str("-"), $3); }
			| default_expr '/' default_expr
				{	$$ = cat3_str($1, make1_str("/"), $3); }
			| default_expr '*' default_expr
				{	$$ = cat3_str($1, make1_str("*"), $3); }
			| default_expr '=' default_expr
				{	yyerror("boolean expressions not supported in DEFAULT"); }
			| default_expr '<' default_expr
				{	yyerror("boolean expressions not supported in DEFAULT"); }
			| default_expr '>' default_expr
				{	yyerror("boolean expressions not supported in DEFAULT"); }
/* not possible in embedded sql 
			| ':' default_expr
				{	$$ = cat2_str(make1_str(":"), $2); }
*/
			| ';' default_expr
				{	$$ = cat2_str(make1_str(";"), $2); }
			| '|' default_expr
				{	$$ = cat2_str(make1_str("|"), $2); }
			| default_expr TYPECAST Typename
				{	$$ = cat3_str($1, make1_str("::"), $3); }
			| CAST '(' default_expr AS Typename ')'
				{
					$$ = cat3_str(make2_str(make1_str("cast("), $3) , make1_str("as"), make2_str($5, make1_str(")")));
				}
			| '(' default_expr ')'
				{	$$ = make3_str(make1_str("("), $2, make1_str(")")); }
			| func_name '(' ')'
				{	$$ = cat2_str($1, make1_str("()")); }
			| func_name '(' default_list ')'
				{	$$ = cat2_str($1, make3_str(make1_str("("), $3, make1_str(")"))); }
			| default_expr Op default_expr
				{
					if (!strcmp("<=", $2) || !strcmp(">=", $2))
						yyerror("boolean expressions not supported in DEFAULT");
					$$ = cat3_str($1, $2, $3);
				}
			| Op default_expr
				{	$$ = cat2_str($1, $2); }
			| default_expr Op
				{	$$ = cat2_str($1, $2); }
			/* XXX - thomas 1997-10-07 v6.2 function-specific code to be changed */
			| CURRENT_DATE
				{	$$ = make1_str("current_date"); }
			| CURRENT_TIME
				{	$$ = make1_str("current_time"); }
			| CURRENT_TIME '(' Iconst ')'
				{
					if ($3 != 0)
						fprintf(stderr, "CURRENT_TIME(%s) precision not implemented; zero used instead",$3);
					$$ = "current_time";
				}
			| CURRENT_TIMESTAMP
				{	$$ = make1_str("current_timestamp"); }
			| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					if ($3 != 0)
						fprintf(stderr, "CURRENT_TIMESTAMP(%s) precision not implemented; zero used instead",$3);
					$$ = "current_timestamp";
				}
			| CURRENT_USER
				{	$$ = make1_str("current_user"); }
			| USER
				{       $$ = make1_str("user"); }
		;

/* ConstraintElem specifies constraint syntax which is not embedded into
 *  a column definition. ColConstraintElem specifies the embedded form.
 * - thomas 1997-12-03
 */
TableConstraint:  CONSTRAINT name ConstraintElem
				{
						$$ = cat3_str(make1_str("constraint"), $2, $3);
				}
		| ConstraintElem
				{ $$ = $1; }
		;

ConstraintElem:  CHECK '(' constraint_expr ')'
				{
					$$ = make3_str(make1_str("check("), $3, make1_str(")"));
				}
		| UNIQUE '(' columnList ')'
				{
					$$ = make3_str(make1_str("unique("), $3, make1_str(")"));
				}
		| PRIMARY KEY '(' columnList ')'
				{
					$$ = make3_str(make1_str("primary key("), $4, make1_str(")"));
				}
		| FOREIGN KEY '(' columnList ')' REFERENCES ColId opt_column_list key_match key_actions
				{
					fprintf(stderr, "CREATE TABLE/FOREIGN KEY clause ignored; not yet implemented");
					$$ = "";
				}
		;

constraint_list:  constraint_list ',' constraint_expr
				{
					$$ = cat3_str($1, make1_str(","), $3);
				}
			| constraint_expr
				{
					$$ = $1;
				}
		;

constraint_expr:  AexprConst
				{	$$ = $1; }
			| NULL_P
				{	$$ = make1_str("null"); }
			| ColId
				{
					$$ = $1;
				}
			| '-' constraint_expr %prec UMINUS
				{	$$ = cat2_str(make1_str("-"), $2); }
			| constraint_expr '+' constraint_expr
				{	$$ = cat3_str($1, make1_str("+"), $3); }
			| constraint_expr '-' constraint_expr
				{	$$ = cat3_str($1, make1_str("-"), $3); }
			| constraint_expr '/' constraint_expr
				{	$$ = cat3_str($1, make1_str("/"), $3); }
			| constraint_expr '*' constraint_expr
				{	$$ = cat3_str($1, make1_str("*"), $3); }
			| constraint_expr '=' constraint_expr
				{	$$ = cat3_str($1, make1_str("="), $3); }
			| constraint_expr '<' constraint_expr
				{	$$ = cat3_str($1, make1_str("<"), $3); }
			| constraint_expr '>' constraint_expr
				{	$$ = cat3_str($1, make1_str(">"), $3); }
/* this one doesn't work with embedded sql anyway
			| ':' constraint_expr
				{	$$ = cat2_str(make1_str(":"), $2); }
*/
			| ';' constraint_expr
				{	$$ = cat2_str(make1_str(";"), $2); }
			| '|' constraint_expr
				{	$$ = cat2_str(make1_str("|"), $2); }
			| constraint_expr TYPECAST Typename
				{
					$$ = cat3_str($1, make1_str("::"), $3);
				}
			| CAST '(' constraint_expr AS Typename ')'
				{
					$$ = cat3_str(make2_str(make1_str("cast("), $3), make1_str("as"), make2_str($5, make1_str(")"))); 
				}
			| '(' constraint_expr ')'
				{	$$ = make3_str(make1_str("("), $2, make1_str(")")); }
			| func_name '(' ')'
				{
				{	$$ = cat2_str($1, make1_str("()")); }
				}
			| func_name '(' constraint_list ')'
				{
					$$ = cat2_str($1, make3_str(make1_str("("), $3, make1_str(")")));
				}
			| constraint_expr Op constraint_expr
				{	$$ = cat3_str($1, $2, $3); }
			| constraint_expr LIKE constraint_expr
				{	$$ = cat3_str($1, make1_str("like"), $3); }
			| constraint_expr NOT LIKE constraint_expr
				{	$$ = cat3_str($1, make1_str("not like"), $4); }
			| constraint_expr AND constraint_expr
				{	$$ = cat3_str($1, make1_str("and"), $3); }
			| constraint_expr OR constraint_expr
				{	$$ = cat3_str($1, make1_str("or"), $3); }
			| NOT constraint_expr
				{	$$ = cat2_str(make1_str("not"), $2); }
			| Op constraint_expr
				{	$$ = cat2_str($1, $2); }
			| constraint_expr Op
				{	$$ = cat2_str($1, $2); }
			| constraint_expr ISNULL
				{	$$ = cat2_str($1, make1_str("isnull")); }
			| constraint_expr IS NULL_P
				{	$$ = cat2_str($1, make1_str("is null")); }
			| constraint_expr NOTNULL
				{	$$ = cat2_str($1, make1_str("notnull")); }
			| constraint_expr IS NOT NULL_P
				{	$$ = cat2_str($1, make1_str("is not null")); }
			| constraint_expr IS TRUE_P
				{	$$ = cat2_str($1, make1_str("is true")); }
			| constraint_expr IS FALSE_P
				{	$$ = cat2_str($1, make1_str("is false")); }
			| constraint_expr IS NOT TRUE_P
				{	$$ = cat2_str($1, make1_str("is not true")); }
			| constraint_expr IS NOT FALSE_P
				{	$$ = cat2_str($1, make1_str("is not false")); }
			| constraint_expr IN '(' c_list ')'
				{	$$ = cat4_str($1, make1_str("in ("), $4, make1_str(")")); }
			| constraint_expr NOT IN '(' c_list ')'
				{	$$ = cat4_str($1, make1_str("not in ("), $5, make1_str(")")); }
			| constraint_expr BETWEEN c_expr AND c_expr
				{	$$ = cat5_str($1, make1_str("between"), $3, make1_str("and"), $5); }
			| constraint_expr NOT BETWEEN c_expr AND c_expr
				{	$$ = cat5_str($1, make1_str("not between"), $4, make1_str("and"), $6); }
		;
c_list:  c_list ',' c_expr
	{
		$$ = make3_str($1, make1_str(", "), $3);
	}
	| c_expr
	{
		$$ = $1;
	}

c_expr:  AexprConst
	{
		$$ = $1;
	}

key_match:  MATCH FULL					{ $$ = make1_str("match full"); }
		| MATCH PARTIAL					{ $$ = make1_str("match partial"); }
		| /*EMPTY*/					{ $$ = make1_str(""); }
		;

key_actions:  key_action key_action		{ $$ = cat2_str($1, $2); }
		| key_action					{ $$ = $1; }
		| /*EMPTY*/					{ $$ = make1_str(""); }
		;

key_action:  ON DELETE key_reference	{ $$ = cat2_str(make1_str("on delete"), $3); }
		| ON UPDATE key_reference		{ $$ = cat2_str(make1_str("on update"), $3); }
		;

key_reference:  NO ACTION	{ $$ = make1_str("no action"); }
		| CASCADE	{ $$ = make1_str("cascade"); }
		| SET DEFAULT	{ $$ = make1_str("set default"); }
		| SET NULL_P	{ $$ = make1_str("set null"); }
		;

OptInherit:  INHERITS '(' relation_name_list ')' { $$ = make3_str(make1_str("inherits ("), $3, make1_str(")")); }
		| /*EMPTY*/ { $$ = make1_str(""); }
		;

CreateAsStmt:  CREATE OptTemp TABLE relation_name OptCreateAs AS SubSelect
		{
			$$ = cat5_str(cat3_str(make1_str("create"), $2, make1_str("table")), $4, $5, make1_str("as"), $7); 
		}
		;

OptCreateAs:  '(' CreateAsList ')' { $$ = make3_str(make1_str("("), $2, make1_str(")")); }
			| /*EMPTY*/ { $$ = make1_str(""); }	
		;

CreateAsList:  CreateAsList ',' CreateAsElement	{ $$ = cat3_str($1, make1_str(","), $3); }
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
					$$ = cat3_str(make1_str("create sequence"), $3, $4);
				}
		;

OptSeqList:  OptSeqList OptSeqElem
				{ $$ = cat2_str($1, $2); }
			|	{ $$ = make1_str(""); }
		;

OptSeqElem:  CACHE IntegerOnly
				{
					$$ = cat2_str(make1_str("cache"), $2);
				}
			| CYCLE
				{
					$$ = make1_str("cycle");
				}
			| INCREMENT IntegerOnly
				{
					$$ = cat2_str(make1_str("increment"), $2);
				}
			| MAXVALUE IntegerOnly
				{
					$$ = cat2_str(make1_str("maxvalue"), $2);
				}
			| MINVALUE IntegerOnly
				{
					$$ = cat2_str(make1_str("minvalue"), $2);
				}
			| START IntegerOnly
				{
					$$ = cat2_str(make1_str("start"), $2);
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
                                       $$ = cat2_str(make1_str("-"), $2);
                               }
               ;


IntegerOnly:  Iconst
				{
					$$ = $1;
				}
			| '-' Iconst
				{
					$$ = cat2_str(make1_str("-"), $2);
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
				$$ = cat4_str(cat5_str(make1_str("create"), $2, make1_str("precedural language"), $5, make1_str("handler")), $7, make1_str("langcompiler"), $9);
			}
		;

PLangTrusted:		TRUSTED { $$ = make1_str("trusted"); }
			|	{ $$ = make1_str(""); }

DropPLangStmt:  DROP PROCEDURAL LANGUAGE Sconst
			{
				$$ = cat2_str(make1_str("drop procedural language"), $4);
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
					$$ = cat2_str(cat5_str(cat5_str(make1_str("create trigger"), $3, $4, $5, make1_str("on")), $7, $8, make1_str("execute procedure"), $11), make3_str(make1_str("("), $13, make1_str(")")));
				}
		;

TriggerActionTime:  BEFORE				{ $$ = make1_str("before"); }
			| AFTER				{ $$ = make1_str("after"); }
		;

TriggerEvents:	TriggerOneEvent
				{
					$$ = $1;
				}
			| TriggerOneEvent OR TriggerOneEvent
				{
					$$ = cat3_str($1, make1_str("or"), $3);
				}
			| TriggerOneEvent OR TriggerOneEvent OR TriggerOneEvent
				{
					$$ = cat5_str($1, make1_str("or"), $3, make1_str("or"), $5);
				}
		;

TriggerOneEvent:  INSERT				{ $$ = make1_str("insert"); }
			| DELETE			{ $$ = make1_str("delete"); }
			| UPDATE			{ $$ = make1_str("update"); }
		;

TriggerForSpec:  FOR TriggerForOpt TriggerForType
				{
					$$ = cat3_str(make1_str("for"), $2, $3);
				}
		;

TriggerForOpt:  EACH					{ $$ = make1_str("each"); }
			| /*EMPTY*/			{ $$ = make1_str(""); }
		;

TriggerForType:  ROW					{ $$ = make1_str("row"); }
			| STATEMENT			{ $$ = make1_str("statement"); }
		;

TriggerFuncArgs:  TriggerFuncArg
				{ $$ = $1; }
			| TriggerFuncArgs ',' TriggerFuncArg
				{ $$ = cat3_str($1, make1_str(","), $3); }
			| /*EMPTY*/
				{ $$ = make1_str(""); }
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

DropTrigStmt:  DROP TRIGGER name ON relation_name
				{
					$$ = cat4_str(make1_str("drop trigger"), $3, make1_str("on"), $5);
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
					$$ = cat3_str(make1_str("create"), $2, $3);
				}
		;

def_rest:  def_name definition
				{
					$$ = cat2_str($1, $2);
				}
		;

def_type:  OPERATOR		{ $$ = make1_str("operator"); }
		| TYPE_P	{ $$ = make1_str("type"); }
		| AGGREGATE	{ $$ = make1_str("aggregate"); }
		;

def_name:  PROCEDURE		{ $$ = make1_str("procedure"); }
		| JOIN		{ $$ = make1_str("join"); }
		| ColId		{ $$ = $1; }
		| MathOp	{ $$ = $1; }
		| Op		{ $$ = $1; }
		;

definition:  '(' def_list ')'				{ $$ = make3_str(make1_str("("), $2, make1_str(")")); }
		;

def_list:  def_elem					{ $$ = $1; }
		| def_list ',' def_elem			{ $$ = cat3_str($1, make1_str(","), $3); }
		;

def_elem:  def_name '=' def_arg	{
					$$ = cat3_str($1, make1_str("="), $3);
				}
		| def_name
				{
					$$ = $1;
				}
		| DEFAULT '=' def_arg
				{
					$$ = cat2_str(make1_str("default ="), $3);
				}
		;

def_arg:  ColId			{  $$ = $1; }
		| all_Op	{  $$ = $1; }
		| NumericOnly	{  $$ = $1; }
		| Sconst	{  $$ = $1; }
		| SETOF ColId
				{
					$$ = cat2_str(make1_str("setof"), $2);
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
					$$ = cat2_str(make1_str("drop table"), $3);
				}
		| DROP SEQUENCE relation_name_list
				{
					$$ = cat2_str(make1_str("drop sequence"), $3);
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
					if (strncmp($2, "relative", strlen("relative")) == 0 && atol($3) == 0L)
						yyerror("FETCH/RELATIVE at current position is not supported");

					$$ = cat4_str(make1_str("fetch"), $2, $3, $4);
				}
		|	MOVE opt_direction fetch_how_many opt_portal_name
				{
					$$ = cat4_str(make1_str("fetch"), $2, $3, $4);
				}
		;

opt_direction:	FORWARD		{ $$ = make1_str("forward"); }
		| BACKWARD	{ $$ = make1_str("backward"); }
		| RELATIVE      { $$ = make1_str("relative"); }
                | ABSOLUTE
 				{
					fprintf(stderr, "FETCH/ABSOLUTE not supported, using RELATIVE");
					$$ = make1_str("absolute");
				}
		| /*EMPTY*/	{ $$ = make1_str(""); /* default */ }
		;

fetch_how_many:   Iconst        { $$ = $1; }
		| '-' Iconst    { $$ = make2_str(make1_str("-"), $2); }
		| ALL		{ $$ = make1_str("all"); }
		| NEXT		{ $$ = make1_str("next"); }
		| PRIOR		{ $$ = make1_str("prior"); }
		| /*EMPTY*/	{ $$ = make1_str(""); /*default*/ }
		;

opt_portal_name:  IN name		{ $$ = cat2_str(make1_str("in"), $2); }
		| FROM name		{ $$ = cat2_str(make1_str("from"), $2); }
/*		| name			{ $$ = cat2_str(make1_str("in"), $1); */
		| /*EMPTY*/		{ $$ = make1_str(""); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				GRANT [privileges] ON [relation_name_list] TO [GROUP] grantee
 *
 *****************************************************************************/

GrantStmt:  GRANT privileges ON relation_name_list TO grantee opt_with_grant
				{
					$$ = cat2_str(cat5_str(make1_str("grant"), $2, make1_str("on"), $4, make1_str("to")), $6);
				}
		;

privileges:  ALL PRIVILEGES
				{
				 $$ = make1_str("all privileges");
				}
		| ALL
				{
				 $$ = make1_str("all");
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
						$$ = cat3_str($1, make1_str(","), $3);
				}
		;

operation:  SELECT
				{
						$$ = make1_str("select");
				}
		| INSERT
				{
						$$ = make1_str("insert");
				}
		| UPDATE
				{
						$$ = make1_str("update");
				}
		| DELETE
				{
						$$ = make1_str("delete");
				}
		| RULE
				{
						$$ = make1_str("rule");
				}
		;

grantee:  PUBLIC
				{
						$$ = make1_str("public");
				}
		| GROUP ColId
				{
						$$ = cat2_str(make1_str("group"), $2);
				}
		| ColId
				{
						$$ = $1;
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
					$$ = cat2_str(cat5_str(make1_str("revoke"), $2, make1_str("on"), $4, make1_str("from")), $6);
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
					$$ = cat5_str(cat5_str(make1_str("create"), $2, make1_str("index"), $4, make1_str("on")), $6, $7, make3_str(make1_str("("), $9, make1_str(")")), $11);
				}
		;

index_opt_unique:  UNIQUE	{ $$ = make1_str("unique"); }
		| /*EMPTY*/	{ $$ = make1_str(""); }
		;

access_method_clause:  USING access_method	{ $$ = cat2_str(make1_str("using"), $2); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
		;

index_params:  index_list			{ $$ = $1; }
		| func_index			{ $$ = $1; }
		;

index_list:  index_list ',' index_elem		{ $$ = cat3_str($1, make1_str(","), $3); }
		| index_elem			{ $$ = $1; }
		;

func_index:  func_name '(' name_list ')' opt_type opt_class
				{
					$$ = cat4_str($1, make3_str(make1_str("("), $3, ")"), $5, $6);
				}
		  ;

index_elem:  attr_name opt_type opt_class
				{
					$$ = cat3_str($1, $2, $3);
				}
		;

opt_type:  ':' Typename		{ $$ = cat2_str(make1_str(":"), $2); }
		| FOR Typename	{ $$ = cat2_str(make1_str("for"), $2); }
		| /*EMPTY*/	{ $$ = make1_str(""); }
		;

/* opt_class "WITH class" conflicts with preceeding opt_type
 *  for Typename of "TIMESTAMP WITH TIME ZONE"
 * So, remove "WITH class" from the syntax. OK??
 * - thomas 1997-10-12
 *		| WITH class							{ $$ = $2; }
 */
opt_class:  class				{ $$ = $1; }
		| USING class			{ $$ = cat2_str(make1_str("using"), $2); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
		;

/*****************************************************************************
 *
 *		QUERY:
 *				extend index <indexname> [where <qual>]
 *
 *****************************************************************************/

ExtendStmt:  EXTEND INDEX index_name where_clause
				{
					$$ = cat3_str(make1_str("extend index"), $3, $4);
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				execute recipe <recipeName>
 *
 *****************************************************************************/

RecipeStmt:  EXECUTE RECIPE recipe_name
				{
					$$ = cat2_str(make1_str("execute recipe"), $3);
				}
		;

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
			 RETURNS func_return opt_with AS Sconst LANGUAGE Sconst
				{
					$$ = cat2_str(cat5_str(cat5_str(make1_str("create function"), $3, $4, make1_str("returns"), $6), $7, make1_str("as"), $9, make1_str("language")), $11);
				}

opt_with:  WITH definition			{ $$ = cat2_str(make1_str("with"), $2); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
		;

func_args:  '(' func_args_list ')'		{ $$ = make3_str(make1_str("("), $2, make1_str(")")); }
		| '(' ')'			{ $$ = make1_str("()"); }
		;

func_args_list:  TypeId				{ $$ = $1; }
		| func_args_list ',' TypeId
				{	$$ = cat3_str($1, make1_str(","), $3); }
		;

func_return:  set_opt TypeId
				{
					$$ = cat2_str($1, $2);
				}
		;

set_opt:  SETOF					{ $$ = make1_str("setof"); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
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
					$$ = cat3_str(make1_str("drop"), $2, $3);;
				}
		;

remove_type:  TYPE_P		{  $$ = make1_str("type"); }
		| INDEX		{  $$ = make1_str("index"); }
		| RULE		{  $$ = make1_str("rule"); }
		| VIEW		{  $$ = make1_str("view"); }
		;


RemoveAggrStmt:  DROP AGGREGATE name aggr_argtype
				{
						$$ = cat3_str(make1_str("drop aggregate"), $3, $4);
				}
		;

aggr_argtype:  name			{ $$ = $1; }
		| '*'			{ $$ = make1_str("*"); }
		;


RemoveFuncStmt:  DROP FUNCTION func_name func_args
				{
						$$ = cat3_str(make1_str("drop function"), $3, $4);
				}
		;


RemoveOperStmt:  DROP OPERATOR all_Op '(' oper_argtypes ')'
				{
					$$ = cat3_str(make1_str("drop operator"), $3, make3_str(make1_str("("), $5, make1_str(")")));
				}
		;

all_Op:  Op | MathOp;

MathOp:	'+'				{ $$ = make1_str("+"); }
		| '-'			{ $$ = make1_str("-"); }
		| '*'			{ $$ = make1_str("*"); }
		| '/'			{ $$ = make1_str("/"); }
		| '<'			{ $$ = make1_str("<"); }
		| '>'			{ $$ = make1_str(">"); }
		| '='			{ $$ = make1_str("="); }
		;

oper_argtypes:	name
				{
				   yyerror("parser: argument type missing (use NONE for unary operators)");
				}
		| name ',' name
				{ $$ = cat3_str($1, make1_str(","), $3); }
		| NONE ',' name			/* left unary */
				{ $$ = cat2_str(make1_str("none,"), $3); }
		| name ',' NONE			/* right unary */
				{ $$ = cat2_str($1, make1_str(", none")); }
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
					$$ = cat4_str(cat5_str(make1_str("alter table"), $3, $4, make1_str("rename"), $6), $7, make1_str("to"), $9);
				}
		;

opt_name:  name							{ $$ = $1; }
		| /*EMPTY*/					{ $$ = make1_str(""); }
		;

opt_column:  COLUMN					{ $$ = make1_str("colmunn"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
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
					$$ = cat2_str(cat5_str(cat5_str(make1_str("create rule"), $3, make1_str("as on"), $7, make1_str("to")), $9, $10, make1_str("do"), $12), $13);
				}
		;

RuleActionList:  NOTHING                               { $$ = make1_str("nothing"); }
               | SelectStmt                            { $$ = $1; }
               | RuleActionStmt                        { $$ = $1; }
               | '[' RuleActionBlock ']'               { $$ = cat3_str(make1_str("["), $2, make1_str("]")); }
               | '(' RuleActionBlock ')'               { $$ = cat3_str(make1_str("("), $2, make1_str(")")); }
                ;

RuleActionBlock:  RuleActionMulti              {  $$ = $1; }
               | RuleActionStmt                {  $$ = $1; }
		;

RuleActionMulti:  RuleActionMulti RuleActionStmt
                                {  $$ = cat2_str($1, $2); }
		| RuleActionMulti RuleActionStmt ';'
				{  $$ = cat3_str($1, $2, make1_str(";")); }
		| RuleActionStmt ';'
				{ $$ = cat2_str($1, make1_str(";")); }
		;

RuleActionStmt:        InsertStmt
                | UpdateStmt
                | DeleteStmt
		| NotifyStmt
                ;

event_object:  relation_name '.' attr_name
				{
					$$ = make3_str($1, make1_str("."), $3);
				}
		| relation_name
				{
					$$ = $1;
				}
		;

/* change me to select, update, etc. some day */
event:	SELECT					{ $$ = make1_str("select"); }
		| UPDATE			{ $$ = make1_str("update"); }
		| DELETE			{ $$ = make1_str("delete"); }
		| INSERT			{ $$ = make1_str("insert"); }
		 ;

opt_instead:  INSTEAD					{ $$ = make1_str("instead"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
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
					$$ = cat2_str(make1_str("notify"), $2);
				}
		;

ListenStmt:  LISTEN relation_name
				{
					$$ = cat2_str(make1_str("listen"), $2);
                                }
;

UnlistenStmt:  UNLISTEN relation_name
				{
					$$ = cat2_str(make1_str("unlisten"), $2);
                                }
		| UNLISTEN '*'
				{
					$$ = make1_str("unlisten *");
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
TransactionStmt:  ABORT_TRANS opt_trans	{ $$ = make1_str("rollback"); }
	| BEGIN_TRANS opt_trans		{ $$ = make1_str("begin transaction"); }
	| COMMIT opt_trans		{ $$ = make1_str("commit"); }
	| END_TRANS opt_trans			{ $$ = make1_str("commit"); }
	| ROLLBACK opt_trans			{ $$ = make1_str("rollback"); }

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
					$$ = cat4_str(make1_str("create view"), $3, make1_str("as"), $5);
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				load make1_str("filename")
 *
 *****************************************************************************/

LoadStmt:  LOAD file_name
				{
					$$ = cat2_str(make1_str("load"), $2);
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
						yyerror("CREATE DATABASE WITH requires at least an option");
#ifndef MULTIBYTE
					if (strlen($6) != 0)
						yyerror("WITH ENCODING is not supported");
#endif
					$$ = cat5_str(make1_str("create database"), $3, make1_str("with"), $5, $6);
				}
		| CREATE DATABASE database_name
        			{
					$$ = cat2_str(make1_str("create database"), $3);
				}
		;

opt_database1:  LOCATION '=' location	{ $$ = cat2_str(make1_str("location ="), $3); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
		;

opt_database2:  ENCODING '=' encoding   { $$ = cat2_str(make1_str("encoding ="), $3); }
                | /*EMPTY*/	        { $$ = NULL; }
                ;

location:  Sconst				{ $$ = $1; }
		| DEFAULT			{ $$ = make1_str("default"); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
		;

encoding:  Sconst		{ $$ = $1; }
		| DEFAULT	{ $$ = make1_str("default"); }
		| /*EMPTY*/	{ $$ = make1_str(""); }
               ;

/*****************************************************************************
 *
 *		QUERY:
 *				destroydb dbname
 *
 *****************************************************************************/

DestroydbStmt:	DROP DATABASE database_name
				{
					$$ = cat2_str(make1_str("drop database"), $3);
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
				   $$ = cat4_str(make1_str("cluster"), $2, make1_str("on"), $4);
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
					$$ = cat3_str(make1_str("vacuum"), $2, $3);
				}
		| VACUUM opt_verbose opt_analyze relation_name opt_va_list
				{
					if ( strlen($5) > 0 && strlen($4) == 0 )
						yyerror("parser: syntax error at or near \"(\"");
					$$ = cat5_str(make1_str("vacuum"), $2, $3, $4, $5);
				}
		;

opt_verbose:  VERBOSE					{ $$ = make1_str("verbose"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

opt_analyze:  ANALYZE					{ $$ = make1_str("analyse"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

opt_va_list:  '(' va_list ')'				{ $$ = make3_str(make1_str("("), $2, make1_str(")")); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

va_list:  name
				{ $$=$1; }
		| va_list ',' name
				{ $$=cat3_str($1, make1_str(","), $3); }
		;


/*****************************************************************************
 *
 *		QUERY:
 *				EXPLAIN query
 *
 *****************************************************************************/

ExplainStmt:  EXPLAIN opt_verbose OptimizableStmt
				{
					$$ = cat3_str(make1_str("explain"), $2, $3);
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

/***S*I***/
/* This rule used 'opt_column_list' between 'relation_name' and 'insert_rest'
 * originally. When the second rule of 'insert_rest' was changed to use
 * the new 'SelectStmt' rule (for INTERSECT and EXCEPT) it produced a shift/red uce
 * conflict. So I just changed the rules 'InsertStmt' and 'insert_rest' to accept
 * the same statements without any shift/reduce conflicts */
InsertStmt:  INSERT INTO relation_name insert_rest
				{
					$$ = cat3_str(make1_str("insert into"), $3, $4);
				}
		;

insert_rest:  VALUES '(' res_target_list2 ')'
				{
					$$ = make3_str(make1_str("values("), $3, make1_str(")"));
				}
		| DEFAULT VALUES
				{
					$$ = make1_str("default values");
				}
		| SelectStmt
				{
					$$ = $1
				}
		| '(' columnList ')' VALUES '(' res_target_list2 ')'
				{
					$$ = make5_str(make1_str("("), $2, make1_str(") values ("), $6, make1_str(")"));
				}
		| '(' columnList ')' SelectStmt
				{
					$$ = make4_str(make1_str("("), $2, make1_str(")"), $4);
				}
		;

opt_column_list:  '(' columnList ')'			{ $$ = make3_str(make1_str("("), $2, make1_str(")")); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

columnList:
		  columnList ',' columnElem
				{ $$ = cat3_str($1, make1_str(","), $3); }
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
					$$ = cat3_str(make1_str("delete from"), $3, $4);
				}
		;

LockStmt:  LOCK_P opt_table relation_name
				{
					$$ = cat3_str(make1_str("lock"), $2, $3);
				}
		|       LOCK_P opt_table relation_name IN opt_lmode ROW IDENT IDENT
				{
					if (strcasecmp($8, "MODE"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $8);
						yyerror(errortext);
					}
					if ($5 != NULL)
                                        {
                                                if (strcasecmp($5, "SHARE"))
						{
                                                        sprintf(errortext, "syntax error at or near \"%s\"", $5);
	                                                yyerror(errortext);
						}
                                                if (strcasecmp($7, "EXCLUSIVE"))
						{
							sprintf(errortext, "syntax error at or near \"%s\"", $7);
	                                                yyerror(errortext);
						}
					}
                                        else
                                        {
                                                if (strcasecmp($7, "SHARE") && strcasecmp($7, "EXCLUSIVE"))
						{
                                               		sprintf(errortext, "syntax error at or near \"%s\"", $7);
	                                                yyerror(errortext);
						}
                                        }

					$$=cat4_str(cat5_str(make1_str("lock"), $2, $3, make1_str("in"), $5), make1_str("row"), $7, $8);
				}
		|       LOCK_P opt_table relation_name IN IDENT IDENT IDENT
				{
					if (strcasecmp($7, "MODE"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $7);
                                                yyerror(errortext);
					}                                
                                        if (strcasecmp($5, "ACCESS"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $5);
                                                yyerror(errortext);
					}
                                        if (strcasecmp($6, "SHARE") && strcasecmp($6, "EXCLUSIVE"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $6);
                                                yyerror(errortext);
					}

					$$=cat3_str(cat5_str(make1_str("lock"), $2, $3, make1_str("in"), $5), $6, $7);
				}
		|       LOCK_P opt_table relation_name IN IDENT IDENT
				{
					if (strcasecmp($6, "MODE"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $6);
						yyerror(errortext);
					}
                                        if (strcasecmp($5, "SHARE") && strcasecmp($5, "EXCLUSIVE"))
					{
                                                sprintf(errortext, "syntax error at or near \"%s\"", $5);
                                                yyerror(errortext);
					}

					$$=cat2_str(cat5_str(make1_str("lock"), $2, $3, make1_str("in"), $5), $6);
				}
		;

opt_lmode:      IDENT           { $$ = $1; }
                | /*EMPTY*/     { $$ = make1_str(""); }
                ;




/*****************************************************************************
 *
 *		QUERY:
 *				UpdateStmt (UPDATE)
 *
 *****************************************************************************/

UpdateStmt:  UPDATE relation_name
			  SET res_target_list
			  from_clause
			  where_clause
				{
					$$ = cat2_str(cat5_str(make1_str("update"), $2, make1_str("set"), $4, $5), $6);
				}
		;


/*****************************************************************************
 *
 *		QUERY:
 *				CURSOR STATEMENTS
 *
 *****************************************************************************/
CursorStmt:  DECLARE name opt_cursor CURSOR FOR SelectStmt cursor_clause
				{
					struct cursor *ptr, *this;
	
					for (ptr = cur; ptr != NULL; ptr = ptr->next)
					{
						if (strcmp($2, ptr->name) == 0)
						{
						        /* re-definition is a bug */
							sprintf(errortext, "cursor %s already defined", $2);
							yyerror(errortext);
				                }
        				}
                        
        				this = (struct cursor *) mm_alloc(sizeof(struct cursor));

			        	/* initial definition */
				        this->next = cur;
				        this->name = $2;
					this->connection = connection;
				        this->command =  cat2_str(cat5_str(make1_str("declare"), mm_strdup($2), $3, make1_str("cursor for"), $6), $7);
					this->argsinsert = argsinsert;
					this->argsresult = argsresult;
					argsinsert = argsresult = NULL;
											
			        	cur = this;
					
					$$ = cat3_str(make1_str("/*"), mm_strdup(this->command), make1_str("*/"));
				}
		;

opt_cursor:  BINARY             { $$ = make1_str("binary"); }
               | INSENSITIVE	{ $$ = make1_str("insensitive"); }
               | SCROLL         { $$ = make1_str("scroll"); }
               | INSENSITIVE SCROLL	{ $$ = make1_str("insensitive scroll"); }
               | /*EMPTY*/      { $$ = make1_str(""); }
               ;

cursor_clause:  FOR opt_readonly	{ $$ = cat2_str(make1_str("for"), $2); }
               | /*EMPTY*/              { $$ = make1_str(""); }

               ;

opt_readonly:  READ ONLY		{ $$ = make1_str("read only"); }
               | UPDATE opt_of
                       {
                               yyerror("DECLARE/UPDATE not supported; Cursors must be READ ONLY.");
                       }
               ;

opt_of:  OF columnList { $$ = make2_str(make1_str("of"), $2); }

/*****************************************************************************
 *
 *		QUERY:
 *				SELECT STATEMENTS
 *
 *****************************************************************************/

/***S*I***/
/* The new 'SelectStmt' rule adapted for the optional use of INTERSECT EXCEPT a nd UNION
 * accepts the use of '(' and ')' to select an order of set operations.
 * The rule returns a SelectStmt Node having the set operations attached to
 * unionClause and intersectClause (NIL if no set operations were present)
 */

SelectStmt:      select_clause sort_clause for_update_clause opt_select_limit
				{
					if (strlen($3) > 0 && ForUpdateNotAllowed != 0)
							yyerror("SELECT FOR UPDATE is not allowed in this context");

					ForUpdateNotAllowed = 0;
					$$ = cat4_str($1, $2, $3, $4);
				}

/***S*I***/ 
/* This rule parses Select statements including UNION INTERSECT and EXCEPT.
 * '(' and ')' can be used to specify the order of the operations 
 * (UNION EXCEPT INTERSECT). Without the use of '(' and ')' we want the
 * operations to be left associative.
 *
 *  The sort_clause is not handled here!
 */
select_clause: '(' select_clause ')'
                        {
                               $$ = make3_str(make1_str("("), $2, make1_str(")")); 
                        }
                | SubSelect
                        {
                               $$ = $1; 
                        }
                | select_clause EXCEPT select_clause
			{
				$$ = cat3_str($1, make1_str("except"), $3);
				ForUpdateNotAllowed = 1;
			}
		| select_clause UNION opt_union select_clause
			{
				$$ = cat3_str($1, make1_str("union"), $3);
				ForUpdateNotAllowed = 1;
			}
		| select_clause INTERSECT opt_union select_clause
			{
				$$ = cat3_str($1, make1_str("intersect"), $3);
				ForUpdateNotAllowed = 1;
			}
		;

/***S*I***/
SubSelect:     SELECT opt_unique res_target_list2
                         result from_clause where_clause
                         group_clause having_clause
				{
					$$ = cat4_str(cat5_str(make1_str("select"), $2, $3, $4, $5), $6, $7, $8);
					if (strlen($7) > 0 || strlen($8) > 0)
						ForUpdateNotAllowed = 1;
				}
		;

result:  INTO OptTemp opt_table relation_name		{ $$= cat4_str(make1_str("into"), $2, $3, $4); }
		| INTO into_list			{ $$ = make1_str(""); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

opt_table:  TABLE					{ $$ = make1_str("table"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

opt_union:  ALL						{ $$ = make1_str("all"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

opt_unique:  DISTINCT					{ $$ = make1_str("distinct"); }
		| DISTINCT ON ColId			{ $$ = cat2_str(make1_str("distinct on"), $3); }
		| ALL					{ $$ = make1_str("all"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

sort_clause:  ORDER BY sortby_list			{ $$ = cat2_str(make1_str("order by"), $3); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

sortby_list:  sortby					{ $$ = $1; }
		| sortby_list ',' sortby		{ $$ = cat3_str($1, make1_str(","), $3); }
		;

sortby: a_expr OptUseOp
				{
					 $$ = cat2_str($1, $2);
                                }
		;

OptUseOp:  USING Op				{ $$ = cat2_str(make1_str("using"), $2); }
		| USING '<'			{ $$ = make1_str("using <"); }
		| USING '>'			{ $$ = make1_str("using >"); }
		| ASC				{ $$ = make1_str("asc"); }
		| DESC				{ $$ = make1_str("desc"); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
		;

opt_select_limit:      LIMIT select_limit_value ',' select_offset_value
                       { $$ = cat4_str(make1_str("limit"), $2, make1_str(","), $4); }
               | LIMIT select_limit_value OFFSET select_offset_value
                       { $$ = cat4_str(make1_str("limit"), $2, make1_str("offset"), $4); }
               | LIMIT select_limit_value
                       { $$ = cat2_str(make1_str("limit"), $2);; }
               | OFFSET select_offset_value LIMIT select_limit_value
                       { $$ = cat4_str(make1_str("offset"), $2, make1_str("limit"), $4); }
               | OFFSET select_offset_value
                       { $$ = cat2_str(make1_str("offset"), $2); }
               | /* EMPTY */
                       { $$ = make1_str(""); }
               ;

select_limit_value:	Iconst	{ $$ = $1; }
	          	| ALL	{ $$ = make1_str("all"); }
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
opt_inh_star:  '*'					{ $$ = make1_str("*"); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

relation_name_list:  name_list { $$ = $1; };

name_list:  name
				{	$$ = $1; }
		| name_list ',' name
				{	$$ = cat3_str($1, make1_str(","), $3); }
		;

group_clause:  GROUP BY expr_list			{ $$ = cat2_str(make1_str("groub by"), $3); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

having_clause:  HAVING a_expr
				{
					$$ = cat2_str(make1_str("having"), $2);
				}
		| /*EMPTY*/		{ $$ = make1_str(""); }
		;

for_update_clause:  FOR UPDATE update_list
		{
                	$$ = make1_str("for update"); 
		}
		| /* EMPTY */
                {
                        $$ = make1_str("");
                }
                ;
update_list:  OF va_list
              {
			$$ = cat2_str(make1_str("of"), $2);
	      }
              | /* EMPTY */
              {
                        $$ = make1_str("");
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
			$$ = cat2_str(make1_str("from"), $2);
		}
		| /* EMPTY */
		{
			$$ = make1_str("");
		}


from_expr:  '(' join_clause_with_union ')'
                                { $$ = make3_str(make1_str("("), $2, make1_str(")")); }
                | join_clause
                                { $$ = $1; }
                | table_list
                                { $$ = $1; }
                ;

table_list:  table_list ',' table_expr
                                { $$ = make3_str($1, make1_str(","), $3); }
                | table_expr
                                { $$ = $1; }
                ;

table_expr:  relation_expr AS ColLabel
                                {
                                        $$ = cat3_str($1, make1_str("as"), $3);
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
                                {       yyerror("UNION JOIN not yet implemented"); }
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
					$$ = cat4_str($1, make1_str("join"), $3, $4);
                                }
                | NATURAL join_type JOIN table_expr
                                {
					$$ = cat4_str(make1_str("natural"), $2, make1_str("join"), $4);
                                }
                | CROSS JOIN table_expr
                                { 	$$ = cat2_str(make1_str("cross join"), $3); }
                ;

/* OUTER is just noise... */
join_type:  FULL join_outer
                                {
                                        $$ = cat2_str(make1_str("full"), $2);
                                        fprintf(stderr,"FULL OUTER JOIN not yet implemented\n");
                                }
                | LEFT join_outer
                                {
                                        $$ = cat2_str(make1_str("left"), $2);
                                        fprintf(stderr,"LEFT OUTER JOIN not yet implemented\n");
                                }
                | RIGHT join_outer
                                {
                                        $$ = cat2_str(make1_str("right"), $2);
                                        fprintf(stderr,"RIGHT OUTER JOIN not yet implemented\n");
                                }
                | OUTER_P
                                {
                                        $$ = make1_str("outer");
                                        fprintf(stderr,"OUTER JOIN not yet implemented\n");
                                }
                | INNER_P
                                {
                                        $$ = make1_str("inner");
				}
                | /* EMPTY */
                                {
                                        $$ = make1_str("");
				}


join_outer:  OUTER_P				{ $$ = make1_str("outer"); }
		| /*EMPTY*/			{ $$ = make1_str("");  /* no qualifiers */ }
		;

/* JOIN qualification clauses
 * Possibilities are:
 *  USING ( column list ) allows only unqualified column names,
 *                        which must match between tables.
 *  ON expr allows more general qualifications.
 * - thomas 1999-01-07
 */

join_qual:  USING '(' using_list ')'                   { $$ = make3_str(make1_str("using ("), $3, make1_str(")")); }
               | ON a_expr			       { $$ = cat2_str(make1_str("on"), $2); }
                ;

using_list:  using_list ',' using_expr                  { $$ = make3_str($1, make1_str(","), $3); }
               | using_expr				{ $$ = $1; }
               ;

using_expr:  ColId
				{
					$$ = $1;
				}
		;

where_clause:  WHERE a_expr			{ $$ = cat2_str(make1_str("where"), $2); }
		| /*EMPTY*/				{ $$ = make1_str("");  /* no qualifiers */ }
		;

relation_expr:	relation_name
				{
					/* normal relations */
					$$ = $1;
				}
		| relation_name '*'				  %prec '='
				{
					/* inheritance query */
					$$ = cat2_str($1, make1_str("*"));
				}

opt_array_bounds:  '[' ']' nest_array_bounds
			{
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make1_str("[]"), $3.str);
                        }
		| '[' Iconst ']' nest_array_bounds
			{
                            $$.index1 = atol($2);
                            $$.index2 = $4.index1;
                            $$.str = cat4_str(make1_str("["), $2, make1_str("]"), $4.str);
                        }
		| /* EMPTY */
			{
                            $$.index1 = -1;
                            $$.index2 = -1;
                            $$.str= make1_str("");
                        }
		;

nest_array_bounds:	'[' ']' nest_array_bounds
                        {
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make1_str("[]"), $3.str);
                        }
		| '[' Iconst ']' nest_array_bounds
			{
                            $$.index1 = atol($2);
                            $$.index2 = $4.index1;
                            $$.str = cat4_str(make1_str("["), $2, make1_str("]"), $4.str);
                        }
		| /* EMPTY */
			{
                            $$.index1 = -1;
                            $$.index2 = -1;
                            $$.str= make1_str("");
                        }
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

Typename:  Array opt_array_bounds
				{
					$$ = cat2_str($1, $2.str);
				}
		| Character	{ $$ = $1; }
		| SETOF Array
				{
					$$ = cat2_str(make1_str("setof"), $2);
				}
		;

Array:  Generic
		| Datetime	{ $$ = $1; }
		| Numeric	{ $$ = $1; }
		;

Generic:  generic
				{
					$$ = $1;
				}
		;

generic:  ident					{ $$ = $1; }
		| TYPE_P			{ $$ = make1_str("type"); }
		| SQL_AT			{ $$ = make1_str("at"); }
		| SQL_BOOL			{ $$ = make1_str("bool"); }
		| SQL_BREAK			{ $$ = make1_str("break"); }
		| SQL_CALL			{ $$ = make1_str("call"); }
		| SQL_CONNECT			{ $$ = make1_str("connect"); }
		| SQL_CONNECTION		{ $$ = make1_str("connection"); }
		| SQL_CONTINUE			{ $$ = make1_str("continue"); }
		| SQL_DEALLOCATE		{ $$ = make1_str("deallocate"); }
		| SQL_DISCONNECT		{ $$ = make1_str("disconnect"); }
		| SQL_FOUND			{ $$ = make1_str("found"); }
		| SQL_GO			{ $$ = make1_str("go"); }
		| SQL_GOTO			{ $$ = make1_str("goto"); }
		| SQL_IDENTIFIED		{ $$ = make1_str("identified"); }
		| SQL_IMMEDIATE			{ $$ = make1_str("immediate"); }
		| SQL_INDICATOR			{ $$ = make1_str("indicator"); }
		| SQL_INT			{ $$ = make1_str("int"); }
		| SQL_LONG			{ $$ = make1_str("long"); }
		| SQL_OPEN			{ $$ = make1_str("open"); }
		| SQL_PREPARE			{ $$ = make1_str("prepare"); }
		| SQL_RELEASE			{ $$ = make1_str("release"); }
		| SQL_SECTION			{ $$ = make1_str("section"); }
		| SQL_SHORT			{ $$ = make1_str("short"); }
		| SQL_SIGNED			{ $$ = make1_str("signed"); }
		| SQL_SQLERROR			{ $$ = make1_str("sqlerror"); }
		| SQL_SQLPRINT			{ $$ = make1_str("sqlprint"); }
		| SQL_SQLWARNING		{ $$ = make1_str("sqlwarning"); }
		| SQL_STOP			{ $$ = make1_str("stop"); }
		| SQL_STRUCT			{ $$ = make1_str("struct"); }
		| SQL_UNSIGNED			{ $$ = make1_str("unsigned"); }
		| SQL_VAR			{ $$ = make1_str("var"); }
		| SQL_WHENEVER			{ $$ = make1_str("whenever"); }
		;

/* SQL92 numeric data types
 * Check FLOAT() precision limits assuming IEEE floating types.
 * Provide real DECIMAL() and NUMERIC() implementations now - Jan 1998-12-30
 * - thomas 1997-09-18
 */
Numeric:  FLOAT opt_float
				{
					$$ = cat2_str(make1_str("float"), $2);
				}
		| DOUBLE PRECISION
				{
					$$ = make1_str("double precision");
				}
		| DECIMAL opt_decimal
				{
					$$ = cat2_str(make1_str("decimal"), $2);
				}
		| NUMERIC opt_numeric
				{
					$$ = cat2_str(make1_str("numeric"), $2);
				}
		;

numeric:  FLOAT
				{	$$ = make1_str("float"); }
		| DOUBLE PRECISION
				{	$$ = make1_str("double precision"); }
		| DECIMAL
				{	$$ = make1_str("decimal"); }
		| NUMERIC
				{	$$ = make1_str("numeric"); }
		;

opt_float:  '(' Iconst ')'
				{
					if (atol($2) < 1)
						yyerror("precision for FLOAT must be at least 1");
					else if (atol($2) >= 16)
						yyerror("precision for FLOAT must be less than 16");
					$$ = make3_str(make1_str("("), $2, make1_str(")"));
				}
		| /*EMPTY*/
				{
					$$ = make1_str("");
				}
		;

opt_numeric:  '(' Iconst ',' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						yyerror(errortext);
					}
					if (atol($4) < 0 || atol($4) > atol($2)) {
						sprintf(errortext, "NUMERIC scale %s must be between 0 and precision %s", $4, $2);
						yyerror(errortext);
					}
					$$ = cat3_str(make2_str(make1_str("("), $2), make1_str(","), make2_str($4, make1_str(")")));
				}
		| '(' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						yyerror(errortext);
					}
					$$ = make3_str(make1_str("("), $2, make1_str(")"));
				}
		| /*EMPTY*/
				{
					$$ = make1_str("");
				}
		;

opt_decimal:  '(' Iconst ',' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						yyerror(errortext);
					}
					if (atol($4) < 0 || atol($4) > atol($2)) {
						sprintf(errortext, "NUMERIC scale %s must be between 0 and precision %s", $4, $2);
						yyerror(errortext);
					}
					$$ = cat3_str(make2_str(make1_str("("), $2), make1_str(","), make2_str($4, make1_str(")")));
				}
		| '(' Iconst ')'
				{
					if (atol($2) < 1 || atol($2) > NUMERIC_MAX_PRECISION) {
						sprintf(errortext, "NUMERIC precision %s must be between 1 and %d", $2, NUMERIC_MAX_PRECISION);
						yyerror(errortext);
					}
					$$ = make3_str(make1_str("("), $2, make1_str(")"));
				}
		| /*EMPTY*/
				{
					$$ = make1_str("");
				}
		;

/* SQL92 character data types
 * The following implements CHAR() and VARCHAR().
 * We do it here instead of the 'Generic' production
 * because we don't want to allow arrays of VARCHAR().
 * I haven't thought about whether that will work or not.
 *								- ay 6/95
 */
Character:  character '(' Iconst ')'
				{
					if (strncasecmp($1, "char", strlen("char")) && strncasecmp($1, "varchar", strlen("varchar")))
						yyerror("internal parsing error; unrecognized character type");
					if (atol($3) < 1) {
						sprintf(errortext, "length for '%s' type must be at least 1",$1);
						yyerror(errortext);
					}
					else if (atol($3) > 4096) {
						/* we can store a char() of length up to the size
						 * of a page (8KB) - page headers and friends but
						 * just to be safe here...	- ay 6/95
						 * XXX note this hardcoded limit - thomas 1997-07-13
						 */
						sprintf(errortext, "length for type '%s' cannot exceed 4096",$1);
						yyerror(errortext);
					}

					$$ = cat2_str($1, make3_str(make1_str("("), $3, make1_str(")")));
				}
		| character
				{
					$$ = $1;
				}
		;

character:  CHARACTER opt_varying opt_charset opt_collate
				{
					if (strlen($4) > 0) 
						fprintf(stderr, "COLLATE %s not yet implemented",$4);

					$$ = cat4_str(make1_str("character"), $2, $3, $4);
				}
		| CHAR opt_varying	{ $$ = cat2_str(make1_str("char"), $2); }
		| VARCHAR		{ $$ = make1_str("varchar"); }
		| NATIONAL CHARACTER opt_varying { $$ = cat2_str(make1_str("national character"), $3); }
		| NCHAR opt_varying		{ $$ = cat2_str(make1_str("nchar"), $2); }
		;

opt_varying:  VARYING			{ $$ = make1_str("varying"); }
		| /*EMPTY*/			{ $$ = make1_str(""); }
		;

opt_charset:  CHARACTER SET ColId	{ $$ = cat2_str(make1_str("character set"), $3); }
		| /*EMPTY*/				{ $$ = make1_str(""); }
		;

opt_collate:  COLLATE ColId		{ $$ = cat2_str(make1_str("collate"), $2); }
		| /*EMPTY*/					{ $$ = make1_str(""); }
		;

Datetime:  datetime
				{
					$$ = $1;
				}
		| TIMESTAMP opt_timezone
				{
					$$ = cat2_str(make1_str("timestamp"), $2);
				}
		| TIME
				{
					$$ = make1_str("time");
				}
		| INTERVAL opt_interval
				{
					$$ = cat2_str(make1_str("interval"), $2);
				}
		;

datetime:  YEAR_P								{ $$ = make1_str("year"); }
		| MONTH_P								{ $$ = make1_str("month"); }
		| DAY_P									{ $$ = make1_str("day"); }
		| HOUR_P								{ $$ = make1_str("hour"); }
		| MINUTE_P								{ $$ = make1_str("minute"); }
		| SECOND_P								{ $$ = make1_str("second"); }
		;

opt_timezone:  WITH TIME ZONE				{ $$ = make1_str("with time zone"); }
		| /*EMPTY*/					{ $$ = make1_str(""); }
		;

opt_interval:  datetime					{ $$ = $1; }
		| YEAR_P TO MONTH_P			{ $$ = make1_str("year to #month"); }
		| DAY_P TO HOUR_P			{ $$ = make1_str("day to hour"); }
		| DAY_P TO MINUTE_P			{ $$ = make1_str("day to minute"); }
		| DAY_P TO SECOND_P			{ $$ = make1_str("day to second"); }
		| HOUR_P TO MINUTE_P			{ $$ = make1_str("hour to minute"); }
		| MINUTE_P TO SECOND_P			{ $$ = make1_str("minute to second"); }
		| HOUR_P TO SECOND_P			{ $$ = make1_str("hour to second"); }
		| /*EMPTY*/					{ $$ = make1_str(""); }
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
					$$ = make1_str("null");
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
					$$ = make5_str(make1_str("("), $2, make1_str(") in ("), $6, make1_str(")"));
				}
		| '(' row_descriptor ')' NOT IN '(' SubSelect ')'
				{
					$$ = make5_str(make1_str("("), $2, make1_str(") not in ("), $7, make1_str(")"));
				}
		| '(' row_descriptor ')' row_op sub_type  '(' SubSelect ')'
				{
					$$ = make4_str(make5_str(make1_str("("), $2, make1_str(")"), $4, $5), make1_str("("), $7, make1_str(")"));
				}
		| '(' row_descriptor ')' row_op '(' SubSelect ')'
				{
					$$ = make3_str(make5_str(make1_str("("), $2, make1_str(")"), $4, make1_str("(")), $6, make1_str(")"));
				}
		| '(' row_descriptor ')' row_op '(' row_descriptor ')'
				{
					$$ = cat3_str(make3_str(make1_str("("), $2, make1_str(")")), $4, make3_str(make1_str("("), $6, make1_str(")")));
				}
		;

row_descriptor:  row_list ',' a_expr
				{
					$$ = cat3_str($1, make1_str(","), $3);
				}
		;

row_op:  Op			{ $$ = $1; }
	| '<'                   { $$ = "<"; }
        | '='                   { $$ = "="; }
        | '>'                   { $$ = ">"; }
        | '+'                   { $$ = "+"; }
        | '-'                   { $$ = "-"; }
        | '*'                   { $$ = "*"; }
        | '/'                   { $$ = "/"; }
              ;

sub_type:  ANY                  { $$ = make1_str("ANY"); }
         | ALL                  { $$ = make1_str("ALL"); }
              ;


row_list:  row_list ',' a_expr
				{
					$$ = cat3_str($1, make1_str(","), $3);
				}
		| a_expr
				{
					$$ = $1;
				}
		;

/* General expressions
 * This is the heart of the expression syntax.
 * Note that the BETWEEN clause looks similar to a boolean expression
 *  and so we must define b_expr which is almost the same as a_expr
 *  but without the boolean expressions.
 * All operations/expressions are allowed in a BETWEEN clause
 *  if surrounded by parens.
 */

a_expr:  attr opt_indirection
				{
					$$ = cat2_str($1, $2);
				}
		| row_expr
				{	$$ = $1;  }
		| AexprConst
				{	$$ = $1;  }
		| ColId
				{
					$$ = $1;
				}
		| '-' a_expr %prec UMINUS
				{	$$ = cat2_str(make1_str("-"), $2); }
		| a_expr '+' a_expr
				{	$$ = cat3_str($1, make1_str("+"), $3); }
		| a_expr '-' a_expr
				{	$$ = cat3_str($1, make1_str("-"), $3); }
		| a_expr '/' a_expr
				{	$$ = cat3_str($1, make1_str("/"), $3); }
		| a_expr '*' a_expr
				{	$$ = cat3_str($1, make1_str("*"), $3); }
		| a_expr '<' a_expr
				{	$$ = cat3_str($1, make1_str("<"), $3); }
		| a_expr '>' a_expr
				{	$$ = cat3_str($1, make1_str(">"), $3); }
		| a_expr '=' a_expr
				{	$$ = cat3_str($1, make1_str("="), $3); }
/* not possible in embedded sql		| ':' a_expr
				{	$$ = cat2_str(make1_str(":"), $2); }
*/
		| ';' a_expr
				{	$$ = cat2_str(make1_str(";"), $2); }
		| '|' a_expr
				{	$$ = cat2_str(make1_str("|"), $2); }
		| a_expr TYPECAST Typename
				{
					$$ = cat3_str($1, make1_str("::"), $3);
				}
		| CAST '(' a_expr AS Typename ')'
				{
					$$ = cat3_str(make2_str(make1_str("cast("), $3), make1_str("as"), make2_str($5, make1_str(")")));
				}
		| '(' a_expr_or_null ')'
				{	$$ = make3_str(make1_str("("), $2, make1_str(")")); }
		| a_expr Op a_expr
				{	$$ = cat3_str($1, $2, $3);	}
		| a_expr LIKE a_expr
				{	$$ = cat3_str($1, make1_str("like"), $3); }
		| a_expr NOT LIKE a_expr
				{	$$ = cat3_str($1, make1_str("not like"), $4); }
		| Op a_expr
				{	$$ = cat2_str($1, $2); }
		| a_expr Op
				{	$$ = cat2_str($1, $2); }
		| func_name '(' '*' ')'
				{
					$$ = cat2_str($1, make1_str("(*)")); 
				}
		| func_name '(' ')'
				{
					$$ = cat2_str($1, make1_str("()")); 
				}
		| func_name '(' expr_list ')'
				{
					$$ = make4_str($1, make1_str("("), $3, make1_str(")")); 
				}
		| CURRENT_DATE
				{
					$$ = make1_str("current_date");
				}
		| CURRENT_TIME
				{
					$$ = make1_str("current_time");
				}
		| CURRENT_TIME '(' Iconst ')'
				{
					if (atol($3) != 0)
						fprintf(stderr,"CURRENT_TIME(%s) precision not implemented; zero used instead", $3);
					$$ = make1_str("current_time");
				}
		| CURRENT_TIMESTAMP
				{
					$$ = make1_str("current_timestamp");
				}
		| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					if (atol($3) != 0)
						fprintf(stderr,"CURRENT_TIMESTAMP(%s) precision not implemented; zero used instead",$3);
					$$ = make1_str("current_timestamp");
				}
		| CURRENT_USER
				{
					$$ = make1_str("current_user");
				}
		| USER
				{
  		     		        $$ = make1_str("user");
			     	}

		| EXISTS '(' SubSelect ')'
				{
					$$ = make3_str(make1_str("exists("), $3, make1_str(")"));
				}
		| EXTRACT '(' extract_list ')'
				{
					$$ = make3_str(make1_str("extract("), $3, make1_str(")"));
				}
		| POSITION '(' position_list ')'
				{
					$$ = make3_str(make1_str("position("), $3, make1_str(")"));
				}
		| SUBSTRING '(' substr_list ')'
				{
					$$ = make3_str(make1_str("substring("), $3, make1_str(")"));
				}
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
				{
					$$ = make3_str(make1_str("trim(both"), $4, make1_str(")"));
				}
		| TRIM '(' LEADING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(leading"), $4, make1_str(")"));
				}
		| TRIM '(' TRAILING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(trailing"), $4, make1_str(")"));
				}
		| TRIM '(' trim_list ')'
				{
					$$ = make3_str(make1_str("trim("), $3, make1_str(")"));
				}
		| a_expr ISNULL
				{	$$ = cat2_str($1, make1_str("isnull")); }
		| a_expr IS NULL_P
				{	$$ = cat2_str($1, make1_str("is null")); }
		| a_expr NOTNULL
				{	$$ = cat2_str($1, make1_str("notnull")); }
		| a_expr IS NOT NULL_P
				{	$$ = cat2_str($1, make1_str("is not null")); }
		/* IS TRUE, IS FALSE, etc used to be function calls
		 *  but let's make them expressions to allow the optimizer
		 *  a chance to eliminate them if a_expr is a constant string.
		 * - thomas 1997-12-22
		 */
		| a_expr IS TRUE_P
				{
				{	$$ = cat2_str($1, make1_str("is true")); }
				}
		| a_expr IS NOT FALSE_P
				{
				{	$$ = cat2_str($1, make1_str("is not false")); }
				}
		| a_expr IS FALSE_P
				{
				{	$$ = cat2_str($1, make1_str("is false")); }
				}
		| a_expr IS NOT TRUE_P
				{
				{	$$ = cat2_str($1, make1_str("is not true")); }
				}
		| a_expr BETWEEN b_expr AND b_expr
				{
					$$ = cat5_str($1, make1_str("between"), $3, make1_str("and"), $5); 
				}
		| a_expr NOT BETWEEN b_expr AND b_expr
				{
					$$ = cat5_str($1, make1_str("not between"), $4, make1_str("and"), $6); 
				}
		| a_expr IN '(' in_expr ')'
				{
					$$ = make4_str($1, make1_str("in ("), $4, make1_str(")")); 
				}
		| a_expr NOT IN '(' not_in_expr ')'
				{
					$$ = make4_str($1, make1_str("not in ("), $5, make1_str(")")); 
				}
		| a_expr Op '(' SubSelect ')'
				{
					$$ = cat3_str($1, $2, make3_str(make1_str("("), $4, make1_str(")"))); 
				}
		| a_expr '+' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("+("), $4, make1_str(")")); 
				}
		| a_expr '-' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("-("), $4, make1_str(")")); 
				}
		| a_expr '/' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("/("), $4, make1_str(")")); 
				}
		| a_expr '*' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("*("), $4, make1_str(")")); 
				}
		| a_expr '<' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("<("), $4, make1_str(")")); 
				}
		| a_expr '>' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str(">("), $4, make1_str(")")); 
				}
		| a_expr '=' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("=("), $4, make1_str(")")); 
				}
		| a_expr Op ANY '(' SubSelect ')'
				{
					$$ = cat3_str($1, $2, make3_str(make1_str("any("), $5, make1_str(")"))); 
				}
		| a_expr '+' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("+any("), $5, make1_str(")")); 
				}
		| a_expr '-' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("-any("), $5, make1_str(")")); 
				}
		| a_expr '/' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("/any("), $5, make1_str(")")); 
				}
		| a_expr '*' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("*any("), $5, make1_str(")")); 
				}
		| a_expr '<' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("<any("), $5, make1_str(")")); 
				}
		| a_expr '>' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str(">any("), $5, make1_str(")")); 
				}
		| a_expr '=' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("=any("), $5, make1_str(")")); 
				}
		| a_expr Op ALL '(' SubSelect ')'
				{
					$$ = cat3_str($1, $2, make3_str(make1_str("all ("), $5, make1_str(")"))); 
				}
		| a_expr '+' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("+all("), $5, make1_str(")")); 
				}
		| a_expr '-' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("-all("), $5, make1_str(")")); 
				}
		| a_expr '/' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("/all("), $5, make1_str(")")); 
				}
		| a_expr '*' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("*all("), $5, make1_str(")")); 
				}
		| a_expr '<' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("<all("), $5, make1_str(")")); 
				}
		| a_expr '>' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str(">all("), $5, make1_str(")")); 
				}
		| a_expr '=' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("=all("), $5, make1_str(")")); 
				}
		| a_expr AND a_expr
				{	$$ = cat3_str($1, make1_str("and"), $3); }
		| a_expr OR a_expr
				{	$$ = cat3_str($1, make1_str("or"), $3); }
		| NOT a_expr
				{	$$ = cat2_str(make1_str("not"), $2); }
		| case_expr
				{       $$ = $1; }
		| cinputvariable
			        { $$ = make1_str(";;"); }
		;

/* Restricted expressions
 * b_expr is a subset of the complete expression syntax
 *  defined by a_expr. b_expr is used in BETWEEN clauses
 *  to eliminate parser ambiguities stemming from the AND keyword.
 */
b_expr:  attr opt_indirection
				{
					$$ = cat2_str($1, $2);
				}
		| AexprConst
				{	$$ = $1;  }
		| ColId
				{
					$$ = $1;
				}
		| '-' b_expr %prec UMINUS
				{	$$ = cat2_str(make1_str("-"), $2); }
		| b_expr '+' b_expr
				{	$$ = cat3_str($1, make1_str("+"), $3); }
		| b_expr '-' b_expr
				{	$$ = cat3_str($1, make1_str("-"), $3); }
		| b_expr '/' b_expr
				{	$$ = cat3_str($1, make1_str("/"), $3); }
		| b_expr '*' b_expr
				{	$$ = cat3_str($1, make1_str("*"), $3); }
/* not possible in embedded sql		| ':' b_expr
				{	$$ = cat2_str(make1_str(":"), $2); }
*/
		| ';' b_expr
				{	$$ = cat2_str(make1_str(";"), $2); }
		| '|' b_expr
				{	$$ = cat2_str(make1_str("|"), $2); }
		| b_expr TYPECAST Typename
				{
					$$ = cat3_str($1, make1_str("::"), $3);
				}
		| CAST '(' b_expr AS Typename ')'
				{
					$$ = cat3_str(make2_str(make1_str("cast("), $3), make1_str("as"), make2_str($5, make1_str(")")));
				}
		| '(' a_expr ')'
				{	$$ = make3_str(make1_str("("), $2, make1_str(")")); }
		| b_expr Op b_expr
				{	$$ = cat3_str($1, $2, $3);	}
		| Op b_expr
				{	$$ = cat2_str($1, $2); }
		| b_expr Op
				{	$$ = cat2_str($1, $2); }
		| func_name '(' ')'
				{
					$$ = cat2_str($1, make1_str("()")); 
				}
		| func_name '(' expr_list ')'
				{
					$$ = make4_str($1, make1_str("("), $3, make1_str(")")); 
				}
		| CURRENT_DATE
				{
					$$ = make1_str("current_date");
				}
		| CURRENT_TIME
				{
					$$ = make1_str("current_time");
				}
		| CURRENT_TIME '(' Iconst ')'
				{
					if ($3 != 0)
						fprintf(stderr,"CURRENT_TIME(%s) precision not implemented; zero used instead", $3);
					$$ = make1_str("current_time");
				}
		| CURRENT_TIMESTAMP
				{
					$$ = make1_str("current_timestamp");
				}
		| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					if (atol($3) != 0)
						fprintf(stderr,"CURRENT_TIMESTAMP(%s) precision not implemented; zero used instead",$3);
					$$ = make1_str("current_timestamp");
				}
		| CURRENT_USER
				{
					$$ = make1_str("current_user");
				}
		| USER
				{
					$$ = make1_str("user");
				}
		| POSITION '(' position_list ')'
				{
					$$ = make3_str(make1_str("position ("), $3, make1_str(")"));
				}
		| SUBSTRING '(' substr_list ')'
				{
					$$ = make3_str(make1_str("substring ("), $3, make1_str(")"));
				}
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
				{
					$$ = make3_str(make1_str("trim(both"), $4, make1_str(")"));
				}
		| TRIM '(' LEADING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(leading"), $4, make1_str(")"));
				}
		| TRIM '(' TRAILING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(trailing"), $4, make1_str(")"));
				}
		| TRIM '(' trim_list ')'
				{
					$$ = make3_str(make1_str("trim("), $3, make1_str(")"));
				}
		| civariableonly
			        { 	$$ = $1; }
		;

opt_indirection:  '[' ecpg_expr ']' opt_indirection
				{
					$$ = cat4_str(make1_str("["), $2, make1_str("]"), $4);
				}
		| '[' ecpg_expr ':' ecpg_expr ']' opt_indirection
				{
					$$ = cat2_str(cat5_str(make1_str("["), $2, make1_str(":"), $4, make1_str("]")), $6);
				}
		| /* EMPTY */
				{	$$ = make1_str(""); }
		;

expr_list:  a_expr_or_null
				{ $$ = $1; }
		| expr_list ',' a_expr_or_null
				{ $$ = cat3_str($1, make1_str(","), $3); }
		| expr_list USING a_expr
				{ $$ = cat3_str($1, make1_str("using"), $3); }
		;

extract_list:  extract_arg FROM a_expr
				{
					$$ = cat3_str($1, make1_str("from"), $3);
				}
		| /* EMPTY */
				{	$$ = make1_str(""); }
		| cinputvariable
			        { $$ = make1_str(";;"); }
		;

extract_arg:  datetime		{ $$ = $1; }
	| TIMEZONE_HOUR 	{ $$ = make1_str("timezone_hour"); }	
	| TIMEZONE_MINUTE 	{ $$ = make1_str("timezone_minute"); }	
		;

position_list:  position_expr IN position_expr
				{	$$ = cat3_str($1, make1_str("in"), $3); }
		| /* EMPTY */
				{	$$ = make1_str(""); }
		;

position_expr:  attr opt_indirection
				{
					$$ = cat2_str($1, $2);
				}
		| AexprConst
				{	$$ = $1;  }
		| '-' position_expr %prec UMINUS
				{	$$ = cat2_str(make1_str("-"), $2); }
		| position_expr '+' position_expr
				{	$$ = cat3_str($1, make1_str("+"), $3); }
		| position_expr '-' position_expr
				{	$$ = cat3_str($1, make1_str("-"), $3); }
		| position_expr '/' position_expr
				{	$$ = cat3_str($1, make1_str("/"), $3); }
		| position_expr '*' position_expr
				{	$$ = cat3_str($1, make1_str("*"), $3); }
		| '|' position_expr
				{	$$ = cat2_str(make1_str("|"), $2); }
		| position_expr TYPECAST Typename
				{
					$$ = cat3_str($1, make1_str("::"), $3);
				}
		| CAST '(' position_expr AS Typename ')'
				{
					$$ = cat3_str(make2_str(make1_str("cast("), $3), make1_str("as"), make2_str($5, make1_str(")")));
				}
		| '(' position_expr ')'
				{	$$ = make3_str(make1_str("("), $2, make1_str(")")); }
		| position_expr Op position_expr
				{	$$ = cat3_str($1, $2, $3); }
		| Op position_expr
				{	$$ = cat2_str($1, $2); }
		| position_expr Op
				{	$$ = cat2_str($1, $2); }
		| ColId
				{
					$$ = $1;
				}
		| func_name '(' ')'
				{
					$$ = cat2_str($1, make1_str("()"));
				}
		| func_name '(' expr_list ')'
				{
					$$ = make4_str($1, make1_str("("), $3, make1_str(")"));
				}
		| POSITION '(' position_list ')'
				{
					$$ = make3_str(make1_str("position("), $3, make1_str(")"));
				}
		| SUBSTRING '(' substr_list ')'
				{
					$$ = make3_str(make1_str("substring("), $3, make1_str(")"));
				}
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
				{
					$$ = make3_str(make1_str("trim(both"), $4, make1_str(")"));
				}
		| TRIM '(' LEADING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(leading"), $4, make1_str(")"));
				}
		| TRIM '(' TRAILING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(trailing"), $4, make1_str(")"));
				}
		| TRIM '(' trim_list ')'
				{
					$$ = make3_str(make1_str("trim("), $3, make1_str(")"));
				}
		;

substr_list:  expr_list substr_from substr_for
				{
					$$ = cat3_str($1, $2, $3);
				}
		| /* EMPTY */
				{	$$ = make1_str(""); }
		;

substr_from:  FROM expr_list
				{	$$ = cat2_str(make1_str("from"), $2); }
		| /* EMPTY */
				{
					$$ = make1_str("");
				}
		;

substr_for:  FOR expr_list
				{	$$ = cat2_str(make1_str("for"), $2); }
		| /* EMPTY */
				{	$$ = make1_str(""); }
		;

trim_list:  a_expr FROM expr_list
				{ $$ = cat3_str($1, make1_str("from"), $3); }
		| FROM expr_list
				{ $$ = cat2_str(make1_str("from"), $2); }
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

in_expr_nodes:  AexprConst
				{	$$ = $1; }
		| in_expr_nodes ',' AexprConst
				{	$$ = cat3_str($1, make1_str(","), $3);}
		;

not_in_expr:  SubSelect
				{
					$$ = $1; 
				}
		| not_in_expr_nodes
				{	$$ = $1; }
		;

not_in_expr_nodes:  AexprConst
				{	$$ = $1; }
		| not_in_expr_nodes ',' AexprConst
				{	$$ = cat3_str($1, make1_str(","), $3);}
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
                                { $$ = cat5_str(make1_str("case"), $2, $3, $4, make1_str("end")); }
                | NULLIF '(' a_expr ',' a_expr ')'
                                {
					$$ = cat5_str(make1_str("nullif("), $3, make1_str(","), $5, make1_str(")"));

					fprintf(stderr, "NULLIF() not yet fully implemented");
                                }
                | COALESCE '(' expr_list ')'
                                {
					$$ = cat3_str(make1_str("coalesce("), $3, make1_str(")"));
				}
		;

when_clause_list:  when_clause_list when_clause
                               { $$ = cat2_str($1, $2); }
               | when_clause
                               { $$ = $1; }
               ;

when_clause:  WHEN a_expr THEN a_expr_or_null
                               {
					$$ = cat4_str(make1_str("when"), $2, make1_str("then"), $4);
                               }
               ;

case_default:  ELSE a_expr_or_null	{ $$ = cat2_str(make1_str("else"), $2); }
               | /*EMPTY*/        	{ $$ = make1_str(""); }
               ;

case_arg:  attr opt_indirection
                               {
                                       $$ = cat2_str($1, $2);
                               }
               | ColId
                               {
                                       $$ = $1;
                               }
               | /*EMPTY*/
                               {       $$ = make1_str(""); }
               ;

attr:  relation_name '.' attrs
				{
					$$ = make3_str($1, make1_str("."), $3);
				}
		| ParamNo '.' attrs
				{
					$$ = make3_str($1, make1_str("."), $3);
				}
		;

attrs:	  attr_name
				{ $$ = $1; }
		| attrs '.' attr_name
				{ $$ = make3_str($1, make1_str("."), $3); }
		| attrs '.' '*'
				{ $$ = make2_str($1, make1_str(".*")); }
		;


/*****************************************************************************
 *
 *	target lists
 *
 *****************************************************************************/

res_target_list:  res_target_list ',' res_target_el
				{	$$ = cat3_str($1, make1_str(","),$3);  }
		| res_target_el
				{	$$ = $1;  }
		| '*'		{ $$ = make1_str("*"); }
		;

res_target_el:  ColId opt_indirection '=' a_expr_or_null
				{
					$$ = cat4_str($1, $2, make1_str("="), $4);
				}
		| attr opt_indirection
				{
					$$ = cat2_str($1, $2);
				}
		| relation_name '.' '*'
				{
					$$ = make2_str($1, make1_str(".*"));
				}
		;

/*
** target list for select.
** should get rid of the other but is still needed by the defunct select into
** and update (uses a subset)
*/
res_target_list2:  res_target_list2 ',' res_target_el2
				{	$$ = cat3_str($1, make1_str(","), $3);  }
		| res_target_el2
				{	$$ = $1;  }
		;

/* AS is not optional because shift/red conflict with unary ops */
res_target_el2:  a_expr_or_null AS ColLabel
				{
					$$ = cat3_str($1, make1_str("as"), $3);
				}
		| a_expr_or_null
				{
					$$ = $1;
				}
		| relation_name '.' '*'
				{
					$$ = make2_str($1, make1_str(".*"));
				}
		| '*'
				{
					$$ = make1_str("*");
				}
		;

opt_id:  ColId									{ $$ = $1; }
		| /* EMPTY */							{ $$ = make1_str(""); }
		;

relation_name:	SpecialRuleRelation
				{
					$$ = $1;
				}
		| ColId
				{
					/* disallow refs to variable system tables */
					if (strcmp(LogRelationName, $1) == 0
					   || strcmp(VariableRelationName, $1) == 0) {
						sprintf(errortext, make1_str("%s cannot be accessed by users"),$1);
						yyerror(errortext);
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
recipe_name:			ident			{ $$ = $1; };

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
		| Typename Sconst
				{
					$$ = cat2_str($1, $2);
				}
		| ParamNo
				{	$$ = $1;  }
		| TRUE_P
				{
					$$ = make1_str("true");
				}
		| FALSE_P
				{
					$$ = make1_str("false");
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
		| ABSOLUTE			{ $$ = make1_str("absolute"); }
		| ACTION			{ $$ = make1_str("action"); }
		| AFTER				{ $$ = make1_str("after"); }
		| AGGREGATE			{ $$ = make1_str("aggregate"); }
		| BACKWARD			{ $$ = make1_str("backward"); }
		| BEFORE			{ $$ = make1_str("before"); }
		| CACHE				{ $$ = make1_str("cache"); }
		| CREATEDB			{ $$ = make1_str("createdb"); }
		| CREATEUSER			{ $$ = make1_str("createuser"); }
		| CYCLE				{ $$ = make1_str("cycle"); }
		| DATABASE			{ $$ = make1_str("database"); }
		| DELIMITERS			{ $$ = make1_str("delimiters"); }
		| DOUBLE			{ $$ = make1_str("double"); }
		| EACH				{ $$ = make1_str("each"); }
		| ENCODING			{ $$ = make1_str("encoding"); }
		| FORWARD			{ $$ = make1_str("forward"); }
		| FUNCTION			{ $$ = make1_str("function"); }
		| HANDLER			{ $$ = make1_str("handler"); }
		| INCREMENT			{ $$ = make1_str("increment"); }
		| INDEX				{ $$ = make1_str("index"); }
		| INHERITS			{ $$ = make1_str("inherits"); }
		| INSENSITIVE			{ $$ = make1_str("insensitive"); }
		| INSTEAD			{ $$ = make1_str("instead"); }
		| ISNULL			{ $$ = make1_str("isnull"); }
		| KEY				{ $$ = make1_str("key"); }
		| LANGUAGE			{ $$ = make1_str("language"); }
		| LANCOMPILER			{ $$ = make1_str("lancompiler"); }
		| LOCATION			{ $$ = make1_str("location"); }
		| MATCH				{ $$ = make1_str("match"); }
		| MAXVALUE			{ $$ = make1_str("maxvalue"); }
		| MINVALUE			{ $$ = make1_str("minvalue"); }
		| NEXT				{ $$ = make1_str("next"); }
		| NOCREATEDB			{ $$ = make1_str("nocreatedb"); }
		| NOCREATEUSER			{ $$ = make1_str("nocreateuser"); }
		| NOTHING			{ $$ = make1_str("nothing"); }
		| NOTNULL			{ $$ = make1_str("notnull"); }
		| OF				{ $$ = make1_str("of"); }
		| OIDS				{ $$ = make1_str("oids"); }
		| ONLY				{ $$ = make1_str("only"); }
		| OPERATOR			{ $$ = make1_str("operator"); }
		| OPTION			{ $$ = make1_str("option"); }
		| PASSWORD			{ $$ = make1_str("password"); }
		| PRIOR				{ $$ = make1_str("prior"); }
		| PRIVILEGES			{ $$ = make1_str("privileges"); }
		| PROCEDURAL			{ $$ = make1_str("procedural"); }
		| READ				{ $$ = make1_str("read"); }
		| RECIPE			{ $$ = make1_str("recipe"); }
		| RELATIVE			{ $$ = make1_str("relative"); }
		| RENAME			{ $$ = make1_str("rename"); }
		| RETURNS			{ $$ = make1_str("returns"); }
		| ROW				{ $$ = make1_str("row"); }
		| RULE				{ $$ = make1_str("rule"); }
		| SCROLL			{ $$ = make1_str("scroll"); }
		| SEQUENCE                      { $$ = make1_str("sequence"); }
		| SERIAL			{ $$ = make1_str("serial"); }
		| START				{ $$ = make1_str("start"); }
		| STATEMENT			{ $$ = make1_str("statement"); }
		| STDIN                         { $$ = make1_str("stdin"); }
		| STDOUT                        { $$ = make1_str("stdout"); }
		| TIME				{ $$ = make1_str("time"); }
		| TIMESTAMP			{ $$ = make1_str("timestamp"); }
		| TIMEZONE_HOUR                 { $$ = make1_str("timezone_hour"); }
                | TIMEZONE_MINUTE               { $$ = make1_str("timezone_minute"); }
		| TRIGGER			{ $$ = make1_str("trigger"); }
		| TRUSTED			{ $$ = make1_str("trusted"); }
		| TYPE_P			{ $$ = make1_str("type"); }
		| VALID				{ $$ = make1_str("valid"); }
		| VERSION			{ $$ = make1_str("version"); }
		| ZONE				{ $$ = make1_str("zone"); }
		| SQL_AT			{ $$ = make1_str("at"); }
		| SQL_BOOL			{ $$ = make1_str("bool"); }
		| SQL_BREAK			{ $$ = make1_str("break"); }
		| SQL_CALL			{ $$ = make1_str("call"); }
		| SQL_CONNECT			{ $$ = make1_str("connect"); }
		| SQL_CONNECTION		{ $$ = make1_str("connection"); }
		| SQL_CONTINUE			{ $$ = make1_str("continue"); }
		| SQL_DEALLOCATE		{ $$ = make1_str("deallocate"); }
		| SQL_DISCONNECT		{ $$ = make1_str("disconnect"); }
		| SQL_FOUND			{ $$ = make1_str("found"); }
		| SQL_GO			{ $$ = make1_str("go"); }
		| SQL_GOTO			{ $$ = make1_str("goto"); }
		| SQL_IDENTIFIED		{ $$ = make1_str("identified"); }
		| SQL_IMMEDIATE			{ $$ = make1_str("immediate"); }
		| SQL_INDICATOR			{ $$ = make1_str("indicator"); }
		| SQL_INT			{ $$ = make1_str("int"); }
		| SQL_LONG			{ $$ = make1_str("long"); }
		| SQL_OPEN			{ $$ = make1_str("open"); }
		| SQL_PREPARE			{ $$ = make1_str("prepare"); }
		| SQL_RELEASE			{ $$ = make1_str("release"); }
		| SQL_SECTION			{ $$ = make1_str("section"); }
		| SQL_SHORT			{ $$ = make1_str("short"); }
		| SQL_SIGNED			{ $$ = make1_str("signed"); }
		| SQL_SQLERROR			{ $$ = make1_str("sqlerror"); }
		| SQL_SQLPRINT			{ $$ = make1_str("sqlprint"); }
		| SQL_SQLWARNING		{ $$ = make1_str("sqlwarning"); }
		| SQL_STOP			{ $$ = make1_str("stop"); }
		| SQL_STRUCT			{ $$ = make1_str("struct"); }
		| SQL_UNSIGNED			{ $$ = make1_str("unsigned"); }
		| SQL_VAR			{ $$ = make1_str("var"); }
		| SQL_WHENEVER			{ $$ = make1_str("whenever"); }
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
		| ABORT_TRANS                                   { $$ = make1_str("abort"); }
		| ANALYZE                                       { $$ = make1_str("analyze"); }
		| BINARY                                        { $$ = make1_str("binary"); }
		| CASE                                        { $$ = make1_str("case"); }
		| CLUSTER						{ $$ = make1_str("cluster"); }
		| COALESCE                                        { $$ = make1_str("coalesce"); }
		| CONSTRAINT					{ $$ = make1_str("constraint"); }
		| COPY							{ $$ = make1_str("copy"); }
		| CURRENT							{ $$ = make1_str("current"); }
		| DO							{ $$ = make1_str("do"); }
		| ELSE                                        { $$ = make1_str("else"); }
		| END_TRANS                                        { $$ = make1_str("end"); }
		| EXPLAIN							{ $$ = make1_str("explain"); }
		| EXTEND							{ $$ = make1_str("extend"); }
		| FALSE_P							{ $$ = make1_str("false"); }
		| FOREIGN						{ $$ = make1_str("foreign"); }
		| GROUP							{ $$ = make1_str("group"); }
		| LISTEN							{ $$ = make1_str("listen"); }
		| LOAD							{ $$ = make1_str("load"); }
		| LOCK_P							{ $$ = make1_str("lock"); }
		| MOVE							{ $$ = make1_str("move"); }
		| NEW							{ $$ = make1_str("new"); }
		| NONE							{ $$ = make1_str("none"); }
		| NULLIF                                        { $$ = make1_str("nullif"); }
		| ORDER							{ $$ = make1_str("order"); }
		| POSITION						{ $$ = make1_str("position"); }
		| PRECISION						{ $$ = make1_str("precision"); }
		| RESET							{ $$ = make1_str("reset"); }
		| SETOF							{ $$ = make1_str("setof"); }
		| SHOW							{ $$ = make1_str("show"); }
		| TABLE							{ $$ = make1_str("table"); }
		| THEN                                        { $$ = make1_str("then"); }
		| TRANSACTION					{ $$ = make1_str("transaction"); }
		| TRUE_P						{ $$ = make1_str("true"); }
		| VACUUM					{ $$ = make1_str("vacuum"); }
		| VERBOSE						{ $$ = make1_str("verbose"); }
		| WHEN                                        { $$ = make1_str("when"); }
		;

SpecialRuleRelation:  CURRENT
				{
					if (QueryIsRule)
						$$ = make1_str("current");
					else
						yyerror("CURRENT used in non-rule query");
				}
		| NEW
				{
					if (QueryIsRule)
						$$ = make1_str("new");
					else
						yyerror("NEW used in non-rule query");
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
			$$ = make5_str($3, make1_str(","), $5, make1_str(","), $4);
                }
	| SQL_CONNECT TO DEFAULT
        	{
                	$$ = make1_str("NULL,NULL,NULL,\"DEFAULT\"");
                }
      /* also allow ORACLE syntax */
        | SQL_CONNECT ora_user
                {
		       $$ = make3_str(make1_str("NULL,"), $2, make1_str(",NULL"));
		}

connection_target: database_name opt_server opt_port
                {
		  /* old style: dbname[@server][:port] */
		  if (strlen($2) > 0 && *($2) != '@')
		  {
		    sprintf(errortext, "parse error at or near '%s'", $2);
		    yyerror(errortext);
		  }

		  $$ = make5_str(make1_str("\""), $1, $2, $3, make1_str("\""));
		}
        |  db_prefix server opt_port '/' database_name opt_options
                {
		  /* new style: <tcp|unix>:postgresql://server[:port][/dbname] */
                  if (strncmp($2, "://", 3) != 0)
		  {
		    sprintf(errortext, "parse error at or near '%s'", $2);
		    yyerror(errortext);
		  }

		  if (strncmp($1, "unix", 4) == 0 && strncmp($2 + 3, "localhost", 9) != 0)
		  {
		    sprintf(errortext, "unix domain sockets only work on 'localhost' but not on '%9.9s'", $2);
                    yyerror(errortext);
		  }

		  if (strncmp($1, "unix", 4) != 0 && strncmp($1, "tcp", 3) != 0)
		  {
		    sprintf(errortext, "only protocols 'tcp' and 'unix' are supported");
                    yyerror(errortext);
		  }
	
		  $$ = make4_str(make5_str(make1_str("\""), $1, $2, $3, make1_str("/")), $5, $6, make1_str("\""));
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
		    yyerror(errortext);	
		  }

		  if (strcmp($1, "tcp") != 0 && strcmp($1, "unix") != 0)
		  {
		    sprintf(errortext, "Illegal connection type %s", $1);
		    yyerror(errortext);
		  }

		  $$ = make3_str($1, make1_str(":"), $2);
		}
        
server: Op server_name
                {
		  if (strcmp($1, "@") != 0 && strcmp($1, "://") != 0)
		  {
		    sprintf(errortext, "parse error at or near '%s'", $1);
		    yyerror(errortext);
		  }

		  $$ = make2_str($1, $2);
	        }

opt_server: server { $$ = $1; }
        | /* empty */ { $$ = make1_str(""); }

server_name: ColId   { $$ = $1; }
        | ColId '.' server_name { $$ = make3_str($1, make1_str("."), $3); }

opt_port: ':' Iconst { $$ = make2_str(make1_str(":"), $2); }
        | /* empty */ { $$ = make1_str(""); }

opt_connection_name: AS connection_target { $$ = $2; }
        | /* empty */ { $$ = make1_str("NULL"); }

opt_user: USER ora_user { $$ = $2; }
          | /* empty */ { $$ = make1_str("NULL,NULL"); }

ora_user: user_name
		{
                        $$ = make2_str($1, make1_str(",NULL"));
	        }
	| user_name '/' ColId
		{
        		$$ = make3_str($1, make1_str(","), $3);
                }
        | user_name SQL_IDENTIFIED BY user_name
                {
        		$$ = make3_str($1, make1_str(","), $4);
                }
        | user_name USING user_name
                {
        		$$ = make3_str($1, make1_str(","), $3);
                }

user_name: UserId       { if ($1[0] == '\"')
				$$ = $1;
			  else
				$$ = make3_str(make1_str("\""), $1, make1_str("\""));
			}
        | char_variable { $$ = $1; }
        | SCONST        { $$ = make3_str(make1_str("\""), $1, make1_str("\"")); }

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
                                $$ = make2_str($1, make1_str(".arr"));
                                break;
                            default:
                                yyerror("invalid datatype");
                                break;
                        }
		}

opt_options: Op ColId
		{
			if (strlen($1) == 0)
				yyerror("parse error");
				
			if (strcmp($1, "?") != 0)
			{
				sprintf(errortext, "parse error at or near %s", $1);
				yyerror(errortext);
			}
			
			$$ = make2_str(make1_str("?"), $2);
		}
	| /* empty */ { $$ = make1_str(""); }

/*
 * Declare a prepared cursor. The syntax is different from the standard
 * declare statement, so we create a new rule.
 */
ECPGCursorStmt:  DECLARE name opt_cursor CURSOR FOR ident cursor_clause
				{
					struct cursor *ptr, *this;
					struct variable *thisquery = (struct variable *)mm_alloc(sizeof(struct variable));

					for (ptr = cur; ptr != NULL; ptr = ptr->next)
					{
						if (strcmp($2, ptr->name) == 0)
						{
						        /* re-definition is a bug */
							sprintf(errortext, "cursor %s already defined", $2);
							yyerror(errortext);
				                }
        				}

        				this = (struct cursor *) mm_alloc(sizeof(struct cursor));

			        	/* initial definition */
				        this->next = cur;
				        this->name = $2;
					this->connection = connection;
				        this->command =  cat5_str(make1_str("declare"), mm_strdup($2), $3, make1_str("cursor for ;;"), $7);
					this->argsresult = NULL;

					thisquery->type = &ecpg_query;
					thisquery->brace_level = 0;
					thisquery->next = NULL;
					thisquery->name = (char *) mm_alloc(sizeof("ECPGprepared_statement(\"\")") + strlen($6));
					sprintf(thisquery->name, "ECPGprepared_statement(\"%s\")", $6);

					this->argsinsert = NULL;
					add_variable(&(this->argsinsert), thisquery, &no_indicator); 

			        	cur = this;
					
					$$ = cat3_str(make1_str("/*"), mm_strdup(this->command), make1_str("*/"));
				}
		;

/*
 * the exec sql deallocate prepare command to deallocate a previously
 * prepared statement
 */
ECPGDeallocate:	SQL_DEALLOCATE SQL_PREPARE ident	{ $$ = make3_str(make1_str("ECPGdeallocate(__LINE__, \""), $3, make1_str("\");")); }

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

sql_startdeclare : ecpgstart BEGIN_TRANS DECLARE SQL_SECTION SQL_SEMI {}

sql_enddeclare: ecpgstart END_TRANS DECLARE SQL_SECTION SQL_SEMI {}

variable_declarations: /* empty */
	{
		$$ = make1_str("");
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
 		$$ = cat4_str($1, $3.type_str, $5, make1_str(";\n"));
	}

storage_clause : S_EXTERN	{ $$ = make1_str("extern"); }
       | S_STATIC		{ $$ = make1_str("static"); }
       | S_SIGNED		{ $$ = make1_str("signed"); }
       | S_CONST		{ $$ = make1_str("const"); }
       | S_REGISTER		{ $$ = make1_str("register"); }
       | S_AUTO			{ $$ = make1_str("auto"); }
       | /* empty */		{ $$ = make1_str(""); }

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
			$$.type_str = make1_str("");
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

			$$.type_str = mm_strdup(this->name);
                        $$.type_enum = this->type->type_enum;
			$$.type_dimension = this->type->type_dimension;
  			$$.type_index = this->type->type_index;
			struct_member_list[struct_level] = ECPGstruct_member_dup(this->struct_member_list);
		}

enum_type: s_enum '{' c_line '}'
	{
		$$ = cat4_str($1, make1_str("{"), $3, make1_str("}"));
	}
	
s_enum: S_ENUM opt_symbol	{ $$ = cat2_str(make1_str("enum"), $2); }

struct_type: s_struct '{' variable_declarations '}'
	{
	    ECPGfree_struct_member(struct_member_list[struct_level]);
	    free(actual_storage[struct_level--]);
	    $$ = cat4_str($1, make1_str("{"), $3, make1_str("}"));
	}

union_type: s_union '{' variable_declarations '}'
	{
	    ECPGfree_struct_member(struct_member_list[struct_level]);
	    free(actual_storage[struct_level--]);
	    $$ = cat4_str($1, make1_str("{"), $3, make1_str("}"));
	}

s_struct : S_STRUCT opt_symbol
        {
            struct_member_list[struct_level++] = NULL;
            if (struct_level >= STRUCT_DEPTH)
                 yyerror("Too many levels in nested structure definition");
	    $$ = cat2_str(make1_str("struct"), $2);
	}

s_union : S_UNION opt_symbol
        {
            struct_member_list[struct_level++] = NULL;
            if (struct_level >= STRUCT_DEPTH)
                 yyerror("Too many levels in nested structure definition");
	    $$ = cat2_str(make1_str("union"), $2);
	}

opt_symbol: /* empty */ 	{ $$ = make1_str(""); }
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
		$$ = cat3_str($1, make1_str(","), $3);
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

                               $$ = make4_str($1, mm_strdup($2), $3.str, $4);
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
			       sprintf(ascii_len, "%d", length);

                               if (length > 0)
                                   $$ = make4_str(make5_str(mm_strdup(actual_storage[struct_level]), make1_str(" struct varchar_"), mm_strdup($2), make1_str(" { int len; char arr["), mm_strdup(ascii_len)), make1_str("]; } "), mm_strdup($2), mm_strdup(dim));
                               else
				   yyerror ("pointer to varchar are not implemented yet");
/*                                   $$ = make4_str(make3_str(mm_strdup(actual_storage[struct_level]), make1_str(" struct varchar_"), mm_strdup($2)), make1_str(" { int len; char *arr; }"), mm_strdup($2), mm_strdup(dim));*/
                               break;
                           case ECPGt_char:
                           case ECPGt_unsigned_char:
                               if (dimension == -1)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, length);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, length), dimension);

			       $$ = make4_str($1, mm_strdup($2), $3.str, $4);
                               break;
                           default:
                               if (dimension < 0)
                                   type = ECPGmake_simple_type(actual_type[struct_level].type_enum, 1);
                               else
                                   type = ECPGmake_array_type(ECPGmake_simple_type(actual_type[struct_level].type_enum, 1), dimension);

			       $$ = make4_str($1, mm_strdup($2), $3.str, $4);
                               break;
			}

			if (struct_level == 0)
				new_variable($2, type);
			else
				ECPGmake_struct_member($2, type, &(struct_member_list[struct_level - 1]));

			free($2);
		}

opt_initializer: /* empty */		{ $$ = make1_str(""); }
	| '=' vartext			{ $$ = make2_str(make1_str("="), $2); }

opt_pointer: /* empty */	{ $$ = make1_str(""); }
	| '*'			{ $$ = make1_str("*"); }

/*
 * As long as the prepare statement is not supported by the backend, we will
 * try to simulate it here so we get dynamic SQL 
 */
ECPGDeclare: DECLARE STATEMENT ident
	{
		/* this is only supported for compatibility */
		$$ = cat3_str(make1_str("/* declare statement"), $3, make1_str("*/"));
	}
/*
 * the exec sql disconnect statement: disconnect from the given database 
 */
ECPGDisconnect: SQL_DISCONNECT dis_name { $$ = $2; }

dis_name: connection_object	{ $$ = $1; }
	| CURRENT	{ $$ = make1_str("CURRENT"); }
	| ALL		{ $$ = make1_str("ALL"); }
	| /* empty */	{ $$ = make1_str("CURRENT"); }

connection_object: connection_target { $$ = $1; }
	| DEFAULT	{ $$ = make1_str("DEFAULT"); }

/*
 * execute a given string as sql command
 */
ECPGExecute : EXECUTE SQL_IMMEDIATE execstring
	{ 
		struct variable *thisquery = (struct variable *)mm_alloc(sizeof(struct variable));

		thisquery->type = &ecpg_query;
		thisquery->brace_level = 0;
		thisquery->next = NULL;
		thisquery->name = $3;

		add_variable(&argsinsert, thisquery, &no_indicator); 

		$$ = make1_str(";;");
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
	} opt_using
	{
		$$ = make1_str(";;");
	}

execstring: char_variable |
	CSTRING	 { $$ = make3_str(make1_str("\""), $1, make1_str("\"")); };

/*
 * the exec sql free command to deallocate a previously
 * prepared statement
 */
ECPGFree:	SQL_FREE ident	{ $$ = $2; }

/*
 * open is an open cursor, at the moment this has to be removed
 */
ECPGOpen: SQL_OPEN name opt_using {
		$$ = $2;
};

opt_using: /* empty */		{ $$ = make1_str(""); }
	| USING variablelist	{
					/* yyerror ("open cursor with variables not implemented yet"); */
					$$ = make1_str("");
				}

variablelist: cinputvariable | cinputvariable ',' variablelist

/*
 * As long as the prepare statement is not supported by the backend, we will
 * try to simulate it here so we get dynamic SQL 
 */
ECPGPrepare: SQL_PREPARE ident FROM char_variable
	{
		$$ = make4_str(make1_str("\""), $2, make1_str("\", "), $4);
	}

/*
 * for compatibility with ORACLE we will also allow the keyword RELEASE
 * after a transaction statement to disconnect from the database.
 */

ECPGRelease: TransactionStmt SQL_RELEASE
	{
		if (strncmp($1, "begin", 5) == 0)
                        yyerror("RELEASE does not make sense when beginning a transaction");

		fprintf(yyout, "ECPGtrans(__LINE__, %s, \"%s\");", connection, $1);
		whenever_action(0);
		fprintf(yyout, "ECPGdisconnect(\"\");"); 
		whenever_action(0);
		free($1);
	}

/* 
 * set the actual connection, this needs a differnet handling as the other
 * set commands
 */
ECPGSetConnection:  SET SQL_CONNECTION connection_object
           		{
				$$ = $3;
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
				yyerror(errortext);
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
                            yyerror("No multi-dimensional array support for simple data types");

        	types = this;

		$$ = cat5_str(cat3_str(make1_str("/* exec sql type"), mm_strdup($2), make1_str("is")), mm_strdup($4.type_str), mm_strdup($5.str), $6, make1_str("*/"));
	}

opt_type_array_bounds:  '[' ']' nest_type_array_bounds
			{
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make1_str("[]"), $3.str);
                        }
		| '(' ')' nest_type_array_bounds
			{
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make1_str("[]"), $3.str);
                        }
		| '[' Iconst ']' nest_type_array_bounds
			{
                            $$.index1 = atol($2);
                            $$.index2 = $4.index1;
                            $$.str = cat4_str(make1_str("["), $2, make1_str("]"), $4.str);
                        }
		| '(' Iconst ')' nest_type_array_bounds
			{
                            $$.index1 = atol($2);
                            $$.index2 = $4.index1;
                            $$.str = cat4_str(make1_str("["), $2, make1_str("]"), $4.str);
                        }
		| /* EMPTY */
			{
                            $$.index1 = -1;
                            $$.index2 = -1;
                            $$.str= make1_str("");
                        }
		;

nest_type_array_bounds:	'[' ']' nest_type_array_bounds
                        {
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make1_str("[]"), $3.str);
                        }
		| '(' ')' nest_type_array_bounds
                        {
                            $$.index1 = 0;
                            $$.index2 = $3.index1;
                            $$.str = cat2_str(make1_str("[]"), $3.str);
                        }
		| '[' Iconst ']' nest_type_array_bounds
			{
                            $$.index1 = atol($2);
                            $$.index2 = $4.index1;
                            $$.str = cat4_str(make1_str("["), $2, make1_str("]"), $4.str);
                        }
		| '(' Iconst ')' nest_type_array_bounds
			{
                            $$.index1 = atol($2);
                            $$.index2 = $4.index1;
                            $$.str = cat4_str(make1_str("["), $2, make1_str("]"), $4.str);
                        }
		| /* EMPTY */
			{
                            $$.index1 = -1;
                            $$.index2 = -1;
                            $$.str= make1_str("");
                        }
                ;
opt_reference: SQL_REFERENCE { $$ = make1_str("reference"); }
	| /* empty */ 	     { $$ = make1_str(""); }

ctype: CHAR
	{
		$$.type_str = make1_str("char");
                $$.type_enum = ECPGt_char;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| VARCHAR
	{
		$$.type_str = make1_str("varchar");
                $$.type_enum = ECPGt_varchar;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| FLOAT
	{
		$$.type_str = make1_str("float");
                $$.type_enum = ECPGt_float;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| DOUBLE
	{
		$$.type_str = make1_str("double");
                $$.type_enum = ECPGt_double;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| opt_signed SQL_INT
	{
		$$.type_str = make1_str("int");
       	        $$.type_enum = ECPGt_int;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_ENUM
	{
		$$.type_str = make1_str("int");
       	        $$.type_enum = ECPGt_int;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| opt_signed SQL_SHORT
	{
		$$.type_str = make1_str("short");
       	        $$.type_enum = ECPGt_short;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| opt_signed SQL_LONG
	{
		$$.type_str = make1_str("long");
       	        $$.type_enum = ECPGt_long;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_BOOL
	{
		$$.type_str = make1_str("bool");
       	        $$.type_enum = ECPGt_bool;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_UNSIGNED SQL_INT
	{
		$$.type_str = make1_str("unsigned int");
       	        $$.type_enum = ECPGt_unsigned_int;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_UNSIGNED SQL_SHORT
	{
		$$.type_str = make1_str("unsigned short");
       	        $$.type_enum = ECPGt_unsigned_short;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_UNSIGNED SQL_LONG
	{
		$$.type_str = make1_str("unsigned long");
       	        $$.type_enum = ECPGt_unsigned_long;
		$$.type_index = -1;
		$$.type_dimension = -1;
	}
	| SQL_STRUCT
	{
		struct_member_list[struct_level++] = NULL;
		if (struct_level >= STRUCT_DEPTH)
        		yyerror("Too many levels in nested structure definition");
	} '{' sql_variable_declarations '}'
	{
		ECPGfree_struct_member(struct_member_list[struct_level--]);
		$$.type_str = cat3_str(make1_str("struct {"), $4, make1_str("}"));
		$$.type_enum = ECPGt_struct;
                $$.type_index = -1;
                $$.type_dimension = -1;
	}
	| UNION
	{
		struct_member_list[struct_level++] = NULL;
		if (struct_level >= STRUCT_DEPTH)
        		yyerror("Too many levels in nested structure definition");
	} '{' sql_variable_declarations '}'
	{
		ECPGfree_struct_member(struct_member_list[struct_level--]);
		$$.type_str = cat3_str(make1_str("union {"), $4, make1_str("}"));
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
		$$ = make1_str("");
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
	sql_variable_list SQL_SEMI
	{
		$$ = cat3_str($1.type_str, $3, make1_str(";"));
	}

sql_variable_list: sql_variable 
	{
		$$ = $1;
	}
	| sql_variable_list ',' sql_variable
	{
		$$ = make3_str($1, make1_str(","), $3);
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
                	            yyerror("No multi-dimensional array support for simple data types");

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

			$$ = cat3_str($1, $2, $3.str);
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
                	    yyerror("No multi-dimensional array support for simple data types");

                        if (dimension < 0)
                            type = ECPGmake_simple_type($4.type_enum, 1);
                        else
                            type = ECPGmake_array_type(ECPGmake_simple_type($4.type_enum, 1), dimension);

			break;
		}	

		ECPGfree_type(p->type);
		p->type = type;

		$$ = cat5_str(cat3_str(make1_str("/* exec sql var"), mm_strdup($2), make1_str("is")), mm_strdup($4.type_str), mm_strdup($5.str), $6, make1_str("*/"));
	}

/*
 * whenever statement: decide what to do in case of error/no data found
 * according to SQL standards we lack: SQLSTATE, CONSTRAINT and SQLEXCEPTION
 */
ECPGWhenever: SQL_WHENEVER SQL_SQLERROR action {
	when_error.code = $<action>3.code;
	when_error.command = $<action>3.command;
	$$ = cat3_str(make1_str("/* exec sql whenever sqlerror "), $3.str, make1_str("; */\n"));
}
	| SQL_WHENEVER NOT SQL_FOUND action {
	when_nf.code = $<action>4.code;
	when_nf.command = $<action>4.command;
	$$ = cat3_str(make1_str("/* exec sql whenever not found "), $4.str, make1_str("; */\n"));
}
	| SQL_WHENEVER SQL_SQLWARNING action {
	when_warn.code = $<action>3.code;
	when_warn.command = $<action>3.command;
	$$ = cat3_str(make1_str("/* exec sql whenever sql_warning "), $3.str, make1_str("; */\n"));
}

action : SQL_CONTINUE {
	$<action>$.code = W_NOTHING;
	$<action>$.command = NULL;
	$<action>$.str = make1_str("continue");
}
       | SQL_SQLPRINT {
	$<action>$.code = W_SQLPRINT;
	$<action>$.command = NULL;
	$<action>$.str = make1_str("sqlprint");
}
       | SQL_STOP {
	$<action>$.code = W_STOP;
	$<action>$.command = NULL;
	$<action>$.str = make1_str("stop");
}
       | SQL_GOTO name {
        $<action>$.code = W_GOTO;
        $<action>$.command = strdup($2);
	$<action>$.str = cat2_str(make1_str("goto "), $2);
}
       | SQL_GO TO name {
        $<action>$.code = W_GOTO;
        $<action>$.command = strdup($3);
	$<action>$.str = cat2_str(make1_str("goto "), $3);
}
       | DO name '(' dotext ')' {
	$<action>$.code = W_DO;
	$<action>$.command = make4_str($2, make1_str("("), $4, make1_str(")"));
	$<action>$.str = cat2_str(make1_str("do"), mm_strdup($<action>$.command));
}
       | DO SQL_BREAK {
        $<action>$.code = W_BREAK;
        $<action>$.command = NULL;
        $<action>$.str = make1_str("break");
}
       | SQL_CALL name '(' dotext ')' {
	$<action>$.code = W_DO;
	$<action>$.command = make4_str($2, make1_str("("), $4, make1_str(")"));
	$<action>$.str = cat2_str(make1_str("call"), mm_strdup($<action>$.command));
}

/* some other stuff for ecpg */
ecpg_expr:  attr opt_indirection
				{
					$$ = cat2_str($1, $2);
				}
		| row_expr
				{	$$ = $1;  }
		| AexprConst
				{	$$ = $1;  }
		| ColId
				{
					$$ = $1;
				}
		| '-' ecpg_expr %prec UMINUS
				{	$$ = cat2_str(make1_str("-"), $2); }
		| a_expr '+' ecpg_expr
				{	$$ = cat3_str($1, make1_str("+"), $3); }
		| a_expr '-' ecpg_expr
				{	$$ = cat3_str($1, make1_str("-"), $3); }
		| a_expr '/' ecpg_expr
				{	$$ = cat3_str($1, make1_str("/"), $3); }
		| a_expr '*' ecpg_expr
				{	$$ = cat3_str($1, make1_str("*"), $3); }
		| a_expr '<' ecpg_expr
				{	$$ = cat3_str($1, make1_str("<"), $3); }
		| a_expr '>' ecpg_expr
				{	$$ = cat3_str($1, make1_str(">"), $3); }
		| a_expr '=' ecpg_expr
				{	$$ = cat3_str($1, make1_str("="), $3); }
	/*	| ':' ecpg_expr
				{	$$ = cat2_str(make1_str(":"), $2); }*/
		| ';' ecpg_expr
				{	$$ = cat2_str(make1_str(";"), $2); }
		| '|' ecpg_expr
				{	$$ = cat2_str(make1_str("|"), $2); }
		| a_expr TYPECAST Typename
				{
					$$ = cat3_str($1, make1_str("::"), $3);
				}
		| CAST '(' a_expr AS Typename ')'
				{
					$$ = cat3_str(make2_str(make1_str("cast("), $3), make1_str("as"), make2_str($5, make1_str(")")));
				}
		| '(' a_expr_or_null ')'
				{	$$ = make3_str(make1_str("("), $2, make1_str(")")); }
		| a_expr Op ecpg_expr
				{	$$ = cat3_str($1, $2, $3);	}
		| a_expr LIKE ecpg_expr
				{	$$ = cat3_str($1, make1_str("like"), $3); }
		| a_expr NOT LIKE ecpg_expr
				{	$$ = cat3_str($1, make1_str("not like"), $4); }
		| Op ecpg_expr
				{	$$ = cat2_str($1, $2); }
		| a_expr Op
				{	$$ = cat2_str($1, $2); }
		| func_name '(' '*' ')'
				{
					$$ = cat2_str($1, make1_str("(*)")); 
				}
		| func_name '(' ')'
				{
					$$ = cat2_str($1, make1_str("()")); 
				}
		| func_name '(' expr_list ')'
				{
					$$ = make4_str($1, make1_str("("), $3, make1_str(")")); 
				}
		| CURRENT_DATE
				{
					$$ = make1_str("current_date");
				}
		| CURRENT_TIME
				{
					$$ = make1_str("current_time");
				}
		| CURRENT_TIME '(' Iconst ')'
				{
					if (atol($3) != 0)
						fprintf(stderr,"CURRENT_TIME(%s) precision not implemented; zero used instead", $3);
					$$ = make1_str("current_time");
				}
		| CURRENT_TIMESTAMP
				{
					$$ = make1_str("current_timestamp");
				}
		| CURRENT_TIMESTAMP '(' Iconst ')'
				{
					if (atol($3) != 0)
						fprintf(stderr,"CURRENT_TIMESTAMP(%s) precision not implemented; zero used instead",$3);
					$$ = make1_str("current_timestamp");
				}
		| CURRENT_USER
				{
					$$ = make1_str("current_user");
				}
		| EXISTS '(' SubSelect ')'
				{
					$$ = make3_str(make1_str("exists("), $3, make1_str(")"));
				}
		| EXTRACT '(' extract_list ')'
				{
					$$ = make3_str(make1_str("extract("), $3, make1_str(")"));
				}
		| POSITION '(' position_list ')'
				{
					$$ = make3_str(make1_str("position("), $3, make1_str(")"));
				}
		| SUBSTRING '(' substr_list ')'
				{
					$$ = make3_str(make1_str("substring("), $3, make1_str(")"));
				}
		/* various trim expressions are defined in SQL92 - thomas 1997-07-19 */
		| TRIM '(' BOTH trim_list ')'
				{
					$$ = make3_str(make1_str("trim(both"), $4, make1_str(")"));
				}
		| TRIM '(' LEADING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(leading"), $4, make1_str(")"));
				}
		| TRIM '(' TRAILING trim_list ')'
				{
					$$ = make3_str(make1_str("trim(trailing"), $4, make1_str(")"));
				}
		| TRIM '(' trim_list ')'
				{
					$$ = make3_str(make1_str("trim("), $3, make1_str(")"));
				}
		| a_expr ISNULL
				{	$$ = cat2_str($1, make1_str("isnull")); }
		| a_expr IS NULL_P
				{	$$ = cat2_str($1, make1_str("is null")); }
		| a_expr NOTNULL
				{	$$ = cat2_str($1, make1_str("notnull")); }
		| a_expr IS NOT NULL_P
				{	$$ = cat2_str($1, make1_str("is not null")); }
		/* IS TRUE, IS FALSE, etc used to be function calls
		 *  but let's make them expressions to allow the optimizer
		 *  a chance to eliminate them if a_expr is a constant string.
		 * - thomas 1997-12-22
		 */
		| a_expr IS TRUE_P
				{
				{	$$ = cat2_str($1, make1_str("is true")); }
				}
		| a_expr IS NOT FALSE_P
				{
				{	$$ = cat2_str($1, make1_str("is not false")); }
				}
		| a_expr IS FALSE_P
				{
				{	$$ = cat2_str($1, make1_str("is false")); }
				}
		| a_expr IS NOT TRUE_P
				{
				{	$$ = cat2_str($1, make1_str("is not true")); }
				}
		| a_expr BETWEEN b_expr AND b_expr
				{
					$$ = cat5_str($1, make1_str("between"), $3, make1_str("and"), $5); 
				}
		| a_expr NOT BETWEEN b_expr AND b_expr
				{
					$$ = cat5_str($1, make1_str("not between"), $4, make1_str("and"), $6); 
				}
		| a_expr IN '(' in_expr ')'
				{
					$$ = make4_str($1, make1_str("in ("), $4, make1_str(")")); 
				}
		| a_expr NOT IN '(' not_in_expr ')'
				{
					$$ = make4_str($1, make1_str("not in ("), $5, make1_str(")")); 
				}
		| a_expr Op '(' SubSelect ')'
				{
					$$ = cat3_str($1, $2, make3_str(make1_str("("), $4, make1_str(")"))); 
				}
		| a_expr '+' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("+("), $4, make1_str(")")); 
				}
		| a_expr '-' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("-("), $4, make1_str(")")); 
				}
		| a_expr '/' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("/("), $4, make1_str(")")); 
				}
		| a_expr '*' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("*("), $4, make1_str(")")); 
				}
		| a_expr '<' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("<("), $4, make1_str(")")); 
				}
		| a_expr '>' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str(">("), $4, make1_str(")")); 
				}
		| a_expr '=' '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("=("), $4, make1_str(")")); 
				}
		| a_expr Op ANY '(' SubSelect ')'
				{
					$$ = cat3_str($1, $2, make3_str(make1_str("any ("), $5, make1_str(")"))); 
				}
		| a_expr '+' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("+any("), $5, make1_str(")")); 
				}
		| a_expr '-' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("-any("), $5, make1_str(")")); 
				}
		| a_expr '/' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("/any("), $5, make1_str(")")); 
				}
		| a_expr '*' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("*any("), $5, make1_str(")")); 
				}
		| a_expr '<' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("<any("), $5, make1_str(")")); 
				}
		| a_expr '>' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str(">any("), $5, make1_str(")")); 
				}
		| a_expr '=' ANY '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("=any("), $5, make1_str(")")); 
				}
		| a_expr Op ALL '(' SubSelect ')'
				{
					$$ = make3_str($1, $2, make3_str(make1_str("all ("), $5, make1_str(")"))); 
				}
		| a_expr '+' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("+all("), $5, make1_str(")")); 
				}
		| a_expr '-' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("-all("), $5, make1_str(")")); 
				}
		| a_expr '/' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("/all("), $5, make1_str(")")); 
				}
		| a_expr '*' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("*all("), $5, make1_str(")")); 
				}
		| a_expr '<' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("<all("), $5, make1_str(")")); 
				}
		| a_expr '>' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str(">all("), $5, make1_str(")")); 
				}
		| a_expr '=' ALL '(' SubSelect ')'
				{
					$$ = make4_str($1, make1_str("=all("), $5, make1_str(")")); 
				}
		| a_expr AND ecpg_expr
				{	$$ = cat3_str($1, make1_str("and"), $3); }
		| a_expr OR ecpg_expr
				{	$$ = cat3_str($1, make1_str("or"), $3); }
		| NOT ecpg_expr
				{	$$ = cat2_str(make1_str("not"), $2); }
		| civariableonly
			        { 	$$ = $1; }
		;

into_list : coutputvariable | into_list ',' coutputvariable;

ecpgstart: SQL_START { reset_variables();}

dotext: /* empty */		{ $$ = make1_str(""); }
	| dotext do_anything	{ $$ = make2_str($1, $2); }

vartext: var_anything		{ $$ = $1; }
        | vartext var_anything { $$ = make2_str($1, $2); }

coutputvariable : cvariable indicator {
		add_variable(&argsresult, find_variable($1), ($2 == NULL) ? &no_indicator : find_variable($2)); 
}

cinputvariable : cvariable indicator {
		add_variable(&argsinsert, find_variable($1), ($2 == NULL) ? &no_indicator : find_variable($2)); 
}

civariableonly : cvariable {
		add_variable(&argsinsert, find_variable($1), &no_indicator); 
		$$ = make1_str(";;");
}

cvariable: CVARIABLE			{ $$ = $1; }

indicator: /* empty */			{ $$ = NULL; }
	| cvariable		 	{ check_indicator((find_variable($1))->type); $$ = $1; }
	| SQL_INDICATOR cvariable 	{ check_indicator((find_variable($2))->type); $$ = $2; }
	| SQL_INDICATOR name		{ check_indicator((find_variable($2))->type); $$ = $2; }

ident: IDENT	{ $$ = $1; }
	| CSTRING	{ $$ = $1; }
/*
 * C stuff
 */

symbol: IDENT	{ $$ = $1; }

cpp_line: CPP_LINE	{ $$ = $1; }

c_line: c_anything { $$ = $1; }
	| c_line c_anything
		{
			$$ = make2_str($1, $2);
		}

c_thing: c_anything | ';' { $$ = make1_str(";"); }

c_anything:  IDENT 	{ $$ = $1; }
	| CSTRING	{ $$ = make3_str(make1_str("\""), $1, make1_str("\"")); }
	| Iconst	{ $$ = $1; }
	| Fconst	{ $$ = $1; }
	| '*'		{ $$ = make1_str("*"); }
	| S_AUTO	{ $$ = make1_str("auto"); }
	| S_BOOL	{ $$ = make1_str("bool"); }
	| S_CHAR	{ $$ = make1_str("char"); }
	| S_CONST	{ $$ = make1_str("const"); }
	| S_DOUBLE	{ $$ = make1_str("double"); }
	| S_ENUM	{ $$ = make1_str("enum"); }
	| S_EXTERN	{ $$ = make1_str("extern"); }
	| S_FLOAT	{ $$ = make1_str("float"); }
        | S_INT		{ $$ = make1_str("int"); }
	| S_LONG	{ $$ = make1_str("long"); }
	| S_REGISTER	{ $$ = make1_str("register"); }
	| S_SHORT	{ $$ = make1_str("short"); }
	| S_SIGNED	{ $$ = make1_str("signed"); }
	| S_STATIC	{ $$ = make1_str("static"); }
        | S_STRUCT	{ $$ = make1_str("struct"); }
        | S_UNION	{ $$ = make1_str("union"); }
	| S_UNSIGNED	{ $$ = make1_str("unsigned"); }
	| S_VARCHAR	{ $$ = make1_str("varchar"); }
	| S_ANYTHING	{ $$ = make_name(); }
        | '['		{ $$ = make1_str("["); }
	| ']'		{ $$ = make1_str("]"); }
	| '('		{ $$ = make1_str("("); }
	| ')'		{ $$ = make1_str(")"); }
	| '='		{ $$ = make1_str("="); }
	| ','		{ $$ = make1_str(","); }

do_anything: IDENT	{ $$ = $1; }
        | CSTRING       { $$ = make3_str(make1_str("\""), $1, make1_str("\""));}
        | Iconst        { $$ = $1; }
	| Fconst	{ $$ = $1; }
	| ','		{ $$ = make1_str(","); }

var_anything: IDENT 		{ $$ = $1; }
	| CSTRING       	{ $$ = make3_str(make1_str("\""), $1, make1_str("\"")); }
	| Iconst		{ $$ = $1; }
	| Fconst		{ $$ = $1; }
	| '{' c_line '}'	{ $$ = make3_str(make1_str("{"), $2, make1_str("}")); }

blockstart : '{' {
    braces_open++;
    $$ = make1_str("{");
}

blockend : '}' {
    remove_variables(braces_open--);
    $$ = make1_str("}");
}

%%

void yyerror(char * error)
{
    fprintf(stderr, "%s:%d: %s\n", input_filename, yylineno, error);
    exit(PARSE_ERROR);
}
