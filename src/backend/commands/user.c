/*-------------------------------------------------------------------------
 *
 * user.c
 *	  use pg_exec_query to create a new user in the catalog
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: user.c,v 1.41 1999-12-12 05:57:28 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_database.h"
#include "catalog/pg_shadow.h"
#include "commands/copy.h"
#include "commands/user.h"
#include "libpq/crypt.h"
#include "miscadmin.h"
#include "tcop/tcopprot.h"
#include "utils/acl.h"
#include "utils/syscache.h"

static void CheckPgUserAclNotNull(void);

#define SQL_LENGTH	512

/*---------------------------------------------------------------------
 * UpdatePgPwdFile
 *
 * copy the modified contents of pg_shadow to a file used by the postmaster
 * for user authentication.  The file is stored as $PGDATA/pg_pwd.
 *
 * NB: caller is responsible for ensuring that only one backend can
 * execute this routine at a time.  Acquiring AccessExclusiveLock on
 * pg_shadow is the standard way to do that.
 *---------------------------------------------------------------------
 */

/* This is the old name. Now uses a lower case name to be able to call this
   from SQL. */
#define UpdatePgPwdFile() update_pg_pwd()

void
update_pg_pwd()
{
	char	   *filename,
			   *tempname;
	int			bufsize;

	/*
	 * Create a temporary filename to be renamed later.  This prevents the
	 * backend from clobbering the pg_pwd file while the postmaster might
	 * be reading from it.
	 */
	filename = crypt_getpwdfilename();
	bufsize = strlen(filename) + 12;
	tempname = (char *) palloc(bufsize);
	snprintf(tempname, bufsize, "%s.%d", filename, MyProcPid);

	/*
	 * Copy the contents of pg_shadow to the pg_pwd ASCII file using the
	 * SEPCHAR character as the delimiter between fields.  Make sure the
	 * file is created with mode 600 (umask 077).
	 */
	DoCopy(ShadowRelationName,	/* relname */
		   false,				/* binary */
		   false,				/* oids */
		   false,				/* from */
		   false,				/* pipe */
		   tempname,			/* filename */
		   CRYPT_PWD_FILE_SEPSTR, /* delim */
		   0077);				/* fileumask */
	/*
	 * And rename the temp file to its final name, deleting the old pg_pwd.
	 */
	rename(tempname, filename);

	/*
	 * Create a flag file the postmaster will detect the next time it
	 * tries to authenticate a user.  The postmaster will know to reload
	 * the pg_pwd file contents.
	 */
	filename = crypt_getpwdreloadfilename();
	creat(filename, S_IRUSR | S_IWUSR);

	pfree((void *) tempname);
}

/*---------------------------------------------------------------------
 * DefineUser
 *
 * Add the user to the pg_shadow relation, and if specified make sure the
 * user is specified in the desired groups of defined in pg_group.
 *---------------------------------------------------------------------
 */
void
DefineUser(CreateUserStmt *stmt, CommandDest dest)
{
	char	   *pg_shadow,
				sql[SQL_LENGTH];
	Relation	pg_shadow_rel;
	TupleDesc	pg_shadow_dsc;
	HeapScanDesc scan;
	HeapTuple	tuple;
	bool		user_exists = false,
                sysid_exists = false,
				inblock,
                havesysid,
				havepassword,
				havevaluntil;
	int			max_id = -1;

    havesysid    = stmt->sysid >= 0;
	havepassword = stmt->password && stmt->password[0];
	havevaluntil = stmt->validUntil && stmt->validUntil[0];

	if (havepassword)
		CheckPgUserAclNotNull();
	if (!(inblock = IsTransactionBlock()))
		BeginTransactionBlock();

	/*
	 * Make sure the user attempting to create a user can insert into the
	 * pg_shadow relation.
	 */
	pg_shadow = GetPgUserName();
	if (pg_aclcheck(ShadowRelationName, pg_shadow, ACL_RD | ACL_WR | ACL_AP) != ACLCHECK_OK)
	{
		UserAbortTransactionBlock();
		elog(ERROR, "DefineUser: user \"%s\" does not have SELECT and INSERT privilege for \"%s\"",
			 pg_shadow, ShadowRelationName);
		return;
	}

	/*
	 * Scan the pg_shadow relation to be certain the user or id doesn't already
	 * exist.  Note we secure exclusive lock, because we also need to be
	 * sure of what the next usesysid should be, and we need to protect
	 * our update of the flat password file.
	 */
	pg_shadow_rel = heap_openr(ShadowRelationName, AccessExclusiveLock);
	pg_shadow_dsc = RelationGetDescr(pg_shadow_rel);

	scan = heap_beginscan(pg_shadow_rel, false, SnapshotNow, 0, NULL);
	while (!user_exists && !sysid_exists && HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
	{
        Datum		datum;
        bool        null;

		datum = heap_getattr(tuple, Anum_pg_shadow_usename, pg_shadow_dsc, &null);
		user_exists = datum && !null && (strcmp((char *) datum, stmt->user) == 0);

		datum = heap_getattr(tuple, Anum_pg_shadow_usesysid, pg_shadow_dsc, &null);
        if (havesysid) /* customized id wanted */
            sysid_exists = datum && !null && ((int)datum == stmt->sysid);
        else /* pick 1 + max */
        {
            if ((int) datum > max_id)
                max_id = (int) datum;
        }
	}
	heap_endscan(scan);

	if (user_exists || sysid_exists)
	{
		heap_close(pg_shadow_rel, AccessExclusiveLock);
		UserAbortTransactionBlock();
        if (user_exists)
            elog(ERROR, "DefineUser: user name \"%s\" already exists", stmt->user);
        else
            elog(ERROR, "DefineUser: sysid %d is already assigned", stmt->sysid);
		return;
	}

	/*
	 * Build the insert statement to be executed.
	 *
	 * XXX Ugly as this code is, it still fails to cope with ' or \ in any of
	 * the provided strings.
	 *
	 * XXX This routine would be *lots* better if it inserted the new
	 * tuple with formtuple/heap_insert.  For one thing, all of the
	 * transaction-block gamesmanship could be eliminated, because
	 * it's only there to make the world safe for a recursive call
	 * to pg_exec_query_dest().
	 */
	snprintf(sql, SQL_LENGTH,
			 "insert into %s (usename,usesysid,usecreatedb,usetrace,"
			 "usesuper,usecatupd,passwd,valuntil) "
			 "values('%s',%d,'%c','f','%c','%c',%s%s%s,%s%s%s)",
			 ShadowRelationName,
			 stmt->user,
			 havesysid ? stmt->sysid : max_id + 1,
			 (stmt->createdb && *stmt->createdb) ? 't' : 'f',
			 (stmt->createuser && *stmt->createuser) ? 't' : 'f',
			 ((stmt->createdb && *stmt->createdb) ||
			  (stmt->createuser && *stmt->createuser)) ? 't' : 'f',
			 havepassword ? "'" : "",
			 havepassword ? stmt->password : "NULL",
			 havepassword ? "'" : "",
			 havevaluntil ? "'" : "",
			 havevaluntil ? stmt->validUntil : "NULL",
			 havevaluntil ? "'" : "");

	/*
	 * XXX If insert fails, say because a bogus valuntil date is given,
	 * need to catch the resulting error and undo our transaction.
	 */
	pg_exec_query_dest(sql, dest, false);

	/*
	 * Add stuff here for groups?
	 */

	/*
	 * Write the updated pg_shadow data to the flat password file.
	 * Because we are still holding AccessExclusiveLock on pg_shadow,
	 * we can be sure no other backend will try to write the flat
	 * file at the same time.
	 */
	UpdatePgPwdFile();

	/*
	 * Now we can clean up.
	 */
	heap_close(pg_shadow_rel, AccessExclusiveLock);

	if (IsTransactionBlock() && !inblock)
		EndTransactionBlock();
}


extern void
AlterUser(AlterUserStmt *stmt, CommandDest dest)
{

	char	   *pg_shadow,
				sql[SQL_LENGTH];
	Relation	pg_shadow_rel;
	TupleDesc	pg_shadow_dsc;
	HeapTuple	tuple;
	bool		inblock;
    bool        comma = false;

	if (stmt->password)
		CheckPgUserAclNotNull();
	if (!(inblock = IsTransactionBlock()))
		BeginTransactionBlock();

	/*
	 * Make sure the user attempting to create a user can insert into the
	 * pg_shadow relation.
	 */
	pg_shadow = GetPgUserName();
	if (pg_aclcheck(ShadowRelationName, pg_shadow, ACL_RD | ACL_WR) != ACLCHECK_OK)
	{
		UserAbortTransactionBlock();
		elog(ERROR, "AlterUser: user \"%s\" does not have SELECT and UPDATE privilege for \"%s\"",
			 pg_shadow, ShadowRelationName);
		return;
	}

	/*
	 * Scan the pg_shadow relation to be certain the user exists.
	 * Note we secure exclusive lock to protect our update of the
	 * flat password file.
	 */
	pg_shadow_rel = heap_openr(ShadowRelationName, AccessExclusiveLock);
	pg_shadow_dsc = RelationGetDescr(pg_shadow_rel);

	tuple = SearchSysCacheTuple(SHADOWNAME,
								PointerGetDatum(stmt->user),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		heap_close(pg_shadow_rel, AccessExclusiveLock);
		UserAbortTransactionBlock();
		elog(ERROR, "AlterUser: user \"%s\" does not exist", stmt->user);
	}

    /* look for duplicate sysid */
	tuple = SearchSysCacheTuple(SHADOWSYSID,
								Int32GetDatum(stmt->sysid),
								0, 0, 0);
    if (HeapTupleIsValid(tuple))
    {
        Datum datum;
        bool null;

		datum = heap_getattr(tuple, Anum_pg_shadow_usename, pg_shadow_dsc, &null);
        if (datum && !null && strcmp((char *) datum, stmt->user) != 0)
        {
            heap_close(pg_shadow_rel, AccessExclusiveLock);
            UserAbortTransactionBlock();
            elog(ERROR, "AlterUser: sysid %d is already assigned", stmt->sysid);
        }
    }


	/*
	 * Create the update statement to modify the user.
	 *
	 * XXX see diatribe in preceding routine.  This code is just as bogus.
	 */
	snprintf(sql, SQL_LENGTH, "update %s set ", ShadowRelationName);

	if (stmt->password)
    {
		snprintf(sql + strlen(sql), SQL_LENGTH - strlen(sql),
				 "passwd = '%s'", stmt->password);
        comma = true;
    }

    if (stmt->sysid>=0)
    {
        if (comma)
            strcat(sql, ", ");
        snprintf(sql + strlen(sql), SQL_LENGTH - strlen(sql),
                 "usesysid = %d", stmt->sysid);
        comma = true;
    }

	if (stmt->createdb)
    {
        if (comma)
            strcat(sql, ", ");
		snprintf(sql + strlen(sql), SQL_LENGTH - strlen(sql),
				 "usecreatedb='%c'",
				 *stmt->createdb ? 't' : 'f');
        comma = true;
    }

	if (stmt->createuser)
    {
        if (comma)
            strcat(sql, ", ");
		snprintf(sql + strlen(sql), SQL_LENGTH - strlen(sql),
				 "usesuper='%c'",
				 *stmt->createuser ? 't' : 'f');
        comma = true;
    }

	if (stmt->validUntil)
    {
        if (comma)
            strcat(sql, ", ");
		snprintf(sql + strlen(sql), SQL_LENGTH - strlen(sql),
				 "valuntil='%s'",
				 stmt->validUntil);
    }

	snprintf(sql + strlen(sql), SQL_LENGTH - strlen(sql),
			 " where usename = '%s'",
			 stmt->user);

	pg_exec_query_dest(sql, dest, false);

	/*
	 * Add stuff here for groups?
	 */

	/*
	 * Write the updated pg_shadow data to the flat password file.
	 * Because we are still holding AccessExclusiveLock on pg_shadow,
	 * we can be sure no other backend will try to write the flat
	 * file at the same time.
	 */
	UpdatePgPwdFile();

	/*
	 * Now we can clean up.
	 */
	heap_close(pg_shadow_rel, AccessExclusiveLock);

	if (IsTransactionBlock() && !inblock)
		EndTransactionBlock();
}


extern void
RemoveUser(char *user, CommandDest dest)
{
	char	   *pg_shadow;
	Relation	pg_shadow_rel,
				pg_rel;
	TupleDesc	pg_dsc;
	HeapScanDesc scan;
	HeapTuple	tuple;
	Datum		datum;
	char		sql[SQL_LENGTH];
	bool		n,
				inblock;
	int32		usesysid;
	int			ndbase = 0;
	char	  **dbase = NULL;

	if (!(inblock = IsTransactionBlock()))
		BeginTransactionBlock();

	/*
	 * Make sure the user attempting to create a user can delete from the
	 * pg_shadow relation.
	 */
	pg_shadow = GetPgUserName();
	if (pg_aclcheck(ShadowRelationName, pg_shadow, ACL_RD | ACL_WR) != ACLCHECK_OK)
	{
		UserAbortTransactionBlock();
		elog(ERROR, "RemoveUser: user \"%s\" does not have SELECT and DELETE privilege for \"%s\"",
			 pg_shadow, ShadowRelationName);
	}

	/*
	 * Scan the pg_shadow relation to find the usesysid of the user to be
	 * deleted.  Note we secure exclusive lock, because we need to protect
	 * our update of the flat password file.
	 */
	pg_shadow_rel = heap_openr(ShadowRelationName, AccessExclusiveLock);
	pg_dsc = RelationGetDescr(pg_shadow_rel);

	tuple = SearchSysCacheTuple(SHADOWNAME,
								PointerGetDatum(user),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		heap_close(pg_shadow_rel, AccessExclusiveLock);
		UserAbortTransactionBlock();
		elog(ERROR, "RemoveUser: user \"%s\" does not exist", user);
	}

	usesysid = (int32) heap_getattr(tuple, Anum_pg_shadow_usesysid, pg_dsc, &n);

	/*
	 * Perform a scan of the pg_database relation to find the databases
	 * owned by usesysid.  Then drop them.
	 */
	pg_rel = heap_openr(DatabaseRelationName, AccessExclusiveLock);
	pg_dsc = RelationGetDescr(pg_rel);

	scan = heap_beginscan(pg_rel, false, SnapshotNow, 0, NULL);
	while (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
	{
		datum = heap_getattr(tuple, Anum_pg_database_datdba, pg_dsc, &n);

		if ((int) datum == usesysid)
		{
			datum = heap_getattr(tuple, Anum_pg_database_datname, pg_dsc, &n);
			if (memcmp((void *) datum, "template1", 9) != 0)
			{
				dbase =
					(char **) repalloc((void *) dbase, sizeof(char *) * (ndbase + 1));
				dbase[ndbase] = (char *) palloc(NAMEDATALEN + 1);
				memcpy((void *) dbase[ndbase], (void *) datum, NAMEDATALEN);
				dbase[ndbase++][NAMEDATALEN] = '\0';
			}
		}
	}
	heap_endscan(scan);
	heap_close(pg_rel, AccessExclusiveLock);

	while (ndbase--)
	{
		elog(NOTICE, "Dropping database %s", dbase[ndbase]);
		snprintf(sql, SQL_LENGTH, "DROP DATABASE %s", dbase[ndbase]);
		pfree((void *) dbase[ndbase]);
		pg_exec_query_dest(sql, dest, false);
	}
	if (dbase)
		pfree((void *) dbase);

	/*
	 * Since pg_shadow is global over all databases, one of two things
	 * must be done to insure complete consistency.  First, pg_shadow
	 * could be made non-global. This would elminate the code above for
	 * deleting database and would require the addition of code to delete
	 * tables, views, etc owned by the user.
	 *
	 * The second option would be to create a means of deleting tables, view,
	 * etc. owned by the user from other databases.  pg_shadow is global
	 * and so this must be done at some point.
	 *
	 * Let us not forget that the user should be removed from the pg_groups
	 * also.
	 *
	 * Todd A. Brandys 11/18/1997
	 *
	 */

	/*
	 * Remove the user from the pg_shadow table
	 */
	snprintf(sql, SQL_LENGTH,
		"delete from %s where usename = '%s'", ShadowRelationName, user);
	pg_exec_query_dest(sql, dest, false);

	/*
	 * Write the updated pg_shadow data to the flat password file.
	 * Because we are still holding AccessExclusiveLock on pg_shadow,
	 * we can be sure no other backend will try to write the flat
	 * file at the same time.
	 */
	UpdatePgPwdFile();

	/*
	 * Now we can clean up.
	 */
	heap_close(pg_shadow_rel, AccessExclusiveLock);

	if (IsTransactionBlock() && !inblock)
		EndTransactionBlock();
}

/*
 * CheckPgUserAclNotNull
 *
 * check to see if there is an ACL on pg_shadow
 */
static void
CheckPgUserAclNotNull()
{
	HeapTuple	htup;

	htup = SearchSysCacheTuple(RELNAME,
							   PointerGetDatum(ShadowRelationName),
							   0, 0, 0);
	if (!HeapTupleIsValid(htup))
	{
		elog(ERROR, "IsPgUserAclNull: class \"%s\" not found",
			 ShadowRelationName);
	}

	if (heap_attisnull(htup, Anum_pg_class_relacl))
	{
		elog(NOTICE, "To use passwords, you have to revoke permissions on pg_shadow");
		elog(NOTICE, "so normal users can not read the passwords.");
		elog(ERROR, "Try 'REVOKE ALL ON pg_shadow FROM PUBLIC'");
	}

	return;
}
