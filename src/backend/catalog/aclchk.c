/*-------------------------------------------------------------------------
 *
 * aclchk.c
 *	  Routines to check access control permissions.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/aclchk.c,v 1.40 2000-09-06 14:15:15 petere Exp $
 *
 * NOTES
 *	  See acl.h.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_aggregate.h"
#include "catalog/pg_group.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_shadow.h"
#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "parser/parse_agg.h"
#include "parser/parse_func.h"
#include "utils/acl.h"
#include "utils/syscache.h"

static int32 aclcheck(char *relname, Acl *acl, AclId id,
					  AclIdType idtype, AclMode mode);

/*
 * Enable use of user relations in place of real system catalogs.
 */
/*#define ACLDEBUG*/

#ifdef ACLDEBUG
/*
 * Fool the code below into thinking that "pgacls" is pg_class.
 * relname and relowner are in the same place, happily.
 */
#undef	Anum_pg_class_relacl
#define Anum_pg_class_relacl			3
#undef	Natts_pg_class
#define Natts_pg_class					3
#undef	Name_pg_class
#define Name_pg_class					"pgacls"
#undef	Name_pg_group
#define Name_pg_group					"pggroup"
#endif

/* warning messages, now more explicit. */
/* should correspond to the order of the ACLCHK_* result codes above. */
char	   *aclcheck_error_strings[] = {
	"No error.",
	"Permission denied.",
	"Table does not exist.",
	"Must be table owner."
};

#ifdef ACLDEBUG_TRACE
static
dumpacl(Acl *acl)
{
	int			i;
	AclItem    *aip;

	elog(DEBUG, "acl size = %d, # acls = %d",
		 ACL_SIZE(acl), ACL_NUM(acl));
	aip = ACL_DAT(acl);
	for (i = 0; i < ACL_NUM(acl); ++i)
		elog(DEBUG, "	acl[%d]: %s", i,
			 DatumGetCString(DirectFunctionCall1(aclitemout,
												 PointerGetDatum(aip + i))));
}

#endif

/*
 *
 */
void
ChangeAcl(char *relname,
		  AclItem *mod_aip,
		  unsigned modechg)
{
	unsigned	i;
	Acl		   *old_acl,
			   *new_acl;
	Relation	relation;
	HeapTuple	tuple;
	Datum		values[Natts_pg_class];
	char		nulls[Natts_pg_class];
	char		replaces[Natts_pg_class];
	Relation	idescs[Num_pg_class_indices];
	bool		isNull;
	bool		free_old_acl = false;

	/*
	 * Find the pg_class tuple matching 'relname' and extract the ACL. If
	 * there's no ACL, create a default using the pg_class.relowner field.
	 */
	relation = heap_openr(RelationRelationName, RowExclusiveLock);
	tuple = SearchSysCacheTuple(RELNAME,
								PointerGetDatum(relname),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		heap_close(relation, RowExclusiveLock);
		elog(ERROR, "ChangeAcl: class \"%s\" not found",
			 relname);
	}

	old_acl = (Acl *) heap_getattr(tuple,
								   Anum_pg_class_relacl,
								   RelationGetDescr(relation),
								   &isNull);
	if (isNull)
	{
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "ChangeAcl: using default ACL");
#endif
		old_acl = acldefault(relname);
		free_old_acl = true;
	}

	/* Need to detoast the old ACL for modification */
	old_acl = DatumGetAclP(PointerGetDatum(old_acl));

	if (ACL_NUM(old_acl) < 1)
	{
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "ChangeAcl: old ACL has zero length");
#endif
		old_acl = acldefault(relname);
		free_old_acl = true;
	}

#ifdef ACLDEBUG_TRACE
	dumpacl(old_acl);
#endif

	new_acl = aclinsert3(old_acl, mod_aip, modechg);

#ifdef ACLDEBUG_TRACE
	dumpacl(new_acl);
#endif

	for (i = 0; i < Natts_pg_class; ++i)
	{
		replaces[i] = ' ';
		nulls[i] = ' ';			/* ignored if replaces[i] == ' ' anyway */
		values[i] = (Datum) NULL;		/* ignored if replaces[i] == ' '
										 * anyway */
	}
	replaces[Anum_pg_class_relacl - 1] = 'r';
	values[Anum_pg_class_relacl - 1] = PointerGetDatum(new_acl);
	tuple = heap_modifytuple(tuple, relation, values, nulls, replaces);

	heap_update(relation, &tuple->t_self, tuple, NULL);

	/* keep the catalog indices up to date */
	CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices,
					   idescs);
	CatalogIndexInsert(idescs, Num_pg_class_indices, relation, tuple);
	CatalogCloseIndices(Num_pg_class_indices, idescs);

	heap_close(relation, RowExclusiveLock);
	if (free_old_acl)
		pfree(old_acl);
	pfree(new_acl);
}

AclId
get_grosysid(char *groname)
{
	HeapTuple	tuple;
	AclId		id = 0;

	tuple = SearchSysCacheTuple(GRONAME,
								PointerGetDatum(groname),
								0, 0, 0);
	if (HeapTupleIsValid(tuple))
		id = ((Form_pg_group) GETSTRUCT(tuple))->grosysid;
	else
		elog(ERROR, "non-existent group \"%s\"", groname);
	return id;
}

char *
get_groname(AclId grosysid)
{
	HeapTuple	tuple;
	char	   *name = NULL;

	tuple = SearchSysCacheTuple(GROSYSID,
								ObjectIdGetDatum(grosysid),
								0, 0, 0);
	if (HeapTupleIsValid(tuple))
		name = NameStr(((Form_pg_group) GETSTRUCT(tuple))->groname);
	else
		elog(NOTICE, "get_groname: group %u not found", grosysid);
	return name;
}

static bool
in_group(AclId uid, AclId gid)
{
	Relation	relation;
	HeapTuple	tuple;
	Acl		   *tmp;
	int			i,
				num;
	AclId	   *aidp;
	bool		found = false;

	relation = heap_openr(GroupRelationName, RowExclusiveLock);
	tuple = SearchSysCacheTuple(GROSYSID,
								ObjectIdGetDatum(gid),
								0, 0, 0);
	if (HeapTupleIsValid(tuple) &&
		!heap_attisnull(tuple, Anum_pg_group_grolist))
	{
		tmp = (IdList *) heap_getattr(tuple,
									  Anum_pg_group_grolist,
									  RelationGetDescr(relation),
									  (bool *) NULL);
		/* be sure the IdList is not toasted */
		tmp = DatumGetIdListP(PointerGetDatum(tmp));
		/* XXX make me a function */
		num = IDLIST_NUM(tmp);
		aidp = IDLIST_DAT(tmp);
		for (i = 0; i < num; ++i)
			if (aidp[i] == uid)
			{
				found = true;
				break;
			}
	}
	else
		elog(NOTICE, "in_group: group %d not found", gid);
	heap_close(relation, RowExclusiveLock);
	return found;
}

/*
 * aclcheck
 * Returns 1 if the 'id' of type 'idtype' has ACL entries in 'acl' to satisfy
 * any one of the requirements of 'mode'.  Returns 0 otherwise.
 */
static int32
aclcheck(char *relname, Acl *acl, AclId id, AclIdType idtype, AclMode mode)
{
	unsigned	i;
	AclItem    *aip,
			   *aidat;
	unsigned	num,
				found_group;

	/* if no acl is found, use world default */
	if (!acl)
		acl = acldefault(relname);

	num = ACL_NUM(acl);
	aidat = ACL_DAT(acl);

	/*
	 * We'll treat the empty ACL like that, too, although this is more
	 * like an error (i.e., you manually blew away your ACL array) -- the
	 * system never creates an empty ACL.
	 */
	if (num < 1)
	{
#if defined(ACLDEBUG_TRACE) || 1
		elog(DEBUG, "aclcheck: zero-length ACL, returning 1");
#endif
		return ACLCHECK_OK;
	}

	switch (idtype)
	{
		case ACL_IDTYPE_UID:
			for (i = 1, aip = aidat + 1;		/* skip world entry */
				 i < num && aip->ai_idtype == ACL_IDTYPE_UID;
				 ++i, ++aip)
			{
				if (aip->ai_id == id)
				{
#ifdef ACLDEBUG_TRACE
					elog(DEBUG, "aclcheck: found %d/%d",
						 aip->ai_id, aip->ai_mode);
#endif
					return (aip->ai_mode & mode) ? ACLCHECK_OK : ACLCHECK_NO_PRIV;
				}
			}
			for (found_group = 0;
				 i < num && aip->ai_idtype == ACL_IDTYPE_GID;
				 ++i, ++aip)
			{
				if (in_group(id, aip->ai_id))
				{
					if (aip->ai_mode & mode)
					{
						found_group = 1;
						break;
					}
				}
			}
			if (found_group)
			{
#ifdef ACLDEBUG_TRACE
				elog(DEBUG, "aclcheck: all groups ok");
#endif
				return ACLCHECK_OK;
			}
			break;
		case ACL_IDTYPE_GID:
			for (i = 1, aip = aidat + 1;		/* skip world entry and
												 * UIDs */
				 i < num && aip->ai_idtype == ACL_IDTYPE_UID;
				 ++i, ++aip)
				;
			for (;
				 i < num && aip->ai_idtype == ACL_IDTYPE_GID;
				 ++i, ++aip)
			{
				if (aip->ai_id == id)
				{
#ifdef ACLDEBUG_TRACE
					elog(DEBUG, "aclcheck: found %d/%d",
						 aip->ai_id, aip->ai_mode);
#endif
					return (aip->ai_mode & mode) ? ACLCHECK_OK : ACLCHECK_NO_PRIV;
				}
			}
			break;
		case ACL_IDTYPE_WORLD:
			break;
		default:
			elog(ERROR, "aclcheck: bogus ACL id type: %d", idtype);
			break;
	}

#ifdef ACLDEBUG_TRACE
	elog(DEBUG, "aclcheck: using world=%d", aidat->ai_mode);
#endif
	return (aidat->ai_mode & mode) ? ACLCHECK_OK : ACLCHECK_NO_PRIV;
}

int32
pg_aclcheck(char *relname, Oid userid, AclMode mode)
{
	HeapTuple	tuple;
	Acl		   *acl = (Acl *) NULL;
	int32		result;
	char       *usename;
	Relation	relation;

	tuple = SearchSysCacheTuple(SHADOWSYSID,
								ObjectIdGetDatum(userid),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "pg_aclcheck: invalid user id %u",
			 (unsigned) userid);

	usename = NameStr(((Form_pg_shadow) GETSTRUCT(tuple))->usename);

	/*
	 * Deny anyone permission to update a system catalog unless
	 * pg_shadow.usecatupd is set.	(This is to let superusers protect
	 * themselves from themselves.)
	 */
	if (((mode & ACL_WR) || (mode & ACL_AP)) &&
		!allowSystemTableMods && IsSystemRelationName(relname) &&
		strncmp(relname, "pg_temp.", strlen("pg_temp.")) != 0 &&
		!((Form_pg_shadow) GETSTRUCT(tuple))->usecatupd)
	{
		elog(DEBUG, "pg_aclcheck: catalog update to \"%s\": permission denied",
			 relname);
		return ACLCHECK_NO_PRIV;
	}

	/*
	 * Otherwise, superusers bypass all permission-checking.
	 */
	if (((Form_pg_shadow) GETSTRUCT(tuple))->usesuper)
	{
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "pg_aclcheck: \"%s\" is superuser",
			 usename);
#endif
		return ACLCHECK_OK;
	}

#ifndef ACLDEBUG
	relation = heap_openr(RelationRelationName, RowExclusiveLock);
	tuple = SearchSysCacheTuple(RELNAME,
								PointerGetDatum(relname),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		elog(ERROR, "pg_aclcheck: class \"%s\" not found",
			 relname);
	}
	if (!heap_attisnull(tuple, Anum_pg_class_relacl))
	{
		/* get a detoasted copy of the ACL */
		acl = DatumGetAclPCopy(heap_getattr(tuple,
											Anum_pg_class_relacl,
											RelationGetDescr(relation),
											(bool *) NULL));
	}
	else
	{

		/*
		 * if the acl is null, by default the owner can do whatever he
		 * wants to with it
		 */
		AclId		ownerId;

		ownerId = ((Form_pg_class) GETSTRUCT(tuple))->relowner;
		acl = aclownerdefault(relname, ownerId);
	}
	heap_close(relation, RowExclusiveLock);
#else
	relation = heap_openr(RelationRelationName, RowExclusiveLock);
	tuple = SearchSysCacheTuple(RELNAME,
								PointerGetDatum(relname),
								0, 0, 0);
	if (HeapTupleIsValid(tuple) &&
		!heap_attisnull(tuple, Anum_pg_class_relacl))
	{
		/* get a detoasted copy of the ACL */
		acl = DatumGetAclPCopy(heap_getattr(tuple,
											Anum_pg_class_relacl,
											RelationGetDescr(relation),
											(bool *) NULL));
	}
	heap_close(relation, RowExclusiveLock);
#endif
	result = aclcheck(relname, acl, userid, (AclIdType) ACL_IDTYPE_UID, mode);
	if (acl)
		pfree(acl);
	return result;
}

int32
pg_ownercheck(Oid userid,
			  const char *value,
			  int cacheid)
{
	HeapTuple	tuple;
	AclId		owner_id = 0;
	char       *usename;

	tuple = SearchSysCacheTuple(SHADOWSYSID,
								ObjectIdGetDatum(userid),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "pg_ownercheck: invalid user id %u",
			 (unsigned) userid);
	usename = NameStr(((Form_pg_shadow) GETSTRUCT(tuple))->usename);

	/*
	 * Superusers bypass all permission-checking.
	 */
	if (((Form_pg_shadow) GETSTRUCT(tuple))->usesuper)
	{
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "pg_ownercheck: user \"%s\" is superuser",
			 usename);
#endif
		return 1;
	}

	tuple = SearchSysCacheTuple(cacheid, PointerGetDatum(value),
								0, 0, 0);
	switch (cacheid)
	{
		case OPEROID:
			if (!HeapTupleIsValid(tuple))
				elog(ERROR, "pg_ownercheck: operator %ld not found",
					 PointerGetDatum(value));
			owner_id = ((Form_pg_operator) GETSTRUCT(tuple))->oprowner;
			break;
		case PROCNAME:
			if (!HeapTupleIsValid(tuple))
				elog(ERROR, "pg_ownercheck: function \"%s\" not found",
					 value);
			owner_id = ((Form_pg_proc) GETSTRUCT(tuple))->proowner;
			break;
		case RELNAME:
			if (!HeapTupleIsValid(tuple))
				elog(ERROR, "pg_ownercheck: class \"%s\" not found",
					 value);
			owner_id = ((Form_pg_class) GETSTRUCT(tuple))->relowner;
			break;
		case TYPENAME:
			if (!HeapTupleIsValid(tuple))
				elog(ERROR, "pg_ownercheck: type \"%s\" not found",
					 value);
			owner_id = ((Form_pg_type) GETSTRUCT(tuple))->typowner;
			break;
		default:
			elog(ERROR, "pg_ownercheck: invalid cache id: %d", cacheid);
			break;
	}

	return userid == owner_id;
}

int32
pg_func_ownercheck(Oid userid,
				   char *funcname,
				   int nargs,
				   Oid *arglist)
{
	HeapTuple	tuple;
	AclId		owner_id;
	char *username;

	tuple = SearchSysCacheTuple(SHADOWSYSID,
								ObjectIdGetDatum(userid),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "pg_func_ownercheck: invalid user id %u",
			 (unsigned) userid);
	username = NameStr(((Form_pg_shadow) GETSTRUCT(tuple))->usename);

	/*
	 * Superusers bypass all permission-checking.
	 */
	if (((Form_pg_shadow) GETSTRUCT(tuple))->usesuper)
	{
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "pg_ownercheck: user \"%s\" is superuser",
			 username);
#endif
		return 1;
	}

	tuple = SearchSysCacheTuple(PROCNAME,
								PointerGetDatum(funcname),
								Int32GetDatum(nargs),
								PointerGetDatum(arglist),
								0);
	if (!HeapTupleIsValid(tuple))
		func_error("pg_func_ownercheck", funcname, nargs, arglist, NULL);

	owner_id = ((Form_pg_proc) GETSTRUCT(tuple))->proowner;

	return userid == owner_id;
}

int32
pg_aggr_ownercheck(Oid userid,
				   char *aggname,
				   Oid basetypeID)
{
	HeapTuple	tuple;
	AclId		owner_id;
	char *username;

	tuple = SearchSysCacheTuple(SHADOWSYSID,
								PointerGetDatum(userid),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "pg_aggr_ownercheck: invalid user id %u",
			 (unsigned) userid);
	username = NameStr(((Form_pg_shadow) GETSTRUCT(tuple))->usename);

	/*
	 * Superusers bypass all permission-checking.
	 */
	if (((Form_pg_shadow) GETSTRUCT(tuple))->usesuper)
	{
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "pg_aggr_ownercheck: user \"%s\" is superuser",
			 username);
#endif
		return 1;
	}

	tuple = SearchSysCacheTuple(AGGNAME,
								PointerGetDatum(aggname),
								ObjectIdGetDatum(basetypeID),
								0, 0);

	if (!HeapTupleIsValid(tuple))
		agg_error("pg_aggr_ownercheck", aggname, basetypeID);

	owner_id = ((Form_pg_aggregate) GETSTRUCT(tuple))->aggowner;

	return userid == owner_id;
}
