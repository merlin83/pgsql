/*-------------------------------------------------------------------------
 *
 * printtup.h
 *
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: printtup.h,v 1.22.2.1 2003-01-21 22:06:36 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PRINTTUP_H
#define PRINTTUP_H

#include "tcop/dest.h"

extern DestReceiver *printtup_create_DR(bool isBinary);

extern void debugSetup(DestReceiver *self, int operation,
		   const char *portalName, TupleDesc typeinfo);
extern void debugtup(HeapTuple tuple, TupleDesc typeinfo,
		 DestReceiver *self);

/* XXX these are really in executor/spi.c */
extern void spi_dest_setup(DestReceiver *self, int operation,
		   const char *portalName, TupleDesc typeinfo);
extern void spi_printtup(HeapTuple tuple, TupleDesc tupdesc,
			 DestReceiver *self);

#endif   /* PRINTTUP_H */
