/*
 * dbsize.c
 *		object size functions
 *
 * Copyright (c) 2002-2005, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/utils/adt/dbsize.c,v 1.7 2005/10/29 00:31:51 petere Exp $
 *
 */

#include "postgres.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "access/heapam.h"
#include "catalog/namespace.h"
#include "catalog/pg_tablespace.h"
#include "commands/dbcommands.h"
#include "commands/tablespace.h"
#include "miscadmin.h"
#include "storage/fd.h"
#include "utils/builtins.h"
#include "utils/syscache.h"
#include "utils/relcache.h"


/* Return physical size of directory contents, or 0 if dir doesn't exist */
static int64
db_dir_size(const char *path)
{
	int64		dirsize = 0;
	struct dirent *direntry;
	DIR		   *dirdesc;
	char		filename[MAXPGPATH];

	dirdesc = AllocateDir(path);

	if (!dirdesc)
		return 0;

	while ((direntry = ReadDir(dirdesc, path)) != NULL)
	{
		struct stat fst;

		if (strcmp(direntry->d_name, ".") == 0 ||
			strcmp(direntry->d_name, "..") == 0)
			continue;

		snprintf(filename, MAXPGPATH, "%s/%s", path, direntry->d_name);

		if (stat(filename, &fst) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not stat file \"%s\": %m", filename)));

		dirsize += fst.st_size;
	}

	FreeDir(dirdesc);
	return dirsize;
}

/*
 * calculate size of database in all tablespaces
 */
static int64
calculate_database_size(Oid dbOid)
{
	int64		totalsize;
	DIR		   *dirdesc;
	struct dirent *direntry;
	char		dirpath[MAXPGPATH];
	char		pathname[MAXPGPATH];

	/* Shared storage in pg_global is not counted */

	/* Include pg_default storage */
	snprintf(pathname, MAXPGPATH, "%s/base/%u", DataDir, dbOid);
	totalsize = db_dir_size(pathname);

	/* Scan the non-default tablespaces */
	snprintf(dirpath, MAXPGPATH, "%s/pg_tblspc", DataDir);
	dirdesc = AllocateDir(dirpath);
	if (!dirdesc)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open tablespace directory \"%s\": %m",
						dirpath)));

	while ((direntry = ReadDir(dirdesc, dirpath)) != NULL)
	{
		if (strcmp(direntry->d_name, ".") == 0 ||
			strcmp(direntry->d_name, "..") == 0)
			continue;

		snprintf(pathname, MAXPGPATH, "%s/pg_tblspc/%s/%u",
				 DataDir, direntry->d_name, dbOid);
		totalsize += db_dir_size(pathname);
	}

	FreeDir(dirdesc);

	/* Complain if we found no trace of the DB at all */
	if (!totalsize)
		ereport(ERROR,
				(ERRCODE_UNDEFINED_DATABASE,
				 errmsg("database with OID %u does not exist", dbOid)));

	return totalsize;
}

Datum
pg_database_size_oid(PG_FUNCTION_ARGS)
{
	Oid			dbOid = PG_GETARG_OID(0);

	PG_RETURN_INT64(calculate_database_size(dbOid));
}

Datum
pg_database_size_name(PG_FUNCTION_ARGS)
{
	Name		dbName = PG_GETARG_NAME(0);
	Oid			dbOid = get_database_oid(NameStr(*dbName));

	if (!OidIsValid(dbOid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_DATABASE),
				 errmsg("database \"%s\" does not exist",
						NameStr(*dbName))));

	PG_RETURN_INT64(calculate_database_size(dbOid));
}


/*
 * calculate total size of tablespace
 */
static int64
calculate_tablespace_size(Oid tblspcOid)
{
	char		tblspcPath[MAXPGPATH];
	char		pathname[MAXPGPATH];
	int64		totalsize = 0;
	DIR		   *dirdesc;
	struct dirent *direntry;

	if (tblspcOid == DEFAULTTABLESPACE_OID)
		snprintf(tblspcPath, MAXPGPATH, "%s/base", DataDir);
	else if (tblspcOid == GLOBALTABLESPACE_OID)
		snprintf(tblspcPath, MAXPGPATH, "%s/global", DataDir);
	else
		snprintf(tblspcPath, MAXPGPATH, "%s/pg_tblspc/%u", DataDir, tblspcOid);

	dirdesc = AllocateDir(tblspcPath);

	if (!dirdesc)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open tablespace directory \"%s\": %m",
						tblspcPath)));

	while ((direntry = ReadDir(dirdesc, tblspcPath)) != NULL)
	{
		struct stat fst;

		if (strcmp(direntry->d_name, ".") == 0 ||
			strcmp(direntry->d_name, "..") == 0)
			continue;

		snprintf(pathname, MAXPGPATH, "%s/%s", tblspcPath, direntry->d_name);

		if (stat(pathname, &fst) < 0)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not stat file \"%s\": %m", pathname)));

		if (fst.st_mode & S_IFDIR)
			totalsize += db_dir_size(pathname);

		totalsize += fst.st_size;
	}

	FreeDir(dirdesc);

	return totalsize;
}

Datum
pg_tablespace_size_oid(PG_FUNCTION_ARGS)
{
	Oid			tblspcOid = PG_GETARG_OID(0);

	PG_RETURN_INT64(calculate_tablespace_size(tblspcOid));
}

Datum
pg_tablespace_size_name(PG_FUNCTION_ARGS)
{
	Name		tblspcName = PG_GETARG_NAME(0);
	Oid			tblspcOid = get_tablespace_oid(NameStr(*tblspcName));

	if (!OidIsValid(tblspcOid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("tablespace \"%s\" does not exist",
						NameStr(*tblspcName))));

	PG_RETURN_INT64(calculate_tablespace_size(tblspcOid));
}


/*
 * calculate size of a relation
 */
static int64
calculate_relation_size(RelFileNode *rfn)
{
	int64		totalsize = 0;
	char		dirpath[MAXPGPATH];
	char		pathname[MAXPGPATH];
	unsigned int segcount = 0;

	Assert(OidIsValid(rfn->spcNode));

	if (rfn->spcNode == DEFAULTTABLESPACE_OID)
		snprintf(dirpath, MAXPGPATH, "%s/base/%u", DataDir, rfn->dbNode);
	else if (rfn->spcNode == GLOBALTABLESPACE_OID)
		snprintf(dirpath, MAXPGPATH, "%s/global", DataDir);
	else
		snprintf(dirpath, MAXPGPATH, "%s/pg_tblspc/%u/%u",
				 DataDir, rfn->spcNode, rfn->dbNode);

	for (segcount = 0;; segcount++)
	{
		struct stat fst;

		if (segcount == 0)
			snprintf(pathname, MAXPGPATH, "%s/%u",
					 dirpath, rfn->relNode);
		else
			snprintf(pathname, MAXPGPATH, "%s/%u.%u",
					 dirpath, rfn->relNode, segcount);

		if (stat(pathname, &fst) < 0)
		{
			if (errno == ENOENT)
				break;
			else
				ereport(ERROR,
						(errcode_for_file_access(),
						 errmsg("could not stat file \"%s\": %m", pathname)));
		}
		totalsize += fst.st_size;
	}

	return totalsize;
}

Datum
pg_relation_size_oid(PG_FUNCTION_ARGS)
{
	Oid			relOid = PG_GETARG_OID(0);
	Relation	rel;
	int64		size;

	rel = relation_open(relOid, AccessShareLock);

	size = calculate_relation_size(&(rel->rd_node));

	relation_close(rel, AccessShareLock);

	PG_RETURN_INT64(size);
}

Datum
pg_relation_size_name(PG_FUNCTION_ARGS)
{
	text	   *relname = PG_GETARG_TEXT_P(0);
	RangeVar   *relrv;
	Relation	rel;
	int64		size;

	relrv = makeRangeVarFromNameList(textToQualifiedNameList(relname));
	rel = relation_openrv(relrv, AccessShareLock);

	size = calculate_relation_size(&(rel->rd_node));

	relation_close(rel, AccessShareLock);

	PG_RETURN_INT64(size);
}


/*
 *	Compute the on-disk size of files for the relation according to the
 *	stat function, optionally including heap data, index data, and/or
 *	toast data.
 */
static int64
calculate_total_relation_size(Oid Relid)
{
	Relation	heapRel;
	Oid			toastOid;
	int64		size;
	ListCell   *cell;

	heapRel = relation_open(Relid, AccessShareLock);
	toastOid = heapRel->rd_rel->reltoastrelid;

	/* Get the heap size */
	size = calculate_relation_size(&(heapRel->rd_node));

	/* Get index size */
	if (heapRel->rd_rel->relhasindex)
	{
		/* recursively include any dependent indexes */
		List	   *index_oids = RelationGetIndexList(heapRel);

		foreach(cell, index_oids)
		{
			Oid			idxOid = lfirst_oid(cell);
			Relation	iRel;

			iRel = relation_open(idxOid, AccessShareLock);

			size += calculate_relation_size(&(iRel->rd_node));

			relation_close(iRel, AccessShareLock);
		}

		list_free(index_oids);
	}

	/* Get toast table (and index) size */
	if (OidIsValid(toastOid))
		size += calculate_total_relation_size(toastOid);

	relation_close(heapRel, AccessShareLock);

	return size;
}

/*
 *	Compute on-disk size of files for 'relation' including
 *	heap data, index data, and toasted data.
 */
Datum
pg_total_relation_size_oid(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);

	PG_RETURN_INT64(calculate_total_relation_size(relid));
}

Datum
pg_total_relation_size_name(PG_FUNCTION_ARGS)
{
	text	   *relname = PG_GETARG_TEXT_P(0);
	RangeVar   *relrv;
	Oid			relid;

	relrv = makeRangeVarFromNameList(textToQualifiedNameList(relname));
	relid = RangeVarGetRelid(relrv, false);

	PG_RETURN_INT64(calculate_total_relation_size(relid));
}

/*
 * formatting with size units
 */
Datum
pg_size_pretty(PG_FUNCTION_ARGS)
{
	int64		size = PG_GETARG_INT64(0);
	char	   *result = palloc(50 + VARHDRSZ);
	int64		limit = 10 * 1024;
	int64		mult = 1;

	if (size < limit * mult)
		snprintf(VARDATA(result), 50, INT64_FORMAT " bytes", size);
	else
	{
		mult *= 1024;
		if (size < limit * mult)
			snprintf(VARDATA(result), 50, INT64_FORMAT " kB",
					 (size + mult / 2) / mult);
		else
		{
			mult *= 1024;
			if (size < limit * mult)
				snprintf(VARDATA(result), 50, INT64_FORMAT " MB",
						 (size + mult / 2) / mult);
			else
			{
				mult *= 1024;
				if (size < limit * mult)
					snprintf(VARDATA(result), 50, INT64_FORMAT " GB",
							 (size + mult / 2) / mult);
				else
				{
					mult *= 1024;
					snprintf(VARDATA(result), 50, INT64_FORMAT " TB",
							 (size + mult / 2) / mult);
				}
			}
		}
	}

	VARATT_SIZEP(result) = strlen(VARDATA(result)) + VARHDRSZ;

	PG_RETURN_TEXT_P(result);
}
