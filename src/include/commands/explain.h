/*-------------------------------------------------------------------------
 *
 * explain.h--
 *	  prototypes for explain.c
 *
 * Copyright (c) 1994-5, Regents of the University of California
 *
 * $Id: explain.h,v 1.3 1997-09-07 04:57:26 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef EXPLAIN_H
#define EXPLAIN_H

extern void		ExplainQuery(Query * query, bool verbose, CommandDest dest);

#endif							/* EXPLAIN_H */
