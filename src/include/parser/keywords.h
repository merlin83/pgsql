/*-------------------------------------------------------------------------
 *
 * keywords.h
 *	  lexical token lookup for reserved words in postgres SQL
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: keywords.h,v 1.16 2002-06-20 20:29:51 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef KEYWORDS_H
#define KEYWORDS_H

typedef struct ScanKeyword
{
	const char *name;
	int			value;
} ScanKeyword;

extern const ScanKeyword *ScanKeywordLookup(const char *text);

#endif   /* KEYWORDS_H */
