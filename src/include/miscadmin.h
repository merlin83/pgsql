/*-------------------------------------------------------------------------
 *
 * miscadmin.h
 *	  this file contains general postgres administration and initialization
 *	  stuff that used to be spread out between the following files:
 *		globals.h						global variables
 *		pdir.h							directory path crud
 *		pinit.h							postgres initialization
 *		pmod.h							processing modes
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: miscadmin.h,v 1.54 2000-02-23 15:46:14 momjian Exp $
 *
 * NOTES
 *	  some of the information in this file will be moved to
 *	  other files.
 *
 *-------------------------------------------------------------------------
 */
#ifndef MISCADMIN_H
#define MISCADMIN_H

#include <sys/types.h>				/* For pid_t */

#include "postgres.h"

#include "utils/trace.h"

/*****************************************************************************
 *	  globals.h --															 *
 *****************************************************************************/

/*
 * from postmaster/postmaster.c
 */
extern int	PostmasterMain(int argc, char *argv[]);

/*
 * from utils/init/globals.c
 */
extern bool Noversion;
extern bool Quiet;
extern bool QueryCancel;
extern char *DataDir;

extern int	MyProcPid;
extern struct Port *MyProcPort;
extern long MyCancelKey;

extern char OutputFileName[];

extern char *UserName;

/*
 * done in storage/backendid.h for now.
 *
 * extern BackendId    MyBackendId;
 * extern BackendTag   MyBackendTag;
 */
extern bool MyDatabaseIdIsInitialized;
extern Oid	MyDatabaseId;
extern bool TransactionInitWasProcessed;

extern bool IsUnderPostmaster;

extern int DebugLvl;

/* Date/Time Configuration
 *
 * Constants to pass info from runtime environment:
 *	USE_POSTGRES_DATES specifies traditional postgres format for output.
 *	USE_ISO_DATES specifies ISO-compliant format for output.
 *	USE_SQL_DATES specified Oracle/Ingres-compliant format for output.
 *	USE_GERMAN_DATES specifies German-style dd.mm/yyyy date format.
 *
 * DateStyle specifies preference for date formatting for output.
 * EuroDates if client prefers dates interpreted and written w/European conventions.
 *
 * HasCTZSet if client timezone is specified by client.
 * CDayLight is the apparent daylight savings time status.
 * CTimeZone is the timezone offset in seconds.
 * CTZName is the timezone label.
 */

#define MAXTZLEN		7

#define USE_POSTGRES_DATES		0
#define USE_ISO_DATES			1
#define USE_SQL_DATES			2
#define USE_GERMAN_DATES		3

extern int	DateStyle;
extern bool EuroDates;
extern bool HasCTZSet;
extern bool CDayLight;
extern int	CTimeZone;
extern char CTZName[];

extern char FloatFormat[];
extern char DateFormat[];

#define disableFsync	pg_options[OPT_NOFSYNC]
extern bool allowSystemTableMods;
extern int	SortMem;

extern Oid	LastOidProcessed;	/* for query rewrite */

/*****************************************************************************
 *	  pdir.h --																 *
 *			POSTGRES directory path definitions.							 *
 *****************************************************************************/

extern char *DatabaseName;
extern char *DatabasePath;

/* in utils/misc/database.c */
extern void GetRawDatabaseInfo(const char *name, Oid *db_id, char *path);
extern char *ExpandDatabasePath(const char *path);

/* now in utils/init/miscinit.c */
extern void SetDatabaseName(const char *name);
extern void SetDatabasePath(const char *path);

/* even if MULTIBYTE is not enabled, this function is neccesary
 * since pg_proc.h does have.
 */
extern const char *getdatabaseencoding(void);
extern const char *pg_encoding_to_char(int);
extern int	pg_char_to_encoding(const char *);

extern char *getpgusername(void);
extern void SetPgUserName(void);
extern int	GetUserId(void);
extern void SetUserId(void);
extern int	ValidateBinary(char *path);
extern int	FindExec(char *backend, char *argv0, char *binary_name);
extern int	CheckPathAccess(char *path, char *name, int open_mode);

/* lower case version for case-insensitive SQL referenced in pg_proc.h */
#define GetPgUserName() getpgusername()

/*****************************************************************************
 *	  pmod.h --																 *
 *			POSTGRES processing mode definitions.							 *
 *****************************************************************************/
/*
 * Description:
 *		There are three processing modes in POSTGRES.  They are 
 * "BootstrapProcessing or "bootstrap," InitProcessing or
 * "initialization," and NormalProcessing or "normal."
 *
 * The first two processing modes are used during special times. When the
 * system state indicates bootstrap processing, transactions are all given
 * transaction id "one" and are consequently guarenteed to commit. This mode
 * is used during the initial generation of template databases.
 *
 * Initialization mode until all normal initialization is complete.	
 * Some code behaves differently when executed in this mode to enable 
 * system bootstrapping.
 *
 * If a POSTGRES binary is in normal mode, then all code may be executed
 * normally. 
 */

typedef enum ProcessingMode
{
	BootstrapProcessing,		/* bootstrap creation of template database */
	InitProcessing,				/* initializing system */
	NormalProcessing			/* normal processing */
} ProcessingMode;


/*****************************************************************************
 *	  pinit.h --															 *
 *			POSTGRES initialization and cleanup definitions.				 *
 *****************************************************************************/
/*
 * Note:
 *		XXX AddExitHandler not defined yet.
 */

typedef int16 ExitStatus;

#define NormalExitStatus		(0)
#define FatalExitStatus			(127)
/* XXX are there any other meaningful exit codes? */

/* in utils/init/postinit.c */

extern bool PostgresIsInitialized;

extern void InitPostgres(const char *dbname);

/* one of the ways to get out of here */
#define ExitPostgres(status) proc_exec(status)

/* processing mode support stuff */
extern ProcessingMode Mode;

#define IsBootstrapProcessingMode() ((bool)(Mode == BootstrapProcessing))
#define IsInitProcessingMode() ((bool)(Mode == InitProcessing))
#define IsNormalProcessingMode() ((bool)(Mode == NormalProcessing))

#define SetProcessingMode(mode) \
    do { \
        AssertArg(mode == BootstrapProcessing || mode == InitProcessing || \
		 		  mode == NormalProcessing); \
        Mode = mode; \
    } while(0)

#define GetProcessingMode() Mode

extern void IgnoreSystemIndexes(bool mode);
extern bool IsIgnoringSystemIndexes(void);
extern bool IsCacheInitialized(void);
extern void SetWaitingForLock(bool);

/* 
 * "postmaster.pid" is a file containing postmaster's pid, being
 * created uder $PGDATA upon postmaster's starting up. When postmaster
 * shuts down, it will be unlinked.
*/
#define PIDFNAME	"postmaster.pid"

extern void SetPidFname(char *datadir);
extern char *GetPidFname(void);
extern void UnlinkPidFile(void);
extern int SetPidFile(pid_t pid);

#endif	 /* MISCADMIN_H */
