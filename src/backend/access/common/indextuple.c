/*-------------------------------------------------------------------------
 *
 * indextuple.c
 *	   This file contains index tuple accessor and mutator routines,
 *	   as well as a few various tuple utilities.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/common/indextuple.c,v 1.37 1999-07-17 20:16:35 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/itup.h"
#include "catalog/pg_type.h"


/* ----------------------------------------------------------------
 *				  index_ tuple interface routines
 * ----------------------------------------------------------------
 */

/* ----------------
 *		index_formtuple
 * ----------------
 */
IndexTuple
index_formtuple(TupleDesc tupleDescriptor,
				Datum *value,
				char *null)
{
	char	   *tp;				/* tuple pointer */
	IndexTuple	tuple;			/* return tuple */
	Size		size,
				hoff;
	int			i;
	unsigned short infomask = 0;
	bool		hasnull = false;
	uint16		tupmask = 0;
	int			numberOfAttributes = tupleDescriptor->natts;

	if (numberOfAttributes > MaxIndexAttributeNumber)
		elog(ERROR, "index_formtuple: numberOfAttributes of %d > %d",
			 numberOfAttributes, MaxIndexAttributeNumber);


	for (i = 0; i < numberOfAttributes && !hasnull; i++)
	{
		if (null[i] != ' ')
			hasnull = true;
	}

	if (hasnull)
		infomask |= INDEX_NULL_MASK;

	hoff = IndexInfoFindDataOffset(infomask);
	size = hoff + ComputeDataSize(tupleDescriptor, value, null);
	size = DOUBLEALIGN(size);	/* be conservative */

	tp = (char *) palloc(size);
	tuple = (IndexTuple) tp;
	MemSet(tp, 0, (int) size);

	DataFill((char *) tp + hoff,
			 tupleDescriptor,
			 value,
			 null,
			 &tupmask,
			 (hasnull ? (bits8 *) tp + sizeof(*tuple) : NULL));

	/*
	 * We do this because DataFill wants to initialize a "tupmask" which
	 * is used for HeapTuples, but we want an indextuple infomask.	The
	 * only "relevent" info is the "has variable attributes" field, which
	 * is in mask position 0x02.  We have already set the null mask above.
	 */

	if (tupmask & 0x02)
		infomask |= INDEX_VAR_MASK;

	/*
	 * Here we make sure that we can actually hold the size.  We also want
	 * to make sure that size is not aligned oddly.  This actually is a
	 * rather odd way to make sure the size is not too large overall.
	 */

	if (size & 0xE000)
		elog(ERROR, "index_formtuple: data takes %d bytes: too big", size);


	infomask |= size;

	/* ----------------
	 * initialize metadata
	 * ----------------
	 */
	tuple->t_info = infomask;
	return tuple;
}

/* ----------------
 *		nocache_index_getattr
 *
 *		This gets called from index_getattr() macro, and only in cases
 *		where we can't use cacheoffset and the value is not null.
 *
 *		This caches attribute offsets in the attribute descriptor.
 *
 *		an alternate way to speed things up would be to cache offsets
 *		with the tuple, but that seems more difficult unless you take
 *		the storage hit of actually putting those offsets into the
 *		tuple you send to disk.  Yuck.
 *
 *		This scheme will be slightly slower than that, but should
 *		preform well for queries which hit large #'s of tuples.  After
 *		you cache the offsets once, examining all the other tuples using
 *		the same attribute descriptor will go much quicker. -cim 5/4/91
 * ----------------
 */
Datum
nocache_index_getattr(IndexTuple tup,
					  int attnum,
					  TupleDesc tupleDesc,
					  bool *isnull)
{
	char	   *tp;				/* ptr to att in tuple */
	char	   *bp = NULL;		/* ptr to att in tuple */
	int			slow;			/* do we have to walk nulls? */
	int			data_off;		/* tuple data offset */
	Form_pg_attribute *att = tupleDesc->attrs;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */

	/* ----------------
	 *	 Three cases:
	 *
	 *	 1: No nulls and no variable length attributes.
	 *	 2: Has a null or a varlena AFTER att.
	 *	 3: Has nulls or varlenas BEFORE att.
	 * ----------------
	 */

#ifdef IN_MACRO
/* This is handled in the macro */
	Assert(PointerIsValid(isnull));
	Assert(attnum > 0);

	*isnull = false;
#endif

	data_off = IndexTupleHasMinHeader(tup) ? sizeof *tup :
		IndexInfoFindDataOffset(tup->t_info);

	if (IndexTupleNoNulls(tup))
	{
		attnum--;

#ifdef IN_MACRO
/* This is handled in the macro */

		/* first attribute is always at position zero */

		if (attnum == 1)
			return (Datum) fetchatt(&(att[0]), (char *) tup + data_off);
		if (att[attnum]->attcacheoff != -1)
		{
			return (Datum) fetchatt(&(att[attnum]),
									(char *) tup + data_off +
									att[attnum]->attcacheoff);
		}
#endif

		slow = 0;
	}
	else
	{							/* there's a null somewhere in the tuple */

		slow = 0;
		/* ----------------
		 *		check to see if desired att is null
		 * ----------------
		 */

		attnum--;

		bp = (char *) tup + sizeof(*tup);		/* "knows" t_bits are
												 * here! */
#ifdef IN_MACRO
/* This is handled in the macro */

		if (att_isnull(attnum, bp))
		{
			*isnull = true;
			return (Datum) NULL;
		}
#endif

		/* ----------------
		 *		Now check to see if any preceeding bits are null...
		 * ----------------
		 */
		{
			int			i = 0;	/* current offset in bp */
			int			mask;	/* bit in byte we're looking at */
			char		n;		/* current byte in bp */
			int			byte,
						finalbit;

			byte = attnum >> 3;
			finalbit = attnum & 0x07;

			for (; i <= byte && !slow; i++)
			{
				n = bp[i];
				if (i < byte)
				{
					/* check for nulls in any "earlier" bytes */
					if ((~n) != 0)
						slow = 1;
				}
				else
				{
					/* check for nulls "before" final bit of last byte */
					mask = (1 << finalbit) - 1;
					if ((~n) & mask)
						slow = 1;
				}
			}
		}
	}

	tp = (char *) tup + data_off;

	/* now check for any non-fixed length attrs before our attribute */

	if (!slow)
	{
		if (att[attnum]->attcacheoff != -1)
		{
			return (Datum) fetchatt(&(att[attnum]),
									tp + att[attnum]->attcacheoff);
		}
		else if (attnum == 0)
			return (Datum) fetchatt(&(att[0]), (char *) tp);
		else if (!IndexTupleAllFixed(tup))
		{
			int			j = 0;

			for (j = 0; j < attnum && !slow; j++)
				if (att[j]->attlen < 1 && !VARLENA_FIXED_SIZE(att[j]))
					slow = 1;
		}
	}

	/*
	 * if slow is zero, and we got here, we know that we have a tuple with
	 * no nulls.  We also know that we have to initialize the remainder of
	 * the attribute cached offset values.
	 */

	if (!slow)
	{
		int			j = 1;
		long		off;

		/*
		 * need to set cache for some atts
		 */

		att[0]->attcacheoff = 0;

		while (att[j]->attcacheoff != -1)
			j++;

		if (!VARLENA_FIXED_SIZE(att[j - 1]))
			off = att[j - 1]->attcacheoff + att[j - 1]->attlen;
		else
			off = att[j - 1]->attcacheoff + att[j - 1]->atttypmod;

		for (; j < attnum + 1; j++)
		{

			/*
			 * Fix me when going to a machine with more than a four-byte
			 * word!
			 */

			off = att_align(off, att[j]->attlen, att[j]->attalign);

			att[j]->attcacheoff = off;

			/* The only varlena/-1 length value to get here is this */
			if (!VARLENA_FIXED_SIZE(att[j]))
				off += att[j]->attlen;
			else
			{
				Assert(att[j]->atttypmod == VARSIZE(tp + off));
				off += att[j]->atttypmod;
			}
		}

		return (Datum) fetchatt(&(att[attnum]), tp + att[attnum]->attcacheoff);
	}
	else
	{
		bool		usecache = true;
		int			off = 0;
		int			i;

		/*
		 * Now we know that we have to walk the tuple CAREFULLY.
		 */

		for (i = 0; i < attnum; i++)
		{
			if (!IndexTupleNoNulls(tup))
			{
				if (att_isnull(i, bp))
				{
					usecache = false;
					continue;
				}
			}

			/* If we know the next offset, we can skip the rest */
			if (usecache && att[i]->attcacheoff != -1)
				off = att[i]->attcacheoff;
			else
			{
				off = att_align(off, att[i]->attlen, att[i]->attalign);

				if (usecache)
					att[i]->attcacheoff = off;
			}

			switch (att[i]->attlen)
			{
				case sizeof(char):
					off++;
					break;
				case sizeof(short):
					off += sizeof(short);
					break;
				case sizeof(int32):
					off += sizeof(int32);
					break;
				case -1:
					Assert(!VARLENA_FIXED_SIZE(att[i]) ||
						   att[i]->atttypmod == VARSIZE(tp + off));
					off += VARSIZE(tp + off);
					if (!VARLENA_FIXED_SIZE(att[i]))
						usecache = false;
					break;
				default:
					off += att[i]->attlen;
					break;
			}
		}

		off = att_align(off, att[attnum]->attlen, att[attnum]->attalign);

		return (Datum) fetchatt(&att[attnum], tp + off);
	}
}

RetrieveIndexResult
FormRetrieveIndexResult(ItemPointer indexItemPointer,
						ItemPointer heapItemPointer)
{
	RetrieveIndexResult result;

	Assert(ItemPointerIsValid(indexItemPointer));
	Assert(ItemPointerIsValid(heapItemPointer));

	result = (RetrieveIndexResult) palloc(sizeof *result);

	result->index_iptr = *indexItemPointer;
	result->heap_iptr = *heapItemPointer;

	return result;
}

/*
 * Copies source into target.  If *target == NULL, we palloc space; otherwise
 * we assume we have space that is already palloc'ed.
 */
void
CopyIndexTuple(IndexTuple source, IndexTuple *target)
{
	Size		size;
	IndexTuple	ret;

	size = IndexTupleSize(source);
	if (*target == NULL)
		*target = (IndexTuple) palloc(size);

	ret = *target;
	memmove((char *) ret, (char *) source, size);
}
