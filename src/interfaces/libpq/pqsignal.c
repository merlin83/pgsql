/*-------------------------------------------------------------------------
 *
 * pqsignal.c--
 *    reliable BSD-style signal(2) routine stolen from RWW who stole it
 *    from Stevens...
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/interfaces/libpq/pqsignal.c,v 1.2 1996-11-08 06:02:30 momjian Exp $
 *
 * NOTES
 *	This shouldn't be in libpq, but the monitor and some other
 *	things need it...
 *
 *-------------------------------------------------------------------------
 */
#include <stdlib.h>

#include "libpq/pqsignal.h"

pqsigfunc
pqsignal(int signo, pqsigfunc func)
{
#if defined(USE_POSIX_SIGNALS)
    struct sigaction act, oact;
    
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (signo != SIGALRM) {
	act.sa_flags |= SA_RESTART;
    }
    if (sigaction(signo, &act, &oact) < 0)
	return(SIG_ERR);
    return(oact.sa_handler);
#else /* !USE_POSIX_SIGNALS */
    exit(1); /* this should never be reached, pqsignal should only
	      be called if USE_POSIX_SIGNALS is true*/
#endif /* !USE_POSIX_SIGNALS */
}
