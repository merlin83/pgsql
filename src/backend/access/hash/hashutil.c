/*-------------------------------------------------------------------------
 *
 * btutils.c--
 *    Utility code for Postgres btree implementation.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/access/hash/hashutil.c,v 1.5 1996-10-31 08:24:47 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */

#include <time.h>

#include "postgres.h"
 
#include "catalog/pg_attribute.h"
#include "access/attnum.h"
#include "nodes/nodes.h"
#include "nodes/pg_list.h" 
#include "access/tupdesc.h"  
#include "storage/fd.h" 
#include "catalog/pg_am.h"
#include "catalog/pg_class.h"
#include "nodes/nodes.h" 
#include "rewrite/prs2lock.h"
#include "access/skey.h"
#include "access/strat.h"
#include "utils/rel.h"
 
#include "storage/block.h"  
#include "storage/off.h"
#include "storage/itemptr.h"
#include "utils/nabstime.h"
#include "access/htup.h"
#include "access/itup.h"   
#include "storage/itemid.h"  
#include "storage/item.h"
#include "storage/buf.h"  
#include "storage/page.h"
#include "storage/bufpage.h" 
#include "access/sdir.h" 
#include "access/funcindex.h"
#include "utils/tqual.h"
#include "access/relscan.h"
#include "access/hash.h"

#include "utils/palloc.h"

#ifndef HAVE_MEMMOVE
# include "regex/utils.h"
#else
# include <string.h>
#endif

#include "fmgr.h"

#include "utils/memutils.h"

#include "access/iqual.h"

ScanKey
_hash_mkscankey(Relation rel, IndexTuple itup, HashMetaPage metap)
{
    ScanKey skey;
    TupleDesc itupdesc;
    int natts;
    AttrNumber i;
    Datum arg;
    RegProcedure proc;
    bool null;
    
    natts = rel->rd_rel->relnatts;
    itupdesc = RelationGetTupleDescriptor(rel);
    
    skey = (ScanKey) palloc(natts * sizeof(ScanKeyData));
    
    for (i = 0; i < natts; i++) {
	arg = index_getattr(itup, i + 1, itupdesc, &null);
	proc = metap->hashm_procid;
	ScanKeyEntryInitialize(&skey[i],
			       0x0, (AttrNumber) (i + 1), proc, arg);
    }
    
    return (skey);
}	

void
_hash_freeskey(ScanKey skey)
{
    pfree(skey);
}


bool
_hash_checkqual(IndexScanDesc scan, IndexTuple itup)
{
    if (scan->numberOfKeys > 0)
	return (index_keytest(itup, 
			      RelationGetTupleDescriptor(scan->relation),
			      scan->numberOfKeys, scan->keyData));
    else
	return (true);
}

HashItem
_hash_formitem(IndexTuple itup)
{
    int nbytes_hitem;
    HashItem hitem;
    Size tuplen;
    
    /* disallow nulls in hash keys */
    if (itup->t_info & INDEX_NULL_MASK)
	elog(WARN, "hash indices cannot include null keys");
    
    /* make a copy of the index tuple with room for the sequence number */
    tuplen = IndexTupleSize(itup);
    nbytes_hitem = tuplen +
	(sizeof(HashItemData) - sizeof(IndexTupleData));
    
    hitem = (HashItem) palloc(nbytes_hitem);
    memmove((char *) &(hitem->hash_itup), (char *) itup, tuplen);
    
    return (hitem);
}

Bucket
_hash_call(Relation rel, HashMetaPage metap, Datum key)
{
    uint32 n;
    Bucket bucket;
    RegProcedure proc;
    
    proc = metap->hashm_procid;
    n = (uint32) fmgr(proc, key);
    bucket = n & metap->hashm_highmask;
    if (bucket > metap->hashm_maxbucket)
	bucket = bucket & metap->hashm_lowmask;
    return (bucket);
}

/*
 * _hash_log2 -- returns ceil(lg2(num))
 */
uint32
_hash_log2(uint32 num)
{
    uint32 i, limit;
    
    limit = 1;
    for (i = 0; limit < num; limit = limit << 1, i++)
	;
    return (i);
}

/*
 * _hash_checkpage -- sanity checks on the format of all hash pages
 */
void
_hash_checkpage(Page page, int flags)
{
    PageHeader ph = (PageHeader) page;
    HashPageOpaque opaque;

    Assert(page);
    Assert(ph->pd_lower >= (sizeof(PageHeaderData) - sizeof(ItemIdData)));
#if 1
    Assert(ph->pd_upper <=
	   (BLCKSZ - DOUBLEALIGN(sizeof(HashPageOpaqueData))));
    Assert(ph->pd_special ==
	   (BLCKSZ - DOUBLEALIGN(sizeof(HashPageOpaqueData))));
    Assert(ph->pd_opaque.od_pagesize == BLCKSZ);
#endif
    if (flags) {
	opaque = (HashPageOpaque) PageGetSpecialPointer(page);
	Assert(opaque->hasho_flag & flags);
    }
}
