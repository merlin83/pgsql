/*-------------------------------------------------------------------------
 *
 * globals.c--
 *    global variable declarations
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/init/globals.c,v 1.1 1996-07-09 06:22:08 scrappy Exp $
 *
 * NOTES
 *    Globals used all over the place should be declared here and not
 *    in other modules.  
 *
 *-------------------------------------------------------------------------
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <math.h>

#include "postgres.h"
#include "miscadmin.h"		/* where the declarations go */

#include "access/heapam.h"
#include "utils/tqual.h"
#include "storage/sinval.h"
#include "storage/sinvaladt.h"
#include "storage/lmgr.h"
#include "utils/elog.h"

#include "catalog/catname.h"

int		Portfd = -1;
int		Noversion = 0;
int             Quiet = 1;

int		MasterPid;
char*           DataDir;

char		OutputFileName[MAXPGPATH] = "";

BackendId	MyBackendId;
BackendTag	MyBackendTag;

char            *UserName = NULL;
char            *DatabaseName = NULL;
char 		*DatabasePath = NULL;

bool		MyDatabaseIdIsInitialized = false;
Oid		MyDatabaseId = InvalidOid;
bool		TransactionInitWasProcessed = false;

bool		IsUnderPostmaster = false;
bool		IsPostmaster = false;

short		DebugLvl = 0;

char *IndexedCatalogNames[] = {
    AttributeRelationName,
    ProcedureRelationName,
    TypeRelationName,
    RelationRelationName,
    0
};


/* ----------------
 * we just do a linear search now so there's no requirement that the list
 * be ordered.  The list is so small it shouldn't make much difference.
 * make sure the list is null-terminated
 *              - jolly 8/19/95
 *                                  
 * OLD COMMENT
 *	WARNING  WARNING  WARNING  WARNING  WARNING  WARNING
 *
 *	keep SharedSystemRelationNames[] in SORTED order!  A binary search
 *	is done on it in catalog.c!
 *
 *	XXX this is a serious hack which should be fixed -cim 1/26/90
 * ----------------
 */
char *SharedSystemRelationNames[] = {
    DatabaseRelationName, 
    DefaultsRelationName,
    DemonRelationName,
    GroupRelationName,
    HostsRelationName,
    LogRelationName,
    MagicRelationName,
    ServerRelationName,
    TimeRelationName,
    UserRelationName,
    VariableRelationName,
    0
};

/* set up global variables, pointers, etc. */
void InitGlobals()
{
    MasterPid = getpid();
    DataDir = GetPGData();
}


