/*-------------------------------------------------------------------------
 *
 * postinit.c
 *	  postgres initialization utilities
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/init/postinit.c,v 1.51 1999-10-06 21:58:10 vadim Exp $
 *
 * NOTES
 *		InitPostgres() is the function called from PostgresMain
 *		which does all non-trival initialization, mainly by calling
 *		all the other initialization functions.  InitPostgres()
 *		is only used within the "postgres" backend and so that routine
 *		is in tcop/postgres.c  InitPostgres() is needed in cinterface.a
 *		because things like the bootstrap backend program need it. Hence
 *		you find that in this file...
 *
 *		If you feel the need to add more initialization code, it should be
 *		done in InitPostgres() or someplace lower.	Do not start
 *		putting stuff in PostgresMain - if you do then someone
 *		will have to clean it up later, and it's not going to be me!
 *		-cim 10/3/90
 *
 *-------------------------------------------------------------------------
 */
#include <fcntl.h>
#include <sys/file.h>
#include <sys/types.h>
#include <math.h>
#include <unistd.h>

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_database.h"
#include "libpq/libpq.h"
#include "miscadmin.h"
#include "storage/backendid.h"
#include "storage/proc.h"
#include "storage/sinval.h"
#include "storage/smgr.h"
#include "utils/inval.h"
#include "utils/portal.h"
#include "utils/relcache.h"
#include "utils/syscache.h"
#include "version.h"

#ifdef MULTIBYTE
#include "mb/pg_wchar.h"
#endif

void		BaseInit(void);

static void VerifySystemDatabase(void);
static void VerifyMyDatabase(void);
static void ReverifyMyDatabase(char *name);
static void InitCommunication(void);
static void InitMyDatabaseInfo(char *name);
static void InitUserid(void);


static IPCKey PostgresIpcKey;

/* ----------------------------------------------------------------
 *						InitPostgres support
 * ----------------------------------------------------------------
 */

/* --------------------------------
 *	InitMyDatabaseInfo() -- Find and record the OID of the database we are
 *						  to open.
 *
 *		The database's oid forms half of the unique key for the system
 *		caches and lock tables.  We therefore want it initialized before
 *		we open any relations, since opening relations puts things in the
 *		cache.	To get around this problem, this code opens and scans the
 *		pg_database relation by hand.
 *
 *		This algorithm relies on the fact that first attribute in the
 *		pg_database relation schema is the database name.  It also knows
 *		about the internal format of tuples on disk and the length of
 *		the datname attribute.	It knows the location of the pg_database
 *		file.
 *		Actually, the code looks as though it is using the pg_database
 *		tuple definition to locate the database name, so the above statement
 *		seems to be no longer correct. - thomas 1997-11-01
 *
 *		This code is called from InitPostgres(), before we chdir() to the
 *		local database directory and before we open any relations.
 *		Used to be called after the chdir(), but we now want to confirm
 *		the location of the target database using pg_database info.
 *		- thomas 1997-11-01
 * --------------------------------
 */
static void
InitMyDatabaseInfo(char *name)
{
	char	   *path,
				myPath[MAXPGPATH + 1];

	SetDatabaseName(name);
	GetRawDatabaseInfo(name, &MyDatabaseId, myPath);

	if (!OidIsValid(MyDatabaseId))
		elog(FATAL,
			 "Database %s does not exist in %s",
			 DatabaseName,
			 DatabaseRelationName);

	path = ExpandDatabasePath(myPath);
	SetDatabasePath(path);
}	/* InitMyDatabaseInfo() */


/*
 * DoChdirAndInitDatabaseNameAndPath
 *		Set current directory to the database directory for the database
 *		named <name>.
 *		Also set global variables DatabasePath and DatabaseName to those
 *		values.  Also check for proper version of database system and
 *		database.  Exit program via elog() if anything doesn't check out.
 *
 * Arguments:
 *		Path and name are invalid if it invalid as a string.
 *		Path is "badly formatted" if it is not a string containing a path
 *		to a writable directory.
 *		Name is "badly formatted" if it contains more than 16 characters or if
 *		it is a bad file name (e.g., it contains a '/' or an 8-bit character).
 *
 * Exceptions:
 *		BadState if called more than once.
 *		BadArg if both path and name are "badly formatted" or invalid.
 *		BadArg if path and name are both "inconsistent" and valid.
 *
 *		This routine is inappropriate in bootstrap mode, since the directories
 *		and version files need not exist yet if we're in bootstrap mode.
 */
static void
VerifySystemDatabase()
{
	char	   *reason;

	/* Failure reason returned by some function.  NULL if no failure */
	int			fd;
	char		errormsg[1000];

	errormsg[0] = '\0';

#ifndef __CYGWIN32__
	if ((fd = open(DataDir, O_RDONLY, 0)) == -1)
#else
	if ((fd = open(DataDir, O_RDONLY | O_DIROPEN, 0)) == -1)
#endif
		sprintf(errormsg, "Database system does not exist.  "
				"PGDATA directory '%s' not found.\n\tNormally, you "
				"create a database system by running initdb.",
				DataDir);
	else
	{
		close(fd);
		ValidatePgVersion(DataDir, &reason);
		if (reason != NULL)
			sprintf(errormsg,
					"InitPostgres could not validate that the database"
					" system version is compatible with this level of"
					" Postgres.\n\tYou may need to run initdb to create"
					" a new database system.\n\t%s", reason);
	}
	if (errormsg[0] != '\0')
		elog(FATAL, errormsg);
	/* Above does not return */
}	/* VerifySystemDatabase() */


static void
VerifyMyDatabase()
{
	const char *name;
	const char *myPath;

	/* Failure reason returned by some function.  NULL if no failure */
	char	   *reason;
	int			fd;
	char		errormsg[1000];

	name = DatabaseName;
	myPath = DatabasePath;

#ifndef __CYGWIN32__
	if ((fd = open(myPath, O_RDONLY, 0)) == -1)
#else
	if ((fd = open(myPath, O_RDONLY | O_DIROPEN, 0)) == -1)
#endif
		sprintf(errormsg,
				"Database '%s' does not exist."
			"\n\tWe know this because the directory '%s' does not exist."
				"\n\tYou can create a database with the SQL command"
				" CREATE DATABASE.\n\tTo see what databases exist,"
				" look at the subdirectories of '%s/base/'.",
				name, myPath, DataDir);
	else
	{
		close(fd);
		ValidatePgVersion(myPath, &reason);
		if (reason != NULL)
			sprintf(errormsg,
					"InitPostgres could not validate that the database"
					" version is compatible with this level of Postgres"
					"\n\teven though the database system as a whole"
					" appears to be at a compatible level."
					"\n\tYou may need to recreate the database with SQL"
					" commands DROP DATABASE and CREATE DATABASE."
					"\n\t%s", reason);
		else
		{

			/*
			 * The directories and PG_VERSION files are in order.
			 */
			int			rc;		/* return code from some function we call */

#ifdef FILEDEBUG
			printf("Try changing directory for database %s to %s\n", name, myPath);
#endif

			rc = chdir(myPath);
			if (rc < 0)
				sprintf(errormsg,
						"InitPostgres unable to change "
						"current directory to '%s', errno = %s (%d).",
						myPath, strerror(errno), errno);
			else
				errormsg[0] = '\0';
		}
	}

	if (errormsg[0] != '\0')
		elog(FATAL, errormsg);
	/* Above does not return */
}	/* VerifyMyDatabase() */

/* --------------------------------
 *		ReverifyMyDatabase
 *
 * Since we are forced to fetch the database OID out of pg_database without
 * benefit of locking or transaction ID checking (see utils/misc/database.c),
 * we might have gotten a wrong answer.  Or, we might have attached to a
 * database that's in process of being destroyed by destroydb().  This
 * routine is called after we have all the locking and other infrastructure
 * running --- now we can check that we are really attached to a valid
 * database.
 *
 * In reality, if destroydb() is running in parallel with our startup,
 * it's pretty likely that we will have failed before now, due to being
 * unable to read some of the system tables within the doomed database.
 * This routine just exists to make *sure* we have not started up in an
 * invalid database.  If we quit now, we should have managed to avoid
 * creating any serious problems.
 *
 * This is also a handy place to fetch the database encoding info out
 * of pg_database, if we are in MULTIBYTE mode.
 * --------------------------------
 */
static void
ReverifyMyDatabase(char *name)
{
	Relation	pgdbrel;
	HeapScanDesc pgdbscan;
	ScanKeyData	key;
	HeapTuple	tup;

	/*
	 * Because we grab AccessShareLock here, we can be sure that
	 * destroydb is not running in parallel with us (any more).
	 */
	pgdbrel = heap_openr(DatabaseRelationName, AccessShareLock);

	ScanKeyEntryInitialize(&key, 0, Anum_pg_database_datname,
						   F_NAMEEQ, NameGetDatum(name));

	pgdbscan = heap_beginscan(pgdbrel, 0, SnapshotNow, 1, &key);

	tup = heap_getnext(pgdbscan, 0);
	if (!HeapTupleIsValid(tup) ||
		tup->t_data->t_oid != MyDatabaseId)
	{
		/* OOPS */
		heap_close(pgdbrel, AccessShareLock);
		/*
		 * The only real problem I could have created is to load dirty
		 * buffers for the dead database into shared buffer cache;
		 * if I did, some other backend will eventually try to write
		 * them and die in mdblindwrt.  Flush any such pages to forestall
		 * trouble.
		 */
		DropBuffers(MyDatabaseId);
		/* Now I can commit hara-kiri with a clear conscience... */
		elog(FATAL, "Database '%s', OID %u, has disappeared from pg_database",
			 name, MyDatabaseId);
	}

	/*
	 * OK, we're golden.  Only other to-do item is to save the MULTIBYTE
	 * encoding info out of the pg_database tuple.  Note we also set the
	 * "template encoding", which is the default encoding for any
	 * CREATE DATABASE commands executed in this backend; essentially,
	 * you get the same encoding of the database you connected to as
	 * the default.  (This replaces code that unreliably grabbed
	 * template1's encoding out of pg_database.  We could do an extra
	 * scan to find template1's tuple, but for 99.99% of all backend
	 * startups it'd be wasted cycles --- and the 'createdb' script
	 * connects to template1 anyway, so there's no difference.)
	 */
#ifdef MULTIBYTE
	SetDatabaseEncoding(((Form_pg_database) GETSTRUCT(tup))->encoding);
	SetTemplateEncoding(((Form_pg_database) GETSTRUCT(tup))->encoding);
#endif

	heap_endscan(pgdbscan);
	heap_close(pgdbrel, AccessShareLock);
}

/* --------------------------------
 *		InitUserid
 *
 *		initializes crap associated with the user id.
 * --------------------------------
 */
static void
InitUserid()
{
	setuid(geteuid());
	SetUserId();
}

/* --------------------------------
 *		InitCommunication
 *
 *		This routine initializes stuff needed for ipc, locking, etc.
 *		it should be called something more informative.
 *
 * Note:
 *		This does not set MyBackendId.	MyBackendTag is set, however.
 * --------------------------------
 */
static void
InitCommunication()
{
	char	   *postid;			/* value of environment variable */
	char	   *postport;		/* value of environment variable */
	char	   *ipc_key;		/* value of environemnt variable */
	IPCKey		key = 0;

	/* ----------------
	 *	try and get the backend tag from POSTID
	 * ----------------
	 */
	MyBackendId = -1;

	postid = getenv("POSTID");
	if (!PointerIsValid(postid))
		MyBackendTag = -1;
	else
	{
		MyBackendTag = atoi(postid);
		Assert(MyBackendTag >= 0);
	}


	ipc_key = getenv("IPC_KEY");
	if (!PointerIsValid(ipc_key))
		key = -1;
	else
	{
		key = atoi(ipc_key);
		Assert(MyBackendTag >= 0);
	}

	postport = getenv("POSTPORT");

	if (PointerIsValid(postport))
	{
		if (MyBackendTag == -1)
			elog(FATAL, "InitCommunication: missing POSTID");
	}
	else if (IsUnderPostmaster)
	{
		elog(FATAL,
			 "InitCommunication: under postmaster and POSTPORT not set");
	}
	else
	{
		/* ----------------
		 *	assume we're running a postgres backend by itself with
		 *	no front end or postmaster.
		 * ----------------
		 */
		if (MyBackendTag == -1)
			MyBackendTag = 1;

		key = PrivateIPCKey;
	}

	/* ----------------
	 *	initialize shared memory and semaphores appropriately.
	 * ----------------
	 */
	if (!IsUnderPostmaster)		/* postmaster already did this */
	{
		PostgresIpcKey = key;
		AttachSharedMemoryAndSemaphores(key);
	}
}

/* --------------------------------
 * InitPostgres
 *		Initialize POSTGRES.
 *
 * Note:
 *		Be very careful with the order of calls in the InitPostgres function.
 * --------------------------------
 */
extern int	NBuffers;

int			lockingOff = 0;		/* backend -L switch */

/*
 */
void
InitPostgres(char *name)		/* database name */
{
	bool		bootstrap;		/* true if BootstrapProcessing */

	/*
	 * See if we're running in BootstrapProcessing mode
	 */
	bootstrap = IsBootstrapProcessingMode();

	/* ----------------
	 *	initialize the backend local portal stack used by
	 *	internal PQ function calls.  see src/lib/libpq/be-dumpdata.c
	 *	This is different from the "portal manager" so this goes here.
	 *	-cim 2/12/91
	 * ----------------
	 */
	be_portalinit();

	/*
	 * initialize the local buffer manager
	 */
	InitLocalBuffer();

#ifndef XLOG
	if (!TransactionFlushEnabled())
		on_shmem_exit(FlushBufferPool, (caddr_t) NULL);
#endif

	/* ----------------
	 *	initialize the database id used for system caches and lock tables
	 * ----------------
	 */
	if (bootstrap)
	{
		SetDatabasePath(ExpandDatabasePath(name));
		SetDatabaseName(name);
		LockDisable(true);
	}
	else
	{
		VerifySystemDatabase();
		InitMyDatabaseInfo(name);
		VerifyMyDatabase();
	}

	/*
	 * Code after this point assumes we are in the proper directory!
	 *
	 * So, how do we implement alternate locations for databases? There are
	 * two possible locations for tables and we need to look in
	 * DataDir/pg_database to find the true location of an individual
	 * database. We can brute-force it as done in InitMyDatabaseInfo(), or
	 * we can be patient and wait until we open pg_database gracefully.
	 * Will try that, but may not work... - thomas 1997-11-01
	 */

	/*
	 * Initialize the transaction system and the relation descriptor cache.
	 * Note we have to make certain the lock manager is off while we do this.
	 */
	AmiTransactionOverride(IsBootstrapProcessingMode());
	LockDisable(true);

	/*
	 * Part of the initialization processing done here sets a read lock on
	 * pg_log.	Since locking is disabled the set doesn't have intended
	 * effect of locking out writers, but this is ok, since we only lock
	 * it to examine AMI transaction status, and this is never written
	 * after initdb is done. -mer 15 June 1992
	 */
	RelationInitialize();		/* pre-allocated reldescs created here */
	InitializeTransactionSystem();		/* pg_log,etc init/crash recovery
										 * here */

	LockDisable(false);

	/*
	 * Set up my per-backend PROC struct in shared memory.
	 */
	InitProcess(PostgresIpcKey);

	/*
	 * Initialize my entry in the shared-invalidation manager's
	 * array of per-backend data.  (Formerly this came before
	 * InitProcess, but now it must happen after, because it uses
	 * MyProc.)  Once I have done this, I am visible to other backends!
	 *
	 * Sets up MyBackendId, a unique backend identifier.
	 */
	InitSharedInvalidationState();

	if (MyBackendId > MAXBACKENDS || MyBackendId <= 0)
	{
		elog(FATAL, "cinit2: bad backend id %d (%d)",
			 MyBackendTag,
			 MyBackendId);
	}

	/*
	 * Initialize the access methods.
	 * Does not touch files (?) - thomas 1997-11-01
	 */
	initam();

	/*
	 * Initialize all the system catalog caches.
	 */
	zerocaches();

	/*
	 * Does not touch files since all routines are builtins (?) - thomas
	 * 1997-11-01
	 */
	InitCatalogCache();

	/*
	 * Set ourselves to the proper user id and figure out our postgres
	 * user id.  If we ever add security so that we check for valid
	 * postgres users, we might do it here.
	 */
	InitUserid();

	/*
	 * Initialize local data in cache invalidation stuff
	 */
	if (!bootstrap)
		InitLocalInvalidateData();

	if (lockingOff)
		LockDisable(true);

	/*
	 * Unless we are bootstrapping, double-check that InitMyDatabaseInfo()
	 * got a correct result.  We can't do this until essentially all the
	 * infrastructure is up, so just do it at the end.
	 */
	if (!bootstrap)
		ReverifyMyDatabase(name);
}

void
BaseInit(void)
{

	/*
	 * Turn on the exception handler. Note: we cannot use elog, Assert,
	 * AssertState, etc. until after exception handling is on.
	 */
	EnableExceptionHandling(true);

	/*
	 * Memory system initialization - we may call palloc after 
	 * EnableMemoryContext()).	Note that EnableMemoryContext() 
	 * must happen before EnablePortalManager().
	 */
	EnableMemoryContext(true);	/* initializes the "top context" */
	EnablePortalManager(true);	/* memory for portal/transaction stuff */

	/*
	 * Attach to shared memory and semaphores, and initialize our
	 * input/output/debugging file descriptors.
	 */
	InitCommunication();
	DebugFileOpen();
	smgrinit();
}
