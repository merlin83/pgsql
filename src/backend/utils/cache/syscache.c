/*-------------------------------------------------------------------------
 *
 * syscache.c
 *	  System cache management routines
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/cache/syscache.c,v 1.56 2000-11-10 00:33:10 tgl Exp $
 *
 * NOTES
 *	  These routines allow the parser/planner/executor to perform
 *	  rapid lookups on the contents of the system catalogs.
 *
 *	  see catalog/syscache.h for a list of the cache id's
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "access/transam.h"
#include "utils/builtins.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_aggregate.h"
#include "catalog/pg_amop.h"
#include "catalog/pg_group.h"
#include "catalog/pg_index.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_language.h"
#include "catalog/pg_listener.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_rewrite.h"
#include "catalog/pg_shadow.h"
#include "catalog/pg_statistic.h"
#include "catalog/pg_type.h"
#include "utils/catcache.h"
#include "utils/syscache.h"
#include "utils/temprel.h"
#include "miscadmin.h"


/*---------------------------------------------------------------------------

	Adding system caches:

	Add your new cache to the list in include/utils/syscache.h.  Keep
	the list sorted alphabetically and adjust the cache numbers
	accordingly.

	Add your entry to the cacheinfo[] array below.	All cache lists are
	alphabetical, so add it in the proper place.  Specify the relation
	name, index name, number of keys, and key attribute numbers.

	In include/catalog/indexing.h, add a define for the number of indexes
	on the relation, add define(s) for the index name(s), add an extern
	array to hold the index names, and use DECLARE_UNIQUE_INDEX to define
	the index.  Cache lookups return only one row, so the index should be
	unique in most cases.

	In backend/catalog/indexing.c, initialize the relation array with
	the index names for the relation.

	Finally, any place your relation gets heap_insert() or
	heap_update calls, include code to do a CatalogIndexInsert() to update
	the system indexes.  The heap_* calls do not update indexes.

	bjm 1999/11/22

  ---------------------------------------------------------------------------
*/

/* ----------------
 *		struct cachedesc: information defining a single syscache
 * ----------------
 */
struct cachedesc
{
	char	   *name;			/* name of the relation being cached */
	char	   *indname;		/* name of index relation for this cache */
	int			nkeys;			/* # of keys needed for cache lookup */
	int			key[4];			/* attribute numbers of key attrs */
};

static struct cachedesc cacheinfo[] = {
	{AggregateRelationName,		/* AGGNAME */
	 AggregateNameTypeIndex,
		2,
		{
			Anum_pg_aggregate_aggname,
			Anum_pg_aggregate_aggbasetype,
			0,
			0
		}},
	{AccessMethodRelationName,	/* AMNAME */
	 AmNameIndex,
		1,
		{
			Anum_pg_am_amname,
			0,
			0,
			0
		}},
	{AccessMethodOperatorRelationName,	/* AMOPOPID */
	 AccessMethodOpidIndex,
		3,
		{
			Anum_pg_amop_amopclaid,
			Anum_pg_amop_amopopr,
			Anum_pg_amop_amopid,
			0
		}},
	{AccessMethodOperatorRelationName,	/* AMOPSTRATEGY */
	 AccessMethodStrategyIndex,
		3,
		{
			Anum_pg_amop_amopid,
			Anum_pg_amop_amopclaid,
			Anum_pg_amop_amopstrategy,
			0
		}},
	{AttributeRelationName,		/* ATTNAME */
	 AttributeRelidNameIndex,
		2,
		{
			Anum_pg_attribute_attrelid,
			Anum_pg_attribute_attname,
			0,
			0
		}},
	{AttributeRelationName,		/* ATTNUM */
	 AttributeRelidNumIndex,
		2,
		{
			Anum_pg_attribute_attrelid,
			Anum_pg_attribute_attnum,
			0,
			0
		}},
	{OperatorClassRelationName, /* CLADEFTYPE */
	 OpclassDeftypeIndex,
		1,
		{
			Anum_pg_opclass_opcdeftype,
			0,
			0,
			0
		}},
	{OperatorClassRelationName, /* CLANAME */
	 OpclassNameIndex,
		1,
		{
			Anum_pg_opclass_opcname,
			0,
			0,
			0
		}},
	{GroupRelationName,			/* GRONAME */
	 GroupNameIndex,
		1,
		{
			Anum_pg_group_groname,
			0,
			0,
			0
		}},
	{GroupRelationName,			/* GROSYSID */
	 GroupSysidIndex,
		1,
		{
			Anum_pg_group_grosysid,
			0,
			0,
			0
		}},
	{IndexRelationName,			/* INDEXRELID */
	 IndexRelidIndex,
		1,
		{
			Anum_pg_index_indexrelid,
			0,
			0,
			0
		}},
	{InheritsRelationName,		/* INHRELID */
	 InheritsRelidSeqnoIndex,
		2,
		{
			Anum_pg_inherits_inhrelid,
			Anum_pg_inherits_inhseqno,
			0,
			0
		}},
	{LanguageRelationName,		/* LANGNAME */
	 LanguageNameIndex,
		1,
		{
			Anum_pg_language_lanname,
			0,
			0,
			0
		}},
	{LanguageRelationName,		/* LANGOID */
	 LanguageOidIndex,
		1,
		{
			ObjectIdAttributeNumber,
			0,
			0,
			0
		}},
	{ListenerRelationName,		/* LISTENREL */
	 ListenerPidRelnameIndex,
		2,
		{
			Anum_pg_listener_pid,
			Anum_pg_listener_relname,
			0,
			0
		}},
	{OperatorRelationName,		/* OPERNAME */
	 OperatorNameIndex,
		4,
		{
			Anum_pg_operator_oprname,
			Anum_pg_operator_oprleft,
			Anum_pg_operator_oprright,
			Anum_pg_operator_oprkind
		}},
	{OperatorRelationName,		/* OPEROID */
	 OperatorOidIndex,
		1,
		{
			ObjectIdAttributeNumber,
			0,
			0,
			0
		}},
	{ProcedureRelationName,		/* PROCNAME */
	 ProcedureNameIndex,
		3,
		{
			Anum_pg_proc_proname,
			Anum_pg_proc_pronargs,
			Anum_pg_proc_proargtypes,
			0
		}},
	{ProcedureRelationName,		/* PROCOID */
	 ProcedureOidIndex,
		1,
		{
			ObjectIdAttributeNumber,
			0,
			0,
			0
		}},
	{RelationRelationName,		/* RELNAME */
	 ClassNameIndex,
		1,
		{
			Anum_pg_class_relname,
			0,
			0,
			0
		}},
	{RelationRelationName,		/* RELOID */
	 ClassOidIndex,
		1,
		{
			ObjectIdAttributeNumber,
			0,
			0,
			0
		}},
	{RewriteRelationName,		/* REWRITENAME */
	 RewriteRulenameIndex,
		1,
		{
			Anum_pg_rewrite_rulename,
			0,
			0,
			0
		}},
	{RewriteRelationName,		/* RULEOID */
	 RewriteOidIndex,
		1,
		{
			ObjectIdAttributeNumber,
			0,
			0,
			0
		}},
	{ShadowRelationName,		/* SHADOWNAME */
	 ShadowNameIndex,
		1,
		{
			Anum_pg_shadow_usename,
			0,
			0,
			0
		}},
	{ShadowRelationName,		/* SHADOWSYSID */
	 ShadowSysidIndex,
		1,
		{
			Anum_pg_shadow_usesysid,
			0,
			0,
			0
		}},
	{StatisticRelationName,		/* STATRELID */
	 StatisticRelidAttnumIndex,
		2,
		{
			Anum_pg_statistic_starelid,
			Anum_pg_statistic_staattnum,
			0,
			0
		}},
	{TypeRelationName,			/* TYPENAME */
	 TypeNameIndex,
		1,
		{
			Anum_pg_type_typname,
			0,
			0,
			0
		}},
	{TypeRelationName,			/* TYPEOID */
	 TypeOidIndex,
		1,
		{
			ObjectIdAttributeNumber,
			0,
			0,
			0
		}}
};

static CatCache *SysCache[lengthof(cacheinfo)];
static int32 SysCacheSize = lengthof(cacheinfo);
static bool CacheInitialized = false;


bool
IsCacheInitialized(void)
{
	return CacheInitialized;
}


/*
 * zerocaches
 *
 *	  Make sure the SysCache structure is zero'd.
 */
void
zerocaches()
{
	MemSet((char *) SysCache, 0, SysCacheSize * sizeof(CatCache *));
}


/*
 * InitCatalogCache - initialize the caches
 */
void
InitCatalogCache()
{
	int			cacheId;		/* XXX type */

	if (!AMI_OVERRIDE)
	{
		for (cacheId = 0; cacheId < SysCacheSize; cacheId += 1)
		{
			Assert(!PointerIsValid(SysCache[cacheId]));

			SysCache[cacheId] = InitSysCache(cacheId,
											 cacheinfo[cacheId].name,
											 cacheinfo[cacheId].indname,
											 cacheinfo[cacheId].nkeys,
											 cacheinfo[cacheId].key);
			if (!PointerIsValid(SysCache[cacheId]))
			{
				elog(ERROR,
					 "InitCatalogCache: Can't init cache %s (%d)",
					 cacheinfo[cacheId].name,
					 cacheId);
			}

		}
	}
	CacheInitialized = true;
}


/*
 * SearchSysCacheTuple
 *
 *	A layer on top of SearchSysCache that does the initialization and
 *	key-setting for you.
 *
 *	Returns the cache copy of the tuple if one is found, NULL if not.
 *	The tuple is the 'cache' copy.
 *
 *	CAUTION: The tuple that is returned must NOT be freed by the caller!
 *
 *	CAUTION: The returned tuple may be flushed from the cache during
 *	subsequent cache lookup operations, or by shared cache invalidation.
 *	Callers should not expect the pointer to remain valid for long.
 *
 *  XXX we ought to have some kind of referencecount mechanism for
 *  cache entries, to ensure entries aren't deleted while in use.
 */
HeapTuple
SearchSysCacheTuple(int cacheId,/* cache selection code */
					Datum key1,
					Datum key2,
					Datum key3,
					Datum key4)
{
	HeapTuple	tp;

	if (cacheId < 0 || cacheId >= SysCacheSize)
	{
		elog(ERROR, "SearchSysCacheTuple: Bad cache id %d", cacheId);
		return (HeapTuple) NULL;
	}

	Assert(AMI_OVERRIDE || PointerIsValid(SysCache[cacheId]));

	if (!PointerIsValid(SysCache[cacheId]))
	{
		SysCache[cacheId] = InitSysCache(cacheId,
										 cacheinfo[cacheId].name,
										 cacheinfo[cacheId].indname,
										 cacheinfo[cacheId].nkeys,
										 cacheinfo[cacheId].key);
		if (!PointerIsValid(SysCache[cacheId]))
			elog(ERROR,
				 "InitCatalogCache: Can't init cache %s(%d)",
				 cacheinfo[cacheId].name,
				 cacheId);
	}

	/*
	 * If someone tries to look up a relname, translate temp relation
	 * names to real names.  Less obviously, apply the same translation
	 * to type names, so that the type tuple of a temp table will be found
	 * when sought.  This is a kluge ... temp table substitution should be
	 * happening at a higher level ...
	 */
	if (cacheId == RELNAME || cacheId == TYPENAME)
	{
		char	   *nontemp_relname;

		nontemp_relname = get_temp_rel_by_username(DatumGetCString(key1));
		if (nontemp_relname != NULL)
			key1 = CStringGetDatum(nontemp_relname);
	}

	tp = SearchSysCache(SysCache[cacheId], key1, key2, key3, key4);
	if (!HeapTupleIsValid(tp))
	{
#ifdef CACHEDEBUG
		elog(DEBUG,
			 "SearchSysCacheTuple: Search %s(%d) %d %d %d %d failed",
			 cacheinfo[cacheId].name,
			 cacheId, key1, key2, key3, key4);
#endif
		return (HeapTuple) NULL;
	}
	return tp;
}


/*
 * SearchSysCacheTupleCopy
 *
 *	This is like SearchSysCacheTuple, except it returns a palloc'd copy of
 *	the tuple.  The caller should heap_freetuple() the returned copy when
 *	done with it.  This routine should be used when the caller intends to
 *	continue to access the tuple for more than a very short period of time.
 */
HeapTuple
SearchSysCacheTupleCopy(int cacheId,	/* cache selection code */
						Datum key1,
						Datum key2,
						Datum key3,
						Datum key4)
{
	HeapTuple	cachetup;

	cachetup = SearchSysCacheTuple(cacheId, key1, key2, key3, key4);
	if (PointerIsValid(cachetup))
		return heap_copytuple(cachetup);
	else
		return cachetup;		/* NULL */
}


/*
 * SysCacheGetAttr
 *
 *		Given a tuple previously fetched by SearchSysCacheTuple() or
 *		SearchSysCacheTupleCopy(), extract a specific attribute.
 *
 * This is equivalent to using heap_getattr() on a tuple fetched
 * from a non-cached relation.	Usually, this is only used for attributes
 * that could be NULL or variable length; the fixed-size attributes in
 * a system table are accessed just by mapping the tuple onto the C struct
 * declarations from include/catalog/.
 *
 * As with heap_getattr(), if the attribute is of a pass-by-reference type
 * then a pointer into the tuple data area is returned --- the caller must
 * not modify or pfree the datum!
 */
Datum
SysCacheGetAttr(int cacheId, HeapTuple tup,
				AttrNumber attributeNumber,
				bool *isNull)
{

	/*
	 * We just need to get the TupleDesc out of the cache entry, and then
	 * we can apply heap_getattr().  We expect that the cache control data
	 * is currently valid --- if the caller recently fetched the tuple, then
	 * it should be.
	 */
	if (cacheId < 0 || cacheId >= SysCacheSize)
		elog(ERROR, "SysCacheGetAttr: Bad cache id %d", cacheId);
	if (!PointerIsValid(SysCache[cacheId]) ||
		!PointerIsValid(SysCache[cacheId]->cc_tupdesc))
		elog(ERROR, "SysCacheGetAttr: missing cache data for id %d", cacheId);

	return heap_getattr(tup, attributeNumber,
						SysCache[cacheId]->cc_tupdesc,
						isNull);
}
