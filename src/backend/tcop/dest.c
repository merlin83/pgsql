/*-------------------------------------------------------------------------
 *
 * dest.c--
 *    support for various communication destinations - see lib/H/tcop/dest.h
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/tcop/dest.c,v 1.1.1.1 1996-07-09 06:21:59 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *   INTERFACE ROUTINES
 * 	BeginCommand - prepare destination for tuples of the given type
 * 	EndCommand - tell destination that no more tuples will arrive
 * 	NullCommand - tell dest that the last of a query sequence was processed
 *	
 *   NOTES
 *	These routines do the appropriate work before and after
 *	tuples are returned by a query to keep the backend and the
 *	"destination" portals synchronized.
 *
 */
#include <stdio.h>		/* for sprintf() */
#include "postgres.h"

#include "access/htup.h"
#include "libpq/libpq-be.h"
#include "access/printtup.h"
#include "utils/portal.h"
#include "utils/elog.h"
#include "utils/palloc.h"

#include "executor/executor.h"

#include "tcop/dest.h"

#include "catalog/pg_type.h"
#include "utils/mcxt.h"

#include "commands/async.h"

/* ----------------
 *	output functions
 * ----------------
 */
void
donothing(List *tuple, List *attrdesc)
{
}

void (*DestToFunction(CommandDest dest))()
{
    switch (dest) {
    case RemoteInternal:
	return printtup_internal;
	break;
	
    case Remote:
	return printtup;
	break;
	
    case Local:
	return be_printtup;
	break;
	
    case Debug:
	return debugtup;
	break;
	
    case None:
    default:
	return donothing;
	break;
    }	
    
    /*
     * never gets here, but DECstation lint appears to be stupid...
     */
    
    return donothing;
}

#define IS_INSERT_TAG(tag) (*tag == 'I' && *(tag+1) == 'N')

/* ----------------
 * 	EndCommand - tell destination that no more tuples will arrive
 * ----------------
 */
void
EndCommand(char *commandTag, CommandDest dest)
{
    char buf[64];
    
    switch (dest) {
    case RemoteInternal:
    case Remote:
	/* ----------------
	 *	tell the fe that the query is over
	 * ----------------
	 */
	pq_putnchar("C", 1);
/*	pq_putint(0, 4); */
	if (IS_INSERT_TAG(commandTag))
	    {
		sprintf(buf, "%s %d", commandTag, GetAppendOid());
		pq_putstr(buf);
	    }
	else
	    pq_putstr(commandTag);
	pq_flush();
	break;
	
    case Local:
    case Debug:
	break;
    case CopyEnd:
	pq_putnchar("Z", 1);
	pq_flush();
	break;
    case None:
    default:
	break;
    }
}

/*
 * These are necessary to sync communications between fe/be processes doing
 * COPY rel TO stdout
 * 
 * or 
 *
 * COPY rel FROM stdin
 *
 */
void
SendCopyBegin()
{
    pq_putnchar("B", 1);
/*    pq_putint(0, 4); */
    pq_flush();
}

void
ReceiveCopyBegin()
{
    pq_putnchar("D", 1);
/*    pq_putint(0, 4); */
    pq_flush();
}

/* ----------------
 * 	NullCommand - tell dest that the last of a query sequence was processed
 * 
 * 	Necessary to implement the hacky FE/BE interface to handle
 *	multiple-return queries.
 * ----------------
 */
void
NullCommand(CommandDest dest)
{
    switch (dest) {
    case RemoteInternal:
    case Remote: {
#if 0
	/* Do any asynchronous notification.  If front end wants to poll,
	   it can send null queries to call this function.
	   */
	PQNotifyList *nPtr;
	MemoryContext orig;
	
	if (notifyContext == NULL) {
	    notifyContext = CreateGlobalMemory("notify");
	}
	orig = MemoryContextSwitchTo((MemoryContext)notifyContext);
	
	for (nPtr = PQnotifies() ;
	     nPtr != NULL;
	     nPtr = (PQNotifyList *)SLGetSucc(&nPtr->Node)) {
	    pq_putnchar("A",1);
	    pq_putint(0, 4);
	    pq_putstr(nPtr->relname);
	    pq_putint(nPtr->be_pid,4);
	    PQremoveNotify(nPtr);
	}
	pq_flush();
	PQcleanNotify();	/* garbage collect */
	(void) MemoryContextSwitchTo(orig);
#endif
	/* ----------------
	 *	tell the fe that the last of the queries has finished
	 * ----------------
	 */
/* 	pq_putnchar("I", 1);  */
 	pq_putstr("I");  
	/* pq_putint(0, 4);*/
	pq_flush();
    }
	break;
	
    case Local:
    case Debug:
    case None:
    default:
	break;
    }	
}

/* ----------------
 * 	BeginCommand - prepare destination for tuples of the given type
 * ----------------
 */
void
BeginCommand(char *pname,
	     int operation,
	     TupleDesc tupdesc,
	     bool isIntoRel,
	     bool isIntoPortal,
	     char *tag,
	     CommandDest dest)
{
    PortalEntry	*entry;
    AttributeTupleForm *attrs = tupdesc->attrs;
    int    natts = tupdesc->natts;
    int    i;
    char   *p;

    switch (dest) {
    case RemoteInternal:
    case Remote:
	/* ----------------
	 *	if this is a "retrieve portal" query, just return
	 *	because nothing needs to be sent to the fe.
	 * ----------------
	 */
        ResetAppendOid();
	if (isIntoPortal)
	    return;
	
	/* ----------------
	 *	if portal name not specified for remote query,
	 *	use the "blank" portal.
	 * ----------------
	 */
	if (pname == NULL)
	    pname = "blank";
	
	/* ----------------
	 *	send fe info on tuples we're about to send
	 * ----------------
	 */
	pq_flush();
	pq_putnchar("P", 1);	/* new portal.. */
	pq_putstr(pname);	/* portal name */
	
	/* ----------------
	 *	if this is a retrieve, then we send back the tuple
	 *	descriptor of the tuples.  "retrieve into" is an
	 *	exception because no tuples are returned in that case.
	 * ----------------
	 */
	if (operation == CMD_SELECT && !isIntoRel) {
	    pq_putnchar("T", 1);	/* type info to follow.. */
	    pq_putint(natts, 2);	/* number of attributes in tuples */
	    
	    for (i = 0; i < natts; ++i) {
		pq_putstr(attrs[i]->attname.data);/* if 16 char name oops.. */
		pq_putint((int) attrs[i]->atttypid, 4);
		pq_putint(attrs[i]->attlen, 2);
	    }
	}
	pq_flush();
	break;
	
    case Local:
	/* ----------------
	 *	prepare local portal buffer for query results
	 *	and setup result for PQexec()
	 * ----------------
	 */
	entry = be_currentportal();
	if (pname != NULL)
	    pbuf_setportalinfo(entry, pname);
	
	if (operation == CMD_SELECT && !isIntoRel) {
	    be_typeinit(entry, tupdesc, natts);
	    p = (char *) palloc(strlen(entry->name)+2);
	    p[0] = 'P';
	    strcpy(p+1,entry->name);
	} else {
	    p = (char *) palloc(strlen(tag)+2);
	    p[0] = 'C';
	    strcpy(p+1,tag);
	}
	entry->result = p;
	break;
	
    case Debug:
	/* ----------------
	 *	show the return type of the tuples
	 * ----------------
	 */
	if (pname == NULL)
	    pname = "blank";
	
	showatts(pname, tupdesc);
	break;
	
    case None:
    default:
	break;
    }
}

static Oid AppendOid;

void
ResetAppendOid()
{
    AppendOid = InvalidOid;
}

#define MULTI_TUPLE_APPEND -1

void
UpdateAppendOid(Oid newoid)
{
    /*
     * First update after AppendOid was reset (at command beginning).
     */
    if (AppendOid == InvalidOid)
	AppendOid = newoid;
    /*
     * Already detected a multiple tuple append, return a void oid ;)
     */
    else if (AppendOid == MULTI_TUPLE_APPEND)
	return;
    /*
     * Oid has been assigned once before, tag this as a multiple tuple
     * append.
     */
    else
	AppendOid = MULTI_TUPLE_APPEND;
}

Oid
GetAppendOid()
{
    if (AppendOid == MULTI_TUPLE_APPEND)
	return InvalidOid;
    return AppendOid;
}
