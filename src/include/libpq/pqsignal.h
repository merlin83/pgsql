/*-------------------------------------------------------------------------
 *
 * pqsignal.h--
 *    prototypes for the reliable BSD-style signal(2) routine.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: pqsignal.h,v 1.3 1996-11-06 10:30:08 scrappy Exp $
 *
 * NOTES
 *    This shouldn't be in libpq, but the monitor and some other
 *    things need it...
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQSIGNAL_H
#define PQSIGNAL_H



typedef void (*pqsigfunc)(int);

extern pqsigfunc pqsignal(int signo, pqsigfunc func);

#if defined(USE_POSIX_SIGNALS)
#define	signal(signo, handler)	pqsignal(signo, (pqsigfunc)(handler))
#endif /* USE_POSIX_SIGNALS */

#endif	/* PQSIGNAL_H */
