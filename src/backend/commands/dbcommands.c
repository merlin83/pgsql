/*-------------------------------------------------------------------------
 *
 * dbcommands.c
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/dbcommands.c,v 1.65 2000-11-08 23:24:24 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "commands/dbcommands.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/catalog.h"
#include "catalog/pg_database.h"
#include "catalog/pg_shadow.h"
#include "commands/comment.h"
#include "miscadmin.h"
#include "storage/sinval.h"		/* for DatabaseHasActiveBackends */
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/syscache.h"


/* non-export function prototypes */
static bool get_user_info(Oid use_sysid, bool *use_super, bool *use_createdb);
static bool get_db_info(const char *name, char *dbpath, Oid *dbIdP, int4 *ownerIdP);
static char * resolve_alt_dbpath(const char * dbpath, Oid dboid);
static bool remove_dbdirs(const char * real_loc, const char * altloc);

/*
 * CREATE DATABASE
 */

void
createdb(const char *dbname, const char *dbpath, int encoding)
{
	char		buf[2 * MAXPGPATH + 100];
	char	   *altloc;
	char	   *real_loc;
	int			ret;
	bool		use_super,
				use_createdb;
	Relation	pg_database_rel;
	HeapTuple	tuple;
	TupleDesc	pg_database_dsc;
	Datum		new_record[Natts_pg_database];
	char		new_record_nulls[Natts_pg_database] = {' ', ' ', ' ', ' ', ' '};
	Oid			dboid;

	if (!get_user_info(GetUserId(), &use_super, &use_createdb))
		elog(ERROR, "current user name is invalid");

	if (!use_createdb && !use_super)
		elog(ERROR, "CREATE DATABASE: permission denied");

	if (get_db_info(dbname, NULL, NULL, NULL))
		elog(ERROR, "CREATE DATABASE: database \"%s\" already exists", dbname);

	/* don't call this in a transaction block */
	if (IsTransactionBlock())
		elog(ERROR, "CREATE DATABASE: may not be called in a transaction block");

	/*
	 * Insert a new tuple into pg_database
	 */
	pg_database_rel = heap_openr(DatabaseRelationName, AccessExclusiveLock);
	pg_database_dsc = RelationGetDescr(pg_database_rel);

	/* 
	 * Preassign OID for pg_database tuple, so that we know current
	 * OID counter value
	 */
	dboid = newoid();

	/* Form tuple */
	new_record[Anum_pg_database_datname - 1] =
		DirectFunctionCall1(namein, CStringGetDatum(dbname));
	new_record[Anum_pg_database_datdba - 1] = Int32GetDatum(GetUserId());
	new_record[Anum_pg_database_encoding - 1] = Int32GetDatum(encoding);
	/* Save current OID val */
	new_record[Anum_pg_database_datlastsysoid - 1] = ObjectIdGetDatum(dboid);
	/* no nulls here, GetRawDatabaseInfo doesn't like them */
	new_record[Anum_pg_database_datpath - 1] =
		DirectFunctionCall1(textin, CStringGetDatum(dbpath ? dbpath : ""));

	tuple = heap_formtuple(pg_database_dsc, new_record, new_record_nulls);

	tuple->t_data->t_oid = dboid;	/* override heap_insert */


	/*
	 * Update table
	 */
	heap_insert(pg_database_rel, tuple);

	real_loc = GetDatabasePath(tuple->t_data->t_oid);
	altloc = resolve_alt_dbpath(dbpath, tuple->t_data->t_oid);

	if (strchr(real_loc, '\'') && strchr(altloc, '\''))
		elog(ERROR, "database path may not contain single quotes");
	/* ... otherwise we'd be open to shell exploits below */

	/*
	 * Update indexes (there aren't any currently)
	 */
#ifdef Num_pg_database_indices
	if (RelationGetForm(pg_database_rel)->relhasindex)
	{
		Relation	idescs[Num_pg_database_indices];

		CatalogOpenIndices(Num_pg_database_indices,
						   Name_pg_database_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_database_indices, pg_database_rel,
						   tuple);
		CatalogCloseIndices(Num_pg_database_indices, idescs);
	}
#endif

	heap_close(pg_database_rel, NoLock);

	/*
	 * Close virtual file descriptors so the kernel has more available for
	 * the mkdir() and system() calls below.
	 */
	closeAllVfds();

	/* Copy the template database to the new location */

	if (mkdir((altloc ? altloc : real_loc), S_IRWXU) != 0)
		elog(ERROR, "CREATE DATABASE: unable to create database directory '%s': %s",
			 (altloc ? altloc : real_loc), strerror(errno));

	if (altloc)
	{
		if (symlink(altloc, real_loc) != 0)
			elog(ERROR, "CREATE DATABASE: could not link %s to %s: %s",
				 real_loc, altloc, strerror(errno));
	}

	snprintf(buf, sizeof(buf), "cp '%s'/* '%s'",
			 GetDatabasePath(TemplateDbOid), real_loc);

	ret = system(buf);
	/* Some versions of SunOS seem to return ECHILD after a system() call */
	if (ret != 0 && errno != ECHILD)
	{
		if (remove_dbdirs(real_loc, altloc))
			elog(ERROR, "CREATE DATABASE: could not initialize database directory");
		else
			elog(ERROR, "CREATE DATABASE: could not initialize database directory; delete failed as well");
	}

#ifdef XLOG
	BufferSync();
#endif
}



/*
 * DROP DATABASE
 */

void
dropdb(const char *dbname)
{
	int4		db_owner;
	bool		use_super;
	Oid			db_id;
	char       *altloc;
	char       *real_loc;
	char        dbpath[MAXPGPATH];
	Relation	pgdbrel;
	HeapScanDesc pgdbscan;
	ScanKeyData key;
	HeapTuple	tup;

	AssertArg(dbname);

	if (strcmp(dbname, "template1") == 0)
		elog(ERROR, "DROP DATABASE: may not be executed on the template1 database");

	if (strcmp(dbname, DatabaseName) == 0)
		elog(ERROR, "DROP DATABASE: cannot be executed on the currently open database");

	if (IsTransactionBlock())
		elog(ERROR, "DROP DATABASE: may not be called in a transaction block");

	if (!get_user_info(GetUserId(), &use_super, NULL))
		elog(ERROR, "current user name is invalid");

	if (!get_db_info(dbname, dbpath, &db_id, &db_owner))
		elog(ERROR, "DROP DATABASE: database \"%s\" does not exist", dbname);

	if (GetUserId() != db_owner && !use_super)
		elog(ERROR, "DROP DATABASE: permission denied");

	real_loc = GetDatabasePath(db_id);
	altloc = resolve_alt_dbpath(dbpath, db_id);

	/*
	 * Obtain exclusive lock on pg_database.  We need this to ensure that
	 * no new backend starts up in the target database while we are
	 * deleting it.  (Actually, a new backend might still manage to start
	 * up, because it will read pg_database without any locking to
	 * discover the database's OID.  But it will detect its error in
	 * ReverifyMyDatabase and shut down before any serious damage is done.
	 * See postinit.c.)
	 */
	pgdbrel = heap_openr(DatabaseRelationName, AccessExclusiveLock);

	/*
	 * Check for active backends in the target database.
	 */
	if (DatabaseHasActiveBackends(db_id))
	{
		heap_close(pgdbrel, AccessExclusiveLock);
		elog(ERROR, "DROP DATABASE: database \"%s\" is being accessed by other users", dbname);
	}

	/*
	 * Find the database's tuple by OID (should be unique, we trust).
	 */
	ScanKeyEntryInitialize(&key, 0, ObjectIdAttributeNumber,
						   F_OIDEQ, ObjectIdGetDatum(db_id));

	pgdbscan = heap_beginscan(pgdbrel, 0, SnapshotNow, 1, &key);

	tup = heap_getnext(pgdbscan, 0);
	if (!HeapTupleIsValid(tup))
	{
		heap_close(pgdbrel, AccessExclusiveLock);

		/*
		 * This error should never come up since the existence of the
		 * database is checked earlier
		 */
		elog(ERROR, "DROP DATABASE: Database \"%s\" doesn't exist despite earlier reports to the contrary",
			 dbname);
	}

	/* Delete any comments associated with the database */
	DeleteComments(db_id);

	/* Remove the database's tuple from pg_database */
	heap_delete(pgdbrel, &tup->t_self, NULL);

	heap_endscan(pgdbscan);

	/*
	 * Close pg_database, but keep exclusive lock till commit to ensure
	 * that any new backend scanning pg_database will see the tuple dead.
	 */
	heap_close(pgdbrel, NoLock);

	/*
	 * Drop pages for this database that are in the shared buffer cache.
	 * This is important to ensure that no remaining backend tries to
	 * write out a dirty buffer to the dead database later...
	 */
	DropBuffers(db_id);

	/*
	 * Close virtual file descriptors so the kernel has more available for
	 * the system() call below.
	 */
	closeAllVfds();

	/*
	 * Remove the database's subdirectory and everything in it.
	 */
	remove_dbdirs(real_loc, altloc);
}



/*
 * Helper functions
 */

static bool
get_db_info(const char *name, char *dbpath, Oid *dbIdP, int4 *ownerIdP)
{
	Relation	relation;
	HeapTuple	tuple;
	ScanKeyData scanKey;
	HeapScanDesc scan;

	AssertArg(name);

	relation = heap_openr(DatabaseRelationName, AccessExclusiveLock /* ??? */ );

	ScanKeyEntryInitialize(&scanKey, 0, Anum_pg_database_datname,
						   F_NAMEEQ, NameGetDatum(name));

	scan = heap_beginscan(relation, 0, SnapshotNow, 1, &scanKey);
	if (!HeapScanIsValid(scan))
		elog(ERROR, "Cannot begin scan of %s.", DatabaseRelationName);

	tuple = heap_getnext(scan, 0);

	if (HeapTupleIsValid(tuple))
	{
		text	   *tmptext;
		bool		isnull;

		/* oid of the database */
		if (dbIdP)
			*dbIdP = tuple->t_data->t_oid;
		/* uid of the owner */
		if (ownerIdP)
		{
			*ownerIdP = (int4) heap_getattr(tuple,
											Anum_pg_database_datdba,
											RelationGetDescr(relation),
											&isnull);
			if (isnull)
				*ownerIdP = -1; /* hopefully no one has that id already ;) */
		}
		/* database path (as registered in pg_database) */
		if (dbpath)
		{
			tmptext = (text *) heap_getattr(tuple,
											Anum_pg_database_datpath,
											RelationGetDescr(relation),
											&isnull);

			if (!isnull)
			{
				Assert(VARSIZE(tmptext) - VARHDRSZ < MAXPGPATH);

				strncpy(dbpath, VARDATA(tmptext), VARSIZE(tmptext) - VARHDRSZ);
				*(dbpath + VARSIZE(tmptext) - VARHDRSZ) = '\0';
			}
			else
				strcpy(dbpath, "");
		}
	}
	else
	{
		if (dbIdP)
			*dbIdP = InvalidOid;
	}

	heap_endscan(scan);

	/* We will keep the lock on the relation until end of transaction. */
	heap_close(relation, NoLock);

	return HeapTupleIsValid(tuple);
}

static bool
get_user_info(Oid use_sysid, bool *use_super, bool *use_createdb)
{
	HeapTuple	utup;

	utup = SearchSysCacheTuple(SHADOWSYSID,
							   ObjectIdGetDatum(use_sysid),
							   0, 0, 0);

	if (!HeapTupleIsValid(utup))
		return false;

	if (use_super)
		*use_super = ((Form_pg_shadow) GETSTRUCT(utup))->usesuper;
	if (use_createdb)
		*use_createdb = ((Form_pg_shadow) GETSTRUCT(utup))->usecreatedb;

	return true;
}


static char *
resolve_alt_dbpath(const char * dbpath, Oid dboid)
{
	const char * prefix;
	char * ret;
	size_t len;

	if (dbpath == NULL || dbpath[0] == '\0')
		return NULL;

	if (strchr(dbpath, '/'))
	{
#ifndef ALLOW_ABSOLUTE_DBPATHS
		elog(ERROR, "Absolute paths are not allowed as database locations");
#endif
		prefix = dbpath;
	}
	else
	{
		/* must be environment variable */
		char * var = getenv(dbpath);
		if (!var)
			elog(ERROR, "environment variable %s not set", dbpath);
		if (var[0] != '/')
			elog(ERROR, "environment variable %s must be absolute path", dbpath);
		prefix = var;
	}

	len = strlen(prefix) + 6 + sizeof(Oid) * 8 + 1;
	ret = palloc(len);
	snprintf(ret, len, "%s/base/%u", prefix, dboid);

	return ret;
}


static bool
remove_dbdirs(const char * real_loc, const char * altloc)
{
	char buf[MAXPGPATH + 100];
	bool success = true;

	if (altloc)
		/* remove symlink */
		if (unlink(real_loc) != 0)
		{
			elog(NOTICE, "could not remove '%s': %s", real_loc, strerror(errno));
			success = false;
		}

	snprintf(buf, sizeof(buf), "rm -rf '%s'", altloc ? altloc : real_loc);
	if (system(buf) != 0 && errno != ECHILD)
	{
		elog(NOTICE, "database directory '%s' could not be removed",
			 altloc ? altloc : real_loc);
		success = false;
	}

	return success;
}
