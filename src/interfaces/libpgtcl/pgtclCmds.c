/*-------------------------------------------------------------------------
 *
 * pgtclCmds.c--
 *    C functions which implement pg_* tcl commands
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/interfaces/libpgtcl/Attic/pgtclCmds.c,v 1.1.1.1 1996-07-09 06:22:16 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */


#include <stdio.h>
#include <stdlib.h>
#include <tcl.h>
#include <string.h>
#include "libpq/pqcomm.h"
#include "libpq-fe.h"
#include "libpq/libpq-fs.h"
#include "pgtclCmds.h"
#include "pgtclId.h"

/**********************************
 * pg_connect
 make a connection to a backend.  
 
 syntax:
 pg_connect dbName [-host hostName] [-port portNumber] [-tty pqtty]]
 
 the return result is either an error message or a handle for a database
 connection.  Handles start with the prefix "pgp"
 
 **********************************/

int
Pg_connect(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    char *pghost = NULL;
    char *pgtty = NULL;
    char *pgport = NULL;
    char *pgoptions = NULL;
    char *dbName; 
    int i;
    PGconn *conn;
  
    if (argc == 1) {
	Tcl_AppendResult(interp, "pg_connect: database name missing\n", 0);
	Tcl_AppendResult(interp, "pg_connect databaseName [-host hostName] [-port portNumber] [-tty pgtty]]", 0);
	return TCL_ERROR;
    
    }
    if (argc > 2) { 
	/* parse for pg environment settings */
	i = 2;
	while (i+1 < argc) {
	    if (strcmp(argv[i], "-host") == 0) {
		pghost = argv[i+1];
		i += 2;
	    }
	    else
		if (strcmp(argv[i], "-port") == 0) {
		    pgport = argv[i+1];
		    i += 2;
		}
		else
		    if (strcmp(argv[i], "-tty") == 0) {
			pgtty = argv[i+1];
			i += 2;
		    }
	            else if (strcmp(argv[i], "-options") == 0) {
			pgoptions = argv[i+1];
			i += 2;
		    }
		    else {
			Tcl_AppendResult(interp, "Bad option to pg_connect : \n",
					 argv[i], 0);
			Tcl_AppendResult(interp, "pg_connect databaseName [-host hostName] [-port portNumber] [-tty pgtty]]",0);
			return TCL_ERROR;
		    }
	} /* while */
	if ((i % 2 != 0) || i != argc) {
	    Tcl_AppendResult(interp, "wrong # of arguments to pg_connect\n", argv[i],0);
	    Tcl_AppendResult(interp, "pg_connect databaseName [-host hostName] [-port portNumber] [-tty pgtty]]",0);
	    return TCL_ERROR;
	}
    }
    dbName = argv[1];

    conn = PQsetdb(pghost, pgport, pgoptions, pgtty, dbName);
    if (conn->status == CONNECTION_OK) {
	PgSetId(interp->result, (void*)conn);
	return TCL_OK;
    }
    else {
	Tcl_AppendResult(interp, "Connection to ", dbName, " failed\n", 0);
	Tcl_AppendResult(interp, conn->errorMessage, 0);
	return TCL_ERROR;
    }
}


/**********************************
 * pg_disconnect
 close a backend connection
 
 syntax:
 pg_disconnect connection
 
 The argument passed in must be a connection pointer.
 
 **********************************/

int
Pg_disconnect(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;

    if (argc != 2) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n", "pg_disconnect connection", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "First argument is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    PQfinish(conn);
    return TCL_OK;
}

/**********************************
 * pg_exec
 send a query string to the backend connection
 
 syntax:
 pg_exec connection query
 
 the return result is either an error message or a handle for a query
 result.  Handles start with the prefix "pgp"
 **********************************/

int
Pg_exec(AlientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    PGresult *result;
    char* connPtrName;

    if (argc != 3) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_exec connection queryString", 0);
	return TCL_ERROR;
    }
    connPtrName = argv[1];

    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    result = PQexec(conn, argv[2]);
    if (result) {
	PgSetId(interp->result, (void*)result);
	return TCL_OK;
    }
    else {
	/* error occurred during the query */
	Tcl_SetResult(interp, conn->errorMessage, TCL_STATIC);
	return TCL_ERROR;
    }
    /* check return status of result */
    return TCL_OK;
}

/**********************************
 * pg_result
 get information about the results of a query
 
 syntax:
 pg_result result ?option? 
 
 the options are:
 -status  
 the status of the result
 -conn
 the connection that produced the result
 -assign arrayName
 assign the results to an array
 -numTuples
 the number of tuples in the query
 -attributes
 returns a list of the name/type pairs of the tuple attributes
 -getTuple tupleNumber
 returns the values of the tuple in a list
 -clear 
 clear the result buffer. Do not reuse after this
 **********************************/
int
Pg_result(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    char* resultPtrName;
    PGresult *result;
    char *opt;
    int i;
    int tupno;
    char arrayInd[MAX_MESSAGE_LEN];
    char *arrVar;

    if (argc != 3 && argc != 4) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",0);
	goto Pg_result_errReturn;
    }

    resultPtrName = argv[1];
    if (! PgValidId(resultPtrName)) {
	Tcl_AppendResult(interp, "First argument is not a valid query result\n", 0);
	return TCL_ERROR;
    }

    result = (PGresult*)PgGetId(resultPtrName);
    opt = argv[2];

    if (strcmp(opt, "-status") == 0) {
	Tcl_AppendResult(interp, pgresStatus[PQresultStatus(result)], 0);
	return TCL_OK;
    }
    else if (strcmp(opt, "-oid") == 0) {
	Tcl_AppendResult(interp, PQoidStatus(result), 0);
	return TCL_OK;
    }
    else if (strcmp(opt, "-conn") == 0) {
	PgSetId(interp->result, (void*)result->conn);
	return TCL_OK;
    }
    else if (strcmp(opt, "-clear") == 0) {
	PQclear(result);
	return TCL_OK;
    }
    else if (strcmp(opt, "-numTuples") == 0) {
	sprintf(interp->result, "%d", PQntuples(result));
	return TCL_OK;
    }
    else if (strcmp(opt, "-assign") == 0) {
	if (argc != 4) {
	    Tcl_AppendResult(interp, "-assign option must be followed by a variable name",0);
	    return TCL_ERROR;
	}
	arrVar = argv[3];
	/* this assignment assigns the table of result tuples into a giant
	   array with the name given in the argument,
	   the indices of the array or (tupno,attrName)*/
	for (tupno = 0; tupno<PQntuples(result); tupno++) {
	    for (i=0;i<PQnfields(result);i++) {
		sprintf(arrayInd, "%d,%s", tupno, PQfname(result,i));
		Tcl_SetVar2(interp, arrVar, arrayInd, 
			    PQgetvalue(result,tupno,i),
			    TCL_LEAVE_ERR_MSG);
	    }
	}
	Tcl_AppendResult(interp, arrVar, 0);
	return TCL_OK;
    }
    else if (strcmp(opt, "-getTuple") == 0) {
	if (argc != 4) {
	    Tcl_AppendResult(interp, "-getTuple option must be followed by a tuple number",0);
	    return TCL_ERROR;
	}
	tupno = atoi(argv[3]);
	
	if (tupno >= PQntuples(result)) {
	    Tcl_AppendResult(interp, "argument to getTuple cannot exceed number of tuples - 1",0);
	    return TCL_ERROR;
	}

/*	Tcl_AppendResult(interp, PQgetvalue(result,tupno,0),NULL); */
        Tcl_AppendElement(interp, PQgetvalue(result,tupno,0));
	for (i=1;i<PQnfields(result);i++) {
/*	  Tcl_AppendResult(interp, " ", PQgetvalue(result,tupno,i),NULL);*/
         Tcl_AppendElement(interp, PQgetvalue(result,tupno,i));
	}
	return TCL_OK;
    }
    else if (strcmp(opt, "-attributes") == 0) {
      Tcl_AppendResult(interp, PQfname(result,0),NULL);
      for (i=1;i<PQnfields(result);i++) {
	Tcl_AppendResult(interp, " ", PQfname(result,i), NULL);
      }
      return TCL_OK;
    }
    else   { 
	Tcl_AppendResult(interp, "Invalid option",0);
	goto Pg_result_errReturn;
    }
  

 Pg_result_errReturn:
    Tcl_AppendResult(interp, 
		     "pg_result result ?option? where ?option is\n", 
		     "\t-status\n",
		     "\t-conn\n",
		     "\t-assign arrayVarName\n",
		     "\t-numTuples\n",
		     "\t-attributes\n"
		     "\t-getTuple tupleNumber\n",
		     "\t-clear\n",
		     "\t-oid\n",
		     0);
    return TCL_ERROR;
  

}

/**********************************
 * pg_lo_open
     open a large object
 
 syntax:
 pg_lo_open conn objOid mode 

 where mode can be either 'r', 'w', or 'rw'
**********************/

int
Pg_lo_open(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    int lobjId;
    int mode;
    int fd;

    if (argc != 4) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_open connection lobjOid mode", 0);
	return TCL_ERROR;
    }
    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    lobjId = atoi(argv[2]);
    if (strlen(argv[3]) < 1 ||
	strlen(argv[3]) > 2)
	{
	Tcl_AppendResult(interp,"mode argument must be 'r', 'w', or 'rw'",0);
        return TCL_ERROR;
    }
    switch (argv[3][0]) {
    case 'r':
    case 'R':
	mode = INV_READ;
	break;
    case 'w':
    case 'W':
	mode = INV_WRITE;
	break;
    default:
	Tcl_AppendResult(interp,"mode argument must be 'r', 'w', or 'rw'",0);
        return TCL_ERROR;
   }
    switch (argv[3][1]) {
    case '\0':
	break;
    case 'r':
    case 'R':
	mode = mode & INV_READ;
	break;
    case 'w':
    case 'W':
	mode = mode & INV_WRITE;
	break;
    default:
	Tcl_AppendResult(interp,"mode argument must be 'r', 'w', or 'rw'",0);
        return TCL_ERROR;
    }

    fd = lo_open(conn,lobjId,mode);
    sprintf(interp->result,"%d",fd);
    return TCL_OK;
}

/**********************************
 * pg_lo_close
     close a large object
 
 syntax:
 pg_lo_close conn fd 

**********************/
int
Pg_lo_close(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    int fd;

    if (argc != 3) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_close connection fd", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    fd = atoi(argv[2]);
    sprintf(interp->result,"%d",lo_close(conn,fd));
    return TCL_OK;
}

/**********************************
 * pg_lo_read
     reads at most len bytes from a large object into a variable named
 bufVar
 
 syntax:
 pg_lo_read conn fd bufVar len

 bufVar is the name of a variable in which to store the contents of the read

**********************/
int
Pg_lo_read(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    int fd;
    int nbytes = 0;
    char *buf;
    char *bufVar;
    int len;

    if (argc != 5) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 " pg_lo_read conn fd bufVar len", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    fd = atoi(argv[2]);

    bufVar = argv[3];

    len = atoi(argv[4]);

    if (len <= 0) {
	sprintf(interp->result,"%d",nbytes);
	return TCL_OK;
    }
    buf = malloc(sizeof(len+1));

    nbytes = lo_read(conn,fd,buf,len);

    Tcl_SetVar(interp,bufVar,buf,TCL_LEAVE_ERR_MSG);
    sprintf(interp->result,"%d",nbytes);
    free(buf);
    return TCL_OK;
    
}

/***********************************
Pg_lo_write
   write at most len bytes to a large object 

 syntax:
 pg_lo_write conn fd buf len

***********************************/
int
Pg_lo_write(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char *connPtrName;
    char *buf;
    int fd;
    int nbytes = 0;
    int len;

    if (argc != 5) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_write conn fd buf len", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    fd = atoi(argv[2]);

    buf = argv[3];

    len = atoi(argv[4]);

    if (len <= 0) {
	sprintf(interp->result,"%d",nbytes);
	return TCL_OK;
    }

    nbytes = lo_write(conn,fd,buf,len);
    sprintf(interp->result,"%d",nbytes);
    return TCL_OK;
}

/***********************************
Pg_lo_lseek
    seek to a certain position in a large object

syntax
  pg_lo_lseek conn fd offset whence

whence can be either
"SEEK_CUR", "SEEK_END", or "SEEK_SET"
***********************************/
int
Pg_lo_lseek(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    int fd;
    char *whenceStr;
    int offset, whence;

    if (argc != 5) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_lseek conn fd offset whence", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    fd = atoi(argv[2]);

    offset = atoi(argv[3]);

    whenceStr = argv[4];
    if (strcmp(whenceStr,"SEEK_SET") == 0) {
	whence = SEEK_SET;
    } else if (strcmp(whenceStr,"SEEK_CUR") == 0) {
	whence = SEEK_CUR;
    } else if (strcmp(whenceStr,"SEEK_END") == 0) {
	whence = SEEK_END;
    } else {
	Tcl_AppendResult(interp, "the whence argument to Pg_lo_lseek must be SEEK_SET, SEEK_CUR or SEEK_END",0);
	return TCL_ERROR;
    }
	
    sprintf(interp->result,"%d",lo_lseek(conn,fd,offset,whence));
    return TCL_OK;
}


/***********************************
Pg_lo_creat
   create a new large object with mode

 syntax:
   pg_lo_creat conn mode

mode can be any OR'ing together of INV_READ, INV_WRITE, and INV_ARCHIVE,
for now, we don't support any additional storage managers.

***********************************/
int
Pg_lo_creat(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    char *modeStr;
    char *modeWord;
    int mode;

    if (argc != 3) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_creat conn mode", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);

    modeStr = argv[2];

    modeWord = strtok(modeStr,"|");
    if (strcmp(modeWord,"INV_READ") == 0) {
	mode = INV_READ;
    } else if (strcmp(modeWord,"INV_WRITE") == 0) {
	mode = INV_WRITE;
    } else if (strcmp(modeWord,"INV_ARCHIVE") == 0) {
	mode = INV_ARCHIVE;
    } else {
	Tcl_AppendResult(interp,
			 "invalid mode argument to Pg_lo_creat\nmode argument must be some OR'd combination of INV_READ, INV_WRITE, and INV_ARCHIVE",
			 0);
	return TCL_ERROR;
    }

    while ( (modeWord = strtok((char*)NULL, "|")) != NULL) {
	if (strcmp(modeWord,"INV_READ") == 0) {
	    mode |= INV_READ;
	} else if (strcmp(modeWord,"INV_WRITE") == 0) {
	    mode |= INV_WRITE;
	} else if (strcmp(modeWord,"INV_ARCHIVE") == 0) {
	    mode |= INV_ARCHIVE;
	} else {
	    Tcl_AppendResult(interp,
			     "invalid mode argument to Pg_lo_creat\nmode argument must be some OR'd combination of INV_READ, INV_WRITE, and INV_ARCHIVE",
			     0);
	    return TCL_ERROR;
	}
    }
    sprintf(interp->result,"%d",lo_creat(conn,mode));
    return TCL_OK;
}

/***********************************
Pg_lo_tell
    returns the current seek location of the large object

 syntax:
   pg_lo_tell conn fd

***********************************/
int
Pg_lo_tell(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    int fd;

    if (argc != 3) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_tell conn fd", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    fd = atoi(argv[2]);

    sprintf(interp->result,"%d",lo_tell(conn,fd));
    return TCL_OK;

}

/***********************************
Pg_lo_unlink
    unlink a file based on lobject id 

 syntax:
   pg_lo_unlink conn lobjId


***********************************/
int
Pg_lo_unlink(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    int lobjId;
    int retval;

    if (argc != 3) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_tell conn fd", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    lobjId = atoi(argv[2]);

    retval = lo_unlink(conn,lobjId);
    if (retval == -1) {
	sprintf(interp->result,"Pg_lo_unlink of '%d' failed",lobjId);
	return TCL_ERROR;
    }
	
    sprintf(interp->result,"%d",retval);
    return TCL_OK;
}

/***********************************
Pg_lo_import
    import a Unix file into an (inversion) large objct
 returns the oid of that object upon success
 returns InvalidOid upon failure

 syntax:
   pg_lo_import conn filename

***********************************/

int
Pg_lo_import(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    char* filename;
    Oid lobjId;

    if (argc != 3) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_import conn filename", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    filename = argv[2];

    lobjId = lo_import(conn,filename);
    if (lobjId == InvalidOid) {
	sprintf(interp->result, "Pg_lo_import of '%s' failed",filename);
	return TCL_ERROR;
    }
    sprintf(interp->result,"%d",lobjId);
    return TCL_OK;
}

/***********************************
Pg_lo_export
    export an Inversion large object to a Unix file
    
 syntax:
   pg_lo_export conn lobjId filename

***********************************/

int
Pg_lo_export(ClientData cData, Tcl_Interp *interp, int argc, char* argv[])
{
    PGconn *conn;
    char* connPtrName;
    char* filename;
    Oid lobjId;
    int retval;

    if (argc != 4) {
	Tcl_AppendResult(interp, "Wrong # of arguments\n",
			 "pg_lo_export conn lobjId filename", 0);
	return TCL_ERROR;
    }

    connPtrName = argv[1];
    if (! PgValidId(connPtrName)) {
	Tcl_AppendResult(interp, "Argument passed in is not a valid connection\n", 0);
	return TCL_ERROR;
    }
  
    conn = (PGconn*)PgGetId(connPtrName);
    lobjId = atoi(argv[2]);
    filename = argv[3];

    retval = lo_export(conn,lobjId,filename);
    if (retval == -1) {
	sprintf(interp->result, "Pg_lo_export %d %s failed",lobjId, filename);
	return TCL_ERROR;
    }
    return TCL_OK;
}


