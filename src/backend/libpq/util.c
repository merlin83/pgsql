/*-------------------------------------------------------------------------
 *
 * util.c
 *	  general routines for libpq backend
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$Id: util.c,v 1.15 2000-01-26 05:56:29 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *	 UTILITY ROUTINES
 *		pqdebug			- send a string to the debugging output port
 *		pqdebug2		- send two strings to stdout
 *		PQtrace			- turn on pqdebug() tracing
 *		PQuntrace		- turn off pqdebug() tracing
 */


#include "postgres.h"
#include "libpq/libpq.h"


/* ----------------
 *		global variables for backend libpq
 * ----------------
 */
char		PQerrormsg[PQERRORMSG_LENGTH];

int			PQtracep = 0;		/* 1 to print out debugging messages */
FILE	   *debug_port = (FILE *) NULL;

/* ----------------
 *		exceptions
 * ----------------
 */
Exception	MemoryError = {"Memory Allocation Error"};
Exception	PortalError = {"Invalid arguments to portal functions"};
Exception	PostquelError = {"Sql Error"};
Exception	ProtocolError = {"Protocol Error"};

/* ----------------------------------------------------------------
 *						PQ utility routines
 * ----------------------------------------------------------------
 */

void
pqdebug(char *target, char *msg)
{
	if (!target)
		return;

	if (PQtracep)
	{

		/*
		 * if nothing else was suggested default to stdout
		 */
		if (!debug_port)
			debug_port = stdout;
		fprintf(debug_port, target, msg);
		fprintf(debug_port, "\n");
	}
}

void
pqdebug2(char *target, char *msg1, char *msg2)
{
	if (!target)
		return;

	if (PQtracep)
	{

		/*
		 * if nothing else was suggested default to stdout
		 */
		if (!debug_port)
			debug_port = stdout;
		fprintf(debug_port, target, msg1, msg2);
		fprintf(debug_port, "\n");
	}
}

/* --------------------------------
 *		PQtrace() / PQuntrace()
 * --------------------------------
 */
void
PQtrace()
{
	PQtracep = 1;
}

void
PQuntrace()
{
	PQtracep = 0;
}
