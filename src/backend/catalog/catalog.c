/*-------------------------------------------------------------------------
 *
 * catalog.c
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/catalog.c,v 1.41 2001-05-30 14:15:26 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/transam.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "utils/lsyscache.h"

#ifdef OLD_FILE_NAMING
/*
 * relpath				- construct path to a relation's file
 *
 * Note that this only works with relations that are visible to the current
 * backend, ie, either in the current database or shared system relations.
 *
 * Result is a palloc'd string.
 */
char *
relpath(const char *relname)
{
	char	   *path;

	if (IsSharedSystemRelationName(relname))
	{
		/* Shared system relations live in {datadir}/global */
		size_t		bufsize = strlen(DataDir) + 8 + sizeof(NameData) + 1;

		path = (char *) palloc(bufsize);
		snprintf(path, bufsize, "%s/global/%s", DataDir, relname);
		return path;
	}

	/*
	 * If it is in the current database, assume it is in current working
	 * directory.  NB: this does not work during bootstrap!
	 */
	return pstrdup(relname);
}

/*
 * relpath_blind			- construct path to a relation's file
 *
 * Construct the path using only the info available to smgrblindwrt,
 * namely the names and OIDs of the database and relation.	(Shared system
 * relations are identified with dbid = 0.)  Note that we may have to
 * access a relation belonging to a different database!
 *
 * Result is a palloc'd string.
 */

char *
relpath_blind(const char *dbname, const char *relname,
			  Oid dbid, Oid relid)
{
	char	   *path;

	if (dbid == (Oid) 0)
	{
		/* Shared system relations live in {datadir}/global */
		path = (char *) palloc(strlen(DataDir) + 8 + sizeof(NameData) + 1);
		sprintf(path, "%s/global/%s", DataDir, relname);
	}
	else if (dbid == MyDatabaseId)
	{
		/* XXX why is this inconsistent with relpath() ? */
		path = (char *) palloc(strlen(DatabasePath) + sizeof(NameData) + 2);
		sprintf(path, "%s/%s", DatabasePath, relname);
	}
	else
	{
		/* this is work around only !!! */
		char		dbpathtmp[MAXPGPATH];
		Oid			id;
		char	   *dbpath;

		GetRawDatabaseInfo(dbname, &id, dbpathtmp);

		if (id != dbid)
			elog(FATAL, "relpath_blind: oid of db %s is not %u",
				 dbname, dbid);
		dbpath = ExpandDatabasePath(dbpathtmp);
		if (dbpath == NULL)
			elog(FATAL, "relpath_blind: can't expand path for db %s",
				 dbname);
		path = (char *) palloc(strlen(dbpath) + sizeof(NameData) + 2);
		sprintf(path, "%s/%s", dbpath, relname);
		pfree(dbpath);
	}
	return path;
}

#else							/* ! OLD_FILE_NAMING */

/*
 * relpath			- construct path to a relation's file
 *
 * Result is a palloc'd string.
 */

char *
relpath(RelFileNode rnode)
{
	char	   *path;

	if (rnode.tblNode == (Oid) 0)		/* "global tablespace" */
	{
		/* Shared system relations live in {datadir}/global */
		path = (char *) palloc(strlen(DataDir) + 8 + sizeof(NameData) + 1);
		sprintf(path, "%s/global/%u", DataDir, rnode.relNode);
	}
	else
	{
		path = (char *) palloc(strlen(DataDir) + 6 + 2 * sizeof(NameData) + 3);
		sprintf(path, "%s/base/%u/%u", DataDir, rnode.tblNode, rnode.relNode);
	}
	return path;
}

/*
 * GetDatabasePath			- construct path to a database dir
 *
 * Result is a palloc'd string.
 */

char *
GetDatabasePath(Oid tblNode)
{
	char	   *path;

	if (tblNode == (Oid) 0)		/* "global tablespace" */
	{
		/* Shared system relations live in {datadir}/global */
		path = (char *) palloc(strlen(DataDir) + 8);
		sprintf(path, "%s/global", DataDir);
	}
	else
	{
		path = (char *) palloc(strlen(DataDir) + 6 + sizeof(NameData) + 1);
		sprintf(path, "%s/base/%u", DataDir, tblNode);
	}
	return path;
}

#endif	 /* OLD_FILE_NAMING */

/*
 * IsSystemRelationName
 *		True iff name is the name of a system catalog relation.
 *
 *		We now make a new requirement where system catalog relns must begin
 *		with pg_ while user relns are forbidden to do so.  Make the test
 *		trivial and instantaneous.
 *
 *		XXX this is way bogus. -- pma
 */
bool
IsSystemRelationName(const char *relname)
{
	if (relname[0] && relname[1] && relname[2])
		return (relname[0] == 'p' &&
				relname[1] == 'g' &&
				relname[2] == '_');
	else
		return FALSE;
}

/*
 * IsSharedSystemRelationName
 *		True iff name is the name of a shared system catalog relation.
 */
bool
IsSharedSystemRelationName(const char *relname)
{
	int			i;

	/*
	 * Quick out: if it's not a system relation, it can't be a shared
	 * system relation.
	 */
	if (!IsSystemRelationName(relname))
		return FALSE;

	i = 0;
	while (SharedSystemRelationNames[i] != NULL)
	{
		if (strcmp(SharedSystemRelationNames[i], relname) == 0)
			return TRUE;
		i++;
	}
	return FALSE;
}

/*
 *		newoid			- returns a unique identifier across all catalogs.
 *
 *		Object Id allocation is now done by GetNewObjectID in
 *		access/transam/varsup.c.  oids are now allocated correctly.
 *
 * old comments:
 *		This needs to change soon, it fails if there are too many more
 *		than one call per second when postgres restarts after it dies.
 *
 *		The distribution of OID's should be done by the POSTMASTER.
 *		Also there needs to be a facility to preallocate OID's.  Ie.,
 *		for a block of OID's to be declared as invalid ones to allow
 *		user programs to use them for temporary object identifiers.
 */
Oid
newoid(void)
{
	Oid			lastoid;

	GetNewObjectId(&lastoid);
	if (!OidIsValid(lastoid))
		elog(ERROR, "newoid: GetNewObjectId returns invalid oid");
	return lastoid;
}
