/*-------------------------------------------------------------------------
 *
 * rint.c
 *	  rint() implementation
 *
 * Copyright (c) 1999, repas AEG Automation GmbH
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/port/qnx4/Attic/rint.c,v 1.2 2000-04-12 17:15:30 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include <math.h>
#include "os.h"

double
rint(double x)
{
	double		f,
				n = 0.;

	f = modf(x, &n);

	if (x > 0.)
	{
		if (f > .5)
			n += 1.;
	}
	else if (x < 0.)
	{
		if (f < -.5)
			n -= 1.;
	}
	return n;
}
