/*-------------------------------------------------------------------------
 *
 * portalcmds.c
 *	  Utility commands affecting portals (that is, SQL cursor commands)
 *
 * Note: see also tcop/pquery.c, which implements portal operations for
 * the FE/BE protocol.  This module uses pquery.c for some operations.
 * And both modules depend on utils/mmgr/portalmem.c, which controls
 * storage management for portals (but doesn't run any queries in them).
 * 
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/portalcmds.c,v 1.14 2003-05-05 00:44:55 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <limits.h>

#include "miscadmin.h"
#include "commands/portalcmds.h"
#include "executor/executor.h"
#include "optimizer/planner.h"
#include "rewrite/rewriteHandler.h"
#include "tcop/pquery.h"
#include "utils/memutils.h"


/*
 * PerformCursorOpen
 *		Execute SQL DECLARE CURSOR command.
 */
void
PerformCursorOpen(DeclareCursorStmt *stmt, CommandDest dest)
{
	List	   *rewritten;
	Query	   *query;
	Plan	   *plan;
	Portal		portal;
	MemoryContext oldContext;

	/*
	 * Disallow empty-string cursor name (conflicts with protocol-level
	 * unnamed portal).
	 */
	if (!stmt->portalname || stmt->portalname[0] == '\0')
		elog(ERROR, "Invalid cursor name: must not be empty");

	/*
	 * If this is a non-holdable cursor, we require that this statement
	 * has been executed inside a transaction block (or else, it would
	 * have no user-visible effect).
	 */
	if (!(stmt->options & CURSOR_OPT_HOLD))
		RequireTransactionChain((void *) stmt, "DECLARE CURSOR");

	/*
	 * The query has been through parse analysis, but not rewriting or
	 * planning as yet.  Note that the grammar ensured we have a SELECT
	 * query, so we are not expecting rule rewriting to do anything strange.
	 */
	rewritten = QueryRewrite((Query *) stmt->query);
	if (length(rewritten) != 1 || !IsA(lfirst(rewritten), Query))
		elog(ERROR, "PerformCursorOpen: unexpected rewrite result");
	query = (Query *) lfirst(rewritten);
	if (query->commandType != CMD_SELECT)
		elog(ERROR, "PerformCursorOpen: unexpected rewrite result");

	if (query->into)
		elog(ERROR, "DECLARE CURSOR may not specify INTO");
	if (query->rowMarks != NIL)
		elog(ERROR, "DECLARE/UPDATE is not supported"
			 "\n\tCursors must be READ ONLY");

	plan = planner(query, true, stmt->options);

	/*
	 * Create a portal and copy the query and plan into its memory context.
	 * (If a duplicate cursor name already exists, warn and drop it.)
	 */
	portal = CreatePortal(stmt->portalname, true, false);

	oldContext = MemoryContextSwitchTo(PortalGetHeapMemory(portal));

	query = copyObject(query);
	plan = copyObject(plan);

	PortalDefineQuery(portal,
					  NULL,		/* unfortunately don't have sourceText */
					  "SELECT",	/* cursor's query is always a SELECT */
					  makeList1(query),
					  makeList1(plan),
					  PortalGetHeapMemory(portal));

	MemoryContextSwitchTo(oldContext);

	/*
	 * Set up options for portal.
	 *
	 * If the user didn't specify a SCROLL type, allow or disallow
	 * scrolling based on whether it would require any additional
	 * runtime overhead to do so.
	 */
	portal->cursorOptions = stmt->options;
	if (!(portal->cursorOptions & (CURSOR_OPT_SCROLL | CURSOR_OPT_NO_SCROLL)))
	{
		if (ExecSupportsBackwardScan(plan))
			portal->cursorOptions |= CURSOR_OPT_SCROLL;
		else
			portal->cursorOptions |= CURSOR_OPT_NO_SCROLL;
	}

	/*
	 * Start execution --- never any params for a cursor.
	 */
	PortalStart(portal, NULL);

	Assert(portal->strategy == PORTAL_ONE_SELECT);

	/*
	 * We're done; the query won't actually be run until PerformPortalFetch
	 * is called.
	 */
}

/*
 * PerformPortalFetch
 *		Execute SQL FETCH or MOVE command.
 *
 *	stmt: parsetree node for command
 *	dest: where to send results
 *	completionTag: points to a buffer of size COMPLETION_TAG_BUFSIZE
 *		in which to store a command completion status string.
 *
 * completionTag may be NULL if caller doesn't want a status string.
 */
void
PerformPortalFetch(FetchStmt *stmt,
				   CommandDest dest,
				   char *completionTag)
{
	Portal		portal;
	long		nprocessed;

	/*
	 * Disallow empty-string cursor name (conflicts with protocol-level
	 * unnamed portal).
	 */
	if (!stmt->portalname || stmt->portalname[0] == '\0')
		elog(ERROR, "Invalid cursor name: must not be empty");

	/* get the portal from the portal name */
	portal = GetPortalByName(stmt->portalname);
	if (!PortalIsValid(portal))
	{
		/* FIXME: shouldn't this be an ERROR? */
		elog(WARNING, "PerformPortalFetch: portal \"%s\" not found",
			 stmt->portalname);
		if (completionTag)
			strcpy(completionTag, stmt->ismove ? "MOVE 0" : "FETCH 0");
		return;
	}

	/*
	 * Adjust dest if needed.  MOVE wants dest = None.
	 *
	 * If fetching from a binary cursor and the requested destination is
	 * Remote, change it to RemoteInternal.  Note we do NOT change if the
	 * destination is RemoteExecute --- so the Execute message's format
	 * specification wins out over the cursor's type.
	 */
	if (stmt->ismove)
		dest = None;
	else if (dest == Remote && (portal->cursorOptions & CURSOR_OPT_BINARY))
		dest = RemoteInternal;

	/* Do it */
	nprocessed = PortalRunFetch(portal,
								stmt->direction,
								stmt->howMany,
								dest);

	/* Return command status if wanted */
	if (completionTag)
		snprintf(completionTag, COMPLETION_TAG_BUFSIZE, "%s %ld",
				 stmt->ismove ? "MOVE" : "FETCH",
				 nprocessed);
}

/*
 * PerformPortalClose
 *		Close a cursor.
 */
void
PerformPortalClose(const char *name)
{
	Portal		portal;

	/*
	 * Disallow empty-string cursor name (conflicts with protocol-level
	 * unnamed portal).
	 */
	if (!name || name[0] == '\0')
		elog(ERROR, "Invalid cursor name: must not be empty");

	/*
	 * get the portal from the portal name
	 */
	portal = GetPortalByName(name);
	if (!PortalIsValid(portal))
	{
		elog(WARNING, "PerformPortalClose: portal \"%s\" not found",
			 name);
		return;
	}

	/*
	 * Note: PortalCleanup is called as a side-effect
	 */
	PortalDrop(portal, false);
}

/*
 * PortalCleanup
 *
 * Clean up a portal when it's dropped.  This is the standard cleanup hook
 * for portals.
 */
void
PortalCleanup(Portal portal, bool isError)
{
	QueryDesc  *queryDesc;

	/*
	 * sanity checks
	 */
	AssertArg(PortalIsValid(portal));
	AssertArg(portal->cleanup == PortalCleanup);

	/*
	 * Delete tuplestore if present.  (Note: portalmem.c is responsible
	 * for removing holdContext.)  We should do this even under error
	 * conditions; since the tuplestore would have been using cross-
	 * transaction storage, its temp files need to be explicitly deleted.
	 */
	if (portal->holdStore)
	{
		MemoryContext oldcontext;

		oldcontext = MemoryContextSwitchTo(portal->holdContext);
		tuplestore_end(portal->holdStore);
		MemoryContextSwitchTo(oldcontext);
		portal->holdStore = NULL;
	}
	/*
	 * Shut down executor, if still running.  We skip this during error
	 * abort, since other mechanisms will take care of releasing executor
	 * resources, and we can't be sure that ExecutorEnd itself wouldn't fail.
	 */
	queryDesc = PortalGetQueryDesc(portal);
	if (queryDesc)
	{
		portal->queryDesc = NULL;
		if (!isError)
			ExecutorEnd(queryDesc);
	}
}

/*
 * PersistHoldablePortal
 *
 * Prepare the specified Portal for access outside of the current
 * transaction. When this function returns, all future accesses to the
 * portal must be done via the Tuplestore (not by invoking the
 * executor).
 */
void
PersistHoldablePortal(Portal portal)
{
	QueryDesc *queryDesc = PortalGetQueryDesc(portal);
	Portal		saveCurrentPortal;
	MemoryContext savePortalContext;
	MemoryContext saveQueryContext;
	MemoryContext oldcxt;

	/*
	 * If we're preserving a holdable portal, we had better be
	 * inside the transaction that originally created it.
	 */
	Assert(portal->createXact == GetCurrentTransactionId());
	Assert(portal->holdStore == NULL);
	Assert(queryDesc != NULL);
	Assert(portal->portalReady);
	Assert(!portal->portalDone);

	/*
	 * This context is used to store the tuple set.
	 * Caller must have created it already.
	 */
	Assert(portal->holdContext != NULL);
	oldcxt = MemoryContextSwitchTo(portal->holdContext);

	/* XXX: Should SortMem be used for this? */
	portal->holdStore = tuplestore_begin_heap(true, true, SortMem);

	/*
	 * Before closing down the executor, we must copy the tupdesc, since
	 * it was created in executor memory.  Note we are copying it into
	 * the holdContext.
	 */
	portal->tupDesc = CreateTupleDescCopy(portal->tupDesc);

	MemoryContextSwitchTo(oldcxt);

	/*
	 * Check for improper portal use, and mark portal active.
	 */
	if (portal->portalActive)
		elog(ERROR, "Portal \"%s\" already active", portal->name);
	portal->portalActive = true;

	/*
	 * Set global portal and context pointers.
	 */
	saveCurrentPortal = CurrentPortal;
	CurrentPortal = portal;
	savePortalContext = PortalContext;
	PortalContext = PortalGetHeapMemory(portal);
	saveQueryContext = QueryContext;
	QueryContext = portal->queryContext;

	MemoryContextSwitchTo(PortalContext);

	/*
	 * Rewind the executor: we need to store the entire result set in
	 * the tuplestore, so that subsequent backward FETCHs can be
	 * processed.
	 */
	ExecutorRewind(queryDesc);

	/* Set the destination to output to the tuplestore */
	queryDesc->dest = Tuplestore;

	/* Fetch the result set into the tuplestore */
	ExecutorRun(queryDesc, ForwardScanDirection, 0L);

	/*
	 * Now shut down the inner executor.
	 */
	portal->queryDesc = NULL;	/* prevent double shutdown */
	ExecutorEnd(queryDesc);

	/* Mark portal not active */
	portal->portalActive = false;

	CurrentPortal = saveCurrentPortal;
	PortalContext = savePortalContext;
	QueryContext = saveQueryContext;

	/*
	 * Reset the position in the result set: ideally, this could be
	 * implemented by just skipping straight to the tuple # that we need
	 * to be at, but the tuplestore API doesn't support that. So we
	 * start at the beginning of the tuplestore and iterate through it
	 * until we reach where we need to be.  FIXME someday?
	 */
	MemoryContextSwitchTo(portal->holdContext);

	if (!portal->atEnd)
	{
		long	store_pos;

		if (portal->posOverflow)		/* oops, cannot trust portalPos */
			elog(ERROR, "Unable to reposition held cursor");

		tuplestore_rescan(portal->holdStore);

		for (store_pos = 0; store_pos < portal->portalPos; store_pos++)
		{
			HeapTuple tup;
			bool should_free;

			tup = tuplestore_gettuple(portal->holdStore, true,
									  &should_free);

			if (tup == NULL)
				elog(ERROR,
					 "PersistHoldablePortal: unexpected end of tuple stream");

			if (should_free)
				pfree(tup);
		}
	}

	MemoryContextSwitchTo(oldcxt);

	/*
	 * We can now release any subsidiary memory of the portal's heap
	 * context; we'll never use it again.  The executor already dropped
	 * its context, but this will clean up anything that glommed onto
	 * the portal's heap via PortalContext.
	 */
	MemoryContextDeleteChildren(PortalGetHeapMemory(portal));
}
