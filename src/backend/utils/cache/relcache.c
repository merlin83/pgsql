/*-------------------------------------------------------------------------
 *
 * relcache.c
 *	  POSTGRES relation descriptor cache code
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/cache/relcache.c,v 1.95 2000-04-12 17:15:54 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 * INTERFACE ROUTINES
 *		RelationInitialize				- initialize relcache
 *		RelationIdCacheGetRelation		- get a reldesc from the cache (id)
 *		RelationNameCacheGetRelation	- get a reldesc from the cache (name)
 *		RelationIdGetRelation			- get a reldesc by relation id
 *		RelationNameGetRelation			- get a reldesc by relation name
 *		RelationClose					- close an open relation
 *
 * NOTES
 *		This file is in the process of being cleaned up
 *		before I add system attribute indexing.  -cim 1/13/91
 *
 *		The following code contains many undocumented hacks.  Please be
 *		careful....
 *
 */
#include <sys/types.h>
#include <errno.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>

#include "postgres.h"

#include "utils/builtins.h"
#include "access/genam.h"
#include "access/heapam.h"
#include "access/istrat.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/index.h"
#include "catalog/indexing.h"
#include "catalog/pg_attrdef.h"
#include "catalog/pg_log.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_relcheck.h"
#include "catalog/pg_rewrite.h"
#include "catalog/pg_type.h"
#include "catalog/pg_variable.h"
#include "commands/trigger.h"
#include "lib/hasht.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"
#include "storage/smgr.h"
#include "utils/catcache.h"
#include "utils/relcache.h"
#include "utils/temprel.h"


/* ----------------
 *		hardcoded tuple descriptors.  see lib/backend/catalog/pg_attribute.h
 * ----------------
 */
static FormData_pg_attribute Desc_pg_class[Natts_pg_class] = {Schema_pg_class};
static FormData_pg_attribute Desc_pg_attribute[Natts_pg_attribute] = {Schema_pg_attribute};
static FormData_pg_attribute Desc_pg_proc[Natts_pg_proc] = {Schema_pg_proc};
static FormData_pg_attribute Desc_pg_type[Natts_pg_type] = {Schema_pg_type};
static FormData_pg_attribute Desc_pg_variable[Natts_pg_variable] = {Schema_pg_variable};
static FormData_pg_attribute Desc_pg_log[Natts_pg_log] = {Schema_pg_log};

/* ----------------
 *		Hash tables that index the relation cache
 *
 *		Relations are cached two ways, by name and by id,
 *		thus there are two hash tables for referencing them.
 * ----------------
 */
static HTAB *RelationNameCache;
static HTAB *RelationIdCache;

/*
 * newlyCreatedRelns -
 *	  relations created during this transaction. We need to keep track of
 *	  these.
 */
static List *newlyCreatedRelns = NULL;


/* ----------------
 *		RelationBuildDescInfo exists so code can be shared
 *		between RelationIdGetRelation() and RelationNameGetRelation()
 * ----------------
 */
typedef struct RelationBuildDescInfo
{
	int			infotype;		/* lookup by id or by name */
#define INFO_RELID 1
#define INFO_RELNAME 2
	union
	{
		Oid			info_id;	/* relation object id */
		char	   *info_name;	/* relation name */
	}			i;
} RelationBuildDescInfo;

typedef struct relidcacheent
{
	Oid			reloid;
	Relation	reldesc;
} RelIdCacheEnt;

typedef struct relnamecacheent
{
	NameData	relname;
	Relation	reldesc;
} RelNameCacheEnt;

/* -----------------
 *		macros to manipulate name cache and id cache
 * -----------------
 */
#define RelationCacheInsert(RELATION)	\
do { \
	RelIdCacheEnt *idhentry; RelNameCacheEnt *namehentry; \
	char *relname; Oid reloid; bool found; \
	relname = RelationGetPhysicalRelationName(RELATION); \
	namehentry = (RelNameCacheEnt*)hash_search(RelationNameCache, \
											   relname, \
											   HASH_ENTER, \
											   &found); \
	if (namehentry == NULL) \
		elog(FATAL, "can't insert into relation descriptor cache"); \
	if (found && !IsBootstrapProcessingMode()) \
		/* used to give notice -- now just keep quiet */ ; \
	namehentry->reldesc = RELATION; \
	reloid = RELATION->rd_id; \
	idhentry = (RelIdCacheEnt*)hash_search(RelationIdCache, \
										   (char *)&reloid, \
										   HASH_ENTER, \
										   &found); \
	if (idhentry == NULL) \
		elog(FATAL, "can't insert into relation descriptor cache"); \
	if (found && !IsBootstrapProcessingMode()) \
		/* used to give notice -- now just keep quiet */ ; \
	idhentry->reldesc = RELATION; \
} while(0)

#define RelationNameCacheLookup(NAME, RELATION) \
do { \
	RelNameCacheEnt *hentry; bool found; \
	hentry = (RelNameCacheEnt*)hash_search(RelationNameCache, \
										   (char *)NAME,HASH_FIND,&found); \
	if (hentry == NULL) \
		elog(FATAL, "error in CACHE"); \
	if (found) \
		RELATION = hentry->reldesc; \
	else \
		RELATION = NULL; \
} while(0)

#define RelationIdCacheLookup(ID, RELATION) \
do { \
	RelIdCacheEnt *hentry; \
	bool found; \
	hentry = (RelIdCacheEnt*)hash_search(RelationIdCache, \
										 (char *)&(ID),HASH_FIND, &found); \
	if (hentry == NULL) \
		elog(FATAL, "error in CACHE"); \
	if (found) \
		RELATION = hentry->reldesc; \
	else \
		RELATION = NULL; \
} while(0)

#define RelationCacheDelete(RELATION) \
do { \
	RelNameCacheEnt *namehentry; RelIdCacheEnt *idhentry; \
	char *relname; Oid reloid; bool found; \
	relname = RelationGetPhysicalRelationName(RELATION); \
	namehentry = (RelNameCacheEnt*)hash_search(RelationNameCache, \
											   relname, \
											   HASH_REMOVE, \
											   &found); \
	if (namehentry == NULL) \
		elog(FATAL, "can't delete from relation descriptor cache"); \
	if (!found) \
		elog(NOTICE, "trying to delete a reldesc that does not exist."); \
	reloid = RELATION->rd_id; \
	idhentry = (RelIdCacheEnt*)hash_search(RelationIdCache, \
										   (char *)&reloid, \
										   HASH_REMOVE, &found); \
	if (idhentry == NULL) \
		elog(FATAL, "can't delete from relation descriptor cache"); \
	if (!found) \
		elog(NOTICE, "trying to delete a reldesc that does not exist."); \
} while(0)

/* non-export function prototypes */

static void RelationClearRelation(Relation relation, bool rebuildIt);
static void RelationFlushRelation(Relation *relationPtr,
					  int skipLocalRelations);
static Relation RelationNameCacheGetRelation(const char *relationName);
static void RelationCacheAbortWalker(Relation *relationPtr, int dummy);
static void init_irels(void);
static void write_irels(void);

static void formrdesc(char *relationName, u_int natts,
		  FormData_pg_attribute *att);

static HeapTuple ScanPgRelation(RelationBuildDescInfo buildinfo);
static HeapTuple scan_pg_rel_seq(RelationBuildDescInfo buildinfo);
static HeapTuple scan_pg_rel_ind(RelationBuildDescInfo buildinfo);
static Relation AllocateRelationDesc(Relation relation, u_int natts,
					 Form_pg_class relp);
static void RelationBuildTupleDesc(RelationBuildDescInfo buildinfo,
					   Relation relation, u_int natts);
static void build_tupdesc_seq(RelationBuildDescInfo buildinfo,
				  Relation relation, u_int natts);
static void build_tupdesc_ind(RelationBuildDescInfo buildinfo,
				  Relation relation, u_int natts);
static Relation RelationBuildDesc(RelationBuildDescInfo buildinfo,
				  Relation oldrelation);
static void IndexedAccessMethodInitialize(Relation relation);
static void AttrDefaultFetch(Relation relation);
static void RelCheckFetch(Relation relation);

static bool criticalRelcacheBuild = false;

/* ----------------------------------------------------------------
 *		RelationIdGetRelation() and RelationNameGetRelation()
 *						support functions
 * ----------------------------------------------------------------
 */


/* --------------------------------
 *		ScanPgRelation
 *
 *		this is used by RelationBuildDesc to find a pg_class
 *		tuple matching either a relation name or a relation id
 *		as specified in buildinfo.
 * --------------------------------
 */
static HeapTuple
ScanPgRelation(RelationBuildDescInfo buildinfo)
{

	/*
	 * If this is bootstrap time (initdb), then we can't use the system
	 * catalog indices, because they may not exist yet.  Otherwise, we
	 * can, and do.
	 */

	if (IsIgnoringSystemIndexes() || !criticalRelcacheBuild)
		return scan_pg_rel_seq(buildinfo);
	else
		return scan_pg_rel_ind(buildinfo);
}

static HeapTuple
scan_pg_rel_seq(RelationBuildDescInfo buildinfo)
{
	HeapTuple	pg_class_tuple;
	HeapTuple	return_tuple;
	Relation	pg_class_desc;
	HeapScanDesc pg_class_scan;
	ScanKeyData key;

	/* ----------------
	 *	form a scan key
	 * ----------------
	 */
	switch (buildinfo.infotype)
	{
		case INFO_RELID:
			ScanKeyEntryInitialize(&key, 0,
								   ObjectIdAttributeNumber,
								   F_OIDEQ,
								   ObjectIdGetDatum(buildinfo.i.info_id));
			break;

		case INFO_RELNAME:
			ScanKeyEntryInitialize(&key, 0,
								   Anum_pg_class_relname,
								   F_NAMEEQ,
								   NameGetDatum(buildinfo.i.info_name));
			break;

		default:
			elog(ERROR, "ScanPgRelation: bad buildinfo");
			return NULL;
	}

	/* ----------------
	 *	open pg_class and fetch a tuple
	 * ----------------
	 */
	pg_class_desc = heap_openr(RelationRelationName, AccessShareLock);
	pg_class_scan = heap_beginscan(pg_class_desc, 0, SnapshotNow, 1, &key);
	pg_class_tuple = heap_getnext(pg_class_scan, 0);

	/* ----------------
	 *	get set to return tuple
	 * ----------------
	 */
	if (!HeapTupleIsValid(pg_class_tuple))
		return_tuple = pg_class_tuple;
	else
	{
		/* ------------------
		 *	a satanic bug used to live here: pg_class_tuple used to be
		 *	returned here without having the corresponding buffer pinned.
		 *	so when the buffer gets replaced, all hell breaks loose.
		 *	this bug is discovered and killed by wei on 9/27/91.
		 * -------------------
		 */
		return_tuple = heap_copytuple(pg_class_tuple);
	}

	/* all done */
	heap_endscan(pg_class_scan);
	heap_close(pg_class_desc, AccessShareLock);

	return return_tuple;
}

static HeapTuple
scan_pg_rel_ind(RelationBuildDescInfo buildinfo)
{
	Relation	pg_class_desc;
	HeapTuple	return_tuple;

	pg_class_desc = heap_openr(RelationRelationName, AccessShareLock);

	switch (buildinfo.infotype)
	{
		case INFO_RELID:
			return_tuple = ClassOidIndexScan(pg_class_desc, buildinfo.i.info_id);
			break;

		case INFO_RELNAME:
			return_tuple = ClassNameIndexScan(pg_class_desc,
											  buildinfo.i.info_name);
			break;

		default:
			elog(ERROR, "ScanPgRelation: bad buildinfo");
			return_tuple = NULL;/* keep compiler quiet */
	}

	heap_close(pg_class_desc, AccessShareLock);

	return return_tuple;
}

/* ----------------
 *		AllocateRelationDesc
 *
 *		This is used to allocate memory for a new relation descriptor
 *		and initialize the rd_rel field.
 *
 *		If 'relation' is NULL, allocate a new RelationData object.
 *		If not, reuse the given object (that path is taken only when
 *		we have to rebuild a relcache entry during RelationClearRelation).
 * ----------------
 */
static Relation
AllocateRelationDesc(Relation relation, u_int natts,
					 Form_pg_class relp)
{
	Form_pg_class relationForm;

	/* ----------------
	 *	allocate space for the relation tuple form
	 * ----------------
	 */
	relationForm = (Form_pg_class) palloc(sizeof(FormData_pg_class));

	memcpy((char *) relationForm, (char *) relp, CLASS_TUPLE_SIZE);

	/* ----------------
	 *	allocate space for new relation descriptor, if needed
	 */
	if (relation == NULL)
		relation = (Relation) palloc(sizeof(RelationData));

	/* ----------------
	 *	clear new reldesc
	 * ----------------
	 */
	MemSet((char *) relation, 0, sizeof(RelationData));

	/* make sure relation is marked as having no open file yet */
	relation->rd_fd = -1;

	/* initialize attribute tuple form */
	relation->rd_att = CreateTemplateTupleDesc(natts);

	/* and initialize relation tuple form */
	relation->rd_rel = relationForm;

	return relation;
}

/* --------------------------------
 *		RelationBuildTupleDesc
 *
 *		Form the relation's tuple descriptor from information in
 *		the pg_attribute, pg_attrdef & pg_relcheck system cataloges.
 * --------------------------------
 */
static void
RelationBuildTupleDesc(RelationBuildDescInfo buildinfo,
					   Relation relation,
					   u_int natts)
{

	/*
	 * If this is bootstrap time (initdb), then we can't use the system
	 * catalog indices, because they may not exist yet.  Otherwise, we
	 * can, and do.
	 */

	if (IsIgnoringSystemIndexes() || !criticalRelcacheBuild)
		build_tupdesc_seq(buildinfo, relation, natts);
	else
		build_tupdesc_ind(buildinfo, relation, natts);
}

static void
SetConstrOfRelation(Relation relation, TupleConstr *constr, int ndef, AttrDefault *attrdef)
{
	if (constr->has_not_null || ndef > 0 || relation->rd_rel->relchecks)
	{
		relation->rd_att->constr = constr;

		if (ndef > 0)			/* DEFAULTs */
		{
			if (ndef < relation->rd_rel->relnatts)
				constr->defval = (AttrDefault *)
					repalloc(attrdef, ndef * sizeof(AttrDefault));
			else
				constr->defval = attrdef;
			constr->num_defval = ndef;
			AttrDefaultFetch(relation);
		}
		else
			constr->num_defval = 0;

		if (relation->rd_rel->relchecks > 0)	/* CHECKs */
		{
			constr->num_check = relation->rd_rel->relchecks;
			constr->check = (ConstrCheck *) palloc(constr->num_check *
												   sizeof(ConstrCheck));
			MemSet(constr->check, 0, constr->num_check * sizeof(ConstrCheck));
			RelCheckFetch(relation);
		}
		else
			constr->num_check = 0;
	}
	else
	{
		pfree(constr);
		relation->rd_att->constr = NULL;
	}
}

static void
build_tupdesc_seq(RelationBuildDescInfo buildinfo,
				  Relation relation,
				  u_int natts)
{
	HeapTuple	pg_attribute_tuple;
	Relation	pg_attribute_desc;
	HeapScanDesc pg_attribute_scan;
	Form_pg_attribute attp;
	ScanKeyData key;
	int			need;
	TupleConstr *constr = (TupleConstr *) palloc(sizeof(TupleConstr));
	AttrDefault *attrdef = NULL;
	int			ndef = 0;

	constr->has_not_null = false;
	/* ----------------
	 *	form a scan key
	 * ----------------
	 */
	ScanKeyEntryInitialize(&key, 0,
						   Anum_pg_attribute_attrelid,
						   F_OIDEQ,
						   ObjectIdGetDatum(RelationGetRelid(relation)));

	/* ----------------
	 *	open pg_attribute and begin a scan
	 * ----------------
	 */
	pg_attribute_desc = heap_openr(AttributeRelationName, AccessShareLock);
	pg_attribute_scan = heap_beginscan(pg_attribute_desc, 0, SnapshotNow, 1, &key);

	/* ----------------
	 *	add attribute data to relation->rd_att
	 * ----------------
	 */
	need = natts;

	pg_attribute_tuple = heap_getnext(pg_attribute_scan, 0);
	while (HeapTupleIsValid(pg_attribute_tuple) && need > 0)
	{
		attp = (Form_pg_attribute) GETSTRUCT(pg_attribute_tuple);

		if (attp->attnum > 0)
		{
			relation->rd_att->attrs[attp->attnum - 1] =
				(Form_pg_attribute) palloc(ATTRIBUTE_TUPLE_SIZE);

			memmove((char *) (relation->rd_att->attrs[attp->attnum - 1]),
					(char *) attp,
					ATTRIBUTE_TUPLE_SIZE);
			need--;
			/* Update if this attribute have a constraint */
			if (attp->attnotnull)
				constr->has_not_null = true;

			if (attp->atthasdef)
			{
				if (attrdef == NULL)
				{
					attrdef = (AttrDefault *) palloc(relation->rd_rel->relnatts *
													 sizeof(AttrDefault));
					MemSet(attrdef, 0,
					   relation->rd_rel->relnatts * sizeof(AttrDefault));
				}
				attrdef[ndef].adnum = attp->attnum;
				attrdef[ndef].adbin = NULL;
				ndef++;
			}
		}
		pg_attribute_tuple = heap_getnext(pg_attribute_scan, 0);
	}

	if (need > 0)
		elog(ERROR, "catalog is missing %d attribute%s for relid %u",
			 need, (need == 1 ? "" : "s"), RelationGetRelid(relation));

	/* ----------------
	 *	end the scan and close the attribute relation
	 * ----------------
	 */
	heap_endscan(pg_attribute_scan);
	heap_close(pg_attribute_desc, AccessShareLock);

	SetConstrOfRelation(relation, constr, ndef, attrdef);
}

static void
build_tupdesc_ind(RelationBuildDescInfo buildinfo,
				  Relation relation,
				  u_int natts)
{
	Relation	attrel;
	HeapTuple	atttup;
	Form_pg_attribute attp;
	TupleConstr *constr = (TupleConstr *) palloc(sizeof(TupleConstr));
	AttrDefault *attrdef = NULL;
	int			ndef = 0;
	int			i;

#ifdef	_DROP_COLUMN_HACK__
	bool		columnDropped;

#endif	 /* _DROP_COLUMN_HACK__ */

	constr->has_not_null = false;

	attrel = heap_openr(AttributeRelationName, AccessShareLock);

	for (i = 1; i <= relation->rd_rel->relnatts; i++)
	{
#ifdef	_DROP_COLUMN_HACK__
		columnDropped = false;
#endif	 /* _DROP_COLUMN_HACK__ */
		atttup = (HeapTuple) AttributeRelidNumIndexScan(attrel,
										  RelationGetRelid(relation), i);

		if (!HeapTupleIsValid(atttup))
#ifdef	_DROP_COLUMN_HACK__
		{
			atttup = (HeapTuple) AttributeRelidNumIndexScan(attrel,
					RelationGetRelid(relation), DROPPED_COLUMN_INDEX(i));
			if (!HeapTupleIsValid(atttup))
#endif	 /* _DROP_COLUMN_HACK__ */
				elog(ERROR, "cannot find attribute %d of relation %s", i,
					 RelationGetRelationName(relation));
#ifdef	_DROP_COLUMN_HACK__
			columnDropped = true;
		}
#endif	 /* _DROP_COLUMN_HACK__ */
		attp = (Form_pg_attribute) GETSTRUCT(atttup);

		relation->rd_att->attrs[i - 1] =
			(Form_pg_attribute) palloc(ATTRIBUTE_TUPLE_SIZE);

		memmove((char *) (relation->rd_att->attrs[i - 1]),
				(char *) attp,
				ATTRIBUTE_TUPLE_SIZE);

#ifdef	_DROP_COLUMN_HACK__
		if (columnDropped)
			continue;
#endif	 /* _DROP_COLUMN_HACK__ */
		/* Update if this attribute have a constraint */
		if (attp->attnotnull)
			constr->has_not_null = true;

		if (attp->atthasdef)
		{
			if (attrdef == NULL)
			{
				attrdef = (AttrDefault *) palloc(relation->rd_rel->relnatts *
												 sizeof(AttrDefault));
				MemSet(attrdef, 0,
					   relation->rd_rel->relnatts * sizeof(AttrDefault));
			}
			attrdef[ndef].adnum = i;
			attrdef[ndef].adbin = NULL;
			ndef++;
		}
	}

	heap_close(attrel, AccessShareLock);

	SetConstrOfRelation(relation, constr, ndef, attrdef);

}

/* --------------------------------
 *		RelationBuildRuleLock
 *
 *		Form the relation's rewrite rules from information in
 *		the pg_rewrite system catalog.
 * --------------------------------
 */
static void
RelationBuildRuleLock(Relation relation)
{
	HeapTuple	pg_rewrite_tuple;
	Relation	pg_rewrite_desc;
	TupleDesc	pg_rewrite_tupdesc;
	HeapScanDesc pg_rewrite_scan;
	ScanKeyData key;
	RuleLock   *rulelock;
	int			numlocks;
	RewriteRule **rules;
	int			maxlocks;

	/* ----------------
	 *	form an array to hold the rewrite rules (the array is extended if
	 *	necessary)
	 * ----------------
	 */
	maxlocks = 4;
	rules = (RewriteRule **) palloc(sizeof(RewriteRule *) * maxlocks);
	numlocks = 0;

	/* ----------------
	 *	form a scan key
	 * ----------------
	 */
	ScanKeyEntryInitialize(&key, 0,
						   Anum_pg_rewrite_ev_class,
						   F_OIDEQ,
						   ObjectIdGetDatum(RelationGetRelid(relation)));

	/* ----------------
	 *	open pg_rewrite and begin a scan
	 * ----------------
	 */
	pg_rewrite_desc = heap_openr(RewriteRelationName, AccessShareLock);
	pg_rewrite_scan = heap_beginscan(pg_rewrite_desc, 0, SnapshotNow, 1, &key);
	pg_rewrite_tupdesc = RelationGetDescr(pg_rewrite_desc);

	while (HeapTupleIsValid(pg_rewrite_tuple = heap_getnext(pg_rewrite_scan, 0)))
	{
		bool		isnull;
		Datum		ruleaction;
		Datum		rule_evqual;
		char	   *ruleaction_str;
		char	   *rule_evqual_str;
		RewriteRule *rule;

		rule = (RewriteRule *) palloc(sizeof(RewriteRule));

		rule->ruleId = pg_rewrite_tuple->t_data->t_oid;

		rule->event = (int) heap_getattr(pg_rewrite_tuple,
							 Anum_pg_rewrite_ev_type, pg_rewrite_tupdesc,
										 &isnull) - 48;
		rule->attrno = (int) heap_getattr(pg_rewrite_tuple,
							 Anum_pg_rewrite_ev_attr, pg_rewrite_tupdesc,
										  &isnull);
		rule->isInstead = !!heap_getattr(pg_rewrite_tuple,
						  Anum_pg_rewrite_is_instead, pg_rewrite_tupdesc,
										 &isnull);

		ruleaction = heap_getattr(pg_rewrite_tuple,
								  Anum_pg_rewrite_ev_action,
								  pg_rewrite_tupdesc,
								  &isnull);
		ruleaction_str = lztextout((lztext *) DatumGetPointer(ruleaction));
		rule->actions = (List *) stringToNode(ruleaction_str);
		pfree(ruleaction_str);

		rule_evqual = heap_getattr(pg_rewrite_tuple,
								   Anum_pg_rewrite_ev_qual,
								   pg_rewrite_tupdesc,
								   &isnull);
		rule_evqual_str = lztextout((lztext *) DatumGetPointer(rule_evqual));
		rule->qual = (Node *) stringToNode(rule_evqual_str);
		pfree(rule_evqual_str);

		if (numlocks >= maxlocks)
		{
			maxlocks *= 2;
			rules = (RewriteRule **) repalloc(rules, sizeof(RewriteRule *) * maxlocks);
		}
		rules[numlocks++] = rule;
	}

	/* ----------------
	 *	end the scan and close the attribute relation
	 * ----------------
	 */
	heap_endscan(pg_rewrite_scan);
	heap_close(pg_rewrite_desc, AccessShareLock);

	/* ----------------
	 *	form a RuleLock and insert into relation
	 * ----------------
	 */
	rulelock = (RuleLock *) palloc(sizeof(RuleLock));
	rulelock->numLocks = numlocks;
	rulelock->rules = rules;

	relation->rd_rules = rulelock;
}

/* --------------------------------
 *		FreeRuleLock
 *
 *		Release the storage used for a set of rewrite rules.
 *
 *		Probably this should be in the rules code someplace...
 * --------------------------------
 */
static void
FreeRuleLock(RuleLock *rlock)
{
	int			i;

	if (rlock == NULL)
		return;
	for (i = 0; i < rlock->numLocks; i++)
	{
		RewriteRule *rule = rlock->rules[i];

#if 0							/* does freefuncs.c still work?  Not sure */
		freeObject(rule->actions);
		freeObject(rule->qual);
#endif
		pfree(rule);
	}
	pfree(rlock->rules);
	pfree(rlock);
}

/* --------------------------------
 *		equalRuleLocks
 *
 *		Determine whether two RuleLocks are equivalent
 *
 *		Probably this should be in the rules code someplace...
 * --------------------------------
 */
static bool
equalRuleLocks(RuleLock *rlock1, RuleLock *rlock2)
{
	int			i,
				j;

	if (rlock1 != NULL)
	{
		if (rlock2 == NULL)
			return false;
		if (rlock1->numLocks != rlock2->numLocks)
			return false;
		for (i = 0; i < rlock1->numLocks; i++)
		{
			RewriteRule *rule1 = rlock1->rules[i];
			RewriteRule *rule2 = NULL;

			/*
			 * We can't assume that the rules are always read from
			 * pg_rewrite in the same order; so use the rule OIDs to
			 * identify the rules to compare.  (We assume here that the
			 * same OID won't appear twice in either ruleset.)
			 */
			for (j = 0; j < rlock2->numLocks; j++)
			{
				rule2 = rlock2->rules[j];
				if (rule1->ruleId == rule2->ruleId)
					break;
			}
			if (j >= rlock2->numLocks)
				return false;
			if (rule1->event != rule2->event)
				return false;
			if (rule1->attrno != rule2->attrno)
				return false;
			if (rule1->isInstead != rule2->isInstead)
				return false;
			if (!equal(rule1->qual, rule2->qual))
				return false;
			if (!equal(rule1->actions, rule2->actions))
				return false;
		}
	}
	else if (rlock2 != NULL)
		return false;
	return true;
}


/* --------------------------------
 *		RelationBuildDesc
 *
 *		Build a relation descriptor --- either a new one, or by
 *		recycling the given old relation object.  The latter case
 *		supports rebuilding a relcache entry without invalidating
 *		pointers to it.
 *
 *		To build a relation descriptor, we have to allocate space,
 *		open the underlying unix file and initialize the following
 *		fields:
 *
 *	File				   rd_fd;		 open file descriptor
 *	int					   rd_nblocks;	 number of blocks in rel
 *										 it will be set in ambeginscan()
 *	uint16				   rd_refcnt;	 reference count
 *	Form_pg_am			   rd_am;		 AM tuple
 *	Form_pg_class		   rd_rel;		 RELATION tuple
 *	Oid					   rd_id;		 relation's object id
 *	LockInfoData		   rd_lockInfo;  lock manager's info
 *	TupleDesc			   rd_att;		 tuple descriptor
 *
 *		Note: rd_ismem (rel is in-memory only) is currently unused
 *		by any part of the system.	someday this will indicate that
 *		the relation lives only in the main-memory buffer pool
 *		-cim 2/4/91
 * --------------------------------
 */
static Relation
RelationBuildDesc(RelationBuildDescInfo buildinfo,
				  Relation oldrelation)
{
	File		fd;
	Relation	relation;
	u_int		natts;
	Oid			relid;
	Oid			relam;
	HeapTuple	pg_class_tuple;
	Form_pg_class relp;
	MemoryContext oldcxt;

	/* ----------------
	 *	find the tuple in pg_class corresponding to the given relation id
	 * ----------------
	 */
	pg_class_tuple = ScanPgRelation(buildinfo);

	/* ----------------
	 *	if no such tuple exists, return NULL
	 * ----------------
	 */
	if (!HeapTupleIsValid(pg_class_tuple))
		return NULL;

	/* ----------------
	 *	get information from the pg_class_tuple
	 * ----------------
	 */
	relid = pg_class_tuple->t_data->t_oid;
	relp = (Form_pg_class) GETSTRUCT(pg_class_tuple);
	natts = relp->relnatts;

	/* ----------------
	 *	allocate storage for the relation descriptor,
	 *	initialize relation->rd_rel and get the access method id.
	 *	The storage is allocated in memory context CacheCxt.
	 * ----------------
	 */
	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);
	relation = AllocateRelationDesc(oldrelation, natts, relp);
	relam = relation->rd_rel->relam;

	/* ----------------
	 *	initialize the relation's relation id (relation->rd_id)
	 * ----------------
	 */
	RelationGetRelid(relation) = relid;

	/* ----------------
	 *	initialize relation->rd_refcnt
	 * ----------------
	 */
	RelationSetReferenceCount(relation, 1);

	/* ----------------
	 *	 normal relations are not nailed into the cache
	 * ----------------
	 */
	relation->rd_isnailed = false;

	/* ----------------
	 *	initialize the access method information (relation->rd_am)
	 * ----------------
	 */
	if (OidIsValid(relam))
		relation->rd_am = AccessMethodObjectIdGetForm(relam);

	/* ----------------
	 *	initialize the tuple descriptor (relation->rd_att).
	 * ----------------
	 */
	RelationBuildTupleDesc(buildinfo, relation, natts);

	/* ----------------
	 *	initialize rules that affect this relation
	 * ----------------
	 */
	if (relp->relhasrules)
		RelationBuildRuleLock(relation);
	else
		relation->rd_rules = NULL;

	/* Triggers */
	if (relp->reltriggers > 0)
		RelationBuildTriggers(relation);
	else
		relation->trigdesc = NULL;

	/* ----------------
	 *	initialize index strategy and support information for this relation
	 * ----------------
	 */
	if (OidIsValid(relam))
		IndexedAccessMethodInitialize(relation);

	/* ----------------
	 *	initialize the relation lock manager information
	 * ----------------
	 */
	RelationInitLockInfo(relation);		/* see lmgr.c */

	/* ----------------
	 *	open the relation and assign the file descriptor returned
	 *	by the storage manager code to rd_fd.
	 * ----------------
	 */
	fd = smgropen(DEFAULT_SMGR, relation);

	Assert(fd >= -1);
	if (fd == -1)
		elog(NOTICE, "RelationIdBuildRelation: smgropen(%s): %m",
			 NameStr(relp->relname));

	relation->rd_fd = fd;

	/* ----------------
	 *	insert newly created relation into proper relcaches,
	 *	restore memory context and return the new reldesc.
	 * ----------------
	 */
	RelationCacheInsert(relation);

	MemoryContextSwitchTo(oldcxt);

	/* -------------------
	 *	free the memory allocated for pg_class_tuple
	 *	and for lock data pointed to by pg_class_tuple
	 * -------------------
	 */
	heap_freetuple(pg_class_tuple);

	return relation;
}

static void
IndexedAccessMethodInitialize(Relation relation)
{
	IndexStrategy strategy;
	RegProcedure *support;
	int			natts;
	Size		stratSize;
	Size		supportSize;
	uint16		relamstrategies;
	uint16		relamsupport;

	natts = relation->rd_rel->relnatts;
	relamstrategies = relation->rd_am->amstrategies;
	stratSize = AttributeNumberGetIndexStrategySize(natts, relamstrategies);
	strategy = (IndexStrategy) palloc(stratSize);
	relamsupport = relation->rd_am->amsupport;

	if (relamsupport > 0)
	{
		supportSize = natts * (relamsupport * sizeof(RegProcedure));
		support = (RegProcedure *) palloc(supportSize);
	}
	else
		support = (RegProcedure *) NULL;

	IndexSupportInitialize(strategy, support,
						   relation->rd_att->attrs[0]->attrelid,
						   relation->rd_rel->relam,
						   relamstrategies, relamsupport, natts);

	RelationSetIndexSupport(relation, strategy, support);
}

/* --------------------------------
 *		formrdesc
 *
 *		This is a special version of RelationBuildDesc()
 *		used by RelationInitialize() in initializing the
 *		relcache.  The system relation descriptors built
 *		here are all nailed in the descriptor caches, for
 *		bootstrapping.
 * --------------------------------
 */
static void
formrdesc(char *relationName,
		  u_int natts,
		  FormData_pg_attribute *att)
{
	Relation	relation;
	Size		len;
	u_int		i;

	/* ----------------
	 *	allocate new relation desc
	 * ----------------
	 */
	len = sizeof(RelationData);
	relation = (Relation) palloc(len);
	MemSet((char *) relation, 0, len);

	/* ----------------
	 *	don't open the unix file yet..
	 * ----------------
	 */
	relation->rd_fd = -1;

	/* ----------------
	 *	initialize reference count
	 * ----------------
	 */
	RelationSetReferenceCount(relation, 1);

	/* ----------------
	 *	initialize relation tuple form
	 * ----------------
	 */
	relation->rd_rel = (Form_pg_class)
		palloc((Size) (sizeof(*relation->rd_rel)));
	MemSet(relation->rd_rel, 0, sizeof(FormData_pg_class));
	strcpy(RelationGetPhysicalRelationName(relation), relationName);

	/* ----------------
	   initialize attribute tuple form
	*/
	relation->rd_att = CreateTemplateTupleDesc(natts);

	/*
	 * For debugging purposes, it's important to distinguish between
	 * shared and non-shared relations, even at bootstrap time.  There's
	 * code in the buffer manager that traces allocations that has to know
	 * about this.
	 */

	if (IsSystemRelationName(relationName))
	{
		relation->rd_rel->relowner = 6; /* XXX use sym const */
		relation->rd_rel->relisshared = IsSharedSystemRelationName(relationName);
	}
	else
	{
		relation->rd_rel->relowner = 0;
		relation->rd_rel->relisshared = false;
	}

	relation->rd_rel->relpages = 1;		/* XXX */
	relation->rd_rel->reltuples = 1;	/* XXX */
	relation->rd_rel->relkind = RELKIND_RELATION;
	relation->rd_rel->relnatts = (uint16) natts;
	relation->rd_isnailed = true;

	/* ----------------
	 *	initialize tuple desc info
	 * ----------------
	 */
	for (i = 0; i < natts; i++)
	{
		relation->rd_att->attrs[i] = (Form_pg_attribute) palloc(ATTRIBUTE_TUPLE_SIZE);

		MemSet((char *) relation->rd_att->attrs[i], 0,
			   ATTRIBUTE_TUPLE_SIZE);
		memmove((char *) relation->rd_att->attrs[i],
				(char *) &att[i],
				ATTRIBUTE_TUPLE_SIZE);
	}

	/* ----------------
	 *	initialize relation id
	 * ----------------
	 */
	RelationGetRelid(relation) = relation->rd_att->attrs[0]->attrelid;

	/* ----------------
	 *	initialize the relation lock manager information
	 * ----------------
	 */
	RelationInitLockInfo(relation);		/* see lmgr.c */

	/* ----------------
	 *	add new reldesc to relcache
	 * ----------------
	 */
	RelationCacheInsert(relation);

	/*
	 * Determining this requires a scan on pg_class, but to do the scan
	 * the rdesc for pg_class must already exist.  Therefore we must do
	 * the check (and possible set) after cache insertion.
	 */
	relation->rd_rel->relhasindex =
		CatalogHasIndex(relationName, RelationGetRelid(relation));
}


/* ----------------------------------------------------------------
 *				 Relation Descriptor Lookup Interface
 * ----------------------------------------------------------------
 */

/* --------------------------------
 *		RelationIdCacheGetRelation
 *
 *		Lookup a reldesc by OID.
 *		Only try to get the reldesc by looking up the cache
 *		do not go to the disk.
 *
 *		NB: relation ref count is incremented if successful.
 *		Caller should eventually decrement count.  (Usually,
 *		that happens by calling RelationClose().)
 * --------------------------------
 */
Relation
RelationIdCacheGetRelation(Oid relationId)
{
	Relation	rd;

	RelationIdCacheLookup(relationId, rd);

	if (RelationIsValid(rd))
	{
		if (rd->rd_fd == -1)
		{
			rd->rd_fd = smgropen(DEFAULT_SMGR, rd);
			Assert(rd->rd_fd != -1 || rd->rd_unlinked);
		}

		RelationIncrementReferenceCount(rd);

	}

	return rd;
}

/* --------------------------------
 *		RelationNameCacheGetRelation
 *
 *		As above, but lookup by name.
 * --------------------------------
 */
static Relation
RelationNameCacheGetRelation(const char *relationName)
{
	Relation	rd;
	NameData	name;

	/*
	 * make sure that the name key used for hash lookup is properly
	 * null-padded
	 */
	namestrcpy(&name, relationName);
	RelationNameCacheLookup(NameStr(name), rd);

	if (RelationIsValid(rd))
	{
		if (rd->rd_fd == -1)
		{
			rd->rd_fd = smgropen(DEFAULT_SMGR, rd);
			Assert(rd->rd_fd != -1 || rd->rd_unlinked);
		}

		RelationIncrementReferenceCount(rd);

	}

	return rd;
}

/* --------------------------------
 *		RelationIdGetRelation
 *
 *		Lookup a reldesc by OID; make one if not already in cache.
 *
 *		NB: relation ref count is incremented, or set to 1 if new entry.
 *		Caller should eventually decrement count.  (Usually,
 *		that happens by calling RelationClose().)
 * --------------------------------
 */
Relation
RelationIdGetRelation(Oid relationId)
{
	Relation	rd;
	RelationBuildDescInfo buildinfo;

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_RelationIdGetRelation);
	IncrHeapAccessStat(global_RelationIdGetRelation);

	/* ----------------
	 *	first try and get a reldesc from the cache
	 * ----------------
	 */
	rd = RelationIdCacheGetRelation(relationId);
	if (RelationIsValid(rd))
		return rd;

	/* ----------------
	 *	no reldesc in the cache, so have RelationBuildDesc()
	 *	build one and add it.
	 * ----------------
	 */
	buildinfo.infotype = INFO_RELID;
	buildinfo.i.info_id = relationId;

	rd = RelationBuildDesc(buildinfo, NULL);
	return rd;
}

/* --------------------------------
 *		RelationNameGetRelation
 *
 *		As above, but lookup by name.
 * --------------------------------
 */
Relation
RelationNameGetRelation(const char *relationName)
{
	char	   *temprelname;
	Relation	rd;
	RelationBuildDescInfo buildinfo;

	/* ----------------
	 *	increment access statistics
	 * ----------------
	 */
	IncrHeapAccessStat(local_RelationNameGetRelation);
	IncrHeapAccessStat(global_RelationNameGetRelation);

	/* ----------------
	 *	if caller is looking for a temp relation, substitute its real name;
	 *	we only index temp rels by their real names.
	 * ----------------
	 */
	temprelname = get_temp_rel_by_username(relationName);
	if (temprelname)
		relationName = temprelname;

	/* ----------------
	 *	first try and get a reldesc from the cache
	 * ----------------
	 */
	rd = RelationNameCacheGetRelation(relationName);
	if (RelationIsValid(rd))
		return rd;

	/* ----------------
	 *	no reldesc in the cache, so have RelationBuildDesc()
	 *	build one and add it.
	 * ----------------
	 */
	buildinfo.infotype = INFO_RELNAME;
	buildinfo.i.info_name = (char *) relationName;

	rd = RelationBuildDesc(buildinfo, NULL);
	return rd;
}

/* ----------------------------------------------------------------
 *				cache invalidation support routines
 * ----------------------------------------------------------------
 */

/* --------------------------------
 * RelationClose - close an open relation
 *
 *	 Actually, we just decrement the refcount.
 * --------------------------------
 */
void
RelationClose(Relation relation)
{
	/* Note: no locking manipulations needed */
	RelationDecrementReferenceCount(relation);
}

/* --------------------------------
 * RelationClearRelation
 *
 *	 Physically blow away a relation cache entry, or reset it and rebuild
 *	 it from scratch (that is, from catalog entries).  The latter path is
 *	 usually used when we are notified of a change to an open relation
 *	 (one with refcount > 0).  However, this routine just does whichever
 *	 it's told to do; callers must determine which they want.
 *
 *	 If we detect a change in the relation's TupleDesc or trigger data
 *	 while rebuilding, we complain unless refcount is 0.
 * --------------------------------
 */
static void
RelationClearRelation(Relation relation, bool rebuildIt)
{
	MemoryContext oldcxt;

	/*
	 * Make sure smgr and lower levels close the relation's files, if they
	 * weren't closed already.  We do this unconditionally; if the
	 * relation is not deleted, the next smgr access should reopen the
	 * files automatically.  This ensures that the low-level file access
	 * state is updated after, say, a vacuum truncation.
	 *
	 * NOTE: this call is a no-op if the relation's smgr file is already
	 * closed or unlinked.
	 */
	smgrclose(DEFAULT_SMGR, relation);

	/*
	 * Never, never ever blow away a nailed-in system relation, because
	 * we'd be unable to recover.
	 */
	if (relation->rd_isnailed)
		return;

	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/*
	 * Remove relation from hash tables
	 *
	 * Note: we might be reinserting it momentarily, but we must not have it
	 * visible in the hash tables until it's valid again, so don't try to
	 * optimize this away...
	 */
	RelationCacheDelete(relation);

	/* Clear out catcache's entries for this relation */
	SystemCacheRelationFlushed(RelationGetRelid(relation));

	/*
	 * Free all the subsidiary data structures of the relcache entry. We
	 * cannot free rd_att if we are trying to rebuild the entry, however,
	 * because pointers to it may be cached in various places. The trigger
	 * manager might also have pointers into the trigdesc, and the rule
	 * manager might have pointers into the rewrite rules. So to begin
	 * with, we can only get rid of these fields:
	 */
	if (relation->rd_am)
		pfree(relation->rd_am);
	if (relation->rd_rel)
		pfree(relation->rd_rel);
	if (relation->rd_istrat)
		pfree(relation->rd_istrat);
	if (relation->rd_support)
		pfree(relation->rd_support);

	/*
	 * If we're really done with the relcache entry, blow it away. But if
	 * someone is still using it, reconstruct the whole deal without
	 * moving the physical RelationData record (so that the someone's
	 * pointer is still valid).
	 */
	if (!rebuildIt)
	{
		/* ok to zap remaining substructure */
		FreeTupleDesc(relation->rd_att);
		FreeRuleLock(relation->rd_rules);
		FreeTriggerDesc(relation->trigdesc);
		pfree(relation);
	}
	else
	{

		/*
		 * When rebuilding an open relcache entry, must preserve ref count
		 * and myxactonly flag.  Also attempt to preserve the tupledesc,
		 * rewrite rules, and trigger substructures in place. Furthermore
		 * we save/restore rd_nblocks (in case it is a local relation)
		 * *and* call RelationGetNumberOfBlocks (in case it isn't).
		 */
		uint16		old_refcnt = relation->rd_refcnt;
		bool		old_myxactonly = relation->rd_myxactonly;
		TupleDesc	old_att = relation->rd_att;
		RuleLock   *old_rules = relation->rd_rules;
		TriggerDesc *old_trigdesc = relation->trigdesc;
		int			old_nblocks = relation->rd_nblocks;
		bool		relDescChanged = false;
		RelationBuildDescInfo buildinfo;

		buildinfo.infotype = INFO_RELID;
		buildinfo.i.info_id = RelationGetRelid(relation);

		if (RelationBuildDesc(buildinfo, relation) != relation)
		{
			/* Should only get here if relation was deleted */
			FreeTupleDesc(old_att);
			FreeRuleLock(old_rules);
			FreeTriggerDesc(old_trigdesc);
			pfree(relation);
			elog(ERROR, "RelationClearRelation: relation %u deleted while still in use",
				 buildinfo.i.info_id);
		}
		RelationSetReferenceCount(relation, old_refcnt);
		relation->rd_myxactonly = old_myxactonly;
		if (equalTupleDescs(old_att, relation->rd_att))
		{
			FreeTupleDesc(relation->rd_att);
			relation->rd_att = old_att;
		}
		else
		{
			FreeTupleDesc(old_att);
			relDescChanged = true;
		}
		if (equalRuleLocks(old_rules, relation->rd_rules))
		{
			FreeRuleLock(relation->rd_rules);
			relation->rd_rules = old_rules;
		}
		else
		{
			FreeRuleLock(old_rules);
			relDescChanged = true;
		}
		if (equalTriggerDescs(old_trigdesc, relation->trigdesc))
		{
			FreeTriggerDesc(relation->trigdesc);
			relation->trigdesc = old_trigdesc;
		}
		else
		{
			FreeTriggerDesc(old_trigdesc);
			relDescChanged = true;
		}
		relation->rd_nblocks = old_nblocks;

		/*
		 * this is kind of expensive, but I think we must do it in case
		 * relation has been truncated...
		 */
		relation->rd_nblocks = RelationGetNumberOfBlocks(relation);

		if (relDescChanged && !RelationHasReferenceCountZero(relation))
			elog(ERROR, "RelationClearRelation: relation %u modified while in use",
				 buildinfo.i.info_id);
	}

	MemoryContextSwitchTo(oldcxt);
}

/* --------------------------------
 * RelationFlushRelation
 *
 *	 Rebuild the relation if it is open (refcount > 0), else blow it away.
 *	 If skipLocalRelations is TRUE, xact-local relations are ignored
 *	 (which is useful when processing SI cache reset, since xact-local
 *	 relations could not be targets of notifications from other backends).
 *
 *	 The peculiar calling convention (pointer to pointer to relation)
 *	 is needed so that we can use this routine as a hash table walker.
 * --------------------------------
 */
static void
RelationFlushRelation(Relation *relationPtr,
					  int skipLocalRelations)
{
	Relation	relation = *relationPtr;
	bool		rebuildIt;

	if (relation->rd_myxactonly)
	{
		if (skipLocalRelations)
			return;				/* don't touch local rels if so commanded */

		/*
		 * Local rels should always be rebuilt, not flushed; the relcache
		 * entry must live until RelationPurgeLocalRelation().
		 */
		rebuildIt = true;
	}
	else
	{

		/*
		 * Nonlocal rels can be dropped from the relcache if not open.
		 */
		rebuildIt = !RelationHasReferenceCountZero(relation);
	}

	RelationClearRelation(relation, rebuildIt);
}

/* --------------------------------
 * RelationForgetRelation -
 *
 *		   RelationClearRelation + if the relation is myxactonly then
 *		   remove the relation descriptor from the newly created
 *		   relation list.
 * --------------------------------
 */
void
RelationForgetRelation(Oid rid)
{
	Relation	relation;

	RelationIdCacheLookup(rid, relation);

	if (PointerIsValid(relation))
	{
		if (relation->rd_myxactonly)
		{
			MemoryContext oldcxt;
			List	   *curr;
			List	   *prev = NIL;

			oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

			foreach(curr, newlyCreatedRelns)
			{
				Relation	reln = lfirst(curr);

				Assert(reln != NULL && reln->rd_myxactonly);
				if (RelationGetRelid(reln) == rid)
					break;
				prev = curr;
			}
			if (curr == NIL)
				elog(FATAL, "Local relation %s not found in list",
					 RelationGetRelationName(relation));
			if (prev == NIL)
				newlyCreatedRelns = lnext(newlyCreatedRelns);
			else
				lnext(prev) = lnext(curr);
			pfree(curr);
			MemoryContextSwitchTo(oldcxt);
		}

		/* Unconditionally destroy the relcache entry */
		RelationClearRelation(relation, false);
	}
}

/* --------------------------------
 *		RelationIdInvalidateRelationCacheByRelationId
 *
 *		This routine is invoked for SI cache flush messages.
 *
 *		We used to skip local relations, on the grounds that they could
 *		not be targets of cross-backend SI update messages; but it seems
 *		safer to process them, so that our *own* SI update messages will
 *		have the same effects during CommandCounterIncrement for both
 *		local and nonlocal relations.
 * --------------------------------
 */
void
RelationIdInvalidateRelationCacheByRelationId(Oid relationId)
{
	Relation	relation;

	RelationIdCacheLookup(relationId, relation);

	if (PointerIsValid(relation))
		RelationFlushRelation(&relation, false);
}

#if NOT_USED
/* only used by RelationIdInvalidateRelationCacheByAccessMethodId,
 * which is dead code.
 */
static void
RelationFlushIndexes(Relation *r,
					 Oid accessMethodId)
{
	Relation	relation = *r;

	if (!RelationIsValid(relation))
	{
		elog(NOTICE, "inval call to RFI");
		return;
	}

	if (relation->rd_rel->relkind == RELKIND_INDEX &&	/* XXX style */
		(!OidIsValid(accessMethodId) ||
		 relation->rd_rel->relam == accessMethodId))
		RelationFlushRelation(&relation, false);
}

#endif


#if NOT_USED
void
RelationIdInvalidateRelationCacheByAccessMethodId(Oid accessMethodId)
{

	/*
	 * 25 aug 1992:  mao commented out the ht walk below.  it should be
	 * doing the right thing, in theory, but flushing reldescs for index
	 * relations apparently doesn't work.  we want to cut 4.0.1, and i
	 * don't want to introduce new bugs.  this code never executed before,
	 * so i'm turning it off for now.  after the release is cut, i'll fix
	 * this up.
	 *
	 * 20 nov 1999:  this code has still never done anything, so I'm cutting
	 * the routine out of the system entirely.	tgl
	 */

	HashTableWalk(RelationNameCache, (HashtFunc) RelationFlushIndexes,
				  accessMethodId);
}

#endif

/*
 * RelationCacheInvalidate
 *	 Blow away cached relation descriptors that have zero reference counts,
 *	 and rebuild those with positive reference counts.
 *
 *	 This is currently used only to recover from SI message buffer overflow,
 *	 so we do not touch transaction-local relations; they cannot be targets
 *	 of cross-backend SI updates (and our own updates now go through a
 *	 separate linked list that isn't limited by the SI message buffer size).
 */
void
RelationCacheInvalidate(void)
{
	HashTableWalk(RelationNameCache, (HashtFunc) RelationFlushRelation,
				  (int) true);
}

/*
 * RelationCacheAbort
 *
 *	Clean up the relcache at transaction abort.
 *
 *	What we need to do here is reset relcache entry ref counts to
 *	their normal not-in-a-transaction state.  A ref count may be
 *	too high because some routine was exited by elog() between
 *	incrementing and decrementing the count.
 *
 *	XXX Maybe we should do this at transaction commit, too, in case
 *	someone forgets to decrement a refcount in a non-error path?
 */
void
RelationCacheAbort(void)
{
	HashTableWalk(RelationNameCache, (HashtFunc) RelationCacheAbortWalker,
				  0);
}

static void
RelationCacheAbortWalker(Relation *relationPtr, int dummy)
{
	Relation	relation = *relationPtr;

	if (relation->rd_isnailed)
		RelationSetReferenceCount(relation, 1);
	else
		RelationSetReferenceCount(relation, 0);
}

/* --------------------------------
 *		RelationRegisterRelation -
 *		   register the Relation descriptor of a newly created relation
 *		   with the relation descriptor Cache.
 * --------------------------------
 */
void
RelationRegisterRelation(Relation relation)
{
	MemoryContext oldcxt;

	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	if (oldcxt != (MemoryContext) CacheCxt)
		elog(NOIND, "RelationRegisterRelation: WARNING: Context != CacheCxt");

	RelationInitLockInfo(relation);

	RelationCacheInsert(relation);

	/*
	 * we've just created the relation. It is invisible to anyone else
	 * before the transaction is committed. Setting rd_myxactonly allows
	 * us to use the local buffer manager for select/insert/etc before the
	 * end of transaction. (We also need to keep track of relations
	 * created during a transaction and does the necessary clean up at the
	 * end of the transaction.)				- ay 3/95
	 */
	relation->rd_myxactonly = TRUE;
	newlyCreatedRelns = lcons(relation, newlyCreatedRelns);

	MemoryContextSwitchTo(oldcxt);
}

/*
 * RelationPurgeLocalRelation -
 *	  find all the Relation descriptors marked rd_myxactonly and reset them.
 *	  This should be called at the end of a transaction (commit/abort) when
 *	  the "local" relations will become visible to others and the multi-user
 *	  buffer pool should be used.
 */
void
RelationPurgeLocalRelation(bool xactCommitted)
{
	MemoryContext oldcxt;

	if (newlyCreatedRelns == NULL)
		return;

	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	while (newlyCreatedRelns)
	{
		List	   *l = newlyCreatedRelns;
		Relation	reln = lfirst(l);

		Assert(reln != NULL && reln->rd_myxactonly);

		reln->rd_myxactonly = false;	/* mark it not on list anymore */

		newlyCreatedRelns = lnext(newlyCreatedRelns);
		pfree(l);

		if (!xactCommitted)
		{

			/*
			 * remove the file if we abort. This is so that files for
			 * tables created inside a transaction block get removed.
			 */
			if (reln->rd_isnoname)
			{
				if (!(reln->rd_unlinked))
				{
					smgrunlink(DEFAULT_SMGR, reln);
					reln->rd_unlinked = TRUE;
				}
			}
			else
				smgrunlink(DEFAULT_SMGR, reln);
		}

		if (!IsBootstrapProcessingMode())
			RelationClearRelation(reln, false);
	}

	MemoryContextSwitchTo(oldcxt);
}

/* --------------------------------
 *		RelationInitialize
 *
 *		This initializes the relation descriptor cache.
 * --------------------------------
 */

#define INITRELCACHESIZE		400

void
RelationInitialize(void)
{
	MemoryContext oldcxt;
	HASHCTL		ctl;

	/* ----------------
	 *	switch to cache memory context
	 * ----------------
	 */
	if (!CacheCxt)
		CacheCxt = CreateGlobalMemory("Cache");

	oldcxt = MemoryContextSwitchTo((MemoryContext) CacheCxt);

	/* ----------------
	 *	create global caches
	 * ----------------
	 */
	MemSet(&ctl, 0, (int) sizeof(ctl));
	ctl.keysize = sizeof(NameData);
	ctl.datasize = sizeof(Relation);
	RelationNameCache = hash_create(INITRELCACHESIZE, &ctl, HASH_ELEM);

	ctl.keysize = sizeof(Oid);
	ctl.hash = tag_hash;
	RelationIdCache = hash_create(INITRELCACHESIZE, &ctl,
								  HASH_ELEM | HASH_FUNCTION);

	/* ----------------
	 *	initialize the cache with pre-made relation descriptors
	 *	for some of the more important system relations.  These
	 *	relations should always be in the cache.
	 * ----------------
	 */
	formrdesc(RelationRelationName, Natts_pg_class, Desc_pg_class);
	formrdesc(AttributeRelationName, Natts_pg_attribute, Desc_pg_attribute);
	formrdesc(ProcedureRelationName, Natts_pg_proc, Desc_pg_proc);
	formrdesc(TypeRelationName, Natts_pg_type, Desc_pg_type);
	formrdesc(VariableRelationName, Natts_pg_variable, Desc_pg_variable);
	formrdesc(LogRelationName, Natts_pg_log, Desc_pg_log);

	/*
	 * If this isn't initdb time, then we want to initialize some index
	 * relation descriptors, as well.  The descriptors are for
	 * pg_attnumind (to make building relation descriptors fast) and
	 * possibly others, as they're added.
	 */

	if (!IsBootstrapProcessingMode())
		init_irels();

	MemoryContextSwitchTo(oldcxt);
}

static void
AttrDefaultFetch(Relation relation)
{
	AttrDefault *attrdef = relation->rd_att->constr->defval;
	int			ndef = relation->rd_att->constr->num_defval;
	Relation	adrel;
	Relation	irel = (Relation) NULL;
	ScanKeyData skey;
	HeapTupleData tuple;
	HeapTuple	htup;
	Form_pg_attrdef adform;
	IndexScanDesc sd = (IndexScanDesc) NULL;
	HeapScanDesc adscan = (HeapScanDesc) NULL;
	RetrieveIndexResult indexRes;
	struct varlena *val;
	bool		isnull;
	int			found;
	int			i;
	bool		hasindex;

	ScanKeyEntryInitialize(&skey,
						   (bits16) 0x0,
						   (AttrNumber) 1,
						   (RegProcedure) F_OIDEQ,
						   ObjectIdGetDatum(RelationGetRelid(relation)));

	adrel = heap_openr(AttrDefaultRelationName, AccessShareLock);
	hasindex = (adrel->rd_rel->relhasindex && !IsIgnoringSystemIndexes());
	if (hasindex)
	{
		irel = index_openr(AttrDefaultIndex);
		sd = index_beginscan(irel, false, 1, &skey);
	}
	else
		adscan = heap_beginscan(adrel, false, SnapshotNow, 1, &skey);
	tuple.t_datamcxt = NULL;
	tuple.t_data = NULL;

	for (found = 0;;)
	{
		Buffer		buffer;

		if (hasindex)
		{
			indexRes = index_getnext(sd, ForwardScanDirection);
			if (!indexRes)
				break;

			tuple.t_self = indexRes->heap_iptr;
			heap_fetch(adrel, SnapshotNow, &tuple, &buffer);
			pfree(indexRes);
			if (tuple.t_data == NULL)
				continue;
			htup = &tuple;
		}
		else
		{
			htup = heap_getnext(adscan, 0);
			if (!HeapTupleIsValid(htup))
				break;
		}
		found++;
		adform = (Form_pg_attrdef) GETSTRUCT(htup);
		for (i = 0; i < ndef; i++)
		{
			if (adform->adnum != attrdef[i].adnum)
				continue;
			if (attrdef[i].adbin != NULL)
				elog(NOTICE, "AttrDefaultFetch: second record found for attr %s in rel %s",
					 NameStr(relation->rd_att->attrs[adform->adnum - 1]->attname),
					 RelationGetRelationName(relation));

			val = (struct varlena *) fastgetattr(htup,
												 Anum_pg_attrdef_adbin,
												 adrel->rd_att, &isnull);
			if (isnull)
				elog(NOTICE, "AttrDefaultFetch: adbin IS NULL for attr %s in rel %s",
					 NameStr(relation->rd_att->attrs[adform->adnum - 1]->attname),
					 RelationGetRelationName(relation));
			attrdef[i].adbin = textout(val);
			break;
		}
		if (hasindex)
			ReleaseBuffer(buffer);

		if (i >= ndef)
			elog(NOTICE, "AttrDefaultFetch: unexpected record found for attr %d in rel %s",
				 adform->adnum,
				 RelationGetRelationName(relation));
	}

	if (found < ndef)
		elog(NOTICE, "AttrDefaultFetch: %d record not found for rel %s",
			 ndef - found, RelationGetRelationName(relation));

	if (hasindex)
	{
		index_endscan(sd);
		index_close(irel);
	}
	else
		heap_endscan(adscan);
	heap_close(adrel, AccessShareLock);
}

static void
RelCheckFetch(Relation relation)
{
	ConstrCheck *check = relation->rd_att->constr->check;
	int			ncheck = relation->rd_att->constr->num_check;
	Relation	rcrel;
	Relation	irel = (Relation) NULL;
	ScanKeyData skey;
	HeapTupleData tuple;
	HeapTuple	htup;
	IndexScanDesc sd = (IndexScanDesc) NULL;
	HeapScanDesc rcscan = (HeapScanDesc) NULL;
	RetrieveIndexResult indexRes;
	Name		rcname;
	struct varlena *val;
	bool		isnull;
	int			found;
	bool		hasindex;

	ScanKeyEntryInitialize(&skey,
						   (bits16) 0x0,
						   (AttrNumber) 1,
						   (RegProcedure) F_OIDEQ,
						   ObjectIdGetDatum(RelationGetRelid(relation)));

	rcrel = heap_openr(RelCheckRelationName, AccessShareLock);
	hasindex = (rcrel->rd_rel->relhasindex && !IsIgnoringSystemIndexes());
	if (hasindex)
	{
		irel = index_openr(RelCheckIndex);
		sd = index_beginscan(irel, false, 1, &skey);
	}
	else
		rcscan = heap_beginscan(rcrel, false, SnapshotNow, 1, &skey);
	tuple.t_datamcxt = NULL;
	tuple.t_data = NULL;

	for (found = 0;;)
	{
		Buffer		buffer;

		if (hasindex)
		{
			indexRes = index_getnext(sd, ForwardScanDirection);
			if (!indexRes)
				break;

			tuple.t_self = indexRes->heap_iptr;
			heap_fetch(rcrel, SnapshotNow, &tuple, &buffer);
			pfree(indexRes);
			if (tuple.t_data == NULL)
				continue;
			htup = &tuple;
		}
		else
		{
			htup = heap_getnext(rcscan, 0);
			if (!HeapTupleIsValid(htup))
				break;
		}
		if (found == ncheck)
			elog(ERROR, "RelCheckFetch: unexpected record found for rel %s",
				 RelationGetRelationName(relation));

		rcname = (Name) fastgetattr(htup,
									Anum_pg_relcheck_rcname,
									rcrel->rd_att, &isnull);
		if (isnull)
			elog(ERROR, "RelCheckFetch: rcname IS NULL for rel %s",
				 RelationGetRelationName(relation));
		check[found].ccname = pstrdup(NameStr(*rcname));
		val = (struct varlena *) fastgetattr(htup,
											 Anum_pg_relcheck_rcbin,
											 rcrel->rd_att, &isnull);
		if (isnull)
			elog(ERROR, "RelCheckFetch: rcbin IS NULL for rel %s",
				 RelationGetRelationName(relation));
		check[found].ccbin = textout(val);
		found++;
		if (hasindex)
			ReleaseBuffer(buffer);
	}

	if (found < ncheck)
		elog(ERROR, "RelCheckFetch: %d record not found for rel %s",
			 ncheck - found, RelationGetRelationName(relation));

	if (hasindex)
	{
		index_endscan(sd);
		index_close(irel);
	}
	else
		heap_endscan(rcscan);
	heap_close(rcrel, AccessShareLock);
}

/*
 *	init_irels(), write_irels() -- handle special-case initialization of
 *								   index relation descriptors.
 *
 *		In late 1992, we started regularly having databases with more than
 *		a thousand classes in them.  With this number of classes, it became
 *		critical to do indexed lookups on the system catalogs.
 *
 *		Bootstrapping these lookups is very hard.  We want to be able to
 *		use an index on pg_attribute, for example, but in order to do so,
 *		we must have read pg_attribute for the attributes in the index,
 *		which implies that we need to use the index.
 *
 *		In order to get around the problem, we do the following:
 *
 *		   +  When the database system is initialized (at initdb time), we
 *			  don't use indices on pg_attribute.  We do sequential scans.
 *
 *		   +  When the backend is started up in normal mode, we load an image
 *			  of the appropriate relation descriptors, in internal format,
 *			  from an initialization file in the data/base/... directory.
 *
 *		   +  If the initialization file isn't there, then we create the
 *			  relation descriptors using sequential scans and write 'em to
 *			  the initialization file for use by subsequent backends.
 *
 *		We could dispense with the initialization file and just build the
 *		critical reldescs the hard way on every backend startup, but that
 *		slows down backend startup noticeably if pg_class is large.
 *
 *		As of v6.5, vacuum.c deletes the initialization file at completion
 *		of a VACUUM, so that it will be rebuilt at the next backend startup.
 *		This ensures that vacuum-collected stats for the system indexes
 *		will eventually get used by the optimizer --- otherwise the relcache
 *		entries for these indexes will show zero sizes forever, since the
 *		relcache entries are pinned in memory and will never be reloaded
 *		from pg_class.
 */

/* pg_attnumind, pg_classnameind, pg_classoidind */
#define Num_indices_bootstrap	3

static void
init_irels(void)
{
	Size		len;
	int			nread;
	File		fd;
	Relation	irel[Num_indices_bootstrap];
	Relation	ird;
	Form_pg_am	am;
	Form_pg_class relform;
	IndexStrategy strat;
	RegProcedure *support;
	int			i;
	int			relno;

#ifndef __CYGWIN32__
	if ((fd = FileNameOpenFile(RELCACHE_INIT_FILENAME, O_RDONLY, 0600)) < 0)
#else
	if ((fd = FileNameOpenFile(RELCACHE_INIT_FILENAME, O_RDONLY | O_BINARY, 0600)) < 0)
#endif
	{
		write_irels();
		return;
	}

	FileSeek(fd, 0L, SEEK_SET);

	for (relno = 0; relno < Num_indices_bootstrap; relno++)
	{
		/* first read the relation descriptor length */
		if ((nread = FileRead(fd, (char *) &len, sizeof(len))) != sizeof(len))
		{
			write_irels();
			return;
		}

		ird = irel[relno] = (Relation) palloc(len);
		MemSet(ird, 0, len);

		/* then, read the Relation structure */
		if ((nread = FileRead(fd, (char *) ird, len)) != len)
		{
			write_irels();
			return;
		}

		/* the file descriptor is not yet opened */
		ird->rd_fd = -1;

		/* next, read the access method tuple form */
		if ((nread = FileRead(fd, (char *) &len, sizeof(len))) != sizeof(len))
		{
			write_irels();
			return;
		}

		am = (Form_pg_am) palloc(len);
		if ((nread = FileRead(fd, (char *) am, len)) != len)
		{
			write_irels();
			return;
		}

		ird->rd_am = am;

		/* next read the relation tuple form */
		if ((nread = FileRead(fd, (char *) &len, sizeof(len))) != sizeof(len))
		{
			write_irels();
			return;
		}

		relform = (Form_pg_class) palloc(len);
		if ((nread = FileRead(fd, (char *) relform, len)) != len)
		{
			write_irels();
			return;
		}

		ird->rd_rel = relform;

		/* initialize attribute tuple forms */
		ird->rd_att = CreateTemplateTupleDesc(relform->relnatts);

		/* next read all the attribute tuple form data entries */
		len = ATTRIBUTE_TUPLE_SIZE;
		for (i = 0; i < relform->relnatts; i++)
		{
			if ((nread = FileRead(fd, (char *) &len, sizeof(len))) != sizeof(len))
			{
				write_irels();
				return;
			}

			ird->rd_att->attrs[i] = (Form_pg_attribute) palloc(len);

			if ((nread = FileRead(fd, (char *) ird->rd_att->attrs[i], len)) != len)
			{
				write_irels();
				return;
			}
		}

		/* next, read the index strategy map */
		if ((nread = FileRead(fd, (char *) &len, sizeof(len))) != sizeof(len))
		{
			write_irels();
			return;
		}

		strat = (IndexStrategy) palloc(len);
		if ((nread = FileRead(fd, (char *) strat, len)) != len)
		{
			write_irels();
			return;
		}

		/* oh, for god's sake... */
#define SMD(i)	strat->strategyMapData[i].entry[0]

		/* have to reinit the function pointers in the strategy maps */
		for (i = 0; i < am->amstrategies * relform->relnatts; i++)
		{
			fmgr_info(SMD(i).sk_procedure,
					  &(SMD(i).sk_func));
			SMD(i).sk_nargs = SMD(i).sk_func.fn_nargs;
		}


		/*
		 * use a real field called rd_istrat instead of the bogosity of
		 * hanging invisible fields off the end of a structure - jolly
		 */
		ird->rd_istrat = strat;

		/* finally, read the vector of support procedures */
		if ((nread = FileRead(fd, (char *) &len, sizeof(len))) != sizeof(len))
		{
			write_irels();
			return;
		}

		support = (RegProcedure *) palloc(len);
		if ((nread = FileRead(fd, (char *) support, len)) != len)
		{
			write_irels();
			return;
		}
		ird->rd_support = support;

		RelationInitLockInfo(ird);

		RelationCacheInsert(ird);
	}
	criticalRelcacheBuild = true;
}

static void
write_irels(void)
{
	Size		len;
	int			nwritten;
	File		fd;
	Relation	irel[Num_indices_bootstrap];
	Relation	ird;
	Form_pg_am	am;
	Form_pg_class relform;
	IndexStrategy strat;
	RegProcedure *support;
	int			i;
	int			relno;
	RelationBuildDescInfo bi;
	char		tempfilename[MAXPGPATH];
	char		finalfilename[MAXPGPATH];

	/*
	 * We must write a temporary file and rename it into place. Otherwise,
	 * another backend starting at about the same time might crash trying
	 * to read the partially-complete file.
	 */
	snprintf(tempfilename, sizeof(tempfilename), "%s%c%s.%d",
			 DatabasePath, SEP_CHAR, RELCACHE_INIT_FILENAME, MyProcPid);
	snprintf(finalfilename, sizeof(finalfilename), "%s%c%s",
			 DatabasePath, SEP_CHAR, RELCACHE_INIT_FILENAME);

#ifndef __CYGWIN32__
	fd = PathNameOpenFile(tempfilename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
#else
	fd = PathNameOpenFile(tempfilename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600);
#endif
	if (fd < 0)
		elog(FATAL, "cannot create init file %s", tempfilename);

	FileSeek(fd, 0L, SEEK_SET);

	/*
	 * Build relation descriptors for the critical system indexes without
	 * resort to the descriptor cache.	In order to do this, we set
	 * ProcessingMode to Bootstrap.  The effect of this is to disable
	 * indexed relation searches -- a necessary step, since we're trying
	 * to instantiate the index relation descriptors here.	Once we have
	 * the descriptors, nail them into cache so we never lose them.
	 */

	/*
	 * Removed the following ProcessingMode change -- inoue At this point
	 * 1) Catalog Cache isn't initialized 2) Relation Cache for the
	 * following critical indexes aren't built oldmode =
	 * GetProcessingMode(); SetProcessingMode(BootstrapProcessing);
	 */

	bi.infotype = INFO_RELNAME;
	bi.i.info_name = AttributeRelidNumIndex;
	irel[0] = RelationBuildDesc(bi, NULL);
	irel[0]->rd_isnailed = true;

	bi.i.info_name = ClassNameIndex;
	irel[1] = RelationBuildDesc(bi, NULL);
	irel[1]->rd_isnailed = true;

	bi.i.info_name = ClassOidIndex;
	irel[2] = RelationBuildDesc(bi, NULL);
	irel[2]->rd_isnailed = true;

	criticalRelcacheBuild = true;

	/*
	 * Removed the following ProcessingMode -- inoue
	 * SetProcessingMode(oldmode);
	 */

	/*
	 * Write out the index reldescs to the special cache file.
	 */
	for (relno = 0; relno < Num_indices_bootstrap; relno++)
	{
		ird = irel[relno];

		/* save the volatile fields in the relation descriptor */
		am = ird->rd_am;
		ird->rd_am = (Form_pg_am) NULL;
		relform = ird->rd_rel;
		ird->rd_rel = (Form_pg_class) NULL;
		strat = ird->rd_istrat;
		support = ird->rd_support;

		/*
		 * first write the relation descriptor , excluding strategy and
		 * support
		 */
		len = sizeof(RelationData);

		/* first, write the relation descriptor length */
		if ((nwritten = FileWrite(fd, (char *) &len, sizeof(len)))
			!= sizeof(len))
			elog(FATAL, "cannot write init file -- descriptor length");

		/* next, write out the Relation structure */
		if ((nwritten = FileWrite(fd, (char *) ird, len)) != len)
			elog(FATAL, "cannot write init file -- reldesc");

		/* next, write the access method tuple form */
		len = sizeof(FormData_pg_am);
		if ((nwritten = FileWrite(fd, (char *) &len, sizeof(len)))
			!= sizeof(len))
			elog(FATAL, "cannot write init file -- am tuple form length");

		if ((nwritten = FileWrite(fd, (char *) am, len)) != len)
			elog(FATAL, "cannot write init file -- am tuple form");

		/* next write the relation tuple form */
		len = sizeof(FormData_pg_class);
		if ((nwritten = FileWrite(fd, (char *) &len, sizeof(len)))
			!= sizeof(len))
			elog(FATAL, "cannot write init file -- relation tuple form length");

		if ((nwritten = FileWrite(fd, (char *) relform, len)) != len)
			elog(FATAL, "cannot write init file -- relation tuple form");

		/* next, do all the attribute tuple form data entries */
		len = ATTRIBUTE_TUPLE_SIZE;
		for (i = 0; i < relform->relnatts; i++)
		{
			if ((nwritten = FileWrite(fd, (char *) &len, sizeof(len)))
				!= sizeof(len))
				elog(FATAL, "cannot write init file -- length of attdesc %d", i);
			if ((nwritten = FileWrite(fd, (char *) ird->rd_att->attrs[i], len))
				!= len)
				elog(FATAL, "cannot write init file -- attdesc %d", i);
		}

		/* next, write the index strategy map */
		len = AttributeNumberGetIndexStrategySize(relform->relnatts,
												  am->amstrategies);
		if ((nwritten = FileWrite(fd, (char *) &len, sizeof(len)))
			!= sizeof(len))
			elog(FATAL, "cannot write init file -- strategy map length");

		if ((nwritten = FileWrite(fd, (char *) strat, len)) != len)
			elog(FATAL, "cannot write init file -- strategy map");

		/* finally, write the vector of support procedures */
		len = relform->relnatts * (am->amsupport * sizeof(RegProcedure));
		if ((nwritten = FileWrite(fd, (char *) &len, sizeof(len)))
			!= sizeof(len))
			elog(FATAL, "cannot write init file -- support vector length");

		if ((nwritten = FileWrite(fd, (char *) support, len)) != len)
			elog(FATAL, "cannot write init file -- support vector");

		/* restore volatile fields */
		ird->rd_am = am;
		ird->rd_rel = relform;
	}

	FileClose(fd);

	/*
	 * And rename the temp file to its final name, deleting any
	 * previously- existing init file.
	 */
	rename(tempfilename, finalfilename);
}
