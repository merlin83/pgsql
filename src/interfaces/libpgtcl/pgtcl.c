/*-------------------------------------------------------------------------
 *
 * pgtcl.c--
 *    
 *    libpgtcl is a tcl package for front-ends to interface with pglite
 *   It's the tcl equivalent of the old libpq C interface.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/interfaces/libpgtcl/Attic/pgtcl.c,v 1.2 1996-10-07 21:19:06 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "tcl.h"
#include "libpgtcl.h"
#include "pgtclCmds.h"

/*
 * PG_Init 
 *    initialization package for the PGLITE Tcl package
 *
 */

int
Pg_Init (Tcl_Interp *interp)
{
  /* register all pgtcl commands */

  Tcl_CreateCommand(interp,
		    "pg_connect",
		    Pg_connect,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_disconnect",
		    Pg_disconnect,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  Tcl_CreateCommand(interp,
		    "pg_exec",
		    Pg_exec,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  Tcl_CreateCommand(interp,
		    "pg_select",
		    Pg_select,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  Tcl_CreateCommand(interp,
		    "pg_result",
		    Pg_result,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  Tcl_CreateCommand(interp,
		    "pg_lo_open",
		    Pg_lo_open,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  Tcl_CreateCommand(interp,
		    "pg_lo_close",
		    Pg_lo_close,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_lo_read",
		    Pg_lo_read,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_lo_write",
		    Pg_lo_write,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_lo_lseek",
		    Pg_lo_lseek,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_lo_creat",
		    Pg_lo_creat,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_lo_tell",
		    Pg_lo_tell,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_lo_unlink",
		    Pg_lo_unlink,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);

  Tcl_CreateCommand(interp,
		    "pg_lo_import",
		    Pg_lo_import,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  Tcl_CreateCommand(interp,
		    "pg_lo_export",
		    Pg_lo_export,
		    (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);
  
  return TCL_OK;
}


