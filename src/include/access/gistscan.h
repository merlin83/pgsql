/*-------------------------------------------------------------------------
 *
 * gistscan.h
 *	  routines defined in access/gist/gistscan.c
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: gistscan.h,v 1.18 2001-10-28 06:25:59 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef GISTSCAN_H
#define GISTSCAN_H

#include "access/relscan.h"

extern Datum gistbeginscan(PG_FUNCTION_ARGS);
extern Datum gistrescan(PG_FUNCTION_ARGS);
extern Datum gistmarkpos(PG_FUNCTION_ARGS);
extern Datum gistrestrpos(PG_FUNCTION_ARGS);
extern Datum gistendscan(PG_FUNCTION_ARGS);
extern void gistadjscans(Relation r, int op, BlockNumber blkno, OffsetNumber offnum);
extern void AtEOXact_gist(void);

#endif	 /* GISTSCAN_H */
