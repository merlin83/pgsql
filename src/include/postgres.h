/*-------------------------------------------------------------------------
 *
 * postgres.h
 *	  definition of (and support for) postgres system types.
 * this file is included by almost every .c in the system
 *
 * Copyright (c) 1995, Regents of the University of California
 *
 * $Id: postgres.h,v 1.22 1999-05-03 19:10:14 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *	 NOTES
 *		this file will eventually contain the definitions for the
 *		following (and perhaps other) system types:
 *
 *				int2	   int4		  float4	   float8
 *				Oid		   regproc	  RegProcedure
 *				aclitem
 *				struct varlena
 *				int28	  oid8
 *				bytea	   text
 *				NameData   Name
 *
 *	 TABLE OF CONTENTS
 *		1)		simple type definitions
 *		2)		varlena and array types
 *		3)		TransactionId and CommandId
 *		4)		genbki macros used by catalog/pg_xxx.h files
 *		5)		random CSIGNBIT, MAXPGPATH, STATUS macros
 *
 * ----------------------------------------------------------------
 */
#ifndef POSTGRES_H
#define POSTGRES_H

#include "postgres_ext.h"
#ifndef WIN32
#include "config.h"
#endif
#include "c.h"
#include "utils/elog.h"
#include "utils/palloc.h"

/* ----------------------------------------------------------------
 *				Section 1:	simple type definitions
 * ----------------------------------------------------------------
 */

typedef int16 int2;
typedef int32 int4;
typedef float float4;
typedef double float8;

typedef int4 aclitem;

#define InvalidOid		0
#define OidIsValid(objectId)  ((bool) (objectId != InvalidOid))

/* unfortunately, both regproc and RegProcedure are used */
typedef Oid regproc;
typedef Oid RegProcedure;

/* ptr to func returning (char *) */
typedef char *((*func_ptr) ());


#define RegProcedureIsValid(p)	OidIsValid(p)

/* ----------------------------------------------------------------
 *				Section 2:	variable length and array types
 * ----------------------------------------------------------------
 */
/* ----------------
 *		struct varlena
 * ----------------
 */
struct varlena
{
	int32		vl_len;
	char		vl_dat[1];
};

#define VARSIZE(PTR)	(((struct varlena *)(PTR))->vl_len)
#define VARDATA(PTR)	(((struct varlena *)(PTR))->vl_dat)
#define VARHDRSZ		sizeof(int32)

typedef struct varlena bytea;
typedef struct varlena text;

typedef int2 int28[8];
typedef Oid oid8[8];

/* We want NameData to have length NAMEDATALEN and int alignment,
 * because that's how the data type 'name' is defined in pg_type.
 * Use a union to make sure the compiler agrees.
 */
typedef union nameData
{
	char		data[NAMEDATALEN];
	int			alignmentDummy;
} NameData;
typedef NameData *Name;

/* ----------------------------------------------------------------
 *				Section 3: TransactionId and CommandId
 * ----------------------------------------------------------------
 */

typedef uint32 TransactionId;

#define InvalidTransactionId	0
typedef uint32 CommandId;

#define FirstCommandId	0

/* ----------------------------------------------------------------
 *				Section 4: genbki macros used by the
 *						   catalog/pg_xxx.h files
 * ----------------------------------------------------------------
 */
#define CATALOG(x) \
	typedef struct CppConcat(FormData_,x)

#define DATA(x) extern int errno
#define DESCR(x) extern int errno
#define DECLARE_INDEX(x) extern int errno

#define BUILD_INDICES
#define BOOTSTRAP

#define BKI_BEGIN
#define BKI_END

/* ----------------------------------------------------------------
 *				Section 5:	random stuff
 *							CSIGNBIT, MAXPGPATH, STATUS...
 * ----------------------------------------------------------------
 */

/* msb for int/unsigned */
#define ISIGNBIT (0x80000000)
#define WSIGNBIT (0x8000)

/* msb for char */
#define CSIGNBIT (0x80)

/* ----------------
 *		global variables which should probably go someplace else.
 * ----------------
 */
#define MAXPGPATH		128
#define MAX_QUERY_SIZE	(BLCKSZ*2)

#define STATUS_OK				(0)
#define STATUS_ERROR			(-1)
#define STATUS_NOT_FOUND		(-2)
#define STATUS_INVALID			(-3)
#define STATUS_UNCATALOGUED		(-4)
#define STATUS_REPLACED			(-5)
#define STATUS_NOT_DONE			(-6)
#define STATUS_BAD_PACKET		(-7)
#define STATUS_FOUND			(1)

/* ---------------
 * Cyrillic on the fly charsets recode
 * ---------------
 */
#ifdef CYR_RECODE
void		SetCharSet();

#endif	 /* CYR_RECODE */

#endif	 /* POSTGRES_H */
