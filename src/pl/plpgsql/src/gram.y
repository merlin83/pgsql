%{
/**********************************************************************
 * gram.y				- Parser for the PL/pgSQL
 *						  procedural language
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/pl/plpgsql/src/gram.y,v 1.64 2004/10/25 06:27:21 neilc Exp $
 *
 *	  This software is copyrighted by Jan Wieck - Hamburg.
 *
 *	  The author hereby grants permission  to  use,  copy,	modify,
 *	  distribute,  and	license this software and its documentation
 *	  for any purpose, provided that existing copyright notices are
 *	  retained	in	all  copies  and  that	this notice is included
 *	  verbatim in any distributions. No written agreement, license,
 *	  or  royalty  fee	is required for any of the authorized uses.
 *	  Modifications to this software may be  copyrighted  by  their
 *	  author  and  need  not  follow  the licensing terms described
 *	  here, provided that the new terms are  clearly  indicated  on
 *	  the first page of each file where they apply.
 *
 *	  IN NO EVENT SHALL THE AUTHOR OR DISTRIBUTORS BE LIABLE TO ANY
 *	  PARTY  FOR  DIRECT,	INDIRECT,	SPECIAL,   INCIDENTAL,	 OR
 *	  CONSEQUENTIAL   DAMAGES  ARISING	OUT  OF  THE  USE  OF  THIS
 *	  SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF, EVEN
 *	  IF  THE  AUTHOR  HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 *	  DAMAGE.
 *
 *	  THE  AUTHOR  AND	DISTRIBUTORS  SPECIFICALLY	 DISCLAIM	ANY
 *	  WARRANTIES,  INCLUDING,  BUT	NOT  LIMITED  TO,  THE	IMPLIED
 *	  WARRANTIES  OF  MERCHANTABILITY,	FITNESS  FOR  A  PARTICULAR
 *	  PURPOSE,	AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON
 *	  AN "AS IS" BASIS, AND THE AUTHOR	AND  DISTRIBUTORS  HAVE  NO
 *	  OBLIGATION   TO	PROVIDE   MAINTENANCE,	 SUPPORT,  UPDATES,
 *	  ENHANCEMENTS, OR MODIFICATIONS.
 *
 **********************************************************************/

#include "plpgsql.h"


static	PLpgSQL_expr	*read_sql_construct(int until,
											int until2,
											const char *expected,
											bool isexpression,
											const char *sqlstart,
											int *endtoken);
static	PLpgSQL_expr	*read_sql_stmt(const char *sqlstart);
static	PLpgSQL_type	*read_datatype(int tok);
static	PLpgSQL_stmt	*make_select_stmt(void);
static	PLpgSQL_stmt	*make_fetch_stmt(void);
static	void check_assignable(PLpgSQL_datum *datum);

%}

%union {
		int32					ival;
		char					*str;
		struct
		{
			char *name;
			int  lineno;
		}						varname;
		struct
		{
			int  nalloc;
			int  nused;
			int	 *nums;
		}						intlist;
		struct
		{
			int  nalloc;
			int  nused;
			PLpgSQL_diag_item *dtitems;
		}						dtlist;
		struct
		{
			char *name;
			int  lineno;
			PLpgSQL_rec	*rec;
			PLpgSQL_row	*row;
		}						forvariable;
		struct
		{
			char *label;
			int  n_initvars;
			int  *initvarnos;
		}						declhdr;
		PLpgSQL_type			*dtype;
		PLpgSQL_datum			*scalar;	/* a VAR, RECFIELD, or TRIGARG */
		PLpgSQL_variable		*variable;	/* a VAR, REC, or ROW */
		PLpgSQL_var				*var;
		PLpgSQL_row				*row;
		PLpgSQL_rec				*rec;
		PLpgSQL_expr			*expr;
		PLpgSQL_stmt			*stmt;
		PLpgSQL_stmts			*stmts;
		PLpgSQL_stmt_block		*program;
		PLpgSQL_condition		*condition;
		PLpgSQL_exception		*exception;
		PLpgSQL_exceptions		*exceptions;
		PLpgSQL_nsitem			*nsitem;
}

%type <declhdr> decl_sect
%type <varname> decl_varname
%type <str>		decl_renname
%type <ival>	decl_const decl_notnull
%type <expr>	decl_defval decl_cursor_query
%type <dtype>	decl_datatype
%type <row>		decl_cursor_args decl_cursor_arglist
%type <nsitem>	decl_aliasitem
%type <str>		decl_stmts decl_stmt

%type <expr>	expr_until_semi expr_until_rightbracket
%type <expr>	expr_until_then expr_until_loop
%type <expr>	opt_exitcond

%type <ival>	assign_var cursor_variable
%type <var>		cursor_varptr
%type <variable>	decl_cursor_arg
%type <forvariable>	for_variable
%type <stmt>	for_control

%type <str>		opt_lblname opt_label
%type <str>		opt_exitlabel
%type <str>		execsql_start

%type <stmts>	proc_sect proc_stmts stmt_else loop_body
%type <stmt>	proc_stmt pl_block
%type <stmt>	stmt_assign stmt_if stmt_loop stmt_while stmt_exit
%type <stmt>	stmt_return stmt_return_next stmt_raise stmt_execsql
%type <stmt>	stmt_for stmt_select stmt_perform
%type <stmt>	stmt_dynexecute stmt_getdiag
%type <stmt>	stmt_open stmt_fetch stmt_close stmt_null

%type <exceptions>	exception_sect proc_exceptions
%type <exception>	proc_exception
%type <condition>	proc_conditions

%type <intlist>	raise_params
%type <ival>	raise_level raise_param
%type <str>		raise_msg

%type <dtlist>	getdiag_list
%type <ival>	getdiag_item getdiag_target

%type <ival>	lno

		/*
		 * Keyword tokens
		 */
%token	K_ALIAS
%token	K_ASSIGN
%token	K_BEGIN
%token	K_CLOSE
%token	K_CONSTANT
%token	K_CURSOR
%token	K_DEBUG
%token	K_DECLARE
%token	K_DEFAULT
%token	K_DIAGNOSTICS
%token	K_DOTDOT
%token	K_ELSE
%token	K_ELSIF
%token	K_END
%token	K_EXCEPTION
%token	K_EXECUTE
%token	K_EXIT
%token	K_FOR
%token	K_FETCH
%token	K_FROM
%token	K_GET
%token	K_IF
%token	K_IN
%token	K_INFO
%token	K_INTO
%token	K_IS
%token	K_LOG
%token	K_LOOP
%token	K_NEXT
%token	K_NOT
%token	K_NOTICE
%token	K_NULL
%token	K_OPEN
%token	K_OR
%token	K_PERFORM
%token	K_ROW_COUNT
%token	K_RAISE
%token	K_RENAME
%token	K_RESULT_OID
%token	K_RETURN
%token	K_RETURN_NEXT
%token	K_REVERSE
%token	K_SELECT
%token	K_THEN
%token	K_TO
%token	K_TYPE
%token	K_WARNING
%token	K_WHEN
%token	K_WHILE

		/*
		 * Other tokens
		 */
%token	T_FUNCTION
%token	T_TRIGGER
%token	T_STRING
%token	T_NUMBER
%token	T_SCALAR				/* a VAR, RECFIELD, or TRIGARG */
%token	T_ROW
%token	T_RECORD
%token	T_DTYPE
%token	T_LABEL
%token	T_WORD
%token	T_ERROR

%token	O_OPTION
%token	O_DUMP

%%

pl_function		: T_FUNCTION comp_optsect pl_block opt_semi
					{
						yylval.program = (PLpgSQL_stmt_block *)$3;
					}
				| T_TRIGGER comp_optsect pl_block opt_semi
					{
						yylval.program = (PLpgSQL_stmt_block *)$3;
					}
				;

comp_optsect	:
				| comp_options
				;

comp_options	: comp_options comp_option
				| comp_option
				;

comp_option		: O_OPTION O_DUMP
					{
						plpgsql_DumpExecTree = 1;
					}
				;

opt_semi		:
				| ';'
				;

pl_block		: decl_sect K_BEGIN lno proc_sect exception_sect K_END
					{
						PLpgSQL_stmt_block *new;

						new = malloc(sizeof(PLpgSQL_stmt_block));
						memset(new, 0, sizeof(PLpgSQL_stmt_block));

						new->cmd_type	= PLPGSQL_STMT_BLOCK;
						new->lineno		= $3;
						new->label		= $1.label;
						new->n_initvars = $1.n_initvars;
						new->initvarnos = $1.initvarnos;
						new->body		= $4;
						new->exceptions	= $5;

						plpgsql_ns_pop();

						$$ = (PLpgSQL_stmt *)new;
					}
				;


decl_sect		: opt_label
					{
						plpgsql_ns_setlocal(false);
						$$.label	  = $1;
						$$.n_initvars = 0;
						$$.initvarnos = NULL;
						plpgsql_add_initdatums(NULL);
					}
				| opt_label decl_start
					{
						plpgsql_ns_setlocal(false);
						$$.label	  = $1;
						$$.n_initvars = 0;
						$$.initvarnos = NULL;
						plpgsql_add_initdatums(NULL);
					}
				| opt_label decl_start decl_stmts
					{
						plpgsql_ns_setlocal(false);
						if ($3 != NULL)
							$$.label = $3;
						else
							$$.label = $1;
						$$.n_initvars = plpgsql_add_initdatums(&($$.initvarnos));
					}
				;

decl_start		: K_DECLARE
					{
						plpgsql_ns_setlocal(true);
					}
				;

decl_stmts		: decl_stmts decl_stmt
					{	$$ = $2;	}
				| decl_stmt
					{	$$ = $1;	}
				;

decl_stmt		: '<' '<' opt_lblname '>' '>'
					{	$$ = $3;	}
				| K_DECLARE
					{	$$ = NULL;	}
				| decl_statement
					{	$$ = NULL;	}
				;

decl_statement	: decl_varname decl_const decl_datatype decl_notnull decl_defval
					{
						PLpgSQL_variable	*var;

						var = plpgsql_build_variable($1.name, $1.lineno,
													 $3, true);
						if ($2)
						{
							if (var->dtype == PLPGSQL_DTYPE_VAR)
								((PLpgSQL_var *) var)->isconst = $2;
							else
								ereport(ERROR,
										(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
										 errmsg("row or record variable cannot be CONSTANT")));
						}
						if ($4)
						{
							if (var->dtype == PLPGSQL_DTYPE_VAR)
								((PLpgSQL_var *) var)->notnull = $4;
							else
								ereport(ERROR,
										(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
										 errmsg("row or record variable cannot be NOT NULL")));
						}
						if ($5 != NULL)
						{
							if (var->dtype == PLPGSQL_DTYPE_VAR)
								((PLpgSQL_var *) var)->default_val = $5;
							else
								ereport(ERROR,
										(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
										 errmsg("default value for row or record variable is not supported")));
						}
					}
				| decl_varname K_ALIAS K_FOR decl_aliasitem ';'
					{
						plpgsql_ns_additem($4->itemtype,
										   $4->itemno, $1.name);
					}
				| K_RENAME decl_renname K_TO decl_renname ';'
					{
						plpgsql_ns_rename($2, $4);
					}
				| decl_varname K_CURSOR
					{ plpgsql_ns_push(NULL); }
				  decl_cursor_args decl_is_from decl_cursor_query
					{
						PLpgSQL_var *new;
						PLpgSQL_expr *curname_def;
						char		buf[1024];
						char		*cp1;
						char		*cp2;

						/* pop local namespace for cursor args */
						plpgsql_ns_pop();

						new = (PLpgSQL_var *)
							plpgsql_build_variable($1.name, $1.lineno,
												   plpgsql_build_datatype(REFCURSOROID,
																		  -1),
												   true);

						curname_def = malloc(sizeof(PLpgSQL_expr));
						memset(curname_def, 0, sizeof(PLpgSQL_expr));

						curname_def->dtype = PLPGSQL_DTYPE_EXPR;
						strcpy(buf, "SELECT '");
						cp1 = new->refname;
						cp2 = buf + strlen(buf);
						while (*cp1 != '\0')
						{
							if (*cp1 == '\\' || *cp1 == '\'')
								*cp2++ = '\\';
							*cp2++ = *cp1++;
						}
						strcpy(cp2, "'::refcursor");
						curname_def->query = strdup(buf);
						new->default_val = curname_def;

						new->cursor_explicit_expr = $6;
						if ($4 == NULL)
							new->cursor_explicit_argrow = -1;
						else
							new->cursor_explicit_argrow = $4->rowno;
					}
				;

decl_cursor_query :
					{
						PLpgSQL_expr *query;

						plpgsql_ns_setlocal(false);
						query = read_sql_stmt("");
						plpgsql_ns_setlocal(true);
						
						$$ = query;
					}
				;

decl_cursor_args :
					{
						$$ = NULL;
					}
				| '(' decl_cursor_arglist ')'
					{
						/* Copy the temp arrays to malloc'd storage */
						int nfields = $2->nfields;
						char **ftmp;
						int *vtmp;

						ftmp = malloc(nfields * sizeof(char *));
						vtmp = malloc(nfields * sizeof(int));
						memcpy(ftmp, $2->fieldnames, nfields * sizeof(char *));
						memcpy(vtmp, $2->varnos, nfields * sizeof(int));

						pfree($2->fieldnames);
						pfree($2->varnos);

						$2->fieldnames = ftmp;
						$2->varnos = vtmp;

						plpgsql_adddatum((PLpgSQL_datum *)$2);

						$$ = $2;
					}
				;

decl_cursor_arglist : decl_cursor_arg
					{
						PLpgSQL_row *new;

						new = malloc(sizeof(PLpgSQL_row));
						memset(new, 0, sizeof(PLpgSQL_row));

						new->dtype = PLPGSQL_DTYPE_ROW;
						new->refname = strdup("*internal*");
						new->lineno = plpgsql_scanner_lineno();
						new->rowtupdesc = NULL;
						/*
						 * We make temporary fieldnames/varnos arrays that
						 * are much bigger than necessary.  We will resize
						 * them to just the needed size in the
						 * decl_cursor_args production.
						 */
						new->fieldnames = palloc(1024 * sizeof(char *));
						new->varnos = palloc(1024 * sizeof(int));
						new->nfields = 1;

						new->fieldnames[0] = $1->refname;
						new->varnos[0] = $1->dno;

						$$ = new;
					}
				| decl_cursor_arglist ',' decl_cursor_arg
					{
						int i = $1->nfields++;

						$1->fieldnames[i] = $3->refname;
						$1->varnos[i] = $3->dno;

						$$ = $1;
					}
				;

decl_cursor_arg : decl_varname decl_datatype
					{
						$$ = plpgsql_build_variable($1.name, $1.lineno,
													$2, true);
					}
				;

decl_is_from	:	K_IS |		/* Oracle */
					K_FOR;		/* ANSI */

decl_aliasitem	: T_WORD
					{
						char	*name;
						PLpgSQL_nsitem *nsi;

						plpgsql_convert_ident(yytext, &name, 1);
						if (name[0] != '$')
							yyerror("only positional parameters may be aliased");

						plpgsql_ns_setlocal(false);
						nsi = plpgsql_ns_lookup(name, NULL);
						if (nsi == NULL)
						{
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_UNDEFINED_PARAMETER),
									 errmsg("function has no parameter \"%s\"",
											name)));
						}

						plpgsql_ns_setlocal(true);

						pfree(name);

						$$ = nsi;
					}
				;

decl_varname	: T_WORD
					{
						char	*name;

						plpgsql_convert_ident(yytext, &name, 1);
						/* name should be malloc'd for use as varname */
						$$.name = strdup(name);
						$$.lineno  = plpgsql_scanner_lineno();
						pfree(name);
					}
				;

decl_renname	: T_WORD
					{
						char	*name;

						plpgsql_convert_ident(yytext, &name, 1);
						/* the result must be palloc'd, see plpgsql_ns_rename */
						$$ = name;
					}
				;

decl_const		:
					{ $$ = 0; }
				| K_CONSTANT
					{ $$ = 1; }
				;

decl_datatype	:
					{
						/*
						 * If there's a lookahead token, read_datatype
						 * should consume it.
						 */
						$$ = read_datatype(yychar);
						yyclearin;
					}
				;

decl_notnull	:
					{ $$ = 0; }
				| K_NOT K_NULL
					{ $$ = 1; }
				;

decl_defval		: ';'
					{ $$ = NULL; }
				| decl_defkey
					{
						plpgsql_ns_setlocal(false);
						$$ = plpgsql_read_expression(';', ";");
						plpgsql_ns_setlocal(true);
					}
				;

decl_defkey		: K_ASSIGN
				| K_DEFAULT
				;

proc_sect		:
					{
							PLpgSQL_stmts	*new;

							new = malloc(sizeof(PLpgSQL_stmts));
							memset(new, 0, sizeof(PLpgSQL_stmts));
							$$ = new;
					}
				| proc_stmts
					{ $$ = $1; }
				;

proc_stmts		: proc_stmts proc_stmt
						{
							if ($2 != NULL)
							{
								if ($1->stmts_used == $1->stmts_alloc)
								{
									$1->stmts_alloc *= 2;
									$1->stmts = realloc($1->stmts, sizeof(PLpgSQL_stmt *) * $1->stmts_alloc);
								}
								$1->stmts[$1->stmts_used++] = $2;
							}
							$$ = $1;
						}
				| proc_stmt
						{
							PLpgSQL_stmts	*new;

							new = malloc(sizeof(PLpgSQL_stmts));
							memset(new, 0, sizeof(PLpgSQL_stmts));

							new->stmts_alloc = 32;
							new->stmts = malloc(sizeof(PLpgSQL_stmt *) * new->stmts_alloc);

							if ($1 != NULL)
								new->stmts[new->stmts_used++] = $1;

							$$ = new;
						}
				;

proc_stmt		: pl_block ';'
						{ $$ = $1; }
				| stmt_assign
						{ $$ = $1; }
				| stmt_if
						{ $$ = $1; }
				| stmt_loop
						{ $$ = $1; }
				| stmt_while
						{ $$ = $1; }
				| stmt_for
						{ $$ = $1; }
				| stmt_select
						{ $$ = $1; }
				| stmt_exit
						{ $$ = $1; }
				| stmt_return
						{ $$ = $1; }
				| stmt_return_next
						{ $$ = $1; }
				| stmt_raise
						{ $$ = $1; }
				| stmt_execsql
						{ $$ = $1; }
				| stmt_dynexecute
						{ $$ = $1; }
				| stmt_perform
						{ $$ = $1; }
				| stmt_getdiag
						{ $$ = $1; }
				| stmt_open
						{ $$ = $1; }
				| stmt_fetch
						{ $$ = $1; }
				| stmt_close
						{ $$ = $1; }
				| stmt_null
						{ $$ = $1; }
				;

stmt_perform	: K_PERFORM lno expr_until_semi
					{
						PLpgSQL_stmt_perform *new;

						new = malloc(sizeof(PLpgSQL_stmt_perform));
						memset(new, 0, sizeof(PLpgSQL_stmt_perform));

						new->cmd_type = PLPGSQL_STMT_PERFORM;
						new->lineno   = $2;
						new->expr  = $3;

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_assign		: assign_var lno K_ASSIGN expr_until_semi
					{
						PLpgSQL_stmt_assign *new;

						new = malloc(sizeof(PLpgSQL_stmt_assign));
						memset(new, 0, sizeof(PLpgSQL_stmt_assign));

						new->cmd_type = PLPGSQL_STMT_ASSIGN;
						new->lineno   = $2;
						new->varno = $1;
						new->expr  = $4;

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_getdiag	: K_GET K_DIAGNOSTICS lno getdiag_list ';'
					{
						PLpgSQL_stmt_getdiag	 *new;

						new = malloc(sizeof(PLpgSQL_stmt_getdiag));
						memset(new, 0, sizeof(PLpgSQL_stmt_getdiag));

						new->cmd_type = PLPGSQL_STMT_GETDIAG;
						new->lineno   = $3;
						new->ndtitems = $4.nused;
						new->dtitems  = malloc(sizeof(PLpgSQL_diag_item) * $4.nused);
						memcpy(new->dtitems, $4.dtitems, sizeof(PLpgSQL_diag_item) * $4.nused);

						$$ = (PLpgSQL_stmt *)new;
					}
				;

getdiag_list : getdiag_list ',' getdiag_target K_ASSIGN getdiag_item
					{
						if ($1.nused == $1.nalloc)
						{
							$1.nalloc *= 2;
							$1.dtitems = repalloc($1.dtitems, sizeof(PLpgSQL_diag_item) * $1.nalloc);
						}
						$1.dtitems[$1.nused].target = $3;
						$1.dtitems[$1.nused].item   = $5;
						$1.nused++;

						$$.nalloc = $1.nalloc;
						$$.nused  = $1.nused;
						$$.dtitems = $1.dtitems;
					}
				| getdiag_target K_ASSIGN getdiag_item
					{
						$$.nalloc = 1;
						$$.nused  = 1;
						$$.dtitems = palloc(sizeof(PLpgSQL_diag_item) * $$.nalloc);
						$$.dtitems[0].target = $1;
						$$.dtitems[0].item   = $3;
					}
				;

getdiag_item : K_ROW_COUNT
					{
						$$ = PLPGSQL_GETDIAG_ROW_COUNT;
					}
				| K_RESULT_OID
					{
						$$ = PLPGSQL_GETDIAG_RESULT_OID;
					}
				;

getdiag_target	: T_SCALAR
					{
						check_assignable(yylval.scalar);
						$$ = yylval.scalar->dno;
					}
				;


assign_var		: T_SCALAR
					{
						check_assignable(yylval.scalar);
						$$ = yylval.scalar->dno;
					}
				| T_ROW
					{
						check_assignable((PLpgSQL_datum *) yylval.row);
						$$ = yylval.row->rowno;
					}
				| T_RECORD
					{
						check_assignable((PLpgSQL_datum *) yylval.rec);
						$$ = yylval.rec->recno;
					}
				| assign_var '[' expr_until_rightbracket
					{
						PLpgSQL_arrayelem	*new;

						new = malloc(sizeof(PLpgSQL_arrayelem));
						memset(new, 0, sizeof(PLpgSQL_arrayelem));

						new->dtype		= PLPGSQL_DTYPE_ARRAYELEM;
						new->subscript	= $3;
						new->arrayparentno = $1;

						plpgsql_adddatum((PLpgSQL_datum *)new);

						$$ = new->dno;
					}
				;

stmt_if			: K_IF lno expr_until_then proc_sect stmt_else K_END K_IF ';'
					{
						PLpgSQL_stmt_if *new;

						new = malloc(sizeof(PLpgSQL_stmt_if));
						memset(new, 0, sizeof(PLpgSQL_stmt_if));

						new->cmd_type	= PLPGSQL_STMT_IF;
						new->lineno		= $2;
						new->cond		= $3;
						new->true_body	= $4;
						new->false_body = $5;

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_else		:
					{
							PLpgSQL_stmts	*new;

							new = malloc(sizeof(PLpgSQL_stmts));
							memset(new, 0, sizeof(PLpgSQL_stmts));
							$$ = new;
					}
				| K_ELSIF lno expr_until_then proc_sect stmt_else
					{
					  /*
					   * Translate the structure:	   into:
					   *
					   * IF c1 THEN					   IF c1 THEN		 
					   *	 ...						   ...				 
					   * ELSIF c2 THEN				   ELSE 
					   *								   IF c2 THEN	
					   *	 ...							   ...				 
					   * ELSE							   ELSE				 
					   *	 ...							   ...				 
					   * END IF							   END IF			 
					   *							   END IF
					   * 
					   */

						PLpgSQL_stmts	*new;
						PLpgSQL_stmt_if *new_if;

						/* first create a new if-statement */
						new_if = malloc(sizeof(PLpgSQL_stmt_if));
						memset(new_if, 0, sizeof(PLpgSQL_stmt_if));

						new_if->cmd_type	= PLPGSQL_STMT_IF;
						new_if->lineno		= $2;
						new_if->cond		= $3;
						new_if->true_body	= $4;
						new_if->false_body	= $5;
						
						/* this is a 'container' for the if-statement */
						new = malloc(sizeof(PLpgSQL_stmts));
						memset(new, 0, sizeof(PLpgSQL_stmts));
						
						new->stmts_alloc = 64;
						new->stmts_used	 = 1;
						new->stmts = malloc(sizeof(PLpgSQL_stmt *) * new->stmts_alloc);
						new->stmts[0] = (PLpgSQL_stmt *) new_if;

						$$ = new;
						
					}

				| K_ELSE proc_sect
					{
						$$ = $2;				
					}
				;

stmt_loop		: opt_label K_LOOP lno loop_body
					{
						PLpgSQL_stmt_loop *new;

						new = malloc(sizeof(PLpgSQL_stmt_loop));
						memset(new, 0, sizeof(PLpgSQL_stmt_loop));

						new->cmd_type = PLPGSQL_STMT_LOOP;
						new->lineno   = $3;
						new->label	  = $1;
						new->body	  = $4;

						plpgsql_ns_pop();

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_while		: opt_label K_WHILE lno expr_until_loop loop_body
					{
						PLpgSQL_stmt_while *new;

						new = malloc(sizeof(PLpgSQL_stmt_while));
						memset(new, 0, sizeof(PLpgSQL_stmt_while));

						new->cmd_type = PLPGSQL_STMT_WHILE;
						new->lineno   = $3;
						new->label	  = $1;
						new->cond	  = $4;
						new->body	  = $5;

						plpgsql_ns_pop();

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_for		: opt_label K_FOR for_control loop_body
					{
						/* This runs after we've scanned the loop body */
						if ($3->cmd_type == PLPGSQL_STMT_FORI)
						{
							PLpgSQL_stmt_fori		*new;

							new = (PLpgSQL_stmt_fori *) $3;
							new->label	  = $1;
							new->body	  = $4;
							$$ = (PLpgSQL_stmt *) new;
						}
						else if ($3->cmd_type == PLPGSQL_STMT_FORS)
						{
							PLpgSQL_stmt_fors		*new;

							new = (PLpgSQL_stmt_fors *) $3;
							new->label	  = $1;
							new->body	  = $4;
							$$ = (PLpgSQL_stmt *) new;
						}
						else
						{
							PLpgSQL_stmt_dynfors	*new;

							Assert($3->cmd_type == PLPGSQL_STMT_DYNFORS);
							new = (PLpgSQL_stmt_dynfors *) $3;
							new->label	  = $1;
							new->body	  = $4;
							$$ = (PLpgSQL_stmt *) new;
						}

						/* close namespace started in opt_label */
						plpgsql_ns_pop();
					}
				;

for_control		: lno for_variable K_IN
					{
						int			tok;
						bool		reverse = false;
						bool		execute = false;
						PLpgSQL_expr *expr1;

						/* check for REVERSE and EXECUTE */
						tok = yylex();
						if (tok == K_REVERSE)
						{
							reverse = true;
							tok = yylex();
						}

						if (tok == K_EXECUTE)
							execute = true;
						else
							plpgsql_push_back_token(tok);

						/* Collect one or two expressions */
						expr1 = read_sql_construct(K_DOTDOT,
												   K_LOOP,
												   "LOOP",
												   true,
												   "SELECT ",
												   &tok);

						if (tok == K_DOTDOT)
						{
							/* Found .., so it must be an integer loop */
							PLpgSQL_stmt_fori	*new;
							PLpgSQL_expr		*expr2;
							PLpgSQL_var			*fvar;

							expr2 = plpgsql_read_expression(K_LOOP, "LOOP");

							if (execute)
							{
								plpgsql_error_lineno = $1;
								yyerror("cannot specify EXECUTE in integer for-loop");
							}

							/* name should be malloc'd for use as varname */
							fvar = (PLpgSQL_var *)
								plpgsql_build_variable(strdup($2.name),
													   $2.lineno,
													   plpgsql_build_datatype(INT4OID,
																			  -1),
													   true);

							/* put the for-variable into the local block */
							plpgsql_add_initdatums(NULL);

							new = malloc(sizeof(PLpgSQL_stmt_fori));
							memset(new, 0, sizeof(PLpgSQL_stmt_fori));

							new->cmd_type = PLPGSQL_STMT_FORI;
							new->lineno   = $1;
							new->var	  = fvar;
							new->reverse  = reverse;
							new->lower	  = expr1;
							new->upper	  = expr2;

							$$ = (PLpgSQL_stmt *) new;
						}
						else if (execute)
						{
							/* No .., so it must be a loop over rows */
							PLpgSQL_stmt_dynfors	*new;

							if (reverse)
							{
								plpgsql_error_lineno = $1;
								yyerror("cannot specify REVERSE in loop over rows");
							}

							new = malloc(sizeof(PLpgSQL_stmt_dynfors));
							memset(new, 0, sizeof(PLpgSQL_stmt_dynfors));

							new->cmd_type = PLPGSQL_STMT_DYNFORS;
							new->lineno   = $1;
							if ($2.rec)
								new->rec = $2.rec;
							else if ($2.row)
								new->row = $2.row;
							else
							{
								plpgsql_error_lineno = $1;
								yyerror("loop variable of loop over rows must be a record or row variable");
							}
							new->query = expr1;

							$$ = (PLpgSQL_stmt *) new;
						}
						else
						{
							/* No .., so it must be a loop over rows */
							PLpgSQL_stmt_fors		*new;
							char					*newquery;

							if (reverse)
							{
								plpgsql_error_lineno = $1;
								yyerror("cannot specify REVERSE in loop over rows");
							}

							new = malloc(sizeof(PLpgSQL_stmt_fors));
							memset(new, 0, sizeof(PLpgSQL_stmt_fors));

							new->cmd_type = PLPGSQL_STMT_FORS;
							new->lineno   = $1;
							if ($2.rec)
								new->rec = $2.rec;
							else if ($2.row)
								new->row = $2.row;
							else
							{
								plpgsql_error_lineno = $1;
								yyerror("loop variable of loop over rows must be a record or row variable");
							}
							/*
							 * Must get rid of the "SELECT " we prepended
							 * to expr1's text
							 */
							newquery = strdup(expr1->query + 7);
							free(expr1->query);
							expr1->query = newquery;

							new->query = expr1;

							$$ = (PLpgSQL_stmt *) new;
						}
					}
				;

for_variable	: T_SCALAR
					{
						char		*name;

						plpgsql_convert_ident(yytext, &name, 1);
						$$.name = name;
						$$.lineno  = plpgsql_scanner_lineno();
						$$.rec = NULL;
						$$.row = NULL;
					}
				| T_WORD
					{
						char		*name;

						plpgsql_convert_ident(yytext, &name, 1);
						$$.name = name;
						$$.lineno  = plpgsql_scanner_lineno();
						$$.rec = NULL;
						$$.row = NULL;
					}
				| T_RECORD
					{
						char		*name;

						plpgsql_convert_ident(yytext, &name, 1);
						$$.name = name;
						$$.lineno  = plpgsql_scanner_lineno();
						$$.rec = yylval.rec;
						$$.row = NULL;
					}
				| T_ROW
					{
						char		*name;

						plpgsql_convert_ident(yytext, &name, 1);
						$$.name = name;
						$$.lineno  = plpgsql_scanner_lineno();
						$$.row = yylval.row;
						$$.rec = NULL;
					}
				;

stmt_select		: K_SELECT lno
					{
						$$ = make_select_stmt();
						$$->lineno = $2;
					}
				;

stmt_exit		: K_EXIT lno opt_exitlabel opt_exitcond
					{
						PLpgSQL_stmt_exit *new;

						new = malloc(sizeof(PLpgSQL_stmt_exit));
						memset(new, 0, sizeof(PLpgSQL_stmt_exit));

						new->cmd_type = PLPGSQL_STMT_EXIT;
						new->lineno   = $2;
						new->label	  = $3;
						new->cond	  = $4;

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_return		: K_RETURN lno
					{
						PLpgSQL_stmt_return *new;

						new = malloc(sizeof(PLpgSQL_stmt_return));
						memset(new, 0, sizeof(PLpgSQL_stmt_return));
						new->expr = NULL;
						new->retrecno	= -1;
						new->retrowno	= -1;

						if (plpgsql_curr_compile->fn_retset)
						{
							if (yylex() != ';')
								yyerror("RETURN cannot have a parameter in function returning set; use RETURN NEXT");
						}
						else if (plpgsql_curr_compile->fn_retistuple)
						{
							switch (yylex())
							{
								case K_NULL:
									break;

								case T_ROW:
									new->retrowno = yylval.row->rowno;
									break;

								case T_RECORD:
									new->retrecno = yylval.rec->recno;
									break;

								default:
									yyerror("RETURN must specify a record or row variable in function returning tuple");
									break;
							}
							if (yylex() != ';')
								yyerror("RETURN must specify a record or row variable in function returning tuple");
						}
						else
						{
							/* ordinary expression case */
							new->expr = plpgsql_read_expression(';', ";");
						}

						new->cmd_type = PLPGSQL_STMT_RETURN;
						new->lineno   = $2;

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_return_next: K_RETURN_NEXT lno
					{
						PLpgSQL_stmt_return_next *new;

						if (!plpgsql_curr_compile->fn_retset)
							yyerror("cannot use RETURN NEXT in a non-SETOF function");

						new = malloc(sizeof(PLpgSQL_stmt_return_next));
						memset(new, 0, sizeof(PLpgSQL_stmt_return_next));

						new->cmd_type	= PLPGSQL_STMT_RETURN_NEXT;
						new->lineno		= $2;

						if (plpgsql_curr_compile->fn_retistuple)
						{
							int tok = yylex();

							if (tok == T_RECORD)
								new->rec = yylval.rec;
							else if (tok == T_ROW)
								new->row = yylval.row;
							else
								yyerror("RETURN NEXT must specify a record or row variable in function returning tuple");

							if (yylex() != ';')
								yyerror("RETURN NEXT must specify a record or row variable in function returning tuple");
						}
						else
							new->expr = plpgsql_read_expression(';', ";");

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_raise		: K_RAISE lno raise_level raise_msg raise_params ';'
					{
						PLpgSQL_stmt_raise		*new;

						new = malloc(sizeof(PLpgSQL_stmt_raise));

						new->cmd_type	= PLPGSQL_STMT_RAISE;
						new->lineno		= $2;
						new->elog_level = $3;
						new->message	= $4;
						new->nparams	= $5.nused;
						new->params		= malloc(sizeof(int) * $5.nused);
						memcpy(new->params, $5.nums, sizeof(int) * $5.nused);

						$$ = (PLpgSQL_stmt *)new;
					}
				| K_RAISE lno raise_level raise_msg ';'
					{
						PLpgSQL_stmt_raise		*new;

						new = malloc(sizeof(PLpgSQL_stmt_raise));

						new->cmd_type	= PLPGSQL_STMT_RAISE;
						new->lineno		= $2;
						new->elog_level = $3;
						new->message	= $4;
						new->nparams	= 0;
						new->params		= NULL;

						$$ = (PLpgSQL_stmt *)new;
					}
				;

raise_msg		: T_STRING
					{
						$$ = plpgsql_get_string_value();
					}
				;

raise_level		: K_EXCEPTION
					{
						$$ = ERROR;
					}
				| K_WARNING
					{
						$$ = WARNING;
					}
				| K_NOTICE
					{
						$$ = NOTICE;
					}
				| K_INFO
					{
						$$ = INFO;
					}
				| K_LOG
					{
						$$ = LOG;
					}
				| K_DEBUG
					{
						$$ = DEBUG2;
					}
				;

raise_params	: raise_params raise_param
					{
						if ($1.nused == $1.nalloc)
						{
							$1.nalloc *= 2;
							$1.nums = repalloc($1.nums, sizeof(int) * $1.nalloc);
						}
						$1.nums[$1.nused++] = $2;

						$$.nalloc = $1.nalloc;
						$$.nused  = $1.nused;
						$$.nums   = $1.nums;
					}
				| raise_param
					{
						$$.nalloc = 1;
						$$.nused  = 1;
						$$.nums   = palloc(sizeof(int) * $$.nalloc);
						$$.nums[0] = $1;
					}
				;

raise_param		: ',' T_SCALAR
					{
						$$ = yylval.scalar->dno;
					}
				;

loop_body		: proc_sect K_END K_LOOP ';'
					{ $$ = $1; }
				;

stmt_execsql	: execsql_start lno
					{
						PLpgSQL_stmt_execsql	*new;

						new = malloc(sizeof(PLpgSQL_stmt_execsql));
						new->cmd_type = PLPGSQL_STMT_EXECSQL;
						new->lineno   = $2;
						new->sqlstmt  = read_sql_stmt($1);

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_dynexecute : K_EXECUTE lno expr_until_semi
						{
							PLpgSQL_stmt_dynexecute *new;

							new = malloc(sizeof(PLpgSQL_stmt_dynexecute));
							new->cmd_type = PLPGSQL_STMT_DYNEXECUTE;
							new->lineno   = $2;
							new->query	  = $3;

							$$ = (PLpgSQL_stmt *)new;
						}
				;

stmt_open		: K_OPEN lno cursor_varptr
					{
						PLpgSQL_stmt_open *new;
						int				  tok;

						new = malloc(sizeof(PLpgSQL_stmt_open));
						memset(new, 0, sizeof(PLpgSQL_stmt_open));

						new->cmd_type = PLPGSQL_STMT_OPEN;
						new->lineno = $2;
						new->curvar = $3->varno;

						if ($3->cursor_explicit_expr == NULL)
						{
						    tok = yylex();

							if (tok != K_FOR)
							{
								plpgsql_error_lineno = $2;
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 errmsg("syntax error at \"%s\"",
												yytext),
										 errdetail("Expected FOR to open a reference cursor.")));
							}

							tok = yylex();
							switch (tok)
							{
								case K_SELECT:
								case '(':
									plpgsql_push_back_token(tok);
									new->query = read_sql_stmt("");
									break;

								case K_EXECUTE:
									new->dynquery = read_sql_stmt("SELECT ");
									break;

								default:
									plpgsql_error_lineno = $2;
									ereport(ERROR,
											(errcode(ERRCODE_SYNTAX_ERROR),
											 errmsg("syntax error at \"%s\"",
													yytext)));
							}

						}
						else
						{
							if ($3->cursor_explicit_argrow >= 0)
							{
								char   *cp;

								tok = yylex();

								if (tok != '(')
								{
									plpgsql_error_lineno = plpgsql_scanner_lineno();
									ereport(ERROR,
											(errcode(ERRCODE_SYNTAX_ERROR),
											 errmsg("cursor \"%s\" has arguments",
													$3->refname)));
								}

								/*
								 * Push back the '(', else read_sql_stmt
								 * will complain about unbalanced parens.
								 */
								plpgsql_push_back_token(tok);

								new->argquery = read_sql_stmt("SELECT ");

								/*
								 * Now remove the leading and trailing parens,
								 * because we want "select 1, 2", not
								 * "select (1, 2)".
								 */
								cp = new->argquery->query;

								if (strncmp(cp, "SELECT", 6) != 0)
								{
									plpgsql_error_lineno = plpgsql_scanner_lineno();
									/* internal error */
									elog(ERROR, "expected \"SELECT (\", got \"%s\"",
										 new->argquery->query);
								}
								cp += 6;
								while (*cp == ' ') /* could be more than 1 space here */
									cp++;
								if (*cp != '(')
								{
									plpgsql_error_lineno = plpgsql_scanner_lineno();
									/* internal error */
									elog(ERROR, "expected \"SELECT (\", got \"%s\"",
										 new->argquery->query);
								}
								*cp = ' ';

								cp += strlen(cp) - 1;

								if (*cp != ')')
									yyerror("expected \")\"");
								*cp = '\0';
							}
							else
							{
								tok = yylex();

								if (tok == '(')
								{
									plpgsql_error_lineno = plpgsql_scanner_lineno();
									ereport(ERROR,
											(errcode(ERRCODE_SYNTAX_ERROR),
											 errmsg("cursor \"%s\" has no arguments",
													$3->refname)));
								}
								
								if (tok != ';')
								{
									plpgsql_error_lineno = plpgsql_scanner_lineno();
									ereport(ERROR,
											(errcode(ERRCODE_SYNTAX_ERROR),
											 errmsg("syntax error at \"%s\"",
													yytext)));
								}
							}
						}

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_fetch		: K_FETCH lno cursor_variable K_INTO
					{
						PLpgSQL_stmt_fetch *new;

						new = (PLpgSQL_stmt_fetch *)make_fetch_stmt();
						new->curvar = $3;

						$$ = (PLpgSQL_stmt *)new;
						$$->lineno = $2;
					}
				;

stmt_close		: K_CLOSE lno cursor_variable ';'
					{
						PLpgSQL_stmt_close *new;

						new = malloc(sizeof(PLpgSQL_stmt_close));
						new->cmd_type = PLPGSQL_STMT_CLOSE;
						new->lineno = $2;
						new->curvar = $3;

						$$ = (PLpgSQL_stmt *)new;
					}
				;

stmt_null		: K_NULL ';'
					{
						/* We do not bother building a node for NULL */
						$$ = NULL;
					}
				;

cursor_varptr	: T_SCALAR
					{
						if (yylval.scalar->dtype != PLPGSQL_DTYPE_VAR)
							yyerror("cursor variable must be a simple variable");

						if (((PLpgSQL_var *) yylval.scalar)->datatype->typoid != REFCURSOROID)
						{
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_DATATYPE_MISMATCH),
									 errmsg("\"%s\" must be of type cursor or refcursor",
											((PLpgSQL_var *) yylval.scalar)->refname)));
						}
						$$ = (PLpgSQL_var *) yylval.scalar;
					}
				;

cursor_variable	: T_SCALAR
					{
						if (yylval.scalar->dtype != PLPGSQL_DTYPE_VAR)
							yyerror("cursor variable must be a simple variable");

						if (((PLpgSQL_var *) yylval.scalar)->datatype->typoid != REFCURSOROID)
						{
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_DATATYPE_MISMATCH),
									 errmsg("\"%s\" must be of type refcursor",
											((PLpgSQL_var *) yylval.scalar)->refname)));
						}
						$$ = yylval.scalar->dno;
					}
				;

execsql_start	: T_WORD
					{ $$ = strdup(yytext); }
				| T_ERROR
					{ $$ = strdup(yytext); }
				;

exception_sect	:
					{ $$ = NULL; }
				| K_EXCEPTION proc_exceptions
					{ $$ = $2; }
				;

proc_exceptions	: proc_exceptions proc_exception
						{
								if ($1->exceptions_used == $1->exceptions_alloc)
								{
									$1->exceptions_alloc *= 2;
									$1->exceptions = realloc($1->exceptions, sizeof(PLpgSQL_exception *) * $1->exceptions_alloc);
								}
								$1->exceptions[$1->exceptions_used++] = $2;

								$$ = $1;
						}
				| proc_exception
						{
								PLpgSQL_exceptions	*new;

								new = malloc(sizeof(PLpgSQL_exceptions));
								memset(new, 0, sizeof(PLpgSQL_exceptions));

								new->exceptions_alloc = 16;
								new->exceptions_used  = 1;
								new->exceptions = malloc(sizeof(PLpgSQL_exception *) * new->exceptions_alloc);
								new->exceptions[0] = $1;

								$$ = new;
						}
				;

proc_exception	: K_WHEN lno proc_conditions K_THEN proc_sect
					{
						PLpgSQL_exception *new;

						new = malloc(sizeof(PLpgSQL_exception));
						memset(new, 0, sizeof(PLpgSQL_exception));

						new->lineno     = $2;
						new->conditions = $3;
						new->action	    = $5;

						$$ = new;
					}
				;

proc_conditions	: proc_conditions K_OR opt_lblname
						{
							PLpgSQL_condition	*old;

							for (old = $1; old->next != NULL; old = old->next)
								/* skip */ ;
							old->next = plpgsql_parse_err_condition($3);

							$$ = $1;
						}
				| opt_lblname
						{
							$$ = plpgsql_parse_err_condition($1);
						}
				;

expr_until_semi :
					{ $$ = plpgsql_read_expression(';', ";"); }
				;

expr_until_rightbracket :
					{ $$ = plpgsql_read_expression(']', "]"); }
				;

expr_until_then :
					{ $$ = plpgsql_read_expression(K_THEN, "THEN"); }
				;

expr_until_loop :
					{ $$ = plpgsql_read_expression(K_LOOP, "LOOP"); }
				;

opt_label		:
					{
						plpgsql_ns_push(NULL);
						$$ = NULL;
					}
				| '<' '<' opt_lblname '>' '>'
					{
						plpgsql_ns_push($3);
						$$ = $3;
					}
				;

opt_exitlabel	:
					{ $$ = NULL; }
				| T_LABEL
					{
						char	*name;

						plpgsql_convert_ident(yytext, &name, 1);
						$$ = strdup(name);
						pfree(name);
					}
				| T_WORD
					{
						/* just to give a better error than "syntax error" */
						yyerror("no such label");
					}
				;

opt_exitcond	: ';'
					{ $$ = NULL; }
				| K_WHEN expr_until_semi
					{ $$ = $2; }
				;

opt_lblname		: T_WORD
					{
						char	*name;

						plpgsql_convert_ident(yytext, &name, 1);
						$$ = strdup(name);
						pfree(name);
					}
				;

lno				:
					{
						$$ = plpgsql_error_lineno = plpgsql_scanner_lineno();
					}
				;

%%


PLpgSQL_expr *
plpgsql_read_expression(int until, const char *expected)
{
	return read_sql_construct(until, 0, expected, true, "SELECT ", NULL);
}

static PLpgSQL_expr *
read_sql_stmt(const char *sqlstart)
{
	return read_sql_construct(';', 0, ";", false, sqlstart, NULL);
}

/*
 * Read a SQL construct and build a PLpgSQL_expr for it.
 *
 * until:		token code for expected terminator
 * until2:		token code for alternate terminator (pass 0 if none)
 * expected:	text to use in complaining that terminator was not found
 * isexpression: whether to say we're reading an "expression" or a "statement"
 * sqlstart:	text to prefix to the accumulated SQL text
 * endtoken:	if not NULL, ending token is stored at *endtoken
 *				(this is only interesting if until2 isn't zero)
 */
static PLpgSQL_expr *
read_sql_construct(int until,
				   int until2,
				   const char *expected,
				   bool isexpression,
				   const char *sqlstart,
				   int *endtoken)
{
	int					tok;
	int					lno;
	PLpgSQL_dstring		ds;
	int					parenlevel = 0;
	int					nparams = 0;
	int					params[1024];
	char				buf[32];
	PLpgSQL_expr		*expr;

	lno = plpgsql_scanner_lineno();
	plpgsql_dstring_init(&ds);
	plpgsql_dstring_append(&ds, sqlstart);

	for (;;)
	{
		tok = yylex();
		if (tok == until && parenlevel == 0)
			break;
		if (tok == until2 && parenlevel == 0)
			break;
		if (tok == '(' || tok == '[')
			parenlevel++;
		else if (tok == ')' || tok == ']')
		{
			parenlevel--;
			if (parenlevel < 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("mismatched parentheses")));
		}
		/*
		 * End of function definition is an error, and we don't expect to
		 * hit a semicolon either (unless it's the until symbol, in which
		 * case we should have fallen out above).
		 */
		if (tok == 0 || tok == ';')
		{
			plpgsql_error_lineno = lno;
			if (parenlevel != 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("mismatched parentheses")));
			if (isexpression)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("missing \"%s\" at end of SQL expression",
								expected)));
			else
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("missing \"%s\" at end of SQL statement",
								expected)));
		}
		if (plpgsql_SpaceScanned)
			plpgsql_dstring_append(&ds, " ");
		switch (tok)
		{
			case T_SCALAR:
				params[nparams] = yylval.scalar->dno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			case T_ROW:
				params[nparams] = yylval.row->rowno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			case T_RECORD:
				params[nparams] = yylval.rec->recno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			default:
				plpgsql_dstring_append(&ds, yytext);
				break;
		}
	}

	if (endtoken)
		*endtoken = tok;

	expr = malloc(sizeof(PLpgSQL_expr) + sizeof(int) * nparams - sizeof(int));
	expr->dtype			= PLPGSQL_DTYPE_EXPR;
	expr->query			= strdup(plpgsql_dstring_get(&ds));
	expr->plan			= NULL;
	expr->nparams		= nparams;
	while(nparams-- > 0)
		expr->params[nparams] = params[nparams];
	plpgsql_dstring_free(&ds);

	return expr;
}

static PLpgSQL_type *
read_datatype(int tok)
{
	int					lno;
	PLpgSQL_dstring		ds;
	PLpgSQL_type		*result;
	bool				needspace = false;
	int					parenlevel = 0;

	lno = plpgsql_scanner_lineno();

	/* Often there will be a lookahead token, but if not, get one */
	if (tok == YYEMPTY)
		tok = yylex();

	if (tok == T_DTYPE)
	{
		/* lexer found word%TYPE and did its thing already */
		return yylval.dtype;
	}

	plpgsql_dstring_init(&ds);

	while (tok != ';')
	{
		if (tok == 0)
		{
			plpgsql_error_lineno = lno;
			if (parenlevel != 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("mismatched parentheses")));
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("incomplete datatype declaration")));
		}
		/* Possible followers for datatype in a declaration */
		if (tok == K_NOT || tok == K_ASSIGN || tok == K_DEFAULT)
			break;
		/* Possible followers for datatype in a cursor_arg list */
		if ((tok == ',' || tok == ')') && parenlevel == 0)
			break;
		if (tok == '(')
			parenlevel++;
		else if (tok == ')')
			parenlevel--;
		if (needspace)
			plpgsql_dstring_append(&ds, " ");
		needspace = true;
		plpgsql_dstring_append(&ds, yytext);

		tok = yylex();
	}

	plpgsql_push_back_token(tok);

	plpgsql_error_lineno = lno;	/* in case of error in parse_datatype */

	result = plpgsql_parse_datatype(plpgsql_dstring_get(&ds));

	plpgsql_dstring_free(&ds);

	return result;
}


static PLpgSQL_stmt *
make_select_stmt(void)
{
	PLpgSQL_dstring		ds;
	int					nparams = 0;
	int					params[1024];
	char				buf[32];
	PLpgSQL_expr		*expr;
	PLpgSQL_row			*row = NULL;
	PLpgSQL_rec			*rec = NULL;
	int					tok = 0;
	int					have_nexttok = 0;
	int					have_into = 0;

	plpgsql_dstring_init(&ds);
	plpgsql_dstring_append(&ds, "SELECT ");

	while(1)
	{
		if (!have_nexttok)
			tok = yylex();
		have_nexttok = 0;
		if (tok == ';')
			break;
		if (tok == 0)
		{
			plpgsql_error_lineno = plpgsql_scanner_lineno();
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("unexpected end of function definition")));
		}
		if (tok == K_INTO)
		{
			if (have_into)
			{
				plpgsql_error_lineno = plpgsql_scanner_lineno();
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("INTO specified more than once")));
			}
			tok = yylex();
			switch (tok)
			{
				case T_ROW:
					row = yylval.row;
					have_into = 1;
					break;

				case T_RECORD:
					rec = yylval.rec;
					have_into = 1;
					break;

				case T_SCALAR:
				{
					int				nfields = 1;
					char			*fieldnames[1024];
					int				varnos[1024];

					check_assignable(yylval.scalar);
					fieldnames[0] = strdup(yytext);
					varnos[0]	  = yylval.scalar->dno;

					while ((tok = yylex()) == ',')
					{
						tok = yylex();
						switch(tok)
						{
							case T_SCALAR:
								check_assignable(yylval.scalar);
								fieldnames[nfields] = strdup(yytext);
								varnos[nfields++]	= yylval.scalar->dno;
								break;

							default:
								plpgsql_error_lineno = plpgsql_scanner_lineno();
								ereport(ERROR,
										(errcode(ERRCODE_SYNTAX_ERROR),
										 errmsg("\"%s\" is not a variable",
												yytext)));
						}
					}
					have_nexttok = 1;

					row = malloc(sizeof(PLpgSQL_row));
					row->dtype = PLPGSQL_DTYPE_ROW;
					row->refname = strdup("*internal*");
					row->lineno = plpgsql_scanner_lineno();
					row->rowtupdesc = NULL;
					row->nfields = nfields;
					row->fieldnames = malloc(sizeof(char *) * nfields);
					row->varnos = malloc(sizeof(int) * nfields);
					while (--nfields >= 0)
					{
						row->fieldnames[nfields] = fieldnames[nfields];
						row->varnos[nfields] = varnos[nfields];
					}

					plpgsql_adddatum((PLpgSQL_datum *)row);

					have_into = 1;
				}
				break;

				default:
					/* Treat the INTO as non-special */
					plpgsql_dstring_append(&ds, " INTO ");
					have_nexttok = 1;
					break;
			}
			continue;
		}

		if (plpgsql_SpaceScanned)
			plpgsql_dstring_append(&ds, " ");
		switch (tok)
		{
			case T_SCALAR:
				params[nparams] = yylval.scalar->dno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			case T_ROW:
				params[nparams] = yylval.row->rowno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			case T_RECORD:
				params[nparams] = yylval.rec->recno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			default:
				plpgsql_dstring_append(&ds, yytext);
				break;
		}
	}

	expr = malloc(sizeof(PLpgSQL_expr) + sizeof(int) * nparams - sizeof(int));
	expr->dtype			= PLPGSQL_DTYPE_EXPR;
	expr->query			= strdup(plpgsql_dstring_get(&ds));
	expr->plan			= NULL;
	expr->nparams		= nparams;
	while(nparams-- > 0)
		expr->params[nparams] = params[nparams];
	plpgsql_dstring_free(&ds);

	if (have_into)
	{
		PLpgSQL_stmt_select *select;

		select = malloc(sizeof(PLpgSQL_stmt_select));
		memset(select, 0, sizeof(PLpgSQL_stmt_select));
		select->cmd_type = PLPGSQL_STMT_SELECT;
		select->rec		 = rec;
		select->row		 = row;
		select->query	 = expr;

		return (PLpgSQL_stmt *)select;
	}
	else
	{
		PLpgSQL_stmt_execsql *execsql;

		execsql = malloc(sizeof(PLpgSQL_stmt_execsql));
		execsql->cmd_type = PLPGSQL_STMT_EXECSQL;
		execsql->sqlstmt  = expr;

		return (PLpgSQL_stmt *)execsql;
	}
}


static PLpgSQL_stmt *
make_fetch_stmt(void)
{
	int					tok;
	PLpgSQL_row		   *row = NULL;
	PLpgSQL_rec		   *rec = NULL;
	PLpgSQL_stmt_fetch *fetch;
	int					have_nexttok = 0;

	/* We have already parsed everything through the INTO keyword */

	tok = yylex();
	switch (tok)
	{
		case T_ROW:
			row = yylval.row;
			break;

		case T_RECORD:
			rec = yylval.rec;
			break;

		case T_SCALAR:
			{
				int				nfields = 1;
				char			*fieldnames[1024];
				int				varnos[1024];

				check_assignable(yylval.scalar);
				fieldnames[0] = strdup(yytext);
				varnos[0]	  = yylval.scalar->dno;

				while ((tok = yylex()) == ',')
				{
					tok = yylex();
					switch(tok)
					{
						case T_SCALAR:
							check_assignable(yylval.scalar);
							fieldnames[nfields] = strdup(yytext);
							varnos[nfields++]	= yylval.scalar->dno;
							break;

						default:
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("\"%s\" is not a variable",
											yytext)));
					}
				}
				have_nexttok = 1;

				row = malloc(sizeof(PLpgSQL_row));
				row->dtype = PLPGSQL_DTYPE_ROW;
				row->refname = strdup("*internal*");
				row->lineno = plpgsql_scanner_lineno();
				row->rowtupdesc = NULL;
				row->nfields = nfields;
				row->fieldnames = malloc(sizeof(char *) * nfields);
				row->varnos = malloc(sizeof(int) * nfields);
				while (--nfields >= 0)
				{
					row->fieldnames[nfields] = fieldnames[nfields];
					row->varnos[nfields] = varnos[nfields];
				}

				plpgsql_adddatum((PLpgSQL_datum *)row);
			}
			break;

		default:
			yyerror("syntax error");
	}

	if (!have_nexttok)
		tok = yylex();

	if (tok != ';')
		yyerror("syntax error");

	fetch = malloc(sizeof(PLpgSQL_stmt_select));
	memset(fetch, 0, sizeof(PLpgSQL_stmt_fetch));
	fetch->cmd_type = PLPGSQL_STMT_FETCH;
	fetch->rec		 = rec;
	fetch->row		 = row;

	return (PLpgSQL_stmt *)fetch;
}


static void
check_assignable(PLpgSQL_datum *datum)
{
	switch (datum->dtype)
	{
		case PLPGSQL_DTYPE_VAR:
			if (((PLpgSQL_var *) datum)->isconst)
			{
				plpgsql_error_lineno = plpgsql_scanner_lineno();
				ereport(ERROR,
						(errcode(ERRCODE_ERROR_IN_ASSIGNMENT),
						 errmsg("\"%s\" is declared CONSTANT",
								((PLpgSQL_var *) datum)->refname)));
			}
			break;
		case PLPGSQL_DTYPE_ROW:
			/* always assignable? */
			break;
		case PLPGSQL_DTYPE_REC:
			/* always assignable?  What about NEW/OLD? */
			break;
		case PLPGSQL_DTYPE_RECFIELD:
			/* always assignable? */
			break;
		case PLPGSQL_DTYPE_ARRAYELEM:
			/* always assignable? */
			break;
		case PLPGSQL_DTYPE_TRIGARG:
			yyerror("cannot assign to tg_argv");
			break;
		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			break;
	}
}

#include "pl_scan.c"
