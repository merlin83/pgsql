/*-------------------------------------------------------------------------
 *
 * pg_database.h--
 *    definition of the system "database" relation (pg_database)
 *    along with the relation's initial contents.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pg_database.h,v 1.3 1997-08-21 02:28:55 momjian Exp $
 *
 * NOTES
 *    the genbki.sh script reads this file and generates .bki
 *    information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_DATABASE_H
#define PG_DATABASE_H

/* ----------------
 *	postgres.h contains the system type definintions and the
 *	CATALOG(), BOOTSTRAP and DATA() sugar words so this file
 *	can be read by both genbki.sh and the C compiler.
 * ----------------
 */

/* ----------------
 *	pg_database definition.  cpp turns this into
 *	typedef struct FormData_pg_database
 * ----------------
 */ 
CATALOG(pg_database) BOOTSTRAP {
    NameData 	datname;
    int4 	datdba;
    text 	datpath;	/* VARIABLE LENGTH FIELD */
} FormData_pg_database;

/* ----------------
 *	Form_pg_database corresponds to a pointer to a tuple with
 *	the format of pg_database relation.
 * ----------------
 */
typedef FormData_pg_database	*Form_pg_database;

/* ----------------
 *	compiler constants for pg_database
 * ----------------
 */
#define Natts_pg_database		3
#define Anum_pg_database_datname	1
#define Anum_pg_database_datdba		2
#define Anum_pg_database_datpath	3


#endif /* PG_DATABASE_H */
