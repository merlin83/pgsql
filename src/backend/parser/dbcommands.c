/*-------------------------------------------------------------------------
 *
 * dbcommands.c--
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/Attic/dbcommands.c,v 1.11 1997-11-10 15:17:44 thomas Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#include "postgres.h"
#include "miscadmin.h"			/* for DataDir */
#include "access/heapam.h"
#include "access/htup.h"
#include "access/relscan.h"
#include "utils/rel.h"
#include "utils/elog.h"
#include "catalog/catname.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_user.h"
#include "catalog/pg_database.h"
#include "utils/syscache.h"
#include "parser/dbcommands.h"
#include "tcop/tcopprot.h"
#include "storage/bufmgr.h"
#include "storage/lmgr.h"
#include "storage/fd.h"


/* non-export function prototypes */
static void
check_permissions(char *command, char *dbpath, char *dbname,
				  Oid *dbIdP, Oid *userIdP);
static HeapTuple get_pg_dbtup(char *command, char *dbname, Relation dbrel);
static void stop_vacuum(char *dbpath, char *dbname);

void
createdb(char *dbname, char *dbpath)
{
	Oid			db_id,
				user_id;
	char		buf[512];
	char	   *lp,
				loc[512];

	/*
	 * If this call returns, the database does not exist and we're allowed
	 * to create databases.
	 */
	check_permissions("createdb", dbpath, dbname, &db_id, &user_id);

	/* close virtual file descriptors so we can do system() calls */
	closeAllVfds();

	/* Now create directory for this new database */
	if ((dbpath != NULL) && (strcmp(dbpath,dbname) != 0))
	{
		if (*(dbpath+strlen(dbpath)-1) == SEP_CHAR)
			*(dbpath+strlen(dbpath)-1) = '\0';
		sprintf(loc, "%s%c%s", dbpath, SEP_CHAR, dbname);
	}
	else
	{
		strcpy(loc, dbname);
	}

	lp = ExpandDatabasePath(loc);

	if (lp == NULL)
		elog(WARN,"Unable to locate path '%s'"
			"\n\tThis may be due to a missing environment variable"
			" in the server",loc);

	if (mkdir(lp,S_IRWXU) != 0)
		elog(WARN,"Unable to create database directory %s",lp);

	sprintf(buf, "%s %s%cbase%ctemplate1%c* %s",
			COPY_CMD, DataDir, SEP_CHAR, SEP_CHAR, SEP_CHAR, lp);
	system(buf);

#if FALSE
	sprintf(buf, "insert into pg_database (datname, datdba, datpath) \
                  values (\'%s\'::char16, \'%d\'::oid, \'%s\'::text);",
			dbname, user_id, dbname);
#endif

	sprintf(buf, "insert into pg_database (datname, datdba, datpath)"
		" values (\'%s\', \'%d\', \'%s\');", dbname, user_id, loc);

	pg_eval(buf, (char **) NULL, (Oid *) NULL, 0);
}

void
destroydb(char *dbname)
{
	Oid			user_id,
				db_id;
	char	   *path;
	char		dbpath[MAXPGPATH+1];
	char		buf[512];

	/*
	 * If this call returns, the database exists and we're allowed to
	 * remove it.
	 */
	check_permissions("destroydb", dbpath, dbname, &db_id, &user_id);

	if (!OidIsValid(db_id))
	{
		elog(FATAL, "impossible: pg_database instance with invalid OID.");
	}

	/* stop the vacuum daemon */
	stop_vacuum(dbpath, dbname);

	path = ExpandDatabasePath(dbpath);
    if (path == NULL)
		elog(WARN,"Unable to locate path '%s'"
			"\n\tThis may be due to a missing environment variable"
			" in the server",dbpath);

	/*
	 * remove the pg_database tuple FIRST, this may fail due to
	 * permissions problems
	 */
	sprintf(buf, "delete from pg_database where pg_database.oid = \'%d\'::oid",
			db_id);
	pg_eval(buf, (char **) NULL, (Oid *) NULL, 0);

	/*
	 * remove the data directory. If the DELETE above failed, this will
	 * not be reached
	 */

	sprintf(buf, "rm -r %s", path);
	system(buf);

	/* drop pages for this database that are in the shared buffer cache */
	DropBuffers(db_id);
}

static HeapTuple
get_pg_dbtup(char *command, char *dbname, Relation dbrel)
{
	HeapTuple	dbtup;
	HeapTuple	tup;
	Buffer		buf;
	HeapScanDesc scan;
	ScanKeyData scanKey;

	ScanKeyEntryInitialize(&scanKey, 0, Anum_pg_database_datname,
						   NameEqualRegProcedure, NameGetDatum(dbname));

	scan = heap_beginscan(dbrel, 0, NowTimeQual, 1, &scanKey);
	if (!HeapScanIsValid(scan))
		elog(WARN, "%s: cannot begin scan of pg_database.", command);

	/*
	 * since we want to return the tuple out of this proc, and we're going
	 * to close the relation, copy the tuple and return the copy.
	 */
	tup = heap_getnext(scan, 0, &buf);

	if (HeapTupleIsValid(tup))
	{
		dbtup = heap_copytuple(tup);
		ReleaseBuffer(buf);
	}
	else
		dbtup = tup;

	heap_endscan(scan);
	return (dbtup);
}

/*
 *	check_permissions() -- verify that the user is permitted to do this.
 *
 *	If the user is not allowed to carry out this operation, this routine
 *	elog(WARN, ...)s, which will abort the xact.  As a side effect, the
 *	user's pg_user tuple OID is returned in userIdP and the target database's
 *	OID is returned in dbIdP.
 */

static void
check_permissions(char *command,
				  char *dbpath,
				  char *dbname,
				  Oid *dbIdP,
				  Oid *userIdP)
{
	Relation	dbrel;
	HeapTuple	dbtup,
				utup;
	Oid			dbowner = (Oid) 0;
	char		use_createdb;
	bool		dbfound;
	bool		use_super;
	char	   *userName;
	text	   *dbtext;
	char		path[MAXPGPATH+1];

	userName = GetPgUserName();
	utup = SearchSysCacheTuple(USENAME, PointerGetDatum(userName),
							   0, 0, 0);
	*userIdP = ((Form_pg_user) GETSTRUCT(utup))->usesysid;
	use_super = ((Form_pg_user) GETSTRUCT(utup))->usesuper;
	use_createdb = ((Form_pg_user) GETSTRUCT(utup))->usecreatedb;

	/* Check to make sure user has permission to use createdb */
	if (!use_createdb)
	{
		elog(WARN, "user \"%s\" is not allowed to create/destroy databases",
			 userName);
	}

	/* Make sure we are not mucking with the template database */
	if (!strcmp(dbname, "template1"))
	{
		elog(WARN, "%s cannot be executed on the template database.", command);
	}

	/* Check to make sure database is not the currently open database */
	if (!strcmp(dbname, GetDatabaseName()))
	{
		elog(WARN, "%s cannot be executed on an open database", command);
	}

	/* Check to make sure database is owned by this user */

	/*
	 * need the reldesc to get the database owner out of dbtup and to set
	 * a write lock on it.
	 */
	dbrel = heap_openr(DatabaseRelationName);

	if (!RelationIsValid(dbrel))
		elog(FATAL, "%s: cannot open relation \"%-.*s\"",
			 command, DatabaseRelationName);

	/*
	 * Acquire a write lock on pg_database from the beginning to avoid
	 * upgrading a read lock to a write lock.  Upgrading causes long
	 * delays when multiple 'createdb's or 'destroydb's are run simult.
	 * -mer 7/3/91
	 */
	RelationSetLockForWrite(dbrel);
	dbtup = get_pg_dbtup(command, dbname, dbrel);
	dbfound = HeapTupleIsValid(dbtup);

	if (dbfound)
	{
		dbowner = (Oid) heap_getattr(dbtup, InvalidBuffer,
									 Anum_pg_database_datdba,
									 RelationGetTupleDescriptor(dbrel),
									 (char *) NULL);
		*dbIdP = dbtup->t_oid;
		dbtext = (text *) heap_getattr(dbtup, InvalidBuffer,
								 Anum_pg_database_datpath,
								 RelationGetTupleDescriptor(dbrel),
								 (char *) NULL);

		strncpy(path, VARDATA(dbtext), (VARSIZE(dbtext)-VARHDRSZ));
		 *(path+VARSIZE(dbtext)-VARHDRSZ) = '\0';
	}
	else
	{
		*dbIdP = InvalidOid;
	}

	heap_close(dbrel);

	/*
	 * Now be sure that the user is allowed to do this.
	 */

	if (dbfound && !strcmp(command, "createdb"))
	{

		elog(WARN, "createdb: database %s already exists.", dbname);

	}
	else if (!dbfound && !strcmp(command, "destroydb"))
	{

		elog(WARN, "destroydb: database %s does not exist.", dbname);

	}
	else if (dbfound && !strcmp(command, "destroydb")
			 && dbowner != *userIdP && use_super == false)
	{

		elog(WARN, "%s: database %s is not owned by you.", command, dbname);

	}

	if (dbfound && !strcmp(command, "destroydb"))
		strcpy(dbpath, path);
} /* check_permissions() */

/*
 *	stop_vacuum() -- stop the vacuum daemon on the database, if one is running.
 */
static void
stop_vacuum(char *dbpath, char *dbname)
{
	char		filename[256];
	FILE	   *fp;
	int			pid;

	if (strchr(dbpath, SEP_CHAR) != 0)
	{
		sprintf(filename, "%s%cbase%c%s%c%s.vacuum", DataDir, SEP_CHAR, SEP_CHAR,
			dbname, SEP_CHAR, dbname);
	}
	else
	{
		sprintf(filename, "%s%c%s.vacuum", dbpath, SEP_CHAR, dbname);
	}

	if ((fp = AllocateFile(filename, "r")) != NULL)
	{
		fscanf(fp, "%d", &pid);
		FreeFile(fp);
		if (kill(pid, SIGKILLDAEMON1) < 0)
		{
			elog(WARN, "can't kill vacuum daemon (pid %d) on %s",
				 pid, dbname);
		}
	}
}
