/*-------------------------------------------------------------------------
 *
 * inv_api.c
 *	  routines for manipulating inversion fs large objects. This file
 *	  contains the user-level large object application interface routines.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/storage/large_object/inv_api.c,v 1.64 1999-12-30 05:05:03 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/nbtree.h"
#include "catalog/catalog.h"
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_type.h"
#include "libpq/libpq-fs.h"
#include "miscadmin.h"
#include "storage/large_object.h"
#include "storage/smgr.h"
#include "utils/relcache.h"

/*
 *	Warning, Will Robinson...  In order to pack data into an inversion
 *	file as densely as possible, we violate the class abstraction here.
 *	When we're appending a new tuple to the end of the table, we check
 *	the last page to see how much data we can put on it.  If it's more
 *	than IMINBLK, we write enough to fill the page.  This limits external
 *	fragmentation.	In no case can we write more than IMAXBLK, since
 *	the 8K postgres page size less overhead leaves only this much space
 *	for data.
 */

/*
 *		In order to prevent buffer leak on transaction commit, large object
 *		scan index handling has been modified. Indexes are persistant inside
 *		a transaction but may be closed between two calls to this API (when
 *		transaction is committed while object is opened, or when no
 *		transaction is active). Scan indexes are thus now reinitialized using
 *		the object current offset. [PA]
 *
 *		Some cleanup has been also done for non freed memory.
 *
 *		For subsequent notes, [PA] is Pascal Andr� <andre@via.ecp.fr>
 */

#define IFREESPC(p)		(PageGetFreeSpace(p) - \
				 MAXALIGN(offsetof(HeapTupleHeaderData,t_bits)) - \
				 MAXALIGN(sizeof(struct varlena) + sizeof(int32)) - \
				 sizeof(double))
#define IMAXBLK			8092
#define IMINBLK			512

/* non-export function prototypes */
static HeapTuple inv_newtuple(LargeObjectDesc *obj_desc, Buffer buffer,
			 Page page, char *dbuf, int nwrite);
static void inv_fetchtup(LargeObjectDesc *obj_desc, HeapTuple tuple, Buffer *buffer);
static int	inv_wrnew(LargeObjectDesc *obj_desc, char *buf, int nbytes);
static int inv_wrold(LargeObjectDesc *obj_desc, char *dbuf, int nbytes,
		  HeapTuple tuple, Buffer buffer);
static void inv_indextup(LargeObjectDesc *obj_desc, HeapTuple tuple);
static int	_inv_getsize(Relation hreln, TupleDesc hdesc, Relation ireln);

/*
 *	inv_create -- create a new large object.
 *
 *		Arguments:
 *		  flags -- was archive, smgr
 *
 *		Returns:
 *		  large object descriptor, appropriately filled in.
 */
LargeObjectDesc *
inv_create(int flags)
{
	int			file_oid;
	LargeObjectDesc *retval;
	Relation	r;
	Relation	indr;
	TupleDesc	tupdesc;
	AttrNumber	attNums[1];
	Oid			classObjectId[1];
	char		objname[NAMEDATALEN];
	char		indname[NAMEDATALEN];

	/*
	 * add one here since the pg_class tuple created will have the next
	 * oid and we want to have the relation name to correspond to the
	 * tuple OID
	 */
	file_oid = newoid() + 1;

	/* come up with some table names */
	sprintf(objname, "xinv%u", file_oid);
	sprintf(indname, "xinx%u", file_oid);

	if (RelnameFindRelid(objname) != InvalidOid)
	{
		elog(ERROR,
		  "internal error: %s already exists -- cannot create large obj",
			 objname);
	}
	if (RelnameFindRelid(indname) != InvalidOid)
	{
		elog(ERROR,
		  "internal error: %s already exists -- cannot create large obj",
			 indname);
	}

	/* this is pretty painful...  want a tuple descriptor */
	tupdesc = CreateTemplateTupleDesc(2);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1,
					   "olastbye",
					   INT4OID,
					   -1, 0, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2,
					   "odata",
					   BYTEAOID,
					   -1, 0, false);

	/*
	 * First create the table to hold the inversion large object.  It will
	 * be located on whatever storage manager the user requested.
	 */

	heap_create_with_catalog(objname, tupdesc, RELKIND_LOBJECT, false);

	/* make the relation visible in this transaction */
	CommandCounterIncrement();

	/*--------------------
	 * We hold AccessShareLock on any large object we have open
	 * by inv_create or inv_open; it is released by inv_close.
	 * Note this will not conflict with ExclusiveLock or ShareLock
	 * that we acquire when actually reading/writing; it just prevents
	 * deletion of the large object while we have it open.
	 *--------------------
	 */
	r = heap_openr(objname, AccessShareLock);

	/*
	 * Now create a btree index on the relation's olastbyte attribute to
	 * make seeks go faster.  The hardwired constants are embarassing to
	 * me, and are symptomatic of the pressure under which this code was
	 * written.
	 *
	 * ok, mao, let's put in some symbolic constants - jolly
	 */

	attNums[0] = 1;
	classObjectId[0] = INT4_OPS_OID;
	index_create(objname, indname, NULL, NULL, BTREE_AM_OID,
				 1, &attNums[0], &classObjectId[0],
				 0, (Datum) NULL, NULL, FALSE, FALSE, FALSE);

	/* make the index visible in this transaction */
	CommandCounterIncrement();
	indr = index_openr(indname);

	if (!RelationIsValid(indr))
	{
		elog(ERROR, "cannot create index for large obj on %s under inversion",
			 smgrout(DEFAULT_SMGR));
	}

	retval = (LargeObjectDesc *) palloc(sizeof(LargeObjectDesc));

	retval->heap_r = r;
	retval->index_r = indr;
	retval->iscan = (IndexScanDesc) NULL;
	retval->hdesc = RelationGetDescr(r);
	retval->idesc = RelationGetDescr(indr);
	retval->offset = retval->lowbyte = retval->highbyte = 0;
	ItemPointerSetInvalid(&(retval->htid));

	if (flags & INV_WRITE)
	{
		LockRelation(r, ExclusiveLock);
		retval->flags = IFS_WRLOCK | IFS_RDLOCK;
	}
	else if (flags & INV_READ)
	{
		LockRelation(r, ShareLock);
		retval->flags = IFS_RDLOCK;
	}
	retval->flags |= IFS_ATEOF;

	return retval;
}

LargeObjectDesc *
inv_open(Oid lobjId, int flags)
{
	LargeObjectDesc *retval;
	Relation	r;
	char	   *indname;
	Relation	indrel;

	r = heap_open(lobjId, AccessShareLock);

	indname = pstrdup(RelationGetRelationName(r));

	/*
	 * hack hack hack...  we know that the fourth character of the
	 * relation name is a 'v', and that the fourth character of the index
	 * name is an 'x', and that they're otherwise identical.
	 */
	indname[3] = 'x';
	indrel = index_openr(indname);

	if (!RelationIsValid(indrel))
		return (LargeObjectDesc *) NULL;

	retval = (LargeObjectDesc *) palloc(sizeof(LargeObjectDesc));

	retval->heap_r = r;
	retval->index_r = indrel;
	retval->iscan = (IndexScanDesc) NULL;
	retval->hdesc = RelationGetDescr(r);
	retval->idesc = RelationGetDescr(indrel);
	retval->offset = retval->lowbyte = retval->highbyte = 0;
	ItemPointerSetInvalid(&(retval->htid));

	if (flags & INV_WRITE)
	{
		LockRelation(r, ExclusiveLock);
		retval->flags = IFS_WRLOCK | IFS_RDLOCK;
	}
	else if (flags & INV_READ)
	{
		LockRelation(r, ShareLock);
		retval->flags = IFS_RDLOCK;
	}

	return retval;
}

/*
 * Closes an existing large object descriptor.
 */
void
inv_close(LargeObjectDesc *obj_desc)
{
	Assert(PointerIsValid(obj_desc));

	if (obj_desc->iscan != (IndexScanDesc) NULL)
	{
		index_endscan(obj_desc->iscan);
		obj_desc->iscan = NULL;
	}

	index_close(obj_desc->index_r);
	heap_close(obj_desc->heap_r, AccessShareLock);

	pfree(obj_desc);
}

/*
 * Destroys an existing large object, and frees its associated pointers.
 *
 * returns -1 if failed
 */
int
inv_drop(Oid lobjId)
{
	Relation	r;

	r = (Relation) RelationIdGetRelation(lobjId);
	if (!RelationIsValid(r) || r->rd_rel->relkind != RELKIND_LOBJECT)
		return -1;

	heap_drop_with_catalog(RelationGetRelationName(r));
	return 1;
}

/*
 *	inv_stat() -- do a stat on an inversion file.
 *
 *		For the time being, this is an insanely expensive operation.  In
 *		order to find the size of the file, we seek to the last block in
 *		it and compute the size from that.	We scan pg_class to determine
 *		the file's owner and create time.  We don't maintain mod time or
 *		access time, yet.
 *
 *		These fields aren't stored in a table anywhere because they're
 *		updated so frequently, and postgres only appends tuples at the
 *		end of relations.  Once clustering works, we should fix this.
 */
#ifdef NOT_USED

struct pgstat
{								/* just the fields we need from stat
								 * structure */
	int			st_ino;
	int			st_mode;
	unsigned int st_size;
	unsigned int st_sizehigh;	/* high order bits */
/* 2^64 == 1.8 x 10^20 bytes */
	int			st_uid;
	int			st_atime_s;		/* just the seconds */
	int			st_mtime_s;		/* since SysV and the new BSD both have */
	int			st_ctime_s;		/* usec fields.. */
};

int
inv_stat(LargeObjectDesc *obj_desc, struct pgstat * stbuf)
{
	Assert(PointerIsValid(obj_desc));
	Assert(stbuf != NULL);

	/* need read lock for stat */
	if (!(obj_desc->flags & IFS_RDLOCK))
	{
		LockRelation(obj_desc->heap_r, ShareLock);
		obj_desc->flags |= IFS_RDLOCK;
	}

	stbuf->st_ino = RelationGetRelid(obj_desc->heap_r);
#if 1
	stbuf->st_mode = (S_IFREG | 0666);	/* IFREG|rw-rw-rw- */
#else
	stbuf->st_mode = 100666;	/* IFREG|rw-rw-rw- */
#endif
	stbuf->st_size = _inv_getsize(obj_desc->heap_r,
								  obj_desc->hdesc,
								  obj_desc->index_r);

	stbuf->st_uid = obj_desc->heap_r->rd_rel->relowner;

	/* we have no good way of computing access times right now */
	stbuf->st_atime_s = stbuf->st_mtime_s = stbuf->st_ctime_s = 0;

	return 0;
}

#endif

int
inv_seek(LargeObjectDesc *obj_desc, int offset, int whence)
{
	int			oldOffset;
	Datum		d;
	ScanKeyData skey;

	Assert(PointerIsValid(obj_desc));

	if (whence == SEEK_CUR)
	{
		offset += obj_desc->offset;		/* calculate absolute position */
		return inv_seek(obj_desc, offset, SEEK_SET);
	}

	/*
	 * if you seek past the end (offset > 0) I have no clue what happens
	 * :-(				  B.L.	 9/1/93
	 */
	if (whence == SEEK_END)
	{
		/* need read lock for getsize */
		if (!(obj_desc->flags & IFS_RDLOCK))
		{
			LockRelation(obj_desc->heap_r, ShareLock);
			obj_desc->flags |= IFS_RDLOCK;
		}
		offset += _inv_getsize(obj_desc->heap_r,
							   obj_desc->hdesc,
							   obj_desc->index_r);
		return inv_seek(obj_desc, offset, SEEK_SET);
	}

	/*
	 * Whenever we do a seek, we turn off the EOF flag bit to force
	 * ourselves to check for real on the next read.
	 */

	obj_desc->flags &= ~IFS_ATEOF;
	oldOffset = obj_desc->offset;
	obj_desc->offset = offset;

	/* try to avoid doing any work, if we can manage it */
	if (offset >= obj_desc->lowbyte
		&& offset <= obj_desc->highbyte
		&& oldOffset <= obj_desc->highbyte
		&& obj_desc->iscan != (IndexScanDesc) NULL)
		return offset;

	/*
	 * To do a seek on an inversion file, we start an index scan that will
	 * bring us to the right place.  Each tuple in an inversion file
	 * stores the offset of the last byte that appears on it, and we have
	 * an index on this.
	 */


	/* right now, just assume that the operation is SEEK_SET */
	if (obj_desc->iscan != (IndexScanDesc) NULL)
	{
		d = Int32GetDatum(offset);
		btmovescan(obj_desc->iscan, d);
	}
	else
	{

		ScanKeyEntryInitialize(&skey, 0x0, 1, F_INT4GE,
							   Int32GetDatum(offset));

		obj_desc->iscan = index_beginscan(obj_desc->index_r,
										  (bool) 0, (uint16) 1,
										  &skey);
	}

	return offset;
}

int
inv_tell(LargeObjectDesc *obj_desc)
{
	Assert(PointerIsValid(obj_desc));

	return obj_desc->offset;
}

int
inv_read(LargeObjectDesc *obj_desc, char *buf, int nbytes)
{
	HeapTupleData tuple;
	int			nread;
	int			off;
	int			ncopy;
	Datum		d;
	struct varlena *fsblock;
	bool		isNull;

	Assert(PointerIsValid(obj_desc));
	Assert(buf != NULL);

	/* if we're already at EOF, we don't need to do any work here */
	if (obj_desc->flags & IFS_ATEOF)
		return 0;

	/* make sure we obey two-phase locking */
	if (!(obj_desc->flags & IFS_RDLOCK))
	{
		LockRelation(obj_desc->heap_r, ShareLock);
		obj_desc->flags |= IFS_RDLOCK;
	}

	nread = 0;

	/* fetch a block at a time */
	while (nread < nbytes)
	{
		Buffer		buffer;

		/* fetch an inversion file system block */
		inv_fetchtup(obj_desc, &tuple, &buffer);

		if (tuple.t_data == NULL)
		{
			obj_desc->flags |= IFS_ATEOF;
			break;
		}

		/* copy the data from this block into the buffer */
		d = heap_getattr(&tuple, 2, obj_desc->hdesc, &isNull);
		ReleaseBuffer(buffer);

		fsblock = (struct varlena *) DatumGetPointer(d);

		off = obj_desc->offset - obj_desc->lowbyte;
		ncopy = obj_desc->highbyte - obj_desc->offset + 1;
		if (ncopy > (nbytes - nread))
			ncopy = (nbytes - nread);
		memmove(buf, &(fsblock->vl_dat[off]), ncopy);

		/* move pointers past the amount we just read */
		buf += ncopy;
		nread += ncopy;
		obj_desc->offset += ncopy;
	}

	return nread;
}

int
inv_write(LargeObjectDesc *obj_desc, char *buf, int nbytes)
{
	HeapTupleData tuple;
	int			nwritten;
	int			tuplen;

	Assert(PointerIsValid(obj_desc));
	Assert(buf != NULL);

	/*
	 * Make sure we obey two-phase locking.  A write lock entitles you to
	 * read the relation, as well.
	 */

	if (!(obj_desc->flags & IFS_WRLOCK))
	{
		LockRelation(obj_desc->heap_r, ExclusiveLock);
		obj_desc->flags |= (IFS_WRLOCK | IFS_RDLOCK);
	}

	nwritten = 0;

	/* write a block at a time */
	while (nwritten < nbytes)
	{
		Buffer		buffer;

		/*
		 * Fetch the current inversion file system block.  If the class
		 * storing the inversion file is empty, we don't want to do an
		 * index lookup, since index lookups choke on empty files (should
		 * be fixed someday).
		 */

		if ((obj_desc->flags & IFS_ATEOF)
			|| obj_desc->heap_r->rd_nblocks == 0)
			tuple.t_data = NULL;
		else
			inv_fetchtup(obj_desc, &tuple, &buffer);

		/* either append or replace a block, as required */
		if (tuple.t_data == NULL)
			tuplen = inv_wrnew(obj_desc, buf, nbytes - nwritten);
		else
		{
			if (obj_desc->offset > obj_desc->highbyte)
			{
				tuplen = inv_wrnew(obj_desc, buf, nbytes - nwritten);
				ReleaseBuffer(buffer);
			}
			else
				tuplen = inv_wrold(obj_desc, buf, nbytes - nwritten, &tuple, buffer);

			/*
			 * inv_wrold() has already issued WriteBuffer() which has
			 * decremented local reference counter (LocalRefCount). So we
			 * should not call ReleaseBuffer() here. -- Tatsuo 99/2/4
			 */
		}

		/* move pointers past the amount we just wrote */
		buf += tuplen;
		nwritten += tuplen;
		obj_desc->offset += tuplen;
	}

	/* that's it */
	return nwritten;
}

/*
 * inv_cleanindex
 *		 Clean opened indexes for large objects, and clears current result.
 *		 This is necessary on transaction commit in order to prevent buffer
 *		 leak.
 *		 This function must be called for each opened large object.
 *		 [ PA, 7/17/98 ]
 */
void
inv_cleanindex(LargeObjectDesc *obj_desc)
{
	Assert(PointerIsValid(obj_desc));

	if (obj_desc->iscan == (IndexScanDesc) NULL)
		return;

	index_endscan(obj_desc->iscan);
	obj_desc->iscan = (IndexScanDesc) NULL;

	ItemPointerSetInvalid(&(obj_desc->htid));
}

/*
 *	inv_fetchtup -- Fetch an inversion file system block.
 *
 *		This routine finds the file system block containing the offset
 *		recorded in the obj_desc structure.  Later, we need to think about
 *		the effects of non-functional updates (can you rewrite the same
 *		block twice in a single transaction?), but for now, we won't bother.
 *
 *		Parameters:
 *				obj_desc -- the object descriptor.
 *				bufP -- pointer to a buffer in the buffer cache; caller
 *						must free this.
 *
 *		Returns:
 *				A heap tuple containing the desired block, or NULL if no
 *				such tuple exists.
 */
static void
inv_fetchtup(LargeObjectDesc *obj_desc, HeapTuple tuple, Buffer *buffer)
{
	RetrieveIndexResult res;
	Datum		d;
	int			firstbyte,
				lastbyte;
	struct varlena *fsblock;
	bool		isNull;

	/*
	 * If we've exhausted the current block, we need to get the next one.
	 * When we support time travel and non-functional updates, we will
	 * need to loop over the blocks, rather than just have an 'if', in
	 * order to find the one we're really interested in.
	 */

	if (obj_desc->offset > obj_desc->highbyte
		|| obj_desc->offset < obj_desc->lowbyte
		|| !ItemPointerIsValid(&(obj_desc->htid)))
	{
		ScanKeyData skey;

		ScanKeyEntryInitialize(&skey, 0x0, 1, F_INT4GE,
							   Int32GetDatum(obj_desc->offset));

		/* initialize scan key if not done */
		if (obj_desc->iscan == (IndexScanDesc) NULL)
		{

			/*
			 * As scan index may be prematurely closed (on commit), we
			 * must use object current offset (was 0) to reinitialize the
			 * entry [ PA ].
			 */
			obj_desc->iscan = index_beginscan(obj_desc->index_r,
											  (bool) 0, (uint16) 1,
											  &skey);
		}
		else
			index_rescan(obj_desc->iscan, false, &skey);
		do
		{
			res = index_getnext(obj_desc->iscan, ForwardScanDirection);

			if (res == (RetrieveIndexResult) NULL)
			{
				ItemPointerSetInvalid(&(obj_desc->htid));
				tuple->t_datamcxt = NULL;
				tuple->t_data = NULL;
				return;
			}

			/*
			 * For time travel, we need to use the actual time qual here,
			 * rather that NowTimeQual.  We currently have no way to pass
			 * a time qual in.
			 *
			 * This is now valid for snapshot !!! And should be fixed in some
			 * way...	- vadim 07/28/98
			 *
			 */
			tuple->t_self = res->heap_iptr;
			heap_fetch(obj_desc->heap_r, SnapshotNow, tuple, buffer);
			pfree(res);
		} while (tuple->t_data == NULL);

		/* remember this tid -- we may need it for later reads/writes */
		ItemPointerCopy(&(tuple->t_self), &obj_desc->htid);
	}
	else
	{
		tuple->t_self = obj_desc->htid;
		heap_fetch(obj_desc->heap_r, SnapshotNow, tuple, buffer);
		if (tuple->t_data == NULL)
			elog(ERROR, "inv_fetchtup: heap_fetch failed");
	}

	/*
	 * By here, we have the heap tuple we're interested in.  We cache the
	 * upper and lower bounds for this block in the object descriptor and
	 * return the tuple.
	 */

	d = heap_getattr(tuple, 1, obj_desc->hdesc, &isNull);
	lastbyte = (int32) DatumGetInt32(d);
	d = heap_getattr(tuple, 2, obj_desc->hdesc, &isNull);
	fsblock = (struct varlena *) DatumGetPointer(d);

	/*
	 * order of + and - is important -- these are unsigned quantites near
	 * 0
	 */
	firstbyte = (lastbyte + 1 + sizeof(fsblock->vl_len)) - fsblock->vl_len;

	obj_desc->lowbyte = firstbyte;
	obj_desc->highbyte = lastbyte;

	return;
}

/*
 *	inv_wrnew() -- append a new filesystem block tuple to the inversion
 *					file.
 *
 *		In response to an inv_write, we append one or more file system
 *		blocks to the class containing the large object.  We violate the
 *		class abstraction here in order to pack things as densely as we
 *		are able.  We examine the last page in the relation, and write
 *		just enough to fill it, assuming that it has above a certain
 *		threshold of space available.  If the space available is less than
 *		the threshold, we allocate a new page by writing a big tuple.
 *
 *		By the time we get here, we know all the parameters passed in
 *		are valid, and that we hold the appropriate lock on the heap
 *		relation.
 *
 *		Parameters:
 *				obj_desc: large object descriptor for which to append block.
 *				buf: buffer containing data to write.
 *				nbytes: amount to write
 *
 *		Returns:
 *				number of bytes actually written to the new tuple.
 */
static int
inv_wrnew(LargeObjectDesc *obj_desc, char *buf, int nbytes)
{
	Relation	hr;
	HeapTuple	ntup;
	Buffer		buffer;
	Page		page;
	int			nblocks;
	int			nwritten;

	hr = obj_desc->heap_r;

	/*
	 * Get the last block in the relation.	If there's no data in the
	 * relation at all, then we just get a new block.  Otherwise, we check
	 * the last block to see whether it has room to accept some or all of
	 * the data that the user wants to write.  If it doesn't, then we
	 * allocate a new block.
	 */

	nblocks = RelationGetNumberOfBlocks(hr);

	if (nblocks > 0)
	{
		buffer = ReadBuffer(hr, nblocks - 1);
		page = BufferGetPage(buffer);
	}
	else
	{
		buffer = ReadBuffer(hr, P_NEW);
		page = BufferGetPage(buffer);
		PageInit(page, BufferGetPageSize(buffer), 0);
	}

	/*
	 * If the last page is too small to hold all the data, and it's too
	 * small to hold IMINBLK, then we allocate a new page.	If it will
	 * hold at least IMINBLK, but less than all the data requested, then
	 * we write IMINBLK here.  The caller is responsible for noticing that
	 * less than the requested number of bytes were written, and calling
	 * this routine again.
	 */

	nwritten = IFREESPC(page);
	if (nwritten < nbytes)
	{
		if (nwritten < IMINBLK)
		{
			ReleaseBuffer(buffer);
			buffer = ReadBuffer(hr, P_NEW);
			page = BufferGetPage(buffer);
			PageInit(page, BufferGetPageSize(buffer), 0);
			if (nbytes > IMAXBLK)
				nwritten = IMAXBLK;
			else
				nwritten = nbytes;
		}
	}
	else
		nwritten = nbytes;

	/*
	 * Insert a new file system block tuple, index it, and write it out.
	 */

	ntup = inv_newtuple(obj_desc, buffer, page, buf, nwritten);
	inv_indextup(obj_desc, ntup);
	heap_freetuple(ntup);

	/* new tuple is inserted */
	WriteBuffer(buffer);

	return nwritten;
}

static int
inv_wrold(LargeObjectDesc *obj_desc,
		  char *dbuf,
		  int nbytes,
		  HeapTuple tuple,
		  Buffer buffer)
{
	Relation	hr;
	HeapTuple	ntup;
	Buffer		newbuf;
	Page		page;
	Page		newpage;
	int			tupbytes;
	Datum		d;
	struct varlena *fsblock;
	int			nwritten,
				nblocks,
				freespc;
	bool		isNull;
	int			keep_offset;
	RetrieveIndexResult res;

	/*
	 * Since we're using a no-overwrite storage manager, the way we
	 * overwrite blocks is to mark the old block invalid and append a new
	 * block.  First mark the old block invalid.  This violates the tuple
	 * abstraction.
	 */

	TransactionIdStore(GetCurrentTransactionId(), &(tuple->t_data->t_xmax));
	tuple->t_data->t_cmax = GetCurrentCommandId();
	tuple->t_data->t_infomask &= ~(HEAP_XMAX_COMMITTED | HEAP_XMAX_INVALID);

	/*
	 * If we're overwriting the entire block, we're lucky.	All we need to
	 * do is to insert a new block.
	 */

	if (obj_desc->offset == obj_desc->lowbyte
		&& obj_desc->lowbyte + nbytes >= obj_desc->highbyte)
	{
		WriteBuffer(buffer);
		return inv_wrnew(obj_desc, dbuf, nbytes);
	}

	/*
	 * By here, we need to overwrite part of the data in the current
	 * tuple.  In order to reduce the degree to which we fragment blocks,
	 * we guarantee that no block will be broken up due to an overwrite.
	 * This means that we need to allocate a tuple on a new page, if
	 * there's not room for the replacement on this one.
	 */

	newbuf = buffer;
	page = BufferGetPage(buffer);
	newpage = BufferGetPage(newbuf);
	hr = obj_desc->heap_r;
	freespc = IFREESPC(page);
	d = heap_getattr(tuple, 2, obj_desc->hdesc, &isNull);
	fsblock = (struct varlena *) DatumGetPointer(d);
	tupbytes = fsblock->vl_len - sizeof(fsblock->vl_len);

	if (freespc < tupbytes)
	{

		/*
		 * First see if there's enough space on the last page of the table
		 * to put this tuple.
		 */

		nblocks = RelationGetNumberOfBlocks(hr);

		if (nblocks > 0)
		{
			newbuf = ReadBuffer(hr, nblocks - 1);
			newpage = BufferGetPage(newbuf);
		}
		else
		{
			newbuf = ReadBuffer(hr, P_NEW);
			newpage = BufferGetPage(newbuf);
			PageInit(newpage, BufferGetPageSize(newbuf), 0);
		}

		freespc = IFREESPC(newpage);

		/*
		 * If there's no room on the last page, allocate a new last page
		 * for the table, and put it there.
		 */

		if (freespc < tupbytes)
		{
			ReleaseBuffer(newbuf);
			newbuf = ReadBuffer(hr, P_NEW);
			newpage = BufferGetPage(newbuf);
			PageInit(newpage, BufferGetPageSize(newbuf), 0);
		}
	}

	nwritten = nbytes;
	if (nwritten > obj_desc->highbyte - obj_desc->offset + 1)
		nwritten = obj_desc->highbyte - obj_desc->offset + 1;
	memmove(VARDATA(fsblock) + (obj_desc->offset - obj_desc->lowbyte),
			dbuf, nwritten);

	/*
	 * we are rewriting the entire old block, therefore we reset offset to
	 * the lowbyte of the original block before jumping into
	 * inv_newtuple()
	 */
	keep_offset = obj_desc->offset;
	obj_desc->offset = obj_desc->lowbyte;
	ntup = inv_newtuple(obj_desc, newbuf, newpage, VARDATA(fsblock),
						tupbytes);
	/* after we are done, we restore to the true offset */
	obj_desc->offset = keep_offset;

	/*
	 * By here, we have a page (newpage) that's guaranteed to have enough
	 * space on it to put the new tuple.  Call inv_newtuple to do the
	 * work.  Passing NULL as a buffer to inv_newtuple() keeps it from
	 * copying any data into the new tuple.  When it returns, the tuple is
	 * ready to receive data from the old tuple and the user's data
	 * buffer.
	 */
/*
	ntup = inv_newtuple(obj_desc, newbuf, newpage, (char *) NULL, tupbytes);
	dptr = ((char *) ntup) + ntup->t_hoff -
				(sizeof(HeapTupleData) - offsetof(HeapTupleData, t_bits)) +
				sizeof(int4)
				+ sizeof(fsblock->vl_len);

	if (obj_desc->offset > obj_desc->lowbyte) {
		memmove(dptr,
				&(fsblock->vl_dat[0]),
				obj_desc->offset - obj_desc->lowbyte);
		dptr += obj_desc->offset - obj_desc->lowbyte;
	}


	nwritten = nbytes;
	if (nwritten > obj_desc->highbyte - obj_desc->offset + 1)
		nwritten = obj_desc->highbyte - obj_desc->offset + 1;

	memmove(dptr, dbuf, nwritten);
	dptr += nwritten;

	if (obj_desc->offset + nwritten < obj_desc->highbyte + 1) {
*/
/*
		loc = (obj_desc->highbyte - obj_desc->offset)
				+ nwritten;
		sz = obj_desc->highbyte - (obj_desc->lowbyte + loc);

		what's going on here?? - jolly
*/
/*
		sz = (obj_desc->highbyte + 1) - (obj_desc->offset + nwritten);
		memmove(&(fsblock->vl_dat[0]), dptr, sz);
	}
*/


	/* index the new tuple */
	inv_indextup(obj_desc, ntup);
	heap_freetuple(ntup);

	/*
	 * move the scandesc forward so we don't reread the newly inserted
	 * tuple on the next index scan
	 */
	res = NULL;
	if (obj_desc->iscan)
		res = index_getnext(obj_desc->iscan, ForwardScanDirection);

	if (res)
		pfree(res);

	/*
	 * Okay, by here, a tuple for the new block is correctly placed,
	 * indexed, and filled.  Write the changed pages out.
	 */

	WriteBuffer(buffer);
	if (newbuf != buffer)
		WriteBuffer(newbuf);

	/* Tuple id is no longer valid */
	ItemPointerSetInvalid(&(obj_desc->htid));

	/* done */
	return nwritten;
}

static HeapTuple
inv_newtuple(LargeObjectDesc *obj_desc,
			 Buffer buffer,
			 Page page,
			 char *dbuf,
			 int nwrite)
{
	HeapTuple	ntup = (HeapTuple) palloc(sizeof(HeapTupleData));
	PageHeader	ph;
	int			tupsize;
	int			hoff;
	Offset		lower;
	Offset		upper;
	ItemId		itemId;
	OffsetNumber off;
	OffsetNumber limit;
	char	   *attptr;

	/* compute tuple size -- no nulls */
	hoff = offsetof(HeapTupleHeaderData, t_bits);
	hoff = MAXALIGN(hoff);

	/* add in olastbyte, varlena.vl_len, varlena.vl_dat */
	tupsize = hoff + (2 * sizeof(int32)) + nwrite;
	tupsize = MAXALIGN(tupsize);

	/*
	 * Allocate the tuple on the page, violating the page abstraction.
	 * This code was swiped from PageAddItem().
	 */

	ph = (PageHeader) page;
	limit = OffsetNumberNext(PageGetMaxOffsetNumber(page));

	/* look for "recyclable" (unused & deallocated) ItemId */
	for (off = FirstOffsetNumber; off < limit; off = OffsetNumberNext(off))
	{
		itemId = &ph->pd_linp[off - 1];
		if ((((*itemId).lp_flags & LP_USED) == 0) &&
			((*itemId).lp_len == 0))
			break;
	}

	if (off > limit)
		lower = (Offset) (((char *) (&ph->pd_linp[off])) - ((char *) page));
	else if (off == limit)
		lower = ph->pd_lower + sizeof(ItemIdData);
	else
		lower = ph->pd_lower;

	upper = ph->pd_upper - tupsize;

	itemId = &ph->pd_linp[off - 1];
	(*itemId).lp_off = upper;
	(*itemId).lp_len = tupsize;
	(*itemId).lp_flags = LP_USED;
	ph->pd_lower = lower;
	ph->pd_upper = upper;

	ntup->t_datamcxt = NULL;
	ntup->t_data = (HeapTupleHeader) ((char *) page + upper);

	/*
	 * Tuple is now allocated on the page.	Next, fill in the tuple
	 * header.	This block of code violates the tuple abstraction.
	 */

	ntup->t_len = tupsize;
	ItemPointerSet(&ntup->t_self, BufferGetBlockNumber(buffer), off);
	LastOidProcessed = ntup->t_data->t_oid = newoid();
	TransactionIdStore(GetCurrentTransactionId(), &(ntup->t_data->t_xmin));
	ntup->t_data->t_cmin = GetCurrentCommandId();
	StoreInvalidTransactionId(&(ntup->t_data->t_xmax));
	ntup->t_data->t_cmax = 0;
	ntup->t_data->t_infomask = HEAP_XMAX_INVALID;
	ntup->t_data->t_natts = 2;
	ntup->t_data->t_hoff = hoff;

	/* if a NULL is passed in, avoid the calculations below */
	if (dbuf == NULL)
		return ntup;

	/*
	 * Finally, copy the user's data buffer into the tuple.  This violates
	 * the tuple and class abstractions.
	 */

	attptr = ((char *) ntup->t_data) + hoff;
	*((int32 *) attptr) = obj_desc->offset + nwrite - 1;
	attptr += sizeof(int32);

	/*
	 * *  mer fixed disk layout of varlenas to get rid of the need for
	 * this. *
	 *
	 * ((int32 *) attptr) = nwrite + sizeof(int32); *  attptr +=
	 * sizeof(int32);
	 */

	*((int32 *) attptr) = nwrite + sizeof(int32);
	attptr += sizeof(int32);

	/*
	 * If a data buffer was passed in, then copy the data from the buffer
	 * to the tuple.  Some callers (eg, inv_wrold()) may not pass in a
	 * buffer, since they have to copy part of the old tuple data and part
	 * of the user's new data into the new tuple.
	 */

	if (dbuf != (char *) NULL)
		memmove(attptr, dbuf, nwrite);

	/* keep track of boundary of current tuple */
	obj_desc->lowbyte = obj_desc->offset;
	obj_desc->highbyte = obj_desc->offset + nwrite - 1;

	/* new tuple is filled -- return it */
	return ntup;
}

static void
inv_indextup(LargeObjectDesc *obj_desc, HeapTuple tuple)
{
	InsertIndexResult res;
	Datum		v[1];
	char		n[1];

	n[0] = ' ';
	v[0] = Int32GetDatum(obj_desc->highbyte);
	res = index_insert(obj_desc->index_r, &v[0], &n[0],
					   &(tuple->t_self), obj_desc->heap_r);

	if (res)
		pfree(res);
}

/*
static void
DumpPage(Page page, int blkno)
{
		ItemId			lp;
		HeapTuple		tup;
		int				flags, i, nline;
		ItemPointerData pointerData;

		printf("\t[subblock=%d]:lower=%d:upper=%d:special=%d\n", 0,
				((PageHeader)page)->pd_lower, ((PageHeader)page)->pd_upper,
				((PageHeader)page)->pd_special);

		printf("\t:MaxOffsetNumber=%d\n",
			   (int16) PageGetMaxOffsetNumber(page));

		nline = (int16) PageGetMaxOffsetNumber(page);

{
		int		i;
		char	*cp;

		i = PageGetSpecialSize(page);
		cp = PageGetSpecialPointer(page);

		printf("\t:SpecialData=");

		while (i > 0) {
				printf(" 0x%02x", *cp);
				cp += 1;
				i -= 1;
		}
		printf("\n");
}
		for (i = 0; i < nline; i++) {
				lp = ((PageHeader)page)->pd_linp + i;
				flags = (*lp).lp_flags;
				ItemPointerSet(&pointerData, blkno, 1 + i);
				printf("%s:off=%d:flags=0x%x:len=%d",
						ItemPointerFormExternal(&pointerData), (*lp).lp_off,
						flags, (*lp).lp_len);

				if (flags & LP_USED) {
						HeapTupleData	htdata;

						printf(":USED");

						memmove((char *) &htdata,
								(char *) &((char *)page)[(*lp).lp_off],
								sizeof(htdata));

						tup = &htdata;

						printf("\n\t:ctid=%s:oid=%d",
								ItemPointerFormExternal(&tup->t_ctid),
								tup->t_oid);
						printf(":natts=%d:thoff=%d:",
								tup->t_natts,
								tup->t_hoff);

						printf("\n\t:cmin=%u:",
								tup->t_cmin);

						printf("xmin=%u:", tup->t_xmin);

						printf("\n\t:cmax=%u:",
								tup->t_cmax);

						printf("xmax=%u:\n", tup->t_xmax);

				} else
						putchar('\n');
		}
}

static char*
ItemPointerFormExternal(ItemPointer pointer)
{
		static char		itemPointerString[32];

		if (!ItemPointerIsValid(pointer)) {
			memmove(itemPointerString, "<-,-,->", sizeof "<-,-,->");
		} else {
			sprintf(itemPointerString, "<%u,%u>",
					ItemPointerGetBlockNumber(pointer),
					ItemPointerGetOffsetNumber(pointer));
		}

		return itemPointerString;
}
*/

static int
_inv_getsize(Relation hreln, TupleDesc hdesc, Relation ireln)
{
	IndexScanDesc iscan;
	RetrieveIndexResult res;
	HeapTupleData tuple;
	Datum		d;
	long		size;
	bool		isNull;
	Buffer		buffer;

	/* scan backwards from end */
	iscan = index_beginscan(ireln, (bool) 1, 0, (ScanKey) NULL);

	do
	{
		res = index_getnext(iscan, BackwardScanDirection);

		/*
		 * If there are no more index tuples, then the relation is empty,
		 * so the file's size is zero.
		 */

		if (res == (RetrieveIndexResult) NULL)
		{
			index_endscan(iscan);
			return 0;
		}

		/*
		 * For time travel, we need to use the actual time qual here,
		 * rather that NowTimeQual.  We currently have no way to pass a
		 * time qual in.
		 */
		tuple.t_self = res->heap_iptr;
		heap_fetch(hreln, SnapshotNow, &tuple, &buffer);
		pfree(res);
	} while (tuple.t_data == NULL);

	/* don't need the index scan anymore */
	index_endscan(iscan);

	/* get olastbyte attribute */
	d = heap_getattr(&tuple, 1, hdesc, &isNull);
	size = DatumGetInt32(d) + 1;
	ReleaseBuffer(buffer);

	return size;
}
