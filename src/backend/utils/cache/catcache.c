/*-------------------------------------------------------------------------
 *
 * catcache.c
 *	  System catalog cache for tuples matching a key.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/cache/catcache.c,v 1.53 1999-11-21 01:58:22 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "access/genam.h"
#include "access/heapam.h"
#include "access/valid.h"
#include "catalog/pg_type.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/syscache.h"

static void CatCacheRemoveCTup(CatCache *cache, Dlelem *e);
static Index CatalogCacheComputeHashIndex(struct catcache * cacheInP);
static Index CatalogCacheComputeTupleHashIndex(struct catcache * cacheInOutP,
								  Relation relation, HeapTuple tuple);
static void CatalogCacheInitializeCache(struct catcache * cache,
							Relation relation);
static long comphash(long l, char *v);

/* ----------------
 *		variables, macros and other stuff
 *
 *	note CCSIZE allocates 51 buckets .. one was already allocated in
 *	the catcache structure.
 * ----------------
 */

#ifdef CACHEDEBUG
#define CACHE1_elog(a,b)				elog(a,b)
#define CACHE2_elog(a,b,c)				elog(a,b,c)
#define CACHE3_elog(a,b,c,d)			elog(a,b,c,d)
#define CACHE4_elog(a,b,c,d,e)			elog(a,b,c,d,e)
#define CACHE5_elog(a,b,c,d,e,f)		elog(a,b,c,d,e,f)
#define CACHE6_elog(a,b,c,d,e,f,g)		elog(a,b,c,d,e,f,g)
#else
#define CACHE1_elog(a,b)
#define CACHE2_elog(a,b,c)
#define CACHE3_elog(a,b,c,d)
#define CACHE4_elog(a,b,c,d,e)
#define CACHE5_elog(a,b,c,d,e,f)
#define CACHE6_elog(a,b,c,d,e,f,g)
#endif

static CatCache   *Caches = NULL; /* head of list of caches */

GlobalMemory CacheCxt;			/* context in which caches are allocated */
/* CacheCxt is global because relcache uses it too. */


/* ----------------
 *		EQPROC is used in CatalogCacheInitializeCache
 *		XXX this should be replaced by catalog lookups soon
 * ----------------
 */
static long eqproc[] = {
	F_BOOLEQ, 0l, F_CHAREQ, F_NAMEEQ, 0l,
	F_INT2EQ, F_KEYFIRSTEQ, F_INT4EQ, 0l, F_TEXTEQ,
	F_OIDEQ, 0l, 0l, 0l, F_OID8EQ
};

#define EQPROC(SYSTEMTYPEOID)	eqproc[(SYSTEMTYPEOID)-16]

/* ----------------------------------------------------------------
 *					internal support functions
 * ----------------------------------------------------------------
 */
/* --------------------------------
 *		CatalogCacheInitializeCache
 * --------------------------------
 */
#ifdef CACHEDEBUG
#define CatalogCacheInitializeCache_DEBUG1 \
do { \
	elog(DEBUG, "CatalogCacheInitializeCache: cache @%08lx", cache); \
	if (relation) \
		elog(DEBUG, "CatalogCacheInitializeCache: called w/relation(inval)"); \
	else \
		elog(DEBUG, "CatalogCacheInitializeCache: called w/relname %s", \
			cache->cc_relname) \
} while(0)

#define CatalogCacheInitializeCache_DEBUG2 \
do { \
		if (cache->cc_key[i] > 0) { \
			elog(DEBUG, "CatalogCacheInitializeCache: load %d/%d w/%d, %d", \
				i+1, cache->cc_nkeys, cache->cc_key[i], \
				relation->rd_att->attrs[cache->cc_key[i] - 1]->attlen); \
		} else { \
			elog(DEBUG, "CatalogCacheInitializeCache: load %d/%d w/%d", \
				i+1, cache->cc_nkeys, cache->cc_key[i]); \
		} \
} while(0)

#else
#define CatalogCacheInitializeCache_DEBUG1
#define CatalogCacheInitializeCache_DEBUG2
#endif

static void
CatalogCacheInitializeCache(struct catcache * cache,
							Relation relation)
{
	MemoryContext oldcxt;
	short		didopen = 0;
	short		i;
	TupleDesc	tupdesc;

	CatalogCacheInitializeCache_DEBUG1;

	/* ----------------
	 *	first switch to the cache context so our allocations
	 *	do not vanish at the end of a transaction
	 * ----------------
	 */
	if (!CacheCxt)
		CacheCxt = CreateGlobalMemory("Cache");
	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/* ----------------
	 *	If no relation was passed we must open it to get access to
	 *	its fields.  If one of the other caches has already opened
	 *	it we use heap_open() instead of heap_openr().
	 *	XXX is that really worth the trouble of checking?
	 * ----------------
	 */
	if (!RelationIsValid(relation))
	{
		struct catcache *cp;

		/* ----------------
		 *	scan the caches to see if any other cache has opened the relation
		 * ----------------
		 */
		for (cp = Caches; cp; cp = cp->cc_next)
		{
			if (strncmp(cp->cc_relname, cache->cc_relname, NAMEDATALEN) == 0)
			{
				if (cp->relationId != InvalidOid)
					break;
			}
		}

		/* ----------------
		 *	open the relation by name or by id
		 * ----------------
		 */
		if (cp)
			relation = heap_open(cp->relationId, NoLock);
		else
			relation = heap_openr(cache->cc_relname, NoLock);

		didopen = 1;
	}

	/* ----------------
	 *	initialize the cache's relation id
	 * ----------------
	 */
	Assert(RelationIsValid(relation));
	cache->relationId = RelationGetRelid(relation);
	tupdesc = cache->cc_tupdesc = RelationGetDescr(relation);

	CACHE3_elog(DEBUG, "CatalogCacheInitializeCache: relid %u, %d keys",
				cache->relationId, cache->cc_nkeys);

	/* ----------------
	 *	initialize cache's key information
	 * ----------------
	 */
	for (i = 0; i < cache->cc_nkeys; ++i)
	{
		CatalogCacheInitializeCache_DEBUG2;

		if (cache->cc_key[i] > 0)
		{

			/*
			 * Yoiks.  The implementation of the hashing code and the
			 * implementation of int28's are at loggerheads.  The right
			 * thing to do is to throw out the implementation of int28's
			 * altogether; until that happens, we do the right thing here
			 * to guarantee that the hash key generator doesn't try to
			 * dereference an int2 by mistake.
			 */

			if (tupdesc->attrs[cache->cc_key[i] - 1]->atttypid == INT28OID)
				cache->cc_klen[i] = sizeof(short);
			else
				cache->cc_klen[i] = tupdesc->attrs[cache->cc_key[i] - 1]->attlen;

			cache->cc_skey[i].sk_procedure = EQPROC(tupdesc->attrs[cache->cc_key[i] - 1]->atttypid);

			fmgr_info(cache->cc_skey[i].sk_procedure,
					  &cache->cc_skey[i].sk_func);
			cache->cc_skey[i].sk_nargs = cache->cc_skey[i].sk_func.fn_nargs;

			CACHE5_elog(DEBUG, "CatalogCacheInit %s %d %d %x",
						RelationGetRelationName(relation),
						i,
						tupdesc->attrs[cache->cc_key[i] - 1]->attlen,
						cache);
		}
	}

	/* ----------------
	 *	close the relation if we opened it
	 * ----------------
	 */
	if (didopen)
		heap_close(relation, NoLock);

	/* ----------------
	 *	initialize index information for the cache.  this
	 *	should only be done once per cache.
	 * ----------------
	 */
	if (cache->cc_indname != NULL && cache->indexId == InvalidOid)
	{
		if (RelationGetForm(relation)->relhasindex)
		{

			/*
			 * If the index doesn't exist we are in trouble.
			 */
			relation = index_openr(cache->cc_indname);
			Assert(relation);
			cache->indexId = RelationGetRelid(relation);
			index_close(relation);
		}
		else
			cache->cc_indname = NULL;
	}

	/* ----------------
	 *	return to the proper memory context
	 * ----------------
	 */
	MemoryContextSwitchTo(oldcxt);
}

/* --------------------------------
 *		CatalogCacheSetId
 *
 *		XXX temporary function
 * --------------------------------
 */
#ifdef NOT_USED
void
CatalogCacheSetId(CatCache *cacheInOutP, int id)
{
	Assert(id == InvalidCatalogCacheId || id >= 0);
	cacheInOutP->id = id;
}

#endif

/* ----------------
 * comphash
 *		Compute a hash value, somehow.
 *
 * XXX explain algorithm here.
 *
 * l is length of the attribute value, v
 * v is the attribute value ("Datum")
 * ----------------
 */
static long
comphash(long l, char *v)
{
	long		i;
	NameData	n;

	CACHE3_elog(DEBUG, "comphash (%d,%x)", l, v);

	switch (l)
	{
		case 1:
		case 2:
		case 4:
			return (long) v;
	}

	if (l == NAMEDATALEN)
	{

		/*
		 * if it's a name, make sure that the values are null-padded.
		 *
		 * Note that this other fixed-length types can also have the same
		 * typelen so this may break them	  - XXX
		 */
		namestrcpy(&n, v);
		v = NameStr(n);
	}
	else if (l < 0)
		l = VARSIZE(v);

	i = 0;
	while (l--)
		i += *v++;
	return i;
}

/* --------------------------------
 *		CatalogCacheComputeHashIndex
 * --------------------------------
 */
static Index
CatalogCacheComputeHashIndex(struct catcache * cacheInP)
{
	Index		hashIndex;

	hashIndex = 0x0;
	CACHE6_elog(DEBUG, "CatalogCacheComputeHashIndex %s %d %d %d %x",
				cacheInP->cc_relname,
				cacheInP->cc_nkeys,
				cacheInP->cc_klen[0],
				cacheInP->cc_klen[1],
				cacheInP);

	switch (cacheInP->cc_nkeys)
	{
		case 4:
			hashIndex ^= comphash(cacheInP->cc_klen[3],
						 (char *) cacheInP->cc_skey[3].sk_argument) << 9;
			/* FALLTHROUGH */
		case 3:
			hashIndex ^= comphash(cacheInP->cc_klen[2],
						 (char *) cacheInP->cc_skey[2].sk_argument) << 6;
			/* FALLTHROUGH */
		case 2:
			hashIndex ^= comphash(cacheInP->cc_klen[1],
						 (char *) cacheInP->cc_skey[1].sk_argument) << 3;
			/* FALLTHROUGH */
		case 1:
			hashIndex ^= comphash(cacheInP->cc_klen[0],
							  (char *) cacheInP->cc_skey[0].sk_argument);
			break;
		default:
			elog(FATAL, "CCComputeHashIndex: %d cc_nkeys", cacheInP->cc_nkeys);
			break;
	}
	hashIndex %= cacheInP->cc_size;
	return hashIndex;
}

/* --------------------------------
 *		CatalogCacheComputeTupleHashIndex
 * --------------------------------
 */
static Index
CatalogCacheComputeTupleHashIndex(struct catcache * cacheInOutP,
								  Relation relation,
								  HeapTuple tuple)
{
	bool		isNull = '\0';

	if (cacheInOutP->relationId == InvalidOid)
		CatalogCacheInitializeCache(cacheInOutP, relation);
	switch (cacheInOutP->cc_nkeys)
	{
		case 4:
			cacheInOutP->cc_skey[3].sk_argument =
				(cacheInOutP->cc_key[3] == ObjectIdAttributeNumber)
				? (Datum) tuple->t_data->t_oid
				: fastgetattr(tuple,
							  cacheInOutP->cc_key[3],
							  RelationGetDescr(relation),
							  &isNull);
			Assert(!isNull);
			/* FALLTHROUGH */
		case 3:
			cacheInOutP->cc_skey[2].sk_argument =
				(cacheInOutP->cc_key[2] == ObjectIdAttributeNumber)
				? (Datum) tuple->t_data->t_oid
				: fastgetattr(tuple,
							  cacheInOutP->cc_key[2],
							  RelationGetDescr(relation),
							  &isNull);
			Assert(!isNull);
			/* FALLTHROUGH */
		case 2:
			cacheInOutP->cc_skey[1].sk_argument =
				(cacheInOutP->cc_key[1] == ObjectIdAttributeNumber)
				? (Datum) tuple->t_data->t_oid
				: fastgetattr(tuple,
							  cacheInOutP->cc_key[1],
							  RelationGetDescr(relation),
							  &isNull);
			Assert(!isNull);
			/* FALLTHROUGH */
		case 1:
			cacheInOutP->cc_skey[0].sk_argument =
				(cacheInOutP->cc_key[0] == ObjectIdAttributeNumber)
				? (Datum) tuple->t_data->t_oid
				: fastgetattr(tuple,
							  cacheInOutP->cc_key[0],
							  RelationGetDescr(relation),
							  &isNull);
			Assert(!isNull);
			break;
		default:
			elog(FATAL, "CCComputeTupleHashIndex: %d cc_nkeys",
				 cacheInOutP->cc_nkeys
				);
			break;
	}

	return CatalogCacheComputeHashIndex(cacheInOutP);
}

/* --------------------------------
 *		CatCacheRemoveCTup
 * --------------------------------
 */
static void
CatCacheRemoveCTup(CatCache *cache, Dlelem *elt)
{
	CatCTup    *ct;
	CatCTup    *other_ct;
	Dlelem	   *other_elt;

	if (elt)
		ct = (CatCTup *) DLE_VAL(elt);
	else
		return;

	other_elt = ct->ct_node;
	other_ct = (CatCTup *) DLE_VAL(other_elt);
	DLRemove(other_elt);
	DLFreeElem(other_elt);
	free(other_ct);
	DLRemove(elt);
	DLFreeElem(elt);
	free(ct);
	--cache->cc_ntup;
}

/* --------------------------------
 *	CatalogCacheIdInvalidate()
 *
 *	Invalidate a tuple given a cache id.  In this case the id should always
 *	be found (whether the cache has opened its relation or not).  Of course,
 *	if the cache has yet to open its relation, there will be no tuples so
 *	no problem.
 * --------------------------------
 */
void
CatalogCacheIdInvalidate(int cacheId,	/* XXX */
						 Index hashIndex,
						 ItemPointer pointer)
{
	CatCache   *ccp;
	CatCTup    *ct;
	Dlelem	   *elt;
	MemoryContext oldcxt;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	Assert(hashIndex < NCCBUCK);
	Assert(ItemPointerIsValid(pointer));
	CACHE1_elog(DEBUG, "CatalogCacheIdInvalidate: called");

	/* ----------------
	 *	switch to the cache context for our memory allocations
	 * ----------------
	 */
	if (!CacheCxt)
		CacheCxt = CreateGlobalMemory("Cache");
	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/* ----------------
	 *	inspect every cache that could contain the tuple
	 * ----------------
	 */
	for (ccp = Caches; ccp; ccp = ccp->cc_next)
	{
		if (cacheId != ccp->id)
			continue;
		/* ----------------
		 *	inspect the hash bucket until we find a match or exhaust
		 * ----------------
		 */
		for (elt = DLGetHead(ccp->cc_cache[hashIndex]);
			 elt;
			 elt = DLGetSucc(elt))
		{
			ct = (CatCTup *) DLE_VAL(elt);
			if (ItemPointerEquals(pointer, &ct->ct_tup->t_self))
				break;
		}

		/* ----------------
		 *	if we found a matching tuple, invalidate it.
		 * ----------------
		 */

		if (elt)
		{
			CatCacheRemoveCTup(ccp, elt);

			CACHE1_elog(DEBUG, "CatalogCacheIdInvalidate: invalidated");
		}

		if (cacheId != InvalidCatalogCacheId)
			break;
	}

	/* ----------------
	 *	return to the proper memory context
	 * ----------------
	 */
	MemoryContextSwitchTo(oldcxt);
	/* sendpm('I', "Invalidated tuple"); */
}

/* ----------------------------------------------------------------
 *					   public functions
 *
 *		ResetSystemCache
 *		InitIndexedSysCache
 *		InitSysCache
 *		SearchSysCache
 *		RelationInvalidateCatalogCacheTuple
 * ----------------------------------------------------------------
 */
/* --------------------------------
 *		ResetSystemCache
 * --------------------------------
 */
void
ResetSystemCache()
{
	MemoryContext oldcxt;
	struct catcache *cache;

	CACHE1_elog(DEBUG, "ResetSystemCache called");

	/* ----------------
	 *	first switch to the cache context so our allocations
	 *	do not vanish at the end of a transaction
	 * ----------------
	 */
	if (!CacheCxt)
		CacheCxt = CreateGlobalMemory("Cache");

	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/* ----------------
	 *	here we purge the contents of all the caches
	 *
	 *	for each system cache
	 *	   for each hash bucket
	 *		   for each tuple in hash bucket
	 *			   remove the tuple
	 * ----------------
	 */
	for (cache = Caches; PointerIsValid(cache); cache = cache->cc_next)
	{
		int			hash;

		for (hash = 0; hash < NCCBUCK; hash += 1)
		{
			Dlelem	   *elt,
					   *nextelt;

			for (elt = DLGetHead(cache->cc_cache[hash]); elt; elt = nextelt)
			{
				nextelt = DLGetSucc(elt);
				CatCacheRemoveCTup(cache, elt);
				if (cache->cc_ntup < 0)
					elog(NOTICE,
						 "ResetSystemCache: cc_ntup<0 (software error)");
			}
		}
		cache->cc_ntup = 0;		/* in case of WARN error above */
		cache->busy = false;	/* to recover from recursive-use error */
	}

	CACHE1_elog(DEBUG, "end of ResetSystemCache call");

	/* ----------------
	 *	back to the old context before we return...
	 * ----------------
	 */
	MemoryContextSwitchTo(oldcxt);
}

/* --------------------------------
 *		SystemCacheRelationFlushed
 *
 *	This is called by RelationFlushRelation() to clear out cached information
 *	about a relation being dropped.  (This could be a DROP TABLE command,
 *	or a temp table being dropped at end of transaction, or a table created
 *	during the current transaction that is being dropped because of abort.)
 *	Remove all cache entries relevant to the specified relation OID.
 *
 *	A special case occurs when relId is itself one of the cacheable system
 *	tables --- although those'll never be dropped, they can get flushed from
 *	the relcache (VACUUM causes this, for example).  In that case we need to
 *	force the next SearchSysCache() call to reinitialize the cache itself,
 *	because we have info (such as cc_tupdesc) that is pointing at the about-
 *	to-be-deleted relcache entry.
 * --------------------------------
 */
void
SystemCacheRelationFlushed(Oid relId)
{
	struct catcache *cache;

	/*
	 * XXX Ideally we'd search the caches and just zap entries that actually
	 * refer to the indicated relation.  For now, we take the brute-force
	 * approach: just flush the caches entirely.
	 */
	ResetSystemCache();

	/*
	 * If relcache is dropping a system relation's cache entry, mark the
	 * associated cache structures invalid, so we can rebuild them from
	 * scratch (not just repopulate them) next time they are used.
	 */
	for (cache = Caches; PointerIsValid(cache); cache = cache->cc_next)
	{
		if (cache->relationId == relId)
			cache->relationId = InvalidOid;
	}
}

/* --------------------------------
 *		InitIndexedSysCache
 *
 *	This allocates and initializes a cache for a system catalog relation.
 *	Actually, the cache is only partially initialized to avoid opening the
 *	relation.  The relation will be opened and the rest of the cache
 *	structure initialized on the first access.
 * --------------------------------
 */
#ifdef CACHEDEBUG
#define InitSysCache_DEBUG1 \
do { \
	elog(DEBUG, "InitSysCache: rid=%u id=%d nkeys=%d size=%d\n", \
		cp->relationId, cp->id, cp->cc_nkeys, cp->cc_size); \
	for (i = 0; i < nkeys; i += 1) \
	{ \
		elog(DEBUG, "InitSysCache: key=%d len=%d skey=[%d %d %d %d]\n", \
			 cp->cc_key[i], cp->cc_klen[i], \
			 cp->cc_skey[i].sk_flags, \
			 cp->cc_skey[i].sk_attno, \
			 cp->cc_skey[i].sk_procedure, \
			 cp->cc_skey[i].sk_argument); \
	} \
} while(0)

#else
#define InitSysCache_DEBUG1
#endif

CatCache   *
InitSysCache(char *relname,
			 char *iname,
			 int id,
			 int nkeys,
			 int *key,
			 HeapTuple (*iScanfuncP) ())
{
	CatCache   *cp;
	int			i;
	MemoryContext oldcxt;

	char	   *indname;

	indname = (iname) ? iname : NULL;

	/* ----------------
	 *	first switch to the cache context so our allocations
	 *	do not vanish at the end of a transaction
	 * ----------------
	 */
	if (!CacheCxt)
		CacheCxt = CreateGlobalMemory("Cache");

	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/* ----------------
	 *	allocate a new cache structure
	 * ----------------
	 */
	cp = (CatCache *) palloc(sizeof(CatCache));
	MemSet((char *) cp, 0, sizeof(CatCache));

	/* ----------------
	 *	initialize the cache buckets (each bucket is a list header)
	 *	and the LRU tuple list
	 * ----------------
	 */
	{

		/*
		 * We can only do this optimization because the number of hash
		 * buckets never changes.  Without it, we call malloc() too much.
		 * We could move this to dllist.c, but the way we do this is not
		 * dynamic/portabl, so why allow other routines to use it.
		 */
		Dllist	   *cache_begin = malloc((NCCBUCK + 1) * sizeof(Dllist));

		for (i = 0; i <= NCCBUCK; ++i)
		{
			cp->cc_cache[i] = &cache_begin[i];
			cp->cc_cache[i]->dll_head = 0;
			cp->cc_cache[i]->dll_tail = 0;
		}
	}

	cp->cc_lrulist = DLNewList();

	/* ----------------
	 *	Caches is the pointer to the head of the list of all the
	 *	system caches.	here we add the new cache to the top of the list.
	 * ----------------
	 */
	cp->cc_next = Caches;		/* list of caches (single link) */
	Caches = cp;

	/* ----------------
	 *	initialize the cache's relation information for the relation
	 *	corresponding to this cache and initialize some of the the new
	 *	cache's other internal fields.
	 * ----------------
	 */
	cp->relationId = InvalidOid;
	cp->indexId = InvalidOid;
	cp->cc_relname = relname;
	cp->cc_indname = indname;
	cp->cc_tupdesc = (TupleDesc) NULL;
	cp->id = id;
	cp->busy = false;
	cp->cc_maxtup = MAXTUP;
	cp->cc_size = NCCBUCK;
	cp->cc_nkeys = nkeys;
	cp->cc_iscanfunc = iScanfuncP;

	/* ----------------
	 *	initialize the cache's key information
	 * ----------------
	 */
	for (i = 0; i < nkeys; ++i)
	{
		cp->cc_key[i] = key[i];
		if (!key[i])
			elog(FATAL, "InitSysCache: called with 0 key[%d]", i);
		if (key[i] < 0)
		{
			if (key[i] != ObjectIdAttributeNumber)
				elog(FATAL, "InitSysCache: called with %d key[%d]", key[i], i);
			else
			{
				cp->cc_klen[i] = sizeof(Oid);

				/*
				 * ScanKeyEntryData and struct skey are equivalent. It
				 * looks like a move was made to obsolete struct skey, but
				 * it didn't reach this file.  Someday we should clean up
				 * this code and consolidate to ScanKeyEntry - mer 10 Nov
				 * 1991
				 */
				ScanKeyEntryInitialize(&cp->cc_skey[i],
									   (bits16) 0,
									   (AttrNumber) key[i],
									   (RegProcedure) F_OIDEQ,
									   (Datum) 0);
				continue;
			}
		}

		cp->cc_skey[i].sk_attno = key[i];
	}

	/* ----------------
	 *	all done.  new cache is initialized.  print some debugging
	 *	information, if appropriate.
	 * ----------------
	 */
	InitSysCache_DEBUG1;

	/* ----------------
	 *	back to the old context before we return...
	 * ----------------
	 */
	MemoryContextSwitchTo(oldcxt);
	return cp;
}


/* --------------------------------
 *		SearchSelfReferences
 *
 *		This call searches a self referencing information,
 *
 *		which causes a cycle in system catalog cache
 *
 *		cache should already be initailized
 * --------------------------------
 */
static HeapTuple
SearchSelfReferences(struct catcache * cache)
{
	HeapTuple		ntp;
	Relation		rel;
	static Oid		indexSelfOid = 0;
	static HeapTuple	indexSelfTuple = 0;

	if (cache->id != INDEXRELID)
		return (HeapTuple)0;

	if (!indexSelfOid)
	{
		rel = heap_openr(RelationRelationName, AccessShareLock);
		ntp = ClassNameIndexScan(rel, IndexRelidIndex);
		if (!HeapTupleIsValid(ntp))
			elog(ERROR, "SearchSelfRefernces: %s not found in %s",
				IndexRelidIndex, RelationRelationName);
		indexSelfOid = ntp->t_data->t_oid;
		pfree(ntp);
		heap_close(rel, AccessShareLock);
	}
	if ((Oid)cache->cc_skey[0].sk_argument != indexSelfOid)
		return (HeapTuple)0;
	if (!indexSelfTuple)
	{
		HeapScanDesc	sd;
		MemoryContext	oldcxt;

		if (!CacheCxt)
			CacheCxt = CreateGlobalMemory("Cache");
		rel = heap_open(cache->relationId, AccessShareLock);
		sd = heap_beginscan(rel, false, SnapshotNow, 1, cache->cc_skey);
		ntp = heap_getnext(sd, 0);
		if (!HeapTupleIsValid(ntp))
			elog(ERROR, "SearchSelfRefernces: tuple not found");
		oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);
		indexSelfTuple = heap_copytuple(ntp);
		MemoryContextSwitchTo(oldcxt);
		heap_endscan(sd);
		heap_close(rel, AccessShareLock);
	}

	return indexSelfTuple;
}

/* --------------------------------
 *		SearchSysCache
 *
 *		This call searches a system cache for a tuple, opening the relation
 *		if necessary (the first access to a particular cache).
 * --------------------------------
 */
HeapTuple
SearchSysCache(struct catcache * cache,
			   Datum v1,
			   Datum v2,
			   Datum v3,
			   Datum v4)
{
	unsigned	hash;
	CatCTup    *ct = NULL;
	CatCTup    *nct;
	CatCTup    *nct2;
	Dlelem	   *elt;
	HeapTuple	ntp = 0;

	Relation	relation;
	MemoryContext oldcxt;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	if (cache->relationId == InvalidOid)
		CatalogCacheInitializeCache(cache, NULL);

	/* ----------------
	 *	initialize the search key information
	 * ----------------
	 */
	cache->cc_skey[0].sk_argument = v1;
	cache->cc_skey[1].sk_argument = v2;
	cache->cc_skey[2].sk_argument = v3;
	cache->cc_skey[3].sk_argument = v4;

	/*
	 *	resolve self referencing informtion
	 */
	if (ntp = SearchSelfReferences(cache), ntp)
	{
			return heap_copytuple(ntp);
	}

	/* ----------------
	 *	find the hash bucket in which to look for the tuple
	 * ----------------
	 */
	hash = CatalogCacheComputeHashIndex(cache);

	/* ----------------
	 *	scan the hash bucket until we find a match or exhaust our tuples
	 * ----------------
	 */
	for (elt = DLGetHead(cache->cc_cache[hash]);
		 elt;
		 elt = DLGetSucc(elt))
	{
		bool		res;

		ct = (CatCTup *) DLE_VAL(elt);
		/* ----------------
		 *	see if the cached tuple matches our key.
		 *	(should we be worried about time ranges? -cim 10/2/90)
		 * ----------------
		 */
		HeapKeyTest(ct->ct_tup,
					cache->cc_tupdesc,
					cache->cc_nkeys,
					cache->cc_skey,
					res);
		if (res)
			break;
	}

	/* ----------------
	 *	if we found a tuple in the cache, move it to the top of the
	 *	lru list, and return it.  We also move it to the front of the
	 *	list for its hashbucket, in order to speed subsequent searches.
	 *	(The most frequently accessed elements in any hashbucket will
	 *	tend to be near the front of the hashbucket's list.)
	 * ----------------
	 */
	if (elt)
	{
		Dlelem	   *old_lru_elt = ((CatCTup *) DLE_VAL(elt))->ct_node;

		DLMoveToFront(old_lru_elt);
		DLMoveToFront(elt);

#ifdef CACHEDEBUG
		relation = heap_open(cache->relationId, NoLock);
		CACHE3_elog(DEBUG, "SearchSysCache(%s): found in bucket %d",
					RelationGetRelationName(relation), hash);
		heap_close(relation, NoLock);
#endif	 /* CACHEDEBUG */

		return ct->ct_tup;
	}

	/* ----------------
	 *	Tuple was not found in cache, so we have to try and
	 *	retrieve it directly from the relation.  If it's found,
	 *	we add it to the cache.
	 *
	 *	To guard against possible infinite recursion, we mark this cache
	 *	"busy" while trying to load a new entry for it.  It is OK to
	 *	recursively invoke SearchSysCache for a different cache, but
	 *	a recursive call for the same cache will error out.  (We could
	 *	store the specific key(s) being looked for, and consider only
	 *	a recursive request for the same key to be an error, but this
	 *	simple scheme is sufficient for now.)
	 * ----------------
	 */

	if (cache->busy)
	{
		elog(ERROR, "SearchSysCache: recursive use of cache %d", cache->id);
	}
	cache->busy = true;

	/* ----------------
	 *	open the relation associated with the cache
	 * ----------------
	 */
	relation = heap_open(cache->relationId, AccessShareLock);
	CACHE2_elog(DEBUG, "SearchSysCache(%s)",
				RelationGetRelationName(relation));

	/* ----------------
	 *	Switch to the cache memory context.
	 * ----------------
	 */

	if (!CacheCxt)
		CacheCxt = CreateGlobalMemory("Cache");

	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/* ----------------
	 *	Scan the relation to find the tuple.  If there's an index, and
	 *	if this isn't bootstrap (initdb) time, use the index.
	 * ----------------
	 */
	CACHE2_elog(DEBUG, "SearchSysCache: performing scan (override==%d)",
				heapisoverride());

	if ((RelationGetForm(relation))->relhasindex
		&& !IsBootstrapProcessingMode())
	{
		/* ----------
		 *	Switch back to old memory context so memory not freed
		 *	in the scan function will go away at transaction end.
		 *	wieck - 10/18/1996
		 * ----------
		 */
		MemoryContextSwitchTo(oldcxt);
		Assert(cache->cc_iscanfunc);
		switch (cache->cc_nkeys)
		{
			case 4:
				ntp = cache->cc_iscanfunc(relation, v1, v2, v3, v4);
				break;
			case 3:
				ntp = cache->cc_iscanfunc(relation, v1, v2, v3);
				break;
			case 2:
				ntp = cache->cc_iscanfunc(relation, v1, v2);
				break;
			case 1:
				ntp = cache->cc_iscanfunc(relation, v1);
				break;
		}
		/* ----------
		 *	Back to Cache context. If we got a tuple copy it
		 *	into our context.
		 *	wieck - 10/18/1996
		 * ----------
		 */
		MemoryContextSwitchTo((MemoryContext) CacheCxt);
		if (HeapTupleIsValid(ntp))
			ntp = heap_copytuple(ntp);
	}
	else
	{
		HeapScanDesc sd;

		/* ----------
		 *	As above do the lookup in the callers memory
		 *	context.
		 *	wieck - 10/18/1996
		 * ----------
		 */
		MemoryContextSwitchTo(oldcxt);

		sd = heap_beginscan(relation, 0, SnapshotNow,
							cache->cc_nkeys, cache->cc_skey);

		ntp = heap_getnext(sd, 0);

		MemoryContextSwitchTo((MemoryContext) CacheCxt);

		if (HeapTupleIsValid(ntp))
		{
			CACHE1_elog(DEBUG, "SearchSysCache: found tuple");
			ntp = heap_copytuple(ntp);
		}

		MemoryContextSwitchTo(oldcxt);

		heap_endscan(sd);

		MemoryContextSwitchTo((MemoryContext) CacheCxt);
	}

	cache->busy = false;

	/* ----------------
	 *	scan is complete.  if tup is valid, we copy it and add the copy to
	 *	the cache.
	 * ----------------
	 */
	if (HeapTupleIsValid(ntp))
	{
		/* ----------------
		 *	allocate a new cache tuple holder, store the pointer
		 *	to the heap tuple there and initialize the list pointers.
		 * ----------------
		 */
		Dlelem	   *lru_elt;

		/*
		 * this is a little cumbersome here because we want the Dlelem's
		 * in both doubly linked lists to point to one another. That makes
		 * it easier to remove something from both the cache bucket and
		 * the lru list at the same time
		 */
		nct = (CatCTup *) malloc(sizeof(CatCTup));
		nct->ct_tup = ntp;
		elt = DLNewElem(nct);
		nct2 = (CatCTup *) malloc(sizeof(CatCTup));
		nct2->ct_tup = ntp;
		lru_elt = DLNewElem(nct2);
		nct2->ct_node = elt;
		nct->ct_node = lru_elt;

		DLAddHead(cache->cc_lrulist, lru_elt);
		DLAddHead(cache->cc_cache[hash], elt);

		/* ----------------
		 *	If we've exceeded the desired size of this cache,
		 *	throw away the least recently used entry.
		 * ----------------
		 */
		if (++cache->cc_ntup > cache->cc_maxtup)
		{
			CatCTup    *ct;

			elt = DLGetTail(cache->cc_lrulist);
			ct = (CatCTup *) DLE_VAL(elt);

			if (ct != nct)		/* shouldn't be possible, but be safe... */
			{
				CACHE2_elog(DEBUG, "SearchSysCache(%s): Overflow, LRU removal",
							RelationGetRelationName(relation));

				CatCacheRemoveCTup(cache, elt);
			}
		}

		CACHE4_elog(DEBUG, "SearchSysCache(%s): Contains %d/%d tuples",
					RelationGetRelationName(relation),
					cache->cc_ntup, cache->cc_maxtup);
		CACHE3_elog(DEBUG, "SearchSysCache(%s): put in bucket %d",
					RelationGetRelationName(relation), hash);
	}

	/* ----------------
	 *	close the relation, switch back to the original memory context
	 *	and return the tuple we found (or NULL)
	 * ----------------
	 */
	heap_close(relation, AccessShareLock);

	MemoryContextSwitchTo(oldcxt);
	return ntp;
}

/* --------------------------------
 *	RelationInvalidateCatalogCacheTuple()
 *
 *	Invalidate a tuple from a specific relation.  This call determines the
 *	cache in question and calls CatalogCacheIdInvalidate().  It is -ok-
 *	if the relation cannot be found, it simply means this backend has yet
 *	to open it.
 * --------------------------------
 */
void
RelationInvalidateCatalogCacheTuple(Relation relation,
									HeapTuple tuple,
							  void (*function) (int, Index, ItemPointer))
{
	struct catcache *ccp;
	MemoryContext oldcxt;
	Oid			relationId;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	Assert(RelationIsValid(relation));
	Assert(HeapTupleIsValid(tuple));
	Assert(PointerIsValid(function));
	CACHE1_elog(DEBUG, "RelationInvalidateCatalogCacheTuple: called");

	/* ----------------
	 *	switch to the cache memory context
	 * ----------------
	 */
	if (!CacheCxt)
		CacheCxt = CreateGlobalMemory("Cache");
	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/* ----------------
	 *	for each cache
	 *	   if the cache contains tuples from the specified relation
	 *		   call the invalidation function on the tuples
	 *		   in the proper hash bucket
	 * ----------------
	 */
	relationId = RelationGetRelid(relation);

	for (ccp = Caches; ccp; ccp = ccp->cc_next)
	{
		if (relationId != ccp->relationId)
			continue;

#ifdef NOT_USED
		/* OPT inline simplification of CatalogCacheIdInvalidate */
		if (!PointerIsValid(function))
			function = CatalogCacheIdInvalidate;
#endif

		(*function) (ccp->id,
				 CatalogCacheComputeTupleHashIndex(ccp, relation, tuple),
					 &tuple->t_self);
	}

	/* ----------------
	 *	return to the proper memory context
	 * ----------------
	 */
	MemoryContextSwitchTo(oldcxt);

	/* sendpm('I', "Invalidated tuple"); */
}
