/*-------------------------------------------------------------------------
 *
 * port.c--
 *    OSF/1-specific routines
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/port/alpha/Attic/port.c,v 1.1.1.1 1996-07-09 06:21:42 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/proc.h>
#include "c.h"
#include "utils/elog.h"

void
init_address_fixup()
{
#ifdef NOFIXADE
    int buffer[] = { SSIN_UACPROC, UAC_SIGBUS };
#endif /* NOFIXADE */
#ifdef NOPRINTADE
    int buffer[] = { SSIN_UACPROC, UAC_NOPRINT };
#endif /* NOPRINTADE */

    if (setsysinfo(SSI_NVPAIRS, buffer, 1, (caddr_t) NULL,
		   (unsigned long) NULL) < 0) {
	elog(NOTICE, "setsysinfo failed: %d\n", errno);
    }
}
