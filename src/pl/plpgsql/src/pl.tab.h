/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.	*/

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.	*/
#ifndef PLPGSQL_YYTOKENTYPE
#define PLPGSQL_YYTOKENTYPE

 /*
  * Put the tokens into the symbol table, so that GDB and other debuggers
  * know about them.
  */
enum plpgsql_yytokentype
{
	K_ALIAS = 258,
	K_ASSIGN = 259,
	K_BEGIN = 260,
	K_CLOSE = 261,
	K_CONSTANT = 262,
	K_CURSOR = 263,
	K_DEBUG = 264,
	K_DECLARE = 265,
	K_DEFAULT = 266,
	K_DIAGNOSTICS = 267,
	K_DOTDOT = 268,
	K_ELSE = 269,
	K_ELSIF = 270,
	K_END = 271,
	K_EXCEPTION = 272,
	K_EXECUTE = 273,
	K_EXIT = 274,
	K_FOR = 275,
	K_FETCH = 276,
	K_FROM = 277,
	K_GET = 278,
	K_IF = 279,
	K_IN = 280,
	K_INFO = 281,
	K_INTO = 282,
	K_IS = 283,
	K_LOG = 284,
	K_LOOP = 285,
	K_NEXT = 286,
	K_NOT = 287,
	K_NOTICE = 288,
	K_NULL = 289,
	K_OPEN = 290,
	K_PERFORM = 291,
	K_ROW_COUNT = 292,
	K_RAISE = 293,
	K_RECORD = 294,
	K_RENAME = 295,
	K_RESULT_OID = 296,
	K_RETURN = 297,
	K_RETURN_NEXT = 298,
	K_REVERSE = 299,
	K_SELECT = 300,
	K_THEN = 301,
	K_TO = 302,
	K_TYPE = 303,
	K_WARNING = 304,
	K_WHEN = 305,
	K_WHILE = 306,
	T_FUNCTION = 307,
	T_TRIGGER = 308,
	T_STRING = 309,
	T_NUMBER = 310,
	T_VARIABLE = 311,
	T_ROW = 312,
	T_RECORD = 313,
	T_DTYPE = 314,
	T_LABEL = 315,
	T_WORD = 316,
	T_ERROR = 317,
	O_OPTION = 318,
	O_DUMP = 319
};
#endif
#define K_ALIAS 258
#define K_ASSIGN 259
#define K_BEGIN 260
#define K_CLOSE 261
#define K_CONSTANT 262
#define K_CURSOR 263
#define K_DEBUG 264
#define K_DECLARE 265
#define K_DEFAULT 266
#define K_DIAGNOSTICS 267
#define K_DOTDOT 268
#define K_ELSE 269
#define K_ELSIF 270
#define K_END 271
#define K_EXCEPTION 272
#define K_EXECUTE 273
#define K_EXIT 274
#define K_FOR 275
#define K_FETCH 276
#define K_FROM 277
#define K_GET 278
#define K_IF 279
#define K_IN 280
#define K_INFO 281
#define K_INTO 282
#define K_IS 283
#define K_LOG 284
#define K_LOOP 285
#define K_NEXT 286
#define K_NOT 287
#define K_NOTICE 288
#define K_NULL 289
#define K_OPEN 290
#define K_PERFORM 291
#define K_ROW_COUNT 292
#define K_RAISE 293
#define K_RECORD 294
#define K_RENAME 295
#define K_RESULT_OID 296
#define K_RETURN 297
#define K_RETURN_NEXT 298
#define K_REVERSE 299
#define K_SELECT 300
#define K_THEN 301
#define K_TO 302
#define K_TYPE 303
#define K_WARNING 304
#define K_WHEN 305
#define K_WHILE 306
#define T_FUNCTION 307
#define T_TRIGGER 308
#define T_STRING 309
#define T_NUMBER 310
#define T_VARIABLE 311
#define T_ROW 312
#define T_RECORD 313
#define T_DTYPE 314
#define T_LABEL 315
#define T_WORD 316
#define T_ERROR 317
#define O_OPTION 318
#define O_DUMP 319




#if ! defined (PLPGSQL_YYSTYPE) && ! defined (PLPGSQL_YYSTYPE_IS_DECLARED)
#line 55 "gram.y"
typedef union PLPGSQL_YYSTYPE
{
	int32		ival;
	char	   *str;
	struct
	{
		char	   *name;
		int			lineno;
	}			varname;
	struct
	{
		int			nalloc;
		int			nused;
		int		   *nums;
	}			intlist;
	struct
	{
		int			nalloc;
		int			nused;
		PLpgSQL_diag_item *dtitems;
	}			dtlist;
	struct
	{
		int			reverse;
		PLpgSQL_expr *expr;
	}			forilow;
	struct
	{
		char	   *label;
		int			n_initvars;
		int		   *initvarnos;
	}			declhdr;
	PLpgSQL_type *dtype;
	PLpgSQL_datum *variable;	/* a VAR, RECFIELD, or TRIGARG */
	PLpgSQL_var *var;
	PLpgSQL_row *row;
	PLpgSQL_rec *rec;
	PLpgSQL_expr *expr;
	PLpgSQL_stmt *stmt;
	PLpgSQL_stmts *stmts;
	PLpgSQL_stmt_block *program;
	PLpgSQL_nsitem *nsitem;
}	PLPGSQL_YYSTYPE;

/* Line 1248 of yacc.c.  */
#line 207 "y.tab.h"
#define plpgsql_yystype PLPGSQL_YYSTYPE /* obsolescent; will be withdrawn */
#define PLPGSQL_YYSTYPE_IS_DECLARED 1
#define PLPGSQL_YYSTYPE_IS_TRIVIAL 1
#endif

extern PLPGSQL_YYSTYPE plpgsql_yylval;
