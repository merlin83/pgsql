/*-------------------------------------------------------------------------
 *
 * pg_class.h--
 *	  definition of the system "relation" relation (pg_class)
 *	  along with the relation's initial contents.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pg_class_mb.h,v 1.2 1998-08-06 05:13:07 momjian Exp $
 *
 * NOTES
 *	  ``pg_relation'' is being replaced by ``pg_class''.  currently
 *	  we are only changing the name in the catalogs but someday the
 *	  code will be changed too. -cim 2/26/90
 *	  [it finally happens.	-ay 11/5/94]
 *
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_RELATION_H
#define PG_RELATION_H

/* ----------------
 *		postgres.h contains the system type definintions and the
 *		CATALOG(), BOOTSTRAP and DATA() sugar words so this file
 *		can be read by both genbki.sh and the C compiler.
 * ----------------
 */

/* ----------------
 *		pg_class definition.  cpp turns this into
 *		typedef struct FormData_pg_class
 *
 *		Note: the #if 0, #endif around the BKI_BEGIN.. END block
 *			  below keeps cpp from seeing what is meant for the
 *			  genbki script: pg_relation is now called pg_class, but
 *			  only in the catalogs -cim 2/26/90
 * ----------------
 */

/* ----------------
 *		This structure is actually variable-length (the last attribute is
 *		a POSTGRES array).	Hence, sizeof(FormData_pg_class) does not
 *		describe the fixed-length or actual size of the structure.
 *		FormData_pg_class.relacl may not be correctly aligned, either,
 *		if aclitem and struct varlena don't align together.  Hence,
 *		you MUST use heap_getattr() to get the relacl field.
 * ----------------
 */
CATALOG(pg_class) BOOTSTRAP
{
	NameData	relname;
	Oid			reltype;
	Oid			relowner;
	Oid			relam;
	int4		relpages;
	int4		reltuples;
	bool		relhasindex;
	bool		relisshared;
	char		relkind;
	int2		relnatts;

	/*
	 * relnatts is the number of user attributes this class has.  There
	 * must be exactly this many instances in Class pg_attribute for this
	 * class which have attnum > 0 (= user attribute).
	 */
	int2		relchecks;		/* # of CHECK constraints, not stored in
								 * db? */
	int2		reltriggers;	/* # of TRIGGERs */
	bool		relhasrules;
	aclitem		relacl[1];		/* this is here for the catalog */
} FormData_pg_class;

#define CLASS_TUPLE_SIZE \
	 (offsetof(FormData_pg_class,relhasrules) + sizeof(bool))

/* ----------------
 *		Form_pg_class corresponds to a pointer to a tuple with
 *		the format of pg_class relation.
 * ----------------
 */
typedef FormData_pg_class *Form_pg_class;

/* ----------------
 *		compiler constants for pg_class
 * ----------------
 */

/* ----------------
 *		Natts_pg_class_fixed is used to tell routines that insert new
 *		pg_class tuples (as opposed to replacing old ones) that there's no
 *		relacl field.
 * ----------------
 */
#define Natts_pg_class_fixed			13
#define Natts_pg_class					14
#define Anum_pg_class_relname			1
#define Anum_pg_class_reltype			2
#define Anum_pg_class_relowner			3
#define Anum_pg_class_relam				4
#define Anum_pg_class_relpages			5
#define Anum_pg_class_reltuples			6
#define Anum_pg_class_relhasindex		7
#define Anum_pg_class_relisshared		8
#define Anum_pg_class_relkind			9
#define Anum_pg_class_relnatts			10
#define Anum_pg_class_relchecks			11
#define Anum_pg_class_reltriggers		12
#define Anum_pg_class_relhasrules		13
#define Anum_pg_class_relacl			14

/* ----------------
 *		initial contents of pg_class
 * ----------------
 */

DATA(insert OID = 1247 (  pg_type 71		  PGUID 0 0 0 f f r 16 0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1249 (  pg_attribute 75	  PGUID 0 0 0 f f r 14 0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1255 (  pg_proc 81		  PGUID 0 0 0 f f r 16 0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1259 (  pg_class 83		  PGUID 0 0 0 f f r 14 0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1260 (  pg_shadow 86		  PGUID 0 0 0 f t r 8  0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1261 (  pg_group 87		  PGUID 0 0 0 f t s 3  0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1262 (  pg_database 88	  PGUID 0 0 0 f t r 4  0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1264 (  pg_variable 90	  PGUID 0 0 0 f t s 2  0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1269 (  pg_log  99		  PGUID 0 0 0 f t s 1  0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1215 (  pg_attrdef 109	  PGUID 0 0 0 t t r 4  0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1216 (  pg_relcheck 110	  PGUID 0 0 0 t t r 4  0 0 f _null_ ));
DESCR("");
DATA(insert OID = 1219 (  pg_trigger 111	  PGUID 0 0 0 t t r 7  0 0 f _null_ ));
DESCR("");

#define RelOid_pg_type			1247
#define RelOid_pg_attribute		1249
#define RelOid_pg_proc			1255
#define RelOid_pg_class			1259
#define RelOid_pg_shadow		1260
#define RelOid_pg_group			1261
#define RelOid_pg_database		1262
#define RelOid_pg_variable		1264
#define RelOid_pg_log			1269
#define RelOid_pg_attrdef		1215
#define RelOid_pg_relcheck		1216
#define RelOid_pg_trigger		1219

#define		  RELKIND_INDEX			  'i'		/* secondary index */
#define		  RELKIND_RELATION		  'r'		/* cataloged heap */
#define		  RELKIND_SPECIAL		  's'		/* special (non-heap) */
#define		  RELKIND_SEQUENCE		  'S'		/* SEQUENCE relation */
#define		  RELKIND_UNCATALOGED	  'u'		/* temporary heap */

#endif							/* PG_RELATION_H */
