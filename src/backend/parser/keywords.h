/*-------------------------------------------------------------------------
 *
 * keywords.h--
 *    string,atom lookup thingy, reduces strcmp traffic greatly   
 *    in the bowels of the system.  Look for actual defs in lib/C/atoms.c
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: keywords.h,v 1.1.1.1 1996-07-09 06:21:40 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef	KEYWORDS_H
#define	KEYWORDS_H

typedef struct ScanKeyword {
    char *name;
    int	value;
} ScanKeyword;

extern ScanKeyword *ScanKeywordLookup(char *text);
extern char* AtomValueGetString(int atomval);

#endif	/* KEYWORDS_H */
