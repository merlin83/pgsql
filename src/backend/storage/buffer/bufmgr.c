/*-------------------------------------------------------------------------
 *
 * bufmgr.c
 *	  buffer manager interface routines
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/buffer/bufmgr.c,v 1.74 2000-02-21 18:47:03 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *
 * BufferAlloc() -- lookup a buffer in the buffer table.  If
 *		it isn't there add it, but do not read it into memory.
 *		This is used when we are about to reinitialize the
 *		buffer so don't care what the current disk contents are.
 *		BufferAlloc() pins the new buffer in memory.
 *
 * ReadBuffer() -- same as BufferAlloc() but reads the data
 *		on a buffer cache miss.
 *
 * ReleaseBuffer() -- unpin the buffer
 *
 * WriteNoReleaseBuffer() -- mark the buffer contents as "dirty"
 *		but don't unpin.  The disk IO is delayed until buffer
 *		replacement if WriteMode is BUFFER_LATE_WRITE.
 *
 * WriteBuffer() -- WriteNoReleaseBuffer() + ReleaseBuffer()
 *
 * FlushBuffer() -- as above but never delayed write.
 *
 * BufferSync() -- flush all dirty buffers in the buffer pool.
 *
 * InitBufferPool() -- Init the buffer module.
 *
 * See other files:
 *		freelist.c -- chooses victim for buffer replacement
 *		buf_table.c -- manages the buffer lookup table
 */
#include <sys/types.h>
#include <sys/file.h>
#include <math.h>
#include <signal.h>

#include "postgres.h"
#include "executor/execdebug.h"
#include "miscadmin.h"
#include "storage/s_lock.h"
#include "storage/smgr.h"
#include "utils/relcache.h"

extern SPINLOCK BufMgrLock;
extern long int ReadBufferCount;
extern long int ReadLocalBufferCount;
extern long int BufferHitCount;
extern long int LocalBufferHitCount;
extern long int BufferFlushCount;
extern long int LocalBufferFlushCount;

/*
 * It's used to avoid disk writes for read-only transactions
 * (i.e. when no one shared buffer was changed by transaction).
 * We set it to true in WriteBuffer/WriteNoReleaseBuffer when
 * marking shared buffer as dirty. We set it to false in xact.c
 * after transaction is committed/aborted.
 */
bool			SharedBufferChanged = false;

static int	WriteMode = BUFFER_LATE_WRITE;		/* Delayed write is
												 * default */

static void WaitIO(BufferDesc *buf, SPINLOCK spinlock);

static void StartBufferIO(BufferDesc *buf, bool forInput);
static void TerminateBufferIO(BufferDesc *buf);
static void ContinueBufferIO(BufferDesc *buf, bool forInput);
extern void InitBufferIO(void);
extern void AbortBufferIO(void);

/*
 * Macro : BUFFER_IS_BROKEN
 *      Note that write error doesn't mean the buffer broken
*/
#define BUFFER_IS_BROKEN(buf) ((buf->flags & BM_IO_ERROR) && !(buf->flags & BM_DIRTY))

#ifndef HAS_TEST_AND_SET
static void SignalIO(BufferDesc *buf);
extern long *NWaitIOBackendP;	/* defined in buf_init.c */

#endif	 /* HAS_TEST_AND_SET */

static Buffer ReadBufferWithBufferLock(Relation relation, BlockNumber blockNum,
						 bool bufferLockHeld);
static BufferDesc *BufferAlloc(Relation reln, BlockNumber blockNum,
			bool *foundPtr, bool bufferLockHeld);
static int	FlushBuffer(Buffer buffer, bool release);
static void BufferSync(void);
static int	BufferReplace(BufferDesc *bufHdr, bool bufferLockHeld);
void		PrintBufferDescs(void);

/* ---------------------------------------------------
 * RelationGetBufferWithBuffer
 *		see if the given buffer is what we want
 *		if yes, we don't need to bother the buffer manager
 * ---------------------------------------------------
 */
Buffer
RelationGetBufferWithBuffer(Relation relation,
							BlockNumber blockNumber,
							Buffer buffer)
{
	BufferDesc *bufHdr;

	if (BufferIsValid(buffer))
	{
		if (!BufferIsLocal(buffer))
		{
			LockRelId  *lrelId = & relation->rd_lockInfo.lockRelId;

			bufHdr = &BufferDescriptors[buffer - 1];
			SpinAcquire(BufMgrLock);
			if (bufHdr->tag.blockNum == blockNumber &&
				bufHdr->tag.relId.relId == lrelId->relId &&
				bufHdr->tag.relId.dbId == lrelId->dbId)
			{
				SpinRelease(BufMgrLock);
				return buffer;
			}
			return ReadBufferWithBufferLock(relation, blockNumber, true);
		}
		else
		{
			bufHdr = &LocalBufferDescriptors[-buffer - 1];
			if (bufHdr->tag.relId.relId == RelationGetRelid(relation) &&
				bufHdr->tag.blockNum == blockNumber)
				return buffer;
		}
	}
	return ReadBuffer(relation, blockNumber);
}

/*
 * ReadBuffer -- returns a buffer containing the requested
 *		block of the requested relation.  If the blknum
 *		requested is P_NEW, extend the relation file and
 *		allocate a new block.
 *
 * Returns: the buffer number for the buffer containing
 *		the block read or NULL on an error.
 *
 * Assume when this function is called, that reln has been
 *		opened already.
 */

#undef ReadBuffer				/* conflicts with macro when BUFMGR_DEBUG
								 * defined */

/*
 * ReadBuffer
 *
 */
Buffer
ReadBuffer(Relation reln, BlockNumber blockNum)
{
	return ReadBufferWithBufferLock(reln, blockNum, false);
}

/*
 * is_userbuffer
 *
 * XXX caller must have already acquired BufMgrLock
 */
#ifdef NOT_USED
static bool
is_userbuffer(Buffer buffer)
{
	BufferDesc *buf = &BufferDescriptors[buffer - 1];

	if (IsSystemRelationName(buf->sb_relname))
		return false;
	return true;
}

#endif

#ifdef NOT_USED
Buffer
ReadBuffer_Debug(char *file,
				 int line,
				 Relation reln,
				 BlockNumber blockNum)
{
	Buffer		buffer;

	buffer = ReadBufferWithBufferLock(reln, blockNum, false);
	if (ShowPinTrace && !BufferIsLocal(buffer) && is_userbuffer(buffer))
	{
		BufferDesc *buf = &BufferDescriptors[buffer - 1];

		fprintf(stderr, "PIN(RD) %ld relname = %s, blockNum = %d, \
refcount = %ld, file: %s, line: %d\n",
				buffer, buf->sb_relname, buf->tag.blockNum,
				PrivateRefCount[buffer - 1], file, line);
	}
	return buffer;
}

#endif

/*
 * ReadBufferWithBufferLock -- does the work of
 *		ReadBuffer() but with the possibility that
 *		the buffer lock has already been held. this
 *		is yet another effort to reduce the number of
 *		semops in the system.
 */
static Buffer
ReadBufferWithBufferLock(Relation reln,
						 BlockNumber blockNum,
						 bool bufferLockHeld)
{
	BufferDesc *bufHdr;
	int			extend;			/* extending the file by one block */
	int			status;
	bool		found;
	bool		isLocalBuf;

	extend = (blockNum == P_NEW);
	isLocalBuf = reln->rd_myxactonly;

	if (isLocalBuf)
	{
		ReadLocalBufferCount++;
		bufHdr = LocalBufferAlloc(reln, blockNum, &found);
		if (found)
			LocalBufferHitCount++;
	}
	else
	{
		ReadBufferCount++;

		/*
		 * lookup the buffer.  IO_IN_PROGRESS is set if the requested
		 * block is not currently in memory.
		 */
		bufHdr = BufferAlloc(reln, blockNum, &found, bufferLockHeld);
		if (found)
			BufferHitCount++;
	}

	if (!bufHdr)
		return InvalidBuffer;

	/* if it's already in the buffer pool, we're done */
	if (found)
	{

		/*
		 * This happens when a bogus buffer was returned previously and is
		 * floating around in the buffer pool.	A routine calling this
		 * would want this extended.
		 */
		if (extend)
		{
			/* new buffers are zero-filled */
			MemSet((char *) MAKE_PTR(bufHdr->data), 0, BLCKSZ);
			smgrextend(DEFAULT_SMGR, reln,
					   (char *) MAKE_PTR(bufHdr->data));
		}
		return BufferDescriptorGetBuffer(bufHdr);

	}

	/*
	 * if we have gotten to this point, the reln pointer must be ok and
	 * the relation file must be open.
	 */
	if (extend)
	{
		/* new buffers are zero-filled */
		MemSet((char *) MAKE_PTR(bufHdr->data), 0, BLCKSZ);
		status = smgrextend(DEFAULT_SMGR, reln,
							(char *) MAKE_PTR(bufHdr->data));
	}
	else
	{
		status = smgrread(DEFAULT_SMGR, reln, blockNum,
						  (char *) MAKE_PTR(bufHdr->data));
	}

	if (isLocalBuf)
		return BufferDescriptorGetBuffer(bufHdr);

	/* lock buffer manager again to update IO IN PROGRESS */
	SpinAcquire(BufMgrLock);

	if (status == SM_FAIL)
	{
		/* IO Failed.  cleanup the data structures and go home */

		if (!BufTableDelete(bufHdr))
		{
			SpinRelease(BufMgrLock);
			elog(FATAL, "BufRead: buffer table broken after IO error\n");
		}
		/* remember that BufferAlloc() pinned the buffer */
		UnpinBuffer(bufHdr);

		/*
		 * Have to reset the flag so that anyone waiting for the buffer
		 * can tell that the contents are invalid.
		 */
		bufHdr->flags |= BM_IO_ERROR;
		bufHdr->flags &= ~BM_IO_IN_PROGRESS;
	}
	else
	{
		/* IO Succeeded.  clear the flags, finish buffer update */

		bufHdr->flags &= ~(BM_IO_ERROR | BM_IO_IN_PROGRESS);
	}

	/* If anyone was waiting for IO to complete, wake them up now */
	TerminateBufferIO(bufHdr);

	SpinRelease(BufMgrLock);

	if (status == SM_FAIL)
		return InvalidBuffer;

	return BufferDescriptorGetBuffer(bufHdr);
}

/*
 * BufferAlloc -- Get a buffer from the buffer pool but dont
 *		read it.
 *
 * Returns: descriptor for buffer
 *
 * When this routine returns, the BufMgrLock is guaranteed NOT be held.
 */
static BufferDesc *
BufferAlloc(Relation reln,
			BlockNumber blockNum,
			bool *foundPtr,
			bool bufferLockHeld)
{
	BufferDesc *buf,
			   *buf2;
	BufferTag	newTag;			/* identity of requested block */
	bool		inProgress;		/* buffer undergoing IO */
	bool		newblock = FALSE;

	/* create a new tag so we can lookup the buffer */
	/* assume that the relation is already open */
	if (blockNum == P_NEW)
	{
		newblock = TRUE;
		blockNum = smgrnblocks(DEFAULT_SMGR, reln);
	}

	INIT_BUFFERTAG(&newTag, reln, blockNum);

	if (!bufferLockHeld)
		SpinAcquire(BufMgrLock);

	/* see if the block is in the buffer pool already */
	buf = BufTableLookup(&newTag);
	if (buf != NULL)
	{

		/*
		 * Found it.  Now, (a) pin the buffer so no one steals it from the
		 * buffer pool, (b) check IO_IN_PROGRESS, someone may be faulting
		 * the buffer into the buffer pool.
		 */

		PinBuffer(buf);
		inProgress = (buf->flags & BM_IO_IN_PROGRESS);

		*foundPtr = TRUE;
		if (inProgress) /* confirm end of IO */
		{
			WaitIO(buf, BufMgrLock);
			inProgress = (buf->flags & BM_IO_IN_PROGRESS);
		}
		if (BUFFER_IS_BROKEN(buf))
		{
			/* I couldn't understand the following old comment.
			 * If there's no IO for the buffer and the buffer
			 * is BROKEN,it should be read again. So start a
			 * new buffer IO here. 

				 *
				 * wierd race condition:
				 *
				 * We were waiting for someone else to read the buffer. While
				 * we were waiting, the reader boof'd in some way, so the
				 * contents of the buffer are still invalid.  By saying
				 * that we didn't find it, we can make the caller
				 * reinitialize the buffer.  If two processes are waiting
				 * for this block, both will read the block.  The second
				 * one to finish may overwrite any updates made by the
				 * first.  (Assume higher level synchronization prevents
				 * this from happening).
				 *
				 * This is never going to happen, don't worry about it.
				 */
			*foundPtr = FALSE;
		}
#ifdef BMTRACE
		_bm_trace((reln->rd_rel->relisshared ? 0 : MyDatabaseId), RelationGetRelid(reln), blockNum, BufferDescriptorGetBuffer(buf), BMT_ALLOCFND);
#endif	 /* BMTRACE */

		if (!(*foundPtr))
			StartBufferIO(buf, true);
		SpinRelease(BufMgrLock);

		return buf;
	}

	*foundPtr = FALSE;

	/*
	 * Didn't find it in the buffer pool.  We'll have to initialize a new
	 * buffer.	First, grab one from the free list.  If it's dirty, flush
	 * it to disk. Remember to unlock BufMgr spinlock while doing the IOs.
	 */
	inProgress = FALSE;
	for (buf = (BufferDesc *) NULL; buf == (BufferDesc *) NULL;)
	{

		/* GetFreeBuffer will abort if it can't find a free buffer */
		buf = GetFreeBuffer();

		/*
		 * But it can return buf == NULL if we are in aborting transaction
		 * now and so elog(ERROR,...) in GetFreeBuffer will not abort
		 * again.
		 */
		if (buf == NULL)
			return NULL;

		/*
		 * There should be exactly one pin on the buffer after it is
		 * allocated -- ours.  If it had a pin it wouldn't have been on
		 * the free list.  No one else could have pinned it between
		 * GetFreeBuffer and here because we have the BufMgrLock.
		 */
		Assert(buf->refcount == 0);
		buf->refcount = 1;
		Assert(PrivateRefCount[BufferDescriptorGetBuffer(buf) - 1] == 0);
		PrivateRefCount[BufferDescriptorGetBuffer(buf) - 1] = 1;

		if (buf->flags & BM_DIRTY)
		{
			bool		smok;

			/*
			 * Set BM_IO_IN_PROGRESS to keep anyone from doing anything
			 * with the contents of the buffer while we write it out. We
			 * don't really care if they try to read it, but if they can
			 * complete a BufferAlloc on it they can then scribble into
			 * it, and we'd really like to avoid that while we are
			 * flushing the buffer.  Setting this flag should block them
			 * in WaitIO until we're done.
			 */
			inProgress = TRUE;
#ifdef HAS_TEST_AND_SET

			/*
			 * All code paths that acquire this lock pin the buffer first;
			 * since no one had it pinned (it just came off the free
			 * list), no one else can have this lock.
			 */
#endif	 /* HAS_TEST_AND_SET */
			StartBufferIO(buf, false);

			/*
			 * Write the buffer out, being careful to release BufMgrLock
			 * before starting the I/O.
			 *
			 * This #ifndef is here because a few extra semops REALLY kill
			 * you on machines that don't have spinlocks.  If you don't
			 * operate with much concurrency, well...
			 */
			smok = BufferReplace(buf, true);
#ifndef OPTIMIZE_SINGLE
			SpinAcquire(BufMgrLock);
#endif	 /* OPTIMIZE_SINGLE */

			if (smok == FALSE)
			{
				elog(NOTICE, "BufferAlloc: cannot write block %u for %s/%s",
					 buf->tag.blockNum, buf->sb_dbname, buf->sb_relname);
				inProgress = FALSE;
				buf->flags |= BM_IO_ERROR;
				buf->flags &= ~BM_IO_IN_PROGRESS;
				TerminateBufferIO(buf);
				PrivateRefCount[BufferDescriptorGetBuffer(buf) - 1] = 0;
				Assert(buf->refcount > 0);
				buf->refcount--;
				if (buf->refcount == 0)
				{
					AddBufferToFreelist(buf);
					buf->flags |= BM_FREE;
				}
				buf = (BufferDesc *) NULL;
			}
			else
			{

				/*
				 * BM_JUST_DIRTIED cleared by BufferReplace and shouldn't
				 * be setted by anyone.		- vadim 01/17/97
				 */
				if (buf->flags & BM_JUST_DIRTIED)
				{
					elog(FATAL, "BufferAlloc: content of block %u (%s) changed while flushing",
						 buf->tag.blockNum, buf->sb_relname);
				}
				else
					buf->flags &= ~BM_DIRTY;
			}

			/*
			 * Somebody could have pinned the buffer while we were doing
			 * the I/O and had given up the BufMgrLock (though they would
			 * be waiting for us to clear the BM_IO_IN_PROGRESS flag).
			 * That's why this is a loop -- if so, we need to clear the
			 * I/O flags, remove our pin and start all over again.
			 *
			 * People may be making buffers free at any time, so there's no
			 * reason to think that we have an immediate disaster on our
			 * hands.
			 */
			if (buf && buf->refcount > 1)
			{
				inProgress = FALSE;
				buf->flags &= ~BM_IO_IN_PROGRESS;
				TerminateBufferIO(buf);
				Assert(PrivateRefCount[BufferDescriptorGetBuffer(buf)-1] == 1);
				PrivateRefCount[BufferDescriptorGetBuffer(buf) - 1] = 0;
				buf->refcount--;
				buf = (BufferDesc *) NULL;
			}

			/*
			 * Somebody could have allocated another buffer for the same
			 * block we are about to read in. (While we flush out the
			 * dirty buffer, we don't hold the lock and someone could have
			 * allocated another buffer for the same block. The problem is
			 * we haven't gotten around to insert the new tag into the
			 * buffer table. So we need to check here.		-ay 3/95
			 */
			buf2 = BufTableLookup(&newTag);
			if (buf2 != NULL)
			{

				/*
				 * Found it. Someone has already done what we're about to
				 * do. We'll just handle this as if it were found in the
				 * buffer pool in the first place.
				 */
				if (buf != NULL)
				{
					TerminateBufferIO(buf);
					/* give up the buffer since we don't need it any more */
					Assert(PrivateRefCount[BufferDescriptorGetBuffer(buf)-1] == 1);
					PrivateRefCount[BufferDescriptorGetBuffer(buf) - 1] = 0;
					Assert(buf->refcount > 0);
					buf->refcount--;
					if (buf->refcount == 0)
					{
						AddBufferToFreelist(buf);
						buf->flags |= BM_FREE;
					}
					buf->flags &= ~BM_IO_IN_PROGRESS;
				}

				PinBuffer(buf2);
				inProgress = (buf2->flags & BM_IO_IN_PROGRESS);

				*foundPtr = TRUE;
				if (inProgress)
				{
					WaitIO(buf2, BufMgrLock);
					inProgress = (buf2->flags & BM_IO_IN_PROGRESS);
				}
				if (BUFFER_IS_BROKEN(buf2))
					*foundPtr = FALSE;

				if (!(*foundPtr))
					StartBufferIO(buf2, true);
				SpinRelease(BufMgrLock);

				return buf2;
			}
		}
	}

	/*
	 * At this point we should have the sole pin on a non-dirty buffer and
	 * we may or may not already have the BM_IO_IN_PROGRESS flag set.
	 */

	/*
	 * Change the name of the buffer in the lookup table:
	 *
	 * Need to update the lookup table before the read starts. If someone
	 * comes along looking for the buffer while we are reading it in, we
	 * don't want them to allocate a new buffer.  For the same reason, we
	 * didn't want to erase the buf table entry for the buffer we were
	 * writing back until now, either.
	 */

	if (!BufTableDelete(buf))
	{
		SpinRelease(BufMgrLock);
		elog(FATAL, "buffer wasn't in the buffer table\n");

	}

	/* record the database name and relation name for this buffer */
	strcpy(buf->sb_relname, RelationGetPhysicalRelationName(reln));
	strcpy(buf->sb_dbname, DatabaseName);

	INIT_BUFFERTAG(&(buf->tag), reln, blockNum);
	if (!BufTableInsert(buf))
	{
		SpinRelease(BufMgrLock);
		elog(FATAL, "Buffer in lookup table twice \n");
	}

	/*
	 * Buffer contents are currently invalid.  Have to mark IO IN PROGRESS
	 * so no one fiddles with them until the read completes.  If this
	 * routine has been called simply to allocate a buffer, no io will be
	 * attempted, so the flag isnt set.
	 */
	if (!inProgress)
	{
		StartBufferIO(buf, true);
	}
	else
		ContinueBufferIO(buf, true);

#ifdef BMTRACE
	_bm_trace((reln->rd_rel->relisshared ? 0 : MyDatabaseId), RelationGetRelid(reln), blockNum, BufferDescriptorGetBuffer(buf), BMT_ALLOCNOTFND);
#endif	 /* BMTRACE */

	SpinRelease(BufMgrLock);

	return buf;
}

/*
 * WriteBuffer
 *
 *		Pushes buffer contents to disk if WriteMode is BUFFER_FLUSH_WRITE.
 *		Otherwise, marks contents as dirty.
 *
 * Assume that buffer is pinned.  Assume that reln is
 *		valid.
 *
 * Side Effects:
 *		Pin count is decremented.
 */

#undef WriteBuffer

int
WriteBuffer(Buffer buffer)
{
	BufferDesc *bufHdr;

	if (WriteMode == BUFFER_FLUSH_WRITE)
		return FlushBuffer(buffer, TRUE);
	else
	{

		if (BufferIsLocal(buffer))
			return WriteLocalBuffer(buffer, TRUE);

		if (BAD_BUFFER_ID(buffer))
			return FALSE;

		bufHdr = &BufferDescriptors[buffer - 1];

		SharedBufferChanged = true;

		SpinAcquire(BufMgrLock);
		Assert(bufHdr->refcount > 0);
		bufHdr->flags |= (BM_DIRTY | BM_JUST_DIRTIED);
		UnpinBuffer(bufHdr);
		SpinRelease(BufMgrLock);
		CommitInfoNeedsSave[buffer - 1] = 0;
	}
	return TRUE;
}

#ifdef NOT_USED
void
WriteBuffer_Debug(char *file, int line, Buffer buffer)
{
	WriteBuffer(buffer);
	if (ShowPinTrace && BufferIsLocal(buffer) && is_userbuffer(buffer))
	{
		BufferDesc *buf;

		buf = &BufferDescriptors[buffer - 1];
		fprintf(stderr, "UNPIN(WR) %ld relname = %s, blockNum = %d, \
refcount = %ld, file: %s, line: %d\n",
				buffer, buf->sb_relname, buf->tag.blockNum,
				PrivateRefCount[buffer - 1], file, line);
	}
}

#endif

/*
 * DirtyBufferCopy() -- For a given dbid/relid/blockno, if the buffer is
 *						in the cache and is dirty, mark it clean and copy
 *						it to the requested location.  This is a logical
 *						write, and has been installed to support the cache
 *						management code for write-once storage managers.
 *
 *	DirtyBufferCopy() -- Copy a given dirty buffer to the requested
 *						 destination.
 *
 *		We treat this as a write.  If the requested buffer is in the pool
 *		and is dirty, we copy it to the location requested and mark it
 *		clean.	This routine supports the Sony jukebox storage manager,
 *		which agrees to take responsibility for the data once we mark
 *		it clean.
 *
 *	NOTE: used by sony jukebox code in postgres 4.2   - ay 2/95
 */
#ifdef NOT_USED
void
DirtyBufferCopy(Oid dbid, Oid relid, BlockNumber blkno, char *dest)
{
	BufferDesc *buf;
	BufferTag	btag;

	btag.relId.relId = relid;
	btag.relId.dbId = dbid;
	btag.blockNum = blkno;

	SpinAcquire(BufMgrLock);
	buf = BufTableLookup(&btag);

	if (buf == (BufferDesc *) NULL
		|| !(buf->flags & BM_DIRTY)
		|| !(buf->flags & BM_VALID))
	{
		SpinRelease(BufMgrLock);
		return;
	}

	/*
	 * hate to do this holding the lock, but release and reacquire is
	 * slower
	 */
	memmove(dest, (char *) MAKE_PTR(buf->data), BLCKSZ);

	buf->flags &= ~BM_DIRTY;

	SpinRelease(BufMgrLock);
}

#endif

/*
 * FlushBuffer -- like WriteBuffer, but force the page to disk.
 *
 * 'buffer' is known to be dirty/pinned, so there should not be a
 * problem reading the BufferDesc members without the BufMgrLock
 * (nobody should be able to change tags, flags, etc. out from under
 * us).
 */
static int
FlushBuffer(Buffer buffer, bool release)
{
	BufferDesc *bufHdr;
	Oid			bufdb;
	Relation	bufrel;
	int			status;

	if (BufferIsLocal(buffer))
		return FlushLocalBuffer(buffer, release) ? STATUS_OK : STATUS_ERROR;

	if (BAD_BUFFER_ID(buffer))
		return STATUS_ERROR;

	bufHdr = &BufferDescriptors[buffer - 1];
	bufdb = bufHdr->tag.relId.dbId;

	Assert(bufdb == MyDatabaseId || bufdb == (Oid) NULL);
	bufrel = RelationIdCacheGetRelation(bufHdr->tag.relId.relId);
	Assert(bufrel != (Relation) NULL);

	SharedBufferChanged = true;

	/* To check if block content changed while flushing. - vadim 01/17/97 */
	SpinAcquire(BufMgrLock);
	WaitIO(bufHdr, BufMgrLock);	/* confirm end of IO */
	bufHdr->flags &= ~BM_JUST_DIRTIED;
	StartBufferIO(bufHdr, false);	/* output IO start */
	SpinRelease(BufMgrLock);

	status = smgrflush(DEFAULT_SMGR, bufrel, bufHdr->tag.blockNum,
					   (char *) MAKE_PTR(bufHdr->data));

	/* drop relcache refcnt incremented by RelationIdCacheGetRelation */
	RelationDecrementReferenceCount(bufrel);

	if (status == SM_FAIL)
	{
		elog(ERROR, "FlushBuffer: cannot flush block %u of the relation %s",
			 bufHdr->tag.blockNum, bufHdr->sb_relname);
		return STATUS_ERROR;
	}
	BufferFlushCount++;

	SpinAcquire(BufMgrLock);
	bufHdr->flags &= ~BM_IO_IN_PROGRESS;	/* mark IO finished */
	TerminateBufferIO(bufHdr);		/* output IO finished */

	/*
	 * If this buffer was marked by someone as DIRTY while we were
	 * flushing it out we must not clear DIRTY flag - vadim 01/17/97
	 */
	if (bufHdr->flags & BM_JUST_DIRTIED)
	{
		elog(NOTICE, "FlushBuffer: content of block %u (%s) changed while flushing",
			 bufHdr->tag.blockNum, bufHdr->sb_relname);
	}
	else
		bufHdr->flags &= ~BM_DIRTY;
	if (release)
		UnpinBuffer(bufHdr);
	SpinRelease(BufMgrLock);
	CommitInfoNeedsSave[buffer - 1] = 0;

	return STATUS_OK;
}

/*
 * WriteNoReleaseBuffer -- like WriteBuffer, but do not unpin the buffer
 *						   when the operation is complete.
 *
 *		We know that the buffer is for a relation in our private cache,
 *		because this routine is called only to write out buffers that
 *		were changed by the executing backend.
 */
int
WriteNoReleaseBuffer(Buffer buffer)
{
	BufferDesc *bufHdr;

	if (WriteMode == BUFFER_FLUSH_WRITE)
		return FlushBuffer(buffer, FALSE);
	else
	{

		if (BufferIsLocal(buffer))
			return WriteLocalBuffer(buffer, FALSE);

		if (BAD_BUFFER_ID(buffer))
			return STATUS_ERROR;

		bufHdr = &BufferDescriptors[buffer - 1];

		SharedBufferChanged = true;

		SpinAcquire(BufMgrLock);
		bufHdr->flags |= (BM_DIRTY | BM_JUST_DIRTIED);
		SpinRelease(BufMgrLock);
		CommitInfoNeedsSave[buffer - 1] = 0;
	}
	return STATUS_OK;
}


#undef ReleaseAndReadBuffer
/*
 * ReleaseAndReadBuffer -- combine ReleaseBuffer() and ReadBuffer()
 *		so that only one semop needs to be called.
 *
 */
Buffer
ReleaseAndReadBuffer(Buffer buffer,
					 Relation relation,
					 BlockNumber blockNum)
{
	BufferDesc *bufHdr;
	Buffer		retbuf;

	if (BufferIsLocal(buffer))
	{
		Assert(LocalRefCount[-buffer - 1] > 0);
		LocalRefCount[-buffer - 1]--;
	}
	else
	{
		if (BufferIsValid(buffer))
		{
			bufHdr = &BufferDescriptors[buffer - 1];
			Assert(PrivateRefCount[buffer - 1] > 0);
			PrivateRefCount[buffer - 1]--;
			if (PrivateRefCount[buffer - 1] == 0)
			{
				SpinAcquire(BufMgrLock);
				Assert(bufHdr->refcount > 0);
				bufHdr->refcount--;
				if (bufHdr->refcount == 0)
				{
					AddBufferToFreelist(bufHdr);
					bufHdr->flags |= BM_FREE;
				}
				if (CommitInfoNeedsSave[buffer - 1])
				{
					bufHdr->flags |= (BM_DIRTY | BM_JUST_DIRTIED);
					CommitInfoNeedsSave[buffer - 1] = 0;
				}
				retbuf = ReadBufferWithBufferLock(relation, blockNum, true);
				return retbuf;
			}
		}
	}

	return ReadBuffer(relation, blockNum);
}

/*
 * BufferSync -- Flush all dirty buffers in the pool.
 *
 *		This is called at transaction commit time.	It does the wrong thing,
 *		right now.	We should flush only our own changes to stable storage,
 *		and we should obey the lock protocol on the buffer manager metadata
 *		as we do it.  Also, we need to be sure that no other transaction is
 *		modifying the page as we flush it.	This is only a problem for objects
 *		that use a non-two-phase locking protocol, like btree indices.	For
 *		those objects, we would like to set a write lock for the duration of
 *		our IO.  Another possibility is to code updates to btree pages
 *		carefully, so that writing them out out of order cannot cause
 *		any unrecoverable errors.
 *
 *		I don't want to think hard about this right now, so I will try
 *		to come back to it later.
 */
static void
BufferSync()
{
	int			i;
	Oid			bufdb;
	Oid			bufrel;
	Relation	reln;
	BufferDesc *bufHdr;
	int			status;

	SpinAcquire(BufMgrLock);
	for (i = 0, bufHdr = BufferDescriptors; i < NBuffers; i++, bufHdr++)
	{
		if ((bufHdr->flags & BM_VALID) && (bufHdr->flags & BM_DIRTY))
		{
			bufdb = bufHdr->tag.relId.dbId;
			bufrel = bufHdr->tag.relId.relId;
			if (bufdb == MyDatabaseId || bufdb == (Oid) 0)
			{
				reln = RelationIdCacheGetRelation(bufrel);

				/*
				 * We have to pin buffer to keep anyone from stealing it
				 * from the buffer pool while we are flushing it or
				 * waiting in WaitIO. It's bad for GetFreeBuffer in
				 * BufferAlloc, but there is no other way to prevent
				 * writing into disk block data from some other buffer,
				 * getting smgr status of some other block and clearing
				 * BM_DIRTY of ...			  - VAdim 09/16/96
				 */
				PinBuffer(bufHdr);
				if (bufHdr->flags & BM_IO_IN_PROGRESS)
				{
					WaitIO(bufHdr, BufMgrLock);
					UnpinBuffer(bufHdr);
					if (bufHdr->flags & BM_IO_ERROR)
					{
						elog(ERROR, "BufferSync: write error %u for %s",
							 bufHdr->tag.blockNum, bufHdr->sb_relname);
					}
					/* drop refcnt from RelationIdCacheGetRelation */
					if (reln != (Relation) NULL)
						RelationDecrementReferenceCount(reln);
					continue;
				}

				/*
				 * To check if block content changed while flushing (see
				 * below). - vadim 01/17/97
				 */
				WaitIO(bufHdr, BufMgrLock);	/* confirm end of IO */
				bufHdr->flags &= ~BM_JUST_DIRTIED;
				StartBufferIO(bufHdr, false);	/* output IO start */

				/*
				 * If we didn't have the reldesc in our local cache, flush
				 * this page out using the 'blind write' storage manager
				 * routine.  If we did find it, use the standard
				 * interface.
				 */

#ifndef OPTIMIZE_SINGLE
				SpinRelease(BufMgrLock);
#endif	 /* OPTIMIZE_SINGLE */
				if (reln == (Relation) NULL)
				{
					status = smgrblindwrt(DEFAULT_SMGR, bufHdr->sb_dbname,
									   bufHdr->sb_relname, bufdb, bufrel,
										  bufHdr->tag.blockNum,
										(char *) MAKE_PTR(bufHdr->data));
				}
				else
				{
					status = smgrwrite(DEFAULT_SMGR, reln,
									   bufHdr->tag.blockNum,
									   (char *) MAKE_PTR(bufHdr->data));
				}
#ifndef OPTIMIZE_SINGLE
				SpinAcquire(BufMgrLock);
#endif	 /* OPTIMIZE_SINGLE */

				UnpinBuffer(bufHdr);
				if (status == SM_FAIL)
				{
					bufHdr->flags |= BM_IO_ERROR;
					elog(ERROR, "BufferSync: cannot write %u for %s",
						 bufHdr->tag.blockNum, bufHdr->sb_relname);
				}
				bufHdr->flags &= ~BM_IO_IN_PROGRESS;	/* mark IO finished */
				TerminateBufferIO(bufHdr);	/* Sync IO finished */
				BufferFlushCount++;

				/*
				 * If this buffer was marked by someone as DIRTY while we
				 * were flushing it out we must not clear DIRTY flag -
				 * vadim 01/17/97
				 */
				if (!(bufHdr->flags & BM_JUST_DIRTIED))
					bufHdr->flags &= ~BM_DIRTY;
				/* drop refcnt from RelationIdCacheGetRelation */
				if (reln != (Relation) NULL)
					RelationDecrementReferenceCount(reln);
			}
		}
	}
	SpinRelease(BufMgrLock);

	LocalBufferSync();
}


/*
 * WaitIO -- Block until the IO_IN_PROGRESS flag on 'buf' is cleared.
 *
 * Should be entered with buffer manager spinlock held; releases it before
 * waiting and re-acquires it afterwards.
 *
 * OLD NOTES:
 *		Because IO_IN_PROGRESS conflicts are
 *		expected to be rare, there is only one BufferIO
 *		lock in the entire system.	All processes block
 *		on this semaphore when they try to use a buffer
 *		that someone else is faulting in.  Whenever a
 *		process finishes an IO and someone is waiting for
 *		the buffer, BufferIO is signaled (SignalIO).  All
 *		waiting processes then wake up and check to see
 *		if their buffer is now ready.  This implementation
 *		is simple, but efficient enough if WaitIO is
 *		rarely called by multiple processes simultaneously.
 *
 * NEW NOTES:
 *		The above is true only on machines without test-and-set
 *		semaphores (which we hope are few, these days).  On better
 *		hardware, each buffer has a spinlock that we can wait on.
 */
#ifdef HAS_TEST_AND_SET

static void
WaitIO(BufferDesc *buf, SPINLOCK spinlock)
{
	/*
	 * Changed to wait until there's no IO - Inoue 01/13/2000
	 */
	while ((buf->flags & BM_IO_IN_PROGRESS) != 0)
	{
		SpinRelease(spinlock);
		S_LOCK(&(buf->io_in_progress_lock));
		S_UNLOCK(&(buf->io_in_progress_lock));
		SpinAcquire(spinlock);
	}
}

#else							/* !HAS_TEST_AND_SET */

IpcSemaphoreId WaitIOSemId;
IpcSemaphoreId WaitCLSemId;

static void
WaitIO(BufferDesc *buf, SPINLOCK spinlock)
{
	bool		inProgress;

	for (;;)
	{

		/* wait until someone releases IO lock */
		(*NWaitIOBackendP)++;
		SpinRelease(spinlock);
		IpcSemaphoreLock(WaitIOSemId, 0, 1);
		SpinAcquire(spinlock);
		inProgress = (buf->flags & BM_IO_IN_PROGRESS);
		if (!inProgress)
			break;
	}
}

/*
 * SignalIO
 */
static void
SignalIO(BufferDesc *buf)
{
	/* somebody better be waiting. */
	Assert(buf->refcount > 1);
	IpcSemaphoreUnlock(WaitIOSemId, 0, *NWaitIOBackendP);
	*NWaitIOBackendP = 0;
}

#endif	 /* HAS_TEST_AND_SET */

long		NDirectFileRead;	/* some I/O's are direct file access.
								 * bypass bufmgr */
long		NDirectFileWrite;	/* e.g., I/O in psort and hashjoin.					*/

void
PrintBufferUsage(FILE *statfp)
{
	float		hitrate;
	float		localhitrate;

	if (ReadBufferCount == 0)
		hitrate = 0.0;
	else
		hitrate = (float) BufferHitCount *100.0 / ReadBufferCount;

	if (ReadLocalBufferCount == 0)
		localhitrate = 0.0;
	else
		localhitrate = (float) LocalBufferHitCount *100.0 / ReadLocalBufferCount;

	fprintf(statfp, "!\tShared blocks: %10ld read, %10ld written, buffer hit rate = %.2f%%\n",
			ReadBufferCount - BufferHitCount, BufferFlushCount, hitrate);
	fprintf(statfp, "!\tLocal  blocks: %10ld read, %10ld written, buffer hit rate = %.2f%%\n",
			ReadLocalBufferCount - LocalBufferHitCount, LocalBufferFlushCount, localhitrate);
	fprintf(statfp, "!\tDirect blocks: %10ld read, %10ld written\n",
			NDirectFileRead, NDirectFileWrite);
}

void
ResetBufferUsage()
{
	BufferHitCount = 0;
	ReadBufferCount = 0;
	BufferFlushCount = 0;
	LocalBufferHitCount = 0;
	ReadLocalBufferCount = 0;
	LocalBufferFlushCount = 0;
	NDirectFileRead = 0;
	NDirectFileWrite = 0;
}

/* ----------------------------------------------
 *		ResetBufferPool
 *
 *		this routine is supposed to be called when a transaction aborts.
 *		it will release all the buffer pins held by the transaction.
 *
 * ----------------------------------------------
 */
void
ResetBufferPool()
{
	int			i;

	for (i = 0; i < NBuffers; i++)
	{
		if (PrivateRefCount[i] != 0)
		{
			BufferDesc *buf = &BufferDescriptors[i];

			SpinAcquire(BufMgrLock);
			Assert(buf->refcount > 0);
			buf->refcount--;
			if (buf->refcount == 0)
			{
				AddBufferToFreelist(buf);
				buf->flags |= BM_FREE;
			}
			SpinRelease(BufMgrLock);
		}
		PrivateRefCount[i] = 0;
		CommitInfoNeedsSave[i] = 0;
	}

	ResetLocalBufferPool();
}

/* -----------------------------------------------
 *		BufferPoolCheckLeak
 *
 *		check if there is buffer leak
 *
 * -----------------------------------------------
 */
int
BufferPoolCheckLeak()
{
	int			i;
	int			result = 0;

	for (i = 1; i <= NBuffers; i++)
	{
		if (PrivateRefCount[i - 1] != 0)
		{
			BufferDesc *buf = &(BufferDescriptors[i - 1]);

			elog(NOTICE,
				 "Buffer Leak: [%03d] (freeNext=%ld, freePrev=%ld, \
relname=%s, blockNum=%d, flags=0x%x, refcount=%d %ld)",
				 i - 1, buf->freeNext, buf->freePrev,
				 buf->sb_relname, buf->tag.blockNum, buf->flags,
				 buf->refcount, PrivateRefCount[i - 1]);
			result = 1;
		}
	}
	return result;
}

/* ------------------------------------------------
 *		FlushBufferPool
 *
 *		flush all dirty blocks in buffer pool to disk
 *
 * ------------------------------------------------
 */
void
FlushBufferPool(void)
{
	BufferSync();
	smgrcommit();
}

/*
 * BufferGetBlockNumber
 *		Returns the block number associated with a buffer.
 *
 * Note:
 *		Assumes that the buffer is valid.
 */
BlockNumber
BufferGetBlockNumber(Buffer buffer)
{
	Assert(BufferIsValid(buffer));

	/* XXX should be a critical section */
	if (BufferIsLocal(buffer))
		return LocalBufferDescriptors[-buffer - 1].tag.blockNum;
	else
		return BufferDescriptors[buffer - 1].tag.blockNum;
}

#ifdef NOT_USED
/*
 * BufferGetRelation
 *		Returns the relation desciptor associated with a buffer.
 *
 * Note:
 *		Assumes buffer is valid.
 */
Relation
BufferGetRelation(Buffer buffer)
{
	Relation	relation;
	Oid			relid;

	Assert(BufferIsValid(buffer));
	Assert(!BufferIsLocal(buffer));		/* not supported for local buffers */

	/* XXX should be a critical section */
	relid = BufferDescriptors[buffer - 1].tag.relId.relId;
	relation = RelationIdGetRelation(relid);
	Assert(relation);

	/* drop relcache refcnt incremented by RelationIdGetRelation */
	RelationDecrementReferenceCount(relation);

	if (RelationHasReferenceCountZero(relation))
	{

		/*
		 * XXX why??
		 */

		RelationIncrementReferenceCount(relation);
	}

	return relation;
}

#endif

/*
 * BufferReplace
 *
 * Flush the buffer corresponding to 'bufHdr'
 *
 */
static int
BufferReplace(BufferDesc *bufHdr, bool bufferLockHeld)
{
	Relation	reln;
	Oid			bufdb,
				bufrel;
	int			status;

	if (!bufferLockHeld)
		SpinAcquire(BufMgrLock);

	/*
	 * first try to find the reldesc in the cache, if no luck, don't
	 * bother to build the reldesc from scratch, just do a blind write.
	 */

	bufdb = bufHdr->tag.relId.dbId;
	bufrel = bufHdr->tag.relId.relId;

	if (bufdb == MyDatabaseId || bufdb == (Oid) NULL)
		reln = RelationIdCacheGetRelation(bufrel);
	else
		reln = (Relation) NULL;

	/* To check if block content changed while flushing. - vadim 01/17/97 */
	bufHdr->flags &= ~BM_JUST_DIRTIED;

	SpinRelease(BufMgrLock);

	if (reln != (Relation) NULL)
	{
		status = smgrflush(DEFAULT_SMGR, reln, bufHdr->tag.blockNum,
						   (char *) MAKE_PTR(bufHdr->data));
	}
	else
	{
		/* blind write always flushes */
		status = smgrblindwrt(DEFAULT_SMGR, bufHdr->sb_dbname,
							  bufHdr->sb_relname, bufdb, bufrel,
							  bufHdr->tag.blockNum,
							  (char *) MAKE_PTR(bufHdr->data));
	}

	/* drop relcache refcnt incremented by RelationIdCacheGetRelation */
	if (reln != (Relation) NULL)
		RelationDecrementReferenceCount(reln);

	if (status == SM_FAIL)
		return FALSE;

	BufferFlushCount++;

	return TRUE;
}

/*
 * RelationGetNumberOfBlocks
 *		Returns the buffer descriptor associated with a page in a relation.
 *
 * Note:
 *		XXX may fail for huge relations.
 *		XXX should be elsewhere.
 *		XXX maybe should be hidden
 */
BlockNumber
RelationGetNumberOfBlocks(Relation relation)
{
	return ((relation->rd_myxactonly) ? relation->rd_nblocks :
			smgrnblocks(DEFAULT_SMGR, relation));
}

/* ---------------------------------------------------------------------
 *		ReleaseRelationBuffers
 *
 *		this function unmarks all the dirty pages of a relation
 *		in the buffer pool so that at the end of transaction
 *		these pages will not be flushed.  This is used when the
 *		relation is about to be deleted.  We assume that the caller
 *		holds an exclusive lock on the relation, which should assure
 *		that no new buffers will be acquired for the rel meanwhile.
 *
 *		XXX currently it sequentially searches the buffer pool, should be
 *		changed to more clever ways of searching.
 * --------------------------------------------------------------------
 */
void
ReleaseRelationBuffers(Relation rel)
{
	Oid			relid = RelationGetRelid(rel);
	bool		holding = false;
	int			i;
	BufferDesc *buf;

	if (rel->rd_myxactonly)
	{
		for (i = 0; i < NLocBuffer; i++)
		{
			buf = &LocalBufferDescriptors[i];
			if (buf->tag.relId.relId == relid)
				buf->flags &= ~ ( BM_DIRTY | BM_JUST_DIRTIED);
		}
		return;
	}

	for (i = 1; i <= NBuffers; i++)
	{
		buf = &BufferDescriptors[i - 1];
		if (!holding)
		{
			SpinAcquire(BufMgrLock);
			holding = true;
		}
	recheck:
		if (buf->tag.relId.dbId == MyDatabaseId &&
			buf->tag.relId.relId == relid)
		{
			/*
			 * If there is I/O in progress, better wait till it's done;
			 * don't want to delete the relation out from under someone
			 * who's just trying to flush the buffer!
			 */
			if (buf->flags & BM_IO_IN_PROGRESS)
			{
				WaitIO(buf, BufMgrLock);
				/* By now, the buffer very possibly belongs to some other
				 * rel, so check again before proceeding.
				 */
				goto recheck;
			}
			/* Now we can do what we came for */
			buf->flags &= ~ ( BM_DIRTY | BM_JUST_DIRTIED);
			CommitInfoNeedsSave[i - 1] = 0;
			/*
			 * Release any refcount we may have.
			 *
			 * This is very probably dead code, and if it isn't then it's
			 * probably wrong.  I added the Assert to find out --- tgl 11/99.
			 */
			if (!(buf->flags & BM_FREE))
			{
				/* Assert checks that buffer will actually get freed! */
				Assert(buf->refcount == 1);
				if (PrivateRefCount[i - 1] <= 0)
				{
					fprintf(stderr, "Nonpositive PrivateRefCount on buffer for %s\n",
							RelationGetRelationName(rel));
					fflush(stderr);
					* ((char *) 0) = 0;
					abort();
				}
				Assert(PrivateRefCount[i - 1] == 1);
				/* ReleaseBuffer expects we do not hold the lock at entry */
				SpinRelease(BufMgrLock);
				holding = false;
				ReleaseBuffer(i);
			}
		}
	}

	if (holding)
		SpinRelease(BufMgrLock);
}

/* ---------------------------------------------------------------------
 *		DropBuffers
 *
 *		This function marks all the buffers in the buffer cache for a
 *		particular database as clean.  This is used when we destroy a
 *		database, to avoid trying to flush data to disk when the directory
 *		tree no longer exists.  Implementation is pretty similar to
 *		ReleaseRelationBuffers() which is for destroying just one relation.
 * --------------------------------------------------------------------
 */
void
DropBuffers(Oid dbid)
{
	int			i;
	BufferDesc *buf;

	SpinAcquire(BufMgrLock);
	for (i = 1; i <= NBuffers; i++)
	{
		buf = &BufferDescriptors[i - 1];
	recheck:
		if (buf->tag.relId.dbId == dbid)
		{
			/*
			 * If there is I/O in progress, better wait till it's done;
			 * don't want to delete the database out from under someone
			 * who's just trying to flush the buffer!
			 */
			if (buf->flags & BM_IO_IN_PROGRESS)
			{
				WaitIO(buf, BufMgrLock);
				/* By now, the buffer very possibly belongs to some other
				 * DB, so check again before proceeding.
				 */
				goto recheck;
			}
			/* Now we can do what we came for */
			buf->flags &= ~ ( BM_DIRTY | BM_JUST_DIRTIED);
			/*
			 * The thing should be free, if caller has checked that
			 * no backends are running in that database.
			 */
			Assert(buf->flags & BM_FREE);
		}
	}
	SpinRelease(BufMgrLock);
}

/* -----------------------------------------------------------------
 *		PrintBufferDescs
 *
 *		this function prints all the buffer descriptors, for debugging
 *		use only.
 * -----------------------------------------------------------------
 */
void
PrintBufferDescs()
{
	int			i;
	BufferDesc *buf = BufferDescriptors;

	if (IsUnderPostmaster)
	{
		SpinAcquire(BufMgrLock);
		for (i = 0; i < NBuffers; ++i, ++buf)
		{
			elog(DEBUG, "[%02d] (freeNext=%ld, freePrev=%ld, relname=%s, \
blockNum=%d, flags=0x%x, refcount=%d %ld)",
				 i, buf->freeNext, buf->freePrev,
				 buf->sb_relname, buf->tag.blockNum, buf->flags,
				 buf->refcount, PrivateRefCount[i]);
		}
		SpinRelease(BufMgrLock);
	}
	else
	{
		/* interactive backend */
		for (i = 0; i < NBuffers; ++i, ++buf)
		{
			printf("[%-2d] (%s, %d) flags=0x%x, refcnt=%d %ld)\n",
				   i, buf->sb_relname, buf->tag.blockNum,
				   buf->flags, buf->refcount, PrivateRefCount[i]);
		}
	}
}

void
PrintPinnedBufs()
{
	int			i;
	BufferDesc *buf = BufferDescriptors;

	SpinAcquire(BufMgrLock);
	for (i = 0; i < NBuffers; ++i, ++buf)
	{
		if (PrivateRefCount[i] > 0)
			elog(NOTICE, "[%02d] (freeNext=%ld, freePrev=%ld, relname=%s, \
blockNum=%d, flags=0x%x, refcount=%d %ld)\n",
				 i, buf->freeNext, buf->freePrev, buf->sb_relname,
				 buf->tag.blockNum, buf->flags,
				 buf->refcount, PrivateRefCount[i]);
	}
	SpinRelease(BufMgrLock);
}

/*
 * BufferPoolBlowaway
 *
 * this routine is solely for the purpose of experiments -- sometimes
 * you may want to blowaway whatever is left from the past in buffer
 * pool and start measuring some performance with a clean empty buffer
 * pool.
 */
#ifdef NOT_USED
void
BufferPoolBlowaway()
{
	int			i;

	BufferSync();
	for (i = 1; i <= NBuffers; i++)
	{
		if (BufferIsValid(i))
		{
			while (BufferIsValid(i))
				ReleaseBuffer(i);
		}
		BufTableDelete(&BufferDescriptors[i - 1]);
	}
}

#endif

/* ---------------------------------------------------------------------
 *		FlushRelationBuffers
 *
 *		This function removes from the buffer pool all pages of a relation
 *		that have blocknumber >= specified block.  If doFlush is true,
 *		dirty buffers are written out --- otherwise it's an error for any
 *		of the buffers to be dirty.
 *
 *		This is used by VACUUM before truncating the relation to the given
 *		number of blocks.  For VACUUM, we pass doFlush = false since it would
 *		mean a bug in VACUUM if any of the unwanted pages were still dirty.
 *		(TRUNCATE TABLE also uses it in the same way.)
 *
 *		This is also used by RENAME TABLE (with block = 0 and doFlush = true)
 *		to clear out the buffer cache before renaming the physical files of
 *		a relation.  Without that, some other backend might try to do a
 *		blind write of a buffer page (relying on the sb_relname of the buffer)
 *		and fail because it's not got the right filename anymore.
 *
 *		In both cases, the caller should be holding AccessExclusiveLock on
 *		the target relation to ensure that no other backend is busy reading
 *		more blocks of the relation...
 *
 *		Returns: 0 - Ok, -1 - DIRTY, -2 - PINNED
 *
 *		XXX currently it sequentially searches the buffer pool, should be
 *		changed to more clever ways of searching.
 * --------------------------------------------------------------------
 */
int
FlushRelationBuffers(Relation rel, BlockNumber block, bool doFlush)
{
	int			i;
	BufferDesc *buf;

	if (rel->rd_myxactonly)
	{
		for (i = 0; i < NLocBuffer; i++)
		{
			buf = &LocalBufferDescriptors[i];
			if (buf->tag.relId.relId == RelationGetRelid(rel) &&
				buf->tag.blockNum >= block)
			{
				if (buf->flags & BM_DIRTY)
				{
					if (doFlush)
					{
						if (FlushBuffer(-i-1, false) != STATUS_OK)
						{
							elog(NOTICE, "FlushRelationBuffers(%s (local), %u): block %u is dirty, could not flush it",
								 RelationGetRelationName(rel),
								 block, buf->tag.blockNum);
							return -1;
						}
					}
					else
					{
						elog(NOTICE, "FlushRelationBuffers(%s (local), %u): block %u is dirty",
							 RelationGetRelationName(rel),
							 block, buf->tag.blockNum);
						return -1;
					}
				}
				if (LocalRefCount[i] > 0)
				{
					elog(NOTICE, "FlushRelationBuffers(%s (local), %u): block %u is referenced (%ld)",
						 RelationGetRelationName(rel), block,
						 buf->tag.blockNum, LocalRefCount[i]);
					return -2;
				}
				buf->tag.relId.relId = InvalidOid;
			}
		}
		return 0;
	}

	SpinAcquire(BufMgrLock);
	for (i = 0; i < NBuffers; i++)
	{
		buf = &BufferDescriptors[i];
		if (buf->tag.relId.dbId == MyDatabaseId &&
			buf->tag.relId.relId == RelationGetRelid(rel) &&
			buf->tag.blockNum >= block)
		{
			if (buf->flags & BM_DIRTY)
			{
				if (doFlush)
				{
					SpinRelease(BufMgrLock);
					if (FlushBuffer(i+1, false) != STATUS_OK)
					{
						elog(NOTICE, "FlushRelationBuffers(%s, %u): block %u is dirty (private %ld, global %d), could not flush it",
							 buf->sb_relname, block, buf->tag.blockNum,
							 PrivateRefCount[i], buf->refcount);
						return -1;
					}
					SpinAcquire(BufMgrLock);
				}
				else
				{
					SpinRelease(BufMgrLock);
					elog(NOTICE, "FlushRelationBuffers(%s, %u): block %u is dirty (private %ld, global %d)",
						 buf->sb_relname, block, buf->tag.blockNum,
						 PrivateRefCount[i], buf->refcount);
					return -1;
				}
			}
			if (!(buf->flags & BM_FREE))
			{
				SpinRelease(BufMgrLock);
				elog(NOTICE, "FlushRelationBuffers(%s, %u): block %u is referenced (private %ld, global %d)",
					 buf->sb_relname, block, buf->tag.blockNum,
					 PrivateRefCount[i], buf->refcount);
				return -2;
			}
			BufTableDelete(buf);
		}
	}
	SpinRelease(BufMgrLock);
	return 0;
}

#undef ReleaseBuffer

/*
 * ReleaseBuffer -- remove the pin on a buffer without
 *		marking it dirty.
 *
 */
int
ReleaseBuffer(Buffer buffer)
{
	BufferDesc *bufHdr;

	if (BufferIsLocal(buffer))
	{
		Assert(LocalRefCount[-buffer - 1] > 0);
		LocalRefCount[-buffer - 1]--;
		return STATUS_OK;
	}

	if (BAD_BUFFER_ID(buffer))
		return STATUS_ERROR;

	bufHdr = &BufferDescriptors[buffer - 1];

	Assert(PrivateRefCount[buffer - 1] > 0);
	PrivateRefCount[buffer - 1]--;
	if (PrivateRefCount[buffer - 1] == 0)
	{
		SpinAcquire(BufMgrLock);
		Assert(bufHdr->refcount > 0);
		bufHdr->refcount--;
		if (bufHdr->refcount == 0)
		{
			AddBufferToFreelist(bufHdr);
			bufHdr->flags |= BM_FREE;
		}
		if (CommitInfoNeedsSave[buffer - 1])
		{
			bufHdr->flags |= (BM_DIRTY | BM_JUST_DIRTIED);
			CommitInfoNeedsSave[buffer - 1] = 0;
		}
		SpinRelease(BufMgrLock);
	}

	return STATUS_OK;
}

#ifdef NOT_USED
void
IncrBufferRefCount_Debug(char *file, int line, Buffer buffer)
{
	IncrBufferRefCount(buffer);
	if (ShowPinTrace && !BufferIsLocal(buffer) && is_userbuffer(buffer))
	{
		BufferDesc *buf = &BufferDescriptors[buffer - 1];

		fprintf(stderr, "PIN(Incr) %ld relname = %s, blockNum = %d, \
refcount = %ld, file: %s, line: %d\n",
				buffer, buf->sb_relname, buf->tag.blockNum,
				PrivateRefCount[buffer - 1], file, line);
	}
}

#endif

#ifdef NOT_USED
void
ReleaseBuffer_Debug(char *file, int line, Buffer buffer)
{
	ReleaseBuffer(buffer);
	if (ShowPinTrace && !BufferIsLocal(buffer) && is_userbuffer(buffer))
	{
		BufferDesc *buf = &BufferDescriptors[buffer - 1];

		fprintf(stderr, "UNPIN(Rel) %ld relname = %s, blockNum = %d, \
refcount = %ld, file: %s, line: %d\n",
				buffer, buf->sb_relname, buf->tag.blockNum,
				PrivateRefCount[buffer - 1], file, line);
	}
}

#endif

#ifdef NOT_USED
int
ReleaseAndReadBuffer_Debug(char *file,
						   int line,
						   Buffer buffer,
						   Relation relation,
						   BlockNumber blockNum)
{
	bool		bufferValid;
	Buffer		b;

	bufferValid = BufferIsValid(buffer);
	b = ReleaseAndReadBuffer(buffer, relation, blockNum);
	if (ShowPinTrace && bufferValid && BufferIsLocal(buffer)
		&& is_userbuffer(buffer))
	{
		BufferDesc *buf = &BufferDescriptors[buffer - 1];

		fprintf(stderr, "UNPIN(Rel&Rd) %ld relname = %s, blockNum = %d, \
refcount = %ld, file: %s, line: %d\n",
				buffer, buf->sb_relname, buf->tag.blockNum,
				PrivateRefCount[buffer - 1], file, line);
	}
	if (ShowPinTrace && BufferIsLocal(buffer) && is_userbuffer(buffer))
	{
		BufferDesc *buf = &BufferDescriptors[b - 1];

		fprintf(stderr, "PIN(Rel&Rd) %ld relname = %s, blockNum = %d, \
refcount = %ld, file: %s, line: %d\n",
				b, buf->sb_relname, buf->tag.blockNum,
				PrivateRefCount[b - 1], file, line);
	}
	return b;
}

#endif

#ifdef BMTRACE

/*
 *	trace allocations and deallocations in a circular buffer in
 *	shared memory.	check the buffer before doing the allocation,
 *	and die if there's anything fishy.
 */

_bm_trace(Oid dbId, Oid relId, int blkNo, int bufNo, int allocType)
{
	long		start,
				cur;
	bmtrace    *tb;

	start = *CurTraceBuf;

	if (start > 0)
		cur = start - 1;
	else
		cur = BMT_LIMIT - 1;

	for (;;)
	{
		tb = &TraceBuf[cur];
		if (tb->bmt_op != BMT_NOTUSED)
		{
			if (tb->bmt_buf == bufNo)
			{
				if ((tb->bmt_op == BMT_DEALLOC)
					|| (tb->bmt_dbid == dbId && tb->bmt_relid == relId
						&& tb->bmt_blkno == blkNo))
					goto okay;

				/* die holding the buffer lock */
				_bm_die(dbId, relId, blkNo, bufNo, allocType, start, cur);
			}
		}

		if (cur == start)
			goto okay;

		if (cur == 0)
			cur = BMT_LIMIT - 1;
		else
			cur--;
	}

okay:
	tb = &TraceBuf[start];
	tb->bmt_pid = MyProcPid;
	tb->bmt_buf = bufNo;
	tb->bmt_dbid = dbId;
	tb->bmt_relid = relId;
	tb->bmt_blkno = blkNo;
	tb->bmt_op = allocType;

	*CurTraceBuf = (start + 1) % BMT_LIMIT;
}

_bm_die(Oid dbId, Oid relId, int blkNo, int bufNo,
		int allocType, long start, long cur)
{
	FILE	   *fp;
	bmtrace    *tb;
	int			i;

	tb = &TraceBuf[cur];

	if ((fp = AllocateFile("/tmp/death_notice", "w")) == NULL)
		elog(FATAL, "buffer alloc trace error and can't open log file");

	fprintf(fp, "buffer alloc trace detected the following error:\n\n");
	fprintf(fp, "    buffer %d being %s inconsistently with a previous %s\n\n",
		 bufNo, (allocType == BMT_DEALLOC ? "deallocated" : "allocated"),
			(tb->bmt_op == BMT_DEALLOC ? "deallocation" : "allocation"));

	fprintf(fp, "the trace buffer contains:\n");

	i = start;
	for (;;)
	{
		tb = &TraceBuf[i];
		if (tb->bmt_op != BMT_NOTUSED)
		{
			fprintf(fp, "     [%3d]%spid %d buf %2d for <%d,%u,%d> ",
					i, (i == cur ? " ---> " : "\t"),
					tb->bmt_pid, tb->bmt_buf,
					tb->bmt_dbid, tb->bmt_relid, tb->bmt_blkno);

			switch (tb->bmt_op)
			{
				case BMT_ALLOCFND:
					fprintf(fp, "allocate (found)\n");
					break;

				case BMT_ALLOCNOTFND:
					fprintf(fp, "allocate (not found)\n");
					break;

				case BMT_DEALLOC:
					fprintf(fp, "deallocate\n");
					break;

				default:
					fprintf(fp, "unknown op type %d\n", tb->bmt_op);
					break;
			}
		}

		i = (i + 1) % BMT_LIMIT;
		if (i == start)
			break;
	}

	fprintf(fp, "\noperation causing error:\n");
	fprintf(fp, "\tpid %d buf %d for <%d,%u,%d> ",
			getpid(), bufNo, dbId, relId, blkNo);

	switch (allocType)
	{
		case BMT_ALLOCFND:
			fprintf(fp, "allocate (found)\n");
			break;

		case BMT_ALLOCNOTFND:
			fprintf(fp, "allocate (not found)\n");
			break;

		case BMT_DEALLOC:
			fprintf(fp, "deallocate\n");
			break;

		default:
			fprintf(fp, "unknown op type %d\n", allocType);
			break;
	}

	FreeFile(fp);

	kill(getpid(), SIGILL);
}

#endif	 /* BMTRACE */

int
SetBufferWriteMode(int mode)
{
	int			old;

	old = WriteMode;
	WriteMode = mode;
	return old;
}

void
SetBufferCommitInfoNeedsSave(Buffer buffer)
{
	if (!BufferIsLocal(buffer))
		CommitInfoNeedsSave[buffer - 1]++;
}

void
UnlockBuffers()
{
	BufferDesc *buf;
	int			i;

	for (i = 0; i < NBuffers; i++)
	{
		if (BufferLocks[i] == 0)
			continue;

		Assert(BufferIsValid(i + 1));
		buf = &(BufferDescriptors[i]);

#ifdef HAS_TEST_AND_SET
		S_LOCK(&(buf->cntx_lock));
#else
		IpcSemaphoreLock(WaitCLSemId, 0, IpcExclusiveLock);
#endif

		if (BufferLocks[i] & BL_R_LOCK)
		{
			Assert(buf->r_locks > 0);
			(buf->r_locks)--;
		}
		if (BufferLocks[i] & BL_RI_LOCK)
		{
			/* 
			 * Someone else could remove our RI lock when acquiring
			 * W lock. This is possible if we came here from elog(ERROR)
			 * from IpcSemaphore{Lock|Unlock}(WaitCLSemId). And so we
			 * don't do Assert(buf->ri_lock) here.
			 */
			buf->ri_lock = false;
		}
		if (BufferLocks[i] & BL_W_LOCK)
		{
			Assert(buf->w_lock);
			buf->w_lock = false;
		}
#ifdef HAS_TEST_AND_SET
		S_UNLOCK(&(buf->cntx_lock));
#else
		IpcSemaphoreUnlock(WaitCLSemId, 0, IpcExclusiveLock);
#endif
		BufferLocks[i] = 0;
	}
}

void
LockBuffer(Buffer buffer, int mode)
{
	BufferDesc *buf;

	Assert(BufferIsValid(buffer));
	if (BufferIsLocal(buffer))
		return;

	buf = &(BufferDescriptors[buffer - 1]);

#ifdef HAS_TEST_AND_SET
	S_LOCK(&(buf->cntx_lock));
#else
	IpcSemaphoreLock(WaitCLSemId, 0, IpcExclusiveLock);
#endif

	if (mode == BUFFER_LOCK_UNLOCK)
	{
		if (BufferLocks[buffer - 1] & BL_R_LOCK)
		{
			Assert(buf->r_locks > 0);
			Assert(!(buf->w_lock));
			Assert(!(BufferLocks[buffer - 1] & (BL_W_LOCK | BL_RI_LOCK)))
				(buf->r_locks)--;
			BufferLocks[buffer - 1] &= ~BL_R_LOCK;
		}
		else if (BufferLocks[buffer - 1] & BL_W_LOCK)
		{
			Assert(buf->w_lock);
			Assert(buf->r_locks == 0);
			Assert(!(BufferLocks[buffer - 1] & (BL_R_LOCK | BL_RI_LOCK)))
				buf->w_lock = false;
			BufferLocks[buffer - 1] &= ~BL_W_LOCK;
		}
		else
			elog(ERROR, "UNLockBuffer: buffer %lu is not locked", buffer);
	}
	else if (mode == BUFFER_LOCK_SHARE)
	{
		unsigned	i = 0;

		Assert(!(BufferLocks[buffer - 1] & (BL_R_LOCK | BL_W_LOCK | BL_RI_LOCK)));
		while (buf->ri_lock || buf->w_lock)
		{
#ifdef HAS_TEST_AND_SET
			S_UNLOCK(&(buf->cntx_lock));
			s_lock_sleep(i++);
			S_LOCK(&(buf->cntx_lock));
#else
			IpcSemaphoreUnlock(WaitCLSemId, 0, IpcExclusiveLock);
			s_lock_sleep(i++);
			IpcSemaphoreLock(WaitCLSemId, 0, IpcExclusiveLock);
#endif
		}
		(buf->r_locks)++;
		BufferLocks[buffer - 1] |= BL_R_LOCK;
	}
	else if (mode == BUFFER_LOCK_EXCLUSIVE)
	{
		unsigned	i = 0;

		Assert(!(BufferLocks[buffer - 1] & (BL_R_LOCK | BL_W_LOCK | BL_RI_LOCK)));
		while (buf->r_locks > 0 || buf->w_lock)
		{
			if (buf->r_locks > 3 || (BufferLocks[buffer - 1] & BL_RI_LOCK))
			{
				/*
				 * Our RI lock might be removed by concurrent W lock
				 * acquiring (see what we do with RI locks below
				 * when our own W acquiring succeeded) and so
				 * we set RI lock again if we already did this.
				 */
				BufferLocks[buffer - 1] |= BL_RI_LOCK;
				buf->ri_lock = true;
			}
#ifdef HAS_TEST_AND_SET
			S_UNLOCK(&(buf->cntx_lock));
			s_lock_sleep(i++);
			S_LOCK(&(buf->cntx_lock));
#else
			IpcSemaphoreUnlock(WaitCLSemId, 0, IpcExclusiveLock);
			s_lock_sleep(i++);
			IpcSemaphoreLock(WaitCLSemId, 0, IpcExclusiveLock);
#endif
		}
		buf->w_lock = true;
		BufferLocks[buffer - 1] |= BL_W_LOCK;
		if (BufferLocks[buffer - 1] & BL_RI_LOCK)
		{
			/*
			 * It's possible to remove RI locks acquired by another
			 * W lockers here, but they'll take care about it.
			 */
			buf->ri_lock = false;
			BufferLocks[buffer - 1] &= ~BL_RI_LOCK;
		}
	}
	else
		elog(ERROR, "LockBuffer: unknown lock mode %d", mode);

#ifdef HAS_TEST_AND_SET
	S_UNLOCK(&(buf->cntx_lock));
#else
	IpcSemaphoreUnlock(WaitCLSemId, 0, IpcExclusiveLock);
#endif

}

/*
 *	Functions for IO error handling
 *
 *	Note : We assume that nested buffer IO never occur.
 *	i.e at most one io_in_progress spinlock is held
 *	per proc.
*/
static	BufferDesc *InProgressBuf = (BufferDesc *)NULL;
static	bool	IsForInput;

/*
 * Function:StartBufferIO
 *	(Assumptions)
 *	My process is executing no IO
 *	BufMgrLock is held
 *	BM_IO_IN_PROGRESS mask is not set for the buffer 
 *	The buffer is Pinned
 *
*/
static void StartBufferIO(BufferDesc *buf, bool forInput)
{
	Assert(!InProgressBuf);
	Assert(!(buf->flags & BM_IO_IN_PROGRESS));
	buf->flags |= BM_IO_IN_PROGRESS;
#ifdef	HAS_TEST_AND_SET
	Assert(S_LOCK_FREE(&(buf->io_in_progress_lock)))
	S_LOCK(&(buf->io_in_progress_lock));
#endif /* HAS_TEST_AND_SET */
	InProgressBuf = buf;
	IsForInput = forInput;
}

/*
 * Function:TerminateBufferIO
 *	(Assumptions)
 *	My process is executing IO for the buffer
 *	BufMgrLock is held
 *	The buffer is Pinned
 *
*/
static void TerminateBufferIO(BufferDesc *buf)
{
	Assert(buf == InProgressBuf);
#ifdef	HAS_TEST_AND_SET
	S_UNLOCK(&(buf->io_in_progress_lock));
#else
	if (buf->refcount > 1)
		SignalIO(buf);
#endif /* HAS_TEST_AND_SET */
	InProgressBuf = (BufferDesc *)0;
}

/*
 * Function:ContinueBufferIO
 *	(Assumptions)
 *	My process is executing IO for the buffer
 *	BufMgrLock is held
 *	The buffer is Pinned
 *
*/
static void ContinueBufferIO(BufferDesc *buf, bool forInput)
{
	Assert(buf == InProgressBuf);
	Assert(buf->flags & BM_IO_IN_PROGRESS);
	IsForInput = forInput;
}

extern void	InitBufferIO(void)
{
	InProgressBuf = (BufferDesc *)0;
}

/*
 *	This function is called from ProcReleaseSpins().
 *	BufMgrLock isn't held when this function is called. 
 *	BM_IO_ERROR is always set. If BM_IO_ERROR was already
 *	set in case of output,this routine would kill all 
 *	backends and reset postmaster.
 */
extern void	AbortBufferIO(void)
{
	BufferDesc *buf = InProgressBuf;
	if (buf)
	{
		Assert(buf->flags & BM_IO_IN_PROGRESS);
		SpinAcquire(BufMgrLock);
		if (IsForInput)
		{
			Assert(!(buf->flags & BM_DIRTY));
		}
		else
		{
			Assert((buf->flags & BM_DIRTY) != 0);
			if (buf->flags & BM_IO_ERROR)
			{
				elog(NOTICE, "!!! write error seems permanent !!!");
				elog(NOTICE, "!!! now kill all backends and reset postmaster !!!");
				proc_exit(255);
			}
			buf->flags |= BM_DIRTY;
		}
		buf->flags |= BM_IO_ERROR;
		TerminateBufferIO(buf);
		buf->flags &= ~BM_IO_IN_PROGRESS;
		SpinRelease(BufMgrLock);
	}
}
