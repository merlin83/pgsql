/*-------------------------------------------------------------------------
 *
 * acl.h
 *	  Definition of (and support for) access control list data structures.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: acl.h,v 1.44 2002-04-27 03:45:03 tgl Exp $
 *
 * NOTES
 *	  For backward-compatibility purposes we have to allow there
 *	  to be a null ACL in a pg_class tuple.  This will be defined as
 *	  meaning "default protection" (i.e., whatever acldefault() returns).
 *
 *	  The AclItems in an ACL array are currently kept in sorted order.
 *	  Things will break hard if you change that without changing the
 *	  code wherever this is included.
 *-------------------------------------------------------------------------
 */
#ifndef ACL_H
#define ACL_H

#include "nodes/parsenodes.h"
#include "utils/array.h"


/*
 * AclId		system identifier for the user, group, etc.
 *				XXX Perhaps replace this type by OID?
 */
typedef uint32 AclId;

#define ACL_ID_WORLD	0		/* placeholder for id in a WORLD acl item */

/*
 * AclIdType	tag that describes if the AclId is a user, group, etc.
 */
#define ACL_IDTYPE_WORLD		0x00	/* PUBLIC */
#define ACL_IDTYPE_UID			0x01	/* user id - from pg_shadow */
#define ACL_IDTYPE_GID			0x02	/* group id - from pg_group */

/*
 * AclMode		a bitmask of privilege bits
 */
typedef uint32 AclMode;

/*
 * AclItem
 *
 * Note: must be same size on all platforms, because the size is hardcoded
 * in the pg_type.h entry for aclitem.
 */
typedef struct AclItem
{
	AclId		ai_id;			/* ID that this item applies to */
	AclMode		ai_privs;		/* AclIdType plus privilege bits */
} AclItem;

/*
 * The AclIdType is stored in the top two bits of the ai_privs field of an
 * AclItem, leaving us with thirty usable privilege bits.
 */
#define ACLITEM_GET_PRIVS(item)   ((item).ai_privs & 0x3FFFFFFF)
#define ACLITEM_GET_IDTYPE(item)  ((item).ai_privs >> 30)
#define ACLITEM_SET_PRIVS_IDTYPE(item,privs,idtype) \
  ((item).ai_privs = ((privs) & 0x3FFFFFFF) | ((idtype) << 30))


/*
 * Definitions for convenient access to Acl (array of AclItem) and IdList
 * (array of AclId).  These are standard Postgres arrays, but are restricted
 * to have one dimension.  We also ignore the lower bound when reading,
 * and set it to zero when writing.
 *
 * CAUTION: as of Postgres 7.1, these arrays are toastable (just like all
 * other array types).	Therefore, be careful to detoast them with the
 * macros provided, unless you know for certain that a particular array
 * can't have been toasted.  Presently, we do not provide toast tables for
 * pg_class or pg_group, so the entries in those tables won't have been
 * stored externally --- but they could have been compressed!
 */


/*
 * Acl			a one-dimensional POSTGRES array of AclItem
 */
typedef ArrayType Acl;

#define ACL_NUM(ACL)			(ARR_DIMS(ACL)[0])
#define ACL_DAT(ACL)			((AclItem *) ARR_DATA_PTR(ACL))
#define ACL_N_SIZE(N)			(ARR_OVERHEAD(1) + ((N) * sizeof(AclItem)))
#define ACL_SIZE(ACL)			ARR_SIZE(ACL)

/*
 * IdList		a one-dimensional POSTGRES array of AclId
 */
typedef ArrayType IdList;

#define IDLIST_NUM(IDL)			(ARR_DIMS(IDL)[0])
#define IDLIST_DAT(IDL)			((AclId *) ARR_DATA_PTR(IDL))
#define IDLIST_N_SIZE(N)		(ARR_OVERHEAD(1) + ((N) * sizeof(AclId)))
#define IDLIST_SIZE(IDL)		ARR_SIZE(IDL)

/*
 * fmgr macros for these types
 */
#define DatumGetAclItemP(X)		   ((AclItem *) DatumGetPointer(X))
#define PG_GETARG_ACLITEM_P(n)	   DatumGetAclItemP(PG_GETARG_DATUM(n))
#define PG_RETURN_ACLITEM_P(x)	   PG_RETURN_POINTER(x)

#define DatumGetAclP(X)			   ((Acl *) PG_DETOAST_DATUM(X))
#define DatumGetAclPCopy(X)		   ((Acl *) PG_DETOAST_DATUM_COPY(X))
#define PG_GETARG_ACL_P(n)		   DatumGetAclP(PG_GETARG_DATUM(n))
#define PG_GETARG_ACL_P_COPY(n)    DatumGetAclPCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_ACL_P(x)		   PG_RETURN_POINTER(x)

#define DatumGetIdListP(X)		   ((IdList *) PG_DETOAST_DATUM(X))
#define DatumGetIdListPCopy(X)	   ((IdList *) PG_DETOAST_DATUM_COPY(X))
#define PG_GETARG_IDLIST_P(n)	   DatumGetIdListP(PG_GETARG_DATUM(n))
#define PG_GETARG_IDLIST_P_COPY(n) DatumGetIdListPCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_IDLIST_P(x)	   PG_RETURN_POINTER(x)


/*
 * ACL modification opcodes
 */
#define ACL_MODECHG_ADD			1
#define ACL_MODECHG_DEL			2
#define ACL_MODECHG_EQL			3

/* external representation of mode indicators for I/O */
#define ACL_MODECHG_ADD_CHR		'+'
#define ACL_MODECHG_DEL_CHR		'-'
#define ACL_MODECHG_EQL_CHR		'='

/*
 * External representations of the privilege bits --- aclitemin/aclitemout
 * represent each possible privilege bit with a distinct 1-character code
 */
#define ACL_INSERT_CHR			'a'		/* formerly known as "append" */
#define ACL_SELECT_CHR			'r'		/* formerly known as "read" */
#define ACL_UPDATE_CHR			'w'		/* formerly known as "write" */
#define ACL_DELETE_CHR			'd'
#define ACL_RULE_CHR			'R'
#define ACL_REFERENCES_CHR		'x'
#define ACL_TRIGGER_CHR			't'
#define ACL_EXECUTE_CHR			'X'
#define ACL_USAGE_CHR			'U'
#define ACL_CREATE_CHR			'C'
#define ACL_CREATE_TEMP_CHR		'T'

/* string holding all privilege code chars, in order by bitmask position */
#define ACL_ALL_RIGHTS_STR	"arwdRxtXUCT"

/*
 * Bitmasks defining "all rights" for each supported object type
 */
#define ACL_ALL_RIGHTS_RELATION		(ACL_INSERT|ACL_SELECT|ACL_UPDATE|ACL_DELETE|ACL_RULE|ACL_REFERENCES|ACL_TRIGGER)
#define ACL_ALL_RIGHTS_DATABASE		(ACL_CREATE|ACL_CREATE_TEMP)
#define ACL_ALL_RIGHTS_FUNCTION		(ACL_EXECUTE)
#define ACL_ALL_RIGHTS_LANGUAGE		(ACL_USAGE)
#define ACL_ALL_RIGHTS_NAMESPACE	(ACL_USAGE|ACL_CREATE)


/* result codes for pg_*_aclcheck */
typedef enum
{
	ACLCHECK_OK = 0,
	ACLCHECK_NO_PRIV,
	ACLCHECK_NOT_OWNER
} AclResult;

/*
 * routines used internally
 */
extern Acl *acldefault(GrantObjectType objtype, AclId ownerid);
extern Acl *aclinsert3(const Acl *old_acl, const AclItem *mod_aip,
					   unsigned modechg);

/*
 * SQL functions (from acl.c)
 */
extern Datum aclitemin(PG_FUNCTION_ARGS);
extern Datum aclitemout(PG_FUNCTION_ARGS);
extern Datum aclinsert(PG_FUNCTION_ARGS);
extern Datum aclremove(PG_FUNCTION_ARGS);
extern Datum aclcontains(PG_FUNCTION_ARGS);

/*
 * prototypes for functions in aclchk.c
 */
extern void ExecuteGrantStmt(GrantStmt *stmt);
extern AclId get_grosysid(char *groname);
extern char *get_groname(AclId grosysid);

extern AclResult pg_class_aclcheck(Oid table_oid, Oid userid, AclMode mode);
extern AclResult pg_database_aclcheck(Oid db_oid, Oid userid, AclMode mode);
extern AclResult pg_proc_aclcheck(Oid proc_oid, Oid userid, AclMode mode);
extern AclResult pg_language_aclcheck(Oid lang_oid, Oid userid, AclMode mode);
extern AclResult pg_namespace_aclcheck(Oid nsp_oid, Oid userid, AclMode mode);

extern void aclcheck_error(AclResult errcode, const char *objectname);

/* ownercheck routines just return true (owner) or false (not) */
extern bool pg_class_ownercheck(Oid class_oid, Oid userid);
extern bool pg_type_ownercheck(Oid type_oid, Oid userid);
extern bool pg_oper_ownercheck(Oid oper_oid, Oid userid);
extern bool pg_proc_ownercheck(Oid proc_oid, Oid userid);
extern bool pg_namespace_ownercheck(Oid nsp_oid, Oid userid);

#endif   /* ACL_H */
