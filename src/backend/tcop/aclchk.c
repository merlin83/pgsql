/*-------------------------------------------------------------------------
 *
 * aclchk.c--
 *    Routines to check access control permissions.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/tcop/Attic/aclchk.c,v 1.3 1996-11-03 06:52:29 scrappy Exp $
 *
 * NOTES
 *    See acl.h.
 *
 *-------------------------------------------------------------------------
 */
#include <string.h>
#include "postgres.h"

#include "utils/acl.h"		/* where declarations for this file goes */
#include "access/heapam.h"
#include "access/htup.h"
#include "access/tupmacs.h"
#include "utils/builtins.h"
#include "utils/palloc.h"
#include "catalog/indexing.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/pg_group.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_user.h"
#include "utils/syscache.h"
#include "parser/catalog_utils.h"
#include "fmgr.h"

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
#define	Anum_pg_class_relacl		3
#undef	Natts_pg_class
#define	Natts_pg_class			3
#undef	Name_pg_class
#define	Name_pg_class			"pgacls"
#undef	Name_pg_group
#define	Name_pg_group			"pggroup"
#endif

#ifdef ACLDEBUG_TRACE
static
dumpacl(Acl *acl)
{
    register unsigned i;
    AclItem *aip;
	
    elog(DEBUG, "acl size = %d, # acls = %d",
	 ACL_SIZE(acl), ACL_NUM(acl));
    aip = (AclItem *) ACL_DAT(acl);
    for (i = 0; i < ACL_NUM(acl); ++i)
	elog(DEBUG, "	acl[%d]: %s", i, aclitemout(aip + i));
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
    register unsigned i;
    Acl *old_acl = (Acl *) NULL, *new_acl;
    Relation relation;
    static ScanKeyData relkey[1] = {
	{ 0, Anum_pg_class_relname, NameEqualRegProcedure }
    };
    HeapScanDesc hsdp;
    HeapTuple htp;
    Buffer buffer;
    Datum values[Natts_pg_class];
    char nulls[Natts_pg_class];
    char replaces[Natts_pg_class];
    ItemPointerData tmp_ipd;
    Relation idescs[Num_pg_class_indices];
    int free_old_acl = 0;

    /*
     * Find the pg_class tuple matching 'relname' and extract the ACL.
     * If there's no ACL, create a default using the pg_class.relowner
     * field.
     *
     * We can't use the syscache here, since we need to do a heap_replace
     * on the tuple we find.  Feh.
     */
    relation = heap_openr(RelationRelationName);
    if (!RelationIsValid(relation))
	elog(WARN, "ChangeAcl: could not open '%s'??",
	     RelationRelationName);
    fmgr_info(NameEqualRegProcedure, &relkey[0].sk_func, &relkey[0].sk_nargs);
    relkey[0].sk_argument = NameGetDatum(relname);
    hsdp = heap_beginscan(relation,
			  0,
			  NowTimeQual,
			  (unsigned) 1,
			  relkey);
    htp = heap_getnext(hsdp, 0, &buffer);
    if (!HeapTupleIsValid(htp)) {
	heap_endscan(hsdp);
	heap_close(relation);
	elog(WARN, "ChangeAcl: class \"%s\" not found",
	     relname);
	return;
    }
    if (!heap_attisnull(htp, Anum_pg_class_relacl))
	old_acl = (Acl *) heap_getattr(htp, buffer,
				       Anum_pg_class_relacl,
				       RelationGetTupleDescriptor(relation),
				       (bool *) NULL);
    if (!old_acl || ACL_NUM(old_acl) < 1) {
#ifdef ACLDEBUG_TRACE
	elog(DEBUG, "ChangeAcl: using default ACL");
#endif
/*	old_acl = acldefault(((Form_pg_class) GETSTRUCT(htp))->relowner); */
	old_acl = acldefault();
	free_old_acl = 1;
    }

#ifdef ACLDEBUG_TRACE
    dumpacl(old_acl);
#endif
    new_acl = aclinsert3(old_acl, mod_aip, modechg);
#ifdef ACLDEBUG_TRACE
    dumpacl(new_acl);
#endif

    for (i = 0; i < Natts_pg_class; ++i) {
	replaces[i] = ' ';
	nulls[i] = ' ';		/* ignored if replaces[i] == ' ' anyway */
	values[i] = (Datum)NULL;/* ignored if replaces[i] == ' ' anyway */
    }
    replaces[Anum_pg_class_relacl - 1] = 'r';
    values[Anum_pg_class_relacl - 1] = (Datum)new_acl;
    htp = heap_modifytuple(htp, buffer, relation, values, nulls, replaces);
    /* XXX is this necessary? */
    ItemPointerCopy(&htp->t_ctid, &tmp_ipd);
    /* XXX handle index on pg_class? */
    setheapoverride(true);
    (void) heap_replace(relation, &tmp_ipd, htp);
    setheapoverride(false);
    heap_endscan(hsdp);

    /* keep the catalog indices up to date */
    CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices,
		       idescs);
    CatalogIndexInsert(idescs, Num_pg_class_indices, relation, htp);
    CatalogCloseIndices(Num_pg_class_indices, idescs);

    heap_close(relation);
    if (free_old_acl)
	pfree(old_acl);
    pfree(new_acl);
}

AclId
get_grosysid(char *groname)
{
    HeapTuple htp;
    AclId id = 0;

    htp = SearchSysCacheTuple(GRONAME, PointerGetDatum(groname), 
			      0,0,0);
    if (HeapTupleIsValid(htp)) {
	id = ((Form_pg_group) GETSTRUCT(htp))->grosysid;
    } else {
	elog(WARN, "non-existent group \"%s\"", groname);
    }
    return(id);
}

char*
get_groname(AclId grosysid)
{
    HeapTuple htp;
    char *name;

    htp = SearchSysCacheTuple(GROSYSID, PointerGetDatum(grosysid),
			      0,0,0);
    if (HeapTupleIsValid(htp)) {
	name = (((Form_pg_group) GETSTRUCT(htp))->groname).data;
    } else {
	elog(NOTICE, "get_groname: group %d not found", grosysid);
    }
    return(name);
}

static int32
in_group(AclId uid, AclId gid)
{
	Relation relation;
	HeapTuple htp;
	Acl *tmp;
	unsigned i, num;
	AclId *aidp;
	int32 found = 0;

	relation = heap_openr(GroupRelationName);
	if (!RelationIsValid(relation)) {
		elog(NOTICE, "in_group: could not open \"%s\"??",
		     GroupRelationName);
		return(0);
	}
	htp = SearchSysCacheTuple(GROSYSID, ObjectIdGetDatum(gid),
				  0,0,0);
	if (HeapTupleIsValid(htp) &&
	    !heap_attisnull(htp, Anum_pg_group_grolist)) {
		tmp = (IdList *) heap_getattr(htp, InvalidBuffer,
					      Anum_pg_group_grolist,
				      RelationGetTupleDescriptor(relation),
					      (bool *) NULL);
		/* XXX make me a function */
		num = IDLIST_NUM(tmp);
		aidp = IDLIST_DAT(tmp);
		for (i = 0; i < num; ++i)
			if (aidp[i] == uid) {
				found = 1;
				break;
			}
	} else {
		elog(NOTICE, "in_group: group %d not found", gid);
	}
	heap_close(relation);
	return(found);
}

/*
 * aclcheck
 * Returns 1 if the 'id' of type 'idtype' has ACL entries in 'acl' to satisfy
 * any one of the requirements of 'mode'.  Returns 0 otherwise.
 */
int32
aclcheck(Acl *acl, AclId id, AclIdType idtype, AclMode mode)
{
    register unsigned i;
    register AclItem *aip, *aidat;
    unsigned num, found_group;

    /* if no acl is found, use world default */
    if (!acl) {
	acl = acldefault();
    }

    num = ACL_NUM(acl);
    aidat = ACL_DAT(acl);

    /*
     * We'll treat the empty ACL like that, too, although this is more
     * like an error (i.e., you manually blew away your ACL array) --
     * the system never creates an empty ACL.
     */
    if (num < 1) {
#ifdef ACLDEBUG_TRACE
	elog(DEBUG, "aclcheck: zero-length ACL, returning 1");
#endif
	return(1);
    }

    switch (idtype) {
    case ACL_IDTYPE_UID:
	for (i = 1, aip = aidat + 1; /* skip world entry */
	     i < num && aip->ai_idtype == ACL_IDTYPE_UID;
	     ++i, ++aip) {
	    if (aip->ai_id == id) {
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "aclcheck: found %d/%d",
		     aip->ai_id, aip->ai_mode);
#endif
		return((aip->ai_mode & mode) ? 1 : 0);
	    }
	}
	for (found_group = 0;
	     i < num && aip->ai_idtype == ACL_IDTYPE_GID;
	     ++i, ++aip) {
	    if (in_group(id, aip->ai_id)) {
		if (aip->ai_mode & mode)
		    ++found_group;
		else {
#ifdef ACLDEBUG_TRACE
		    elog(DEBUG, "aclcheck: found %d/%d",
			 aip->ai_id, aip->ai_mode);
#endif
		    return(0);
		}
	    }
	}
	if (found_group) {
#ifdef ACLDEBUG_TRACE
	    elog(DEBUG,"aclcheck: all groups ok");
#endif
	    return(1);
	}
	break;
    case ACL_IDTYPE_GID:
	for (i = 1, aip = aidat + 1; /* skip world entry and UIDs */
	     i < num && aip->ai_idtype == ACL_IDTYPE_UID;
	     ++i, ++aip)
	    ;
	for (;
	     i < num && aip->ai_idtype == ACL_IDTYPE_GID;
	     ++i, ++aip) {
	    if (aip->ai_id == id) {
#ifdef ACLDEBUG_TRACE
		elog(DEBUG, "aclcheck: found %d/%d",
		     aip->ai_id, aip->ai_mode);
#endif
		return((aip->ai_mode & mode) ? 1 : 0);
	    }
	}
	break;
    case ACL_IDTYPE_WORLD:
	break;
    default:
	elog(WARN, "aclcheck: bogus ACL id type: %d", idtype);
	break;
    }
	
#ifdef ACLDEBUG_TRACE
    elog(DEBUG, "aclcheck: using world=%d", aidat->ai_mode);
#endif
    return((aidat->ai_mode & mode) ? 1 : 0);
}

int32
pg_aclcheck(char *relname, char *usename, AclMode mode)
{
    HeapTuple htp;
    AclId id;
    Acl *acl = (Acl *) NULL, *tmp;
    int32 result;
    Relation relation;

    htp = SearchSysCacheTuple(USENAME, PointerGetDatum(usename), 
			      0,0,0);
    if (!HeapTupleIsValid(htp))
	elog(WARN, "pg_aclcheck: user \"%-.*s\" not found",
	     NAMEDATALEN, usename);
    id = (AclId) ((Form_pg_user) GETSTRUCT(htp))->usesysid;

    /* for the 'pg_database' relation, check the usecreatedb
       field before checking normal permissions */
    if ( strcmp(DatabaseRelationName, relname) == 0 &&
	(((Form_pg_user) GETSTRUCT(htp))->usecreatedb)) {
	/* note that even though the user can now append to the
	   pg_database table, there is still additional permissions checking
	   in dbcommands.c */
	if (mode & ACL_AP)
	    return (1);
    }

    /*
     * Deny anyone permission to update a system catalog unless
     * pg_user.usecatupd is set.  (This is to let superusers protect
     * themselves from themselves.)
     */
    if (((mode & ACL_WR) || (mode & ACL_AP)) &&
	IsSystemRelationName(relname) &&
	!((Form_pg_user) GETSTRUCT(htp))->usecatupd) {
	elog(DEBUG, "pg_aclcheck: catalog update to \"%-.*s\": permission denied",
	     NAMEDATALEN, relname);
	return(0);
    }
	
    /*
     * Otherwise, superusers bypass all permission-checking.
     */
    if (((Form_pg_user) GETSTRUCT(htp))->usesuper) {
#ifdef ACLDEBUG_TRACE
	elog(DEBUG, "pg_aclcheck: \"%-.*s\" is superuser",
	     NAMEDATALEN, usename);
#endif
	return(1);
    }
	
#ifndef ACLDEBUG
    htp = SearchSysCacheTuple(RELNAME, PointerGetDatum(relname), 
			      0,0,0);
    if (!HeapTupleIsValid(htp)) {
	elog(WARN, "pg_aclcheck: class \"%-.*s\" not found",
	     NAMEDATALEN, relname);
	return(1);
    }
    if (!heap_attisnull(htp, Anum_pg_class_relacl)) {
	relation = heap_openr(RelationRelationName);
	tmp = (Acl *) heap_getattr(htp, InvalidBuffer,
				   Anum_pg_class_relacl,
				   RelationGetTupleDescriptor(relation),
				   (bool *) NULL);
	acl = makeacl(ACL_NUM(tmp));
	memmove((char *) acl, (char *) tmp, ACL_SIZE(tmp));
	heap_close(relation);
    } else {
	/* if the acl is null, by default the owner can do whatever
	   he wants to with it */
	Oid ownerId;
	relation = heap_openr(RelationRelationName);
	ownerId = (Oid)heap_getattr(htp, InvalidBuffer,
				    Anum_pg_class_relowner,
				    RelationGetTupleDescriptor(relation),
				    (bool*) NULL);
	acl = aclownerdefault(ownerId);
    }
#else
    {				/* This is why the syscache is great... */
	static ScanKeyData relkey[1] = {
	    { 0, Anum_pg_class_relname, NameEqualRegProcedure }
	};
	HeapScanDesc hsdp;

	relation = heap_openr(RelationRelationName);
	if (!RelationIsValid(relation)) {
	    elog(NOTICE, "pg_checkacl: could not open \"%-.*s\"??",
		 RelationRelationName);
	    return(1);
	}
	fmgr_info(NameEqualRegProcedure,
		  &relkey[0].sk_func,
		  &relkey[0].sk_nargs);
	relkey[0].sk_argument = NameGetDatum(relname);
	hsdp = heap_beginscan(relation, 0, NowTimeQual, 1, relkey);
	htp = heap_getnext(hsdp, 0, (Buffer *) 0);
	if (HeapTupleIsValid(htp) &&
	    !heap_attisnull(htp, Anum_pg_class_relacl)) {
	    tmp = (Acl *) heap_getattr(htp, InvalidBuffer,
				       Anum_pg_class_relacl,
				       RelationGetTupleDescriptor(relation),
				       (bool *) NULL);
	    acl = makeacl(ACL_NUM(tmp));
	    memmove((char *) acl, (char *) tmp, ACL_SIZE(tmp));
	}
	heap_endscan(hsdp);
	heap_close(relation);
    }
#endif
    result = aclcheck(acl, id, (AclIdType) ACL_IDTYPE_UID, mode);
    if (acl)
	pfree(acl);
    return(result);
}

int32
pg_ownercheck(char *usename,
	      char *value,
	      int cacheid)
{
    HeapTuple htp;
    AclId user_id, owner_id;

    htp = SearchSysCacheTuple(USENAME, PointerGetDatum(usename), 
			      0,0,0);
    if (!HeapTupleIsValid(htp))
	elog(WARN, "pg_ownercheck: user \"%-.*s\" not found",
	     NAMEDATALEN, usename);
    user_id = (AclId) ((Form_pg_user) GETSTRUCT(htp))->usesysid;

    /*
     * Superusers bypass all permission-checking.
     */
    if (((Form_pg_user) GETSTRUCT(htp))->usesuper) {
#ifdef ACLDEBUG_TRACE
	elog(DEBUG, "pg_ownercheck: user \"%-.*s\" is superuser",
	     NAMEDATALEN, usename);
#endif
	return(1);
    }

    htp = SearchSysCacheTuple(cacheid, PointerGetDatum(value), 
			      0,0,0);
    switch (cacheid) {
    case OPROID:
	if (!HeapTupleIsValid(htp))
	    elog(WARN, "pg_ownercheck: operator %d not found",
		 (int) value);
	owner_id = ((OperatorTupleForm) GETSTRUCT(htp))->oprowner;
	break;
    case PRONAME:
	if (!HeapTupleIsValid(htp))
	    elog(WARN, "pg_ownercheck: function \"%-.*s\" not found",
		 NAMEDATALEN, value);
	owner_id = ((Form_pg_proc) GETSTRUCT(htp))->proowner;
	break;
    case RELNAME:
	if (!HeapTupleIsValid(htp))
	    elog(WARN, "pg_ownercheck: class \"%-.*s\" not found",
		 NAMEDATALEN, value);
	owner_id = ((Form_pg_class) GETSTRUCT(htp))->relowner;
	break;
    case TYPNAME:
	if (!HeapTupleIsValid(htp))
	    elog(WARN, "pg_ownercheck: type \"%-.*s\" not found",
		 NAMEDATALEN, value);
	owner_id = ((TypeTupleForm) GETSTRUCT(htp))->typowner;
	break;
    default:
	elog(WARN, "pg_ownercheck: invalid cache id: %d",
	     cacheid);
	break;
    }
	
    return(user_id == owner_id);
}

int32
pg_func_ownercheck(char *usename, 
		   char *funcname,
		   int nargs,
		   Oid *arglist)
{
    HeapTuple htp;
    AclId user_id, owner_id;

    htp = SearchSysCacheTuple(USENAME, PointerGetDatum(usename), 
			      0,0,0);
    if (!HeapTupleIsValid(htp))
	elog(WARN, "pg_func_ownercheck: user \"%-.*s\" not found",
	     NAMEDATALEN, usename);
    user_id = (AclId) ((Form_pg_user) GETSTRUCT(htp))->usesysid;

    /*
     * Superusers bypass all permission-checking.
     */
    if (((Form_pg_user) GETSTRUCT(htp))->usesuper) {
#ifdef ACLDEBUG_TRACE
	elog(DEBUG, "pg_ownercheck: user \"%-.*s\" is superuser",
	     NAMEDATALEN, usename);
#endif
	return(1);
    }

    htp = SearchSysCacheTuple(PRONAME, 
			      PointerGetDatum(funcname),
			      PointerGetDatum(nargs),
			      PointerGetDatum(arglist),
			      0);
    if (!HeapTupleIsValid(htp))
	func_error("pg_func_ownercheck", funcname, nargs, (int*)arglist);

    owner_id = ((Form_pg_proc) GETSTRUCT(htp))->proowner;
	
    return(user_id == owner_id);
}
