/*-------------------------------------------------------------------------
 *
 * printtup.h
 *
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: printtup.h,v 1.19 2002-02-27 19:35:40 tgl Exp $
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

/* XXX this one is really in executor/spi.c */
extern void spi_printtup(HeapTuple tuple, TupleDesc tupdesc,
			 DestReceiver *self);

extern bool getTypeOutputInfo(Oid type, Oid *typOutput, Oid *typElem,
				  bool *typIsVarlena);

#endif   /* PRINTTUP_H */
