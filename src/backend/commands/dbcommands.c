/*-------------------------------------------------------------------------
 *
 * dbcommands.c
 *		Database management commands (create/drop database).
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/dbcommands.c,v 1.97 2002-07-20 05:16:57 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

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
#include "catalog/indexing.h"
#include "commands/comment.h"
#include "commands/dbcommands.h"
#include "miscadmin.h"
#include "storage/freespace.h"
#include "storage/sinval.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"		/* encoding check */
#endif


/* non-export function prototypes */
static bool get_db_info(const char *name, Oid *dbIdP, int4 *ownerIdP,
			int *encodingP, bool *dbIsTemplateP, Oid *dbLastSysOidP,
			TransactionId *dbVacuumXidP, TransactionId *dbFrozenXidP,
			char *dbpath);
static bool have_createdb_privilege(void);
static char *resolve_alt_dbpath(const char *dbpath, Oid dboid);
static bool remove_dbdirs(const char *real_loc, const char *altloc);

/*
 * CREATE DATABASE
 */

void
createdb(const CreatedbStmt *stmt)
{
	char	   *nominal_loc;
	char	   *alt_loc;
	char	   *target_dir;
	char		src_loc[MAXPGPATH];
	char		buf[2 * MAXPGPATH + 100];
	Oid			src_dboid;
	int4		src_owner;
	int			src_encoding;
	bool		src_istemplate;
	Oid			src_lastsysoid;
	TransactionId src_vacuumxid;
	TransactionId src_frozenxid;
	char		src_dbpath[MAXPGPATH];
	Relation	pg_database_rel;
	HeapTuple	tuple;
	TupleDesc	pg_database_dsc;
	Datum		new_record[Natts_pg_database];
	char		new_record_nulls[Natts_pg_database];
	Oid			dboid;
	int32		datdba;
	List	   *option;
	DefElem    *downer = NULL;
	DefElem	   *dpath = NULL;
	DefElem	   *dtemplate = NULL;
	DefElem	   *dencoding = NULL;
	char	   *dbname = stmt->dbname;
	char	   *dbowner = NULL;
	char	   *dbpath = NULL;
	char	   *dbtemplate = NULL;
	int		    encoding = -1;

	/* Extract options from the statement node tree */
	foreach(option, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "owner") == 0)
		{
			if (downer)
				elog(ERROR, "CREATE DATABASE: conflicting options");
			downer = defel;
		}
		else if (strcmp(defel->defname, "location") == 0)
		{
			if (dpath)
				elog(ERROR, "CREATE DATABASE: conflicting options");
			dpath = defel;
		}
		else if (strcmp(defel->defname, "template") == 0)
		{
			if (dtemplate)
				elog(ERROR, "CREATE DATABASE: conflicting options");
			dtemplate = defel;
		}
		else if (strcmp(defel->defname, "encoding") == 0)
		{
			if (dencoding)
				elog(ERROR, "CREATE DATABASE: conflicting options");
			dencoding = defel;
		}
		else
			elog(ERROR, "CREATE DATABASE: option \"%s\" not recognized",
				 defel->defname);
	}

	if (downer)
		dbowner = strVal(downer->arg);
	if (dpath)
		dbpath = strVal(dpath->arg);
	if (dtemplate)
		dbtemplate = strVal(dtemplate->arg);
	if (dencoding)
		encoding = intVal(dencoding->arg);

	/* obtain sysid of proposed owner */
	if (dbowner)
		datdba = get_usesysid(dbowner);	/* will elog if no such user */
	else
		datdba = GetUserId();

	if (datdba == (int32) GetUserId())
	{
		/* creating database for self: can be superuser or createdb */
		if (!superuser() && !have_createdb_privilege())
			elog(ERROR, "CREATE DATABASE: permission denied");
	}
	else
	{
		/* creating database for someone else: must be superuser */
		/* note that the someone else need not have any permissions */
		if (!superuser())
			elog(ERROR, "CREATE DATABASE: permission denied");
	}

	/* don't call this in a transaction block */
	if (IsTransactionBlock())
		elog(ERROR, "CREATE DATABASE: may not be called in a transaction block");

	/*
	 * Check for db name conflict.	There is a race condition here, since
	 * another backend could create the same DB name before we commit.
	 * However, holding an exclusive lock on pg_database for the whole
	 * time we are copying the source database doesn't seem like a good
	 * idea, so accept possibility of race to create.  We will check again
	 * after we grab the exclusive lock.
	 */
	if (get_db_info(dbname, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
		elog(ERROR, "CREATE DATABASE: database \"%s\" already exists", dbname);

	/*
	 * Lookup database (template) to be cloned.
	 */
	if (!dbtemplate)
		dbtemplate = "template1";		/* Default template database name */

	if (!get_db_info(dbtemplate, &src_dboid, &src_owner, &src_encoding,
					 &src_istemplate, &src_lastsysoid,
					 &src_vacuumxid, &src_frozenxid,
					 src_dbpath))
		elog(ERROR, "CREATE DATABASE: template \"%s\" does not exist",
			 dbtemplate);

	/*
	 * Permission check: to copy a DB that's not marked datistemplate, you
	 * must be superuser or the owner thereof.
	 */
	if (!src_istemplate)
	{
		if (!superuser() && GetUserId() != src_owner )
			elog(ERROR, "CREATE DATABASE: permission to copy \"%s\" denied",
				 dbtemplate);
	}

	/*
	 * Determine physical path of source database
	 */
	alt_loc = resolve_alt_dbpath(src_dbpath, src_dboid);
	if (!alt_loc)
		alt_loc = GetDatabasePath(src_dboid);
	strcpy(src_loc, alt_loc);

	/*
	 * The source DB can't have any active backends, except this one
	 * (exception is to allow CREATE DB while connected to template1).
	 * Otherwise we might copy inconsistent data.  This check is not
	 * bulletproof, since someone might connect while we are copying...
	 */
	if (DatabaseHasActiveBackends(src_dboid, true))
		elog(ERROR, "CREATE DATABASE: source database \"%s\" is being accessed by other users", dbtemplate);

	/* If encoding is defaulted, use source's encoding */
	if (encoding < 0)
		encoding = src_encoding;

#ifdef MULTIBYTE
	/* Some encodings are client only */
	if (!PG_VALID_BE_ENCODING(encoding))
		elog(ERROR, "CREATE DATABASE: invalid backend encoding");
#else
	Assert(encoding == 0);		/* zero is PG_SQL_ASCII */
#endif

	/*
	 * Preassign OID for pg_database tuple, so that we can compute db
	 * path.
	 */
	dboid = newoid();

	/*
	 * Compute nominal location (where we will try to access the
	 * database), and resolve alternate physical location if one is
	 * specified.
	 *
	 * If an alternate location is specified but is the same as the
	 * normal path, just drop the alternate-location spec (this seems
	 * friendlier than erroring out).  We must test this case to avoid
	 * creating a circular symlink below.
	 */
	nominal_loc = GetDatabasePath(dboid);
	alt_loc = resolve_alt_dbpath(dbpath, dboid);

	if (alt_loc && strcmp(alt_loc, nominal_loc) == 0)
	{
		alt_loc = NULL;
		dbpath = NULL;
	}

	if (strchr(nominal_loc, '\''))
		elog(ERROR, "database path may not contain single quotes");
	if (alt_loc && strchr(alt_loc, '\''))
		elog(ERROR, "database path may not contain single quotes");
	if (strchr(src_loc, '\''))
		elog(ERROR, "database path may not contain single quotes");
	/* ... otherwise we'd be open to shell exploits below */

	/*
	 * Force dirty buffers out to disk, to ensure source database is
	 * up-to-date for the copy.  (We really only need to flush buffers for
	 * the source database...)
	 */
	BufferSync();

	/*
	 * Close virtual file descriptors so the kernel has more available for
	 * the mkdir() and system() calls below.
	 */
	closeAllVfds();

	/*
	 * Check we can create the target directory --- but then remove it
	 * because we rely on cp(1) to create it for real.
	 */
	target_dir = alt_loc ? alt_loc : nominal_loc;

	if (mkdir(target_dir, S_IRWXU) != 0)
		elog(ERROR, "CREATE DATABASE: unable to create database directory '%s': %m",
			 target_dir);
	if (rmdir(target_dir) != 0)
		elog(ERROR, "CREATE DATABASE: unable to remove temp directory '%s': %m",
			 target_dir);

	/* Make the symlink, if needed */
	if (alt_loc)
	{
		if (symlink(alt_loc, nominal_loc) != 0)
			elog(ERROR, "CREATE DATABASE: could not link '%s' to '%s': %m",
				 nominal_loc, alt_loc);
	}

	/* Copy the template database to the new location */
	snprintf(buf, sizeof(buf), "cp -r '%s' '%s'", src_loc, target_dir);

	if (system(buf) != 0)
	{
		if (remove_dbdirs(nominal_loc, alt_loc))
			elog(ERROR, "CREATE DATABASE: could not initialize database directory");
		else
			elog(ERROR, "CREATE DATABASE: could not initialize database directory; delete failed as well");
	}

	/*
	 * Now OK to grab exclusive lock on pg_database.
	 */
	pg_database_rel = heap_openr(DatabaseRelationName, AccessExclusiveLock);

	/* Check to see if someone else created same DB name meanwhile. */
	if (get_db_info(dbname, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
	{
		/* Don't hold lock while doing recursive remove */
		heap_close(pg_database_rel, AccessExclusiveLock);
		remove_dbdirs(nominal_loc, alt_loc);
		elog(ERROR, "CREATE DATABASE: database \"%s\" already exists", dbname);
	}

	/*
	 * Insert a new tuple into pg_database
	 */
	pg_database_dsc = RelationGetDescr(pg_database_rel);

	/* Form tuple */
	MemSet(new_record, 0, sizeof(new_record));
	MemSet(new_record_nulls, ' ', sizeof(new_record_nulls));

	new_record[Anum_pg_database_datname - 1] =
		DirectFunctionCall1(namein, CStringGetDatum(dbname));
	new_record[Anum_pg_database_datdba - 1] = Int32GetDatum(datdba);
	new_record[Anum_pg_database_encoding - 1] = Int32GetDatum(encoding);
	new_record[Anum_pg_database_datistemplate - 1] = BoolGetDatum(false);
	new_record[Anum_pg_database_datallowconn - 1] = BoolGetDatum(true);
	new_record[Anum_pg_database_datlastsysoid - 1] = ObjectIdGetDatum(src_lastsysoid);
	new_record[Anum_pg_database_datvacuumxid - 1] = TransactionIdGetDatum(src_vacuumxid);
	new_record[Anum_pg_database_datfrozenxid - 1] = TransactionIdGetDatum(src_frozenxid);
	/* do not set datpath to null, GetRawDatabaseInfo won't cope */
	new_record[Anum_pg_database_datpath - 1] =
		DirectFunctionCall1(textin, CStringGetDatum(dbpath ? dbpath : ""));

	new_record_nulls[Anum_pg_database_datconfig - 1] = 'n';
	new_record_nulls[Anum_pg_database_datacl - 1] = 'n';

	tuple = heap_formtuple(pg_database_dsc, new_record, new_record_nulls);

	AssertTupleDescHasOid(pg_database_dsc);
	HeapTupleSetOid(tuple, dboid);		/* override heap_insert's OID
										 * selection */

	simple_heap_insert(pg_database_rel, tuple);

	/*
	 * Update indexes
	 */
	if (RelationGetForm(pg_database_rel)->relhasindex)
	{
		Relation	idescs[Num_pg_database_indices];

		CatalogOpenIndices(Num_pg_database_indices,
						   Name_pg_database_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_database_indices, pg_database_rel,
						   tuple);
		CatalogCloseIndices(Num_pg_database_indices, idescs);
	}

	/* Close pg_database, but keep lock till commit */
	heap_close(pg_database_rel, NoLock);

	/*
	 * Force dirty buffers out to disk, so that newly-connecting backends
	 * will see the new database in pg_database right away.  (They'll see
	 * an uncommitted tuple, but they don't care; see GetRawDatabaseInfo.)
	 */
	BufferSync();
}


/*
 * DROP DATABASE
 */
void
dropdb(const char *dbname)
{
	int4		db_owner;
	bool		db_istemplate;
	Oid			db_id;
	char	   *alt_loc;
	char	   *nominal_loc;
	char		dbpath[MAXPGPATH];
	Relation	pgdbrel;
	HeapScanDesc pgdbscan;
	ScanKeyData key;
	HeapTuple	tup;

	AssertArg(dbname);

	if (strcmp(dbname, DatabaseName) == 0)
		elog(ERROR, "DROP DATABASE: cannot be executed on the currently open database");

	if (IsTransactionBlock())
		elog(ERROR, "DROP DATABASE: may not be called in a transaction block");

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

	if (!get_db_info(dbname, &db_id, &db_owner, NULL,
					 &db_istemplate, NULL, NULL, NULL, dbpath))
		elog(ERROR, "DROP DATABASE: database \"%s\" does not exist", dbname);

	if (GetUserId() != db_owner && !superuser())
		elog(ERROR, "DROP DATABASE: permission denied");

	/*
	 * Disallow dropping a DB that is marked istemplate.  This is just to
	 * prevent people from accidentally dropping template0 or template1;
	 * they can do so if they're really determined ...
	 */
	if (db_istemplate)
		elog(ERROR, "DROP DATABASE: database is marked as a template");

	nominal_loc = GetDatabasePath(db_id);
	alt_loc = resolve_alt_dbpath(dbpath, db_id);

	/*
	 * Check for active backends in the target database.
	 */
	if (DatabaseHasActiveBackends(db_id, false))
		elog(ERROR, "DROP DATABASE: database \"%s\" is being accessed by other users", dbname);

	/*
	 * Find the database's tuple by OID (should be unique).
	 */
	ScanKeyEntryInitialize(&key, 0, ObjectIdAttributeNumber,
						   F_OIDEQ, ObjectIdGetDatum(db_id));

	pgdbscan = heap_beginscan(pgdbrel, SnapshotNow, 1, &key);

	tup = heap_getnext(pgdbscan, ForwardScanDirection);
	if (!HeapTupleIsValid(tup))
	{
		/*
		 * This error should never come up since the existence of the
		 * database is checked earlier
		 */
		elog(ERROR, "DROP DATABASE: Database \"%s\" doesn't exist despite earlier reports to the contrary",
			 dbname);
	}

	/* Remove the database's tuple from pg_database */
	simple_heap_delete(pgdbrel, &tup->t_self);

	heap_endscan(pgdbscan);

	/*
	 * Delete any comments associated with the database
	 *
	 * NOTE: this is probably dead code since any such comments should have
	 * been in that database, not mine.
	 */
	DeleteComments(db_id, RelationGetRelid(pgdbrel), 0);

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
	 * Also, clean out any entries in the shared free space map.
	 */
	FreeSpaceMapForgetDatabase(db_id);

	/*
	 * Remove the database's subdirectory and everything in it.
	 */
	remove_dbdirs(nominal_loc, alt_loc);

	/*
	 * Force dirty buffers out to disk, so that newly-connecting backends
	 * will see the database tuple marked dead in pg_database right away.
	 * (They'll see an uncommitted deletion, but they don't care; see
	 * GetRawDatabaseInfo.)
	 */
	BufferSync();
}



/*
 * ALTER DATABASE name SET ...
 */
void
AlterDatabaseSet(AlterDatabaseSetStmt *stmt)
{
	char	   *valuestr;
	HeapTuple	tuple,
				newtuple;
	Relation	rel;
	ScanKeyData	scankey;
	HeapScanDesc scan;
	Datum		repl_val[Natts_pg_database];
	char		repl_null[Natts_pg_database];
	char		repl_repl[Natts_pg_database];

	valuestr = flatten_set_variable_args(stmt->variable, stmt->value);

	rel = heap_openr(DatabaseRelationName, RowExclusiveLock);
	ScanKeyEntryInitialize(&scankey, 0, Anum_pg_database_datname,
						   F_NAMEEQ, NameGetDatum(stmt->dbname));
	scan = heap_beginscan(rel, SnapshotNow, 1, &scankey);
	tuple = heap_getnext(scan, ForwardScanDirection);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "database \"%s\" does not exist", stmt->dbname);

	if (!(superuser()
		  || ((Form_pg_database) GETSTRUCT(tuple))->datdba == GetUserId()))
		elog(ERROR, "permission denied");

	MemSet(repl_repl, ' ', sizeof(repl_repl));
	repl_repl[Anum_pg_database_datconfig-1] = 'r';

	if (strcmp(stmt->variable, "all")==0 && valuestr == NULL)
	{
		/* RESET ALL */
		repl_null[Anum_pg_database_datconfig-1] = 'n';
		repl_val[Anum_pg_database_datconfig-1] = (Datum) 0;
	}
	else
	{
		Datum datum;
		bool isnull;
		ArrayType *a;

		repl_null[Anum_pg_database_datconfig-1] = ' ';

		datum = heap_getattr(tuple, Anum_pg_database_datconfig,
							 RelationGetDescr(rel), &isnull);

		a = isnull ? ((ArrayType *) NULL) : DatumGetArrayTypeP(datum);

		if (valuestr)
			a = GUCArrayAdd(a, stmt->variable, valuestr);
		else
			a = GUCArrayDelete(a, stmt->variable);

		repl_val[Anum_pg_database_datconfig-1] = PointerGetDatum(a);
	}

	newtuple = heap_modifytuple(tuple, rel, repl_val, repl_null, repl_repl);
	simple_heap_update(rel, &tuple->t_self, newtuple);

	/*
	 * Update indexes
	 */
	if (RelationGetForm(rel)->relhasindex)
	{
		Relation	idescs[Num_pg_database_indices];

		CatalogOpenIndices(Num_pg_database_indices,
						   Name_pg_database_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_database_indices, rel,
						   newtuple);
		CatalogCloseIndices(Num_pg_database_indices, idescs);
	}

	heap_endscan(scan);
	heap_close(rel, RowExclusiveLock);
}



/*
 * Helper functions
 */

static bool
get_db_info(const char *name, Oid *dbIdP, int4 *ownerIdP,
			int *encodingP, bool *dbIsTemplateP, Oid *dbLastSysOidP,
			TransactionId *dbVacuumXidP, TransactionId *dbFrozenXidP,
			char *dbpath)
{
	Relation	relation;
	ScanKeyData scanKey;
	HeapScanDesc scan;
	HeapTuple	tuple;
	bool		gottuple;

	AssertArg(name);

	/* Caller may wish to grab a better lock on pg_database beforehand... */
	relation = heap_openr(DatabaseRelationName, AccessShareLock);

	ScanKeyEntryInitialize(&scanKey, 0, Anum_pg_database_datname,
						   F_NAMEEQ, NameGetDatum(name));

	scan = heap_beginscan(relation, SnapshotNow, 1, &scanKey);

	tuple = heap_getnext(scan, ForwardScanDirection);

	gottuple = HeapTupleIsValid(tuple);
	if (gottuple)
	{
		Form_pg_database dbform = (Form_pg_database) GETSTRUCT(tuple);

		/* oid of the database */
		if (dbIdP)
		{
			AssertTupleDescHasOid(relation->rd_att);
			*dbIdP = HeapTupleGetOid(tuple);
		}
		/* sysid of the owner */
		if (ownerIdP)
			*ownerIdP = dbform->datdba;
		/* multibyte encoding */
		if (encodingP)
			*encodingP = dbform->encoding;
		/* allowed as template? */
		if (dbIsTemplateP)
			*dbIsTemplateP = dbform->datistemplate;
		/* last system OID used in database */
		if (dbLastSysOidP)
			*dbLastSysOidP = dbform->datlastsysoid;
		/* limit of vacuumed XIDs */
		if (dbVacuumXidP)
			*dbVacuumXidP = dbform->datvacuumxid;
		/* limit of frozen XIDs */
		if (dbFrozenXidP)
			*dbFrozenXidP = dbform->datfrozenxid;
		/* database path (as registered in pg_database) */
		if (dbpath)
		{
			Datum		datum;
			bool		isnull;

			datum = heap_getattr(tuple,
								 Anum_pg_database_datpath,
								 RelationGetDescr(relation),
								 &isnull);
			if (!isnull)
			{
				text	   *pathtext = DatumGetTextP(datum);
				int			pathlen = VARSIZE(pathtext) - VARHDRSZ;

				Assert(pathlen >= 0 && pathlen < MAXPGPATH);
				strncpy(dbpath, VARDATA(pathtext), pathlen);
				*(dbpath + pathlen) = '\0';
			}
			else
				strcpy(dbpath, "");
		}
	}

	heap_endscan(scan);
	heap_close(relation, AccessShareLock);

	return gottuple;
}

static bool
have_createdb_privilege(void)
{
	HeapTuple	utup;
	bool		retval;

	utup = SearchSysCache(SHADOWSYSID,
						  ObjectIdGetDatum(GetUserId()),
						  0, 0, 0);

	if (!HeapTupleIsValid(utup))
		retval = false;
	else
		retval = ((Form_pg_shadow) GETSTRUCT(utup))->usecreatedb;

	ReleaseSysCache(utup);

	return retval;
}


static char *
resolve_alt_dbpath(const char *dbpath, Oid dboid)
{
	const char *prefix;
	char	   *ret;
	size_t		len;

	if (dbpath == NULL || dbpath[0] == '\0')
		return NULL;

	if (strchr(dbpath, '/'))
	{
		if (dbpath[0] != '/')
			elog(ERROR, "Relative paths are not allowed as database locations");
#ifndef ALLOW_ABSOLUTE_DBPATHS
		elog(ERROR, "Absolute paths are not allowed as database locations");
#endif
		prefix = dbpath;
	}
	else
	{
		/* must be environment variable */
		char	   *var = getenv(dbpath);

		if (!var)
			elog(ERROR, "Postmaster environment variable '%s' not set", dbpath);
		if (var[0] != '/')
			elog(ERROR, "Postmaster environment variable '%s' must be absolute path", dbpath);
		prefix = var;
	}

	len = strlen(prefix) + 6 + sizeof(Oid) * 8 + 1;
	if (len >= MAXPGPATH - 100)
		elog(ERROR, "Alternate path is too long");

	ret = palloc(len);
	snprintf(ret, len, "%s/base/%u", prefix, dboid);

	return ret;
}


static bool
remove_dbdirs(const char *nominal_loc, const char *alt_loc)
{
	const char *target_dir;
	char		buf[MAXPGPATH + 100];
	bool		success = true;

	target_dir = alt_loc ? alt_loc : nominal_loc;

	/*
	 * Close virtual file descriptors so the kernel has more available for
	 * the system() call below.
	 */
	closeAllVfds();

	if (alt_loc)
	{
		/* remove symlink */
		if (unlink(nominal_loc) != 0)
		{
			elog(WARNING, "could not remove '%s': %m", nominal_loc);
			success = false;
		}
	}

	snprintf(buf, sizeof(buf), "rm -rf '%s'", target_dir);

	if (system(buf) != 0)
	{
		elog(WARNING, "database directory '%s' could not be removed",
			 target_dir);
		success = false;
	}

	return success;
}
