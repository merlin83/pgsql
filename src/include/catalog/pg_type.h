/*-------------------------------------------------------------------------
 *
 * pg_type.h
 *	  definition of the system "type" relation (pg_type)
 *	  along with the relation's initial contents.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pg_type.h,v 1.60 1999-05-10 00:46:20 momjian Exp $
 *
 * NOTES
 *	  the genbki.sh script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_TYPE_H
#define PG_TYPE_H

#include <utils/rel.h>

/* ----------------
 *		postgres.h contains the system type definitions and the
 *		CATALOG(), BOOTSTRAP and DATA() sugar words so this file
 *		can be read by both genbki.sh and the C compiler.
 * ----------------
 */

/* ----------------
 *		pg_type definition.  cpp turns this into
 *		typedef struct FormData_pg_type
 *
 *		Some of the values in a pg_type instance are copied into
 *		pg_attribute instances.  Some parts of Postgres use the pg_type copy,
 *		while others use the pg_attribute copy, so they must match.
 *		See struct FormData_pg_attribute for details.
 * ----------------
 */
CATALOG(pg_type) BOOTSTRAP
{
	NameData	typname;
	int4		typowner;
	int2		typlen;

	/*
	 * typlen is the number of bytes we use to represent a value of this
	 * type, e.g. 4 for an int4.  But for a variable length type, typlen
	 * is -1.
	 */
	int2		typprtlen;
	bool		typbyval;

	/*
	 * typbyval determines whether internal Postgres routines pass a value
	 * of this type by value or by reference.  Only char, short, and int-
	 * equivalent items can be passed by value, so if the type is not
	 * 1, 2, or 4 bytes long, Postgres does not have the option of passing
	 * by value and so typbyval had better be FALSE.  Variable-length types
	 * are always passed by reference.
	 * Note that typbyval can be false even if the length would allow
	 * pass-by-value; this is currently true for type float4, for example.
	 */
	char		typtype;

	/*
	 * typtype is 'b' for a basic type and 'c' for a catalog type (ie a class).
	 * If typtype is 'c', typrelid is the OID of the class' entry in pg_class.
	 * (Why do we need an entry in pg_type for classes, anyway?)
	 */
	bool		typisdefined;
	char		typdelim;
	Oid			typrelid;		/* 0 if not a class type */
	Oid			typelem;

	/*
	 * typelem is 0 if this is not an array type.  If this is an array
	 * type, typelem is the OID of the type of the elements of the array
	 * (it identifies another row in Table pg_type).
	 */
	regproc		typinput;
	regproc		typoutput;
	regproc		typreceive;
	regproc		typsend;
	char		typalign;

	/* ----------------
	 * typalign is the alignment required when storing a value of this
	 * type.  It applies to storage on disk as well as most
	 * representations of the value inside Postgres.  When multiple values
	 * are stored consecutively, such as in the representation of a
	 * complete row on disk, padding is inserted before a datum of this
	 * type so that it begins on the specified boundary.  The alignment
	 * reference is the beginning of the first datum in the sequence.
	 *
	 * 'c' = CHAR alignment, ie no alignment needed.
	 * 's' = SHORT alignment (2 bytes on most machines).
	 * 'i' = INT alignment (4 bytes on most machines).
	 * 'd' = DOUBLE alignment (8 bytes on many machines, but by no means all).
	 *
	 * See include/utils/memutils.h for the macros that compute these
	 * alignment requirements.
	 *
	 * NOTE: for types used in system tables, it is critical that the
	 * size and alignment defined in pg_type agree with the way that the
	 * compiler will lay out the field in a struct representing a table row.
	 * ----------------
	 */
	text		typdefault;		/* VARIABLE LENGTH FIELD */
} FormData_pg_type;

/* ----------------
 *		Form_pg_type corresponds to a pointer to a row with
 *		the format of pg_type relation.
 * ----------------
 */
typedef FormData_pg_type *Form_pg_type;

/* ----------------
 *		compiler constants for pg_type
 * ----------------
 */
#define Natts_pg_type					16
#define Anum_pg_type_typname			1
#define Anum_pg_type_typowner			2
#define Anum_pg_type_typlen				3
#define Anum_pg_type_typprtlen			4
#define Anum_pg_type_typbyval			5
#define Anum_pg_type_typtype			6
#define Anum_pg_type_typisdefined		7
#define Anum_pg_type_typdelim			8
#define Anum_pg_type_typrelid			9
#define Anum_pg_type_typelem			10
#define Anum_pg_type_typinput			11
#define Anum_pg_type_typoutput			12
#define Anum_pg_type_typreceive			13
#define Anum_pg_type_typsend			14
#define Anum_pg_type_typalign			15
#define Anum_pg_type_typdefault			16

/* ----------------
 *		initial contents of pg_type
 * ----------------
 */

/* keep the following ordered by OID so that later changes can be made easier*/

/* Make sure the typlen, typbyval, and typalign values here match the initial
   values for attlen, attbyval, and attalign in both places in pg_attribute.h
   for every instance.
*/

/* OIDS 1 - 99 */
DATA(insert OID = 16 (	bool	   PGUID  1   1 t b t \054 0   0 boolin boolout boolin boolout c _null_ ));
DESCR("boolean 'true'/'false'");
#define BOOLOID			16

DATA(insert OID = 17 (	bytea	   PGUID -1  -1 f b t \054 0  18 byteain byteaout byteain byteaout i _null_ ));
DESCR("variable length array of bytes");
#define BYTEAOID		17

DATA(insert OID = 18 (	char	   PGUID  1   1 t b t \054 0   0 charin charout charin charout c _null_ ));
DESCR("single character");
#define CHAROID			18

DATA(insert OID = 19 (	name	   PGUID NAMEDATALEN NAMEDATALEN  f b t \054 0	18 namein nameout namein nameout i _null_ ));
DESCR("31-character type for storing system identifiers");
#define NAMEOID			19

DATA(insert OID = 20 (	int8	   PGUID  8  20 f b t \054 0   0 int8in int8out int8in int8out d _null_ ));
DESCR("eight-byte integer, >18 digits");
#define INT8OID			20

DATA(insert OID = 21 (	int2	   PGUID  2   5 t b t \054 0   0 int2in int2out int2in int2out s _null_ ));
DESCR("two-byte integer, -32k to 32k");
#define INT2OID			21

DATA(insert OID = 22 (	int28	   PGUID 16  50 f b t \054 0  21 int28in int28out int28in int28out i _null_ ));
DESCR("8 2-byte integers, used internally");
/*
 * XXX -- the implementation of int28's in postgres is a hack, and will
 *		  go away someday.	until that happens, there is a case (in the
 *		  catalog cache management code) where we need to step gingerly
 *		  over piles of int28's on the sidewalk.  in order to do so, we
 *		  need the OID of the int28 row from pg_type.
 */
#define INT28OID		22

DATA(insert OID = 23 (	int4	   PGUID  4  10 t b t \054 0   0 int4in int4out int4in int4out i _null_ ));
DESCR("4-byte integer, -2B to 2B");
#define INT4OID			23

DATA(insert OID = 24 (	regproc    PGUID  4  16 t b t \054 0   0 regprocin regprocout regprocin regprocout i _null_ ));
DESCR("registered procedure");
#define REGPROCOID		24

DATA(insert OID = 25 (	text	   PGUID -1  -1 f b t \054 0  18 textin textout textin textout i _null_ ));
DESCR("native variable-length string");
#define TEXTOID			25

DATA(insert OID = 26 (	oid		   PGUID  4  10 t b t \054 0   0 oidin oidout oidin oidout i _null_ ));
DESCR("object identifier type");
#define OIDOID			26

DATA(insert OID = 27 (	tid		   PGUID  6  19 f b t \054 0   0 tidin tidout tidin tidout i _null_ ));
DESCR("tuple identifier type, physical location of tuple");
#define TIDOID		27

DATA(insert OID = 28 (	xid		   PGUID  4  12 t b t \054 0   0 xidin xidout xidin xidout i _null_ ));
DESCR("transaction id");
#define XIDOID 28

DATA(insert OID = 29 (	cid		   PGUID  4  10 t b t \054 0   0 cidin cidout cidin cidout i _null_ ));
DESCR("command identifier type, sequence in transaction id");
#define CIDOID 29

DATA(insert OID = 30 (	oid8	   PGUID 32  89 f b t \054 0  26 oid8in oid8out oid8in oid8out i _null_ ));
DESCR("array of 8 oid, used in system tables");
DATA(insert OID = 32 (	SET		   PGUID -1  -1 f b t \054 0   0 textin textout textin textout i _null_ ));
DESCR("set of tuples");

DATA(insert OID = 71 (	pg_type		 PGUID 4 4 t c t \054 1247 0 foo bar foo bar i _null_));
DATA(insert OID = 75 (	pg_attribute PGUID 4 4 t c t \054 1249 0 foo bar foo bar i _null_));
DATA(insert OID = 81 (	pg_proc		 PGUID 4 4 t c t \054 1255 0 foo bar foo bar i _null_));
DATA(insert OID = 83 (	pg_class	 PGUID 4 4 t c t \054 1259 0 foo bar foo bar i _null_));
DATA(insert OID = 86 (	pg_shadow	 PGUID 4 4 t c t \054 1260 0 foo bar foo bar i _null_));
DATA(insert OID = 87 (	pg_group	 PGUID 4 4 t c t \054 1261 0 foo bar foo bar i _null_));
DATA(insert OID = 88 (	pg_database  PGUID 4 4 t c t \054 1262 0 foo bar foo bar i _null_));
DATA(insert OID = 90 (	pg_variable  PGUID 4 4 t c t \054 1264 0 foo bar foo bar i _null_));
DATA(insert OID = 99 (	pg_log		 PGUID 4 4 t c t \054 1269 0 foo bar foo bar i _null_));

/* OIDS 100 - 199 */

DATA(insert OID = 109 (  pg_attrdef  PGUID 4 4 t c t \054 1215 0 foo bar foo bar i _null_));
DATA(insert OID = 110 (  pg_relcheck PGUID 4 4 t c t \054 1216 0 foo bar foo bar i _null_));
DATA(insert OID = 111 (  pg_trigger  PGUID 4 4 t c t \054 1219 0 foo bar foo bar i _null_));

/* OIDS 200 - 299 */

DATA(insert OID = 210 (  smgr	   PGUID 2	12 t b t \054 0 0 smgrin smgrout smgrin smgrout s _null_ ));
DESCR("storage manager");

/* OIDS 300 - 399 */

/* OIDS 400 - 499 */

/* OIDS 500 - 599 */

/* OIDS 600 - 699 */
DATA(insert OID = 600 (  point	   PGUID 16  24 f b t \054 0 701 point_in point_out point_in point_out d _null_ ));
DESCR("geometric point '(x, y)'");
#define POINTOID		600
DATA(insert OID = 601 (  lseg	   PGUID 32  48 f b t \054 0 600 lseg_in lseg_out lseg_in lseg_out d _null_ ));
DESCR("geometric line segment '(pt1,pt2)'");
#define LSEGOID			601
DATA(insert OID = 602 (  path	   PGUID -1  -1 f b t \054 0 600 path_in path_out path_in path_out d _null_ ));
DESCR("geometric path '(pt1,...)'");
#define PATHOID			602
DATA(insert OID = 603 (  box	   PGUID 32 100 f b t \073 0 600 box_in box_out box_in box_out d _null_ ));
DESCR("geometric box '(lower left,upper right)'");
#define BOXOID			603
DATA(insert OID = 604 (  polygon   PGUID -1  -1 f b t \054 0   0 poly_in poly_out poly_in poly_out d _null_ ));
DESCR("geometric polygon '(pt1,...)'");
#define POLYGONOID		604
DATA(insert OID = 605 (  filename  PGUID 256 -1 f b t \054 0  18 filename_in filename_out filename_in filename_out i _null_ ));
DESCR("filename used in system tables");

DATA(insert OID = 628 (  line	   PGUID 32  48 f b t \054 0 701 line_in line_out line_in line_out d _null_ ));
DESCR("geometric line '(pt1,pt2)'");
#define LINEOID			628
DATA(insert OID = 629 (  _line	   PGUID  -1 -1 f b t \054 0 628 array_in array_out array_in array_out d _null_ ));
DESCR("");

/* OIDS 700 - 799 */

DATA(insert OID = 700 (  float4    PGUID  4  12 f b t \054 0   0 float4in float4out float4in float4out i _null_ ));
DESCR("single-precision floating point number, 4-byte");
#define FLOAT4OID 700
DATA(insert OID = 701 (  float8    PGUID  8  24 f b t \054 0   0 float8in float8out float8in float8out d _null_ ));
DESCR("double-precision floating point number, 8-byte");
#define FLOAT8OID 701
DATA(insert OID = 702 (  abstime   PGUID  4  20 t b t \054 0   0 nabstimein nabstimeout nabstimein nabstimeout i _null_ ));
DESCR("absolute, limited-range date and time (Unix system time)");
#define ABSTIMEOID		702
DATA(insert OID = 703 (  reltime   PGUID  4  20 t b t \054 0   0 reltimein reltimeout reltimein reltimeout i _null_ ));
DESCR("relative, limited-range time interval (Unix delta time)");
#define RELTIMEOID		703
DATA(insert OID = 704 (  tinterval PGUID 12  47 f b t \054 0   0 tintervalin tintervalout tintervalin tintervalout i _null_ ));
DESCR("time interval '(abstime,abstime)'");
DATA(insert OID = 705 (  unknown   PGUID -1  -1 f b t \054 0   18 textin textout textin textout i _null_ ));
DESCR("");
#define UNKNOWNOID		705

DATA(insert OID = 718 (  circle    PGUID  24 47 f b t \054 0	0 circle_in circle_out circle_in circle_out d _null_ ));
DESCR("geometric circle '(center,radius)'");
#define CIRCLEOID		718
DATA(insert OID = 719 (  _circle   PGUID  -1 -1 f b t \054 0  718 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 790 (  money	   PGUID   4 24 f b t \054 0	0 cash_in cash_out cash_in cash_out i _null_ ));
DESCR("money '$d,ddd.cc'");
#define CASHOID 790
DATA(insert OID = 791 (  _money    PGUID  -1 -1 f b t \054 0  790 array_in array_out array_in array_out i _null_ ));

/* OIDS 800 - 899 */
DATA(insert OID = 829 ( macaddr	   PGUID  6 -1 f b t \054 0	0 macaddr_in macaddr_out macaddr_in macaddr_out i _null_ ));
DESCR("MAC address");
DATA(insert OID = 869 ( inet	   PGUID  -1 -1 f b t \054 0 0 inet_in inet_out inet_in inet_out i _null_ ));
DESCR("Host address");
#define INETOID 869
DATA(insert OID = 650 ( cidr	   PGUID  -1 -1 f b t \054 0 0 cidr_in cidr_out cidr_in cidr_out i _null_ ));
DESCR("Network address");
#define CIDROID 650

/* OIDS 900 - 999 */

/* OIDS 1000 - 1099 */
DATA(insert OID = 1000 (  _bool		 PGUID -1  -1 f b t \054 0	16 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1001 (  _bytea	 PGUID -1  -1 f b t \054 0	17 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1002 (  _char		 PGUID -1  -1 f b t \054 0	18 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1003 (  _name		 PGUID -1  -1 f b t \054 0	19 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1005 (  _int2		 PGUID -1  -1 f b t \054 0	21 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1006 (  _int28	 PGUID -1  -1 f b t \054 0	22 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1007 (  _int4		 PGUID -1  -1 f b t \054 0	23 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1008 (  _regproc	 PGUID -1  -1 f b t \054 0	24 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1009 (  _text		 PGUID -1  -1 f b t \054 0	25 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1028 (  _oid		 PGUID -1  -1 f b t \054 0	26 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1010 (  _tid		 PGUID -1  -1 f b t \054 0	27 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1011 (  _xid		 PGUID -1  -1 f b t \054 0	28 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1012 (  _cid		 PGUID -1  -1 f b t \054 0	29 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1013 (  _oid8		 PGUID -1  -1 f b t \054 0	30 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1014 (  _bpchar	 PGUID -1  -1 f b t \054 0 1042 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1015 (  _varchar	 PGUID -1  -1 f b t \054 0 1043 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1017 (  _point	 PGUID -1  -1 f b t \054 0 600 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 1018 (  _lseg		 PGUID -1  -1 f b t \054 0 601 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 1019 (  _path		 PGUID -1  -1 f b t \054 0 602 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 1020 (  _box		 PGUID -1  -1 f b t \073 0 603 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 1021 (  _float4	 PGUID -1  -1 f b t \054 0 700 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1022 (  _float8	 PGUID -1  -1 f b t \054 0 701 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 1023 (  _abstime	 PGUID -1  -1 f b t \054 0 702 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1024 (  _reltime	 PGUID -1  -1 f b t \054 0 703 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1025 (  _tinterval PGUID -1  -1 f b t \054 0 704 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1026 (  _filename  PGUID -1  -1 f b t \054 0 605 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1027 (  _polygon	 PGUID -1  -1 f b t \054 0 604 array_in array_out array_in array_out d _null_ ));
/* Note: the size of an aclitem needs to match sizeof(AclItem) in acl.h */
DATA(insert OID = 1033 (  aclitem	 PGUID 8   -1 f b t \054 0 0 aclitemin aclitemout aclitemin aclitemout i _null_ ));
DESCR("access control list");
DATA(insert OID = 1034 (  _aclitem	 PGUID -1 -1 f b t \054 0 1033 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1040 (  _macaddr   PGUID -1 -1 f b t \054 0  829 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1041 (  _inet    PGUID -1 -1 f b t \054 0  869 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 651  (  _cidr    PGUID -1 -1 f b t \054 0  650 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1042 ( bpchar		 PGUID -1  -1 f b t \054 0	18 bpcharin bpcharout bpcharin bpcharout i _null_ ));
DESCR("blank-padded characters, length specifed when created");
#define BPCHAROID		1042
DATA(insert OID = 1043 ( varchar	 PGUID -1  -1 f b t \054 0	18 varcharin varcharout varcharin varcharout i _null_ ));
DESCR("non-blank-padded-length string, length specified when created");
#define VARCHAROID		1043

DATA(insert OID = 1082 ( date		 PGUID	4  10 t b t \054 0	0 date_in date_out date_in date_out i _null_ ));
DESCR("ANSI SQL date 'yyyy-mm-dd'");
#define DATEOID			1082
DATA(insert OID = 1083 ( time		 PGUID	8  16 f b t \054 0	0 time_in time_out time_in time_out d _null_ ));
DESCR("ANSI SQL time 'hh:mm:ss'");
#define TIMEOID			1083

/* OIDS 1100 - 1199 */
DATA(insert OID = 1182 ( _date		 PGUID	-1 -1 f b t \054 0	1082 array_in array_out array_in array_out i _null_ ));
DATA(insert OID = 1183 ( _time		 PGUID	-1 -1 f b t \054 0	1083 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 1184 ( datetime	 PGUID	8  47 f b t \054 0	0 datetime_in datetime_out datetime_in datetime_out d _null_ ));
DESCR("date and time 'yyyy-mm-dd hh:mm:ss'");
#define DATETIMEOID		1184
DATA(insert OID = 1185 ( _datetime	 PGUID	-1 -1 f b t \054 0	1184 array_in array_out array_in array_out d _null_ ));
DATA(insert OID = 1186 ( timespan	 PGUID 12  47 f b t \054 0	0 timespan_in timespan_out timespan_in timespan_out d _null_ ));
DESCR("time interval '@ <number> <units>'");
#define TIMESPANOID		1186
DATA(insert OID = 1187 ( _timespan	 PGUID	-1 -1 f b t \054 0	1186 array_in array_out array_in array_out d _null_ ));

/* OIDS 1200 - 1299 */
DATA(insert OID = 1296 ( timestamp	 PGUID	4  19 t b t \054 0	0 timestamp_in timestamp_out timestamp_in timestamp_out i _null_ ));
DESCR("limited-range ISO-format date and time");
#define TIMESTAMPOID	1296

/* OIDS 1700 - 1799 */
DATA(insert OID = 1700 ( numeric	   PGUID -1  -1 f b t \054 0  0 numeric_in numeric_out numeric_in numeric_out i _null_ ));
DESCR("arbitrary precision exact numeric data type");
#define NUMERICOID		1700


#define VARLENA_FIXED_SIZE(attr)	((attr)->atttypid == BPCHAROID && (attr)->atttypmod > 0)

/*
 * prototypes for functions in pg_type.c
 */
extern Oid	TypeGet(char *typeName, bool *defined);
extern Oid	TypeShellMake(char *typeName);
extern Oid TypeCreate(char *typeName,
		   Oid relationOid,
		   int16 internalSize,
		   int16 externalSize,
		   char typeType,
		   char typDelim,
		   char *inputProcedure,
		   char *outputProcedure,
		   char *receiveProcedure,
		   char *sendProcedure,
		   char *elementTypeName,
		   char *defaultTypeValue,
		   bool passedByValue, char alignment);
extern void TypeRename(char *oldTypeName, char *newTypeName);
extern char *makeArrayTypeName(char *typeName);


#endif	 /* PG_TYPE_H */
