/*-------------------------------------------------------------------------
 *
 * nbtpage.c
 *	  BTree-specific page management code for the Postgres btree access
 *	  method.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/nbtree/nbtpage.c,v 1.36 2000-04-12 17:14:49 momjian Exp $
 *
 *	NOTES
 *	   Postgres btree pages look like ordinary relation pages.	The opaque
 *	   data at high addresses includes pointers to left and right siblings
 *	   and flag data describing page state.  The first page in a btree, page
 *	   zero, is special -- it stores meta-information describing the tree.
 *	   Pages one and higher store the actual tree data.
 *
 *-------------------------------------------------------------------------
 */
#include <time.h>

#include "postgres.h"

#include "access/nbtree.h"
#include "miscadmin.h"

#define BTREE_METAPAGE	0
#define BTREE_MAGIC		0x053162

#define BTREE_VERSION	1

typedef struct BTMetaPageData
{
	uint32		btm_magic;
	uint32		btm_version;
	BlockNumber btm_root;
	int32		btm_level;
} BTMetaPageData;

#define BTPageGetMeta(p) \
	((BTMetaPageData *) &((PageHeader) p)->pd_linp[0])


/*
 *	We use high-concurrency locking on btrees.	There are two cases in
 *	which we don't do locking.  One is when we're building the btree.
 *	Since the creating transaction has not committed, no one can see
 *	the index, and there's no reason to share locks.  The second case
 *	is when we're just starting up the database system.  We use some
 *	special-purpose initialization code in the relation cache manager
 *	(see utils/cache/relcache.c) to allow us to do indexed scans on
 *	the system catalogs before we'd normally be able to.  This happens
 *	before the lock table is fully initialized, so we can't use it.
 *	Strictly speaking, this violates 2pl, but we don't do 2pl on the
 *	system catalogs anyway, so I declare this to be okay.
 */

#define USELOCKING		(!BuildingBtree && !IsInitProcessingMode())

/*
 *	_bt_metapinit() -- Initialize the metadata page of a btree.
 */
void
_bt_metapinit(Relation rel)
{
	Buffer		buf;
	Page		pg;
	int			nblocks;
	BTMetaPageData metad;
	BTPageOpaque op;

	/* can't be sharing this with anyone, now... */
	if (USELOCKING)
		LockRelation(rel, AccessExclusiveLock);

	if ((nblocks = RelationGetNumberOfBlocks(rel)) != 0)
	{
		elog(ERROR, "Cannot initialize non-empty btree %s",
			 RelationGetRelationName(rel));
	}

	buf = ReadBuffer(rel, P_NEW);
	pg = BufferGetPage(buf);
	_bt_pageinit(pg, BufferGetPageSize(buf));

	metad.btm_magic = BTREE_MAGIC;
	metad.btm_version = BTREE_VERSION;
	metad.btm_root = P_NONE;
	metad.btm_level = 0;
	memmove((char *) BTPageGetMeta(pg), (char *) &metad, sizeof(metad));

	op = (BTPageOpaque) PageGetSpecialPointer(pg);
	op->btpo_flags = BTP_META;

	WriteBuffer(buf);

	/* all done */
	if (USELOCKING)
		UnlockRelation(rel, AccessExclusiveLock);
}

#ifdef NOT_USED
/*
 *	_bt_checkmeta() -- Verify that the metadata stored in a btree are
 *					   reasonable.
 */
void
_bt_checkmeta(Relation rel)
{
	Buffer		metabuf;
	Page		metap;
	BTMetaPageData *metad;
	BTPageOpaque op;
	int			nblocks;

	/* if the relation is empty, this is init time; don't complain */
	if ((nblocks = RelationGetNumberOfBlocks(rel)) == 0)
		return;

	metabuf = _bt_getbuf(rel, BTREE_METAPAGE, BT_READ);
	metap = BufferGetPage(metabuf);
	op = (BTPageOpaque) PageGetSpecialPointer(metap);
	if (!(op->btpo_flags & BTP_META))
	{
		elog(ERROR, "Invalid metapage for index %s",
			 RelationGetRelationName(rel));
	}
	metad = BTPageGetMeta(metap);

	if (metad->btm_magic != BTREE_MAGIC)
	{
		elog(ERROR, "Index %s is not a btree",
			 RelationGetRelationName(rel));
	}

	if (metad->btm_version != BTREE_VERSION)
	{
		elog(ERROR, "Version mismatch on %s:  version %d file, version %d code",
			 RelationGetRelationName(rel),
			 metad->btm_version, BTREE_VERSION);
	}

	_bt_relbuf(rel, metabuf, BT_READ);
}

#endif

/*
 *	_bt_getroot() -- Get the root page of the btree.
 *
 *		Since the root page can move around the btree file, we have to read
 *		its location from the metadata page, and then read the root page
 *		itself.  If no root page exists yet, we have to create one.  The
 *		standard class of race conditions exists here; I think I covered
 *		them all in the Hopi Indian rain dance of lock requests below.
 *
 *		We pass in the access type (BT_READ or BT_WRITE), and return the
 *		root page's buffer with the appropriate lock type set.  Reference
 *		count on the root page gets bumped by ReadBuffer.  The metadata
 *		page is unlocked and unreferenced by this process when this routine
 *		returns.
 */
Buffer
_bt_getroot(Relation rel, int access)
{
	Buffer		metabuf;
	Page		metapg;
	BTPageOpaque metaopaque;
	Buffer		rootbuf;
	Page		rootpg;
	BTPageOpaque rootopaque;
	BlockNumber rootblkno;
	BTMetaPageData *metad;

	metabuf = _bt_getbuf(rel, BTREE_METAPAGE, BT_READ);
	metapg = BufferGetPage(metabuf);
	metaopaque = (BTPageOpaque) PageGetSpecialPointer(metapg);
	Assert(metaopaque->btpo_flags & BTP_META);
	metad = BTPageGetMeta(metapg);

	if (metad->btm_magic != BTREE_MAGIC)
	{
		elog(ERROR, "Index %s is not a btree",
			 RelationGetRelationName(rel));
	}

	if (metad->btm_version != BTREE_VERSION)
	{
		elog(ERROR, "Version mismatch on %s:  version %d file, version %d code",
			 RelationGetRelationName(rel),
			 metad->btm_version, BTREE_VERSION);
	}

	/* if no root page initialized yet, do it */
	if (metad->btm_root == P_NONE)
	{

		/* turn our read lock in for a write lock */
		_bt_relbuf(rel, metabuf, BT_READ);
		metabuf = _bt_getbuf(rel, BTREE_METAPAGE, BT_WRITE);
		metapg = BufferGetPage(metabuf);
		metaopaque = (BTPageOpaque) PageGetSpecialPointer(metapg);
		Assert(metaopaque->btpo_flags & BTP_META);
		metad = BTPageGetMeta(metapg);

		/*
		 * Race condition:	if someone else initialized the metadata
		 * between the time we released the read lock and acquired the
		 * write lock, above, we want to avoid doing it again.
		 */

		if (metad->btm_root == P_NONE)
		{

			/*
			 * Get, initialize, write, and leave a lock of the appropriate
			 * type on the new root page.  Since this is the first page in
			 * the tree, it's a leaf.
			 */

			rootbuf = _bt_getbuf(rel, P_NEW, BT_WRITE);
			rootblkno = BufferGetBlockNumber(rootbuf);
			rootpg = BufferGetPage(rootbuf);
			metad->btm_root = rootblkno;
			metad->btm_level = 1;
			_bt_pageinit(rootpg, BufferGetPageSize(rootbuf));
			rootopaque = (BTPageOpaque) PageGetSpecialPointer(rootpg);
			rootopaque->btpo_flags |= (BTP_LEAF | BTP_ROOT);
			_bt_wrtnorelbuf(rel, rootbuf);

			/* swap write lock for read lock, if appropriate */
			if (access != BT_WRITE)
			{
				LockBuffer(rootbuf, BUFFER_LOCK_UNLOCK);
				LockBuffer(rootbuf, BT_READ);
			}

			/* okay, metadata is correct */
			_bt_wrtbuf(rel, metabuf);
		}
		else
		{

			/*
			 * Metadata initialized by someone else.  In order to
			 * guarantee no deadlocks, we have to release the metadata
			 * page and start all over again.
			 */

			_bt_relbuf(rel, metabuf, BT_WRITE);
			return _bt_getroot(rel, access);
		}
	}
	else
	{
		rootblkno = metad->btm_root;
		_bt_relbuf(rel, metabuf, BT_READ);		/* done with the meta page */

		rootbuf = _bt_getbuf(rel, rootblkno, access);
	}

	/*
	 * Race condition:	If the root page split between the time we looked
	 * at the metadata page and got the root buffer, then we got the wrong
	 * buffer.
	 */

	rootpg = BufferGetPage(rootbuf);
	rootopaque = (BTPageOpaque) PageGetSpecialPointer(rootpg);
	if (!(rootopaque->btpo_flags & BTP_ROOT))
	{

		/* it happened, try again */
		_bt_relbuf(rel, rootbuf, access);
		return _bt_getroot(rel, access);
	}

	/*
	 * By here, we have a correct lock on the root block, its reference
	 * count is correct, and we have no lock set on the metadata page.
	 * Return the root block.
	 */

	return rootbuf;
}

/*
 *	_bt_getbuf() -- Get a buffer by block number for read or write.
 *
 *		When this routine returns, the appropriate lock is set on the
 *		requested buffer its reference count is correct.
 */
Buffer
_bt_getbuf(Relation rel, BlockNumber blkno, int access)
{
	Buffer		buf;
	Page		page;

	if (blkno != P_NEW)
	{
		buf = ReadBuffer(rel, blkno);
		LockBuffer(buf, access);
	}
	else
	{

		/*
		 * Extend bufmgr code is unclean and so we have to use locking
		 * here.
		 */
		LockPage(rel, 0, ExclusiveLock);
		buf = ReadBuffer(rel, blkno);
		UnlockPage(rel, 0, ExclusiveLock);
		blkno = BufferGetBlockNumber(buf);
		page = BufferGetPage(buf);
		_bt_pageinit(page, BufferGetPageSize(buf));
		LockBuffer(buf, access);
	}

	/* ref count and lock type are correct */
	return buf;
}

/*
 *	_bt_relbuf() -- release a locked buffer.
 */
void
_bt_relbuf(Relation rel, Buffer buf, int access)
{
	LockBuffer(buf, BUFFER_LOCK_UNLOCK);
	ReleaseBuffer(buf);
}

/*
 *	_bt_wrtbuf() -- write a btree page to disk.
 *
 *		This routine releases the lock held on the buffer and our reference
 *		to it.	It is an error to call _bt_wrtbuf() without a write lock
 *		or a reference to the buffer.
 */
void
_bt_wrtbuf(Relation rel, Buffer buf)
{
	LockBuffer(buf, BUFFER_LOCK_UNLOCK);
	WriteBuffer(buf);
}

/*
 *	_bt_wrtnorelbuf() -- write a btree page to disk, but do not release
 *						 our reference or lock.
 *
 *		It is an error to call _bt_wrtnorelbuf() without a write lock
 *		or a reference to the buffer.
 */
void
_bt_wrtnorelbuf(Relation rel, Buffer buf)
{
	WriteNoReleaseBuffer(buf);
}

/*
 *	_bt_pageinit() -- Initialize a new page.
 */
void
_bt_pageinit(Page page, Size size)
{

	/*
	 * Cargo_cult programming -- don't really need this to be zero, but
	 * creating new pages is an infrequent occurrence and it makes me feel
	 * good when I know they're empty.
	 */

	MemSet(page, 0, size);

	PageInit(page, size, sizeof(BTPageOpaqueData));
	((BTPageOpaque) PageGetSpecialPointer(page))->btpo_parent =
		InvalidBlockNumber;
}

/*
 *	_bt_metaproot() -- Change the root page of the btree.
 *
 *		Lehman and Yao require that the root page move around in order to
 *		guarantee deadlock-free short-term, fine-granularity locking.  When
 *		we split the root page, we record the new parent in the metadata page
 *		for the relation.  This routine does the work.
 *
 *		No direct preconditions, but if you don't have the a write lock on
 *		at least the old root page when you call this, you're making a big
 *		mistake.  On exit, metapage data is correct and we no longer have
 *		a reference to or lock on the metapage.
 */
void
_bt_metaproot(Relation rel, BlockNumber rootbknum, int level)
{
	Buffer		metabuf;
	Page		metap;
	BTPageOpaque metaopaque;
	BTMetaPageData *metad;

	metabuf = _bt_getbuf(rel, BTREE_METAPAGE, BT_WRITE);
	metap = BufferGetPage(metabuf);
	metaopaque = (BTPageOpaque) PageGetSpecialPointer(metap);
	Assert(metaopaque->btpo_flags & BTP_META);
	metad = BTPageGetMeta(metap);
	metad->btm_root = rootbknum;
	if (level == 0)				/* called from _do_insert */
		metad->btm_level += 1;
	else
		metad->btm_level = level;		/* called from btsort */
	_bt_wrtbuf(rel, metabuf);
}

/*
 *	_bt_getstackbuf() -- Walk back up the tree one step, and find the item
 *						 we last looked at in the parent.
 *
 *		This is possible because we save a bit image of the last item
 *		we looked at in the parent, and the update algorithm guarantees
 *		that if items above us in the tree move, they only move right.
 *
 *		Also, re-set bts_blkno & bts_offset if changed and
 *		bts_btitem (it may be changed - see _bt_insertonpg).
 */
Buffer
_bt_getstackbuf(Relation rel, BTStack stack, int access)
{
	Buffer		buf;
	BlockNumber blkno;
	OffsetNumber start,
				offnum,
				maxoff;
	OffsetNumber i;
	Page		page;
	ItemId		itemid;
	BTItem		item;
	BTPageOpaque opaque;
	BTItem		item_save;
	int			item_nbytes;

	blkno = stack->bts_blkno;
	buf = _bt_getbuf(rel, blkno, access);
	page = BufferGetPage(buf);
	opaque = (BTPageOpaque) PageGetSpecialPointer(page);
	maxoff = PageGetMaxOffsetNumber(page);

	if (stack->bts_offset == InvalidOffsetNumber ||
		maxoff >= stack->bts_offset)
	{

		/*
		 * _bt_insertonpg set bts_offset to InvalidOffsetNumber in the
		 * case of concurrent ROOT page split
		 */
		if (stack->bts_offset == InvalidOffsetNumber)
			i = P_RIGHTMOST(opaque) ? P_HIKEY : P_FIRSTKEY;
		else
		{
			itemid = PageGetItemId(page, stack->bts_offset);
			item = (BTItem) PageGetItem(page, itemid);

			/* if the item is where we left it, we're done */
			if (BTItemSame(item, stack->bts_btitem))
			{
				pfree(stack->bts_btitem);
				item_nbytes = ItemIdGetLength(itemid);
				item_save = (BTItem) palloc(item_nbytes);
				memmove((char *) item_save, (char *) item, item_nbytes);
				stack->bts_btitem = item_save;
				return buf;
			}
			i = OffsetNumberNext(stack->bts_offset);
		}

		/* if the item has just moved right on this page, we're done */
		for (;
			 i <= maxoff;
			 i = OffsetNumberNext(i))
		{
			itemid = PageGetItemId(page, i);
			item = (BTItem) PageGetItem(page, itemid);

			/* if the item is where we left it, we're done */
			if (BTItemSame(item, stack->bts_btitem))
			{
				stack->bts_offset = i;
				pfree(stack->bts_btitem);
				item_nbytes = ItemIdGetLength(itemid);
				item_save = (BTItem) palloc(item_nbytes);
				memmove((char *) item_save, (char *) item, item_nbytes);
				stack->bts_btitem = item_save;
				return buf;
			}
		}
	}

	/* by here, the item we're looking for moved right at least one page */
	for (;;)
	{
		blkno = opaque->btpo_next;
		if (P_RIGHTMOST(opaque))
			elog(FATAL, "my bits moved right off the end of the world!\
\n\tRecreate index %s.", RelationGetRelationName(rel));

		_bt_relbuf(rel, buf, access);
		buf = _bt_getbuf(rel, blkno, access);
		page = BufferGetPage(buf);
		maxoff = PageGetMaxOffsetNumber(page);
		opaque = (BTPageOpaque) PageGetSpecialPointer(page);

		/* if we have a right sibling, step over the high key */
		start = P_RIGHTMOST(opaque) ? P_HIKEY : P_FIRSTKEY;

		/* see if it's on this page */
		for (offnum = start;
			 offnum <= maxoff;
			 offnum = OffsetNumberNext(offnum))
		{
			itemid = PageGetItemId(page, offnum);
			item = (BTItem) PageGetItem(page, itemid);
			if (BTItemSame(item, stack->bts_btitem))
			{
				stack->bts_offset = offnum;
				stack->bts_blkno = blkno;
				pfree(stack->bts_btitem);
				item_nbytes = ItemIdGetLength(itemid);
				item_save = (BTItem) palloc(item_nbytes);
				memmove((char *) item_save, (char *) item, item_nbytes);
				stack->bts_btitem = item_save;
				return buf;
			}
		}
	}
}

void
_bt_pagedel(Relation rel, ItemPointer tid)
{
	Buffer		buf;
	Page		page;
	BlockNumber blkno;
	OffsetNumber offno;

	blkno = ItemPointerGetBlockNumber(tid);
	offno = ItemPointerGetOffsetNumber(tid);

	buf = _bt_getbuf(rel, blkno, BT_WRITE);
	page = BufferGetPage(buf);

	PageIndexTupleDelete(page, offno);

	/* write the buffer and release the lock */
	_bt_wrtbuf(rel, buf);
}
