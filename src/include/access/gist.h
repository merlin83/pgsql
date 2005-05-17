/*-------------------------------------------------------------------------
 *
 * gist.h
 *	  common declarations for the GiST access method code.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/access/gist.h,v 1.45 2005/05/17 00:59:30 neilc Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef GIST_H
#define GIST_H

#include "access/itup.h"
#include "access/relscan.h"
#include "access/sdir.h"
#include "access/xlog.h"

/*
 * You can have as many strategies as you please in GiSTs,
 * as long as your consistent method can handle them.
 * The system doesn't really care what they are.
 */
#define GISTNStrategies					100

/*
 * amproc indexes for GiST indexes.
 */
#define GIST_CONSISTENT_PROC			1
#define GIST_UNION_PROC					2
#define GIST_COMPRESS_PROC				3
#define GIST_DECOMPRESS_PROC			4
#define GIST_PENALTY_PROC				5
#define GIST_PICKSPLIT_PROC				6
#define GIST_EQUAL_PROC					7
#define GISTNProcs						7


/*
 * Page opaque data in a GiST index page.
 */
#define F_LEAF			(1 << 0)

typedef struct GISTPageOpaqueData
{
	uint32		flags;
} GISTPageOpaqueData;

typedef GISTPageOpaqueData *GISTPageOpaque;

#define GIST_LEAF(entry) (((GISTPageOpaque) PageGetSpecialPointer((entry)->page))->flags & F_LEAF)

/*
 *	When we descend a tree, we keep a stack of parent pointers. This
 *	allows us to follow a chain of internal node points until we reach
 *	a leaf node, and then back up the stack to re-examine the internal
 *	nodes.
 *
 * 'parent' is the previous stack entry -- i.e. the node we arrived
 * from. 'block' is the node's block number. 'offset' is the offset in
 * the node's page that we stopped at (i.e. we followed the child
 * pointer located at the specified offset).
 */
typedef struct GISTSTACK
{
	struct GISTSTACK *parent;
	OffsetNumber offset;
	BlockNumber block;
} GISTSTACK;

typedef struct GISTSTATE
{
	FmgrInfo	consistentFn[INDEX_MAX_KEYS];
	FmgrInfo	unionFn[INDEX_MAX_KEYS];
	FmgrInfo	compressFn[INDEX_MAX_KEYS];
	FmgrInfo	decompressFn[INDEX_MAX_KEYS];
	FmgrInfo	penaltyFn[INDEX_MAX_KEYS];
	FmgrInfo	picksplitFn[INDEX_MAX_KEYS];
	FmgrInfo	equalFn[INDEX_MAX_KEYS];

	TupleDesc	tupdesc;
} GISTSTATE;

#define isAttByVal( gs, anum ) (gs)->tupdesc->attrs[anum]->attbyval

/*
 *	When we're doing a scan, we need to keep track of the parent stack
 *	for the marked and current items.
 */
typedef struct GISTScanOpaqueData
{
	GISTSTACK			*stack;
	GISTSTACK			*markstk;
	uint16				 flags;
	GISTSTATE			*giststate;
	MemoryContext		 tempCxt;
	Buffer				 curbuf;
	Buffer				 markbuf;
} GISTScanOpaqueData;

typedef GISTScanOpaqueData *GISTScanOpaque;

/*
 *	When we're doing a scan and updating a tree at the same time, the
 *	updates may affect the scan.  We use the flags entry of the scan's
 *	opaque space to record our actual position in response to updates
 *	that we can't handle simply by adjusting pointers.
 */
#define GS_CURBEFORE	((uint16) (1 << 0))
#define GS_MRKBEFORE	((uint16) (1 << 1))

/* root page of a gist index */
#define GIST_ROOT_BLKNO				0

/*
 *	When we update a relation on which we're doing a scan, we need to
 *	check the scan and fix it if the update affected any of the pages it
 *	touches.  Otherwise, we can miss records that we should see.  The only
 *	times we need to do this are for deletions and splits.	See the code in
 *	gistscan.c for how the scan is fixed. These two constants tell us what sort
 *	of operation changed the index.
 */
#define GISTOP_DEL		0
#define GISTOP_SPLIT	1

/*
 * This is the Split Vector to be returned by the PickSplit method.
 */
typedef struct GIST_SPLITVEC
{
	OffsetNumber *spl_left;		/* array of entries that go left */
	int			spl_nleft;		/* size of this array */
	Datum		spl_ldatum;		/* Union of keys in spl_left */
	Datum		spl_lattr[INDEX_MAX_KEYS];		/* Union of subkeys in
												 * spl_left */
	int			spl_lattrsize[INDEX_MAX_KEYS];
	bool		spl_lisnull[INDEX_MAX_KEYS];

	OffsetNumber *spl_right;	/* array of entries that go right */
	int			spl_nright;		/* size of the array */
	Datum		spl_rdatum;		/* Union of keys in spl_right */
	Datum		spl_rattr[INDEX_MAX_KEYS];		/* Union of subkeys in
												 * spl_right */
	int			spl_rattrsize[INDEX_MAX_KEYS];
	bool		spl_risnull[INDEX_MAX_KEYS];

	int		   *spl_idgrp;
	int		   *spl_ngrp;		/* number in each group */
	char	   *spl_grpflag;	/* flags of each group */
} GIST_SPLITVEC;

/*
 * An entry on a GiST node.  Contains the key, as well as
 * its own location (rel,page,offset) which can supply the matching
 * pointer.  The size of the key is in bytes, and leafkey is a flag to
 * tell us if the entry is in a leaf node.
 */
typedef struct GISTENTRY
{
	Datum		key;
	Relation	rel;
	Page		page;
	OffsetNumber offset;
	int			bytes;
	bool		leafkey;
} GISTENTRY;


/*
 * Vector of GISTENTRY struct's, user-defined
 * methods union andpick split takes it as one of args
 */

typedef struct
{
	int32		n;				/* number of elements */
	GISTENTRY	vector[1];
} GistEntryVector;

#define GEVHDRSZ	( offsetof(GistEntryVector, vector[0]) )

/*
 * macro to initialize a GISTENTRY
 */
#define gistentryinit(e, k, r, pg, o, b, l) \
	do { (e).key = (k); (e).rel = (r); (e).page = (pg); \
		 (e).offset = (o); (e).bytes = (b); (e).leafkey = (l); } while (0)

/* gist.c */
extern Datum gistbuild(PG_FUNCTION_ARGS);
extern Datum gistinsert(PG_FUNCTION_ARGS);
extern Datum gistbulkdelete(PG_FUNCTION_ARGS);
extern void _gistdump(Relation r);
extern void initGISTstate(GISTSTATE *giststate, Relation index);
extern void freeGISTstate(GISTSTATE *giststate);
extern void gistdentryinit(GISTSTATE *giststate, int nkey, GISTENTRY *e,
			   Datum k, Relation r, Page pg, OffsetNumber o,
			   int b, bool l, bool isNull);

extern void gist_redo(XLogRecPtr lsn, XLogRecord *record);
extern void gist_undo(XLogRecPtr lsn, XLogRecord *record);
extern void gist_desc(char *buf, uint8 xl_info, char *rec);
extern MemoryContext createTempGistContext(void);

/* gistget.c */
extern Datum gistgettuple(PG_FUNCTION_ARGS);
extern Datum gistgetmulti(PG_FUNCTION_ARGS);

#endif   /* GIST_H */
