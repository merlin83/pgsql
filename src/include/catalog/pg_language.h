/*-------------------------------------------------------------------------
 *
 * pg_language.h
 *	  definition of the system "language" relation (pg_language)
 *	  along with the relation's initial contents.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pg_language.h,v 1.16 2002-02-18 23:11:35 petere Exp $
 *
 * NOTES
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_LANGUAGE_H
#define PG_LANGUAGE_H

/* ----------------
 *		postgres.h contains the system type definintions and the
 *		CATALOG(), BOOTSTRAP and DATA() sugar words so this file
 *		can be read by both genbki.sh and the C compiler.
 * ----------------
 */

/* ----------------
 *		pg_language definition.  cpp turns this into
 *		typedef struct FormData_pg_language
 * ----------------
 */
CATALOG(pg_language)
{
	NameData	lanname;
	bool		lanispl;		/* Is a procedural language */
	bool		lanpltrusted;	/* PL is trusted */
	Oid			lanplcallfoid;	/* Call handler for PL */
	text		lancompiler;	/* VARIABLE LENGTH FIELD */
    aclitem		lanacl[1];		/* Access privileges */
} FormData_pg_language;

/* ----------------
 *		Form_pg_language corresponds to a pointer to a tuple with
 *		the format of pg_language relation.
 * ----------------
 */
typedef FormData_pg_language *Form_pg_language;

/* ----------------
 *		compiler constants for pg_language
 * ----------------
 */
#define Natts_pg_language				6
#define Anum_pg_language_lanname		1
#define Anum_pg_language_lanispl		2
#define Anum_pg_language_lanpltrusted		3
#define Anum_pg_language_lanplcallfoid		4
#define Anum_pg_language_lancompiler		5
#define Anum_pg_language_lanacl			6

/* ----------------
 *		initial contents of pg_language
 * ----------------
 */

DATA(insert OID = 12 ( "internal" f f 0 "n/a" _null_ ));
DESCR("Built-in functions");
#define INTERNALlanguageId 12
DATA(insert OID = 13 ( "c" f f 0 "/bin/cc" _null_ ));
DESCR("Dynamically-loaded C functions");
#define ClanguageId 13
DATA(insert OID = 14 ( "sql" f t 0 "postgres" _null_ ));
DESCR("SQL-language functions");
#define SQLlanguageId 14

#endif   /* PG_LANGUAGE_H */
