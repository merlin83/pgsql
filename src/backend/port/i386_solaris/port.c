/*-------------------------------------------------------------------------
 *
 * port.c--
 *	  SunOS5-specific routines
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/port/i386_solaris/Attic/port.c,v 1.3 1997-09-08 02:26:28 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <math.h>				/* for pow() prototype */

#include <errno.h>
#include "rusagestub.h"

long
random()
{
	return (lrand48());
}

void
srandom(int seed)
{
	srand48((long int) seed);
}

int
getrusage(int who, struct rusage * rusage)
{
	struct tms	tms;
	register int tick_rate = CLK_TCK;	/* ticks per second */
	clock_t		u,
				s;

	if (rusage == (struct rusage *) NULL)
	{
		errno = EFAULT;
		return (-1);
	}
	if (times(&tms) < 0)
	{
		/* errno set by times */
		return (-1);
	}
	switch (who)
	{
		case RUSAGE_SELF:
			u = tms.tms_utime;
			s = tms.tms_stime;
			break;
		case RUSAGE_CHILDREN:
			u = tms.tms_cutime;
			s = tms.tms_cstime;
			break;
		default:
			errno = EINVAL;
			return (-1);
	}
#define TICK_TO_SEC(T, RATE)	((T)/(RATE))
#define TICK_TO_USEC(T,RATE)	(((T)%(RATE)*1000000)/RATE)
	rusage->ru_utime.tv_sec = TICK_TO_SEC(u, tick_rate);
	rusage->ru_utime.tv_usec = TICK_TO_USEC(u, tick_rate);
	rusage->ru_stime.tv_sec = TICK_TO_SEC(s, tick_rate);
	rusage->ru_stime.tv_usec = TICK_TO_USEC(u, tick_rate);
	return (0);
}
