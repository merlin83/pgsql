/*-------------------------------------------------------------------------
 *
 * hash.h
 *	  header file for postgres hash access method implementation
 *
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: hash.h,v 1.52 2003-09-02 18:13:32 tgl Exp $
 *
 * NOTES
 *		modeled after Margo Seltzer's hash implementation for unix.
 *
 *-------------------------------------------------------------------------
 */
#ifndef HASH_H
#define HASH_H

#include "access/itup.h"
#include "access/relscan.h"
#include "access/sdir.h"
#include "access/xlog.h"
#include "fmgr.h"

/*
 * Mapping from hash bucket number to physical block number of bucket's
 * starting page.  Beware of multiple evaluations of argument!
 */
typedef uint32 Bucket;

#define BUCKET_TO_BLKNO(metap,B) \
		((BlockNumber) ((B) + ((B) ? (metap)->hashm_spares[_hash_log2((B)+1)-1] : 0)) + 1)

/*
 * Special space for hash index pages.
 *
 * hasho_flag tells us which type of page we're looking at.  For
 * example, knowing overflow pages from bucket pages is necessary
 * information when you're deleting tuples from a page. If all the
 * tuples are deleted from an overflow page, the overflow is made
 * available to other buckets by calling _hash_freeovflpage(). If all
 * the tuples are deleted from a bucket page, no additional action is
 * necessary.
 */
#define LH_UNUSED_PAGE			(0)
#define LH_OVERFLOW_PAGE		(1 << 0)
#define LH_BUCKET_PAGE			(1 << 1)
#define LH_BITMAP_PAGE			(1 << 2)
#define LH_META_PAGE			(1 << 3)

typedef struct HashPageOpaqueData
{
	BlockNumber hasho_prevblkno;	/* previous ovfl (or bucket) blkno */
	BlockNumber hasho_nextblkno;	/* next ovfl blkno */
	Bucket		hasho_bucket;	/* bucket number this pg belongs to */
	uint16		hasho_flag;		/* page type code, see above */
	uint16		hasho_filler;	/* available for future use */
	/*
	 * We presently set hasho_filler to HASHO_FILL (0x1234); this is for
	 * the convenience of pg_filedump, which otherwise would have a hard
	 * time telling HashPageOpaqueData from BTPageOpaqueData.  If we ever
	 * need that space for some other purpose, pg_filedump will have to
	 * find another way.
	 */
} HashPageOpaqueData;

typedef HashPageOpaqueData *HashPageOpaque;

#define HASHO_FILL		0x1234

/*
 *	ScanOpaqueData is used to remember which buffers we're currently
 *	examining in the scan.	We keep these buffers locked and pinned and
 *	recorded in the opaque entry of the scan in order to avoid doing a
 *	ReadBuffer() for every tuple in the index.
 */
typedef struct HashScanOpaqueData
{
	Buffer		hashso_curbuf;
	Buffer		hashso_mrkbuf;
} HashScanOpaqueData;

typedef HashScanOpaqueData *HashScanOpaque;

/*
 * Definitions for metapage.
 */

#define HASH_METAPAGE	0		/* metapage is always block 0 */

#define HASH_MAGIC		0x6440640
#define HASH_VERSION	1		/* new for Pg 7.4 */

/*
 * Spares[] holds the number of overflow pages currently allocated at or
 * before a certain splitpoint. For example, if spares[3] = 7 then there are
 * 7 ovflpages before splitpoint 3 (compare BUCKET_TO_BLKNO macro).  The
 * value in spares[ovflpoint] increases as overflow pages are added at the
 * end of the index.  Once ovflpoint increases (ie, we have actually allocated
 * the bucket pages belonging to that splitpoint) the number of spares at the
 * prior splitpoint cannot change anymore.
 *
 * ovflpages that have been recycled for reuse can be found by looking at
 * bitmaps that are stored within ovflpages dedicated for the purpose.
 * The blknos of these bitmap pages are kept in bitmaps[]; nmaps is the
 * number of currently existing bitmaps.
 *
 * The limitation on the size of spares[] comes from the fact that there's
 * no point in having more than 2^32 buckets with only uint32 hashcodes.
 * There is no particular upper limit on the size of mapp[], other than
 * needing to fit into the metapage.  (With 8K block size, 128 bitmaps
 * limit us to 64 Gb of overflow space...)
 */
#define HASH_MAX_SPLITPOINTS		32
#define HASH_MAX_BITMAPS			128

typedef struct HashMetaPageData
{
	PageHeaderData hashm_phdr;	/* pad for page header (do not use) */
	uint32		hashm_magic;	/* magic no. for hash tables */
	uint32		hashm_version;	/* version ID */
	double		hashm_ntuples;	/* number of tuples stored in the table */
	uint16		hashm_ffactor;	/* target fill factor (tuples/bucket) */
	uint16		hashm_bsize;	/* index page size (bytes) */
	uint16		hashm_bmsize;	/* bitmap array size (bytes) - must be a
								 * power of 2 */
	uint16		hashm_bmshift;	/* log2(bitmap array size in BITS) */
	uint32		hashm_maxbucket;	/* ID of maximum bucket in use */
	uint32		hashm_highmask; /* mask to modulo into entire table */
	uint32		hashm_lowmask;	/* mask to modulo into lower half of table */
	uint32		hashm_ovflpoint;/* splitpoint from which ovflpgs being
								 * allocated */
	uint32		hashm_firstfree;	/* lowest-number free ovflpage (bit#) */
	uint32		hashm_nmaps;	/* number of bitmap pages */
	RegProcedure hashm_procid;	/* hash procedure id from pg_proc */
	uint32		hashm_spares[HASH_MAX_SPLITPOINTS];	/* spare pages before
													 * each splitpoint */
	BlockNumber hashm_mapp[HASH_MAX_BITMAPS];	/* blknos of ovfl bitmaps */
} HashMetaPageData;

typedef HashMetaPageData *HashMetaPage;

typedef struct HashItemData
{
	IndexTupleData hash_itup;
} HashItemData;

typedef HashItemData *HashItem;

/*
 * Constants
 */
#define DEFAULT_FFACTOR			300
#define BYTE_TO_BIT				3		/* 2^3 bits/byte */
#define ALL_SET					((uint32) ~0)

/*
 * Bitmap pages do not contain tuples.	They do contain the standard
 * page headers and trailers; however, everything in between is a
 * giant bit array.  The number of bits that fit on a page obviously
 * depends on the page size and the header/trailer overhead.  We require
 * the number of bits per page to be a power of 2.
 */
#define BMPGSZ_BYTE(metap)		((metap)->hashm_bmsize)
#define BMPGSZ_BIT(metap)		((metap)->hashm_bmsize << BYTE_TO_BIT)
#define BMPG_SHIFT(metap)		((metap)->hashm_bmshift)
#define BMPG_MASK(metap)		(BMPGSZ_BIT(metap) - 1)
#define HashPageGetBitmap(pg) \
	((uint32 *) (((char *) (pg)) + MAXALIGN(sizeof(PageHeaderData))))

/*
 * The number of bits in an ovflpage bitmap word.
 */
#define BITS_PER_MAP	32		/* Number of bits in uint32 */

/* Given the address of the beginning of a bit map, clear/set the nth bit */
#define CLRBIT(A, N)	((A)[(N)/BITS_PER_MAP] &= ~(1<<((N)%BITS_PER_MAP)))
#define SETBIT(A, N)	((A)[(N)/BITS_PER_MAP] |= (1<<((N)%BITS_PER_MAP)))
#define ISSET(A, N)		((A)[(N)/BITS_PER_MAP] & (1<<((N)%BITS_PER_MAP)))

/*
 * page locking modes
 */
#define HASH_READ		0
#define HASH_WRITE		1

/*
 *	Strategy number. There's only one valid strategy for hashing: equality.
 */
#define HTEqualStrategyNumber			1
#define HTMaxStrategyNumber				1

/*
 *	When a new operator class is declared, we require that the user supply
 *	us with an amproc procudure for hashing a key of the new type.
 *	Since we only have one such proc in amproc, it's number 1.
 */
#define HASHPROC		1


extern bool BuildingHash;

/* public routines */

extern Datum hashbuild(PG_FUNCTION_ARGS);
extern Datum hashinsert(PG_FUNCTION_ARGS);
extern Datum hashgettuple(PG_FUNCTION_ARGS);
extern Datum hashbeginscan(PG_FUNCTION_ARGS);
extern Datum hashrescan(PG_FUNCTION_ARGS);
extern Datum hashendscan(PG_FUNCTION_ARGS);
extern Datum hashmarkpos(PG_FUNCTION_ARGS);
extern Datum hashrestrpos(PG_FUNCTION_ARGS);
extern Datum hashbulkdelete(PG_FUNCTION_ARGS);

/*
 * Datatype-specific hash functions in hashfunc.c.
 *
 * These support both hash indexes and hash joins.
 *
 * NOTE: some of these are also used by catcache operations, without
 * any direct connection to hash indexes.  Also, the common hash_any
 * routine is also used by dynahash tables.
 */
extern Datum hashchar(PG_FUNCTION_ARGS);
extern Datum hashint2(PG_FUNCTION_ARGS);
extern Datum hashint4(PG_FUNCTION_ARGS);
extern Datum hashint8(PG_FUNCTION_ARGS);
extern Datum hashoid(PG_FUNCTION_ARGS);
extern Datum hashfloat4(PG_FUNCTION_ARGS);
extern Datum hashfloat8(PG_FUNCTION_ARGS);
extern Datum hashoidvector(PG_FUNCTION_ARGS);
extern Datum hashint2vector(PG_FUNCTION_ARGS);
extern Datum hashname(PG_FUNCTION_ARGS);
extern Datum hashtext(PG_FUNCTION_ARGS);
extern Datum hashvarlena(PG_FUNCTION_ARGS);
extern Datum hash_any(register const unsigned char *k, register int keylen);

/* private routines */

/* hashinsert.c */
extern InsertIndexResult _hash_doinsert(Relation rel, HashItem hitem);

/* hashovfl.c */
extern Buffer _hash_addovflpage(Relation rel, Buffer metabuf, Buffer buf);
extern BlockNumber _hash_freeovflpage(Relation rel, Buffer ovflbuf);
extern void _hash_initbitmap(Relation rel, HashMetaPage metap,
							 BlockNumber blkno);
extern void _hash_squeezebucket(Relation rel,
								Bucket bucket, BlockNumber bucket_blkno);

/* hashpage.c */
extern void _hash_metapinit(Relation rel);
extern Buffer _hash_getbuf(Relation rel, BlockNumber blkno, int access);
extern void _hash_relbuf(Relation rel, Buffer buf, int access);
extern void _hash_wrtbuf(Relation rel, Buffer buf);
extern void _hash_wrtnorelbuf(Buffer buf);
extern void _hash_chgbufaccess(Relation rel, Buffer buf, int from_access,
				   int to_access);
extern void _hash_pageinit(Page page, Size size);
extern void _hash_expandtable(Relation rel, Buffer metabuf);

/* hashscan.c */
extern void _hash_regscan(IndexScanDesc scan);
extern void _hash_dropscan(IndexScanDesc scan);
extern void _hash_adjscans(Relation rel, ItemPointer tid);
extern void AtEOXact_hash(void);

/* hashsearch.c */
extern void _hash_search(Relation rel, int keysz, ScanKey scankey,
			 Buffer *bufP, HashMetaPage metap);
extern bool _hash_next(IndexScanDesc scan, ScanDirection dir);
extern bool _hash_first(IndexScanDesc scan, ScanDirection dir);
extern bool _hash_step(IndexScanDesc scan, Buffer *bufP, ScanDirection dir,
		   Buffer metabuf);

/* hashutil.c */
extern ScanKey _hash_mkscankey(Relation rel, IndexTuple itup);
extern void _hash_freeskey(ScanKey skey);
extern bool _hash_checkqual(IndexScanDesc scan, IndexTuple itup);
extern HashItem _hash_formitem(IndexTuple itup);
extern Bucket _hash_call(Relation rel, HashMetaPage metap, Datum key);
extern uint32 _hash_log2(uint32 num);
extern void _hash_checkpage(Relation rel, Page page, int flags);

/* hash.c */
extern void hash_redo(XLogRecPtr lsn, XLogRecord *record);
extern void hash_undo(XLogRecPtr lsn, XLogRecord *record);
extern void hash_desc(char *buf, uint8 xl_info, char *rec);

#endif   /* HASH_H */
