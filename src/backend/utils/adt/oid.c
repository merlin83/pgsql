/*-------------------------------------------------------------------------
 *
 * oid.c--
 *    Functions for the built-in type Oid.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/adt/oid.c,v 1.8 1997-08-24 23:07:35 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>

#include "postgres.h"
#include "utils/builtins.h"	/* where function declarations go */

/***************************************************************************** 
 *   USER I/O ROUTINES                                                       *
 *****************************************************************************/

/*
 *	oid8in		- converts "num num ..." to internal form
 *
 *	Note:
 *		Fills any nonexistent digits with NULL oids.
 */
Oid *oid8in(char *oidString)
{
    register Oid	(*result)[];
    int			nums;
    
    if (oidString == NULL)
	return(NULL);
    result = (Oid (*)[]) palloc(sizeof(Oid [8]));
    if ((nums = sscanf(oidString, "%d%d%d%d%d%d%d%d",
		       &(*result)[0],
		       &(*result)[1],
		       &(*result)[2],
		       &(*result)[3],
		       &(*result)[4],
		       &(*result)[5],
		       &(*result)[6],
		       &(*result)[7])) != 8) {
	do
	    (*result)[nums++] = 0;
	while (nums < 8);
    }
    return((Oid *) result);
}

/*
 *	oid8out	- converts internal form to "num num ..."
 */
char *oid8out(Oid	(*oidArray)[])
{
    register int		num;
    register Oid	*sp;
    register char		*rp;
    char			*result;
    
    if (oidArray == NULL) {
	result = (char *) palloc(2);
	result[0] = '-';
	result[1] = '\0';
	return(result);
    }
    
    /* assumes sign, 10 digits, ' ' */
    rp = result = (char *) palloc(8 * 12);
    sp = *oidArray;
    for (num = 8; num != 0; num--) {
	ltoa(*sp++, rp);
	while (*++rp != '\0')
	    ;
	*rp++ = ' ';
    }
    *--rp = '\0';
    return(result);
}

Oid oidin(char *s)
{
    return(int4in(s));
}

char *oidout(Oid o)
{
    return(int4out(o));
}

/***************************************************************************** 
 *   PUBLIC ROUTINES                                                         *
 *****************************************************************************/

/*
 * If you change this function, change heap_keytest()
 * because we have hardcoded this in there as an optimization
 */
bool oideq(Oid arg1, Oid arg2)
{
    return(arg1 == arg2);
}

bool oidne(Oid arg1, Oid arg2)
{
    return(arg1 != arg2);
}

bool oid8eq(Oid arg1[], Oid arg2[])
{
    return (bool)(memcmp(arg1, arg2, 8 * sizeof(Oid)) == 0);
}

bool oideqint4(Oid arg1, int32 arg2)
{
/* oid is unsigned, but int4 is signed */
    return (arg2 >= 0 && arg1 == arg2);
}

bool int4eqoid(int32 arg1, Oid arg2)
{
/* oid is unsigned, but int4 is signed */
    return (arg1 >= 0 && arg1 == arg2);
}

