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
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ABORT_P = 258,
     ABSOLUTE_P = 259,
     ACCESS = 260,
     ACTION = 261,
     ADD = 262,
     AFTER = 263,
     AGGREGATE = 264,
     ALL = 265,
     ALTER = 266,
     ANALYSE = 267,
     ANALYZE = 268,
     AND = 269,
     ANY = 270,
     ARRAY = 271,
     AS = 272,
     ASC = 273,
     ASSERTION = 274,
     ASSIGNMENT = 275,
     AT = 276,
     AUTHORIZATION = 277,
     BACKWARD = 278,
     BEFORE = 279,
     BEGIN_P = 280,
     BETWEEN = 281,
     BIGINT = 282,
     BINARY = 283,
     BIT = 284,
     BOOLEAN_P = 285,
     BOTH = 286,
     BY = 287,
     CACHE = 288,
     CALLED = 289,
     CASCADE = 290,
     CASE = 291,
     CAST = 292,
     CHAIN = 293,
     CHAR_P = 294,
     CHARACTER = 295,
     CHARACTERISTICS = 296,
     CHECK = 297,
     CHECKPOINT = 298,
     CLASS = 299,
     CLOSE = 300,
     CLUSTER = 301,
     COALESCE = 302,
     COLLATE = 303,
     COLUMN = 304,
     COMMENT = 305,
     COMMIT = 306,
     COMMITTED = 307,
     CONSTRAINT = 308,
     CONSTRAINTS = 309,
     CONVERSION_P = 310,
     CONVERT = 311,
     COPY = 312,
     CREATE = 313,
     CREATEDB = 314,
     CREATEUSER = 315,
     CROSS = 316,
     CURRENT_DATE = 317,
     CURRENT_TIME = 318,
     CURRENT_TIMESTAMP = 319,
     CURRENT_USER = 320,
     CURSOR = 321,
     CYCLE = 322,
     DATABASE = 323,
     DAY_P = 324,
     DEALLOCATE = 325,
     DEC = 326,
     DECIMAL_P = 327,
     DECLARE = 328,
     DEFAULT = 329,
     DEFAULTS = 330,
     DEFERRABLE = 331,
     DEFERRED = 332,
     DEFINER = 333,
     DELETE_P = 334,
     DELIMITER = 335,
     DELIMITERS = 336,
     DESC = 337,
     DISTINCT = 338,
     DO = 339,
     DOMAIN_P = 340,
     DOUBLE_P = 341,
     DROP = 342,
     EACH = 343,
     ELSE = 344,
     ENCODING = 345,
     ENCRYPTED = 346,
     END_P = 347,
     ESCAPE = 348,
     EXCEPT = 349,
     EXCLUDING = 350,
     EXCLUSIVE = 351,
     EXECUTE = 352,
     EXISTS = 353,
     EXPLAIN = 354,
     EXTERNAL = 355,
     EXTRACT = 356,
     FALSE_P = 357,
     FETCH = 358,
     FIRST_P = 359,
     FLOAT_P = 360,
     FOR = 361,
     FORCE = 362,
     FOREIGN = 363,
     FORWARD = 364,
     FREEZE = 365,
     FROM = 366,
     FULL = 367,
     FUNCTION = 368,
     GLOBAL = 369,
     GRANT = 370,
     GROUP_P = 371,
     HANDLER = 372,
     HAVING = 373,
     HOLD = 374,
     HOUR_P = 375,
     ILIKE = 376,
     IMMEDIATE = 377,
     IMMUTABLE = 378,
     IMPLICIT_P = 379,
     IN_P = 380,
     INCLUDING = 381,
     INCREMENT = 382,
     INDEX = 383,
     INHERITS = 384,
     INITIALLY = 385,
     INNER_P = 386,
     INOUT = 387,
     INPUT_P = 388,
     INSENSITIVE = 389,
     INSERT = 390,
     INSTEAD = 391,
     INT_P = 392,
     INTEGER = 393,
     INTERSECT = 394,
     INTERVAL = 395,
     INTO = 396,
     INVOKER = 397,
     IS = 398,
     ISNULL = 399,
     ISOLATION = 400,
     JOIN = 401,
     KEY = 402,
     LANCOMPILER = 403,
     LANGUAGE = 404,
     LAST_P = 405,
     LEADING = 406,
     LEFT = 407,
     LEVEL = 408,
     LIKE = 409,
     LIMIT = 410,
     LISTEN = 411,
     LOAD = 412,
     LOCAL = 413,
     LOCALTIME = 414,
     LOCALTIMESTAMP = 415,
     LOCATION = 416,
     LOCK_P = 417,
     MATCH = 418,
     MAXVALUE = 419,
     MINUTE_P = 420,
     MINVALUE = 421,
     MODE = 422,
     MONTH_P = 423,
     MOVE = 424,
     NAMES = 425,
     NATIONAL = 426,
     NATURAL = 427,
     NCHAR = 428,
     NEW = 429,
     NEXT = 430,
     NO = 431,
     NOCREATEDB = 432,
     NOCREATEUSER = 433,
     NONE = 434,
     NOT = 435,
     NOTHING = 436,
     NOTIFY = 437,
     NOTNULL = 438,
     NULL_P = 439,
     NULLIF = 440,
     NUMERIC = 441,
     OF = 442,
     OFF = 443,
     OFFSET = 444,
     OIDS = 445,
     OLD = 446,
     ON = 447,
     ONLY = 448,
     OPERATOR = 449,
     OPTION = 450,
     OR = 451,
     ORDER = 452,
     OUT_P = 453,
     OUTER_P = 454,
     OVERLAPS = 455,
     OVERLAY = 456,
     OWNER = 457,
     PARTIAL = 458,
     PASSWORD = 459,
     PATH_P = 460,
     PENDANT = 461,
     PLACING = 462,
     POSITION = 463,
     PRECISION = 464,
     PRESERVE = 465,
     PREPARE = 466,
     PRIMARY = 467,
     PRIOR = 468,
     PRIVILEGES = 469,
     PROCEDURAL = 470,
     PROCEDURE = 471,
     READ = 472,
     REAL = 473,
     RECHECK = 474,
     REFERENCES = 475,
     REINDEX = 476,
     RELATIVE_P = 477,
     RENAME = 478,
     REPLACE = 479,
     RESET = 480,
     RESTART = 481,
     RESTRICT = 482,
     RETURNS = 483,
     REVOKE = 484,
     RIGHT = 485,
     ROLLBACK = 486,
     ROW = 487,
     ROWS = 488,
     RULE = 489,
     SCHEMA = 490,
     SCROLL = 491,
     SECOND_P = 492,
     SECURITY = 493,
     SELECT = 494,
     SEQUENCE = 495,
     SERIALIZABLE = 496,
     SESSION = 497,
     SESSION_USER = 498,
     SET = 499,
     SETOF = 500,
     SHARE = 501,
     SHOW = 502,
     SIMILAR = 503,
     SIMPLE = 504,
     SMALLINT = 505,
     SOME = 506,
     STABLE = 507,
     START = 508,
     STATEMENT = 509,
     STATISTICS = 510,
     STDIN = 511,
     STDOUT = 512,
     STORAGE = 513,
     STRICT_P = 514,
     SUBSTRING = 515,
     SYSID = 516,
     TABLE = 517,
     TEMP = 518,
     TEMPLATE = 519,
     TEMPORARY = 520,
     THEN = 521,
     TIME = 522,
     TIMESTAMP = 523,
     TO = 524,
     TOAST = 525,
     TRAILING = 526,
     TRANSACTION = 527,
     TREAT = 528,
     TRIGGER = 529,
     TRIM = 530,
     TRUE_P = 531,
     TRUNCATE = 532,
     TRUSTED = 533,
     TYPE_P = 534,
     UNENCRYPTED = 535,
     UNION = 536,
     UNIQUE = 537,
     UNKNOWN = 538,
     UNLISTEN = 539,
     UNTIL = 540,
     UPDATE = 541,
     USAGE = 542,
     USER = 543,
     USING = 544,
     VACUUM = 545,
     VALID = 546,
     VALIDATOR = 547,
     VALUES = 548,
     VARCHAR = 549,
     VARYING = 550,
     VERBOSE = 551,
     VERSION = 552,
     VIEW = 553,
     VOLATILE = 554,
     WHEN = 555,
     WHERE = 556,
     WITH = 557,
     WITHOUT = 558,
     WORK = 559,
     WRITE = 560,
     YEAR_P = 561,
     ZONE = 562,
     UNIONJOIN = 563,
     IDENT = 564,
     FCONST = 565,
     SCONST = 566,
     BCONST = 567,
     XCONST = 568,
     Op = 569,
     ICONST = 570,
     PARAM = 571,
     POSTFIXOP = 572,
     UMINUS = 573,
     TYPECAST = 574
   };
#endif
#define ABORT_P 258
#define ABSOLUTE_P 259
#define ACCESS 260
#define ACTION 261
#define ADD 262
#define AFTER 263
#define AGGREGATE 264
#define ALL 265
#define ALTER 266
#define ANALYSE 267
#define ANALYZE 268
#define AND 269
#define ANY 270
#define ARRAY 271
#define AS 272
#define ASC 273
#define ASSERTION 274
#define ASSIGNMENT 275
#define AT 276
#define AUTHORIZATION 277
#define BACKWARD 278
#define BEFORE 279
#define BEGIN_P 280
#define BETWEEN 281
#define BIGINT 282
#define BINARY 283
#define BIT 284
#define BOOLEAN_P 285
#define BOTH 286
#define BY 287
#define CACHE 288
#define CALLED 289
#define CASCADE 290
#define CASE 291
#define CAST 292
#define CHAIN 293
#define CHAR_P 294
#define CHARACTER 295
#define CHARACTERISTICS 296
#define CHECK 297
#define CHECKPOINT 298
#define CLASS 299
#define CLOSE 300
#define CLUSTER 301
#define COALESCE 302
#define COLLATE 303
#define COLUMN 304
#define COMMENT 305
#define COMMIT 306
#define COMMITTED 307
#define CONSTRAINT 308
#define CONSTRAINTS 309
#define CONVERSION_P 310
#define CONVERT 311
#define COPY 312
#define CREATE 313
#define CREATEDB 314
#define CREATEUSER 315
#define CROSS 316
#define CURRENT_DATE 317
#define CURRENT_TIME 318
#define CURRENT_TIMESTAMP 319
#define CURRENT_USER 320
#define CURSOR 321
#define CYCLE 322
#define DATABASE 323
#define DAY_P 324
#define DEALLOCATE 325
#define DEC 326
#define DECIMAL_P 327
#define DECLARE 328
#define DEFAULT 329
#define DEFAULTS 330
#define DEFERRABLE 331
#define DEFERRED 332
#define DEFINER 333
#define DELETE_P 334
#define DELIMITER 335
#define DELIMITERS 336
#define DESC 337
#define DISTINCT 338
#define DO 339
#define DOMAIN_P 340
#define DOUBLE_P 341
#define DROP 342
#define EACH 343
#define ELSE 344
#define ENCODING 345
#define ENCRYPTED 346
#define END_P 347
#define ESCAPE 348
#define EXCEPT 349
#define EXCLUDING 350
#define EXCLUSIVE 351
#define EXECUTE 352
#define EXISTS 353
#define EXPLAIN 354
#define EXTERNAL 355
#define EXTRACT 356
#define FALSE_P 357
#define FETCH 358
#define FIRST_P 359
#define FLOAT_P 360
#define FOR 361
#define FORCE 362
#define FOREIGN 363
#define FORWARD 364
#define FREEZE 365
#define FROM 366
#define FULL 367
#define FUNCTION 368
#define GLOBAL 369
#define GRANT 370
#define GROUP_P 371
#define HANDLER 372
#define HAVING 373
#define HOLD 374
#define HOUR_P 375
#define ILIKE 376
#define IMMEDIATE 377
#define IMMUTABLE 378
#define IMPLICIT_P 379
#define IN_P 380
#define INCLUDING 381
#define INCREMENT 382
#define INDEX 383
#define INHERITS 384
#define INITIALLY 385
#define INNER_P 386
#define INOUT 387
#define INPUT_P 388
#define INSENSITIVE 389
#define INSERT 390
#define INSTEAD 391
#define INT_P 392
#define INTEGER 393
#define INTERSECT 394
#define INTERVAL 395
#define INTO 396
#define INVOKER 397
#define IS 398
#define ISNULL 399
#define ISOLATION 400
#define JOIN 401
#define KEY 402
#define LANCOMPILER 403
#define LANGUAGE 404
#define LAST_P 405
#define LEADING 406
#define LEFT 407
#define LEVEL 408
#define LIKE 409
#define LIMIT 410
#define LISTEN 411
#define LOAD 412
#define LOCAL 413
#define LOCALTIME 414
#define LOCALTIMESTAMP 415
#define LOCATION 416
#define LOCK_P 417
#define MATCH 418
#define MAXVALUE 419
#define MINUTE_P 420
#define MINVALUE 421
#define MODE 422
#define MONTH_P 423
#define MOVE 424
#define NAMES 425
#define NATIONAL 426
#define NATURAL 427
#define NCHAR 428
#define NEW 429
#define NEXT 430
#define NO 431
#define NOCREATEDB 432
#define NOCREATEUSER 433
#define NONE 434
#define NOT 435
#define NOTHING 436
#define NOTIFY 437
#define NOTNULL 438
#define NULL_P 439
#define NULLIF 440
#define NUMERIC 441
#define OF 442
#define OFF 443
#define OFFSET 444
#define OIDS 445
#define OLD 446
#define ON 447
#define ONLY 448
#define OPERATOR 449
#define OPTION 450
#define OR 451
#define ORDER 452
#define OUT_P 453
#define OUTER_P 454
#define OVERLAPS 455
#define OVERLAY 456
#define OWNER 457
#define PARTIAL 458
#define PASSWORD 459
#define PATH_P 460
#define PENDANT 461
#define PLACING 462
#define POSITION 463
#define PRECISION 464
#define PRESERVE 465
#define PREPARE 466
#define PRIMARY 467
#define PRIOR 468
#define PRIVILEGES 469
#define PROCEDURAL 470
#define PROCEDURE 471
#define READ 472
#define REAL 473
#define RECHECK 474
#define REFERENCES 475
#define REINDEX 476
#define RELATIVE_P 477
#define RENAME 478
#define REPLACE 479
#define RESET 480
#define RESTART 481
#define RESTRICT 482
#define RETURNS 483
#define REVOKE 484
#define RIGHT 485
#define ROLLBACK 486
#define ROW 487
#define ROWS 488
#define RULE 489
#define SCHEMA 490
#define SCROLL 491
#define SECOND_P 492
#define SECURITY 493
#define SELECT 494
#define SEQUENCE 495
#define SERIALIZABLE 496
#define SESSION 497
#define SESSION_USER 498
#define SET 499
#define SETOF 500
#define SHARE 501
#define SHOW 502
#define SIMILAR 503
#define SIMPLE 504
#define SMALLINT 505
#define SOME 506
#define STABLE 507
#define START 508
#define STATEMENT 509
#define STATISTICS 510
#define STDIN 511
#define STDOUT 512
#define STORAGE 513
#define STRICT_P 514
#define SUBSTRING 515
#define SYSID 516
#define TABLE 517
#define TEMP 518
#define TEMPLATE 519
#define TEMPORARY 520
#define THEN 521
#define TIME 522
#define TIMESTAMP 523
#define TO 524
#define TOAST 525
#define TRAILING 526
#define TRANSACTION 527
#define TREAT 528
#define TRIGGER 529
#define TRIM 530
#define TRUE_P 531
#define TRUNCATE 532
#define TRUSTED 533
#define TYPE_P 534
#define UNENCRYPTED 535
#define UNION 536
#define UNIQUE 537
#define UNKNOWN 538
#define UNLISTEN 539
#define UNTIL 540
#define UPDATE 541
#define USAGE 542
#define USER 543
#define USING 544
#define VACUUM 545
#define VALID 546
#define VALIDATOR 547
#define VALUES 548
#define VARCHAR 549
#define VARYING 550
#define VERBOSE 551
#define VERSION 552
#define VIEW 553
#define VOLATILE 554
#define WHEN 555
#define WHERE 556
#define WITH 557
#define WITHOUT 558
#define WORK 559
#define WRITE 560
#define YEAR_P 561
#define ZONE 562
#define UNIONJOIN 563
#define IDENT 564
#define FCONST 565
#define SCONST 566
#define BCONST 567
#define XCONST 568
#define Op 569
#define ICONST 570
#define PARAM 571
#define POSTFIXOP 572
#define UMINUS 573
#define TYPECAST 574




/* Copy the first part of user declarations.  */
#line 1 "gram.y"


/*#define YYDEBUG 1*/
/*-------------------------------------------------------------------------
 *
 * gram.y
 *	  POSTGRES SQL YACC rules/actions
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/Attic/gram.c,v 2.90.2.1 2003-09-07 22:51:50 momjian Exp $
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
 *	  In general, nothing in this file should initiate database accesses
 *	  nor depend on changeable state (such as SET variables).  If you do
 *	  database accesses, your code will fail when we have aborted the
 *	  current transaction and are just parsing commands to find the next
 *	  ROLLBACK or COMMIT.  If you make use of SET variables, then you
 *	  will do the wrong thing in multi-query strings like this:
 *			SET SQL_inheritance TO off; SELECT * FROM foo;
 *	  because the entire string is parsed by gram.y before the SET gets
 *	  executed.  Anything that depends on the database or changeable state
 *	  should be handled inside parse_analyze() so that it happens at the
 *	  right time not the wrong time.  The handling of SQL_inheritance is
 *	  a good example.
 *
 * WARNINGS
 *	  If you use a list, make sure the datum is a node so that the printing
 *	  routines work.
 *
 *	  Sometimes we assign constants to makeStrings. Make sure we don't free
 *	  those.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include <limits.h>

#include "access/htup.h"
#include "catalog/index.h"
#include "catalog/namespace.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "nodes/params.h"
#include "nodes/parsenodes.h"
#include "parser/gramparse.h"
#include "storage/lmgr.h"
#include "utils/numeric.h"
#include "utils/datetime.h"
#include "utils/date.h"

extern List *parsetree;			/* final parse result is delivered here */

static bool QueryIsRule = FALSE;

/*
 * If you need access to certain yacc-generated variables and find that
 * they're static by default, uncomment the next line.  (this is not a
 * problem, yet.)
 */
/*#define __YYSCLASS*/

static Node *makeTypeCast(Node *arg, TypeName *typename);
static Node *makeStringConst(char *str, TypeName *typename);
static Node *makeIntConst(int val);
static Node *makeFloatConst(char *str);
static Node *makeAConst(Value *v);
static Node *makeRowExpr(List *opr, List *largs, List *rargs);
static Node *makeDistinctExpr(List *largs, List *rargs);
static Node *makeRowNullTest(NullTestType test, List *args);
static DefElem *makeDefElem(char *name, Node *arg);
static A_Const *makeBoolConst(bool state);
static FuncCall *makeOverlaps(List *largs, List *rargs);
static SelectStmt *findLeftmostSelect(SelectStmt *node);
static void insertSelectOptions(SelectStmt *stmt,
								List *sortClause, List *forUpdate,
								Node *limitOffset, Node *limitCount);
static Node *makeSetOp(SetOperation op, bool all, Node *larg, Node *rarg);
static Node *doNegate(Node *n);
static void doNegateFloat(Value *v);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 101 "gram.y"
typedef union YYSTYPE {
	int					ival;
	char				chr;
	char				*str;
	const char			*keyword;
	bool				boolean;
	JoinType			jtype;
	DropBehavior		dbehavior;
	OnCommitAction		oncommit;
	List				*list;
	FastList			fastlist;
	Node				*node;
	Value				*value;
	ColumnRef			*columnref;
	ObjectType			objtype;

	TypeName			*typnam;
	DefElem				*defelt;
	SortBy				*sortby;
	JoinExpr			*jexpr;
	IndexElem			*ielem;
	Alias				*alias;
	RangeVar			*range;
	A_Indices			*aind;
	ResTarget			*target;
	PrivTarget			*privtarget;

	InsertStmt			*istmt;
	VariableSetStmt		*vsetstmt;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 842 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 854 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  506
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   29425

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  337
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  362
/* YYNRULES -- Number of rules. */
#define YYNRULES  1464
/* YYNRULES -- Number of states. */
#define YYNSTATES  2491

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   574

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned short yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,   325,     2,     2,
     330,   331,   323,   321,   335,   322,   333,   324,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   336,   334,
     318,   317,   319,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   328,     2,   329,   326,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   320,   327,   332
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     9,    11,    13,    15,    17,    19,
      21,    23,    25,    27,    29,    31,    33,    35,    37,    39,
      41,    43,    45,    47,    49,    51,    53,    55,    57,    59,
      61,    63,    65,    67,    69,    71,    73,    75,    77,    79,
      81,    83,    85,    87,    89,    91,    93,    95,    97,    99,
     101,   103,   105,   107,   109,   111,   113,   115,   117,   119,
     121,   123,   125,   127,   129,   131,   133,   135,   137,   139,
     141,   143,   145,   147,   149,   151,   153,   154,   160,   162,
     163,   169,   175,   180,   184,   187,   188,   191,   195,   199,
     202,   204,   206,   208,   210,   214,   218,   222,   224,   230,
     233,   234,   237,   240,   247,   249,   251,   255,   262,   267,
     269,   270,   273,   274,   276,   278,   280,   283,   287,   291,
     295,   299,   303,   306,   312,   315,   319,   323,   325,   327,
     329,   333,   335,   337,   339,   342,   344,   346,   348,   350,
     352,   354,   356,   360,   367,   369,   371,   373,   375,   377,
     378,   380,   382,   385,   389,   394,   398,   401,   404,   408,
     413,   417,   420,   425,   427,   429,   431,   433,   435,   442,
     450,   460,   470,   480,   490,   498,   504,   512,   519,   526,
     533,   540,   544,   547,   549,   551,   552,   555,   566,   568,
     570,   572,   574,   576,   579,   580,   582,   584,   588,   592,
     594,   595,   598,   599,   603,   604,   606,   607,   618,   630,
     632,   634,   637,   640,   643,   646,   647,   649,   650,   652,
     656,   658,   660,   662,   666,   669,   670,   674,   676,   678,
     681,   683,   685,   688,   693,   696,   702,   704,   707,   710,
     713,   717,   720,   723,   724,   728,   730,   735,   740,   746,
     758,   762,   763,   765,   769,   771,   774,   777,   780,   781,
     783,   785,   788,   791,   792,   796,   800,   803,   805,   807,
     810,   813,   818,   819,   822,   825,   826,   830,   835,   840,
     841,   849,   853,   854,   856,   860,   862,   868,   873,   876,
     877,   880,   882,   885,   889,   892,   895,   898,   901,   905,
     909,   911,   912,   914,   916,   918,   921,   923,   926,   936,
     938,   939,   941,   943,   946,   947,   950,   951,   957,   959,
     960,   975,   995,   997,   999,  1001,  1005,  1011,  1013,  1015,
    1017,  1021,  1022,  1024,  1025,  1027,  1029,  1031,  1035,  1036,
    1038,  1040,  1042,  1044,  1046,  1048,  1051,  1052,  1054,  1057,
    1059,  1062,  1063,  1066,  1068,  1071,  1074,  1081,  1090,  1095,
    1100,  1105,  1110,  1118,  1122,  1124,  1128,  1132,  1134,  1136,
    1138,  1140,  1142,  1155,  1157,  1161,  1166,  1174,  1179,  1182,
    1184,  1185,  1187,  1188,  1196,  1201,  1203,  1205,  1207,  1209,
    1211,  1213,  1215,  1217,  1219,  1223,  1225,  1227,  1231,  1238,
    1248,  1256,  1266,  1275,  1284,  1291,  1300,  1302,  1304,  1306,
    1308,  1310,  1312,  1314,  1316,  1318,  1320,  1322,  1327,  1330,
    1335,  1338,  1339,  1341,  1343,  1345,  1347,  1350,  1353,  1355,
    1357,  1359,  1362,  1365,  1367,  1370,  1373,  1375,  1378,  1380,
    1382,  1390,  1399,  1401,  1403,  1406,  1408,  1412,  1414,  1416,
    1418,  1420,  1422,  1424,  1426,  1428,  1430,  1432,  1434,  1436,
    1438,  1441,  1444,  1447,  1450,  1453,  1455,  1459,  1461,  1464,
    1468,  1469,  1473,  1474,  1476,  1480,  1483,  1495,  1497,  1498,
    1501,  1502,  1504,  1508,  1511,  1517,  1522,  1524,  1527,  1528,
    1538,  1541,  1542,  1546,  1549,  1551,  1555,  1558,  1560,  1562,
    1564,  1566,  1568,  1570,  1575,  1577,  1580,  1583,  1586,  1588,
    1590,  1592,  1597,  1603,  1605,  1609,  1613,  1616,  1619,  1621,
    1625,  1628,  1629,  1635,  1643,  1645,  1647,  1655,  1657,  1661,
    1665,  1669,  1671,  1675,  1687,  1698,  1701,  1704,  1705,  1714,
    1719,  1724,  1726,  1728,  1730,  1731,  1741,  1748,  1755,  1763,
    1770,  1777,  1787,  1794,  1803,  1812,  1819,  1821,  1822,  1824,
    1825,  1826,  1841,  1843,  1845,  1849,  1853,  1855,  1857,  1859,
    1861,  1863,  1865,  1867,  1868,  1870,  1872,  1874,  1876,  1878,
    1879,  1886,  1889,  1892,  1895,  1898,  1901,  1904,  1908,  1911,
    1914,  1917,  1919,  1921,  1922,  1926,  1928,  1933,  1938,  1940,
    1941,  1944,  1947,  1955,  1958,  1964,  1967,  1968,  1972,  1976,
    1980,  1984,  1988,  1992,  1996,  2000,  2004,  2006,  2007,  2013,
    2018,  2022,  2029,  2034,  2041,  2048,  2054,  2062,  2069,  2071,
    2072,  2083,  2088,  2091,  2093,  2098,  2104,  2110,  2113,  2118,
    2120,  2122,  2124,  2125,  2127,  2128,  2130,  2131,  2135,  2136,
    2141,  2143,  2145,  2147,  2149,  2151,  2153,  2155,  2156,  2162,
    2166,  2167,  2169,  2173,  2175,  2177,  2179,  2181,  2185,  2195,
    2199,  2200,  2203,  2207,  2212,  2217,  2220,  2222,  2230,  2235,
    2237,  2241,  2244,  2249,  2254,  2258,  2259,  2262,  2265,  2268,
    2272,  2274,  2278,  2280,  2283,  2290,  2298,  2299,  2303,  2306,
    2309,  2312,  2313,  2316,  2319,  2321,  2323,  2327,  2331,  2333,
    2336,  2341,  2346,  2348,  2350,  2359,  2364,  2369,  2374,  2377,
    2378,  2382,  2386,  2391,  2396,  2401,  2406,  2409,  2411,  2413,
    2414,  2416,  2418,  2419,  2421,  2427,  2429,  2430,  2432,  2433,
    2437,  2439,  2443,  2447,  2450,  2453,  2455,  2460,  2465,  2468,
    2471,  2476,  2478,  2479,  2481,  2483,  2485,  2489,  2490,  2493,
    2494,  2498,  2502,  2504,  2505,  2508,  2509,  2512,  2513,  2515,
    2519,  2521,  2524,  2526,  2529,  2535,  2542,  2548,  2550,  2553,
    2555,  2560,  2564,  2569,  2573,  2579,  2584,  2590,  2595,  2601,
    2604,  2609,  2611,  2614,  2617,  2620,  2622,  2624,  2625,  2630,
    2633,  2635,  2638,  2641,  2646,  2650,  2655,  2658,  2659,  2661,
    2665,  2668,  2671,  2675,  2681,  2688,  2692,  2697,  2698,  2700,
    2702,  2704,  2706,  2708,  2711,  2717,  2720,  2722,  2724,  2726,
    2728,  2730,  2732,  2734,  2736,  2738,  2740,  2742,  2745,  2748,
    2751,  2754,  2757,  2759,  2763,  2764,  2770,  2774,  2775,  2781,
    2785,  2786,  2788,  2790,  2792,  2794,  2800,  2803,  2805,  2807,
    2809,  2811,  2817,  2820,  2823,  2826,  2828,  2832,  2836,  2839,
    2841,  2842,  2846,  2847,  2853,  2856,  2862,  2865,  2867,  2871,
    2875,  2876,  2878,  2880,  2882,  2884,  2886,  2888,  2892,  2896,
    2900,  2904,  2908,  2912,  2916,  2917,  2921,  2926,  2931,  2935,
    2939,  2943,  2948,  2952,  2958,  2963,  2968,  2972,  2976,  2980,
    2982,  2984,  2986,  2988,  2990,  2992,  2994,  2996,  2998,  3000,
    3002,  3004,  3006,  3008,  3010,  3015,  3017,  3022,  3024,  3028,
    3034,  3037,  3040,  3043,  3046,  3049,  3052,  3056,  3060,  3064,
    3068,  3072,  3076,  3080,  3084,  3088,  3092,  3095,  3098,  3102,
    3106,  3109,  3113,  3119,  3124,  3131,  3135,  3141,  3146,  3153,
    3158,  3165,  3171,  3179,  3182,  3186,  3189,  3194,  3198,  3203,
    3207,  3212,  3216,  3221,  3227,  3234,  3242,  3248,  3255,  3259,
    3264,  3269,  3276,  3279,  3281,  3283,  3287,  3290,  3293,  3296,
    3299,  3302,  3305,  3309,  3313,  3317,  3321,  3325,  3329,  3333,
    3337,  3341,  3345,  3348,  3351,  3357,  3364,  3372,  3374,  3376,
    3380,  3386,  3391,  3393,  3397,  3402,  3408,  3414,  3419,  3421,
    3423,  3428,  3430,  3435,  3437,  3442,  3444,  3449,  3451,  3453,
    3455,  3462,  3467,  3472,  3477,  3482,  3489,  3495,  3501,  3507,
    3512,  3519,  3524,  3526,  3529,  3532,  3535,  3540,  3547,  3548,
    3550,  3554,  3558,  3559,  3563,  3565,  3567,  3571,  3575,  3579,
    3581,  3583,  3585,  3587,  3589,  3591,  3593,  3595,  3600,  3604,
    3607,  3611,  3612,  3616,  3620,  3623,  3626,  3628,  3629,  3632,
    3635,  3639,  3642,  3644,  3646,  3650,  3656,  3663,  3668,  3670,
    3673,  3678,  3681,  3682,  3684,  3685,  3688,  3691,  3694,  3697,
    3700,  3704,  3706,  3710,  3714,  3716,  3718,  3720,  3724,  3729,
    3734,  3736,  3740,  3742,  3744,  3746,  3748,  3750,  3754,  3756,
    3758,  3760,  3764,  3766,  3768,  3770,  3772,  3774,  3776,  3778,
    3780,  3782,  3784,  3786,  3788,  3790,  3793,  3797,  3804,  3807,
    3809,  3811,  3813,  3815,  3817,  3819,  3821,  3823,  3825,  3827,
    3829,  3831,  3833,  3835,  3837,  3839,  3841,  3843,  3845,  3847,
    3849,  3851,  3853,  3855,  3857,  3859,  3861,  3863,  3865,  3867,
    3869,  3871,  3873,  3875,  3877,  3879,  3881,  3883,  3885,  3887,
    3889,  3891,  3893,  3895,  3897,  3899,  3901,  3903,  3905,  3907,
    3909,  3911,  3913,  3915,  3917,  3919,  3921,  3923,  3925,  3927,
    3929,  3931,  3933,  3935,  3937,  3939,  3941,  3943,  3945,  3947,
    3949,  3951,  3953,  3955,  3957,  3959,  3961,  3963,  3965,  3967,
    3969,  3971,  3973,  3975,  3977,  3979,  3981,  3983,  3985,  3987,
    3989,  3991,  3993,  3995,  3997,  3999,  4001,  4003,  4005,  4007,
    4009,  4011,  4013,  4015,  4017,  4019,  4021,  4023,  4025,  4027,
    4029,  4031,  4033,  4035,  4037,  4039,  4041,  4043,  4045,  4047,
    4049,  4051,  4053,  4055,  4057,  4059,  4061,  4063,  4065,  4067,
    4069,  4071,  4073,  4075,  4077,  4079,  4081,  4083,  4085,  4087,
    4089,  4091,  4093,  4095,  4097,  4099,  4101,  4103,  4105,  4107,
    4109,  4111,  4113,  4115,  4117,  4119,  4121,  4123,  4125,  4127,
    4129,  4131,  4133,  4135,  4137,  4139,  4141,  4143,  4145,  4147,
    4149,  4151,  4153,  4155,  4157,  4159,  4161,  4163,  4165,  4167,
    4169,  4171,  4173,  4175,  4177,  4179,  4181,  4183,  4185,  4187,
    4189,  4191,  4193,  4195,  4197,  4199,  4201,  4203,  4205,  4207,
    4209,  4211,  4213,  4215,  4217,  4219,  4221,  4223,  4225,  4227,
    4229,  4231,  4233,  4235,  4237,  4239,  4241,  4243,  4245,  4247,
    4249,  4251,  4253,  4255,  4257,  4259,  4261,  4263,  4265,  4267,
    4269,  4271,  4273,  4275,  4277,  4279,  4281,  4283,  4285,  4287,
    4289,  4291,  4293,  4295,  4297,  4299,  4301,  4303,  4305,  4307,
    4309,  4311,  4313,  4315,  4317,  4319,  4321,  4323,  4325,  4327,
    4329,  4331,  4333,  4335,  4337,  4339,  4341,  4343,  4345,  4347,
    4349,  4351,  4353,  4355,  4357,  4359,  4361,  4363,  4365,  4367,
    4369,  4371,  4373,  4375,  4377,  4379,  4381,  4383,  4385,  4387,
    4389,  4391,  4393,  4395,  4397,  4399,  4401,  4403,  4405,  4407,
    4409,  4411,  4413,  4415,  4417,  4419,  4421,  4423,  4425,  4427,
    4429,  4431,  4433,  4435,  4437,  4439,  4441,  4443,  4445,  4447,
    4449,  4451,  4453,  4455,  4457
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     338,     0,    -1,   339,    -1,   339,   334,   340,    -1,   340,
      -1,   539,    -1,   542,    -1,   352,    -1,   418,    -1,   375,
      -1,   344,    -1,   343,    -1,   547,    -1,   374,    -1,   378,
      -1,   545,    -1,   464,    -1,   371,    -1,   379,    -1,   413,
      -1,   446,    -1,   507,    -1,   544,    -1,   541,    -1,   489,
      -1,   349,    -1,   453,    -1,   425,    -1,   355,    -1,   417,
      -1,   388,    -1,   432,    -1,   341,    -1,   535,    -1,   562,
      -1,   572,    -1,   448,    -1,   567,    -1,   447,    -1,   509,
      -1,   354,    -1,   458,    -1,   430,    -1,   524,    -1,   459,
      -1,   445,    -1,   345,    -1,   540,    -1,   560,    -1,   553,
      -1,   467,    -1,   471,    -1,   483,    -1,   563,    -1,   526,
      -1,   534,    -1,   568,    -1,   525,    -1,   556,    -1,   510,
      -1,   502,    -1,   501,    -1,   504,    -1,   513,    -1,   472,
      -1,   516,    -1,   575,    -1,   528,    -1,   463,    -1,   527,
      -1,   571,    -1,   546,    -1,   370,    -1,   359,    -1,   369,
      -1,   533,    -1,    -1,    58,   288,   689,   342,   346,    -1,
     302,    -1,    -1,    11,   288,   689,   342,   346,    -1,    11,
     288,   689,   244,   360,    -1,    11,   288,   689,   370,    -1,
      87,   288,   348,    -1,   346,   347,    -1,    -1,   204,   688,
      -1,    91,   204,   688,    -1,   280,   204,   688,    -1,   261,
     687,    -1,    59,    -1,   177,    -1,    60,    -1,   178,    -1,
     125,   116,   348,    -1,   291,   285,   688,    -1,   348,   335,
     689,    -1,   689,    -1,    58,   116,   689,   342,   350,    -1,
     350,   351,    -1,    -1,   288,   348,    -1,   261,   687,    -1,
      11,   116,   689,   353,   288,   348,    -1,     7,    -1,    87,
      -1,    87,   116,   689,    -1,    58,   235,   356,    22,   689,
     357,    -1,    58,   235,   690,   357,    -1,   690,    -1,    -1,
     357,   358,    -1,    -1,   388,    -1,   471,    -1,   533,    -1,
     244,   360,    -1,   244,   158,   360,    -1,   244,   242,   360,
      -1,   690,   269,   361,    -1,   690,   317,   361,    -1,   267,
     307,   366,    -1,   272,   530,    -1,   242,    41,    17,   272,
     530,    -1,   170,   367,    -1,   242,    22,   368,    -1,   242,
      22,    74,    -1,   362,    -1,    74,    -1,   363,    -1,   362,
     335,   363,    -1,   365,    -1,   368,    -1,   422,    -1,   217,
      52,    -1,   241,    -1,   276,    -1,   102,    -1,   192,    -1,
     188,    -1,   688,    -1,   309,    -1,   632,   688,   634,    -1,
     632,   330,   687,   331,   688,   634,    -1,   422,    -1,    74,
      -1,   158,    -1,   688,    -1,    74,    -1,    -1,   690,    -1,
     311,    -1,   247,   690,    -1,   247,   267,   307,    -1,   247,
     272,   145,   153,    -1,   247,   242,    22,    -1,   247,    10,
      -1,   225,   690,    -1,   225,   267,   307,    -1,   225,   272,
     145,   153,    -1,   225,   242,    22,    -1,   225,    10,    -1,
     244,    54,   372,   373,    -1,    10,    -1,   678,    -1,    77,
      -1,   122,    -1,    43,    -1,    11,   262,   606,     7,   515,
     393,    -1,    11,   262,   606,    11,   515,   690,   376,    -1,
      11,   262,   606,    11,   515,   690,    87,   180,   184,    -1,
      11,   262,   606,    11,   515,   690,   244,   180,   184,    -1,
      11,   262,   606,    11,   515,   690,   244,   255,   424,    -1,
      11,   262,   606,    11,   515,   690,   244,   258,   690,    -1,
      11,   262,   606,    87,   515,   690,   377,    -1,    11,   262,
     606,     7,   400,    -1,    11,   262,   606,    87,    53,   679,
     377,    -1,    11,   262,   606,   244,   303,   190,    -1,    11,
     262,   677,    58,   270,   262,    -1,    11,   262,   677,   202,
     269,   689,    -1,    11,   262,   677,    46,   192,   679,    -1,
     244,    74,   643,    -1,    87,    74,    -1,    35,    -1,   227,
      -1,    -1,    45,   679,    -1,    57,   384,   677,   402,   385,
     380,   381,   386,   342,   382,    -1,   111,    -1,   269,    -1,
     688,    -1,   256,    -1,   257,    -1,   382,   383,    -1,    -1,
      28,    -1,   190,    -1,    80,   543,   688,    -1,   184,   543,
     688,    -1,    28,    -1,    -1,   302,   190,    -1,    -1,   387,
      81,   688,    -1,    -1,   289,    -1,    -1,    58,   389,   262,
     677,   330,   390,   331,   410,   411,   412,    -1,    58,   389,
     262,   677,   187,   677,   330,   390,   331,   411,   412,    -1,
     265,    -1,   263,    -1,   158,   265,    -1,   158,   263,    -1,
     114,   265,    -1,   114,   263,    -1,    -1,   391,    -1,    -1,
     392,    -1,   391,   335,   392,    -1,   393,    -1,   398,    -1,
     400,    -1,   690,   611,   394,    -1,   394,   395,    -1,    -1,
      53,   679,   396,    -1,   396,    -1,   397,    -1,   180,   184,
      -1,   184,    -1,   282,    -1,   212,   147,    -1,    42,   330,
     643,   331,    -1,    74,   644,    -1,   220,   677,   402,   405,
     406,    -1,    76,    -1,   180,    76,    -1,   130,    77,    -1,
     130,   122,    -1,   154,   677,   399,    -1,   126,    75,    -1,
      95,    75,    -1,    -1,    53,   679,   401,    -1,   401,    -1,
      42,   330,   643,   331,    -1,   282,   330,   403,   331,    -1,
     212,   147,   330,   403,   331,    -1,   108,   147,   330,   403,
     331,   220,   677,   402,   405,   406,   442,    -1,   330,   403,
     331,    -1,    -1,   404,    -1,   403,   335,   404,    -1,   690,
      -1,   163,   112,    -1,   163,   203,    -1,   163,   249,    -1,
      -1,   407,    -1,   408,    -1,   407,   408,    -1,   408,   407,
      -1,    -1,   192,   286,   409,    -1,   192,    79,   409,    -1,
     176,     6,    -1,   227,    -1,    35,    -1,   244,   184,    -1,
     244,    74,    -1,   129,   330,   676,   331,    -1,    -1,   302,
     190,    -1,   303,   190,    -1,    -1,   192,    51,    87,    -1,
     192,    51,    79,   233,    -1,   192,    51,   210,   233,    -1,
      -1,    58,   389,   262,   677,   414,    17,   575,    -1,   330,
     415,   331,    -1,    -1,   416,    -1,   415,   335,   416,    -1,
     690,    -1,    58,   389,   240,   677,   419,    -1,    11,   240,
     677,   419,    -1,   419,   420,    -1,    -1,    33,   422,    -1,
      67,    -1,   176,    67,    -1,   127,   421,   422,    -1,   164,
     422,    -1,   166,   422,    -1,   176,   164,    -1,   176,   166,
      -1,   253,   342,   422,    -1,   226,   342,   422,    -1,    32,
      -1,    -1,   423,    -1,   424,    -1,   310,    -1,   322,   310,
      -1,   687,    -1,   322,   687,    -1,    58,   426,   431,   149,
     368,   117,   427,   429,   428,    -1,   278,    -1,    -1,   679,
      -1,   667,    -1,   148,   688,    -1,    -1,   292,   427,    -1,
      -1,    87,   431,   149,   368,   377,    -1,   215,    -1,    -1,
      58,   274,   679,   433,   434,   192,   677,   436,    97,   216,
     685,   330,   439,   331,    -1,    58,    53,   274,   679,     8,
     434,   192,   677,   441,   442,   106,    88,   232,    97,   216,
     685,   330,   439,   331,    -1,    24,    -1,     8,    -1,   435,
      -1,   435,   196,   435,    -1,   435,   196,   435,   196,   435,
      -1,   135,    -1,    79,    -1,   286,    -1,   106,   437,   438,
      -1,    -1,    88,    -1,    -1,   232,    -1,   254,    -1,   440,
      -1,   439,   335,   440,    -1,    -1,   315,    -1,   310,    -1,
     688,    -1,   312,    -1,   313,    -1,   690,    -1,   111,   677,
      -1,    -1,   443,    -1,   443,   444,    -1,   444,    -1,   444,
     443,    -1,    -1,   180,    76,    -1,    76,    -1,   130,   122,
      -1,   130,    77,    -1,    87,   274,   679,   192,   677,   377,
      -1,    58,    19,   679,    42,   330,   643,   331,   442,    -1,
      87,    19,   679,   377,    -1,    58,     9,   685,   449,    -1,
      58,   194,   506,   449,    -1,    58,   279,   462,   449,    -1,
      58,   279,   462,    17,   330,   609,   331,    -1,   330,   450,
     331,    -1,   451,    -1,   450,   335,   451,    -1,   693,   317,
     452,    -1,   693,    -1,   495,    -1,   642,    -1,   422,    -1,
     688,    -1,    58,   194,    44,   462,   456,   106,   279,   611,
     289,   681,    17,   454,    -1,   455,    -1,   454,   335,   455,
      -1,   194,   687,   506,   457,    -1,   194,   687,   506,   330,
     505,   331,   457,    -1,   113,   687,   685,   491,    -1,   258,
     611,    -1,    74,    -1,    -1,   219,    -1,    -1,    87,   194,
      44,   462,   289,   681,   377,    -1,    87,   460,   461,   377,
      -1,   262,    -1,   240,    -1,   298,    -1,   128,    -1,   279,
      -1,    85,    -1,    55,    -1,   235,    -1,   462,    -1,   461,
     335,   462,    -1,   690,    -1,   667,    -1,   277,   582,   677,
      -1,    50,   192,   465,   462,   143,   466,    -1,    50,   192,
       9,   685,   330,   503,   331,   143,   466,    -1,    50,   192,
     113,   685,   491,   143,   466,    -1,    50,   192,   194,   506,
     330,   505,   331,   143,   466,    -1,    50,   192,    53,   679,
     192,   462,   143,   466,    -1,    50,   192,   234,   679,   192,
     462,   143,   466,    -1,    50,   192,   234,   679,   143,   466,
      -1,    50,   192,   274,   679,   192,   462,   143,   466,    -1,
      49,    -1,    68,    -1,   235,    -1,   128,    -1,   240,    -1,
     262,    -1,    85,    -1,   279,    -1,   298,    -1,   688,    -1,
     184,    -1,   103,   468,   470,   679,    -1,   103,   679,    -1,
     169,   468,   470,   679,    -1,   169,   679,    -1,    -1,   175,
      -1,   213,    -1,   104,    -1,   150,    -1,     4,   469,    -1,
     222,   469,    -1,   469,    -1,    10,    -1,   109,    -1,   109,
     469,    -1,   109,    10,    -1,    23,    -1,    23,   469,    -1,
      23,    10,    -1,   687,    -1,   322,   687,    -1,   111,    -1,
     125,    -1,   115,   473,   192,   476,   269,   477,   479,    -1,
     229,   480,   473,   192,   476,   111,   477,   377,    -1,   474,
      -1,    10,    -1,    10,   214,    -1,   475,    -1,   474,   335,
     475,    -1,   239,    -1,   135,    -1,   286,    -1,    79,    -1,
     234,    -1,   220,    -1,   274,    -1,    97,    -1,   287,    -1,
      58,    -1,   265,    -1,   263,    -1,   676,    -1,   262,   676,
      -1,   113,   481,    -1,    68,   678,    -1,   149,   678,    -1,
     235,   678,    -1,   478,    -1,   477,   335,   478,    -1,   690,
      -1,   116,   690,    -1,   302,   115,   195,    -1,    -1,   115,
     195,   106,    -1,    -1,   482,    -1,   481,   335,   482,    -1,
     685,   491,    -1,    58,   484,   128,   683,   192,   677,   485,
     330,   486,   331,   608,    -1,   282,    -1,    -1,   289,   681,
      -1,    -1,   487,    -1,   486,   335,   487,    -1,   682,   488,
      -1,   685,   330,   647,   331,   488,    -1,   330,   643,   331,
     488,    -1,   462,    -1,   289,   462,    -1,    -1,    58,   490,
     113,   685,   491,   228,   495,   497,   500,    -1,   196,   224,
      -1,    -1,   330,   492,   331,    -1,   330,   331,    -1,   493,
      -1,   492,   335,   493,    -1,   494,   496,    -1,   496,    -1,
     125,    -1,   198,    -1,   132,    -1,   496,    -1,   611,    -1,
     691,   668,   325,   279,    -1,   498,    -1,   497,   498,    -1,
      17,   499,    -1,   149,   368,    -1,   123,    -1,   252,    -1,
     299,    -1,    34,   192,   184,   133,    -1,   228,   184,   192,
     184,   133,    -1,   259,    -1,   100,   238,    78,    -1,   100,
     238,   142,    -1,   238,    78,    -1,   238,   142,    -1,   688,
      -1,   688,   335,   688,    -1,   302,   449,    -1,    -1,    87,
     113,   685,   491,   377,    -1,    87,     9,   685,   330,   503,
     331,   377,    -1,   611,    -1,   323,    -1,    87,   194,   506,
     330,   505,   331,   377,    -1,   611,    -1,   611,   335,   611,
      -1,   179,   335,   611,    -1,   611,   335,   179,    -1,   639,
      -1,   690,   333,   506,    -1,    58,    37,   330,   611,    17,
     611,   331,   302,   113,   482,   508,    -1,    58,    37,   330,
     611,    17,   611,   331,   303,   113,   508,    -1,    17,   124,
      -1,    17,    20,    -1,    -1,    87,    37,   330,   611,    17,
     611,   331,   377,    -1,   221,   511,   677,   512,    -1,   221,
      68,   679,   512,    -1,   128,    -1,   262,    -1,   107,    -1,
      -1,    11,     9,   685,   330,   503,   331,   223,   269,   679,
      -1,    11,    55,   462,   223,   269,   679,    -1,    11,    68,
     680,   223,   269,   680,    -1,    11,   113,   685,   491,   223,
     269,   679,    -1,    11,   116,   689,   223,   269,   689,    -1,
      11,   149,   679,   223,   269,   679,    -1,    11,   194,    44,
     462,   289,   681,   223,   269,   679,    -1,    11,   235,   679,
     223,   269,   679,    -1,    11,   262,   606,   223,   515,   514,
     269,   679,    -1,    11,   274,   679,   192,   606,   223,   269,
     679,    -1,    11,   288,   689,   223,   269,   689,    -1,   679,
      -1,    -1,    49,    -1,    -1,    -1,    58,   490,   234,   679,
      17,   517,   192,   522,   269,   677,   608,    84,   523,   518,
      -1,   181,    -1,   520,    -1,   330,   519,   331,    -1,   519,
     334,   521,    -1,   521,    -1,   575,    -1,   563,    -1,   571,
      -1,   567,    -1,   525,    -1,   520,    -1,    -1,   239,    -1,
     286,    -1,    79,    -1,   135,    -1,   136,    -1,    -1,    87,
     234,   679,   192,   677,   377,    -1,   182,   677,    -1,   156,
     677,    -1,   284,   677,    -1,   284,   323,    -1,     3,   529,
      -1,    25,   529,    -1,   253,   272,   531,    -1,    51,   529,
      -1,    92,   529,    -1,   231,   529,    -1,   304,    -1,   272,
      -1,    -1,   145,   153,   364,    -1,   532,    -1,   145,   153,
     364,   532,    -1,   532,   145,   153,   364,    -1,   530,    -1,
      -1,   217,   193,    -1,   217,   305,    -1,    58,   490,   298,
     677,   402,    17,   575,    -1,   157,   684,    -1,    58,    68,
     680,   342,   536,    -1,   536,   537,    -1,    -1,   161,   538,
     688,    -1,   161,   538,    74,    -1,   264,   538,   679,    -1,
     264,   538,    74,    -1,    90,   538,   688,    -1,    90,   538,
     687,    -1,    90,   538,    74,    -1,   202,   538,   679,    -1,
     202,   538,    74,    -1,   317,    -1,    -1,    11,    68,   680,
     244,   360,    -1,    11,    68,   680,   370,    -1,    87,    68,
     680,    -1,    58,    85,   462,   543,   611,   394,    -1,    11,
      85,   462,   376,    -1,    11,    85,   462,    87,   180,   184,
      -1,    11,    85,   462,   244,   180,   184,    -1,    11,    85,
     462,     7,   400,    -1,    11,    85,   462,    87,    53,   679,
     377,    -1,    11,    85,   462,   202,   269,   689,    -1,    17,
      -1,    -1,    58,   456,    55,   462,   106,   688,   269,   688,
     111,   462,    -1,    46,   683,   192,   677,    -1,    46,   677,
      -1,    46,    -1,   290,   550,   551,   549,    -1,   290,   550,
     551,   549,   677,    -1,   290,   550,   551,   549,   547,    -1,
     548,   549,    -1,   548,   549,   677,   552,    -1,    13,    -1,
      12,    -1,   296,    -1,    -1,   112,    -1,    -1,   110,    -1,
      -1,   330,   678,   331,    -1,    -1,    99,   555,   549,   554,
      -1,   575,    -1,   563,    -1,   571,    -1,   567,    -1,   572,
      -1,   560,    -1,   548,    -1,    -1,   211,   679,   557,    17,
     559,    -1,   330,   558,   331,    -1,    -1,   611,    -1,   558,
     335,   611,    -1,   575,    -1,   563,    -1,   571,    -1,   567,
      -1,    97,   679,   561,    -1,    58,   389,   262,   677,   414,
      17,    97,   679,   561,    -1,   330,   647,   331,    -1,    -1,
      70,   679,    -1,    70,   211,   679,    -1,   135,   141,   677,
     564,    -1,   293,   330,   673,   331,    -1,    74,   293,    -1,
     575,    -1,   330,   565,   331,   293,   330,   673,   331,    -1,
     330,   565,   331,   575,    -1,   566,    -1,   565,   335,   566,
      -1,   690,   646,    -1,    79,   111,   606,   608,    -1,   162,
     582,   676,   569,    -1,   125,   570,   167,    -1,    -1,     5,
     246,    -1,   232,   246,    -1,   232,    96,    -1,   246,   286,
      96,    -1,   246,    -1,   246,   232,    96,    -1,    96,    -1,
       5,    96,    -1,   286,   606,   244,   671,   598,   608,    -1,
      73,   679,   573,    66,   574,   106,   575,    -1,    -1,   573,
     176,   236,    -1,   573,   236,    -1,   573,    28,    -1,   573,
     134,    -1,    -1,   302,   119,    -1,   303,   119,    -1,   577,
      -1,   576,    -1,   330,   577,   331,    -1,   330,   576,   331,
      -1,   579,    -1,   578,   586,    -1,   578,   585,   595,   590,
      -1,   578,   585,   589,   596,    -1,   579,    -1,   576,    -1,
     239,   584,   669,   580,   598,   608,   593,   594,    -1,   578,
     281,   583,   578,    -1,   578,   139,   583,   578,    -1,   578,
      94,   583,   578,    -1,   141,   581,    -1,    -1,   265,   582,
     677,    -1,   263,   582,   677,    -1,   158,   265,   582,   677,
      -1,   158,   263,   582,   677,    -1,   114,   265,   582,   677,
      -1,   114,   263,   582,   677,    -1,   262,   677,    -1,   677,
      -1,   262,    -1,    -1,    10,    -1,    83,    -1,    -1,    83,
      -1,    83,   192,   330,   647,   331,    -1,    10,    -1,    -1,
     586,    -1,    -1,   197,    32,   587,    -1,   588,    -1,   587,
     335,   588,    -1,   643,   289,   642,    -1,   643,    18,    -1,
     643,    82,    -1,   643,    -1,   155,   591,   189,   592,    -1,
     189,   592,   155,   591,    -1,   155,   591,    -1,   189,   592,
      -1,   155,   591,   335,   592,    -1,   589,    -1,    -1,   643,
      -1,    10,    -1,   643,    -1,   116,    32,   647,    -1,    -1,
     118,   643,    -1,    -1,   106,   286,   597,    -1,   106,   217,
     193,    -1,   595,    -1,    -1,   187,   678,    -1,    -1,   111,
     599,    -1,    -1,   600,    -1,   599,   335,   600,    -1,   606,
      -1,   606,   602,    -1,   607,    -1,   607,   602,    -1,   607,
      17,   330,   609,   331,    -1,   607,    17,   690,   330,   609,
     331,    -1,   607,   690,   330,   609,   331,    -1,   576,    -1,
     576,   602,    -1,   601,    -1,   330,   601,   331,   602,    -1,
     330,   601,   331,    -1,   600,    61,   146,   600,    -1,   600,
     308,   600,    -1,   600,   603,   146,   600,   605,    -1,   600,
     146,   600,   605,    -1,   600,   172,   603,   146,   600,    -1,
     600,   172,   146,   600,    -1,    17,   690,   330,   678,   331,
      -1,    17,   690,    -1,   690,   330,   678,   331,    -1,   690,
      -1,   112,   604,    -1,   152,   604,    -1,   230,   604,    -1,
     131,    -1,   199,    -1,    -1,   289,   330,   678,   331,    -1,
     192,   643,    -1,   677,    -1,   677,   323,    -1,   193,   677,
      -1,   193,   330,   677,   331,    -1,   685,   330,   331,    -1,
     685,   330,   647,   331,    -1,   301,   643,    -1,    -1,   610,
      -1,   609,   335,   610,    -1,   690,   611,    -1,   613,   612,
      -1,   245,   613,   612,    -1,   613,    16,   328,   687,   329,
      -1,   245,   613,    16,   328,   687,   329,    -1,   612,   328,
     329,    -1,   612,   328,   687,   329,    -1,    -1,   615,    -1,
     616,    -1,   620,    -1,   624,    -1,   631,    -1,   632,   634,
      -1,   632,   330,   687,   331,   634,    -1,   691,   668,    -1,
     615,    -1,   616,    -1,   621,    -1,   625,    -1,   631,    -1,
     691,    -1,   137,    -1,   138,    -1,   250,    -1,    27,    -1,
     218,    -1,   105,   617,    -1,    86,   209,    -1,    72,   619,
      -1,    71,   619,    -1,   186,   618,    -1,    30,    -1,   330,
     687,   331,    -1,    -1,   330,   687,   335,   687,   331,    -1,
     330,   687,   331,    -1,    -1,   330,   687,   335,   687,   331,
      -1,   330,   687,   331,    -1,    -1,   622,    -1,   623,    -1,
     622,    -1,   623,    -1,    29,   629,   330,   687,   331,    -1,
      29,   629,    -1,   626,    -1,   627,    -1,   626,    -1,   627,
      -1,   628,   330,   687,   331,   630,    -1,   628,   630,    -1,
      40,   629,    -1,    39,   629,    -1,   294,    -1,   171,    40,
     629,    -1,   171,    39,   629,    -1,   173,   629,    -1,   295,
      -1,    -1,    40,   244,   690,    -1,    -1,   268,   330,   687,
     331,   633,    -1,   268,   633,    -1,   267,   330,   687,   331,
     633,    -1,   267,   633,    -1,   140,    -1,   302,   267,   307,
      -1,   303,   267,   307,    -1,    -1,   306,    -1,   168,    -1,
      69,    -1,   120,    -1,   165,    -1,   237,    -1,   306,   269,
     168,    -1,    69,   269,   120,    -1,    69,   269,   165,    -1,
      69,   269,   237,    -1,   120,   269,   165,    -1,   120,   269,
     237,    -1,   165,   269,   237,    -1,    -1,   636,   125,   576,
      -1,   636,   180,   125,   576,    -1,   636,   642,   638,   576,
      -1,   636,   642,   576,    -1,   636,   642,   636,    -1,   636,
     143,   184,    -1,   636,   143,   180,   184,    -1,   636,   200,
     636,    -1,   636,   143,    83,   111,   636,    -1,   232,   330,
     637,   331,    -1,   232,   330,   643,   331,    -1,   232,   330,
     331,    -1,   330,   637,   331,    -1,   647,   335,   643,    -1,
      15,    -1,   251,    -1,    10,    -1,   314,    -1,   640,    -1,
     321,    -1,   322,    -1,   323,    -1,   324,    -1,   325,    -1,
     326,    -1,   318,    -1,   319,    -1,   317,    -1,   314,    -1,
     194,   330,   506,   331,    -1,   639,    -1,   194,   330,   506,
     331,    -1,   645,    -1,   643,   332,   611,    -1,   643,    21,
     267,   307,   645,    -1,   321,   643,    -1,   322,   643,    -1,
     325,   643,    -1,   326,   643,    -1,   643,   325,    -1,   643,
     326,    -1,   643,   321,   643,    -1,   643,   322,   643,    -1,
     643,   323,   643,    -1,   643,   324,   643,    -1,   643,   325,
     643,    -1,   643,   326,   643,    -1,   643,   318,   643,    -1,
     643,   319,   643,    -1,   643,   317,   643,    -1,   643,   641,
     643,    -1,   641,   643,    -1,   643,   641,    -1,   643,    14,
     643,    -1,   643,   196,   643,    -1,   180,   643,    -1,   643,
     154,   643,    -1,   643,   154,   643,    93,   643,    -1,   643,
     180,   154,   643,    -1,   643,   180,   154,   643,    93,   643,
      -1,   643,   121,   643,    -1,   643,   121,   643,    93,   643,
      -1,   643,   180,   121,   643,    -1,   643,   180,   121,   643,
      93,   643,    -1,   643,   248,   269,   643,    -1,   643,   248,
     269,   643,    93,   643,    -1,   643,   180,   248,   269,   643,
      -1,   643,   180,   248,   269,   643,    93,   643,    -1,   643,
     144,    -1,   643,   143,   184,    -1,   643,   183,    -1,   643,
     143,   180,   184,    -1,   643,   143,   276,    -1,   643,   143,
     180,   276,    -1,   643,   143,   102,    -1,   643,   143,   180,
     102,    -1,   643,   143,   283,    -1,   643,   143,   180,   283,
      -1,   643,   143,    83,   111,   643,    -1,   643,   143,   187,
     330,   649,   331,    -1,   643,   143,   180,   187,   330,   649,
     331,    -1,   643,    26,   644,    14,   644,    -1,   643,   180,
      26,   644,    14,   644,    -1,   643,   125,   660,    -1,   643,
     180,   125,   660,    -1,   643,   642,   638,   576,    -1,   643,
     642,   638,   330,   643,   331,    -1,   282,   576,    -1,   635,
      -1,   645,    -1,   644,   332,   611,    -1,   321,   644,    -1,
     322,   644,    -1,   325,   644,    -1,   326,   644,    -1,   644,
     325,    -1,   644,   326,    -1,   644,   321,   644,    -1,   644,
     322,   644,    -1,   644,   323,   644,    -1,   644,   324,   644,
      -1,   644,   325,   644,    -1,   644,   326,   644,    -1,   644,
     318,   644,    -1,   644,   319,   644,    -1,   644,   317,   644,
      -1,   644,   641,   644,    -1,   641,   644,    -1,   644,   641,
      -1,   644,   143,    83,   111,   644,    -1,   644,   143,   187,
     330,   649,   331,    -1,   644,   143,   180,   187,   330,   649,
     331,    -1,   666,    -1,   686,    -1,   316,   668,   646,    -1,
     330,   643,   331,   668,   646,    -1,   330,   643,   331,   646,
      -1,   661,    -1,   685,   330,   331,    -1,   685,   330,   647,
     331,    -1,   685,   330,    10,   647,   331,    -1,   685,   330,
      83,   647,   331,    -1,   685,   330,   323,   331,    -1,    62,
      -1,    63,    -1,    63,   330,   687,   331,    -1,    64,    -1,
      64,   330,   687,   331,    -1,   159,    -1,   159,   330,   687,
     331,    -1,   160,    -1,   160,   330,   687,   331,    -1,    65,
      -1,   243,    -1,   288,    -1,    37,   330,   643,    17,   611,
     331,    -1,   101,   330,   648,   331,    -1,   201,   330,   653,
     331,    -1,   208,   330,   655,   331,    -1,   260,   330,   656,
     331,    -1,   273,   330,   643,    17,   611,   331,    -1,   275,
     330,    31,   659,   331,    -1,   275,   330,   151,   659,   331,
      -1,   275,   330,   271,   659,   331,    -1,   275,   330,   659,
     331,    -1,    56,   330,   643,   289,   462,   331,    -1,    56,
     330,   647,   331,    -1,   576,    -1,    98,   576,    -1,    16,
     576,    -1,    16,   651,    -1,   646,   328,   643,   329,    -1,
     646,   328,   643,   336,   643,   329,    -1,    -1,   643,    -1,
     647,   335,   643,    -1,   652,   111,   643,    -1,    -1,   649,
     335,   611,    -1,   611,    -1,   651,    -1,   650,   335,   651,
      -1,   328,   647,   329,    -1,   328,   650,   329,    -1,   309,
      -1,   306,    -1,   168,    -1,    69,    -1,   120,    -1,   165,
      -1,   237,    -1,   311,    -1,   643,   654,   657,   658,    -1,
     643,   654,   657,    -1,   207,   643,    -1,   644,   125,   644,
      -1,    -1,   643,   657,   658,    -1,   643,   658,   657,    -1,
     643,   657,    -1,   643,   658,    -1,   647,    -1,    -1,   111,
     643,    -1,   106,   643,    -1,   643,   111,   647,    -1,   111,
     647,    -1,   647,    -1,   576,    -1,   330,   647,   331,    -1,
      36,   665,   662,   664,    92,    -1,   185,   330,   643,   335,
     643,   331,    -1,    47,   330,   647,   331,    -1,   663,    -1,
     662,   663,    -1,   300,   643,   266,   643,    -1,    89,   643,
      -1,    -1,   643,    -1,    -1,   675,   646,    -1,   667,   646,
      -1,   675,   668,    -1,   333,   682,    -1,   333,   323,    -1,
     333,   682,   668,    -1,   670,    -1,   669,   335,   670,    -1,
     643,    17,   693,    -1,   643,    -1,   323,    -1,   672,    -1,
     671,   335,   672,    -1,   690,   646,   317,   643,    -1,   690,
     646,   317,    74,    -1,   674,    -1,   673,   335,   674,    -1,
     670,    -1,    74,    -1,   698,    -1,   690,    -1,   677,    -1,
     676,   335,   677,    -1,   675,    -1,   667,    -1,   679,    -1,
     678,   335,   679,    -1,   690,    -1,   690,    -1,   690,    -1,
     690,    -1,   690,    -1,   688,    -1,   692,    -1,   667,    -1,
     687,    -1,   310,    -1,   688,    -1,   312,    -1,   313,    -1,
     614,   688,    -1,   632,   688,   634,    -1,   632,   330,   687,
     331,   688,   634,    -1,   316,   646,    -1,   276,    -1,   102,
      -1,   184,    -1,   315,    -1,   311,    -1,   690,    -1,   309,
      -1,   694,    -1,   695,    -1,   309,    -1,   694,    -1,   309,
      -1,   694,    -1,   696,    -1,   309,    -1,   694,    -1,   695,
      -1,   696,    -1,   697,    -1,     3,    -1,     4,    -1,     5,
      -1,     6,    -1,     7,    -1,     8,    -1,     9,    -1,    11,
      -1,    19,    -1,    20,    -1,    21,    -1,    23,    -1,    24,
      -1,    25,    -1,    32,    -1,    33,    -1,    34,    -1,    35,
      -1,    38,    -1,    41,    -1,    43,    -1,    44,    -1,    45,
      -1,    46,    -1,    50,    -1,    51,    -1,    52,    -1,    54,
      -1,    55,    -1,    57,    -1,    59,    -1,    60,    -1,    66,
      -1,    67,    -1,    68,    -1,    69,    -1,    70,    -1,    73,
      -1,    75,    -1,    77,    -1,    78,    -1,    79,    -1,    80,
      -1,    81,    -1,    85,    -1,    86,    -1,    87,    -1,    88,
      -1,    90,    -1,    91,    -1,    93,    -1,    95,    -1,    96,
      -1,    97,    -1,    99,    -1,   100,    -1,   103,    -1,   104,
      -1,   107,    -1,   109,    -1,   113,    -1,   114,    -1,   117,
      -1,   119,    -1,   120,    -1,   122,    -1,   123,    -1,   124,
      -1,   126,    -1,   127,    -1,   128,    -1,   129,    -1,   132,
      -1,   133,    -1,   134,    -1,   135,    -1,   136,    -1,   142,
      -1,   145,    -1,   147,    -1,   148,    -1,   149,    -1,   150,
      -1,   153,    -1,   156,    -1,   157,    -1,   158,    -1,   161,
      -1,   162,    -1,   163,    -1,   164,    -1,   165,    -1,   166,
      -1,   167,    -1,   168,    -1,   169,    -1,   170,    -1,   171,
      -1,   175,    -1,   176,    -1,   177,    -1,   178,    -1,   181,
      -1,   182,    -1,   187,    -1,   190,    -1,   194,    -1,   195,
      -1,   198,    -1,   202,    -1,   203,    -1,   204,    -1,   205,
      -1,   206,    -1,   209,    -1,   211,    -1,   210,    -1,   213,
      -1,   214,    -1,   215,    -1,   216,    -1,   217,    -1,   219,
      -1,   221,    -1,   222,    -1,   223,    -1,   224,    -1,   225,
      -1,   226,    -1,   227,    -1,   228,    -1,   229,    -1,   231,
      -1,   233,    -1,   234,    -1,   235,    -1,   236,    -1,   237,
      -1,   238,    -1,   240,    -1,   241,    -1,   242,    -1,   244,
      -1,   246,    -1,   247,    -1,   249,    -1,   252,    -1,   253,
      -1,   254,    -1,   255,    -1,   256,    -1,   257,    -1,   258,
      -1,   261,    -1,   259,    -1,   263,    -1,   264,    -1,   265,
      -1,   270,    -1,   272,    -1,   274,    -1,   277,    -1,   278,
      -1,   279,    -1,   280,    -1,   283,    -1,   284,    -1,   285,
      -1,   286,    -1,   287,    -1,   290,    -1,   291,    -1,   292,
      -1,   293,    -1,   295,    -1,   297,    -1,   298,    -1,   299,
      -1,   302,    -1,   303,    -1,   304,    -1,   305,    -1,   306,
      -1,   307,    -1,    27,    -1,    29,    -1,    30,    -1,    39,
      -1,    40,    -1,    47,    -1,    56,    -1,    71,    -1,    72,
      -1,    98,    -1,   101,    -1,   105,    -1,   137,    -1,   138,
      -1,   140,    -1,   173,    -1,   179,    -1,   185,    -1,   186,
      -1,   201,    -1,   208,    -1,   218,    -1,   232,    -1,   245,
      -1,   250,    -1,   260,    -1,   267,    -1,   268,    -1,   273,
      -1,   275,    -1,   294,    -1,    22,    -1,    26,    -1,    28,
      -1,    61,    -1,   110,    -1,   112,    -1,   121,    -1,   125,
      -1,   131,    -1,   143,    -1,   144,    -1,   146,    -1,   152,
      -1,   154,    -1,   172,    -1,   183,    -1,   199,    -1,   200,
      -1,   230,    -1,   248,    -1,   296,    -1,    10,    -1,    12,
      -1,    13,    -1,    14,    -1,    15,    -1,    16,    -1,    17,
      -1,    18,    -1,    31,    -1,    36,    -1,    37,    -1,    42,
      -1,    48,    -1,    49,    -1,    53,    -1,    58,    -1,    62,
      -1,    63,    -1,    64,    -1,    65,    -1,    74,    -1,    76,
      -1,    82,    -1,    83,    -1,    84,    -1,    89,    -1,    92,
      -1,    94,    -1,   102,    -1,   106,    -1,   108,    -1,   111,
      -1,   115,    -1,   116,    -1,   118,    -1,   130,    -1,   139,
      -1,   141,    -1,   151,    -1,   155,    -1,   159,    -1,   160,
      -1,   174,    -1,   180,    -1,   184,    -1,   188,    -1,   189,
      -1,   191,    -1,   192,    -1,   193,    -1,   196,    -1,   197,
      -1,   207,    -1,   212,    -1,   220,    -1,   239,    -1,   243,
      -1,   251,    -1,   262,    -1,   266,    -1,   269,    -1,   271,
      -1,   276,    -1,   281,    -1,   282,    -1,   288,    -1,   289,
      -1,   300,    -1,   301,    -1,   191,    -1,   174,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   461,   461,   465,   471,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,   513,   514,
     515,   516,   517,   518,   519,   520,   521,   522,   523,   524,
     525,   526,   527,   528,   529,   530,   531,   532,   533,   534,
     535,   536,   537,   538,   539,   540,   541,   542,   543,   544,
     545,   546,   547,   548,   549,   550,   552,   563,   573,   574,
     585,   596,   604,   625,   637,   638,   642,   646,   650,   654,
     658,   662,   666,   670,   674,   678,   684,   685,   698,   711,
     712,   716,   720,   735,   745,   746,   758,   774,   786,   798,
     799,   803,   804,   812,   813,   814,   828,   834,   840,   848,
     855,   862,   870,   877,   884,   892,   899,   909,   910,   913,
     914,   917,   919,   921,   925,   926,   930,   931,   932,   933,
     945,   949,   953,   966,   993,   994,   995,   999,  1000,  1001,
    1005,  1006,  1011,  1017,  1023,  1029,  1035,  1044,  1050,  1056,
    1062,  1068,  1078,  1088,  1089,  1093,  1094,  1102,  1118,  1127,
    1137,  1146,  1155,  1165,  1176,  1186,  1195,  1205,  1213,  1222,
    1232,  1243,  1251,  1255,  1256,  1257,  1269,  1288,  1312,  1313,
    1322,  1323,  1324,  1330,  1331,  1336,  1340,  1344,  1348,  1357,
    1361,  1365,  1369,  1374,  1378,  1382,  1383,  1394,  1407,  1432,
    1433,  1434,  1435,  1436,  1437,  1438,  1442,  1443,  1447,  1451,
    1458,  1459,  1460,  1463,  1475,  1476,  1480,  1501,  1502,  1521,
    1531,  1541,  1551,  1561,  1571,  1589,  1617,  1623,  1629,  1635,
    1653,  1664,  1665,  1666,  1675,  1696,  1700,  1709,  1719,  1729,
    1747,  1748,  1752,  1753,  1756,  1762,  1766,  1773,  1778,  1790,
    1792,  1794,  1796,  1799,  1802,  1805,  1809,  1810,  1811,  1812,
    1813,  1816,  1817,  1821,  1822,  1823,  1826,  1827,  1828,  1829,
    1839,  1861,  1862,  1866,  1867,  1871,  1897,  1908,  1917,  1918,
    1921,  1925,  1929,  1933,  1937,  1941,  1945,  1949,  1953,  1957,
    1963,  1964,  1968,  1969,  1972,  1973,  1981,  1985,  2001,  2014,
    2015,  2023,  2025,  2029,  2030,  2034,  2035,  2039,  2049,  2050,
    2062,  2080,  2104,  2105,  2109,  2115,  2121,  2130,  2131,  2132,
    2136,  2141,  2151,  2152,  2156,  2157,  2161,  2162,  2163,  2167,
    2173,  2174,  2175,  2176,  2177,  2181,  2182,  2186,  2188,  2196,
    2203,  2212,  2216,  2217,  2221,  2222,  2227,  2248,  2267,  2290,
    2298,  2306,  2314,  2350,  2353,  2354,  2357,  2361,  2368,  2369,
    2370,  2371,  2384,  2398,  2399,  2403,  2413,  2423,  2432,  2441,
    2442,  2445,  2446,  2451,  2470,  2480,  2481,  2482,  2483,  2484,
    2485,  2486,  2487,  2491,  2492,  2495,  2496,  2507,  2529,  2538,
    2548,  2557,  2567,  2576,  2585,  2595,  2607,  2608,  2609,  2610,
    2611,  2612,  2613,  2614,  2615,  2619,  2620,  2630,  2637,  2646,
    2653,  2666,  2672,  2679,  2686,  2693,  2700,  2707,  2714,  2721,
    2728,  2735,  2742,  2749,  2756,  2763,  2773,  2774,  2777,  2778,
    2788,  2802,  2820,  2821,  2822,  2826,  2827,  2833,  2834,  2835,
    2836,  2837,  2838,  2839,  2840,  2841,  2842,  2843,  2844,  2851,
    2858,  2865,  2872,  2879,  2886,  2897,  2898,  2901,  2912,  2927,
    2928,  2932,  2933,  2938,  2939,  2944,  2963,  2978,  2979,  2983,
    2984,  2987,  2988,  2996,  3003,  3016,  3025,  3026,  3027,  3042,
    3057,  3058,  3061,  3062,  3066,  3067,  3070,  3078,  3081,  3082,
    3089,  3099,  3113,  3114,  3126,  3127,  3131,  3135,  3139,  3143,
    3147,  3151,  3155,  3159,  3163,  3167,  3171,  3175,  3181,  3182,
    3189,  3190,  3205,  3216,  3227,  3228,  3232,  3243,  3249,  3251,
    3253,  3258,  3260,  3271,  3281,  3293,  3294,  3295,  3299,  3320,
    3329,  3341,  3342,  3345,  3346,  3356,  3365,  3373,  3381,  3390,
    3398,  3406,  3415,  3423,  3435,  3444,  3454,  3455,  3458,  3459,
    3470,  3469,  3488,  3489,  3490,  3495,  3501,  3510,  3511,  3512,
    3513,  3514,  3518,  3519,  3523,  3524,  3525,  3526,  3530,  3531,
    3536,  3556,  3564,  3573,  3579,  3600,  3607,  3614,  3621,  3628,
    3635,  3644,  3645,  3646,  3650,  3653,  3656,  3663,  3673,  3675,
    3679,  3680,  3691,  3711,  3727,  3737,  3738,  3742,  3746,  3750,
    3754,  3758,  3762,  3766,  3770,  3774,  3785,  3786,  3797,  3805,
    3823,  3839,  3851,  3860,  3868,  3876,  3885,  3895,  3905,  3906,
    3920,  3943,  3950,  3957,  3974,  3986,  3998,  4010,  4022,  4037,
    4038,  4042,  4043,  4046,  4047,  4050,  4051,  4055,  4056,  4067,
    4078,  4079,  4080,  4081,  4082,  4083,  4087,  4088,  4098,  4108,
    4109,  4112,  4113,  4118,  4119,  4120,  4121,  4131,  4139,  4155,
    4156,  4166,  4172,  4188,  4196,  4203,  4210,  4217,  4224,  4234,
    4235,  4240,  4258,  4267,  4277,  4278,  4281,  4282,  4283,  4284,
    4285,  4286,  4287,  4288,  4299,  4320,  4332,  4333,  4334,  4335,
    4336,  4339,  4340,  4341,  4389,  4390,  4394,  4395,  4405,  4406,
    4412,  4418,  4427,  4428,  4455,  4470,  4474,  4478,  4485,  4486,
    4494,  4499,  4504,  4509,  4514,  4519,  4524,  4529,  4536,  4537,
    4540,  4541,  4542,  4549,  4550,  4551,  4552,  4556,  4557,  4561,
    4565,  4566,  4569,  4576,  4583,  4590,  4601,  4603,  4605,  4607,
    4609,  4620,  4622,  4626,  4627,  4637,  4649,  4650,  4654,  4655,
    4659,  4660,  4664,  4665,  4669,  4670,  4682,  4683,  4687,  4688,
    4698,  4702,  4707,  4714,  4722,  4729,  4739,  4749,  4768,  4775,
    4779,  4805,  4809,  4821,  4835,  4848,  4862,  4873,  4888,  4894,
    4899,  4905,  4912,  4913,  4914,  4915,  4919,  4920,  4932,  4933,
    4938,  4945,  4952,  4959,  4969,  4978,  4991,  4992,  4997,  5001,
    5007,  5028,  5033,  5039,  5045,  5055,  5057,  5060,  5072,  5073,
    5074,  5075,  5076,  5077,  5083,  5101,  5119,  5120,  5121,  5122,
    5123,  5127,  5138,  5142,  5146,  5150,  5154,  5158,  5162,  5166,
    5171,  5176,  5181,  5187,  5203,  5209,  5224,  5235,  5242,  5257,
    5268,  5279,  5283,  5291,  5295,  5303,  5324,  5345,  5349,  5355,
    5359,  5372,  5407,  5430,  5432,  5434,  5436,  5438,  5440,  5445,
    5446,  5450,  5451,  5455,  5481,  5500,  5522,  5537,  5541,  5542,
    5543,  5547,  5548,  5549,  5550,  5551,  5552,  5553,  5555,  5557,
    5560,  5563,  5565,  5568,  5570,  5585,  5594,  5605,  5615,  5625,
    5630,  5634,  5638,  5642,  5680,  5681,  5682,  5683,  5686,  5689,
    5690,  5691,  5694,  5695,  5698,  5699,  5700,  5701,  5702,  5703,
    5704,  5705,  5706,  5709,  5711,  5715,  5717,  5736,  5737,  5739,
    5757,  5759,  5761,  5763,  5765,  5767,  5769,  5771,  5773,  5775,
    5777,  5779,  5781,  5783,  5785,  5788,  5790,  5792,  5795,  5797,
    5799,  5802,  5804,  5813,  5815,  5824,  5826,  5835,  5837,  5847,
    5858,  5867,  5878,  5897,  5904,  5911,  5918,  5925,  5932,  5939,
    5946,  5953,  5960,  5967,  5969,  5973,  5977,  5983,  5989,  6016,
    6045,  6054,  6061,  6076,  6089,  6091,  6093,  6095,  6097,  6099,
    6101,  6103,  6105,  6107,  6109,  6111,  6113,  6115,  6117,  6119,
    6121,  6123,  6125,  6127,  6129,  6131,  6135,  6149,  6150,  6151,
    6163,  6171,  6184,  6186,  6195,  6204,  6217,  6226,  6251,  6277,
    6300,  6330,  6353,  6384,  6407,  6437,  6460,  6491,  6500,  6509,
    6518,  6520,  6529,  6543,  6553,  6565,  6581,  6593,  6602,  6611,
    6620,  6634,  6643,  6652,  6661,  6670,  6679,  6686,  6694,  6697,
    6702,  6712,  6719,  6722,  6726,  6732,  6734,  6738,  6744,  6757,
    6758,  6759,  6760,  6761,  6762,  6763,  6764,  6773,  6777,  6784,
    6791,  6792,  6807,  6811,  6815,  6819,  6826,  6831,  6835,  6838,
    6841,  6842,  6843,  6846,  6853,  6875,  6883,  6887,  6897,  6898,
    6902,  6912,  6913,  6916,  6917,  6925,  6931,  6940,  6944,  6946,
    6948,  6962,  6963,  6967,  6974,  6981,  7000,  7001,  7005,  7012,
    7023,  7024,  7028,  7029,  7046,  7047,  7051,  7052,  7056,  7063,
    7088,  7090,  7095,  7098,  7101,  7103,  7105,  7107,  7109,  7111,
    7118,  7125,  7132,  7139,  7146,  7158,  7166,  7177,  7200,  7208,
    7212,  7216,  7224,  7225,  7226,  7241,  7242,  7243,  7248,  7249,
    7255,  7256,  7257,  7263,  7264,  7265,  7266,  7267,  7283,  7284,
    7285,  7286,  7287,  7288,  7289,  7290,  7291,  7292,  7293,  7294,
    7295,  7296,  7297,  7298,  7299,  7300,  7301,  7302,  7303,  7304,
    7305,  7306,  7307,  7308,  7309,  7310,  7311,  7312,  7313,  7314,
    7315,  7316,  7317,  7318,  7319,  7320,  7321,  7322,  7323,  7324,
    7325,  7326,  7327,  7328,  7329,  7330,  7331,  7332,  7333,  7334,
    7335,  7336,  7337,  7338,  7339,  7340,  7341,  7342,  7343,  7344,
    7345,  7346,  7347,  7348,  7349,  7350,  7351,  7352,  7353,  7354,
    7355,  7356,  7357,  7358,  7359,  7360,  7361,  7362,  7363,  7364,
    7365,  7366,  7367,  7368,  7369,  7370,  7371,  7372,  7373,  7374,
    7375,  7376,  7377,  7378,  7379,  7380,  7381,  7382,  7383,  7384,
    7385,  7386,  7387,  7388,  7389,  7390,  7391,  7392,  7393,  7394,
    7395,  7396,  7397,  7398,  7399,  7400,  7401,  7402,  7403,  7404,
    7405,  7406,  7407,  7408,  7409,  7410,  7411,  7412,  7413,  7414,
    7415,  7416,  7417,  7418,  7419,  7420,  7421,  7422,  7423,  7424,
    7425,  7426,  7427,  7428,  7429,  7430,  7431,  7432,  7433,  7434,
    7435,  7436,  7437,  7438,  7439,  7440,  7441,  7442,  7443,  7444,
    7445,  7446,  7447,  7448,  7449,  7450,  7451,  7452,  7453,  7454,
    7455,  7456,  7457,  7458,  7459,  7460,  7461,  7462,  7463,  7464,
    7465,  7466,  7480,  7481,  7482,  7483,  7484,  7485,  7486,  7487,
    7488,  7489,  7490,  7491,  7492,  7493,  7494,  7495,  7496,  7497,
    7498,  7499,  7500,  7501,  7502,  7503,  7504,  7505,  7506,  7507,
    7508,  7509,  7510,  7524,  7525,  7526,  7527,  7528,  7529,  7530,
    7531,  7532,  7533,  7534,  7535,  7536,  7537,  7538,  7539,  7540,
    7541,  7542,  7543,  7544,  7554,  7555,  7556,  7557,  7558,  7559,
    7560,  7561,  7562,  7563,  7564,  7565,  7566,  7567,  7568,  7569,
    7570,  7571,  7572,  7573,  7574,  7575,  7576,  7577,  7578,  7579,
    7580,  7581,  7582,  7583,  7584,  7585,  7586,  7587,  7588,  7589,
    7590,  7591,  7592,  7593,  7594,  7595,  7596,  7597,  7598,  7599,
    7600,  7601,  7602,  7603,  7604,  7605,  7606,  7607,  7608,  7609,
    7610,  7611,  7612,  7613,  7614,  7615,  7616,  7617,  7618,  7619,
    7620,  7621,  7622,  7627,  7636
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ABORT_P", "ABSOLUTE_P", "ACCESS", 
  "ACTION", "ADD", "AFTER", "AGGREGATE", "ALL", "ALTER", "ANALYSE", 
  "ANALYZE", "AND", "ANY", "ARRAY", "AS", "ASC", "ASSERTION", 
  "ASSIGNMENT", "AT", "AUTHORIZATION", "BACKWARD", "BEFORE", "BEGIN_P", 
  "BETWEEN", "BIGINT", "BINARY", "BIT", "BOOLEAN_P", "BOTH", "BY", 
  "CACHE", "CALLED", "CASCADE", "CASE", "CAST", "CHAIN", "CHAR_P", 
  "CHARACTER", "CHARACTERISTICS", "CHECK", "CHECKPOINT", "CLASS", "CLOSE", 
  "CLUSTER", "COALESCE", "COLLATE", "COLUMN", "COMMENT", "COMMIT", 
  "COMMITTED", "CONSTRAINT", "CONSTRAINTS", "CONVERSION_P", "CONVERT", 
  "COPY", "CREATE", "CREATEDB", "CREATEUSER", "CROSS", "CURRENT_DATE", 
  "CURRENT_TIME", "CURRENT_TIMESTAMP", "CURRENT_USER", "CURSOR", "CYCLE", 
  "DATABASE", "DAY_P", "DEALLOCATE", "DEC", "DECIMAL_P", "DECLARE", 
  "DEFAULT", "DEFAULTS", "DEFERRABLE", "DEFERRED", "DEFINER", "DELETE_P", 
  "DELIMITER", "DELIMITERS", "DESC", "DISTINCT", "DO", "DOMAIN_P", 
  "DOUBLE_P", "DROP", "EACH", "ELSE", "ENCODING", "ENCRYPTED", "END_P", 
  "ESCAPE", "EXCEPT", "EXCLUDING", "EXCLUSIVE", "EXECUTE", "EXISTS", 
  "EXPLAIN", "EXTERNAL", "EXTRACT", "FALSE_P", "FETCH", "FIRST_P", 
  "FLOAT_P", "FOR", "FORCE", "FOREIGN", "FORWARD", "FREEZE", "FROM", 
  "FULL", "FUNCTION", "GLOBAL", "GRANT", "GROUP_P", "HANDLER", "HAVING", 
  "HOLD", "HOUR_P", "ILIKE", "IMMEDIATE", "IMMUTABLE", "IMPLICIT_P", 
  "IN_P", "INCLUDING", "INCREMENT", "INDEX", "INHERITS", "INITIALLY", 
  "INNER_P", "INOUT", "INPUT_P", "INSENSITIVE", "INSERT", "INSTEAD", 
  "INT_P", "INTEGER", "INTERSECT", "INTERVAL", "INTO", "INVOKER", "IS", 
  "ISNULL", "ISOLATION", "JOIN", "KEY", "LANCOMPILER", "LANGUAGE", 
  "LAST_P", "LEADING", "LEFT", "LEVEL", "LIKE", "LIMIT", "LISTEN", "LOAD", 
  "LOCAL", "LOCALTIME", "LOCALTIMESTAMP", "LOCATION", "LOCK_P", "MATCH", 
  "MAXVALUE", "MINUTE_P", "MINVALUE", "MODE", "MONTH_P", "MOVE", "NAMES", 
  "NATIONAL", "NATURAL", "NCHAR", "NEW", "NEXT", "NO", "NOCREATEDB", 
  "NOCREATEUSER", "NONE", "NOT", "NOTHING", "NOTIFY", "NOTNULL", "NULL_P", 
  "NULLIF", "NUMERIC", "OF", "OFF", "OFFSET", "OIDS", "OLD", "ON", "ONLY", 
  "OPERATOR", "OPTION", "OR", "ORDER", "OUT_P", "OUTER_P", "OVERLAPS", 
  "OVERLAY", "OWNER", "PARTIAL", "PASSWORD", "PATH_P", "PENDANT", 
  "PLACING", "POSITION", "PRECISION", "PRESERVE", "PREPARE", "PRIMARY", 
  "PRIOR", "PRIVILEGES", "PROCEDURAL", "PROCEDURE", "READ", "REAL", 
  "RECHECK", "REFERENCES", "REINDEX", "RELATIVE_P", "RENAME", "REPLACE", 
  "RESET", "RESTART", "RESTRICT", "RETURNS", "REVOKE", "RIGHT", 
  "ROLLBACK", "ROW", "ROWS", "RULE", "SCHEMA", "SCROLL", "SECOND_P", 
  "SECURITY", "SELECT", "SEQUENCE", "SERIALIZABLE", "SESSION", 
  "SESSION_USER", "SET", "SETOF", "SHARE", "SHOW", "SIMILAR", "SIMPLE", 
  "SMALLINT", "SOME", "STABLE", "START", "STATEMENT", "STATISTICS", 
  "STDIN", "STDOUT", "STORAGE", "STRICT_P", "SUBSTRING", "SYSID", "TABLE", 
  "TEMP", "TEMPLATE", "TEMPORARY", "THEN", "TIME", "TIMESTAMP", "TO", 
  "TOAST", "TRAILING", "TRANSACTION", "TREAT", "TRIGGER", "TRIM", 
  "TRUE_P", "TRUNCATE", "TRUSTED", "TYPE_P", "UNENCRYPTED", "UNION", 
  "UNIQUE", "UNKNOWN", "UNLISTEN", "UNTIL", "UPDATE", "USAGE", "USER", 
  "USING", "VACUUM", "VALID", "VALIDATOR", "VALUES", "VARCHAR", "VARYING", 
  "VERBOSE", "VERSION", "VIEW", "VOLATILE", "WHEN", "WHERE", "WITH", 
  "WITHOUT", "WORK", "WRITE", "YEAR_P", "ZONE", "UNIONJOIN", "IDENT", 
  "FCONST", "SCONST", "BCONST", "XCONST", "Op", "ICONST", "PARAM", "'='", 
  "'<'", "'>'", "POSTFIXOP", "'+'", "'-'", "'*'", "'/'", "'%'", "'^'", 
  "UMINUS", "'['", "']'", "'('", "')'", "TYPECAST", "'.'", "';'", "','", 
  "':'", "$accept", "stmtblock", "stmtmulti", "stmt", "CreateUserStmt", 
  "opt_with", "AlterUserStmt", "AlterUserSetStmt", "DropUserStmt", 
  "OptUserList", "OptUserElem", "user_list", "CreateGroupStmt", 
  "OptGroupList", "OptGroupElem", "AlterGroupStmt", "add_drop", 
  "DropGroupStmt", "CreateSchemaStmt", "OptSchemaName", 
  "OptSchemaEltList", "schema_stmt", "VariableSetStmt", "set_rest", 
  "var_list_or_default", "var_list", "var_value", "iso_level", 
  "opt_boolean", "zone_value", "opt_encoding", "ColId_or_Sconst", 
  "VariableShowStmt", "VariableResetStmt", "ConstraintsSetStmt", 
  "constraints_set_list", "constraints_set_mode", "CheckPointStmt", 
  "AlterTableStmt", "alter_column_default", "opt_drop_behavior", 
  "ClosePortalStmt", "CopyStmt", "copy_from", "copy_file_name", 
  "copy_opt_list", "copy_opt_item", "opt_binary", "opt_oids", 
  "copy_delimiter", "opt_using", "CreateStmt", "OptTemp", 
  "OptTableElementList", "TableElementList", "TableElement", "columnDef", 
  "ColQualList", "ColConstraint", "ColConstraintElem", "ConstraintAttr", 
  "TableLikeClause", "like_including_defaults", "TableConstraint", 
  "ConstraintElem", "opt_column_list", "columnList", "columnElem", 
  "key_match", "key_actions", "key_update", "key_delete", "key_action", 
  "OptInherit", "OptWithOids", "OnCommitOption", "CreateAsStmt", 
  "OptCreateAs", "CreateAsList", "CreateAsElement", "CreateSeqStmt", 
  "AlterSeqStmt", "OptSeqList", "OptSeqElem", "opt_by", "NumericOnly", 
  "FloatOnly", "IntegerOnly", "CreatePLangStmt", "opt_trusted", 
  "handler_name", "opt_lancompiler", "opt_validator", "DropPLangStmt", 
  "opt_procedural", "CreateTrigStmt", "TriggerActionTime", 
  "TriggerEvents", "TriggerOneEvent", "TriggerForSpec", "TriggerForOpt", 
  "TriggerForType", "TriggerFuncArgs", "TriggerFuncArg", 
  "OptConstrFromTable", "ConstraintAttributeSpec", 
  "ConstraintDeferrabilitySpec", "ConstraintTimeSpec", "DropTrigStmt", 
  "CreateAssertStmt", "DropAssertStmt", "DefineStmt", "definition", 
  "def_list", "def_elem", "def_arg", "CreateOpClassStmt", 
  "opclass_item_list", "opclass_item", "opt_default", "opt_recheck", 
  "DropOpClassStmt", "DropStmt", "drop_type", "any_name_list", "any_name", 
  "TruncateStmt", "CommentStmt", "comment_type", "comment_text", 
  "FetchStmt", "fetch_direction", "fetch_count", "from_in", "GrantStmt", 
  "RevokeStmt", "privileges", "privilege_list", "privilege", 
  "privilege_target", "grantee_list", "grantee", "opt_grant_grant_option", 
  "opt_revoke_grant_option", "function_with_argtypes_list", 
  "function_with_argtypes", "IndexStmt", "index_opt_unique", 
  "access_method_clause", "index_params", "index_elem", "opt_class", 
  "CreateFunctionStmt", "opt_or_replace", "func_args", "func_args_list", 
  "func_arg", "opt_arg", "func_return", "func_type", 
  "createfunc_opt_list", "createfunc_opt_item", "func_as", 
  "opt_definition", "RemoveFuncStmt", "RemoveAggrStmt", "aggr_argtype", 
  "RemoveOperStmt", "oper_argtypes", "any_operator", "CreateCastStmt", 
  "cast_context", "DropCastStmt", "ReindexStmt", "reindex_type", 
  "opt_force", "RenameStmt", "opt_name", "opt_column", "RuleStmt", "@1", 
  "RuleActionList", "RuleActionMulti", "RuleActionStmt", 
  "RuleActionStmtOrEmpty", "event", "opt_instead", "DropRuleStmt", 
  "NotifyStmt", "ListenStmt", "UnlistenStmt", "TransactionStmt", 
  "opt_transaction", "transaction_mode_list", 
  "transaction_mode_list_or_empty", "transaction_access_mode", "ViewStmt", 
  "LoadStmt", "CreatedbStmt", "createdb_opt_list", "createdb_opt_item", 
  "opt_equal", "AlterDatabaseSetStmt", "DropdbStmt", "CreateDomainStmt", 
  "AlterDomainStmt", "opt_as", "CreateConversionStmt", "ClusterStmt", 
  "VacuumStmt", "AnalyzeStmt", "analyze_keyword", "opt_verbose", 
  "opt_full", "opt_freeze", "opt_name_list", "ExplainStmt", 
  "ExplainableStmt", "opt_analyze", "PrepareStmt", "prep_type_clause", 
  "prep_type_list", "PreparableStmt", "ExecuteStmt", 
  "execute_param_clause", "DeallocateStmt", "InsertStmt", "insert_rest", 
  "insert_column_list", "insert_column_item", "DeleteStmt", "LockStmt", 
  "opt_lock", "lock_type", "UpdateStmt", "DeclareCursorStmt", 
  "cursor_options", "opt_hold", "SelectStmt", "select_with_parens", 
  "select_no_parens", "select_clause", "simple_select", "into_clause", 
  "OptTempTableName", "opt_table", "opt_all", "opt_distinct", 
  "opt_sort_clause", "sort_clause", "sortby_list", "sortby", 
  "select_limit", "opt_select_limit", "select_limit_value", 
  "select_offset_value", "group_clause", "having_clause", 
  "for_update_clause", "opt_for_update_clause", "update_list", 
  "from_clause", "from_list", "table_ref", "joined_table", "alias_clause", 
  "join_type", "join_outer", "join_qual", "relation_expr", "func_table", 
  "where_clause", "TableFuncElementList", "TableFuncElement", "Typename", 
  "opt_array_bounds", "SimpleTypename", "ConstTypename", "GenericType", 
  "Numeric", "opt_float", "opt_numeric", "opt_decimal", "Bit", "ConstBit", 
  "BitWithLength", "BitWithoutLength", "Character", "ConstCharacter", 
  "CharacterWithLength", "CharacterWithoutLength", "character", 
  "opt_varying", "opt_charset", "ConstDatetime", "ConstInterval", 
  "opt_timezone", "opt_interval", "r_expr", "row", "row_descriptor", 
  "sub_type", "all_Op", "MathOp", "qual_Op", "qual_all_Op", "a_expr", 
  "b_expr", "c_expr", "opt_indirection", "expr_list", "extract_list", 
  "type_list", "array_expr_list", "array_expr", "extract_arg", 
  "overlay_list", "overlay_placing", "position_list", "substr_list", 
  "substr_from", "substr_for", "trim_list", "in_expr", "case_expr", 
  "when_clause_list", "when_clause", "case_default", "case_arg", 
  "columnref", "dotted_name", "attrs", "target_list", "target_el", 
  "update_target_list", "update_target_el", "insert_target_list", 
  "insert_target_el", "relation_name", "qualified_name_list", 
  "qualified_name", "name_list", "name", "database_name", "access_method", 
  "attr_name", "index_name", "file_name", "func_name", "AexprConst", 
  "Iconst", "Sconst", "UserId", "ColId", "type_name", "function_name", 
  "ColLabel", "unreserved_keyword", "col_name_keyword", 
  "func_name_keyword", "reserved_keyword", "SpecialRuleRelation", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,   513,   514,
     515,   516,   517,   518,   519,   520,   521,   522,   523,   524,
     525,   526,   527,   528,   529,   530,   531,   532,   533,   534,
     535,   536,   537,   538,   539,   540,   541,   542,   543,   544,
     545,   546,   547,   548,   549,   550,   551,   552,   553,   554,
     555,   556,   557,   558,   559,   560,   561,   562,   563,   564,
     565,   566,   567,   568,   569,   570,   571,    61,    60,    62,
     572,    43,    45,    42,    47,    37,    94,   573,    91,    93,
      40,    41,   574,    46,    59,    44,    58
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned short yyr1[] =
{
       0,   337,   338,   339,   339,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   340,   340,   340,
     340,   340,   340,   340,   340,   340,   340,   341,   342,   342,
     343,   344,   344,   345,   346,   346,   347,   347,   347,   347,
     347,   347,   347,   347,   347,   347,   348,   348,   349,   350,
     350,   351,   351,   352,   353,   353,   354,   355,   355,   356,
     356,   357,   357,   358,   358,   358,   359,   359,   359,   360,
     360,   360,   360,   360,   360,   360,   360,   361,   361,   362,
     362,   363,   363,   363,   364,   364,   365,   365,   365,   365,
     366,   366,   366,   366,   366,   366,   366,   367,   367,   367,
     368,   368,   369,   369,   369,   369,   369,   370,   370,   370,
     370,   370,   371,   372,   372,   373,   373,   374,   375,   375,
     375,   375,   375,   375,   375,   375,   375,   375,   375,   375,
     375,   376,   376,   377,   377,   377,   378,   379,   380,   380,
     381,   381,   381,   382,   382,   383,   383,   383,   383,   384,
     384,   385,   385,   386,   386,   387,   387,   388,   388,   389,
     389,   389,   389,   389,   389,   389,   390,   390,   391,   391,
     392,   392,   392,   393,   394,   394,   395,   395,   395,   396,
     396,   396,   396,   396,   396,   396,   397,   397,   397,   397,
     398,   399,   399,   399,   400,   400,   401,   401,   401,   401,
     402,   402,   403,   403,   404,   405,   405,   405,   405,   406,
     406,   406,   406,   406,   407,   408,   409,   409,   409,   409,
     409,   410,   410,   411,   411,   411,   412,   412,   412,   412,
     413,   414,   414,   415,   415,   416,   417,   418,   419,   419,
     420,   420,   420,   420,   420,   420,   420,   420,   420,   420,
     421,   421,   422,   422,   423,   423,   424,   424,   425,   426,
     426,   427,   427,   428,   428,   429,   429,   430,   431,   431,
     432,   432,   433,   433,   434,   434,   434,   435,   435,   435,
     436,   436,   437,   437,   438,   438,   439,   439,   439,   440,
     440,   440,   440,   440,   440,   441,   441,   442,   442,   442,
     442,   442,   443,   443,   444,   444,   445,   446,   447,   448,
     448,   448,   448,   449,   450,   450,   451,   451,   452,   452,
     452,   452,   453,   454,   454,   455,   455,   455,   455,   456,
     456,   457,   457,   458,   459,   460,   460,   460,   460,   460,
     460,   460,   460,   461,   461,   462,   462,   463,   464,   464,
     464,   464,   464,   464,   464,   464,   465,   465,   465,   465,
     465,   465,   465,   465,   465,   466,   466,   467,   467,   467,
     467,   468,   468,   468,   468,   468,   468,   468,   468,   468,
     468,   468,   468,   468,   468,   468,   469,   469,   470,   470,
     471,   472,   473,   473,   473,   474,   474,   475,   475,   475,
     475,   475,   475,   475,   475,   475,   475,   475,   475,   476,
     476,   476,   476,   476,   476,   477,   477,   478,   478,   479,
     479,   480,   480,   481,   481,   482,   483,   484,   484,   485,
     485,   486,   486,   487,   487,   487,   488,   488,   488,   489,
     490,   490,   491,   491,   492,   492,   493,   493,   494,   494,
     494,   495,   496,   496,   497,   497,   498,   498,   498,   498,
     498,   498,   498,   498,   498,   498,   498,   498,   499,   499,
     500,   500,   501,   502,   503,   503,   504,   505,   505,   505,
     505,   506,   506,   507,   507,   508,   508,   508,   509,   510,
     510,   511,   511,   512,   512,   513,   513,   513,   513,   513,
     513,   513,   513,   513,   513,   513,   514,   514,   515,   515,
     517,   516,   518,   518,   518,   519,   519,   520,   520,   520,
     520,   520,   521,   521,   522,   522,   522,   522,   523,   523,
     524,   525,   526,   527,   527,   528,   528,   528,   528,   528,
     528,   529,   529,   529,   530,   530,   530,   530,   531,   531,
     532,   532,   533,   534,   535,   536,   536,   537,   537,   537,
     537,   537,   537,   537,   537,   537,   538,   538,   539,   539,
     540,   541,   542,   542,   542,   542,   542,   542,   543,   543,
     544,   545,   545,   545,   546,   546,   546,   547,   547,   548,
     548,   549,   549,   550,   550,   551,   551,   552,   552,   553,
     554,   554,   554,   554,   554,   554,   555,   555,   556,   557,
     557,   558,   558,   559,   559,   559,   559,   560,   560,   561,
     561,   562,   562,   563,   564,   564,   564,   564,   564,   565,
     565,   566,   567,   568,   569,   569,   570,   570,   570,   570,
     570,   570,   570,   570,   571,   572,   573,   573,   573,   573,
     573,   574,   574,   574,   575,   575,   576,   576,   577,   577,
     577,   577,   578,   578,   579,   579,   579,   579,   580,   580,
     581,   581,   581,   581,   581,   581,   581,   581,   582,   582,
     583,   583,   583,   584,   584,   584,   584,   585,   585,   586,
     587,   587,   588,   588,   588,   588,   589,   589,   589,   589,
     589,   590,   590,   591,   591,   592,   593,   593,   594,   594,
     595,   595,   596,   596,   597,   597,   598,   598,   599,   599,
     600,   600,   600,   600,   600,   600,   600,   600,   600,   600,
     600,   601,   601,   601,   601,   601,   601,   601,   602,   602,
     602,   602,   603,   603,   603,   603,   604,   604,   605,   605,
     606,   606,   606,   606,   607,   607,   608,   608,   609,   609,
     610,   611,   611,   611,   611,   612,   612,   612,   613,   613,
     613,   613,   613,   613,   613,   613,   614,   614,   614,   614,
     614,   615,   616,   616,   616,   616,   616,   616,   616,   616,
     616,   616,   616,   617,   617,   618,   618,   618,   619,   619,
     619,   620,   620,   621,   621,   622,   623,   624,   624,   625,
     625,   626,   627,   628,   628,   628,   628,   628,   628,   629,
     629,   630,   630,   631,   631,   631,   631,   632,   633,   633,
     633,   634,   634,   634,   634,   634,   634,   634,   634,   634,
     634,   634,   634,   634,   634,   635,   635,   635,   635,   635,
     635,   635,   635,   635,   636,   636,   636,   636,   637,   638,
     638,   638,   639,   639,   640,   640,   640,   640,   640,   640,
     640,   640,   640,   641,   641,   642,   642,   643,   643,   643,
     643,   643,   643,   643,   643,   643,   643,   643,   643,   643,
     643,   643,   643,   643,   643,   643,   643,   643,   643,   643,
     643,   643,   643,   643,   643,   643,   643,   643,   643,   643,
     643,   643,   643,   643,   643,   643,   643,   643,   643,   643,
     643,   643,   643,   643,   643,   643,   643,   643,   643,   643,
     643,   643,   643,   643,   644,   644,   644,   644,   644,   644,
     644,   644,   644,   644,   644,   644,   644,   644,   644,   644,
     644,   644,   644,   644,   644,   644,   644,   645,   645,   645,
     645,   645,   645,   645,   645,   645,   645,   645,   645,   645,
     645,   645,   645,   645,   645,   645,   645,   645,   645,   645,
     645,   645,   645,   645,   645,   645,   645,   645,   645,   645,
     645,   645,   645,   645,   645,   645,   646,   646,   646,   647,
     647,   648,   648,   649,   649,   650,   650,   651,   651,   652,
     652,   652,   652,   652,   652,   652,   652,   653,   653,   654,
     655,   655,   656,   656,   656,   656,   656,   656,   657,   658,
     659,   659,   659,   660,   660,   661,   661,   661,   662,   662,
     663,   664,   664,   665,   665,   666,   666,   667,   668,   668,
     668,   669,   669,   670,   670,   670,   671,   671,   672,   672,
     673,   673,   674,   674,   675,   675,   676,   676,   677,   677,
     678,   678,   679,   680,   681,   682,   683,   684,   685,   685,
     686,   686,   686,   686,   686,   686,   686,   686,   686,   686,
     686,   686,   687,   688,   689,   690,   690,   690,   691,   691,
     692,   692,   692,   693,   693,   693,   693,   693,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   694,   694,   694,   694,   694,   694,   694,   694,
     694,   694,   695,   695,   695,   695,   695,   695,   695,   695,
     695,   695,   695,   695,   695,   695,   695,   695,   695,   695,
     695,   695,   695,   695,   695,   695,   695,   695,   695,   695,
     695,   695,   695,   696,   696,   696,   696,   696,   696,   696,
     696,   696,   696,   696,   696,   696,   696,   696,   696,   696,
     696,   696,   696,   696,   697,   697,   697,   697,   697,   697,
     697,   697,   697,   697,   697,   697,   697,   697,   697,   697,
     697,   697,   697,   697,   697,   697,   697,   697,   697,   697,
     697,   697,   697,   697,   697,   697,   697,   697,   697,   697,
     697,   697,   697,   697,   697,   697,   697,   697,   697,   697,
     697,   697,   697,   697,   697,   697,   697,   697,   697,   697,
     697,   697,   697,   697,   697,   697,   697,   697,   697,   697,
     697,   697,   697,   698,   698
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     5,     1,     0,
       5,     5,     4,     3,     2,     0,     2,     3,     3,     2,
       1,     1,     1,     1,     3,     3,     3,     1,     5,     2,
       0,     2,     2,     6,     1,     1,     3,     6,     4,     1,
       0,     2,     0,     1,     1,     1,     2,     3,     3,     3,
       3,     3,     2,     5,     2,     3,     3,     1,     1,     1,
       3,     1,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     3,     6,     1,     1,     1,     1,     1,     0,
       1,     1,     2,     3,     4,     3,     2,     2,     3,     4,
       3,     2,     4,     1,     1,     1,     1,     1,     6,     7,
       9,     9,     9,     9,     7,     5,     7,     6,     6,     6,
       6,     3,     2,     1,     1,     0,     2,    10,     1,     1,
       1,     1,     1,     2,     0,     1,     1,     3,     3,     1,
       0,     2,     0,     3,     0,     1,     0,    10,    11,     1,
       1,     2,     2,     2,     2,     0,     1,     0,     1,     3,
       1,     1,     1,     3,     2,     0,     3,     1,     1,     2,
       1,     1,     2,     4,     2,     5,     1,     2,     2,     2,
       3,     2,     2,     0,     3,     1,     4,     4,     5,    11,
       3,     0,     1,     3,     1,     2,     2,     2,     0,     1,
       1,     2,     2,     0,     3,     3,     2,     1,     1,     2,
       2,     4,     0,     2,     2,     0,     3,     4,     4,     0,
       7,     3,     0,     1,     3,     1,     5,     4,     2,     0,
       2,     1,     2,     3,     2,     2,     2,     2,     3,     3,
       1,     0,     1,     1,     1,     2,     1,     2,     9,     1,
       0,     1,     1,     2,     0,     2,     0,     5,     1,     0,
      14,    19,     1,     1,     1,     3,     5,     1,     1,     1,
       3,     0,     1,     0,     1,     1,     1,     3,     0,     1,
       1,     1,     1,     1,     1,     2,     0,     1,     2,     1,
       2,     0,     2,     1,     2,     2,     6,     8,     4,     4,
       4,     4,     7,     3,     1,     3,     3,     1,     1,     1,
       1,     1,    12,     1,     3,     4,     7,     4,     2,     1,
       0,     1,     0,     7,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     1,     3,     6,     9,
       7,     9,     8,     8,     6,     8,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     2,     4,
       2,     0,     1,     1,     1,     1,     2,     2,     1,     1,
       1,     2,     2,     1,     2,     2,     1,     2,     1,     1,
       7,     8,     1,     1,     2,     1,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       2,     2,     2,     2,     2,     1,     3,     1,     2,     3,
       0,     3,     0,     1,     3,     2,    11,     1,     0,     2,
       0,     1,     3,     2,     5,     4,     1,     2,     0,     9,
       2,     0,     3,     2,     1,     3,     2,     1,     1,     1,
       1,     1,     1,     4,     1,     2,     2,     2,     1,     1,
       1,     4,     5,     1,     3,     3,     2,     2,     1,     3,
       2,     0,     5,     7,     1,     1,     7,     1,     3,     3,
       3,     1,     3,    11,    10,     2,     2,     0,     8,     4,
       4,     1,     1,     1,     0,     9,     6,     6,     7,     6,
       6,     9,     6,     8,     8,     6,     1,     0,     1,     0,
       0,    14,     1,     1,     3,     3,     1,     1,     1,     1,
       1,     1,     1,     0,     1,     1,     1,     1,     1,     0,
       6,     2,     2,     2,     2,     2,     2,     3,     2,     2,
       2,     1,     1,     0,     3,     1,     4,     4,     1,     0,
       2,     2,     7,     2,     5,     2,     0,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     0,     5,     4,
       3,     6,     4,     6,     6,     5,     7,     6,     1,     0,
      10,     4,     2,     1,     4,     5,     5,     2,     4,     1,
       1,     1,     0,     1,     0,     1,     0,     3,     0,     4,
       1,     1,     1,     1,     1,     1,     1,     0,     5,     3,
       0,     1,     3,     1,     1,     1,     1,     3,     9,     3,
       0,     2,     3,     4,     4,     2,     1,     7,     4,     1,
       3,     2,     4,     4,     3,     0,     2,     2,     2,     3,
       1,     3,     1,     2,     6,     7,     0,     3,     2,     2,
       2,     0,     2,     2,     1,     1,     3,     3,     1,     2,
       4,     4,     1,     1,     8,     4,     4,     4,     2,     0,
       3,     3,     4,     4,     4,     4,     2,     1,     1,     0,
       1,     1,     0,     1,     5,     1,     0,     1,     0,     3,
       1,     3,     3,     2,     2,     1,     4,     4,     2,     2,
       4,     1,     0,     1,     1,     1,     3,     0,     2,     0,
       3,     3,     1,     0,     2,     0,     2,     0,     1,     3,
       1,     2,     1,     2,     5,     6,     5,     1,     2,     1,
       4,     3,     4,     3,     5,     4,     5,     4,     5,     2,
       4,     1,     2,     2,     2,     1,     1,     0,     4,     2,
       1,     2,     2,     4,     3,     4,     2,     0,     1,     3,
       2,     2,     3,     5,     6,     3,     4,     0,     1,     1,
       1,     1,     1,     2,     5,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     2,     1,     3,     0,     5,     3,     0,     5,     3,
       0,     1,     1,     1,     1,     5,     2,     1,     1,     1,
       1,     5,     2,     2,     2,     1,     3,     3,     2,     1,
       0,     3,     0,     5,     2,     5,     2,     1,     3,     3,
       0,     1,     1,     1,     1,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     0,     3,     4,     4,     3,     3,
       3,     4,     3,     5,     4,     4,     3,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     1,     4,     1,     3,     5,
       2,     2,     2,     2,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     3,     3,
       2,     3,     5,     4,     6,     3,     5,     4,     6,     4,
       6,     5,     7,     2,     3,     2,     4,     3,     4,     3,
       4,     3,     4,     5,     6,     7,     5,     6,     3,     4,
       4,     6,     2,     1,     1,     3,     2,     2,     2,     2,
       2,     2,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     5,     6,     7,     1,     1,     3,
       5,     4,     1,     3,     4,     5,     5,     4,     1,     1,
       4,     1,     4,     1,     4,     1,     4,     1,     1,     1,
       6,     4,     4,     4,     4,     6,     5,     5,     5,     4,
       6,     4,     1,     2,     2,     2,     4,     6,     0,     1,
       3,     3,     0,     3,     1,     1,     3,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     4,     3,     2,
       3,     0,     3,     3,     2,     2,     1,     0,     2,     2,
       3,     2,     1,     1,     3,     5,     6,     4,     1,     2,
       4,     2,     0,     1,     0,     2,     2,     2,     2,     2,
       3,     1,     3,     3,     1,     1,     1,     3,     4,     4,
       1,     3,     1,     1,     1,     1,     1,     3,     1,     1,
       1,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     3,     6,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned short yydefact[] =
{
      76,   593,     0,   640,   639,   593,   167,     0,   633,     0,
     593,   200,   491,     0,     0,     0,   319,   593,     0,   657,
     421,     0,     0,     0,     0,   729,   421,     0,     0,     0,
       0,   472,   593,   736,     0,     0,     0,   729,     0,     0,
     644,     0,     0,     2,     4,    32,    11,    10,    46,    25,
       7,    40,    28,    73,    74,    72,    17,    13,     9,    14,
      18,    30,    19,    29,     8,    27,    42,    31,    45,    20,
      38,    36,    26,    41,    44,    68,    16,    50,    51,    64,
      52,    24,    61,    60,    62,    21,    39,    59,    63,    65,
      43,    57,    54,    69,    67,    75,    55,    33,     5,    47,
      23,     6,    22,    15,    71,    12,   642,    49,    58,    48,
      34,    53,    37,    56,    70,    35,    66,   713,   704,   738,
     712,   592,   591,   585,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   586,  1158,  1159,
    1160,  1161,  1162,  1163,  1164,  1165,  1166,  1167,  1168,  1169,
    1170,  1171,  1342,  1343,  1344,  1172,  1173,  1174,  1175,  1176,
    1345,  1346,  1177,  1178,  1179,  1180,  1181,  1347,  1182,  1183,
    1184,  1185,  1186,  1348,  1187,  1188,  1189,  1190,  1191,  1192,
    1193,  1194,  1349,  1350,  1195,  1196,  1197,  1198,  1199,  1200,
    1201,  1202,  1203,  1204,  1205,  1206,  1207,  1208,  1209,  1210,
    1211,  1351,  1212,  1213,  1352,  1214,  1215,  1353,  1216,  1217,
    1218,  1219,  1220,  1221,  1222,  1223,  1224,  1225,  1226,  1227,
    1228,  1229,  1230,  1231,  1232,  1233,  1234,  1354,  1355,  1356,
    1235,  1236,  1237,  1238,  1239,  1240,  1241,  1242,  1243,  1244,
    1245,  1246,  1247,  1248,  1249,  1250,  1251,  1252,  1253,  1254,
    1255,  1357,  1256,  1257,  1258,  1259,  1358,  1260,  1261,  1359,
    1360,  1262,  1263,  1264,  1265,  1266,  1361,  1267,  1268,  1269,
    1270,  1271,  1362,  1272,  1274,  1273,  1275,  1276,  1277,  1278,
    1279,  1363,  1280,  1281,  1282,  1283,  1284,  1285,  1286,  1287,
    1288,  1289,  1290,  1364,  1291,  1292,  1293,  1294,  1295,  1296,
    1297,  1298,  1299,  1300,  1365,  1301,  1302,  1303,  1366,  1304,
    1305,  1306,  1307,  1308,  1309,  1310,  1312,  1367,  1311,  1313,
    1314,  1315,  1368,  1369,  1316,  1317,  1370,  1318,  1371,  1319,
    1320,  1321,  1322,  1323,  1324,  1325,  1326,  1327,  1328,  1329,
    1330,  1331,  1372,  1332,  1333,  1334,  1335,  1336,  1337,  1338,
    1339,  1340,  1341,  1145,   186,  1122,  1146,  1147,  1464,  1463,
    1119,  1118,   632,     0,  1115,  1114,     0,   588,   199,     0,
       0,     0,     0,     0,     0,   379,     0,     0,     0,     0,
       0,     0,   110,   210,   209,     0,   309,     0,   477,     0,
       0,   319,     0,     0,     0,  1273,   671,   696,     0,     0,
       0,     0,   391,     0,   390,     0,     0,   388,     0,   318,
       0,   392,   386,   385,     0,   389,     0,   387,     0,     0,
     589,   670,   656,   642,  1159,   429,   433,   424,   430,   425,
     422,   423,  1282,  1142,     0,     0,   428,   418,   436,   443,
     456,   450,   454,   448,   452,   451,   447,   458,   457,   453,
     449,   455,     0,   442,   445,     0,   582,  1115,  1143,   603,
    1127,   728,     0,     0,   420,   581,   660,     0,   541,   542,
       0,   161,  1299,  1368,  1317,   157,     0,     0,   590,   735,
     733,     0,  1185,  1244,   149,  1299,  1368,  1317,   116,     0,
     156,  1299,  1368,  1317,   152,   599,     0,   584,   583,     0,
       0,   800,   643,   646,   713,     0,     1,    76,   641,   637,
     732,   732,     0,   732,     0,   709,  1373,  1374,  1375,  1376,
    1377,  1378,  1379,  1380,  1381,  1382,  1383,  1384,  1385,  1386,
    1387,  1388,  1389,  1390,  1391,  1392,  1393,  1145,  1129,     0,
       0,  1128,  1146,  1152,     0,   396,   395,     0,  1123,     0,
       0,     0,  1144,     0,     0,     0,   289,     0,   800,     0,
      79,     0,  1097,     0,     0,   406,     0,   407,   412,     0,
     409,     0,     0,   408,   410,   411,     0,   413,   414,     0,
     251,     0,     0,     0,     0,    79,   629,   214,   213,    79,
     212,   211,  1179,   912,   922,   920,   921,   914,   915,   916,
     917,   918,   919,     0,   531,   913,     0,   490,     0,   112,
       0,     0,    79,     0,     0,     0,     0,     0,     0,     0,
       0,   672,     0,   807,     0,   185,     0,   620,     0,   106,
    1179,     0,     0,     0,    83,    97,     0,   185,   393,     0,
     667,     0,   426,   435,   434,   432,   431,   427,   437,   438,
     439,     0,   444,     0,     0,     0,   685,  1116,     0,     0,
       0,   544,   544,   160,   158,     0,     0,     0,     0,     0,
    1342,  1343,  1344,  1094,     0,  1345,  1346,  1347,  1348,  1018,
    1019,  1021,  1027,  1349,  1350,  1203,  1351,  1352,  1140,  1353,
    1354,  1355,  1356,  1023,  1025,  1255,  1357,     0,  1141,  1359,
    1360,  1264,  1361,  1362,  1363,  1364,  1028,  1366,  1367,  1368,
    1369,  1370,  1371,  1139,     0,  1029,  1372,  1145,  1131,  1133,
    1134,   923,  1048,     0,     0,  1105,     0,     0,     0,  1042,
       0,   826,   827,   828,   853,   854,   829,   859,   860,   872,
     830,     0,   983,     0,     0,  1104,   927,  1012,  1007,  1048,
     719,  1101,  1048,     0,  1008,  1130,  1132,   831,  1146,   163,
       0,   164,  1120,  1299,   117,   148,   124,   147,     0,  1177,
     118,     0,     0,     0,   122,   595,     0,     0,   155,   153,
       0,   598,   587,   397,     0,   802,     0,   801,   645,   642,
     707,   706,     3,   648,   730,   731,     0,     0,     0,     0,
       0,     0,     0,   763,   752,     0,     0,     0,     0,   619,
       0,     0,     0,     0,   622,     0,     0,   104,   105,     0,
       0,     0,     0,     0,   287,   559,   559,   559,   559,     0,
       0,     0,     0,     0,     0,     0,    78,    85,    82,  1099,
    1098,  1125,   631,     0,     0,     0,     0,     0,     0,     0,
       0,   202,     0,   359,     0,   835,   870,   842,   870,   870,
     850,   850,   844,   832,   833,   877,   870,   847,   836,     0,
     834,   880,   880,   865,  1148,     0,   817,   818,   819,   820,
     851,   852,   821,   857,   858,   822,   894,   831,  1149,     0,
     606,   628,     0,   100,   380,   360,     0,     0,   108,   323,
     322,     0,     0,   361,    85,   289,   282,     0,     0,     0,
    1126,     0,     0,   251,   699,   701,   700,     0,   698,     0,
     682,     0,   183,   184,   358,     0,   185,     0,     0,     0,
       0,     0,   151,   185,   150,     0,   384,  1049,     0,   215,
     649,   655,   651,   653,   652,   654,   650,   417,  1192,  1218,
    1239,  1293,     0,     0,   459,   446,     0,     0,     0,   673,
     676,     0,     0,   683,   419,     0,   661,     0,   543,   540,
     539,   159,   471,     0,     0,     0,  1044,  1045,   869,   856,
    1093,     0,     0,   864,   863,     0,     0,     0,     0,     0,
     840,   839,   838,  1043,  1052,     0,   837,     0,     0,   870,
     870,   868,   950,     0,     0,   841,     0,     0,  1071,     0,
    1077,     0,     0,     0,   876,     0,   874,     0,     0,   982,
    1138,  1048,   930,   931,   932,   933,  1042,     0,  1049,     0,
    1135,     0,     0,   862,     0,   894,     0,     0,     0,     0,
       0,   925,     0,   946,     0,     0,     0,     0,     0,     0,
       0,   963,     0,     0,   965,     0,     0,     0,   923,   922,
     920,   921,   914,   915,   916,   917,   934,   935,     0,   947,
       0,  1096,     0,     0,   767,  1095,     0,   165,   166,   162,
       0,     0,   126,   125,     0,   145,   146,   141,   304,     0,
     121,   144,   302,   303,     0,   306,   140,     0,   600,   601,
       0,   128,   137,   139,   138,   136,   119,   127,   129,   131,
     132,   133,   120,   154,     0,   767,  1106,  1048,   634,     0,
     638,   713,   717,   712,   716,   739,   740,   745,   715,     0,
     765,   754,   748,   753,   749,   755,   762,   711,   751,   710,
     525,     0,   524,     0,     0,   618,     0,     0,     0,     0,
       0,   625,   245,     0,   182,     0,     0,     0,     0,   498,
     500,   499,   493,     0,   494,     0,   497,   502,   831,     0,
       0,     0,     0,     0,     0,     0,   291,   301,     0,     0,
       0,    79,    79,   288,   558,   175,     0,     0,     0,     0,
     557,     0,     0,     0,     0,     0,     0,    81,    80,  1100,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   252,
     254,     0,     0,  1394,  1395,  1396,  1397,  1398,  1399,  1400,
    1401,  1402,  1403,  1404,  1405,  1406,  1407,  1408,  1409,  1410,
    1411,  1412,  1413,  1414,  1415,  1416,  1417,  1418,  1419,  1420,
    1421,  1422,  1423,  1424,  1425,  1426,  1427,  1428,  1429,  1430,
    1431,  1432,  1433,  1434,  1435,  1436,  1437,  1438,  1439,  1440,
    1441,  1442,  1443,  1444,  1445,  1446,  1447,  1448,  1449,  1450,
    1451,  1452,  1453,  1454,  1455,  1456,  1457,  1458,  1459,  1460,
    1461,  1462,  1153,     0,   364,   367,  1154,  1155,  1156,  1157,
       0,   817,     0,     0,   811,   883,   884,   885,   882,   886,
     881,     0,   823,   825,     0,   604,   225,    98,     0,   532,
     112,   215,   111,   113,   114,   115,   328,   327,   329,     0,
     324,     0,    77,   286,     0,   217,     0,     0,     0,     0,
       0,   560,     0,     0,     0,     0,   697,   806,     0,     0,
     522,     0,     0,     0,   527,   185,   185,    96,   317,   394,
     669,     0,     0,   462,   461,   473,     0,   463,   464,   460,
       0,   675,     0,     0,   679,  1048,     0,   692,     0,   690,
       0,  1117,   659,     0,   658,   664,   666,   665,   663,     0,
       0,     0,     0,  1055,     0,     0,  1092,  1088,     0,     0,
    1049,     0,     0,     0,     0,  1062,  1063,  1064,  1061,  1065,
    1060,  1059,  1066,     0,     0,     0,     0,     0,   867,   866,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   984,     0,   906,     0,  1049,  1049,  1076,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1049,
    1082,     0,     0,  1009,   907,  1048,     0,     0,     0,     0,
    1136,   895,     0,     0,   900,     0,     0,     0,     0,   902,
     911,   909,   910,     0,   898,   899,     0,   948,  1103,     0,
       0,   955,     0,  1083,   978,     0,   969,     0,   964,     0,
     967,   971,   951,     0,     0,     0,     0,     0,     0,   949,
       0,   944,   942,   943,   936,   937,   938,   939,   940,   941,
     928,   945,     0,  1219,  1244,     0,   729,   729,   718,   727,
    1102,     0,   807,     0,     0,     0,  1013,     0,  1121,     0,
     305,   307,     0,   894,     0,   135,   594,     0,     0,   803,
       0,   807,     0,   636,   635,     0,     0,   743,   744,     0,
     761,     0,   760,     0,     0,     0,     0,   546,   547,     0,
       0,     0,     0,     0,   185,   623,   627,   181,   624,   492,
       0,   496,   825,     0,   549,   103,   550,     0,  1124,   552,
     290,   300,     0,   294,   295,   292,   296,   297,     0,     0,
     168,     0,     0,   185,   185,     0,   556,   177,   180,   178,
     179,     0,   555,    90,    92,     0,     0,    91,    93,     0,
       0,     0,     0,    84,     0,     0,     0,     0,   416,   404,
     415,     0,     0,   398,   250,     0,   201,   188,   189,     0,
     363,     0,     0,     0,     0,   812,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   617,   617,   617,   617,   605,
     621,     0,     0,    99,     0,   107,     0,     0,     0,     0,
       0,   808,     0,     0,     0,     0,   216,   218,   220,   221,
     222,     0,   283,   285,     0,     0,     0,   480,     0,     0,
       0,   702,   703,     0,   185,     0,   185,     0,   185,     0,
     580,   356,  1050,     0,     0,   475,     0,   470,   465,   467,
    1113,  1112,     0,  1110,     0,     0,   681,   693,   686,   688,
     687,     0,     0,   684,   662,     0,   734,  1057,  1058,     0,
       0,     0,     0,  1089,     0,     0,  1087,     0,  1041,  1020,
    1022,   849,     0,  1031,     0,   843,  1024,  1026,     0,   846,
       0,   924,     0,     0,  1032,   986,   987,   988,   989,     0,
    1002,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   990,   991,     0,  1003,  1033,   904,   905,     0,     0,
    1074,  1075,  1034,   878,   879,   880,   880,     0,     0,  1081,
       0,     0,     0,  1039,     0,  1011,  1048,   908,   871,   872,
       0,     0,   901,   896,     0,   897,     0,     0,     0,     0,
       0,   970,   966,     0,   968,   972,     0,     0,     0,   957,
     979,   953,     0,     0,   959,     0,   980,   729,   729,   729,
     729,   726,     0,     0,     0,   777,   766,   768,   779,   770,
     772,  1119,     0,   757,     0,     0,  1017,  1014,   123,     0,
     142,   134,   596,   597,   130,  1107,   694,     0,   647,   741,
     742,   764,   746,   750,   747,     0,     0,   244,     0,     0,
       0,   626,   495,     0,   548,     0,   293,   299,   298,   225,
       0,     0,   169,   176,   174,     0,     0,     0,     0,    86,
      89,     0,     0,     0,     0,   400,     0,     0,     0,   253,
     191,   192,   204,   190,   365,  1264,   915,   370,   366,   368,
     501,   369,   371,   351,     0,     0,     0,   815,     0,   888,
     889,   890,   891,   892,   893,   887,   894,     0,   616,     0,
       0,     0,     0,     0,     0,     0,   236,     0,     0,   230,
       0,     0,   231,   224,   227,   228,   102,   101,     0,     0,
     331,   325,   362,     0,   810,   217,   243,   272,     0,   281,
       0,     0,   280,   316,   312,   311,  1122,     0,     0,     0,
       0,     0,   602,   695,   523,   185,   383,   529,   526,   530,
     528,   282,   474,   468,     0,     0,   440,   674,     0,     0,
     678,   680,   691,   689,   185,  1056,   855,     0,  1091,  1085,
       0,     0,     0,  1051,     0,     0,  1069,  1068,  1070,     0,
       0,     0,  1000,   998,   999,   992,   993,   994,   995,   996,
     997,   985,  1001,  1079,  1078,  1072,  1073,   875,   873,     0,
    1036,  1037,  1038,  1080,  1046,     0,  1010,   861,   894,   903,
     926,   929,   976,   956,  1084,   973,     0,  1054,     0,   952,
       0,     0,     0,   961,   924,     0,     0,     0,     0,     0,
       0,   721,   720,   777,     0,   779,     0,   778,   791,     0,
       0,   797,   795,     0,   797,     0,   797,     0,     0,   771,
       0,   773,   791,     0,     0,   759,  1015,  1016,     0,  1109,
    1108,     0,   246,     0,     0,   247,   503,     0,   223,     0,
       0,     0,     0,   553,   554,    87,    94,    88,    95,     0,
     402,     0,   403,   405,   205,    79,     0,   353,     0,     0,
     357,   347,   349,     0,     0,     0,   813,   816,   824,   346,
     613,   612,   611,   608,   607,   615,   614,   610,   609,     0,
       0,   234,   238,   239,   237,   229,   232,   251,     0,     0,
     333,     0,     0,   809,     0,     0,     0,   240,     0,   275,
     219,   284,   285,   670,     0,   314,     0,   479,     0,     0,
       0,     0,   508,     0,     0,     0,   509,   513,   510,   521,
     504,   576,   577,   574,   575,     0,   538,     0,     0,     0,
     466,  1111,     0,   441,  1090,  1030,  1040,   848,  1086,   845,
    1067,     0,     0,     0,  1035,     0,  1137,     0,   974,     0,
     977,   958,   954,     0,   960,   981,   725,   724,   723,   722,
     781,   789,     0,   769,     0,   796,   792,     0,   793,     0,
       0,   794,   783,     0,     0,   789,     0,   804,     0,     0,
       0,   714,   894,   545,     0,   248,   551,   170,   171,     0,
     172,   173,   399,   401,   194,     0,   355,   354,   352,   348,
     350,   814,     0,   537,     0,   351,     0,     0,   226,   258,
       0,   217,   332,     0,     0,   326,   275,   242,   241,     0,
       0,     0,   279,   668,   315,     0,   308,     0,     0,     0,
     481,   488,     0,  1125,   506,   518,     0,     0,   507,     0,
     516,   517,     0,   505,   489,     0,     0,   469,     0,  1004,
       0,     0,  1047,   975,  1053,   962,   780,     0,     0,   782,
       0,     0,   785,   787,     0,     0,     0,     0,     0,  1122,
     805,   756,   758,   143,     0,   187,   203,   537,     0,   534,
     345,     0,   233,     0,   263,     0,   334,   335,   330,     0,
     279,     0,   273,   274,     0,   207,   313,   630,     0,   807,
       0,     0,   486,   483,     0,     0,     0,   514,   515,     0,
     520,   807,   677,     0,  1005,     0,   790,   799,     0,   786,
     784,   774,     0,   776,   251,   195,   629,   629,   196,   193,
     533,   536,   535,     0,   255,   256,   257,     0,   235,   259,
     260,     0,     0,   208,   271,     0,   488,   476,   482,   487,
       0,   519,   511,     0,     0,  1006,   788,     0,   775,   258,
       0,     0,     0,     0,     0,     0,   261,     0,   262,     0,
       0,     0,   372,   373,   338,     0,   276,     0,   485,   488,
     512,   579,   798,   263,   197,   198,     0,   268,     0,   267,
       0,   265,   264,     0,     0,   378,     0,   340,   342,   343,
     339,     0,   336,   341,   344,   277,   278,   484,   578,     0,
     351,     0,   266,   270,   269,     0,   382,   374,   320,     0,
     562,   573,   561,   563,   571,   568,   570,   569,   567,   249,
       0,   377,   381,     0,   375,   337,     0,   572,   566,   713,
     704,     0,     0,   564,   573,   338,   382,   565,     0,   376,
     321
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,    42,    43,    44,    45,   837,    46,    47,    48,  1198,
    1603,   634,    49,  1307,  1643,    50,   820,    51,    52,   608,
     898,  1312,    53,   488,  1106,  1107,  1108,  1526,  1109,  1090,
     766,  1110,    54,    55,    56,   760,  1079,    57,    58,   814,
     924,    59,    60,  1619,  1882,  2315,  2369,   369,  1212,  2095,
    2096,    61,   390,  1655,  1656,  1657,  1658,  1640,  1923,  1924,
    1925,  1659,  2137,  1660,  1152,   851,  1208,  1209,  2324,  2378,
    2379,  2380,  2431,  2139,  2262,  2335,    62,  1326,  1661,  1662,
      63,    64,   824,  1183,  1572,  1111,  1092,  1093,    65,   391,
    1943,  2266,  2145,    66,   418,    67,   901,  1319,  1320,  2131,
    2253,  2328,  2441,  2442,  2245,  2100,  2101,  2102,    68,    69,
      70,    71,   853,  1283,  1284,  1888,    72,  2412,  2413,   392,
    2474,    73,    74,   419,   637,  2342,    75,    76,   579,  1609,
      77,   435,   436,   651,    78,    79,   452,   453,   454,   953,
    1687,  1688,  1966,   477,  1354,  1355,    80,   393,  1949,  2269,
    2270,  2343,    81,   394,   816,  1163,  1164,  1165,  1889,  1166,
    2159,  2160,  2274,  2284,    82,    83,  1141,    84,  1343,   603,
      85,  2319,    86,    87,   470,   969,    88,  1585,  1186,    89,
    1669,  2462,  2476,  2477,  2478,  2165,  2449,    90,  2464,    92,
      93,    94,   123,   774,   782,   775,    95,    96,    97,  1305,
    1639,  1909,    98,    99,   100,   101,   892,   102,   103,   104,
     105,   106,   509,   503,   789,  1120,   107,   940,   423,   108,
     660,   965,  1374,   109,   640,   110,  2465,   959,  1363,  1364,
    2466,   113,   963,  1370,  2467,   115,   622,  1335,  2468,   729,
     118,   119,   120,  1074,  1508,   462,   796,   481,   514,   515,
    1125,  1126,   803,  1139,  1132,  1134,  2065,  2221,   804,  1137,
    1542,  1512,  1816,  1817,  1818,  2047,  2058,  2206,  2302,  1819,
    1820,   920,  1650,  1651,  1167,  1294,   876,   730,   731,   732,
     996,  1005,   990,   879,   733,   734,   735,   882,   736,   737,
     738,   739,   979,  1033,   740,   741,  1014,  1302,   742,   743,
    1027,  1466,  1041,   605,   744,  1070,   937,  1421,   746,  1020,
    1029,  1403,  2028,  1382,   977,  1404,  1414,  1733,  1423,  1429,
    1760,  1761,  1441,  1474,   747,  1386,  1387,  1714,   981,   748,
     749,   562,   750,  1691,  1115,  1116,  1692,  1693,   752,   954,
     501,  2298,   762,   547,  1567,  2271,   363,   459,   753,   754,
     755,   756,   635,   457,   757,   541,  1285,   356,   357,   543,
    1289,   365
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -2171
static const short yypact[] =
{
    5414,   306,  1595, -2171, -2171,   306, -2171, 26390, 23320,    -1,
     306,   203, 29137, 26697, 26390,   349,  4037,   306, 26390,   823,
   18602,   500,   422, 23320,   262,   340, 18602, 23320, 26390,    59,
   23627,   496,   306,   449, 27004, 23934,   429,   340, 17041, 22092,
     568,   -67,   715,   402, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171,   470, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171,    85, -2171,    94,
      92, -2171, -2171, -2171, 21171, 23320, 26390, 23320, 21171, 26390,
   26390,   660, 26390, 23320, 22092, 26390, 26390, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171,   516, -2171,   600,   611, -2171,  1052, -2171, -2171, 23320,
   21171, 26390,   552,   617, 26390, -2171, 23320,   515, 26390,   561,
   15745,   692, 26390, -2171, -2171, 26390, -2171, 23320, -2171, 26390,
     436,   717,   901,   844,   335, 26390, -2171, -2171, 22092, 21171,
   26390,   665, -2171, 26390, -2171, 21171, 26390, -2171, 16069, -2171,
   26390, -2171, -2171, -2171, 26390, -2171, 26390, -2171,   903, 23320,
   -2171,   679, -2171,   470,   353, -2171,   102,    75,   151,    77,
      79,    93,   353, -2171,   745,   526, -2171, -2171, -2171,   849,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171,   888,   774, -2171, 23320, -2171, -2171, -2171, -2171,
   -2171, -2171, 23320,   526, -2171, -2171,   795, 26390, -2171, -2171,
   23320, -2171,  1114,   848,  1013, -2171,   983,   500, -2171, -2171,
     972,  9021, 24241, 27311,    26, 24548,   874,   404, -2171,   -79,
   -2171,  1178,   911,  1076, -2171,   404, 23320, -2171, -2171, 14821,
     988,   917, -2171,  1133,   919,   944, -2171,  5414, -2171, 23320,
     546,   546,  1247,   546,    80,   495, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171,   959, -2171,   516,
     974, -2171,   976, -2171,  1088, -2171,   980,   671, -2171,   212,
     985,   112, -2171,  1097, 23320,  1104, -2171,   506,    48,  1136,
     619, 17348, -2171, 23320, 21171, -2171, 26390, -2171, -2171, 21171,
   -2171, 16393, 26390, -2171, -2171, -2171, 26390, -2171, -2171, 23320,
     999,  1000,  1290, 28539, 26390,  1031,  1318, -2171, -2171,  1031,
   -2171, -2171, 23320, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171,  1000, -2171, -2171,  1006, -2171,  1319,  1323,
     217,    64,  1031, 23320, 23320,  1191, 23320, 26390, 21171, 26390,
   23320, -2171,    89,  1047,  1021,    78, 28539, -2171,   985, -2171,
   23320,  1024,  1165,  1167,  1025, -2171, 20555,    51, -2171, 10989,
   -2171,   426, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, 26390, -2171, 22399,  1138,    21,   -22, -2171, 26390, 28539,
    1351,  1262,  1262, -2171, -2171,  1220,  1268,  1183,  1048,   596,
    1069,   363,  1071, 10989,  1055,    49,    49,  1056,  1058, -2171,
    1059,  1060, -2171,   370,   370,  1182,  1062,  1063, -2171,   396,
    1084,  1087,   421,  1066,  1070,   949,    49, 10989, -2171,  1081,
     480,  1086,  1092,  1106,  1102,  1108, -2171,  1107,  1110,   465,
     468,  1111,  1115, -2171,  1062, -2171,    43,   536, -2171, -2171,
   -2171, -2171,   516, 10989, 10989, -2171, 10989, 10989,  9349, -2171,
     262, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,    50,
   -2171,   550, -2171,  1554, 10989,  3207, -2171, -2171, -2171,  1117,
      -6, -2171,   516,  1118, -2171, -2171, -2171, -2171,   559, -2171,
     129,  1082, -2171,   863, -2171, -2171, -2171, -2171, 20246,  1416,
   -2171,   545,  1281,    35, -2171,  1299, 17962, 17962, -2171, -2171,
    1282, -2171, -2171, -2171, 23320, -2171, 26390, -2171, -2171,   470,
   -2171, -2171, -2171,  1119, -2171, -2171,   -67,   -67, 10989,   -67,
     279,  9677, 10989,  1345,   431, 17655,  1184,  1185, 27311, -2171,
      76,   511,  1187,   100, -2171,  8052,  1234, -2171, -2171,  1189,
    1171,  1192,  1173,  1198,  1112,   110,  1420,   678,  1420,  1170,
    1278,  1204,  1207, 22092,  1208, 27311, -2171, -2171, -2171, -2171,
     516, -2171, -2171,  1149,  1288,   985,  1151,   438,  1291,  1339,
   26390,  1186, 20864, -2171,  1157, -2171,  1194, -2171,  1194,  1194,
    1168,  1168,  1175, -2171, -2171, -2171,  1194,  1176, -2171, 28846,
   -2171,   359,   570, -2171, -2171,  1480,  1487, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171,   132,   516, -2171,  1499,
   -2171, -2171, 28539, -2171,  1434, -2171, 16393, 26390,   159, -2171,
   -2171,    25,  1179, -2171, -2171, -2171,   -29, 20555,  1404,  1320,
   -2171,   985,  1496,   999, -2171,   814, -2171,  1284, -2171, 10989,
   -2171, 17655, -2171, -2171, -2171,  1497,    78,  1229, 27618, 23320,
   23320, 26390, -2171,    78, -2171, 23320, -2171, 19341,   576,   408,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, 26390, 21171,
   26390, 26390, 23320,  1248,  1193, -2171,  1236,  1197, 15129, -2171,
   -2171,   152, 23320, -2171, -2171,   624, -2171,   247, -2171, -2171,
   -2171, -2171, -2171, 22399, 10989, 10005, -2171, -2171, -2171,  1200,
   19341,  1221, 10989, -2171, -2171, 10989, 10989,   745,   745,   745,
   -2171, -2171, -2171, -2171,   737,   745, -2171,   745,   745,  1194,
    1194, -2171,  3764, 10989,   745, -2171, 16393, 10989, 11317,  7085,
   10989,  1266,  1267,   745, -2171,   745, -2171, 10989,  8365, -2171,
    1210, -2171,  1209,  1209,    61,    63,   528,  1212,  5633,  1205,
   -2171,  1295,   745, -2171,   745,   386,  1062,   412,  1419,  1215,
     -57, -2171,    99,   809, 10989, 20864,  1279, 11317, 10989,  1217,
     807, -2171, 10989,   474, -2171,  1218, 10989,  1283,    81, 10989,
   10989, 10989, 10989, 10989, 10989, 10989, 12616, 12944, 28539, 11645,
      95,  1210, 22706,  9021,  1439,  1210,  6756, -2171, -2171, -2171,
   26390,  1416, -2171, -2171,  1285, -2171, -2171, -2171, -2171,   288,
   -2171, -2171, -2171, -2171,   584, -2171, -2171,   399, -2171, -2171,
    1398, -2171, -2171, -2171, -2171, -2171, -2171,  1219, -2171, -2171,
   -2171, -2171, -2171, -2171,  1224,    31, -2171, -2171, 21785, 26390,
   -2171, -2171,  1417, -2171, -2171,  1223, -2171,  4877,  1417,  1367,
    1374, -2171,   -50, 19341,  1407, 19341, -2171, -2171, -2171, -2171,
   -2171,  1232, -2171, 26390, 26390, -2171,  1237, 26390,  1421,  1425,
    1246, -2171, -2171, 26390, -2171,  1382, 26390, 10989,  1399, -2171,
     181,   196, -2171,   704, -2171, 28539, -2171, -2171,   516,  1315,
   26390, 26390, 26390, 26390, 26390,   635, -2171,  1555,   635,   635,
     461,  1031,  1031, -2171, -2171, -2171, 26390, 26390, 26390, 26390,
   26390,  1396, 26390,  1327, 26390,  1369, 26390, -2171,  1103, -2171,
   17655, 23320,  1447, 27618,    -5, 23320, 23320,    -5,   710, -2171,
   -2171,  1403,    27, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171,   716, -2171,  1289, -2171, -2171, -2171, -2171,
   10989,  1578, 28539,  1286,  1297,  1326,  1338,  1341, -2171, -2171,
    1344,   745, -2171, -2171,    25,   392, -2171,   -41,  1490, -2171,
   -2171,   456, -2171, -2171, -2171, -2171, -2171, -2171, -2171,  1424,
    1422, 26390,  1103,  1112, 23320, 21478,  1606,  1509,   262, 23320,
    1401, -2171,  1610,  1511,  1512,  1526, -2171, 19341,  1302, 28539,
   -2171, 26390,  1304,  1310,  1313,    78,    78, -2171, -2171, -2171,
   -2171, 10989,  1387,  1082,  1316, -2171,   985,  1082,  1082,  1193,
   24855, -2171,  8693,   724, -2171, -2171,    41, -2171,    73,   304,
    1485, -2171, -2171, 28539, -2171, -2171, -2171, -2171, -2171,  1543,
     727,   440,   492, -2171,   745, 10989,   140, -2171,  5869,   760,
    5948,   765,  1325,  1328,   768, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171,  1329,  1546,  1335,  1336,  1337, -2171, -2171,
    1992,   771,  1340,  6198,  1343, 11317, 11317, 11317, 11317,  9349,
   11317,  1256, -2171,  1346, -2171,  1347,  6314,  5398,  1334,  1350,
    1365,  1377,  1354,  1355,  6348, 10333, 10989, 10333, 10333, 18857,
    1334,  1357, 10989,  1210, -2171,   516, 10989, 26390,  1361,  1364,
   -2171, -2171,  1547,  1492, -2171,  1062, 16393,  1108, 10989, -2171,
   -2171, -2171, -2171,  9349, -2171, -2171,  1062,  3764, -2171,  1393,
     694,  1321,  9349, -2171, -2171,  1591, -2171,   738, -2171,  1373,
   -2171, -2171,  2390, 11317, 10989,  1217, 10989,  1435, 16393,  2461,
   10989,  4945,  3357,  3357,   145,   145,    61,    61,    61,    63,
   -2171,   809,  1376,   701,   775, 23320,    87,   723, -2171, -2171,
   -2171, 14201,  1047, 10989, 10989,  1378, -2171,   776, -2171,   404,
   -2171, -2171,   745,   386,  1655, -2171,  1493,   399, 18282, -2171,
   26390,  1047,   126, -2171, -2171,   783, 10989, -2171, -2171,  2253,
   -2171, 26390, -2171, 10989, 10989,  9677,  1489, -2171, -2171, 10989,
     153,  1384,  1385, 26390,    78, -2171, -2171, 19341, -2171, -2171,
   27925, -2171,  1391, 26390, -2171,  1025, -2171,  1495, -2171, -2171,
   -2171, -2171,   635, -2171, -2171, -2171, -2171, -2171,   635,   635,
   -2171, 28539,    60,    78,    78,  1450, -2171, -2171, -2171, -2171,
   -2171,  1452, -2171, -2171, -2171,  1518,  1607, -2171, -2171,   262,
     745,  1521,  1441, -2171,  1397,  1588,    -5,  1402, -2171, -2171,
   -2171,  1589,  1593, -2171, -2171, 26390, -2171, -2171, -2171,   388,
   -2171, 20864, 16717, 18877,  1411,  1297,  1409,   745,   423,    56,
      17,  1504,  1579,  1415,  1557,  1433,  1433,  1433,  1433, -2171,
    1085,   745, 26390, -2171,  1472,   159,  1491,  1454, 23320,    25,
     788, -2171, 28539,  1426, 23320,  1429,  1427, -2171, -2171, -2171,
   -2171,   791, -2171, 28539,    33, 23320,  1486,  1468, 28539,  1569,
     -67, -2171, -2171,   -67,    78,  1432,    78, 28539,    78, 28232,
   -2171, -2171, 19341, 23320, 21171, -2171, 26390,   185, -2171, -2171,
   -2171, -2171,   793, -2171,   210, 26390,  1210, -2171, -2171, -2171,
   -2171,  1668,  1670, -2171, -2171, 24855, -2171, -2171, -2171,  1440,
    1436, 18901, 10989, -2171,  1677, 28539, -2171, 23320, -2171, -2171,
   -2171, -2171,   745, -2171, 10989, -2171, -2171, -2171, 10989, -2171,
     745, -2171, 10989,  1659, -2171,  1443,  1443,   549,  1443,  5633,
    1105, 11317,    90,  1086, 11317, 11317, 11317, 11317, 11317, 11317,
   11317, 13259, 13575, 28539, 12288, -2171, -2171, -2171, 10989, 10989,
    1665,  1659, -2171, -2171, -2171,   960,   960, 28539,  1442,  1334,
    1445,  1446, 10989, -2171,  2804,  1210, -2171,  4529, -2171,  1732,
     262,   -57, -2171, -2171,  1456, -2171, 11973, 11317, 10989,   811,
   10989, -2171, -2171,  1448, -2171, -2171, 28539, 10989,   935,  2669,
   -2171,  4502, 10989,  1457,  5978,  9349, -2171,   340,   340,   340,
     340, -2171, 23320, 23320, 13889, 25162,  1455,    37, -2171, 25162,
   25469,  1117,  1451,  1667,   813,   816, -2171, -2171, -2171,  1461,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, 10661, -2171, -2171,
   -2171,  1082, -2171, -2171, -2171,  1516, 18918, -2171, 26390, 26390,
     819, -2171, -2171,  1514, -2171,  1525, -2171, -2171, -2171, -2171,
     130,   502, -2171, -2171, -2171, 26390, 26390,   262, 26390, -2171,
   -2171,   262,   262,  1652,    -5, -2171,  1654,    -5,    -5, -2171,
   -2171, -2171,    72, -2171, -2171,  1215,   288, -2171, -2171, -2171,
   -2171, -2171, -2171,   407,   745,   969,  1470, -2171,  1471, -2171,
   -2171, -2171, -2171, -2171, -2171, -2171,   386, 23320, -2171,    42,
      96, 25776, 26083,  1473, 26390, 11317, -2171,   131,   371, -2171,
    1657, 23320, -2171, -2171, -2171, -2171, -2171,  1025, 28539, 23320,
    1695,  1611, -2171, 26390, -2171, 21478,   120,  1673, 21478, -2171,
   26390, 26390, -2171,  1520, -2171, -2171,   980,   262, 26390,  1476,
     459,   409, -2171, -2171, -2171,    78, -2171, -2171, -2171, -2171,
   -2171,  1479, -2171, -2171,  1698, 24855, -2171, -2171,  8693,  1488,
   -2171, -2171, -2171, -2171,    62, -2171, -2171, 10989, 19341, -2171,
    1498,  1501,  1502, 19341, 18949,  1505, 19341,  1665,  2205,  1699,
    1630,  1507,  2205,  2546,  2546,   637,   637,   549,   549,   549,
    1443, -2171,  1105, 19341, 19341, -2171, -2171, -2171, -2171,  1508,
   -2171, -2171, -2171,  1334, -2171, 10989,  1210, -2171,   386, -2171,
   -2171, -2171,  1083,  1277, -2171,   675, 28539, -2171,   822,  1277,
   11317, 10989, 10989, 19916,   105, 10989, 19133, 23320, 23320, 23320,
   23320, -2171, -2171,  7743,    37,  1515, 26390, -2171,  1510, 14201,
    1674,  1623, -2171, 14201,  1623,   578,  1623, 14201,  1681, -2171,
   15437, -2171,  1522,  7414,  1796,  1720, -2171, -2171,   262, -2171,
   19341, 26390, -2171,   825,   840, -2171, -2171, 26390,  1085,  1663,
    1666,   510, 26390, -2171, -2171, -2171,  1025, -2171, -2171,    -5,
   -2171,    -5, -2171, -2171, -2171,  1031,  1770, -2171,   136,  1777,
   -2171,  1724,   156,  1527,  1742,  1745, -2171, -2171, -2171,  1748,
   -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, 10989,
      82,  2205, -2171, -2171, -2171, -2171, -2171,   999,  1571,    47,
    1775,  1768,    25, -2171,  1535,  1792,  1795, -2171,  1544,   993,
   -2171, -2171, -2171,   679, 23320,  1736,  1776, -2171, 14513,   262,
    1694,  1650, -2171, 20555,  1707,   114, -2171, -2171, -2171,   411,
   -2171, -2171, -2171, -2171, -2171,  1625, -2171, 26390,  1881,  1706,
   -2171, -2171,  8693, -2171, 19341, -2171, -2171, -2171, -2171, -2171,
   -2171, 11317,  1572, 28539, -2171, 19190, -2171,   841, -2171, 28539,
    1083,  1277,  1277, 10989,  1277, -2171, -2171, -2171, -2171, -2171,
   25162,  1573, 26390,    37, 14201, -2171, -2171,   676, -2171, 14201,
    1758, -2171, -2171, 14201, 26390,  1575, 26390, -2171,   842, 10989,
   10989, -2171,   386, -2171,  1686, -2171, -2171, -2171, -2171,   745,
   -2171, -2171, -2171, -2171, -2171,   262, -2171, -2171, -2171, -2171,
   -2171, -2171, 21171,  1892, 23320,   407, 19231,  1727, -2171,  1749,
   26390, 21478, -2171,   462,  1697, -2171,   993, -2171, -2171, 23320,
    1725,  1726,  1722, -2171, -2171,   262, -2171, 23320, 10989,   860,
   -2171, 23013,  1587,   980, -2171,  1583,  1738,   441, -2171,  1728,
   -2171, -2171,  1000, -2171, -2171, 23320,  1826, -2171,   868,  1169,
   28539,   875, -2171, -2171, -2171,  1277, -2171, 26390,   876, -2171,
   10989,  1594, -2171, -2171, 14201,   676,   878, 26390,   881, 28539,
   -2171,  1334, 19341, -2171, 23320,   556, -2171,  1892,   158, -2171,
   -2171,  1820, -2171,    34,  1740,  1913, -2171, -2171, -2171, 21171,
    1722,   889, -2171, -2171,  1882, -2171, -2171, -2171, 19280,  1047,
   14513, 23320, -2171, -2171, 10989,   262,  1802, -2171, -2171,  1754,
   -2171,  1047, -2171,   892, -2171,   898, -2171, 19341, 26390, -2171,
   -2171, -2171,   899, -2171,   999, -2171,  1318,  1318, -2171, -2171,
   -2171, -2171, -2171,  1851, -2171, -2171, -2171,   160, -2171,  1750,
    1753,    28,  1616, -2171, -2171,    98, 23013, -2171, -2171, -2171,
     907, -2171, -2171,  1807,  1857, -2171, -2171,   910, -2171,  1749,
     262,   262,  1715,   415,   415,  1872, -2171,  1669, -2171,   745,
     745, 28539,  1617, -2171, 19671,  1721, -2171,  1723, -2171, 23013,
   -2171,  1817, -2171,  1740, -2171, -2171,  1860, -2171,  1952, -2171,
      97, -2171, -2171, 21171, 16393, -2171,    28, -2171, -2171, -2171,
   -2171,   933, -2171, -2171, -2171, -2171, -2171, -2171, -2171,    29,
     407,  1743, -2171, -2171, -2171,   985,   -71, -2171, -2171, 19671,
   -2171,    32, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   21171, -2171, -2171, 27618, -2171, -2171,   410, -2171, -2171,   648,
     944,  1633,  1629, -2171,    32, 19671,  1747, -2171,   939, -2171,
   -2171
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
   -2171, -2171, -2171,  1460, -2171,  -564, -2171, -2171, -2171,  1065,
   -2171, -1107, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
     654, -2171, -2171,  -405,  1195, -2171,   442,   444, -2171, -2171,
   -2171,  -628, -2171,  -317, -2171, -2171, -2171, -2171, -2171,   391,
    -603, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171,  -852,  -867,    39, -2171,    40,   789,   117, -2171,  -140,
   -2171, -2171, -2171,   -48,   432,  -896, -1417,   368,  -415,  -437,
    -393,  -391,  -414, -2171,  -267,  -339, -2171,    44, -2171,    52,
   -2171, -2171,  1089, -2171, -2171,  -714, -2171,   -85, -2171, -2171,
    -147, -2171, -2171, -2171,  1609, -2171, -2171,   697, -1580, -2171,
   -2171, -2171,  -487,  -457, -2171, -2138,   -99,   -97, -2171, -2171,
   -2171, -2171,  -583, -2171,   387, -2171, -2171, -2171,  -429,  1116,
    -477, -2171, -2171, -2171, -2171,  -112, -2171, -2171, -2171, -1172,
   -2171,  1985,   605,  1551,  -843, -2171,  1538, -2171,  1362,  1044,
     314,    55, -2171, -2171, -2171, -1618, -2171, -2171, -2171, -2171,
    -319, -2170, -2171,   711,  -621, -2171,   463, -2171,   356, -1097,
   -2171,  -134, -2171, -2171, -2171, -2171,  -845, -2171, -1191,  -406,
   -2171,  -291, -2171, -2171, -2171,  1368, -2171, -2171,   457, -2171,
   -2171, -2171, -2171,  -420,  -452, -2171, -2171, -2171,    67, -2171,
   -2171, -2171,   869,  -466, -2171,   507,  -840, -2171, -2171, -2171,
   -2171,  -314, -2171, -2171, -2171, -2171, -1281, -2171, -2171, -2171,
     916,  2016,  -350, -2171, -2171, -2171, -2171, -2171, -2171, -2171,
   -2171, -2171, -2171,  1395,  -106, -2171,    19, -2171, -2171,   343,
      36, -2171, -2171, -2171,    38,  1400, -2171, -2171,    23,     0,
     -40,   174,   226, -2171, -2171,    16,   566, -2171, -2171, -2171,
   -2171,   504,  1240, -2171,   501,  -242, -2171, -2171,  1242, -2171,
   -2171,   932, -2171, -1715,   234, -1749,    -3,  -982,  -255,    10,
   -2171, -1468, -1959,   121,   316,   764,  1196, -2171,  1517,  2805,
   -2171, -2171,  -522, -2171, -2171,  2833,  2845, -2171, -2171,  3025,
    3215, -2171,  -542,   281,  3260,  1934,  -678, -1008, -2171,  -981,
    1049,   991,  -375, -2171,  4430,  -725,  4318,  -811,  1019,  -712,
    -585, -2171, -1938, -2171,  -915, -2171, -2171, -2171, -2171, -2171,
   -1095, -1659,  -369,   579, -2171, -2171,   677, -2171, -2171, -2171,
     414,  -660, -2171,  -416, -2171,   540,  -101,   104,  1166,  -451,
    1228,  -476,  2262,  -329, -1302,  1513,  1458, -2171,   -72, -2171,
     321,    18,   -86,    -4,  3597, -2171,  1032,  2227,  -805,  -801,
   -2171, -2171
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1316
static const short yytable[] =
{
     117,   505,   631,   355,   364,   604,   761,   926,   933,   355,
     355,   656,  1607,   544,   355,   549,   355,  1332,  1042,   111,
     895,   890,   355,   116,   355,   893,   475,  1450,   903,   781,
     489,   494,  1016,   604,   936,  1613,   112,  1071,   114,  1676,
    1075,   504,   460,   551,  1823,   585,  1313,  1287,   904,   500,
     560,  1288,   540,   496,   938,  1314,   550,  1091,  1315,  1459,
    1383,  1465,  1021,  1836,  1565,   751,  1962,    91,  1561,  1931,
    2059,  2061,  1352,   641,   627, -1215,  1338, -1240,   764, -1256,
     770,   902,  1046,  -865,  1046,  -705,   922, -1313,  2187,  -870,
    1031,  -912,  -708, -1275,   830,   956,  -912,   922,  2050,  2044,
     765,  2005, -1169,   961,  1316,  1460,   831,  2321,    15,  1460,
    1461,    15,   643,   922,  1461,  -926,  2110,   914,  1146,   817,
    -926,   546,   548,   546,  1913,   552,   355,   467,   355,  1147,
    1941,   355,   552,   983,   984,  1072,  1850,  1697,  1617,  1543,
    1083,  2409,  1511,  -705,   557, -1313,  2374,  1860,  2472,  2051,
    -708, -1217,  1146,  -206,  1001,   915,  1915,  1366,  1324,  1184,
    1317,   645,   991,  1147,    22,   846,  1046,    22,  2052,  1699,
    2113,  2453,    33,  1989,  1157,  1457,  1899,  2415,  2371,  1608,
    1199, -1313,  1902,  2053,  1148,  2416,   800,   468,   510,  2054,
     776,   366,  2280, -1313,  1016,  1146,   604, -1230, -1313,   818,
    -705,  1295, -1313, -1313,  1154, -1313,  1077,  -708,  2122,  2055,
    2460,    27, -1266,  2236,    27,  2135,  2418,  1311,  1148,   810,
    1641,  1900,  2410,   916,  1202,   899, -1313,  1303,  1098,  1712,
     809,   368,  2097,   511,  1324,   801,  1470,  2375,   777,  2403,
    1287,   900, -1313,   838,  1288,  2291,  2136,  1642,  1367,  2447,
     832,  1078,  1296,  2123,  1903,  2306,  2281,  2308,  2237,  2473,
      33,  1148,  2247,    41,   586,   917,  1919,  2056,    33,   802,
    1990,    33,    33,  1458,    21,   611, -1313,  1991,   923,  1327,
    1158,  2454,  2372,  2376, -1313,  1544,  2411,  1698,  1149,   923,
    1330,   512,   589,  1901,  1920, -1254,  1618,  1297,   581,   811,
    1298,  1325,  1921,   612,  1861,   923,   458,   638,  2417,  1443,
    2079,  1318,  2469,   962,   957,    39,   983,   984,    39,  1700,
     629,   469,  1149,  1340,  1001,   918,    15,   624,  2180,  1073,
    1348,  1457,  -912,   628,  2203,   819,  2099,   458,  2207,   991,
    1099,   438,  2212, -1254,   978,  2057,  1462,   438,  2362,   461,
    1462,   958,  2353,   458,  -865,  1604,  -926,   433,  1150,  2461,
    -870,  2094,    41,    41,  1922,  1149,  1530,   355, -1313,  1299,
     548,   787,   546,  -865,   552,   513,   606,  2251,   609,  -870,
    1032,   355,    22,   546,  1368,   552,   935,  1067, -1313,  1380,
    1381,   355,  1150,  1068,   852,  1068,   355,  1965,  1369,   548,
    1389,  1391,   552,  1145,   606,  1532,   355,   458,   623, -1215,
     355, -1240,   552, -1256,   812,   546,  -705,   433, -1313,  -705,
   -1313, -1313,   360,  -708,   434,  1428,  -708, -1275,  2149,  1463,
    1197,  2073,  2074,  1440,  1875,  1150, -1169,   360,  1300,  1118,
    1385,   360,   822,  1837,  1646,  2150,  2404,  2124,   618,    33,
    2427,  2296,   360,   360,  1442,  1295,   813,  1408,  1409,   479,
     398,  1570,  1301,   355,  1573,  1574,   433,   849,  1064,  1065,
    1066,  1067,  1353,   434,  1357,  1358,  2149,  1068,   355,   489,
     894,   489,  1635,  2097,   939, -1217,    33,  1964,  2161,  2299,
    1309,  1517,   843,  2150,  2303,  1452,  1129,   845,  2305,    14,
    1483,  1359,   767,  1969,   908,    15,  1296,   117,  1562, -1230,
     439,  2151, -1230,   825, -1230,  1830, -1230,   826,   927,  2347,
    1965,   604,   377,    18, -1266,  1890,   111, -1266,  1575, -1266,
     116, -1266,   480,    39,  2152,  1927,  1701,  2098,   538,   545,
      41,   545,   538,   112,  2162,   114,   911,   360,   360,   772,
     546,  1297,  2255,  1636,  1298,  2125,   794,   841,   440,  2151,
    2153,    22,   355,   455,  1153,  1130,   379,   606,   355,   619,
     377,  1890,   355,   458,    91,   546,  1157,    41,   121,   441,
     355,  1204,  2152,  2348,  2365,  1154,   801,  2099,   546,  2359,
    1702,  2428,  1453,   827,  1637,  1484,  1454,   442,  1520,  1485,
    1412,  -737,   461,   433,  1735,  1736,  1737,  1738,  2153,  1740,
     122,   476,   546,   910,   379,   355,  1524,  1578,  1579,  1085,
     802,   773,  -713,  1299,  2317,  1576,   546,  1577,  1486,   795,
    1205,   604,   934,   620,  -713,   443,  2366,   649,  1987,  2154,
    1525,   117,  2429,  1535,  1880,  1881,  2147,   355,  2163,  2155,
    -737,   650,   381,  1696,   355,   117,  1638,  1510,   978,  2430,
     942,  1011,  1012,  2156,   946,    33,  2006,  -713,   433,   976,
    2157,   383,  1798,   384,  -870,   434,   613,   943,   960,   944,
     502,  -850,  2080,  -713,  -737,   865,   993,  2154,   505,  1013,
    2051,  1155,  1300,  -870,  2326,  2164,  1046,  2155,   614,   458,
     989,   495,  2090,  1086,   554,  2092,  2093,  -844,  1787,  2052,
    2158,  2156,    39,  2282,  1019,   506,  2327,  -713,  2157,   383,
     444,   384,  1487, -1315,  2209,  -713,   995,  1184,  1026,   828,
    2054,  1188,  -877,  1775,   445,  1685,   507,  2050,   433,   446,
    2367,  2483,  1680,  1681,  2484,   438,  2368,   438,  1030,   438,
     829,  -877,  1897,   438,  -491,   648,    41,  2081,  2158,  1035,
    2082,  2086,  1151,   447,   934,   448,   508,  1011,  1012,  1707,
    1011,  1012,   934,   934,   449,  1351,  -880,  1185,   587,  -880,
     588, -1315,  1117,   360,   538,  1776,   450,   451,  2051,  1096,
     545,  -847,   563,  1313,  1975,  1013,  1121,  1121,  1015,  1121,
    2019,   545,  1314, -1126,   489,  1315,  1395,  2052,  2056,  -713,
    1004,  1310,   360,   538,  1840,  1548,  1287, -1315, -1316,   538,
    1288,  1708,  2053,  1349,   590,   433,   591,  1709,  2054, -1315,
    1046,   489,  2229,   545, -1315,     3,     4,  1742, -1315, -1315,
    1791, -1315,   834,  1195,    30,  1347,  1210, -1148,  2055,   561,
    1440,  1769,  1440,  1440,  1087,  1088,   458,  1396,  1856,   790,
     433,   458, -1315,   835,  1857,  1858, -1150,  1089,  2300,   360,
   -1149,  2387,  1011,  1012,   137,  1752,   360,  1356, -1315,   367,
    1034,  1753,   583,  2394,   360,   768,   420,  1789,  1743, -1151,
    1475,   584,   606,   552,   807,   458,    30,  1891,  2108,   875,
    1015,   478,  1397,   934,  1081,  1398,  2056,  1350,  1887,  1476,
     360,  1351, -1315,   360,  1522,   808,   607,  2232,   505,  2233,
   -1315,   836,  1792,   360,   975,  1793,    41,   552,  1824,  1825,
    1988,   546,   409,  1992,  1993,  1994,  1995,  1996,  1997,  1998,
    1999,  2000,   925,  2002,   355,  1088,   355,   355,  2325,  2030,
     433,  1851,  1050,  1051,  1365,  1372,   616,  1089,   504,  1373,
    1749,  1750,  1751,  1752,  1807,  2301,  1808,   117,   545,  1753,
    1122,  1124,   617,  1128,  1399,   966,  2022,   360,   538,   790,
    1863,  1864,  -705,   538,  2057,   461,  1375,  1477,   999,  1000,
    1378,  1478,  1054,   545,  1479,   626,  1062,  1063,  1064,  1065,
    1066,  1067,   606,  1376, -1315,  1377,   545,  1068,   721,   639,
    2186,  1744,  1745,  1746,  1794,  1747,  1748,  1749,  1750,  1751,
    1752,  1795,  1123,  1123, -1315,  1123,  1753,   360,   360,   642,
     545,   644,   538,   646,   360,  1559,  1451,   647,  1809,  1560,
    1810,  1614,  1464,  1400,   545,  1615,  1401,  1620,  1402,  1473,
    1784,  1621,   636,  1828, -1315,  1694, -1315, -1315,  1706,  1695,
     433,   564,  1351,   652,  2016,  1841,  1768,   360,  1770,  1771,
    1556,  1954,  2208,  1956,  2211,  1958,   355,   797,  1742,   799,
     653,   604,  1803,  1480,  1564,  2400,  2401,  2007,  2008,  1605,
    1481,  1716,  1095,  1611,  1612,  1351,  1718,  1095,  1095,  1721,
    1351,   565,  1729,  1722,  2121,   566,  1730,  1827,  1590,   654,
    1592,  1351,  1523,   604,  1838,   355,  1333,  1334,  1080,  1932,
     567,  1142,  1939,  1933,  1967,   659,  1940,  1913,  1968,  1743,
    1062,  1063,  1064,  1065,  1066,  1067,   663,   568,  1914,   355,
     548,  1068,  2024,   355,  2066,  1175,  1351,  2067,  1351,   355,
    2075,  1351,   552,  2188,  1615,   664,  2224,  2189,   665,  1915,
    1615,  1916,  1593,  1594,   668,   569,   552,   552,   355,  1568,
     355,  2225,  2293,  2310,   361,  1615,  2189,  1351,   666,  1176,
     570,   771,  1581,  1582,   355,  1584,   355,  2013,   355,   361,
     552,  2339,   552,   361,  1595,  2340,   440,   546,   360,  2352,
     778,   546,   546,  1968,   361,   361,  2354,  2356,  1306,  2361,
    2189,  1080,  2363,  1933,  2313,  1917,  1933,   441,   779,  2190,
    2384,   780,  1610,  2395,   962,  1610,  1742,  2189,  1596,  2396,
    2398,  2249,   786,  1080,  1933,   442,   362,  1142,  2419,  1177,
     787,  2422,  1351,   788,  1344,  1080,   571,   360,  1742,   721,
     790,   456,  1744,  1745,  1746,   465,  1747,  1748,  1749,  1750,
    1751,  1752,  1011,  1012,  2458,  1918,   498,  1753,  2459,  1919,
    2490,  2104,  2105,   443,  2459,   791,  1178,  1743,  1179,   798,
    1597,  1598,  2482,  1187,  1189,  1190,   572,   573,  1180, -1150,
     539,   539,   574,   539,   539,  2260,  2261,  1920,  1046,   361,
     361,  1842,  1843,  1047,   805,  1921, -1151,  1599,  1392,  1393,
    1394,   806, -1316, -1115,   575,   815,  1405,  1652,  1406,  1407,
     821,  1663,  1910,  1911,  1912,  1411,   576,   823,   833,   850,
     852,   577,   854,   836,  1432,   891,  1433,  1568,  1181,   896,
     907,   897,  1046,   360,   360,  -109,  1666,  1047,   919,   545,
     578,   921,  2166,  1448,   928,  1449,  1689,   929,   444,   930,
     931,   556,   558,   538,  1600,  1182,   360,  1922,   967,   968,
    2289,  2173,   445,   971,   972,   973,   360,   446,   974,   505,
    -835,  1741,  -842,  1601,  1500,   982,   985,   360,   986,   987,
     988,   992,    41,   994,  1602,  -832,   997,   721,  -833,  1742,
     998,   447,  1049,   448,  1747,  1748,  1749,  1750,  1751,  1752,
    1521,  1003,   449,  -836,  1788,  1753,  1006,  1080,  -834,  1026,
    1050,  1051,  1007,   505,   450,   451,  1747,  1748,  1749,  1750,
    1751,  1752,   505,  1084,  1097,  1113,  1008,  1753,  1009,  1822,
    1010,  1017, -1316,  1778,  1100,  1018,  1049, -1129,  1076,  1119,
    1743,   800,   606,  1143,  1144,  1783,  1156,  1169,  1170,  1171,
    1054,  1172,  1173,  1026,  1050,  1051,  1785,  1174,  2399,  1184,
    1192,  1055,  1026,  1191,  1193, -1316,  1194,  1196,  2218,  1200,
    1201,  1203,  1207,  1206,   606,  1473,   360,  1290,  1211,   978,
    1747,  1748,  1749,  1750,  1751,  1752,  1095,  1292,   989,  1095,
    1095,  1753,  1806,  1293,  1054,   995,  1004,  1304,   375,  1321,
    1328,  1815,  1329,  1331,  1339,  1055,  1142,  1360,  1341,  1344,
    1336,  1385,  1812,  1813,   934,  2278,  1117,  1362,   962,  1361,
    1384,  2234,   360,  1430,  1431,   361,   539,   355,  1442,  1447,
    1446,  1068,   539,  1444,  1455,  1456,  1469,  1472,  1488,  1210,
    1511,  1527,  1490,   539,  1528,  1529,   511,  1519,  1536,   355,
    1540,  1541,  1545,  1546,   361,   539,  1555,  1549,  1551, -1316,
     721,   539,  1552,  1744,  1745,  1746,  1553,  1747,  1748,  1749,
    1750,  1751,  1752,  1558,  1563,   539,  1587,  1571,  1753,  1589,
    1606,  1058,  1591,  1616,  1624,  1629,  1644,   580,  1062,  1063,
    1064,  1065,  1066,  1067,   124,  1981,  1622,  1630,  1626,  1068,
    1631,  1210,  1356,  1632,  1627,   545,  1648,  1869,  1649,   545,
     545,   361,  1633,  1664,  1610,  1628,  1665,  1670,   361,  1668,
    1671,  1672,  1673,  1674,  2311,  1058,   361,  1883,   552,  1677,
    1892,  1678,  1062,  1063,  1064,  1065,  1066,  1067,  1679,  1683,
     125,  1684,  1703,  1068,  1705,  1675,  1719,  1724,  1781,  1720,
    1723,  1946,   361,   126,   117,   361,  1725,  1726,  1727,  1351,
     117,  1731,  1763,   117,  1734,   361,  1782,  1755,  1756,  1036,
     127,  1762,  1963,   655,  1764,  1765,  1766,  1942,  1773,  1704,
     657,  1365,  1779,  1952,   117,  1780,  1953,  1037,   662,  2350,
    1786,  1689,  1790,  1796,  1802,  1710,  1805,  1831,   128,  1826,
     773,   129,  1845,   546,  1848,  1849,  1853,  1970,  1855,  1865,
     539,  1866,  1867,  1868,   783,  1871,  1872,   785,  1873,   361,
     539,  1874,  1877,  1876,  1038,   539,  1878,   793,   360,  1894,
    1895,  1904,  1822,   360,   130,   539,  1906,  1905,  1039,  1907,
    1908,  1928,   620,  1929,  1040,  1947,  1935,  1948,   539,  2390,
    1937,  1951,  1938,  1955,  1972,   505,  1973,  1976,   975,  1979,
    1759,  1758,  1031,  2010,   505,  1753,  2011,  2012,  2026,   361,
     361,  2063,   539,  2064,   539,  2071,   361,  2020,  2034,   131,
    2049,   842,  2068,  2076,  2077,  2089,   539,  2091,  2018,  2106,
    2107,  2130,  2138,  2119,  2126,  1026,  2148,  2132,  2331,  2167,
    2181,  2048,  2144,  2169,  2043,  2048,  2062,  2182,  2172,   361,
    2204,  2355,  2205,  2037,  2038,  2039,  2040,  2213,  2219,  2175,
     132,  2355,  2176,  2177,  2471,   133,  2179,  2183,  2220,  2184,
    2202,   905,   906,  1829,  1210,  1210,  2200,  2227,   913,  1095,
    2228,  2235,  2216,  2238,  2098,  2242,  2241,   134,  2243,  2244,
    2250,   355,   355,  2252,   552,  2254,  2256,  2257,   593,   135,
    2258,   594,   595,   596,  2259,   597,   598,   599,   600,   601,
     602,   657,  2397,   136,  2265,  2085,  2276,  2267,  2277,  2087,
    2088,  2279,  1610,  1095,  2285,  1610,  1610,  1859,  2286,  1095,
    1095,  2287,  2290,  2297,  2304,  2307,  2314,   355,   355,  2318,
     355,  2125,  2323,  2329,  2334,  2332,  2333,  2344,  2345,   360,
    2349,  1870,  2346,  1941,  2358,  1821,  2373,  2112,  2114,  1652,
    2381,  1581,  2377,  2385,  1581,  2392,  2142,   355,  2393,  2402,
    2420,  2421,  2405,  1095,  1568,  2407,  2414,  2426,  1896,  1898,
     361,  2403,  2436,  2448,  2445,  2404,  2446,  2451,  2452,  2470,
    2486,  1689,  1926,  2485,  1645,  2146,  2472,   792,  1934,  1322,
    1834,  1833,  1112,  1862,  2134,  1580,  2078,  1822,  2140,  1859,
    2248,  1822,  1847,  1879,  2423,  1822,  2450,  2408,  2406,  2330,
    2432,  2383,  2141,  1957,  1323,  1960,  2230,  2264,  2488,   361,
     615,  1634,  2475,  2240,  2239,  2168,  1044,  2457,  1884,  2489,
    1308,   463,  1114,  1046,   658,   667,   955,  1379,  1047,  1974,
    2170,  2388,  1647,  1852,  1950,  2283,  2370,  1422,  2456,  2463,
     970,  1980,  2487,  1832,  1533,   422,   941,  2263,  1971,  2048,
    1839,   945,  2201,  1982,  1138,  1136,  1844,  1531,  2045,  1815,
    2360,  1985,  2210,  1815,  2133,  1625,  2215,  1815,  1425,   604,
    2017,  1502,   360,  1713,  1800,  1291,  1422,   355,   360,  2001,
    1835,  2288,  2171,   355,   840,   909,  2272,  1468,  2231,  1944,
       0,     0,     0,  2009,     0,     0,  2222,     0,     0,     0,
       0,     0,     0,     0,     0,   361,   361,   360,   538,     0,
     877,   539,     0,     0,     0,     0,     0,  1610,     0,  1610,
       0,     0,  2027,  1048,     0,   539,     0,  1049,   361,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   361,     0,
       0,   545,  1822,     0,     0,  1050,  1051,  1822,     0,   361,
    1946,  1822,     0,   877,  2273,     0,  1052,     0,     0,   934,
       0,     0,     0,     0,     0,  2337,     0,  1345,  1346,     0,
       0,     0,     0,  2142,     0,     0,     0,  2275,     0,     0,
    1356,     0,  1053,     0,     0,  1054,   877,     0,     0,     0,
     657,     0,     0,     0,     0,     0,  1055,     0,  1056,     0,
    1371,     0,     0,     0,     0,     0,  2048,     0,   355,     0,
       0,   657,     0,     0,  1815,     0,     0,  1521,     0,  1815,
    1652,     0,  2309,  1815,     0,  2103,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   360,   360,  1821,  2389,
    2111,     0,  1822,     0,     0,     0,     0,     0,   361,     0,
    1057,     0,     0,     0,  2128,     0,  1568,  1581,     0,     0,
       0,     0,     0,  2316,     0,     0,     0,  2382,     0,     0,
       0,     0,     0,   546,     0,     0,     0,   546,  2272,   354,
       0,     0,     0,     0,     0,   396,   397,     0,     0,     0,
     421,     0,   437,  2336,   361,     0,     0,     0,   464,     0,
     466,     0,     0,   355,     0,     0,     0,     0,     0,     0,
    1509,     0,     0,  2309,  1815,     0,  1058,     0,     0,  1059,
    1060,  1061,     0,  1062,  1063,  1064,  1065,  1066,  1067,     0,
       0,   360,   877,     0,  1068,     0,     0,  1728,     0,     0,
       0,     0,   877,     0,     0,   360,  2273,   546,     0,     0,
       0,     0,  2027,   360,     0,     0,  1534,     0,  1742,     0,
       0,   542,     0,     0,   355,   542,     0,     0,     0,     0,
       0,  2455,     0,  2391,     0,     0,     0,   539,     0,     0,
       0,   539,   539,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   546,     0,     0,     0,   877,     0,     0,     0,
       0,     0,   553,     0,   555,     0,     0,   559,  2481,  1743,
       0,     0,  1095,     0,     0,     0,     0,     0,     0,   877,
    2444,  1046,     0,     0,     0,   546,  1047,     0,  2424,  2425,
       0,  2480,     0,     0,     0,     0,     0,     0,     0,     0,
     606,     0,  2443,     0,  1422,  1422,  1422,  1422,   877,  1422,
       0,     0,     0,     0,     0,   877,     0,  1039,     0,   117,
       0,   360,   360,   360,   360,  2444,     0,     0,     0,     0,
       0,  2479,     0,  1821,     0,     0,     0,  1821,     0,     0,
       0,  1821,     0,     0,     0,  1044,     0,  2443,     0,     0,
       0,  2444,  1046,  1797,   117,     0,     0,  1047,     0,     0,
     361,     0,     0,     0,     0,   361,     0,     0,     0,  2027,
       0,     0,  1422,  2443,     0,  2294,     0,     0,     0,     0,
       0, -1316,     0,     0,     0,  1049,     0,   886,     0,   721,
       0,     0,  1744,  1745,  1746,     0,  1747,  1748,  1749,  1750,
    1751,  1752,     0,  1050,  1051,     0,     0,  1753,     0,     0,
       0,     0,     0,     0, -1316,     0,     0,     0,     0,     0,
    1521,     0,  1653,     0,     0,     0,     0,  1667,  1944,     0,
     886,     0,   538,     0,     0,     0,     0,   593,     0,     0,
     594,   595,   596,  1054,   597,   598,   599,   600,   601,   602,
       0,     0,  1048,     0,  1055,   877,  1049,     0,     0,     0,
       0,     0,     0,   886,     0,     0,     0,   542,     0,     0,
       0,     0,     0,     0,  1050,  1051,  2027,     0,     0,     0,
       0,     0,     0,     0,     0,  1052,     0,     0,  1821,     0,
       0,     0,     0,  1821,     0,  1934,   542,  1821,     0,     0,
       0,     0,   542,   582,     0,     0,     0,     0, -1316,     0,
       0,  1053,     0,     0,  1054,     0,     0,   610,     0,     0,
       0,     0,     0,     0,     0,  1055,   538,   621,   360,     0,
       0,     0,   625,     0,     0,     0,     0,     0,     0,     0,
       0,   361,   632,   360,     0,     0,   633,   361,     0,     0,
       0,   545,   877,     0,     0,   545,     0,     0,     0,  1742,
    1046,     0,     0,     0,     0,  1047,     0,     0,     0,   360,
       0,     0,     0,     0,  1058,  1094,     0,     0,   758,  1057,
       0,  1062,  1063,  1064,  1065,  1066,  1067,   877,  1821,     0,
     877,     0,  1068,     0,     0,     0,     0,  2435,   360,   661,
    2433,  2434,     0,  1811,     0,     0,     0,     0,     0,   886,
    1743,     0,     0,   538,     0,     0,     0,     0,     0,   886,
       0,     0,     0,     0,   538,   545,     0,     0,     0,     0,
    1422,     0,  2031,  1422,  1422,  1422,  1422,  1422,  1422,  1422,
    1422,  1422,     0,  1422,     0,  1058,     0,     0,  1059,  1060,
    1061,     0,  1062,  1063,  1064,  1065,  1066,  1067,     0,  1344,
   -1316,   542,     0,  1068,  1049,     0,   542,     0,     0,     0,
     545,     0,     0,   886,     0,  2021,  1422,     0,     0,   877,
     888,     0,  1050,  1051,   361,     0,     0,     0,  1044,     0,
     361,     0,     0, -1316,     0,  1046,   886,     0,   844,     0,
    1047,   539,     0,   545,   847,     0,     0,     0,   848,     0,
       0,     0,     0,     0,     0,   542,   889,   538,     0,   361,
     539,     0,  1054,   888,     0,   886,   877,     0,     0,     0,
     721,     0,   886,  1055, -1316, -1316,   758,  1747,  1748,  1749,
    1750,  1751,  1752,     0,     0,     0,  1930,     0,  1753,     0,
       0,   912,  1936,   539,   538,     0,   888,     0,     0,     0,
     877,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     758,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1961,     0,   947,     0,     0,     0, -1316,     0,     0,
     964,     0,     0,     0,   758,  1048,     0,     0,     0,  1049,
       0,     0,     0,     0,  1422,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1050,  1051,     0,
     758,   758,     0,   758,   758,   758,     0,     0,  1052,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   758,     0,     0,     0,     0,     0,     0,   361,   361,
     361,     0,     0,  1058,  1053,     0,     0,  1054,     0,     0,
    1062,  1063,  1064,  1065,  1066,  1067,     0,     0,  1055,     0,
    1056,  1068,   886,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   758,     0,     0,   758,   758,
       0,     0,   888,     0,     0,     0,     0,     0,     0,     0,
    2041,  2042,   888,     0,     0,     0,     0,     0,     0,  1422,
       0,     0,  1057,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   361,     0,     0,     0,   877,     0,  1286,
       0,     0,     0,     0,     0,     0,     0,   361,     0,     0,
       0,     0,     0,     0,     0,   361,   888,     0,   877,   886,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1058,   888,
       0,  1059,  1060,  1061,     0,  1062,  1063,  1064,  1065,  1066,
    1067,     0,     0,  2014,   886,  2109,  1068,   886,     0,   877,
    2015,     0,     0,     0,     0,     0,   758,     0,   888,  2127,
       0,     0,     0,     0,     0,   888,     0,  2129,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   877,
       0,     0,     0,     0,     0,     0,   542,     0,     0,     0,
     877,     0,     0,     0,     0,   877,     0,     0,     0,     0,
       0,     0,     0,     0,   877,     0,   877,     0,     0,     0,
    1422,   758,   758,   361,   361,   361,   361,     0,     0,   758,
       0,     0,   758,   758,     0,   361,     0,     0,     0,   361,
       0,  1044,     0,   361,  1045,     0,   886,     0,  1046,     0,
     758,     0,   877,  1047,   758,   758,   758,   758,     0,     0,
       0,     0,     0,     0,   758,   758,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  2196,  2197,  2198,  2199,     0,
     877,   758,  1286,   886,   758,   758,     0,     0,     0,   758,
       0,     0,     0,   758,   877,     0,   758,   758,   758,   758,
     758,   758,   758,   758,   758,   888,   758,     0,     0,     0,
     758,     0,     0,   758,     0,     0,     0,   886,     0,     0,
     539,     0,     0,   877,   539,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1048,     0,
       0,     0,  1049,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1518,     0,     0,     0,     0,     0,     0,     0,
    1050,  1051,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1052,     0,     0,     0,     0,     0,     0,     0,     0,
     361,     0,     0,     0,     0,   361,     0,     0,  1046,   361,
       0,     0,     0,  1047,   758,     0,     0,  1053,   878,     0,
    1054,     0,   888,     0,     0,     0,     0,     0,     0,     0,
       0,  1055,     0,  1056,     0,  1547,     0,     0,   539,  1550,
     361,     0,     0,     0,     0,  1554,   880,     0,     0,     0,
       0,     0,     0,     0,     0,   361,     0,   888,   881,     0,
     888,   878,     0,   539,  1566,     0,  1569,   539,     0,     0,
       0,     0,     0,     0,     0,   877,     0,     0,     0,     0,
    1583,   361,  1586,     0,  1588,  1057,     0,     0,     0,   880,
       0,     0,     0,     0,   878,     0,     0,     0,     0,     0,
     361,   881,  2320,     0,     0,     0,     0,     0,  1048,     0,
     361,     0,  1049,     0,     0,     0,     0,   657,     0,     0,
       0,     0,   880,     0,   886,   539,     0,     0,     0,     0,
    1050,  1051,     0,     0,   881,     0,   539,   539,     0,     0,
       0,  1052,     0,  2351,     0,   886,     0,   758,     0,   888,
       0,  1058,     0,     0,  1059,  1060,  1061,     0,  1062,  1063,
    1064,  1065,  1066,  1067,     0,     0,     0,     0,     0,  1068,
    1054,     0,  2364,   877,     0,     0,     0,     0,     0,     0,
       0,  1055,   539,     0,     0,     0,   886,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   888,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   758,     0,
       0,     0,     0,     0,     0,   539,   886,     0,     0,   758,
       0,     0,     0,     0,     0,     0,     0,   886,     0,   539,
     888,     0,   886,     0,     0,  1057,     0,     0,   883,     0,
     878,   886,   758,   886,     0,     0,     0,     0,     0,     0,
     878,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   539,     0,   880,     0,
       0,     0,   758,   758,   758,   758,   758,   758,   880,   886,
     881,   883,     0,     0,     0,     0,     0,     0,     0,     0,
     881,     0,   758,   758,   758,   758,     0,     0,     0,   758,
       0,  1058,     0,   758,   878, -1316, -1316,     0,  1062,  1063,
    1064,  1065,  1066,  1067,   883,   758,     0,   886,     0,  1068,
     758,     0,     0,     0,     0,     0,     0,   878,     0,   758,
     877,   886,   880,     0,     0,     0,   877,     0,     0,     0,
     758,   758,     0,   758,   881,     0,     0,   758,     0,     0,
       0,     0,     0,     0,     0,   880,   878,     0,     0,     0,
     886,     0,     0,   878,     0,     0,     0,   881,   542,     0,
     758,   758,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   880,     0,     0,     0,     0,     0,
       0,   880,     0,   758,     0,     0,   881,     0,     0,     0,
     758,   758,   758,   881,     0,     0,   758,     0,     0,     0,
       0,     0,     0,     0,     0,  1046,     0,   888,     0,     0,
    1047,     0,     0,     0,     0,     0,     0,     0,   884,     0,
       0,     0,     0,     0,     0,     0,     0,   877,   888,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1854,   877,     0,     0,     0,
     883,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     883,   884,     0,   885,     0,     0,     0,     0,  1286,   888,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   886,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   878,   884,     0,     0,     0,     0,   888,
       0,     0,     0,     0,     0,  1048,   885,     0,     0,  1049,
     888,     0,     0,     0,   883,   888,     0,     0,     0,     0,
       0,   880,     0,     0,   888,     0,   888,  1050,  1051,     0,
       0,   542,     0,   881,     0,     0,     0,   883,  1052,   885,
       0,     0,     0,     0,     0,     0,     0,  1945,   877,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   758,
       0,     0,   888,     0,  1053,     0,   883,  1054,     0,     0,
       0,   758,     0,   883,     0,   758,     0,     0,  1055,   758,
     886,     0,     0,     0,     0,     0,     0,     0,   758,     0,
     878,   758,   758,   758,   758,   758,   758,   758,   758,   758,
     888,   758,     0,     0,     0,   758,   758,     0,     0,     0,
     877,     0,     0,     0,   888,     0,     0,     0,   880,   758,
       0,     0,     0,     0,     0,   878,     0,     0,   878,     0,
     881,     0,  1057,   758,   758,   758,     0,   758,     0,     0,
     884,     0,     0,   888,   758,     0,     0,     0,     0,   758,
     884,     0,   758,   880,     0,     0,   880,     0,     0,     0,
       0,   542,     0,     0,     0,   881,   399,     0,   881,     0,
       0,     0,     0,     0,     0,     0,   400,     0,     0,     0,
       0,     0,     0,     0,   758,   885,     0,     0,     0,     0,
       0,     0,     0,     0,   401,   885,     0,     0,  1058,     0,
       0,  1059,  1060,  1061,   884,  1062,  1063,  1064,  1065,  1066,
    1067,     0,   402,   883,     0,     0,  1068,   878,     0,     0,
       0,     0,     0,     0,     0,   403,     0,   884,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   886,     0,     0,
       0,     0,   404,   886,     0,   880,     0,  2083,  2084,   885,
       0,     0,     0,     0,     0,     0,   884,   881,     0,     0,
       0,     0,   758,   884,   878,     0,     0,     0,     0,     0,
     405,     0,   885,   406,     0,   888,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   407,     0,     0,     0,     0,
       0,     0,   880,  2116,  2118,     0,  2120,     0,   878,     0,
     887,   885,     0,     0,   881,     0,     0,     0,   885,     0,
     883,     0,     0,     0,     0,   758,     0,     0,     0,     0,
       0,     0,     0,  2143,   758,     0,   880,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   881,     0,
       0,     0,     0,   887,   886,   883,     0,     0,   883,     0,
       0,   408,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   758,   886,     0,     0,     0,     0,     0,     0,
       0,     0,   409,   888,     0,     0,   887,   758,   758,   758,
       0,     0,   758,     0,     0,     0,     0,     0,     0,     0,
       0,   410,   411,     0,     0,     0,   542,   412,     0,     0,
     542,     0,     0,   884,   542,     0,     0,     0,     0,     0,
     758,     0,     0,     0,     0,     0,     0,     0,     0,   413,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   414,     0,     0,     0,     0,   415,   883,     0,     0,
       0,     0,     0,     0,     0,   416,     0,     0,   885,     0,
       0,     0,     0,  2223,     0,   417,     0,     0,     0,  2226,
       0,     0,     0,     0,     0,   886,   758,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   883,   878,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   542,     0,     0,     0,     0,
     884,     0,     0,     0,     0,     0,   878,     0,     0,     0,
       0,     0,     0,   880,     0,     0,     0,     0,   883,   758,
       0,     0,   887,     0,     0,   881,  1945,   886,   758,     0,
     888,     0,  1168,     0,   880,   884,   888,     0,   884,     0,
     758,     0,     0,     0,     0,   885,   881,   878,     0,     0,
       0,   542,     0,     0,     0,     0,   542,     0,     0,     0,
     542,     0,     0,     0,     0,     0,   758,   758,     0,     0,
       0,     0,     0,     0,     0,   880,     0,   878,     0,     0,
     885,     0,     0,   885,     0,     0,   887,   881,   878,   542,
       0,     0,     0,   878,     0,     0,     0,     0,     0,     0,
       0,     0,   878,     0,   878,   880,     0,     0,     0,   887,
       0,     0,     0,     0,     0,   758,   880,   881,     0,     0,
       0,   880,     0,     0,     0,     0,     0,   884,   881,     0,
     880,     0,   880,   881,     0,     0,     0,   888,   887,     0,
     878,     0,   881,  1046,   881,   887,     0,   758,  1047,     0,
       0,   542,     0,     0,     0,     0,   888,     0,     0,     0,
       0,     0,     0,  1044,     0,     0,     0,     0,   880,     0,
    1046,     0,   885,     0,   884,  1047,   542,     0,   878,     0,
     881,     0,     0,     0,     0,     0,     0,   542,     0,     0,
       0,   758,   878,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   883,   880,     0,   884,     0,
       0,     0,     0,     0,     0,  2032,     0,     0,   881,   885,
     880,   878,     0,     0,     0,     0,   883,     0,     0,     0,
       0,     0,   881,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0, -1316,     0,     0,     0,  1049,     0,   880,
       0,     0,     0,   885,     0,     0,     0,     0,   888,     0,
       0,   881,     0,     0,     0,  1050,  1051,   883,     0,     0,
    1048,     0,     0,     0,  1049,     0, -1316,     0,     0,     0,
     542,     0,     0,     0,     0,   887,     0,     0,     0,     0,
       0,     0,  1050,  1051,     0,     0,     0,   883,     0,     0,
       0,     0,     0,  1052,     0,  1054,     0,     0,   883,     0,
       0,     0,     0,   883,     0,     0,  1055,   542,     0,     0,
     888,     0,   883,     0,   883,     0,     0,     0,     0,  1053,
       0,     0,  1054,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1055,     0,  1056,     0,     0,     0,     0,
       0,     0,     0,   878,     0,     0,     0,     0,     0,     0,
     883,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   -1316,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   880,  1168,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   881,     0,   884,     0,  1057,   883,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   883,     0,     0,     0,   884,   887,     0,   745,
     887,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1058,     0,     0,     0,
     885,   883,     0,  1062,  1063,  1064,  1065,  1066,  1067,     0,
       0,   878,     0,     0,  1068,     0,     0,   884,     0,     0,
       0,   885,     0,  1058,     0,     0,  1059,  1060,  1061,     0,
    1062,  1063,  1064,  1065,  1066,  1067,     0,     0,     0,   880,
       0,  1068,     0,     0, -1050,     0,     0,   884,     0,     0,
       0,   881,     0,     0,     0,     0,     0,     0,   884,     0,
       0,     0,   885,   884,     0,     0,     0,     0,     0,   887,
       0,  1044,   884,     0,   884,  1537,     0,     0,  1046,     0,
       0,     0,     0,  1047,     0,     0,     0,     0,     0,     0,
       0,     0,   885,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   885,     0,     0,     0,     0,   885,     0,
     884,     0,     0,     0,     0,     0,   887,   885,     0,   885,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   883,     0,     0,     0,     0,     0,  1538,
       0,     0,     0,     0,     0,     0,  1046,     0,   884,     0,
     887,  1047,     0,     0,     0,   885,     0,     0,     0,     0,
       0,     0,   884,     0,     0,     0,     0,     0,   878,     0,
       0,   980,     0,     0,   878,     0,     0,     0,  1048,     0,
       0,     0,  1049,     0,     0,     0,     0,     0,     0,     0,
       0,   884,     0,   885,     0,  1002,   880,     0,     0,     0,
    1050,  1051,   880,     0,     0,     0,     0,   885,   881,     0,
       0,  1052,     0,     0,   881,     0,     0,     0,     0,     0,
       0,  1022,  1023,     0,  1024,  1025,  1028,     0,     0,     0,
       0,   883,     0,     0,     0,     0,   885,  1053,     0,     0,
    1054,     0,  1043,     0,     0,     0,  1048,     0,     0,     0,
    1049,  1055,     0,  1056,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1050,  1051,
       0,     0,     0,     0,     0,   878,     0,     0,     0,  1052,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   878,     0,  1127,     0,     0,  1133,
    1135,     0,     0,   880,     0,  1057,     0,     0,  1054,     0,
       0,     0,     0,     0,     0,   881,     0,     0,     0,  1055,
       0,     0,   880,   884,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   881,     0,     0,  1168,     0,     0,
       0,     0,     0,     0,     0,     0,  1539,     0,     0,     0,
       0,     0,     0,     0,     0,  1069,     0,     0,   887,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   885,     0,
       0,  1058,     0,  1057,  1059,  1060,  1061,     0,  1062,  1063,
    1064,  1065,  1066,  1067,     0,     0,     0,     0,   883,  1068,
       0,     0,     0,     0,   883,     0,   878,     0,     0,  1168,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1337,     0,     0,
       0,   884,     0,     0,   880,     0,     0,     0,     0,   887,
       0,     0,     0,     0,     0,     0,   881,     0,     0,  1058,
     887,     0,  1059,  1060,  1061,  1168,  1062,  1063,  1064,  1065,
    1066,  1067,     0,     0,   887,     0,   887,  1068,   878,     0,
       0,     0,     0,     0,     0,     0,   885,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1388,     0,     0,     0,  1390,     0,   880,     0,     0,     0,
       0,     0,   887,     0,     0,   883,     0,     0,   881,     0,
       0,  1410,     0,     0,     0,  1413,     0,  1426,  1427,     0,
       0,     0,     0,     0,   883,  1434,  1439,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     887,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1467,     0,   887,     0,  1471,  1069,     0,     0,
    1482,     0,     0,     0,  1489,     0,     0,  1491,  1492,  1493,
    1494,  1495,  1496,  1497,  1498,  1499,     0,  1501,     0,     0,
       0,   745,     0,   887,     0,     0,     0,     0,   884,     0,
       0,     0,     0,     0,   884,     0,     0,     0,     0,     0,
    1069,     0,  1044,     0,     0,     0,     0,     1,     0,  1046,
       0,     0,     0,     0,  1047,     2,     3,     4,     0,     0,
       0,     0,  1069,     0,     0,     0,   883,     0,  1420,     5,
       0,     0,     0,   885,     0,     0,     0,     0,     0,   885,
       0,     0,  1069,  1069,  1069,  1069,     0,     6,  1069,     7,
       8,     0,     0,     0,     9,    10,     0,     0,     0,     0,
       0,    11,    12,  1069,     0,  1557,     0,  1420,     0,     0,
       0,     0,     0,     0,    13,     0,     0,    14,     0,     0,
       0,     0,     0,    15,     0,     0,     0,     0,   883,     0,
       0,    16,     0,     0,  1758,   884,    17,     0,     0,  1759,
       0,    18,     0,    19,     0,     0,     0,    20,     0,  1048,
       0,     0,     0,  1049,   884,   887,     0,     0,     0,    21,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1050,  1051,     0,     0,     0,     0,     0,     0,    22,
     885,     0,  1052,     0,     0,     0,     0,  1069,     0,     0,
       0,     0,     0,  1069,     0,  1069,     0,     0,     0,   885,
      23,    24,     0,     0,     0,     0,    25,     0,  1053,     0,
       0,  1054,     0,    26,     0,     0,     0,     0,     0,     0,
       0,     0,  1055,     0,  1056,     0,    27,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1623,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   887,     0,    28,   884,     0,     0,     0,
       0,     0,     0,     0,     0,    29,     0,     0,     0,    30,
       0,     0,     0,    31,     0,    32,  1057,  1044,     0,     0,
       0,     0,     0,    33,  1046,     0,     0,     0,    34,  1047,
       0,    35,     0,     0,     0,     0,     0,    36,     0,  1682,
       0,   885,     0,     0,     0,     0,     0,     0,     0,     0,
     745,     0,     0,     0,     0,     0,     0,     0,   884,     0,
       0,    37,     0,     0,     0,     0,     0,     0,    38,     0,
      39,     0,     0,  1711,    40,     0,     0,     0,     0,     0,
       0,     0,  1058,     0,     0,  1059,  1060,  1061,     0,  1062,
    1063,  1064,  1065,  1066,  1067,     0,     0,     0,     0,     0,
    1068,     0,     0,   885,     0,     0,     0,  1739,     0,     0,
       0,     0,     0,     0,    41,     0,     0,     0,     0,     0,
       0,     0,     0,  1439,  1048,  1439,  1439,     0,  1049,     0,
    1774,     0,     0,     0,  1777,     0,     0,  1069,     0,     0,
       0,     0,     0,     0,     0,     0,  1050,  1051,     0,     0,
     887,     0,     0,     0,     0,     0,   887,  1052,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1799,     0,  1801,     0,     0,     0,  1804,     0,
       0,     0,     0,  1053,     0,     0,  1054,     0,  1069,     0,
    1069,     0,     0,     0,     0,     0,     0,  1055,     0,  1056,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1069,     0,     0,  1069,     0,  1420,  1420,  1420,  1420,     0,
    1420,  1754,     0,     0,  1127,     0,  1069,  1069,     0,     0,
       0,  1135,  1135,  1133,  1069,     0,     0,  1846,     0,  1069,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1057,     0,  1044,     0,     0,  1715,   887,     0,     0,
    1046,     0,     0,     0,     0,  1047,     0,  1069,     0,     0,
    1754,  1069,     0,     0,     0,     0,   887,     0,     0,     0,
       0,     0,  1069,  1420,     0,     0,     0,     0,     0,  1069,
       0,  1069,  1069,  1069,  1069,  1069,  1069,  1069,  1069,  1069,
       0,  1069,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1058,     0,     0,
    1059,  1060,  1061,     0,  1062,  1063,  1064,  1065,  1066,  1067,
       0,     0,  1044,     0,  1445,  1068,     0,     0,     0,  1046,
       0,     0,     0,     0,  1047,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1069,     0,     0,
    1048,     0,     0,     0,  1049,     0,     0,     0,     0,  1046,
       0,     0,     0,     0,  1047,     0,     0,     0,   887,     0,
       0,     0,  1050,  1051,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1052,     0,     0,     0,     0,     0,     0,
    1978,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1983,     0,     0,     0,  1984,     0,     0,  1053,
    1986,     0,  1054,  1069,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1055,     0,  1056,     0,     0,     0,  1048,
     887,  2035,     0,  1049,     0,     0,  2003,  2004,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1050,  1051,     0,     0,     0,     0,     0,     0, -1316,
       0,     0,  1052,  1049,     0,     0,  2023,     0,  2025,     0,
       0,     0,  1069,     0,     0,  2029,     0,  1057,     0,     0,
    2033,  1050,  1051,  2036,     0,     0,     0,     0,  1053,     0,
       0,  1054, -1316,     0,     0,     0,     0,     0,     0,     0,
       0,  1069,  1055,     0,  1056,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  2070,     0,     0,     0,     0,
       0,  1054,     0,     0,     0,  1754,  1754,  1754,  1754,  1069,
    1754,  1420,  1055,     0,  1420,  1420,  1420,  1420,  1420,  1420,
    1420,  1420,  1420,  1058,  1420,     0,  1059,  1060,  1061,     0,
    1062,  1063,  1064,  1065,  1066,  1067,  1057,     0,     0,     0,
       0,  1068,     0,     0,  1069,     0,     0,  1069,     0,     0,
       0,     0,  1044,     0,     0,     0,     0,  1420,     0,  1046,
       0,     0,     0,     0,  1047,     0, -1316,     0,  1754,  1069,
       0,  1069,     0,     0,  1069,     0,     0,  1717,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1058,     0,     0,  1059,  1060,  1061,     0,  1062,
    1063,  1064,  1065,  1066,  1067,     0,  1069,     0,     0,     0,
    1068,     0,     0,     0,     0,     0,   745,     0,     0,     0,
       0,     0,  1058,     0,     0,  2174,     0,     0,     0,  1062,
    1063,  1064,  1065,  1066,  1067,     0,     0,     0,     0,     0,
    1068,     0,     0,     0,     0,     0,     0,     0,     0,  1048,
       0,     0,     0,  1049,     0,     0,     0,     0,  1044,     0,
       0,     0,     0,  2185,     0,  1046,     0,     0,     0,     0,
    1047,  1050,  1051,     0,     0,  1420,     0,     0,     0,  2191,
    2192,     0,  1052,  2194,     0,     0,     0,     0,     0,     0,
       0,     0,  1044,     0,     0,  1767,     0,     0,     0,  1046,
       0,     0,     0,     0,  1047,     0,     0,     0,  1053,     0,
       0,  1054,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1055,     0,  1056,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1732,     0,     0,  1069,     0,
       0,     0,     0,  1069,  1069,     0,  1069,     0,  1754,     0,
       0,     0,  1754,  1754,  1754,  1754,  1754,  1754,  1754,  1754,
    1754,     0,  1754,  1069,  1069,  1048,     0,  2246,     0,  1049,
       0,     0,     0,     0,     0,     0,  1057,     0,     0,     0,
       0,     0,  1754,  1069,     0,  1069,     0,  1050,  1051,  1069,
    1420,     0,     0,  1069,     0,     0,  1069,     0,  1052,  1048,
       0,     0,     0,  1049,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     745,  1050,  1051,     0,  1053,     0,     0,  1054,     0,     0,
    1069,     0,  1052,     0,     0,     0,     0,     0,  1055,     0,
    1056,  2295,  1058,     0,     0,  1059,  1060,  1061,     0,  1062,
    1063,  1064,  1065,  1066,  1067,     0,     0,     0,  1053,     0,
    1068,  1054,     0,     0,     0,     0,     0,     0,  2312,     0,
       0,     0,  1055,     0,  1056,     0,     0,     0,     0,     0,
       0,  1754,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1057,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  2338,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1057,     0,     0,     0,
       0,     0,     0,     0,  1069,     0,     0,     0,     0,     0,
       0,  1420,     0,     0,     0,  1069,     0,     0,  2357,     0,
    1754,  1069,  1069,     0,  1069,     0,     0,     0,  1058,     0,
       0,  1059,  1060,  1061,     0,  1062,  1063,  1064,  1065,  1066,
    1067,     0,     0,     0,     0,  1757,  1068,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1058,     0,     0,  1059,  1060,  1061,     0,  1062,
    1063,  1064,  1065,  1066,  1067,     0,  1069,     0,     0,     0,
    1068,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1754,
       0,     0,     0,     0,     0,  1069,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1069,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   138,
     139,   140,   141,   142,   143,   144,  1513,   145,  1069,     0,
       0,     0,   669,     0,     0,   146,   147,   148,   516,   149,
     150,   151,   517,   670,   518,   671,   672,  1069,   155,   156,
     157,   158,   673,   674,   159,   675,   676,   162,     0,   163,
     164,   165,   166,   677,     0,     0,   168,   169,   170,     0,
     171,   172,   678,   174,     0,   175,   176,   519,   679,   680,
     681,   682,   177,   178,   179,   180,   181,   683,   684,   184,
       0,   185,     0,   186,   187,   188,   189,   190,     0,  1514,
       0,   191,   685,   193,   194,     0,   195,   196,     0,   197,
       0,   198,   199,   200,   686,   202,   203,   687,   688,   205,
     206,   689,     0,   208,     0,   209,   520,     0,   521,   210,
     211,     0,     0,   212,     0,   213,   214,   522,   215,   216,
     217,   523,   218,   219,   220,   221,     0,   524,   222,   223,
     224,   225,   226,   690,   691,     0,   692,     0,   230,   525,
     526,   231,   527,   232,   233,   234,   235,     0,   528,   236,
     529,     0,   237,   238,   239,   693,   694,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   695,   530,   696,
     358,   252,   253,   254,   255,   256,   697,   257,   258,   531,
     698,   699,   700,   261,     0,     0,   262,   359,     0,     0,
     701,   264,     0,     0,   265,   532,   533,   702,   267,   268,
     269,   270,   271,     0,   703,   273,   274,   275,     0,   276,
     277,   278,   279,   280,   704,   282,     0,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   534,   292,   705,   294,
     295,   296,   297,   298,   299,     0,   300,   301,   302,   706,
     303,   304,   305,   306,   535,   307,   707,     0,   309,   310,
     311,   312,   313,   314,   315,   316,   708,   318,     0,   319,
     320,   321,     0,   709,   710,     0,   324,     0,   325,   711,
     327,   712,   713,   329,   330,   331,   332,     0,   714,   333,
     334,   335,   336,   337,   715,     0,   338,   339,   340,   341,
     716,   343,   536,   344,   345,   346,     0,     0,   347,   348,
     349,   350,   351,   352,     0,   717,   718,   458,   719,   720,
     721,   433,   722,     0,     0,     0,     0,   723,   724,  1515,
       0,   726,   727,     0,     0,     0,   728,  1516,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,   669,     0,     0,   146,   147,   148,   516,   149,   150,
     151,   517,   670,   518,   671,   672,     0,   155,   156,   157,
     158,   673,   674,   159,   675,   676,   162,     0,   163,   164,
     165,   166,   677,     0,     0,   168,   169,   170,     0,   171,
     172,   678,   174,     0,   175,   176,   519,   679,   680,   681,
     682,   177,   178,   179,   180,   181,   683,   684,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   685,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,   686,   202,   203,   687,   688,   205,   206,
     689,     0,   208,     0,   209,   520,     0,   521,   210,   211,
       0,     0,   212,     0,   213,   214,   522,   215,   216,   217,
     523,   218,   219,   220,   221,     0,   524,   222,   223,   224,
     225,   226,   690,   691,     0,   692,     0,   230,   525,   526,
     231,   527,   232,   233,   234,   235,     0,   528,   236,   529,
       0,   237,   238,   239,   693,   694,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   695,   530,   696,   358,
     252,   253,   254,   255,   256,   697,   257,   258,   531,   698,
     699,   700,   261,     0,     0,   262,   359,     0,     0,   701,
     264,     0,     0,   265,   532,   533,   702,   267,   268,   269,
     270,   271,     0,   703,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   704,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   534,   292,   705,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,   706,   303,
     304,   305,   306,   535,   307,   707,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   708,   318,     0,   319,   320,
     321,     0,   709,   710,     0,   324,     0,   325,   711,   327,
     712,   713,   329,   330,   331,   332,     0,   714,   333,   334,
     335,   336,   337,   715,     0,   338,   339,   340,   341,   716,
     343,   536,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   717,   718,   458,   719,   720,   721,
     433,   722,     0,     0,     0,     0,   723,   724,     0,     0,
     726,   727,     0,     0,     0,   728,  1424,   138,   139,   140,
     141,   142,   143,   144,     0,   145,     0,     0,     0,     0,
     669,     0,     0,   146,   147,   148,   516,   149,   150,   151,
     517,   670,   518,   671,   672,     0,   155,   156,   157,   158,
     673,   674,   159,   675,   676,   162,     0,   163,   164,   165,
     166,   677,     0,     0,   168,   169,   170,     0,   171,   172,
     678,   174,     0,   175,   176,   519,   679,   680,   681,   682,
     177,   178,   179,   180,   181,   683,   684,   184,     0,   185,
       0,   186,   187,   188,   189,   190,     0,     0,     0,   191,
     685,   193,   194,     0,   195,   196,     0,   197,     0,   198,
     199,   200,   686,   202,   203,   687,   688,   205,   206,   689,
       0,   208,     0,   209,   520,     0,   521,   210,   211,     0,
       0,   212,     0,   213,   214,   522,   215,   216,   217,   523,
     218,   219,   220,   221,     0,   524,   222,   223,   224,   225,
     226,   690,   691,     0,   692,     0,   230,   525,   526,   231,
     527,   232,   233,   234,   235,     0,   528,   236,   529,     0,
     237,   238,   239,   693,   694,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   695,   530,   696,   358,   252,
     253,   254,   255,   256,   697,   257,   258,   531,   698,   699,
     700,   261,     0,     0,   262,   359,     0,     0,   701,   264,
       0,     0,   265,   532,   533,   702,   267,   268,   269,   270,
     271,     0,   703,   273,   274,   275,     0,   276,   277,   278,
     279,   280,   704,   282,     0,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   534,   292,   705,   294,   295,   296,
     297,   298,   299,     0,   300,   301,   302,   706,   303,   304,
     305,   306,   535,   307,   707,     0,   309,   310,   311,   312,
     313,   314,   315,   316,   708,   318,     0,   319,   320,   321,
       0,   709,   710,     0,   324,     0,   325,   711,   327,   712,
     713,   329,   330,   331,   332,     0,   714,   333,   334,   335,
     336,   337,   715,     0,   338,   339,   340,   341,   716,   343,
     536,   344,   345,   346,     0,     0,   347,   348,   349,   350,
     351,   352,     0,   717,   718,   458,   719,   720,   721,   433,
     722,     0,     0,     0,     0,   723,   724,     0,     0,   726,
     727,     0,     0,     0,   728,  2217,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,     0,
    2046,     0,   146,   147,   148,     0,   149,   150,   151,     0,
     152,     0,   153,   154,     0,   155,   156,   157,   158,     0,
       0,   159,   160,   161,   162,     0,   163,   164,   165,   166,
     167,     0,     0,   168,   169,   170,     0,   171,   172,   173,
     174,     0,   175,   176,     0,     0,     0,     0,     0,   177,
     178,   179,   180,   181,   182,   183,   184,     0,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   192,
     193,   194,     0,   195,   196,     0,   197,  -713,   198,   199,
     200,   201,   202,   203,   204,     0,   205,   206,   207,  -713,
     208,     0,   209,     0,     0,     0,   210,   211,     0,     0,
     212,     0,   213,   214,     0,   215,   216,   217,     0,   218,
     219,   220,   221,     0,     0,   222,   223,   224,   225,   226,
     227,   228,  -713,   229,     0,   230,     0,     0,   231,     0,
     232,   233,   234,   235,     0,     0,   236,     0,  -713,   237,
     238,   239,     0,     0,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,     0,   251,     0,   252,   253,
     254,   255,   256,     0,   257,   258,     0,     0,   259,   260,
     261,     0,  -713,   262,     0,     0,     0,   263,   264,     0,
    -713,   265,     0,     0,   266,   267,   268,   269,   270,   271,
       0,   272,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   281,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,     0,   292,   293,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,     0,   303,   304,   305,
     306,     0,   307,   308,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,     0,   319,   320,   321,     0,
     322,   323,     0,   324,     0,   325,   326,   327,   328,     0,
     329,   330,   331,   332,  -713,     0,   333,   334,   335,   336,
     337,     0,     0,   338,   339,   340,   341,   342,   343,     0,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   353,     0,     0,   138,   139,   140,   141,   142,
     143,   144,     0,   145,     0,     0,     0,     0,     0,     0,
       0,   146,   147,   148,   790,   149,   150,   151,     0,   855,
       0,   856,   857,     0,   155,   156,   157,   158,     0,     0,
     159,   858,   859,   162,     0,   163,   164,   165,   166,     0,
       0,     0,   168,   169,   170,     0,   171,   172,     0,   174,
       0,   175,   176,     0,     0,     0,     0,     0,   177,   178,
     179,   180,   181,   860,   861,   184,     0,   185,     0,   186,
     187,   188,   189,   190,     0,     0,     0,   191,   685,   193,
     194,     0,   195,   196,     0,   197,     0,   198,   199,   200,
       0,   202,   203,     0,     0,   205,   206,   862,     0,   208,
       0,   209,     0,     0,     0,   210,   211,     0,     0,   212,
       0,   213,   214,     0,   215,   216,   217,  1159,   218,   219,
     220,   221,     0,     0,  1160,   223,   224,   225,   226,   863,
     864,     0,   865,     0,   230,     0,     0,   231,     0,   232,
     233,   234,   235,     0,     0,   236,     0,     0,   237,   238,
     239,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   695,     0,   866,     0,   252,   253,   254,
     255,     0,     0,   257,   258,     0,     0,     0,   867,   261,
       0,     0,   262,     0,     0,     0,   263,   264,     0,     0,
    1161,     0,     0,     0,   267,   268,   269,   270,   271,     0,
       0,   273,   274,   275,     0,   276,   277,   278,   279,   280,
     868,   282,     0,   283,   284,   285,   286,   287,   288,   289,
     290,   291,     0,   292,     0,   294,   295,   296,   297,   298,
     299,     0,   300,   301,   302,     0,   303,   869,   305,   306,
       0,   307,   870,     0,   309,   310,   311,   312,   313,   314,
     315,   316,     0,   318,     0,   319,   320,   321,     0,   871,
     872,     0,   324,     0,   325,     0,   327,     0,     0,   329,
     330,   331,   332,     0,     0,   333,   334,   335,   336,   337,
       0,     0,   338,   339,   340,   341,   873,   343,     0,   344,
     345,   346,     0,     0,   347,   348,   349,   350,   351,   352,
       0,   874,     0,     0,     0,     0,     0,     0,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,   669,     0,  1162,   146,   147,   148,   516,   149,   150,
     151,   517,   670,   518,   671,   672,  1435,   155,   156,   157,
     158,   673,   674,   159,   675,   676,   162,     0,   163,   164,
     165,   166,   677,     0,     0,   168,   169,   170,     0,   171,
     172,   678,   174,     0,   175,   176,   519,   679,   680,   681,
     682,   177,   178,   179,   180,   181,   683,   684,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   685,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,   686,   202,   203,   687,   688,   205,   206,
     689,     0,   208,     0,   209,   520,  1436,   521,   210,   211,
       0,     0,   212,     0,   213,   214,   522,   215,   216,   217,
     523,   218,   219,   220,   221,     0,   524,   222,   223,   224,
     225,   226,   690,   691,     0,   692,     0,   230,   525,   526,
     231,   527,   232,   233,   234,   235,  1437,   528,   236,   529,
       0,   237,   238,   239,   693,   694,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   695,   530,   696,   358,
     252,   253,   254,   255,   256,   697,   257,   258,   531,   698,
     699,   700,   261,     0,     0,   262,   359,     0,     0,   701,
     264,     0,     0,   265,   532,   533,   702,   267,   268,   269,
     270,   271,     0,   703,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   704,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   534,   292,   705,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,   706,   303,
     304,   305,   306,   535,   307,   707,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   708,   318,     0,   319,   320,
     321,     0,   709,   710,     0,   324,  1438,   325,   711,   327,
     712,   713,   329,   330,   331,   332,     0,   714,   333,   334,
     335,   336,   337,   715,     0,   338,   339,   340,   341,   716,
     343,   536,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   717,   718,   458,   719,   720,   721,
     433,   722,     0,     0,     0,     0,   723,   724,     0,     0,
     726,   727,     0,     0,     0,   728,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,   669,
       0,     0,   146,   147,   148,   516,   149,   150,   151,   517,
     670,   518,   671,   672,     0,   155,   156,   157,   158,   673,
     674,   159,   675,   676,   162,     0,   163,   164,   165,   166,
     677,     0,     0,   168,   169,   170,     0,   171,   172,   678,
     174,     0,   175,   176,   519,   679,   680,   681,   682,   177,
     178,   179,   180,   181,   683,   684,   184,  1690,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   685,
     193,   194,     0,   195,   196,     0,   197,     0,   198,   199,
     200,   686,   202,   203,   687,   688,   205,   206,   689,     0,
     208,     0,   209,   520,     0,   521,   210,   211,     0,     0,
     212,     0,   213,   214,   522,   215,   216,   217,   523,   218,
     219,   220,   221,     0,   524,   222,   223,   224,   225,   226,
     690,   691,     0,   692,     0,   230,   525,   526,   231,   527,
     232,   233,   234,   235,     0,   528,   236,   529,     0,   237,
     238,   239,   693,   694,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   695,   530,   696,   358,   252,   253,
     254,   255,   256,   697,   257,   258,   531,   698,   699,   700,
     261,     0,     0,   262,   359,     0,     0,   701,   264,     0,
       0,   265,   532,   533,   702,   267,   268,   269,   270,   271,
       0,   703,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   704,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   534,   292,   705,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,   706,   303,   304,   305,
     306,   535,   307,   707,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   708,   318,     0,   319,   320,   321,     0,
     709,   710,     0,   324,     0,   325,   711,   327,   712,   713,
     329,   330,   331,   332,     0,   714,   333,   334,   335,   336,
     337,   715,     0,   338,   339,   340,   341,   716,   343,   536,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   717,   718,   458,   719,   720,   721,   433,   722,
       0,     0,     0,     0,   723,   724,   725,     0,   726,   727,
       0,     0,     0,   728,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,   669,     0,     0,
     146,   147,   148,   516,   149,   150,   151,   517,   670,   518,
     671,   672,     0,   155,   156,   157,   158,   673,   674,   159,
     675,   676,   162,     0,   163,   164,   165,   166,   677,     0,
       0,   168,   169,   170,     0,   171,   172,   678,   174,     0,
     175,   176,   519,   679,   680,   681,   682,   177,   178,   179,
     180,   181,   683,   684,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   685,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   686,
     202,   203,   687,   688,   205,   206,   689,     0,   208,     0,
     209,   520,     0,   521,   210,   211,     0,     0,   212,     0,
     213,   214,   522,   215,   216,   217,   523,   218,   219,   220,
     221,     0,   524,   222,   223,   224,   225,   226,   690,   691,
       0,   692,     0,   230,   525,   526,   231,   527,   232,   233,
     234,   235,     0,   528,   236,   529,     0,   237,   238,   239,
     693,   694,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   695,   530,   696,   358,   252,   253,   254,   255,
     256,   697,   257,   258,   531,   698,   699,   700,   261,     0,
       0,   262,   359,     0,     0,   701,   264,     0,     0,   265,
     532,   533,   702,   267,   268,   269,   270,   271,     0,   703,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   704,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   534,   292,   705,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,   706,   303,   304,   305,   306,   535,
     307,   707,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   708,   318,     0,   319,   320,   321,     0,   709,   710,
       0,   324,     0,   325,   711,   327,   712,   713,   329,   330,
     331,   332,     0,   714,   333,   334,   335,   336,   337,   715,
       0,   338,   339,   340,   341,   716,   343,   536,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     717,   718,   458,   719,   720,   721,   433,   722,     0,     0,
       0,     0,   723,   724,   725,     0,   726,   727,     0,     0,
       0,   728,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,   669,     0,     0,   146,   147,
     148,   516,   149,   150,   151,   517,   670,   518,   671,   672,
       0,   155,   156,   157,   158,   673,   674,   159,   675,   676,
     162,     0,   163,   164,   165,   166,   677,     0,     0,   168,
     169,   170,     0,   171,   172,   678,   174,     0,   175,   176,
     519,   679,   680,   681,   682,   177,   178,   179,   180,   181,
     683,   684,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   685,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,   686,   202,   203,
     687,   688,   205,   206,   689,     0,   208,     0,   209,   520,
       0,   521,   210,   211,     0,     0,   212,     0,   213,   214,
     522,   215,   216,   217,   523,   218,   219,   220,   221,     0,
     524,   222,   223,   224,   225,   226,   690,   691,     0,   692,
       0,   230,   525,   526,   231,   527,   232,   233,   234,   235,
       0,   528,   236,   529,     0,   237,   238,   239,   693,   694,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     695,   530,   696,   358,   252,   253,   254,   255,   256,   697,
     257,   258,   531,   698,   699,   700,   261,     0,     0,   262,
     359,     0,     0,   701,   264,     0,     0,   265,   532,   533,
     702,   267,   268,   269,   270,   271,     0,   703,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   704,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   534,
     292,   705,   294,   295,   296,   297,   298,   299,    33,   300,
     301,   302,   706,   303,   304,   305,   306,   535,   307,   707,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   708,
     318,     0,   319,   320,   321,     0,   709,   710,     0,   324,
       0,   325,   711,   327,   712,   713,   329,   330,   331,   332,
       0,   714,   333,   334,   335,   336,   337,   715,     0,   338,
     339,   340,   341,   716,   343,   536,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   717,   718,
     458,   719,   720,   721,   433,   722,     0,     0,     0,     0,
     723,   724,     0,     0,   726,   727,     0,     0,     0,   728,
     138,   139,   140,   141,   142,   143,   144,  1131,   145,     0,
       0,     0,     0,   669,     0,     0,   146,   147,   148,   516,
     149,   150,   151,   517,   670,   518,   671,   672,     0,   155,
     156,   157,   158,   673,   674,   159,   675,   676,   162,     0,
     163,   164,   165,   166,   677,     0,     0,   168,   169,   170,
       0,   171,   172,   678,   174,     0,   175,   176,   519,   679,
     680,   681,   682,   177,   178,   179,   180,   181,   683,   684,
     184,     0,   185,     0,   186,   187,   188,   189,   190,     0,
       0,     0,   191,   685,   193,   194,     0,   195,   196,     0,
     197,     0,   198,   199,   200,   686,   202,   203,   687,   688,
     205,   206,   689,     0,   208,     0,   209,   520,     0,   521,
     210,   211,     0,     0,   212,     0,   213,   214,   522,   215,
     216,   217,   523,   218,   219,   220,   221,     0,   524,   222,
     223,   224,   225,   226,   690,   691,     0,   692,     0,   230,
     525,   526,   231,   527,   232,   233,   234,   235,     0,   528,
     236,   529,     0,   237,   238,   239,   693,   694,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   695,   530,
     696,   358,   252,   253,   254,   255,   256,   697,   257,   258,
     531,   698,   699,   700,   261,     0,     0,   262,   359,     0,
       0,   701,   264,     0,     0,   265,   532,   533,   702,   267,
     268,   269,   270,   271,     0,   703,   273,   274,   275,     0,
     276,   277,   278,   279,   280,   704,   282,     0,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   534,   292,   705,
     294,   295,   296,   297,   298,   299,     0,   300,   301,   302,
     706,   303,   304,   305,   306,   535,   307,   707,     0,   309,
     310,   311,   312,   313,   314,   315,   316,   708,   318,     0,
     319,   320,   321,     0,   709,   710,     0,   324,     0,   325,
     711,   327,   712,   713,   329,   330,   331,   332,     0,   714,
     333,   334,   335,   336,   337,   715,     0,   338,   339,   340,
     341,   716,   343,   536,   344,   345,   346,     0,     0,   347,
     348,   349,   350,   351,   352,     0,   717,   718,   458,   719,
     720,   721,   433,   722,     0,     0,     0,     0,   723,   724,
       0,     0,   726,   727,     0,     0,     0,   728,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,   669,     0,     0,   146,   147,   148,   516,   149,   150,
     151,   517,   670,   518,   671,   672,     0,   155,   156,   157,
     158,   673,   674,   159,   675,   676,   162,     0,   163,   164,
     165,   166,   677,     0,     0,   168,   169,   170,     0,   171,
     172,   678,   174,     0,   175,   176,   519,   679,   680,   681,
     682,   177,   178,   179,   180,   181,   683,   684,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   685,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,   686,   202,   203,   687,   688,   205,   206,
     689,     0,   208,     0,   209,   520,     0,   521,   210,   211,
       0,     0,   212,     0,   213,   214,   522,   215,   216,   217,
     523,   218,   219,   220,   221,     0,   524,   222,   223,   224,
     225,   226,   690,   691,     0,   692,     0,   230,   525,   526,
     231,   527,   232,   233,   234,   235,     0,   528,   236,   529,
       0,   237,   238,   239,   693,   694,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   695,   530,   696,   358,
     252,   253,   254,   255,   256,   697,   257,   258,   531,   698,
     699,   700,   261,     0,     0,   262,   359,     0,     0,   701,
     264,     0,     0,   265,   532,   533,   702,   267,   268,   269,
     270,   271,     0,   703,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   704,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   534,   292,   705,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,   706,   303,
     304,   305,   306,   535,   307,   707,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   708,   318,     0,   319,   320,
     321,     0,   709,   710,     0,   324,     0,   325,   711,   327,
     712,   713,   329,   330,   331,   332,     0,   714,   333,   334,
     335,   336,   337,   715,     0,   338,   339,   340,   341,   716,
     343,   536,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   717,   718,   458,   719,   720,   721,
     433,   722,     0,     0,     0,     0,   723,   724,     0,     0,
     726,   727,     0,   975,     0,   728,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,   669,
       0,     0,   146,   147,   148,   516,   149,   150,   151,   517,
     670,   518,   671,   672,     0,   155,   156,   157,   158,   673,
     674,   159,   675,   676,   162,     0,   163,   164,   165,   166,
     677,     0,     0,   168,   169,   170,     0,   171,   172,   678,
     174,     0,   175,   176,   519,   679,   680,   681,   682,   177,
     178,   179,   180,   181,   683,   684,   184,     0,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   685,
     193,   194,     0,   195,   196,     0,   197,     0,   198,   199,
     200,   686,   202,   203,   687,   688,   205,   206,   689,     0,
     208,     0,   209,   520,  1436,   521,   210,   211,     0,     0,
     212,     0,   213,   214,   522,   215,   216,   217,   523,   218,
     219,   220,   221,     0,   524,   222,   223,   224,   225,   226,
     690,   691,     0,   692,     0,   230,   525,   526,   231,   527,
     232,   233,   234,   235,     0,   528,   236,   529,     0,   237,
     238,   239,   693,   694,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   695,   530,   696,   358,   252,   253,
     254,   255,   256,   697,   257,   258,   531,   698,   699,   700,
     261,     0,     0,   262,   359,     0,     0,   701,   264,     0,
       0,   265,   532,   533,   702,   267,   268,   269,   270,   271,
       0,   703,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   704,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   534,   292,   705,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,   706,   303,   304,   305,
     306,   535,   307,   707,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   708,   318,     0,   319,   320,   321,     0,
     709,   710,     0,   324,     0,   325,   711,   327,   712,   713,
     329,   330,   331,   332,     0,   714,   333,   334,   335,   336,
     337,   715,     0,   338,   339,   340,   341,   716,   343,   536,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   717,   718,   458,   719,   720,   721,   433,   722,
       0,     0,     0,     0,   723,   724,     0,     0,   726,   727,
       0,     0,     0,   728,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,   669,     0,     0,
     146,   147,   148,   516,   149,   150,   151,   517,   670,   518,
     671,   672,     0,   155,   156,   157,   158,   673,   674,   159,
     675,   676,   162,     0,   163,   164,   165,   166,   677,     0,
       0,   168,   169,   170,     0,   171,   172,   678,   174,     0,
     175,   176,   519,   679,   680,   681,   682,   177,   178,   179,
     180,   181,   683,   684,   184,  2069,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   685,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   686,
     202,   203,   687,   688,   205,   206,   689,     0,   208,     0,
     209,   520,     0,   521,   210,   211,     0,     0,   212,     0,
     213,   214,   522,   215,   216,   217,   523,   218,   219,   220,
     221,     0,   524,   222,   223,   224,   225,   226,   690,   691,
       0,   692,     0,   230,   525,   526,   231,   527,   232,   233,
     234,   235,     0,   528,   236,   529,     0,   237,   238,   239,
     693,   694,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   695,   530,   696,   358,   252,   253,   254,   255,
     256,   697,   257,   258,   531,   698,   699,   700,   261,     0,
       0,   262,   359,     0,     0,   701,   264,     0,     0,   265,
     532,   533,   702,   267,   268,   269,   270,   271,     0,   703,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   704,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   534,   292,   705,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,   706,   303,   304,   305,   306,   535,
     307,   707,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   708,   318,     0,   319,   320,   321,     0,   709,   710,
       0,   324,     0,   325,   711,   327,   712,   713,   329,   330,
     331,   332,     0,   714,   333,   334,   335,   336,   337,   715,
       0,   338,   339,   340,   341,   716,   343,   536,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     717,   718,   458,   719,   720,   721,   433,   722,     0,     0,
       0,     0,   723,   724,     0,     0,   726,   727,     0,     0,
       0,   728,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,   669,     0,     0,   146,   147,
     148,   516,   149,   150,   151,   517,   670,   518,   671,   672,
       0,   155,   156,   157,   158,   673,   674,   159,   675,   676,
     162,     0,   163,   164,   165,   166,   677,     0,     0,   168,
     169,   170,     0,   171,   172,   678,   174,     0,   175,   176,
     519,   679,   680,   681,   682,   177,   178,   179,   180,   181,
     683,   684,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   685,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,   686,   202,   203,
     687,   688,   205,   206,   689,     0,   208,     0,   209,   520,
       0,   521,   210,   211,     0,     0,   212,     0,   213,   214,
     522,   215,   216,   217,   523,   218,   219,   220,   221,     0,
     524,   222,   223,   224,   225,   226,   690,   691,     0,   692,
       0,   230,   525,   526,   231,   527,   232,   233,   234,   235,
       0,   528,   236,   529,     0,   237,   238,   239,   693,   694,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     695,   530,   696,   358,   252,   253,   254,   255,   256,   697,
     257,   258,   531,   698,   699,   700,   261,     0,     0,   262,
     359,     0,     0,   701,   264,     0,     0,   265,   532,   533,
     702,   267,   268,   269,   270,   271,     0,   703,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   704,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   534,
     292,   705,   294,   295,   296,   297,   298,   299,     0,   300,
     301,   302,   706,   303,   304,   305,   306,   535,   307,   707,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   708,
     318,     0,   319,   320,   321,     0,   709,   710,     0,   324,
       0,   325,   711,   327,   712,   713,   329,   330,   331,   332,
       0,   714,   333,   334,   335,   336,   337,   715,     0,   338,
     339,   340,   341,   716,   343,   536,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   717,   718,
     458,   719,   720,   721,   433,   722,     0,     0,     0,     0,
     723,   724,     0,     0,   726,   727,     0,     0,     0,   728,
     138,   139,   140,   141,   142,   143,   144,     0,   145,     0,
       0,     0,     0,   669,     0,     0,   146,   147,   148,   516,
     149,   150,   151,   517,   670,   518,   671,   672,     0,   155,
     156,   157,   158,   673,   674,   159,   675,   676,   162,     0,
     163,   164,   165,   166,   677,     0,     0,   168,   169,   170,
       0,   171,   172,   678,   174,     0,   175,   176,   519,   679,
     680,   681,   682,   177,   178,   179,   180,   181,   683,   684,
     184,     0,   185,     0,   186,   187,   188,   189,   190,     0,
       0,     0,   191,   685,   193,   194,     0,   195,   196,     0,
     197,     0,   198,   199,   200,   686,   202,   203,   687,   688,
     205,   206,   689,     0,   208,     0,   209,   520,     0,   521,
     210,   211,     0,     0,   212,     0,   213,   214,   522,   215,
     216,   217,   523,   218,   219,   220,   221,     0,   524,   222,
     223,   224,   225,   226,   690,   691,     0,   692,     0,   230,
     525,   526,   231,   527,   232,   233,   234,   235,     0,   528,
     236,   529,     0,   237,   238,   239,   693,   694,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   695,   530,
     696,   358,   252,   253,   254,   255,   256,     0,   257,   258,
     531,   698,   699,   700,   261,     0,     0,   262,   359,     0,
       0,   701,   264,     0,     0,   265,   532,   533,   702,   267,
     268,   269,   270,   271,     0,   703,   273,   274,   275,     0,
     276,   277,   278,   279,   280,   704,   282,     0,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   534,   292,   293,
     294,   295,   296,   297,   298,   299,     0,   300,   301,   302,
     706,   303,   304,   305,   306,   535,   307,   707,     0,   309,
     310,   311,   312,   313,   314,   315,   316,   708,   318,     0,
     319,   320,   321,     0,   709,   710,     0,   324,     0,   325,
     711,   327,   712,   713,   329,   330,   331,   332,     0,     0,
     333,   334,   335,   336,   337,   715,     0,   338,   339,   340,
     341,   716,   343,   536,   344,   345,   346,     0,     0,   347,
     348,   349,   350,   351,   352,     0,   717,   718,   458,   719,
     720,   721,   433,   722,     0,     0,     0,     0,  1415,  1416,
       0,     0,  1417,  1418,     0,     0,     0,  1419,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,   669,     0,     0,   146,   147,   148,   516,   149,   150,
     151,     0,   670,   518,   671,   672,     0,   155,   156,   157,
     158,   673,   674,   159,   675,   676,   162,     0,   163,   164,
     165,   166,   677,     0,     0,   168,   169,   170,     0,   171,
     172,   678,   174,     0,   175,   176,   519,   679,   680,   681,
     682,   177,   178,   179,   180,   181,   683,   684,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   685,   193,   194,     0,   195,   196,     0,     0,     0,
     198,   199,   200,   686,   202,   203,   687,   688,   205,   206,
     689,     0,   208,     0,   209,   520,     0,   521,   210,   211,
       0,     0,   212,     0,   213,   214,     0,   215,   216,   217,
       0,   218,   219,   220,   221,     0,   524,   222,   223,   224,
     225,   226,   690,   691,     0,   692,     0,   230,   525,   526,
     231,   527,   232,   233,   234,   235,     0,   528,   236,     0,
       0,   237,   238,   239,   693,   694,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   695,   530,   696,   358,
     252,   253,   254,   255,   256,     0,   257,   258,   531,   698,
     699,   700,   261,     0,     0,   262,   359,     0,     0,   701,
     264,     0,     0,   265,   532,   533,   702,   267,   268,   269,
     270,   271,     0,   703,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   704,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   534,   292,   705,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,   706,   303,
     304,   305,   306,     0,   307,   707,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   708,   318,     0,   319,   320,
     321,     0,   709,   710,     0,   324,     0,   325,   711,   327,
     712,   713,   329,   330,   331,   332,     0,   714,   333,   334,
     335,   336,   337,   715,     0,   338,   339,   340,   341,   716,
     343,   536,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   717,   718,   458,   719,   720,   721,
     433,   722,     0,     0,     0,     0,   723,   724,     0,     0,
     726,   727,     0,     0,     0,   728,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,   669,
       0,     0,   146,   147,   148,   516,   149,   150,   151,   517,
     670,   518,   671,   672,     0,   155,   156,   157,   158,   673,
     674,   159,   675,   676,   162,     0,   163,   164,   165,   166,
     677,     0,     0,   168,   169,   170,     0,   171,   172,   678,
     174,     0,   175,   176,   519,   679,   680,   681,   682,   177,
     178,   179,   180,   181,   683,   684,   184,     0,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   685,
     193,   194,     0,   195,   196,     0,   197,     0,   198,   199,
     200,   686,   202,   203,   687,   688,   205,   206,   689,     0,
     208,     0,   209,   520,     0,   521,   210,   211,     0,     0,
     212,     0,   213,   214,   522,   215,   216,   217,   523,   218,
     219,   220,   221,     0,   524,   222,   223,   224,   225,   226,
     690,   691,     0,   692,     0,   230,   525,   526,   231,   527,
     232,   233,   234,   235,     0,   528,   236,   529,     0,   237,
     238,   239,   693,   694,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   695,   530,   696,   358,   252,   253,
     254,   255,   256,     0,   257,   258,   531,   698,   699,   700,
     261,     0,     0,   262,   359,     0,     0,   263,   264,     0,
       0,   265,   532,   533,   702,   267,   268,   269,   270,   271,
       0,   703,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   704,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   534,   292,   293,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,   706,   303,   304,   305,
     306,   535,   307,   707,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   708,   318,     0,   319,   320,   321,     0,
     709,   710,     0,   324,     0,   325,   711,   327,   712,   713,
     329,   330,   331,   332,     0,     0,   333,   334,   335,   336,
     337,   715,     0,   338,   339,   340,   341,   716,   343,   536,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   717,   718,   458,   719,   720,     0,   433,   722,
       0,   138,   139,   140,   141,   142,   143,   144,     0,   145,
       0,     0,     0,  1419,   669,     0,     0,   146,   147,   148,
     516,   149,   150,   151,     0,   670,   518,   671,   672,     0,
     155,   156,   157,   158,   673,   674,   159,   675,   676,   162,
       0,   163,   164,   165,   166,   677,     0,     0,   168,   169,
     170,     0,   171,   172,   678,   174,     0,   175,   176,   519,
     679,   680,   681,   682,   177,   178,   179,   180,   181,   683,
     684,   184,     0,   185,     0,   186,   187,   188,   189,   190,
       0,     0,     0,   191,   685,   193,   194,     0,   195,   196,
       0,     0,     0,   198,   199,   200,   686,   202,   203,   687,
     688,   205,   206,   689,     0,   208,     0,   209,   520,     0,
     521,   210,   211,     0,     0,   212,     0,   213,   214,     0,
     215,   216,   217,     0,   218,   219,   220,   221,     0,   524,
     222,   223,   224,   225,   226,   690,   691,     0,   692,     0,
     230,   525,   526,   231,   527,   232,   233,   234,   235,     0,
     528,   236,     0,     0,   237,   238,   239,   693,   694,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   695,
     530,   696,   358,   252,   253,   254,   255,   256,     0,   257,
     258,   531,   698,   699,   700,   261,     0,     0,   262,   359,
       0,     0,   701,   264,     0,     0,   265,   532,   533,   702,
     267,   268,   269,   270,   271,     0,   703,   273,   274,   275,
       0,   276,   277,   278,   279,   280,   704,   282,     0,   283,
     284,   285,   286,   287,   288,   289,   290,   291,   534,   292,
     293,   294,   295,   296,   297,   298,   299,     0,   300,   301,
     302,   706,   303,   304,   305,   306,     0,   307,   707,     0,
     309,   310,   311,   312,   313,   314,   315,   316,   708,   318,
       0,   319,   320,   321,     0,   709,   710,     0,   324,     0,
     325,   711,   327,   712,   713,   329,   330,   331,   332,     0,
       0,   333,   334,   335,   336,   337,   715,     0,   338,   339,
     340,   341,   716,   343,   536,   344,   345,   346,     0,     0,
     347,   348,   349,   350,   351,   352,     0,   717,   718,   458,
     719,   720,   721,   433,   722,     0,     0,     0,     0,  1415,
    1416,     0,     0,  1417,  1418,     0,     0,     0,  1419,   138,
     139,   140,   141,   142,   143,   144,  -918,   145,     0,     0,
       0,  -918,   669,     0,     0,   146,   147,   148,   516,   149,
     150,   151,     0,   670,   518,   671,   672,     0,   155,   156,
     157,   158,   673,   674,   159,   675,   676,   162,     0,   163,
     164,   165,   166,   677,     0,     0,   168,   169,   170,     0,
     171,   172,   678,   174,     0,   175,   176,   519,   679,   680,
     681,   682,   177,   178,   179,   180,   181,   683,   684,   184,
       0,   185,     0,   186,   187,   188,   189,   190,     0,     0,
       0,   191,   685,   193,   194,     0,   195,   196,     0,     0,
       0,   198,   199,   200,   686,   202,   203,   687,   688,   205,
     206,   689,     0,   208,     0,   209,   520,     0,   521,   210,
     211,     0,     0,   212,     0,   213,   214,     0,   215,   216,
     217,     0,   218,   219,   220,   221,     0,   524,   222,   223,
     224,   225,   226,   690,   691,     0,   692,     0,   230,     0,
       0,   231,   527,   232,   233,   234,   235,     0,   528,   236,
       0,     0,   237,   238,   239,   693,   694,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   695,   530,   696,
     358,   252,   253,   254,   255,   256,     0,   257,   258,     0,
     698,   699,   700,   261,     0,     0,   262,   359,     0,     0,
       0,   264,     0,     0,   265,   532,   533,   702,   267,   268,
     269,   270,   271,     0,   703,   273,   274,   275,     0,   276,
     277,   278,   279,   280,   704,   282,     0,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   534,   292,   705,   294,
     295,   296,   297,   298,   299,     0,   300,   301,   302,   706,
     303,   304,   305,   306,     0,   307,   707,  -918,   309,   310,
     311,   312,   313,   314,   315,   316,   708,   318,     0,   319,
     320,   321,     0,   709,   710,     0,   324,     0,   325,   711,
     327,   712,   713,   329,   330,   331,   332,     0,   714,   333,
     334,   335,   336,   337,   715,     0,   338,   339,   340,   341,
     716,   343,   536,   344,   345,   346,     0,     0,   347,   348,
     349,   350,   351,   352,     0,   717,   718,   458,   719,   720,
       0,   433,   722,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   727,     0,     0,     0,   728,   138,   139,   140,
     141,   142,   143,   144,  -919,   145,     0,     0,     0,  -919,
     669,     0,     0,   146,   147,   148,   516,   149,   150,   151,
       0,   670,   518,   671,   672,     0,   155,   156,   157,   158,
     673,   674,   159,   675,   676,   162,     0,   163,   164,   165,
     166,   677,     0,     0,   168,   169,   170,     0,   171,   172,
     678,   174,     0,   175,   176,   519,   679,   680,   681,   682,
     177,   178,   179,   180,   181,   683,   684,   184,     0,   185,
       0,   186,   187,   188,   189,   190,     0,     0,     0,   191,
     685,   193,   194,     0,   195,   196,     0,     0,     0,   198,
     199,   200,   686,   202,   203,   687,   688,   205,   206,   689,
       0,   208,     0,   209,   520,     0,   521,   210,   211,     0,
       0,   212,     0,   213,   214,     0,   215,   216,   217,     0,
     218,   219,   220,   221,     0,   524,   222,   223,   224,   225,
     226,   690,   691,     0,   692,     0,   230,     0,     0,   231,
     527,   232,   233,   234,   235,     0,   528,   236,     0,     0,
     237,   238,   239,   693,   694,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   695,   530,   696,   358,   252,
     253,   254,   255,   256,     0,   257,   258,     0,   698,   699,
     700,   261,     0,     0,   262,   359,     0,     0,     0,   264,
       0,     0,   265,   532,   533,   702,   267,   268,   269,   270,
     271,     0,   703,   273,   274,   275,     0,   276,   277,   278,
     279,   280,   704,   282,     0,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   534,   292,   705,   294,   295,   296,
     297,   298,   299,     0,   300,   301,   302,   706,   303,   304,
     305,   306,     0,   307,   707,  -919,   309,   310,   311,   312,
     313,   314,   315,   316,   708,   318,     0,   319,   320,   321,
       0,   709,   710,     0,   324,     0,   325,   711,   327,   712,
     713,   329,   330,   331,   332,     0,   714,   333,   334,   335,
     336,   337,   715,     0,   338,   339,   340,   341,   716,   343,
     536,   344,   345,   346,     0,     0,   347,   348,   349,   350,
     351,   352,     0,   717,   718,   458,   719,   720,     0,   433,
     722,     0,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,   728,   669,     0,     0,   146,   147,
     148,   516,   149,   150,   151,     0,   670,   518,   671,   672,
       0,   155,   156,   157,   158,   673,   674,   159,   675,   676,
     162,     0,   163,   164,   165,   166,   677,     0,     0,   168,
     169,   170,     0,   171,   172,   678,   174,     0,   175,   176,
     519,   679,   680,   681,   682,   177,   178,   179,   180,   181,
     683,   684,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   685,   193,   194,     0,   195,
     196,     0,     0,     0,   198,   199,   200,   686,   202,   203,
     687,   688,   205,   206,   689,     0,   208,     0,   209,   520,
       0,   521,   210,   211,     0,     0,   212,     0,   213,   214,
       0,   215,   216,   217,     0,   218,   219,   220,   221,     0,
     524,   222,   223,   224,   225,   226,   690,   691,     0,   692,
       0,   230,     0,     0,   231,   527,   232,   233,   234,   235,
       0,   528,   236,     0,     0,   237,   238,   239,   693,   694,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     695,   530,   696,   358,   252,   253,   254,   255,   256,     0,
     257,   258,     0,     0,   699,   700,   261,     0,     0,   262,
     359,     0,     0,     0,   264,     0,     0,   265,   532,   533,
     702,   267,   268,   269,   270,   271,     0,   703,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   704,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   534,
     292,   293,   294,   295,   296,   297,   298,   299,     0,   300,
     301,   302,   706,   303,   304,   305,   306,     0,   307,   707,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   708,
     318,     0,   319,   320,   321,     0,   709,   710,     0,   324,
       0,   325,   711,   327,   712,   713,   329,   330,   331,   332,
       0,     0,   333,   334,   335,   336,   337,   715,     0,   338,
     339,   340,   341,   716,   343,   536,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   717,   718,
     458,   719,   720,     0,   433,   722,     0,     0,   138,   139,
     140,   141,   142,   143,   144,  1418,   145,     0,     0,  1419,
       0,   669,     0,     0,   146,   147,   148,   516,   149,   150,
     151,     0,   670,   518,   671,   672,     0,   155,   156,   157,
     158,   673,   674,   159,   675,   676,   162,     0,   163,   164,
     165,   166,   677,     0,     0,   168,   169,   170,     0,   171,
     172,   678,   174,     0,   175,   176,   519,   679,   680,   681,
     682,   177,   178,   179,   180,   181,   683,   684,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   685,   193,   194,     0,   195,   196,     0,     0,     0,
     198,   199,   200,   686,   202,   203,   687,   688,   205,   206,
     689,     0,   208,     0,   209,   520,     0,   521,   210,   211,
       0,     0,   212,     0,   213,   214,     0,   215,   216,   217,
       0,   218,   219,   220,   221,     0,   524,   222,   223,   224,
     225,   226,   690,   691,     0,   692,     0,   230,     0,     0,
     231,   527,   232,   233,   234,   235,     0,   528,   236,     0,
       0,   237,   238,   239,   693,   694,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   695,   530,   696,   358,
     252,   253,   254,   255,   256,     0,   257,   258,     0,     0,
     699,   700,   261,     0,     0,   262,   359,     0,     0,     0,
     264,     0,     0,   265,   532,   533,   702,   267,   268,   269,
     270,   271,     0,   703,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   704,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,   534,   292,   293,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,   706,   303,
     304,   305,   306,     0,   307,   707,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   708,   318,     0,   319,   320,
     321,     0,   709,   710,     0,   324,     0,   325,   711,   327,
     712,   713,   329,   330,   331,   332,     0,     0,   333,   334,
     335,   336,   337,   715,     0,   338,   339,   340,   341,   716,
     343,   536,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   717,   718,   458,   719,   720,     0,
     433,   722,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,  1419,     0,     0,   146,   147,
     148,   516,   149,   150,   151,   517,   152,   518,   153,   154,
       0,   155,   156,   157,   158,     0,     0,   159,   160,   161,
     162,     0,   163,   164,   165,   166,   167,     0,     0,   168,
     169,   170,     0,   171,   172,   173,   174,     0,   175,   176,
     519,     0,     0,     0,     0,   177,   178,   179,   180,   181,
     182,   183,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   192,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,   201,   202,   203,
     204,     0,   205,   206,   207,     0,   208,     0,   209,   520,
       0,   521,   210,   211,     0,     0,   212,     0,   213,   214,
     522,   215,   216,   217,   523,   218,   219,   220,   221,     0,
     524,   222,   223,   224,   225,   226,   227,   228,     0,   229,
       0,   230,   525,   526,   231,   527,   232,   233,   234,   235,
       0,   528,   236,   529,     0,   237,   238,   239,     0,     0,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,   530,   251,   358,   252,   253,   254,   255,   256,     0,
     257,   258,   531,     0,   259,   260,   261,     0,     0,   262,
     359,     0,   499,   263,   264,     0,     0,   265,   532,   533,
     266,   267,   268,   269,   270,   271,     0,   272,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   281,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,   534,
     292,   293,   294,   295,   296,   297,   298,   299,    33,   300,
     301,   302,     0,   303,   304,   305,   306,   535,   307,   308,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,     0,   319,   320,   321,     0,   322,   323,     0,   324,
       0,   325,   326,   327,   328,     0,   329,   330,   331,   332,
       0,     0,   333,   334,   335,   336,   337,     0,     0,   338,
     339,   340,   341,   342,   343,   536,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   537,     0,
       0,     0,     0,     0,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,     0,     0,  1814,
     146,   147,   148,   516,   149,   150,   151,   517,   152,   518,
     153,   154,     0,   155,   156,   157,   158,     0,     0,   159,
     160,   161,   162,     0,   163,   164,   165,   166,   167,     0,
       0,   168,   169,   170,     0,   171,   172,   173,   174,     0,
     175,   176,   519,     0,     0,     0,     0,   177,   178,   179,
     180,   181,   182,   183,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   192,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   201,
     202,   203,   204,     0,   205,   206,   207,     0,   208,     0,
     209,   520,     0,   521,   210,   211,     0,     0,   212,     0,
     213,   214,   522,   215,   216,   217,   523,   218,   219,   220,
     221,     0,   524,   222,   223,   224,   225,   226,   227,   228,
       0,   229,     0,   230,   525,   526,   231,   527,   232,   233,
     234,   235,     0,   528,   236,   529,     0,   237,   238,   239,
       0,     0,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   530,   251,   358,   252,   253,   254,   255,
     256,     0,   257,   258,   531,     0,   259,   260,   261,     0,
       0,   262,   359,     0,   499,   263,   264,     0,     0,   265,
     532,   533,   266,   267,   268,   269,   270,   271,     0,   272,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   281,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   534,   292,   293,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,     0,   303,   304,   305,   306,   535,
     307,   308,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,     0,   319,   320,   321,     0,   322,   323,
       0,   324,     0,   325,   326,   327,   328,     0,   329,   330,
     331,   332,     0,     0,   333,   334,   335,   336,   337,     0,
       0,   338,   339,   340,   341,   342,   343,   536,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     537,     0,     0,     0,     0,     0,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,     0,
       0,  1814,   146,   147,   148,   516,   149,   150,   151,   517,
     152,   518,   153,   154,     0,   155,   156,   157,   158,     0,
       0,   159,   160,   161,   162,     0,   163,   164,   165,   166,
     167,     0,     0,   168,   169,   170,     0,   171,   172,   173,
     174,     0,   175,   176,   519,     0,     0,     0,     0,   177,
     178,   179,   180,   181,   182,   183,   184,     0,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   192,
     193,   194,     0,   195,   196,     0,   197,     0,   198,   199,
     200,   201,   202,   203,   204,     0,   205,   206,   207,     0,
     208,     0,   209,   520,     0,   521,   210,   211,     0,     0,
     212,     0,   213,   214,   522,   215,   216,   217,   523,   218,
     219,   220,   221,     0,   524,   222,   223,   224,   225,   226,
     227,   228,     0,   229,     0,   230,   525,   526,   231,   527,
     232,   233,   234,   235,     0,   528,   236,   529,     0,   237,
     238,   239,     0,     0,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   530,   251,   358,   252,   253,
     254,   255,   256,     0,   257,   258,   531,     0,   259,   260,
     261,     0,     0,   262,   359,     0,     0,   263,   264,     0,
       0,   265,   532,   533,   266,   267,   268,   269,   270,   271,
       0,   272,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   281,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,   534,   292,   293,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,     0,   303,   304,   305,
     306,   535,   307,   308,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,     0,   319,   320,   321,     0,
     322,   323,     0,   324,     0,   325,   326,   327,   328,     0,
     329,   330,   331,   332,     0,     0,   333,   334,   335,   336,
     337,     0,     0,   338,   339,   340,   341,   342,   343,   536,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   537,     0,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,     0,     0,     0,
     146,   147,   148,  2268,   149,   150,   151,     0,   152,     0,
     153,   154,     0,   155,   156,   157,   158,     0,     0,   159,
     160,   161,   162,     0,   163,   164,   165,   166,   167,     0,
       0,   168,   169,   170,     0,   171,   172,   173,   174,     0,
     175,   176,     0,     0,     0,     0,     0,   177,   178,   179,
     180,   181,   182,   183,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   192,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   201,
     202,   203,   204,     0,   205,   206,   207,     0,   208,     0,
     209,     0,     0,     0,   210,   211,     0,     0,   212,     0,
     213,   214,     0,   215,   216,   217,     0,   218,   219,   220,
     221,     0,     0,   222,   223,   224,   225,   226,   227,   228,
       0,   229,     0,   230,     0,     0,   231,     0,   232,   233,
     234,   235,     0,     0,   236,     0,     0,   237,   238,   239,
       0,     0,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,     0,   251,   358,   252,   253,   254,   255,
     256,     0,   257,   258,     0,     0,   259,   260,   261,     0,
       0,   262,   359,     0,     0,   263,   264,     0,     0,   265,
       0,     0,   266,   267,   268,   269,   270,   271,     0,   272,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   281,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,     0,   292,   293,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,     0,   303,   304,   305,   306,     0,
     307,   308,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,     0,   319,   320,   321,     0,   322,   323,
       0,   324,     0,   325,   326,   327,   328,     0,   329,   330,
     331,   332,     0,     0,   333,   334,   335,   336,   337,     0,
       0,   338,   339,   340,   341,   342,   343,     0,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     353,     0,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,     0,     0,     0,   146,   147,
     148,   784,   149,   150,   151,     0,   152,     0,   153,   154,
       0,   155,   156,   157,   158,     0,     0,   159,   160,   161,
     162,     0,   163,   164,   165,   166,   167,     0,     0,   168,
     169,   170,     0,   171,   172,   173,   174,     0,   175,   176,
       0,     0,     0,     0,     0,   177,   178,   179,   180,   181,
     182,   183,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   192,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,   201,   202,   203,
     204,     0,   205,   206,   207,     0,   208,     0,   209,     0,
       0,     0,   210,   211,     0,     0,   212,     0,   213,   214,
       0,   215,   216,   217,     0,   218,   219,   220,   221,     0,
       0,   222,   223,   224,   225,   226,   227,   228,     0,   229,
       0,   230,     0,     0,   231,     0,   232,   233,   234,   235,
       0,     0,   236,     0,     0,   237,   238,   239,     0,     0,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,     0,   251,     0,   252,   253,   254,   255,   256,     0,
     257,   258,     0,     0,   259,   260,   261,     0,     0,   262,
       0,     0,     0,   263,   264,     0,     0,   265,     0,     0,
     266,   267,   268,   269,   270,   271,     0,   272,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   281,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,     0,
     292,   293,   294,   295,   296,   297,   298,   299,    33,   300,
     301,   302,     0,   303,   304,   305,   306,     0,   307,   308,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,     0,   319,   320,   321,     0,   322,   323,     0,   324,
       0,   325,   326,   327,   328,     0,   329,   330,   331,   332,
       0,     0,   333,   334,   335,   336,   337,     0,     0,   338,
     339,   340,   341,   342,   343,     0,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   353,     0,
     138,   139,   140,   141,   142,   143,   144,     0,   145,     0,
       0,     0,     0,     0,     0,     0,   146,   147,   148,    41,
     149,   150,   151,     0,   152,     0,   153,   154,     0,   155,
     156,   157,   158,     0,     0,   159,   160,   161,   162,     0,
     163,   164,   165,   166,   167,     0,     0,   168,   169,   170,
       0,   171,   172,   173,   174,     0,   175,   176,     0,     0,
       0,     0,     0,   177,   178,   179,   180,   181,   182,   183,
     184,     0,   185,     0,   186,   187,   188,   189,   190,     0,
       0,     0,   191,   192,   193,   194,     0,   195,   196,     0,
     197,     0,   198,   199,   200,   201,   202,   203,   204,     0,
     205,   206,   207,     0,   208,     0,   209,     0,     0,     0,
     210,   211,     0,     0,   212,     0,   213,   214,     0,   215,
     216,   217,     0,   218,   219,   220,   221,     0,     0,   222,
     223,   224,   225,   226,   227,   228,     0,   229,     0,   230,
       0,     0,   231,     0,   232,   233,   234,   235,     0,     0,
     236,     0,     0,   237,   238,   239,     0,     0,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,     0,
     251,     0,   252,   253,   254,   255,   256,     0,   257,   258,
       0,     0,   259,   260,   261,     0,     0,   262,     0,     0,
       0,   263,   264,     0,     0,   265,     0,     0,   266,   267,
     268,   269,   270,   271,     0,   272,   273,   274,   275,     0,
     276,   277,   278,   279,   280,   281,   282,     0,   283,   284,
     285,   286,   287,   288,   289,   290,   291,     0,   292,   293,
     294,   295,   296,   297,   298,   299,     0,   300,   301,   302,
       0,   303,   304,   305,   306,     0,   307,   308,     0,   309,
     310,   311,   312,   313,   314,   315,   316,   317,   318,     0,
     319,   320,   321,     0,   322,   323,     0,   324,     0,   325,
     326,   327,   328,     0,   329,   330,   331,   332,     0,     0,
     333,   334,   335,   336,   337,     0,     0,   338,   339,   340,
     341,   342,   343,     0,   344,   345,   346,     0,     0,   347,
     348,   349,   350,   351,   352,     0,   353,     0,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,     0,     0,     0,   146,   147,   148,  2214,   149,   150,
     151,     0,   152,     0,   153,   154,     0,   155,   156,   157,
     158,     0,     0,   159,   160,   161,   162,     0,   163,   592,
     165,   166,   167,     0,     0,   168,   169,   170,     0,   171,
     172,   173,   174,     0,   175,   176,     0,     0,     0,     0,
       0,   177,   178,   179,   180,   181,   182,   183,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   192,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,   201,   202,   203,   204,     0,   205,   206,
     207,     0,   208,     0,   209,     0,     0,     0,   210,   211,
       0,     0,   212,     0,   213,   214,     0,   215,   216,   217,
       0,   218,   219,   220,   221,     0,     0,   222,   223,   224,
     225,   226,   227,   228,     0,   229,     0,   230,     0,     0,
     231,     0,   232,   233,   234,   235,     0,     0,   236,     0,
       0,   237,   238,   239,     0,     0,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,     0,   251,     0,
     252,   253,   254,   255,   256,     0,   257,   258,     0,     0,
     259,   260,   261,     0,     0,   262,     0,     0,     0,   263,
     264,     0,     0,   265,     0,     0,   266,   267,   268,   269,
     270,   271,     0,   272,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   281,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,     0,   292,   293,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,     0,   303,
     304,   305,   306,     0,   307,   308,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,     0,   319,   320,
     321,     0,   322,   323,     0,   324,     0,   325,   326,   327,
     328,     0,   329,   330,   331,   332,     0,     0,   333,   334,
     335,   336,   337,     0,     0,   338,   339,   340,   341,   342,
     343,     0,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   353,     0,     0,     0,     0,   593,
       0,     0,   594,   595,   596,     0,   597,   598,   599,   600,
     601,   602,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,     0,     0,     0,   146,   147,
     148,     0,   149,   150,   151,     0,   152,     0,   153,   154,
       0,   155,   156,   157,   158,     0,     0,   159,   160,   161,
     162,     0,   163,   630,   165,   166,   167,     0,     0,   168,
     169,   170,     0,   171,   172,   173,   174,     0,   175,   176,
       0,     0,     0,     0,     0,   177,   178,   179,   180,   181,
     182,   183,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   192,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,   201,   202,   203,
     204,     0,   205,   206,   207,     0,   208,     0,   209,     0,
       0,     0,   210,   211,     0,     0,   212,     0,   213,   214,
       0,   215,   216,   217,     0,   218,   219,   220,   221,     0,
       0,   222,   223,   224,   225,   226,   227,   228,     0,   229,
       0,   230,     0,     0,   231,     0,   232,   233,   234,   235,
       0,     0,   236,     0,     0,   237,   238,   239,     0,     0,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,     0,   251,     0,   252,   253,   254,   255,   256,     0,
     257,   258,     0,     0,   259,   260,   261,     0,     0,   262,
       0,     0,     0,   263,   264,     0,     0,   265,     0,     0,
     266,   267,   268,   269,   270,   271,     0,   272,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   281,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,     0,
     292,   293,   294,   295,   296,   297,   298,   299,     0,   300,
     301,   302,     0,   303,   304,   305,   306,     0,   307,   308,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,     0,   319,   320,   321,     0,   322,   323,     0,   324,
       0,   325,   326,   327,   328,     0,   329,   330,   331,   332,
       0,     0,   333,   334,   335,   336,   337,     0,     0,   338,
     339,   340,   341,   342,   343,     0,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   353,     0,
       0,     0,     0,   593,     0,     0,   594,   595,   596,     0,
     597,   598,   599,   600,   601,   602,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,     0,
       0,     0,   146,   147,   148,     0,   149,   150,   151,     0,
     152,     0,   153,   154,     0,   155,   156,   157,   158,     0,
       0,   159,   160,   161,   162,     0,   163,   164,   165,   166,
     167,     0,     0,   168,   169,   170,     0,   171,   172,   173,
     174,     0,   175,   176,     0,     0,     0,     0,     0,   177,
     178,   179,   180,   181,   182,   183,   184,     0,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   192,
     193,   194,     0,   195,   196,     0,   197,     0,   198,   199,
     200,   201,   202,   203,   204,     0,   205,   206,   207,     0,
     208,     0,   209,     0,     0,     0,   210,   211,     0,     0,
     212,     0,   213,   214,     0,   215,   216,   217,     0,   218,
     219,   220,   221,     0,     0,   222,   223,   224,   225,   226,
     227,   228,     0,   229,     0,   230,     0,     0,   231,     0,
     232,   233,   234,   235,     0,     0,   236,     0,     0,   237,
     238,   239,     0,     0,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,     0,   251,     0,   252,   253,
     254,   255,   256,     0,   257,   258,     0,     0,   259,   260,
     261,     0,     0,   262,     0,     0,     0,   263,   264,     0,
       0,   265,     0,     0,   266,   267,   268,   269,   270,   271,
       0,   272,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   281,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,     0,   292,   293,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,     0,   303,   304,   305,
     306,     0,   307,   308,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,     0,   319,   320,   321,     0,
     322,   323,     0,   324,     0,   325,   326,   327,   328,     0,
     329,   330,   331,   332,     0,     0,   333,   334,   335,   336,
     337,     0,     0,   338,   339,   340,   341,   342,   343,     0,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   353,     0,     0,     0,     0,   593,     0,     0,
     594,   595,   596,     0,   597,   598,   599,   600,   601,   602,
     138,   139,   140,   141,   142,   143,   144,     0,   145,     0,
       0,     0,     0,     0,     0,     0,   146,   147,   148,     0,
     149,   150,   151,     0,   855,     0,   856,   857,     0,   155,
     156,   157,   158,     0,     0,   159,   858,   859,   162,     0,
     163,   164,   165,   166,     0,     0,     0,   168,   169,   170,
       0,   171,   172,     0,   174,     0,   175,   176,     0,     0,
       0,     0,     0,   177,   178,   179,   180,   181,   860,   861,
     184,     0,   185,     0,   186,   187,   188,   189,   190,     0,
       0,     0,   191,   685,   193,   194,     0,   195,   196,     0,
     197,     0,   198,   199,   200,     0,   202,   203,     0,     0,
     205,   206,   862,     0,   208,     0,   209,     0,     0,     0,
     210,   211,     0,     0,   212,     0,   213,   214,     0,   215,
     216,   217,     0,   218,   219,   220,   221,     0,     0,   222,
     223,   224,   225,   226,   863,   864,     0,   865,     0,   230,
       0,     0,   231,     0,   232,   233,   234,   235,     0,     0,
     236,     0,     0,   237,   238,   239,     0,     0,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   695,     0,
     866,     0,   252,   253,   254,   255,     0,     0,   257,   258,
       0,     0,     0,   867,   261,     0,     0,   262,     0,     0,
       0,  1885,   264,     0,     0,   265,     0,     0,     0,   267,
     268,   269,   270,   271,     0,     0,   273,   274,   275,     0,
     276,   277,   278,   279,   280,   868,   282,     0,   283,   284,
     285,   286,   287,   288,   289,   290,   291,     0,   292,     0,
     294,   295,   296,   297,   298,   299,     0,   300,   301,   302,
       0,   303,   869,   305,   306,     0,   307,   870,     0,   309,
     310,   311,   312,   313,   314,   315,   316,     0,   318,     0,
     319,   320,   321,     0,   871,   872,     0,   324,     0,   325,
       0,   327,     0,     0,   329,   330,   331,   332,     0,     0,
     333,   334,   335,   336,   337,     0,     0,   338,   339,   340,
     341,   873,   343,     0,   344,   345,   346,     0,     0,   347,
     348,   349,   350,   351,   352,     0,   874,  1088,   458,     0,
       0,   593,   433,     0,   594,   595,   596,     0,   597,  1886,
     599,   600,   601,   602,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,     0,     0,     0,
     146,   147,   148,     0,   149,   150,   151,     0,   152,     0,
     153,   154,     0,   155,   156,   157,   158,     0,     0,   159,
     160,   161,   162,     0,   163,   164,   165,   166,   167,     0,
       0,   168,   169,   170,     0,   171,   172,   173,   174,     0,
     175,   176,     0,     0,     0,     0,     0,   177,   178,   179,
     180,   181,   182,   183,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   192,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   201,
     202,   203,   204,     0,   205,   206,   207,     0,   208,     0,
     209,     0,     0,     0,   210,   211,     0,     0,   212,     0,
     213,   214,     0,   215,   216,   217,     0,   218,   219,   220,
     221,     0,     0,   222,   223,   224,   225,   226,   227,   228,
       0,   229,     0,   230,     0,     0,   231,     0,   232,   233,
     234,   235,     0,     0,   236,     0,     0,   237,   238,   239,
       0,     0,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,     0,   251,   358,   252,   253,   254,   255,
     256,     0,   257,   258,     0,     0,   259,   260,   261,     0,
       0,   262,   359,     0,     0,   263,   264,     0,     0,   265,
       0,     0,   266,   267,   268,   269,   270,   271,     0,   272,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   281,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,     0,   292,   293,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,     0,   303,   304,   305,   306,     0,
     307,   308,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,     0,   319,   320,   321,     0,   322,   323,
       0,   324,     0,   325,   326,   327,   328,     0,   329,   330,
     331,   332,     0,     0,   333,   334,   335,   336,   337,     0,
       0,   338,   339,   340,   341,   342,   343,     0,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     353,   138,   139,   140,   141,   142,   143,   144,     0,   145,
       0,     0,     0,     0,   497,     0,     0,   146,   147,   148,
       0,   149,   150,   151,     0,   152,     0,   153,   154,     0,
     155,   156,   157,   158,     0,     0,   159,   160,   161,   162,
       0,   163,   164,   165,   166,   167,     0,     0,   168,   169,
     170,     0,   171,   172,   173,   174,     0,   175,   176,     0,
       0,     0,     0,     0,   177,   178,   179,   180,   181,   182,
     183,   184,     0,   185,     0,   186,   187,   188,   189,   190,
       0,     0,     0,   191,   192,   193,   194,     0,   195,   196,
       0,   197,     0,   198,   199,   200,   201,   202,   203,   204,
       0,   205,   206,   207,     0,   208,     0,   209,     0,     0,
       0,   210,   211,     0,     0,   212,     0,   213,   214,     0,
     215,   216,   217,     0,   218,   219,   220,   221,     0,     0,
     222,   223,   224,   225,   226,   227,   228,     0,   229,     0,
     230,     0,     0,   231,     0,   232,   233,   234,   235,     0,
       0,   236,     0,     0,   237,   238,   239,     0,     0,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
       0,   251,     0,   252,   253,   254,   255,   256,     0,   257,
     258,     0,     0,   259,   260,   261,     0,     0,   262,     0,
       0,     0,   263,   264,     0,     0,   265,     0,     0,   266,
     267,   268,   269,   270,   271,     0,   272,   273,   274,   275,
       0,   276,   277,   278,   279,   280,   281,   282,     0,   283,
     284,   285,   286,   287,   288,   289,   290,   291,     0,   292,
     293,   294,   295,   296,   297,   298,   299,     0,   300,   301,
     302,     0,   303,   304,   305,   306,     0,   307,   308,     0,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
       0,   319,   320,   321,     0,   322,   323,     0,   324,     0,
     325,   326,   327,   328,     0,   329,   330,   331,   332,     0,
       0,   333,   334,   335,   336,   337,     0,     0,   338,   339,
     340,   341,   342,   343,     0,   344,   345,   346,     0,     0,
     347,   348,   349,   350,   351,   352,     0,   353,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,   839,     0,     0,   146,   147,   148,     0,   149,   150,
     151,     0,   855,     0,   856,   857,     0,   155,   156,   157,
     158,     0,     0,   159,   858,   859,   162,     0,   163,   164,
     165,   166,     0,     0,     0,   168,   169,   170,     0,   171,
     172,     0,   174,     0,   175,   176,     0,     0,     0,     0,
       0,   177,   178,   179,   180,   181,   860,   861,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   685,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,     0,   202,   203,     0,     0,   205,   206,
     862,     0,   208,     0,   209,     0,     0,     0,   210,   211,
       0,     0,   212,     0,   213,   214,     0,   215,   216,   217,
       0,   218,   219,   220,   221,     0,     0,   222,   223,   224,
     225,   226,   863,   864,     0,   865,     0,   230,     0,     0,
     231,     0,   232,   233,   234,   235,     0,     0,   236,     0,
       0,   237,   238,   239,     0,     0,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   695,     0,   866,     0,
     252,   253,   254,   255,     0,     0,   257,   258,     0,     0,
       0,   867,   261,     0,     0,   262,     0,     0,     0,   263,
     264,     0,     0,   265,     0,     0,     0,   267,   268,   269,
     270,   271,     0,     0,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   868,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,     0,   292,     0,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,     0,   303,
     869,   305,   306,     0,   307,   870,     0,   309,   310,   311,
     312,   313,   314,   315,   316,     0,   318,     0,   319,   320,
     321,     0,   871,   872,     0,   324,     0,   325,     0,   327,
       0,     0,   329,   330,   331,   332,     0,     0,   333,   334,
     335,   336,   337,     0,     0,   338,   339,   340,   341,   873,
     343,     0,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   874,   138,   139,   140,   141,   142,
     143,   144,     0,   145,     0,     0,     0,     0,  1140,     0,
       0,   146,   147,   148,     0,   149,   150,   151,     0,   152,
       0,   153,   154,     0,   155,   156,   157,   158,     0,     0,
     159,   160,   161,   162,     0,   163,   164,   165,   166,   167,
       0,     0,   168,   169,   170,     0,   171,   172,   173,   174,
       0,   175,   176,     0,     0,     0,     0,     0,   177,   178,
     179,   180,   181,   182,   183,   184,  1101,   185,     0,   186,
     187,   188,   189,   190,     0,     0,     0,   191,   192,   193,
     194,     0,   195,   196,     0,   197,     0,   198,   199,   200,
     201,   202,   203,   204,  1102,   205,   206,   207,     0,   208,
       0,   209,     0,     0,     0,   210,   211,     0,     0,   212,
       0,   213,   214,     0,   215,   216,   217,     0,   218,   219,
     220,   221,     0,     0,   222,   223,   224,   225,   226,   227,
     228,     0,   229,     0,   230,     0,     0,   231,     0,   232,
     233,   234,   235,     0,     0,   236,     0,     0,   237,   238,
     239,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,     0,   251,     0,   252,   253,   254,
     255,   256,     0,   257,   258,     0,     0,   259,   260,   261,
    1103,     0,   262,     0,  1104,     0,   263,   264,     0,     0,
     265,     0,     0,   266,   267,   268,   269,   270,   271,     0,
     272,   273,   274,   275,     0,   276,   277,   278,   279,   280,
     281,   282,     0,   283,   284,   285,   286,   287,   288,   289,
     290,   291,     0,   292,   293,   294,   295,   296,   297,   298,
     299,     0,   300,   301,   302,     0,   303,   304,   305,   306,
       0,   307,   308,     0,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,     0,   319,   320,   321,     0,   322,
     323,     0,   324,     0,   325,   326,   327,   328,  1105,   329,
     330,   331,   332,     0,     0,   333,   334,   335,   336,   337,
       0,     0,   338,   339,   340,   341,   342,   343,     0,   344,
     345,   346,     0,     0,   347,   348,   349,   350,   351,   352,
       0,   353,  1088,   932,     0,     0,     0,   433,     0,     0,
       0,     0,     0,     0,  1089,   138,   139,   140,   141,   142,
     143,   144,     0,   145,     0,     0,     0,     0,     0,     0,
       0,   146,   147,   148,     0,   149,   150,   151,     0,   152,
       0,   153,   154,     0,   155,   156,   157,   158,     0,     0,
     159,   160,   161,   162,     0,   163,   164,   165,   166,   167,
       0,     0,   168,   169,   170,     0,   171,   172,   173,   174,
       0,   175,   176,     0,     0,     0,     0,     0,   177,   178,
     179,   180,   181,   182,   183,   184,     0,   185,     0,   186,
     187,   188,   189,   190,     0,     0,     0,   191,   192,   193,
     194,     0,   195,   196,     0,   197,     0,   198,   199,   200,
     201,   202,   203,   204,  1102,   205,   206,   207,     0,   208,
       0,   209,     0,     0,     0,   210,   211,     0,     0,   212,
       0,   213,   214,     0,   215,   216,   217,     0,   218,   219,
     220,   221,     0,     0,   222,   223,   224,   225,   226,   227,
     228,     0,   229,     0,   230,     0,     0,   231,     0,   232,
     233,   234,   235,     0,     0,   236,     0,     0,   237,   238,
     239,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,     0,   251,     0,   252,   253,   254,
     255,   256,     0,   257,   258,     0,     0,   259,   260,   261,
    1103,     0,   262,     0,  1104,     0,   263,   264,     0,     0,
     265,     0,     0,   266,   267,   268,   269,   270,   271,     0,
     272,   273,   274,   275,     0,   276,   277,   278,   279,   280,
     281,   282,     0,   283,   284,   285,   286,   287,   288,   289,
     290,   291,     0,   292,   293,   294,   295,   296,   297,   298,
     299,     0,   300,   301,   302,     0,   303,   304,   305,   306,
       0,   307,   308,     0,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,     0,   319,   320,   321,     0,   322,
     323,     0,   324,     0,   325,   326,   327,   328,  1105,   329,
     330,   331,   332,     0,     0,   333,   334,   335,   336,   337,
       0,     0,   338,   339,   340,   341,   342,   343,     0,   344,
     345,   346,     0,     0,   347,   348,   349,   350,   351,   352,
       0,   353,  1088,   932,     0,     0,     0,   433,     0,     0,
       0,     0,     0,     0,  1089,   138,   424,   140,   141,   142,
     143,   144,   425,   145,     0,     0,     0,     0,     0,     0,
       0,   146,   147,   148,     0,   426,   150,   151,     0,   152,
       0,   153,   154,     0,   155,   156,   157,   158,     0,     0,
     159,   160,   161,   162,     0,   163,   164,   165,   166,   167,
       0,     0,   168,   169,   170,     0,   171,   172,   173,   174,
       0,   175,   176,     0,     0,     0,     0,     0,   177,   178,
     179,   180,   181,   182,   183,   184,     0,   185,     0,   186,
     187,   188,   189,   190,     0,     0,     0,   191,   192,   193,
     194,     0,   195,   196,     0,   197,     0,   198,   199,   200,
     201,   202,   203,   204,     0,   205,   427,   207,     0,   208,
       0,   428,     0,     0,     0,   210,   211,     0,     0,   212,
       0,   213,   214,     0,   215,   216,   217,     0,   218,   219,
     220,   221,     0,     0,   222,   223,   224,   225,   226,   227,
     228,     0,   229,     0,   230,     0,     0,   231,     0,   232,
     233,   234,   429,     0,     0,   236,     0,     0,   237,   238,
     239,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,     0,   251,     0,   430,   253,   254,
     255,   256,     0,   257,   258,     0,     0,   259,   260,   261,
       0,     0,   262,     0,     0,     0,   263,   264,     0,     0,
     265,     0,     0,   266,   267,   268,   269,   270,   271,     0,
     272,   273,   274,   275,     0,   431,   277,   278,   279,   280,
     281,   282,     0,   283,   432,   285,   286,   287,   288,   289,
     290,   291,     0,   292,   293,   294,   295,   296,   297,   298,
     299,     0,   300,   301,   302,     0,   303,   304,   305,   306,
       0,   307,   308,     0,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,     0,   319,   320,   321,     0,   322,
     323,  1044,   324,     0,   325,   326,   327,   328,  1046,   329,
     330,   331,   332,  1047,     0,   333,   334,   335,   336,   337,
       0,  1044,   338,   339,   340,   341,   342,   343,  1046,   344,
     345,   346,     0,  1047,   347,   348,   349,   350,   351,   352,
       0,   353,     0,     0,     0,  1044,     0,   433,     0,     0,
       0,     0,  1046,     0,   434,     0,     0,  1047,     0,     0,
       0,     0,  1044,     0,     0,     0,     0,     0,     0,  1046,
       0,     0,     0,     0,  1047,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1044,     0,     0,     0,     0,  1772,     0,
    1046,     0,     0,     0,     0,  1047,     0,     0,  1048,     0,
       0,     0,  1049,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1048,     0,
    1050,  1051,  1049,     0,     0,     0,     0,     0,     0,     0,
       0,  1052,     0,     0,     0,     0,     0,     0,     0,     0,
    1050,  1051,  1048,     0,     0,     0,  1049,     0,     0,     0,
       0,  1052,     0,     0,     0,     0,     0,  1053,     0,  1048,
    1054,     0,     0,  1049,  1050,  1051,     0,     0,     0,     0,
       0,  1055,     0,  1056,     0,  1052,     0,  1053,     0,     0,
    1054,  1050,  1051,     0,     0,     0,     0,     0,     0,     0,
    1048,  1055,  1052,  1056,  1049,     0,     0,     0,     0,     0,
       0,  1053,     0,     0,  1054,     0,     0,     0,     0,     0,
       0,     0,  1050,  1051,     0,  1055,     0,  1056,  1053,     0,
       0,  1054,     0,  1052,     0,  1057,     0,     0,     0,     0,
       0,     0,  1055,     0,  1056,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1057,     0,     0,     0,  1053,
       0,     0,  1054,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1055,     0,  1056,     0,  1044,     0,  1057,
       0,     0,     0,     0,  1046,     0,     0,     0,     0,  1047,
       0,     0,     0,     0,     0,     0,  1057,  1977,     0,     0,
       0,  1058,     0,     0,  1059,  1060,  1061,     0,  1062,  1063,
    1064,  1065,  1066,  1067,     0,     0,     0,     0,     0,  1068,
       0,  1058,     0,     0,  1059,  1060,  1061,  1057,  1062,  1063,
    1064,  1065,  1066,  1067,  1044,     0,     0,     0,  1893,  1068,
       0,  1046,     0,     0,     0,  1058,  1047,     0,  1059,  1060,
    1061,     0,  1062,  1063,  1064,  1065,  1066,  1067,     0,     0,
       0,     0,  1058,  1068,     0,  1059,  1060,  1061,     0,  1062,
    1063,  1064,  1065,  1066,  1067,  1044,     0,     0,     0,  2072,
    1068,     0,  1046,     0,  1048,     0,     0,  1047,  1049,     0,
       0,     0,     0,  1058,     0,     0,  1059,  1060,  1061,     0,
    1062,  1063,  1064,  1065,  1066,  1067,  1050,  1051,     0,     0,
    2178,  1068,     0,     0,     0,     0,     0,  1052,     0,     0,
       0,     0,     0,     0,  1044,     0,     0,     0,     0,     0,
       0,  1046,     0,     0,     0,     0,  1047,     0,     0,     0,
       0,  1048,     0,  1053,     0,  1049,  1054,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1055,     0,  1056,
       0,     0,     0,  1050,  1051,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1052,     0,     0,     0,     0,     0,
       0,     0,  1048,     0,     0,  1044,  1049,     0,     0,     0,
       0,     0,  1046,     0,     0,     0,     0,  1047,     0,     0,
    1053,     0,     0,  1054,  1050,  1051,     0,     0,     0,     0,
       0,  1057,     0,     0,  1055,  1052,  1056,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1048,     0,     0,     0,  1049,     0,     0,     0,     0,
       0,  1053,     0,     0,  1054,     0,     0,     0,     0,     0,
       0,     0,     0,  1050,  1051,  1055,     0,  1056,     0,     0,
       0,     0,     0,     0,  1052,     0,     0,     0,  1057,     0,
       0,     0,     0,     0,     0,     0,     0,  1058,     0,     0,
    1059,  1060,  1061,     0,  1062,  1063,  1064,  1065,  1066,  1067,
    1053,     0,  1048,  1054,  2195,  1068,  1049,     0,     0,     0,
       0,     0,     0,     0,  1055,     0,  1056,     0,     0,  1057,
       0,     0,     0,     0,  1050,  1051,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1052,     0,     0,     0,     0,
       0,     0,     0,     0,  1058,     0,     0,  1059,  1060,  1061,
       0,  1062,  1063,  1064,  1065,  1066,  1067,     0,     0,  2292,
       0,  1053,  1068,     0,  1054,     0,     0,     0,  1057,     0,
       0,     0,     0,     0,     0,  1055,     0,  1056,     0,     0,
       0,     0,     0,     0,     0,  1058,     0,     0,  1059,  1060,
    1061,     0,  1062,  1063,  1064,  1065,  1066,  1067,     0,     0,
       0,     0,  2322,  1068,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1057,
       0,     0,     0,     0,  1058,     0,     0,  1059,  1060,  1061,
       0,  1062,  1063,  1064,  1065,  1066,  1067,     0,     0,     0,
       0,  2386,  1068,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1058,     0,     0,  1059,  1060,
    1061,     0,  1062,  1063,  1064,  1065,  1066,  1067,     0,     0,
       0,     0,     0,  1068,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,     0,     0,     0,
     146,   147,   148,     0,   149,   150,   151,     0,   152,     0,
     153,   154,     0,   155,   156,   157,   158,     0,     0,   159,
     160,   161,   162,     0,   163,   164,   165,   166,   167,     0,
       0,   168,   169,   170,     0,   171,   172,   173,   174,     0,
     175,   176,     0,     0,     0,     0,     0,   177,   178,   179,
     180,   181,   182,   183,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   192,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   201,
     202,   203,   204,     0,   205,   206,   207,     0,   208,     0,
     209,     0,     0,     0,   210,   211,     0,     0,   212,     0,
     213,   214,     0,   215,   216,   217,     0,   218,   219,   220,
     221,     0,     0,   222,   223,   224,   225,   226,   227,   228,
       0,   229,     0,   230,     0,     0,   231,     0,   232,   233,
     234,   235,     0,     0,   236,     0,     0,   237,   238,   239,
       0,     0,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,     0,   251,     0,   252,   253,   254,   255,
     256,     0,   257,   258,     0,     0,   259,   260,   261,     0,
       0,   262,     0,     0,     0,   263,   264,     0,     0,   265,
       0,     0,   266,   267,   268,   269,   270,   271,     0,   272,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   281,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,     0,   292,   293,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,     0,   303,   304,   305,   306,     0,
     307,   308,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,     0,   319,   320,   321,  1046,   322,   323,
       0,   324,  1047,   325,   326,   327,   328,     0,   329,   330,
     331,   332,     0,     0,   333,   334,   335,   336,   337,     0,
       0,   338,   339,   340,   341,   342,   343,     0,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     353,  2437,   458,  2438,  2439,     0,  2440,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  2193,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0, -1316,     0,     0,
       0,  1049,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1050,
    1051,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   -1316,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1054,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1055,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0, -1316,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1058,     0,     0,     0,     0,     0,     0,  1062,  1063,  1064,
    1065,  1066,  1067,     0,     0,     0,     0,     0,  1068,   138,
     139,   140,   141,   142,   143,   144,     0,   145,     0,     0,
       0,     0,     0,     0,     0,   146,   147,   148,     0,   149,
     150,   151,     0,   152,     0,   153,   154,     0,   155,   156,
     157,   158,     0,     0,   159,   160,   161,   162,     0,   163,
     164,   165,   166,   167,     0,     0,   168,   169,   170,     0,
     171,   172,   173,   174,     0,   175,   176,     0,     0,     0,
       0,     0,   177,   178,   179,   180,   181,   182,   183,   184,
    1082,   185,     0,   186,   187,   188,   189,   190,     0,     0,
       0,   191,   192,   193,   194,     0,   195,   196,     0,   197,
       0,   198,   199,   200,   201,   202,   203,   204,     0,   205,
     206,   207,     0,   208,     0,   209,     0,     0,     0,   210,
     211,     0,     0,   212,     0,   213,   214,     0,   215,   216,
     217,     0,   218,   219,   220,   221,     0,     0,   222,   223,
     224,   225,   226,   227,   228,     0,   229,     0,   230,     0,
       0,   231,     0,   232,   233,   234,   235,     0,     0,   236,
       0,     0,   237,   238,   239,     0,     0,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,     0,   251,
       0,   252,   253,   254,   255,   256,     0,   257,   258,     0,
       0,   259,   260,   261,     0,     0,   262,     0,     0,     0,
     263,   264,     0,     0,   265,     0,     0,   266,   267,   268,
     269,   270,   271,     0,   272,   273,   274,   275,     0,   276,
     277,   278,   279,   280,   281,   282,     0,   283,   284,   285,
     286,   287,   288,   289,   290,   291,     0,   292,   293,   294,
     295,   296,   297,   298,   299,     0,   300,   301,   302,     0,
     303,   304,   305,   306,     0,   307,   308,     0,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   318,     0,   319,
     320,   321,     0,   322,   323,     0,   324,     0,   325,   326,
     327,   328,     0,   329,   330,   331,   332,     0,     0,   333,
     334,   335,   336,   337,     0,     0,   338,   339,   340,   341,
     342,   343,     0,   344,   345,   346,     0,     0,   347,   348,
     349,   350,   351,   352,     0,   353,     0,   932,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,     0,     0,     0,   146,   147,   148,     0,   149,   150,
     151,     0,   152,     0,   153,   154,     0,   155,   156,   157,
     158,     0,     0,   159,   160,   161,   162,     0,   163,   164,
     165,   166,   167,     0,     0,   168,   169,   170,     0,   171,
     172,   173,   174,     0,   175,   176,     0,     0,     0,     0,
       0,   177,   178,   179,   180,   181,   182,   183,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   192,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,   201,   202,   203,   204,     0,   205,   206,
     207,     0,   208,     0,   209,     0,     0,     0,   210,   211,
       0,     0,   212,     0,   213,   214,     0,   215,   216,   217,
       0,   218,   219,   220,   221,     0,     0,   222,   223,   224,
     225,   226,   227,   228,     0,   229,     0,   230,     0,     0,
     231,     0,   232,   233,   234,   235,     0,     0,   236,     0,
       0,   237,   238,   239,     0,     0,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,     0,   251,     0,
     252,   253,   254,   255,   256,     0,   257,   258,     0,     0,
     259,   260,   261,     0,     0,   262,     0,     0,     0,   263,
     264,     0,     0,   265,     0,     0,   266,   267,   268,   269,
     270,   271,     0,   272,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   281,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,     0,   292,   293,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,     0,   303,
     304,   305,   306,     0,   307,   308,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,     0,   319,   320,
     321,     0,   322,   323,     0,   324,     0,   325,   326,   327,
     328,     0,   329,   330,   331,   332,     0,     0,   333,   334,
     335,   336,   337,     0,     0,   338,   339,   340,   341,   342,
     343,     0,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   353,     0,   932,   138,   139,   140,
     141,   142,   143,   144,  1213,   145,  1214,  1215,  1216,  1217,
    1218,  1219,  1220,   146,   147,   148,   516,   149,   150,   151,
     517,   152,   518,   153,   154,  1221,   155,   156,   157,   158,
    1222,  1223,   159,   160,   161,   162,  1224,   163,   164,   165,
     166,   167,  1225,  1226,   168,   169,   170,  1227,   171,   172,
     173,   174,  1228,   175,   176,   519,  1229,  1230,  1231,  1232,
     177,   178,   179,   180,   181,   182,   183,   184,  1233,   185,
    1234,   186,   187,   188,   189,   190,  1235,  1236,  1237,   191,
     192,   193,   194,  1238,   195,   196,  1239,   197,  1240,   198,
     199,   200,   201,   202,   203,   204,  1241,   205,   206,   207,
    1242,   208,  1243,   209,   520,  1244,   521,   210,   211,  1245,
    1246,   212,  1247,   213,   214,   522,   215,   216,   217,   523,
     218,   219,   220,   221,  1248,   524,   222,   223,   224,   225,
     226,   227,   228,  1249,   229,  1250,   230,   525,   526,   231,
     527,   232,   233,   234,   235,  1251,   528,   236,   529,  1252,
     237,   238,   239,  1253,  1254,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   530,   251,  1255,   252,
     253,   254,   255,   256,  1256,   257,   258,   531,  1257,   259,
     260,   261,  1258,  1259,   262,  1260,  1261,  1262,   263,   264,
    1263,  1264,   265,   532,   533,   266,   267,   268,   269,   270,
     271,  1265,   272,   273,   274,   275,  1266,   276,   277,   278,
     279,   280,   281,   282,  1267,   283,   284,   285,   286,   287,
     288,   289,   290,   291,   534,   292,   293,   294,   295,   296,
     297,   298,   299,  1268,   300,   301,   302,  1269,   303,   304,
     305,   306,   535,   307,   308,  1270,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,  1271,   319,   320,   321,
    1272,   322,   323,  1273,   324,  1274,   325,   326,   327,   328,
    1275,   329,   330,   331,   332,  1276,  1277,   333,   334,   335,
     336,   337,  1278,  1279,   338,   339,   340,   341,   342,   343,
     536,   344,   345,   346,  1280,  1281,   347,   348,   349,   350,
     351,   352,     0,  1282,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,     0,     0,     0,
     146,   147,   148,   516,   149,   150,   151,   517,   152,   518,
     153,   154,     0,   155,   156,   157,   158,     0,     0,   159,
     160,   161,   162,     0,   163,   164,   165,   166,   167,     0,
       0,   168,   169,   170,     0,   171,   172,   173,   174,     0,
     175,   176,   519,     0,     0,     0,     0,   177,   178,   179,
     180,   181,   182,   183,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   192,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   201,
     202,   203,   204,     0,   205,   206,   207,     0,   208,     0,
     209,   520,     0,   521,   210,   211,     0,     0,   212,     0,
     213,   214,   522,   215,   216,   217,   523,   218,   219,   220,
     221,     0,   524,   222,   223,   224,   225,   226,   227,   228,
       0,   229,     0,   230,   525,   526,   231,   527,   232,   233,
     234,   235,     0,   528,   236,   529,     0,   237,   238,   239,
       0,     0,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,   530,   251,   358,   252,   253,   254,   255,
     256,     0,   257,   258,   531,     0,   259,   260,   261,     0,
       0,   262,   359,     0,     0,   263,   264,     0,     0,   265,
     532,   533,   266,   267,   268,   269,   270,   271,     0,   272,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   281,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,   534,   292,   293,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,     0,   303,   304,   305,   306,   535,
     307,   308,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,     0,   319,   320,   321,     0,   322,   323,
       0,   324,     0,   325,   326,   327,   328,     0,   329,   330,
     331,   332,     0,     0,   333,   334,   335,   336,   337,     0,
       0,   338,   339,   340,   341,   342,   343,   536,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     537,   138,   139,   140,   141,   142,   143,   144,     0,   145,
       0,     0,     0,     0,     0,     0,     0,   146,   147,   148,
       0,   149,   150,   151,     0,   152,     0,   153,   154,     0,
     155,   156,   157,   158,     0,     0,   159,   160,   161,   162,
    1146,   163,   164,   165,   166,   167,     0,     0,   168,   169,
     170,  1147,   171,   172,   173,   174,     0,   175,   176,     0,
       0,     0,     0,     0,   177,   178,   179,   180,   181,   182,
     183,   184,     0,   185,     0,   186,   187,   188,   189,   190,
       0,     0,     0,   191,   192,   193,   194,     0,   195,   196,
       0,   197,     0,   198,   199,   200,   201,   202,   203,   204,
       0,   205,   206,   207,     0,   208,  1148,   209,     0,     0,
       0,   210,   211,     0,     0,   212,     0,   213,   214,     0,
     215,   216,   217,     0,   218,   219,   220,   221,     0,     0,
     222,   223,   224,   225,   226,   227,   228,     0,   229,     0,
     230,     0,     0,   231,     0,   232,   233,   234,   235,     0,
       0,   236,  1654,     0,   237,   238,   239,     0,     0,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
       0,   251,     0,   252,   253,   254,   255,   256,     0,   257,
     258,     0,     0,   259,   260,   261,     0,     0,   262,     0,
       0,     0,   263,   264,     0,     0,   265,     0,     0,   266,
     267,   268,   269,   270,   271,     0,   272,   273,   274,   275,
    1149,   276,   277,   278,   279,   280,   281,   282,     0,   283,
     284,   285,   286,   287,   288,   289,   290,   291,     0,   292,
     293,   294,   295,   296,   297,   298,   299,     0,   300,   301,
     302,     0,   303,   304,   305,   306,     0,   307,   308,     0,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
       0,   319,   320,   321,     0,   322,   323,     0,   324,     0,
     325,   326,   327,   328,     0,   329,   330,   331,   332,     0,
    1150,   333,   334,   335,   336,   337,     0,     0,   338,   339,
     340,   341,   342,   343,     0,   344,   345,   346,     0,     0,
     347,   348,   349,   350,   351,   352,     0,   353,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     3,     4,     0,
       0,     0,     0,     0,   146,   147,   148,     0,   149,   150,
     151,     0,   152,     0,   153,   154,     0,   155,   156,   157,
     158,     0,     0,   159,   160,   161,   162,     0,   163,   164,
     165,   166,   167,     0,     0,   168,   169,   170,     0,   171,
     172,   173,   174,     0,   175,   176,     0,     0,     0,     0,
       0,   177,   178,   179,   180,   181,   182,   183,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   192,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,   201,   202,   203,   204,     0,   205,   206,
     207,     0,   208,     0,   209,     0,     0,     0,   210,   211,
       0,     0,   212,     0,   213,   214,     0,   215,   216,   217,
       0,   218,   219,   220,   221,     0,     0,   222,   223,   224,
     225,   226,   227,   228,     0,   229,     0,   230,     0,     0,
     231,     0,   232,   233,   234,   235,     0,     0,   236,     0,
       0,   237,   238,   239,     0,     0,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,     0,   251,   358,
     252,   253,   254,   255,   256,     0,   257,   258,     0,     0,
     259,   260,   261,     0,     0,   262,   359,     0,     0,   263,
     264,     0,     0,   265,     0,     0,   266,   267,   268,   269,
     270,   271,     0,   272,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   281,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,     0,   292,   293,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,     0,   303,
     304,   305,   306,     0,   307,   308,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,     0,   319,   320,
     321,     0,   322,   323,     0,   324,     0,   325,   326,   327,
     328,     0,   329,   330,   331,   332,     0,     0,   333,   334,
     335,   336,   337,     0,     0,   338,   339,   340,   341,   342,
     343,     0,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   353,   138,   139,   140,   141,   142,
     143,   144,     0,   145,     0,     0,     0,     0,     0,     0,
       0,   146,   147,   148,     0,   149,   150,   151,     0,   152,
       0,   153,   154,     0,   155,   156,   157,   158,     0,     0,
     159,   160,   161,   162,     0,   163,   164,   165,   166,   167,
       0,     0,   168,   169,   170,     0,   171,   172,   173,   174,
       0,   175,   176,     0,     0,     0,     0,     0,   177,   178,
     179,   180,   181,   182,   183,   184,     0,   185,     0,   186,
     187,   188,   189,   190,     0,     0,     0,   191,   192,   193,
     194,     0,   195,   196,     0,   197,     0,   198,   199,   200,
     201,   202,   203,   204,     0,   205,   206,   207,     0,   208,
       0,   209,     0,     0,     0,   210,   211,     0,     0,   212,
       0,   213,   214,     0,   215,   216,   217,     0,   218,   219,
     220,   221,     0,     0,   222,   223,   224,   225,   226,   227,
     228,     0,   229,     0,   230,     0,     0,   231,     0,   232,
     233,   234,   235,     0,     0,   236,     0,     0,   237,   238,
     239,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,     0,   251,   358,   252,   253,   254,
     255,   256,     0,   257,   258,     0,     0,   259,   260,   261,
       0,     0,   262,   359,     0,   499,   263,   264,     0,     0,
     265,     0,     0,   266,   267,   268,   269,   270,   271,     0,
     272,   273,   274,   275,     0,   276,   277,   278,   279,   280,
     281,   282,     0,   283,   284,   285,   286,   287,   288,   289,
     290,   291,     0,   292,   293,   294,   295,   296,   297,   298,
     299,     0,   300,   301,   302,     0,   303,   304,   305,   306,
       0,   307,   308,     0,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,     0,   319,   320,   321,     0,   322,
     323,     0,   324,     0,   325,   326,   327,   328,     0,   329,
     330,   331,   332,     0,     0,   333,   334,   335,   336,   337,
       0,     0,   338,   339,   340,   341,   342,   343,     0,   344,
     345,   346,     0,     0,   347,   348,   349,   350,   351,   352,
       0,   353,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,     0,     0,     0,   146,   147,
     148,     0,   149,   150,   151,     0,   152,     0,   153,   154,
       0,   155,   156,   157,   158,     0,     0,   159,   160,   161,
     162,     0,   163,   164,   165,   166,   167,     0,     0,   168,
     169,   170,     0,   171,   172,   173,   174,     0,   175,   176,
       0,     0,     0,     0,     0,   177,   178,   948,   180,   181,
     182,   183,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   192,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,   201,   202,   203,
     204,     0,   205,   206,   207,     0,   208,     0,   209,     0,
       0,     0,   949,   211,     0,     0,   212,     0,   213,   214,
       0,   215,   216,   217,     0,   218,   219,   220,   221,     0,
       0,   222,   223,   224,   225,   226,   227,   228,     0,   229,
       0,   230,     0,     0,   231,     0,   232,   233,   950,   235,
       0,     0,   236,     0,     0,   237,   238,   239,     0,     0,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,     0,   251,   358,   252,   253,   254,   255,   256,     0,
     257,   258,     0,     0,   259,   260,   261,     0,     0,   262,
     359,     0,     0,   263,   264,     0,     0,   265,     0,     0,
     266,   267,   268,   269,   270,   271,     0,   272,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   281,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,     0,
     292,   293,   294,   295,   951,   297,   298,   299,     0,   300,
     301,   302,     0,   303,   304,   305,   306,     0,   307,   308,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,   952,   319,   320,   321,     0,   322,   323,     0,   324,
       0,   325,   326,   327,   328,     0,   329,   330,   331,   332,
       0,     0,   333,   334,   335,   336,   337,     0,     0,   338,
     339,   340,   341,   342,   343,     0,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   353,   138,
     139,   140,   141,   142,   143,   144,     0,   145,     0,     0,
       0,     0,     0,     0,     0,   146,   147,   148,     0,   149,
     150,   151,     0,   152,     0,   153,   154,     0,   155,   156,
     157,   158,     0,     0,   159,   160,   161,   162,     0,   163,
     164,   165,   166,   167,     0,     0,   168,   169,   170,     0,
     171,   172,   173,   174,     0,   175,   176,     0,     0,     0,
       0,     0,   177,   178,   179,   180,   181,   182,   183,   184,
       0,   185,     0,   186,   187,   188,   189,   190,     0,     0,
       0,   191,   192,   193,   194,     0,   195,   196,     0,   197,
       0,   198,   199,   200,   201,   202,   203,   204,     0,   205,
     206,   207,     0,   208,     0,   209,     0,     0,     0,   210,
    1503,     0,     0,   212,     0,   213,   214,     0,   215,   216,
     217,     0,   218,   219,   220,   221,     0,     0,   222,   223,
     224,   225,   226,   227,   228,     0,   229,     0,   230,     0,
       0,   231,     0,   232,   233,   234,   235,     0,     0,   236,
       0,     0,   237,   238,  1504,     0,     0,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,     0,   251,
     358,   252,   253,   254,   255,   256,     0,   257,   258,     0,
       0,   259,   260,   261,     0,     0,   262,   359,     0,     0,
     263,   264,     0,     0,   265,     0,     0,   266,   267,   268,
     269,   270,   271,     0,   272,   273,   274,   275,     0,   276,
     277,   278,   279,   280,   281,   282,     0,   283,   284,   285,
     286,   287,   288,   289,   290,   291,     0,   292,   293,   294,
     295,   296,   297,   298,   299,     0,   300,   301,   302,     0,
     303,   304,   305,   306,     0,   307,   308,     0,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   318,  1505,  1506,
     320,  1507,     0,   322,   323,     0,   324,     0,   325,   326,
     327,   328,     0,   329,   330,   331,   332,     0,     0,   333,
     334,   335,   336,   337,     0,     0,   338,   339,   340,   341,
     342,   343,     0,   344,   345,   346,     0,     0,   347,   348,
     349,   350,   351,   352,     0,   353,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,     0,
       0,     0,   146,   147,   148,     0,   149,   150,   151,     0,
     152,     0,   153,   154,     0,   155,   156,   157,   158,     0,
       0,   159,   160,   161,   162,     0,   163,   164,   165,   166,
     167,     0,     0,   168,   169,   170,     0,   171,   172,   173,
     174,     0,   175,   176,     0,     0,     0,     0,     0,   177,
     178,   179,   180,   181,   182,   183,   184,     0,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   192,
     193,   194,     0,   195,   196,     0,   197,     0,   198,   199,
     200,   201,   202,   203,   204,     0,   205,   206,   207,     0,
     208,     0,   209,     0,     0,     0,   210,   211,     0,     0,
     212,     0,   213,   214,     0,   215,   216,   217,     0,   218,
     219,   220,   221,     0,     0,   222,   223,   224,   225,   226,
     227,   228,     0,   229,     0,   230,     0,     0,   231,     0,
     232,   233,   234,   235,     0,     0,   236,     0,     0,   237,
     238,   239,     0,     0,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,     0,   251,   358,   252,   253,
     254,   255,   256,     0,   257,   258,     0,     0,   259,   260,
     261,     0,     0,   262,   359,     0,     0,   263,   264,     0,
       0,   265,     0,     0,   266,   267,   268,   269,   270,   271,
       0,   272,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   281,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,     0,   292,   293,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,     0,   303,   304,   305,
     306,     0,   307,   308,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,     0,   319,   320,   321,     0,
     322,   323,     0,   324,     0,   325,   326,   327,   328,     0,
     329,   330,   331,   332,     0,     0,   333,   334,   335,   336,
     337,     0,  2341,   338,   339,   340,   341,   342,   343,     0,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   353,   138,   139,   140,   141,   142,   143,   144,
       0,   145,     0,     0,     0,     0,     0,     0,     0,   146,
     147,   148,     0,   149,   150,   151,     0,   152,     0,   153,
     154,     0,   155,   156,   157,   158,     0,     0,   159,   160,
     161,   162,     0,   163,   164,   165,   166,   167,     0,     0,
     168,   169,   170,     0,   171,   172,   173,   174,     0,   175,
     176,     0,     0,     0,     0,     0,   177,   178,   179,   180,
     181,   182,   183,   184,     0,   185,     0,   186,   187,   188,
     189,   190,     0,     0,     0,   191,   192,   193,   194,     0,
     195,   196,     0,   197,     0,   198,   199,   200,   201,   202,
     203,   204,     0,   205,   206,   207,     0,   208,     0,   209,
       0,     0,     0,   210,   211,     0,     0,   212,     0,   213,
     214,     0,   215,   216,   217,     0,   218,   219,   220,   221,
       0,     0,   222,   223,   224,   225,   226,   227,   228,     0,
     229,     0,   230,     0,     0,   231,     0,   232,   233,   234,
     235,     0,     0,   236,     0,     0,   237,   238,   239,     0,
       0,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,     0,   251,   358,   252,   253,   254,   255,   256,
       0,   257,   258,     0,     0,   259,   260,   261,     0,     0,
     262,   359,     0,     0,   263,   264,     0,     0,   265,     0,
       0,   266,   267,   268,   269,   270,   271,     0,   272,   273,
     274,   275,     0,   276,   277,   278,   279,   280,   281,   282,
       0,   283,   284,   285,   286,   287,   288,   289,   290,   291,
       0,   292,   293,   294,   295,   296,   297,   298,   299,     0,
     300,   301,   302,     0,   303,   304,   305,   306,     0,   307,
     308,     0,   309,   310,   311,   312,   313,   314,   315,   316,
     317,   318,     0,   319,   320,   321,     0,   322,   323,     0,
     324,     0,   325,   326,   327,   328,     0,   329,   330,   331,
     332,     0,     0,   333,   334,   335,   336,   337,     0,     0,
     338,   339,   340,   341,   342,   343,     0,   344,   345,   346,
       0,     0,   347,   348,   349,   350,   351,   352,     0,   353,
     138,   139,   140,   141,   142,   143,   144,   471,   145,     0,
       0,     0,     0,     0,     0,     0,   146,   147,   148,     0,
     149,   150,   151,     0,   152,     0,   153,   154,     0,   155,
     156,   157,   158,     0,     0,   159,   160,   161,   162,     0,
     163,   164,   165,   166,   167,     0,     0,   168,   169,   170,
       0,   171,   172,   173,   174,     0,   175,   176,     0,     0,
       0,     0,     0,   177,   178,   179,   180,   181,   182,   183,
     184,     0,   185,     0,   186,   187,   188,   189,   190,     0,
       0,     0,   191,   192,   193,   194,     0,   195,   196,     0,
     197,     0,   198,   199,   200,   201,   202,   203,   204,     0,
     205,   206,   207,     0,   208,     0,   209,     0,     0,     0,
     210,   211,     0,     0,   212,     0,   213,   214,     0,   215,
     216,   217,     0,   218,   219,   220,   221,     0,     0,   222,
     223,   224,   225,   226,   227,   228,     0,   229,     0,   230,
       0,     0,   231,     0,   232,   233,   234,   235,     0,     0,
     236,     0,     0,   237,   238,   239,     0,     0,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,     0,
     251,     0,   252,   253,   254,   255,   256,     0,   257,   258,
       0,     0,   259,   260,   261,     0,     0,   262,     0,     0,
       0,   263,   264,     0,     0,   265,     0,     0,   266,   267,
     268,   269,   270,   271,     0,   272,   273,   274,   275,     0,
     276,   277,   278,   279,   280,   281,   282,     0,   283,   284,
     285,   286,   287,   288,   289,   290,   291,     0,   292,   293,
     294,   295,   296,   297,   298,   299,     0,   300,   301,   472,
       0,   303,   304,   305,   306,     0,   307,   308,     0,   309,
     310,   311,   312,   313,   314,   315,   316,   317,   318,     0,
     319,   320,   321,     0,   473,   323,     0,   324,     0,   474,
     326,   327,   328,     0,   329,   330,   331,   332,     0,     0,
     333,   334,   335,   336,   337,     0,     0,   338,   339,   340,
     341,   342,   343,     0,   344,   345,   346,     0,     0,   347,
     348,   349,   350,   351,   352,     0,   353,   138,   139,   140,
     141,   142,   143,   144,   490,   145,     0,     0,     0,     0,
       0,     0,     0,   146,   147,   148,     0,   149,   150,   151,
       0,   152,     0,   153,   154,     0,   155,   156,   157,   158,
       0,     0,   159,   160,   161,   162,     0,   163,   164,   165,
     166,   167,     0,     0,   168,   169,   170,     0,   171,   172,
     173,   174,     0,   175,   176,     0,     0,     0,     0,     0,
     177,   178,   179,   180,   181,   182,   183,   184,     0,   185,
       0,   186,   187,   188,   189,   190,     0,     0,     0,   191,
     192,   193,   194,     0,   195,   196,     0,   197,     0,   198,
     199,   200,   201,   202,   203,   204,     0,   205,   206,   207,
       0,   208,     0,   209,     0,     0,     0,   210,   211,     0,
       0,   212,     0,   213,   214,     0,   215,   216,   217,     0,
     218,   219,   220,   221,     0,     0,   222,   223,   224,   225,
     226,   227,   228,     0,   229,     0,   230,     0,     0,   231,
       0,   232,   233,   234,   235,     0,     0,   236,     0,     0,
     237,   238,   239,     0,     0,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,     0,   251,     0,   252,
     253,   254,   255,   256,     0,   257,   258,     0,     0,   259,
     260,   261,     0,     0,   262,     0,     0,     0,   263,   264,
       0,     0,   265,     0,     0,   266,   267,   268,   269,   270,
     271,     0,   272,   273,   274,   275,     0,   276,   277,   278,
     279,   280,   281,   282,     0,   283,   284,   285,   286,   287,
     288,   289,   290,   291,     0,   292,   293,   294,   295,   296,
     297,   298,   299,     0,   300,   301,   491,     0,   303,   304,
     305,   306,     0,   307,   308,     0,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,     0,   319,   320,   321,
       0,   492,   323,     0,   324,     0,   493,   326,   327,   328,
       0,   329,   330,   331,   332,     0,     0,   333,   334,   335,
     336,   337,     0,     0,   338,   339,   340,   341,   342,   343,
       0,   344,   345,   346,     0,     0,   347,   348,   349,   350,
     351,   352,     0,   353,   138,   139,   140,   141,   142,   143,
     144,   759,   145,     0,     0,     0,     0,     0,     0,     0,
     146,   147,   148,     0,   149,   150,   151,     0,   152,     0,
     153,   154,     0,   155,   156,   157,   158,     0,     0,   159,
     160,   161,   162,     0,   163,   164,   165,   166,   167,     0,
       0,   168,   169,   170,     0,   171,   172,   173,   174,     0,
     175,   176,     0,     0,     0,     0,     0,   177,   178,   179,
     180,   181,   182,   183,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   192,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   201,
     202,   203,   204,     0,   205,   206,   207,     0,   208,     0,
     209,     0,     0,     0,   210,   211,     0,     0,   212,     0,
     213,   214,     0,   215,   216,   217,     0,   218,   219,   220,
     221,     0,     0,   222,   223,   224,   225,   226,   227,   228,
       0,   229,     0,   230,     0,     0,   231,     0,   232,   233,
     234,   235,     0,     0,   236,     0,     0,   237,   238,   239,
       0,     0,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   249,   250,     0,   251,     0,   252,   253,   254,   255,
     256,     0,   257,   258,     0,     0,   259,   260,   261,     0,
       0,   262,     0,     0,     0,   263,   264,     0,     0,   265,
       0,     0,   266,   267,   268,   269,   270,   271,     0,   272,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   281,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,     0,   292,   293,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   302,     0,   303,   304,   305,   306,     0,
     307,   308,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,     0,   319,   320,   321,     0,   322,   323,
       0,   324,     0,   325,   326,   327,   328,     0,   329,   330,
     331,   332,     0,     0,   333,   334,   335,   336,   337,     0,
       0,   338,   339,   340,   341,   342,   343,     0,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     353,   138,   139,   140,   141,   142,   143,   144,     0,   145,
       0,     0,     0,     0,     0,     0,     0,   146,   147,   148,
     768,   149,   150,   151,     0,   152,     0,   153,   154,     0,
     155,   156,   157,   158,     0,     0,   159,   160,   161,   769,
       0,   163,   164,   165,   166,   167,     0,     0,   168,   169,
     170,     0,   171,   172,   173,   174,     0,   175,   176,     0,
       0,     0,     0,     0,   177,   178,   179,   180,   181,   182,
     183,   184,     0,   185,     0,   186,   187,   188,   189,   190,
       0,     0,     0,   191,   192,   193,   194,     0,   195,   196,
       0,   197,     0,   198,   199,   200,   201,   202,   203,   204,
       0,   205,   206,   207,     0,   208,     0,   209,     0,     0,
       0,   210,   211,     0,     0,   212,     0,   213,   214,     0,
     215,   216,   217,     0,   218,   219,   220,   221,     0,     0,
     222,   223,   224,   225,   226,   227,   228,     0,   229,     0,
     230,     0,     0,   231,     0,   232,   233,   234,   235,     0,
       0,   236,     0,     0,   237,   238,   239,     0,     0,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   484,   250,
       0,   251,     0,   252,   253,   254,   255,   256,     0,   257,
     258,     0,     0,   259,   260,   261,     0,     0,   262,     0,
       0,     0,   263,   264,     0,     0,   265,     0,     0,   266,
     267,   268,   269,   270,   271,     0,   272,   273,   274,   275,
       0,   276,   277,   278,   279,   280,   281,   282,     0,   283,
     284,   285,   286,   287,   288,   289,   290,   291,     0,   292,
     293,   294,   295,   296,   297,   298,   299,     0,   300,   301,
     763,     0,   303,   304,   305,   306,     0,   307,   308,     0,
     309,   310,   311,   312,   313,   314,   315,   316,   317,   318,
       0,   319,   320,   321,     0,   486,   323,     0,   324,     0,
     487,   326,   327,   328,     0,   329,   330,   331,   332,     0,
       0,   333,   334,   335,   336,   337,     0,     0,   338,   339,
     340,   341,   342,   343,     0,   344,   345,   346,     0,     0,
     347,   348,   349,   350,   351,   352,     0,   353,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,     0,     0,     0,   146,   147,   148,     0,   149,   150,
     151,     0,   152,     0,   153,   154,     0,   155,   156,   157,
     158,     0,     0,   159,   160,   161,   162,     0,   163,   164,
     165,   166,   167,     0,     0,   168,   169,   170,     0,   171,
     172,   173,   174,     0,   175,   176,     0,     0,     0,     0,
       0,   177,   178,   179,   180,   181,   182,   183,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   192,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,   201,   202,   203,   204,     0,   205,   206,
     207,     0,   208,     0,   209,     0,     0,     0,   210,   211,
       0,  1686,   212,     0,   213,   214,     0,   215,   216,   217,
       0,   218,   219,   220,   221,     0,     0,   222,   223,   224,
     225,   226,   227,   228,     0,   229,     0,   230,     0,     0,
     231,     0,   232,   233,   234,   235,     0,     0,   236,     0,
       0,   237,   238,   239,     0,     0,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,     0,   251,     0,
     252,   253,   254,   255,   256,     0,   257,   258,     0,     0,
     259,   260,   261,     0,     0,   262,     0,     0,     0,   263,
     264,     0,     0,   265,     0,     0,   266,   267,   268,   269,
     270,   271,     0,   272,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   281,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,     0,   292,   293,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,     0,   303,
     304,   305,   306,     0,   307,   308,     0,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,     0,   319,   320,
     321,     0,   322,   323,     0,   324,     0,   325,   326,   327,
     328,     0,   329,   330,   331,   332,     0,     0,   333,   334,
     335,   336,   337,     0,     0,   338,   339,   340,   341,   342,
     343,     0,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   353,   138,   139,   140,   141,   142,
     143,   144,     0,   145,     0,     0,     0,     0,     0,  2046,
       0,   146,   147,   148,     0,   149,   150,   151,     0,   152,
       0,   153,   154,     0,   155,   156,   157,   158,     0,     0,
     159,   160,   161,   162,     0,   163,   164,   165,   166,   167,
       0,     0,   168,   169,   170,     0,   171,   172,   173,   174,
       0,   175,   176,     0,     0,     0,     0,     0,   177,   178,
     179,   180,   181,   182,   183,   184,     0,   185,     0,   186,
     187,   188,   189,   190,     0,     0,     0,   191,   192,   193,
     194,     0,   195,   196,     0,   197,     0,   198,   199,   200,
     201,   202,   203,   204,     0,   205,   206,   207,     0,   208,
       0,   209,     0,     0,     0,   210,   211,     0,     0,   212,
       0,   213,   214,     0,   215,   216,   217,     0,   218,   219,
     220,   221,     0,     0,   222,   223,   224,   225,   226,   227,
     228,     0,   229,     0,   230,     0,     0,   231,     0,   232,
     233,   234,   235,     0,     0,   236,     0,     0,   237,   238,
     239,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,     0,   251,     0,   252,   253,   254,
     255,   256,     0,   257,   258,     0,     0,   259,   260,   261,
       0,     0,   262,     0,     0,     0,   263,   264,     0,     0,
     265,     0,     0,   266,   267,   268,   269,   270,   271,     0,
     272,   273,   274,   275,     0,   276,   277,   278,   279,   280,
     281,   282,     0,   283,   284,   285,   286,   287,   288,   289,
     290,   291,     0,   292,   293,   294,   295,   296,   297,   298,
     299,     0,   300,   301,   302,     0,   303,   304,   305,   306,
       0,   307,   308,     0,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,     0,   319,   320,   321,     0,   322,
     323,     0,   324,     0,   325,   326,   327,   328,     0,   329,
     330,   331,   332,     0,     0,   333,   334,   335,   336,   337,
       0,     0,   338,   339,   340,   341,   342,   343,     0,   344,
     345,   346,     0,     0,   347,   348,   349,   350,   351,   352,
       0,   353,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,     0,  2060,     0,   146,   147,
     148,     0,   149,   150,   151,     0,   152,     0,   153,   154,
       0,   155,   156,   157,   158,     0,     0,   159,   160,   161,
     162,     0,   163,   164,   165,   166,   167,     0,     0,   168,
     169,   170,     0,   171,   172,   173,   174,     0,   175,   176,
       0,     0,     0,     0,     0,   177,   178,   179,   180,   181,
     182,   183,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   192,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,   201,   202,   203,
     204,     0,   205,   206,   207,     0,   208,     0,   209,     0,
       0,     0,   210,   211,     0,     0,   212,     0,   213,   214,
       0,   215,   216,   217,     0,   218,   219,   220,   221,     0,
       0,   222,   223,   224,   225,   226,   227,   228,     0,   229,
       0,   230,     0,     0,   231,     0,   232,   233,   234,   235,
       0,     0,   236,     0,     0,   237,   238,   239,     0,     0,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     250,     0,   251,     0,   252,   253,   254,   255,   256,     0,
     257,   258,     0,     0,   259,   260,   261,     0,     0,   262,
       0,     0,     0,   263,   264,     0,     0,   265,     0,     0,
     266,   267,   268,   269,   270,   271,     0,   272,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   281,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,     0,
     292,   293,   294,   295,   296,   297,   298,   299,     0,   300,
     301,   302,     0,   303,   304,   305,   306,     0,   307,   308,
       0,   309,   310,   311,   312,   313,   314,   315,   316,   317,
     318,     0,   319,   320,   321,     0,   322,   323,     0,   324,
       0,   325,   326,   327,   328,     0,   329,   330,   331,   332,
       0,     0,   333,   334,   335,   336,   337,     0,     0,   338,
     339,   340,   341,   342,   343,     0,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   353,   138,
     139,   140,   141,   142,   143,   144,     0,   145,     0,     0,
       0,     0,     0,     0,     0,   146,   147,   148,     0,   149,
     150,   151,     0,   152,     0,   153,   154,     0,   155,   156,
     157,   158,     0,     0,   159,   160,   161,   162,     0,   163,
     164,   165,   166,   167,     0,     0,   168,   169,   170,     0,
     171,   172,   173,   174,     0,   175,   176,     0,     0,     0,
       0,     0,   177,   178,   179,   180,   181,   182,   183,   184,
    2115,   185,     0,   186,   187,   188,   189,   190,     0,     0,
       0,   191,   192,   193,   194,     0,   195,   196,     0,   197,
       0,   198,   199,   200,   201,   202,   203,   204,     0,   205,
     206,   207,     0,   208,     0,   209,     0,     0,     0,   210,
     211,     0,     0,   212,     0,   213,   214,     0,   215,   216,
     217,     0,   218,   219,   220,   221,     0,     0,   222,   223,
     224,   225,   226,   227,   228,     0,   229,     0,   230,     0,
       0,   231,     0,   232,   233,   234,   235,     0,     0,   236,
       0,     0,   237,   238,   239,     0,     0,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,     0,   251,
       0,   252,   253,   254,   255,   256,     0,   257,   258,     0,
       0,   259,   260,   261,     0,     0,   262,     0,     0,     0,
     263,   264,     0,     0,   265,     0,     0,   266,   267,   268,
     269,   270,   271,     0,   272,   273,   274,   275,     0,   276,
     277,   278,   279,   280,   281,   282,     0,   283,   284,   285,
     286,   287,   288,   289,   290,   291,     0,   292,   293,   294,
     295,   296,   297,   298,   299,     0,   300,   301,   302,     0,
     303,   304,   305,   306,     0,   307,   308,     0,   309,   310,
     311,   312,   313,   314,   315,   316,   317,   318,     0,   319,
     320,   321,     0,   322,   323,     0,   324,     0,   325,   326,
     327,   328,     0,   329,   330,   331,   332,     0,     0,   333,
     334,   335,   336,   337,     0,     0,   338,   339,   340,   341,
     342,   343,     0,   344,   345,   346,     0,     0,   347,   348,
     349,   350,   351,   352,     0,   353,   138,   139,   140,   141,
     142,   143,   144,     0,   145,     0,     0,     0,     0,     0,
       0,     0,   146,   147,   148,     0,   149,   150,   151,     0,
     152,     0,   153,   154,     0,   155,   156,   157,   158,     0,
       0,   159,   160,   161,   162,     0,   163,   164,   165,   166,
     167,     0,     0,   168,   169,   170,     0,   171,   172,   173,
     174,     0,   175,   176,     0,     0,     0,     0,     0,   177,
     178,   179,   180,   181,   182,   183,   184,  2117,   185,     0,
     186,   187,   188,   189,   190,     0,     0,     0,   191,   192,
     193,   194,     0,   195,   196,     0,   197,     0,   198,   199,
     200,   201,   202,   203,   204,     0,   205,   206,   207,     0,
     208,     0,   209,     0,     0,     0,   210,   211,     0,     0,
     212,     0,   213,   214,     0,   215,   216,   217,     0,   218,
     219,   220,   221,     0,     0,   222,   223,   224,   225,   226,
     227,   228,     0,   229,     0,   230,     0,     0,   231,     0,
     232,   233,   234,   235,     0,     0,   236,     0,     0,   237,
     238,   239,     0,     0,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,     0,   251,     0,   252,   253,
     254,   255,   256,     0,   257,   258,     0,     0,   259,   260,
     261,     0,     0,   262,     0,     0,     0,   263,   264,     0,
       0,   265,     0,     0,   266,   267,   268,   269,   270,   271,
       0,   272,   273,   274,   275,     0,   276,   277,   278,   279,
     280,   281,   282,     0,   283,   284,   285,   286,   287,   288,
     289,   290,   291,     0,   292,   293,   294,   295,   296,   297,
     298,   299,     0,   300,   301,   302,     0,   303,   304,   305,
     306,     0,   307,   308,     0,   309,   310,   311,   312,   313,
     314,   315,   316,   317,   318,     0,   319,   320,   321,     0,
     322,   323,     0,   324,     0,   325,   326,   327,   328,     0,
     329,   330,   331,   332,     0,     0,   333,   334,   335,   336,
     337,     0,     0,   338,   339,   340,   341,   342,   343,     0,
     344,   345,   346,     0,     0,   347,   348,   349,   350,   351,
     352,     0,   353,   138,   139,   140,   141,   142,   143,   144,
       0,   145,     0,     0,     0,     0,     0,     0,     0,   146,
     147,   148,     0,   149,   150,   151,     0,   152,     0,   153,
     154,     0,   155,   156,   157,   158,     0,     0,   159,   160,
     161,   162,     0,   163,   164,   165,   166,   167,     0,     0,
     168,   169,   170,     0,   171,   172,   173,   174,     0,   175,
     176,     0,     0,     0,     0,     0,   177,   178,   179,   180,
     181,   182,   183,   184,     0,   185,     0,   186,   187,   188,
     189,   190,     0,     0,     0,   191,   192,   193,   194,     0,
     195,   196,     0,   197,     0,   198,   199,   200,   201,   202,
     203,   204,     0,   205,   206,   207,     0,   208,     0,   209,
       0,     0,     0,   210,   211,     0,     0,   212,     0,   213,
     214,     0,   215,   216,   217,     0,   218,   219,   220,   221,
       0,     0,   222,   223,   224,   225,   226,   227,   228,     0,
     229,     0,   230,     0,     0,   231,     0,   232,   233,   234,
     235,     0,     0,   236,     0,     0,   237,   238,   239,     0,
       0,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,     0,   251,     0,   252,   253,   254,   255,   256,
       0,   257,   258,     0,     0,   259,   260,   261,     0,     0,
     262,     0,     0,     0,   263,   264,     0,     0,   265,     0,
       0,   266,   267,   268,   269,   270,   271,     0,   272,   273,
     274,   275,     0,   276,   277,   278,   279,   280,   281,   282,
       0,   283,   284,   285,   286,   287,   288,   289,   290,   291,
       0,   292,   293,   294,   295,   296,   297,   298,   299,     0,
     300,   301,   302,     0,   303,   304,   305,   306,     0,   307,
     308,     0,   309,   310,   311,   312,   313,   314,   315,   316,
     317,   318,     0,   319,   320,   321,     0,   322,   323,     0,
     324,     0,   325,   326,   327,   328,     0,   329,   330,   331,
     332,     0,     0,   333,   334,   335,   336,   337,     0,     0,
     338,   339,   340,   341,   342,   343,     0,   344,   345,   346,
       0,     0,   347,   348,   349,   350,   351,   352,     0,   353,
     138,   139,   140,   141,   142,   143,   144,     0,   145,     0,
       0,     0,     0,     0,     0,     0,   146,   147,   148,     0,
     149,   150,   151,     0,   152,     0,   153,   154,     0,   155,
     156,   157,   158,     0,     0,   159,   160,   161,   162,     0,
     163,   164,   165,   166,   167,     0,     0,   168,   169,   170,
       0,   171,   172,   173,   174,     0,   175,   176,     0,     0,
       0,     0,     0,   177,   178,   179,   180,   181,   182,   183,
     184,     0,   185,     0,   186,   187,   188,   189,   190,     0,
       0,     0,   191,   192,   193,   194,     0,   195,   196,     0,
     197,     0,   198,   199,   200,   201,   202,   203,   204,     0,
     205,   206,   207,     0,   208,     0,   209,     0,     0,     0,
     210,   211,     0,     0,   212,     0,   213,   214,     0,   215,
     216,   217,     0,   218,   219,   220,   221,     0,     0,   222,
     223,   224,   225,   226,   227,   228,     0,   229,     0,   230,
       0,     0,   231,     0,   232,   233,   234,   235,     0,     0,
     236,     0,     0,   237,   238,   239,     0,     0,   240,   241,
     242,   243,   244,   245,   246,   247,   248,   249,   250,     0,
     251,     0,   252,   253,   254,   255,   256,     0,   257,   258,
       0,     0,   259,   260,   261,     0,     0,   262,     0,     0,
       0,   263,   264,     0,     0,   265,     0,     0,   266,   267,
     268,   269,   270,   271,     0,   272,   273,   274,   395,     0,
     276,   277,   278,   279,   280,   281,   282,     0,   283,   284,
     285,   286,   287,   288,   289,   290,   291,     0,   292,   293,
     294,   295,   296,   297,   298,   299,     0,   300,   301,   302,
       0,   303,   304,   305,   306,     0,   307,   308,     0,   309,
     310,   311,   312,   313,   314,   315,   316,   317,   318,     0,
     319,   320,   321,     0,   322,   323,     0,   324,     0,   325,
     326,   327,   328,     0,   329,   330,   331,   332,     0,     0,
     333,   334,   335,   336,   337,     0,     0,   338,   339,   340,
     341,   342,   343,     0,   344,   345,   346,     0,     0,   347,
     348,   349,   350,   351,   352,     0,   353,   138,   139,   140,
     141,   142,   143,   144,     0,   145,     0,     0,     0,     0,
       0,     0,     0,   146,   147,   148,     0,   149,   150,   151,
       0,   152,     0,   153,   154,     0,   155,   156,   157,   158,
       0,     0,   159,   160,   161,   162,     0,   163,   164,   165,
     166,   167,     0,     0,   168,   169,   170,     0,   482,   172,
     173,   174,     0,   175,   176,     0,     0,     0,     0,     0,
     177,   178,   179,   180,   181,   182,   183,   184,     0,   185,
       0,   186,   187,   188,   189,   190,     0,     0,     0,   191,
     192,   193,   194,     0,   195,   196,     0,   197,     0,   198,
     199,   200,   201,   202,   203,   204,     0,   205,   206,   207,
       0,   208,     0,   209,     0,     0,     0,   210,   211,     0,
       0,   212,     0,   213,   214,     0,   215,   216,   217,     0,
     218,   219,   220,   221,     0,     0,   222,   223,   224,   225,
     226,   227,   228,     0,   229,     0,   230,     0,     0,   231,
       0,   232,   233,   234,   235,     0,     0,   236,     0,     0,
     237,   238,   483,     0,     0,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   484,   250,     0,   251,     0,   252,
     253,   254,   255,   256,     0,   257,   258,     0,     0,   259,
     260,   261,     0,     0,   262,     0,     0,     0,   263,   264,
       0,     0,   265,     0,     0,   266,   267,   268,   269,   270,
     271,     0,   272,   273,   274,   275,     0,   276,   277,   278,
     279,   280,   281,   282,     0,   283,   284,   285,   286,   287,
     288,   289,   290,   291,     0,   292,   293,   294,   295,   296,
     297,   298,   299,     0,   300,   301,   485,     0,   303,   304,
     305,   306,     0,   307,   308,     0,   309,   310,   311,   312,
     313,   314,   315,   316,   317,   318,     0,   319,   320,   321,
       0,   486,   323,     0,   324,     0,   487,   326,   327,   328,
       0,   329,   330,   331,   332,     0,     0,   333,   334,   335,
     336,   337,     0,     0,   338,   339,   340,   341,   342,   343,
       0,   344,   345,   346,     0,     0,   347,   348,   349,   350,
     351,   352,     0,   353,   138,   139,   140,   141,   142,   143,
     144,     0,   145,     0,     0,     0,     0,     0,     0,     0,
     146,   147,   148,     0,   149,   150,   151,     0,   152,     0,
     153,   154,     0,   155,   156,   157,   158,     0,     0,   159,
     160,   161,   162,     0,   163,   164,   165,   166,   167,     0,
       0,   168,   169,   170,     0,   171,   172,   173,   174,     0,
     175,   176,     0,     0,     0,     0,     0,   177,   178,   179,
     180,   181,   182,   183,   184,     0,   185,     0,   186,   187,
     188,   189,   190,     0,     0,     0,   191,   192,   193,   194,
       0,   195,   196,     0,   197,     0,   198,   199,   200,   201,
     202,   203,   204,     0,   205,   206,   207,     0,   208,     0,
     209,     0,     0,     0,   210,   211,     0,     0,   212,     0,
     213,   214,     0,   215,   216,   217,     0,   218,   219,   220,
     221,     0,     0,   222,   223,   224,   225,   226,   227,   228,
       0,   229,     0,   230,     0,     0,   231,     0,   232,   233,
     234,   235,     0,     0,   236,     0,     0,   237,   238,   239,
       0,     0,   240,   241,   242,   243,   244,   245,   246,   247,
     248,   484,   250,     0,   251,     0,   252,   253,   254,   255,
     256,     0,   257,   258,     0,     0,   259,   260,   261,     0,
       0,   262,     0,     0,     0,   263,   264,     0,     0,   265,
       0,     0,   266,   267,   268,   269,   270,   271,     0,   272,
     273,   274,   275,     0,   276,   277,   278,   279,   280,   281,
     282,     0,   283,   284,   285,   286,   287,   288,   289,   290,
     291,     0,   292,   293,   294,   295,   296,   297,   298,   299,
       0,   300,   301,   763,     0,   303,   304,   305,   306,     0,
     307,   308,     0,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,     0,   319,   320,   321,     0,   486,   323,
       0,   324,     0,   487,   326,   327,   328,     0,   329,   330,
     331,   332,     0,     0,   333,   334,   335,   336,   337,     0,
       0,   338,   339,   340,   341,   342,   343,     0,   344,   345,
     346,     0,     0,   347,   348,   349,   350,   351,   352,     0,
     353,   138,   139,   140,   141,   142,   143,   144,     0,   145,
       0,     0,     0,     0,     0,     0,     0,   146,   147,   148,
       0,   149,   150,   151,     0,   855,     0,   856,   857,     0,
     155,   156,   157,   158,     0,     0,   159,   858,   859,   162,
       0,   163,   164,   165,   166,     0,     0,     0,   168,   169,
     170,     0,   171,   172,     0,   174,     0,   175,   176,     0,
       0,     0,     0,     0,   177,   178,   179,   180,   181,   860,
     861,   184,     0,   185,     0,   186,   187,   188,   189,   190,
       0,     0,     0,   191,   685,   193,   194,     0,   195,   196,
       0,   197,     0,   198,   199,   200,     0,   202,   203,     0,
       0,   205,   206,   862,     0,   208,     0,   209,     0,     0,
       0,   210,   211,     0,     0,   212,     0,   213,   214,     0,
     215,   216,   217,     0,   218,   219,   220,   221,     0,     0,
     222,   223,   224,   225,   226,   863,   864,     0,   865,     0,
     230,     0,     0,   231,     0,   232,   233,   234,   235,     0,
       0,   236,     0,     0,   237,   238,   239,     0,     0,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   695,
       0,   866,     0,   252,   253,   254,   255,  1342,     0,   257,
     258,     0,     0,     0,   867,   261,     0,     0,   262,     0,
       0,     0,   263,   264,     0,     0,   265,     0,     0,     0,
     267,   268,   269,   270,   271,     0,     0,   273,   274,   275,
       0,   276,   277,   278,   279,   280,   868,   282,     0,   283,
     284,   285,   286,   287,   288,   289,   290,   291,     0,   292,
       0,   294,   295,   296,   297,   298,   299,     0,   300,   301,
     302,     0,   303,   869,   305,   306,     0,   307,   870,     0,
     309,   310,   311,   312,   313,   314,   315,   316,     0,   318,
       0,   319,   320,   321,     0,   871,   872,     0,   324,     0,
     325,     0,   327,     0,     0,   329,   330,   331,   332,     0,
       0,   333,   334,   335,   336,   337,     0,     0,   338,   339,
     340,   341,   873,   343,     0,   344,   345,   346,     0,     0,
     347,   348,   349,   350,   351,   352,     0,   874,   138,   139,
     140,   141,   142,   143,   144,     0,   145,     0,     0,     0,
       0,     0,     0,     0,   146,   147,   148,     0,   149,   150,
     151,     0,   855,     0,   856,   857,     0,   155,   156,   157,
     158,     0,     0,   159,   858,   859,   162,     0,   163,   164,
     165,   166,     0,     0,     0,   168,   169,   170,     0,   171,
     172,     0,   174,     0,   175,   176,     0,     0,     0,     0,
       0,   177,   178,   179,   180,   181,   860,   861,   184,     0,
     185,     0,   186,   187,   188,   189,   190,     0,     0,     0,
     191,   685,   193,   194,     0,   195,   196,     0,   197,     0,
     198,   199,   200,     0,   202,   203,     0,     0,   205,   206,
     862,     0,   208,     0,   209,     0,     0,     0,   210,   211,
       0,     0,   212,     0,   213,   214,     0,   215,   216,   217,
    1159,   218,   219,   220,   221,     0,     0,  1160,   223,   224,
     225,   226,   863,   864,     0,   865,     0,   230,     0,     0,
     231,     0,   232,   233,   234,   235,     0,     0,   236,     0,
       0,   237,   238,   239,     0,     0,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   695,     0,   866,     0,
     252,   253,   254,   255,     0,     0,   257,   258,     0,     0,
       0,   867,   261,     0,     0,   262,     0,     0,     0,   263,
     264,     0,     0,  1161,     0,     0,     0,   267,   268,   269,
     270,   271,     0,     0,   273,   274,   275,     0,   276,   277,
     278,   279,   280,   868,   282,     0,   283,   284,   285,   286,
     287,   288,   289,   290,   291,     0,   292,     0,   294,   295,
     296,   297,   298,   299,     0,   300,   301,   302,     0,   303,
     869,   305,   306,     0,   307,   870,     0,   309,   310,   311,
     312,   313,   314,   315,   316,     0,   318,     0,   319,   320,
     321,     0,   871,   872,     0,   324,     0,   325,     0,   327,
       0,     0,   329,   330,   331,   332,     0,     0,   333,   334,
     335,   336,   337,     0,     0,   338,   339,   340,   341,   873,
     343,     0,   344,   345,   346,     0,     0,   347,   348,   349,
     350,   351,   352,     0,   874,   138,   139,   140,   141,   142,
     143,   144,     0,   145,     0,     0,     0,     0,     0,     0,
       0,   146,   147,   148,     0,   149,   150,   151,     0,   855,
       0,   856,   857,     0,   155,   156,   157,   158,     0,     0,
     159,   858,   859,   162,     0,   163,   164,   165,   166,     0,
       0,     0,   168,   169,   170,     0,   171,   172,     0,   174,
       0,   175,   176,     0,     0,     0,     0,     0,   177,   178,
     179,   180,   181,   860,   861,   184,     0,   185,     0,   186,
     187,   188,   189,   190,     0,     0,     0,   191,   685,   193,
     194,     0,   195,   196,     0,   197,     0,   198,   199,   200,
       0,   202,   203,     0,     0,   205,   206,   862,     0,   208,
       0,   209,     0,     0,     0,   210,   211,     0,     0,   212,
       0,   213,   214,     0,   215,   216,   217,     0,   218,   219,
     220,   221,     0,     0,   222,   223,   224,   225,   226,   863,
     864,     0,   865,     0,   230,     0,     0,   231,     0,   232,
     233,   234,   235,     0,     0,   236,     0,     0,   237,   238,
     239,     0,     0,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   695,     0,   866,     0,   252,   253,   254,
     255,  1959,     0,   257,   258,     0,     0,     0,   867,   261,
       0,     0,   262,     0,     0,     0,   263,   264,     0,     0,
     265,     0,     0,     0,   267,   268,   269,   270,   271,     0,
       0,   273,   274,   275,     0,   276,   277,   278,   279,   280,
     868,   282,     0,   283,   284,   285,   286,   287,   288,   289,
     290,   291,     0,   292,     0,   294,   295,   296,   297,   298,
     299,     0,   300,   301,   302,     0,   303,   869,   305,   306,
       0,   307,   870,     0,   309,   310,   311,   312,   313,   314,
     315,   316,     0,   318,     0,   319,   320,   321,     0,   871,
     872,     0,   324,     0,   325,     0,   327,     0,     0,   329,
     330,   331,   332,     0,     0,   333,   334,   335,   336,   337,
       0,     0,   338,   339,   340,   341,   873,   343,     0,   344,
     345,   346,     0,     0,   347,   348,   349,   350,   351,   352,
       0,   874,   138,   139,   140,   141,   142,   143,   144,     0,
     145,     0,     0,     0,     0,     0,     0,     0,   146,   147,
     148,     0,   149,   150,   151,     0,   855,     0,   856,   857,
       0,   155,   156,   157,   158,     0,     0,   159,   858,   859,
     162,     0,   163,   164,   165,   166,     0,     0,     0,   168,
     169,   170,     0,   171,   172,     0,   174,     0,   175,   176,
       0,     0,     0,     0,     0,   177,   178,   179,   180,   181,
     860,   861,   184,     0,   185,     0,   186,   187,   188,   189,
     190,     0,     0,     0,   191,   685,   193,   194,     0,   195,
     196,     0,   197,     0,   198,   199,   200,     0,   202,   203,
       0,     0,   205,   206,   862,     0,   208,     0,   209,     0,
       0,     0,   210,   211,     0,     0,   212,     0,   213,   214,
       0,   215,   216,   217,     0,   218,   219,   220,   221,     0,
       0,   222,   223,   224,   225,   226,   863,   864,     0,   865,
       0,   230,     0,     0,   231,     0,   232,   233,   234,   235,
       0,     0,   236,     0,     0,   237,   238,   239,     0,     0,
     240,   241,   242,   243,   244,   245,   246,   247,   248,   249,
     695,     0,   866,     0,   252,   253,   254,   255,     0,     0,
     257,   258,     0,     0,     0,   867,   261,     0,     0,   262,
       0,     0,     0,   263,   264,     0,     0,   265,     0,     0,
       0,   267,   268,   269,   270,   271,     0,     0,   273,   274,
     275,     0,   276,   277,   278,   279,   280,   868,   282,     0,
     283,   284,   285,   286,   287,   288,   289,   290,   291,     0,
     292,     0,   294,   295,   296,   297,   298,   299,     0,   300,
     301,   302,     0,   303,   869,   305,   306,     0,   307,   870,
       0,   309,   310,   311,   312,   313,   314,   315,   316,     0,
     318,     0,   319,   320,   321,     0,   871,   872,     0,   324,
       0,   325,     0,   327,     0,     0,   329,   330,   331,   332,
       0,     0,   333,   334,   335,   336,   337,     0,     0,   338,
     339,   340,   341,   873,   343,     0,   344,   345,   346,     0,
       0,   347,   348,   349,   350,   351,   352,     0,   874,   138,
     139,   140,   141,   142,   143,   144,     0,   145,     0,     0,
       0,     0,     0,     0,     0,   146,   147,   148,     0,   149,
     150,   151,     0,   855,     0,   856,   857,     0,   155,   156,
     157,   158,     0,     0,   159,   858,   859,   162,     0,   163,
     164,   165,   166,     0,     0,     0,   168,   169,   170,     0,
     171,   172,     0,   174,     0,   175,   176,     0,     0,     0,
       0,     0,   177,   178,   179,   180,   181,   860,   861,   184,
       0,   185,     0,   186,   187,   188,   189,   190,     0,     0,
       0,   191,   685,   193,   194,     0,   195,   196,     0,   197,
       0,   198,   199,   200,     0,   202,   203,     0,     0,   205,
     206,   862,     0,   208,     0,   209,     0,     0,     0,   210,
     211,     0,     0,   212,     0,   213,   214,     0,   215,   216,
     217,     0,   218,   219,   220,   221,     0,     0,   222,   223,
     224,   225,   226,   863,   864,     0,   865,     0,   230,     0,
       0,   231,     0,   232,   233,   234,   235,     0,     0,   236,
       0,     0,   237,   238,   239,     0,     0,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   695,     0,   866,
       0,   252,   253,   254,   255,     0,     0,   257,   258,     0,
       0,     0,   867,   261,     0,     0,   262,     0,     0,     0,
     263,   264,     0,     0,   265,     0,     0,     0,   267,   268,
     269,   270,   271,     0,     0,   273,   274,   275,     0,   276,
     277,   278,   279,   280,   868,   282,     0,   283,   284,   285,
     286,   287,   288,   289,   290,   291,     0,   292,     0,   294,
     295,   296,   297,   298,   299,     0,   300,   301,   302,     0,
     303,     0,   305,   306,     0,   307,   870,     0,   309,   310,
     311,   312,   313,   314,   315,   316,     0,   318,     0,   319,
     320,   321,     0,   871,   872,     0,   324,     0,   325,     0,
     327,     0,     0,   329,   330,   331,   332,     0,     0,   333,
     334,   335,   336,   337,     0,     0,   338,   339,   340,   341,
     873,   343,     0,   344,   345,   346,   370,     0,   347,   348,
     349,   350,   351,   352,     0,   874,   371,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   372,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     373,     0,  -380,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   374,     0,     0,     0,     0,
       0,   375,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   376,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   377,     0,   378,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  -478,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  -310,     0,     0,     0,
       0,     0,     0,     0,     0,   379,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   380,     0,   381,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  -310,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   382,     0,     0,     0,     0,  -215,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -215,
     383,     0,   384,     0,     0,     0,     0,     0,     0,     0,
       0,   385,     0,     0,     0,   386,   387,     0,     0,   388,
       0,     0,     0,     0,     0,   389
};

static const short yycheck[] =
{
       0,    41,   408,     7,     8,   380,   482,   628,   636,    13,
      14,   462,  1203,   125,    18,   127,    20,   913,   743,     0,
     603,   585,    26,     0,    28,   589,    30,  1035,   611,   495,
      34,    35,   710,   408,   637,  1207,     0,   749,     0,  1341,
     752,    41,    24,   129,  1512,   374,   898,   852,   612,    39,
     136,   852,   124,    37,   639,   898,   128,   771,   898,  1040,
     975,  1042,   722,  1531,  1171,   481,  1684,     0,  1165,  1649,
    1819,  1820,   939,   423,   403,     0,   921,     0,   483,     0,
     485,    17,    21,    40,    21,     0,    35,     0,  2026,    40,
      40,    10,     0,     0,    46,    74,    15,    35,    61,  1814,
      74,  1760,     0,   125,    79,    10,    58,  2245,    79,    10,
      15,    79,    10,    35,    15,    10,    74,    28,    42,     7,
      15,   125,   126,   127,    42,   129,   130,    68,   132,    53,
      97,   135,   136,   675,   676,   141,  1553,    96,   111,   189,
     768,   113,   111,    58,   134,    58,   112,    87,   219,   112,
      58,     0,    42,    81,   696,    66,    74,     5,   187,    49,
     135,    10,   684,    53,   135,   571,    21,   135,   131,    96,
      74,    74,   239,    83,    74,   232,   120,    79,    20,   184,
     840,    94,   165,   146,   108,    87,   106,   128,    94,   152,
     269,   192,    78,   106,   872,    42,   571,    16,   111,    87,
     115,    69,   115,   116,    74,   118,    77,   115,    77,   172,
     181,   182,    16,    77,   182,    95,  2386,    58,   108,     7,
     261,   165,   194,   134,   845,     8,   139,   887,   193,    89,
     547,    28,    76,   139,   187,   155,  1047,   203,   317,    79,
    1045,    24,   155,   560,  1045,  2183,   126,   288,    96,  2419,
     202,   122,   120,   122,   237,  2214,   142,  2216,   122,   330,
     239,   108,   180,   330,   376,   176,   184,   230,   239,   189,
     180,   239,   239,   330,   115,   387,   189,   187,   227,   907,
     180,   184,   124,   249,   197,   335,   258,   246,   212,   227,
     911,   197,   378,   237,   212,   269,   269,   165,   370,    87,
     168,   330,   220,   389,   244,   227,   311,   419,   210,  1021,
     180,   286,  2450,   335,   293,   286,   858,   859,   286,   246,
     406,   262,   212,   926,   866,   236,    79,   399,  1987,   335,
     933,   232,   251,   405,  2049,   223,   180,   311,  2053,   861,
     305,    20,  2057,   317,   295,   308,   251,    26,  2307,   262,
     251,   330,  2290,   311,   311,  1200,   251,   315,   282,   330,
     311,   289,   330,   330,   282,   212,   335,   371,   281,   237,
     374,   323,   376,   330,   378,   281,   380,   330,   382,   330,
     330,   385,   135,   387,   232,   389,   335,   326,   301,   974,
     975,   395,   282,   332,   330,   332,   400,   335,   246,   403,
     985,   986,   406,   808,   408,  1117,   410,   311,   398,   334,
     414,   334,   416,   334,   202,   419,   331,   315,   331,   334,
     333,   334,     8,   331,   322,  1010,   334,   334,    17,   330,
     835,  1848,  1849,  1018,  1606,   282,   334,    23,   306,   789,
     300,    27,   554,   317,  1311,    34,   286,    76,   113,   239,
      35,  2200,    38,    39,   328,    69,   244,   999,  1000,    10,
     111,  1175,   330,   467,  1178,  1179,   315,   579,   323,   324,
     325,   326,   948,   322,   950,   951,    17,   332,   482,   483,
     592,   485,    90,    76,    58,   334,   239,   302,    79,  2204,
     896,  1076,   564,    34,  2209,    83,   217,   569,  2213,    73,
      26,   952,   484,   293,   616,    79,   120,   507,  1168,   328,
      10,   100,   331,     7,   333,  1523,   335,    11,   630,    78,
     335,   896,   114,    97,   328,  1622,   507,   331,    67,   333,
     507,   335,    83,   286,   123,  1642,   232,   130,   124,   125,
     330,   127,   128,   507,   135,   507,   618,   133,   134,   145,
     554,   165,  2132,   161,   168,   184,    10,   561,    58,   100,
     149,   135,   566,   141,    53,   286,   158,   571,   572,   234,
     114,  1668,   576,   311,   507,   579,    74,   330,   272,    79,
     584,   143,   123,   142,    28,    74,   155,   180,   592,  2304,
     286,   176,   180,    87,   202,   121,   184,    97,   310,   125,
    1006,   106,   262,   315,  1415,  1416,  1417,  1418,   149,  1420,
     304,   115,   616,   617,   158,   619,   217,  1181,  1182,    74,
     189,   217,    94,   237,  2242,   164,   630,   166,   154,    83,
     192,  1006,   636,   298,   106,   135,    80,   111,  1733,   228,
     241,   641,   227,  1119,   256,   257,  1948,   651,   239,   238,
     155,   125,   196,  1365,   658,   655,   264,  1073,   295,   244,
     641,   302,   303,   252,   641,   239,  1761,   139,   315,   669,
     259,   263,  1483,   265,   311,   322,   240,   641,   655,   641,
     112,   311,   180,   155,   189,   140,   686,   228,   728,   330,
     112,   180,   306,   330,   232,   286,    21,   238,   262,   311,
     330,   272,  1874,   158,    44,  1877,  1878,   311,    14,   131,
     299,   252,   286,   302,   714,     0,   254,   189,   259,   263,
     220,   265,   248,     0,   146,   197,   330,    49,   728,   223,
     152,    53,   311,  1445,   234,  1356,   334,    61,   315,   239,
     184,   331,  1345,  1346,   334,   424,   190,   426,   730,   428,
     244,   330,   329,   432,   298,   434,   330,   255,   299,   741,
     258,  1868,   810,   263,   768,   265,   296,   302,   303,   329,
     302,   303,   776,   777,   274,   335,   311,   825,   263,   311,
     265,    58,   786,   369,   370,  1445,   286,   287,   112,   771,
     376,   311,   192,  1645,  1709,   330,   796,   797,   330,   799,
    1781,   387,  1645,   192,   808,  1645,    69,   131,   230,   281,
     330,   897,   398,   399,  1539,  1144,  1621,    94,   143,   405,
    1621,   329,   146,   935,   263,   315,   265,   335,   152,   106,
      21,   835,   322,   419,   111,    12,    13,   143,   115,   116,
     102,   118,   223,   833,   225,   931,   850,   311,   172,   333,
    1435,  1436,  1437,  1438,   309,   310,   311,   120,  1572,   331,
     315,   311,   139,   244,  1578,  1579,   330,   322,   192,   455,
     311,  2339,   302,   303,     5,   326,   462,   949,   155,    10,
     330,   332,   330,  2351,   470,    22,    17,  1472,   194,   330,
      83,   274,   896,   897,   223,   311,   225,  1622,  1906,   583,
     330,    32,   165,   907,    41,   168,   230,   331,  1622,   102,
     496,   335,   189,   499,   330,   244,   224,  2089,   958,  2091,
     197,   302,   184,   509,   328,   187,   330,   931,  1513,  1514,
    1741,   935,   215,  1744,  1745,  1746,  1747,  1748,  1749,  1750,
    1751,  1752,   626,  1754,   948,   310,   950,   951,  2250,    14,
     315,  1554,   143,   144,   958,   331,    55,   322,   958,   335,
     323,   324,   325,   326,   263,   289,   265,   967,   554,   332,
     796,   797,   128,   799,   237,   659,  1787,   563,   564,   331,
    1583,  1584,   334,   569,   308,   262,   967,   180,    39,    40,
     967,   184,   183,   579,   187,   330,   321,   322,   323,   324,
     325,   326,  1006,   967,   281,   967,   592,   332,   314,   330,
    2018,   317,   318,   319,   276,   321,   322,   323,   324,   325,
     326,   283,   796,   797,   301,   799,   332,   613,   614,   424,
     616,   426,   618,   428,   620,   331,  1036,   432,   263,   335,
     265,   331,  1042,   306,   630,   335,   309,   331,   311,  1049,
    1456,   335,   149,  1519,   331,   331,   333,   334,   331,   335,
     315,     9,   335,   214,  1776,  1541,  1435,   653,  1437,  1438,
    1156,  1674,  2054,  1676,  2056,  1678,  1080,   511,   143,   513,
     192,  1456,  1488,   276,  1170,  2366,  2367,  1765,  1766,  1201,
     283,   331,   771,  1205,  1206,   335,   331,   776,   777,   331,
     335,    49,   331,   335,  1915,    53,   335,   331,  1194,   335,
    1196,   335,  1094,  1488,   331,  1119,   302,   303,   335,   331,
      68,   805,   331,   335,   331,   330,   335,    42,   335,   194,
     321,   322,   323,   324,   325,   326,    22,    85,    53,  1143,
    1144,   332,   331,  1147,   331,    33,   335,   331,   335,  1153,
     331,   335,  1156,   331,   335,   307,   331,   335,   145,    74,
     335,    76,    59,    60,   192,   113,  1170,  1171,  1172,  1173,
    1174,   331,   331,   331,     8,   335,   335,   335,   195,    67,
     128,   307,  1186,  1187,  1188,  1189,  1190,  1772,  1192,    23,
    1194,   331,  1196,    27,    91,   335,    58,  1201,   784,   331,
      22,  1205,  1206,   335,    38,    39,   331,   331,   892,   331,
     335,   335,   331,   335,  2222,   130,   335,    79,   307,  2030,
     331,   145,  1204,   331,   335,  1207,   143,   335,   125,   331,
     331,  2127,   244,   335,   335,    97,     8,   921,   331,   127,
     323,   331,   335,   110,   928,   335,   194,   833,   143,   314,
     331,    23,   317,   318,   319,    27,   321,   322,   323,   324,
     325,   326,   302,   303,   331,   180,    38,   332,   335,   184,
     331,   302,   303,   135,   335,   331,   164,   194,   166,    32,
     177,   178,  2473,   826,   827,   828,   234,   235,   176,   330,
     124,   125,   240,   127,   128,   302,   303,   212,    21,   133,
     134,  1543,  1544,    26,   330,   220,   330,   204,   987,   988,
     989,   223,   143,   333,   262,   330,   995,  1321,   997,   998,
     223,  1325,  1636,  1637,  1638,  1004,   274,   223,   192,   330,
     330,   279,    42,   302,  1013,    17,  1015,  1341,   226,   333,
     149,    22,    21,   929,   930,    22,  1328,    26,   301,   935,
     298,   330,  1955,  1032,   330,  1034,  1360,   192,   220,   192,
     335,   133,   134,   949,   261,   253,   952,   282,    17,   107,
    2181,  1974,   234,   153,   106,   192,   962,   239,   330,  1419,
     311,   125,   311,   280,  1068,   330,   330,   973,   330,   330,
     330,   209,   330,   330,   291,   311,   330,   314,   311,   143,
     330,   263,   125,   265,   321,   322,   323,   324,   325,   326,
    1089,   330,   274,   311,    93,   332,   330,   335,   311,  1419,
     143,   144,   330,  1463,   286,   287,   321,   322,   323,   324,
     325,   326,  1472,    17,   153,   153,   330,   332,   330,  1511,
     330,   330,   121,  1447,   145,   330,   125,   330,   330,   330,
     194,   106,  1456,   269,   269,  1455,   269,   223,   269,   288,
     183,   269,   289,  1463,   143,   144,  1466,   269,  2364,    49,
     192,   194,  1472,   303,   270,   154,   269,   269,  2063,   330,
     192,   330,   143,   192,  1488,  1485,  1072,   330,   302,   295,
     321,   322,   323,   324,   325,   326,  1175,    17,   330,  1178,
    1179,   332,  1502,    16,   183,   330,   330,     8,    74,   330,
     106,  1511,   192,    17,    17,   194,  1200,   269,   289,  1203,
     236,   300,  1506,  1507,  1528,  2153,  1530,   330,   335,   293,
     330,  2095,  1118,   267,   267,   369,   370,  1541,   328,   244,
     335,   332,   376,   331,   125,   330,   267,   330,   330,  1553,
     111,   153,   269,   387,   335,   331,   139,   272,   335,  1563,
     193,   187,   155,   331,   398,   399,   184,   330,   147,   248,
     314,   405,   147,   317,   318,   319,   330,   321,   322,   323,
     324,   325,   326,   184,   269,   419,   190,    32,   332,   262,
     143,   314,   223,   190,    16,   269,   106,   369,   321,   322,
     323,   324,   325,   326,     9,  1717,   317,   269,  1292,   332,
     269,  1615,  1684,   269,   328,  1201,   192,  1599,   196,  1205,
    1206,   455,  1301,    17,  1606,   328,   117,    17,   462,   228,
     119,   119,   106,   331,  2219,   314,   470,  1619,  1642,   335,
    1622,   331,   321,   322,   323,   324,   325,   326,   335,   262,
      55,   335,   167,   332,   111,  1339,   331,   111,   111,   331,
     331,  1665,   496,    68,  1664,   499,   331,   331,   331,   335,
    1670,   331,   307,  1673,   331,   509,   184,   331,   331,   125,
      85,   331,  1686,   455,   307,   331,   331,  1664,   331,  1373,
     462,  1695,   331,  1670,  1694,   331,  1673,   143,   470,  2282,
     307,  1705,   111,   330,   269,  1384,   330,    52,   113,   331,
     217,   116,   223,  1717,   330,   330,   325,  1694,   223,   269,
     554,   269,   204,   116,   496,   204,   285,   499,   331,   563,
     564,   143,   143,   331,   180,   569,   143,   509,  1324,   328,
     331,   237,  1814,  1329,   149,   579,   331,   168,   194,   192,
     317,   279,   298,   262,   200,   269,   330,   289,   592,  2344,
     331,   192,   335,   331,    96,  1805,    96,   331,   328,    92,
     111,   106,    40,   331,  1814,   332,   331,   331,   330,   613,
     614,   330,   616,   116,   618,   269,   620,   331,   331,   194,
     335,   563,   331,   279,   269,   143,   630,   143,  1780,   329,
     329,   106,   129,   330,   147,  1805,   330,   196,  2259,   330,
     111,  1815,   292,   115,  1814,  1819,  1820,   187,   330,   653,
     146,  2297,   199,  1807,  1808,  1809,  1810,   146,    32,   331,
     235,  2307,   331,   331,  2455,   240,   331,   330,   118,   331,
     330,   613,   614,  1522,  1848,  1849,   331,   184,   620,  1528,
     184,    81,   330,    76,   130,   113,   329,   262,   113,   111,
     289,  1865,  1866,    88,  1868,    97,   331,    75,   314,   274,
      75,   317,   318,   319,   330,   321,   322,   323,   324,   325,
     326,   653,  2358,   288,   148,  1867,   192,   111,   238,  1871,
    1872,   184,  1874,  1572,   269,  1877,  1878,  1581,    17,  1578,
    1579,   195,   330,   330,   146,   330,   220,  1911,  1912,    17,
    1914,   184,   163,   216,   192,   190,   190,   330,   335,  1505,
     192,  1600,   184,    97,   330,  1511,   106,  1909,  1910,  1933,
      17,  1935,   192,    51,  1938,   133,  1940,  1941,   184,    88,
     133,    84,   192,  1622,  1948,   192,   330,   232,  1627,  1628,
     784,    79,   335,   136,   233,   286,   233,    97,     6,   216,
     331,  1965,  1641,   330,  1310,  1947,   219,   507,  1652,   904,
    1528,  1527,   777,  1582,  1935,  1186,  1859,  2049,  1938,  1663,
    2120,  2053,  1550,  1615,  2399,  2057,  2423,  2380,  2379,  2256,
    2404,  2330,  1940,  1677,   905,  1679,  2081,  2144,  2485,   833,
     391,  1304,  2459,  2102,  2101,  1961,    14,  2436,  1621,  2486,
     894,    26,   784,    21,   463,   477,   654,   973,    26,  1705,
    1965,  2340,  1311,  1560,  1668,  2159,  2317,  1008,  2434,  2449,
     662,  1715,  2484,  1526,  1118,    19,   641,  2143,  1695,  2043,
    1536,   641,  2046,  1722,   804,   803,  1545,  1115,  1814,  2049,
    2305,  1730,  2055,  2053,  1933,  1291,  2060,  2057,  1009,  2434,
    1779,  1070,  1648,  1386,  1485,   869,  1047,  2071,  1654,  1753,
    1530,  2172,  1968,  2077,   561,   617,  2148,  1045,  2082,  1665,
      -1,    -1,    -1,  1767,    -1,    -1,  2068,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   929,   930,  1683,  1684,    -1,
     583,   935,    -1,    -1,    -1,    -1,    -1,  2089,    -1,  2091,
      -1,    -1,  1796,   121,    -1,   949,    -1,   125,   952,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   962,    -1,
      -1,  1717,  2204,    -1,    -1,   143,   144,  2209,    -1,   973,
    2144,  2213,    -1,   626,  2148,    -1,   154,    -1,    -1,  2153,
      -1,    -1,    -1,    -1,    -1,  2267,    -1,   929,   930,    -1,
      -1,    -1,    -1,  2167,    -1,    -1,    -1,  2149,    -1,    -1,
    2242,    -1,   180,    -1,    -1,   183,   659,    -1,    -1,    -1,
     952,    -1,    -1,    -1,    -1,    -1,   194,    -1,   196,    -1,
     962,    -1,    -1,    -1,    -1,    -1,  2200,    -1,  2202,    -1,
      -1,   973,    -1,    -1,  2204,    -1,    -1,  1886,    -1,  2209,
    2214,    -1,  2216,  2213,    -1,  1894,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1812,  1813,  1814,  2341,
    1909,    -1,  2304,    -1,    -1,    -1,    -1,    -1,  1072,    -1,
     248,    -1,    -1,    -1,  1928,    -1,  2250,  2251,    -1,    -1,
      -1,    -1,    -1,  2235,    -1,    -1,    -1,  2329,    -1,    -1,
      -1,    -1,    -1,  2267,    -1,    -1,    -1,  2271,  2340,     7,
      -1,    -1,    -1,    -1,    -1,    13,    14,    -1,    -1,    -1,
      18,    -1,    20,  2265,  1118,    -1,    -1,    -1,    26,    -1,
      28,    -1,    -1,  2297,    -1,    -1,    -1,    -1,    -1,    -1,
    1072,    -1,    -1,  2307,  2304,    -1,   314,    -1,    -1,   317,
     318,   319,    -1,   321,   322,   323,   324,   325,   326,    -1,
      -1,  1907,   805,    -1,   332,    -1,    -1,   335,    -1,    -1,
      -1,    -1,   815,    -1,    -1,  1921,  2340,  2341,    -1,    -1,
      -1,    -1,  2026,  1929,    -1,    -1,  1118,    -1,   143,    -1,
      -1,   124,    -1,    -1,  2358,   128,    -1,    -1,    -1,    -1,
      -1,  2433,    -1,  2345,    -1,    -1,    -1,  1201,    -1,    -1,
      -1,  1205,  1206,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  2386,    -1,    -1,    -1,   869,    -1,    -1,    -1,
      -1,    -1,   130,    -1,   132,    -1,    -1,   135,  2470,   194,
      -1,    -1,  2081,    -1,    -1,    -1,    -1,    -1,    -1,   892,
    2414,    21,    -1,    -1,    -1,  2419,    26,    -1,  2400,  2401,
      -1,  2461,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    2434,    -1,  2414,    -1,  1415,  1416,  1417,  1418,   921,  1420,
      -1,    -1,    -1,    -1,    -1,   928,    -1,   194,    -1,  2449,
      -1,  2037,  2038,  2039,  2040,  2459,    -1,    -1,    -1,    -1,
      -1,  2461,    -1,  2049,    -1,    -1,    -1,  2053,    -1,    -1,
      -1,  2057,    -1,    -1,    -1,    14,    -1,  2459,    -1,    -1,
      -1,  2485,    21,    93,  2484,    -1,    -1,    26,    -1,    -1,
    1324,    -1,    -1,    -1,    -1,  1329,    -1,    -1,    -1,  2183,
      -1,    -1,  1483,  2485,    -1,  2189,    -1,    -1,    -1,    -1,
      -1,   121,    -1,    -1,    -1,   125,    -1,   583,    -1,   314,
      -1,    -1,   317,   318,   319,    -1,   321,   322,   323,   324,
     325,   326,    -1,   143,   144,    -1,    -1,   332,    -1,    -1,
      -1,    -1,    -1,    -1,   154,    -1,    -1,    -1,    -1,    -1,
    2229,    -1,  1324,    -1,    -1,    -1,    -1,  1329,  2144,    -1,
     626,    -1,  2148,    -1,    -1,    -1,    -1,   314,    -1,    -1,
     317,   318,   319,   183,   321,   322,   323,   324,   325,   326,
      -1,    -1,   121,    -1,   194,  1068,   125,    -1,    -1,    -1,
      -1,    -1,    -1,   659,    -1,    -1,    -1,   370,    -1,    -1,
      -1,    -1,    -1,    -1,   143,   144,  2290,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   154,    -1,    -1,  2204,    -1,
      -1,    -1,    -1,  2209,    -1,  2309,   399,  2213,    -1,    -1,
      -1,    -1,   405,   371,    -1,    -1,    -1,    -1,   248,    -1,
      -1,   180,    -1,    -1,   183,    -1,    -1,   385,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   194,  2242,   395,  2244,    -1,
      -1,    -1,   400,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1505,   410,  2259,    -1,    -1,   414,  1511,    -1,    -1,
      -1,  2267,  1165,    -1,    -1,  2271,    -1,    -1,    -1,   143,
      21,    -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,  2285,
      -1,    -1,    -1,    -1,   314,   771,    -1,    -1,   481,   248,
      -1,   321,   322,   323,   324,   325,   326,  1200,  2304,    -1,
    1203,    -1,   332,    -1,    -1,    -1,    -1,  2411,  2314,   467,
    2409,  2410,    -1,  1505,    -1,    -1,    -1,    -1,    -1,   805,
     194,    -1,    -1,  2329,    -1,    -1,    -1,    -1,    -1,   815,
      -1,    -1,    -1,    -1,  2340,  2341,    -1,    -1,    -1,    -1,
    1741,    -1,    93,  1744,  1745,  1746,  1747,  1748,  1749,  1750,
    1751,  1752,    -1,  1754,    -1,   314,    -1,    -1,   317,   318,
     319,    -1,   321,   322,   323,   324,   325,   326,    -1,  2473,
     121,   564,    -1,   332,   125,    -1,   569,    -1,    -1,    -1,
    2386,    -1,    -1,   869,    -1,  1786,  1787,    -1,    -1,  1292,
     583,    -1,   143,   144,  1648,    -1,    -1,    -1,    14,    -1,
    1654,    -1,    -1,   154,    -1,    21,   892,    -1,   566,    -1,
      26,  1665,    -1,  2419,   572,    -1,    -1,    -1,   576,    -1,
      -1,    -1,    -1,    -1,    -1,   618,   584,  2433,    -1,  1683,
    1684,    -1,   183,   626,    -1,   921,  1339,    -1,    -1,    -1,
     314,    -1,   928,   194,   318,   319,   639,   321,   322,   323,
     324,   325,   326,    -1,    -1,    -1,  1648,    -1,   332,    -1,
      -1,   619,  1654,  1717,  2470,    -1,   659,    -1,    -1,    -1,
    1373,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     673,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1683,    -1,   651,    -1,    -1,    -1,   248,    -1,    -1,
     658,    -1,    -1,    -1,   697,   121,    -1,    -1,    -1,   125,
      -1,    -1,    -1,    -1,  1915,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,   144,    -1,
     723,   724,    -1,   726,   727,   728,    -1,    -1,   154,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   744,    -1,    -1,    -1,    -1,    -1,    -1,  1812,  1813,
    1814,    -1,    -1,   314,   180,    -1,    -1,   183,    -1,    -1,
     321,   322,   323,   324,   325,   326,    -1,    -1,   194,    -1,
     196,   332,  1068,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   798,    -1,    -1,   801,   802,
      -1,    -1,   805,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1812,  1813,   815,    -1,    -1,    -1,    -1,    -1,    -1,  2030,
      -1,    -1,   248,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1907,    -1,    -1,    -1,  1560,    -1,   852,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1921,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1929,   869,    -1,  1581,  1165,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   314,   892,
      -1,   317,   318,   319,    -1,   321,   322,   323,   324,   325,
     326,    -1,    -1,   329,  1200,  1907,   332,  1203,    -1,  1622,
     336,    -1,    -1,    -1,    -1,    -1,   919,    -1,   921,  1921,
      -1,    -1,    -1,    -1,    -1,   928,    -1,  1929,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1652,
      -1,    -1,    -1,    -1,    -1,    -1,   949,    -1,    -1,    -1,
    1663,    -1,    -1,    -1,    -1,  1668,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1677,    -1,  1679,    -1,    -1,    -1,
    2181,   974,   975,  2037,  2038,  2039,  2040,    -1,    -1,   982,
      -1,    -1,   985,   986,    -1,  2049,    -1,    -1,    -1,  2053,
      -1,    14,    -1,  2057,    17,    -1,  1292,    -1,    21,    -1,
    1003,    -1,  1715,    26,  1007,  1008,  1009,  1010,    -1,    -1,
      -1,    -1,    -1,    -1,  1017,  1018,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  2037,  2038,  2039,  2040,    -1,
    1753,  1044,  1045,  1339,  1047,  1048,    -1,    -1,    -1,  1052,
      -1,    -1,    -1,  1056,  1767,    -1,  1059,  1060,  1061,  1062,
    1063,  1064,  1065,  1066,  1067,  1068,  1069,    -1,    -1,    -1,
    1073,    -1,    -1,  1076,    -1,    -1,    -1,  1373,    -1,    -1,
    2144,    -1,    -1,  1796,  2148,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
      -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1080,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,   144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    2204,    -1,    -1,    -1,    -1,  2209,    -1,    -1,    21,  2213,
      -1,    -1,    -1,    26,  1157,    -1,    -1,   180,   583,    -1,
     183,    -1,  1165,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   194,    -1,   196,    -1,  1143,    -1,    -1,  2242,  1147,
    2244,    -1,    -1,    -1,    -1,  1153,   583,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  2259,    -1,  1200,   583,    -1,
    1203,   626,    -1,  2267,  1172,    -1,  1174,  2271,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1928,    -1,    -1,    -1,    -1,
    1188,  2285,  1190,    -1,  1192,   248,    -1,    -1,    -1,   626,
      -1,    -1,    -1,    -1,   659,    -1,    -1,    -1,    -1,    -1,
    2304,   626,  2244,    -1,    -1,    -1,    -1,    -1,   121,    -1,
    2314,    -1,   125,    -1,    -1,    -1,    -1,  2259,    -1,    -1,
      -1,    -1,   659,    -1,  1560,  2329,    -1,    -1,    -1,    -1,
     143,   144,    -1,    -1,   659,    -1,  2340,  2341,    -1,    -1,
      -1,   154,    -1,  2285,    -1,  1581,    -1,  1290,    -1,  1292,
      -1,   314,    -1,    -1,   317,   318,   319,    -1,   321,   322,
     323,   324,   325,   326,    -1,    -1,    -1,    -1,    -1,   332,
     183,    -1,  2314,  2026,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   194,  2386,    -1,    -1,    -1,  1622,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1339,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1351,    -1,
      -1,    -1,    -1,    -1,    -1,  2419,  1652,    -1,    -1,  1362,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1663,    -1,  2433,
    1373,    -1,  1668,    -1,    -1,   248,    -1,    -1,   583,    -1,
     805,  1677,  1385,  1679,    -1,    -1,    -1,    -1,    -1,    -1,
     815,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  2470,    -1,   805,    -1,
      -1,    -1,  1415,  1416,  1417,  1418,  1419,  1420,   815,  1715,
     805,   626,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     815,    -1,  1435,  1436,  1437,  1438,    -1,    -1,    -1,  1442,
      -1,   314,    -1,  1446,   869,   318,   319,    -1,   321,   322,
     323,   324,   325,   326,   659,  1458,    -1,  1753,    -1,   332,
    1463,    -1,    -1,    -1,    -1,    -1,    -1,   892,    -1,  1472,
    2183,  1767,   869,    -1,    -1,    -1,  2189,    -1,    -1,    -1,
    1483,  1484,    -1,  1486,   869,    -1,    -1,  1490,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   892,   921,    -1,    -1,    -1,
    1796,    -1,    -1,   928,    -1,    -1,    -1,   892,  1511,    -1,
    1513,  1514,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   921,    -1,    -1,    -1,    -1,    -1,
      -1,   928,    -1,  1536,    -1,    -1,   921,    -1,    -1,    -1,
    1543,  1544,  1545,   928,    -1,    -1,  1549,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    21,    -1,  1560,    -1,    -1,
      26,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   583,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  2290,  1581,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1563,  2309,    -1,    -1,    -1,
     805,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     815,   626,    -1,   583,    -1,    -1,    -1,    -1,  1621,  1622,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1928,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1068,   659,    -1,    -1,    -1,    -1,  1652,
      -1,    -1,    -1,    -1,    -1,   121,   626,    -1,    -1,   125,
    1663,    -1,    -1,    -1,   869,  1668,    -1,    -1,    -1,    -1,
      -1,  1068,    -1,    -1,  1677,    -1,  1679,   143,   144,    -1,
      -1,  1684,    -1,  1068,    -1,    -1,    -1,   892,   154,   659,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1665,  2411,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1712,
      -1,    -1,  1715,    -1,   180,    -1,   921,   183,    -1,    -1,
      -1,  1724,    -1,   928,    -1,  1728,    -1,    -1,   194,  1732,
    2026,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1741,    -1,
    1165,  1744,  1745,  1746,  1747,  1748,  1749,  1750,  1751,  1752,
    1753,  1754,    -1,    -1,    -1,  1758,  1759,    -1,    -1,    -1,
    2473,    -1,    -1,    -1,  1767,    -1,    -1,    -1,  1165,  1772,
      -1,    -1,    -1,    -1,    -1,  1200,    -1,    -1,  1203,    -1,
    1165,    -1,   248,  1786,  1787,  1788,    -1,  1790,    -1,    -1,
     805,    -1,    -1,  1796,  1797,    -1,    -1,    -1,    -1,  1802,
     815,    -1,  1805,  1200,    -1,    -1,  1203,    -1,    -1,    -1,
      -1,  1814,    -1,    -1,    -1,  1200,     9,    -1,  1203,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1837,   805,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,   815,    -1,    -1,   314,    -1,
      -1,   317,   318,   319,   869,   321,   322,   323,   324,   325,
     326,    -1,    55,  1068,    -1,    -1,   332,  1292,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    68,    -1,   892,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  2183,    -1,    -1,
      -1,    -1,    85,  2189,    -1,  1292,    -1,  1865,  1866,   869,
      -1,    -1,    -1,    -1,    -1,    -1,   921,  1292,    -1,    -1,
      -1,    -1,  1915,   928,  1339,    -1,    -1,    -1,    -1,    -1,
     113,    -1,   892,   116,    -1,  1928,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,
      -1,    -1,  1339,  1911,  1912,    -1,  1914,    -1,  1373,    -1,
     583,   921,    -1,    -1,  1339,    -1,    -1,    -1,   928,    -1,
    1165,    -1,    -1,    -1,    -1,  1968,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1941,  1977,    -1,  1373,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1373,    -1,
      -1,    -1,    -1,   626,  2290,  1200,    -1,    -1,  1203,    -1,
      -1,   194,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  2015,  2309,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   215,  2026,    -1,    -1,   659,  2030,  2031,  2032,
      -1,    -1,  2035,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   234,   235,    -1,    -1,    -1,  2049,   240,    -1,    -1,
    2053,    -1,    -1,  1068,  2057,    -1,    -1,    -1,    -1,    -1,
    2063,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   262,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   274,    -1,    -1,    -1,    -1,   279,  1292,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   288,    -1,    -1,  1068,    -1,
      -1,    -1,    -1,  2071,    -1,   298,    -1,    -1,    -1,  2077,
      -1,    -1,    -1,    -1,    -1,  2411,  2119,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1339,  1560,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  2148,    -1,    -1,    -1,    -1,
    1165,    -1,    -1,    -1,    -1,    -1,  1581,    -1,    -1,    -1,
      -1,    -1,    -1,  1560,    -1,    -1,    -1,    -1,  1373,  2172,
      -1,    -1,   805,    -1,    -1,  1560,  2144,  2473,  2181,    -1,
    2183,    -1,   815,    -1,  1581,  1200,  2189,    -1,  1203,    -1,
    2193,    -1,    -1,    -1,    -1,  1165,  1581,  1622,    -1,    -1,
      -1,  2204,    -1,    -1,    -1,    -1,  2209,    -1,    -1,    -1,
    2213,    -1,    -1,    -1,    -1,    -1,  2219,  2220,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1622,    -1,  1652,    -1,    -1,
    1200,    -1,    -1,  1203,    -1,    -1,   869,  1622,  1663,  2242,
      -1,    -1,    -1,  1668,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1677,    -1,  1679,  1652,    -1,    -1,    -1,   892,
      -1,    -1,    -1,    -1,    -1,  2268,  1663,  1652,    -1,    -1,
      -1,  1668,    -1,    -1,    -1,    -1,    -1,  1292,  1663,    -1,
    1677,    -1,  1679,  1668,    -1,    -1,    -1,  2290,   921,    -1,
    1715,    -1,  1677,    21,  1679,   928,    -1,  2300,    26,    -1,
      -1,  2304,    -1,    -1,    -1,    -1,  2309,    -1,    -1,    -1,
      -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,  1715,    -1,
      21,    -1,  1292,    -1,  1339,    26,  2329,    -1,  1753,    -1,
    1715,    -1,    -1,    -1,    -1,    -1,    -1,  2340,    -1,    -1,
      -1,  2344,  1767,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1560,  1753,    -1,  1373,    -1,
      -1,    -1,    -1,    -1,    -1,    93,    -1,    -1,  1753,  1339,
    1767,  1796,    -1,    -1,    -1,    -1,  1581,    -1,    -1,    -1,
      -1,    -1,  1767,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   121,    -1,    -1,    -1,   125,    -1,  1796,
      -1,    -1,    -1,  1373,    -1,    -1,    -1,    -1,  2411,    -1,
      -1,  1796,    -1,    -1,    -1,   143,   144,  1622,    -1,    -1,
     121,    -1,    -1,    -1,   125,    -1,   154,    -1,    -1,    -1,
    2433,    -1,    -1,    -1,    -1,  1068,    -1,    -1,    -1,    -1,
      -1,    -1,   143,   144,    -1,    -1,    -1,  1652,    -1,    -1,
      -1,    -1,    -1,   154,    -1,   183,    -1,    -1,  1663,    -1,
      -1,    -1,    -1,  1668,    -1,    -1,   194,  2470,    -1,    -1,
    2473,    -1,  1677,    -1,  1679,    -1,    -1,    -1,    -1,   180,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   194,    -1,   196,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1928,    -1,    -1,    -1,    -1,    -1,    -1,
    1715,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     248,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1928,  1165,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1928,    -1,  1560,    -1,   248,  1753,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1767,    -1,    -1,    -1,  1581,  1200,    -1,   481,
    1203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   314,    -1,    -1,    -1,
    1560,  1796,    -1,   321,   322,   323,   324,   325,   326,    -1,
      -1,  2026,    -1,    -1,   332,    -1,    -1,  1622,    -1,    -1,
      -1,  1581,    -1,   314,    -1,    -1,   317,   318,   319,    -1,
     321,   322,   323,   324,   325,   326,    -1,    -1,    -1,  2026,
      -1,   332,    -1,    -1,   335,    -1,    -1,  1652,    -1,    -1,
      -1,  2026,    -1,    -1,    -1,    -1,    -1,    -1,  1663,    -1,
      -1,    -1,  1622,  1668,    -1,    -1,    -1,    -1,    -1,  1292,
      -1,    14,  1677,    -1,  1679,    18,    -1,    -1,    21,    -1,
      -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1652,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1663,    -1,    -1,    -1,    -1,  1668,    -1,
    1715,    -1,    -1,    -1,    -1,    -1,  1339,  1677,    -1,  1679,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1928,    -1,    -1,    -1,    -1,    -1,    82,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    -1,  1753,    -1,
    1373,    26,    -1,    -1,    -1,  1715,    -1,    -1,    -1,    -1,
      -1,    -1,  1767,    -1,    -1,    -1,    -1,    -1,  2183,    -1,
      -1,   673,    -1,    -1,  2189,    -1,    -1,    -1,   121,    -1,
      -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1796,    -1,  1753,    -1,   697,  2183,    -1,    -1,    -1,
     143,   144,  2189,    -1,    -1,    -1,    -1,  1767,  2183,    -1,
      -1,   154,    -1,    -1,  2189,    -1,    -1,    -1,    -1,    -1,
      -1,   723,   724,    -1,   726,   727,   728,    -1,    -1,    -1,
      -1,  2026,    -1,    -1,    -1,    -1,  1796,   180,    -1,    -1,
     183,    -1,   744,    -1,    -1,    -1,   121,    -1,    -1,    -1,
     125,   194,    -1,   196,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,   144,
      -1,    -1,    -1,    -1,    -1,  2290,    -1,    -1,    -1,   154,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  2309,    -1,   798,    -1,    -1,   801,
     802,    -1,    -1,  2290,    -1,   248,    -1,    -1,   183,    -1,
      -1,    -1,    -1,    -1,    -1,  2290,    -1,    -1,    -1,   194,
      -1,    -1,  2309,  1928,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  2309,    -1,    -1,  1560,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   289,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   745,    -1,    -1,  1581,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1928,    -1,
      -1,   314,    -1,   248,   317,   318,   319,    -1,   321,   322,
     323,   324,   325,   326,    -1,    -1,    -1,    -1,  2183,   332,
      -1,    -1,    -1,    -1,  2189,    -1,  2411,    -1,    -1,  1622,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   919,    -1,    -1,
      -1,  2026,    -1,    -1,  2411,    -1,    -1,    -1,    -1,  1652,
      -1,    -1,    -1,    -1,    -1,    -1,  2411,    -1,    -1,   314,
    1663,    -1,   317,   318,   319,  1668,   321,   322,   323,   324,
     325,   326,    -1,    -1,  1677,    -1,  1679,   332,  2473,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  2026,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     982,    -1,    -1,    -1,   986,    -1,  2473,    -1,    -1,    -1,
      -1,    -1,  1715,    -1,    -1,  2290,    -1,    -1,  2473,    -1,
      -1,  1003,    -1,    -1,    -1,  1007,    -1,  1009,  1010,    -1,
      -1,    -1,    -1,    -1,  2309,  1017,  1018,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1753,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1044,    -1,  1767,    -1,  1048,   937,    -1,    -1,
    1052,    -1,    -1,    -1,  1056,    -1,    -1,  1059,  1060,  1061,
    1062,  1063,  1064,  1065,  1066,  1067,    -1,  1069,    -1,    -1,
      -1,  1073,    -1,  1796,    -1,    -1,    -1,    -1,  2183,    -1,
      -1,    -1,    -1,    -1,  2189,    -1,    -1,    -1,    -1,    -1,
     980,    -1,    14,    -1,    -1,    -1,    -1,     3,    -1,    21,
      -1,    -1,    -1,    -1,    26,    11,    12,    13,    -1,    -1,
      -1,    -1,  1002,    -1,    -1,    -1,  2411,    -1,  1008,    25,
      -1,    -1,    -1,  2183,    -1,    -1,    -1,    -1,    -1,  2189,
      -1,    -1,  1022,  1023,  1024,  1025,    -1,    43,  1028,    45,
      46,    -1,    -1,    -1,    50,    51,    -1,    -1,    -1,    -1,
      -1,    57,    58,  1043,    -1,  1157,    -1,  1047,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    73,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,  2473,    -1,
      -1,    87,    -1,    -1,   106,  2290,    92,    -1,    -1,   111,
      -1,    97,    -1,    99,    -1,    -1,    -1,   103,    -1,   121,
      -1,    -1,    -1,   125,  2309,  1928,    -1,    -1,    -1,   115,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,   144,    -1,    -1,    -1,    -1,    -1,    -1,   135,
    2290,    -1,   154,    -1,    -1,    -1,    -1,  1127,    -1,    -1,
      -1,    -1,    -1,  1133,    -1,  1135,    -1,    -1,    -1,  2309,
     156,   157,    -1,    -1,    -1,    -1,   162,    -1,   180,    -1,
      -1,   183,    -1,   169,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   194,    -1,   196,    -1,   182,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1290,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  2026,    -1,   211,  2411,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   221,    -1,    -1,    -1,   225,
      -1,    -1,    -1,   229,    -1,   231,   248,    14,    -1,    -1,
      -1,    -1,    -1,   239,    21,    -1,    -1,    -1,   244,    26,
      -1,   247,    -1,    -1,    -1,    -1,    -1,   253,    -1,  1351,
      -1,  2411,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1362,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2473,    -1,
      -1,   277,    -1,    -1,    -1,    -1,    -1,    -1,   284,    -1,
     286,    -1,    -1,  1385,   290,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   314,    -1,    -1,   317,   318,   319,    -1,   321,
     322,   323,   324,   325,   326,    -1,    -1,    -1,    -1,    -1,
     332,    -1,    -1,  2473,    -1,    -1,    -1,  1419,    -1,    -1,
      -1,    -1,    -1,    -1,   330,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1435,   121,  1437,  1438,    -1,   125,    -1,
    1442,    -1,    -1,    -1,  1446,    -1,    -1,  1337,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   143,   144,    -1,    -1,
    2183,    -1,    -1,    -1,    -1,    -1,  2189,   154,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1484,    -1,  1486,    -1,    -1,    -1,  1490,    -1,
      -1,    -1,    -1,   180,    -1,    -1,   183,    -1,  1388,    -1,
    1390,    -1,    -1,    -1,    -1,    -1,    -1,   194,    -1,   196,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1410,    -1,    -1,  1413,    -1,  1415,  1416,  1417,  1418,    -1,
    1420,  1421,    -1,    -1,  1536,    -1,  1426,  1427,    -1,    -1,
      -1,  1543,  1544,  1545,  1434,    -1,    -1,  1549,    -1,  1439,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   248,    -1,    14,    -1,    -1,    17,  2290,    -1,    -1,
      21,    -1,    -1,    -1,    -1,    26,    -1,  1467,    -1,    -1,
    1470,  1471,    -1,    -1,    -1,    -1,  2309,    -1,    -1,    -1,
      -1,    -1,  1482,  1483,    -1,    -1,    -1,    -1,    -1,  1489,
      -1,  1491,  1492,  1493,  1494,  1495,  1496,  1497,  1498,  1499,
      -1,  1501,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   314,    -1,    -1,
     317,   318,   319,    -1,   321,   322,   323,   324,   325,   326,
      -1,    -1,    14,    -1,   331,   332,    -1,    -1,    -1,    21,
      -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1557,    -1,    -1,
     121,    -1,    -1,    -1,   125,    -1,    -1,    -1,    -1,    21,
      -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,  2411,    -1,
      -1,    -1,   143,   144,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   154,    -1,    -1,    -1,    -1,    -1,    -1,
    1712,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1724,    -1,    -1,    -1,  1728,    -1,    -1,   180,
    1732,    -1,   183,  1623,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   194,    -1,   196,    -1,    -1,    -1,   121,
    2473,    93,    -1,   125,    -1,    -1,  1758,  1759,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   143,   144,    -1,    -1,    -1,    -1,    -1,    -1,   121,
      -1,    -1,   154,   125,    -1,    -1,  1788,    -1,  1790,    -1,
      -1,    -1,  1682,    -1,    -1,  1797,    -1,   248,    -1,    -1,
    1802,   143,   144,  1805,    -1,    -1,    -1,    -1,   180,    -1,
      -1,   183,   154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1711,   194,    -1,   196,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1837,    -1,    -1,    -1,    -1,
      -1,   183,    -1,    -1,    -1,  1735,  1736,  1737,  1738,  1739,
    1740,  1741,   194,    -1,  1744,  1745,  1746,  1747,  1748,  1749,
    1750,  1751,  1752,   314,  1754,    -1,   317,   318,   319,    -1,
     321,   322,   323,   324,   325,   326,   248,    -1,    -1,    -1,
      -1,   332,    -1,    -1,  1774,    -1,    -1,  1777,    -1,    -1,
      -1,    -1,    14,    -1,    -1,    -1,    -1,  1787,    -1,    21,
      -1,    -1,    -1,    -1,    26,    -1,   248,    -1,  1798,  1799,
      -1,  1801,    -1,    -1,  1804,    -1,    -1,   289,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   314,    -1,    -1,   317,   318,   319,    -1,   321,
     322,   323,   324,   325,   326,    -1,  1846,    -1,    -1,    -1,
     332,    -1,    -1,    -1,    -1,    -1,  1968,    -1,    -1,    -1,
      -1,    -1,   314,    -1,    -1,  1977,    -1,    -1,    -1,   321,
     322,   323,   324,   325,   326,    -1,    -1,    -1,    -1,    -1,
     332,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
      -1,    -1,    -1,   125,    -1,    -1,    -1,    -1,    14,    -1,
      -1,    -1,    -1,  2015,    -1,    21,    -1,    -1,    -1,    -1,
      26,   143,   144,    -1,    -1,  1915,    -1,    -1,    -1,  2031,
    2032,    -1,   154,  2035,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    14,    -1,    -1,    17,    -1,    -1,    -1,    21,
      -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,   180,    -1,
      -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   194,    -1,   196,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   207,    -1,    -1,  1978,    -1,
      -1,    -1,    -1,  1983,  1984,    -1,  1986,    -1,  1988,    -1,
      -1,    -1,  1992,  1993,  1994,  1995,  1996,  1997,  1998,  1999,
    2000,    -1,  2002,  2003,  2004,   121,    -1,  2119,    -1,   125,
      -1,    -1,    -1,    -1,    -1,    -1,   248,    -1,    -1,    -1,
      -1,    -1,  2022,  2023,    -1,  2025,    -1,   143,   144,  2029,
    2030,    -1,    -1,  2033,    -1,    -1,  2036,    -1,   154,   121,
      -1,    -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    2172,   143,   144,    -1,   180,    -1,    -1,   183,    -1,    -1,
    2070,    -1,   154,    -1,    -1,    -1,    -1,    -1,   194,    -1,
     196,  2193,   314,    -1,    -1,   317,   318,   319,    -1,   321,
     322,   323,   324,   325,   326,    -1,    -1,    -1,   180,    -1,
     332,   183,    -1,    -1,    -1,    -1,    -1,    -1,  2220,    -1,
      -1,    -1,   194,    -1,   196,    -1,    -1,    -1,    -1,    -1,
      -1,  2121,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   248,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  2268,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   248,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  2174,    -1,    -1,    -1,    -1,    -1,
      -1,  2181,    -1,    -1,    -1,  2185,    -1,    -1,  2300,    -1,
    2190,  2191,  2192,    -1,  2194,    -1,    -1,    -1,   314,    -1,
      -1,   317,   318,   319,    -1,   321,   322,   323,   324,   325,
     326,    -1,    -1,    -1,    -1,   331,   332,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   314,    -1,    -1,   317,   318,   319,    -1,   321,
     322,   323,   324,   325,   326,    -1,  2246,    -1,    -1,    -1,
     332,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2289,
      -1,    -1,    -1,    -1,    -1,  2295,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  2312,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,     6,     7,     8,     9,    10,    11,  2338,    -1,
      -1,    -1,    16,    -1,    -1,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,  2357,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    -1,    43,
      44,    45,    46,    47,    -1,    -1,    50,    51,    52,    -1,
      54,    55,    56,    57,    -1,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      -1,    75,    -1,    77,    78,    79,    80,    81,    -1,    83,
      -1,    85,    86,    87,    88,    -1,    90,    91,    -1,    93,
      -1,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,   107,    -1,   109,   110,    -1,   112,   113,
     114,    -1,    -1,   117,    -1,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,    -1,   131,   132,   133,
     134,   135,   136,   137,   138,    -1,   140,    -1,   142,   143,
     144,   145,   146,   147,   148,   149,   150,    -1,   152,   153,
     154,    -1,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,   180,   181,   182,   183,
     184,   185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,
     194,   195,    -1,    -1,   198,   199,   200,   201,   202,   203,
     204,   205,   206,    -1,   208,   209,   210,   211,    -1,   213,
     214,   215,   216,   217,   218,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   236,   237,   238,    -1,   240,   241,   242,   243,
     244,   245,   246,   247,   248,   249,   250,    -1,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,    -1,   263,
     264,   265,    -1,   267,   268,    -1,   270,    -1,   272,   273,
     274,   275,   276,   277,   278,   279,   280,    -1,   282,   283,
     284,   285,   286,   287,   288,    -1,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,    -1,    -1,   302,   303,
     304,   305,   306,   307,    -1,   309,   310,   311,   312,   313,
     314,   315,   316,    -1,    -1,    -1,    -1,   321,   322,   323,
      -1,   325,   326,    -1,    -1,    -1,   330,   331,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    16,    -1,    -1,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    -1,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,   107,    -1,   109,   110,    -1,   112,   113,   114,
      -1,    -1,   117,    -1,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,    -1,   131,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,   143,   144,
     145,   146,   147,   148,   149,   150,    -1,   152,   153,   154,
      -1,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,   194,
     195,    -1,    -1,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,   276,   277,   278,   279,   280,    -1,   282,   283,   284,
     285,   286,   287,   288,    -1,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,   310,   311,   312,   313,   314,
     315,   316,    -1,    -1,    -1,    -1,   321,   322,    -1,    -1,
     325,   326,    -1,    -1,    -1,   330,   331,     3,     4,     5,
       6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,
      16,    -1,    -1,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    -1,    43,    44,    45,
      46,    47,    -1,    -1,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    -1,    75,
      -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,    85,
      86,    87,    88,    -1,    90,    91,    -1,    93,    -1,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,   107,    -1,   109,   110,    -1,   112,   113,   114,    -1,
      -1,   117,    -1,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,    -1,   131,   132,   133,   134,   135,
     136,   137,   138,    -1,   140,    -1,   142,   143,   144,   145,
     146,   147,   148,   149,   150,    -1,   152,   153,   154,    -1,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,    -1,    -1,   190,   191,    -1,    -1,   194,   195,
      -1,    -1,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,   208,   209,   210,   211,    -1,   213,   214,   215,
     216,   217,   218,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,    -1,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,    -1,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,    -1,   263,   264,   265,
      -1,   267,   268,    -1,   270,    -1,   272,   273,   274,   275,
     276,   277,   278,   279,   280,    -1,   282,   283,   284,   285,
     286,   287,   288,    -1,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,    -1,    -1,   302,   303,   304,   305,
     306,   307,    -1,   309,   310,   311,   312,   313,   314,   315,
     316,    -1,    -1,    -1,    -1,   321,   322,    -1,    -1,   325,
     326,    -1,    -1,    -1,   330,   331,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,
      17,    -1,    19,    20,    21,    -1,    23,    24,    25,    -1,
      27,    -1,    29,    30,    -1,    32,    33,    34,    35,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    69,    70,    71,    72,    73,    -1,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    94,    95,    96,
      97,    98,    99,   100,   101,    -1,   103,   104,   105,   106,
     107,    -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,
     117,    -1,   119,   120,    -1,   122,   123,   124,    -1,   126,
     127,   128,   129,    -1,    -1,   132,   133,   134,   135,   136,
     137,   138,   139,   140,    -1,   142,    -1,    -1,   145,    -1,
     147,   148,   149,   150,    -1,    -1,   153,    -1,   155,   156,
     157,   158,    -1,    -1,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,    -1,   173,    -1,   175,   176,
     177,   178,   179,    -1,   181,   182,    -1,    -1,   185,   186,
     187,    -1,   189,   190,    -1,    -1,    -1,   194,   195,    -1,
     197,   198,    -1,    -1,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,    -1,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,    -1,   244,   245,   246,
     247,    -1,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,    -1,
     277,   278,   279,   280,   281,    -1,   283,   284,   285,   286,
     287,    -1,    -1,   290,   291,   292,   293,   294,   295,    -1,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    21,   331,    23,    24,    25,    -1,    27,
      -1,    29,    30,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    -1,
      -1,    -1,    50,    51,    52,    -1,    54,    55,    -1,    57,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    71,    72,    73,    -1,    75,    -1,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    87,
      88,    -1,    90,    91,    -1,    93,    -1,    95,    96,    97,
      -1,    99,   100,    -1,    -1,   103,   104,   105,    -1,   107,
      -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,
      -1,   119,   120,    -1,   122,   123,   124,   125,   126,   127,
     128,   129,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,
     148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,
     158,    -1,    -1,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    -1,   173,    -1,   175,   176,   177,
     178,    -1,    -1,   181,   182,    -1,    -1,    -1,   186,   187,
      -1,    -1,   190,    -1,    -1,    -1,   194,   195,    -1,    -1,
     198,    -1,    -1,    -1,   202,   203,   204,   205,   206,    -1,
      -1,   209,   210,   211,    -1,   213,   214,   215,   216,   217,
     218,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,    -1,   231,    -1,   233,   234,   235,   236,   237,
     238,    -1,   240,   241,   242,    -1,   244,   245,   246,   247,
      -1,   249,   250,    -1,   252,   253,   254,   255,   256,   257,
     258,   259,    -1,   261,    -1,   263,   264,   265,    -1,   267,
     268,    -1,   270,    -1,   272,    -1,   274,    -1,    -1,   277,
     278,   279,   280,    -1,    -1,   283,   284,   285,   286,   287,
      -1,    -1,   290,   291,   292,   293,   294,   295,    -1,   297,
     298,   299,    -1,    -1,   302,   303,   304,   305,   306,   307,
      -1,   309,    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    16,    -1,   331,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,   107,    -1,   109,   110,   111,   112,   113,   114,
      -1,    -1,   117,    -1,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,    -1,   131,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
      -1,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,   194,
     195,    -1,    -1,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,    -1,   282,   283,   284,
     285,   286,   287,   288,    -1,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,   310,   311,   312,   313,   314,
     315,   316,    -1,    -1,    -1,    -1,   321,   322,    -1,    -1,
     325,   326,    -1,    -1,    -1,   330,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    16,
      -1,    -1,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    -1,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,    -1,
     107,    -1,   109,   110,    -1,   112,   113,   114,    -1,    -1,
     117,    -1,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,    -1,   142,   143,   144,   145,   146,
     147,   148,   149,   150,    -1,   152,   153,   154,    -1,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,    -1,    -1,   190,   191,    -1,    -1,   194,   195,    -1,
      -1,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,   276,
     277,   278,   279,   280,    -1,   282,   283,   284,   285,   286,
     287,   288,    -1,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,   310,   311,   312,   313,   314,   315,   316,
      -1,    -1,    -1,    -1,   321,   322,   323,    -1,   325,   326,
      -1,    -1,    -1,   330,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    16,    -1,    -1,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    -1,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,    -1,   107,    -1,
     109,   110,    -1,   112,   113,   114,    -1,    -1,   117,    -1,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,    -1,   131,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,   143,   144,   145,   146,   147,   148,
     149,   150,    -1,   152,   153,   154,    -1,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,    -1,
      -1,   190,   191,    -1,    -1,   194,   195,    -1,    -1,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,   276,   277,   278,
     279,   280,    -1,   282,   283,   284,   285,   286,   287,   288,
      -1,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,   310,   311,   312,   313,   314,   315,   316,    -1,    -1,
      -1,    -1,   321,   322,   323,    -1,   325,   326,    -1,    -1,
      -1,   330,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    16,    -1,    -1,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,   107,    -1,   109,   110,
      -1,   112,   113,   114,    -1,    -1,   117,    -1,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,    -1,
     131,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,   143,   144,   145,   146,   147,   148,   149,   150,
      -1,   152,   153,   154,    -1,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,    -1,    -1,   190,
     191,    -1,    -1,   194,   195,    -1,    -1,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,   276,   277,   278,   279,   280,
      -1,   282,   283,   284,   285,   286,   287,   288,    -1,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,   310,
     311,   312,   313,   314,   315,   316,    -1,    -1,    -1,    -1,
     321,   322,    -1,    -1,   325,   326,    -1,    -1,    -1,   330,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      -1,    -1,    -1,    16,    -1,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    -1,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    50,    51,    52,
      -1,    54,    55,    56,    57,    -1,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    -1,    75,    -1,    77,    78,    79,    80,    81,    -1,
      -1,    -1,    85,    86,    87,    88,    -1,    90,    91,    -1,
      93,    -1,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,    -1,   107,    -1,   109,   110,    -1,   112,
     113,   114,    -1,    -1,   117,    -1,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,    -1,   131,   132,
     133,   134,   135,   136,   137,   138,    -1,   140,    -1,   142,
     143,   144,   145,   146,   147,   148,   149,   150,    -1,   152,
     153,   154,    -1,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,    -1,    -1,   190,   191,    -1,
      -1,   194,   195,    -1,    -1,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    -1,   208,   209,   210,   211,    -1,
     213,   214,   215,   216,   217,   218,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,    -1,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,    -1,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,    -1,
     263,   264,   265,    -1,   267,   268,    -1,   270,    -1,   272,
     273,   274,   275,   276,   277,   278,   279,   280,    -1,   282,
     283,   284,   285,   286,   287,   288,    -1,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,    -1,    -1,   302,
     303,   304,   305,   306,   307,    -1,   309,   310,   311,   312,
     313,   314,   315,   316,    -1,    -1,    -1,    -1,   321,   322,
      -1,    -1,   325,   326,    -1,    -1,    -1,   330,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    16,    -1,    -1,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    -1,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,   107,    -1,   109,   110,    -1,   112,   113,   114,
      -1,    -1,   117,    -1,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,    -1,   131,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,   143,   144,
     145,   146,   147,   148,   149,   150,    -1,   152,   153,   154,
      -1,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,   194,
     195,    -1,    -1,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,   276,   277,   278,   279,   280,    -1,   282,   283,   284,
     285,   286,   287,   288,    -1,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,   310,   311,   312,   313,   314,
     315,   316,    -1,    -1,    -1,    -1,   321,   322,    -1,    -1,
     325,   326,    -1,   328,    -1,   330,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    16,
      -1,    -1,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    -1,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    -1,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,    -1,
     107,    -1,   109,   110,   111,   112,   113,   114,    -1,    -1,
     117,    -1,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,    -1,   142,   143,   144,   145,   146,
     147,   148,   149,   150,    -1,   152,   153,   154,    -1,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,   180,   181,   182,   183,   184,   185,   186,
     187,    -1,    -1,   190,   191,    -1,    -1,   194,   195,    -1,
      -1,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,   276,
     277,   278,   279,   280,    -1,   282,   283,   284,   285,   286,
     287,   288,    -1,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,   310,   311,   312,   313,   314,   315,   316,
      -1,    -1,    -1,    -1,   321,   322,    -1,    -1,   325,   326,
      -1,    -1,    -1,   330,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    16,    -1,    -1,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    -1,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,    -1,   107,    -1,
     109,   110,    -1,   112,   113,   114,    -1,    -1,   117,    -1,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,    -1,   131,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,   143,   144,   145,   146,   147,   148,
     149,   150,    -1,   152,   153,   154,    -1,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,    -1,
      -1,   190,   191,    -1,    -1,   194,   195,    -1,    -1,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,   243,   244,   245,   246,   247,   248,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,   276,   277,   278,
     279,   280,    -1,   282,   283,   284,   285,   286,   287,   288,
      -1,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,   310,   311,   312,   313,   314,   315,   316,    -1,    -1,
      -1,    -1,   321,   322,    -1,    -1,   325,   326,    -1,    -1,
      -1,   330,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    16,    -1,    -1,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,   107,    -1,   109,   110,
      -1,   112,   113,   114,    -1,    -1,   117,    -1,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,    -1,
     131,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,   143,   144,   145,   146,   147,   148,   149,   150,
      -1,   152,   153,   154,    -1,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,    -1,    -1,   190,
     191,    -1,    -1,   194,   195,    -1,    -1,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,    -1,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,   276,   277,   278,   279,   280,
      -1,   282,   283,   284,   285,   286,   287,   288,    -1,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,   310,
     311,   312,   313,   314,   315,   316,    -1,    -1,    -1,    -1,
     321,   322,    -1,    -1,   325,   326,    -1,    -1,    -1,   330,
       3,     4,     5,     6,     7,     8,     9,    -1,    11,    -1,
      -1,    -1,    -1,    16,    -1,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    -1,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    50,    51,    52,
      -1,    54,    55,    56,    57,    -1,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    -1,    75,    -1,    77,    78,    79,    80,    81,    -1,
      -1,    -1,    85,    86,    87,    88,    -1,    90,    91,    -1,
      93,    -1,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,    -1,   107,    -1,   109,   110,    -1,   112,
     113,   114,    -1,    -1,   117,    -1,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,    -1,   131,   132,
     133,   134,   135,   136,   137,   138,    -1,   140,    -1,   142,
     143,   144,   145,   146,   147,   148,   149,   150,    -1,   152,
     153,   154,    -1,   156,   157,   158,   159,   160,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,   172,
     173,   174,   175,   176,   177,   178,   179,    -1,   181,   182,
     183,   184,   185,   186,   187,    -1,    -1,   190,   191,    -1,
      -1,   194,   195,    -1,    -1,   198,   199,   200,   201,   202,
     203,   204,   205,   206,    -1,   208,   209,   210,   211,    -1,
     213,   214,   215,   216,   217,   218,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,   230,   231,   232,
     233,   234,   235,   236,   237,   238,    -1,   240,   241,   242,
     243,   244,   245,   246,   247,   248,   249,   250,    -1,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,    -1,
     263,   264,   265,    -1,   267,   268,    -1,   270,    -1,   272,
     273,   274,   275,   276,   277,   278,   279,   280,    -1,    -1,
     283,   284,   285,   286,   287,   288,    -1,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   299,    -1,    -1,   302,
     303,   304,   305,   306,   307,    -1,   309,   310,   311,   312,
     313,   314,   315,   316,    -1,    -1,    -1,    -1,   321,   322,
      -1,    -1,   325,   326,    -1,    -1,    -1,   330,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    16,    -1,    -1,    19,    20,    21,    22,    23,    24,
      25,    -1,    27,    28,    29,    30,    -1,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    -1,    -1,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,   107,    -1,   109,   110,    -1,   112,   113,   114,
      -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,   124,
      -1,   126,   127,   128,   129,    -1,   131,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,   143,   144,
     145,   146,   147,   148,   149,   150,    -1,   152,   153,    -1,
      -1,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,    -1,   181,   182,   183,   184,
     185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,   194,
     195,    -1,    -1,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,   243,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,   276,   277,   278,   279,   280,    -1,   282,   283,   284,
     285,   286,   287,   288,    -1,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,   310,   311,   312,   313,   314,
     315,   316,    -1,    -1,    -1,    -1,   321,   322,    -1,    -1,
     325,   326,    -1,    -1,    -1,   330,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    16,
      -1,    -1,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    -1,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    -1,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,    -1,
     107,    -1,   109,   110,    -1,   112,   113,   114,    -1,    -1,
     117,    -1,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,    -1,   142,   143,   144,   145,   146,
     147,   148,   149,   150,    -1,   152,   153,   154,    -1,   156,
     157,   158,   159,   160,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,    -1,   181,   182,   183,   184,   185,   186,
     187,    -1,    -1,   190,   191,    -1,    -1,   194,   195,    -1,
      -1,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,   276,
     277,   278,   279,   280,    -1,    -1,   283,   284,   285,   286,
     287,   288,    -1,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,   310,   311,   312,   313,    -1,   315,   316,
      -1,     3,     4,     5,     6,     7,     8,     9,    -1,    11,
      -1,    -1,    -1,   330,    16,    -1,    -1,    19,    20,    21,
      22,    23,    24,    25,    -1,    27,    28,    29,    30,    -1,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    -1,    -1,    50,    51,
      52,    -1,    54,    55,    56,    57,    -1,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    -1,    75,    -1,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,    91,
      -1,    -1,    -1,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,   107,    -1,   109,   110,    -1,
     112,   113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,
     122,   123,   124,    -1,   126,   127,   128,   129,    -1,   131,
     132,   133,   134,   135,   136,   137,   138,    -1,   140,    -1,
     142,   143,   144,   145,   146,   147,   148,   149,   150,    -1,
     152,   153,    -1,    -1,   156,   157,   158,   159,   160,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   179,    -1,   181,
     182,   183,   184,   185,   186,   187,    -1,    -1,   190,   191,
      -1,    -1,   194,   195,    -1,    -1,   198,   199,   200,   201,
     202,   203,   204,   205,   206,    -1,   208,   209,   210,   211,
      -1,   213,   214,   215,   216,   217,   218,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   237,   238,    -1,   240,   241,
     242,   243,   244,   245,   246,   247,    -1,   249,   250,    -1,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
      -1,   263,   264,   265,    -1,   267,   268,    -1,   270,    -1,
     272,   273,   274,   275,   276,   277,   278,   279,   280,    -1,
      -1,   283,   284,   285,   286,   287,   288,    -1,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,    -1,    -1,
     302,   303,   304,   305,   306,   307,    -1,   309,   310,   311,
     312,   313,   314,   315,   316,    -1,    -1,    -1,    -1,   321,
     322,    -1,    -1,   325,   326,    -1,    -1,    -1,   330,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    -1,    -1,
      -1,    15,    16,    -1,    -1,    19,    20,    21,    22,    23,
      24,    25,    -1,    27,    28,    29,    30,    -1,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    -1,    43,
      44,    45,    46,    47,    -1,    -1,    50,    51,    52,    -1,
      54,    55,    56,    57,    -1,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      -1,    75,    -1,    77,    78,    79,    80,    81,    -1,    -1,
      -1,    85,    86,    87,    88,    -1,    90,    91,    -1,    -1,
      -1,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,   107,    -1,   109,   110,    -1,   112,   113,
     114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,
     124,    -1,   126,   127,   128,   129,    -1,   131,   132,   133,
     134,   135,   136,   137,   138,    -1,   140,    -1,   142,    -1,
      -1,   145,   146,   147,   148,   149,   150,    -1,   152,   153,
      -1,    -1,   156,   157,   158,   159,   160,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,    -1,   181,   182,    -1,
     184,   185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,
      -1,   195,    -1,    -1,   198,   199,   200,   201,   202,   203,
     204,   205,   206,    -1,   208,   209,   210,   211,    -1,   213,
     214,   215,   216,   217,   218,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,   231,   232,   233,
     234,   235,   236,   237,   238,    -1,   240,   241,   242,   243,
     244,   245,   246,   247,    -1,   249,   250,   251,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,    -1,   263,
     264,   265,    -1,   267,   268,    -1,   270,    -1,   272,   273,
     274,   275,   276,   277,   278,   279,   280,    -1,   282,   283,
     284,   285,   286,   287,   288,    -1,   290,   291,   292,   293,
     294,   295,   296,   297,   298,   299,    -1,    -1,   302,   303,
     304,   305,   306,   307,    -1,   309,   310,   311,   312,   313,
      -1,   315,   316,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   326,    -1,    -1,    -1,   330,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    -1,    -1,    15,
      16,    -1,    -1,    19,    20,    21,    22,    23,    24,    25,
      -1,    27,    28,    29,    30,    -1,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    -1,    43,    44,    45,
      46,    47,    -1,    -1,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    -1,    75,
      -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,    85,
      86,    87,    88,    -1,    90,    91,    -1,    -1,    -1,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
      -1,   107,    -1,   109,   110,    -1,   112,   113,   114,    -1,
      -1,   117,    -1,   119,   120,    -1,   122,   123,   124,    -1,
     126,   127,   128,   129,    -1,   131,   132,   133,   134,   135,
     136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,   145,
     146,   147,   148,   149,   150,    -1,   152,   153,    -1,    -1,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,    -1,   181,   182,    -1,   184,   185,
     186,   187,    -1,    -1,   190,   191,    -1,    -1,    -1,   195,
      -1,    -1,   198,   199,   200,   201,   202,   203,   204,   205,
     206,    -1,   208,   209,   210,   211,    -1,   213,   214,   215,
     216,   217,   218,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,    -1,   240,   241,   242,   243,   244,   245,
     246,   247,    -1,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,    -1,   263,   264,   265,
      -1,   267,   268,    -1,   270,    -1,   272,   273,   274,   275,
     276,   277,   278,   279,   280,    -1,   282,   283,   284,   285,
     286,   287,   288,    -1,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,    -1,    -1,   302,   303,   304,   305,
     306,   307,    -1,   309,   310,   311,   312,   313,    -1,   315,
     316,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,   330,    16,    -1,    -1,    19,    20,
      21,    22,    23,    24,    25,    -1,    27,    28,    29,    30,
      -1,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    -1,    -1,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,   107,    -1,   109,   110,
      -1,   112,   113,   114,    -1,    -1,   117,    -1,   119,   120,
      -1,   122,   123,   124,    -1,   126,   127,   128,   129,    -1,
     131,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,    -1,    -1,   145,   146,   147,   148,   149,   150,
      -1,   152,   153,    -1,    -1,   156,   157,   158,   159,   160,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,    -1,
     181,   182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,
     191,    -1,    -1,    -1,   195,    -1,    -1,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,    -1,   240,
     241,   242,   243,   244,   245,   246,   247,    -1,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,   276,   277,   278,   279,   280,
      -1,    -1,   283,   284,   285,   286,   287,   288,    -1,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,   310,
     311,   312,   313,    -1,   315,   316,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,   326,    11,    -1,    -1,   330,
      -1,    16,    -1,    -1,    19,    20,    21,    22,    23,    24,
      25,    -1,    27,    28,    29,    30,    -1,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    -1,    -1,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,   107,    -1,   109,   110,    -1,   112,   113,   114,
      -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,   124,
      -1,   126,   127,   128,   129,    -1,   131,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,
     145,   146,   147,   148,   149,   150,    -1,   152,   153,    -1,
      -1,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,    -1,   181,   182,    -1,    -1,
     185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,    -1,
     195,    -1,    -1,   198,   199,   200,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,   243,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,   276,   277,   278,   279,   280,    -1,    -1,   283,   284,
     285,   286,   287,   288,    -1,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,   310,   311,   312,   313,    -1,
     315,   316,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,   330,    -1,    -1,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      -1,    32,    33,    34,    35,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      61,    -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    98,    99,   100,
     101,    -1,   103,   104,   105,    -1,   107,    -1,   109,   110,
      -1,   112,   113,   114,    -1,    -1,   117,    -1,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,    -1,
     131,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,   143,   144,   145,   146,   147,   148,   149,   150,
      -1,   152,   153,   154,    -1,   156,   157,   158,    -1,    -1,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   172,   173,   174,   175,   176,   177,   178,   179,    -1,
     181,   182,   183,    -1,   185,   186,   187,    -1,    -1,   190,
     191,    -1,   193,   194,   195,    -1,    -1,   198,   199,   200,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,   230,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,    -1,   244,   245,   246,   247,   248,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,    -1,   277,   278,   279,   280,
      -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,   290,
     291,   292,   293,   294,   295,   296,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,    -1,
      -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,   330,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    -1,    32,    33,    34,    35,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    61,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,    -1,   103,   104,   105,    -1,   107,    -1,
     109,   110,    -1,   112,   113,   114,    -1,    -1,   117,    -1,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,    -1,   131,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,   143,   144,   145,   146,   147,   148,
     149,   150,    -1,   152,   153,   154,    -1,   156,   157,   158,
      -1,    -1,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,    -1,   181,   182,   183,    -1,   185,   186,   187,    -1,
      -1,   190,   191,    -1,   193,   194,   195,    -1,    -1,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,    -1,   244,   245,   246,   247,   248,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,    -1,   277,   278,
     279,   280,    -1,    -1,   283,   284,   285,   286,   287,    -1,
      -1,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,
      -1,   330,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    -1,    32,    33,    34,    35,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    61,    -1,    -1,    -1,    -1,    66,
      67,    68,    69,    70,    71,    72,    73,    -1,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    -1,    95,    96,
      97,    98,    99,   100,   101,    -1,   103,   104,   105,    -1,
     107,    -1,   109,   110,    -1,   112,   113,   114,    -1,    -1,
     117,    -1,   119,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,    -1,   131,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,    -1,   142,   143,   144,   145,   146,
     147,   148,   149,   150,    -1,   152,   153,   154,    -1,   156,
     157,   158,    -1,    -1,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,   172,   173,   174,   175,   176,
     177,   178,   179,    -1,   181,   182,   183,    -1,   185,   186,
     187,    -1,    -1,   190,   191,    -1,    -1,   194,   195,    -1,
      -1,   198,   199,   200,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,    -1,   244,   245,   246,
     247,   248,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,    -1,
     277,   278,   279,   280,    -1,    -1,   283,   284,   285,   286,
     287,    -1,    -1,   290,   291,   292,   293,   294,   295,   296,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,    -1,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,   330,    23,    24,    25,    -1,    27,    -1,
      29,    30,    -1,    32,    33,    34,    35,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,    -1,   103,   104,   105,    -1,   107,    -1,
     109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,    -1,
     119,   120,    -1,   122,   123,   124,    -1,   126,   127,   128,
     129,    -1,    -1,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,   148,
     149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,   158,
      -1,    -1,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,    -1,   173,   174,   175,   176,   177,   178,
     179,    -1,   181,   182,    -1,    -1,   185,   186,   187,    -1,
      -1,   190,   191,    -1,    -1,   194,   195,    -1,    -1,   198,
      -1,    -1,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,    -1,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,    -1,   244,   245,   246,   247,    -1,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,    -1,   277,   278,
     279,   280,    -1,    -1,   283,   284,   285,   286,   287,    -1,
      -1,   290,   291,   292,   293,   294,   295,    -1,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,    -1,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,   330,    23,    24,    25,    -1,    27,    -1,    29,    30,
      -1,    32,    33,    34,    35,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    98,    99,   100,
     101,    -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,
      -1,    -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,
      -1,   122,   123,   124,    -1,   126,   127,   128,   129,    -1,
      -1,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,    -1,    -1,   145,    -1,   147,   148,   149,   150,
      -1,    -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,    -1,   173,    -1,   175,   176,   177,   178,   179,    -1,
     181,   182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,
      -1,    -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,    -1,
     231,   232,   233,   234,   235,   236,   237,   238,   239,   240,
     241,   242,    -1,   244,   245,   246,   247,    -1,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,    -1,   277,   278,   279,   280,
      -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,   290,
     291,   292,   293,   294,   295,    -1,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,    -1,
       3,     4,     5,     6,     7,     8,     9,    -1,    11,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,   330,
      23,    24,    25,    -1,    27,    -1,    29,    30,    -1,    32,
      33,    34,    35,    -1,    -1,    38,    39,    40,    41,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    50,    51,    52,
      -1,    54,    55,    56,    57,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    71,    72,
      73,    -1,    75,    -1,    77,    78,    79,    80,    81,    -1,
      -1,    -1,    85,    86,    87,    88,    -1,    90,    91,    -1,
      93,    -1,    95,    96,    97,    98,    99,   100,   101,    -1,
     103,   104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,
     113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,
     123,   124,    -1,   126,   127,   128,   129,    -1,    -1,   132,
     133,   134,   135,   136,   137,   138,    -1,   140,    -1,   142,
      -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,
     153,    -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,    -1,
     173,    -1,   175,   176,   177,   178,   179,    -1,   181,   182,
      -1,    -1,   185,   186,   187,    -1,    -1,   190,    -1,    -1,
      -1,   194,   195,    -1,    -1,   198,    -1,    -1,   201,   202,
     203,   204,   205,   206,    -1,   208,   209,   210,   211,    -1,
     213,   214,   215,   216,   217,   218,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,    -1,   231,   232,
     233,   234,   235,   236,   237,   238,    -1,   240,   241,   242,
      -1,   244,   245,   246,   247,    -1,   249,   250,    -1,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,    -1,
     263,   264,   265,    -1,   267,   268,    -1,   270,    -1,   272,
     273,   274,   275,    -1,   277,   278,   279,   280,    -1,    -1,
     283,   284,   285,   286,   287,    -1,    -1,   290,   291,   292,
     293,   294,   295,    -1,   297,   298,   299,    -1,    -1,   302,
     303,   304,   305,   306,   307,    -1,   309,    -1,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,   330,    23,    24,
      25,    -1,    27,    -1,    29,    30,    -1,    32,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    98,    99,   100,   101,    -1,   103,   104,
     105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,
      -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,   124,
      -1,   126,   127,   128,   129,    -1,    -1,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,
     145,    -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,
      -1,   156,   157,   158,    -1,    -1,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,    -1,   173,    -1,
     175,   176,   177,   178,   179,    -1,   181,   182,    -1,    -1,
     185,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,   194,
     195,    -1,    -1,   198,    -1,    -1,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,    -1,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,    -1,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,    -1,   277,   278,   279,   280,    -1,    -1,   283,   284,
     285,   286,   287,    -1,    -1,   290,   291,   292,   293,   294,
     295,    -1,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,    -1,    -1,    -1,    -1,   314,
      -1,    -1,   317,   318,   319,    -1,   321,   322,   323,   324,
     325,   326,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    -1,    23,    24,    25,    -1,    27,    -1,    29,    30,
      -1,    32,    33,    34,    35,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    98,    99,   100,
     101,    -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,
      -1,    -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,
      -1,   122,   123,   124,    -1,   126,   127,   128,   129,    -1,
      -1,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,    -1,    -1,   145,    -1,   147,   148,   149,   150,
      -1,    -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,    -1,   173,    -1,   175,   176,   177,   178,   179,    -1,
     181,   182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,
      -1,    -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,    -1,
     231,   232,   233,   234,   235,   236,   237,   238,    -1,   240,
     241,   242,    -1,   244,   245,   246,   247,    -1,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,    -1,   277,   278,   279,   280,
      -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,   290,
     291,   292,   293,   294,   295,    -1,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,    -1,
      -1,    -1,    -1,   314,    -1,    -1,   317,   318,   319,    -1,
     321,   322,   323,   324,   325,   326,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    -1,    23,    24,    25,    -1,
      27,    -1,    29,    30,    -1,    32,    33,    34,    35,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    69,    70,    71,    72,    73,    -1,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    -1,    95,    96,
      97,    98,    99,   100,   101,    -1,   103,   104,   105,    -1,
     107,    -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,
     117,    -1,   119,   120,    -1,   122,   123,   124,    -1,   126,
     127,   128,   129,    -1,    -1,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,
     147,   148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,
     157,   158,    -1,    -1,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,    -1,   173,    -1,   175,   176,
     177,   178,   179,    -1,   181,   182,    -1,    -1,   185,   186,
     187,    -1,    -1,   190,    -1,    -1,    -1,   194,   195,    -1,
      -1,   198,    -1,    -1,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,    -1,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,    -1,   244,   245,   246,
     247,    -1,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,    -1,
     277,   278,   279,   280,    -1,    -1,   283,   284,   285,   286,
     287,    -1,    -1,   290,   291,   292,   293,   294,   295,    -1,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,    -1,    -1,    -1,    -1,   314,    -1,    -1,
     317,   318,   319,    -1,   321,   322,   323,   324,   325,   326,
       3,     4,     5,     6,     7,     8,     9,    -1,    11,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,    -1,
      23,    24,    25,    -1,    27,    -1,    29,    30,    -1,    32,
      33,    34,    35,    -1,    -1,    38,    39,    40,    41,    -1,
      43,    44,    45,    46,    -1,    -1,    -1,    50,    51,    52,
      -1,    54,    55,    -1,    57,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    71,    72,
      73,    -1,    75,    -1,    77,    78,    79,    80,    81,    -1,
      -1,    -1,    85,    86,    87,    88,    -1,    90,    91,    -1,
      93,    -1,    95,    96,    97,    -1,    99,   100,    -1,    -1,
     103,   104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,
     113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,
     123,   124,    -1,   126,   127,   128,   129,    -1,    -1,   132,
     133,   134,   135,   136,   137,   138,    -1,   140,    -1,   142,
      -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,
     153,    -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,    -1,
     173,    -1,   175,   176,   177,   178,    -1,    -1,   181,   182,
      -1,    -1,    -1,   186,   187,    -1,    -1,   190,    -1,    -1,
      -1,   194,   195,    -1,    -1,   198,    -1,    -1,    -1,   202,
     203,   204,   205,   206,    -1,    -1,   209,   210,   211,    -1,
     213,   214,   215,   216,   217,   218,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,    -1,   231,    -1,
     233,   234,   235,   236,   237,   238,    -1,   240,   241,   242,
      -1,   244,   245,   246,   247,    -1,   249,   250,    -1,   252,
     253,   254,   255,   256,   257,   258,   259,    -1,   261,    -1,
     263,   264,   265,    -1,   267,   268,    -1,   270,    -1,   272,
      -1,   274,    -1,    -1,   277,   278,   279,   280,    -1,    -1,
     283,   284,   285,   286,   287,    -1,    -1,   290,   291,   292,
     293,   294,   295,    -1,   297,   298,   299,    -1,    -1,   302,
     303,   304,   305,   306,   307,    -1,   309,   310,   311,    -1,
      -1,   314,   315,    -1,   317,   318,   319,    -1,   321,   322,
     323,   324,   325,   326,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    -1,    23,    24,    25,    -1,    27,    -1,
      29,    30,    -1,    32,    33,    34,    35,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,    -1,   103,   104,   105,    -1,   107,    -1,
     109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,    -1,
     119,   120,    -1,   122,   123,   124,    -1,   126,   127,   128,
     129,    -1,    -1,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,   148,
     149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,   158,
      -1,    -1,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,    -1,   173,   174,   175,   176,   177,   178,
     179,    -1,   181,   182,    -1,    -1,   185,   186,   187,    -1,
      -1,   190,   191,    -1,    -1,   194,   195,    -1,    -1,   198,
      -1,    -1,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,    -1,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,    -1,   244,   245,   246,   247,    -1,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,    -1,   277,   278,
     279,   280,    -1,    -1,   283,   284,   285,   286,   287,    -1,
      -1,   290,   291,   292,   293,   294,   295,    -1,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,     3,     4,     5,     6,     7,     8,     9,    -1,    11,
      -1,    -1,    -1,    -1,   323,    -1,    -1,    19,    20,    21,
      -1,    23,    24,    25,    -1,    27,    -1,    29,    30,    -1,
      32,    33,    34,    35,    -1,    -1,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    -1,    -1,    50,    51,
      52,    -1,    54,    55,    56,    57,    -1,    59,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    71,
      72,    73,    -1,    75,    -1,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,    91,
      -1,    93,    -1,    95,    96,    97,    98,    99,   100,   101,
      -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,    -1,
      -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,
     122,   123,   124,    -1,   126,   127,   128,   129,    -1,    -1,
     132,   133,   134,   135,   136,   137,   138,    -1,   140,    -1,
     142,    -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,
      -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
      -1,   173,    -1,   175,   176,   177,   178,   179,    -1,   181,
     182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,    -1,
      -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,   201,
     202,   203,   204,   205,   206,    -1,   208,   209,   210,   211,
      -1,   213,   214,   215,   216,   217,   218,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,    -1,   231,
     232,   233,   234,   235,   236,   237,   238,    -1,   240,   241,
     242,    -1,   244,   245,   246,   247,    -1,   249,   250,    -1,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
      -1,   263,   264,   265,    -1,   267,   268,    -1,   270,    -1,
     272,   273,   274,   275,    -1,   277,   278,   279,   280,    -1,
      -1,   283,   284,   285,   286,   287,    -1,    -1,   290,   291,
     292,   293,   294,   295,    -1,   297,   298,   299,    -1,    -1,
     302,   303,   304,   305,   306,   307,    -1,   309,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,   323,    -1,    -1,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    -1,    29,    30,    -1,    32,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    -1,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    -1,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    -1,    99,   100,    -1,    -1,   103,   104,
     105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,
      -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,   124,
      -1,   126,   127,   128,   129,    -1,    -1,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,
     145,    -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,
      -1,   156,   157,   158,    -1,    -1,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,    -1,   173,    -1,
     175,   176,   177,   178,    -1,    -1,   181,   182,    -1,    -1,
      -1,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,   194,
     195,    -1,    -1,   198,    -1,    -1,    -1,   202,   203,   204,
     205,   206,    -1,    -1,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,    -1,   231,    -1,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,    -1,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,    -1,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,    -1,   274,
      -1,    -1,   277,   278,   279,   280,    -1,    -1,   283,   284,
     285,   286,   287,    -1,    -1,   290,   291,   292,   293,   294,
     295,    -1,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    -1,    -1,    -1,    -1,   323,    -1,
      -1,    19,    20,    21,    -1,    23,    24,    25,    -1,    27,
      -1,    29,    30,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    47,
      -1,    -1,    50,    51,    52,    -1,    54,    55,    56,    57,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    -1,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    87,
      88,    -1,    90,    91,    -1,    93,    -1,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,   107,
      -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,
      -1,   119,   120,    -1,   122,   123,   124,    -1,   126,   127,
     128,   129,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,
     148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,
     158,    -1,    -1,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    -1,   173,    -1,   175,   176,   177,
     178,   179,    -1,   181,   182,    -1,    -1,   185,   186,   187,
     188,    -1,   190,    -1,   192,    -1,   194,   195,    -1,    -1,
     198,    -1,    -1,   201,   202,   203,   204,   205,   206,    -1,
     208,   209,   210,   211,    -1,   213,   214,   215,   216,   217,
     218,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,    -1,   231,   232,   233,   234,   235,   236,   237,
     238,    -1,   240,   241,   242,    -1,   244,   245,   246,   247,
      -1,   249,   250,    -1,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,    -1,   263,   264,   265,    -1,   267,
     268,    -1,   270,    -1,   272,   273,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   283,   284,   285,   286,   287,
      -1,    -1,   290,   291,   292,   293,   294,   295,    -1,   297,
     298,   299,    -1,    -1,   302,   303,   304,   305,   306,   307,
      -1,   309,   310,   311,    -1,    -1,    -1,   315,    -1,    -1,
      -1,    -1,    -1,    -1,   322,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    21,    -1,    23,    24,    25,    -1,    27,
      -1,    29,    30,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    47,
      -1,    -1,    50,    51,    52,    -1,    54,    55,    56,    57,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    71,    72,    73,    -1,    75,    -1,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    87,
      88,    -1,    90,    91,    -1,    93,    -1,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,    -1,   107,
      -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,
      -1,   119,   120,    -1,   122,   123,   124,    -1,   126,   127,
     128,   129,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,
     148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,
     158,    -1,    -1,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    -1,   173,    -1,   175,   176,   177,
     178,   179,    -1,   181,   182,    -1,    -1,   185,   186,   187,
     188,    -1,   190,    -1,   192,    -1,   194,   195,    -1,    -1,
     198,    -1,    -1,   201,   202,   203,   204,   205,   206,    -1,
     208,   209,   210,   211,    -1,   213,   214,   215,   216,   217,
     218,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,    -1,   231,   232,   233,   234,   235,   236,   237,
     238,    -1,   240,   241,   242,    -1,   244,   245,   246,   247,
      -1,   249,   250,    -1,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,    -1,   263,   264,   265,    -1,   267,
     268,    -1,   270,    -1,   272,   273,   274,   275,   276,   277,
     278,   279,   280,    -1,    -1,   283,   284,   285,   286,   287,
      -1,    -1,   290,   291,   292,   293,   294,   295,    -1,   297,
     298,   299,    -1,    -1,   302,   303,   304,   305,   306,   307,
      -1,   309,   310,   311,    -1,    -1,    -1,   315,    -1,    -1,
      -1,    -1,    -1,    -1,   322,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    21,    -1,    23,    24,    25,    -1,    27,
      -1,    29,    30,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    47,
      -1,    -1,    50,    51,    52,    -1,    54,    55,    56,    57,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    71,    72,    73,    -1,    75,    -1,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    87,
      88,    -1,    90,    91,    -1,    93,    -1,    95,    96,    97,
      98,    99,   100,   101,    -1,   103,   104,   105,    -1,   107,
      -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,
      -1,   119,   120,    -1,   122,   123,   124,    -1,   126,   127,
     128,   129,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,
     148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,
     158,    -1,    -1,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    -1,   173,    -1,   175,   176,   177,
     178,   179,    -1,   181,   182,    -1,    -1,   185,   186,   187,
      -1,    -1,   190,    -1,    -1,    -1,   194,   195,    -1,    -1,
     198,    -1,    -1,   201,   202,   203,   204,   205,   206,    -1,
     208,   209,   210,   211,    -1,   213,   214,   215,   216,   217,
     218,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,    -1,   231,   232,   233,   234,   235,   236,   237,
     238,    -1,   240,   241,   242,    -1,   244,   245,   246,   247,
      -1,   249,   250,    -1,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,    -1,   263,   264,   265,    -1,   267,
     268,    14,   270,    -1,   272,   273,   274,   275,    21,   277,
     278,   279,   280,    26,    -1,   283,   284,   285,   286,   287,
      -1,    14,   290,   291,   292,   293,   294,   295,    21,   297,
     298,   299,    -1,    26,   302,   303,   304,   305,   306,   307,
      -1,   309,    -1,    -1,    -1,    14,    -1,   315,    -1,    -1,
      -1,    -1,    21,    -1,   322,    -1,    -1,    26,    -1,    -1,
      -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,   111,    -1,
      21,    -1,    -1,    -1,    -1,    26,    -1,    -1,   121,    -1,
      -1,    -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,
     143,   144,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     143,   144,   121,    -1,    -1,    -1,   125,    -1,    -1,    -1,
      -1,   154,    -1,    -1,    -1,    -1,    -1,   180,    -1,   121,
     183,    -1,    -1,   125,   143,   144,    -1,    -1,    -1,    -1,
      -1,   194,    -1,   196,    -1,   154,    -1,   180,    -1,    -1,
     183,   143,   144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     121,   194,   154,   196,   125,    -1,    -1,    -1,    -1,    -1,
      -1,   180,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   143,   144,    -1,   194,    -1,   196,   180,    -1,
      -1,   183,    -1,   154,    -1,   248,    -1,    -1,    -1,    -1,
      -1,    -1,   194,    -1,   196,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   248,    -1,    -1,    -1,   180,
      -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   194,    -1,   196,    -1,    14,    -1,   248,
      -1,    -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,    26,
      -1,    -1,    -1,    -1,    -1,    -1,   248,   266,    -1,    -1,
      -1,   314,    -1,    -1,   317,   318,   319,    -1,   321,   322,
     323,   324,   325,   326,    -1,    -1,    -1,    -1,    -1,   332,
      -1,   314,    -1,    -1,   317,   318,   319,   248,   321,   322,
     323,   324,   325,   326,    14,    -1,    -1,    -1,   331,   332,
      -1,    21,    -1,    -1,    -1,   314,    26,    -1,   317,   318,
     319,    -1,   321,   322,   323,   324,   325,   326,    -1,    -1,
      -1,    -1,   314,   332,    -1,   317,   318,   319,    -1,   321,
     322,   323,   324,   325,   326,    14,    -1,    -1,    -1,   331,
     332,    -1,    21,    -1,   121,    -1,    -1,    26,   125,    -1,
      -1,    -1,    -1,   314,    -1,    -1,   317,   318,   319,    -1,
     321,   322,   323,   324,   325,   326,   143,   144,    -1,    -1,
     331,   332,    -1,    -1,    -1,    -1,    -1,   154,    -1,    -1,
      -1,    -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,
      -1,   121,    -1,   180,    -1,   125,   183,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   194,    -1,   196,
      -1,    -1,    -1,   143,   144,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   154,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   121,    -1,    -1,    14,   125,    -1,    -1,    -1,
      -1,    -1,    21,    -1,    -1,    -1,    -1,    26,    -1,    -1,
     180,    -1,    -1,   183,   143,   144,    -1,    -1,    -1,    -1,
      -1,   248,    -1,    -1,   194,   154,   196,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   121,    -1,    -1,    -1,   125,    -1,    -1,    -1,    -1,
      -1,   180,    -1,    -1,   183,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   143,   144,   194,    -1,   196,    -1,    -1,
      -1,    -1,    -1,    -1,   154,    -1,    -1,    -1,   248,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   314,    -1,    -1,
     317,   318,   319,    -1,   321,   322,   323,   324,   325,   326,
     180,    -1,   121,   183,   331,   332,   125,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   194,    -1,   196,    -1,    -1,   248,
      -1,    -1,    -1,    -1,   143,   144,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   154,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   314,    -1,    -1,   317,   318,   319,
      -1,   321,   322,   323,   324,   325,   326,    -1,    -1,   329,
      -1,   180,   332,    -1,   183,    -1,    -1,    -1,   248,    -1,
      -1,    -1,    -1,    -1,    -1,   194,    -1,   196,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   314,    -1,    -1,   317,   318,
     319,    -1,   321,   322,   323,   324,   325,   326,    -1,    -1,
      -1,    -1,   331,   332,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   248,
      -1,    -1,    -1,    -1,   314,    -1,    -1,   317,   318,   319,
      -1,   321,   322,   323,   324,   325,   326,    -1,    -1,    -1,
      -1,   331,   332,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   314,    -1,    -1,   317,   318,
     319,    -1,   321,   322,   323,   324,   325,   326,    -1,    -1,
      -1,    -1,    -1,   332,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    -1,    23,    24,    25,    -1,    27,    -1,
      29,    30,    -1,    32,    33,    34,    35,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,    -1,   103,   104,   105,    -1,   107,    -1,
     109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,    -1,
     119,   120,    -1,   122,   123,   124,    -1,   126,   127,   128,
     129,    -1,    -1,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,   148,
     149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,   158,
      -1,    -1,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,    -1,   173,    -1,   175,   176,   177,   178,
     179,    -1,   181,   182,    -1,    -1,   185,   186,   187,    -1,
      -1,   190,    -1,    -1,    -1,   194,   195,    -1,    -1,   198,
      -1,    -1,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,    -1,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,    -1,   244,   245,   246,   247,    -1,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    21,   267,   268,
      -1,   270,    26,   272,   273,   274,   275,    -1,   277,   278,
     279,   280,    -1,    -1,   283,   284,   285,   286,   287,    -1,
      -1,   290,   291,   292,   293,   294,   295,    -1,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,   310,   311,   312,   313,    -1,   315,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    93,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,    -1,    -1,
      -1,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   143,
     144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     154,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     194,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   248,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     314,    -1,    -1,    -1,    -1,    -1,    -1,   321,   322,   323,
     324,   325,   326,    -1,    -1,    -1,    -1,    -1,   332,     3,
       4,     5,     6,     7,     8,     9,    -1,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,
      24,    25,    -1,    27,    -1,    29,    30,    -1,    32,    33,
      34,    35,    -1,    -1,    38,    39,    40,    41,    -1,    43,
      44,    45,    46,    47,    -1,    -1,    50,    51,    52,    -1,
      54,    55,    56,    57,    -1,    59,    60,    -1,    -1,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    -1,    77,    78,    79,    80,    81,    -1,    -1,
      -1,    85,    86,    87,    88,    -1,    90,    91,    -1,    93,
      -1,    95,    96,    97,    98,    99,   100,   101,    -1,   103,
     104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,
     114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,
     124,    -1,   126,   127,   128,   129,    -1,    -1,   132,   133,
     134,   135,   136,   137,   138,    -1,   140,    -1,   142,    -1,
      -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,   153,
      -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,    -1,   173,
      -1,   175,   176,   177,   178,   179,    -1,   181,   182,    -1,
      -1,   185,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,
     194,   195,    -1,    -1,   198,    -1,    -1,   201,   202,   203,
     204,   205,   206,    -1,   208,   209,   210,   211,    -1,   213,
     214,   215,   216,   217,   218,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,    -1,   231,   232,   233,
     234,   235,   236,   237,   238,    -1,   240,   241,   242,    -1,
     244,   245,   246,   247,    -1,   249,   250,    -1,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,    -1,   263,
     264,   265,    -1,   267,   268,    -1,   270,    -1,   272,   273,
     274,   275,    -1,   277,   278,   279,   280,    -1,    -1,   283,
     284,   285,   286,   287,    -1,    -1,   290,   291,   292,   293,
     294,   295,    -1,   297,   298,   299,    -1,    -1,   302,   303,
     304,   305,   306,   307,    -1,   309,    -1,   311,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    -1,    29,    30,    -1,    32,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    98,    99,   100,   101,    -1,   103,   104,
     105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,
      -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,   124,
      -1,   126,   127,   128,   129,    -1,    -1,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,
     145,    -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,
      -1,   156,   157,   158,    -1,    -1,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,    -1,   173,    -1,
     175,   176,   177,   178,   179,    -1,   181,   182,    -1,    -1,
     185,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,   194,
     195,    -1,    -1,   198,    -1,    -1,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,    -1,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,    -1,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,    -1,   277,   278,   279,   280,    -1,    -1,   283,   284,
     285,   286,   287,    -1,    -1,   290,   291,   292,   293,   294,
     295,    -1,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,    -1,   311,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    92,    93,    94,    95,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     146,   147,   148,   149,   150,   151,   152,   153,   154,   155,
     156,   157,   158,   159,   160,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   189,   190,   191,   192,   193,   194,   195,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   207,   208,   209,   210,   211,   212,   213,   214,   215,
     216,   217,   218,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,   283,   284,   285,
     286,   287,   288,   289,   290,   291,   292,   293,   294,   295,
     296,   297,   298,   299,   300,   301,   302,   303,   304,   305,
     306,   307,    -1,   309,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    -1,    32,    33,    34,    35,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    61,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,    -1,   103,   104,   105,    -1,   107,    -1,
     109,   110,    -1,   112,   113,   114,    -1,    -1,   117,    -1,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,    -1,   131,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,   143,   144,   145,   146,   147,   148,
     149,   150,    -1,   152,   153,   154,    -1,   156,   157,   158,
      -1,    -1,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   173,   174,   175,   176,   177,   178,
     179,    -1,   181,   182,   183,    -1,   185,   186,   187,    -1,
      -1,   190,   191,    -1,    -1,   194,   195,    -1,    -1,   198,
     199,   200,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,   230,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,    -1,   244,   245,   246,   247,   248,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,    -1,   277,   278,
     279,   280,    -1,    -1,   283,   284,   285,   286,   287,    -1,
      -1,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,     3,     4,     5,     6,     7,     8,     9,    -1,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,
      -1,    23,    24,    25,    -1,    27,    -1,    29,    30,    -1,
      32,    33,    34,    35,    -1,    -1,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    -1,    50,    51,
      52,    53,    54,    55,    56,    57,    -1,    59,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    71,
      72,    73,    -1,    75,    -1,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,    91,
      -1,    93,    -1,    95,    96,    97,    98,    99,   100,   101,
      -1,   103,   104,   105,    -1,   107,   108,   109,    -1,    -1,
      -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,
     122,   123,   124,    -1,   126,   127,   128,   129,    -1,    -1,
     132,   133,   134,   135,   136,   137,   138,    -1,   140,    -1,
     142,    -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,
      -1,   153,   154,    -1,   156,   157,   158,    -1,    -1,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
      -1,   173,    -1,   175,   176,   177,   178,   179,    -1,   181,
     182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,    -1,
      -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,   201,
     202,   203,   204,   205,   206,    -1,   208,   209,   210,   211,
     212,   213,   214,   215,   216,   217,   218,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,    -1,   231,
     232,   233,   234,   235,   236,   237,   238,    -1,   240,   241,
     242,    -1,   244,   245,   246,   247,    -1,   249,   250,    -1,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
      -1,   263,   264,   265,    -1,   267,   268,    -1,   270,    -1,
     272,   273,   274,   275,    -1,   277,   278,   279,   280,    -1,
     282,   283,   284,   285,   286,   287,    -1,    -1,   290,   291,
     292,   293,   294,   295,    -1,   297,   298,   299,    -1,    -1,
     302,   303,   304,   305,   306,   307,    -1,   309,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    12,    13,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    -1,    29,    30,    -1,    32,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    98,    99,   100,   101,    -1,   103,   104,
     105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,
      -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,   124,
      -1,   126,   127,   128,   129,    -1,    -1,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,
     145,    -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,
      -1,   156,   157,   158,    -1,    -1,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,    -1,   173,   174,
     175,   176,   177,   178,   179,    -1,   181,   182,    -1,    -1,
     185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,   194,
     195,    -1,    -1,   198,    -1,    -1,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,    -1,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,    -1,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,    -1,   277,   278,   279,   280,    -1,    -1,   283,   284,
     285,   286,   287,    -1,    -1,   290,   291,   292,   293,   294,
     295,    -1,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    21,    -1,    23,    24,    25,    -1,    27,
      -1,    29,    30,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    47,
      -1,    -1,    50,    51,    52,    -1,    54,    55,    56,    57,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    71,    72,    73,    -1,    75,    -1,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    87,
      88,    -1,    90,    91,    -1,    93,    -1,    95,    96,    97,
      98,    99,   100,   101,    -1,   103,   104,   105,    -1,   107,
      -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,
      -1,   119,   120,    -1,   122,   123,   124,    -1,   126,   127,
     128,   129,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,
     148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,
     158,    -1,    -1,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    -1,   173,   174,   175,   176,   177,
     178,   179,    -1,   181,   182,    -1,    -1,   185,   186,   187,
      -1,    -1,   190,   191,    -1,   193,   194,   195,    -1,    -1,
     198,    -1,    -1,   201,   202,   203,   204,   205,   206,    -1,
     208,   209,   210,   211,    -1,   213,   214,   215,   216,   217,
     218,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,    -1,   231,   232,   233,   234,   235,   236,   237,
     238,    -1,   240,   241,   242,    -1,   244,   245,   246,   247,
      -1,   249,   250,    -1,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,    -1,   263,   264,   265,    -1,   267,
     268,    -1,   270,    -1,   272,   273,   274,   275,    -1,   277,
     278,   279,   280,    -1,    -1,   283,   284,   285,   286,   287,
      -1,    -1,   290,   291,   292,   293,   294,   295,    -1,   297,
     298,   299,    -1,    -1,   302,   303,   304,   305,   306,   307,
      -1,   309,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    -1,    23,    24,    25,    -1,    27,    -1,    29,    30,
      -1,    32,    33,    34,    35,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    98,    99,   100,
     101,    -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,
      -1,    -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,
      -1,   122,   123,   124,    -1,   126,   127,   128,   129,    -1,
      -1,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,    -1,    -1,   145,    -1,   147,   148,   149,   150,
      -1,    -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,    -1,   173,   174,   175,   176,   177,   178,   179,    -1,
     181,   182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,
     191,    -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,    -1,
     231,   232,   233,   234,   235,   236,   237,   238,    -1,   240,
     241,   242,    -1,   244,   245,   246,   247,    -1,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,    -1,   277,   278,   279,   280,
      -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,   290,
     291,   292,   293,   294,   295,    -1,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,     3,
       4,     5,     6,     7,     8,     9,    -1,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,
      24,    25,    -1,    27,    -1,    29,    30,    -1,    32,    33,
      34,    35,    -1,    -1,    38,    39,    40,    41,    -1,    43,
      44,    45,    46,    47,    -1,    -1,    50,    51,    52,    -1,
      54,    55,    56,    57,    -1,    59,    60,    -1,    -1,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      -1,    75,    -1,    77,    78,    79,    80,    81,    -1,    -1,
      -1,    85,    86,    87,    88,    -1,    90,    91,    -1,    93,
      -1,    95,    96,    97,    98,    99,   100,   101,    -1,   103,
     104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,
     114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,
     124,    -1,   126,   127,   128,   129,    -1,    -1,   132,   133,
     134,   135,   136,   137,   138,    -1,   140,    -1,   142,    -1,
      -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,   153,
      -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,    -1,   173,
     174,   175,   176,   177,   178,   179,    -1,   181,   182,    -1,
      -1,   185,   186,   187,    -1,    -1,   190,   191,    -1,    -1,
     194,   195,    -1,    -1,   198,    -1,    -1,   201,   202,   203,
     204,   205,   206,    -1,   208,   209,   210,   211,    -1,   213,
     214,   215,   216,   217,   218,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,    -1,   231,   232,   233,
     234,   235,   236,   237,   238,    -1,   240,   241,   242,    -1,
     244,   245,   246,   247,    -1,   249,   250,    -1,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,   262,   263,
     264,   265,    -1,   267,   268,    -1,   270,    -1,   272,   273,
     274,   275,    -1,   277,   278,   279,   280,    -1,    -1,   283,
     284,   285,   286,   287,    -1,    -1,   290,   291,   292,   293,
     294,   295,    -1,   297,   298,   299,    -1,    -1,   302,   303,
     304,   305,   306,   307,    -1,   309,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    -1,    23,    24,    25,    -1,
      27,    -1,    29,    30,    -1,    32,    33,    34,    35,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    69,    70,    71,    72,    73,    -1,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    -1,    95,    96,
      97,    98,    99,   100,   101,    -1,   103,   104,   105,    -1,
     107,    -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,
     117,    -1,   119,   120,    -1,   122,   123,   124,    -1,   126,
     127,   128,   129,    -1,    -1,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,
     147,   148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,
     157,   158,    -1,    -1,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,    -1,   173,   174,   175,   176,
     177,   178,   179,    -1,   181,   182,    -1,    -1,   185,   186,
     187,    -1,    -1,   190,   191,    -1,    -1,   194,   195,    -1,
      -1,   198,    -1,    -1,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,    -1,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,    -1,   244,   245,   246,
     247,    -1,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,    -1,
     277,   278,   279,   280,    -1,    -1,   283,   284,   285,   286,
     287,    -1,   289,   290,   291,   292,   293,   294,   295,    -1,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,     3,     4,     5,     6,     7,     8,     9,
      -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
      20,    21,    -1,    23,    24,    25,    -1,    27,    -1,    29,
      30,    -1,    32,    33,    34,    35,    -1,    -1,    38,    39,
      40,    41,    -1,    43,    44,    45,    46,    47,    -1,    -1,
      50,    51,    52,    -1,    54,    55,    56,    57,    -1,    59,
      60,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,    69,
      70,    71,    72,    73,    -1,    75,    -1,    77,    78,    79,
      80,    81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,
      90,    91,    -1,    93,    -1,    95,    96,    97,    98,    99,
     100,   101,    -1,   103,   104,   105,    -1,   107,    -1,   109,
      -1,    -1,    -1,   113,   114,    -1,    -1,   117,    -1,   119,
     120,    -1,   122,   123,   124,    -1,   126,   127,   128,   129,
      -1,    -1,   132,   133,   134,   135,   136,   137,   138,    -1,
     140,    -1,   142,    -1,    -1,   145,    -1,   147,   148,   149,
     150,    -1,    -1,   153,    -1,    -1,   156,   157,   158,    -1,
      -1,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,    -1,   173,   174,   175,   176,   177,   178,   179,
      -1,   181,   182,    -1,    -1,   185,   186,   187,    -1,    -1,
     190,   191,    -1,    -1,   194,   195,    -1,    -1,   198,    -1,
      -1,   201,   202,   203,   204,   205,   206,    -1,   208,   209,
     210,   211,    -1,   213,   214,   215,   216,   217,   218,   219,
      -1,   221,   222,   223,   224,   225,   226,   227,   228,   229,
      -1,   231,   232,   233,   234,   235,   236,   237,   238,    -1,
     240,   241,   242,    -1,   244,   245,   246,   247,    -1,   249,
     250,    -1,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,    -1,   263,   264,   265,    -1,   267,   268,    -1,
     270,    -1,   272,   273,   274,   275,    -1,   277,   278,   279,
     280,    -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,
     290,   291,   292,   293,   294,   295,    -1,   297,   298,   299,
      -1,    -1,   302,   303,   304,   305,   306,   307,    -1,   309,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,    -1,
      23,    24,    25,    -1,    27,    -1,    29,    30,    -1,    32,
      33,    34,    35,    -1,    -1,    38,    39,    40,    41,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    50,    51,    52,
      -1,    54,    55,    56,    57,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    71,    72,
      73,    -1,    75,    -1,    77,    78,    79,    80,    81,    -1,
      -1,    -1,    85,    86,    87,    88,    -1,    90,    91,    -1,
      93,    -1,    95,    96,    97,    98,    99,   100,   101,    -1,
     103,   104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,
     113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,
     123,   124,    -1,   126,   127,   128,   129,    -1,    -1,   132,
     133,   134,   135,   136,   137,   138,    -1,   140,    -1,   142,
      -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,
     153,    -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,    -1,
     173,    -1,   175,   176,   177,   178,   179,    -1,   181,   182,
      -1,    -1,   185,   186,   187,    -1,    -1,   190,    -1,    -1,
      -1,   194,   195,    -1,    -1,   198,    -1,    -1,   201,   202,
     203,   204,   205,   206,    -1,   208,   209,   210,   211,    -1,
     213,   214,   215,   216,   217,   218,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,    -1,   231,   232,
     233,   234,   235,   236,   237,   238,    -1,   240,   241,   242,
      -1,   244,   245,   246,   247,    -1,   249,   250,    -1,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,    -1,
     263,   264,   265,    -1,   267,   268,    -1,   270,    -1,   272,
     273,   274,   275,    -1,   277,   278,   279,   280,    -1,    -1,
     283,   284,   285,   286,   287,    -1,    -1,   290,   291,   292,
     293,   294,   295,    -1,   297,   298,   299,    -1,    -1,   302,
     303,   304,   305,   306,   307,    -1,   309,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    20,    21,    -1,    23,    24,    25,
      -1,    27,    -1,    29,    30,    -1,    32,    33,    34,    35,
      -1,    -1,    38,    39,    40,    41,    -1,    43,    44,    45,
      46,    47,    -1,    -1,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    69,    70,    71,    72,    73,    -1,    75,
      -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,    85,
      86,    87,    88,    -1,    90,    91,    -1,    93,    -1,    95,
      96,    97,    98,    99,   100,   101,    -1,   103,   104,   105,
      -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,    -1,
      -1,   117,    -1,   119,   120,    -1,   122,   123,   124,    -1,
     126,   127,   128,   129,    -1,    -1,   132,   133,   134,   135,
     136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,   145,
      -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,    -1,
     156,   157,   158,    -1,    -1,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,    -1,   173,    -1,   175,
     176,   177,   178,   179,    -1,   181,   182,    -1,    -1,   185,
     186,   187,    -1,    -1,   190,    -1,    -1,    -1,   194,   195,
      -1,    -1,   198,    -1,    -1,   201,   202,   203,   204,   205,
     206,    -1,   208,   209,   210,   211,    -1,   213,   214,   215,
     216,   217,   218,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,    -1,   231,   232,   233,   234,   235,
     236,   237,   238,    -1,   240,   241,   242,    -1,   244,   245,
     246,   247,    -1,   249,   250,    -1,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,    -1,   263,   264,   265,
      -1,   267,   268,    -1,   270,    -1,   272,   273,   274,   275,
      -1,   277,   278,   279,   280,    -1,    -1,   283,   284,   285,
     286,   287,    -1,    -1,   290,   291,   292,   293,   294,   295,
      -1,   297,   298,   299,    -1,    -1,   302,   303,   304,   305,
     306,   307,    -1,   309,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    -1,    23,    24,    25,    -1,    27,    -1,
      29,    30,    -1,    32,    33,    34,    35,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,    -1,   103,   104,   105,    -1,   107,    -1,
     109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,    -1,
     119,   120,    -1,   122,   123,   124,    -1,   126,   127,   128,
     129,    -1,    -1,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,   148,
     149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,   158,
      -1,    -1,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,    -1,   173,    -1,   175,   176,   177,   178,
     179,    -1,   181,   182,    -1,    -1,   185,   186,   187,    -1,
      -1,   190,    -1,    -1,    -1,   194,   195,    -1,    -1,   198,
      -1,    -1,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,    -1,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,    -1,   244,   245,   246,   247,    -1,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,    -1,   277,   278,
     279,   280,    -1,    -1,   283,   284,   285,   286,   287,    -1,
      -1,   290,   291,   292,   293,   294,   295,    -1,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,     3,     4,     5,     6,     7,     8,     9,    -1,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,
      22,    23,    24,    25,    -1,    27,    -1,    29,    30,    -1,
      32,    33,    34,    35,    -1,    -1,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    47,    -1,    -1,    50,    51,
      52,    -1,    54,    55,    56,    57,    -1,    59,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    71,
      72,    73,    -1,    75,    -1,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,    91,
      -1,    93,    -1,    95,    96,    97,    98,    99,   100,   101,
      -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,    -1,
      -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,
     122,   123,   124,    -1,   126,   127,   128,   129,    -1,    -1,
     132,   133,   134,   135,   136,   137,   138,    -1,   140,    -1,
     142,    -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,
      -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
      -1,   173,    -1,   175,   176,   177,   178,   179,    -1,   181,
     182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,    -1,
      -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,   201,
     202,   203,   204,   205,   206,    -1,   208,   209,   210,   211,
      -1,   213,   214,   215,   216,   217,   218,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,    -1,   231,
     232,   233,   234,   235,   236,   237,   238,    -1,   240,   241,
     242,    -1,   244,   245,   246,   247,    -1,   249,   250,    -1,
     252,   253,   254,   255,   256,   257,   258,   259,   260,   261,
      -1,   263,   264,   265,    -1,   267,   268,    -1,   270,    -1,
     272,   273,   274,   275,    -1,   277,   278,   279,   280,    -1,
      -1,   283,   284,   285,   286,   287,    -1,    -1,   290,   291,
     292,   293,   294,   295,    -1,   297,   298,   299,    -1,    -1,
     302,   303,   304,   305,   306,   307,    -1,   309,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    -1,    29,    30,    -1,    32,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    56,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    98,    99,   100,   101,    -1,   103,   104,
     105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,
      -1,   116,   117,    -1,   119,   120,    -1,   122,   123,   124,
      -1,   126,   127,   128,   129,    -1,    -1,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,
     145,    -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,
      -1,   156,   157,   158,    -1,    -1,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,    -1,   173,    -1,
     175,   176,   177,   178,   179,    -1,   181,   182,    -1,    -1,
     185,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,   194,
     195,    -1,    -1,   198,    -1,    -1,   201,   202,   203,   204,
     205,   206,    -1,   208,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,    -1,   231,   232,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,    -1,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,   273,   274,
     275,    -1,   277,   278,   279,   280,    -1,    -1,   283,   284,
     285,   286,   287,    -1,    -1,   290,   291,   292,   293,   294,
     295,    -1,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    17,
      -1,    19,    20,    21,    -1,    23,    24,    25,    -1,    27,
      -1,    29,    30,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    47,
      -1,    -1,    50,    51,    52,    -1,    54,    55,    56,    57,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    71,    72,    73,    -1,    75,    -1,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    87,
      88,    -1,    90,    91,    -1,    93,    -1,    95,    96,    97,
      98,    99,   100,   101,    -1,   103,   104,   105,    -1,   107,
      -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,
      -1,   119,   120,    -1,   122,   123,   124,    -1,   126,   127,
     128,   129,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,
     148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,
     158,    -1,    -1,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    -1,   173,    -1,   175,   176,   177,
     178,   179,    -1,   181,   182,    -1,    -1,   185,   186,   187,
      -1,    -1,   190,    -1,    -1,    -1,   194,   195,    -1,    -1,
     198,    -1,    -1,   201,   202,   203,   204,   205,   206,    -1,
     208,   209,   210,   211,    -1,   213,   214,   215,   216,   217,
     218,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,    -1,   231,   232,   233,   234,   235,   236,   237,
     238,    -1,   240,   241,   242,    -1,   244,   245,   246,   247,
      -1,   249,   250,    -1,   252,   253,   254,   255,   256,   257,
     258,   259,   260,   261,    -1,   263,   264,   265,    -1,   267,
     268,    -1,   270,    -1,   272,   273,   274,   275,    -1,   277,
     278,   279,   280,    -1,    -1,   283,   284,   285,   286,   287,
      -1,    -1,   290,   291,   292,   293,   294,   295,    -1,   297,
     298,   299,    -1,    -1,   302,   303,   304,   305,   306,   307,
      -1,   309,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    -1,    17,    -1,    19,    20,
      21,    -1,    23,    24,    25,    -1,    27,    -1,    29,    30,
      -1,    32,    33,    34,    35,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    56,    57,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    98,    99,   100,
     101,    -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,
      -1,    -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,
      -1,   122,   123,   124,    -1,   126,   127,   128,   129,    -1,
      -1,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,    -1,    -1,   145,    -1,   147,   148,   149,   150,
      -1,    -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,    -1,   173,    -1,   175,   176,   177,   178,   179,    -1,
     181,   182,    -1,    -1,   185,   186,   187,    -1,    -1,   190,
      -1,    -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,
     201,   202,   203,   204,   205,   206,    -1,   208,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,    -1,
     231,   232,   233,   234,   235,   236,   237,   238,    -1,   240,
     241,   242,    -1,   244,   245,   246,   247,    -1,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,   273,   274,   275,    -1,   277,   278,   279,   280,
      -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,   290,
     291,   292,   293,   294,   295,    -1,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,     3,
       4,     5,     6,     7,     8,     9,    -1,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,
      24,    25,    -1,    27,    -1,    29,    30,    -1,    32,    33,
      34,    35,    -1,    -1,    38,    39,    40,    41,    -1,    43,
      44,    45,    46,    47,    -1,    -1,    50,    51,    52,    -1,
      54,    55,    56,    57,    -1,    59,    60,    -1,    -1,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    -1,    77,    78,    79,    80,    81,    -1,    -1,
      -1,    85,    86,    87,    88,    -1,    90,    91,    -1,    93,
      -1,    95,    96,    97,    98,    99,   100,   101,    -1,   103,
     104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,
     114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,
     124,    -1,   126,   127,   128,   129,    -1,    -1,   132,   133,
     134,   135,   136,   137,   138,    -1,   140,    -1,   142,    -1,
      -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,   153,
      -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,    -1,   173,
      -1,   175,   176,   177,   178,   179,    -1,   181,   182,    -1,
      -1,   185,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,
     194,   195,    -1,    -1,   198,    -1,    -1,   201,   202,   203,
     204,   205,   206,    -1,   208,   209,   210,   211,    -1,   213,
     214,   215,   216,   217,   218,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,    -1,   231,   232,   233,
     234,   235,   236,   237,   238,    -1,   240,   241,   242,    -1,
     244,   245,   246,   247,    -1,   249,   250,    -1,   252,   253,
     254,   255,   256,   257,   258,   259,   260,   261,    -1,   263,
     264,   265,    -1,   267,   268,    -1,   270,    -1,   272,   273,
     274,   275,    -1,   277,   278,   279,   280,    -1,    -1,   283,
     284,   285,   286,   287,    -1,    -1,   290,   291,   292,   293,
     294,   295,    -1,   297,   298,   299,    -1,    -1,   302,   303,
     304,   305,   306,   307,    -1,   309,     3,     4,     5,     6,
       7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    19,    20,    21,    -1,    23,    24,    25,    -1,
      27,    -1,    29,    30,    -1,    32,    33,    34,    35,    -1,
      -1,    38,    39,    40,    41,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,    -1,    54,    55,    56,
      57,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    -1,
      77,    78,    79,    80,    81,    -1,    -1,    -1,    85,    86,
      87,    88,    -1,    90,    91,    -1,    93,    -1,    95,    96,
      97,    98,    99,   100,   101,    -1,   103,   104,   105,    -1,
     107,    -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,
     117,    -1,   119,   120,    -1,   122,   123,   124,    -1,   126,
     127,   128,   129,    -1,    -1,   132,   133,   134,   135,   136,
     137,   138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,
     147,   148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,
     157,   158,    -1,    -1,   161,   162,   163,   164,   165,   166,
     167,   168,   169,   170,   171,    -1,   173,    -1,   175,   176,
     177,   178,   179,    -1,   181,   182,    -1,    -1,   185,   186,
     187,    -1,    -1,   190,    -1,    -1,    -1,   194,   195,    -1,
      -1,   198,    -1,    -1,   201,   202,   203,   204,   205,   206,
      -1,   208,   209,   210,   211,    -1,   213,   214,   215,   216,
     217,   218,   219,    -1,   221,   222,   223,   224,   225,   226,
     227,   228,   229,    -1,   231,   232,   233,   234,   235,   236,
     237,   238,    -1,   240,   241,   242,    -1,   244,   245,   246,
     247,    -1,   249,   250,    -1,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,    -1,   263,   264,   265,    -1,
     267,   268,    -1,   270,    -1,   272,   273,   274,   275,    -1,
     277,   278,   279,   280,    -1,    -1,   283,   284,   285,   286,
     287,    -1,    -1,   290,   291,   292,   293,   294,   295,    -1,
     297,   298,   299,    -1,    -1,   302,   303,   304,   305,   306,
     307,    -1,   309,     3,     4,     5,     6,     7,     8,     9,
      -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
      20,    21,    -1,    23,    24,    25,    -1,    27,    -1,    29,
      30,    -1,    32,    33,    34,    35,    -1,    -1,    38,    39,
      40,    41,    -1,    43,    44,    45,    46,    47,    -1,    -1,
      50,    51,    52,    -1,    54,    55,    56,    57,    -1,    59,
      60,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,    69,
      70,    71,    72,    73,    -1,    75,    -1,    77,    78,    79,
      80,    81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,
      90,    91,    -1,    93,    -1,    95,    96,    97,    98,    99,
     100,   101,    -1,   103,   104,   105,    -1,   107,    -1,   109,
      -1,    -1,    -1,   113,   114,    -1,    -1,   117,    -1,   119,
     120,    -1,   122,   123,   124,    -1,   126,   127,   128,   129,
      -1,    -1,   132,   133,   134,   135,   136,   137,   138,    -1,
     140,    -1,   142,    -1,    -1,   145,    -1,   147,   148,   149,
     150,    -1,    -1,   153,    -1,    -1,   156,   157,   158,    -1,
      -1,   161,   162,   163,   164,   165,   166,   167,   168,   169,
     170,   171,    -1,   173,    -1,   175,   176,   177,   178,   179,
      -1,   181,   182,    -1,    -1,   185,   186,   187,    -1,    -1,
     190,    -1,    -1,    -1,   194,   195,    -1,    -1,   198,    -1,
      -1,   201,   202,   203,   204,   205,   206,    -1,   208,   209,
     210,   211,    -1,   213,   214,   215,   216,   217,   218,   219,
      -1,   221,   222,   223,   224,   225,   226,   227,   228,   229,
      -1,   231,   232,   233,   234,   235,   236,   237,   238,    -1,
     240,   241,   242,    -1,   244,   245,   246,   247,    -1,   249,
     250,    -1,   252,   253,   254,   255,   256,   257,   258,   259,
     260,   261,    -1,   263,   264,   265,    -1,   267,   268,    -1,
     270,    -1,   272,   273,   274,   275,    -1,   277,   278,   279,
     280,    -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,
     290,   291,   292,   293,   294,   295,    -1,   297,   298,   299,
      -1,    -1,   302,   303,   304,   305,   306,   307,    -1,   309,
       3,     4,     5,     6,     7,     8,     9,    -1,    11,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,    -1,
      23,    24,    25,    -1,    27,    -1,    29,    30,    -1,    32,
      33,    34,    35,    -1,    -1,    38,    39,    40,    41,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    50,    51,    52,
      -1,    54,    55,    56,    57,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    71,    72,
      73,    -1,    75,    -1,    77,    78,    79,    80,    81,    -1,
      -1,    -1,    85,    86,    87,    88,    -1,    90,    91,    -1,
      93,    -1,    95,    96,    97,    98,    99,   100,   101,    -1,
     103,   104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,
     113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,
     123,   124,    -1,   126,   127,   128,   129,    -1,    -1,   132,
     133,   134,   135,   136,   137,   138,    -1,   140,    -1,   142,
      -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,
     153,    -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,
     163,   164,   165,   166,   167,   168,   169,   170,   171,    -1,
     173,    -1,   175,   176,   177,   178,   179,    -1,   181,   182,
      -1,    -1,   185,   186,   187,    -1,    -1,   190,    -1,    -1,
      -1,   194,   195,    -1,    -1,   198,    -1,    -1,   201,   202,
     203,   204,   205,   206,    -1,   208,   209,   210,   211,    -1,
     213,   214,   215,   216,   217,   218,   219,    -1,   221,   222,
     223,   224,   225,   226,   227,   228,   229,    -1,   231,   232,
     233,   234,   235,   236,   237,   238,    -1,   240,   241,   242,
      -1,   244,   245,   246,   247,    -1,   249,   250,    -1,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,    -1,
     263,   264,   265,    -1,   267,   268,    -1,   270,    -1,   272,
     273,   274,   275,    -1,   277,   278,   279,   280,    -1,    -1,
     283,   284,   285,   286,   287,    -1,    -1,   290,   291,   292,
     293,   294,   295,    -1,   297,   298,   299,    -1,    -1,   302,
     303,   304,   305,   306,   307,    -1,   309,     3,     4,     5,
       6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    19,    20,    21,    -1,    23,    24,    25,
      -1,    27,    -1,    29,    30,    -1,    32,    33,    34,    35,
      -1,    -1,    38,    39,    40,    41,    -1,    43,    44,    45,
      46,    47,    -1,    -1,    50,    51,    52,    -1,    54,    55,
      56,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,
      66,    67,    68,    69,    70,    71,    72,    73,    -1,    75,
      -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,    85,
      86,    87,    88,    -1,    90,    91,    -1,    93,    -1,    95,
      96,    97,    98,    99,   100,   101,    -1,   103,   104,   105,
      -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,    -1,
      -1,   117,    -1,   119,   120,    -1,   122,   123,   124,    -1,
     126,   127,   128,   129,    -1,    -1,   132,   133,   134,   135,
     136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,   145,
      -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,    -1,
     156,   157,   158,    -1,    -1,   161,   162,   163,   164,   165,
     166,   167,   168,   169,   170,   171,    -1,   173,    -1,   175,
     176,   177,   178,   179,    -1,   181,   182,    -1,    -1,   185,
     186,   187,    -1,    -1,   190,    -1,    -1,    -1,   194,   195,
      -1,    -1,   198,    -1,    -1,   201,   202,   203,   204,   205,
     206,    -1,   208,   209,   210,   211,    -1,   213,   214,   215,
     216,   217,   218,   219,    -1,   221,   222,   223,   224,   225,
     226,   227,   228,   229,    -1,   231,   232,   233,   234,   235,
     236,   237,   238,    -1,   240,   241,   242,    -1,   244,   245,
     246,   247,    -1,   249,   250,    -1,   252,   253,   254,   255,
     256,   257,   258,   259,   260,   261,    -1,   263,   264,   265,
      -1,   267,   268,    -1,   270,    -1,   272,   273,   274,   275,
      -1,   277,   278,   279,   280,    -1,    -1,   283,   284,   285,
     286,   287,    -1,    -1,   290,   291,   292,   293,   294,   295,
      -1,   297,   298,   299,    -1,    -1,   302,   303,   304,   305,
     306,   307,    -1,   309,     3,     4,     5,     6,     7,     8,
       9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      19,    20,    21,    -1,    23,    24,    25,    -1,    27,    -1,
      29,    30,    -1,    32,    33,    34,    35,    -1,    -1,    38,
      39,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    50,    51,    52,    -1,    54,    55,    56,    57,    -1,
      59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,    68,
      69,    70,    71,    72,    73,    -1,    75,    -1,    77,    78,
      79,    80,    81,    -1,    -1,    -1,    85,    86,    87,    88,
      -1,    90,    91,    -1,    93,    -1,    95,    96,    97,    98,
      99,   100,   101,    -1,   103,   104,   105,    -1,   107,    -1,
     109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,    -1,
     119,   120,    -1,   122,   123,   124,    -1,   126,   127,   128,
     129,    -1,    -1,   132,   133,   134,   135,   136,   137,   138,
      -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,   148,
     149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,   158,
      -1,    -1,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,    -1,   173,    -1,   175,   176,   177,   178,
     179,    -1,   181,   182,    -1,    -1,   185,   186,   187,    -1,
      -1,   190,    -1,    -1,    -1,   194,   195,    -1,    -1,   198,
      -1,    -1,   201,   202,   203,   204,   205,   206,    -1,   208,
     209,   210,   211,    -1,   213,   214,   215,   216,   217,   218,
     219,    -1,   221,   222,   223,   224,   225,   226,   227,   228,
     229,    -1,   231,   232,   233,   234,   235,   236,   237,   238,
      -1,   240,   241,   242,    -1,   244,   245,   246,   247,    -1,
     249,   250,    -1,   252,   253,   254,   255,   256,   257,   258,
     259,   260,   261,    -1,   263,   264,   265,    -1,   267,   268,
      -1,   270,    -1,   272,   273,   274,   275,    -1,   277,   278,
     279,   280,    -1,    -1,   283,   284,   285,   286,   287,    -1,
      -1,   290,   291,   292,   293,   294,   295,    -1,   297,   298,
     299,    -1,    -1,   302,   303,   304,   305,   306,   307,    -1,
     309,     3,     4,     5,     6,     7,     8,     9,    -1,    11,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,    21,
      -1,    23,    24,    25,    -1,    27,    -1,    29,    30,    -1,
      32,    33,    34,    35,    -1,    -1,    38,    39,    40,    41,
      -1,    43,    44,    45,    46,    -1,    -1,    -1,    50,    51,
      52,    -1,    54,    55,    -1,    57,    -1,    59,    60,    -1,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    71,
      72,    73,    -1,    75,    -1,    77,    78,    79,    80,    81,
      -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,    91,
      -1,    93,    -1,    95,    96,    97,    -1,    99,   100,    -1,
      -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,    -1,
      -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,    -1,
     122,   123,   124,    -1,   126,   127,   128,   129,    -1,    -1,
     132,   133,   134,   135,   136,   137,   138,    -1,   140,    -1,
     142,    -1,    -1,   145,    -1,   147,   148,   149,   150,    -1,
      -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,   161,
     162,   163,   164,   165,   166,   167,   168,   169,   170,   171,
      -1,   173,    -1,   175,   176,   177,   178,   179,    -1,   181,
     182,    -1,    -1,    -1,   186,   187,    -1,    -1,   190,    -1,
      -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,    -1,
     202,   203,   204,   205,   206,    -1,    -1,   209,   210,   211,
      -1,   213,   214,   215,   216,   217,   218,   219,    -1,   221,
     222,   223,   224,   225,   226,   227,   228,   229,    -1,   231,
      -1,   233,   234,   235,   236,   237,   238,    -1,   240,   241,
     242,    -1,   244,   245,   246,   247,    -1,   249,   250,    -1,
     252,   253,   254,   255,   256,   257,   258,   259,    -1,   261,
      -1,   263,   264,   265,    -1,   267,   268,    -1,   270,    -1,
     272,    -1,   274,    -1,    -1,   277,   278,   279,   280,    -1,
      -1,   283,   284,   285,   286,   287,    -1,    -1,   290,   291,
     292,   293,   294,   295,    -1,   297,   298,   299,    -1,    -1,
     302,   303,   304,   305,   306,   307,    -1,   309,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,    24,
      25,    -1,    27,    -1,    29,    30,    -1,    32,    33,    34,
      35,    -1,    -1,    38,    39,    40,    41,    -1,    43,    44,
      45,    46,    -1,    -1,    -1,    50,    51,    52,    -1,    54,
      55,    -1,    57,    -1,    59,    60,    -1,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    71,    72,    73,    -1,
      75,    -1,    77,    78,    79,    80,    81,    -1,    -1,    -1,
      85,    86,    87,    88,    -1,    90,    91,    -1,    93,    -1,
      95,    96,    97,    -1,    99,   100,    -1,    -1,   103,   104,
     105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,   114,
      -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,   124,
     125,   126,   127,   128,   129,    -1,    -1,   132,   133,   134,
     135,   136,   137,   138,    -1,   140,    -1,   142,    -1,    -1,
     145,    -1,   147,   148,   149,   150,    -1,    -1,   153,    -1,
      -1,   156,   157,   158,    -1,    -1,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,    -1,   173,    -1,
     175,   176,   177,   178,    -1,    -1,   181,   182,    -1,    -1,
      -1,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,   194,
     195,    -1,    -1,   198,    -1,    -1,    -1,   202,   203,   204,
     205,   206,    -1,    -1,   209,   210,   211,    -1,   213,   214,
     215,   216,   217,   218,   219,    -1,   221,   222,   223,   224,
     225,   226,   227,   228,   229,    -1,   231,    -1,   233,   234,
     235,   236,   237,   238,    -1,   240,   241,   242,    -1,   244,
     245,   246,   247,    -1,   249,   250,    -1,   252,   253,   254,
     255,   256,   257,   258,   259,    -1,   261,    -1,   263,   264,
     265,    -1,   267,   268,    -1,   270,    -1,   272,    -1,   274,
      -1,    -1,   277,   278,   279,   280,    -1,    -1,   283,   284,
     285,   286,   287,    -1,    -1,   290,   291,   292,   293,   294,
     295,    -1,   297,   298,   299,    -1,    -1,   302,   303,   304,
     305,   306,   307,    -1,   309,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    21,    -1,    23,    24,    25,    -1,    27,
      -1,    29,    30,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    39,    40,    41,    -1,    43,    44,    45,    46,    -1,
      -1,    -1,    50,    51,    52,    -1,    54,    55,    -1,    57,
      -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    66,    67,
      68,    69,    70,    71,    72,    73,    -1,    75,    -1,    77,
      78,    79,    80,    81,    -1,    -1,    -1,    85,    86,    87,
      88,    -1,    90,    91,    -1,    93,    -1,    95,    96,    97,
      -1,    99,   100,    -1,    -1,   103,   104,   105,    -1,   107,
      -1,   109,    -1,    -1,    -1,   113,   114,    -1,    -1,   117,
      -1,   119,   120,    -1,   122,   123,   124,    -1,   126,   127,
     128,   129,    -1,    -1,   132,   133,   134,   135,   136,   137,
     138,    -1,   140,    -1,   142,    -1,    -1,   145,    -1,   147,
     148,   149,   150,    -1,    -1,   153,    -1,    -1,   156,   157,
     158,    -1,    -1,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,    -1,   173,    -1,   175,   176,   177,
     178,   179,    -1,   181,   182,    -1,    -1,    -1,   186,   187,
      -1,    -1,   190,    -1,    -1,    -1,   194,   195,    -1,    -1,
     198,    -1,    -1,    -1,   202,   203,   204,   205,   206,    -1,
      -1,   209,   210,   211,    -1,   213,   214,   215,   216,   217,
     218,   219,    -1,   221,   222,   223,   224,   225,   226,   227,
     228,   229,    -1,   231,    -1,   233,   234,   235,   236,   237,
     238,    -1,   240,   241,   242,    -1,   244,   245,   246,   247,
      -1,   249,   250,    -1,   252,   253,   254,   255,   256,   257,
     258,   259,    -1,   261,    -1,   263,   264,   265,    -1,   267,
     268,    -1,   270,    -1,   272,    -1,   274,    -1,    -1,   277,
     278,   279,   280,    -1,    -1,   283,   284,   285,   286,   287,
      -1,    -1,   290,   291,   292,   293,   294,   295,    -1,   297,
     298,   299,    -1,    -1,   302,   303,   304,   305,   306,   307,
      -1,   309,     3,     4,     5,     6,     7,     8,     9,    -1,
      11,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,    20,
      21,    -1,    23,    24,    25,    -1,    27,    -1,    29,    30,
      -1,    32,    33,    34,    35,    -1,    -1,    38,    39,    40,
      41,    -1,    43,    44,    45,    46,    -1,    -1,    -1,    50,
      51,    52,    -1,    54,    55,    -1,    57,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,
      71,    72,    73,    -1,    75,    -1,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    85,    86,    87,    88,    -1,    90,
      91,    -1,    93,    -1,    95,    96,    97,    -1,    99,   100,
      -1,    -1,   103,   104,   105,    -1,   107,    -1,   109,    -1,
      -1,    -1,   113,   114,    -1,    -1,   117,    -1,   119,   120,
      -1,   122,   123,   124,    -1,   126,   127,   128,   129,    -1,
      -1,   132,   133,   134,   135,   136,   137,   138,    -1,   140,
      -1,   142,    -1,    -1,   145,    -1,   147,   148,   149,   150,
      -1,    -1,   153,    -1,    -1,   156,   157,   158,    -1,    -1,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,    -1,   173,    -1,   175,   176,   177,   178,    -1,    -1,
     181,   182,    -1,    -1,    -1,   186,   187,    -1,    -1,   190,
      -1,    -1,    -1,   194,   195,    -1,    -1,   198,    -1,    -1,
      -1,   202,   203,   204,   205,   206,    -1,    -1,   209,   210,
     211,    -1,   213,   214,   215,   216,   217,   218,   219,    -1,
     221,   222,   223,   224,   225,   226,   227,   228,   229,    -1,
     231,    -1,   233,   234,   235,   236,   237,   238,    -1,   240,
     241,   242,    -1,   244,   245,   246,   247,    -1,   249,   250,
      -1,   252,   253,   254,   255,   256,   257,   258,   259,    -1,
     261,    -1,   263,   264,   265,    -1,   267,   268,    -1,   270,
      -1,   272,    -1,   274,    -1,    -1,   277,   278,   279,   280,
      -1,    -1,   283,   284,   285,   286,   287,    -1,    -1,   290,
     291,   292,   293,   294,   295,    -1,   297,   298,   299,    -1,
      -1,   302,   303,   304,   305,   306,   307,    -1,   309,     3,
       4,     5,     6,     7,     8,     9,    -1,    11,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    19,    20,    21,    -1,    23,
      24,    25,    -1,    27,    -1,    29,    30,    -1,    32,    33,
      34,    35,    -1,    -1,    38,    39,    40,    41,    -1,    43,
      44,    45,    46,    -1,    -1,    -1,    50,    51,    52,    -1,
      54,    55,    -1,    57,    -1,    59,    60,    -1,    -1,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      -1,    75,    -1,    77,    78,    79,    80,    81,    -1,    -1,
      -1,    85,    86,    87,    88,    -1,    90,    91,    -1,    93,
      -1,    95,    96,    97,    -1,    99,   100,    -1,    -1,   103,
     104,   105,    -1,   107,    -1,   109,    -1,    -1,    -1,   113,
     114,    -1,    -1,   117,    -1,   119,   120,    -1,   122,   123,
     124,    -1,   126,   127,   128,   129,    -1,    -1,   132,   133,
     134,   135,   136,   137,   138,    -1,   140,    -1,   142,    -1,
      -1,   145,    -1,   147,   148,   149,   150,    -1,    -1,   153,
      -1,    -1,   156,   157,   158,    -1,    -1,   161,   162,   163,
     164,   165,   166,   167,   168,   169,   170,   171,    -1,   173,
      -1,   175,   176,   177,   178,    -1,    -1,   181,   182,    -1,
      -1,    -1,   186,   187,    -1,    -1,   190,    -1,    -1,    -1,
     194,   195,    -1,    -1,   198,    -1,    -1,    -1,   202,   203,
     204,   205,   206,    -1,    -1,   209,   210,   211,    -1,   213,
     214,   215,   216,   217,   218,   219,    -1,   221,   222,   223,
     224,   225,   226,   227,   228,   229,    -1,   231,    -1,   233,
     234,   235,   236,   237,   238,    -1,   240,   241,   242,    -1,
     244,    -1,   246,   247,    -1,   249,   250,    -1,   252,   253,
     254,   255,   256,   257,   258,   259,    -1,   261,    -1,   263,
     264,   265,    -1,   267,   268,    -1,   270,    -1,   272,    -1,
     274,    -1,    -1,   277,   278,   279,   280,    -1,    -1,   283,
     284,   285,   286,   287,    -1,    -1,   290,   291,   292,   293,
     294,   295,    -1,   297,   298,   299,     9,    -1,   302,   303,
     304,   305,   306,   307,    -1,   309,    19,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      53,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    68,    -1,    -1,    -1,    -1,
      -1,    74,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   114,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   128,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   149,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   158,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   194,    -1,   196,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   215,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   235,    -1,    -1,    -1,    -1,   240,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   262,
     263,    -1,   265,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   274,    -1,    -1,    -1,   278,   279,    -1,    -1,   282,
      -1,    -1,    -1,    -1,    -1,   288
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned short yystos[] =
{
       0,     3,    11,    12,    13,    25,    43,    45,    46,    50,
      51,    57,    58,    70,    73,    79,    87,    92,    97,    99,
     103,   115,   135,   156,   157,   162,   169,   182,   211,   221,
     225,   229,   231,   239,   244,   247,   253,   277,   284,   286,
     290,   330,   338,   339,   340,   341,   343,   344,   345,   349,
     352,   354,   355,   359,   369,   370,   371,   374,   375,   378,
     379,   388,   413,   417,   418,   425,   430,   432,   445,   446,
     447,   448,   453,   458,   459,   463,   464,   467,   471,   472,
     483,   489,   501,   502,   504,   507,   509,   510,   513,   516,
     524,   525,   526,   527,   528,   533,   534,   535,   539,   540,
     541,   542,   544,   545,   546,   547,   548,   553,   556,   560,
     562,   563,   567,   568,   571,   572,   575,   576,   577,   578,
     579,   272,   304,   529,     9,    55,    68,    85,   113,   116,
     149,   194,   235,   240,   262,   274,   288,   529,     3,     4,
       5,     6,     7,     8,     9,    11,    19,    20,    21,    23,
      24,    25,    27,    29,    30,    32,    33,    34,    35,    38,
      39,    40,    41,    43,    44,    45,    46,    47,    50,    51,
      52,    54,    55,    56,    57,    59,    60,    66,    67,    68,
      69,    70,    71,    72,    73,    75,    77,    78,    79,    80,
      81,    85,    86,    87,    88,    90,    91,    93,    95,    96,
      97,    98,    99,   100,   101,   103,   104,   105,   107,   109,
     113,   114,   117,   119,   120,   122,   123,   124,   126,   127,
     128,   129,   132,   133,   134,   135,   136,   137,   138,   140,
     142,   145,   147,   148,   149,   150,   153,   156,   157,   158,
     161,   162,   163,   164,   165,   166,   167,   168,   169,   170,
     171,   173,   175,   176,   177,   178,   179,   181,   182,   185,
     186,   187,   190,   194,   195,   198,   201,   202,   203,   204,
     205,   206,   208,   209,   210,   211,   213,   214,   215,   216,
     217,   218,   219,   221,   222,   223,   224,   225,   226,   227,
     228,   229,   231,   232,   233,   234,   235,   236,   237,   238,
     240,   241,   242,   244,   245,   246,   247,   249,   250,   252,
     253,   254,   255,   256,   257,   258,   259,   260,   261,   263,
     264,   265,   267,   268,   270,   272,   273,   274,   275,   277,
     278,   279,   280,   283,   284,   285,   286,   287,   290,   291,
     292,   293,   294,   295,   297,   298,   299,   302,   303,   304,
     305,   306,   307,   309,   679,   690,   694,   695,   174,   191,
     667,   675,   677,   683,   690,   698,   192,   529,    28,   384,
       9,    19,    37,    53,    68,    74,    85,   114,   116,   158,
     194,   196,   235,   263,   265,   274,   278,   279,   282,   288,
     389,   426,   456,   484,   490,   211,   679,   679,   111,     9,
      19,    37,    55,    68,    85,   113,   116,   128,   194,   215,
     234,   235,   240,   262,   274,   279,   288,   298,   431,   460,
     529,   679,   548,   555,     4,    10,    23,   104,   109,   150,
     175,   213,   222,   315,   322,   468,   469,   679,   687,    10,
      58,    79,    97,   135,   220,   234,   239,   263,   265,   274,
     286,   287,   473,   474,   475,   141,   677,   690,   311,   684,
     688,   262,   582,   468,   679,   677,   679,    68,   128,   262,
     511,    10,   242,   267,   272,   690,   115,   480,   529,    10,
      83,   584,    54,   158,   170,   242,   267,   272,   360,   690,
      10,   242,   267,   272,   690,   272,   582,   323,   677,   193,
     606,   677,   112,   550,   576,   577,     0,   334,   296,   549,
      94,   139,   197,   281,   585,   586,    22,    26,    28,    61,
     110,   112,   121,   125,   131,   143,   144,   146,   152,   154,
     172,   183,   199,   200,   230,   248,   296,   309,   667,   675,
     685,   692,   694,   696,   462,   667,   690,   680,   690,   462,
     685,   689,   690,   679,    44,   679,   677,   606,   677,   679,
     689,   333,   668,   192,     9,    49,    53,    68,    85,   113,
     128,   194,   234,   235,   240,   262,   274,   279,   298,   465,
     677,   685,   679,   330,   274,   680,   462,   263,   265,   689,
     263,   265,    44,   314,   317,   318,   319,   321,   322,   323,
     324,   325,   326,   506,   639,   640,   690,   224,   356,   690,
     679,   462,   689,   240,   262,   431,    55,   128,   113,   234,
     298,   679,   573,   606,   685,   679,   330,   680,   685,   689,
      44,   506,   679,   679,   348,   689,   149,   461,   462,   330,
     561,   549,   469,    10,   469,    10,   469,   469,   687,   111,
     125,   470,   214,   192,   335,   677,   676,   677,   470,   330,
     557,   679,   677,    22,   307,   145,   195,   473,   192,    16,
      27,    29,    30,    36,    37,    39,    40,    47,    56,    62,
      63,    64,    65,    71,    72,    86,    98,   101,   102,   105,
     137,   138,   140,   159,   160,   171,   173,   180,   184,   185,
     186,   194,   201,   208,   218,   232,   243,   250,   260,   267,
     268,   273,   275,   276,   282,   288,   294,   309,   310,   312,
     313,   314,   316,   321,   322,   323,   325,   326,   330,   576,
     614,   615,   616,   621,   622,   623,   625,   626,   627,   628,
     631,   632,   635,   636,   641,   643,   645,   661,   666,   667,
     669,   670,   675,   685,   686,   687,   688,   691,   694,    10,
     372,   678,   679,   242,   360,    74,   367,   688,    22,    41,
     360,   307,   145,   217,   530,   532,   269,   317,    22,   307,
     145,   530,   531,   677,   330,   677,   244,   323,   110,   551,
     331,   331,   340,   677,    10,    83,   583,   583,    32,   583,
     106,   155,   189,   589,   595,   330,   223,   223,   244,   370,
       7,    87,   202,   244,   376,   330,   491,     7,    87,   223,
     353,   223,   462,   223,   419,     7,    11,    87,   223,   244,
      46,    58,   202,   192,   223,   244,   302,   342,   370,   323,
     682,   690,   677,   685,   679,   685,   506,   679,   679,   462,
     330,   402,   330,   449,    42,    27,    29,    30,    39,    40,
      71,    72,   105,   137,   138,   140,   173,   186,   218,   245,
     250,   267,   268,   294,   309,   611,   613,   615,   616,   620,
     622,   623,   624,   626,   627,   631,   632,   691,   694,   679,
     342,    17,   543,   342,   462,   449,   333,    22,   357,     8,
      24,   433,    17,   449,   342,   677,   677,   149,   462,   683,
     690,   685,   679,   677,    28,    66,   134,   176,   236,   301,
     608,   330,    35,   227,   377,   611,   491,   462,   330,   192,
     192,   335,   311,   368,   690,   335,   377,   643,   647,    58,
     554,   560,   563,   567,   571,   572,   575,   679,    68,   113,
     149,   235,   262,   476,   676,   475,    74,   293,   330,   564,
     575,   125,   335,   569,   679,   558,   611,    17,   107,   512,
     512,   153,   106,   192,   330,   328,   576,   651,   295,   629,
     643,   665,   330,   629,   629,   330,   330,   330,   330,   330,
     619,   619,   209,   576,   330,   330,   617,   330,   330,    39,
      40,   629,   643,   330,   330,   618,   330,   330,   330,   330,
     330,   302,   303,   330,   633,   330,   633,   330,   330,   576,
     646,   668,   643,   643,   643,   643,   576,   637,   643,   647,
     688,    40,   330,   630,   330,   688,   125,   143,   180,   194,
     200,   639,   642,   643,    14,    17,    21,    26,   121,   125,
     143,   144,   154,   180,   183,   194,   196,   248,   314,   317,
     318,   319,   321,   322,   323,   324,   325,   326,   332,   641,
     642,   646,   141,   335,   580,   646,   330,    77,   122,   373,
     335,    41,    74,   368,    17,    74,   158,   309,   310,   322,
     366,   422,   423,   424,   632,   687,   688,   153,   193,   305,
     145,    74,   102,   188,   192,   276,   361,   362,   363,   365,
     368,   422,   361,   153,   677,   671,   672,   690,   549,   330,
     552,   576,   578,   579,   578,   587,   588,   643,   578,   217,
     286,    10,   591,   643,   592,   643,   595,   596,   589,   590,
     323,   503,   611,   269,   269,   360,    42,    53,   108,   212,
     282,   400,   401,    53,    74,   180,   269,    74,   180,   125,
     132,   198,   331,   492,   493,   494,   496,   611,   691,   223,
     269,   288,   269,   289,   269,    33,    67,   127,   164,   166,
     176,   226,   253,   420,    49,   400,   515,   515,    53,   515,
     515,   303,   192,   270,   269,   606,   269,   360,   346,   668,
     330,   192,   491,   330,   143,   192,   192,   143,   403,   404,
     690,   302,   385,    10,    12,    13,    14,    15,    16,    17,
      18,    31,    36,    37,    42,    48,    49,    53,    58,    62,
      63,    64,    65,    74,    76,    82,    83,    84,    89,    92,
      94,   102,   106,   108,   111,   115,   116,   118,   130,   139,
     141,   151,   155,   159,   160,   174,   180,   184,   188,   189,
     191,   192,   193,   196,   197,   207,   212,   220,   239,   243,
     251,   262,   266,   269,   271,   276,   281,   282,   288,   289,
     300,   301,   309,   450,   451,   693,   694,   695,   696,   697,
     330,   613,    17,    16,   612,    69,   120,   165,   168,   237,
     306,   330,   634,   668,     8,   536,   611,   350,   456,   506,
     689,    58,   358,   388,   471,   533,    79,   135,   286,   434,
     435,   330,   346,   419,   187,   330,   414,   368,   106,   192,
     491,    17,   402,   302,   303,   574,   236,   643,   503,    17,
     377,   289,   179,   505,   611,   677,   677,   689,   377,   462,
     331,   335,   389,   678,   481,   482,   685,   678,   678,   676,
     269,   293,   330,   565,   566,   690,     5,    96,   232,   246,
     570,   677,   331,   335,   559,   563,   567,   571,   575,   476,
     647,   647,   650,   651,   330,   300,   662,   663,   643,   647,
     643,   647,   687,   687,   687,    69,   120,   165,   168,   237,
     306,   309,   311,   648,   652,   687,   687,   687,   629,   629,
     643,   687,   506,   643,   653,   321,   322,   325,   326,   330,
     641,   644,   645,   655,   331,   637,   643,   643,   647,   656,
     267,   267,   687,   687,   643,    31,   111,   151,   271,   643,
     647,   659,   328,   646,   331,   331,   335,   244,   687,   687,
     634,   576,    83,   180,   184,   125,   330,   232,   330,   636,
      10,    15,   251,   330,   576,   636,   638,   643,   693,   267,
     644,   643,   330,   576,   660,    83,   102,   180,   184,   187,
     276,   283,   643,    26,   121,   125,   154,   248,   330,   643,
     269,   643,   643,   643,   643,   643,   643,   643,   643,   643,
     611,   643,   638,   114,   158,   262,   263,   265,   581,   677,
     670,   111,   598,    10,    83,   323,   331,   647,   679,   272,
     310,   687,   330,   688,   217,   241,   364,   153,   335,   331,
     335,   598,   646,   547,   677,   678,   335,    18,    82,   289,
     193,   187,   597,   189,   335,   155,   331,   679,   680,   330,
     679,   147,   147,   330,   679,   184,   689,   643,   184,   331,
     335,   496,   668,   269,   689,   348,   679,   681,   690,   679,
     422,    32,   421,   422,   422,    67,   164,   166,   342,   342,
     393,   690,   690,   679,   690,   514,   679,   190,   679,   262,
     689,   223,   689,    59,    60,    91,   125,   177,   178,   204,
     261,   280,   291,   347,   503,   462,   143,   505,   184,   466,
     688,   462,   462,   466,   331,   335,   190,   111,   269,   380,
     331,   335,   317,   643,    16,   612,   611,   328,   328,   269,
     269,   269,   269,   687,   434,    90,   161,   202,   264,   537,
     394,   261,   288,   351,   106,   357,   389,   490,   192,   196,
     609,   610,   690,   677,   154,   390,   391,   392,   393,   398,
     400,   415,   416,   690,    17,   117,   688,   677,   228,   517,
      17,   119,   119,   106,   331,   611,   681,   335,   331,   335,
     377,   377,   643,   262,   335,   491,   116,   477,   478,   690,
      74,   670,   673,   674,   331,   335,   646,    96,   246,    96,
     246,   232,   286,   167,   611,   111,   331,   329,   329,   335,
     687,   643,    89,   663,   664,    17,   331,   289,   331,   331,
     331,   331,   335,   331,   111,   331,   331,   331,   335,   331,
     335,   331,   207,   654,   331,   644,   644,   644,   644,   643,
     644,   125,   143,   194,   317,   318,   319,   321,   322,   323,
     324,   325,   326,   332,   641,   331,   331,   331,   106,   111,
     657,   658,   331,   307,   307,   331,   331,    17,   659,   647,
     659,   659,   111,   331,   643,   646,   668,   643,   690,   331,
     331,   111,   184,   576,   506,   576,   307,    14,    93,   647,
     111,   102,   184,   187,   276,   283,   330,    93,   644,   643,
     660,   643,   269,   506,   643,   330,   576,   263,   265,   263,
     265,   677,   582,   582,   330,   576,   599,   600,   601,   606,
     607,   667,   685,   608,   647,   647,   331,   331,   530,   687,
     634,    52,   532,   364,   363,   672,   608,   317,   331,   588,
     642,   678,   592,   592,   591,   223,   643,   401,   330,   330,
     403,   377,   493,   325,   679,   223,   422,   422,   422,   611,
      87,   244,   376,   377,   377,   269,   269,   204,   116,   688,
     687,   204,   285,   331,   143,   466,   331,   143,   143,   404,
     256,   257,   381,   688,   451,   194,   322,   422,   452,   495,
     496,   642,   688,   331,   328,   331,   687,   329,   687,   120,
     165,   237,   165,   237,   237,   168,   331,   192,   317,   538,
     538,   538,   538,    42,    53,    74,    76,   130,   180,   184,
     212,   220,   282,   395,   396,   397,   687,   348,   279,   262,
     677,   435,   331,   335,   611,   330,   677,   331,   335,   331,
     335,    97,   575,   427,   667,   679,   690,   269,   289,   485,
     495,   192,   575,   575,   377,   331,   377,   611,   377,   179,
     611,   677,   482,   690,   302,   335,   479,   331,   335,   293,
     575,   566,    96,    96,   477,   651,   331,   266,   643,    92,
     611,   462,   687,   643,   643,   687,   643,   657,   644,    83,
     180,   187,   644,   644,   644,   644,   644,   644,   644,   644,
     644,   611,   644,   643,   643,   658,   657,   633,   633,   611,
     331,   331,   331,   647,   329,   336,   646,   630,   688,   636,
     331,   645,   644,   643,   331,   643,   330,   611,   649,   643,
      14,    93,    93,   643,   331,    93,   643,   582,   582,   582,
     582,   677,   677,   576,   600,   601,    17,   602,   690,   335,
      61,   112,   131,   146,   152,   172,   230,   308,   603,   602,
      17,   602,   690,   330,   116,   593,   331,   331,   331,    74,
     643,   269,   331,   403,   403,   331,   279,   269,   394,   180,
     180,   255,   258,   679,   679,   688,   348,   688,   688,   143,
     466,   143,   466,   466,   289,   386,   387,    76,   130,   180,
     442,   443,   444,   687,   302,   303,   329,   329,   634,   677,
      74,   687,   688,    74,   688,    74,   679,    74,   679,   330,
     679,   644,    77,   122,    76,   184,   147,   677,   611,   677,
     106,   436,   196,   610,   390,    95,   126,   399,   129,   410,
     392,   416,   690,   679,   292,   429,   688,   681,   330,    17,
      34,   100,   123,   149,   228,   238,   252,   259,   299,   497,
     498,    79,   135,   239,   286,   522,   377,   330,   414,   115,
     478,   674,   330,   377,   643,   331,   331,   331,   331,   331,
     658,   111,   187,   330,   331,   643,   634,   649,   331,   335,
     644,   643,   643,    93,   643,   331,   677,   677,   677,   677,
     331,   690,   330,   600,   146,   199,   604,   600,   604,   146,
     603,   604,   600,   146,   330,   690,   330,   331,   647,    32,
     118,   594,   688,   679,   331,   331,   679,   184,   184,   322,
     424,   690,   466,   466,   342,    81,    77,   122,    76,   444,
     443,   329,   113,   113,   111,   441,   643,   180,   396,   402,
     289,   330,    88,   437,    97,   435,   331,    75,    75,   330,
     302,   303,   411,   561,   427,   148,   428,   111,   330,   486,
     487,   682,   685,   690,   499,   688,   192,   238,   368,   184,
      78,   142,   302,   498,   500,   269,    17,   195,   673,   644,
     330,   649,   329,   331,   611,   643,   602,   330,   678,   600,
     192,   289,   605,   600,   146,   600,   609,   330,   609,   690,
     331,   647,   643,   634,   220,   382,   688,   482,    17,   508,
     677,   442,   331,   163,   405,   681,   232,   254,   438,   216,
     411,   676,   190,   190,   192,   412,   688,   462,   643,   331,
     335,   289,   462,   488,   330,   335,   184,    78,   142,   192,
     449,   677,   331,   649,   331,   678,   331,   643,   330,   600,
     605,   331,   609,   331,   677,    28,    80,   184,   190,   383,
     508,    20,   124,   106,   112,   203,   249,   192,   406,   407,
     408,    17,   685,   412,   331,    51,   331,   608,   487,   462,
     647,   688,   133,   184,   608,   331,   331,   678,   331,   402,
     543,   543,    88,    79,   286,   192,   408,   192,   407,   113,
     194,   258,   454,   455,   330,    79,    87,   210,   488,   331,
     133,    84,   331,   405,   688,   688,   232,    35,   176,   227,
     244,   409,   409,   687,   687,   611,   335,   310,   312,   313,
     315,   439,   440,   688,   690,   233,   233,   488,   136,   523,
     406,    97,     6,    74,   184,   685,   506,   455,   331,   335,
     181,   330,   518,   520,   525,   563,   567,   571,   575,   442,
     216,   491,   219,   330,   457,   440,   519,   520,   521,   576,
     577,   685,   505,   331,   334,   330,   331,   521,   439,   457,
     331
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 461 "gram.y"
    { parsetree = yyvsp[0].list; }
    break;

  case 3:
#line 466 "gram.y"
    { if (yyvsp[0].node != (Node *)NULL)
					yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node);
				  else
					yyval.list = yyvsp[-2].list;
				}
    break;

  case 4:
#line 472 "gram.y"
    { if (yyvsp[0].node != (Node *)NULL)
						yyval.list = makeList1(yyvsp[0].node);
					  else
						yyval.list = NIL;
					}
    break;

  case 76:
#line 552 "gram.y"
    { yyval.node = (Node *)NULL; }
    break;

  case 77:
#line 564 "gram.y"
    {
					CreateUserStmt *n = makeNode(CreateUserStmt);
					n->user = yyvsp[-2].str;
					n->options = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 78:
#line 573 "gram.y"
    {}
    break;

  case 79:
#line 574 "gram.y"
    {}
    break;

  case 80:
#line 586 "gram.y"
    {
					AlterUserStmt *n = makeNode(AlterUserStmt);
					n->user = yyvsp[-2].str;
					n->options = yyvsp[0].list;
					yyval.node = (Node *)n;
				 }
    break;

  case 81:
#line 597 "gram.y"
    {
					AlterUserSetStmt *n = makeNode(AlterUserSetStmt);
					n->user = yyvsp[-2].str;
					n->variable = yyvsp[0].vsetstmt->name;
					n->value = yyvsp[0].vsetstmt->args;
					yyval.node = (Node *)n;
				}
    break;

  case 82:
#line 605 "gram.y"
    {
					AlterUserSetStmt *n = makeNode(AlterUserSetStmt);
					n->user = yyvsp[-1].str;
					n->variable = ((VariableResetStmt *)yyvsp[0].node)->name;
					n->value = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 83:
#line 626 "gram.y"
    {
					DropUserStmt *n = makeNode(DropUserStmt);
					n->users = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 84:
#line 637 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].defelt); }
    break;

  case 85:
#line 638 "gram.y"
    { yyval.list = NIL; }
    break;

  case 86:
#line 643 "gram.y"
    {
					yyval.defelt = makeDefElem("password", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 87:
#line 647 "gram.y"
    {
					yyval.defelt = makeDefElem("encryptedPassword", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 88:
#line 651 "gram.y"
    {
					yyval.defelt = makeDefElem("unencryptedPassword", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 89:
#line 655 "gram.y"
    {
					yyval.defelt = makeDefElem("sysid", (Node *)makeInteger(yyvsp[0].ival));
				}
    break;

  case 90:
#line 659 "gram.y"
    {
					yyval.defelt = makeDefElem("createdb", (Node *)makeInteger(TRUE));
				}
    break;

  case 91:
#line 663 "gram.y"
    {
					yyval.defelt = makeDefElem("createdb", (Node *)makeInteger(FALSE));
				}
    break;

  case 92:
#line 667 "gram.y"
    {
					yyval.defelt = makeDefElem("createuser", (Node *)makeInteger(TRUE));
				}
    break;

  case 93:
#line 671 "gram.y"
    {
					yyval.defelt = makeDefElem("createuser", (Node *)makeInteger(FALSE));
				}
    break;

  case 94:
#line 675 "gram.y"
    {
					yyval.defelt = makeDefElem("groupElts", (Node *)yyvsp[0].list);
				}
    break;

  case 95:
#line 679 "gram.y"
    {
					yyval.defelt = makeDefElem("validUntil", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 96:
#line 684 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, makeString(yyvsp[0].str)); }
    break;

  case 97:
#line 685 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 98:
#line 699 "gram.y"
    {
					CreateGroupStmt *n = makeNode(CreateGroupStmt);
					n->name = yyvsp[-2].str;
					n->options = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 99:
#line 711 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].defelt); }
    break;

  case 100:
#line 712 "gram.y"
    { yyval.list = NIL; }
    break;

  case 101:
#line 717 "gram.y"
    {
					yyval.defelt = makeDefElem("userElts", (Node *)yyvsp[0].list);
				}
    break;

  case 102:
#line 721 "gram.y"
    {
					yyval.defelt = makeDefElem("sysid", (Node *)makeInteger(yyvsp[0].ival));
				}
    break;

  case 103:
#line 736 "gram.y"
    {
					AlterGroupStmt *n = makeNode(AlterGroupStmt);
					n->name = yyvsp[-3].str;
					n->action = yyvsp[-2].ival;
					n->listUsers = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 104:
#line 745 "gram.y"
    { yyval.ival = +1; }
    break;

  case 105:
#line 746 "gram.y"
    { yyval.ival = -1; }
    break;

  case 106:
#line 759 "gram.y"
    {
					DropGroupStmt *n = makeNode(DropGroupStmt);
					n->name = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 107:
#line 775 "gram.y"
    {
					CreateSchemaStmt *n = makeNode(CreateSchemaStmt);
					/* One can omit the schema name or the authorization id. */
					if (yyvsp[-3].str != NULL)
						n->schemaname = yyvsp[-3].str;
					else
						n->schemaname = yyvsp[-1].str;
					n->authid = yyvsp[-1].str;
					n->schemaElts = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 108:
#line 787 "gram.y"
    {
					CreateSchemaStmt *n = makeNode(CreateSchemaStmt);
					/* ...but not both */
					n->schemaname = yyvsp[-1].str;
					n->authid = NULL;
					n->schemaElts = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 109:
#line 798 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 110:
#line 799 "gram.y"
    { yyval.str = NULL; }
    break;

  case 111:
#line 803 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].node); }
    break;

  case 112:
#line 804 "gram.y"
    { yyval.list = NIL; }
    break;

  case 116:
#line 829 "gram.y"
    {
					VariableSetStmt *n = yyvsp[0].vsetstmt;
					n->is_local = false;
					yyval.node = (Node *) n;
				}
    break;

  case 117:
#line 835 "gram.y"
    {
					VariableSetStmt *n = yyvsp[0].vsetstmt;
					n->is_local = true;
					yyval.node = (Node *) n;
				}
    break;

  case 118:
#line 841 "gram.y"
    {
					VariableSetStmt *n = yyvsp[0].vsetstmt;
					n->is_local = false;
					yyval.node = (Node *) n;
				}
    break;

  case 119:
#line 849 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = yyvsp[-2].str;
					n->args = yyvsp[0].list;
					yyval.vsetstmt = n;
				}
    break;

  case 120:
#line 856 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = yyvsp[-2].str;
					n->args = yyvsp[0].list;
					yyval.vsetstmt = n;
				}
    break;

  case 121:
#line 863 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = "timezone";
					if (yyvsp[0].node != NULL)
						n->args = makeList1(yyvsp[0].node);
					yyval.vsetstmt = n;
				}
    break;

  case 122:
#line 871 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = "TRANSACTION";
					n->args = yyvsp[0].list;
					yyval.vsetstmt = n;
				}
    break;

  case 123:
#line 878 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = "SESSION CHARACTERISTICS";
					n->args = yyvsp[0].list;
					yyval.vsetstmt = n;
				}
    break;

  case 124:
#line 885 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = "client_encoding";
					if (yyvsp[0].str != NULL)
						n->args = makeList1(makeStringConst(yyvsp[0].str, NULL));
					yyval.vsetstmt = n;
				}
    break;

  case 125:
#line 893 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = "session_authorization";
					n->args = makeList1(makeStringConst(yyvsp[0].str, NULL));
					yyval.vsetstmt = n;
				}
    break;

  case 126:
#line 900 "gram.y"
    {
					VariableSetStmt *n = makeNode(VariableSetStmt);
					n->name = "session_authorization";
					n->args = NIL;
					yyval.vsetstmt = n;
				}
    break;

  case 127:
#line 909 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 128:
#line 910 "gram.y"
    { yyval.list = NIL; }
    break;

  case 129:
#line 913 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 130:
#line 914 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 131:
#line 918 "gram.y"
    { yyval.node = makeStringConst(yyvsp[0].str, NULL); }
    break;

  case 132:
#line 920 "gram.y"
    { yyval.node = makeStringConst(yyvsp[0].str, NULL); }
    break;

  case 133:
#line 922 "gram.y"
    { yyval.node = makeAConst(yyvsp[0].value); }
    break;

  case 134:
#line 925 "gram.y"
    { yyval.str = "read committed"; }
    break;

  case 135:
#line 926 "gram.y"
    { yyval.str = "serializable"; }
    break;

  case 136:
#line 930 "gram.y"
    { yyval.str = "true"; }
    break;

  case 137:
#line 931 "gram.y"
    { yyval.str = "false"; }
    break;

  case 138:
#line 932 "gram.y"
    { yyval.str = "on"; }
    break;

  case 139:
#line 933 "gram.y"
    { yyval.str = "off"; }
    break;

  case 140:
#line 946 "gram.y"
    {
					yyval.node = makeStringConst(yyvsp[0].str, NULL);
				}
    break;

  case 141:
#line 950 "gram.y"
    {
					yyval.node = makeStringConst(yyvsp[0].str, NULL);
				}
    break;

  case 142:
#line 954 "gram.y"
    {
					A_Const *n = (A_Const *) makeStringConst(yyvsp[-1].str, yyvsp[-2].typnam);
					if (yyvsp[0].ival != INTERVAL_FULL_RANGE)
					{
						if ((yyvsp[0].ival & ~(INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE))) != 0)
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("time zone interval must be HOUR or HOUR TO MINUTE")));
						n->typename->typmod = INTERVAL_TYPMOD(INTERVAL_FULL_PRECISION, yyvsp[0].ival);
					}
					yyval.node = (Node *)n;
				}
    break;

  case 143:
#line 967 "gram.y"
    {
					A_Const *n = (A_Const *) makeStringConst(yyvsp[-1].str, yyvsp[-5].typnam);
					if (yyvsp[-3].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("INTERVAL(%d) precision must not be negative",
										yyvsp[-3].ival)));
					if (yyvsp[-3].ival > MAX_INTERVAL_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("INTERVAL(%d) precision reduced to maximum allowed, %d",
										yyvsp[-3].ival, MAX_INTERVAL_PRECISION)));
						yyvsp[-3].ival = MAX_INTERVAL_PRECISION;
					}

					if ((yyvsp[0].ival != INTERVAL_FULL_RANGE)
						&& ((yyvsp[0].ival & ~(INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE))) != 0))
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("time zone interval must be HOUR or HOUR TO MINUTE")));

					n->typename->typmod = INTERVAL_TYPMOD(yyvsp[-3].ival, yyvsp[0].ival);

					yyval.node = (Node *)n;
				}
    break;

  case 144:
#line 993 "gram.y"
    { yyval.node = makeAConst(yyvsp[0].value); }
    break;

  case 145:
#line 994 "gram.y"
    { yyval.node = NULL; }
    break;

  case 146:
#line 995 "gram.y"
    { yyval.node = NULL; }
    break;

  case 147:
#line 999 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 148:
#line 1000 "gram.y"
    { yyval.str = NULL; }
    break;

  case 149:
#line 1001 "gram.y"
    { yyval.str = NULL; }
    break;

  case 150:
#line 1005 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 151:
#line 1006 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 152:
#line 1012 "gram.y"
    {
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 153:
#line 1018 "gram.y"
    {
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name = "timezone";
					yyval.node = (Node *) n;
				}
    break;

  case 154:
#line 1024 "gram.y"
    {
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name = "transaction_isolation";
					yyval.node = (Node *) n;
				}
    break;

  case 155:
#line 1030 "gram.y"
    {
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name = "session_authorization";
					yyval.node = (Node *) n;
				}
    break;

  case 156:
#line 1036 "gram.y"
    {
					VariableShowStmt *n = makeNode(VariableShowStmt);
					n->name = "all";
					yyval.node = (Node *) n;
				}
    break;

  case 157:
#line 1045 "gram.y"
    {
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 158:
#line 1051 "gram.y"
    {
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name = "timezone";
					yyval.node = (Node *) n;
				}
    break;

  case 159:
#line 1057 "gram.y"
    {
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name = "transaction_isolation";
					yyval.node = (Node *) n;
				}
    break;

  case 160:
#line 1063 "gram.y"
    {
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name = "session_authorization";
					yyval.node = (Node *) n;
				}
    break;

  case 161:
#line 1069 "gram.y"
    {
					VariableResetStmt *n = makeNode(VariableResetStmt);
					n->name = "all";
					yyval.node = (Node *) n;
				}
    break;

  case 162:
#line 1079 "gram.y"
    {
					ConstraintsSetStmt *n = makeNode(ConstraintsSetStmt);
					n->constraints = yyvsp[-1].list;
					n->deferred    = yyvsp[0].boolean;
					yyval.node = (Node *) n;
				}
    break;

  case 163:
#line 1088 "gram.y"
    { yyval.list = NIL; }
    break;

  case 164:
#line 1089 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 165:
#line 1093 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 166:
#line 1094 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 167:
#line 1103 "gram.y"
    {
					CheckPointStmt *n = makeNode(CheckPointStmt);
					yyval.node = (Node *)n;
				}
    break;

  case 168:
#line 1119 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'A';
					n->relation = yyvsp[-3].range;
					n->def = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 169:
#line 1128 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'T';
					n->relation = yyvsp[-4].range;
					n->name = yyvsp[-1].str;
					n->def = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 170:
#line 1138 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'N';
					n->relation = yyvsp[-6].range;
					n->name = yyvsp[-3].str;
					yyval.node = (Node *)n;
				}
    break;

  case 171:
#line 1147 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'n';
					n->relation = yyvsp[-6].range;
					n->name = yyvsp[-3].str;
					yyval.node = (Node *)n;
				}
    break;

  case 172:
#line 1156 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'S';
					n->relation = yyvsp[-6].range;
					n->name = yyvsp[-3].str;
					n->def = (Node *) yyvsp[0].value;
					yyval.node = (Node *)n;
				}
    break;

  case 173:
#line 1167 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'M';
					n->relation = yyvsp[-6].range;
					n->name = yyvsp[-3].str;
					n->def = (Node *) makeString(yyvsp[0].str);
					yyval.node = (Node *)n;
				}
    break;

  case 174:
#line 1177 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'D';
					n->relation = yyvsp[-4].range;
					n->name = yyvsp[-1].str;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 175:
#line 1187 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'C';
					n->relation = yyvsp[-2].range;
					n->def = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 176:
#line 1196 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'X';
					n->relation = yyvsp[-4].range;
					n->name = yyvsp[-1].str;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 177:
#line 1206 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->relation = yyvsp[-3].range;
					n->subtype = 'o';
					yyval.node = (Node *)n;
				}
    break;

  case 178:
#line 1214 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'E';
					yyvsp[-3].range->inhOpt = INH_NO;
					n->relation = yyvsp[-3].range;
					yyval.node = (Node *)n;
				}
    break;

  case 179:
#line 1223 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'U';
					yyvsp[-3].range->inhOpt = INH_NO;
					n->relation = yyvsp[-3].range;
					n->name = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 180:
#line 1233 "gram.y"
    {
					AlterTableStmt *n = makeNode(AlterTableStmt);
					n->subtype = 'L';
					n->relation = yyvsp[-3].range;
					n->name = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 181:
#line 1244 "gram.y"
    {
					/* Treat SET DEFAULT NULL the same as DROP DEFAULT */
					if (exprIsNullConstant(yyvsp[0].node))
						yyval.node = NULL;
					else
						yyval.node = yyvsp[0].node;
				}
    break;

  case 182:
#line 1251 "gram.y"
    { yyval.node = NULL; }
    break;

  case 183:
#line 1255 "gram.y"
    { yyval.dbehavior = DROP_CASCADE; }
    break;

  case 184:
#line 1256 "gram.y"
    { yyval.dbehavior = DROP_RESTRICT; }
    break;

  case 185:
#line 1257 "gram.y"
    { yyval.dbehavior = DROP_RESTRICT; /* default */ }
    break;

  case 186:
#line 1270 "gram.y"
    {
					ClosePortalStmt *n = makeNode(ClosePortalStmt);
					n->portalname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 187:
#line 1290 "gram.y"
    {
					CopyStmt *n = makeNode(CopyStmt);
					n->relation = yyvsp[-7].range;
					n->attlist = yyvsp[-6].list;
					n->is_from = yyvsp[-4].boolean;
					n->filename = yyvsp[-3].str;

					n->options = NIL;
					/* Concatenate user-supplied flags */
					if (yyvsp[-8].defelt)
						n->options = lappend(n->options, yyvsp[-8].defelt);
					if (yyvsp[-5].defelt)
						n->options = lappend(n->options, yyvsp[-5].defelt);
					if (yyvsp[-2].defelt)
						n->options = lappend(n->options, yyvsp[-2].defelt);
					if (yyvsp[0].list)
						n->options = nconc(n->options, yyvsp[0].list);
					yyval.node = (Node *)n;
				}
    break;

  case 188:
#line 1312 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 189:
#line 1313 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 190:
#line 1322 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 191:
#line 1323 "gram.y"
    { yyval.str = NULL; }
    break;

  case 192:
#line 1324 "gram.y"
    { yyval.str = NULL; }
    break;

  case 193:
#line 1330 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].defelt); }
    break;

  case 194:
#line 1331 "gram.y"
    { yyval.list = NIL; }
    break;

  case 195:
#line 1337 "gram.y"
    {
					yyval.defelt = makeDefElem("binary", (Node *)makeInteger(TRUE));
				}
    break;

  case 196:
#line 1341 "gram.y"
    {
					yyval.defelt = makeDefElem("oids", (Node *)makeInteger(TRUE));
				}
    break;

  case 197:
#line 1345 "gram.y"
    {
					yyval.defelt = makeDefElem("delimiter", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 198:
#line 1349 "gram.y"
    {
					yyval.defelt = makeDefElem("null", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 199:
#line 1358 "gram.y"
    {
					yyval.defelt = makeDefElem("binary", (Node *)makeInteger(TRUE));
				}
    break;

  case 200:
#line 1361 "gram.y"
    { yyval.defelt = NULL; }
    break;

  case 201:
#line 1366 "gram.y"
    {
					yyval.defelt = makeDefElem("oids", (Node *)makeInteger(TRUE));
				}
    break;

  case 202:
#line 1369 "gram.y"
    { yyval.defelt = NULL; }
    break;

  case 203:
#line 1375 "gram.y"
    {
					yyval.defelt = makeDefElem("delimiter", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 204:
#line 1378 "gram.y"
    { yyval.defelt = NULL; }
    break;

  case 205:
#line 1382 "gram.y"
    {}
    break;

  case 206:
#line 1383 "gram.y"
    {}
    break;

  case 207:
#line 1396 "gram.y"
    {
					CreateStmt *n = makeNode(CreateStmt);
					yyvsp[-6].range->istemp = yyvsp[-8].boolean;
					n->relation = yyvsp[-6].range;
					n->tableElts = yyvsp[-4].list;
					n->inhRelations = yyvsp[-2].list;
					n->constraints = NIL;
					n->hasoids = yyvsp[-1].boolean;
					n->oncommit = yyvsp[0].oncommit;
					yyval.node = (Node *)n;
				}
    break;

  case 208:
#line 1409 "gram.y"
    {
					/* SQL99 CREATE TABLE OF <UDT> (cols) seems to be satisfied
					 * by our inheritance capabilities. Let's try it...
					 */
					CreateStmt *n = makeNode(CreateStmt);
					yyvsp[-7].range->istemp = yyvsp[-9].boolean;
					n->relation = yyvsp[-7].range;
					n->tableElts = yyvsp[-3].list;
					n->inhRelations = makeList1(yyvsp[-5].range);
					n->constraints = NIL;
					n->hasoids = yyvsp[-1].boolean;
					n->oncommit = yyvsp[0].oncommit;
					yyval.node = (Node *)n;
				}
    break;

  case 209:
#line 1432 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 210:
#line 1433 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 211:
#line 1434 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 212:
#line 1435 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 213:
#line 1436 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 214:
#line 1437 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 215:
#line 1438 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 216:
#line 1442 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 217:
#line 1443 "gram.y"
    { yyval.list = NIL; }
    break;

  case 218:
#line 1448 "gram.y"
    {
					yyval.list = makeList1(yyvsp[0].node);
				}
    break;

  case 219:
#line 1452 "gram.y"
    {
					yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node);
				}
    break;

  case 220:
#line 1458 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 221:
#line 1459 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 222:
#line 1460 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 223:
#line 1464 "gram.y"
    {
					ColumnDef *n = makeNode(ColumnDef);
					n->colname = yyvsp[-2].str;
					n->typename = yyvsp[-1].typnam;
					n->constraints = yyvsp[0].list;
					n->is_local = true;
					yyval.node = (Node *)n;
				}
    break;

  case 224:
#line 1475 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].node); }
    break;

  case 225:
#line 1476 "gram.y"
    { yyval.list = NIL; }
    break;

  case 226:
#line 1481 "gram.y"
    {
					switch (nodeTag(yyvsp[0].node))
					{
						case T_Constraint:
							{
								Constraint *n = (Constraint *)yyvsp[0].node;
								n->name = yyvsp[-1].str;
							}
							break;
						case T_FkConstraint:
							{
								FkConstraint *n = (FkConstraint *)yyvsp[0].node;
								n->constr_name = yyvsp[-1].str;
							}
							break;
						default:
							break;
					}
					yyval.node = yyvsp[0].node;
				}
    break;

  case 227:
#line 1501 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 228:
#line 1502 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 229:
#line 1522 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_NOTNULL;
					n->name = NULL;
					n->raw_expr = NULL;
					n->cooked_expr = NULL;
					n->keys = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 230:
#line 1532 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_NULL;
					n->name = NULL;
					n->raw_expr = NULL;
					n->cooked_expr = NULL;
					n->keys = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 231:
#line 1542 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_UNIQUE;
					n->name = NULL;
					n->raw_expr = NULL;
					n->cooked_expr = NULL;
					n->keys = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 232:
#line 1552 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_PRIMARY;
					n->name = NULL;
					n->raw_expr = NULL;
					n->cooked_expr = NULL;
					n->keys = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 233:
#line 1562 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_CHECK;
					n->name = NULL;
					n->raw_expr = yyvsp[-1].node;
					n->cooked_expr = NULL;
					n->keys = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 234:
#line 1572 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_DEFAULT;
					n->name = NULL;
					if (exprIsNullConstant(yyvsp[0].node))
					{
						/* DEFAULT NULL should be reported as empty expr */
						n->raw_expr = NULL;
					}
					else
					{
						n->raw_expr = yyvsp[0].node;
					}
					n->cooked_expr = NULL;
					n->keys = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 235:
#line 1590 "gram.y"
    {
					FkConstraint *n = makeNode(FkConstraint);
					n->constr_name		= NULL;
					n->pktable			= yyvsp[-3].range;
					n->fk_attrs			= NIL;
					n->pk_attrs			= yyvsp[-2].list;
					n->fk_matchtype		= yyvsp[-1].ival;
					n->fk_upd_action	= (char) (yyvsp[0].ival >> 8);
					n->fk_del_action	= (char) (yyvsp[0].ival & 0xFF);
					n->deferrable		= FALSE;
					n->initdeferred		= FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 236:
#line 1618 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_ATTR_DEFERRABLE;
					yyval.node = (Node *)n;
				}
    break;

  case 237:
#line 1624 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_ATTR_NOT_DEFERRABLE;
					yyval.node = (Node *)n;
				}
    break;

  case 238:
#line 1630 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_ATTR_DEFERRED;
					yyval.node = (Node *)n;
				}
    break;

  case 239:
#line 1636 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_ATTR_IMMEDIATE;
					yyval.node = (Node *)n;
				}
    break;

  case 240:
#line 1654 "gram.y"
    {
					InhRelation *n = makeNode(InhRelation);
					n->relation = yyvsp[-1].range;
					n->including_defaults = yyvsp[0].boolean;

					yyval.node = (Node *)n;
				}
    break;

  case 241:
#line 1664 "gram.y"
    { yyval.boolean = true; }
    break;

  case 242:
#line 1665 "gram.y"
    { yyval.boolean = false; }
    break;

  case 243:
#line 1666 "gram.y"
    { yyval.boolean = false; }
    break;

  case 244:
#line 1676 "gram.y"
    {
					switch (nodeTag(yyvsp[0].node))
					{
						case T_Constraint:
							{
								Constraint *n = (Constraint *)yyvsp[0].node;
								n->name = yyvsp[-1].str;
							}
							break;
						case T_FkConstraint:
							{
								FkConstraint *n = (FkConstraint *)yyvsp[0].node;
								n->constr_name = yyvsp[-1].str;
							}
							break;
						default:
							break;
					}
					yyval.node = yyvsp[0].node;
				}
    break;

  case 245:
#line 1696 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 246:
#line 1701 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_CHECK;
					n->name = NULL;
					n->raw_expr = yyvsp[-1].node;
					n->cooked_expr = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 247:
#line 1710 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_UNIQUE;
					n->name = NULL;
					n->raw_expr = NULL;
					n->cooked_expr = NULL;
					n->keys = yyvsp[-1].list;
					yyval.node = (Node *)n;
				}
    break;

  case 248:
#line 1720 "gram.y"
    {
					Constraint *n = makeNode(Constraint);
					n->contype = CONSTR_PRIMARY;
					n->name = NULL;
					n->raw_expr = NULL;
					n->cooked_expr = NULL;
					n->keys = yyvsp[-1].list;
					yyval.node = (Node *)n;
				}
    break;

  case 249:
#line 1731 "gram.y"
    {
					FkConstraint *n = makeNode(FkConstraint);
					n->constr_name		= NULL;
					n->pktable			= yyvsp[-4].range;
					n->fk_attrs			= yyvsp[-7].list;
					n->pk_attrs			= yyvsp[-3].list;
					n->fk_matchtype		= yyvsp[-2].ival;
					n->fk_upd_action	= (char) (yyvsp[-1].ival >> 8);
					n->fk_del_action	= (char) (yyvsp[-1].ival & 0xFF);
					n->deferrable		= (yyvsp[0].ival & 1) != 0;
					n->initdeferred		= (yyvsp[0].ival & 2) != 0;
					yyval.node = (Node *)n;
				}
    break;

  case 250:
#line 1747 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 251:
#line 1748 "gram.y"
    { yyval.list = NIL; }
    break;

  case 252:
#line 1752 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 253:
#line 1753 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 254:
#line 1757 "gram.y"
    {
					yyval.node = (Node *) makeString(yyvsp[0].str);
				}
    break;

  case 255:
#line 1763 "gram.y"
    {
				yyval.ival = FKCONSTR_MATCH_FULL;
			}
    break;

  case 256:
#line 1767 "gram.y"
    {
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("FOREIGN KEY/MATCH PARTIAL is not yet implemented")));
				yyval.ival = FKCONSTR_MATCH_PARTIAL;
			}
    break;

  case 257:
#line 1774 "gram.y"
    {
				yyval.ival = FKCONSTR_MATCH_UNSPECIFIED;
			}
    break;

  case 258:
#line 1778 "gram.y"
    {
				yyval.ival = FKCONSTR_MATCH_UNSPECIFIED;
			}
    break;

  case 259:
#line 1791 "gram.y"
    { yyval.ival = (yyvsp[0].ival << 8) | (FKCONSTR_ACTION_NOACTION & 0xFF); }
    break;

  case 260:
#line 1793 "gram.y"
    { yyval.ival = (FKCONSTR_ACTION_NOACTION << 8) | (yyvsp[0].ival & 0xFF); }
    break;

  case 261:
#line 1795 "gram.y"
    { yyval.ival = (yyvsp[-1].ival << 8) | (yyvsp[0].ival & 0xFF); }
    break;

  case 262:
#line 1797 "gram.y"
    { yyval.ival = (yyvsp[0].ival << 8) | (yyvsp[-1].ival & 0xFF); }
    break;

  case 263:
#line 1799 "gram.y"
    { yyval.ival = (FKCONSTR_ACTION_NOACTION << 8) | (FKCONSTR_ACTION_NOACTION & 0xFF); }
    break;

  case 264:
#line 1802 "gram.y"
    { yyval.ival = yyvsp[0].ival; }
    break;

  case 265:
#line 1805 "gram.y"
    { yyval.ival = yyvsp[0].ival; }
    break;

  case 266:
#line 1809 "gram.y"
    { yyval.ival = FKCONSTR_ACTION_NOACTION; }
    break;

  case 267:
#line 1810 "gram.y"
    { yyval.ival = FKCONSTR_ACTION_RESTRICT; }
    break;

  case 268:
#line 1811 "gram.y"
    { yyval.ival = FKCONSTR_ACTION_CASCADE; }
    break;

  case 269:
#line 1812 "gram.y"
    { yyval.ival = FKCONSTR_ACTION_SETNULL; }
    break;

  case 270:
#line 1813 "gram.y"
    { yyval.ival = FKCONSTR_ACTION_SETDEFAULT; }
    break;

  case 271:
#line 1816 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 272:
#line 1817 "gram.y"
    { yyval.list = NIL; }
    break;

  case 273:
#line 1821 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 274:
#line 1822 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 275:
#line 1823 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 276:
#line 1826 "gram.y"
    { yyval.oncommit = ONCOMMIT_DROP; }
    break;

  case 277:
#line 1827 "gram.y"
    { yyval.oncommit = ONCOMMIT_DELETE_ROWS; }
    break;

  case 278:
#line 1828 "gram.y"
    { yyval.oncommit = ONCOMMIT_PRESERVE_ROWS; }
    break;

  case 279:
#line 1829 "gram.y"
    { yyval.oncommit = ONCOMMIT_NOOP; }
    break;

  case 280:
#line 1840 "gram.y"
    {
					/*
					 * When the SelectStmt is a set-operation tree, we must
					 * stuff the INTO information into the leftmost component
					 * Select, because that's where analyze.c will expect
					 * to find it.	Similarly, the output column names must
					 * be attached to that Select's target list.
					 */
					SelectStmt *n = findLeftmostSelect((SelectStmt *) yyvsp[0].node);
					if (n->into != NULL)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("CREATE TABLE AS may not specify INTO")));
					yyvsp[-3].range->istemp = yyvsp[-5].boolean;
					n->into = yyvsp[-3].range;
					n->intoColNames = yyvsp[-2].list;
					yyval.node = yyvsp[0].node;
				}
    break;

  case 281:
#line 1861 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 282:
#line 1862 "gram.y"
    { yyval.list = NIL; }
    break;

  case 283:
#line 1866 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 284:
#line 1867 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 285:
#line 1872 "gram.y"
    {
					ColumnDef *n = makeNode(ColumnDef);
					n->colname = yyvsp[0].str;
					n->typename = NULL;
					n->inhcount = 0;
					n->is_local = true;
					n->is_not_null = false;
					n->raw_default = NULL;
					n->cooked_default = NULL;
					n->constraints = NIL;
					n->support = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 286:
#line 1898 "gram.y"
    {
					CreateSeqStmt *n = makeNode(CreateSeqStmt);
					yyvsp[-1].range->istemp = yyvsp[-3].boolean;
					n->sequence = yyvsp[-1].range;
					n->options = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 287:
#line 1909 "gram.y"
    {
					AlterSeqStmt *n = makeNode(AlterSeqStmt);
					n->sequence = yyvsp[-1].range;
					n->options = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 288:
#line 1917 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].defelt); }
    break;

  case 289:
#line 1918 "gram.y"
    { yyval.list = NIL; }
    break;

  case 290:
#line 1922 "gram.y"
    {
					yyval.defelt = makeDefElem("cache", (Node *)yyvsp[0].value);
				}
    break;

  case 291:
#line 1926 "gram.y"
    {
					yyval.defelt = makeDefElem("cycle", (Node *)true);
				}
    break;

  case 292:
#line 1930 "gram.y"
    {
					yyval.defelt = makeDefElem("cycle", (Node *)false);
				}
    break;

  case 293:
#line 1934 "gram.y"
    {
					yyval.defelt = makeDefElem("increment", (Node *)yyvsp[0].value);
				}
    break;

  case 294:
#line 1938 "gram.y"
    {
					yyval.defelt = makeDefElem("maxvalue", (Node *)yyvsp[0].value);
				}
    break;

  case 295:
#line 1942 "gram.y"
    {
					yyval.defelt = makeDefElem("minvalue", (Node *)yyvsp[0].value);
				}
    break;

  case 296:
#line 1946 "gram.y"
    {
					yyval.defelt = makeDefElem("maxvalue", (Node *)NULL);
				}
    break;

  case 297:
#line 1950 "gram.y"
    {
					yyval.defelt = makeDefElem("minvalue", (Node *)NULL);
				}
    break;

  case 298:
#line 1954 "gram.y"
    {
					yyval.defelt = makeDefElem("start", (Node *)yyvsp[0].value);
				}
    break;

  case 299:
#line 1958 "gram.y"
    {
					yyval.defelt = makeDefElem("restart", (Node *)yyvsp[0].value);
				}
    break;

  case 300:
#line 1963 "gram.y"
    {}
    break;

  case 301:
#line 1964 "gram.y"
    {}
    break;

  case 302:
#line 1968 "gram.y"
    { yyval.value = yyvsp[0].value; }
    break;

  case 303:
#line 1969 "gram.y"
    { yyval.value = yyvsp[0].value; }
    break;

  case 304:
#line 1972 "gram.y"
    { yyval.value = makeFloat(yyvsp[0].str); }
    break;

  case 305:
#line 1974 "gram.y"
    {
					yyval.value = makeFloat(yyvsp[0].str);
					doNegateFloat(yyval.value);
				}
    break;

  case 306:
#line 1982 "gram.y"
    {
					yyval.value = makeInteger(yyvsp[0].ival);
				}
    break;

  case 307:
#line 1986 "gram.y"
    {
					yyval.value = makeInteger(yyvsp[0].ival);
					yyval.value->val.ival = - yyval.value->val.ival;
				}
    break;

  case 308:
#line 2003 "gram.y"
    {
				CreatePLangStmt *n = makeNode(CreatePLangStmt);
				n->plname = yyvsp[-4].str;
				n->plhandler = yyvsp[-2].list;
				n->plvalidator = yyvsp[-1].list;
				n->pltrusted = yyvsp[-7].boolean;
				yyval.node = (Node *)n;
			}
    break;

  case 309:
#line 2014 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 310:
#line 2015 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 311:
#line 2024 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 312:
#line 2025 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 313:
#line 2029 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 314:
#line 2030 "gram.y"
    { yyval.str = ""; }
    break;

  case 315:
#line 2034 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 316:
#line 2035 "gram.y"
    { yyval.list = NULL; }
    break;

  case 317:
#line 2040 "gram.y"
    {
					DropPLangStmt *n = makeNode(DropPLangStmt);
					n->plname = yyvsp[-1].str;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 318:
#line 2049 "gram.y"
    {}
    break;

  case 319:
#line 2050 "gram.y"
    {}
    break;

  case 320:
#line 2065 "gram.y"
    {
					CreateTrigStmt *n = makeNode(CreateTrigStmt);
					n->trigname = yyvsp[-11].str;
					n->relation = yyvsp[-7].range;
					n->funcname = yyvsp[-3].list;
					n->args = yyvsp[-1].list;
					n->before = yyvsp[-10].boolean;
					n->row = yyvsp[-6].boolean;
					memcpy(n->actions, yyvsp[-9].str, 4);
					n->isconstraint  = FALSE;
					n->deferrable	 = FALSE;
					n->initdeferred  = FALSE;
					n->constrrel = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 321:
#line 2085 "gram.y"
    {
					CreateTrigStmt *n = makeNode(CreateTrigStmt);
					n->trigname = yyvsp[-15].str;
					n->relation = yyvsp[-11].range;
					n->funcname = yyvsp[-3].list;
					n->args = yyvsp[-1].list;
					n->before = FALSE;
					n->row = TRUE;
					memcpy(n->actions, yyvsp[-13].str, 4);
					n->isconstraint  = TRUE;
					n->deferrable = (yyvsp[-9].ival & 1) != 0;
					n->initdeferred = (yyvsp[-9].ival & 2) != 0;

					n->constrrel = yyvsp[-10].range;
					yyval.node = (Node *)n;
				}
    break;

  case 322:
#line 2104 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 323:
#line 2105 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 324:
#line 2110 "gram.y"
    {
					char *e = palloc(4);
					e[0] = yyvsp[0].chr; e[1] = '\0';
					yyval.str = e;
				}
    break;

  case 325:
#line 2116 "gram.y"
    {
					char *e = palloc(4);
					e[0] = yyvsp[-2].chr; e[1] = yyvsp[0].chr; e[2] = '\0';
					yyval.str = e;
				}
    break;

  case 326:
#line 2122 "gram.y"
    {
					char *e = palloc(4);
					e[0] = yyvsp[-4].chr; e[1] = yyvsp[-2].chr; e[2] = yyvsp[0].chr; e[3] = '\0';
					yyval.str = e;
				}
    break;

  case 327:
#line 2130 "gram.y"
    { yyval.chr = 'i'; }
    break;

  case 328:
#line 2131 "gram.y"
    { yyval.chr = 'd'; }
    break;

  case 329:
#line 2132 "gram.y"
    { yyval.chr = 'u'; }
    break;

  case 330:
#line 2137 "gram.y"
    {
					yyval.boolean = yyvsp[0].boolean;
				}
    break;

  case 331:
#line 2141 "gram.y"
    {
					/*
					 * If ROW/STATEMENT not specified, default to
					 * STATEMENT, per SQL
					 */
					yyval.boolean = FALSE;
				}
    break;

  case 332:
#line 2151 "gram.y"
    {}
    break;

  case 333:
#line 2152 "gram.y"
    {}
    break;

  case 334:
#line 2156 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 335:
#line 2157 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 336:
#line 2161 "gram.y"
    { yyval.list = makeList1(yyvsp[0].value); }
    break;

  case 337:
#line 2162 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].value); }
    break;

  case 338:
#line 2163 "gram.y"
    { yyval.list = NIL; }
    break;

  case 339:
#line 2168 "gram.y"
    {
					char buf[64];
					snprintf(buf, sizeof(buf), "%d", yyvsp[0].ival);
					yyval.value = makeString(pstrdup(buf));
				}
    break;

  case 340:
#line 2173 "gram.y"
    { yyval.value = makeString(yyvsp[0].str); }
    break;

  case 341:
#line 2174 "gram.y"
    { yyval.value = makeString(yyvsp[0].str); }
    break;

  case 342:
#line 2175 "gram.y"
    { yyval.value = makeString(yyvsp[0].str); }
    break;

  case 343:
#line 2176 "gram.y"
    { yyval.value = makeString(yyvsp[0].str); }
    break;

  case 344:
#line 2177 "gram.y"
    { yyval.value = makeString(yyvsp[0].str); }
    break;

  case 345:
#line 2181 "gram.y"
    { yyval.range = yyvsp[0].range; }
    break;

  case 346:
#line 2182 "gram.y"
    { yyval.range = NULL; }
    break;

  case 347:
#line 2187 "gram.y"
    { yyval.ival = yyvsp[0].ival; }
    break;

  case 348:
#line 2189 "gram.y"
    {
					if (yyvsp[-1].ival == 0 && yyvsp[0].ival != 0)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("INITIALLY DEFERRED constraint must be DEFERRABLE")));
					yyval.ival = yyvsp[-1].ival | yyvsp[0].ival;
				}
    break;

  case 349:
#line 2197 "gram.y"
    {
					if (yyvsp[0].ival != 0)
						yyval.ival = 3;
					else
						yyval.ival = 0;
				}
    break;

  case 350:
#line 2204 "gram.y"
    {
					if (yyvsp[0].ival == 0 && yyvsp[-1].ival != 0)
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("INITIALLY DEFERRED constraint must be DEFERRABLE")));
					yyval.ival = yyvsp[-1].ival | yyvsp[0].ival;
				}
    break;

  case 351:
#line 2212 "gram.y"
    { yyval.ival = 0; }
    break;

  case 352:
#line 2216 "gram.y"
    { yyval.ival = 0; }
    break;

  case 353:
#line 2217 "gram.y"
    { yyval.ival = 1; }
    break;

  case 354:
#line 2221 "gram.y"
    { yyval.ival = 0; }
    break;

  case 355:
#line 2222 "gram.y"
    { yyval.ival = 2; }
    break;

  case 356:
#line 2228 "gram.y"
    {
					DropPropertyStmt *n = makeNode(DropPropertyStmt);
					n->relation = yyvsp[-1].range;
					n->property = yyvsp[-3].str;
					n->behavior = yyvsp[0].dbehavior;
					n->removeType = OBJECT_TRIGGER;
					yyval.node = (Node *) n;
				}
    break;

  case 357:
#line 2250 "gram.y"
    {
					CreateTrigStmt *n = makeNode(CreateTrigStmt);
					n->trigname = yyvsp[-5].str;
					n->args = makeList1(yyvsp[-2].node);
					n->isconstraint  = TRUE;
					n->deferrable = (yyvsp[0].ival & 1) != 0;
					n->initdeferred = (yyvsp[0].ival & 2) != 0;

					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("CREATE ASSERTION is not yet implemented")));

					yyval.node = (Node *)n;
				}
    break;

  case 358:
#line 2268 "gram.y"
    {
					DropPropertyStmt *n = makeNode(DropPropertyStmt);
					n->relation = NULL;
					n->property = yyvsp[-1].str;
					n->behavior = yyvsp[0].dbehavior;
					n->removeType = OBJECT_TRIGGER; /* XXX */
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("DROP ASSERTION is not yet implemented")));
					yyval.node = (Node *) n;
				}
    break;

  case 359:
#line 2291 "gram.y"
    {
					DefineStmt *n = makeNode(DefineStmt);
					n->kind = OBJECT_AGGREGATE;
					n->defnames = yyvsp[-1].list;
					n->definition = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 360:
#line 2299 "gram.y"
    {
					DefineStmt *n = makeNode(DefineStmt);
					n->kind = OBJECT_OPERATOR;
					n->defnames = yyvsp[-1].list;
					n->definition = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 361:
#line 2307 "gram.y"
    {
					DefineStmt *n = makeNode(DefineStmt);
					n->kind = OBJECT_TYPE;
					n->defnames = yyvsp[-1].list;
					n->definition = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 362:
#line 2315 "gram.y"
    {
					CompositeTypeStmt *n = makeNode(CompositeTypeStmt);
					RangeVar *r = makeNode(RangeVar);

					/* can't use qualified_name, sigh */
					switch (length(yyvsp[-4].list))
					{
						case 1:
							r->catalogname = NULL;
							r->schemaname = NULL;
							r->relname = strVal(lfirst(yyvsp[-4].list));
							break;
						case 2:
							r->catalogname = NULL;
							r->schemaname = strVal(lfirst(yyvsp[-4].list));
							r->relname = strVal(lsecond(yyvsp[-4].list));
							break;
						case 3:
							r->catalogname = strVal(lfirst(yyvsp[-4].list));
							r->schemaname = strVal(lsecond(yyvsp[-4].list));
							r->relname = strVal(lthird(yyvsp[-4].list));
							break;
						default:
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("improper qualified name (too many dotted names): %s",
											NameListToString(yyvsp[-4].list))));
							break;
					}
					n->typevar = r;
					n->coldeflist = yyvsp[-1].list;
					yyval.node = (Node *)n;
				}
    break;

  case 363:
#line 2350 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 364:
#line 2353 "gram.y"
    { yyval.list = makeList1(yyvsp[0].defelt); }
    break;

  case 365:
#line 2354 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].defelt); }
    break;

  case 366:
#line 2358 "gram.y"
    {
					yyval.defelt = makeDefElem(yyvsp[-2].str, (Node *)yyvsp[0].node);
				}
    break;

  case 367:
#line 2362 "gram.y"
    {
					yyval.defelt = makeDefElem(yyvsp[0].str, (Node *)NULL);
				}
    break;

  case 368:
#line 2368 "gram.y"
    { yyval.node = (Node *)yyvsp[0].typnam; }
    break;

  case 369:
#line 2369 "gram.y"
    { yyval.node = (Node *)yyvsp[0].list; }
    break;

  case 370:
#line 2370 "gram.y"
    { yyval.node = (Node *)yyvsp[0].value; }
    break;

  case 371:
#line 2371 "gram.y"
    { yyval.node = (Node *)makeString(yyvsp[0].str); }
    break;

  case 372:
#line 2386 "gram.y"
    {
					CreateOpClassStmt *n = makeNode(CreateOpClassStmt);
					n->opclassname = yyvsp[-8].list;
					n->isDefault = yyvsp[-7].boolean;
					n->datatype = yyvsp[-4].typnam;
					n->amname = yyvsp[-2].str;
					n->items = yyvsp[0].list;
					yyval.node = (Node *) n;
				}
    break;

  case 373:
#line 2398 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 374:
#line 2399 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 375:
#line 2404 "gram.y"
    {
					CreateOpClassItem *n = makeNode(CreateOpClassItem);
					n->itemtype = OPCLASS_ITEM_OPERATOR;
					n->name = yyvsp[-1].list;
					n->args = NIL;
					n->number = yyvsp[-2].ival;
					n->recheck = yyvsp[0].boolean;
					yyval.node = (Node *) n;
				}
    break;

  case 376:
#line 2414 "gram.y"
    {
					CreateOpClassItem *n = makeNode(CreateOpClassItem);
					n->itemtype = OPCLASS_ITEM_OPERATOR;
					n->name = yyvsp[-4].list;
					n->args = yyvsp[-2].list;
					n->number = yyvsp[-5].ival;
					n->recheck = yyvsp[0].boolean;
					yyval.node = (Node *) n;
				}
    break;

  case 377:
#line 2424 "gram.y"
    {
					CreateOpClassItem *n = makeNode(CreateOpClassItem);
					n->itemtype = OPCLASS_ITEM_FUNCTION;
					n->name = yyvsp[-1].list;
					n->args = yyvsp[0].list;
					n->number = yyvsp[-2].ival;
					yyval.node = (Node *) n;
				}
    break;

  case 378:
#line 2433 "gram.y"
    {
					CreateOpClassItem *n = makeNode(CreateOpClassItem);
					n->itemtype = OPCLASS_ITEM_STORAGETYPE;
					n->storedtype = yyvsp[0].typnam;
					yyval.node = (Node *) n;
				}
    break;

  case 379:
#line 2441 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 380:
#line 2442 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 381:
#line 2445 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 382:
#line 2446 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 383:
#line 2452 "gram.y"
    {
					RemoveOpClassStmt *n = makeNode(RemoveOpClassStmt);
					n->opclassname = yyvsp[-3].list;
					n->amname = yyvsp[-1].str;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *) n;
				}
    break;

  case 384:
#line 2471 "gram.y"
    {
					DropStmt *n = makeNode(DropStmt);
					n->removeType = yyvsp[-2].objtype;
					n->objects = yyvsp[-1].list;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 385:
#line 2480 "gram.y"
    { yyval.objtype = OBJECT_TABLE; }
    break;

  case 386:
#line 2481 "gram.y"
    { yyval.objtype = OBJECT_SEQUENCE; }
    break;

  case 387:
#line 2482 "gram.y"
    { yyval.objtype = OBJECT_VIEW; }
    break;

  case 388:
#line 2483 "gram.y"
    { yyval.objtype = OBJECT_INDEX; }
    break;

  case 389:
#line 2484 "gram.y"
    { yyval.objtype = OBJECT_TYPE; }
    break;

  case 390:
#line 2485 "gram.y"
    { yyval.objtype = OBJECT_DOMAIN; }
    break;

  case 391:
#line 2486 "gram.y"
    { yyval.objtype = OBJECT_CONVERSION; }
    break;

  case 392:
#line 2487 "gram.y"
    { yyval.objtype = OBJECT_SCHEMA; }
    break;

  case 393:
#line 2491 "gram.y"
    { yyval.list = makeList1(yyvsp[0].list); }
    break;

  case 394:
#line 2492 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].list); }
    break;

  case 395:
#line 2495 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 396:
#line 2496 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 397:
#line 2508 "gram.y"
    {
					TruncateStmt *n = makeNode(TruncateStmt);
					n->relation = yyvsp[0].range;
					yyval.node = (Node *)n;
				}
    break;

  case 398:
#line 2530 "gram.y"
    {
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = yyvsp[-3].objtype;
					n->objname = yyvsp[-2].list;
					n->objargs = NIL;
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 399:
#line 2540 "gram.y"
    {
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = OBJECT_AGGREGATE;
					n->objname = yyvsp[-5].list;
					n->objargs = makeList1(yyvsp[-3].typnam);
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 400:
#line 2549 "gram.y"
    {
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = OBJECT_FUNCTION;
					n->objname = yyvsp[-3].list;
					n->objargs = yyvsp[-2].list;
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 401:
#line 2559 "gram.y"
    {
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = OBJECT_OPERATOR;
					n->objname = yyvsp[-5].list;
					n->objargs = yyvsp[-3].list;
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 402:
#line 2568 "gram.y"
    {
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = OBJECT_CONSTRAINT;
					n->objname = lappend(yyvsp[-2].list, makeString(yyvsp[-4].str));
					n->objargs = NIL;
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 403:
#line 2577 "gram.y"
    {
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = OBJECT_RULE;
					n->objname = lappend(yyvsp[-2].list, makeString(yyvsp[-4].str));
					n->objargs = NIL;
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 404:
#line 2586 "gram.y"
    {
					/* Obsolete syntax supported for awhile for compatibility */
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = OBJECT_RULE;
					n->objname = makeList1(makeString(yyvsp[-2].str));
					n->objargs = NIL;
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 405:
#line 2596 "gram.y"
    {
					CommentStmt *n = makeNode(CommentStmt);
					n->objtype = OBJECT_TRIGGER;
					n->objname = lappend(yyvsp[-2].list, makeString(yyvsp[-4].str));
					n->objargs = NIL;
					n->comment = yyvsp[0].str;
					yyval.node = (Node *) n;
				}
    break;

  case 406:
#line 2607 "gram.y"
    { yyval.objtype = OBJECT_COLUMN; }
    break;

  case 407:
#line 2608 "gram.y"
    { yyval.objtype = OBJECT_DATABASE; }
    break;

  case 408:
#line 2609 "gram.y"
    { yyval.objtype = OBJECT_SCHEMA; }
    break;

  case 409:
#line 2610 "gram.y"
    { yyval.objtype = OBJECT_INDEX; }
    break;

  case 410:
#line 2611 "gram.y"
    { yyval.objtype = OBJECT_SEQUENCE; }
    break;

  case 411:
#line 2612 "gram.y"
    { yyval.objtype = OBJECT_TABLE; }
    break;

  case 412:
#line 2613 "gram.y"
    { yyval.objtype = OBJECT_TYPE; }
    break;

  case 413:
#line 2614 "gram.y"
    { yyval.objtype = OBJECT_TYPE; }
    break;

  case 414:
#line 2615 "gram.y"
    { yyval.objtype = OBJECT_VIEW; }
    break;

  case 415:
#line 2619 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 416:
#line 2620 "gram.y"
    { yyval.str = NULL; }
    break;

  case 417:
#line 2631 "gram.y"
    {
					FetchStmt *n = (FetchStmt *) yyvsp[-2].node;
					n->portalname = yyvsp[0].str;
					n->ismove = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 418:
#line 2638 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = 1;
					n->portalname = yyvsp[0].str;
					n->ismove = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 419:
#line 2647 "gram.y"
    {
					FetchStmt *n = (FetchStmt *) yyvsp[-2].node;
					n->portalname = yyvsp[0].str;
					n->ismove = TRUE;
					yyval.node = (Node *)n;
				}
    break;

  case 420:
#line 2654 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = 1;
					n->portalname = yyvsp[0].str;
					n->ismove = TRUE;
					yyval.node = (Node *)n;
				}
    break;

  case 421:
#line 2666 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = 1;
					yyval.node = (Node *)n;
				}
    break;

  case 422:
#line 2673 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = 1;
					yyval.node = (Node *)n;
				}
    break;

  case 423:
#line 2680 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_BACKWARD;
					n->howMany = 1;
					yyval.node = (Node *)n;
				}
    break;

  case 424:
#line 2687 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_ABSOLUTE;
					n->howMany = 1;
					yyval.node = (Node *)n;
				}
    break;

  case 425:
#line 2694 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_ABSOLUTE;
					n->howMany = -1;
					yyval.node = (Node *)n;
				}
    break;

  case 426:
#line 2701 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_ABSOLUTE;
					n->howMany = yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 427:
#line 2708 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_RELATIVE;
					n->howMany = yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 428:
#line 2715 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 429:
#line 2722 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = FETCH_ALL;
					yyval.node = (Node *)n;
				}
    break;

  case 430:
#line 2729 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = 1;
					yyval.node = (Node *)n;
				}
    break;

  case 431:
#line 2736 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 432:
#line 2743 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_FORWARD;
					n->howMany = FETCH_ALL;
					yyval.node = (Node *)n;
				}
    break;

  case 433:
#line 2750 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_BACKWARD;
					n->howMany = 1;
					yyval.node = (Node *)n;
				}
    break;

  case 434:
#line 2757 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_BACKWARD;
					n->howMany = yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 435:
#line 2764 "gram.y"
    {
					FetchStmt *n = makeNode(FetchStmt);
					n->direction = FETCH_BACKWARD;
					n->howMany = FETCH_ALL;
					yyval.node = (Node *)n;
				}
    break;

  case 436:
#line 2773 "gram.y"
    { yyval.ival = yyvsp[0].ival; }
    break;

  case 437:
#line 2774 "gram.y"
    { yyval.ival = - yyvsp[0].ival; }
    break;

  case 438:
#line 2777 "gram.y"
    {}
    break;

  case 439:
#line 2778 "gram.y"
    {}
    break;

  case 440:
#line 2790 "gram.y"
    {
					GrantStmt *n = makeNode(GrantStmt);
					n->is_grant = true;
					n->privileges = yyvsp[-5].list;
					n->objtype = (yyvsp[-3].privtarget)->objtype;
					n->objects = (yyvsp[-3].privtarget)->objs;
					n->grantees = yyvsp[-1].list;
					n->grant_option = yyvsp[0].boolean;
					yyval.node = (Node*)n;
				}
    break;

  case 441:
#line 2804 "gram.y"
    {
					GrantStmt *n = makeNode(GrantStmt);
					n->is_grant = false;
					n->privileges = yyvsp[-5].list;
					n->objtype = (yyvsp[-3].privtarget)->objtype;
					n->objects = (yyvsp[-3].privtarget)->objs;
					n->grantees = yyvsp[-1].list;
					n->grant_option = yyvsp[-6].boolean;
					n->behavior = yyvsp[0].dbehavior;

					yyval.node = (Node *)n;
				}
    break;

  case 442:
#line 2820 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 443:
#line 2821 "gram.y"
    { yyval.list = makeListi1(ACL_ALL_RIGHTS); }
    break;

  case 444:
#line 2822 "gram.y"
    { yyval.list = makeListi1(ACL_ALL_RIGHTS); }
    break;

  case 445:
#line 2826 "gram.y"
    { yyval.list = makeListi1(yyvsp[0].ival); }
    break;

  case 446:
#line 2827 "gram.y"
    { yyval.list = lappendi(yyvsp[-2].list, yyvsp[0].ival); }
    break;

  case 447:
#line 2833 "gram.y"
    { yyval.ival = ACL_SELECT; }
    break;

  case 448:
#line 2834 "gram.y"
    { yyval.ival = ACL_INSERT; }
    break;

  case 449:
#line 2835 "gram.y"
    { yyval.ival = ACL_UPDATE; }
    break;

  case 450:
#line 2836 "gram.y"
    { yyval.ival = ACL_DELETE; }
    break;

  case 451:
#line 2837 "gram.y"
    { yyval.ival = ACL_RULE; }
    break;

  case 452:
#line 2838 "gram.y"
    { yyval.ival = ACL_REFERENCES; }
    break;

  case 453:
#line 2839 "gram.y"
    { yyval.ival = ACL_TRIGGER; }
    break;

  case 454:
#line 2840 "gram.y"
    { yyval.ival = ACL_EXECUTE; }
    break;

  case 455:
#line 2841 "gram.y"
    { yyval.ival = ACL_USAGE; }
    break;

  case 456:
#line 2842 "gram.y"
    { yyval.ival = ACL_CREATE; }
    break;

  case 457:
#line 2843 "gram.y"
    { yyval.ival = ACL_CREATE_TEMP; }
    break;

  case 458:
#line 2844 "gram.y"
    { yyval.ival = ACL_CREATE_TEMP; }
    break;

  case 459:
#line 2852 "gram.y"
    {
					PrivTarget *n = makeNode(PrivTarget);
					n->objtype = ACL_OBJECT_RELATION;
					n->objs = yyvsp[0].list;
					yyval.privtarget = n;
				}
    break;

  case 460:
#line 2859 "gram.y"
    {
					PrivTarget *n = makeNode(PrivTarget);
					n->objtype = ACL_OBJECT_RELATION;
					n->objs = yyvsp[0].list;
					yyval.privtarget = n;
				}
    break;

  case 461:
#line 2866 "gram.y"
    {
					PrivTarget *n = makeNode(PrivTarget);
					n->objtype = ACL_OBJECT_FUNCTION;
					n->objs = yyvsp[0].list;
					yyval.privtarget = n;
				}
    break;

  case 462:
#line 2873 "gram.y"
    {
					PrivTarget *n = makeNode(PrivTarget);
					n->objtype = ACL_OBJECT_DATABASE;
					n->objs = yyvsp[0].list;
					yyval.privtarget = n;
				}
    break;

  case 463:
#line 2880 "gram.y"
    {
					PrivTarget *n = makeNode(PrivTarget);
					n->objtype = ACL_OBJECT_LANGUAGE;
					n->objs = yyvsp[0].list;
					yyval.privtarget = n;
				}
    break;

  case 464:
#line 2887 "gram.y"
    {
					PrivTarget *n = makeNode(PrivTarget);
					n->objtype = ACL_OBJECT_NAMESPACE;
					n->objs = yyvsp[0].list;
					yyval.privtarget = n;
				}
    break;

  case 465:
#line 2897 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 466:
#line 2898 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 467:
#line 2902 "gram.y"
    {
					PrivGrantee *n = makeNode(PrivGrantee);
					/* This hack lets us avoid reserving PUBLIC as a keyword*/
					if (strcmp(yyvsp[0].str, "public") == 0)
						n->username = NULL;
					else
						n->username = yyvsp[0].str;
					n->groupname = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 468:
#line 2913 "gram.y"
    {
					PrivGrantee *n = makeNode(PrivGrantee);
					/* Treat GROUP PUBLIC as a synonym for PUBLIC */
					if (strcmp(yyvsp[0].str, "public") == 0)
						n->groupname = NULL;
					else
						n->groupname = yyvsp[0].str;
					n->username = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 469:
#line 2927 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 470:
#line 2928 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 471:
#line 2932 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 472:
#line 2933 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 473:
#line 2938 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 474:
#line 2940 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 475:
#line 2945 "gram.y"
    {
					FuncWithArgs *n = makeNode(FuncWithArgs);
					n->funcname = yyvsp[-1].list;
					n->funcargs = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 476:
#line 2965 "gram.y"
    {
					IndexStmt *n = makeNode(IndexStmt);
					n->unique = yyvsp[-9].boolean;
					n->idxname = yyvsp[-7].str;
					n->relation = yyvsp[-5].range;
					n->accessMethod = yyvsp[-4].str;
					n->indexParams = yyvsp[-2].list;
					n->whereClause = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 477:
#line 2978 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 478:
#line 2979 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 479:
#line 2983 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 480:
#line 2984 "gram.y"
    { yyval.str = DEFAULT_INDEX_TYPE; }
    break;

  case 481:
#line 2987 "gram.y"
    { yyval.list = makeList1(yyvsp[0].ielem); }
    break;

  case 482:
#line 2988 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].ielem); }
    break;

  case 483:
#line 2997 "gram.y"
    {
					yyval.ielem = makeNode(IndexElem);
					yyval.ielem->name = yyvsp[-1].str;
					yyval.ielem->expr = NULL;
					yyval.ielem->opclass = yyvsp[0].list;
				}
    break;

  case 484:
#line 3004 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = yyvsp[-4].list;
					n->args = yyvsp[-2].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;

					yyval.ielem = makeNode(IndexElem);
					yyval.ielem->name = NULL;
					yyval.ielem->expr = (Node *)n;
					yyval.ielem->opclass = yyvsp[0].list;
				}
    break;

  case 485:
#line 3017 "gram.y"
    {
					yyval.ielem = makeNode(IndexElem);
					yyval.ielem->name = NULL;
					yyval.ielem->expr = yyvsp[-2].node;
					yyval.ielem->opclass = yyvsp[0].list;
				}
    break;

  case 486:
#line 3025 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 487:
#line 3026 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 488:
#line 3027 "gram.y"
    { yyval.list = NIL; }
    break;

  case 489:
#line 3044 "gram.y"
    {
					CreateFunctionStmt *n = makeNode(CreateFunctionStmt);
					n->replace = yyvsp[-7].boolean;
					n->funcname = yyvsp[-5].list;
					n->argTypes = yyvsp[-4].list;
					n->returnType = yyvsp[-2].typnam;
					n->options = yyvsp[-1].list;
					n->withClause = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 490:
#line 3057 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 491:
#line 3058 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 492:
#line 3061 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 493:
#line 3062 "gram.y"
    { yyval.list = NIL; }
    break;

  case 494:
#line 3066 "gram.y"
    { yyval.list = makeList1(yyvsp[0].typnam); }
    break;

  case 495:
#line 3067 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].typnam); }
    break;

  case 496:
#line 3071 "gram.y"
    {
					/* We can catch over-specified arguments here if we want to,
					 * but for now better to silently swallow typmod, etc.
					 * - thomas 2000-03-22
					 */
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 497:
#line 3078 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 498:
#line 3081 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 499:
#line 3083 "gram.y"
    {
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("CREATE FUNCTION / OUT parameters are not implemented")));
					yyval.boolean = TRUE;
				}
    break;

  case 500:
#line 3090 "gram.y"
    {
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("CREATE FUNCTION / INOUT parameters are not implemented")));
					yyval.boolean = FALSE;
				}
    break;

  case 501:
#line 3100 "gram.y"
    {
					/* We can catch over-specified arguments here if we want to,
					 * but for now better to silently swallow typmod, etc.
					 * - thomas 2000-03-22
					 */
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 502:
#line 3113 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 503:
#line 3115 "gram.y"
    {
					yyval.typnam = makeNode(TypeName);
					yyval.typnam->names = lcons(makeString(yyvsp[-3].str), yyvsp[-2].list);
					yyval.typnam->pct_type = true;
					yyval.typnam->typmod = -1;
				}
    break;

  case 504:
#line 3126 "gram.y"
    { yyval.list = makeList1(yyvsp[0].defelt); }
    break;

  case 505:
#line 3127 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].defelt); }
    break;

  case 506:
#line 3132 "gram.y"
    {
					yyval.defelt = makeDefElem("as", (Node *)yyvsp[0].list);
				}
    break;

  case 507:
#line 3136 "gram.y"
    {
					yyval.defelt = makeDefElem("language", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 508:
#line 3140 "gram.y"
    {
					yyval.defelt = makeDefElem("volatility", (Node *)makeString("immutable"));
				}
    break;

  case 509:
#line 3144 "gram.y"
    {
					yyval.defelt = makeDefElem("volatility", (Node *)makeString("stable"));
				}
    break;

  case 510:
#line 3148 "gram.y"
    {
					yyval.defelt = makeDefElem("volatility", (Node *)makeString("volatile"));
				}
    break;

  case 511:
#line 3152 "gram.y"
    {
					yyval.defelt = makeDefElem("strict", (Node *)makeInteger(FALSE));
				}
    break;

  case 512:
#line 3156 "gram.y"
    {
					yyval.defelt = makeDefElem("strict", (Node *)makeInteger(TRUE));
				}
    break;

  case 513:
#line 3160 "gram.y"
    {
					yyval.defelt = makeDefElem("strict", (Node *)makeInteger(TRUE));
				}
    break;

  case 514:
#line 3164 "gram.y"
    {
					yyval.defelt = makeDefElem("security", (Node *)makeInteger(TRUE));
				}
    break;

  case 515:
#line 3168 "gram.y"
    {
					yyval.defelt = makeDefElem("security", (Node *)makeInteger(FALSE));
				}
    break;

  case 516:
#line 3172 "gram.y"
    {
					yyval.defelt = makeDefElem("security", (Node *)makeInteger(TRUE));
				}
    break;

  case 517:
#line 3176 "gram.y"
    {
					yyval.defelt = makeDefElem("security", (Node *)makeInteger(FALSE));
				}
    break;

  case 518:
#line 3181 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 519:
#line 3183 "gram.y"
    {
					yyval.list = makeList2(makeString(yyvsp[-2].str), makeString(yyvsp[0].str));
				}
    break;

  case 520:
#line 3189 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 521:
#line 3190 "gram.y"
    { yyval.list = NIL; }
    break;

  case 522:
#line 3206 "gram.y"
    {
					RemoveFuncStmt *n = makeNode(RemoveFuncStmt);
					n->funcname = yyvsp[-2].list;
					n->args = yyvsp[-1].list;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 523:
#line 3217 "gram.y"
    {
						RemoveAggrStmt *n = makeNode(RemoveAggrStmt);
						n->aggname = yyvsp[-4].list;
						n->aggtype = yyvsp[-2].typnam;
						n->behavior = yyvsp[0].dbehavior;
						yyval.node = (Node *)n;
				}
    break;

  case 524:
#line 3227 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 525:
#line 3228 "gram.y"
    { yyval.typnam = NULL; }
    break;

  case 526:
#line 3233 "gram.y"
    {
					RemoveOperStmt *n = makeNode(RemoveOperStmt);
					n->opname = yyvsp[-4].list;
					n->args = yyvsp[-2].list;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 527:
#line 3244 "gram.y"
    {
				   ereport(ERROR,
						   (errcode(ERRCODE_SYNTAX_ERROR),
							errmsg("argument type missing (use NONE for unary operators)")));
				}
    break;

  case 528:
#line 3250 "gram.y"
    { yyval.list = makeList2(yyvsp[-2].typnam, yyvsp[0].typnam); }
    break;

  case 529:
#line 3252 "gram.y"
    { yyval.list = makeList2(NULL, yyvsp[0].typnam); }
    break;

  case 530:
#line 3254 "gram.y"
    { yyval.list = makeList2(yyvsp[-2].typnam, NULL); }
    break;

  case 531:
#line 3259 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 532:
#line 3261 "gram.y"
    { yyval.list = lcons(makeString(yyvsp[-2].str), yyvsp[0].list); }
    break;

  case 533:
#line 3273 "gram.y"
    {
					CreateCastStmt *n = makeNode(CreateCastStmt);
					n->sourcetype = yyvsp[-7].typnam;
					n->targettype = yyvsp[-5].typnam;
					n->func = (FuncWithArgs *) yyvsp[-1].node;
					n->context = (CoercionContext) yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 534:
#line 3283 "gram.y"
    {
					CreateCastStmt *n = makeNode(CreateCastStmt);
					n->sourcetype = yyvsp[-6].typnam;
					n->targettype = yyvsp[-4].typnam;
					n->func = NULL;
					n->context = (CoercionContext) yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 535:
#line 3293 "gram.y"
    { yyval.ival = COERCION_IMPLICIT; }
    break;

  case 536:
#line 3294 "gram.y"
    { yyval.ival = COERCION_ASSIGNMENT; }
    break;

  case 537:
#line 3295 "gram.y"
    { yyval.ival = COERCION_EXPLICIT; }
    break;

  case 538:
#line 3300 "gram.y"
    {
					DropCastStmt *n = makeNode(DropCastStmt);
					n->sourcetype = yyvsp[-4].typnam;
					n->targettype = yyvsp[-2].typnam;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 539:
#line 3321 "gram.y"
    {
					ReindexStmt *n = makeNode(ReindexStmt);
					n->kind = yyvsp[-2].objtype;
					n->relation = yyvsp[-1].range;
					n->name = NULL;
					n->force = yyvsp[0].boolean;
					yyval.node = (Node *)n;
				}
    break;

  case 540:
#line 3330 "gram.y"
    {
					ReindexStmt *n = makeNode(ReindexStmt);
					n->kind = OBJECT_DATABASE;
					n->name = yyvsp[-1].str;
					n->relation = NULL;
					n->force = yyvsp[0].boolean;
					yyval.node = (Node *)n;
				}
    break;

  case 541:
#line 3341 "gram.y"
    { yyval.objtype = OBJECT_INDEX; }
    break;

  case 542:
#line 3342 "gram.y"
    { yyval.objtype = OBJECT_TABLE; }
    break;

  case 543:
#line 3345 "gram.y"
    {  yyval.boolean = TRUE; }
    break;

  case 544:
#line 3346 "gram.y"
    {  yyval.boolean = FALSE; }
    break;

  case 545:
#line 3357 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_AGGREGATE;
					n->object = yyvsp[-6].list;
					n->objarg = makeList1(yyvsp[-4].typnam);
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 546:
#line 3366 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_CONVERSION;
					n->object = yyvsp[-3].list;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 547:
#line 3374 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_DATABASE;
					n->subname = yyvsp[-3].str;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 548:
#line 3382 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_FUNCTION;
					n->object = yyvsp[-4].list;
					n->objarg = yyvsp[-3].list;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 549:
#line 3391 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_GROUP;
					n->subname = yyvsp[-3].str;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 550:
#line 3399 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_LANGUAGE;
					n->subname = yyvsp[-3].str;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 551:
#line 3407 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_OPCLASS;
					n->object = yyvsp[-5].list;
					n->subname = yyvsp[-3].str;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 552:
#line 3416 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_SCHEMA;
					n->subname = yyvsp[-3].str;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 553:
#line 3424 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->relation = yyvsp[-5].range;
					n->subname = yyvsp[-2].str;
					n->newname = yyvsp[0].str;
					if (yyvsp[-2].str == NULL)
						n->renameType = OBJECT_TABLE;
					else
						n->renameType = OBJECT_COLUMN;
					yyval.node = (Node *)n;
				}
    break;

  case 554:
#line 3436 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->relation = yyvsp[-3].range;
					n->subname = yyvsp[-5].str;
					n->newname = yyvsp[0].str;
					n->renameType = OBJECT_TRIGGER;
					yyval.node = (Node *)n;
				}
    break;

  case 555:
#line 3445 "gram.y"
    {
					RenameStmt *n = makeNode(RenameStmt);
					n->renameType = OBJECT_USER;
					n->subname = yyvsp[-3].str;
					n->newname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 556:
#line 3454 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 557:
#line 3455 "gram.y"
    { yyval.str = NULL; }
    break;

  case 558:
#line 3458 "gram.y"
    { yyval.ival = COLUMN; }
    break;

  case 559:
#line 3459 "gram.y"
    { yyval.ival = 0; }
    break;

  case 560:
#line 3470 "gram.y"
    { QueryIsRule=TRUE; }
    break;

  case 561:
#line 3473 "gram.y"
    {
					RuleStmt *n = makeNode(RuleStmt);
					n->replace = yyvsp[-12].boolean;
					n->relation = yyvsp[-4].range;
					n->rulename = yyvsp[-10].str;
					n->whereClause = yyvsp[-3].node;
					n->event = yyvsp[-6].ival;
					n->instead = yyvsp[-1].boolean;
					n->actions = yyvsp[0].list;
					yyval.node = (Node *)n;
					QueryIsRule=FALSE;
				}
    break;

  case 562:
#line 3488 "gram.y"
    { yyval.list = NIL; }
    break;

  case 563:
#line 3489 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 564:
#line 3490 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 565:
#line 3496 "gram.y"
    { if (yyvsp[0].node != (Node *) NULL)
					yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node);
				  else
					yyval.list = yyvsp[-2].list;
				}
    break;

  case 566:
#line 3502 "gram.y"
    { if (yyvsp[0].node != (Node *) NULL)
					yyval.list = makeList1(yyvsp[0].node);
				  else
					yyval.list = NIL;
				}
    break;

  case 572:
#line 3518 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 573:
#line 3519 "gram.y"
    { yyval.node = (Node *)NULL; }
    break;

  case 574:
#line 3523 "gram.y"
    { yyval.ival = CMD_SELECT; }
    break;

  case 575:
#line 3524 "gram.y"
    { yyval.ival = CMD_UPDATE; }
    break;

  case 576:
#line 3525 "gram.y"
    { yyval.ival = CMD_DELETE; }
    break;

  case 577:
#line 3526 "gram.y"
    { yyval.ival = CMD_INSERT; }
    break;

  case 578:
#line 3530 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 579:
#line 3531 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 580:
#line 3537 "gram.y"
    {
					DropPropertyStmt *n = makeNode(DropPropertyStmt);
					n->relation = yyvsp[-1].range;
					n->property = yyvsp[-3].str;
					n->behavior = yyvsp[0].dbehavior;
					n->removeType = OBJECT_RULE;
					yyval.node = (Node *) n;
				}
    break;

  case 581:
#line 3557 "gram.y"
    {
					NotifyStmt *n = makeNode(NotifyStmt);
					n->relation = yyvsp[0].range;
					yyval.node = (Node *)n;
				}
    break;

  case 582:
#line 3565 "gram.y"
    {
					ListenStmt *n = makeNode(ListenStmt);
					n->relation = yyvsp[0].range;
					yyval.node = (Node *)n;
				}
    break;

  case 583:
#line 3574 "gram.y"
    {
					UnlistenStmt *n = makeNode(UnlistenStmt);
					n->relation = yyvsp[0].range;
					yyval.node = (Node *)n;
				}
    break;

  case 584:
#line 3580 "gram.y"
    {
					UnlistenStmt *n = makeNode(UnlistenStmt);
					n->relation = makeNode(RangeVar);
					n->relation->relname = "*";
					n->relation->schemaname = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 585:
#line 3601 "gram.y"
    {
					TransactionStmt *n = makeNode(TransactionStmt);
					n->kind = TRANS_STMT_ROLLBACK;
					n->options = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 586:
#line 3608 "gram.y"
    {
					TransactionStmt *n = makeNode(TransactionStmt);
					n->kind = TRANS_STMT_BEGIN;
					n->options = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 587:
#line 3615 "gram.y"
    {
					TransactionStmt *n = makeNode(TransactionStmt);
					n->kind = TRANS_STMT_START;
					n->options = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 588:
#line 3622 "gram.y"
    {
					TransactionStmt *n = makeNode(TransactionStmt);
					n->kind = TRANS_STMT_COMMIT;
					n->options = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 589:
#line 3629 "gram.y"
    {
					TransactionStmt *n = makeNode(TransactionStmt);
					n->kind = TRANS_STMT_COMMIT;
					n->options = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 590:
#line 3636 "gram.y"
    {
					TransactionStmt *n = makeNode(TransactionStmt);
					n->kind = TRANS_STMT_ROLLBACK;
					n->options = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 591:
#line 3644 "gram.y"
    {}
    break;

  case 592:
#line 3645 "gram.y"
    {}
    break;

  case 593:
#line 3646 "gram.y"
    {}
    break;

  case 594:
#line 3651 "gram.y"
    { yyval.list = makeList1(makeDefElem("transaction_isolation",
												 makeStringConst(yyvsp[0].str, NULL))); }
    break;

  case 595:
#line 3654 "gram.y"
    { yyval.list = makeList1(makeDefElem("transaction_read_only",
												 makeIntConst(yyvsp[0].boolean))); }
    break;

  case 596:
#line 3657 "gram.y"
    {
						yyval.list = makeList2(makeDefElem("transaction_isolation",
												   makeStringConst(yyvsp[-1].str, NULL)),
									   makeDefElem("transaction_read_only",
												   makeIntConst(yyvsp[0].boolean)));
					}
    break;

  case 597:
#line 3664 "gram.y"
    {
						yyval.list = makeList2(makeDefElem("transaction_read_only",
												   makeIntConst(yyvsp[-3].boolean)),
									   makeDefElem("transaction_isolation",
												   makeStringConst(yyvsp[0].str, NULL)));
					}
    break;

  case 599:
#line 3675 "gram.y"
    { yyval.list = NIL; }
    break;

  case 600:
#line 3679 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 601:
#line 3680 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 602:
#line 3693 "gram.y"
    {
					ViewStmt *n = makeNode(ViewStmt);
					n->replace = yyvsp[-5].boolean;
					n->view = yyvsp[-3].range;
					n->aliases = yyvsp[-2].list;
					n->query = (Query *) yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 603:
#line 3712 "gram.y"
    {
					LoadStmt *n = makeNode(LoadStmt);
					n->filename = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 604:
#line 3728 "gram.y"
    {
					CreatedbStmt *n = makeNode(CreatedbStmt);
					n->dbname = yyvsp[-2].str;
					n->options = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 605:
#line 3737 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].defelt); }
    break;

  case 606:
#line 3738 "gram.y"
    { yyval.list = NIL; }
    break;

  case 607:
#line 3743 "gram.y"
    {
					yyval.defelt = makeDefElem("location", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 608:
#line 3747 "gram.y"
    {
					yyval.defelt = makeDefElem("location", NULL);
				}
    break;

  case 609:
#line 3751 "gram.y"
    {
					yyval.defelt = makeDefElem("template", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 610:
#line 3755 "gram.y"
    {
					yyval.defelt = makeDefElem("template", NULL);
				}
    break;

  case 611:
#line 3759 "gram.y"
    {
					yyval.defelt = makeDefElem("encoding", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 612:
#line 3763 "gram.y"
    {
					yyval.defelt = makeDefElem("encoding", (Node *)makeInteger(yyvsp[0].ival));
				}
    break;

  case 613:
#line 3767 "gram.y"
    {
					yyval.defelt = makeDefElem("encoding", NULL);
				}
    break;

  case 614:
#line 3771 "gram.y"
    {
					yyval.defelt = makeDefElem("owner", (Node *)makeString(yyvsp[0].str));
				}
    break;

  case 615:
#line 3775 "gram.y"
    {
					yyval.defelt = makeDefElem("owner", NULL);
				}
    break;

  case 616:
#line 3785 "gram.y"
    {}
    break;

  case 617:
#line 3786 "gram.y"
    {}
    break;

  case 618:
#line 3798 "gram.y"
    {
					AlterDatabaseSetStmt *n = makeNode(AlterDatabaseSetStmt);
					n->dbname = yyvsp[-2].str;
					n->variable = yyvsp[0].vsetstmt->name;
					n->value = yyvsp[0].vsetstmt->args;
					yyval.node = (Node *)n;
				}
    break;

  case 619:
#line 3806 "gram.y"
    {
					AlterDatabaseSetStmt *n = makeNode(AlterDatabaseSetStmt);
					n->dbname = yyvsp[-1].str;
					n->variable = ((VariableResetStmt *)yyvsp[0].node)->name;
					n->value = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 620:
#line 3824 "gram.y"
    {
					DropdbStmt *n = makeNode(DropdbStmt);
					n->dbname = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 621:
#line 3840 "gram.y"
    {
					CreateDomainStmt *n = makeNode(CreateDomainStmt);
					n->domainname = yyvsp[-3].list;
					n->typename = yyvsp[-1].typnam;
					n->constraints = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 622:
#line 3852 "gram.y"
    {
					AlterDomainStmt *n = makeNode(AlterDomainStmt);
					n->subtype = 'T';
					n->typename = yyvsp[-1].list;
					n->def = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 623:
#line 3861 "gram.y"
    {
					AlterDomainStmt *n = makeNode(AlterDomainStmt);
					n->subtype = 'N';
					n->typename = yyvsp[-3].list;
					yyval.node = (Node *)n;
				}
    break;

  case 624:
#line 3869 "gram.y"
    {
					AlterDomainStmt *n = makeNode(AlterDomainStmt);
					n->subtype = 'O';
					n->typename = yyvsp[-3].list;
					yyval.node = (Node *)n;
				}
    break;

  case 625:
#line 3877 "gram.y"
    {
					AlterDomainStmt *n = makeNode(AlterDomainStmt);
					n->subtype = 'C';
					n->typename = yyvsp[-2].list;
					n->def = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 626:
#line 3886 "gram.y"
    {
					AlterDomainStmt *n = makeNode(AlterDomainStmt);
					n->subtype = 'X';
					n->typename = yyvsp[-4].list;
					n->name = yyvsp[-1].str;
					n->behavior = yyvsp[0].dbehavior;
					yyval.node = (Node *)n;
				}
    break;

  case 627:
#line 3896 "gram.y"
    {
					AlterDomainStmt *n = makeNode(AlterDomainStmt);
					n->subtype = 'U';
					n->typename = yyvsp[-3].list;
					n->name = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 628:
#line 3905 "gram.y"
    {}
    break;

  case 629:
#line 3906 "gram.y"
    {}
    break;

  case 630:
#line 3922 "gram.y"
    {
			  CreateConversionStmt *n = makeNode(CreateConversionStmt);
			  n->conversion_name = yyvsp[-6].list;
			  n->for_encoding_name = yyvsp[-4].str;
			  n->to_encoding_name = yyvsp[-2].str;
			  n->func_name = yyvsp[0].list;
			  n->def = yyvsp[-8].boolean;
			  yyval.node = (Node *)n;
			}
    break;

  case 631:
#line 3944 "gram.y"
    {
				   ClusterStmt *n = makeNode(ClusterStmt);
				   n->relation = yyvsp[0].range;
				   n->indexname = yyvsp[-2].str;
				   yyval.node = (Node*)n;
				}
    break;

  case 632:
#line 3951 "gram.y"
    {
			       ClusterStmt *n = makeNode(ClusterStmt);
				   n->relation = yyvsp[0].range;
				   n->indexname = NULL;
				   yyval.node = (Node*)n;
				}
    break;

  case 633:
#line 3958 "gram.y"
    {
				   ClusterStmt *n = makeNode(ClusterStmt);
				   n->relation = NULL;
				   n->indexname = NULL;
				   yyval.node = (Node*)n;
				}
    break;

  case 634:
#line 3975 "gram.y"
    {
					VacuumStmt *n = makeNode(VacuumStmt);
					n->vacuum = true;
					n->analyze = false;
					n->full = yyvsp[-2].boolean;
					n->freeze = yyvsp[-1].boolean;
					n->verbose = yyvsp[0].boolean;
					n->relation = NULL;
					n->va_cols = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 635:
#line 3987 "gram.y"
    {
					VacuumStmt *n = makeNode(VacuumStmt);
					n->vacuum = true;
					n->analyze = false;
					n->full = yyvsp[-3].boolean;
					n->freeze = yyvsp[-2].boolean;
					n->verbose = yyvsp[-1].boolean;
					n->relation = yyvsp[0].range;
					n->va_cols = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 636:
#line 3999 "gram.y"
    {
					VacuumStmt *n = (VacuumStmt *) yyvsp[0].node;
					n->vacuum = true;
					n->full = yyvsp[-3].boolean;
					n->freeze = yyvsp[-2].boolean;
					n->verbose |= yyvsp[-1].boolean;
					yyval.node = (Node *)n;
				}
    break;

  case 637:
#line 4011 "gram.y"
    {
					VacuumStmt *n = makeNode(VacuumStmt);
					n->vacuum = false;
					n->analyze = true;
					n->full = false;
					n->freeze = false;
					n->verbose = yyvsp[0].boolean;
					n->relation = NULL;
					n->va_cols = NIL;
					yyval.node = (Node *)n;
				}
    break;

  case 638:
#line 4023 "gram.y"
    {
					VacuumStmt *n = makeNode(VacuumStmt);
					n->vacuum = false;
					n->analyze = true;
					n->full = false;
					n->freeze = false;
					n->verbose = yyvsp[-2].boolean;
					n->relation = yyvsp[-1].range;
					n->va_cols = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 639:
#line 4037 "gram.y"
    {}
    break;

  case 640:
#line 4038 "gram.y"
    {}
    break;

  case 641:
#line 4042 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 642:
#line 4043 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 643:
#line 4046 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 644:
#line 4047 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 645:
#line 4050 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 646:
#line 4051 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 647:
#line 4055 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 648:
#line 4056 "gram.y"
    { yyval.list = NIL; }
    break;

  case 649:
#line 4068 "gram.y"
    {
					ExplainStmt *n = makeNode(ExplainStmt);
					n->analyze = yyvsp[-2].boolean;
					n->verbose = yyvsp[-1].boolean;
					n->query = (Query*)yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 656:
#line 4087 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 657:
#line 4088 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 658:
#line 4099 "gram.y"
    {
					PrepareStmt *n = makeNode(PrepareStmt);
					n->name = yyvsp[-3].str;
					n->argtypes = yyvsp[-2].list;
					n->query = (Query *) yyvsp[0].node;
					yyval.node = (Node *) n;
				}
    break;

  case 659:
#line 4108 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 660:
#line 4109 "gram.y"
    { yyval.list = NIL; }
    break;

  case 661:
#line 4112 "gram.y"
    { yyval.list = makeList1(yyvsp[0].typnam); }
    break;

  case 662:
#line 4114 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].typnam); }
    break;

  case 667:
#line 4132 "gram.y"
    {
					ExecuteStmt *n = makeNode(ExecuteStmt);
					n->name = yyvsp[-1].str;
					n->params = yyvsp[0].list;
					n->into = NULL;
					yyval.node = (Node *) n;
				}
    break;

  case 668:
#line 4140 "gram.y"
    {
					ExecuteStmt *n = makeNode(ExecuteStmt);
					n->name = yyvsp[-1].str;
					n->params = yyvsp[0].list;
					yyvsp[-5].range->istemp = yyvsp[-7].boolean;
					n->into = yyvsp[-5].range;
					if (yyvsp[-4].list)
						ereport(ERROR,
								(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
								 errmsg("column name list not allowed in CREATE TABLE / AS EXECUTE")));
					/* ... because it's not implemented, but it could be */
					yyval.node = (Node *) n;
				}
    break;

  case 669:
#line 4155 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 670:
#line 4156 "gram.y"
    { yyval.list = NIL; }
    break;

  case 671:
#line 4167 "gram.y"
    {
						DeallocateStmt *n = makeNode(DeallocateStmt);
						n->name = yyvsp[0].str;
						yyval.node = (Node *) n;
					}
    break;

  case 672:
#line 4173 "gram.y"
    {
						DeallocateStmt *n = makeNode(DeallocateStmt);
						n->name = yyvsp[0].str;
						yyval.node = (Node *) n;
					}
    break;

  case 673:
#line 4189 "gram.y"
    {
					yyvsp[0].istmt->relation = yyvsp[-1].range;
					yyval.node = (Node *) yyvsp[0].istmt;
				}
    break;

  case 674:
#line 4197 "gram.y"
    {
					yyval.istmt = makeNode(InsertStmt);
					yyval.istmt->cols = NIL;
					yyval.istmt->targetList = yyvsp[-1].list;
					yyval.istmt->selectStmt = NULL;
				}
    break;

  case 675:
#line 4204 "gram.y"
    {
					yyval.istmt = makeNode(InsertStmt);
					yyval.istmt->cols = NIL;
					yyval.istmt->targetList = NIL;
					yyval.istmt->selectStmt = NULL;
				}
    break;

  case 676:
#line 4211 "gram.y"
    {
					yyval.istmt = makeNode(InsertStmt);
					yyval.istmt->cols = NIL;
					yyval.istmt->targetList = NIL;
					yyval.istmt->selectStmt = yyvsp[0].node;
				}
    break;

  case 677:
#line 4218 "gram.y"
    {
					yyval.istmt = makeNode(InsertStmt);
					yyval.istmt->cols = yyvsp[-5].list;
					yyval.istmt->targetList = yyvsp[-1].list;
					yyval.istmt->selectStmt = NULL;
				}
    break;

  case 678:
#line 4225 "gram.y"
    {
					yyval.istmt = makeNode(InsertStmt);
					yyval.istmt->cols = yyvsp[-2].list;
					yyval.istmt->targetList = NIL;
					yyval.istmt->selectStmt = yyvsp[0].node;
				}
    break;

  case 679:
#line 4234 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 680:
#line 4236 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 681:
#line 4241 "gram.y"
    {
					ResTarget *n = makeNode(ResTarget);
					n->name = yyvsp[-1].str;
					n->indirection = yyvsp[0].list;
					n->val = NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 682:
#line 4259 "gram.y"
    {
					DeleteStmt *n = makeNode(DeleteStmt);
					n->relation = yyvsp[-1].range;
					n->whereClause = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 683:
#line 4268 "gram.y"
    {
					LockStmt *n = makeNode(LockStmt);

					n->relations = yyvsp[-1].list;
					n->mode = yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 684:
#line 4277 "gram.y"
    { yyval.ival = yyvsp[-1].ival; }
    break;

  case 685:
#line 4278 "gram.y"
    { yyval.ival = AccessExclusiveLock; }
    break;

  case 686:
#line 4281 "gram.y"
    { yyval.ival = AccessShareLock; }
    break;

  case 687:
#line 4282 "gram.y"
    { yyval.ival = RowShareLock; }
    break;

  case 688:
#line 4283 "gram.y"
    { yyval.ival = RowExclusiveLock; }
    break;

  case 689:
#line 4284 "gram.y"
    { yyval.ival = ShareUpdateExclusiveLock; }
    break;

  case 690:
#line 4285 "gram.y"
    { yyval.ival = ShareLock; }
    break;

  case 691:
#line 4286 "gram.y"
    { yyval.ival = ShareRowExclusiveLock; }
    break;

  case 692:
#line 4287 "gram.y"
    { yyval.ival = ExclusiveLock; }
    break;

  case 693:
#line 4288 "gram.y"
    { yyval.ival = AccessExclusiveLock; }
    break;

  case 694:
#line 4303 "gram.y"
    {
					UpdateStmt *n = makeNode(UpdateStmt);
					n->relation = yyvsp[-4].range;
					n->targetList = yyvsp[-2].list;
					n->fromClause = yyvsp[-1].list;
					n->whereClause = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 695:
#line 4321 "gram.y"
    {
					DeclareCursorStmt *n = makeNode(DeclareCursorStmt);
					n->portalname = yyvsp[-5].str;
					n->options = yyvsp[-4].ival;
					n->query = yyvsp[0].node;
					if (yyvsp[-2].boolean)
						n->options |= CURSOR_OPT_HOLD;
					yyval.node = (Node *)n;
				}
    break;

  case 696:
#line 4332 "gram.y"
    { yyval.ival = 0; }
    break;

  case 697:
#line 4333 "gram.y"
    { yyval.ival = yyvsp[-2].ival | CURSOR_OPT_NO_SCROLL; }
    break;

  case 698:
#line 4334 "gram.y"
    { yyval.ival = yyvsp[-1].ival | CURSOR_OPT_SCROLL; }
    break;

  case 699:
#line 4335 "gram.y"
    { yyval.ival = yyvsp[-1].ival | CURSOR_OPT_BINARY; }
    break;

  case 700:
#line 4336 "gram.y"
    { yyval.ival = yyvsp[-1].ival | CURSOR_OPT_INSENSITIVE; }
    break;

  case 701:
#line 4339 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 702:
#line 4340 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 703:
#line 4341 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 706:
#line 4394 "gram.y"
    { yyval.node = yyvsp[-1].node; }
    break;

  case 707:
#line 4395 "gram.y"
    { yyval.node = yyvsp[-1].node; }
    break;

  case 708:
#line 4405 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 709:
#line 4407 "gram.y"
    {
					insertSelectOptions((SelectStmt *) yyvsp[-1].node, yyvsp[0].list, NIL,
										NULL, NULL);
					yyval.node = yyvsp[-1].node;
				}
    break;

  case 710:
#line 4413 "gram.y"
    {
					insertSelectOptions((SelectStmt *) yyvsp[-3].node, yyvsp[-2].list, yyvsp[-1].list,
										nth(0, yyvsp[0].list), nth(1, yyvsp[0].list));
					yyval.node = yyvsp[-3].node;
				}
    break;

  case 711:
#line 4419 "gram.y"
    {
					insertSelectOptions((SelectStmt *) yyvsp[-3].node, yyvsp[-2].list, yyvsp[0].list,
										nth(0, yyvsp[-1].list), nth(1, yyvsp[-1].list));
					yyval.node = yyvsp[-3].node;
				}
    break;

  case 712:
#line 4427 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 713:
#line 4428 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 714:
#line 4458 "gram.y"
    {
					SelectStmt *n = makeNode(SelectStmt);
					n->distinctClause = yyvsp[-6].list;
					n->targetList = yyvsp[-5].list;
					n->into = yyvsp[-4].range;
					n->intoColNames = NIL;
					n->fromClause = yyvsp[-3].list;
					n->whereClause = yyvsp[-2].node;
					n->groupClause = yyvsp[-1].list;
					n->havingClause = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 715:
#line 4471 "gram.y"
    {
					yyval.node = makeSetOp(SETOP_UNION, yyvsp[-1].boolean, yyvsp[-3].node, yyvsp[0].node);
				}
    break;

  case 716:
#line 4475 "gram.y"
    {
					yyval.node = makeSetOp(SETOP_INTERSECT, yyvsp[-1].boolean, yyvsp[-3].node, yyvsp[0].node);
				}
    break;

  case 717:
#line 4479 "gram.y"
    {
					yyval.node = makeSetOp(SETOP_EXCEPT, yyvsp[-1].boolean, yyvsp[-3].node, yyvsp[0].node);
				}
    break;

  case 718:
#line 4485 "gram.y"
    { yyval.range = yyvsp[0].range; }
    break;

  case 719:
#line 4486 "gram.y"
    { yyval.range = NULL; }
    break;

  case 720:
#line 4495 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = true;
				}
    break;

  case 721:
#line 4500 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = true;
				}
    break;

  case 722:
#line 4505 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = true;
				}
    break;

  case 723:
#line 4510 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = true;
				}
    break;

  case 724:
#line 4515 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = true;
				}
    break;

  case 725:
#line 4520 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = true;
				}
    break;

  case 726:
#line 4525 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = false;
				}
    break;

  case 727:
#line 4530 "gram.y"
    {
					yyval.range = yyvsp[0].range;
					yyval.range->istemp = false;
				}
    break;

  case 728:
#line 4536 "gram.y"
    {}
    break;

  case 729:
#line 4537 "gram.y"
    {}
    break;

  case 730:
#line 4540 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 731:
#line 4541 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 732:
#line 4542 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 733:
#line 4549 "gram.y"
    { yyval.list = makeList1(NIL); }
    break;

  case 734:
#line 4550 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 735:
#line 4551 "gram.y"
    { yyval.list = NIL; }
    break;

  case 736:
#line 4552 "gram.y"
    { yyval.list = NIL; }
    break;

  case 737:
#line 4556 "gram.y"
    { yyval.list = yyvsp[0].list;}
    break;

  case 738:
#line 4557 "gram.y"
    { yyval.list = NIL; }
    break;

  case 739:
#line 4561 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 740:
#line 4565 "gram.y"
    { yyval.list = makeList1(yyvsp[0].sortby); }
    break;

  case 741:
#line 4566 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].sortby); }
    break;

  case 742:
#line 4570 "gram.y"
    {
					yyval.sortby = makeNode(SortBy);
					yyval.sortby->node = yyvsp[-2].node;
					yyval.sortby->sortby_kind = SORTBY_USING;
					yyval.sortby->useOp = yyvsp[0].list;
				}
    break;

  case 743:
#line 4577 "gram.y"
    {
					yyval.sortby = makeNode(SortBy);
					yyval.sortby->node = yyvsp[-1].node;
					yyval.sortby->sortby_kind = SORTBY_ASC;
					yyval.sortby->useOp = NIL;
				}
    break;

  case 744:
#line 4584 "gram.y"
    {
					yyval.sortby = makeNode(SortBy);
					yyval.sortby->node = yyvsp[-1].node;
					yyval.sortby->sortby_kind = SORTBY_DESC;
					yyval.sortby->useOp = NIL;
				}
    break;

  case 745:
#line 4591 "gram.y"
    {
					yyval.sortby = makeNode(SortBy);
					yyval.sortby->node = yyvsp[0].node;
					yyval.sortby->sortby_kind = SORTBY_ASC;	/* default */
					yyval.sortby->useOp = NIL;
				}
    break;

  case 746:
#line 4602 "gram.y"
    { yyval.list = makeList2(yyvsp[0].node, yyvsp[-2].node); }
    break;

  case 747:
#line 4604 "gram.y"
    { yyval.list = makeList2(yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 748:
#line 4606 "gram.y"
    { yyval.list = makeList2(NULL, yyvsp[0].node); }
    break;

  case 749:
#line 4608 "gram.y"
    { yyval.list = makeList2(yyvsp[0].node, NULL); }
    break;

  case 750:
#line 4610 "gram.y"
    {
					/* Disabled because it was too confusing, bjm 2002-02-18 */
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("LIMIT #,# syntax is not supported"),
							 errhint("Use separate LIMIT and OFFSET clauses.")));
				}
    break;

  case 751:
#line 4620 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 752:
#line 4622 "gram.y"
    { yyval.list = makeList2(NULL,NULL); }
    break;

  case 753:
#line 4626 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 754:
#line 4628 "gram.y"
    {
					/* LIMIT ALL is represented as a NULL constant */
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Null;
					yyval.node = (Node *)n;
				}
    break;

  case 755:
#line 4637 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 756:
#line 4649 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 757:
#line 4650 "gram.y"
    { yyval.list = NIL; }
    break;

  case 758:
#line 4654 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 759:
#line 4655 "gram.y"
    { yyval.node = NULL; }
    break;

  case 760:
#line 4659 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 761:
#line 4660 "gram.y"
    { yyval.list = NULL; }
    break;

  case 762:
#line 4664 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 763:
#line 4665 "gram.y"
    { yyval.list = NULL; }
    break;

  case 764:
#line 4669 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 765:
#line 4670 "gram.y"
    { yyval.list = makeList1(NULL); }
    break;

  case 766:
#line 4682 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 767:
#line 4683 "gram.y"
    { yyval.list = NIL; }
    break;

  case 768:
#line 4687 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 769:
#line 4688 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 770:
#line 4699 "gram.y"
    {
					yyval.node = (Node *) yyvsp[0].range;
				}
    break;

  case 771:
#line 4703 "gram.y"
    {
					yyvsp[-1].range->alias = yyvsp[0].alias;
					yyval.node = (Node *) yyvsp[-1].range;
				}
    break;

  case 772:
#line 4708 "gram.y"
    {
					RangeFunction *n = makeNode(RangeFunction);
					n->funccallnode = yyvsp[0].node;
					n->coldeflist = NIL;
					yyval.node = (Node *) n;
				}
    break;

  case 773:
#line 4715 "gram.y"
    {
					RangeFunction *n = makeNode(RangeFunction);
					n->funccallnode = yyvsp[-1].node;
					n->alias = yyvsp[0].alias;
					n->coldeflist = NIL;
					yyval.node = (Node *) n;
				}
    break;

  case 774:
#line 4723 "gram.y"
    {
					RangeFunction *n = makeNode(RangeFunction);
					n->funccallnode = yyvsp[-4].node;
					n->coldeflist = yyvsp[-1].list;
					yyval.node = (Node *) n;
				}
    break;

  case 775:
#line 4730 "gram.y"
    {
					RangeFunction *n = makeNode(RangeFunction);
					Alias *a = makeNode(Alias);
					n->funccallnode = yyvsp[-5].node;
					a->aliasname = yyvsp[-3].str;
					n->alias = a;
					n->coldeflist = yyvsp[-1].list;
					yyval.node = (Node *) n;
				}
    break;

  case 776:
#line 4740 "gram.y"
    {
					RangeFunction *n = makeNode(RangeFunction);
					Alias *a = makeNode(Alias);
					n->funccallnode = yyvsp[-4].node;
					a->aliasname = yyvsp[-3].str;
					n->alias = a;
					n->coldeflist = yyvsp[-1].list;
					yyval.node = (Node *) n;
				}
    break;

  case 777:
#line 4750 "gram.y"
    {
					/*
					 * The SQL spec does not permit a subselect
					 * (<derived_table>) without an alias clause,
					 * so we don't either.  This avoids the problem
					 * of needing to invent a unique refname for it.
					 * That could be surmounted if there's sufficient
					 * popular demand, but for now let's just implement
					 * the spec and see if anyone complains.
					 * However, it does seem like a good idea to emit
					 * an error message that's better than "syntax error".
					 */
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("sub-select in FROM must have an alias"),
							 errhint("For example, FROM (SELECT ...) [AS] foo.")));
					yyval.node = NULL;
				}
    break;

  case 778:
#line 4769 "gram.y"
    {
					RangeSubselect *n = makeNode(RangeSubselect);
					n->subquery = yyvsp[-1].node;
					n->alias = yyvsp[0].alias;
					yyval.node = (Node *) n;
				}
    break;

  case 779:
#line 4776 "gram.y"
    {
					yyval.node = (Node *) yyvsp[0].jexpr;
				}
    break;

  case 780:
#line 4780 "gram.y"
    {
					yyvsp[-2].jexpr->alias = yyvsp[0].alias;
					yyval.node = (Node *) yyvsp[-2].jexpr;
				}
    break;

  case 781:
#line 4806 "gram.y"
    {
					yyval.jexpr = yyvsp[-1].jexpr;
				}
    break;

  case 782:
#line 4810 "gram.y"
    {
					/* CROSS JOIN is same as unqualified inner join */
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = JOIN_INNER;
					n->isNatural = FALSE;
					n->larg = yyvsp[-3].node;
					n->rarg = yyvsp[0].node;
					n->using = NIL;
					n->quals = NULL;
					yyval.jexpr = n;
				}
    break;

  case 783:
#line 4822 "gram.y"
    {
					/* UNION JOIN is made into 1 token to avoid shift/reduce
					 * conflict against regular UNION keyword.
					 */
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = JOIN_UNION;
					n->isNatural = FALSE;
					n->larg = yyvsp[-2].node;
					n->rarg = yyvsp[0].node;
					n->using = NIL;
					n->quals = NULL;
					yyval.jexpr = n;
				}
    break;

  case 784:
#line 4836 "gram.y"
    {
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = yyvsp[-3].jtype;
					n->isNatural = FALSE;
					n->larg = yyvsp[-4].node;
					n->rarg = yyvsp[-1].node;
					if (yyvsp[0].node != NULL && IsA(yyvsp[0].node, List))
						n->using = (List *) yyvsp[0].node; /* USING clause */
					else
						n->quals = yyvsp[0].node; /* ON clause */
					yyval.jexpr = n;
				}
    break;

  case 785:
#line 4849 "gram.y"
    {
					/* letting join_type reduce to empty doesn't work */
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = JOIN_INNER;
					n->isNatural = FALSE;
					n->larg = yyvsp[-3].node;
					n->rarg = yyvsp[-1].node;
					if (yyvsp[0].node != NULL && IsA(yyvsp[0].node, List))
						n->using = (List *) yyvsp[0].node; /* USING clause */
					else
						n->quals = yyvsp[0].node; /* ON clause */
					yyval.jexpr = n;
				}
    break;

  case 786:
#line 4863 "gram.y"
    {
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = yyvsp[-2].jtype;
					n->isNatural = TRUE;
					n->larg = yyvsp[-4].node;
					n->rarg = yyvsp[0].node;
					n->using = NIL; /* figure out which columns later... */
					n->quals = NULL; /* fill later */
					yyval.jexpr = n;
				}
    break;

  case 787:
#line 4874 "gram.y"
    {
					/* letting join_type reduce to empty doesn't work */
					JoinExpr *n = makeNode(JoinExpr);
					n->jointype = JOIN_INNER;
					n->isNatural = TRUE;
					n->larg = yyvsp[-3].node;
					n->rarg = yyvsp[0].node;
					n->using = NIL; /* figure out which columns later... */
					n->quals = NULL; /* fill later */
					yyval.jexpr = n;
				}
    break;

  case 788:
#line 4889 "gram.y"
    {
					yyval.alias = makeNode(Alias);
					yyval.alias->aliasname = yyvsp[-3].str;
					yyval.alias->colnames = yyvsp[-1].list;
				}
    break;

  case 789:
#line 4895 "gram.y"
    {
					yyval.alias = makeNode(Alias);
					yyval.alias->aliasname = yyvsp[0].str;
				}
    break;

  case 790:
#line 4900 "gram.y"
    {
					yyval.alias = makeNode(Alias);
					yyval.alias->aliasname = yyvsp[-3].str;
					yyval.alias->colnames = yyvsp[-1].list;
				}
    break;

  case 791:
#line 4906 "gram.y"
    {
					yyval.alias = makeNode(Alias);
					yyval.alias->aliasname = yyvsp[0].str;
				}
    break;

  case 792:
#line 4912 "gram.y"
    { yyval.jtype = JOIN_FULL; }
    break;

  case 793:
#line 4913 "gram.y"
    { yyval.jtype = JOIN_LEFT; }
    break;

  case 794:
#line 4914 "gram.y"
    { yyval.jtype = JOIN_RIGHT; }
    break;

  case 795:
#line 4915 "gram.y"
    { yyval.jtype = JOIN_INNER; }
    break;

  case 796:
#line 4919 "gram.y"
    { yyval.node = NULL; }
    break;

  case 797:
#line 4920 "gram.y"
    { yyval.node = NULL; }
    break;

  case 798:
#line 4932 "gram.y"
    { yyval.node = (Node *) yyvsp[-1].list; }
    break;

  case 799:
#line 4933 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 800:
#line 4939 "gram.y"
    {
					/* default inheritance */
					yyval.range = yyvsp[0].range;
					yyval.range->inhOpt = INH_DEFAULT;
					yyval.range->alias = NULL;
				}
    break;

  case 801:
#line 4946 "gram.y"
    {
					/* inheritance query */
					yyval.range = yyvsp[-1].range;
					yyval.range->inhOpt = INH_YES;
					yyval.range->alias = NULL;
				}
    break;

  case 802:
#line 4953 "gram.y"
    {
					/* no inheritance */
					yyval.range = yyvsp[0].range;
					yyval.range->inhOpt = INH_NO;
					yyval.range->alias = NULL;
				}
    break;

  case 803:
#line 4960 "gram.y"
    {
					/* no inheritance, SQL99-style syntax */
					yyval.range = yyvsp[-1].range;
					yyval.range->inhOpt = INH_NO;
					yyval.range->alias = NULL;
				}
    break;

  case 804:
#line 4970 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = yyvsp[-2].list;
					n->args = NIL;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 805:
#line 4979 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = yyvsp[-3].list;
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 806:
#line 4991 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 807:
#line 4992 "gram.y"
    { yyval.node = NULL; }
    break;

  case 808:
#line 4998 "gram.y"
    {
					yyval.list = makeList1(yyvsp[0].node);
				}
    break;

  case 809:
#line 5002 "gram.y"
    {
					yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node);
				}
    break;

  case 810:
#line 5008 "gram.y"
    {
					ColumnDef *n = makeNode(ColumnDef);
					n->colname = yyvsp[-1].str;
					n->typename = yyvsp[0].typnam;
					n->constraints = NIL;
					n->is_local = true;
					yyval.node = (Node *)n;
				}
    break;

  case 811:
#line 5029 "gram.y"
    {
					yyval.typnam = yyvsp[-1].typnam;
					yyval.typnam->arrayBounds = yyvsp[0].list;
				}
    break;

  case 812:
#line 5034 "gram.y"
    {
					yyval.typnam = yyvsp[-1].typnam;
					yyval.typnam->arrayBounds = yyvsp[0].list;
					yyval.typnam->setof = TRUE;
				}
    break;

  case 813:
#line 5040 "gram.y"
    {
					/* SQL99's redundant syntax */
					yyval.typnam = yyvsp[-4].typnam;
					yyval.typnam->arrayBounds = makeList1(makeInteger(yyvsp[-1].ival));
				}
    break;

  case 814:
#line 5046 "gram.y"
    {
					/* SQL99's redundant syntax */
					yyval.typnam = yyvsp[-4].typnam;
					yyval.typnam->arrayBounds = makeList1(makeInteger(yyvsp[-1].ival));
					yyval.typnam->setof = TRUE;
				}
    break;

  case 815:
#line 5056 "gram.y"
    {  yyval.list = lappend(yyvsp[-2].list, makeInteger(-1)); }
    break;

  case 816:
#line 5058 "gram.y"
    {  yyval.list = lappend(yyvsp[-3].list, makeInteger(yyvsp[-1].ival)); }
    break;

  case 817:
#line 5060 "gram.y"
    {  yyval.list = NIL; }
    break;

  case 818:
#line 5072 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 819:
#line 5073 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 820:
#line 5074 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 821:
#line 5075 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 822:
#line 5076 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 823:
#line 5078 "gram.y"
    {
					yyval.typnam = yyvsp[-1].typnam;
					if (yyvsp[0].ival != INTERVAL_FULL_RANGE)
						yyval.typnam->typmod = INTERVAL_TYPMOD(INTERVAL_FULL_PRECISION, yyvsp[0].ival);
				}
    break;

  case 824:
#line 5084 "gram.y"
    {
					yyval.typnam = yyvsp[-4].typnam;
					if (yyvsp[-2].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("INTERVAL(%d) precision must not be negative",
										yyvsp[-2].ival)));
					if (yyvsp[-2].ival > MAX_INTERVAL_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("INTERVAL(%d) precision reduced to maximum allowed, %d",
										yyvsp[-2].ival, MAX_INTERVAL_PRECISION)));
						yyvsp[-2].ival = MAX_INTERVAL_PRECISION;
					}
					yyval.typnam->typmod = INTERVAL_TYPMOD(yyvsp[-2].ival, yyvsp[0].ival);
				}
    break;

  case 825:
#line 5102 "gram.y"
    {
					yyval.typnam = makeNode(TypeName);
					yyval.typnam->names = lcons(makeString(yyvsp[-1].str), yyvsp[0].list);
					yyval.typnam->typmod = -1;
				}
    break;

  case 826:
#line 5119 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 827:
#line 5120 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 828:
#line 5121 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 829:
#line 5122 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 830:
#line 5123 "gram.y"
    { yyval.typnam = yyvsp[0].typnam; }
    break;

  case 831:
#line 5128 "gram.y"
    {
					yyval.typnam = makeTypeName(yyvsp[0].str);
				}
    break;

  case 832:
#line 5139 "gram.y"
    {
					yyval.typnam = SystemTypeName("int4");
				}
    break;

  case 833:
#line 5143 "gram.y"
    {
					yyval.typnam = SystemTypeName("int4");
				}
    break;

  case 834:
#line 5147 "gram.y"
    {
					yyval.typnam = SystemTypeName("int2");
				}
    break;

  case 835:
#line 5151 "gram.y"
    {
					yyval.typnam = SystemTypeName("int8");
				}
    break;

  case 836:
#line 5155 "gram.y"
    {
					yyval.typnam = SystemTypeName("float4");
				}
    break;

  case 837:
#line 5159 "gram.y"
    {
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 838:
#line 5163 "gram.y"
    {
					yyval.typnam = SystemTypeName("float8");
				}
    break;

  case 839:
#line 5167 "gram.y"
    {
					yyval.typnam = SystemTypeName("numeric");
					yyval.typnam->typmod = yyvsp[0].ival;
				}
    break;

  case 840:
#line 5172 "gram.y"
    {
					yyval.typnam = SystemTypeName("numeric");
					yyval.typnam->typmod = yyvsp[0].ival;
				}
    break;

  case 841:
#line 5177 "gram.y"
    {
					yyval.typnam = SystemTypeName("numeric");
					yyval.typnam->typmod = yyvsp[0].ival;
				}
    break;

  case 842:
#line 5182 "gram.y"
    {
					yyval.typnam = SystemTypeName("bool");
				}
    break;

  case 843:
#line 5188 "gram.y"
    {
					if (yyvsp[-1].ival < 1)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("precision for FLOAT must be at least 1 bit")));
					else if (yyvsp[-1].ival <= 24)
						yyval.typnam = SystemTypeName("float4");
					else if (yyvsp[-1].ival <= 53)
						yyval.typnam = SystemTypeName("float8");
					else
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("precision for FLOAT must be less than 54 bits")));
				}
    break;

  case 844:
#line 5203 "gram.y"
    {
					yyval.typnam = SystemTypeName("float8");
				}
    break;

  case 845:
#line 5210 "gram.y"
    {
					if (yyvsp[-3].ival < 1 || yyvsp[-3].ival > NUMERIC_MAX_PRECISION)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("NUMERIC precision %d must be between 1 and %d",
										yyvsp[-3].ival, NUMERIC_MAX_PRECISION)));
					if (yyvsp[-1].ival < 0 || yyvsp[-1].ival > yyvsp[-3].ival)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("NUMERIC scale %d must be between 0 and precision %d",
										yyvsp[-1].ival, yyvsp[-3].ival)));

					yyval.ival = ((yyvsp[-3].ival << 16) | yyvsp[-1].ival) + VARHDRSZ;
				}
    break;

  case 846:
#line 5225 "gram.y"
    {
					if (yyvsp[-1].ival < 1 || yyvsp[-1].ival > NUMERIC_MAX_PRECISION)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("NUMERIC precision %d must be between 1 and %d",
										yyvsp[-1].ival, NUMERIC_MAX_PRECISION)));

					yyval.ival = (yyvsp[-1].ival << 16) + VARHDRSZ;
				}
    break;

  case 847:
#line 5235 "gram.y"
    {
					/* Insert "-1" meaning "no limit" */
					yyval.ival = -1;
				}
    break;

  case 848:
#line 5243 "gram.y"
    {
					if (yyvsp[-3].ival < 1 || yyvsp[-3].ival > NUMERIC_MAX_PRECISION)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("DECIMAL precision %d must be between 1 and %d",
										yyvsp[-3].ival, NUMERIC_MAX_PRECISION)));
					if (yyvsp[-1].ival < 0 || yyvsp[-1].ival > yyvsp[-3].ival)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("DECIMAL scale %d must be between 0 and precision %d",
										yyvsp[-1].ival, yyvsp[-3].ival)));

					yyval.ival = ((yyvsp[-3].ival << 16) | yyvsp[-1].ival) + VARHDRSZ;
				}
    break;

  case 849:
#line 5258 "gram.y"
    {
					if (yyvsp[-1].ival < 1 || yyvsp[-1].ival > NUMERIC_MAX_PRECISION)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("DECIMAL precision %d must be between 1 and %d",
										yyvsp[-1].ival, NUMERIC_MAX_PRECISION)));

					yyval.ival = (yyvsp[-1].ival << 16) + VARHDRSZ;
				}
    break;

  case 850:
#line 5268 "gram.y"
    {
					/* Insert "-1" meaning "no limit" */
					yyval.ival = -1;
				}
    break;

  case 851:
#line 5280 "gram.y"
    {
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 852:
#line 5284 "gram.y"
    {
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 853:
#line 5292 "gram.y"
    {
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 854:
#line 5296 "gram.y"
    {
					yyval.typnam->typmod = -1;
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 855:
#line 5304 "gram.y"
    {
					char *typname;

					typname = yyvsp[-3].boolean ? "varbit" : "bit";
					yyval.typnam = SystemTypeName(typname);
					if (yyvsp[-1].ival < 1)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("length for type %s must be at least 1",
										typname)));
					else if (yyvsp[-1].ival > (MaxAttrSize * BITS_PER_BYTE))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("length for type %s cannot exceed %d",
										typname, MaxAttrSize * BITS_PER_BYTE)));
					yyval.typnam->typmod = yyvsp[-1].ival;
				}
    break;

  case 856:
#line 5325 "gram.y"
    {
					/* bit defaults to bit(1), varbit to no limit */
					if (yyvsp[0].boolean)
					{
						yyval.typnam = SystemTypeName("varbit");
						yyval.typnam->typmod = -1;
					}
					else
					{
						yyval.typnam = SystemTypeName("bit");
						yyval.typnam->typmod = 1;
					}
				}
    break;

  case 857:
#line 5346 "gram.y"
    {
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 858:
#line 5350 "gram.y"
    {
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 859:
#line 5356 "gram.y"
    {
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 860:
#line 5360 "gram.y"
    {
					/* Length was not specified so allow to be unrestricted.
					 * This handles problems with fixed-length (bpchar) strings
					 * which in column definitions must default to a length
					 * of one, but should not be constrained if the length
					 * was not specified.
					 */
					yyvsp[0].typnam->typmod = -1;
					yyval.typnam = yyvsp[0].typnam;
				}
    break;

  case 861:
#line 5373 "gram.y"
    {
					if ((yyvsp[0].str != NULL) && (strcmp(yyvsp[0].str, "sql_text") != 0))
					{
						char *type;

						type = palloc(strlen(yyvsp[-4].str) + 1 + strlen(yyvsp[0].str) + 1);
						strcpy(type, yyvsp[-4].str);
						strcat(type, "_");
						strcat(type, yyvsp[0].str);
						yyvsp[-4].str = type;
					}

					yyval.typnam = SystemTypeName(yyvsp[-4].str);

					if (yyvsp[-2].ival < 1)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("length for type %s must be at least 1",
										yyvsp[-4].str)));
					else if (yyvsp[-2].ival > MaxAttrSize)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("length for type %s cannot exceed %d",
										yyvsp[-4].str, MaxAttrSize)));

					/* we actually implement these like a varlen, so
					 * the first 4 bytes is the length. (the difference
					 * between these and "text" is that we blank-pad and
					 * truncate where necessary)
					 */
					yyval.typnam->typmod = VARHDRSZ + yyvsp[-2].ival;
				}
    break;

  case 862:
#line 5408 "gram.y"
    {
					if ((yyvsp[0].str != NULL) && (strcmp(yyvsp[0].str, "sql_text") != 0))
					{
						char *type;

						type = palloc(strlen(yyvsp[-1].str) + 1 + strlen(yyvsp[0].str) + 1);
						strcpy(type, yyvsp[-1].str);
						strcat(type, "_");
						strcat(type, yyvsp[0].str);
						yyvsp[-1].str = type;
					}

					yyval.typnam = SystemTypeName(yyvsp[-1].str);

					/* char defaults to char(1), varchar to no limit */
					if (strcmp(yyvsp[-1].str, "bpchar") == 0)
						yyval.typnam->typmod = VARHDRSZ + 1;
					else
						yyval.typnam->typmod = -1;
				}
    break;

  case 863:
#line 5431 "gram.y"
    { yyval.str = yyvsp[0].boolean ? "varchar": "bpchar"; }
    break;

  case 864:
#line 5433 "gram.y"
    { yyval.str = yyvsp[0].boolean ? "varchar": "bpchar"; }
    break;

  case 865:
#line 5435 "gram.y"
    { yyval.str = "varchar"; }
    break;

  case 866:
#line 5437 "gram.y"
    { yyval.str = yyvsp[0].boolean ? "varchar": "bpchar"; }
    break;

  case 867:
#line 5439 "gram.y"
    { yyval.str = yyvsp[0].boolean ? "varchar": "bpchar"; }
    break;

  case 868:
#line 5441 "gram.y"
    { yyval.str = yyvsp[0].boolean ? "varchar": "bpchar"; }
    break;

  case 869:
#line 5445 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 870:
#line 5446 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 871:
#line 5450 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 872:
#line 5451 "gram.y"
    { yyval.str = NULL; }
    break;

  case 873:
#line 5456 "gram.y"
    {
					if (yyvsp[0].boolean)
						yyval.typnam = SystemTypeName("timestamptz");
					else
						yyval.typnam = SystemTypeName("timestamp");
					/* XXX the timezone field seems to be unused
					 * - thomas 2001-09-06
					 */
					yyval.typnam->timezone = yyvsp[0].boolean;
					if (yyvsp[-2].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("TIMESTAMP(%d)%s precision must not be negative",
										yyvsp[-2].ival, (yyvsp[0].boolean ? " WITH TIME ZONE": ""))));
					if (yyvsp[-2].ival > MAX_TIMESTAMP_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("TIMESTAMP(%d)%s precision reduced to maximum allowed, %d",
										yyvsp[-2].ival, (yyvsp[0].boolean ? " WITH TIME ZONE": ""),
										MAX_TIMESTAMP_PRECISION)));
						yyvsp[-2].ival = MAX_TIMESTAMP_PRECISION;
					}
					yyval.typnam->typmod = yyvsp[-2].ival;
				}
    break;

  case 874:
#line 5482 "gram.y"
    {
					if (yyvsp[0].boolean)
						yyval.typnam = SystemTypeName("timestamptz");
					else
						yyval.typnam = SystemTypeName("timestamp");
					/* XXX the timezone field seems to be unused
					 * - thomas 2001-09-06
					 */
					yyval.typnam->timezone = yyvsp[0].boolean;
					/* SQL99 specified a default precision of six
					 * for schema definitions. But for timestamp
					 * literals we don't want to throw away precision
					 * so leave this as unspecified for now.
					 * Later, we may want a different production
					 * for schemas. - thomas 2001-12-07
					 */
					yyval.typnam->typmod = -1;
				}
    break;

  case 875:
#line 5501 "gram.y"
    {
					if (yyvsp[0].boolean)
						yyval.typnam = SystemTypeName("timetz");
					else
						yyval.typnam = SystemTypeName("time");
					if (yyvsp[-2].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("TIME(%d)%s precision must not be negative",
										yyvsp[-2].ival, (yyvsp[0].boolean ? " WITH TIME ZONE": ""))));
					if (yyvsp[-2].ival > MAX_TIME_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("TIME(%d)%s precision reduced to maximum allowed, %d",
										yyvsp[-2].ival, (yyvsp[0].boolean ? " WITH TIME ZONE": ""),
										MAX_TIME_PRECISION)));
						yyvsp[-2].ival = MAX_TIME_PRECISION;
					}
					yyval.typnam->typmod = yyvsp[-2].ival;
				}
    break;

  case 876:
#line 5523 "gram.y"
    {
					if (yyvsp[0].boolean)
						yyval.typnam = SystemTypeName("timetz");
					else
						yyval.typnam = SystemTypeName("time");
					/* SQL99 specified a default precision of zero.
					 * See comments for timestamp above on why we will
					 * leave this unspecified for now. - thomas 2001-12-07
					 */
					yyval.typnam->typmod = -1;
				}
    break;

  case 877:
#line 5537 "gram.y"
    { yyval.typnam = SystemTypeName("interval"); }
    break;

  case 878:
#line 5541 "gram.y"
    { yyval.boolean = TRUE; }
    break;

  case 879:
#line 5542 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 880:
#line 5543 "gram.y"
    { yyval.boolean = FALSE; }
    break;

  case 881:
#line 5547 "gram.y"
    { yyval.ival = INTERVAL_MASK(YEAR); }
    break;

  case 882:
#line 5548 "gram.y"
    { yyval.ival = INTERVAL_MASK(MONTH); }
    break;

  case 883:
#line 5549 "gram.y"
    { yyval.ival = INTERVAL_MASK(DAY); }
    break;

  case 884:
#line 5550 "gram.y"
    { yyval.ival = INTERVAL_MASK(HOUR); }
    break;

  case 885:
#line 5551 "gram.y"
    { yyval.ival = INTERVAL_MASK(MINUTE); }
    break;

  case 886:
#line 5552 "gram.y"
    { yyval.ival = INTERVAL_MASK(SECOND); }
    break;

  case 887:
#line 5554 "gram.y"
    { yyval.ival = INTERVAL_MASK(YEAR) | INTERVAL_MASK(MONTH); }
    break;

  case 888:
#line 5556 "gram.y"
    { yyval.ival = INTERVAL_MASK(DAY) | INTERVAL_MASK(HOUR); }
    break;

  case 889:
#line 5558 "gram.y"
    { yyval.ival = INTERVAL_MASK(DAY) | INTERVAL_MASK(HOUR)
						| INTERVAL_MASK(MINUTE); }
    break;

  case 890:
#line 5561 "gram.y"
    { yyval.ival = INTERVAL_MASK(DAY) | INTERVAL_MASK(HOUR)
						| INTERVAL_MASK(MINUTE) | INTERVAL_MASK(SECOND); }
    break;

  case 891:
#line 5564 "gram.y"
    { yyval.ival = INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE); }
    break;

  case 892:
#line 5566 "gram.y"
    { yyval.ival = INTERVAL_MASK(HOUR) | INTERVAL_MASK(MINUTE)
						| INTERVAL_MASK(SECOND); }
    break;

  case 893:
#line 5569 "gram.y"
    { yyval.ival = INTERVAL_MASK(MINUTE) | INTERVAL_MASK(SECOND); }
    break;

  case 894:
#line 5570 "gram.y"
    { yyval.ival = INTERVAL_FULL_RANGE; }
    break;

  case 895:
#line 5586 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subLinkType = ANY_SUBLINK;
					n->lefthand = yyvsp[-2].list;
					n->operName = makeList1(makeString("="));
					n->subselect = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 896:
#line 5595 "gram.y"
    {
					/* Make an IN node */
					SubLink *n = makeNode(SubLink);
					n->subLinkType = ANY_SUBLINK;
					n->lefthand = yyvsp[-3].list;
					n->operName = makeList1(makeString("="));
					n->subselect = yyvsp[0].node;
					/* Stick a NOT on top */
					yyval.node = (Node *) makeA_Expr(AEXPR_NOT, NIL, NULL, (Node *) n);
				}
    break;

  case 897:
#line 5607 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subLinkType = yyvsp[-1].ival;
					n->lefthand = yyvsp[-3].list;
					n->operName = yyvsp[-2].list;
					n->subselect = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 898:
#line 5617 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subLinkType = MULTIEXPR_SUBLINK;
					n->lefthand = yyvsp[-2].list;
					n->operName = yyvsp[-1].list;
					n->subselect = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 899:
#line 5627 "gram.y"
    {
					yyval.node = makeRowExpr(yyvsp[-1].list, yyvsp[-2].list, yyvsp[0].list);
				}
    break;

  case 900:
#line 5631 "gram.y"
    {
					yyval.node = makeRowNullTest(IS_NULL, yyvsp[-2].list);
				}
    break;

  case 901:
#line 5635 "gram.y"
    {
					yyval.node = makeRowNullTest(IS_NOT_NULL, yyvsp[-3].list);
				}
    break;

  case 902:
#line 5639 "gram.y"
    {
					yyval.node = (Node *)makeOverlaps(yyvsp[-2].list, yyvsp[0].list);
				}
    break;

  case 903:
#line 5644 "gram.y"
    {
					/* IS DISTINCT FROM has the following rules for non-array types:
					 * a) the row lengths must be equal
					 * b) if both rows are zero-length, then they are not distinct
					 * c) if any element is distinct, the rows are distinct
					 * The rules for an element being distinct:
					 * a) if the elements are both NULL, then they are not distinct
					 * b) if the elements compare to be equal, then they are not distinct
					 * c) otherwise, they are distinct
					 */
					List *largs = yyvsp[-4].list;
					List *rargs = yyvsp[0].list;
					/* lengths don't match? then complain */
					if (length(largs) != length(rargs))
					{
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("unequal number of entries in row expression")));
					}
					/* both are zero-length rows? then they are not distinct */
					else if (length(largs) <= 0)
					{
						yyval.node = (Node *)makeBoolConst(FALSE);
					}
					/* otherwise, we need to compare each element */
					else
					{
						yyval.node = (Node *)makeDistinctExpr(largs, rargs);
					}
				}
    break;

  case 904:
#line 5680 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 905:
#line 5681 "gram.y"
    { yyval.list = makeList1(yyvsp[-1].node); }
    break;

  case 906:
#line 5682 "gram.y"
    { yyval.list = NULL; }
    break;

  case 907:
#line 5683 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 908:
#line 5686 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node); }
    break;

  case 909:
#line 5689 "gram.y"
    { yyval.ival = ANY_SUBLINK; }
    break;

  case 910:
#line 5690 "gram.y"
    { yyval.ival = ANY_SUBLINK; }
    break;

  case 911:
#line 5691 "gram.y"
    { yyval.ival = ALL_SUBLINK; }
    break;

  case 912:
#line 5694 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 913:
#line 5695 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 914:
#line 5698 "gram.y"
    { yyval.str = "+"; }
    break;

  case 915:
#line 5699 "gram.y"
    { yyval.str = "-"; }
    break;

  case 916:
#line 5700 "gram.y"
    { yyval.str = "*"; }
    break;

  case 917:
#line 5701 "gram.y"
    { yyval.str = "/"; }
    break;

  case 918:
#line 5702 "gram.y"
    { yyval.str = "%"; }
    break;

  case 919:
#line 5703 "gram.y"
    { yyval.str = "^"; }
    break;

  case 920:
#line 5704 "gram.y"
    { yyval.str = "<"; }
    break;

  case 921:
#line 5705 "gram.y"
    { yyval.str = ">"; }
    break;

  case 922:
#line 5706 "gram.y"
    { yyval.str = "="; }
    break;

  case 923:
#line 5710 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 924:
#line 5711 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 925:
#line 5716 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 926:
#line 5717 "gram.y"
    { yyval.list = yyvsp[-1].list; }
    break;

  case 927:
#line 5736 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 928:
#line 5738 "gram.y"
    { yyval.node = makeTypeCast(yyvsp[-2].node, yyvsp[0].typnam); }
    break;

  case 929:
#line 5740 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("timezone");
					n->args = makeList2(yyvsp[0].node, yyvsp[-4].node);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) n;
				}
    break;

  case 930:
#line 5758 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "+", NULL, yyvsp[0].node); }
    break;

  case 931:
#line 5760 "gram.y"
    { yyval.node = doNegate(yyvsp[0].node); }
    break;

  case 932:
#line 5762 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "%", NULL, yyvsp[0].node); }
    break;

  case 933:
#line 5764 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "^", NULL, yyvsp[0].node); }
    break;

  case 934:
#line 5766 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "%", yyvsp[-1].node, NULL); }
    break;

  case 935:
#line 5768 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "^", yyvsp[-1].node, NULL); }
    break;

  case 936:
#line 5770 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "+", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 937:
#line 5772 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "-", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 938:
#line 5774 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "*", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 939:
#line 5776 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "/", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 940:
#line 5778 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "%", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 941:
#line 5780 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "^", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 942:
#line 5782 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "<", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 943:
#line 5784 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, ">", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 944:
#line 5786 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "=", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 945:
#line 5789 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_OP, yyvsp[-1].list, yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 946:
#line 5791 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_OP, yyvsp[-1].list, NULL, yyvsp[0].node); }
    break;

  case 947:
#line 5793 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_OP, yyvsp[0].list, yyvsp[-1].node, NULL); }
    break;

  case 948:
#line 5796 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_AND, NIL, yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 949:
#line 5798 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_OR, NIL, yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 950:
#line 5800 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_NOT, NIL, NULL, yyvsp[0].node); }
    break;

  case 951:
#line 5803 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "~~", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 952:
#line 5805 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("like_escape");
					n->args = makeList2(yyvsp[-2].node, yyvsp[0].node);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "~~", yyvsp[-4].node, (Node *) n);
				}
    break;

  case 953:
#line 5814 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "!~~", yyvsp[-3].node, yyvsp[0].node); }
    break;

  case 954:
#line 5816 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("like_escape");
					n->args = makeList2(yyvsp[-2].node, yyvsp[0].node);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "!~~", yyvsp[-5].node, (Node *) n);
				}
    break;

  case 955:
#line 5825 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "~~*", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 956:
#line 5827 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("like_escape");
					n->args = makeList2(yyvsp[-2].node, yyvsp[0].node);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "~~*", yyvsp[-4].node, (Node *) n);
				}
    break;

  case 957:
#line 5836 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "!~~*", yyvsp[-3].node, yyvsp[0].node); }
    break;

  case 958:
#line 5838 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("like_escape");
					n->args = makeList2(yyvsp[-2].node, yyvsp[0].node);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "!~~*", yyvsp[-5].node, (Node *) n);
				}
    break;

  case 959:
#line 5848 "gram.y"
    {
					A_Const *c = makeNode(A_Const);
					FuncCall *n = makeNode(FuncCall);
					c->val.type = T_Null;
					n->funcname = SystemFuncName("similar_escape");
					n->args = makeList2(yyvsp[0].node, (Node *) c);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "~", yyvsp[-3].node, (Node *) n);
				}
    break;

  case 960:
#line 5859 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("similar_escape");
					n->args = makeList2(yyvsp[-2].node, yyvsp[0].node);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "~", yyvsp[-5].node, (Node *) n);
				}
    break;

  case 961:
#line 5868 "gram.y"
    {
					A_Const *c = makeNode(A_Const);
					FuncCall *n = makeNode(FuncCall);
					c->val.type = T_Null;
					n->funcname = SystemFuncName("similar_escape");
					n->args = makeList2(yyvsp[0].node, (Node *) c);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "!~", yyvsp[-4].node, (Node *) n);
				}
    break;

  case 962:
#line 5879 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("similar_escape");
					n->args = makeList2(yyvsp[-2].node, yyvsp[0].node);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "!~", yyvsp[-6].node, (Node *) n);
				}
    break;

  case 963:
#line 5898 "gram.y"
    {
					NullTest *n = makeNode(NullTest);
					n->arg = (Expr *) yyvsp[-1].node;
					n->nulltesttype = IS_NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 964:
#line 5905 "gram.y"
    {
					NullTest *n = makeNode(NullTest);
					n->arg = (Expr *) yyvsp[-2].node;
					n->nulltesttype = IS_NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 965:
#line 5912 "gram.y"
    {
					NullTest *n = makeNode(NullTest);
					n->arg = (Expr *) yyvsp[-1].node;
					n->nulltesttype = IS_NOT_NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 966:
#line 5919 "gram.y"
    {
					NullTest *n = makeNode(NullTest);
					n->arg = (Expr *) yyvsp[-3].node;
					n->nulltesttype = IS_NOT_NULL;
					yyval.node = (Node *)n;
				}
    break;

  case 967:
#line 5926 "gram.y"
    {
					BooleanTest *b = makeNode(BooleanTest);
					b->arg = (Expr *) yyvsp[-2].node;
					b->booltesttype = IS_TRUE;
					yyval.node = (Node *)b;
				}
    break;

  case 968:
#line 5933 "gram.y"
    {
					BooleanTest *b = makeNode(BooleanTest);
					b->arg = (Expr *) yyvsp[-3].node;
					b->booltesttype = IS_NOT_TRUE;
					yyval.node = (Node *)b;
				}
    break;

  case 969:
#line 5940 "gram.y"
    {
					BooleanTest *b = makeNode(BooleanTest);
					b->arg = (Expr *) yyvsp[-2].node;
					b->booltesttype = IS_FALSE;
					yyval.node = (Node *)b;
				}
    break;

  case 970:
#line 5947 "gram.y"
    {
					BooleanTest *b = makeNode(BooleanTest);
					b->arg = (Expr *) yyvsp[-3].node;
					b->booltesttype = IS_NOT_FALSE;
					yyval.node = (Node *)b;
				}
    break;

  case 971:
#line 5954 "gram.y"
    {
					BooleanTest *b = makeNode(BooleanTest);
					b->arg = (Expr *) yyvsp[-2].node;
					b->booltesttype = IS_UNKNOWN;
					yyval.node = (Node *)b;
				}
    break;

  case 972:
#line 5961 "gram.y"
    {
					BooleanTest *b = makeNode(BooleanTest);
					b->arg = (Expr *) yyvsp[-3].node;
					b->booltesttype = IS_NOT_UNKNOWN;
					yyval.node = (Node *)b;
				}
    break;

  case 973:
#line 5968 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_DISTINCT, "=", yyvsp[-4].node, yyvsp[0].node); }
    break;

  case 974:
#line 5970 "gram.y"
    {
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OF, "=", yyvsp[-5].node, (Node *) yyvsp[-1].list);
				}
    break;

  case 975:
#line 5974 "gram.y"
    {
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OF, "!=", yyvsp[-6].node, (Node *) yyvsp[-1].list);
				}
    break;

  case 976:
#line 5978 "gram.y"
    {
					yyval.node = (Node *) makeA_Expr(AEXPR_AND, NIL,
						(Node *) makeSimpleA_Expr(AEXPR_OP, ">=", yyvsp[-4].node, yyvsp[-2].node),
						(Node *) makeSimpleA_Expr(AEXPR_OP, "<=", yyvsp[-4].node, yyvsp[0].node));
				}
    break;

  case 977:
#line 5984 "gram.y"
    {
					yyval.node = (Node *) makeA_Expr(AEXPR_OR, NIL,
						(Node *) makeSimpleA_Expr(AEXPR_OP, "<", yyvsp[-5].node, yyvsp[-2].node),
						(Node *) makeSimpleA_Expr(AEXPR_OP, ">", yyvsp[-5].node, yyvsp[0].node));
				}
    break;

  case 978:
#line 5990 "gram.y"
    {
					/* in_expr returns a SubLink or a list of a_exprs */
					if (IsA(yyvsp[0].node, SubLink))
					{
							SubLink *n = (SubLink *)yyvsp[0].node;
							n->subLinkType = ANY_SUBLINK;
							n->lefthand = makeList1(yyvsp[-2].node);
							n->operName = makeList1(makeString("="));
							yyval.node = (Node *)n;
					}
					else
					{
						Node *n = NULL;
						List *l;
						foreach(l, (List *) yyvsp[0].node)
						{
							Node *cmp;
							cmp = (Node *) makeSimpleA_Expr(AEXPR_OP, "=", yyvsp[-2].node, lfirst(l));
							if (n == NULL)
								n = cmp;
							else
								n = (Node *) makeA_Expr(AEXPR_OR, NIL, n, cmp);
						}
						yyval.node = n;
					}
				}
    break;

  case 979:
#line 6017 "gram.y"
    {
					/* in_expr returns a SubLink or a list of a_exprs */
					if (IsA(yyvsp[0].node, SubLink))
					{
						/* Make an IN node */
						SubLink *n = (SubLink *)yyvsp[0].node;
						n->subLinkType = ANY_SUBLINK;
						n->lefthand = makeList1(yyvsp[-3].node);
						n->operName = makeList1(makeString("="));
						/* Stick a NOT on top */
						yyval.node = (Node *) makeA_Expr(AEXPR_NOT, NIL, NULL, (Node *) n);
					}
					else
					{
						Node *n = NULL;
						List *l;
						foreach(l, (List *) yyvsp[0].node)
						{
							Node *cmp;
							cmp = (Node *) makeSimpleA_Expr(AEXPR_OP, "<>", yyvsp[-3].node, lfirst(l));
							if (n == NULL)
								n = cmp;
							else
								n = (Node *) makeA_Expr(AEXPR_AND, NIL, n, cmp);
						}
						yyval.node = n;
					}
				}
    break;

  case 980:
#line 6046 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subLinkType = yyvsp[-1].ival;
					n->lefthand = makeList1(yyvsp[-3].node);
					n->operName = yyvsp[-2].list;
					n->subselect = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 981:
#line 6055 "gram.y"
    {
					if (yyvsp[-3].ival == ANY_SUBLINK)
						yyval.node = (Node *) makeA_Expr(AEXPR_OP_ANY, yyvsp[-4].list, yyvsp[-5].node, yyvsp[-1].node);
					else
						yyval.node = (Node *) makeA_Expr(AEXPR_OP_ALL, yyvsp[-4].list, yyvsp[-5].node, yyvsp[-1].node);
				}
    break;

  case 982:
#line 6062 "gram.y"
    {
					/* Not sure how to get rid of the parentheses
					 * but there are lots of shift/reduce errors without them.
					 *
					 * Should be able to implement this by plopping the entire
					 * select into a node, then transforming the target expressions
					 * from whatever they are into count(*), and testing the
					 * entire result equal to one.
					 * But, will probably implement a separate node in the executor.
					 */
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("UNIQUE predicate is not yet implemented")));
				}
    break;

  case 983:
#line 6077 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 984:
#line 6090 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 985:
#line 6092 "gram.y"
    { yyval.node = makeTypeCast(yyvsp[-2].node, yyvsp[0].typnam); }
    break;

  case 986:
#line 6094 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "+", NULL, yyvsp[0].node); }
    break;

  case 987:
#line 6096 "gram.y"
    { yyval.node = doNegate(yyvsp[0].node); }
    break;

  case 988:
#line 6098 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "%", NULL, yyvsp[0].node); }
    break;

  case 989:
#line 6100 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "^", NULL, yyvsp[0].node); }
    break;

  case 990:
#line 6102 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "%", yyvsp[-1].node, NULL); }
    break;

  case 991:
#line 6104 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "^", yyvsp[-1].node, NULL); }
    break;

  case 992:
#line 6106 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "+", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 993:
#line 6108 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "-", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 994:
#line 6110 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "*", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 995:
#line 6112 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "/", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 996:
#line 6114 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "%", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 997:
#line 6116 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "^", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 998:
#line 6118 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "<", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 999:
#line 6120 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, ">", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 1000:
#line 6122 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OP, "=", yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 1001:
#line 6124 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_OP, yyvsp[-1].list, yyvsp[-2].node, yyvsp[0].node); }
    break;

  case 1002:
#line 6126 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_OP, yyvsp[-1].list, NULL, yyvsp[0].node); }
    break;

  case 1003:
#line 6128 "gram.y"
    { yyval.node = (Node *) makeA_Expr(AEXPR_OP, yyvsp[0].list, yyvsp[-1].node, NULL); }
    break;

  case 1004:
#line 6130 "gram.y"
    { yyval.node = (Node *) makeSimpleA_Expr(AEXPR_DISTINCT, "=", yyvsp[-4].node, yyvsp[0].node); }
    break;

  case 1005:
#line 6132 "gram.y"
    {
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OF, "=", yyvsp[-5].node, (Node *) yyvsp[-1].list);
				}
    break;

  case 1006:
#line 6136 "gram.y"
    {
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_OF, "!=", yyvsp[-6].node, (Node *) yyvsp[-1].list);
				}
    break;

  case 1007:
#line 6149 "gram.y"
    { yyval.node = (Node *) yyvsp[0].columnref; }
    break;

  case 1008:
#line 6150 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 1009:
#line 6152 "gram.y"
    {
					/*
					 * PARAM without field names is considered a constant,
					 * but with 'em, it is not.  Not very consistent ...
					 */
					ParamRef *n = makeNode(ParamRef);
					n->number = yyvsp[-2].ival;
					n->fields = yyvsp[-1].list;
					n->indirection = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 1010:
#line 6164 "gram.y"
    {
					ExprFieldSelect *n = makeNode(ExprFieldSelect);
					n->arg = yyvsp[-3].node;
					n->fields = yyvsp[-1].list;
					n->indirection = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 1011:
#line 6172 "gram.y"
    {
					if (yyvsp[0].list)
					{
						ExprFieldSelect *n = makeNode(ExprFieldSelect);
						n->arg = yyvsp[-2].node;
						n->fields = NIL;
						n->indirection = yyvsp[0].list;
						yyval.node = (Node *)n;
					}
					else
						yyval.node = yyvsp[-2].node;
				}
    break;

  case 1012:
#line 6185 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 1013:
#line 6187 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = yyvsp[-2].list;
					n->args = NIL;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1014:
#line 6196 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = yyvsp[-3].list;
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1015:
#line 6205 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = yyvsp[-4].list;
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					/* Ideally we'd mark the FuncCall node to indicate
					 * "must be an aggregate", but there's no provision
					 * for that in FuncCall at the moment.
					 */
					yyval.node = (Node *)n;
				}
    break;

  case 1016:
#line 6218 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = yyvsp[-4].list;
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = TRUE;
					yyval.node = (Node *)n;
				}
    break;

  case 1017:
#line 6227 "gram.y"
    {
					/*
					 * For now, we transform AGGREGATE(*) into AGGREGATE(1).
					 *
					 * This does the right thing for COUNT(*) (in fact,
					 * any certainly-non-null expression would do for COUNT),
					 * and there are no other aggregates in SQL92 that accept
					 * '*' as parameter.
					 *
					 * The FuncCall node is also marked agg_star = true,
					 * so that later processing can detect what the argument
					 * really was.
					 */
					FuncCall *n = makeNode(FuncCall);
					A_Const *star = makeNode(A_Const);

					star->val.type = T_Integer;
					star->val.val.ival = 1;
					n->funcname = yyvsp[-3].list;
					n->args = makeList1(star);
					n->agg_star = TRUE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1018:
#line 6252 "gram.y"
    {
					/*
					 * Translate as "'now'::text::date".
					 *
					 * We cannot use "'now'::date" because coerce_type() will
					 * immediately reduce that to a constant representing
					 * today's date.  We need to delay the conversion until
					 * runtime, else the wrong things will happen when
					 * CURRENT_DATE is used in a column default value or rule.
					 *
					 * This could be simplified if we had a way to generate
					 * an expression tree representing runtime application
					 * of type-input conversion functions...
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");

					d = SystemTypeName("date");

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1019:
#line 6278 "gram.y"
    {
					/*
					 * Translate as "'now'::text::timetz".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");

					d = SystemTypeName("timetz");
					/* SQL99 mandates a default precision of zero for TIME
					 * fields in schemas. However, for CURRENT_TIME
					 * let's preserve the microsecond precision we
					 * might see from the system clock. - thomas 2001-12-07
					 */
					d->typmod = 6;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1020:
#line 6301 "gram.y"
    {
					/*
					 * Translate as "'now'::text::timetz(n)".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");
					d = SystemTypeName("timetz");
					if (yyvsp[-1].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("CURRENT_TIME(%d) precision must not be negative",
										yyvsp[-1].ival)));
					if (yyvsp[-1].ival > MAX_TIME_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("CURRENT_TIME(%d) precision reduced to maximum allowed, %d",
										yyvsp[-1].ival, MAX_TIME_PRECISION)));
						yyvsp[-1].ival = MAX_TIME_PRECISION;
					}
					d->typmod = yyvsp[-1].ival;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1021:
#line 6331 "gram.y"
    {
					/*
					 * Translate as "'now'::text::timestamptz".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");

					d = SystemTypeName("timestamptz");
					/* SQL99 mandates a default precision of 6 for timestamp.
					 * Also, that is about as precise as we will get since
					 * we are using a microsecond time interface.
					 * - thomas 2001-12-07
					 */
					d->typmod = 6;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1022:
#line 6354 "gram.y"
    {
					/*
					 * Translate as "'now'::text::timestamptz(n)".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");

					d = SystemTypeName("timestamptz");
					if (yyvsp[-1].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("CURRENT_TIMESTAMP(%d) precision must not be negative",
										yyvsp[-1].ival)));
					if (yyvsp[-1].ival > MAX_TIMESTAMP_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("CURRENT_TIMESTAMP(%d) precision reduced to maximum allowed, %d",
										yyvsp[-1].ival, MAX_TIMESTAMP_PRECISION)));
						yyvsp[-1].ival = MAX_TIMESTAMP_PRECISION;
					}
					d->typmod = yyvsp[-1].ival;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1023:
#line 6385 "gram.y"
    {
					/*
					 * Translate as "'now'::text::time".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");

					d = SystemTypeName("time");
					/* SQL99 mandates a default precision of zero for TIME
					 * fields in schemas. However, for LOCALTIME
					 * let's preserve the microsecond precision we
					 * might see from the system clock. - thomas 2001-12-07
					 */
					d->typmod = 6;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1024:
#line 6408 "gram.y"
    {
					/*
					 * Translate as "'now'::text::time(n)".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");
					d = SystemTypeName("time");
					if (yyvsp[-1].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("LOCALTIME(%d) precision must not be negative",
										yyvsp[-1].ival)));
					if (yyvsp[-1].ival > MAX_TIME_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("LOCALTIME(%d) precision reduced to maximum allowed, %d",
										yyvsp[-1].ival, MAX_TIME_PRECISION)));
						yyvsp[-1].ival = MAX_TIME_PRECISION;
					}
					d->typmod = yyvsp[-1].ival;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1025:
#line 6438 "gram.y"
    {
					/*
					 * Translate as "'now'::text::timestamp".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");

					d = SystemTypeName("timestamp");
					/* SQL99 mandates a default precision of 6 for timestamp.
					 * Also, that is about as precise as we will get since
					 * we are using a microsecond time interface.
					 * - thomas 2001-12-07
					 */
					d->typmod = 6;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1026:
#line 6461 "gram.y"
    {
					/*
					 * Translate as "'now'::text::timestamp(n)".
					 * See comments for CURRENT_DATE.
					 */
					A_Const *s = makeNode(A_Const);
					TypeName *d;

					s->val.type = T_String;
					s->val.val.str = "now";
					s->typename = SystemTypeName("text");

					d = SystemTypeName("timestamp");
					if (yyvsp[-1].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("LOCALTIMESTAMP(%d) precision must not be negative",
										yyvsp[-1].ival)));
					if (yyvsp[-1].ival > MAX_TIMESTAMP_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("LOCALTIMESTAMP(%d) precision reduced to maximum allowed, %d",
										yyvsp[-1].ival, MAX_TIMESTAMP_PRECISION)));
						yyvsp[-1].ival = MAX_TIMESTAMP_PRECISION;
					}
					d->typmod = yyvsp[-1].ival;

					yyval.node = (Node *)makeTypeCast((Node *)s, d);
				}
    break;

  case 1027:
#line 6492 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("current_user");
					n->args = NIL;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1028:
#line 6501 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("session_user");
					n->args = NIL;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1029:
#line 6510 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("current_user");
					n->args = NIL;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1030:
#line 6519 "gram.y"
    { yyval.node = makeTypeCast(yyvsp[-3].node, yyvsp[-1].typnam); }
    break;

  case 1031:
#line 6521 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("date_part");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1032:
#line 6530 "gram.y"
    {
					/* overlay(A PLACING B FROM C FOR D) is converted to
					 * substring(A, 1, C-1) || B || substring(A, C+1, C+D)
					 * overlay(A PLACING B FROM C) is converted to
					 * substring(A, 1, C-1) || B || substring(A, C+1, C+char_length(B))
					 */
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("overlay");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1033:
#line 6544 "gram.y"
    {
					/* position(A in B) is converted to position(B, A) */
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("position");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1034:
#line 6554 "gram.y"
    {
					/* substring(A from B for C) is converted to
					 * substring(A, B, C) - thomas 2000-11-28
					 */
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("substring");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1035:
#line 6566 "gram.y"
    {
					/* TREAT(expr AS target) converts expr of a particular type to target,
					 * which is defined to be a subtype of the original expression.
					 * In SQL99, this is intended for use with structured UDTs,
					 * but let's make this a generally useful form allowing stronger
					 * coersions than are handled by implicit casting.
					 */
					FuncCall *n = makeNode(FuncCall);
					/* Convert SystemTypeName() to SystemFuncName() even though
					 * at the moment they result in the same thing.
					 */
					n->funcname = SystemFuncName(((Value *)llast(yyvsp[-1].typnam->names))->val.str);
					n->args = makeList1(yyvsp[-3].node);
					yyval.node = (Node *)n;
				}
    break;

  case 1036:
#line 6582 "gram.y"
    {
					/* various trim expressions are defined in SQL92
					 * - thomas 1997-07-19
					 */
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("btrim");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1037:
#line 6594 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("ltrim");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1038:
#line 6603 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("rtrim");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1039:
#line 6612 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("btrim");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1040:
#line 6621 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					A_Const *c = makeNode(A_Const);

					c->val.type = T_String;
					c->val.val.str = NameListToQuotedString(yyvsp[-1].list);

					n->funcname = SystemFuncName("convert_using");
					n->args = makeList2(yyvsp[-3].node, c);
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1041:
#line 6635 "gram.y"
    {
					FuncCall *n = makeNode(FuncCall);
					n->funcname = SystemFuncName("convert");
					n->args = yyvsp[-1].list;
					n->agg_star = FALSE;
					n->agg_distinct = FALSE;
					yyval.node = (Node *)n;
				}
    break;

  case 1042:
#line 6644 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subLinkType = EXPR_SUBLINK;
					n->lefthand = NIL;
					n->operName = NIL;
					n->subselect = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 1043:
#line 6653 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subLinkType = EXISTS_SUBLINK;
					n->lefthand = NIL;
					n->operName = NIL;
					n->subselect = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 1044:
#line 6662 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subLinkType = ARRAY_SUBLINK;
					n->lefthand = NIL;
					n->operName = NIL;
					n->subselect = yyvsp[0].node;
					yyval.node = (Node *)n;
				}
    break;

  case 1045:
#line 6671 "gram.y"
    {	yyval.node = yyvsp[0].node;	}
    break;

  case 1046:
#line 6680 "gram.y"
    {
					A_Indices *ai = makeNode(A_Indices);
					ai->lidx = NULL;
					ai->uidx = yyvsp[-1].node;
					yyval.list = lappend(yyvsp[-3].list, ai);
				}
    break;

  case 1047:
#line 6687 "gram.y"
    {
					A_Indices *ai = makeNode(A_Indices);
					ai->lidx = yyvsp[-3].node;
					ai->uidx = yyvsp[-1].node;
					yyval.list = lappend(yyvsp[-5].list, ai);
				}
    break;

  case 1048:
#line 6694 "gram.y"
    { yyval.list = NIL; }
    break;

  case 1049:
#line 6698 "gram.y"
    {
					FastList *dst = (FastList *) &yyval.list;
					makeFastList1(dst, yyvsp[0].node);
				}
    break;

  case 1050:
#line 6703 "gram.y"
    {
					FastList *dst = (FastList *) &yyval.list;
					FastList *src = (FastList *) &yyvsp[-2].list;
					*dst = *src;
					FastAppend(dst, yyvsp[0].node);
				}
    break;

  case 1051:
#line 6713 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = yyvsp[-2].str;
					yyval.list = makeList2((Node *) n, yyvsp[0].node);
				}
    break;

  case 1052:
#line 6719 "gram.y"
    { yyval.list = NIL; }
    break;

  case 1053:
#line 6723 "gram.y"
    {
					yyval.list = lappend(yyvsp[-2].list, yyvsp[0].typnam);
				}
    break;

  case 1054:
#line 6727 "gram.y"
    {
					yyval.list = makeList1(yyvsp[0].typnam);
				}
    break;

  case 1055:
#line 6733 "gram.y"
    {	yyval.list = makeList1(yyvsp[0].node);		}
    break;

  case 1056:
#line 6735 "gram.y"
    {	yyval.list = lappend(yyvsp[-2].list, yyvsp[0].node);	}
    break;

  case 1057:
#line 6739 "gram.y"
    {
					ArrayExpr *n = makeNode(ArrayExpr);
					n->elements = yyvsp[-1].list;
					yyval.node = (Node *)n;
				}
    break;

  case 1058:
#line 6745 "gram.y"
    {
					ArrayExpr *n = makeNode(ArrayExpr);
					n->elements = yyvsp[-1].list;
					yyval.node = (Node *)n;
				}
    break;

  case 1059:
#line 6757 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1060:
#line 6758 "gram.y"
    { yyval.str = "year"; }
    break;

  case 1061:
#line 6759 "gram.y"
    { yyval.str = "month"; }
    break;

  case 1062:
#line 6760 "gram.y"
    { yyval.str = "day"; }
    break;

  case 1063:
#line 6761 "gram.y"
    { yyval.str = "hour"; }
    break;

  case 1064:
#line 6762 "gram.y"
    { yyval.str = "minute"; }
    break;

  case 1065:
#line 6763 "gram.y"
    { yyval.str = "second"; }
    break;

  case 1066:
#line 6764 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1067:
#line 6774 "gram.y"
    {
					yyval.list = makeList4(yyvsp[-3].node, yyvsp[-2].node, yyvsp[-1].node, yyvsp[0].node);
				}
    break;

  case 1068:
#line 6778 "gram.y"
    {
					yyval.list = makeList3(yyvsp[-2].node, yyvsp[-1].node, yyvsp[0].node);
				}
    break;

  case 1069:
#line 6785 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 1070:
#line 6791 "gram.y"
    { yyval.list = makeList2(yyvsp[0].node, yyvsp[-2].node); }
    break;

  case 1071:
#line 6792 "gram.y"
    { yyval.list = NIL; }
    break;

  case 1072:
#line 6808 "gram.y"
    {
					yyval.list = makeList3(yyvsp[-2].node, yyvsp[-1].node, yyvsp[0].node);
				}
    break;

  case 1073:
#line 6812 "gram.y"
    {
					yyval.list = makeList3(yyvsp[-2].node, yyvsp[0].node, yyvsp[-1].node);
				}
    break;

  case 1074:
#line 6816 "gram.y"
    {
					yyval.list = makeList2(yyvsp[-1].node, yyvsp[0].node);
				}
    break;

  case 1075:
#line 6820 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Integer;
					n->val.val.ival = 1;
					yyval.list = makeList3(yyvsp[-1].node, (Node *)n, yyvsp[0].node);
				}
    break;

  case 1076:
#line 6827 "gram.y"
    {
					yyval.list = yyvsp[0].list;
				}
    break;

  case 1077:
#line 6831 "gram.y"
    { yyval.list = NIL; }
    break;

  case 1078:
#line 6835 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 1079:
#line 6838 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 1080:
#line 6841 "gram.y"
    { yyval.list = lappend(yyvsp[0].list, yyvsp[-2].node); }
    break;

  case 1081:
#line 6842 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 1082:
#line 6843 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 1083:
#line 6847 "gram.y"
    {
					SubLink *n = makeNode(SubLink);
					n->subselect = yyvsp[0].node;
					/* other fields will be filled later */
					yyval.node = (Node *)n;
				}
    break;

  case 1084:
#line 6853 "gram.y"
    { yyval.node = (Node *)yyvsp[-1].list; }
    break;

  case 1085:
#line 6876 "gram.y"
    {
					CaseExpr *c = makeNode(CaseExpr);
					c->arg = (Expr *) yyvsp[-3].node;
					c->args = yyvsp[-2].list;
					c->defresult = (Expr *) yyvsp[-1].node;
					yyval.node = (Node *)c;
				}
    break;

  case 1086:
#line 6884 "gram.y"
    {
					yyval.node = (Node *) makeSimpleA_Expr(AEXPR_NULLIF, "=", yyvsp[-3].node, yyvsp[-1].node);
				}
    break;

  case 1087:
#line 6888 "gram.y"
    {
					CoalesceExpr *c = makeNode(CoalesceExpr);
					c->args = yyvsp[-1].list;
					yyval.node = (Node *)c;
				}
    break;

  case 1088:
#line 6897 "gram.y"
    { yyval.list = makeList1(yyvsp[0].node); }
    break;

  case 1089:
#line 6898 "gram.y"
    { yyval.list = lappend(yyvsp[-1].list, yyvsp[0].node); }
    break;

  case 1090:
#line 6903 "gram.y"
    {
					CaseWhen *w = makeNode(CaseWhen);
					w->expr = (Expr *) yyvsp[-2].node;
					w->result = (Expr *) yyvsp[0].node;
					yyval.node = (Node *)w;
				}
    break;

  case 1091:
#line 6912 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 1092:
#line 6913 "gram.y"
    { yyval.node = NULL; }
    break;

  case 1093:
#line 6916 "gram.y"
    { yyval.node = yyvsp[0].node; }
    break;

  case 1094:
#line 6917 "gram.y"
    { yyval.node = NULL; }
    break;

  case 1095:
#line 6926 "gram.y"
    {
					yyval.columnref = makeNode(ColumnRef);
					yyval.columnref->fields = makeList1(makeString(yyvsp[-1].str));
					yyval.columnref->indirection = yyvsp[0].list;
				}
    break;

  case 1096:
#line 6932 "gram.y"
    {
					yyval.columnref = makeNode(ColumnRef);
					yyval.columnref->fields = yyvsp[-1].list;
					yyval.columnref->indirection = yyvsp[0].list;
				}
    break;

  case 1097:
#line 6941 "gram.y"
    { yyval.list = lcons(makeString(yyvsp[-1].str), yyvsp[0].list); }
    break;

  case 1098:
#line 6945 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 1099:
#line 6947 "gram.y"
    { yyval.list = makeList1(makeString("*")); }
    break;

  case 1100:
#line 6949 "gram.y"
    { yyval.list = lcons(makeString(yyvsp[-1].str), yyvsp[0].list); }
    break;

  case 1101:
#line 6962 "gram.y"
    { yyval.list = makeList1(yyvsp[0].target); }
    break;

  case 1102:
#line 6963 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].target); }
    break;

  case 1103:
#line 6968 "gram.y"
    {
					yyval.target = makeNode(ResTarget);
					yyval.target->name = yyvsp[0].str;
					yyval.target->indirection = NIL;
					yyval.target->val = (Node *)yyvsp[-2].node;
				}
    break;

  case 1104:
#line 6975 "gram.y"
    {
					yyval.target = makeNode(ResTarget);
					yyval.target->name = NULL;
					yyval.target->indirection = NIL;
					yyval.target->val = (Node *)yyvsp[0].node;
				}
    break;

  case 1105:
#line 6982 "gram.y"
    {
					ColumnRef *n = makeNode(ColumnRef);
					n->fields = makeList1(makeString("*"));
					n->indirection = NIL;
					yyval.target = makeNode(ResTarget);
					yyval.target->name = NULL;
					yyval.target->indirection = NIL;
					yyval.target->val = (Node *)n;
				}
    break;

  case 1106:
#line 7000 "gram.y"
    { yyval.list = makeList1(yyvsp[0].target); }
    break;

  case 1107:
#line 7001 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list,yyvsp[0].target); }
    break;

  case 1108:
#line 7006 "gram.y"
    {
					yyval.target = makeNode(ResTarget);
					yyval.target->name = yyvsp[-3].str;
					yyval.target->indirection = yyvsp[-2].list;
					yyval.target->val = (Node *) yyvsp[0].node;
				}
    break;

  case 1109:
#line 7013 "gram.y"
    {
					yyval.target = makeNode(ResTarget);
					yyval.target->name = yyvsp[-3].str;
					yyval.target->indirection = yyvsp[-2].list;
					yyval.target->val = (Node *) makeNode(SetToDefault);
				}
    break;

  case 1110:
#line 7023 "gram.y"
    { yyval.list = makeList1(yyvsp[0].target); }
    break;

  case 1111:
#line 7024 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].target); }
    break;

  case 1112:
#line 7028 "gram.y"
    { yyval.target = yyvsp[0].target; }
    break;

  case 1113:
#line 7030 "gram.y"
    {
					yyval.target = makeNode(ResTarget);
					yyval.target->name = NULL;
					yyval.target->indirection = NIL;
					yyval.target->val = (Node *) makeNode(SetToDefault);
				}
    break;

  case 1114:
#line 7046 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1115:
#line 7047 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1116:
#line 7051 "gram.y"
    { yyval.list = makeList1(yyvsp[0].range); }
    break;

  case 1117:
#line 7052 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, yyvsp[0].range); }
    break;

  case 1118:
#line 7057 "gram.y"
    {
					yyval.range = makeNode(RangeVar);
					yyval.range->catalogname = NULL;
					yyval.range->schemaname = NULL;
					yyval.range->relname = yyvsp[0].str;
				}
    break;

  case 1119:
#line 7064 "gram.y"
    {
					yyval.range = makeNode(RangeVar);
					switch (length(yyvsp[0].list))
					{
						case 2:
							yyval.range->catalogname = NULL;
							yyval.range->schemaname = strVal(lfirst(yyvsp[0].list));
							yyval.range->relname = strVal(lsecond(yyvsp[0].list));
							break;
						case 3:
							yyval.range->catalogname = strVal(lfirst(yyvsp[0].list));
							yyval.range->schemaname = strVal(lsecond(yyvsp[0].list));
							yyval.range->relname = strVal(lthird(yyvsp[0].list));
							break;
						default:
							ereport(ERROR,
									(errcode(ERRCODE_SYNTAX_ERROR),
									 errmsg("improper qualified name (too many dotted names): %s",
											NameListToString(yyvsp[0].list))));
							break;
					}
				}
    break;

  case 1120:
#line 7089 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 1121:
#line 7091 "gram.y"
    { yyval.list = lappend(yyvsp[-2].list, makeString(yyvsp[0].str)); }
    break;

  case 1122:
#line 7095 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1123:
#line 7098 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1124:
#line 7101 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1125:
#line 7103 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1126:
#line 7105 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1127:
#line 7107 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1128:
#line 7110 "gram.y"
    { yyval.list = makeList1(makeString(yyvsp[0].str)); }
    break;

  case 1129:
#line 7111 "gram.y"
    { yyval.list = yyvsp[0].list; }
    break;

  case 1130:
#line 7119 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Integer;
					n->val.val.ival = yyvsp[0].ival;
					yyval.node = (Node *)n;
				}
    break;

  case 1131:
#line 7126 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Float;
					n->val.val.str = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 1132:
#line 7133 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->val.type = T_String;
					n->val.val.str = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 1133:
#line 7140 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->val.type = T_BitString;
					n->val.val.str = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 1134:
#line 7147 "gram.y"
    {
					/* This is a bit constant per SQL99:
					 * Without Feature F511, "BIT data type",
					 * a <general literal> shall not be a
					 * <bit string literal> or a <hex string literal>.
					 */
					A_Const *n = makeNode(A_Const);
					n->val.type = T_BitString;
					n->val.val.str = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 1135:
#line 7159 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->typename = yyvsp[-1].typnam;
					n->val.type = T_String;
					n->val.val.str = yyvsp[0].str;
					yyval.node = (Node *)n;
				}
    break;

  case 1136:
#line 7167 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->typename = yyvsp[-2].typnam;
					n->val.type = T_String;
					n->val.val.str = yyvsp[-1].str;
					/* precision is not specified, but fields may be... */
					if (yyvsp[0].ival != INTERVAL_FULL_RANGE)
						n->typename->typmod = INTERVAL_TYPMOD(INTERVAL_FULL_PRECISION, yyvsp[0].ival);
					yyval.node = (Node *)n;
				}
    break;

  case 1137:
#line 7178 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->typename = yyvsp[-5].typnam;
					n->val.type = T_String;
					n->val.val.str = yyvsp[-1].str;
					/* precision specified, and fields may be... */
					if (yyvsp[-3].ival < 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("INTERVAL(%d) precision must not be negative",
										yyvsp[-3].ival)));
					if (yyvsp[-3].ival > MAX_INTERVAL_PRECISION)
					{
						ereport(NOTICE,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("INTERVAL(%d) precision reduced to maximum allowed, %d",
										yyvsp[-3].ival, MAX_INTERVAL_PRECISION)));
						yyvsp[-3].ival = MAX_INTERVAL_PRECISION;
					}
					n->typename->typmod = INTERVAL_TYPMOD(yyvsp[-3].ival, yyvsp[0].ival);
					yyval.node = (Node *)n;
				}
    break;

  case 1138:
#line 7201 "gram.y"
    {
					ParamRef *n = makeNode(ParamRef);
					n->number = yyvsp[-1].ival;
					n->fields = NIL;
					n->indirection = yyvsp[0].list;
					yyval.node = (Node *)n;
				}
    break;

  case 1139:
#line 7209 "gram.y"
    {
					yyval.node = (Node *)makeBoolConst(TRUE);
				}
    break;

  case 1140:
#line 7213 "gram.y"
    {
					yyval.node = (Node *)makeBoolConst(FALSE);
				}
    break;

  case 1141:
#line 7217 "gram.y"
    {
					A_Const *n = makeNode(A_Const);
					n->val.type = T_Null;
					yyval.node = (Node *)n;
				}
    break;

  case 1142:
#line 7224 "gram.y"
    { yyval.ival = yyvsp[0].ival; }
    break;

  case 1143:
#line 7225 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1144:
#line 7226 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1145:
#line 7241 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1146:
#line 7242 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1147:
#line 7243 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1148:
#line 7248 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1149:
#line 7249 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1150:
#line 7255 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1151:
#line 7256 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1152:
#line 7257 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1153:
#line 7263 "gram.y"
    { yyval.str = yyvsp[0].str; }
    break;

  case 1154:
#line 7264 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1155:
#line 7265 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1156:
#line 7266 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1157:
#line 7267 "gram.y"
    { yyval.str = pstrdup(yyvsp[0].keyword); }
    break;

  case 1463:
#line 7628 "gram.y"
    {
					if (QueryIsRule)
						yyval.str = "*OLD*";
					else
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("OLD used in non-rule query")));
				}
    break;

  case 1464:
#line 7637 "gram.y"
    {
					if (QueryIsRule)
						yyval.str = "*NEW*";
					else
						ereport(ERROR,
								(errcode(ERRCODE_SYNTAX_ERROR),
								 errmsg("NEW used in non-rule query")));
				}
    break;


    }

/* Line 991 of yacc.c.  */
#line 18169 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 7647 "gram.y"


static Node *
makeTypeCast(Node *arg, TypeName *typename)
{
	/*
	 * Simply generate a TypeCast node.
	 *
	 * Earlier we would determine whether an A_Const would
	 * be acceptable, however Domains require coerce_type()
	 * to process them -- applying constraints as required.
	 */
	TypeCast *n = makeNode(TypeCast);
	n->arg = arg;
	n->typename = typename;
	return (Node *) n;
}

static Node *
makeStringConst(char *str, TypeName *typename)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_String;
	n->val.val.str = str;
	n->typename = typename;

	return (Node *)n;
}

static Node *
makeIntConst(int val)
{
	A_Const *n = makeNode(A_Const);
	n->val.type = T_Integer;
	n->val.val.ival = val;
	n->typename = SystemTypeName("int4");

	return (Node *)n;
}

static Node *
makeFloatConst(char *str)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_Float;
	n->val.val.str = str;
	n->typename = SystemTypeName("float8");

	return (Node *)n;
}

static Node *
makeAConst(Value *v)
{
	Node *n;

	switch (v->type)
	{
		case T_Float:
			n = makeFloatConst(v->val.str);
			break;

		case T_Integer:
			n = makeIntConst(v->val.ival);
			break;

		case T_String:
		default:
			n = makeStringConst(v->val.str, NULL);
			break;
	}

	return n;
}

/* makeDefElem()
 * Create a DefElem node and set contents.
 * Could be moved to nodes/makefuncs.c if this is useful elsewhere.
 */
static DefElem *
makeDefElem(char *name, Node *arg)
{
	DefElem *f = makeNode(DefElem);
	f->defname = name;
	f->arg = arg;
	return f;
}

/* makeBoolConst()
 * Create an A_Const node and initialize to a boolean constant.
 */
static A_Const *
makeBoolConst(bool state)
{
	A_Const *n = makeNode(A_Const);
	n->val.type = T_String;
	n->val.val.str = (state? "t": "f");
	n->typename = SystemTypeName("bool");
	return n;
}

/* makeRowExpr()
 * Generate separate operator nodes for a single row descriptor expression.
 * Perhaps this should go deeper in the parser someday...
 * - thomas 1997-12-22
 */
static Node *
makeRowExpr(List *opr, List *largs, List *rargs)
{
	Node *expr = NULL;
	Node *larg, *rarg;
	char *oprname;

	if (length(largs) != length(rargs))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("unequal number of entries in row expression")));

	if (lnext(largs) != NIL)
		expr = makeRowExpr(opr, lnext(largs), lnext(rargs));

	larg = lfirst(largs);
	rarg = lfirst(rargs);

	oprname = strVal(llast(opr));

	if ((strcmp(oprname, "=") == 0) ||
		(strcmp(oprname, "<") == 0) ||
		(strcmp(oprname, "<=") == 0) ||
		(strcmp(oprname, ">") == 0) ||
		(strcmp(oprname, ">=") == 0))
	{
		if (expr == NULL)
			expr = (Node *) makeA_Expr(AEXPR_OP, opr, larg, rarg);
		else
			expr = (Node *) makeA_Expr(AEXPR_AND, NIL, expr,
									   (Node *) makeA_Expr(AEXPR_OP, opr,
														   larg, rarg));
	}
	else if (strcmp(oprname, "<>") == 0)
	{
		if (expr == NULL)
			expr = (Node *) makeA_Expr(AEXPR_OP, opr, larg, rarg);
		else
			expr = (Node *) makeA_Expr(AEXPR_OR, NIL, expr,
									   (Node *) makeA_Expr(AEXPR_OP, opr,
														   larg, rarg));
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("operator %s is not supported for row expressions",
						oprname)));
	}

	return expr;
}

/* makeDistinctExpr()
 * Generate separate operator nodes for a single row descriptor expression.
 * Same comments as for makeRowExpr().
 */
static Node *
makeDistinctExpr(List *largs, List *rargs)
{
	Node *expr = NULL;
	Node *larg, *rarg;

	if (length(largs) != length(rargs))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("unequal number of entries in row expression")));

	if (lnext(largs) != NIL)
		expr = makeDistinctExpr(lnext(largs), lnext(rargs));

	larg = lfirst(largs);
	rarg = lfirst(rargs);

	if (expr == NULL)
		expr = (Node *) makeSimpleA_Expr(AEXPR_DISTINCT, "=", larg, rarg);
	else
		expr = (Node *) makeA_Expr(AEXPR_OR, NIL, expr,
								   (Node *) makeSimpleA_Expr(AEXPR_DISTINCT, "=",
															 larg, rarg));

	return expr;
}

/* makeRowNullTest()
 * Generate separate operator nodes for a single row descriptor test.
 */
static Node *
makeRowNullTest(NullTestType test, List *args)
{
	Node *expr = NULL;
	NullTest *n;

	if (lnext(args) != NIL)
		expr = makeRowNullTest(test, lnext(args));

	n = makeNode(NullTest);
	n->arg = (Expr *) lfirst(args);
	n->nulltesttype = test;

	if (expr == NULL)
		expr = (Node *) n;
	else if (test == IS_NOT_NULL)
		expr = (Node *) makeA_Expr(AEXPR_OR, NIL, expr, (Node *)n);
	else
		expr = (Node *) makeA_Expr(AEXPR_AND, NIL, expr, (Node *)n);

	return expr;
}

/* makeOverlaps()
 * Create and populate a FuncCall node to support the OVERLAPS operator.
 */
static FuncCall *
makeOverlaps(List *largs, List *rargs)
{
	FuncCall *n = makeNode(FuncCall);
	n->funcname = SystemFuncName("overlaps");
	if (length(largs) == 1)
		largs = lappend(largs, largs);
	else if (length(largs) != 2)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("wrong number of parameters on left side of OVERLAPS expression")));
	if (length(rargs) == 1)
		rargs = lappend(rargs, rargs);
	else if (length(rargs) != 2)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("wrong number of parameters on right side of OVERLAPS expression")));
	n->args = nconc(largs, rargs);
	n->agg_star = FALSE;
	n->agg_distinct = FALSE;
	return n;
}

/* findLeftmostSelect()
 * Find the leftmost component SelectStmt in a set-operation parsetree.
 */
static SelectStmt *
findLeftmostSelect(SelectStmt *node)
{
	while (node && node->op != SETOP_NONE)
		node = node->larg;
	Assert(node && IsA(node, SelectStmt) && node->larg == NULL);
	return node;
}

/* insertSelectOptions()
 * Insert ORDER BY, etc into an already-constructed SelectStmt.
 *
 * This routine is just to avoid duplicating code in SelectStmt productions.
 */
static void
insertSelectOptions(SelectStmt *stmt,
					List *sortClause, List *forUpdate,
					Node *limitOffset, Node *limitCount)
{
	/*
	 * Tests here are to reject constructs like
	 *	(SELECT foo ORDER BY bar) ORDER BY baz
	 */
	if (sortClause)
	{
		if (stmt->sortClause)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("multiple ORDER BY clauses not allowed")));
		stmt->sortClause = sortClause;
	}
	if (forUpdate)
	{
		if (stmt->forUpdate)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("multiple FOR UPDATE clauses not allowed")));
		stmt->forUpdate = forUpdate;
	}
	if (limitOffset)
	{
		if (stmt->limitOffset)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("multiple OFFSET clauses not allowed")));
		stmt->limitOffset = limitOffset;
	}
	if (limitCount)
	{
		if (stmt->limitCount)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("multiple LIMIT clauses not allowed")));
		stmt->limitCount = limitCount;
	}
}

static Node *
makeSetOp(SetOperation op, bool all, Node *larg, Node *rarg)
{
	SelectStmt *n = makeNode(SelectStmt);

	n->op = op;
	n->all = all;
	n->larg = (SelectStmt *) larg;
	n->rarg = (SelectStmt *) rarg;
	return (Node *) n;
}

/* SystemFuncName()
 * Build a properly-qualified reference to a built-in function.
 */
List *
SystemFuncName(char *name)
{
	return makeList2(makeString("pg_catalog"), makeString(name));
}

/* SystemTypeName()
 * Build a properly-qualified reference to a built-in type.
 *
 * typmod is defaulted, but may be changed afterwards by caller.
 */
TypeName *
SystemTypeName(char *name)
{
	TypeName   *n = makeNode(TypeName);

	n->names = makeList2(makeString("pg_catalog"), makeString(name));
	n->typmod = -1;
	return n;
}

/* parser_init()
 * Initialize to parse one query string
 */
void
parser_init(void)
{
	QueryIsRule = FALSE;
}

/* exprIsNullConstant()
 * Test whether an a_expr is a plain NULL constant or not.
 */
bool
exprIsNullConstant(Node *arg)
{
	if (arg && IsA(arg, A_Const))
	{
		A_Const *con = (A_Const *) arg;

		if (con->val.type == T_Null &&
			con->typename == NULL)
			return TRUE;
	}
	return FALSE;
}

/* doNegate()
 * Handle negation of a numeric constant.
 *
 * Formerly, we did this here because the optimizer couldn't cope with
 * indexquals that looked like "var = -4" --- it wants "var = const"
 * and a unary minus operator applied to a constant didn't qualify.
 * As of Postgres 7.0, that problem doesn't exist anymore because there
 * is a constant-subexpression simplifier in the optimizer.  However,
 * there's still a good reason for doing this here, which is that we can
 * postpone committing to a particular internal representation for simple
 * negative constants.	It's better to leave "-123.456" in string form
 * until we know what the desired type is.
 */
static Node *
doNegate(Node *n)
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
			doNegateFloat(&con->val);
			return n;
		}
	}

	return (Node *) makeSimpleA_Expr(AEXPR_OP, "-", NULL, n);
}

static void
doNegateFloat(Value *v)
{
	char   *oldval = v->val.str;

	Assert(IsA(v, Float));
	if (*oldval == '+')
		oldval++;
	if (*oldval == '-')
		v->val.str = oldval+1;	/* just strip the '-' */
	else
	{
		char   *newval = (char *) palloc(strlen(oldval) + 2);

		*newval = '-';
		strcpy(newval+1, oldval);
		v->val.str = newval;
	}
}

#include "scan.c"

