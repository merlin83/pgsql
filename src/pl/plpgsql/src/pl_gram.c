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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with plpgsql_yy or PLPGSQL_YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define PLPGSQL_YYBISON 1

/* Skeleton name.  */
#define PLPGSQL_YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define PLPGSQL_YYPURE 0

/* Using locations.  */
#define PLPGSQL_YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 1 "gram.y"

/**********************************************************************
 * gram.y				- Parser for the PL/pgSQL
 *						  procedural language
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/pl/plpgsql/src/Attic/pl_gram.c,v 1.1.2.1 2003-09-07 22:51:55 momjian Exp $
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


static PLpgSQL_expr *read_sql_construct(int until,
				   const char *expected,
				   bool isexpression,
				   const char *sqlstart);
static PLpgSQL_expr *read_sql_stmt(const char *sqlstart);
static PLpgSQL_type *read_datatype(int tok);
static PLpgSQL_stmt *make_select_stmt(void);
static PLpgSQL_stmt *make_fetch_stmt(void);
static PLpgSQL_expr *make_tupret_expr(PLpgSQL_row * row);
static void check_assignable(PLpgSQL_datum * datum);



/* Enabling traces.  */
#ifndef PLPGSQL_YYDEBUG
#define PLPGSQL_YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef PLPGSQL_YYERROR_VERBOSE
#undef PLPGSQL_YYERROR_VERBOSE
#define PLPGSQL_YYERROR_VERBOSE 1
#else
#define PLPGSQL_YYERROR_VERBOSE 0
#endif

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

/* Line 191 of yacc.c.	*/
#line 300 "y.tab.c"
#define plpgsql_yystype PLPGSQL_YYSTYPE /* obsolescent; will be withdrawn */
#define PLPGSQL_YYSTYPE_IS_DECLARED 1
#define PLPGSQL_YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.	*/
#line 312 "y.tab.c"

#if ! defined (plpgsql_yyoverflow) || PLPGSQL_YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#if PLPGSQL_YYSTACK_USE_ALLOCA
#define PLPGSQL_YYSTACK_ALLOC alloca
#else
#ifndef PLPGSQL_YYSTACK_USE_ALLOCA
#if defined (alloca) || defined (_ALLOCA_H)
#define PLPGSQL_YYSTACK_ALLOC alloca
#else
#ifdef __GNUC__
#define PLPGSQL_YYSTACK_ALLOC __builtin_alloca
#endif
#endif
#endif
#endif

#ifdef PLPGSQL_YYSTACK_ALLOC
 /* Pacify GCC's `empty if-body' warning. */
#define PLPGSQL_YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#else
#if defined (__STDC__) || defined (__cplusplus)
#include <stdlib.h>				/* INFRINGES ON USER NAME SPACE */
#define PLPGSQL_YYSIZE_T size_t
#endif
#define PLPGSQL_YYSTACK_ALLOC malloc
#define PLPGSQL_YYSTACK_FREE free
#endif
#endif   /* ! defined (plpgsql_yyoverflow) ||
								 * PLPGSQL_YYERROR_VERBOSE */


#if (! defined (plpgsql_yyoverflow) \
	 && (! defined (__cplusplus) \
	 || (PLPGSQL_YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union plpgsql_yyalloc
{
	short		plpgsql_yyss;
	PLPGSQL_YYSTYPE plpgsql_yyvs;
};

/* The size of the maximum gap between one aligned stack and the next.	*/
#define PLPGSQL_YYSTACK_GAP_MAXIMUM (sizeof (union plpgsql_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.	*/
#define PLPGSQL_YYSTACK_BYTES(N) \
	 ((N) * (sizeof (short) + sizeof (PLPGSQL_YYSTYPE))				\
	  + PLPGSQL_YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.	The source and destination do
   not overlap.  */
#ifndef PLPGSQL_YYCOPY
#if 1 < __GNUC__
#define PLPGSQL_YYCOPY(To, From, Count) \
	  __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#else
#define PLPGSQL_YYCOPY(To, From, Count)		 \
	  do					\
	{					\
	  register PLPGSQL_YYSIZE_T plpgsql_yyi;		\
	  for (plpgsql_yyi = 0; plpgsql_yyi < (Count); plpgsql_yyi++)	\
		(To)[plpgsql_yyi] = (From)[plpgsql_yyi];		\
	}					\
	  while (0)
#endif
#endif

/* Relocate STACK from its old location to the new one.  The
   local variables PLPGSQL_YYSIZE and PLPGSQL_YYSTACKSIZE give the old and new number of
   elements in the stack, and PLPGSQL_YYPTR gives the new location of the
   stack.  Advance PLPGSQL_YYPTR to a properly aligned location for the next
   stack.  */
#define PLPGSQL_YYSTACK_RELOCATE(Stack)					   \
	do									\
	  {									\
	PLPGSQL_YYSIZE_T plpgsql_yynewbytes;						\
	PLPGSQL_YYCOPY (&plpgsql_yyptr->Stack, Stack, plpgsql_yysize);				\
	Stack = &plpgsql_yyptr->Stack;						\
	plpgsql_yynewbytes = plpgsql_yystacksize * sizeof (*Stack) + PLPGSQL_YYSTACK_GAP_MAXIMUM; \
	plpgsql_yyptr += plpgsql_yynewbytes / sizeof (*plpgsql_yyptr);				\
	  }									\
	while (0)
#endif

#if defined (__STDC__) || defined (__cplusplus)
typedef signed char plpgsql_yysigned_char;

#else
typedef short plpgsql_yysigned_char;
#endif

/* PLPGSQL_YYFINAL -- State number of the termination state. */
#define PLPGSQL_YYFINAL  9
/* PLPGSQL_YYLAST -- Last index in PLPGSQL_YYTABLE.  */
#define PLPGSQL_YYLAST	 205

/* PLPGSQL_YYNTOKENS -- Number of terminals. */
#define PLPGSQL_YYNTOKENS  72
/* PLPGSQL_YYNNTS -- Number of nonterminals. */
#define PLPGSQL_YYNNTS	75
/* PLPGSQL_YYNRULES -- Number of rules. */
#define PLPGSQL_YYNRULES  135
/* PLPGSQL_YYNRULES -- Number of states. */
#define PLPGSQL_YYNSTATES  234

/* PLPGSQL_YYTRANSLATE(PLPGSQL_YYLEX) -- Bison symbol number corresponding to PLPGSQL_YYLEX.  */
#define PLPGSQL_YYUNDEFTOK	2
#define PLPGSQL_YYMAXUTOK	319

#define PLPGSQL_YYTRANSLATE(PLPGSQL_YYX)						\
  ((unsigned int) (PLPGSQL_YYX) <= PLPGSQL_YYMAXUTOK ? plpgsql_yytranslate[PLPGSQL_YYX] : PLPGSQL_YYUNDEFTOK)

/* PLPGSQL_YYTRANSLATE[PLPGSQL_YYLEX] -- Bison symbol number corresponding to PLPGSQL_YYLEX.  */
static const unsigned char plpgsql_yytranslate[] =
{
	0, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	68, 69, 2, 2, 70, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 65,
	66, 2, 67, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 71, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 1, 2, 3, 4,
	5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34,
	35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
	45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
	55, 56, 57, 58, 59, 60, 61, 62, 63, 64
};

#if PLPGSQL_YYDEBUG
/* PLPGSQL_YYPRHS[PLPGSQL_YYN] -- Index of the first RHS symbol of rule number PLPGSQL_YYN in
   PLPGSQL_YYRHS.  */
static const unsigned short plpgsql_yyprhs[] =
{
	0, 0, 3, 8, 13, 14, 16, 19, 21, 24,
	25, 27, 33, 35, 38, 42, 44, 47, 49, 55,
	57, 59, 65, 69, 73, 79, 85, 86, 94, 95,
	96, 100, 102, 106, 109, 111, 113, 115, 117, 119,
	121, 122, 124, 125, 126, 129, 131, 133, 135, 137,
	138, 140, 143, 145, 148, 150, 152, 154, 156, 158,
	160, 162, 164, 166, 168, 170, 172, 174, 176, 178,
	180, 182, 184, 186, 190, 195, 201, 207, 211, 213,
	215, 217, 219, 223, 232, 233, 239, 242, 247, 253,
	262, 264, 266, 268, 269, 278, 287, 289, 291, 294,
	299, 302, 305, 312, 318, 320, 322, 324, 326, 328,
	330, 332, 335, 337, 340, 345, 348, 352, 356, 361,
	366, 368, 370, 372, 374, 375, 376, 377, 378, 379,
	385, 386, 388, 390, 393, 395
};

/* PLPGSQL_YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short plpgsql_yyrhs[] =
{
	73, 0, -1, 52, 74, 78, 77, -1, 53, 74,
	78, 77, -1, -1, 75, -1, 75, 76, -1, 76,
	-1, 63, 64, -1, -1, 65, -1, 79, 5, 146,
	99, 16, -1, 142, -1, 142, 80, -1, 142, 80,
	81, -1, 10, -1, 81, 82, -1, 82, -1, 66,
	66, 145, 67, 67, -1, 10, -1, 83, -1, 92,
	94, 95, 96, 97, -1, 92, 39, 65, -1, 92,
	91, 65, -1, 92, 3, 20, 90, 65, -1, 40,
	93, 47, 93, 65, -1, -1, 92, 8, 84, 86,
	89, 45, 85, -1, -1, -1, 68, 87, 69, -1,
	88, -1, 87, 70, 88, -1, 92, 95, -1, 28,
	-1, 20, -1, 61, -1, 57, -1, 61, -1, 61,
	-1, -1, 7, -1, -1, -1, 32, 34, -1, 65,
	-1, 98, -1, 4, -1, 11, -1, -1, 100, -1,
	100, 101, -1, 101, -1, 78, 65, -1, 103, -1,
	109, -1, 111, -1, 112, -1, 113, -1, 117, -1,
	120, -1, 121, -1, 122, -1, 123, -1, 124, -1,
	130, -1, 131, -1, 118, -1, 102, -1, 104, -1,
	132, -1, 133, -1, 134, -1, 36, 146, 138, -1,
	108, 146, 4, 138, -1, 23, 12, 146, 105, 65,
	-1, 105, 70, 107, 4, 106, -1, 107, 4, 106,
	-1, 37, -1, 41, -1, 56, -1, 56, -1, 108,
	71, 139, -1, 24, 146, 140, 99, 110, 16, 24,
	65, -1, -1, 15, 146, 140, 99, 110, -1, 14,
	99, -1, 142, 30, 146, 129, -1, 142, 51, 146,
	141, 129, -1, 142, 20, 146, 114, 25, 116, 141,
	129, -1, 115, -1, 56, -1, 61, -1, -1, 142,
	20, 146, 119, 25, 45, 141, 129, -1, 142, 20,
	146, 119, 25, 18, 141, 129, -1, 58, -1, 57,
	-1, 45, 146, -1, 19, 146, 143, 144, -1, 42,
	146, -1, 43, 146, -1, 38, 146, 126, 125, 127,
	65, -1, 38, 146, 126, 125, 65, -1, 54, -1,
	17, -1, 49, -1, 33, -1, 26, -1, 29, -1,
	9, -1, 127, 128, -1, 128, -1, 70, 56, -1,
	99, 16, 30, 65, -1, 137, 146, -1, 18, 146,
	138, -1, 35, 146, 135, -1, 21, 146, 136, 27,
	-1, 6, 146, 136, 65, -1, 56, -1, 56, -1,
	61, -1, 62, -1, -1, -1, -1, -1, -1, 66,
	66, 145, 67, 67, -1, -1, 60, -1, 65, -1,
	50, 138, -1, 61, -1, -1
};

/* PLPGSQL_YYRLINE[PLPGSQL_YYN] -- source line where rule number PLPGSQL_YYN was defined.  */
static const unsigned short plpgsql_yyrline[] =
{
	0, 212, 212, 216, 222, 223, 226, 227, 230, 236,
	237, 240, 261, 269, 277, 288, 294, 296, 300, 302,
	304, 308, 362, 376, 386, 391, 396, 395, 447, 459,
	462, 486, 512, 523, 546, 547, 549, 577, 583, 595,
	606, 607, 612, 623, 624, 628, 630, 681, 682, 686,
	693, 697, 708, 725, 727, 729, 731, 733, 735, 737,
	739, 741, 743, 745, 747, 749, 751, 753, 755, 757,
	759, 761, 763, 767, 782, 798, 815, 830, 840, 844,
	850, 858, 863, 880, 898, 905, 948, 954, 972, 991,
	1013, 1039, 1049, 1062, 1080, 1111, 1142, 1144, 1150, 1157,
	1173, 1216, 1247, 1263, 1280, 1286, 1290, 1294, 1298, 1302,
	1306, 1312, 1325, 1334, 1340, 1344, 1357, 1370, 1501, 1513,
	1526, 1543, 1560, 1562, 1567, 1571, 1575, 1579, 1583, 1587,
	1595, 1596, 1600, 1602, 1606, 1617
};
#endif

#if PLPGSQL_YYDEBUG || PLPGSQL_YYERROR_VERBOSE
/* PLPGSQL_YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at PLPGSQL_YYNTOKENS, nonterminals. */
static const char *const plpgsql_yytname[] =
{
	"$end", "error", "$undefined", "K_ALIAS", "K_ASSIGN", "K_BEGIN",
	"K_CLOSE", "K_CONSTANT", "K_CURSOR", "K_DEBUG", "K_DECLARE",
	"K_DEFAULT", "K_DIAGNOSTICS", "K_DOTDOT", "K_ELSE", "K_ELSIF", "K_END",
	"K_EXCEPTION", "K_EXECUTE", "K_EXIT", "K_FOR", "K_FETCH", "K_FROM",
	"K_GET", "K_IF", "K_IN", "K_INFO", "K_INTO", "K_IS", "K_LOG", "K_LOOP",
	"K_NEXT", "K_NOT", "K_NOTICE", "K_NULL", "K_OPEN", "K_PERFORM",
	"K_ROW_COUNT", "K_RAISE", "K_RECORD", "K_RENAME", "K_RESULT_OID",
	"K_RETURN", "K_RETURN_NEXT", "K_REVERSE", "K_SELECT", "K_THEN", "K_TO",
	"K_TYPE", "K_WARNING", "K_WHEN", "K_WHILE", "T_FUNCTION", "T_TRIGGER",
	"T_STRING", "T_NUMBER", "T_VARIABLE", "T_ROW", "T_RECORD", "T_DTYPE",
	"T_LABEL", "T_WORD", "T_ERROR", "O_OPTION", "O_DUMP", "';'", "'<'",
	"'>'", "'('", "')'", "','", "'['", "$accept", "pl_function",
	"comp_optsect", "comp_options", "comp_option", "opt_semi", "pl_block",
	"decl_sect", "decl_start", "decl_stmts", "decl_stmt", "decl_statement",
	"@1", "decl_cursor_query", "decl_cursor_args", "decl_cursor_arglist",
	"decl_cursor_arg", "decl_is_from", "decl_aliasitem", "decl_rowtype",
	"decl_varname", "decl_renname", "decl_const", "decl_datatype",
	"decl_notnull", "decl_defval", "decl_defkey", "proc_sect", "proc_stmts",
	"proc_stmt", "stmt_perform", "stmt_assign", "stmt_getdiag",
	"getdiag_list", "getdiag_item", "getdiag_target", "assign_var",
	"stmt_if", "stmt_else", "stmt_loop", "stmt_while", "stmt_fori",
	"fori_var", "fori_varname", "fori_lower", "stmt_fors", "stmt_dynfors",
	"fors_target", "stmt_select", "stmt_exit", "stmt_return",
	"stmt_return_next", "stmt_raise", "raise_msg", "raise_level",
	"raise_params", "raise_param", "loop_body", "stmt_execsql",
	"stmt_dynexecute", "stmt_open", "stmt_fetch", "stmt_close",
	"cursor_varptr", "cursor_variable", "execsql_start", "expr_until_semi",
	"expr_until_rightbracket", "expr_until_then", "expr_until_loop",
	"opt_label", "opt_exitlabel", "opt_exitcond", "opt_lblname", "lno", 0
};
#endif

#ifdef PLPGSQL_YYPRINT
/* PLPGSQL_YYTOKNUM[PLPGSQL_YYLEX-NUM] -- Internal token number corresponding to
   token PLPGSQL_YYLEX-NUM.  */
static const unsigned short plpgsql_yytoknum[] =
{
	0, 256, 257, 258, 259, 260, 261, 262, 263, 264,
	265, 266, 267, 268, 269, 270, 271, 272, 273, 274,
	275, 276, 277, 278, 279, 280, 281, 282, 283, 284,
	285, 286, 287, 288, 289, 290, 291, 292, 293, 294,
	295, 296, 297, 298, 299, 300, 301, 302, 303, 304,
	305, 306, 307, 308, 309, 310, 311, 312, 313, 314,
	315, 316, 317, 318, 319, 59, 60, 62, 40, 41,
	44, 91
};
#endif

/* PLPGSQL_YYR1[PLPGSQL_YYN] -- Symbol number of symbol that rule PLPGSQL_YYN derives.	*/
static const unsigned char plpgsql_yyr1[] =
{
	0, 72, 73, 73, 74, 74, 75, 75, 76, 77,
	77, 78, 79, 79, 79, 80, 81, 81, 82, 82,
	82, 83, 83, 83, 83, 83, 84, 83, 85, 86,
	86, 87, 87, 88, 89, 89, 90, 91, 92, 93,
	94, 94, 95, 96, 96, 97, 97, 98, 98, 99,
	99, 100, 100, 101, 101, 101, 101, 101, 101, 101,
	101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
	101, 101, 101, 102, 103, 104, 105, 105, 106, 106,
	107, 108, 108, 109, 110, 110, 110, 111, 112, 113,
	114, 115, 115, 116, 117, 118, 119, 119, 120, 121,
	122, 123, 124, 124, 125, 126, 126, 126, 126, 126,
	126, 127, 127, 128, 129, 130, 131, 132, 133, 134,
	135, 136, 137, 137, 138, 139, 140, 141, 142, 142,
	143, 143, 144, 144, 145, 146
};

/* PLPGSQL_YYR2[PLPGSQL_YYN] -- Number of symbols composing right hand side of rule PLPGSQL_YYN.  */
static const unsigned char plpgsql_yyr2[] =
{
	0, 2, 4, 4, 0, 1, 2, 1, 2, 0,
	1, 5, 1, 2, 3, 1, 2, 1, 5, 1,
	1, 5, 3, 3, 5, 5, 0, 7, 0, 0,
	3, 1, 3, 2, 1, 1, 1, 1, 1, 1,
	0, 1, 0, 0, 2, 1, 1, 1, 1, 0,
	1, 2, 1, 2, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 3, 4, 5, 5, 3, 1, 1,
	1, 1, 3, 8, 0, 5, 2, 4, 5, 8,
	1, 1, 1, 0, 8, 8, 1, 1, 2, 4,
	2, 2, 6, 5, 1, 1, 1, 1, 1, 1,
	1, 2, 1, 2, 4, 2, 3, 3, 4, 4,
	1, 1, 1, 1, 0, 0, 0, 0, 0, 5,
	0, 1, 1, 2, 1, 0
};

/* PLPGSQL_YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when PLPGSQL_YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char plpgsql_yydefact[] =
{
	0, 4, 4, 0, 0, 128, 5, 7, 128, 1,
	8, 0, 9, 0, 12, 6, 9, 0, 10, 2,
	135, 15, 13, 3, 134, 0, 128, 19, 0, 38,
	0, 14, 17, 20, 40, 0, 135, 135, 135, 135,
	0, 135, 135, 135, 135, 135, 135, 135, 81, 122,
	123, 0, 0, 128, 52, 68, 54, 69, 135, 55,
	56, 57, 58, 59, 67, 60, 61, 62, 63, 64,
	65, 66, 70, 71, 72, 135, 12, 39, 0, 0,
	16, 0, 41, 26, 0, 37, 0, 42, 129, 0,
	124, 130, 0, 135, 126, 0, 124, 0, 100, 101,
	98, 53, 11, 51, 125, 0, 115, 135, 135, 135,
	0, 0, 0, 29, 22, 23, 43, 121, 0, 116,
	131, 0, 0, 0, 128, 120, 117, 73, 110, 105,
	108, 109, 107, 106, 0, 82, 124, 0, 128, 127,
	0, 0, 36, 0, 0, 0, 0, 0, 119, 124,
	132, 99, 118, 80, 0, 0, 84, 104, 0, 74,
	91, 97, 96, 92, 0, 90, 0, 0, 87, 128,
	25, 18, 24, 0, 31, 42, 35, 34, 0, 44,
	47, 48, 45, 21, 46, 133, 75, 0, 0, 128,
	135, 0, 103, 0, 0, 112, 93, 0, 0, 88,
	30, 0, 33, 28, 0, 78, 79, 77, 86, 126,
	0, 113, 102, 111, 127, 127, 127, 0, 32, 27,
	0, 128, 0, 128, 128, 128, 114, 76, 84, 83,
	89, 95, 94, 85
};

/* PLPGSQL_YYDEFGOTO[NTERM-NUM]. */
static const short plpgsql_yydefgoto[] =
{
	-1, 3, 5, 6, 7, 19, 51, 13, 22, 31,
	32, 33, 113, 219, 145, 173, 174, 178, 143, 86,
	34, 78, 87, 116, 147, 183, 184, 167, 53, 54,
	55, 56, 57, 154, 207, 155, 58, 59, 191, 60,
	61, 62, 164, 165, 214, 63, 64, 166, 65, 66,
	67, 68, 69, 158, 134, 194, 195, 168, 70, 71,
	72, 73, 74, 126, 118, 75, 119, 135, 124, 169,
	76, 121, 151, 25, 26
};

/* PLPGSQL_YYPACT[STATE-NUM] -- Index in PLPGSQL_YYTABLE of the portion describing
   STATE-NUM.  */
#define PLPGSQL_YYPACT_NINF -144
static const short plpgsql_yypact[] =
{
	47, -22, -22, 18, -14, -5, -22, -144, -5, -144,
	-144, 2, -13, 85, 91, -144, -13, 49, -144, -144,
	-144, -144, 5, -144, -144, 45, 124, -144, 58, -144,
	54, 5, -144, -144, 28, 55, -144, -144, -144, -144,
	109, -144, -144, -144, -144, -144, -144, -144, -144, -144,
	-144, 59, 107, 41, -144, -144, -144, -144, 56, -144,
	-144, -144, -144, -144, -144, -144, -144, -144, -144, -144,
	-144, -144, -144, -144, -144, -144, 12, -144, 82, 49,
	-144, 111, -144, -144, 69, -144, 71, -144, -144, 81,
	-144, 78, 81, -144, -144, 83, -144, 20, -144, -144,
	-144, -144, -144, -144, -144, 137, -144, -144, -144, -144,
	58, 77, 88, 86, -144, -144, 118, -144, 92, -144,
	-144, -45, 126, 102, 90, -144, -144, -144, -144, -144,
	-144, -144, -144, -144, 110, -144, -144, 31, 124, -144,
	96, 98, -144, 103, 112, -3, 136, 10, -144, -144,
	-144, -144, -144, -144, -46, 167, 101, -144, -37, -144,
	-144, -144, -144, -144, 147, -144, 149, 159, -144, 124,
	-144, -144, -144, 48, -144, -144, -144, -144, 131, -144,
	-144, -144, -144, -144, -144, -144, -144, 102, 7, 124,
	-144, 161, -144, 122, -31, -144, -144, -2, 151, -144,
	-144, 112, -144, -144, 175, -144, -144, -144, -144, -144,
	158, -144, -144, -144, -144, -144, -144, 119, -144, -144,
	7, 90, 123, 124, 124, 124, -144, -144, 101, -144,
	-144, -144, -144, -144
};

/* PLPGSQL_YYPGOTO[NTERM-NUM].	*/
static const short plpgsql_yypgoto[] =
{
	-144, -144, 181, -144, 183, 171, 22, -144, -144, -144,
	160, -144, -144, -144, -144, -144, -9, -144, -144, -144,
	-131, 84, -144, 21, -144, -144, -144, -26, -144, 140,
	-144, -144, -144, -144, -23, 11, -144, -144, -29, -144,
	-144, -144, -144, -144, -144, -144, -144, -144, -144, -144,
	-144, -144, -144, -144, -144, -144, 6, -143, -144, -144,
	-144, -144, -144, -144, 113, -144, -58, -144, -8, -121,
	46, -144, -144, 125, -35
};

/* PLPGSQL_YYTABLE[PLPGSQL_YYPACT[STATE-NUM]].	What to do in state STATE-NUM.	If
   positive, shift that token.	If negative, reduce the rule which
   number is the opposite.	If zero, do what PLPGSQL_YYDEFACT says.
   If PLPGSQL_YYTABLE_NINF, syntax error.  */
#define PLPGSQL_YYTABLE_NINF -51
static const short plpgsql_yytable[] =
{
	52, 89, 90, 91, 92, 149, 94, 95, 96, 97,
	98, 99, 100, 175, 180, 27, 215, 176, 9, 186,
	150, 181, 21, 105, 187, 177, 199, 12, 192, 128,
	16, 81, 107, 193, 212, 82, 83, 129, 127, 193,
	106, 4, 108, 216, 205, 28, 130, 36, 206, 131,
	10, 14, 18, 132, 14, -50, -50, -50, 123, 37,
	38, 11, 39, 109, 40, 41, 29, 84, 17, 133,
	175, 30, 137, 138, 139, 182, 42, 43, 159, 44,
	230, 231, 232, 45, 46, 85, 47, 160, 161, 162,
	20, 185, 163, 223, 224, 225, 36, 48, 156, 1,
	2, 21, 49, 50, -49, -49, -49, 11, 37, 38,
	24, 39, 35, 40, 41, 189, 190, 200, 201, 77,
	79, 93, 88, 102, 101, 42, 43, 104, 44, 110,
	36, 112, 45, 46, 114, 47, 115, 117, 120, 125,
	-49, 136, 37, 38, 141, 39, 48, 40, 41, 142,
	146, 49, 50, 152, 144, 209, 11, 148, 153, 42,
	43, 170, 44, 208, 157, 171, 45, 46, 172, 47,
	179, 188, 196, 29, 197, 198, 203, 210, 211, 220,
	48, 217, 222, 8, 226, 49, 50, 23, 229, 15,
	11, 80, 218, 103, 140, 228, 202, 227, 204, 233,
	213, 221, 0, 0, 111, 122
};

static const short plpgsql_yycheck[] =
{
	26, 36, 37, 38, 39, 50, 41, 42, 43, 44,
	45, 46, 47, 144, 4, 10, 18, 20, 0, 65,
	65, 11, 10, 58, 70, 28, 169, 5, 65, 9,
	8, 3, 20, 70, 65, 7, 8, 17, 96, 70,
	75, 63, 30, 45, 37, 40, 26, 6, 41, 29,
	64, 5, 65, 33, 8, 14, 15, 16, 93, 18,
	19, 66, 21, 51, 23, 24, 61, 39, 66, 49,
	201, 66, 107, 108, 109, 65, 35, 36, 136, 38,
	223, 224, 225, 42, 43, 57, 45, 56, 57, 58,
	5, 149, 61, 214, 215, 216, 6, 56, 124, 52,
	53, 10, 61, 62, 14, 15, 16, 66, 18, 19,
	61, 21, 67, 23, 24, 14, 15, 69, 70, 61,
	66, 12, 67, 16, 65, 35, 36, 71, 38, 47,
	6, 20, 42, 43, 65, 45, 65, 56, 60, 56,
	16, 4, 18, 19, 67, 21, 56, 23, 24, 61,
	32, 61, 62, 27, 68, 190, 66, 65, 56, 35,
	36, 65, 38, 189, 54, 67, 42, 43, 65, 45,
	34, 4, 25, 61, 25, 16, 45, 16, 56, 4,
	56, 30, 24, 2, 65, 61, 62, 16, 65, 6,
	66, 31, 201, 53, 110, 221, 175, 220, 187, 228,
	194, 209, -1, -1, 79, 92
};

/* PLPGSQL_YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char plpgsql_yystos[] =
{
	0, 52, 53, 73, 63, 74, 75, 76, 74, 0,
	64, 66, 78, 79, 142, 76, 78, 66, 65, 77,
	5, 10, 80, 77, 61, 145, 146, 10, 40, 61,
	66, 81, 82, 83, 92, 67, 6, 18, 19, 21,
	23, 24, 35, 36, 38, 42, 43, 45, 56, 61,
	62, 78, 99, 100, 101, 102, 103, 104, 108, 109,
	111, 112, 113, 117, 118, 120, 121, 122, 123, 124,
	130, 131, 132, 133, 134, 137, 142, 61, 93, 66,
	82, 3, 7, 8, 39, 57, 91, 94, 67, 146,
	146, 146, 146, 12, 146, 146, 146, 146, 146, 146,
	146, 65, 16, 101, 71, 146, 146, 20, 30, 51,
	47, 145, 20, 84, 65, 65, 95, 56, 136, 138,
	60, 143, 136, 146, 140, 56, 135, 138, 9, 17,
	26, 29, 33, 49, 126, 139, 4, 146, 146, 146,
	93, 67, 61, 90, 68, 86, 32, 96, 65, 50,
	65, 144, 27, 56, 105, 107, 99, 54, 125, 138,
	56, 57, 58, 61, 114, 115, 119, 99, 129, 141,
	65, 67, 65, 87, 88, 92, 20, 28, 89, 34,
	4, 11, 65, 97, 98, 138, 65, 70, 4, 14,
	15, 110, 65, 70, 127, 128, 25, 25, 16, 129,
	69, 70, 95, 45, 107, 37, 41, 106, 99, 146,
	16, 56, 65, 128, 116, 18, 45, 30, 88, 85,
	4, 140, 24, 141, 141, 141, 65, 106, 99, 65,
	129, 129, 129, 110
};

#if ! defined (PLPGSQL_YYSIZE_T) && defined (__SIZE_TYPE__)
#define PLPGSQL_YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (PLPGSQL_YYSIZE_T) && defined (size_t)
#define PLPGSQL_YYSIZE_T size_t
#endif
#if ! defined (PLPGSQL_YYSIZE_T)
#if defined (__STDC__) || defined (__cplusplus)
#include <stddef.h>				/* INFRINGES ON USER NAME SPACE */
#define PLPGSQL_YYSIZE_T size_t
#endif
#endif
#if ! defined (PLPGSQL_YYSIZE_T)
#define PLPGSQL_YYSIZE_T unsigned int
#endif

#define plpgsql_yyerrok		(plpgsql_yyerrstatus = 0)
#define plpgsql_yyclearin	(plpgsql_yychar = PLPGSQL_YYEMPTY)
#define PLPGSQL_YYEMPTY		(-2)
#define PLPGSQL_YYEOF		0

#define PLPGSQL_YYACCEPT	goto plpgsql_yyacceptlab
#define PLPGSQL_YYABORT		goto plpgsql_yyabortlab
#define PLPGSQL_YYERROR		goto plpgsql_yyerrlab1

/* Like PLPGSQL_YYERROR except do call plpgsql_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of PLPGSQL_YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define PLPGSQL_YYFAIL		goto plpgsql_yyerrlab

#define PLPGSQL_YYRECOVERING()	(!!plpgsql_yyerrstatus)

#define PLPGSQL_YYBACKUP(Token, Value)					\
do								\
  if (plpgsql_yychar == PLPGSQL_YYEMPTY && plpgsql_yylen == 1)				\
	{								\
	  plpgsql_yychar = (Token);						\
	  plpgsql_yylval = (Value);						\
	  plpgsql_yytoken = PLPGSQL_YYTRANSLATE (plpgsql_yychar);				\
	  PLPGSQL_YYPOPSTACK;						\
	  goto plpgsql_yybackup;						\
	}								\
  else								\
	{								\
	  plpgsql_yyerror ("syntax error: cannot back up");\
	  PLPGSQL_YYERROR;							\
	}								\
while (0)

#define PLPGSQL_YYTERROR	1
#define PLPGSQL_YYERRCODE	256

/* PLPGSQL_YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef PLPGSQL_YYLLOC_DEFAULT
#define PLPGSQL_YYLLOC_DEFAULT(Current, Rhs, N)			\
  Current.first_line   = Rhs[1].first_line;		 \
  Current.first_column = Rhs[1].first_column;	 \
  Current.last_line    = Rhs[N].last_line;		 \
  Current.last_column  = Rhs[N].last_column;
#endif

/* PLPGSQL_YYLEX -- calling `plpgsql_yylex' with the right arguments.  */

#ifdef PLPGSQL_YYLEX_PARAM
#define PLPGSQL_YYLEX plpgsql_yylex (PLPGSQL_YYLEX_PARAM)
#else
#define PLPGSQL_YYLEX plpgsql_yylex ()
#endif

/* Enable debugging if requested.  */
#if PLPGSQL_YYDEBUG

#ifndef PLPGSQL_YYFPRINTF
#include <stdio.h>				/* INFRINGES ON USER NAME SPACE */
#define PLPGSQL_YYFPRINTF fprintf
#endif

#define PLPGSQL_YYDPRINTF(Args)			   \
do {						\
  if (plpgsql_yydebug)					\
	PLPGSQL_YYFPRINTF Args;				\
} while (0)

#define PLPGSQL_YYDSYMPRINT(Args)		   \
do {						\
  if (plpgsql_yydebug)					\
	plpgsql_yysymprint Args;				\
} while (0)

#define PLPGSQL_YYDSYMPRINTF(Title, Token, Value, Location)		   \
do {								\
  if (plpgsql_yydebug)							\
	{								\
	  PLPGSQL_YYFPRINTF (stderr, "%s ", Title);				\
	  plpgsql_yysymprint (stderr,					\
				  Token, Value);	\
	  PLPGSQL_YYFPRINTF (stderr, "\n");					\
	}								\
} while (0)

/*------------------------------------------------------------------.
| plpgsql_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).													|
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
plpgsql_yy_stack_print(short *bottom, short *top)
#else
static void
plpgsql_yy_stack_print(bottom, top)
short	   *bottom;
short	   *top;
#endif
{
	PLPGSQL_YYFPRINTF(stderr, "Stack now");
	for ( /* Nothing. */ ; bottom <= top; ++bottom)
		PLPGSQL_YYFPRINTF(stderr, " %d", *bottom);
	PLPGSQL_YYFPRINTF(stderr, "\n");
}

#define PLPGSQL_YY_STACK_PRINT(Bottom, Top)				   \
do {								\
  if (plpgsql_yydebug)							\
	plpgsql_yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the PLPGSQL_YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
plpgsql_yy_reduce_print(int plpgsql_yyrule)
#else
static void
plpgsql_yy_reduce_print(plpgsql_yyrule)
int			plpgsql_yyrule;
#endif
{
	int			plpgsql_yyi;
	unsigned int plpgsql_yylineno = plpgsql_yyrline[plpgsql_yyrule];

	PLPGSQL_YYFPRINTF(stderr, "Reducing stack by rule %d (line %u), ",
					  plpgsql_yyrule - 1, plpgsql_yylineno);
	/* Print the symbols being reduced, and their result.  */
	for (plpgsql_yyi = plpgsql_yyprhs[plpgsql_yyrule]; 0 <= plpgsql_yyrhs[plpgsql_yyi]; plpgsql_yyi++)
		PLPGSQL_YYFPRINTF(stderr, "%s ", plpgsql_yytname[plpgsql_yyrhs[plpgsql_yyi]]);
	PLPGSQL_YYFPRINTF(stderr, "-> %s\n", plpgsql_yytname[plpgsql_yyr1[plpgsql_yyrule]]);
}

#define PLPGSQL_YY_REDUCE_PRINT(Rule)	   \
do {					\
  if (plpgsql_yydebug)				\
	plpgsql_yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int			plpgsql_yydebug;

#else							/* !PLPGSQL_YYDEBUG */
#define PLPGSQL_YYDPRINTF(Args)
#define PLPGSQL_YYDSYMPRINT(Args)
#define PLPGSQL_YYDSYMPRINTF(Title, Token, Value, Location)
#define PLPGSQL_YY_STACK_PRINT(Bottom, Top)
#define PLPGSQL_YY_REDUCE_PRINT(Rule)
#endif   /* !PLPGSQL_YYDEBUG */


/* PLPGSQL_YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef PLPGSQL_YYINITDEPTH
#define PLPGSQL_YYINITDEPTH 200
#endif

/* PLPGSQL_YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < PLPGSQL_YYSTACK_BYTES (PLPGSQL_YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if PLPGSQL_YYMAXDEPTH == 0
#undef PLPGSQL_YYMAXDEPTH
#endif

#ifndef PLPGSQL_YYMAXDEPTH
#define PLPGSQL_YYMAXDEPTH 10000
#endif




#if PLPGSQL_YYERROR_VERBOSE

#ifndef plpgsql_yystrlen
#if defined (__GLIBC__) && defined (_STRING_H)
#define plpgsql_yystrlen strlen
#else
/* Return the length of PLPGSQL_YYSTR.	*/
static PLPGSQL_YYSIZE_T
#if defined (__STDC__) || defined (__cplusplus)
plpgsql_yystrlen(const char *plpgsql_yystr)
#else
plpgsql_yystrlen(plpgsql_yystr)
const char *plpgsql_yystr;
#endif
{
	register const char *plpgsql_yys = plpgsql_yystr;

	while (*plpgsql_yys++ != '\0')
		continue;

	return plpgsql_yys - plpgsql_yystr - 1;
}
#endif
#endif

#ifndef plpgsql_yystpcpy
#if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#define plpgsql_yystpcpy stpcpy
#else
/* Copy PLPGSQL_YYSRC to PLPGSQL_YYDEST, returning the address of the terminating '\0' in
   PLPGSQL_YYDEST.	*/
static char *
#if defined (__STDC__) || defined (__cplusplus)
plpgsql_yystpcpy(char *plpgsql_yydest, const char *plpgsql_yysrc)
#else
plpgsql_yystpcpy(plpgsql_yydest, plpgsql_yysrc)
char	   *plpgsql_yydest;
const char *plpgsql_yysrc;
#endif
{
	register char *plpgsql_yyd = plpgsql_yydest;
	register const char *plpgsql_yys = plpgsql_yysrc;

	while ((*plpgsql_yyd++ = *plpgsql_yys++) != '\0')
		continue;

	return plpgsql_yyd - 1;
}
#endif
#endif
#endif   /* !PLPGSQL_YYERROR_VERBOSE */




#if PLPGSQL_YYDEBUG
/*--------------------------------.
| Print this symbol on PLPGSQL_YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
plpgsql_yysymprint(FILE *plpgsql_yyoutput, int plpgsql_yytype, PLPGSQL_YYSTYPE * plpgsql_yyvaluep)
#else
static void
plpgsql_yysymprint(plpgsql_yyoutput, plpgsql_yytype, plpgsql_yyvaluep)
FILE	   *plpgsql_yyoutput;
int			plpgsql_yytype;
PLPGSQL_YYSTYPE *plpgsql_yyvaluep;
#endif
{
	/* Pacify ``unused variable'' warnings.  */
	(void) plpgsql_yyvaluep;

	if (plpgsql_yytype < PLPGSQL_YYNTOKENS)
	{
		PLPGSQL_YYFPRINTF(plpgsql_yyoutput, "token %s (", plpgsql_yytname[plpgsql_yytype]);
#ifdef PLPGSQL_YYPRINT
		PLPGSQL_YYPRINT(plpgsql_yyoutput, plpgsql_yytoknum[plpgsql_yytype], *plpgsql_yyvaluep);
#endif
	}
	else
		PLPGSQL_YYFPRINTF(plpgsql_yyoutput, "nterm %s (", plpgsql_yytname[plpgsql_yytype]);

	switch (plpgsql_yytype)
	{
		default:
			break;
	}
	PLPGSQL_YYFPRINTF(plpgsql_yyoutput, ")");
}
#endif   /* ! PLPGSQL_YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
plpgsql_yydestruct(int plpgsql_yytype, PLPGSQL_YYSTYPE * plpgsql_yyvaluep)
#else
static void
plpgsql_yydestruct(plpgsql_yytype, plpgsql_yyvaluep)
int			plpgsql_yytype;
PLPGSQL_YYSTYPE *plpgsql_yyvaluep;
#endif
{
	/* Pacify ``unused variable'' warnings.  */
	(void) plpgsql_yyvaluep;

	switch (plpgsql_yytype)
	{

		default:
			break;
	}
}



/* Prevent warnings from -Wmissing-prototypes.	*/

#ifdef PLPGSQL_YYPARSE_PARAM
#if defined (__STDC__) || defined (__cplusplus)
int			plpgsql_yyparse(void *PLPGSQL_YYPARSE_PARAM);

#else
int			plpgsql_yyparse();
#endif
#else							/* ! PLPGSQL_YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int			plpgsql_yyparse(void);

#else
int			plpgsql_yyparse();
#endif
#endif   /* ! PLPGSQL_YYPARSE_PARAM */



/* The lookahead symbol.  */
int			plpgsql_yychar;

/* The semantic value of the lookahead symbol.	*/
PLPGSQL_YYSTYPE plpgsql_yylval;

/* Number of syntax errors so far.	*/
int			plpgsql_yynerrs;



/*----------.
| plpgsql_yyparse.	|
`----------*/

#ifdef PLPGSQL_YYPARSE_PARAM
#if defined (__STDC__) || defined (__cplusplus)
int
plpgsql_yyparse(void *PLPGSQL_YYPARSE_PARAM)
#else
int
plpgsql_yyparse(PLPGSQL_YYPARSE_PARAM)
void	   *PLPGSQL_YYPARSE_PARAM;
#endif
#else							/* ! PLPGSQL_YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
plpgsql_yyparse(void)
#else
int
plpgsql_yyparse()
#endif
#endif
{

	register int plpgsql_yystate;
	register int plpgsql_yyn;
	int			plpgsql_yyresult;

	/* Number of tokens to shift before error messages enabled.  */
	int			plpgsql_yyerrstatus;

	/* Lookahead token as an internal (translated) token number.  */
	int			plpgsql_yytoken = 0;

	/*
	 * Three stacks and their tools: `plpgsql_yyss': related to states,
	 * `plpgsql_yyvs': related to semantic values, `plpgsql_yyls': related
	 * to locations.
	 *
	 * Refer to the stacks thru separate pointers, to allow
	 * plpgsql_yyoverflow to reallocate them elsewhere.
	 */

	/* The state stack.  */
	short		plpgsql_yyssa[PLPGSQL_YYINITDEPTH];
	short	   *plpgsql_yyss = plpgsql_yyssa;
	register short *plpgsql_yyssp;

	/* The semantic value stack.  */
	PLPGSQL_YYSTYPE plpgsql_yyvsa[PLPGSQL_YYINITDEPTH];
	PLPGSQL_YYSTYPE *plpgsql_yyvs = plpgsql_yyvsa;
	register PLPGSQL_YYSTYPE *plpgsql_yyvsp;



#define PLPGSQL_YYPOPSTACK	 (plpgsql_yyvsp--, plpgsql_yyssp--)

	PLPGSQL_YYSIZE_T plpgsql_yystacksize = PLPGSQL_YYINITDEPTH;

	/*
	 * The variables used to return semantic value and location from the
	 * action routines.
	 */
	PLPGSQL_YYSTYPE plpgsql_yyval;


	/*
	 * When reducing, the number of symbols on the RHS of the reduced
	 * rule.
	 */
	int			plpgsql_yylen;

	PLPGSQL_YYDPRINTF((stderr, "Starting parse\n"));

	plpgsql_yystate = 0;
	plpgsql_yyerrstatus = 0;
	plpgsql_yynerrs = 0;
	plpgsql_yychar = PLPGSQL_YYEMPTY;	/* Cause a token to be read.  */

	/*
	 * Initialize stack pointers. Waste one element of value and location
	 * stack so that they stay on the same level as the state stack. The
	 * wasted elements are never initialized.
	 */

	plpgsql_yyssp = plpgsql_yyss;
	plpgsql_yyvsp = plpgsql_yyvs;

	goto plpgsql_yysetstate;

/*------------------------------------------------------------.
| plpgsql_yynewstate -- Push a new state, which is found in plpgsql_yystate.  |
`------------------------------------------------------------*/
plpgsql_yynewstate:

	/*
	 * In all cases, when you get here, the value and location stacks have
	 * just been pushed. so pushing a state here evens the stacks.
	 */
	plpgsql_yyssp++;

plpgsql_yysetstate:
	*plpgsql_yyssp = plpgsql_yystate;

	if (plpgsql_yyss + plpgsql_yystacksize - 1 <= plpgsql_yyssp)
	{
		/* Get the current used size of the three stacks, in elements.	*/
		PLPGSQL_YYSIZE_T plpgsql_yysize = plpgsql_yyssp - plpgsql_yyss + 1;

#ifdef plpgsql_yyoverflow
		{
			/*
			 * Give user a chance to reallocate the stack. Use copies of
			 * these so that the &'s don't force the real ones into
			 * memory.
			 */
			PLPGSQL_YYSTYPE *plpgsql_yyvs1 = plpgsql_yyvs;
			short	   *plpgsql_yyss1 = plpgsql_yyss;


			/*
			 * Each stack pointer address is followed by the size of the
			 * data in use in that stack, in bytes.  This used to be a
			 * conditional around just the two extra args, but that might
			 * be undefined if plpgsql_yyoverflow is a macro.
			 */
			plpgsql_yyoverflow("parser stack overflow",
				 &plpgsql_yyss1, plpgsql_yysize * sizeof(*plpgsql_yyssp),
				 &plpgsql_yyvs1, plpgsql_yysize * sizeof(*plpgsql_yyvsp),

							   &plpgsql_yystacksize);

			plpgsql_yyss = plpgsql_yyss1;
			plpgsql_yyvs = plpgsql_yyvs1;
		}
#else							/* no plpgsql_yyoverflow */
#ifndef PLPGSQL_YYSTACK_RELOCATE
		goto plpgsql_yyoverflowlab;
#else
		/* Extend the stack our own way.  */
		if (PLPGSQL_YYMAXDEPTH <= plpgsql_yystacksize)
			goto plpgsql_yyoverflowlab;
		plpgsql_yystacksize *= 2;
		if (PLPGSQL_YYMAXDEPTH < plpgsql_yystacksize)
			plpgsql_yystacksize = PLPGSQL_YYMAXDEPTH;

		{
			short	   *plpgsql_yyss1 = plpgsql_yyss;
			union plpgsql_yyalloc *plpgsql_yyptr =
			(union plpgsql_yyalloc *) PLPGSQL_YYSTACK_ALLOC(PLPGSQL_YYSTACK_BYTES(plpgsql_yystacksize));

			if (!plpgsql_yyptr)
				goto plpgsql_yyoverflowlab;
			PLPGSQL_YYSTACK_RELOCATE(plpgsql_yyss);
			PLPGSQL_YYSTACK_RELOCATE(plpgsql_yyvs);

#undef PLPGSQL_YYSTACK_RELOCATE
			if (plpgsql_yyss1 != plpgsql_yyssa)
				PLPGSQL_YYSTACK_FREE(plpgsql_yyss1);
		}
#endif
#endif   /* no plpgsql_yyoverflow */

		plpgsql_yyssp = plpgsql_yyss + plpgsql_yysize - 1;
		plpgsql_yyvsp = plpgsql_yyvs + plpgsql_yysize - 1;


		PLPGSQL_YYDPRINTF((stderr, "Stack size increased to %lu\n",
						   (unsigned long int) plpgsql_yystacksize));

		if (plpgsql_yyss + plpgsql_yystacksize - 1 <= plpgsql_yyssp)
			PLPGSQL_YYABORT;
	}

	PLPGSQL_YYDPRINTF((stderr, "Entering state %d\n", plpgsql_yystate));

	goto plpgsql_yybackup;

/*-----------.
| plpgsql_yybackup.  |
`-----------*/
plpgsql_yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* plpgsql_yyresume: */

	/*
	 * First try to decide what to do without reference to lookahead
	 * token.
	 */

	plpgsql_yyn = plpgsql_yypact[plpgsql_yystate];
	if (plpgsql_yyn == PLPGSQL_YYPACT_NINF)
		goto plpgsql_yydefault;

	/* Not known => get a lookahead token if don't already have one.  */

	/*
	 * PLPGSQL_YYCHAR is either PLPGSQL_YYEMPTY or PLPGSQL_YYEOF or a
	 * valid lookahead symbol.
	 */
	if (plpgsql_yychar == PLPGSQL_YYEMPTY)
	{
		PLPGSQL_YYDPRINTF((stderr, "Reading a token: "));
		plpgsql_yychar = PLPGSQL_YYLEX;
	}

	if (plpgsql_yychar <= PLPGSQL_YYEOF)
	{
		plpgsql_yychar = plpgsql_yytoken = PLPGSQL_YYEOF;
		PLPGSQL_YYDPRINTF((stderr, "Now at end of input.\n"));
	}
	else
	{
		plpgsql_yytoken = PLPGSQL_YYTRANSLATE(plpgsql_yychar);
		PLPGSQL_YYDSYMPRINTF("Next token is", plpgsql_yytoken, &plpgsql_yylval, &plpgsql_yylloc);
	}

	/*
	 * If the proper action on seeing token PLPGSQL_YYTOKEN is to reduce
	 * or to detect an error, take that action.
	 */
	plpgsql_yyn += plpgsql_yytoken;
	if (plpgsql_yyn < 0 || PLPGSQL_YYLAST < plpgsql_yyn || plpgsql_yycheck[plpgsql_yyn] != plpgsql_yytoken)
		goto plpgsql_yydefault;
	plpgsql_yyn = plpgsql_yytable[plpgsql_yyn];
	if (plpgsql_yyn <= 0)
	{
		if (plpgsql_yyn == 0 || plpgsql_yyn == PLPGSQL_YYTABLE_NINF)
			goto plpgsql_yyerrlab;
		plpgsql_yyn = -plpgsql_yyn;
		goto plpgsql_yyreduce;
	}

	if (plpgsql_yyn == PLPGSQL_YYFINAL)
		PLPGSQL_YYACCEPT;

	/* Shift the lookahead token.  */
	PLPGSQL_YYDPRINTF((stderr, "Shifting token %s, ", plpgsql_yytname[plpgsql_yytoken]));

	/* Discard the token being shifted unless it is eof.  */
	if (plpgsql_yychar != PLPGSQL_YYEOF)
		plpgsql_yychar = PLPGSQL_YYEMPTY;

	*++plpgsql_yyvsp = plpgsql_yylval;


	/*
	 * Count tokens shifted since error; after three, turn off error
	 * status.
	 */
	if (plpgsql_yyerrstatus)
		plpgsql_yyerrstatus--;

	plpgsql_yystate = plpgsql_yyn;
	goto plpgsql_yynewstate;


/*-----------------------------------------------------------.
| plpgsql_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
plpgsql_yydefault:
	plpgsql_yyn = plpgsql_yydefact[plpgsql_yystate];
	if (plpgsql_yyn == 0)
		goto plpgsql_yyerrlab;
	goto plpgsql_yyreduce;


/*-----------------------------.
| plpgsql_yyreduce -- Do a reduction.  |
`-----------------------------*/
plpgsql_yyreduce:
	/* plpgsql_yyn is the number of a rule to reduce with.	*/
	plpgsql_yylen = plpgsql_yyr2[plpgsql_yyn];

	/*
	 * If PLPGSQL_YYLEN is nonzero, implement the default value of the
	 * action: `$$ = $1'.
	 *
	 * Otherwise, the following line sets PLPGSQL_YYVAL to garbage. This
	 * behavior is undocumented and Bison users should not rely upon it.
	 * Assigning to PLPGSQL_YYVAL unconditionally makes the parser a bit
	 * smaller, and it avoids a GCC warning that PLPGSQL_YYVAL may be used
	 * uninitialized.
	 */
	plpgsql_yyval = plpgsql_yyvsp[1 - plpgsql_yylen];


	PLPGSQL_YY_REDUCE_PRINT(plpgsql_yyn);
	switch (plpgsql_yyn)
	{
		case 2:
#line 213 "gram.y"
				plpgsql_yylval.program = (PLpgSQL_stmt_block *) plpgsql_yyvsp[-1].stmt;
			break;

		case 3:
#line 217 "gram.y"
				plpgsql_yylval.program = (PLpgSQL_stmt_block *) plpgsql_yyvsp[-1].stmt;
			break;

		case 8:
#line 231 "gram.y"
				plpgsql_DumpExecTree = 1;
			break;

		case 11:
#line 241 "gram.y"
			{
				PLpgSQL_stmt_block *new;

				new = malloc(sizeof(PLpgSQL_stmt_block));
				memset(new, 0, sizeof(PLpgSQL_stmt_block));

				new->cmd_type = PLPGSQL_STMT_BLOCK;
				new->lineno = plpgsql_yyvsp[-2].ival;
				new->label = plpgsql_yyvsp[-4].declhdr.label;
				new->n_initvars = plpgsql_yyvsp[-4].declhdr.n_initvars;
				new->initvarnos = plpgsql_yyvsp[-4].declhdr.initvarnos;
				new->body = plpgsql_yyvsp[-1].stmts;

				plpgsql_ns_pop();

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 12:
#line 262 "gram.y"
			{
				plpgsql_ns_setlocal(false);
				plpgsql_yyval.declhdr.label = plpgsql_yyvsp[0].str;
				plpgsql_yyval.declhdr.n_initvars = 0;
				plpgsql_yyval.declhdr.initvarnos = NULL;
				plpgsql_add_initdatums(NULL);
			}
			break;

		case 13:
#line 270 "gram.y"
			{
				plpgsql_ns_setlocal(false);
				plpgsql_yyval.declhdr.label = plpgsql_yyvsp[-1].str;
				plpgsql_yyval.declhdr.n_initvars = 0;
				plpgsql_yyval.declhdr.initvarnos = NULL;
				plpgsql_add_initdatums(NULL);
			}
			break;

		case 14:
#line 278 "gram.y"
			{
				plpgsql_ns_setlocal(false);
				if (plpgsql_yyvsp[0].str != NULL)
					plpgsql_yyval.declhdr.label = plpgsql_yyvsp[0].str;
				else
					plpgsql_yyval.declhdr.label = plpgsql_yyvsp[-2].str;
				plpgsql_yyval.declhdr.n_initvars = plpgsql_add_initdatums(&(plpgsql_yyval.declhdr.initvarnos));
			}
			break;

		case 15:
#line 289 "gram.y"
				plpgsql_ns_setlocal(true);
			break;

		case 16:
#line 295 "gram.y"
				plpgsql_yyval.str = plpgsql_yyvsp[0].str;
			break;

		case 17:
#line 297 "gram.y"
				plpgsql_yyval.str = plpgsql_yyvsp[0].str;
			break;

		case 18:
#line 301 "gram.y"
				plpgsql_yyval.str = plpgsql_yyvsp[-2].str;
			break;

		case 19:
#line 303 "gram.y"
				plpgsql_yyval.str = NULL;
			break;

		case 20:
#line 305 "gram.y"
				plpgsql_yyval.str = NULL;
			break;

		case 21:
#line 309 "gram.y"
			{
				if (!OidIsValid(plpgsql_yyvsp[-2].dtype->typrelid))
				{
					/* Ordinary scalar datatype */
					PLpgSQL_var *var;

					var = malloc(sizeof(PLpgSQL_var));
					memset(var, 0, sizeof(PLpgSQL_var));

					var->dtype = PLPGSQL_DTYPE_VAR;
					var->refname = plpgsql_yyvsp[-4].varname.name;
					var->lineno = plpgsql_yyvsp[-4].varname.lineno;

					var->datatype = plpgsql_yyvsp[-2].dtype;
					var->isconst = plpgsql_yyvsp[-3].ival;
					var->notnull = plpgsql_yyvsp[-1].ival;
					var->default_val = plpgsql_yyvsp[0].expr;

					plpgsql_adddatum((PLpgSQL_datum *) var);
					plpgsql_ns_additem(PLPGSQL_NSTYPE_VAR,
									   var->varno,
									   plpgsql_yyvsp[-4].varname.name);
				}
				else
				{
					/* Composite type --- treat as rowtype */
					PLpgSQL_row *row;

					row = plpgsql_build_rowtype(plpgsql_yyvsp[-2].dtype->typrelid);
					row->dtype = PLPGSQL_DTYPE_ROW;
					row->refname = plpgsql_yyvsp[-4].varname.name;
					row->lineno = plpgsql_yyvsp[-4].varname.lineno;

					if (plpgsql_yyvsp[-3].ival)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("rowtype variable cannot be CONSTANT")));
					if (plpgsql_yyvsp[-1].ival)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("rowtype variable cannot be NOT NULL")));
					if (plpgsql_yyvsp[0].expr != NULL)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("default value for rowtype variable is not supported")));

					plpgsql_adddatum((PLpgSQL_datum *) row);
					plpgsql_ns_additem(PLPGSQL_NSTYPE_ROW,
									   row->rowno,
									   plpgsql_yyvsp[-4].varname.name);

				}
			}
			break;

		case 22:
#line 363 "gram.y"
			{
				PLpgSQL_rec *var;

				var = malloc(sizeof(PLpgSQL_rec));

				var->dtype = PLPGSQL_DTYPE_REC;
				var->refname = plpgsql_yyvsp[-2].varname.name;
				var->lineno = plpgsql_yyvsp[-2].varname.lineno;

				plpgsql_adddatum((PLpgSQL_datum *) var);
				plpgsql_ns_additem(PLPGSQL_NSTYPE_REC, var->recno,
								   plpgsql_yyvsp[-2].varname.name);
			}
			break;

		case 23:
#line 377 "gram.y"
			{
				plpgsql_yyvsp[-1].row->dtype = PLPGSQL_DTYPE_ROW;
				plpgsql_yyvsp[-1].row->refname = plpgsql_yyvsp[-2].varname.name;
				plpgsql_yyvsp[-1].row->lineno = plpgsql_yyvsp[-2].varname.lineno;

				plpgsql_adddatum((PLpgSQL_datum *) plpgsql_yyvsp[-1].row);
				plpgsql_ns_additem(PLPGSQL_NSTYPE_ROW, plpgsql_yyvsp[-1].row->rowno,
								   plpgsql_yyvsp[-2].varname.name);
			}
			break;

		case 24:
#line 387 "gram.y"
			{
				plpgsql_ns_additem(plpgsql_yyvsp[-1].nsitem->itemtype,
								   plpgsql_yyvsp[-1].nsitem->itemno, plpgsql_yyvsp[-4].varname.name);
			}
			break;

		case 25:
#line 392 "gram.y"
				plpgsql_ns_rename(plpgsql_yyvsp[-3].str, plpgsql_yyvsp[-1].str);
			break;

		case 26:
#line 396 "gram.y"
				plpgsql_ns_push(NULL);
			break;

		case 27:
#line 398 "gram.y"
			{
				PLpgSQL_var *new;
				PLpgSQL_expr *curname_def;
				char		buf[1024];
				char	   *cp1;
				char	   *cp2;

				/* pop local namespace for cursor args */
				plpgsql_ns_pop();

				new = malloc(sizeof(PLpgSQL_var));
				memset(new, 0, sizeof(PLpgSQL_var));

				curname_def = malloc(sizeof(PLpgSQL_expr));
				memset(curname_def, 0, sizeof(PLpgSQL_expr));

				new->dtype = PLPGSQL_DTYPE_VAR;
				new->refname = plpgsql_yyvsp[-6].varname.name;
				new->lineno = plpgsql_yyvsp[-6].varname.lineno;

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

				new->datatype = plpgsql_parse_datatype("refcursor");

				new->cursor_explicit_expr = plpgsql_yyvsp[0].expr;
				if (plpgsql_yyvsp[-3].row == NULL)
					new->cursor_explicit_argrow = -1;
				else
					new->cursor_explicit_argrow = plpgsql_yyvsp[-3].row->rowno;

				plpgsql_adddatum((PLpgSQL_datum *) new);
				plpgsql_ns_additem(PLPGSQL_NSTYPE_VAR, new->varno,
								   plpgsql_yyvsp[-6].varname.name);
			}
			break;

		case 28:
#line 447 "gram.y"
			{
				PLpgSQL_expr *query;

				plpgsql_ns_setlocal(false);
				query = read_sql_stmt("SELECT ");
				plpgsql_ns_setlocal(true);

				plpgsql_yyval.expr = query;
			}
			break;

		case 29:
#line 459 "gram.y"
				plpgsql_yyval.row = NULL;
			break;

		case 30:
#line 463 "gram.y"
			{
				/* Copy the temp arrays to malloc'd storage */
				int			nfields = plpgsql_yyvsp[-1].row->nfields;
				char	  **ftmp;
				int		   *vtmp;

				ftmp = malloc(nfields * sizeof(char *));
				vtmp = malloc(nfields * sizeof(int));
				memcpy(ftmp, plpgsql_yyvsp[-1].row->fieldnames, nfields * sizeof(char *));
				memcpy(vtmp, plpgsql_yyvsp[-1].row->varnos, nfields * sizeof(int));

				pfree((char *) (plpgsql_yyvsp[-1].row->fieldnames));
				pfree((char *) (plpgsql_yyvsp[-1].row->varnos));

				plpgsql_yyvsp[-1].row->fieldnames = ftmp;
				plpgsql_yyvsp[-1].row->varnos = vtmp;

				plpgsql_adddatum((PLpgSQL_datum *) plpgsql_yyvsp[-1].row);

				plpgsql_yyval.row = plpgsql_yyvsp[-1].row;
			}
			break;

		case 31:
#line 487 "gram.y"
			{
				PLpgSQL_row *new;

				new = malloc(sizeof(PLpgSQL_row));
				memset(new, 0, sizeof(PLpgSQL_row));

				new->dtype = PLPGSQL_DTYPE_ROW;
				new->refname = strdup("*internal*");
				new->lineno = plpgsql_scanner_lineno();
				new->rowtypeclass = InvalidOid;

				/*
				 * We make temporary fieldnames/varnos arrays that are
				 * much bigger than necessary.	We will resize them to
				 * just the needed size in the decl_cursor_args
				 * production.
				 */
				new->fieldnames = palloc(1024 * sizeof(char *));
				new->varnos = palloc(1024 * sizeof(int));
				new->nfields = 1;

				new->fieldnames[0] = plpgsql_yyvsp[0].var->refname;
				new->varnos[0] = plpgsql_yyvsp[0].var->varno;

				plpgsql_yyval.row = new;
			}
			break;

		case 32:
#line 513 "gram.y"
			{
				int			i = plpgsql_yyvsp[-2].row->nfields++;

				plpgsql_yyvsp[-2].row->fieldnames[i] = plpgsql_yyvsp[0].var->refname;
				plpgsql_yyvsp[-2].row->varnos[i] = plpgsql_yyvsp[0].var->varno;

				plpgsql_yyval.row = plpgsql_yyvsp[-2].row;
			}
			break;

		case 33:
#line 524 "gram.y"
			{
				PLpgSQL_var *new;

				new = malloc(sizeof(PLpgSQL_var));
				memset(new, 0, sizeof(PLpgSQL_var));

				new->dtype = PLPGSQL_DTYPE_VAR;
				new->refname = plpgsql_yyvsp[-1].varname.name;
				new->lineno = plpgsql_yyvsp[-1].varname.lineno;

				new->datatype = plpgsql_yyvsp[0].dtype;
				new->isconst = false;
				new->notnull = false;

				plpgsql_adddatum((PLpgSQL_datum *) new);
				plpgsql_ns_additem(PLPGSQL_NSTYPE_VAR, new->varno,
								   plpgsql_yyvsp[-1].varname.name);

				plpgsql_yyval.var = new;
			}
			break;

		case 36:
#line 550 "gram.y"
			{
				char	   *name;
				PLpgSQL_nsitem *nsi;

				plpgsql_convert_ident(plpgsql_yytext, &name, 1);
				if (name[0] != '$')
					plpgsql_yyerror("only positional parameters may be aliased");

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

				plpgsql_yyval.nsitem = nsi;
			}
			break;

		case 37:
#line 578 "gram.y"
				plpgsql_yyval.row = plpgsql_yylval.row;
			break;

		case 38:
#line 584 "gram.y"
			{
				char	   *name;

				plpgsql_convert_ident(plpgsql_yytext, &name, 1);
				/* name should be malloc'd for use as varname */
				plpgsql_yyval.varname.name = strdup(name);
				plpgsql_yyval.varname.lineno = plpgsql_scanner_lineno();
				pfree(name);
			}
			break;

		case 39:
#line 596 "gram.y"
			{
				char	   *name;

				plpgsql_convert_ident(plpgsql_yytext, &name, 1);
				/* the result must be palloc'd, see plpgsql_ns_rename */
				plpgsql_yyval.str = name;
			}
			break;

		case 40:
#line 606 "gram.y"
				plpgsql_yyval.ival = 0;
			break;

		case 41:
#line 608 "gram.y"
				plpgsql_yyval.ival = 1;
			break;

		case 42:
#line 612 "gram.y"
			{
				/*
				 * If there's a lookahead token, read_datatype should
				 * consume it.
				 */
				plpgsql_yyval.dtype = read_datatype(plpgsql_yychar);
				plpgsql_yyclearin;
			}
			break;

		case 43:
#line 623 "gram.y"
				plpgsql_yyval.ival = 0;
			break;

		case 44:
#line 625 "gram.y"
				plpgsql_yyval.ival = 1;
			break;

		case 45:
#line 629 "gram.y"
				plpgsql_yyval.expr = NULL;
			break;

		case 46:
#line 631 "gram.y"
			{
				int			tok;
				int			lno;
				PLpgSQL_dstring ds;
				PLpgSQL_expr *expr;

				lno = plpgsql_scanner_lineno();
				expr = malloc(sizeof(PLpgSQL_expr));
				plpgsql_dstring_init(&ds);
				plpgsql_dstring_append(&ds, "SELECT ");

				expr->dtype = PLPGSQL_DTYPE_EXPR;
				expr->plan = NULL;
				expr->nparams = 0;

				tok = plpgsql_yylex();
				switch (tok)
				{
					case 0:
						plpgsql_yyerror("unexpected end of function");
					case K_NULL:
						if (plpgsql_yylex() != ';')
							plpgsql_yyerror("expected \";\" after \"NULL\"");

						free(expr);
						plpgsql_dstring_free(&ds);

						plpgsql_yyval.expr = NULL;
						break;

					default:
						plpgsql_dstring_append(&ds, plpgsql_yytext);
						while ((tok = plpgsql_yylex()) != ';')
						{
							if (tok == 0)
								plpgsql_yyerror("unterminated default value");

							if (plpgsql_SpaceScanned)
								plpgsql_dstring_append(&ds, " ");
							plpgsql_dstring_append(&ds, plpgsql_yytext);
						}
						expr->query = strdup(plpgsql_dstring_get(&ds));
						plpgsql_dstring_free(&ds);

						plpgsql_yyval.expr = expr;
						break;
				}
			}
			break;

		case 49:
#line 686 "gram.y"
			{
				PLpgSQL_stmts *new;

				new = malloc(sizeof(PLpgSQL_stmts));
				memset(new, 0, sizeof(PLpgSQL_stmts));
				plpgsql_yyval.stmts = new;
			}
			break;

		case 50:
#line 694 "gram.y"
				plpgsql_yyval.stmts = plpgsql_yyvsp[0].stmts;
			break;

		case 51:
#line 698 "gram.y"
			{
				if (plpgsql_yyvsp[-1].stmts->stmts_used == plpgsql_yyvsp[-1].stmts->stmts_alloc)
				{
					plpgsql_yyvsp[-1].stmts->stmts_alloc *= 2;
					plpgsql_yyvsp[-1].stmts->stmts = realloc(plpgsql_yyvsp[-1].stmts->stmts, sizeof(PLpgSQL_stmt *) * plpgsql_yyvsp[-1].stmts->stmts_alloc);
				}
				plpgsql_yyvsp[-1].stmts->stmts[plpgsql_yyvsp[-1].stmts->stmts_used++] = (struct PLpgSQL_stmt *) plpgsql_yyvsp[0].stmt;

				plpgsql_yyval.stmts = plpgsql_yyvsp[-1].stmts;
			}
			break;

		case 52:
#line 709 "gram.y"
			{
				PLpgSQL_stmts *new;

				new = malloc(sizeof(PLpgSQL_stmts));
				memset(new, 0, sizeof(PLpgSQL_stmts));

				new->stmts_alloc = 64;
				new->stmts_used = 1;
				new->stmts = malloc(sizeof(PLpgSQL_stmt *) * new->stmts_alloc);
				new->stmts[0] = (struct PLpgSQL_stmt *) plpgsql_yyvsp[0].stmt;

				plpgsql_yyval.stmts = new;

			}
			break;

		case 53:
#line 726 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[-1].stmt;
			break;

		case 54:
#line 728 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 55:
#line 730 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 56:
#line 732 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 57:
#line 734 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 58:
#line 736 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 59:
#line 738 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 60:
#line 740 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 61:
#line 742 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 62:
#line 744 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 63:
#line 746 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 64:
#line 748 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 65:
#line 750 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 66:
#line 752 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 67:
#line 754 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 68:
#line 756 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 69:
#line 758 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 70:
#line 760 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 71:
#line 762 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 72:
#line 764 "gram.y"
				plpgsql_yyval.stmt = plpgsql_yyvsp[0].stmt;
			break;

		case 73:
#line 768 "gram.y"
			{
				PLpgSQL_stmt_perform *new;

				new = malloc(sizeof(PLpgSQL_stmt_perform));
				memset(new, 0, sizeof(PLpgSQL_stmt_perform));

				new->cmd_type = PLPGSQL_STMT_PERFORM;
				new->lineno = plpgsql_yyvsp[-1].ival;
				new->expr = plpgsql_yyvsp[0].expr;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 74:
#line 783 "gram.y"
			{
				PLpgSQL_stmt_assign *new;

				new = malloc(sizeof(PLpgSQL_stmt_assign));
				memset(new, 0, sizeof(PLpgSQL_stmt_assign));

				new->cmd_type = PLPGSQL_STMT_ASSIGN;
				new->lineno = plpgsql_yyvsp[-2].ival;
				new->varno = plpgsql_yyvsp[-3].ival;
				new->expr = plpgsql_yyvsp[0].expr;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 75:
#line 799 "gram.y"
			{
				PLpgSQL_stmt_getdiag *new;

				new = malloc(sizeof(PLpgSQL_stmt_getdiag));
				memset(new, 0, sizeof(PLpgSQL_stmt_getdiag));

				new->cmd_type = PLPGSQL_STMT_GETDIAG;
				new->lineno = plpgsql_yyvsp[-2].ival;
				new->ndtitems = plpgsql_yyvsp[-1].dtlist.nused;
				new->dtitems = malloc(sizeof(PLpgSQL_diag_item) * plpgsql_yyvsp[-1].dtlist.nused);
				memcpy(new->dtitems, plpgsql_yyvsp[-1].dtlist.dtitems, sizeof(PLpgSQL_diag_item) * plpgsql_yyvsp[-1].dtlist.nused);

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 76:
#line 816 "gram.y"
			{
				if (plpgsql_yyvsp[-4].dtlist.nused == plpgsql_yyvsp[-4].dtlist.nalloc)
				{
					plpgsql_yyvsp[-4].dtlist.nalloc *= 2;
					plpgsql_yyvsp[-4].dtlist.dtitems = repalloc(plpgsql_yyvsp[-4].dtlist.dtitems, sizeof(PLpgSQL_diag_item) * plpgsql_yyvsp[-4].dtlist.nalloc);
				}
				plpgsql_yyvsp[-4].dtlist.dtitems[plpgsql_yyvsp[-4].dtlist.nused].target = plpgsql_yyvsp[-2].ival;
				plpgsql_yyvsp[-4].dtlist.dtitems[plpgsql_yyvsp[-4].dtlist.nused].item = plpgsql_yyvsp[0].ival;
				plpgsql_yyvsp[-4].dtlist.nused++;

				plpgsql_yyval.dtlist.nalloc = plpgsql_yyvsp[-4].dtlist.nalloc;
				plpgsql_yyval.dtlist.nused = plpgsql_yyvsp[-4].dtlist.nused;
				plpgsql_yyval.dtlist.dtitems = plpgsql_yyvsp[-4].dtlist.dtitems;
			}
			break;

		case 77:
#line 831 "gram.y"
			{
				plpgsql_yyval.dtlist.nalloc = 1;
				plpgsql_yyval.dtlist.nused = 1;
				plpgsql_yyval.dtlist.dtitems = palloc(sizeof(PLpgSQL_diag_item) * plpgsql_yyval.dtlist.nalloc);
				plpgsql_yyval.dtlist.dtitems[0].target = plpgsql_yyvsp[-2].ival;
				plpgsql_yyval.dtlist.dtitems[0].item = plpgsql_yyvsp[0].ival;
			}
			break;

		case 78:
#line 841 "gram.y"
				plpgsql_yyval.ival = PLPGSQL_GETDIAG_ROW_COUNT;
			break;

		case 79:
#line 845 "gram.y"
				plpgsql_yyval.ival = PLPGSQL_GETDIAG_RESULT_OID;
			break;

		case 80:
#line 851 "gram.y"
			{
				check_assignable(plpgsql_yylval.variable);
				plpgsql_yyval.ival = plpgsql_yylval.variable->dno;
			}
			break;

		case 81:
#line 859 "gram.y"
			{
				check_assignable(plpgsql_yylval.variable);
				plpgsql_yyval.ival = plpgsql_yylval.variable->dno;
			}
			break;

		case 82:
#line 864 "gram.y"
			{
				PLpgSQL_arrayelem *new;

				new = malloc(sizeof(PLpgSQL_arrayelem));
				memset(new, 0, sizeof(PLpgSQL_arrayelem));

				new->dtype = PLPGSQL_DTYPE_ARRAYELEM;
				new->subscript = plpgsql_yyvsp[0].expr;
				new->arrayparentno = plpgsql_yyvsp[-2].ival;

				plpgsql_adddatum((PLpgSQL_datum *) new);

				plpgsql_yyval.ival = new->dno;
			}
			break;

		case 83:
#line 881 "gram.y"
			{
				PLpgSQL_stmt_if *new;

				new = malloc(sizeof(PLpgSQL_stmt_if));
				memset(new, 0, sizeof(PLpgSQL_stmt_if));

				new->cmd_type = PLPGSQL_STMT_IF;
				new->lineno = plpgsql_yyvsp[-6].ival;
				new->cond = plpgsql_yyvsp[-5].expr;
				new->true_body = plpgsql_yyvsp[-4].stmts;
				new->false_body = plpgsql_yyvsp[-3].stmts;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 84:
#line 898 "gram.y"
			{
				PLpgSQL_stmts *new;

				new = malloc(sizeof(PLpgSQL_stmts));
				memset(new, 0, sizeof(PLpgSQL_stmts));
				plpgsql_yyval.stmts = new;
			}
			break;

		case 85:
#line 906 "gram.y"
			{
				/*
				 * Translate the structure:		 into:
				 *
				 * IF c1 THEN					 IF c1 THEN ...
				 * ... ELSIF c2 THEN				 ELSE IF c2 THEN ...
				 * ... ELSE								 ELSE ...
				 * ... END IF							 END IF END IF
				 *
				 */

				PLpgSQL_stmts *new;
				PLpgSQL_stmt_if *new_if;

				/* first create a new if-statement */
				new_if = malloc(sizeof(PLpgSQL_stmt_if));
				memset(new_if, 0, sizeof(PLpgSQL_stmt_if));

				new_if->cmd_type = PLPGSQL_STMT_IF;
				new_if->lineno = plpgsql_yyvsp[-3].ival;
				new_if->cond = plpgsql_yyvsp[-2].expr;
				new_if->true_body = plpgsql_yyvsp[-1].stmts;
				new_if->false_body = plpgsql_yyvsp[0].stmts;

				/* this is a 'container' for the if-statement */
				new = malloc(sizeof(PLpgSQL_stmts));
				memset(new, 0, sizeof(PLpgSQL_stmts));

				new->stmts_alloc = 64;
				new->stmts_used = 1;
				new->stmts = malloc(sizeof(PLpgSQL_stmt *) * new->stmts_alloc);
				new->stmts[0] = (struct PLpgSQL_stmt *) new_if;

				plpgsql_yyval.stmts = new;

			}
			break;

		case 86:
#line 949 "gram.y"
				plpgsql_yyval.stmts = plpgsql_yyvsp[0].stmts;
			break;

		case 87:
#line 955 "gram.y"
			{
				PLpgSQL_stmt_loop *new;

				new = malloc(sizeof(PLpgSQL_stmt_loop));
				memset(new, 0, sizeof(PLpgSQL_stmt_loop));

				new->cmd_type = PLPGSQL_STMT_LOOP;
				new->lineno = plpgsql_yyvsp[-1].ival;
				new->label = plpgsql_yyvsp[-3].str;
				new->body = plpgsql_yyvsp[0].stmts;

				plpgsql_ns_pop();

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 88:
#line 973 "gram.y"
			{
				PLpgSQL_stmt_while *new;

				new = malloc(sizeof(PLpgSQL_stmt_while));
				memset(new, 0, sizeof(PLpgSQL_stmt_while));

				new->cmd_type = PLPGSQL_STMT_WHILE;
				new->lineno = plpgsql_yyvsp[-2].ival;
				new->label = plpgsql_yyvsp[-4].str;
				new->cond = plpgsql_yyvsp[-1].expr;
				new->body = plpgsql_yyvsp[0].stmts;

				plpgsql_ns_pop();

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 89:
#line 992 "gram.y"
			{
				PLpgSQL_stmt_fori *new;

				new = malloc(sizeof(PLpgSQL_stmt_fori));
				memset(new, 0, sizeof(PLpgSQL_stmt_fori));

				new->cmd_type = PLPGSQL_STMT_FORI;
				new->lineno = plpgsql_yyvsp[-5].ival;
				new->label = plpgsql_yyvsp[-7].str;
				new->var = plpgsql_yyvsp[-4].var;
				new->reverse = plpgsql_yyvsp[-2].forilow.reverse;
				new->lower = plpgsql_yyvsp[-2].forilow.expr;
				new->upper = plpgsql_yyvsp[-1].expr;
				new->body = plpgsql_yyvsp[0].stmts;

				plpgsql_ns_pop();

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 90:
#line 1014 "gram.y"
			{
				PLpgSQL_var *new;

				new = malloc(sizeof(PLpgSQL_var));
				memset(new, 0, sizeof(PLpgSQL_var));

				new->dtype = PLPGSQL_DTYPE_VAR;
				new->refname = plpgsql_yyvsp[0].varname.name;
				new->lineno = plpgsql_yyvsp[0].varname.lineno;

				new->datatype = plpgsql_parse_datatype("integer");
				new->isconst = false;
				new->notnull = false;
				new->default_val = NULL;

				plpgsql_adddatum((PLpgSQL_datum *) new);
				plpgsql_ns_additem(PLPGSQL_NSTYPE_VAR, new->varno,
								   plpgsql_yyvsp[0].varname.name);

				plpgsql_add_initdatums(NULL);

				plpgsql_yyval.var = new;
			}
			break;

		case 91:
#line 1040 "gram.y"
			{
				char	   *name;

				plpgsql_convert_ident(plpgsql_yytext, &name, 1);
				/* name should be malloc'd for use as varname */
				plpgsql_yyval.varname.name = strdup(name);
				plpgsql_yyval.varname.lineno = plpgsql_scanner_lineno();
				pfree(name);
			}
			break;

		case 92:
#line 1050 "gram.y"
			{
				char	   *name;

				plpgsql_convert_ident(plpgsql_yytext, &name, 1);
				/* name should be malloc'd for use as varname */
				plpgsql_yyval.varname.name = strdup(name);
				plpgsql_yyval.varname.lineno = plpgsql_scanner_lineno();
				pfree(name);
			}
			break;

		case 93:
#line 1062 "gram.y"
			{
				int			tok;

				tok = plpgsql_yylex();
				if (tok == K_REVERSE)
					plpgsql_yyval.forilow.reverse = 1;
				else
				{
					plpgsql_yyval.forilow.reverse = 0;
					plpgsql_push_back_token(tok);
				}

				plpgsql_yyval.forilow.expr = plpgsql_read_expression(K_DOTDOT, "..");
			}
			break;

		case 94:
#line 1081 "gram.y"
			{
				PLpgSQL_stmt_fors *new;

				new = malloc(sizeof(PLpgSQL_stmt_fors));
				memset(new, 0, sizeof(PLpgSQL_stmt_fors));

				new->cmd_type = PLPGSQL_STMT_FORS;
				new->lineno = plpgsql_yyvsp[-5].ival;
				new->label = plpgsql_yyvsp[-7].str;
				switch (plpgsql_yyvsp[-4].rec->dtype)
				{
					case PLPGSQL_DTYPE_REC:
						new->rec = plpgsql_yyvsp[-4].rec;
						break;
					case PLPGSQL_DTYPE_ROW:
						new->row = (PLpgSQL_row *) plpgsql_yyvsp[-4].rec;
						break;
					default:
						elog(ERROR, "unrecognized dtype: %d",
							 plpgsql_yyvsp[-4].rec->dtype);
				}
				new->query = plpgsql_yyvsp[-1].expr;
				new->body = plpgsql_yyvsp[0].stmts;

				plpgsql_ns_pop();

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 95:
#line 1112 "gram.y"
			{
				PLpgSQL_stmt_dynfors *new;

				new = malloc(sizeof(PLpgSQL_stmt_dynfors));
				memset(new, 0, sizeof(PLpgSQL_stmt_dynfors));

				new->cmd_type = PLPGSQL_STMT_DYNFORS;
				new->lineno = plpgsql_yyvsp[-5].ival;
				new->label = plpgsql_yyvsp[-7].str;
				switch (plpgsql_yyvsp[-4].rec->dtype)
				{
					case PLPGSQL_DTYPE_REC:
						new->rec = plpgsql_yyvsp[-4].rec;
						break;
					case PLPGSQL_DTYPE_ROW:
						new->row = (PLpgSQL_row *) plpgsql_yyvsp[-4].rec;
						break;
					default:
						elog(ERROR, "unrecognized dtype: %d",
							 plpgsql_yyvsp[-4].rec->dtype);
				}
				new->query = plpgsql_yyvsp[-1].expr;
				new->body = plpgsql_yyvsp[0].stmts;

				plpgsql_ns_pop();

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 96:
#line 1143 "gram.y"
				plpgsql_yyval.rec = plpgsql_yylval.rec;
			break;

		case 97:
#line 1145 "gram.y"
				plpgsql_yyval.rec = (PLpgSQL_rec *) (plpgsql_yylval.row);
			break;

		case 98:
#line 1151 "gram.y"
			{
				plpgsql_yyval.stmt = make_select_stmt();
				plpgsql_yyval.stmt->lineno = plpgsql_yyvsp[0].ival;
			}
			break;

		case 99:
#line 1158 "gram.y"
			{
				PLpgSQL_stmt_exit *new;

				new = malloc(sizeof(PLpgSQL_stmt_exit));
				memset(new, 0, sizeof(PLpgSQL_stmt_exit));

				new->cmd_type = PLPGSQL_STMT_EXIT;
				new->lineno = plpgsql_yyvsp[-2].ival;
				new->label = plpgsql_yyvsp[-1].str;
				new->cond = plpgsql_yyvsp[0].expr;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 100:
#line 1174 "gram.y"
			{
				PLpgSQL_stmt_return *new;

				new = malloc(sizeof(PLpgSQL_stmt_return));
				memset(new, 0, sizeof(PLpgSQL_stmt_return));

				if (plpgsql_curr_compile->fn_retistuple &&
					!plpgsql_curr_compile->fn_retset)
				{
					new->retrecno = -1;
					switch (plpgsql_yylex())
					{
						case K_NULL:
							new->expr = NULL;
							break;

						case T_ROW:
							new->expr = make_tupret_expr(plpgsql_yylval.row);
							break;

						case T_RECORD:
							new->retrecno = plpgsql_yylval.rec->recno;
							new->expr = NULL;
							break;

						default:
							plpgsql_yyerror("return type mismatch in function returning tuple");
							break;
					}
					if (plpgsql_yylex() != ';')
						plpgsql_yyerror("expected \";\"");
				}
				else
					new->expr = plpgsql_read_expression(';', ";");

				new->cmd_type = PLPGSQL_STMT_RETURN;
				new->lineno = plpgsql_yyvsp[0].ival;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 101:
#line 1217 "gram.y"
			{
				PLpgSQL_stmt_return_next *new;

				new = malloc(sizeof(PLpgSQL_stmt_return_next));
				memset(new, 0, sizeof(PLpgSQL_stmt_return_next));

				new->cmd_type = PLPGSQL_STMT_RETURN_NEXT;
				new->lineno = plpgsql_yyvsp[0].ival;

				if (plpgsql_curr_compile->fn_retistuple)
				{
					int			tok = plpgsql_yylex();

					if (tok == T_RECORD)
						new->rec = plpgsql_yylval.rec;
					else if (tok == T_ROW)
						new->row = plpgsql_yylval.row;
					else
						plpgsql_yyerror("incorrect argument to RETURN NEXT");

					if (plpgsql_yylex() != ';')
						plpgsql_yyerror("expected \";\"");
				}
				else
					new->expr = plpgsql_read_expression(';', ";");

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 102:
#line 1248 "gram.y"
			{
				PLpgSQL_stmt_raise *new;

				new = malloc(sizeof(PLpgSQL_stmt_raise));

				new->cmd_type = PLPGSQL_STMT_RAISE;
				new->lineno = plpgsql_yyvsp[-4].ival;
				new->elog_level = plpgsql_yyvsp[-3].ival;
				new->message = plpgsql_yyvsp[-2].str;
				new->nparams = plpgsql_yyvsp[-1].intlist.nused;
				new->params = malloc(sizeof(int) * plpgsql_yyvsp[-1].intlist.nused);
				memcpy(new->params, plpgsql_yyvsp[-1].intlist.nums, sizeof(int) * plpgsql_yyvsp[-1].intlist.nused);

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 103:
#line 1264 "gram.y"
			{
				PLpgSQL_stmt_raise *new;

				new = malloc(sizeof(PLpgSQL_stmt_raise));

				new->cmd_type = PLPGSQL_STMT_RAISE;
				new->lineno = plpgsql_yyvsp[-3].ival;
				new->elog_level = plpgsql_yyvsp[-2].ival;
				new->message = plpgsql_yyvsp[-1].str;
				new->nparams = 0;
				new->params = NULL;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 104:
#line 1281 "gram.y"
				plpgsql_yyval.str = strdup(plpgsql_yytext);
			break;

		case 105:
#line 1287 "gram.y"
				plpgsql_yyval.ival = ERROR;
			break;

		case 106:
#line 1291 "gram.y"
				plpgsql_yyval.ival = WARNING;
			break;

		case 107:
#line 1295 "gram.y"
				plpgsql_yyval.ival = NOTICE;
			break;

		case 108:
#line 1299 "gram.y"
				plpgsql_yyval.ival = INFO;
			break;

		case 109:
#line 1303 "gram.y"
				plpgsql_yyval.ival = LOG;
			break;

		case 110:
#line 1307 "gram.y"
				plpgsql_yyval.ival = DEBUG2;
			break;

		case 111:
#line 1313 "gram.y"
			{
				if (plpgsql_yyvsp[-1].intlist.nused == plpgsql_yyvsp[-1].intlist.nalloc)
				{
					plpgsql_yyvsp[-1].intlist.nalloc *= 2;
					plpgsql_yyvsp[-1].intlist.nums = repalloc(plpgsql_yyvsp[-1].intlist.nums, sizeof(int) * plpgsql_yyvsp[-1].intlist.nalloc);
				}
				plpgsql_yyvsp[-1].intlist.nums[plpgsql_yyvsp[-1].intlist.nused++] = plpgsql_yyvsp[0].ival;

				plpgsql_yyval.intlist.nalloc = plpgsql_yyvsp[-1].intlist.nalloc;
				plpgsql_yyval.intlist.nused = plpgsql_yyvsp[-1].intlist.nused;
				plpgsql_yyval.intlist.nums = plpgsql_yyvsp[-1].intlist.nums;
			}
			break;

		case 112:
#line 1326 "gram.y"
			{
				plpgsql_yyval.intlist.nalloc = 1;
				plpgsql_yyval.intlist.nused = 1;
				plpgsql_yyval.intlist.nums = palloc(sizeof(int) * plpgsql_yyval.intlist.nalloc);
				plpgsql_yyval.intlist.nums[0] = plpgsql_yyvsp[0].ival;
			}
			break;

		case 113:
#line 1335 "gram.y"
				plpgsql_yyval.ival = plpgsql_yylval.variable->dno;
			break;

		case 114:
#line 1341 "gram.y"
				plpgsql_yyval.stmts = plpgsql_yyvsp[-3].stmts;
			break;

		case 115:
#line 1345 "gram.y"
			{
				PLpgSQL_stmt_execsql *new;

				new = malloc(sizeof(PLpgSQL_stmt_execsql));
				new->cmd_type = PLPGSQL_STMT_EXECSQL;
				new->lineno = plpgsql_yyvsp[0].ival;
				new->sqlstmt = read_sql_stmt(plpgsql_yyvsp[-1].str);

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 116:
#line 1358 "gram.y"
			{
				PLpgSQL_stmt_dynexecute *new;

				new = malloc(sizeof(PLpgSQL_stmt_dynexecute));
				new->cmd_type = PLPGSQL_STMT_DYNEXECUTE;
				new->lineno = plpgsql_yyvsp[-1].ival;
				new->query = plpgsql_yyvsp[0].expr;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 117:
#line 1371 "gram.y"
			{
				PLpgSQL_stmt_open *new;
				int			tok;

				new = malloc(sizeof(PLpgSQL_stmt_open));
				memset(new, 0, sizeof(PLpgSQL_stmt_open));

				new->cmd_type = PLPGSQL_STMT_OPEN;
				new->lineno = plpgsql_yyvsp[-1].ival;
				new->curvar = plpgsql_yyvsp[0].var->varno;

				if (plpgsql_yyvsp[0].var->cursor_explicit_expr == NULL)
				{
					tok = plpgsql_yylex();

					if (tok != K_FOR)
					{
						plpgsql_error_lineno = plpgsql_yyvsp[-1].ival;
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("syntax error at \"%s\"",
										plpgsql_yytext),
								 errdetail("Expected FOR to open a reference cursor.")));
					}

					tok = plpgsql_yylex();
					switch (tok)
					{
						case K_SELECT:
							new->query = read_sql_stmt("SELECT ");
							break;

						case K_EXECUTE:
							new->dynquery = read_sql_stmt("SELECT ");
							break;

						default:
							plpgsql_error_lineno = plpgsql_yyvsp[-1].ival;
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("syntax error at \"%s\"",
											plpgsql_yytext)));
					}

				}
				else
				{
					if (plpgsql_yyvsp[0].var->cursor_explicit_argrow >= 0)
					{
						char	   *cp;

						tok = plpgsql_yylex();

						if (tok != '(')
						{
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("cursor \"%s\" has arguments",
										plpgsql_yyvsp[0].var->refname)));
						}

						/*
						 * Push back the '(', else read_sql_stmt will
						 * complain about unbalanced parens.
						 */
						plpgsql_push_back_token(tok);

						new->argquery = read_sql_stmt("SELECT ");

						/*
						 * Now remove the leading and trailing parens,
						 * because we want "select 1, 2", not "select (1,
						 * 2)".
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
						while (*cp == ' ')		/* could be more than 1
												 * space here */
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
							plpgsql_yyerror("expected \")\"");
						*cp = '\0';
					}
					else
					{
						tok = plpgsql_yylex();

						if (tok == '(')
						{
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("cursor \"%s\" has no arguments",
										plpgsql_yyvsp[0].var->refname)));
						}

						if (tok != ';')
						{
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("syntax error at \"%s\"",
											plpgsql_yytext)));
						}
					}
				}

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 118:
#line 1502 "gram.y"
			{
				PLpgSQL_stmt_fetch *new;

				new = (PLpgSQL_stmt_fetch *) make_fetch_stmt();
				new->curvar = plpgsql_yyvsp[-1].ival;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
				plpgsql_yyval.stmt->lineno = plpgsql_yyvsp[-2].ival;
			}
			break;

		case 119:
#line 1514 "gram.y"
			{
				PLpgSQL_stmt_close *new;

				new = malloc(sizeof(PLpgSQL_stmt_close));
				new->cmd_type = PLPGSQL_STMT_CLOSE;
				new->lineno = plpgsql_yyvsp[-2].ival;
				new->curvar = plpgsql_yyvsp[-1].ival;

				plpgsql_yyval.stmt = (PLpgSQL_stmt *) new;
			}
			break;

		case 120:
#line 1527 "gram.y"
			{
				if (plpgsql_yylval.variable->dtype != PLPGSQL_DTYPE_VAR)
					plpgsql_yyerror("cursor variable must be a simple variable");

				if (((PLpgSQL_var *) plpgsql_yylval.variable)->datatype->typoid != REFCURSOROID)
				{
					plpgsql_error_lineno = plpgsql_scanner_lineno();
					ereport(ERROR,
							(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("\"%s\" must be of type cursor or refcursor",
					((PLpgSQL_var *) plpgsql_yylval.variable)->refname)));
				}
				plpgsql_yyval.var = (PLpgSQL_var *) plpgsql_yylval.variable;
			}
			break;

		case 121:
#line 1544 "gram.y"
			{
				if (plpgsql_yylval.variable->dtype != PLPGSQL_DTYPE_VAR)
					plpgsql_yyerror("cursor variable must be a simple variable");

				if (((PLpgSQL_var *) plpgsql_yylval.variable)->datatype->typoid != REFCURSOROID)
				{
					plpgsql_error_lineno = plpgsql_scanner_lineno();
					ereport(ERROR,
							(errcode(ERRCODE_DATATYPE_MISMATCH),
							 errmsg("\"%s\" must be of type refcursor",
					((PLpgSQL_var *) plpgsql_yylval.variable)->refname)));
				}
				plpgsql_yyval.ival = plpgsql_yylval.variable->dno;
			}
			break;

		case 122:
#line 1561 "gram.y"
				plpgsql_yyval.str = strdup(plpgsql_yytext);
			break;

		case 123:
#line 1563 "gram.y"
				plpgsql_yyval.str = strdup(plpgsql_yytext);
			break;

		case 124:
#line 1567 "gram.y"
			{
				plpgsql_yyval.expr = plpgsql_read_expression(';', ";");
			}
			break;

		case 125:
#line 1571 "gram.y"
				plpgsql_yyval.expr = plpgsql_read_expression(']', "]");
			break;

		case 126:
#line 1575 "gram.y"
				plpgsql_yyval.expr = plpgsql_read_expression(K_THEN, "THEN");
			break;

		case 127:
#line 1579 "gram.y"
				plpgsql_yyval.expr = plpgsql_read_expression(K_LOOP, "LOOP");
			break;

		case 128:
#line 1583 "gram.y"
			{
				plpgsql_ns_push(NULL);
				plpgsql_yyval.str = NULL;
			}
			break;

		case 129:
#line 1588 "gram.y"
			{
				plpgsql_ns_push(plpgsql_yyvsp[-2].str);
				plpgsql_yyval.str = plpgsql_yyvsp[-2].str;
			}
			break;

		case 130:
#line 1595 "gram.y"
				plpgsql_yyval.str = NULL;
			break;

		case 131:
#line 1597 "gram.y"
				plpgsql_yyval.str = strdup(plpgsql_yytext);
			break;

		case 132:
#line 1601 "gram.y"
				plpgsql_yyval.expr = NULL;
			break;

		case 133:
#line 1603 "gram.y"
				plpgsql_yyval.expr = plpgsql_yyvsp[0].expr;
			break;

		case 134:
#line 1607 "gram.y"
			{
				char	   *name;

				plpgsql_convert_ident(plpgsql_yytext, &name, 1);
				plpgsql_yyval.str = strdup(name);
				pfree(name);
			}
			break;

		case 135:
#line 1617 "gram.y"
				plpgsql_yyval.ival = plpgsql_error_lineno = plpgsql_scanner_lineno();
			break;


	}

/* Line 991 of yacc.c.	*/
#line 3050 "y.tab.c"


	plpgsql_yyvsp -= plpgsql_yylen;
	plpgsql_yyssp -= plpgsql_yylen;


	PLPGSQL_YY_STACK_PRINT(plpgsql_yyss, plpgsql_yyssp);

	*++plpgsql_yyvsp = plpgsql_yyval;


	/*
	 * Now `shift' the result of the reduction.  Determine what state that
	 * goes to, based on the state we popped back to and the rule number
	 * reduced by.
	 */

	plpgsql_yyn = plpgsql_yyr1[plpgsql_yyn];

	plpgsql_yystate = plpgsql_yypgoto[plpgsql_yyn - PLPGSQL_YYNTOKENS] + *plpgsql_yyssp;
	if (0 <= plpgsql_yystate && plpgsql_yystate <= PLPGSQL_YYLAST && plpgsql_yycheck[plpgsql_yystate] == *plpgsql_yyssp)
		plpgsql_yystate = plpgsql_yytable[plpgsql_yystate];
	else
		plpgsql_yystate = plpgsql_yydefgoto[plpgsql_yyn - PLPGSQL_YYNTOKENS];

	goto plpgsql_yynewstate;


/*------------------------------------.
| plpgsql_yyerrlab -- here on detecting error |
`------------------------------------*/
plpgsql_yyerrlab:
	/* If not already recovering from an error, report this error.	*/
	if (!plpgsql_yyerrstatus)
	{
		++plpgsql_yynerrs;
#if PLPGSQL_YYERROR_VERBOSE
		plpgsql_yyn = plpgsql_yypact[plpgsql_yystate];

		if (PLPGSQL_YYPACT_NINF < plpgsql_yyn && plpgsql_yyn < PLPGSQL_YYLAST)
		{
			PLPGSQL_YYSIZE_T plpgsql_yysize = 0;
			int			plpgsql_yytype = PLPGSQL_YYTRANSLATE(plpgsql_yychar);
			char	   *plpgsql_yymsg;
			int			plpgsql_yyx,
						plpgsql_yycount;

			plpgsql_yycount = 0;

			/*
			 * Start PLPGSQL_YYX at -PLPGSQL_YYN if negative to avoid
			 * negative indexes in PLPGSQL_YYCHECK.
			 */
			for (plpgsql_yyx = plpgsql_yyn < 0 ? -plpgsql_yyn : 0;
				 plpgsql_yyx < (int) (sizeof(plpgsql_yytname) / sizeof(char *)); plpgsql_yyx++)
				if (plpgsql_yycheck[plpgsql_yyx + plpgsql_yyn] == plpgsql_yyx && plpgsql_yyx != PLPGSQL_YYTERROR)
					plpgsql_yysize += plpgsql_yystrlen(plpgsql_yytname[plpgsql_yyx]) + 15, plpgsql_yycount++;
			plpgsql_yysize += plpgsql_yystrlen("syntax error, unexpected ") + 1;
			plpgsql_yysize += plpgsql_yystrlen(plpgsql_yytname[plpgsql_yytype]);
			plpgsql_yymsg = (char *) PLPGSQL_YYSTACK_ALLOC(plpgsql_yysize);
			if (plpgsql_yymsg != 0)
			{
				char	   *plpgsql_yyp = plpgsql_yystpcpy(plpgsql_yymsg, "syntax error, unexpected ");

				plpgsql_yyp = plpgsql_yystpcpy(plpgsql_yyp, plpgsql_yytname[plpgsql_yytype]);

				if (plpgsql_yycount < 5)
				{
					plpgsql_yycount = 0;
					for (plpgsql_yyx = plpgsql_yyn < 0 ? -plpgsql_yyn : 0;
						 plpgsql_yyx < (int) (sizeof(plpgsql_yytname) / sizeof(char *));
						 plpgsql_yyx++)
						if (plpgsql_yycheck[plpgsql_yyx + plpgsql_yyn] == plpgsql_yyx && plpgsql_yyx != PLPGSQL_YYTERROR)
						{
							const char *plpgsql_yyq = !plpgsql_yycount ? ", expecting " : " or ";

							plpgsql_yyp = plpgsql_yystpcpy(plpgsql_yyp, plpgsql_yyq);
							plpgsql_yyp = plpgsql_yystpcpy(plpgsql_yyp, plpgsql_yytname[plpgsql_yyx]);
							plpgsql_yycount++;
						}
				}
				plpgsql_yyerror(plpgsql_yymsg);
				PLPGSQL_YYSTACK_FREE(plpgsql_yymsg);
			}
			else
				plpgsql_yyerror("syntax error; also virtual memory exhausted");
		}
		else
#endif   /* PLPGSQL_YYERROR_VERBOSE */
			plpgsql_yyerror("syntax error");
	}



	if (plpgsql_yyerrstatus == 3)
	{
		/*
		 * If just tried and failed to reuse lookahead token after an
		 * error, discard it.
		 */

		/* Return failure if at end of input.  */
		if (plpgsql_yychar == PLPGSQL_YYEOF)
		{
			/* Pop the error token.  */
			PLPGSQL_YYPOPSTACK;
			/* Pop the rest of the stack.  */
			while (plpgsql_yyss < plpgsql_yyssp)
			{
				PLPGSQL_YYDSYMPRINTF("Error: popping", plpgsql_yystos[*plpgsql_yyssp], plpgsql_yyvsp, plpgsql_yylsp);
				plpgsql_yydestruct(plpgsql_yystos[*plpgsql_yyssp], plpgsql_yyvsp);
				PLPGSQL_YYPOPSTACK;
			}
			PLPGSQL_YYABORT;
		}

		PLPGSQL_YYDSYMPRINTF("Error: discarding", plpgsql_yytoken, &plpgsql_yylval, &plpgsql_yylloc);
		plpgsql_yydestruct(plpgsql_yytoken, &plpgsql_yylval);
		plpgsql_yychar = PLPGSQL_YYEMPTY;

	}

	/*
	 * Else will try to reuse lookahead token after shifting the error
	 * token.
	 */
	goto plpgsql_yyerrlab2;


/*----------------------------------------------------.
| plpgsql_yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
plpgsql_yyerrlab1:

	/*
	 * Suppress GCC warning that plpgsql_yyerrlab1 is unused when no
	 * action invokes PLPGSQL_YYERROR.
	 */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
	__attribute__((__unused__))
#endif


		goto plpgsql_yyerrlab2;


/*---------------------------------------------------------------.
| plpgsql_yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
plpgsql_yyerrlab2:
	plpgsql_yyerrstatus = 3;	/* Each real token shifted decrements
								 * this.  */

	for (;;)
	{
		plpgsql_yyn = plpgsql_yypact[plpgsql_yystate];
		if (plpgsql_yyn != PLPGSQL_YYPACT_NINF)
		{
			plpgsql_yyn += PLPGSQL_YYTERROR;
			if (0 <= plpgsql_yyn && plpgsql_yyn <= PLPGSQL_YYLAST && plpgsql_yycheck[plpgsql_yyn] == PLPGSQL_YYTERROR)
			{
				plpgsql_yyn = plpgsql_yytable[plpgsql_yyn];
				if (0 < plpgsql_yyn)
					break;
			}
		}

		/* Pop the current state because it cannot handle the error token.	*/
		if (plpgsql_yyssp == plpgsql_yyss)
			PLPGSQL_YYABORT;

		PLPGSQL_YYDSYMPRINTF("Error: popping", plpgsql_yystos[*plpgsql_yyssp], plpgsql_yyvsp, plpgsql_yylsp);
		plpgsql_yydestruct(plpgsql_yystos[plpgsql_yystate], plpgsql_yyvsp);
		plpgsql_yyvsp--;
		plpgsql_yystate = *--plpgsql_yyssp;

		PLPGSQL_YY_STACK_PRINT(plpgsql_yyss, plpgsql_yyssp);
	}

	if (plpgsql_yyn == PLPGSQL_YYFINAL)
		PLPGSQL_YYACCEPT;

	PLPGSQL_YYDPRINTF((stderr, "Shifting error token, "));

	*++plpgsql_yyvsp = plpgsql_yylval;


	plpgsql_yystate = plpgsql_yyn;
	goto plpgsql_yynewstate;


/*-------------------------------------.
| plpgsql_yyacceptlab -- PLPGSQL_YYACCEPT comes here.  |
`-------------------------------------*/
plpgsql_yyacceptlab:
	plpgsql_yyresult = 0;
	goto plpgsql_yyreturn;

/*-----------------------------------.
| plpgsql_yyabortlab -- PLPGSQL_YYABORT comes here.  |
`-----------------------------------*/
plpgsql_yyabortlab:
	plpgsql_yyresult = 1;
	goto plpgsql_yyreturn;

#ifndef plpgsql_yyoverflow
/*----------------------------------------------.
| plpgsql_yyoverflowlab -- parser overflow comes here.	|
`----------------------------------------------*/
plpgsql_yyoverflowlab:
	plpgsql_yyerror("parser stack overflow");
	plpgsql_yyresult = 2;
	/* Fall through.  */
#endif

plpgsql_yyreturn:
#ifndef plpgsql_yyoverflow
	if (plpgsql_yyss != plpgsql_yyssa)
		PLPGSQL_YYSTACK_FREE(plpgsql_yyss);
#endif
	return plpgsql_yyresult;
}


#line 1622 "gram.y"



PLpgSQL_expr *
plpgsql_read_expression(int until, const char *expected)
{
	return read_sql_construct(until, expected, true, "SELECT ");
}

static PLpgSQL_expr *
read_sql_stmt(const char *sqlstart)
{
	return read_sql_construct(';', ";", false, sqlstart);
}

static PLpgSQL_expr *
read_sql_construct(int until,
				   const char *expected,
				   bool isexpression,
				   const char *sqlstart)
{
	int			tok;
	int			lno;
	PLpgSQL_dstring ds;
	int			parenlevel = 0;
	int			nparams = 0;
	int			params[1024];
	char		buf[32];
	PLpgSQL_expr *expr;

	lno = plpgsql_scanner_lineno();
	plpgsql_dstring_init(&ds);
	plpgsql_dstring_append(&ds, (char *) sqlstart);

	for (;;)
	{
		tok = plpgsql_yylex();
		if (tok == until && parenlevel == 0)
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
			break;
		}
		if (plpgsql_SpaceScanned)
			plpgsql_dstring_append(&ds, " ");
		switch (tok)
		{
			case T_VARIABLE:
				params[nparams] = plpgsql_yylval.variable->dno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			default:
				plpgsql_dstring_append(&ds, plpgsql_yytext);
				break;
		}
	}

	expr = malloc(sizeof(PLpgSQL_expr) + sizeof(int) * nparams - sizeof(int));
	expr->dtype = PLPGSQL_DTYPE_EXPR;
	expr->query = strdup(plpgsql_dstring_get(&ds));
	expr->plan = NULL;
	expr->nparams = nparams;
	while (nparams-- > 0)
		expr->params[nparams] = params[nparams];
	plpgsql_dstring_free(&ds);

	return expr;
}

static PLpgSQL_type *
read_datatype(int tok)
{
	int			lno;
	PLpgSQL_dstring ds;
	PLpgSQL_type *result;
	bool		needspace = false;
	int			parenlevel = 0;

	lno = plpgsql_scanner_lineno();

	/* Often there will be a lookahead token, but if not, get one */
	if (tok == PLPGSQL_YYEMPTY)
		tok = plpgsql_yylex();

	if (tok == T_DTYPE)
	{
		/* lexer found word%TYPE and did its thing already */
		return plpgsql_yylval.dtype;
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
		plpgsql_dstring_append(&ds, plpgsql_yytext);

		tok = plpgsql_yylex();
	}

	plpgsql_push_back_token(tok);

	plpgsql_error_lineno = lno; /* in case of error in parse_datatype */

	result = plpgsql_parse_datatype(plpgsql_dstring_get(&ds));

	plpgsql_dstring_free(&ds);

	return result;
}


static PLpgSQL_stmt *
make_select_stmt(void)
{
	PLpgSQL_dstring ds;
	int			nparams = 0;
	int			params[1024];
	char		buf[32];
	PLpgSQL_expr *expr;
	PLpgSQL_row *row = NULL;
	PLpgSQL_rec *rec = NULL;
	int			tok = 0;
	int			have_nexttok = 0;
	int			have_into = 0;

	plpgsql_dstring_init(&ds);
	plpgsql_dstring_append(&ds, "SELECT ");

	while (1)
	{
		if (!have_nexttok)
			tok = plpgsql_yylex();
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
			tok = plpgsql_yylex();
			switch (tok)
			{
				case T_ROW:
					row = plpgsql_yylval.row;
					have_into = 1;
					break;

				case T_RECORD:
					rec = plpgsql_yylval.rec;
					have_into = 1;
					break;

				case T_VARIABLE:
					{
						int			nfields = 1;
						char	   *fieldnames[1024];
						int			varnos[1024];

						check_assignable(plpgsql_yylval.variable);
						fieldnames[0] = strdup(plpgsql_yytext);
						varnos[0] = plpgsql_yylval.variable->dno;

						while ((tok = plpgsql_yylex()) == ',')
						{
							tok = plpgsql_yylex();
							switch (tok)
							{
								case T_VARIABLE:
									check_assignable(plpgsql_yylval.variable);
									fieldnames[nfields] = strdup(plpgsql_yytext);
									varnos[nfields++] = plpgsql_yylval.variable->dno;
									break;

								default:
									plpgsql_error_lineno = plpgsql_scanner_lineno();
									ereport(ERROR,
										  (errcode(ERRCODE_SYNTAX_ERROR),
									   errmsg("\"%s\" is not a variable",
											  plpgsql_yytext)));
							}
						}
						have_nexttok = 1;

						row = malloc(sizeof(PLpgSQL_row));
						row->dtype = PLPGSQL_DTYPE_ROW;
						row->refname = strdup("*internal*");
						row->lineno = plpgsql_scanner_lineno();
						row->rowtypeclass = InvalidOid;
						row->nfields = nfields;
						row->fieldnames = malloc(sizeof(char *) * nfields);
						row->varnos = malloc(sizeof(int) * nfields);
						while (--nfields >= 0)
						{
							row->fieldnames[nfields] = fieldnames[nfields];
							row->varnos[nfields] = varnos[nfields];
						}

						plpgsql_adddatum((PLpgSQL_datum *) row);

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
			case T_VARIABLE:
				params[nparams] = plpgsql_yylval.variable->dno;
				snprintf(buf, sizeof(buf), " $%d ", ++nparams);
				plpgsql_dstring_append(&ds, buf);
				break;

			default:
				plpgsql_dstring_append(&ds, plpgsql_yytext);
				break;
		}
	}

	expr = malloc(sizeof(PLpgSQL_expr) + sizeof(int) * nparams - sizeof(int));
	expr->dtype = PLPGSQL_DTYPE_EXPR;
	expr->query = strdup(plpgsql_dstring_get(&ds));
	expr->plan = NULL;
	expr->nparams = nparams;
	while (nparams-- > 0)
		expr->params[nparams] = params[nparams];
	plpgsql_dstring_free(&ds);

	if (have_into)
	{
		PLpgSQL_stmt_select *select;

		select = malloc(sizeof(PLpgSQL_stmt_select));
		memset(select, 0, sizeof(PLpgSQL_stmt_select));
		select->cmd_type = PLPGSQL_STMT_SELECT;
		select->rec = rec;
		select->row = row;
		select->query = expr;

		return (PLpgSQL_stmt *) select;
	}
	else
	{
		PLpgSQL_stmt_execsql *execsql;

		execsql = malloc(sizeof(PLpgSQL_stmt_execsql));
		execsql->cmd_type = PLPGSQL_STMT_EXECSQL;
		execsql->sqlstmt = expr;

		return (PLpgSQL_stmt *) execsql;
	}
}


static PLpgSQL_stmt *
make_fetch_stmt(void)
{
	int			tok;
	PLpgSQL_row *row = NULL;
	PLpgSQL_rec *rec = NULL;
	PLpgSQL_stmt_fetch *fetch;
	int			have_nexttok = 0;

	/* We have already parsed everything through the INTO keyword */

	tok = plpgsql_yylex();
	switch (tok)
	{
		case T_ROW:
			row = plpgsql_yylval.row;
			break;

		case T_RECORD:
			rec = plpgsql_yylval.rec;
			break;

		case T_VARIABLE:
			{
				int			nfields = 1;
				char	   *fieldnames[1024];
				int			varnos[1024];

				check_assignable(plpgsql_yylval.variable);
				fieldnames[0] = strdup(plpgsql_yytext);
				varnos[0] = plpgsql_yylval.variable->dno;

				while ((tok = plpgsql_yylex()) == ',')
				{
					tok = plpgsql_yylex();
					switch (tok)
					{
						case T_VARIABLE:
							check_assignable(plpgsql_yylval.variable);
							fieldnames[nfields] = strdup(plpgsql_yytext);
							varnos[nfields++] = plpgsql_yylval.variable->dno;
							break;

						default:
							plpgsql_error_lineno = plpgsql_scanner_lineno();
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("\"%s\" is not a variable",
											plpgsql_yytext)));
					}
				}
				have_nexttok = 1;

				row = malloc(sizeof(PLpgSQL_row));
				row->dtype = PLPGSQL_DTYPE_ROW;
				row->refname = strdup("*internal*");
				row->lineno = plpgsql_scanner_lineno();
				row->rowtypeclass = InvalidOid;
				row->nfields = nfields;
				row->fieldnames = malloc(sizeof(char *) * nfields);
				row->varnos = malloc(sizeof(int) * nfields);
				while (--nfields >= 0)
				{
					row->fieldnames[nfields] = fieldnames[nfields];
					row->varnos[nfields] = varnos[nfields];
				}

				plpgsql_adddatum((PLpgSQL_datum *) row);
			}
			break;

		default:
			plpgsql_yyerror("syntax error");
	}

	if (!have_nexttok)
		tok = plpgsql_yylex();

	if (tok != ';')
		plpgsql_yyerror("syntax error");

	fetch = malloc(sizeof(PLpgSQL_stmt_select));
	memset(fetch, 0, sizeof(PLpgSQL_stmt_fetch));
	fetch->cmd_type = PLPGSQL_STMT_FETCH;
	fetch->rec = rec;
	fetch->row = row;

	return (PLpgSQL_stmt *) fetch;
}


static PLpgSQL_expr *
make_tupret_expr(PLpgSQL_row * row)
{
	PLpgSQL_dstring ds;
	PLpgSQL_expr *expr;
	int			i;
	char		buf[16];

	expr = malloc(sizeof(PLpgSQL_expr) + sizeof(int) * (row->nfields - 1));
	expr->dtype = PLPGSQL_DTYPE_EXPR;

	plpgsql_dstring_init(&ds);
	plpgsql_dstring_append(&ds, "SELECT ");

	for (i = 0; i < row->nfields; i++)
	{
		sprintf(buf, "%s$%d", (i > 0) ? "," : "", i + 1);
		plpgsql_dstring_append(&ds, buf);
		expr->params[i] = row->varnos[i];
	}

	expr->query = strdup(plpgsql_dstring_get(&ds));
	expr->plan = NULL;
	expr->plan_argtypes = NULL;
	expr->nparams = row->nfields;

	plpgsql_dstring_free(&ds);
	return expr;
}

static void
check_assignable(PLpgSQL_datum * datum)
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
		case PLPGSQL_DTYPE_RECFIELD:
			/* always assignable? */
			break;
		case PLPGSQL_DTYPE_ARRAYELEM:
			/* always assignable? */
			break;
		case PLPGSQL_DTYPE_TRIGARG:
			plpgsql_yyerror("cannot assign to tg_argv");
			break;
		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			break;
	}
}

#include "pl_scan.c"
