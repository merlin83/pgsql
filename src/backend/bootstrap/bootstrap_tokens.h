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
#ifndef YYTOKENTYPE
#define YYTOKENTYPE

 /*
  * Put the tokens into the symbol table, so that GDB and other debuggers
  * know about them.
  */
enum Int_yytokentype
{
	CONST_P = 258,
	ID = 259,
	OPEN = 260,
	XCLOSE = 261,
	XCREATE = 262,
	INSERT_TUPLE = 263,
	STRING = 264,
	XDEFINE = 265,
	XDECLARE = 266,
	INDEX = 267,
	ON = 268,
	USING = 269,
	XBUILD = 270,
	INDICES = 271,
	UNIQUE = 272,
	COMMA = 273,
	EQUALS = 274,
	LPAREN = 275,
	RPAREN = 276,
	OBJ_ID = 277,
	XBOOTSTRAP = 278,
	XSHARED_RELATION = 279,
	XWITHOUT_OIDS = 280,
	NULLVAL = 281,
	low = 282,
	high = 283
};
#endif
#define CONST_P 258
#define ID 259
#define OPEN 260
#define XCLOSE 261
#define XCREATE 262
#define INSERT_TUPLE 263
#define STRING 264
#define XDEFINE 265
#define XDECLARE 266
#define INDEX 267
#define ON 268
#define USING 269
#define XBUILD 270
#define INDICES 271
#define UNIQUE 272
#define COMMA 273
#define EQUALS 274
#define LPAREN 275
#define RPAREN 276
#define OBJ_ID 277
#define XBOOTSTRAP 278
#define XSHARED_RELATION 279
#define XWITHOUT_OIDS 280
#define NULLVAL 281
#define low 282
#define high 283




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 81 "bootparse.y"
typedef union YYSTYPE
{
	List	   *list;
	IndexElem  *ielem;
	char	   *str;
	int			ival;
	Oid			oidval;
} YYSTYPE;

/* Line 1248 of yacc.c.  */
#line 100 "y.tab.h"
#define Int_yystype YYSTYPE		/* obsolescent; will be withdrawn */
#define YYSTYPE_IS_DECLARED 1
#define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE Int_yylval;
