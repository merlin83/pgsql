/*-------------------------------------------------------------------------
 *
 * tcopprot.h--
 *	  prototypes for postgres.c.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: tcopprot.h,v 1.8 1997-11-25 22:07:10 momjian Exp $
 *
 * OLD COMMENTS
 *	  This file was created so that other c files could get the two
 *	  function prototypes without having to include tcop.h which single
 *	  handedly includes the whole f*cking tree -- mer 5 Nov. 1991
 *
 *-------------------------------------------------------------------------
 */
#ifndef TCOPPROT_H
#define TCOPPROT_H

#include <executor/execdesc.h>
#include <parser/parse_node.h>

#ifndef BOOTSTRAP_INCLUDE
extern List *
pg_plan(char *query_string, Oid *typev, int nargs,
		QueryTreeList **queryListP, CommandDest dest);
extern void pg_eval(char *query_string, char **argv, Oid *typev, int nargs);
extern void
pg_eval_dest(char *query_string, char **argv, Oid *typev,
			 int nargs, CommandDest dest);

#endif							/* BOOTSTRAP_HEADER */

extern void handle_warn(SIGNAL_ARGS);
extern void die(SIGNAL_ARGS);
extern int	PostgresMain(int argc, char *argv[]);
extern void ResetUsage(void);
extern void ShowUsage(void);

#endif							/* tcopprotIncluded */
