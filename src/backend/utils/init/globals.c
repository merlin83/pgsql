/*-------------------------------------------------------------------------
 *
 * globals.c
 *	  global variable declarations
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/init/globals.c,v 1.65 2002-06-20 20:29:40 momjian Exp $
 *
 * NOTES
 *	  Globals used all over the place should be declared here and not
 *	  in other modules.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "libpq/pqcomm.h"
#include "miscadmin.h"
#include "storage/backendid.h"


ProtocolVersion FrontendProtocol = PG_PROTOCOL_LATEST;

bool		Noversion = false;

volatile bool InterruptPending = false;
volatile bool QueryCancelPending = false;
volatile bool ProcDiePending = false;
volatile bool ImmediateInterruptOK = false;
volatile uint32 InterruptHoldoffCount = 0;
volatile uint32 CritSectionCount = 0;

int			MyProcPid;
struct Port *MyProcPort;
long		MyCancelKey;

char	   *DataDir = NULL;
 /*
  * The PGDATA directory user says to use, or defaults to via environment
  * variable.  NULL if no option given and no environment variable set
  */

char		OutputFileName[MAXPGPATH];

char		pg_pathname[MAXPGPATH];		/* full path to postgres
										 * executable */

BackendId	MyBackendId;

char	   *DatabaseName = NULL;
char	   *DatabasePath = NULL;

Oid			MyDatabaseId = InvalidOid;

bool		IsUnderPostmaster = false;

int			DateStyle = USE_ISO_DATES;
bool		EuroDates = false;
bool		HasCTZSet = false;
bool		CDayLight = false;
int			CTimeZone = 0;
char		CTZName[MAXTZLEN + 1] = "";

char		DateFormat[20] = "%d-%m-%Y";		/* mjl: sizes! or better
												 * malloc? XXX */
char		FloatFormat[20] = "%f";

bool		enableFsync = true;
bool		allowSystemTableMods = false;
int			SortMem = 512;
int			VacuumMem = 8192;
int			NBuffers = DEF_NBUFFERS;
