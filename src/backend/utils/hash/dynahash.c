/*-------------------------------------------------------------------------
 *
 * dynahash.c
 *	  dynamic hashing
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/hash/dynahash.c,v 1.35 2001-03-22 03:59:59 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 *
 * Dynamic hashing, after CACM April 1988 pp 446-457, by Per-Ake Larson.
 * Coded into C, with minor code improvements, and with hsearch(3) interface,
 * by ejp@ausmelb.oz, Jul 26, 1988: 13:16;
 * also, hcreate/hdestroy routines added to simulate hsearch(3).
 *
 * These routines simulate hsearch(3) and family, with the important
 * difference that the hash table is dynamic - can grow indefinitely
 * beyond its original size (as supplied to hcreate()).
 *
 * Performance appears to be comparable to that of hsearch(3).
 * The 'source-code' options referred to in hsearch(3)'s 'man' page
 * are not implemented; otherwise functionality is identical.
 *
 * Compilation controls:
 * DEBUG controls some informative traces, mainly for debugging.
 * HASH_STATISTICS causes HashAccesses and HashCollisions to be maintained;
 * when combined with HASH_DEBUG, these are displayed by hdestroy().
 *
 * Problems & fixes to ejp@ausmelb.oz. WARNING: relies on pre-processor
 * concatenation property, in probably unnecessary code 'optimisation'.
 *
 * Modified margo@postgres.berkeley.edu February 1990
 *		added multiple table interface
 * Modified by sullivan@postgres.berkeley.edu April 1990
 *		changed ctl structure for shared memory
 */
#include <sys/types.h>

#include "postgres.h"
#include "utils/dynahash.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"

/*
 * Fast MOD arithmetic, assuming that y is a power of 2 !
 */

#define MOD(x,y)			   ((x) & ((y)-1))

/*
 * Private function prototypes
 */
static void *DynaHashAlloc(Size size);
static uint32 call_hash(HTAB *hashp, char *k);
static SEG_OFFSET seg_alloc(HTAB *hashp);
static int	bucket_alloc(HTAB *hashp);
static int	dir_realloc(HTAB *hashp);
static int	expand_table(HTAB *hashp);
static int	hdefault(HTAB *hashp);
static int	init_htab(HTAB *hashp, int nelem);


/* ----------------
 * memory allocation routines
 *
 * for postgres: all hash elements have to be in
 * the global cache context.  Otherwise the postgres
 * garbage collector is going to corrupt them. -wei
 *
 * ??? the "cache" memory context is intended to store only
 *	   system cache information.  The user of the hashing
 *	   routines should specify which context to use or we
 *	   should create a separate memory context for these
 *	   hash routines.  For now I have modified this code to
 *	   do the latter -cim 1/19/91
 * ----------------
 */
static MemoryContext DynaHashCxt = NULL;

static void *
DynaHashAlloc(Size size)
{
	if (!DynaHashCxt)
		DynaHashCxt = AllocSetContextCreate(TopMemoryContext,
											"DynaHash",
											ALLOCSET_DEFAULT_MINSIZE,
											ALLOCSET_DEFAULT_INITSIZE,
											ALLOCSET_DEFAULT_MAXSIZE);

	return MemoryContextAlloc(DynaHashCxt, size);
}

#define MEM_ALLOC		DynaHashAlloc
#define MEM_FREE		pfree


/*
 * pointer access macros.  Shared memory implementation cannot
 * store pointers in the hash table data structures because
 * pointer values will be different in different address spaces.
 * these macros convert offsets to pointers and pointers to offsets.
 * Shared memory need not be contiguous, but all addresses must be
 * calculated relative to some offset (segbase).
 */

#define GET_SEG(hp,seg_num)\
  (SEGMENT) (((unsigned long) (hp)->segbase) + (hp)->dir[seg_num])

#define GET_BUCKET(hp,bucket_offs)\
  (ELEMENT *) (((unsigned long) (hp)->segbase) + bucket_offs)

#define MAKE_HASHOFFSET(hp,ptr)\
  ( ((unsigned long) ptr) - ((unsigned long) (hp)->segbase) )

#if HASH_STATISTICS
static long hash_accesses,
			hash_collisions,
			hash_expansions;

#endif

/************************** CREATE ROUTINES **********************/

HTAB *
hash_create(int nelem, HASHCTL *info, int flags)
{
	HHDR	   *hctl;
	HTAB	   *hashp;


	hashp = (HTAB *) MEM_ALLOC(sizeof(HTAB));
	MemSet(hashp, 0, sizeof(HTAB));

	if (flags & HASH_FUNCTION)
		hashp->hash = info->hash;
	else
	{
		/* default */
		hashp->hash = string_hash;
	}

	if (flags & HASH_SHARED_MEM)
	{

		/*
		 * ctl structure is preallocated for shared memory tables. Note
		 * that HASH_DIRSIZE had better be set as well.
		 */

		hashp->hctl = (HHDR *) info->hctl;
		hashp->segbase = (char *) info->segbase;
		hashp->alloc = info->alloc;
		hashp->dir = (SEG_OFFSET *) info->dir;

		/* hash table already exists, we're just attaching to it */
		if (flags & HASH_ATTACH)
			return hashp;

	}
	else
	{
		/* setup hash table defaults */

		hashp->hctl = NULL;
		hashp->alloc = MEM_ALLOC;
		hashp->dir = NULL;
		hashp->segbase = NULL;

	}

	if (!hashp->hctl)
	{
		hashp->hctl = (HHDR *) hashp->alloc(sizeof(HHDR));
		if (!hashp->hctl)
			return 0;
	}

	if (!hdefault(hashp))
		return 0;
	hctl = hashp->hctl;
#ifdef HASH_STATISTICS
	hctl->accesses = hctl->collisions = 0;
#endif

	if (flags & HASH_SEGMENT)
	{
		hctl->ssize = info->ssize;
		hctl->sshift = my_log2(info->ssize);
		/* ssize had better be a power of 2 */
		Assert(hctl->ssize == (1L << hctl->sshift));
	}
	if (flags & HASH_FFACTOR)
		hctl->ffactor = info->ffactor;

	/*
	 * SHM hash tables have fixed directory size passed by the caller.
	 */
	if (flags & HASH_DIRSIZE)
	{
		hctl->max_dsize = info->max_dsize;
		hctl->dsize = info->dsize;
	}

	/*
	 * hash table now allocates space for key and data but you have to say
	 * how much space to allocate
	 */
	if (flags & HASH_ELEM)
	{
		hctl->keysize = info->keysize;
		hctl->datasize = info->datasize;
	}
	if (flags & HASH_ALLOC)
		hashp->alloc = info->alloc;

	if (init_htab(hashp, nelem))
	{
		hash_destroy(hashp);
		return 0;
	}
	return hashp;
}

/*
 * Set default HHDR parameters.
 */
static int
hdefault(HTAB *hashp)
{
	HHDR	   *hctl;

	MemSet(hashp->hctl, 0, sizeof(HHDR));

	hctl = hashp->hctl;
	hctl->ssize = DEF_SEGSIZE;
	hctl->sshift = DEF_SEGSIZE_SHIFT;
	hctl->dsize = DEF_DIRSIZE;
	hctl->ffactor = DEF_FFACTOR;
	hctl->nkeys = 0;
	hctl->nsegs = 0;

	/* I added these MS. */

	/* default memory allocation for hash buckets */
	hctl->keysize = sizeof(char *);
	hctl->datasize = sizeof(char *);

	/* table has no fixed maximum size */
	hctl->max_dsize = NO_MAX_DSIZE;

	/* garbage collection for HASH_REMOVE */
	hctl->freeBucketIndex = INVALID_INDEX;

	return 1;
}


static int
init_htab(HTAB *hashp, int nelem)
{
	SEG_OFFSET *segp;
	int			nbuckets;
	int			nsegs;
	HHDR	   *hctl;

	hctl = hashp->hctl;

	/*
	 * Divide number of elements by the fill factor to determine a desired
	 * number of buckets.  Allocate space for the next greater power of
	 * two number of buckets
	 */
	nelem = (nelem - 1) / hctl->ffactor + 1;

	nbuckets = 1 << my_log2(nelem);

	hctl->max_bucket = hctl->low_mask = nbuckets - 1;
	hctl->high_mask = (nbuckets << 1) - 1;

	/*
	 * Figure number of directory segments needed, round up to a power of
	 * 2
	 */
	nsegs = (nbuckets - 1) / hctl->ssize + 1;
	nsegs = 1 << my_log2(nsegs);

	/*
	 * Make sure directory is big enough. If pre-allocated directory is
	 * too small, choke (caller screwed up).
	 */
	if (nsegs > hctl->dsize)
	{
		if (!(hashp->dir))
			hctl->dsize = nsegs;
		else
			return -1;
	}

	/* Allocate a directory */
	if (!(hashp->dir))
	{
		hashp->dir = (SEG_OFFSET *)
			hashp->alloc(hctl->dsize * sizeof(SEG_OFFSET));
		if (!hashp->dir)
			return -1;
	}

	/* Allocate initial segments */
	for (segp = hashp->dir; hctl->nsegs < nsegs; hctl->nsegs++, segp++)
	{
		*segp = seg_alloc(hashp);
		if (*segp == (SEG_OFFSET) 0)
			return -1;
	}

#if HASH_DEBUG
	fprintf(stderr, "%s\n%s%x\n%s%d\n%s%d\n%s%d\n%s%d\n%s%d\n%s%x\n%s%x\n%s%d\n%s%d\n",
			"init_htab:",
			"TABLE POINTER   ", hashp,
			"DIRECTORY SIZE  ", hctl->dsize,
			"SEGMENT SIZE    ", hctl->ssize,
			"SEGMENT SHIFT   ", hctl->sshift,
			"FILL FACTOR     ", hctl->ffactor,
			"MAX BUCKET      ", hctl->max_bucket,
			"HIGH MASK       ", hctl->high_mask,
			"LOW  MASK       ", hctl->low_mask,
			"NSEGS           ", hctl->nsegs,
			"NKEYS           ", hctl->nkeys);
#endif
	return 0;
}

/*
 * Estimate the space needed for a hashtable containing the given number
 * of entries of given size.
 * NOTE: this is used to estimate the footprint of hashtables in shared
 * memory; therefore it does not count HTAB which is in local memory.
 * NB: assumes that all hash structure parameters have default values!
 */
long
hash_estimate_size(long num_entries, long keysize, long datasize)
{
	long		size = 0;
	long		nBuckets,
				nSegments,
				nDirEntries,
				nRecordAllocs,
				recordSize;

	/* estimate number of buckets wanted */
	nBuckets = 1L << my_log2((num_entries - 1) / DEF_FFACTOR + 1);
	/* # of segments needed for nBuckets */
	nSegments = 1L << my_log2((nBuckets - 1) / DEF_SEGSIZE + 1);
	/* directory entries */
	nDirEntries = DEF_DIRSIZE;
	while (nDirEntries < nSegments)
		nDirEntries <<= 1;		/* dir_alloc doubles dsize at each call */

	/* fixed control info */
	size += MAXALIGN(sizeof(HHDR));		/* but not HTAB, per above */
	/* directory */
	size += MAXALIGN(nDirEntries * sizeof(SEG_OFFSET));
	/* segments */
	size += nSegments * MAXALIGN(DEF_SEGSIZE * sizeof(BUCKET_INDEX));
	/* records --- allocated in groups of BUCKET_ALLOC_INCR */
	recordSize = sizeof(BUCKET_INDEX) + keysize + datasize;
	recordSize = MAXALIGN(recordSize);
	nRecordAllocs = (num_entries - 1) / BUCKET_ALLOC_INCR + 1;
	size += nRecordAllocs * BUCKET_ALLOC_INCR * recordSize;

	return size;
}

/*
 * Select an appropriate directory size for a hashtable with the given
 * maximum number of entries.
 * This is only needed for hashtables in shared memory, whose directories
 * cannot be expanded dynamically.
 * NB: assumes that all hash structure parameters have default values!
 *
 * XXX this had better agree with the behavior of init_htab()...
 */
long
hash_select_dirsize(long num_entries)
{
	long		nBuckets,
				nSegments,
				nDirEntries;

	/* estimate number of buckets wanted */
	nBuckets = 1L << my_log2((num_entries - 1) / DEF_FFACTOR + 1);
	/* # of segments needed for nBuckets */
	nSegments = 1L << my_log2((nBuckets - 1) / DEF_SEGSIZE + 1);
	/* directory entries */
	nDirEntries = DEF_DIRSIZE;
	while (nDirEntries < nSegments)
		nDirEntries <<= 1;		/* dir_alloc doubles dsize at each call */

	return nDirEntries;
}


/********************** DESTROY ROUTINES ************************/

/*
 * XXX this sure looks thoroughly broken to me --- tgl 2/99.
 * It's freeing every entry individually --- but they weren't
 * allocated individually, see bucket_alloc!!  Why doesn't it crash?
 * ANSWER: it probably does crash, but is never invoked in normal
 * operations...
 */

void
hash_destroy(HTAB *hashp)
{
	if (hashp != NULL)
	{
		SEG_OFFSET	segNum;
		SEGMENT		segp;
		int			nsegs = hashp->hctl->nsegs;
		int			j;
		BUCKET_INDEX *elp,
					p,
					q;
		ELEMENT    *curr;

		/* cannot destroy a shared memory hash table */
		Assert(!hashp->segbase);
		/* allocation method must be one we know how to free, too */
		Assert(hashp->alloc == MEM_ALLOC);

		hash_stats("destroy", hashp);

		for (segNum = 0; nsegs > 0; nsegs--, segNum++)
		{

			segp = GET_SEG(hashp, segNum);
			for (j = 0, elp = segp; j < hashp->hctl->ssize; j++, elp++)
			{
				for (p = *elp; p != INVALID_INDEX; p = q)
				{
					curr = GET_BUCKET(hashp, p);
					q = curr->next;
					MEM_FREE((char *) curr);
				}
			}
			MEM_FREE((char *) segp);
		}
		MEM_FREE((char *) hashp->dir);
		MEM_FREE((char *) hashp->hctl);
		MEM_FREE((char *) hashp);
	}
}

void
hash_stats(char *where, HTAB *hashp)
{
#if HASH_STATISTICS

	fprintf(stderr, "%s: this HTAB -- accesses %ld collisions %ld\n",
			where, hashp->hctl->accesses, hashp->hctl->collisions);

	fprintf(stderr, "hash_stats: keys %ld keysize %ld maxp %d segmentcount %d\n",
			hashp->hctl->nkeys, hashp->hctl->keysize,
			hashp->hctl->max_bucket, hashp->hctl->nsegs);
	fprintf(stderr, "%s: total accesses %ld total collisions %ld\n",
			where, hash_accesses, hash_collisions);
	fprintf(stderr, "hash_stats: total expansions %ld\n",
			hash_expansions);

#endif

}

/*******************************SEARCH ROUTINES *****************************/

static uint32
call_hash(HTAB *hashp, char *k)
{
	HHDR	   *hctl = hashp->hctl;
	long		hash_val,
				bucket;

	hash_val = hashp->hash(k, (int) hctl->keysize);

	bucket = hash_val & hctl->high_mask;
	if (bucket > hctl->max_bucket)
		bucket = bucket & hctl->low_mask;

	return bucket;
}

/*
 * hash_search -- look up key in table and perform action
 *
 * action is one of HASH_FIND/HASH_ENTER/HASH_REMOVE
 *
 * RETURNS: NULL if table is corrupted, a pointer to the element
 *		found/removed/entered if applicable, TRUE otherwise.
 *		foundPtr is TRUE if we found an element in the table
 *		(FALSE if we entered one).
 */
long *
hash_search(HTAB *hashp,
			char *keyPtr,
			HASHACTION action,	/* HASH_FIND / HASH_ENTER / HASH_REMOVE
								 * HASH_FIND_SAVE / HASH_REMOVE_SAVED */
			bool *foundPtr)
{
	uint32		bucket;
	long		segment_num;
	long		segment_ndx;
	SEGMENT		segp;
	ELEMENT    *curr;
	HHDR	   *hctl;
	BUCKET_INDEX currIndex;
	BUCKET_INDEX *prevIndexPtr;
	char	   *destAddr;
	static struct State
	{
		ELEMENT    *currElem;
		BUCKET_INDEX currIndex;
		BUCKET_INDEX *prevIndex;
	}			saveState;

	Assert((hashp && keyPtr));
	Assert((action == HASH_FIND) || (action == HASH_REMOVE) || (action == HASH_ENTER) || (action == HASH_FIND_SAVE) || (action == HASH_REMOVE_SAVED));

	hctl = hashp->hctl;

#if HASH_STATISTICS
	hash_accesses++;
	hashp->hctl->accesses++;
#endif
	if (action == HASH_REMOVE_SAVED)
	{
		curr = saveState.currElem;
		currIndex = saveState.currIndex;
		prevIndexPtr = saveState.prevIndex;

		/*
		 * Try to catch subsequent errors
		 */
		Assert(saveState.currElem && !(saveState.currElem = 0));
	}
	else
	{
		bucket = call_hash(hashp, keyPtr);
		segment_num = bucket >> hctl->sshift;
		segment_ndx = MOD(bucket, hctl->ssize);

		segp = GET_SEG(hashp, segment_num);

		Assert(segp);

		prevIndexPtr = &segp[segment_ndx];
		currIndex = *prevIndexPtr;

		/*
		 * Follow collision chain
		 */
		for (curr = NULL; currIndex != INVALID_INDEX;)
		{
			/* coerce bucket index into a pointer */
			curr = GET_BUCKET(hashp, currIndex);

			if (!memcmp((char *) &(curr->key), keyPtr, hctl->keysize))
				break;
			prevIndexPtr = &(curr->next);
			currIndex = *prevIndexPtr;
#if HASH_STATISTICS
			hash_collisions++;
			hashp->hctl->collisions++;
#endif
		}
	}

	/*
	 * if we found an entry or if we weren't trying to insert, we're done
	 * now.
	 */
	*foundPtr = (bool) (currIndex != INVALID_INDEX);
	switch (action)
	{
		case HASH_ENTER:
			if (currIndex != INVALID_INDEX)
				return &(curr->key);
			break;
		case HASH_REMOVE:
		case HASH_REMOVE_SAVED:
			if (currIndex != INVALID_INDEX)
			{
				Assert(hctl->nkeys > 0);
				hctl->nkeys--;

				/* remove record from hash bucket's chain. */
				*prevIndexPtr = curr->next;

				/* add the record to the freelist for this table.  */
				curr->next = hctl->freeBucketIndex;
				hctl->freeBucketIndex = currIndex;

				/*
				 * better hope the caller is synchronizing access to this
				 * element, because someone else is going to reuse it the
				 * next time something is added to the table
				 */
				return &(curr->key);
			}
			return (long *) TRUE;
		case HASH_FIND:
			if (currIndex != INVALID_INDEX)
				return &(curr->key);
			return (long *) TRUE;
		case HASH_FIND_SAVE:
			if (currIndex != INVALID_INDEX)
			{
				saveState.currElem = curr;
				saveState.prevIndex = prevIndexPtr;
				saveState.currIndex = currIndex;
				return &(curr->key);
			}
			return (long *) TRUE;
		default:
			/* can't get here */
			return NULL;
	}

	/*
	 * If we got here, then we didn't find the element and we have to
	 * insert it into the hash table
	 */
	Assert(currIndex == INVALID_INDEX);

	/* get the next free bucket */
	currIndex = hctl->freeBucketIndex;
	if (currIndex == INVALID_INDEX)
	{
		/* no free elements.  allocate another chunk of buckets */
		if (!bucket_alloc(hashp))
			return NULL;
		currIndex = hctl->freeBucketIndex;
	}
	Assert(currIndex != INVALID_INDEX);

	curr = GET_BUCKET(hashp, currIndex);
	hctl->freeBucketIndex = curr->next;

	/* link into chain */
	*prevIndexPtr = currIndex;

	/* copy key into record */
	destAddr = (char *) &(curr->key);
	memmove(destAddr, keyPtr, hctl->keysize);
	curr->next = INVALID_INDEX;

	/*
	 * let the caller initialize the data field after hash_search returns.
	 */
	/* memmove(destAddr,keyPtr,hctl->keysize+hctl->datasize); */

	/*
	 * Check if it is time to split the segment
	 */
	if (++hctl->nkeys / (hctl->max_bucket + 1) > hctl->ffactor)
	{

		/*
		 * NOTE: failure to expand table is not a fatal error, it just
		 * means we have to run at higher fill factor than we wanted.
		 */
		expand_table(hashp);
	}
	return &(curr->key);
}

/*
 * hash_seq_init/_search
 *			Sequentially search through hash table and return
 *			all the elements one by one, return NULL on error and
 *			return (long *) TRUE in the end.
 *
 * NOTE: caller may delete the returned element before continuing the scan.
 * However, deleting any other element while the scan is in progress is
 * UNDEFINED (it might be the one that curIndex is pointing at!).  Also,
 * if elements are added to the table while the scan is in progress, it is
 * unspecified whether they will be visited by the scan or not.
 */
void
hash_seq_init(HASH_SEQ_STATUS *status, HTAB *hashp)
{
	status->hashp = hashp;
	status->curBucket = 0;
	status->curIndex = INVALID_INDEX;
}

long *
hash_seq_search(HASH_SEQ_STATUS *status)
{
	HTAB	   *hashp = status->hashp;
	HHDR	   *hctl = hashp->hctl;

	while (status->curBucket <= hctl->max_bucket)
	{
		long		segment_num;
		long		segment_ndx;
		SEGMENT		segp;

		if (status->curIndex != INVALID_INDEX)
		{
			/* Continuing scan of curBucket... */
			ELEMENT    *curElem;

			curElem = GET_BUCKET(hashp, status->curIndex);
			status->curIndex = curElem->next;
			if (status->curIndex == INVALID_INDEX)		/* end of this bucket */
				++status->curBucket;
			return &(curElem->key);
		}

		/*
		 * initialize the search within this bucket.
		 */
		segment_num = status->curBucket >> hctl->sshift;
		segment_ndx = MOD(status->curBucket, hctl->ssize);

		/*
		 * first find the right segment in the table directory.
		 */
		segp = GET_SEG(hashp, segment_num);
		if (segp == NULL)
			/* this is probably an error */
			return (long *) NULL;

		/*
		 * now find the right index into the segment for the first item in
		 * this bucket's chain.  if the bucket is not empty (its entry in
		 * the dir is valid), we know this must correspond to a valid
		 * element and not a freed element because it came out of the
		 * directory of valid stuff.  if there are elements in the bucket
		 * chains that point to the freelist we're in big trouble.
		 */
		status->curIndex = segp[segment_ndx];

		if (status->curIndex == INVALID_INDEX)	/* empty bucket */
			++status->curBucket;
	}

	return (long *) TRUE;		/* out of buckets */
}


/********************************* UTILITIES ************************/

/*
 * Expand the table by adding one more hash bucket.
 */
static int
expand_table(HTAB *hashp)
{
	HHDR	   *hctl;
	SEGMENT		old_seg,
				new_seg;
	long		old_bucket,
				new_bucket;
	long		new_segnum,
				new_segndx;
	long		old_segnum,
				old_segndx;
	ELEMENT    *chain;
	BUCKET_INDEX *old,
			   *newbi;
	BUCKET_INDEX chainIndex,
				nextIndex;

#ifdef HASH_STATISTICS
	hash_expansions++;
#endif

	hctl = hashp->hctl;

	new_bucket = hctl->max_bucket + 1;
	new_segnum = new_bucket >> hctl->sshift;
	new_segndx = MOD(new_bucket, hctl->ssize);

	if (new_segnum >= hctl->nsegs)
	{
		/* Allocate new segment if necessary -- could fail if dir full */
		if (new_segnum >= hctl->dsize)
			if (!dir_realloc(hashp))
				return 0;
		if (!(hashp->dir[new_segnum] = seg_alloc(hashp)))
			return 0;
		hctl->nsegs++;
	}

	/* OK, we created a new bucket */
	hctl->max_bucket++;

	/*
	 * *Before* changing masks, find old bucket corresponding to same hash
	 * values; values in that bucket may need to be relocated to new
	 * bucket. Note that new_bucket is certainly larger than low_mask at
	 * this point, so we can skip the first step of the regular hash mask
	 * calc.
	 */
	old_bucket = (new_bucket & hctl->low_mask);

	/*
	 * If we crossed a power of 2, readjust masks.
	 */
	if (new_bucket > hctl->high_mask)
	{
		hctl->low_mask = hctl->high_mask;
		hctl->high_mask = new_bucket | hctl->low_mask;
	}

	/*
	 * Relocate records to the new bucket.	NOTE: because of the way the
	 * hash masking is done in call_hash, only one old bucket can need to
	 * be split at this point.	With a different way of reducing the hash
	 * value, that might not be true!
	 */
	old_segnum = old_bucket >> hctl->sshift;
	old_segndx = MOD(old_bucket, hctl->ssize);

	old_seg = GET_SEG(hashp, old_segnum);
	new_seg = GET_SEG(hashp, new_segnum);

	old = &old_seg[old_segndx];
	newbi = &new_seg[new_segndx];
	for (chainIndex = *old;
		 chainIndex != INVALID_INDEX;
		 chainIndex = nextIndex)
	{
		chain = GET_BUCKET(hashp, chainIndex);
		nextIndex = chain->next;
		if ((long) call_hash(hashp, (char *) &(chain->key)) == old_bucket)
		{
			*old = chainIndex;
			old = &chain->next;
		}
		else
		{
			*newbi = chainIndex;
			newbi = &chain->next;
		}
	}
	/* don't forget to terminate the rebuilt hash chains... */
	*old = INVALID_INDEX;
	*newbi = INVALID_INDEX;
	return 1;
}


static int
dir_realloc(HTAB *hashp)
{
	char	   *p;
	char	   *old_p;
	long		new_dsize;
	long		old_dirsize;
	long		new_dirsize;

	if (hashp->hctl->max_dsize != NO_MAX_DSIZE)
		return 0;

	/* Reallocate directory */
	new_dsize = hashp->hctl->dsize << 1;
	old_dirsize = hashp->hctl->dsize * sizeof(SEG_OFFSET);
	new_dirsize = new_dsize * sizeof(SEG_OFFSET);

	old_p = (char *) hashp->dir;
	p = (char *) hashp->alloc((Size) new_dirsize);

	if (p != NULL)
	{
		memmove(p, old_p, old_dirsize);
		MemSet(p + old_dirsize, 0, new_dirsize - old_dirsize);
		MEM_FREE((char *) old_p);
		hashp->dir = (SEG_OFFSET *) p;
		hashp->hctl->dsize = new_dsize;
		return 1;
	}
	return 0;
}


static SEG_OFFSET
seg_alloc(HTAB *hashp)
{
	SEGMENT		segp;
	SEG_OFFSET	segOffset;

	segp = (SEGMENT) hashp->alloc(sizeof(BUCKET_INDEX) * hashp->hctl->ssize);

	if (!segp)
		return 0;

	MemSet((char *) segp, 0,
		   (long) sizeof(BUCKET_INDEX) * hashp->hctl->ssize);

	segOffset = MAKE_HASHOFFSET(hashp, segp);
	return segOffset;
}

/*
 * allocate some new buckets and link them into the free list
 */
static int
bucket_alloc(HTAB *hashp)
{
	int			i;
	ELEMENT    *tmpBucket;
	long		bucketSize;
	BUCKET_INDEX tmpIndex,
				lastIndex;

	/* Each bucket has a BUCKET_INDEX header plus user data. */
	bucketSize = sizeof(BUCKET_INDEX) + hashp->hctl->keysize + hashp->hctl->datasize;

	/* make sure its aligned correctly */
	bucketSize = MAXALIGN(bucketSize);

	tmpBucket = (ELEMENT *) hashp->alloc(BUCKET_ALLOC_INCR * bucketSize);

	if (!tmpBucket)
		return 0;

	/* tmpIndex is the shmem offset into the first bucket of the array */
	tmpIndex = MAKE_HASHOFFSET(hashp, tmpBucket);

	/* set the freebucket list to point to the first bucket */
	lastIndex = hashp->hctl->freeBucketIndex;
	hashp->hctl->freeBucketIndex = tmpIndex;

	/*
	 * initialize each bucket to point to the one behind it. NOTE: loop
	 * sets last bucket incorrectly; we fix below.
	 */
	for (i = 0; i < BUCKET_ALLOC_INCR; i++)
	{
		tmpBucket = GET_BUCKET(hashp, tmpIndex);
		tmpIndex += bucketSize;
		tmpBucket->next = tmpIndex;
	}

	/*
	 * the last bucket points to the old freelist head (which is probably
	 * invalid or we wouldn't be here)
	 */
	tmpBucket->next = lastIndex;

	return 1;
}

/* calculate ceil(log base 2) of num */
int
my_log2(long num)
{
	int			i;
	long		limit;

	for (i = 0, limit = 1; limit < num; i++, limit <<= 1)
		;
	return i;
}
