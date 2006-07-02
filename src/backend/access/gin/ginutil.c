/*-------------------------------------------------------------------------
 *
 * ginutil.c
 *    utilities routines for the postgres inverted index access method.
 *
 *
 * Portions Copyright (c) 1996-2006, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *          $PostgreSQL: pgsql/src/backend/access/gin/ginutil.c,v 1.2 2006/07/02 02:23:18 momjian Exp $
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "access/genam.h"
#include "access/gin.h"
#include "access/heapam.h"
#include "catalog/index.h"
#include "miscadmin.h"
#include "storage/freespace.h"

void 
initGinState( GinState *state, Relation index ) {
	if ( index->rd_att->natts != 1 )
			elog(ERROR, "numberOfAttributes %d != 1",
					index->rd_att->natts);
	
	state->tupdesc = index->rd_att;

	fmgr_info_copy(&(state->compareFn),
			index_getprocinfo(index, 1, GIN_COMPARE_PROC),
			CurrentMemoryContext);
	fmgr_info_copy(&(state->extractValueFn),
			index_getprocinfo(index, 1, GIN_EXTRACTVALUE_PROC),
			CurrentMemoryContext);
	fmgr_info_copy(&(state->extractQueryFn),
			index_getprocinfo(index, 1, GIN_EXTRACTQUERY_PROC),
			CurrentMemoryContext);
	fmgr_info_copy(&(state->consistentFn),
			index_getprocinfo(index, 1, GIN_CONSISTENT_PROC),
			CurrentMemoryContext);
}

/*
 * Allocate a new page (either by recycling, or by extending the index file)
 * The returned buffer is already pinned and exclusive-locked
 * Caller is responsible for initializing the page by calling GinInitBuffer
 */

Buffer
GinNewBuffer(Relation index) {
	Buffer	buffer;
	bool	needLock;

	/* First, try to get a page from FSM */
	for(;;) {
		BlockNumber blkno = GetFreeIndexPage(&index->rd_node);
		if (blkno == InvalidBlockNumber)
			break;

		buffer = ReadBuffer(index, blkno);

		/*
		 * We have to guard against the possibility that someone else already
		 * recycled this page; the buffer may be locked if so.
		 */
		 if (ConditionalLockBuffer(buffer)) {
			Page        page = BufferGetPage(buffer);

			if (PageIsNew(page))
				return buffer;  /* OK to use, if never initialized */

			if (GinPageIsDeleted(page))
				return buffer;  /* OK to use */

			LockBuffer(buffer, GIN_UNLOCK);
		}

		/* Can't use it, so release buffer and try again */
		ReleaseBuffer(buffer);
	}

	/* Must extend the file */
	needLock = !RELATION_IS_LOCAL(index);
	if (needLock)
		LockRelationForExtension(index, ExclusiveLock);

	buffer = ReadBuffer(index, P_NEW);
	LockBuffer(buffer, GIN_EXCLUSIVE);

	if (needLock)
		UnlockRelationForExtension(index, ExclusiveLock);

	return buffer;
}

void
GinInitPage(Page page, uint32 f, Size pageSize) {
	GinPageOpaque opaque;

	PageInit(page, pageSize, sizeof(GinPageOpaqueData));

	opaque = GinPageGetOpaque(page);
	memset( opaque, 0, sizeof(GinPageOpaqueData) );
	opaque->flags = f;   
	opaque->rightlink = InvalidBlockNumber;
}

void
GinInitBuffer(Buffer b, uint32 f) {
	GinInitPage( BufferGetPage(b), f, BufferGetPageSize(b) );
}

int
compareEntries(GinState *ginstate, Datum a, Datum b) {
	return DatumGetInt32(
		FunctionCall2(
			&ginstate->compareFn,
			a, b
		)
	);
}

static FmgrInfo* cmpDatumPtr=NULL;
static bool needUnique = FALSE;

static int
cmpEntries(const void * a, const void * b) {
	int res = DatumGetInt32(
		FunctionCall2(
			cmpDatumPtr,
			*(Datum*)a,
			*(Datum*)b
		)
	);

	if ( res == 0 ) 
		needUnique = TRUE;

	return res;
}

Datum*
extractEntriesS(GinState *ginstate, Datum value, uint32 *nentries) {
	Datum *entries;

   	entries = (Datum*)DatumGetPointer(
							FunctionCall2(
								&ginstate->extractValueFn,
								value,
								PointerGetDatum( nentries )
							)
			);

	if ( entries == NULL )
		*nentries = 0;

	if ( *nentries > 1 ) {
		cmpDatumPtr = &ginstate->compareFn;
		needUnique = FALSE;
		qsort(entries, *nentries, sizeof(Datum), cmpEntries); 
	}

	return entries;
}


Datum*
extractEntriesSU(GinState *ginstate, Datum value, uint32 *nentries) {
	Datum *entries = extractEntriesS(ginstate, value, nentries);

	if ( *nentries>1 && needUnique ) {
		Datum *ptr, *res;

		ptr = res = entries;

		while( ptr - entries < *nentries ) {
			if ( compareEntries(ginstate, *ptr, *res ) != 0 ) 
				*(++res) = *ptr++;
			else
				ptr++;
		}

		*nentries = res + 1 - entries;
	}

	return entries;
}

/*
 * It's analog of PageGetTempPage(), but copies whole page
 */
Page
GinPageGetCopyPage( Page page ) {
	Size    pageSize = PageGetPageSize( page );
	Page tmppage;

	tmppage=(Page)palloc( pageSize );
	memcpy( tmppage, page, pageSize );
	
	return tmppage;
}

Datum
ginoption(PG_FUNCTION_ARGS)
{
	ArrayType	   *options = (ArrayType *) PG_GETARG_POINTER(0);

	if (options != NULL)
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("GIN does not support parameters at all")));

	/* Do not use PG_RETURN_NULL. */
	PG_RETURN_BYTEA_P(NULL);
}
