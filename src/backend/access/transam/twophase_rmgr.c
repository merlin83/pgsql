/*-------------------------------------------------------------------------
 *
 * twophase_rmgr.c
 *	  Two-phase-commit resource managers tables
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/access/transam/twophase_rmgr.c,v 1.2.2.1 2009/11/23 09:59:21 heikki Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/multixact.h"
#include "access/twophase_rmgr.h"
#include "commands/async.h"
#include "storage/lock.h"
#include "utils/flatfiles.h"
#include "utils/inval.h"


const TwoPhaseCallback twophase_recover_callbacks[TWOPHASE_RM_MAX_ID + 1] =
{
	NULL,						/* END ID */
	lock_twophase_recover,		/* Lock */
	NULL,						/* Inval */
	NULL,						/* flat file update */
	NULL,						/* notify/listen */
	multixact_twophase_recover	/* MultiXact */
};

const TwoPhaseCallback twophase_postcommit_callbacks[TWOPHASE_RM_MAX_ID + 1] =
{
	NULL,						/* END ID */
	lock_twophase_postcommit,	/* Lock */
	inval_twophase_postcommit,	/* Inval */
	flatfile_twophase_postcommit,		/* flat file update */
	notify_twophase_postcommit,	/* notify/listen */
	multixact_twophase_postcommit /* MultiXact */
};

const TwoPhaseCallback twophase_postabort_callbacks[TWOPHASE_RM_MAX_ID + 1] =
{
	NULL,						/* END ID */
	lock_twophase_postabort,	/* Lock */
	NULL,						/* Inval */
	NULL,						/* flat file update */
	NULL,						/* notify/listen */
	multixact_twophase_postabort /* MultiXact */
};
