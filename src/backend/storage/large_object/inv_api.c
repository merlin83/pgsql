/*-------------------------------------------------------------------------
 *
 * inv_api.c
 *	  routines for manipulating inversion fs large objects. This file
 *	  contains the user-level large object application interface routines.
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/storage/large_object/inv_api.c,v 1.108 2004/12/31 22:00:59 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/tuptoaster.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_largeobject.h"
#include "commands/comment.h"
#include "libpq/libpq-fs.h"
#include "storage/large_object.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/resowner.h"


/*
 * All accesses to pg_largeobject and its index make use of a single Relation
 * reference, so that we only need to open pg_relation once per transaction.
 * To avoid problems when the first such reference occurs inside a
 * subtransaction, we execute a slightly klugy maneuver to assign ownership of
 * the Relation reference to TopTransactionResourceOwner.
 */
static Relation lo_heap_r = NULL;
static Relation lo_index_r = NULL;


/*
 * Open pg_largeobject and its index, if not already done in current xact
 */
static void
open_lo_relation(void)
{
	ResourceOwner currentOwner;

	if (lo_heap_r && lo_index_r)
		return;					/* already open in current xact */

	/* Arrange for the top xact to own these relation references */
	currentOwner = CurrentResourceOwner;
	PG_TRY();
	{
		CurrentResourceOwner = TopTransactionResourceOwner;

		/* Use RowExclusiveLock since we might either read or write */
		if (lo_heap_r == NULL)
			lo_heap_r = heap_openr(LargeObjectRelationName, RowExclusiveLock);
		if (lo_index_r == NULL)
			lo_index_r = index_openr(LargeObjectLOidPNIndex);
	}
	PG_CATCH();
	{
		/* Ensure CurrentResourceOwner is restored on error */
		CurrentResourceOwner = currentOwner;
		PG_RE_THROW();
	}
	PG_END_TRY();
	CurrentResourceOwner = currentOwner;
}

/*
 * Clean up at main transaction end
 */
void
close_lo_relation(bool isCommit)
{
	if (lo_heap_r || lo_index_r)
	{
		/*
		 * Only bother to close if committing; else abort cleanup will
		 * handle it
		 */
		if (isCommit)
		{
			ResourceOwner currentOwner;

			currentOwner = CurrentResourceOwner;
			PG_TRY();
			{
				CurrentResourceOwner = TopTransactionResourceOwner;

				if (lo_index_r)
					index_close(lo_index_r);
				if (lo_heap_r)
					heap_close(lo_heap_r, NoLock);
			}
			PG_CATCH();
			{
				/* Ensure CurrentResourceOwner is restored on error */
				CurrentResourceOwner = currentOwner;
				PG_RE_THROW();
			}
			PG_END_TRY();
			CurrentResourceOwner = currentOwner;
		}
		lo_heap_r = NULL;
		lo_index_r = NULL;
	}
}


static int32
getbytealen(bytea *data)
{
	Assert(!VARATT_IS_EXTENDED(data));
	if (VARSIZE(data) < VARHDRSZ)
		elog(ERROR, "invalid VARSIZE(data)");
	return (VARSIZE(data) - VARHDRSZ);
}


/*
 *	inv_create -- create a new large object.
 *
 *		Arguments:
 *		  flags
 *
 *		Returns:
 *		  large object descriptor, appropriately filled in.
 */
LargeObjectDesc *
inv_create(int flags)
{
	Oid			file_oid;
	LargeObjectDesc *retval;

	/*
	 * Allocate an OID to be the LO's identifier.
	 */
	file_oid = newoid();

	/* Check for duplicate (shouldn't happen) */
	if (LargeObjectExists(file_oid))
		elog(ERROR, "large object %u already exists", file_oid);

	/*
	 * Create the LO by writing an empty first page for it in
	 * pg_largeobject
	 */
	LargeObjectCreate(file_oid);

	/*
	 * Advance command counter so that new tuple will be seen by later
	 * large-object operations in this transaction.
	 */
	CommandCounterIncrement();

	/*
	 * Prepare LargeObjectDesc data structure for accessing LO
	 */
	retval = (LargeObjectDesc *) palloc(sizeof(LargeObjectDesc));

	retval->id = file_oid;
	retval->subid = GetCurrentSubTransactionId();
	retval->offset = 0;

	if (flags & INV_WRITE)
		retval->flags = IFS_WRLOCK | IFS_RDLOCK;
	else if (flags & INV_READ)
		retval->flags = IFS_RDLOCK;
	else
		elog(ERROR, "invalid flags: %d", flags);

	return retval;
}

/*
 *	inv_open -- access an existing large object.
 *
 *		Returns:
 *		  large object descriptor, appropriately filled in.
 */
LargeObjectDesc *
inv_open(Oid lobjId, int flags)
{
	LargeObjectDesc *retval;

	if (!LargeObjectExists(lobjId))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("large object %u does not exist", lobjId)));

	retval = (LargeObjectDesc *) palloc(sizeof(LargeObjectDesc));

	retval->id = lobjId;
	retval->subid = GetCurrentSubTransactionId();
	retval->offset = 0;

	if (flags & INV_WRITE)
		retval->flags = IFS_WRLOCK | IFS_RDLOCK;
	else if (flags & INV_READ)
		retval->flags = IFS_RDLOCK;
	else
		elog(ERROR, "invalid flags: %d", flags);

	return retval;
}

/*
 * Closes an existing large object descriptor.
 */
void
inv_close(LargeObjectDesc *obj_desc)
{
	Assert(PointerIsValid(obj_desc));
	pfree(obj_desc);
}

/*
 * Destroys an existing large object (not to be confused with a descriptor!)
 *
 * returns -1 if failed
 */
int
inv_drop(Oid lobjId)
{
	Oid			classoid;

	LargeObjectDrop(lobjId);

	/* pg_largeobject doesn't have a hard-coded OID, so must look it up */
	classoid = get_system_catalog_relid(LargeObjectRelationName);

	/* Delete any comments on the large object */
	DeleteComments(lobjId, classoid, 0);

	/*
	 * Advance command counter so that tuple removal will be seen by later
	 * large-object operations in this transaction.
	 */
	CommandCounterIncrement();

	return 1;
}

/*
 * Determine size of a large object
 *
 * NOTE: LOs can contain gaps, just like Unix files.  We actually return
 * the offset of the last byte + 1.
 */
static uint32
inv_getsize(LargeObjectDesc *obj_desc)
{
	bool		found = false;
	uint32		lastbyte = 0;
	ScanKeyData skey[1];
	IndexScanDesc sd;
	HeapTuple	tuple;

	Assert(PointerIsValid(obj_desc));

	open_lo_relation();

	ScanKeyInit(&skey[0],
				Anum_pg_largeobject_loid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(obj_desc->id));

	sd = index_beginscan(lo_heap_r, lo_index_r,
						 SnapshotNow, 1, skey);

	/*
	 * Because the pg_largeobject index is on both loid and pageno, but we
	 * constrain only loid, a backwards scan should visit all pages of the
	 * large object in reverse pageno order.  So, it's sufficient to
	 * examine the first valid tuple (== last valid page).
	 */
	while ((tuple = index_getnext(sd, BackwardScanDirection)) != NULL)
	{
		Form_pg_largeobject data;
		bytea	   *datafield;
		bool		pfreeit;

		found = true;
		data = (Form_pg_largeobject) GETSTRUCT(tuple);
		datafield = &(data->data);
		pfreeit = false;
		if (VARATT_IS_EXTENDED(datafield))
		{
			datafield = (bytea *)
				heap_tuple_untoast_attr((varattrib *) datafield);
			pfreeit = true;
		}
		lastbyte = data->pageno * LOBLKSIZE + getbytealen(datafield);
		if (pfreeit)
			pfree(datafield);
		break;
	}

	index_endscan(sd);

	if (!found)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("large object %u does not exist", obj_desc->id)));
	return lastbyte;
}

int
inv_seek(LargeObjectDesc *obj_desc, int offset, int whence)
{
	Assert(PointerIsValid(obj_desc));

	switch (whence)
	{
		case SEEK_SET:
			if (offset < 0)
				elog(ERROR, "invalid seek offset: %d", offset);
			obj_desc->offset = offset;
			break;
		case SEEK_CUR:
			if (offset < 0 && obj_desc->offset < ((uint32) (-offset)))
				elog(ERROR, "invalid seek offset: %d", offset);
			obj_desc->offset += offset;
			break;
		case SEEK_END:
			{
				uint32		size = inv_getsize(obj_desc);

				if (offset < 0 && size < ((uint32) (-offset)))
					elog(ERROR, "invalid seek offset: %d", offset);
				obj_desc->offset = size + offset;
			}
			break;
		default:
			elog(ERROR, "invalid whence: %d", whence);
	}
	return obj_desc->offset;
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
	int			nread = 0;
	int			n;
	int			off;
	int			len;
	int32		pageno = (int32) (obj_desc->offset / LOBLKSIZE);
	uint32		pageoff;
	ScanKeyData skey[2];
	IndexScanDesc sd;
	HeapTuple	tuple;

	Assert(PointerIsValid(obj_desc));
	Assert(buf != NULL);

	if (nbytes <= 0)
		return 0;

	open_lo_relation();

	ScanKeyInit(&skey[0],
				Anum_pg_largeobject_loid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(obj_desc->id));

	ScanKeyInit(&skey[1],
				Anum_pg_largeobject_pageno,
				BTGreaterEqualStrategyNumber, F_INT4GE,
				Int32GetDatum(pageno));

	sd = index_beginscan(lo_heap_r, lo_index_r,
						 SnapshotNow, 2, skey);

	while ((tuple = index_getnext(sd, ForwardScanDirection)) != NULL)
	{
		Form_pg_largeobject data;
		bytea	   *datafield;
		bool		pfreeit;

		data = (Form_pg_largeobject) GETSTRUCT(tuple);

		/*
		 * We assume the indexscan will deliver pages in order.  However,
		 * there may be missing pages if the LO contains unwritten
		 * "holes". We want missing sections to read out as zeroes.
		 */
		pageoff = ((uint32) data->pageno) * LOBLKSIZE;
		if (pageoff > obj_desc->offset)
		{
			n = pageoff - obj_desc->offset;
			n = (n <= (nbytes - nread)) ? n : (nbytes - nread);
			MemSet(buf + nread, 0, n);
			nread += n;
			obj_desc->offset += n;
		}

		if (nread < nbytes)
		{
			Assert(obj_desc->offset >= pageoff);
			off = (int) (obj_desc->offset - pageoff);
			Assert(off >= 0 && off < LOBLKSIZE);

			datafield = &(data->data);
			pfreeit = false;
			if (VARATT_IS_EXTENDED(datafield))
			{
				datafield = (bytea *)
					heap_tuple_untoast_attr((varattrib *) datafield);
				pfreeit = true;
			}
			len = getbytealen(datafield);
			if (len > off)
			{
				n = len - off;
				n = (n <= (nbytes - nread)) ? n : (nbytes - nread);
				memcpy(buf + nread, VARDATA(datafield) + off, n);
				nread += n;
				obj_desc->offset += n;
			}
			if (pfreeit)
				pfree(datafield);
		}

		if (nread >= nbytes)
			break;
	}

	index_endscan(sd);

	return nread;
}

int
inv_write(LargeObjectDesc *obj_desc, char *buf, int nbytes)
{
	int			nwritten = 0;
	int			n;
	int			off;
	int			len;
	int32		pageno = (int32) (obj_desc->offset / LOBLKSIZE);
	ScanKeyData skey[2];
	IndexScanDesc sd;
	HeapTuple	oldtuple;
	Form_pg_largeobject olddata;
	bool		neednextpage;
	bytea	   *datafield;
	bool		pfreeit;
	struct
	{
		bytea		hdr;
		char		data[LOBLKSIZE];
	}			workbuf;
	char	   *workb = VARATT_DATA(&workbuf.hdr);
	HeapTuple	newtup;
	Datum		values[Natts_pg_largeobject];
	char		nulls[Natts_pg_largeobject];
	char		replace[Natts_pg_largeobject];
	CatalogIndexState indstate;

	Assert(PointerIsValid(obj_desc));
	Assert(buf != NULL);

	if (nbytes <= 0)
		return 0;

	open_lo_relation();

	indstate = CatalogOpenIndexes(lo_heap_r);

	ScanKeyInit(&skey[0],
				Anum_pg_largeobject_loid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(obj_desc->id));

	ScanKeyInit(&skey[1],
				Anum_pg_largeobject_pageno,
				BTGreaterEqualStrategyNumber, F_INT4GE,
				Int32GetDatum(pageno));

	sd = index_beginscan(lo_heap_r, lo_index_r,
						 SnapshotNow, 2, skey);

	oldtuple = NULL;
	olddata = NULL;
	neednextpage = true;

	while (nwritten < nbytes)
	{
		/*
		 * If possible, get next pre-existing page of the LO.  We assume
		 * the indexscan will deliver these in order --- but there may be
		 * holes.
		 */
		if (neednextpage)
		{
			if ((oldtuple = index_getnext(sd, ForwardScanDirection)) != NULL)
			{
				olddata = (Form_pg_largeobject) GETSTRUCT(oldtuple);
				Assert(olddata->pageno >= pageno);
			}
			neednextpage = false;
		}

		/*
		 * If we have a pre-existing page, see if it is the page we want
		 * to write, or a later one.
		 */
		if (olddata != NULL && olddata->pageno == pageno)
		{
			/*
			 * Update an existing page with fresh data.
			 *
			 * First, load old data into workbuf
			 */
			datafield = &(olddata->data);
			pfreeit = false;
			if (VARATT_IS_EXTENDED(datafield))
			{
				datafield = (bytea *)
					heap_tuple_untoast_attr((varattrib *) datafield);
				pfreeit = true;
			}
			len = getbytealen(datafield);
			Assert(len <= LOBLKSIZE);
			memcpy(workb, VARDATA(datafield), len);
			if (pfreeit)
				pfree(datafield);

			/*
			 * Fill any hole
			 */
			off = (int) (obj_desc->offset % LOBLKSIZE);
			if (off > len)
				MemSet(workb + len, 0, off - len);

			/*
			 * Insert appropriate portion of new data
			 */
			n = LOBLKSIZE - off;
			n = (n <= (nbytes - nwritten)) ? n : (nbytes - nwritten);
			memcpy(workb + off, buf + nwritten, n);
			nwritten += n;
			obj_desc->offset += n;
			off += n;
			/* compute valid length of new page */
			len = (len >= off) ? len : off;
			VARATT_SIZEP(&workbuf.hdr) = len + VARHDRSZ;

			/*
			 * Form and insert updated tuple
			 */
			memset(values, 0, sizeof(values));
			memset(nulls, ' ', sizeof(nulls));
			memset(replace, ' ', sizeof(replace));
			values[Anum_pg_largeobject_data - 1] = PointerGetDatum(&workbuf);
			replace[Anum_pg_largeobject_data - 1] = 'r';
			newtup = heap_modifytuple(oldtuple, lo_heap_r,
									  values, nulls, replace);
			simple_heap_update(lo_heap_r, &newtup->t_self, newtup);
			CatalogIndexInsert(indstate, newtup);
			heap_freetuple(newtup);

			/*
			 * We're done with this old page.
			 */
			oldtuple = NULL;
			olddata = NULL;
			neednextpage = true;
		}
		else
		{
			/*
			 * Write a brand new page.
			 *
			 * First, fill any hole
			 */
			off = (int) (obj_desc->offset % LOBLKSIZE);
			if (off > 0)
				MemSet(workb, 0, off);

			/*
			 * Insert appropriate portion of new data
			 */
			n = LOBLKSIZE - off;
			n = (n <= (nbytes - nwritten)) ? n : (nbytes - nwritten);
			memcpy(workb + off, buf + nwritten, n);
			nwritten += n;
			obj_desc->offset += n;
			/* compute valid length of new page */
			len = off + n;
			VARATT_SIZEP(&workbuf.hdr) = len + VARHDRSZ;

			/*
			 * Form and insert updated tuple
			 */
			memset(values, 0, sizeof(values));
			memset(nulls, ' ', sizeof(nulls));
			values[Anum_pg_largeobject_loid - 1] = ObjectIdGetDatum(obj_desc->id);
			values[Anum_pg_largeobject_pageno - 1] = Int32GetDatum(pageno);
			values[Anum_pg_largeobject_data - 1] = PointerGetDatum(&workbuf);
			newtup = heap_formtuple(lo_heap_r->rd_att, values, nulls);
			simple_heap_insert(lo_heap_r, newtup);
			CatalogIndexInsert(indstate, newtup);
			heap_freetuple(newtup);
		}
		pageno++;
	}

	index_endscan(sd);

	CatalogCloseIndexes(indstate);

	/*
	 * Advance command counter so that my tuple updates will be seen by
	 * later large-object operations in this transaction.
	 */
	CommandCounterIncrement();

	return nwritten;
}
