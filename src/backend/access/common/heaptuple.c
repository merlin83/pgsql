/*-------------------------------------------------------------------------
 *
 * heaptuple.c
 *	  This file contains heap tuple accessor and mutator routines, as well
 *	  as various tuple utilities.
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/access/common/heaptuple.c,v 1.95 2004/12/31 21:59:07 pgsql Exp $
 *
 * NOTES
 *	  The old interface functions have been converted to macros
 *	  and moved to heapam.h
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/tuptoaster.h"
#include "catalog/pg_type.h"


/* ----------------------------------------------------------------
 *						misc support routines
 * ----------------------------------------------------------------
 */

/* ----------------
 *		ComputeDataSize
 *
 * Determine size of the data area of a tuple to be constructed
 * ----------------
 */
Size
ComputeDataSize(TupleDesc tupleDesc,
				Datum *values,
				char *nulls)
{
	Size		data_length = 0;
	int			i;
	int			numberOfAttributes = tupleDesc->natts;
	Form_pg_attribute *att = tupleDesc->attrs;

	for (i = 0; i < numberOfAttributes; i++)
	{
		if (nulls[i] != ' ')
			continue;

		data_length = att_align(data_length, att[i]->attalign);
		data_length = att_addlength(data_length, att[i]->attlen, values[i]);
	}

	return data_length;
}

/* ----------------
 *		DataFill
 *
 * Load data portion of a tuple from values/nulls arrays
 * ----------------
 */
void
DataFill(char *data,
		 TupleDesc tupleDesc,
		 Datum *values,
		 char *nulls,
		 uint16 *infomask,
		 bits8 *bit)
{
	bits8	   *bitP;
	int			bitmask;
	int			i;
	int			numberOfAttributes = tupleDesc->natts;
	Form_pg_attribute *att = tupleDesc->attrs;

	if (bit != NULL)
	{
		bitP = &bit[-1];
		bitmask = CSIGNBIT;
	}
	else
	{
		/* just to keep compiler quiet */
		bitP = NULL;
		bitmask = 0;
	}

	*infomask &= ~(HEAP_HASNULL | HEAP_HASVARWIDTH | HEAP_HASEXTENDED);

	for (i = 0; i < numberOfAttributes; i++)
	{
		Size		data_length;

		if (bit != NULL)
		{
			if (bitmask != CSIGNBIT)
				bitmask <<= 1;
			else
			{
				bitP += 1;
				*bitP = 0x0;
				bitmask = 1;
			}

			if (nulls[i] == 'n')
			{
				*infomask |= HEAP_HASNULL;
				continue;
			}

			*bitP |= bitmask;
		}

		/* XXX we are aligning the pointer itself, not the offset */
		data = (char *) att_align((long) data, att[i]->attalign);

		if (att[i]->attbyval)
		{
			/* pass-by-value */
			store_att_byval(data, values[i], att[i]->attlen);
			data_length = att[i]->attlen;
		}
		else if (att[i]->attlen == -1)
		{
			/* varlena */
			*infomask |= HEAP_HASVARWIDTH;
			if (VARATT_IS_EXTERNAL(values[i]))
				*infomask |= HEAP_HASEXTERNAL;
			if (VARATT_IS_COMPRESSED(values[i]))
				*infomask |= HEAP_HASCOMPRESSED;
			data_length = VARATT_SIZE(DatumGetPointer(values[i]));
			memcpy(data, DatumGetPointer(values[i]), data_length);
		}
		else if (att[i]->attlen == -2)
		{
			/* cstring */
			*infomask |= HEAP_HASVARWIDTH;
			data_length = strlen(DatumGetCString(values[i])) + 1;
			memcpy(data, DatumGetPointer(values[i]), data_length);
		}
		else
		{
			/* fixed-length pass-by-reference */
			Assert(att[i]->attlen > 0);
			data_length = att[i]->attlen;
			memcpy(data, DatumGetPointer(values[i]), data_length);
		}

		data += data_length;
	}
}

/* ----------------------------------------------------------------
 *						heap tuple interface
 * ----------------------------------------------------------------
 */

/* ----------------
 *		heap_attisnull	- returns 1 iff tuple attribute is not present
 * ----------------
 */
int
heap_attisnull(HeapTuple tup, int attnum)
{
	if (attnum > (int) tup->t_data->t_natts)
		return 1;

	if (attnum > 0)
	{
		if (HeapTupleNoNulls(tup))
			return 0;
		return att_isnull(attnum - 1, tup->t_data->t_bits);
	}

	switch (attnum)
	{
		case TableOidAttributeNumber:
		case SelfItemPointerAttributeNumber:
		case ObjectIdAttributeNumber:
		case MinTransactionIdAttributeNumber:
		case MinCommandIdAttributeNumber:
		case MaxTransactionIdAttributeNumber:
		case MaxCommandIdAttributeNumber:
			/* these are never null */
			break;

		default:
			elog(ERROR, "invalid attnum: %d", attnum);
	}

	return 0;
}

/* ----------------
 *		nocachegetattr
 *
 *		This only gets called from fastgetattr() macro, in cases where
 *		we can't use a cacheoffset and the value is not null.
 *
 *		This caches attribute offsets in the attribute descriptor.
 *
 *		An alternate way to speed things up would be to cache offsets
 *		with the tuple, but that seems more difficult unless you take
 *		the storage hit of actually putting those offsets into the
 *		tuple you send to disk.  Yuck.
 *
 *		This scheme will be slightly slower than that, but should
 *		perform well for queries which hit large #'s of tuples.  After
 *		you cache the offsets once, examining all the other tuples using
 *		the same attribute descriptor will go much quicker. -cim 5/4/91
 *
 *		NOTE: if you need to change this code, see also heap_deformtuple.
 * ----------------
 */
Datum
nocachegetattr(HeapTuple tuple,
			   int attnum,
			   TupleDesc tupleDesc,
			   bool *isnull)
{
	HeapTupleHeader tup = tuple->t_data;
	Form_pg_attribute *att = tupleDesc->attrs;
	char	   *tp;				/* ptr to att in tuple */
	bits8	   *bp = tup->t_bits;		/* ptr to null bitmask in tuple */
	bool		slow = false;	/* do we have to walk nulls? */

	(void) isnull;				/* not used */
#ifdef IN_MACRO
/* This is handled in the macro */
	Assert(attnum > 0);

	if (isnull)
		*isnull = false;
#endif

	attnum--;

	/* ----------------
	 *	 Three cases:
	 *
	 *	 1: No nulls and no variable-width attributes.
	 *	 2: Has a null or a var-width AFTER att.
	 *	 3: Has nulls or var-widths BEFORE att.
	 * ----------------
	 */

	if (HeapTupleNoNulls(tuple))
	{
#ifdef IN_MACRO
/* This is handled in the macro */
		if (att[attnum]->attcacheoff != -1)
		{
			return fetchatt(att[attnum],
							(char *) tup + tup->t_hoff +
							att[attnum]->attcacheoff);
		}
#endif
	}
	else
	{
		/*
		 * there's a null somewhere in the tuple
		 *
		 * check to see if desired att is null
		 */

#ifdef IN_MACRO
/* This is handled in the macro */
		if (att_isnull(attnum, bp))
		{
			if (isnull)
				*isnull = true;
			return (Datum) NULL;
		}
#endif

		/*
		 * Now check to see if any preceding bits are null...
		 */
		{
			int			byte = attnum >> 3;
			int			finalbit = attnum & 0x07;

			/* check for nulls "before" final bit of last byte */
			if ((~bp[byte]) & ((1 << finalbit) - 1))
				slow = true;
			else
			{
				/* check for nulls in any "earlier" bytes */
				int			i;

				for (i = 0; i < byte; i++)
				{
					if (bp[i] != 0xFF)
					{
						slow = true;
						break;
					}
				}
			}
		}
	}

	tp = (char *) tup + tup->t_hoff;

	/*
	 * now check for any non-fixed length attrs before our attribute
	 */
	if (!slow)
	{
		if (att[attnum]->attcacheoff != -1)
		{
			return fetchatt(att[attnum],
							tp + att[attnum]->attcacheoff);
		}
		else if (HeapTupleHasVarWidth(tuple))
		{
			int			j;

			/*
			 * In for(), we test <= and not < because we want to see if we
			 * can go past it in initializing offsets.
			 */
			for (j = 0; j <= attnum; j++)
			{
				if (att[j]->attlen <= 0)
				{
					slow = true;
					break;
				}
			}
		}
	}

	/*
	 * If slow is false, and we got here, we know that we have a tuple
	 * with no nulls or var-widths before the target attribute. If
	 * possible, we also want to initialize the remainder of the attribute
	 * cached offset values.
	 */
	if (!slow)
	{
		int			j = 1;
		long		off;

		/*
		 * need to set cache for some atts
		 */

		att[0]->attcacheoff = 0;

		while (j < attnum && att[j]->attcacheoff > 0)
			j++;

		off = att[j - 1]->attcacheoff + att[j - 1]->attlen;

		for (; j <= attnum ||
		/* Can we compute more?  We will probably need them */
			 (j < tup->t_natts &&
			  att[j]->attcacheoff == -1 &&
			  (HeapTupleNoNulls(tuple) || !att_isnull(j, bp)) &&
			  (HeapTupleAllFixed(tuple) || att[j]->attlen > 0)); j++)
		{
			off = att_align(off, att[j]->attalign);

			att[j]->attcacheoff = off;

			off = att_addlength(off, att[j]->attlen, tp + off);
		}

		return fetchatt(att[attnum], tp + att[attnum]->attcacheoff);
	}
	else
	{
		bool		usecache = true;
		int			off = 0;
		int			i;

		/*
		 * Now we know that we have to walk the tuple CAREFULLY.
		 *
		 * Note - This loop is a little tricky.  On iteration i we first set
		 * the offset for attribute i and figure out how much the offset
		 * should be incremented.  Finally, we need to align the offset
		 * based on the size of attribute i+1 (for which the offset has
		 * been computed). -mer 12 Dec 1991
		 */

		for (i = 0; i < attnum; i++)
		{
			if (HeapTupleHasNulls(tuple) && att_isnull(i, bp))
			{
				usecache = false;
				continue;
			}

			/* If we know the next offset, we can skip the rest */
			if (usecache && att[i]->attcacheoff != -1)
				off = att[i]->attcacheoff;
			else
			{
				off = att_align(off, att[i]->attalign);

				if (usecache)
					att[i]->attcacheoff = off;
			}

			off = att_addlength(off, att[i]->attlen, tp + off);

			if (usecache && att[i]->attlen <= 0)
				usecache = false;
		}

		off = att_align(off, att[attnum]->attalign);

		return fetchatt(att[attnum], tp + off);
	}
}

/* ----------------
 *		heap_getsysattr
 *
 *		Fetch the value of a system attribute for a tuple.
 *
 * This is a support routine for the heap_getattr macro.  The macro
 * has already determined that the attnum refers to a system attribute.
 * ----------------
 */
Datum
heap_getsysattr(HeapTuple tup, int attnum, TupleDesc tupleDesc, bool *isnull)
{
	Datum		result;

	Assert(tup);

	/* Currently, no sys attribute ever reads as NULL. */
	if (isnull)
		*isnull = false;

	switch (attnum)
	{
		case SelfItemPointerAttributeNumber:
			/* pass-by-reference datatype */
			result = PointerGetDatum(&(tup->t_self));
			break;
		case ObjectIdAttributeNumber:
			result = ObjectIdGetDatum(HeapTupleGetOid(tup));
			break;
		case MinTransactionIdAttributeNumber:
			result = TransactionIdGetDatum(HeapTupleHeaderGetXmin(tup->t_data));
			break;
		case MinCommandIdAttributeNumber:
			result = CommandIdGetDatum(HeapTupleHeaderGetCmin(tup->t_data));
			break;
		case MaxTransactionIdAttributeNumber:
			result = TransactionIdGetDatum(HeapTupleHeaderGetXmax(tup->t_data));
			break;
		case MaxCommandIdAttributeNumber:
			result = CommandIdGetDatum(HeapTupleHeaderGetCmax(tup->t_data));
			break;
		case TableOidAttributeNumber:
			result = ObjectIdGetDatum(tup->t_tableOid);
			break;

			/*
			 * If the attribute number is 0, then we are supposed to
			 * return the entire tuple as a row-type Datum.  (Using zero
			 * for this purpose is unclean since it risks confusion with
			 * "invalid attr" result codes, but it's not worth changing
			 * now.)
			 *
			 * We have to make a copy of the tuple so we can safely insert
			 * the Datum overhead fields, which are not set in on-disk
			 * tuples.
			 */
		case InvalidAttrNumber:
			{
				HeapTupleHeader dtup;

				dtup = (HeapTupleHeader) palloc(tup->t_len);
				memcpy((char *) dtup, (char *) tup->t_data, tup->t_len);

				HeapTupleHeaderSetDatumLength(dtup, tup->t_len);
				HeapTupleHeaderSetTypeId(dtup, tupleDesc->tdtypeid);
				HeapTupleHeaderSetTypMod(dtup, tupleDesc->tdtypmod);

				result = PointerGetDatum(dtup);
			}
			break;

		default:
			elog(ERROR, "invalid attnum: %d", attnum);
			result = 0;			/* keep compiler quiet */
			break;
	}
	return result;
}

/* ----------------
 *		heap_copytuple
 *
 *		returns a copy of an entire tuple
 *
 * The HeapTuple struct, tuple header, and tuple data are all allocated
 * as a single palloc() block.
 * ----------------
 */
HeapTuple
heap_copytuple(HeapTuple tuple)
{
	HeapTuple	newTuple;

	if (!HeapTupleIsValid(tuple) || tuple->t_data == NULL)
		return NULL;

	newTuple = (HeapTuple) palloc(HEAPTUPLESIZE + tuple->t_len);
	newTuple->t_len = tuple->t_len;
	newTuple->t_self = tuple->t_self;
	newTuple->t_tableOid = tuple->t_tableOid;
	newTuple->t_datamcxt = CurrentMemoryContext;
	newTuple->t_data = (HeapTupleHeader) ((char *) newTuple + HEAPTUPLESIZE);
	memcpy((char *) newTuple->t_data, (char *) tuple->t_data, tuple->t_len);
	return newTuple;
}

/* ----------------
 *		heap_copytuple_with_tuple
 *
 *		copy a tuple into a caller-supplied HeapTuple management struct
 * ----------------
 */
void
heap_copytuple_with_tuple(HeapTuple src, HeapTuple dest)
{
	if (!HeapTupleIsValid(src) || src->t_data == NULL)
	{
		dest->t_data = NULL;
		return;
	}

	dest->t_len = src->t_len;
	dest->t_self = src->t_self;
	dest->t_tableOid = src->t_tableOid;
	dest->t_datamcxt = CurrentMemoryContext;
	dest->t_data = (HeapTupleHeader) palloc(src->t_len);
	memcpy((char *) dest->t_data, (char *) src->t_data, src->t_len);
}

/* ----------------
 *		heap_formtuple
 *
 *		construct a tuple from the given values[] and nulls[] arrays
 *
 *		Null attributes are indicated by a 'n' in the appropriate byte
 *		of nulls[]. Non-null attributes are indicated by a ' ' (space).
 * ----------------
 */
HeapTuple
heap_formtuple(TupleDesc tupleDescriptor,
			   Datum *values,
			   char *nulls)
{
	HeapTuple	tuple;			/* return tuple */
	HeapTupleHeader td;			/* tuple data */
	unsigned long len;
	int			hoff;
	bool		hasnull = false;
	Form_pg_attribute *att = tupleDescriptor->attrs;
	int			numberOfAttributes = tupleDescriptor->natts;
	int			i;

	if (numberOfAttributes > MaxTupleAttributeNumber)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_COLUMNS),
				 errmsg("number of columns (%d) exceeds limit (%d)",
						numberOfAttributes, MaxTupleAttributeNumber)));

	/*
	 * Check for nulls and embedded tuples; expand any toasted attributes
	 * in embedded tuples.	This preserves the invariant that toasting can
	 * only go one level deep.
	 *
	 * We can skip calling toast_flatten_tuple_attribute() if the attribute
	 * couldn't possibly be of composite type.  All composite datums are
	 * varlena and have alignment 'd'; furthermore they aren't arrays.
	 * Also, if an attribute is already toasted, it must have been sent to
	 * disk already and so cannot contain toasted attributes.
	 */
	for (i = 0; i < numberOfAttributes; i++)
	{
		if (nulls[i] != ' ')
			hasnull = true;
		else if (att[i]->attlen == -1 &&
				 att[i]->attalign == 'd' &&
				 att[i]->attndims == 0 &&
				 !VARATT_IS_EXTENDED(values[i]))
		{
			values[i] = toast_flatten_tuple_attribute(values[i],
													  att[i]->atttypid,
													  att[i]->atttypmod);
		}
	}

	/*
	 * Determine total space needed
	 */
	len = offsetof(HeapTupleHeaderData, t_bits);

	if (hasnull)
		len += BITMAPLEN(numberOfAttributes);

	if (tupleDescriptor->tdhasoid)
		len += sizeof(Oid);

	hoff = len = MAXALIGN(len); /* align user data safely */

	len += ComputeDataSize(tupleDescriptor, values, nulls);

	/*
	 * Allocate and zero the space needed.	Note that the tuple body and
	 * HeapTupleData management structure are allocated in one chunk.
	 */
	tuple = (HeapTuple) palloc0(HEAPTUPLESIZE + len);
	tuple->t_datamcxt = CurrentMemoryContext;
	tuple->t_data = td = (HeapTupleHeader) ((char *) tuple + HEAPTUPLESIZE);

	/*
	 * And fill in the information.  Note we fill the Datum fields even
	 * though this tuple may never become a Datum.
	 */
	tuple->t_len = len;
	ItemPointerSetInvalid(&(tuple->t_self));
	tuple->t_tableOid = InvalidOid;

	HeapTupleHeaderSetDatumLength(td, len);
	HeapTupleHeaderSetTypeId(td, tupleDescriptor->tdtypeid);
	HeapTupleHeaderSetTypMod(td, tupleDescriptor->tdtypmod);

	td->t_natts = numberOfAttributes;
	td->t_hoff = hoff;

	if (tupleDescriptor->tdhasoid)		/* else leave infomask = 0 */
		td->t_infomask = HEAP_HASOID;

	DataFill((char *) td + hoff,
			 tupleDescriptor,
			 values,
			 nulls,
			 &td->t_infomask,
			 (hasnull ? td->t_bits : NULL));

	return tuple;
}

/* ----------------
 *		heap_modifytuple
 *
 *		forms a new tuple from an old tuple and a set of replacement values.
 *		returns a new palloc'ed tuple.
 *
 *		XXX it is misdesign that this is passed a Relation and not just a
 *		TupleDesc to describe the tuple structure.
 * ----------------
 */
HeapTuple
heap_modifytuple(HeapTuple tuple,
				 Relation relation,
				 Datum *replValues,
				 char *replNulls,
				 char *replActions)
{
	TupleDesc	tupleDesc = RelationGetDescr(relation);
	int			numberOfAttributes = tupleDesc->natts;
	int			attoff;
	Datum	   *values;
	char	   *nulls;
	HeapTuple	newTuple;

	/*
	 * allocate and fill values and nulls arrays from either the tuple or
	 * the repl information, as appropriate.
	 *
	 * NOTE: it's debatable whether to use heap_deformtuple() here or just
	 * heap_getattr() only the non-replaced colums.  The latter could win
	 * if there are many replaced columns and few non-replaced ones.
	 * However, heap_deformtuple costs only O(N) while the heap_getattr
	 * way would cost O(N^2) if there are many non-replaced columns, so it
	 * seems better to err on the side of linear cost.
	 */
	values = (Datum *) palloc(numberOfAttributes * sizeof(Datum));
	nulls = (char *) palloc(numberOfAttributes * sizeof(char));

	heap_deformtuple(tuple, tupleDesc, values, nulls);

	for (attoff = 0; attoff < numberOfAttributes; attoff++)
	{
		if (replActions[attoff] == 'r')
		{
			values[attoff] = replValues[attoff];
			nulls[attoff] = replNulls[attoff];
		}
		else if (replActions[attoff] != ' ')
			elog(ERROR, "unrecognized replace flag: %d",
				 (int) replActions[attoff]);
	}

	/*
	 * create a new tuple from the values and nulls arrays
	 */
	newTuple = heap_formtuple(tupleDesc, values, nulls);

	pfree(values);
	pfree(nulls);

	/*
	 * copy the identification info of the old tuple: t_ctid, t_self, and
	 * OID (if any)
	 */
	newTuple->t_data->t_ctid = tuple->t_data->t_ctid;
	newTuple->t_self = tuple->t_self;
	newTuple->t_tableOid = tuple->t_tableOid;
	if (tupleDesc->tdhasoid)
		HeapTupleSetOid(newTuple, HeapTupleGetOid(tuple));

	return newTuple;
}

/* ----------------
 *		heap_deformtuple
 *
 *		Given a tuple, extract data into values/nulls arrays; this is
 *		the inverse of heap_formtuple.
 *
 *		Storage for the values/nulls arrays is provided by the caller;
 *		it should be sized according to tupleDesc->natts not tuple->t_natts.
 *
 *		Note that for pass-by-reference datatypes, the pointer placed
 *		in the Datum will point into the given tuple.
 *
 *		When all or most of a tuple's fields need to be extracted,
 *		this routine will be significantly quicker than a loop around
 *		heap_getattr; the loop will become O(N^2) as soon as any
 *		noncacheable attribute offsets are involved.
 * ----------------
 */
void
heap_deformtuple(HeapTuple tuple,
				 TupleDesc tupleDesc,
				 Datum *values,
				 char *nulls)
{
	HeapTupleHeader tup = tuple->t_data;
	Form_pg_attribute *att = tupleDesc->attrs;
	int			tdesc_natts = tupleDesc->natts;
	int			natts;			/* number of atts to extract */
	int			attnum;
	char	   *tp;				/* ptr to tuple data */
	long		off;			/* offset in tuple data */
	bits8	   *bp = tup->t_bits;		/* ptr to null bitmask in tuple */
	bool		slow = false;	/* can we use/set attcacheoff? */

	natts = tup->t_natts;

	/*
	 * In inheritance situations, it is possible that the given tuple
	 * actually has more fields than the caller is expecting.  Don't run
	 * off the end of the caller's arrays.
	 */
	natts = Min(natts, tdesc_natts);

	tp = (char *) tup + tup->t_hoff;

	off = 0;

	for (attnum = 0; attnum < natts; attnum++)
	{
		if (HeapTupleHasNulls(tuple) && att_isnull(attnum, bp))
		{
			values[attnum] = (Datum) 0;
			nulls[attnum] = 'n';
			slow = true;		/* can't use attcacheoff anymore */
			continue;
		}

		nulls[attnum] = ' ';

		if (!slow && att[attnum]->attcacheoff >= 0)
			off = att[attnum]->attcacheoff;
		else
		{
			off = att_align(off, att[attnum]->attalign);

			if (!slow)
				att[attnum]->attcacheoff = off;
		}

		values[attnum] = fetchatt(att[attnum], tp + off);

		off = att_addlength(off, att[attnum]->attlen, tp + off);

		if (att[attnum]->attlen <= 0)
			slow = true;		/* can't use attcacheoff anymore */
	}

	/*
	 * If tuple doesn't have all the atts indicated by tupleDesc, read the
	 * rest as null
	 */
	for (; attnum < tdesc_natts; attnum++)
	{
		values[attnum] = (Datum) 0;
		nulls[attnum] = 'n';
	}
}

/* ----------------
 *		heap_freetuple
 * ----------------
 */
void
heap_freetuple(HeapTuple htup)
{
	if (htup->t_data != NULL)
		if (htup->t_datamcxt != NULL && (char *) (htup->t_data) !=
			((char *) htup + HEAPTUPLESIZE))
			pfree(htup->t_data);

	pfree(htup);
}


/* ----------------
 *		heap_addheader
 *
 * This routine forms a HeapTuple by copying the given structure (tuple
 * data) and adding a generic header.  Note that the tuple data is
 * presumed to contain no null fields and no varlena fields.
 *
 * This routine is really only useful for certain system tables that are
 * known to be fixed-width and null-free.  It is used in some places for
 * pg_class, but that is a gross hack (it only works because relacl can
 * be omitted from the tuple entirely in those places).
 * ----------------
 */
HeapTuple
heap_addheader(int natts,		/* max domain index */
			   bool withoid,	/* reserve space for oid */
			   Size structlen,	/* its length */
			   void *structure) /* pointer to the struct */
{
	HeapTuple	tuple;
	HeapTupleHeader td;
	Size		len;
	int			hoff;

	AssertArg(natts > 0);

	/* header needs no null bitmap */
	hoff = offsetof(HeapTupleHeaderData, t_bits);
	if (withoid)
		hoff += sizeof(Oid);
	hoff = MAXALIGN(hoff);
	len = hoff + structlen;

	tuple = (HeapTuple) palloc0(HEAPTUPLESIZE + len);
	tuple->t_datamcxt = CurrentMemoryContext;
	tuple->t_data = td = (HeapTupleHeader) ((char *) tuple + HEAPTUPLESIZE);

	tuple->t_len = len;
	ItemPointerSetInvalid(&(tuple->t_self));
	tuple->t_tableOid = InvalidOid;

	/* we don't bother to fill the Datum fields */

	td->t_natts = natts;
	td->t_hoff = hoff;

	if (withoid)				/* else leave infomask = 0 */
		td->t_infomask = HEAP_HASOID;

	memcpy((char *) td + hoff, structure, structlen);

	return tuple;
}
