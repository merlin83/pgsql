/*-------------------------------------------------------------------------
 *
 * index.c
 *	  code to create and destroy POSTGRES index relations
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/index.c,v 1.209 2003-02-19 04:06:28 momjian Exp $
 *
 *
 * INTERFACE ROUTINES
 *		index_create()			- Create a cataloged index relation
 *		index_drop()			- Removes index relation from catalogs
 *		BuildIndexInfo()		- Prepare to insert index tuples
 *		FormIndexDatum()		- Construct datum vector for one index tuple
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <unistd.h>

#include "access/genam.h"
#include "access/heapam.h"
#include "access/istrat.h"
#include "bootstrap/bootstrap.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/dependency.h"
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/indexing.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_index.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "executor/executor.h"
#include "miscadmin.h"
#include "optimizer/clauses.h"
#include "optimizer/prep.h"
#include "parser/parse_func.h"
#include "storage/sinval.h"
#include "storage/smgr.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/fmgroids.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/relcache.h"
#include "utils/syscache.h"


/*
 * macros used in guessing how many tuples are on a page.
 */
#define AVG_ATTR_SIZE 8
#define NTUPLES_PER_PAGE(natts) \
	((BLCKSZ - MAXALIGN(sizeof(PageHeaderData))) / \
	((natts) * AVG_ATTR_SIZE + MAXALIGN(sizeof(HeapTupleHeaderData))))

/* non-export function prototypes */
static TupleDesc BuildFuncTupleDesc(Oid funcOid,
				   Oid *classObjectId);
static TupleDesc ConstructTupleDescriptor(Relation heapRelation,
						 int numatts, AttrNumber *attNums,
						 Oid *classObjectId);
static void UpdateRelationRelation(Relation indexRelation);
static void InitializeAttributeOids(Relation indexRelation,
						int numatts, Oid indexoid);
static void AppendAttributeTuples(Relation indexRelation, int numatts);
static void UpdateIndexRelation(Oid indexoid, Oid heapoid,
					IndexInfo *indexInfo,
					Oid *classOids,
					bool primary);
static Oid	IndexGetRelation(Oid indexId);
static bool activate_index(Oid indexId, bool activate, bool inplace);


static bool reindexing = false;


bool
SetReindexProcessing(bool reindexmode)
{
	bool		old = reindexing;

	reindexing = reindexmode;
	return old;
}

bool
IsReindexProcessing(void)
{
	return reindexing;
}

static TupleDesc
BuildFuncTupleDesc(Oid funcOid,
				   Oid *classObjectId)
{
	TupleDesc	funcTupDesc;
	HeapTuple	tuple;
	Oid			keyType;
	Oid			retType;
	Form_pg_type typeTup;

	/*
	 * Allocate and zero a tuple descriptor for a one-column tuple.
	 */
	funcTupDesc = CreateTemplateTupleDesc(1, false);
	funcTupDesc->attrs[0] = (Form_pg_attribute) palloc0(ATTRIBUTE_TUPLE_SIZE);

	/*
	 * Lookup the function to get its name and return type.
	 */
	tuple = SearchSysCache(PROCOID,
						   ObjectIdGetDatum(funcOid),
						   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "Function %u does not exist", funcOid);
	retType = ((Form_pg_proc) GETSTRUCT(tuple))->prorettype;

	/*
	 * make the attributes name the same as the functions
	 */
	namestrcpy(&funcTupDesc->attrs[0]->attname,
			   NameStr(((Form_pg_proc) GETSTRUCT(tuple))->proname));

	ReleaseSysCache(tuple);

	/*
	 * Check the opclass to see if it provides a keytype (overriding the
	 * function result type).
	 */
	tuple = SearchSysCache(CLAOID,
						   ObjectIdGetDatum(classObjectId[0]),
						   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "Opclass %u does not exist", classObjectId[0]);
	keyType = ((Form_pg_opclass) GETSTRUCT(tuple))->opckeytype;
	ReleaseSysCache(tuple);

	if (!OidIsValid(keyType))
		keyType = retType;

	/*
	 * Lookup the key type in pg_type for the type length etc.
	 */
	tuple = SearchSysCache(TYPEOID,
						   ObjectIdGetDatum(keyType),
						   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "Type %u does not exist", keyType);
	typeTup = (Form_pg_type) GETSTRUCT(tuple);

	/*
	 * Assign some of the attributes values. Leave the rest as 0.
	 */
	funcTupDesc->attrs[0]->attnum = 1;
	funcTupDesc->attrs[0]->atttypid = keyType;
	funcTupDesc->attrs[0]->attlen = typeTup->typlen;
	funcTupDesc->attrs[0]->attbyval = typeTup->typbyval;
	funcTupDesc->attrs[0]->attstorage = typeTup->typstorage;
	funcTupDesc->attrs[0]->attalign = typeTup->typalign;
	funcTupDesc->attrs[0]->attcacheoff = -1;
	funcTupDesc->attrs[0]->atttypmod = -1;
	funcTupDesc->attrs[0]->attislocal = true;

	ReleaseSysCache(tuple);

	return funcTupDesc;
}

/* ----------------------------------------------------------------
 *		ConstructTupleDescriptor
 *
 * Build an index tuple descriptor for a new index (plain not functional)
 * ----------------------------------------------------------------
 */
static TupleDesc
ConstructTupleDescriptor(Relation heapRelation,
						 int numatts,
						 AttrNumber *attNums,
						 Oid *classObjectId)
{
	TupleDesc	heapTupDesc;
	TupleDesc	indexTupDesc;
	int			natts;			/* #atts in heap rel --- for error checks */
	int			i;

	heapTupDesc = RelationGetDescr(heapRelation);
	natts = RelationGetForm(heapRelation)->relnatts;

	/*
	 * allocate the new tuple descriptor
	 */

	indexTupDesc = CreateTemplateTupleDesc(numatts, false);

	/* ----------------
	 *	  for each attribute we are indexing, obtain its attribute
	 *	  tuple form from either the static table of system attribute
	 *	  tuple forms or the relation tuple descriptor
	 * ----------------
	 */
	for (i = 0; i < numatts; i++)
	{
		AttrNumber	atnum;		/* attributeNumber[attributeOffset] */
		Form_pg_attribute from;
		Form_pg_attribute to;
		HeapTuple	tuple;
		Oid			keyType;

		/*
		 * get the attribute number and make sure it's valid; determine
		 * which attribute descriptor to copy
		 */
		atnum = attNums[i];

		if (!AttrNumberIsForUserDefinedAttr(atnum))
		{
			/*
			 * here we are indexing on a system attribute (-1...-n)
			 */
			from = SystemAttributeDefinition(atnum,
									   heapRelation->rd_rel->relhasoids);
		}
		else
		{
			/*
			 * here we are indexing on a normal attribute (1...n)
			 */
			if (atnum > natts)
				elog(ERROR, "cannot create index: column %d does not exist",
					 atnum);

			from = heapTupDesc->attrs[AttrNumberGetAttrOffset(atnum)];
		}

		/*
		 * now that we've determined the "from", let's copy the tuple desc
		 * data...
		 */
		indexTupDesc->attrs[i] = to =
			(Form_pg_attribute) palloc(ATTRIBUTE_TUPLE_SIZE);
		memcpy(to, from, ATTRIBUTE_TUPLE_SIZE);

		/*
		 * Fix the stuff that should not be the same as the underlying
		 * attr
		 */
		to->attnum = i + 1;

		to->attstattarget = 0;
		to->attcacheoff = -1;
		to->attnotnull = false;
		to->atthasdef = false;
		to->attislocal = true;
		to->attinhcount = 0;

		/*
		 * We do not yet have the correct relation OID for the index, so
		 * just set it invalid for now.  InitializeAttributeOids() will
		 * fix it later.
		 */
		to->attrelid = InvalidOid;

		/*
		 * Check the opclass to see if it provides a keytype (overriding
		 * the attribute type).
		 */
		tuple = SearchSysCache(CLAOID,
							   ObjectIdGetDatum(classObjectId[i]),
							   0, 0, 0);
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "Opclass %u does not exist", classObjectId[i]);
		keyType = ((Form_pg_opclass) GETSTRUCT(tuple))->opckeytype;
		ReleaseSysCache(tuple);

		if (OidIsValid(keyType) && keyType != to->atttypid)
		{
			/* index value and heap value have different types */
			Form_pg_type typeTup;

			tuple = SearchSysCache(TYPEOID,
								   ObjectIdGetDatum(keyType),
								   0, 0, 0);
			if (!HeapTupleIsValid(tuple))
				elog(ERROR, "Type %u does not exist", keyType);
			typeTup = (Form_pg_type) GETSTRUCT(tuple);

			to->atttypid = keyType;
			to->atttypmod = -1;
			to->attlen = typeTup->typlen;
			to->attbyval = typeTup->typbyval;
			to->attalign = typeTup->typalign;
			to->attstorage = typeTup->typstorage;

			ReleaseSysCache(tuple);
		}
	}

	return indexTupDesc;
}

/* ----------------------------------------------------------------
 *		UpdateRelationRelation
 * ----------------------------------------------------------------
 */
static void
UpdateRelationRelation(Relation indexRelation)
{
	Relation	pg_class;
	HeapTuple	tuple;

	pg_class = heap_openr(RelationRelationName, RowExclusiveLock);

	/* XXX Natts_pg_class_fixed is a hack - see pg_class.h */
	tuple = heap_addheader(Natts_pg_class_fixed,
						   true,
						   CLASS_TUPLE_SIZE,
						   (void *) indexRelation->rd_rel);

	/*
	 * the new tuple must have the oid already chosen for the index. sure
	 * would be embarrassing to do this sort of thing in polite company.
	 */
	HeapTupleSetOid(tuple, RelationGetRelid(indexRelation));
	simple_heap_insert(pg_class, tuple);

	/* update the system catalog indexes */
	CatalogUpdateIndexes(pg_class, tuple);

	heap_freetuple(tuple);
	heap_close(pg_class, RowExclusiveLock);
}

/* ----------------------------------------------------------------
 *		InitializeAttributeOids
 * ----------------------------------------------------------------
 */
static void
InitializeAttributeOids(Relation indexRelation,
						int numatts,
						Oid indexoid)
{
	TupleDesc	tupleDescriptor;
	int			i;

	tupleDescriptor = RelationGetDescr(indexRelation);

	for (i = 0; i < numatts; i += 1)
		tupleDescriptor->attrs[i]->attrelid = indexoid;
}

/* ----------------------------------------------------------------
 *		AppendAttributeTuples
 * ----------------------------------------------------------------
 */
static void
AppendAttributeTuples(Relation indexRelation, int numatts)
{
	Relation	pg_attribute;
	CatalogIndexState indstate;
	TupleDesc	indexTupDesc;
	HeapTuple	new_tuple;
	int			i;

	/*
	 * open the attribute relation and its indexes
	 */
	pg_attribute = heap_openr(AttributeRelationName, RowExclusiveLock);

	indstate = CatalogOpenIndexes(pg_attribute);

	/*
	 * insert data from new index's tupdesc into pg_attribute
	 */
	indexTupDesc = RelationGetDescr(indexRelation);

	for (i = 0; i < numatts; i++)
	{
		/*
		 * There used to be very grotty code here to set these fields, but
		 * I think it's unnecessary.  They should be set already.
		 */
		Assert(indexTupDesc->attrs[i]->attnum == i + 1);
		Assert(indexTupDesc->attrs[i]->attcacheoff == -1);

		new_tuple = heap_addheader(Natts_pg_attribute,
								   false,
								   ATTRIBUTE_TUPLE_SIZE,
								   (void *) indexTupDesc->attrs[i]);

		simple_heap_insert(pg_attribute, new_tuple);

		CatalogIndexInsert(indstate, new_tuple);

		heap_freetuple(new_tuple);
	}

	CatalogCloseIndexes(indstate);

	heap_close(pg_attribute, RowExclusiveLock);
}

/* ----------------------------------------------------------------
 *		UpdateIndexRelation
 * ----------------------------------------------------------------
 */
static void
UpdateIndexRelation(Oid indexoid,
					Oid heapoid,
					IndexInfo *indexInfo,
					Oid *classOids,
					bool primary)
{
	int16		indkey[INDEX_MAX_KEYS];
	Oid			indclass[INDEX_MAX_KEYS];
	Datum		predDatum;
	Datum		values[Natts_pg_index];
	char		nulls[Natts_pg_index];
	Relation	pg_index;
	HeapTuple	tuple;
	int			i;

	/*
	 * Copy the index key and opclass info into zero-filled vectors
	 *
	 * (zero filling is essential 'cause we don't store the number of
	 * index columns explicitly in pg_index, which is pretty grotty...)
	 */
	MemSet(indkey, 0, sizeof(indkey));
	for (i = 0; i < indexInfo->ii_NumKeyAttrs; i++)
		indkey[i] = indexInfo->ii_KeyAttrNumbers[i];

	MemSet(indclass, 0, sizeof(indclass));
	for (i = 0; i < indexInfo->ii_NumIndexAttrs; i++)
		indclass[i] = classOids[i];

	/*
	 * Convert the index-predicate (if any) to a text datum
	 */
	if (indexInfo->ii_Predicate != NIL)
	{
		char	   *predString;

		predString = nodeToString(indexInfo->ii_Predicate);
		predDatum = DirectFunctionCall1(textin,
										CStringGetDatum(predString));
		pfree(predString);
	}
	else
		predDatum = DirectFunctionCall1(textin,
										CStringGetDatum(""));

	/*
	 * open the system catalog index relation
	 */
	pg_index = heap_openr(IndexRelationName, RowExclusiveLock);

	/*
	 * Build a pg_index tuple
	 */
	MemSet(nulls, ' ', sizeof(nulls));

	values[Anum_pg_index_indexrelid - 1] = ObjectIdGetDatum(indexoid);
	values[Anum_pg_index_indrelid - 1] = ObjectIdGetDatum(heapoid);
	values[Anum_pg_index_indproc - 1] = ObjectIdGetDatum(indexInfo->ii_FuncOid);
	values[Anum_pg_index_indkey - 1] = PointerGetDatum(indkey);
	values[Anum_pg_index_indclass - 1] = PointerGetDatum(indclass);
	values[Anum_pg_index_indisclustered - 1] = BoolGetDatum(false);
	values[Anum_pg_index_indisunique - 1] = BoolGetDatum(indexInfo->ii_Unique);
	values[Anum_pg_index_indisprimary - 1] = BoolGetDatum(primary);
	values[Anum_pg_index_indreference - 1] = ObjectIdGetDatum(InvalidOid);
	values[Anum_pg_index_indpred - 1] = predDatum;

	tuple = heap_formtuple(RelationGetDescr(pg_index), values, nulls);

	/*
	 * insert the tuple into the pg_index catalog
	 */
	simple_heap_insert(pg_index, tuple);

	/* update the indexes on pg_index */
	CatalogUpdateIndexes(pg_index, tuple);

	/*
	 * close the relation and free the tuple
	 */
	heap_close(pg_index, RowExclusiveLock);
	heap_freetuple(tuple);
}


/* ----------------------------------------------------------------
 *		index_create
 *
 * Returns OID of the created index.
 * ----------------------------------------------------------------
 */
Oid
index_create(Oid heapRelationId,
			 const char *indexRelationName,
			 IndexInfo *indexInfo,
			 Oid accessMethodObjectId,
			 Oid *classObjectId,
			 bool primary,
			 bool isconstraint,
			 bool allow_system_table_mods)
{
	Relation	heapRelation;
	Relation	indexRelation;
	TupleDesc	indexTupDesc;
	bool		shared_relation;
	Oid			namespaceId;
	Oid			indexoid;
	int			i;

	SetReindexProcessing(false);

	/*
	 * Only SELECT ... FOR UPDATE are allowed while doing this
	 */
	heapRelation = heap_open(heapRelationId, ShareLock);

	/*
	 * The index will be in the same namespace as its parent table, and is
	 * shared across databases if and only if the parent is.
	 */
	namespaceId = RelationGetNamespace(heapRelation);
	shared_relation = heapRelation->rd_rel->relisshared;

	/*
	 * check parameters
	 */
	if (indexInfo->ii_NumIndexAttrs < 1 ||
		indexInfo->ii_NumKeyAttrs < 1)
		elog(ERROR, "must index at least one column");

	if (!allow_system_table_mods &&
		IsSystemRelation(heapRelation) &&
		IsNormalProcessingMode())
		elog(ERROR, "User-defined indexes on system catalogs are not supported");

	/*
	 * We cannot allow indexing a shared relation after initdb (because
	 * there's no way to make the entry in other databases' pg_class).
	 * Unfortunately we can't distinguish initdb from a manually started
	 * standalone backend.	However, we can at least prevent this mistake
	 * under normal multi-user operation.
	 */
	if (shared_relation && IsUnderPostmaster)
		elog(ERROR, "Shared indexes cannot be created after initdb");

	if (get_relname_relid(indexRelationName, namespaceId))
		elog(ERROR, "relation named \"%s\" already exists",
			 indexRelationName);

	/*
	 * construct tuple descriptor for index tuples
	 */
	if (OidIsValid(indexInfo->ii_FuncOid))
		indexTupDesc = BuildFuncTupleDesc(indexInfo->ii_FuncOid,
										  classObjectId);
	else
		indexTupDesc = ConstructTupleDescriptor(heapRelation,
												indexInfo->ii_NumKeyAttrs,
											indexInfo->ii_KeyAttrNumbers,
												classObjectId);

	indexTupDesc->tdhasoid = false;

	/*
	 * create the index relation's relcache entry and physical disk file.
	 * (If we fail further down, it's the smgr's responsibility to remove
	 * the disk file again.)
	 */
	indexRelation = heap_create(indexRelationName,
								namespaceId,
								indexTupDesc,
								shared_relation,
								true,
								allow_system_table_mods);

	/* Fetch the relation OID assigned by heap_create */
	indexoid = RelationGetRelid(indexRelation);

	/*
	 * Obtain exclusive lock on it.  Although no other backends can see it
	 * until we commit, this prevents deadlock-risk complaints from lock
	 * manager in cases such as CLUSTER.
	 */
	LockRelation(indexRelation, AccessExclusiveLock);

	/*
	 * Fill in fields of the index's pg_class entry that are not set
	 * correctly by heap_create.
	 *
	 * XXX should have a cleaner way to create cataloged indexes
	 */
	indexRelation->rd_rel->relowner = GetUserId();
	indexRelation->rd_rel->relam = accessMethodObjectId;
	indexRelation->rd_rel->relkind = RELKIND_INDEX;
	indexRelation->rd_rel->relhasoids = false;

	/*
	 * store index's pg_class entry
	 */
	UpdateRelationRelation(indexRelation);

	/*
	 * now update the object id's of all the attribute tuple forms in the
	 * index relation's tuple descriptor
	 */
	InitializeAttributeOids(indexRelation,
							indexInfo->ii_NumIndexAttrs,
							indexoid);

	/*
	 * append ATTRIBUTE tuples for the index
	 */
	AppendAttributeTuples(indexRelation, indexInfo->ii_NumIndexAttrs);

	/* ----------------
	 *	  update pg_index
	 *	  (append INDEX tuple)
	 *
	 *	  Note that this stows away a representation of "predicate".
	 *	  (Or, could define a rule to maintain the predicate) --Nels, Feb '92
	 * ----------------
	 */
	UpdateIndexRelation(indexoid, heapRelationId, indexInfo,
						classObjectId, primary);

	/*
	 * Register constraint and dependencies for the index.
	 *
	 * If the index is from a CONSTRAINT clause, construct a pg_constraint
	 * entry.  The index is then linked to the constraint, which in turn
	 * is linked to the table.	If it's not a CONSTRAINT, make the
	 * dependency directly on the table.
	 *
	 * We don't need a dependency on the namespace, because there'll be an
	 * indirect dependency via our parent table.
	 *
	 * During bootstrap we can't register any dependencies, and we don't try
	 * to make a constraint either.
	 */
	if (!IsBootstrapProcessingMode())
	{
		ObjectAddress myself,
					referenced;

		myself.classId = RelOid_pg_class;
		myself.objectId = indexoid;
		myself.objectSubId = 0;

		if (isconstraint)
		{
			char		constraintType;
			Oid			conOid;

			if (primary)
				constraintType = CONSTRAINT_PRIMARY;
			else if (indexInfo->ii_Unique)
				constraintType = CONSTRAINT_UNIQUE;
			else
			{
				elog(ERROR, "index_create: constraint must be PRIMARY or UNIQUE");
				constraintType = 0;		/* keep compiler quiet */
			}

			conOid = CreateConstraintEntry(indexRelationName,
										   namespaceId,
										   constraintType,
										   false,		/* isDeferrable */
										   false,		/* isDeferred */
										   heapRelationId,
										   indexInfo->ii_KeyAttrNumbers,
										   indexInfo->ii_NumKeyAttrs,
										   InvalidOid,	/* no domain */
										   InvalidOid,	/* no foreign key */
										   NULL,
										   0,
										   ' ',
										   ' ',
										   ' ',
										   InvalidOid, /* no associated index */
										   NULL,		/* no check constraint */
										   NULL,
										   NULL);

			referenced.classId = get_system_catalog_relid(ConstraintRelationName);
			referenced.objectId = conOid;
			referenced.objectSubId = 0;

			recordDependencyOn(&myself, &referenced, DEPENDENCY_INTERNAL);
		}
		else
		{
			for (i = 0; i < indexInfo->ii_NumKeyAttrs; i++)
			{
				referenced.classId = RelOid_pg_class;
				referenced.objectId = heapRelationId;
				referenced.objectSubId = indexInfo->ii_KeyAttrNumbers[i];

				recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);
			}
		}

		/* Store dependency on operator classes */
		referenced.classId = get_system_catalog_relid(OperatorClassRelationName);
		for (i = 0; i < indexInfo->ii_NumIndexAttrs; i++)
		{
			referenced.objectId = classObjectId[i];
			referenced.objectSubId = 0;

			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
		}

		/* Store the dependency on the function (if appropriate) */
		if (OidIsValid(indexInfo->ii_FuncOid))
		{
			referenced.classId = RelOid_pg_proc;
			referenced.objectId = indexInfo->ii_FuncOid;
			referenced.objectSubId = 0;

			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
		}
	}

	/*
	 * Fill in the index strategy structure with information from the
	 * catalogs.  First we must advance the command counter so that we
	 * will see the newly-entered index catalog tuples.
	 */
	CommandCounterIncrement();

	RelationInitIndexAccessInfo(indexRelation);

	/*
	 * If this is bootstrap (initdb) time, then we don't actually fill in
	 * the index yet.  We'll be creating more indexes and classes later,
	 * so we delay filling them in until just before we're done with
	 * bootstrapping.  Otherwise, we call the routine that constructs the
	 * index.
	 *
	 * In normal processing mode, the heap and index relations are closed by
	 * index_build() --- but we continue to hold the ShareLock on the heap
	 * and the exclusive lock on the index that we acquired above, until
	 * end of transaction.
	 */
	if (IsBootstrapProcessingMode())
	{
		index_register(heapRelationId, indexoid, indexInfo);
		/* XXX shouldn't we close the heap and index rels here? */
	}
	else
		index_build(heapRelation, indexRelation, indexInfo);

	return indexoid;
}

/*
 *		index_drop
 *
 * NOTE: this routine should now only be called through performDeletion(),
 * else associated dependencies won't be cleaned up.
 */
void
index_drop(Oid indexId)
{
	Oid			heapId;
	Relation	userHeapRelation;
	Relation	userIndexRelation;
	Relation	indexRelation;
	HeapTuple	tuple;
	int			i;

	Assert(OidIsValid(indexId));

	/*
	 * To drop an index safely, we must grab exclusive lock on its parent
	 * table; otherwise there could be other backends using the index!
	 * Exclusive lock on the index alone is insufficient because another
	 * backend might be in the midst of devising a query plan that will
	 * use the index.  The parser and planner take care to hold an
	 * appropriate lock on the parent table while working, but having them
	 * hold locks on all the indexes too seems overly complex.	We do grab
	 * exclusive lock on the index too, just to be safe. Both locks must
	 * be held till end of transaction, else other backends will still see
	 * this index in pg_index.
	 */
	heapId = IndexGetRelation(indexId);
	userHeapRelation = heap_open(heapId, AccessExclusiveLock);

	userIndexRelation = index_open(indexId);
	LockRelation(userIndexRelation, AccessExclusiveLock);

	/*
	 * fix RELATION relation
	 */
	DeleteRelationTuple(indexId);

	/*
	 * fix ATTRIBUTE relation
	 */
	DeleteAttributeTuples(indexId);

	/*
	 * fix INDEX relation
	 */
	indexRelation = heap_openr(IndexRelationName, RowExclusiveLock);

	tuple = SearchSysCache(INDEXRELID,
						   ObjectIdGetDatum(indexId),
						   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "index_drop: cache lookup failed for index %u",
			 indexId);

	simple_heap_delete(indexRelation, &tuple->t_self);

	ReleaseSysCache(tuple);
	heap_close(indexRelation, RowExclusiveLock);

	/*
	 * flush buffer cache and physically remove the file
	 */
	i = FlushRelationBuffers(userIndexRelation, (BlockNumber) 0);
	if (i < 0)
		elog(ERROR, "index_drop: FlushRelationBuffers returned %d", i);

	smgrunlink(DEFAULT_SMGR, userIndexRelation);

	/*
	 * We are presently too lazy to attempt to compute the new correct
	 * value of relhasindex (the next VACUUM will fix it if necessary).
	 * So there is no need to update the pg_class tuple for the owning
	 * relation. But we must send out a shared-cache-inval notice on the
	 * owning relation to ensure other backends update their relcache
	 * lists of indexes.
	 */
	CacheInvalidateRelcache(heapId);

	/*
	 * Close rels, but keep locks
	 */
	index_close(userIndexRelation);
	heap_close(userHeapRelation, NoLock);

	RelationForgetRelation(indexId);
}

/* ----------------------------------------------------------------
 *						index_build support
 * ----------------------------------------------------------------
 */

/* ----------------
 *		BuildIndexInfo
 *			Construct an IndexInfo record given the index's pg_index tuple
 *
 * IndexInfo stores the information about the index that's needed by
 * FormIndexDatum, which is used for both index_build() and later insertion
 * of individual index tuples.	Normally we build an IndexInfo for an index
 * just once per command, and then use it for (potentially) many tuples.
 * ----------------
 */
IndexInfo *
BuildIndexInfo(Form_pg_index indexStruct)
{
	IndexInfo  *ii = makeNode(IndexInfo);
	int			i;
	int			numKeys;

	/*
	 * count the number of keys, and copy them into the IndexInfo
	 */
	numKeys = 0;
	for (i = 0; i < INDEX_MAX_KEYS &&
		 indexStruct->indkey[i] != InvalidAttrNumber; i++)
	{
		ii->ii_KeyAttrNumbers[i] = indexStruct->indkey[i];
		numKeys++;
	}
	ii->ii_NumKeyAttrs = numKeys;

	/*
	 * Handle functional index.
	 *
	 * If we have a functional index then the number of attributes defined in
	 * the index must be 1 (the function's single return value). Otherwise
	 * it's same as number of keys.
	 */
	ii->ii_FuncOid = indexStruct->indproc;

	if (OidIsValid(indexStruct->indproc))
	{
		ii->ii_NumIndexAttrs = 1;
		/* Do a lookup on the function, too */
		fmgr_info(indexStruct->indproc, &ii->ii_FuncInfo);
	}
	else
		ii->ii_NumIndexAttrs = numKeys;

	/*
	 * If partial index, convert predicate into expression nodetree
	 */
	if (VARSIZE(&indexStruct->indpred) > VARHDRSZ)
	{
		char	   *predString;

		predString = DatumGetCString(DirectFunctionCall1(textout,
								PointerGetDatum(&indexStruct->indpred)));
		ii->ii_Predicate = stringToNode(predString);
		ii->ii_PredicateState = NIL;
		pfree(predString);
	}
	else
	{
		ii->ii_Predicate = NIL;
		ii->ii_PredicateState = NIL;
	}

	/* Other info */
	ii->ii_Unique = indexStruct->indisunique;

	return ii;
}

/* ----------------
 *		FormIndexDatum
 *			Construct Datum[] and nullv[] arrays for a new index tuple.
 *
 *	indexInfo		Info about the index
 *	heapTuple		Heap tuple for which we must prepare an index entry
 *	heapDescriptor	tupledesc for heap tuple
 *	resultCxt		Temporary memory context for any palloc'd datums created
 *	datum			Array of index Datums (output area)
 *	nullv			Array of is-null indicators (output area)
 *
 * For largely historical reasons, we don't actually call index_formtuple()
 * here, we just prepare its input arrays datum[] and nullv[].
 * ----------------
 */
void
FormIndexDatum(IndexInfo *indexInfo,
			   HeapTuple heapTuple,
			   TupleDesc heapDescriptor,
			   MemoryContext resultCxt,
			   Datum *datum,
			   char *nullv)
{
	MemoryContext oldContext;
	int			i;
	Datum		iDatum;
	bool		isNull;

	oldContext = MemoryContextSwitchTo(resultCxt);

	if (OidIsValid(indexInfo->ii_FuncOid))
	{
		/*
		 * Functional index --- compute the single index attribute
		 */
		FunctionCallInfoData fcinfo;
		bool		anynull = false;

		MemSet(&fcinfo, 0, sizeof(fcinfo));
		fcinfo.flinfo = &indexInfo->ii_FuncInfo;
		fcinfo.nargs = indexInfo->ii_NumKeyAttrs;

		for (i = 0; i < indexInfo->ii_NumKeyAttrs; i++)
		{
			fcinfo.arg[i] = heap_getattr(heapTuple,
										 indexInfo->ii_KeyAttrNumbers[i],
										 heapDescriptor,
										 &fcinfo.argnull[i]);
			anynull |= fcinfo.argnull[i];
		}
		if (indexInfo->ii_FuncInfo.fn_strict && anynull)
		{
			/* force a null result for strict function */
			iDatum = (Datum) 0;
			isNull = true;
		}
		else
		{
			iDatum = FunctionCallInvoke(&fcinfo);
			isNull = fcinfo.isnull;
		}
		datum[0] = iDatum;
		nullv[0] = (isNull) ? 'n' : ' ';
	}
	else
	{
		/*
		 * Plain index --- for each attribute we need from the heap tuple,
		 * get the attribute and stick it into the datum and nullv arrays.
		 */
		for (i = 0; i < indexInfo->ii_NumIndexAttrs; i++)
		{
			iDatum = heap_getattr(heapTuple,
								  indexInfo->ii_KeyAttrNumbers[i],
								  heapDescriptor,
								  &isNull);
			datum[i] = iDatum;
			nullv[i] = (isNull) ? 'n' : ' ';
		}
	}

	MemoryContextSwitchTo(oldContext);
}


/* ---------------------------------------------
 *		Indexes of the relation active ?
 *
 * Caller must hold an adequate lock on the relation to ensure the
 * answer won't be changing.
 * ---------------------------------------------
 */
bool
IndexesAreActive(Relation heaprel)
{
	bool		isactive;
	Relation	indexRelation;
	HeapScanDesc scan;
	ScanKeyData entry;

	if (heaprel->rd_rel->relkind != RELKIND_RELATION &&
		heaprel->rd_rel->relkind != RELKIND_TOASTVALUE)
		elog(ERROR, "relation %s isn't an indexable relation",
			 RelationGetRelationName(heaprel));

	/* If pg_class.relhasindex is set, indexes are active */
	isactive = heaprel->rd_rel->relhasindex;
	if (isactive)
		return isactive;

	/* Otherwise, look to see if there are any indexes */
	indexRelation = heap_openr(IndexRelationName, AccessShareLock);
	ScanKeyEntryInitialize(&entry, 0,
						   Anum_pg_index_indrelid, F_OIDEQ,
						   ObjectIdGetDatum(RelationGetRelid(heaprel)));
	scan = heap_beginscan(indexRelation, SnapshotNow, 1, &entry);
	if (heap_getnext(scan, ForwardScanDirection) == NULL)
		isactive = true;		/* no indexes, so report "active" */
	heap_endscan(scan);
	heap_close(indexRelation, AccessShareLock);
	return isactive;
}

/* ----------------
 *		set relhasindex of relation's pg_class entry
 *
 * If isprimary is TRUE, we are defining a primary index, so also set
 * relhaspkey to TRUE.	Otherwise, leave relhaspkey alone.
 *
 * If reltoastidxid is not InvalidOid, also set reltoastidxid to that value.
 * This is only used for TOAST relations.
 *
 * NOTE: an important side-effect of this operation is that an SI invalidation
 * message is sent out to all backends --- including me --- causing relcache
 * entries to be flushed or updated with the new hasindex data.  This must
 * happen even if we find that no change is needed in the pg_class row.
 * ----------------
 */
void
setRelhasindex(Oid relid, bool hasindex, bool isprimary, Oid reltoastidxid)
{
	Relation	pg_class;
	HeapTuple	tuple;
	Form_pg_class classtuple;
	bool		dirty = false;
	HeapScanDesc pg_class_scan = NULL;

	/*
	 * Find the tuple to update in pg_class.
	 */
	pg_class = heap_openr(RelationRelationName, RowExclusiveLock);

	if (!IsIgnoringSystemIndexes() &&
		(!IsReindexProcessing() || pg_class->rd_rel->relhasindex))
	{
		tuple = SearchSysCacheCopy(RELOID,
								   ObjectIdGetDatum(relid),
								   0, 0, 0);
	}
	else
	{
		ScanKeyData key[1];

		ScanKeyEntryInitialize(&key[0], 0,
							   ObjectIdAttributeNumber,
							   F_OIDEQ,
							   ObjectIdGetDatum(relid));

		pg_class_scan = heap_beginscan(pg_class, SnapshotNow, 1, key);
		tuple = heap_getnext(pg_class_scan, ForwardScanDirection);
	}

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "setRelhasindex: cannot find relation %u in pg_class",
			 relid);

	/*
	 * Update fields in the pg_class tuple.
	 */
	if (pg_class_scan)
		LockBuffer(pg_class_scan->rs_cbuf, BUFFER_LOCK_EXCLUSIVE);

	classtuple = (Form_pg_class) GETSTRUCT(tuple);

	if (classtuple->relhasindex != hasindex)
	{
		classtuple->relhasindex = hasindex;
		dirty = true;
	}
	if (isprimary)
	{
		if (!classtuple->relhaspkey)
		{
			classtuple->relhaspkey = true;
			dirty = true;
		}
	}
	if (OidIsValid(reltoastidxid))
	{
		Assert(classtuple->relkind == RELKIND_TOASTVALUE);
		if (classtuple->reltoastidxid != reltoastidxid)
		{
			classtuple->reltoastidxid = reltoastidxid;
			dirty = true;
		}
	}

	if (pg_class_scan)
		LockBuffer(pg_class_scan->rs_cbuf, BUFFER_LOCK_UNLOCK);

	if (pg_class_scan)
	{
		/* Write the modified tuple in-place */
		WriteNoReleaseBuffer(pg_class_scan->rs_cbuf);
		/* Send out shared cache inval if necessary */
		if (!IsBootstrapProcessingMode())
			CacheInvalidateHeapTuple(pg_class, tuple);
		BufferSync();
	}
	else if (dirty)
	{
		simple_heap_update(pg_class, &tuple->t_self, tuple);

		/* Keep the catalog indexes up to date */
		CatalogUpdateIndexes(pg_class, tuple);
	}
	else
	{
		/* no need to change tuple, but force relcache rebuild anyway */
		CacheInvalidateRelcache(relid);
	}

	if (!pg_class_scan)
		heap_freetuple(tuple);
	else
		heap_endscan(pg_class_scan);

	heap_close(pg_class, RowExclusiveLock);
}

/*
 * setNewRelfilenode		- assign a new relfilenode value to the relation
 *
 * Caller must already hold exclusive lock on the relation.
 */
void
setNewRelfilenode(Relation relation)
{
	Oid			newrelfilenode;
	Relation	pg_class;
	HeapTuple	tuple;
	Form_pg_class rd_rel;
	HeapScanDesc pg_class_scan = NULL;
	bool		in_place_upd;
	RelationData workrel;

	Assert(!IsSystemRelation(relation) || IsToastRelation(relation) ||
		   relation->rd_rel->relkind == RELKIND_INDEX);

	/* Allocate a new relfilenode */
	newrelfilenode = newoid();

	/*
	 * Find the RELATION relation tuple for the given relation.
	 */
	pg_class = heap_openr(RelationRelationName, RowExclusiveLock);

	in_place_upd = IsIgnoringSystemIndexes();

	if (!in_place_upd)
	{
		tuple = SearchSysCacheCopy(RELOID,
								   ObjectIdGetDatum(RelationGetRelid(relation)),
								   0, 0, 0);
	}
	else
	{
		ScanKeyData key[1];

		ScanKeyEntryInitialize(&key[0], 0,
							   ObjectIdAttributeNumber,
							   F_OIDEQ,
							   ObjectIdGetDatum(RelationGetRelid(relation)));

		pg_class_scan = heap_beginscan(pg_class, SnapshotNow, 1, key);
		tuple = heap_getnext(pg_class_scan, ForwardScanDirection);
	}

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "setNewRelfilenode: cannot find relation %u in pg_class",
			 RelationGetRelid(relation));
	rd_rel = (Form_pg_class) GETSTRUCT(tuple);

	/* schedule unlinking old relfilenode */
	smgrunlink(DEFAULT_SMGR, relation);

	/* create another storage file. Is it a little ugly ? */
	memcpy((char *) &workrel, relation, sizeof(RelationData));
	workrel.rd_fd = -1;
	workrel.rd_node.relNode = newrelfilenode;
	heap_storage_create(&workrel);
	smgrclose(DEFAULT_SMGR, &workrel);

	/* update the pg_class row */
	if (in_place_upd)
	{
		LockBuffer(pg_class_scan->rs_cbuf, BUFFER_LOCK_EXCLUSIVE);
		rd_rel->relfilenode = newrelfilenode;
		LockBuffer(pg_class_scan->rs_cbuf, BUFFER_LOCK_UNLOCK);
		WriteNoReleaseBuffer(pg_class_scan->rs_cbuf);
		BufferSync();
		/* Send out shared cache inval if necessary */
		if (!IsBootstrapProcessingMode())
			CacheInvalidateHeapTuple(pg_class, tuple);
	}
	else
	{
		rd_rel->relfilenode = newrelfilenode;
		simple_heap_update(pg_class, &tuple->t_self, tuple);
		CatalogUpdateIndexes(pg_class, tuple);
	}

	if (!pg_class_scan)
		heap_freetuple(tuple);
	else
		heap_endscan(pg_class_scan);

	heap_close(pg_class, RowExclusiveLock);

	/* Make sure the relfilenode change is visible */
	CommandCounterIncrement();
}


/* ----------------
 *		UpdateStats
 *
 * Update pg_class' relpages and reltuples statistics for the given relation
 * (which can be either a table or an index).  Note that this is not used
 * in the context of VACUUM.
 * ----------------
 */
void
UpdateStats(Oid relid, double reltuples)
{
	Relation	whichRel;
	Relation	pg_class;
	HeapTuple	tuple;
	BlockNumber relpages;
	Form_pg_class rd_rel;
	HeapScanDesc pg_class_scan = NULL;
	bool		in_place_upd;

	/*
	 * This routine handles updates for both the heap and index relation
	 * statistics.	In order to guarantee that we're able to *see* the
	 * index relation tuple, we bump the command counter id here.  The
	 * index relation tuple was created in the current transaction.
	 */
	CommandCounterIncrement();

	/*
	 * CommandCounterIncrement() flushes invalid cache entries, including
	 * those for the heap and index relations for which we're updating
	 * statistics.	Now that the cache is flushed, it's safe to open the
	 * relation again.	We need the relation open in order to figure out
	 * how many blocks it contains.
	 */

	/*
	 * Grabbing lock here is probably redundant ...
	 */
	whichRel = relation_open(relid, ShareLock);

	/*
	 * Find the RELATION relation tuple for the given relation.
	 */
	pg_class = heap_openr(RelationRelationName, RowExclusiveLock);

	in_place_upd = (IsIgnoringSystemIndexes() || IsReindexProcessing());

	if (!in_place_upd)
	{
		tuple = SearchSysCacheCopy(RELOID,
								   ObjectIdGetDatum(relid),
								   0, 0, 0);
	}
	else
	{
		ScanKeyData key[1];

		ScanKeyEntryInitialize(&key[0], 0,
							   ObjectIdAttributeNumber,
							   F_OIDEQ,
							   ObjectIdGetDatum(relid));

		pg_class_scan = heap_beginscan(pg_class, SnapshotNow, 1, key);
		tuple = heap_getnext(pg_class_scan, ForwardScanDirection);
	}

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "UpdateStats: cannot find relation %u in pg_class",
			 relid);

	/*
	 * Figure values to insert.
	 *
	 * If we found zero tuples in the scan, do NOT believe it; instead put a
	 * bogus estimate into the statistics fields.  Otherwise, the common
	 * pattern "CREATE TABLE; CREATE INDEX; insert data" leaves the table
	 * with zero size statistics until a VACUUM is done.  The optimizer
	 * will generate very bad plans if the stats claim the table is empty
	 * when it is actually sizable.  See also CREATE TABLE in heap.c.
	 *
	 * Note: this path is also taken during bootstrap, because bootstrap.c
	 * passes reltuples = 0 after loading a table.	We have to estimate
	 * some number for reltuples based on the actual number of pages.
	 */
	relpages = RelationGetNumberOfBlocks(whichRel);

	if (reltuples == 0)
	{
		if (relpages == 0)
		{
			/* Bogus defaults for a virgin table, same as heap.c */
			reltuples = 1000;
			relpages = 10;
		}
		else if (whichRel->rd_rel->relkind == RELKIND_INDEX && relpages <= 2)
		{
			/* Empty index, leave bogus defaults in place */
			reltuples = 1000;
		}
		else
			reltuples = ((double) relpages) * NTUPLES_PER_PAGE(whichRel->rd_rel->relnatts);
	}

	/*
	 * Update statistics in pg_class, if they changed.  (Avoiding an
	 * unnecessary update is not just a tiny performance improvement;
	 * it also reduces the window wherein concurrent CREATE INDEX commands
	 * may conflict.)
	 */
	rd_rel = (Form_pg_class) GETSTRUCT(tuple);

	if (rd_rel->relpages != (int32) relpages ||
		rd_rel->reltuples != (float4) reltuples)
	{
		if (in_place_upd)
		{
			/*
			 * At bootstrap time, we don't need to worry about concurrency or
			 * visibility of changes, so we cheat.	Also cheat if REINDEX.
			 */
			LockBuffer(pg_class_scan->rs_cbuf, BUFFER_LOCK_EXCLUSIVE);
			rd_rel->relpages = (int32) relpages;
			rd_rel->reltuples = (float4) reltuples;
			LockBuffer(pg_class_scan->rs_cbuf, BUFFER_LOCK_UNLOCK);
			WriteNoReleaseBuffer(pg_class_scan->rs_cbuf);
			if (!IsBootstrapProcessingMode())
				CacheInvalidateHeapTuple(pg_class, tuple);
		}
		else
		{
			/* During normal processing, must work harder. */
			rd_rel->relpages = (int32) relpages;
			rd_rel->reltuples = (float4) reltuples;
			simple_heap_update(pg_class, &tuple->t_self, tuple);
			CatalogUpdateIndexes(pg_class, tuple);
		}
	}

	if (!pg_class_scan)
		heap_freetuple(tuple);
	else
		heap_endscan(pg_class_scan);

	/*
	 * We shouldn't have to do this, but we do...  Modify the reldesc in
	 * place with the new values so that the cache contains the latest
	 * copy.  (XXX is this really still necessary?  The relcache will get
	 * fixed at next CommandCounterIncrement, so why bother here?)
	 */
	whichRel->rd_rel->relpages = (int32) relpages;
	whichRel->rd_rel->reltuples = (float4) reltuples;

	heap_close(pg_class, RowExclusiveLock);
	relation_close(whichRel, NoLock);
}


/*
 * index_build - invoke access-method-specific index build procedure
 */
void
index_build(Relation heapRelation,
			Relation indexRelation,
			IndexInfo *indexInfo)
{
	RegProcedure procedure;

	/*
	 * sanity checks
	 */
	Assert(RelationIsValid(indexRelation));
	Assert(PointerIsValid(indexRelation->rd_am));

	procedure = indexRelation->rd_am->ambuild;
	Assert(RegProcedureIsValid(procedure));

	/*
	 * Call the access method's build procedure
	 */
	OidFunctionCall3(procedure,
					 PointerGetDatum(heapRelation),
					 PointerGetDatum(indexRelation),
					 PointerGetDatum(indexInfo));
}


/*
 * IndexBuildHeapScan - scan the heap relation to find tuples to be indexed
 *
 * This is called back from an access-method-specific index build procedure
 * after the AM has done whatever setup it needs.  The parent heap relation
 * is scanned to find tuples that should be entered into the index.  Each
 * such tuple is passed to the AM's callback routine, which does the right
 * things to add it to the new index.  After we return, the AM's index
 * build procedure does whatever cleanup is needed; in particular, it should
 * close the heap and index relations.
 *
 * The total count of heap tuples is returned.	This is for updating pg_class
 * statistics.	(It's annoying not to be able to do that here, but we can't
 * do it until after the relation is closed.)  Note that the index AM itself
 * must keep track of the number of index tuples; we don't do so here because
 * the AM might reject some of the tuples for its own reasons, such as being
 * unable to store NULLs.
 */
double
IndexBuildHeapScan(Relation heapRelation,
				   Relation indexRelation,
				   IndexInfo *indexInfo,
				   IndexBuildCallback callback,
				   void *callback_state)
{
	HeapScanDesc scan;
	HeapTuple	heapTuple;
	TupleDesc	heapDescriptor;
	Datum		attdata[INDEX_MAX_KEYS];
	char		nulls[INDEX_MAX_KEYS];
	double		reltuples;
	List	   *predicate;
	TupleTable	tupleTable;
	TupleTableSlot *slot;
	EState	   *estate;
	ExprContext *econtext;
	Snapshot	snapshot;
	TransactionId OldestXmin;

	/*
	 * sanity checks
	 */
	Assert(OidIsValid(indexRelation->rd_rel->relam));

	heapDescriptor = RelationGetDescr(heapRelation);

	/*
	 * Need an EState for evaluation of functional-index functions
	 * and partial-index predicates.
	 */
	estate = CreateExecutorState();
	econtext = GetPerTupleExprContext(estate);

	/*
	 * If this is a predicate (partial) index, we will need to evaluate
	 * the predicate using ExecQual, which requires the current tuple to
	 * be in a slot of a TupleTable.
	 */
	if (indexInfo->ii_Predicate != NIL)
	{
		tupleTable = ExecCreateTupleTable(1);
		slot = ExecAllocTableSlot(tupleTable);
		ExecSetSlotDescriptor(slot, heapDescriptor, false);

		/* Arrange for econtext's scan tuple to be the tuple under test */
		econtext->ecxt_scantuple = slot;

		/*
		 * Set up execution state for predicate.  Note: we mustn't attempt to
		 * cache this in the indexInfo, since we're building it in a transient
		 * EState.
		 */
		predicate = (List *)
			ExecPrepareExpr((Expr *) indexInfo->ii_Predicate,
							estate);
	}
	else
	{
		tupleTable = NULL;
		slot = NULL;
		predicate = NIL;
	}

	/*
	 * Ok, begin our scan of the base relation.  We use SnapshotAny
	 * because we must retrieve all tuples and do our own time qual
	 * checks.
	 */
	if (IsBootstrapProcessingMode())
	{
		snapshot = SnapshotNow;
		OldestXmin = InvalidTransactionId;
	}
	else
	{
		snapshot = SnapshotAny;
		OldestXmin = GetOldestXmin(heapRelation->rd_rel->relisshared);
	}

	scan = heap_beginscan(heapRelation, /* relation */
						  snapshot,		/* seeself */
						  0,	/* number of keys */
						  (ScanKey) NULL);		/* scan key */

	reltuples = 0;

	/*
	 * Scan all tuples in the base relation.
	 */
	while ((heapTuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		bool		tupleIsAlive;

		CHECK_FOR_INTERRUPTS();

		if (snapshot == SnapshotAny)
		{
			/* do our own time qual check */
			bool		indexIt;
			uint16		sv_infomask;

			/*
			 * HeapTupleSatisfiesVacuum may update tuple's hint status
			 * bits. We could possibly get away with not locking the
			 * buffer here, since caller should hold ShareLock on the
			 * relation, but let's be conservative about it.
			 */
			LockBuffer(scan->rs_cbuf, BUFFER_LOCK_SHARE);
			sv_infomask = heapTuple->t_data->t_infomask;

			switch (HeapTupleSatisfiesVacuum(heapTuple->t_data, OldestXmin))
			{
				case HEAPTUPLE_DEAD:
					indexIt = false;
					tupleIsAlive = false;
					break;
				case HEAPTUPLE_LIVE:
					indexIt = true;
					tupleIsAlive = true;
					break;
				case HEAPTUPLE_RECENTLY_DEAD:

					/*
					 * If tuple is recently deleted then we must index it
					 * anyway to keep VACUUM from complaining.
					 */
					indexIt = true;
					tupleIsAlive = false;
					break;
				case HEAPTUPLE_INSERT_IN_PROGRESS:

					/*
					 * Since caller should hold ShareLock or better, we
					 * should not see any tuples inserted by open
					 * transactions --- unless it's our own transaction.
					 * (Consider INSERT followed by CREATE INDEX within a
					 * transaction.)
					 */
					if (!TransactionIdIsCurrentTransactionId(
							  HeapTupleHeaderGetXmin(heapTuple->t_data)))
						elog(ERROR, "IndexBuildHeapScan: concurrent insert in progress");
					indexIt = true;
					tupleIsAlive = true;
					break;
				case HEAPTUPLE_DELETE_IN_PROGRESS:

					/*
					 * Since caller should hold ShareLock or better, we
					 * should not see any tuples deleted by open
					 * transactions --- unless it's our own transaction.
					 * (Consider DELETE followed by CREATE INDEX within a
					 * transaction.)
					 */
					if (!TransactionIdIsCurrentTransactionId(
							  HeapTupleHeaderGetXmax(heapTuple->t_data)))
						elog(ERROR, "IndexBuildHeapScan: concurrent delete in progress");
					indexIt = true;
					tupleIsAlive = false;
					break;
				default:
					elog(ERROR, "Unexpected HeapTupleSatisfiesVacuum result");
					indexIt = tupleIsAlive = false;		/* keep compiler quiet */
					break;
			}

			/* check for hint-bit update by HeapTupleSatisfiesVacuum */
			if (sv_infomask != heapTuple->t_data->t_infomask)
				SetBufferCommitInfoNeedsSave(scan->rs_cbuf);

			LockBuffer(scan->rs_cbuf, BUFFER_LOCK_UNLOCK);

			if (!indexIt)
				continue;
		}
		else
		{
			/* heap_getnext did the time qual check */
			tupleIsAlive = true;
		}

		reltuples += 1;

		MemoryContextReset(econtext->ecxt_per_tuple_memory);

		/*
		 * In a partial index, discard tuples that don't satisfy the
		 * predicate.  We can also discard recently-dead tuples, since
		 * VACUUM doesn't complain about tuple count mismatch for partial
		 * indexes.
		 */
		if (predicate != NIL)
		{
			if (!tupleIsAlive)
				continue;
			ExecStoreTuple(heapTuple, slot, InvalidBuffer, false);
			if (!ExecQual(predicate, econtext, false))
				continue;
		}

		/*
		 * For the current heap tuple, extract all the attributes we use
		 * in this index, and note which are null.	This also performs
		 * evaluation of the function, if this is a functional index.
		 */
		FormIndexDatum(indexInfo,
					   heapTuple,
					   heapDescriptor,
					   econtext->ecxt_per_tuple_memory,
					   attdata,
					   nulls);

		/*
		 * You'd think we should go ahead and build the index tuple here,
		 * but some index AMs want to do further processing on the data
		 * first.  So pass the attdata and nulls arrays, instead.
		 */

		/* Call the AM's callback routine to process the tuple */
		callback(indexRelation, heapTuple, attdata, nulls, tupleIsAlive,
				 callback_state);
	}

	heap_endscan(scan);

	if (tupleTable)
		ExecDropTupleTable(tupleTable, true);

	FreeExecutorState(estate);

	return reltuples;
}


/*
 * IndexGetRelation: given an index's relation OID, get the OID of the
 * relation it is an index on.	Uses the system cache.
 */
static Oid
IndexGetRelation(Oid indexId)
{
	HeapTuple	tuple;
	Form_pg_index index;
	Oid			result;

	tuple = SearchSysCache(INDEXRELID,
						   ObjectIdGetDatum(indexId),
						   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "IndexGetRelation: can't find index id %u",
			 indexId);
	index = (Form_pg_index) GETSTRUCT(tuple);
	Assert(index->indexrelid == indexId);

	result = index->indrelid;
	ReleaseSysCache(tuple);
	return result;
}

/* ---------------------------------
 * activate_index -- activate/deactivate the specified index.
 *		Note that currently PostgreSQL doesn't hold the
 *		status per index
 * ---------------------------------
 */
static bool
activate_index(Oid indexId, bool activate, bool inplace)
{
	if (!activate)				/* Currently does nothing */
		return true;
	return reindex_index(indexId, false, inplace);
}

/* --------------------------------
 * reindex_index - This routine is used to recreate an index
 * --------------------------------
 */
bool
reindex_index(Oid indexId, bool force, bool inplace)
{
	Relation	iRel,
				heapRelation;
	IndexInfo  *indexInfo;
	Oid			heapId;
	bool		old;

	/*
	 * Open our index relation and get an exclusive lock on it.
	 *
	 * Note: doing this before opening the parent heap relation means there's
	 * a possibility for deadlock failure against another xact that is
	 * doing normal accesses to the heap and index.  However, it's not
	 * real clear why you'd be needing to do REINDEX on a table that's in
	 * active use, so I'd rather have the protection of making sure the
	 * index is locked down.
	 */
	iRel = index_open(indexId);
	if (iRel == NULL)
		elog(ERROR, "reindex_index: can't open index relation");
	LockRelation(iRel, AccessExclusiveLock);

	old = SetReindexProcessing(true);

	/* Get OID of index's parent table */
	heapId = iRel->rd_index->indrelid;

	/* Open the parent heap relation */
	heapRelation = heap_open(heapId, AccessExclusiveLock);
	if (heapRelation == NULL)
		elog(ERROR, "reindex_index: can't open heap relation");

	/*
	 * If it's a shared index, we must do inplace processing (because we
	 * have no way to update relfilenode in other databases).  Also, if
	 * it's a nailed-in-cache index, we must do inplace processing because
	 * the relcache can't cope with changing its relfilenode.
	 *
	 * In either of these cases, we are definitely processing a system
	 * index, so we'd better be ignoring system indexes.
	 */
	if (iRel->rd_rel->relisshared)
	{
		if (!IsIgnoringSystemIndexes())
			elog(ERROR, "the target relation %u is shared", indexId);
		inplace = true;
	}
	if (iRel->rd_isnailed)
	{
		if (!IsIgnoringSystemIndexes())
			elog(ERROR, "the target relation %u is nailed", indexId);
		inplace = true;
	}

	/* Fetch info needed for index_build */
	indexInfo = BuildIndexInfo(iRel->rd_index);

	if (inplace)
	{
		/*
		 * Release any buffers associated with this index.	If they're
		 * dirty, they're just dropped without bothering to flush to disk.
		 */
		DropRelationBuffers(iRel);

		/* Now truncate the actual data and set blocks to zero */
		smgrtruncate(DEFAULT_SMGR, iRel, 0);
		iRel->rd_nblocks = 0;
		iRel->rd_targblock = InvalidBlockNumber;
	}
	else
	{
		/*
		 * We'll build a new physical relation for the index.
		 */
		setNewRelfilenode(iRel);
	}

	/* Initialize the index and rebuild */
	index_build(heapRelation, iRel, indexInfo);

	/*
	 * index_build will close both the heap and index relations (but not
	 * give up the locks we hold on them).	So we're done.
	 */

	SetReindexProcessing(old);

	return true;
}

/*
 * ----------------------------
 * activate_indexes_of_a_table
 *	activate/deactivate indexes of the specified table.
 *
 * Caller must already hold exclusive lock on the table.
 * ----------------------------
 */
bool
activate_indexes_of_a_table(Relation heaprel, bool activate)
{
	if (IndexesAreActive(heaprel))
	{
		if (!activate)
			setRelhasindex(RelationGetRelid(heaprel), false, false,
						   InvalidOid);
		else
			return false;
	}
	else
	{
		if (activate)
			reindex_relation(RelationGetRelid(heaprel), false);
		else
			return false;
	}
	return true;
}

/* --------------------------------
 * reindex_relation - This routine is used to recreate indexes
 * of a relation.
 * --------------------------------
 */
bool
reindex_relation(Oid relid, bool force)
{
	Relation	indexRelation;
	ScanKeyData entry;
	HeapScanDesc scan;
	HeapTuple	indexTuple;
	bool		old,
				reindexed;
	bool		deactivate_needed,
				overwrite;
	Relation	rel;

	overwrite = deactivate_needed = false;

	/*
	 * Ensure to hold an exclusive lock throughout the transaction. The
	 * lock could be less intensive (in the non-overwrite path) but for
	 * now it's AccessExclusiveLock for simplicity.
	 */
	rel = heap_open(relid, AccessExclusiveLock);

	/*
	 * ignore the indexes of the target system relation while processing
	 * reindex.
	 */
	if (!IsIgnoringSystemIndexes() &&
		IsSystemRelation(rel) && !IsToastRelation(rel))
		deactivate_needed = true;

	/*
	 * Shared system indexes must be overwritten because it's impossible
	 * to update pg_class tuples of all databases.
	 */
	if (rel->rd_rel->relisshared)
	{
		if (IsIgnoringSystemIndexes())
		{
			overwrite = true;
			deactivate_needed = true;
		}
		else
			elog(ERROR, "the target relation %u is shared", relid);
	}

	old = SetReindexProcessing(true);

	if (deactivate_needed)
	{
		if (IndexesAreActive(rel))
		{
			if (!force)
			{
				SetReindexProcessing(old);
				heap_close(rel, NoLock);
				return false;
			}
			activate_indexes_of_a_table(rel, false);
			CommandCounterIncrement();
		}
	}

	/*
	 * Continue to hold the lock.
	 */
	heap_close(rel, NoLock);

	indexRelation = heap_openr(IndexRelationName, AccessShareLock);
	ScanKeyEntryInitialize(&entry, 0, Anum_pg_index_indrelid,
						   F_OIDEQ, ObjectIdGetDatum(relid));
	scan = heap_beginscan(indexRelation, SnapshotNow, 1, &entry);
	reindexed = false;
	while ((indexTuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		Form_pg_index index = (Form_pg_index) GETSTRUCT(indexTuple);

		if (activate_index(index->indexrelid, true, overwrite))
			reindexed = true;
		else
		{
			reindexed = false;
			break;
		}
	}
	heap_endscan(scan);
	heap_close(indexRelation, AccessShareLock);
	if (reindexed)
	{
		/*
		 * Ok,we could use the reindexed indexes of the target system
		 * relation now.
		 */
		if (deactivate_needed)
		{
			if (!overwrite && relid == RelOid_pg_class)
			{
				/*
				 * For pg_class, relhasindex should be set to true here in
				 * place.
				 */
				setRelhasindex(relid, true, false, InvalidOid);
				CommandCounterIncrement();

				/*
				 * However the following setRelhasindex() is needed to
				 * keep consistency with WAL.
				 */
			}
			setRelhasindex(relid, true, false, InvalidOid);
		}
	}
	SetReindexProcessing(old);

	return reindexed;
}
