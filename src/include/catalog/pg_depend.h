/*-------------------------------------------------------------------------
 *
 * pg_depend.h
 *	  definition of the system "dependency" relation (pg_depend)
 *	  along with the relation's initial contents.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/catalog/pg_depend.h,v 1.5 2004/12/31 22:03:24 pgsql Exp $
 *
 * NOTES
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_DEPEND_H
#define PG_DEPEND_H

/* ----------------
 *		postgres.h contains the system type definitions and the
 *		CATALOG(), BOOTSTRAP and DATA() sugar words so this file
 *		can be read by both genbki.sh and the C compiler.
 * ----------------
 */

/* ----------------
 *		pg_depend definition.  cpp turns this into
 *		typedef struct FormData_pg_depend
 * ----------------
 */
CATALOG(pg_depend) BKI_WITHOUT_OIDS
{
	/*
	 * Identification of the dependent (referencing) object.
	 *
	 * These fields are all zeroes for a DEPENDENCY_PIN entry.
	 */
	Oid			classid;		/* OID of table containing object */
	Oid			objid;			/* OID of object itself */
	int4		objsubid;		/* column number, or 0 if not used */

	/*
	 * Identification of the independent (referenced) object.
	 */
	Oid			refclassid;		/* OID of table containing object */
	Oid			refobjid;		/* OID of object itself */
	int4		refobjsubid;	/* column number, or 0 if not used */

	/*
	 * Precise semantics of the relationship are specified by the deptype
	 * field.  See DependencyType in catalog/dependency.h.
	 */
	char		deptype;		/* see codes in dependency.h */
} FormData_pg_depend;

/* ----------------
 *		Form_pg_depend corresponds to a pointer to a row with
 *		the format of pg_depend relation.
 * ----------------
 */
typedef FormData_pg_depend *Form_pg_depend;

/* ----------------
 *		compiler constants for pg_depend
 * ----------------
 */
#define Natts_pg_depend				7
#define Anum_pg_depend_classid		1
#define Anum_pg_depend_objid		2
#define Anum_pg_depend_objsubid		3
#define Anum_pg_depend_refclassid	4
#define Anum_pg_depend_refobjid		5
#define Anum_pg_depend_refobjsubid	6
#define Anum_pg_depend_deptype		7


/*
 * pg_depend has no preloaded contents; system-defined dependencies are
 * loaded into it during a late stage of the initdb process.
 *
 * NOTE: we do not represent all possible dependency pairs in pg_depend;
 * for example, there's not much value in creating an explicit dependency
 * from an attribute to its relation.  Usually we make a dependency for
 * cases where the relationship is conditional rather than essential
 * (for example, not all triggers are dependent on constraints, but all
 * attributes are dependent on relations) or where the dependency is not
 * convenient to find from the contents of other catalogs.
 */

#endif   /* PG_DEPEND_H */
