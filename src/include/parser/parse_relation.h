/*-------------------------------------------------------------------------
 *
 * parse_query.h
 *	  prototypes for parse_query.c.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parse_relation.h,v 1.10 1999-07-15 15:21:27 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_QUERY_H
#define PARSE_RANGE_H

#include <parser/parse_node.h>

extern RangeTblEntry *refnameRangeTableEntry(ParseState *pstate, char *refname);
extern int refnameRangeTablePosn(ParseState *pstate,
					  char *refname, int *sublevels_up);
extern RangeTblEntry *colnameRangeTableEntry(ParseState *pstate, char *colname);
extern RangeTblEntry *addRangeTableEntry(ParseState *pstate,
				   char *relname,
				   char *refname,
				   bool inh,
				   bool inFromCl);
extern List *expandAll(ParseState *pstate, char *relname, char *refname,
		  int *this_resno);
extern int	attnameAttNum(Relation rd, char *a);
extern bool attnameIsSet(Relation rd, char *name);
extern int	attnumAttNelems(Relation rd, int attid);
extern Oid	attnumTypeId(Relation rd, int attid);
extern void handleTargetColname(ParseState *pstate, char **resname,
					char *refname, char *colname);

#endif	 /* PARSE_RANGE_H */
