/*-------------------------------------------------------------------------
 *
 * acl.h
 *	  Definition of (and support for) access control list data structures.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/utils/acl.h,v 1.76 2004/12/31 22:03:45 pgsql Exp $
 *
 * NOTES
 *	  An ACL array is simply an array of AclItems, representing the union
 *	  of the privileges represented by the individual items.  A zero-length
 *	  array represents "no privileges".  There are no assumptions about the
 *	  ordering of the items, but we do expect that there are no two entries
 *	  in the array with the same grantor and grantee.
 *
 *	  For backward-compatibility purposes we have to allow null ACL entries
 *	  in system catalogs.  A null ACL will be treated as meaning "default
 *	  protection" (i.e., whatever acldefault() returns).
 *-------------------------------------------------------------------------
 */
#ifndef ACL_H
#define ACL_H

#include "nodes/parsenodes.h"
#include "utils/array.h"


/*
 * typedef AclId is declared in c.h
 *
 * typedef AclMode is declared in parsenodes.h, also the individual privilege
 * bit meanings are defined there
 */

#define ACL_ID_WORLD	0		/* placeholder for id in a WORLD acl item */

/*
 * AclIdType	tag that describes if the AclId is a user, group, etc.
 */
#define ACL_IDTYPE_WORLD		0x00	/* PUBLIC */
#define ACL_IDTYPE_UID			0x01	/* user id - from pg_shadow */
#define ACL_IDTYPE_GID			0x02	/* group id - from pg_group */

/*
 * AclItem
 *
 * The IDTYPE included in ai_privs identifies the type of the grantee ID.
 * The grantor ID currently must always be a user, never a group.  (FIXME)
 *
 * Note: must be same size on all platforms, because the size is hardcoded
 * in the pg_type.h entry for aclitem.
 */
typedef struct AclItem
{
	AclId		ai_grantee;		/* ID that this item grants privs to */
	AclId		ai_grantor;		/* grantor of privs (always a user id) */
	AclMode		ai_privs;		/* AclIdType plus privilege bits */
} AclItem;

/*
 * The AclIdType is stored in the top two bits of the ai_privs field
 * of an AclItem.  The middle 15 bits are the grant option markers,
 * and the lower 15 bits are the actual privileges.  We use "rights"
 * to mean the combined grant option and privilege bits fields.
 */
#define ACLITEM_GET_PRIVS(item)    ((item).ai_privs & 0x7FFF)
#define ACLITEM_GET_GOPTIONS(item) (((item).ai_privs >> 15) & 0x7FFF)
#define ACLITEM_GET_RIGHTS(item)   ((item).ai_privs & 0x3FFFFFFF)
#define ACLITEM_GET_IDTYPE(item)   ((item).ai_privs >> 30)

#define ACL_GRANT_OPTION_FOR(privs) (((AclMode) (privs) & 0x7FFF) << 15)
#define ACL_OPTION_TO_PRIVS(privs)	(((AclMode) (privs) >> 15) & 0x7FFF)

#define ACLITEM_SET_PRIVS(item,privs) \
  ((item).ai_privs = ((item).ai_privs & ~((AclMode) 0x7FFF)) | \
					 ((AclMode) (privs) & 0x7FFF))
#define ACLITEM_SET_GOPTIONS(item,goptions) \
  ((item).ai_privs = ((item).ai_privs & ~(((AclMode) 0x7FFF) << 15)) | \
					 (((AclMode) (goptions) & 0x7FFF) << 15))
#define ACLITEM_SET_RIGHTS(item,rights) \
  ((item).ai_privs = ((item).ai_privs & ~((AclMode) 0x3FFFFFFF)) | \
					 ((AclMode) (rights) & 0x3FFFFFFF))
#define ACLITEM_SET_IDTYPE(item,idtype) \
  ((item).ai_privs = ((item).ai_privs & ~(((AclMode) 0x03) << 30)) | \
					 (((AclMode) (idtype) & 0x03) << 30))

#define ACLITEM_SET_PRIVS_IDTYPE(item,privs,goption,idtype) \
  ((item).ai_privs = ((AclMode) (privs) & 0x7FFF) | \
					 (((AclMode) (goption) & 0x7FFF) << 15) | \
					 ((AclMode) (idtype) << 30))

#define ACLITEM_ALL_PRIV_BITS		((AclMode) 0x7FFF)
#define ACLITEM_ALL_GOPTION_BITS	((AclMode) 0x7FFF << 15)

/*
 * Definitions for convenient access to Acl (array of AclItem) and IdList
 * (array of AclId).  These are standard PostgreSQL arrays, but are restricted
 * to have one dimension.  We also ignore the lower bound when reading,
 * and set it to one when writing.
 *
 * CAUTION: as of PostgreSQL 7.1, these arrays are toastable (just like all
 * other array types).	Therefore, be careful to detoast them with the
 * macros provided, unless you know for certain that a particular array
 * can't have been toasted.  Presently, we do not provide toast tables for
 * pg_class or pg_group, so the entries in those tables won't have been
 * stored externally --- but they could have been compressed!
 */


/*
 * Acl			a one-dimensional array of AclItem
 */
typedef ArrayType Acl;

#define ACL_NUM(ACL)			(ARR_DIMS(ACL)[0])
#define ACL_DAT(ACL)			((AclItem *) ARR_DATA_PTR(ACL))
#define ACL_N_SIZE(N)			(ARR_OVERHEAD(1) + ((N) * sizeof(AclItem)))
#define ACL_SIZE(ACL)			ARR_SIZE(ACL)

/*
 * IdList		a one-dimensional array of AclId
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
 * ACL modification opcodes for aclupdate
 */
#define ACL_MODECHG_ADD			1
#define ACL_MODECHG_DEL			2
#define ACL_MODECHG_EQL			3

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
#define ACL_ALL_RIGHTS_TABLESPACE	(ACL_CREATE)

/* operation codes for pg_*_aclmask */
typedef enum
{
	ACLMASK_ALL,				/* normal case: compute all bits */
	ACLMASK_ANY					/* return when result is known nonzero */
} AclMaskHow;

/* result codes for pg_*_aclcheck */
typedef enum
{
	ACLCHECK_OK = 0,
	ACLCHECK_NO_PRIV,
	ACLCHECK_NOT_OWNER
} AclResult;

/* this enum covers all object types that can have privilege errors */
/* currently it's only used to tell aclcheck_error what to say */
typedef enum AclObjectKind
{
	ACL_KIND_CLASS,				/* pg_class */
	ACL_KIND_DATABASE,			/* pg_database */
	ACL_KIND_PROC,				/* pg_proc */
	ACL_KIND_OPER,				/* pg_operator */
	ACL_KIND_TYPE,				/* pg_type */
	ACL_KIND_LANGUAGE,			/* pg_language */
	ACL_KIND_NAMESPACE,			/* pg_namespace */
	ACL_KIND_OPCLASS,			/* pg_opclass */
	ACL_KIND_CONVERSION,		/* pg_conversion */
	ACL_KIND_TABLESPACE,		/* pg_tablespace */
	MAX_ACL_KIND				/* MUST BE LAST */
} AclObjectKind;

/*
 * routines used internally
 */
extern Acl *acldefault(GrantObjectType objtype, AclId ownerid);
extern Acl *aclupdate(const Acl *old_acl, const AclItem *mod_aip,
		  int modechg, AclId ownerid, DropBehavior behavior);
extern Acl *aclnewowner(const Acl *old_acl, AclId oldownerid, AclId newownerid);

extern AclMode aclmask(const Acl *acl, AclId userid, AclId ownerid,
		AclMode mask, AclMaskHow how);

/*
 * SQL functions (from acl.c)
 */
extern Datum aclitemin(PG_FUNCTION_ARGS);
extern Datum aclitemout(PG_FUNCTION_ARGS);
extern Datum aclinsert(PG_FUNCTION_ARGS);
extern Datum aclremove(PG_FUNCTION_ARGS);
extern Datum aclcontains(PG_FUNCTION_ARGS);
extern Datum makeaclitem(PG_FUNCTION_ARGS);
extern Datum aclitem_eq(PG_FUNCTION_ARGS);
extern Datum hash_aclitem(PG_FUNCTION_ARGS);

/*
 * prototypes for functions in aclchk.c
 */
extern void ExecuteGrantStmt(GrantStmt *stmt);
extern AclId get_grosysid(char *groname);
extern char *get_groname(AclId grosysid);

extern AclMode pg_class_aclmask(Oid table_oid, AclId userid,
				 AclMode mask, AclMaskHow how);
extern AclMode pg_database_aclmask(Oid db_oid, AclId userid,
					AclMode mask, AclMaskHow how);
extern AclMode pg_proc_aclmask(Oid proc_oid, AclId userid,
				AclMode mask, AclMaskHow how);
extern AclMode pg_language_aclmask(Oid lang_oid, AclId userid,
					AclMode mask, AclMaskHow how);
extern AclMode pg_namespace_aclmask(Oid nsp_oid, AclId userid,
					 AclMode mask, AclMaskHow how);
extern AclMode pg_tablespace_aclmask(Oid spc_oid, AclId userid,
					  AclMode mask, AclMaskHow how);

extern AclResult pg_class_aclcheck(Oid table_oid, AclId userid, AclMode mode);
extern AclResult pg_database_aclcheck(Oid db_oid, AclId userid, AclMode mode);
extern AclResult pg_proc_aclcheck(Oid proc_oid, AclId userid, AclMode mode);
extern AclResult pg_language_aclcheck(Oid lang_oid, AclId userid, AclMode mode);
extern AclResult pg_namespace_aclcheck(Oid nsp_oid, AclId userid, AclMode mode);
extern AclResult pg_tablespace_aclcheck(Oid spc_oid, AclId userid, AclMode mode);

extern void aclcheck_error(AclResult aclerr, AclObjectKind objectkind,
			   const char *objectname);

/* ownercheck routines just return true (owner) or false (not) */
extern bool pg_class_ownercheck(Oid class_oid, AclId userid);
extern bool pg_type_ownercheck(Oid type_oid, AclId userid);
extern bool pg_oper_ownercheck(Oid oper_oid, AclId userid);
extern bool pg_proc_ownercheck(Oid proc_oid, AclId userid);
extern bool pg_namespace_ownercheck(Oid nsp_oid, AclId userid);
extern bool pg_tablespace_ownercheck(Oid spc_oid, AclId userid);
extern bool pg_opclass_ownercheck(Oid opc_oid, AclId userid);
extern bool pg_database_ownercheck(Oid db_oid, AclId userid);
extern bool pg_conversion_ownercheck(Oid conv_oid, AclId userid);

#endif   /* ACL_H */
