/*-------------------------------------------------------------------------
 *
 * command.c
 *	  random postgres portal and utility support code
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/Attic/command.c,v 1.117 2001-01-23 01:48:16 momjian Exp $
 *
 * NOTES
 *	  The PerformAddAttribute() code, like most of the relation
 *	  manipulating code in the commands/ directory, should go
 *	  someplace closer to the lib/catalog code.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/tuptoaster.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/index.h"
#include "catalog/indexing.h"
#include "catalog/pg_attrdef.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_rewrite.h"
#include "commands/command.h"
#include "executor/spi.h"
#include "catalog/heap.h"
#include "miscadmin.h"
#include "optimizer/prep.h"
#include "utils/acl.h"
#include "utils/fmgroids.h"
#include "commands/trigger.h"

#include "parser/parse_expr.h"
#include "parser/parse_clause.h"
#include "parser/parse_relation.h"
#include "nodes/makefuncs.h"
#include "optimizer/planmain.h"
#include "optimizer/clauses.h"
#include "rewrite/rewriteSupport.h"
#include "commands/view.h"
#include "utils/temprel.h"
#include "executor/spi_priv.h"
#include "catalog/pg_index.h"
#include "catalog/pg_shadow.h"
#include "utils/relcache.h"

#ifdef	_DROP_COLUMN_HACK__
#include "parser/parse.h"
#endif	 /* _DROP_COLUMN_HACK__ */
#include "access/genam.h"


static bool needs_toast_table(Relation rel);
static bool is_relation(char *name);


/* --------------------------------
 *		PortalCleanup
 * --------------------------------
 */
void
PortalCleanup(Portal portal)
{
	MemoryContext oldcontext;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	AssertArg(PortalIsValid(portal));
	AssertArg(portal->cleanup == PortalCleanup);

	/* ----------------
	 *	set proper portal-executor context before calling ExecMain.
	 * ----------------
	 */
	oldcontext = MemoryContextSwitchTo(PortalGetHeapMemory(portal));

	/* ----------------
	 *	tell the executor to shutdown the query
	 * ----------------
	 */
	ExecutorEnd(PortalGetQueryDesc(portal), PortalGetState(portal));

	/* ----------------
	 *	switch back to previous context
	 * ----------------
	 */
	MemoryContextSwitchTo(oldcontext);
}

/* --------------------------------
 *		PerformPortalFetch
 * --------------------------------
 */
void
PerformPortalFetch(char *name,
				   bool forward,
				   int count,
				   char *tag,
				   CommandDest dest)
{
	Portal		portal;
	int			feature;
	QueryDesc  *queryDesc;
	MemoryContext oldcontext;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	if (name == NULL)
	{
		elog(NOTICE, "PerformPortalFetch: missing portal name");
		return;
	}

	/* ----------------
	 *	get the portal from the portal name
	 * ----------------
	 */
	portal = GetPortalByName(name);
	if (!PortalIsValid(portal))
	{
		elog(NOTICE, "PerformPortalFetch: portal \"%s\" not found",
			 name);
		return;
	}

	/* ----------------
	 *	switch into the portal context
	 * ----------------
	 */
	oldcontext = MemoryContextSwitchTo(PortalGetHeapMemory(portal));

	/* ----------------
	 *	setup "feature" to tell the executor which direction to go in.
	 * ----------------
	 */
	if (forward)
		feature = EXEC_FOR;
	else
		feature = EXEC_BACK;

	/* ----------------
	 *	tell the destination to prepare to receive some tuples
	 * ----------------
	 */
	queryDesc = PortalGetQueryDesc(portal);

	if (dest == None)			/* MOVE */
	{
		QueryDesc  *qdesc = (QueryDesc *) palloc(sizeof(QueryDesc));

		memcpy(qdesc, queryDesc, sizeof(QueryDesc));
		qdesc->dest = dest;
		queryDesc = qdesc;
	}

	BeginCommand(name,
				 queryDesc->operation,
				 portal->attinfo, /* QueryDescGetTypeInfo(queryDesc) */
				 false,			/* portal fetches don't end up in
								 * relations */
				 false,			/* this is a portal fetch, not a "retrieve
								 * portal" */
				 tag,
				 dest);

	/* ----------------
	 *	execute the portal fetch operation
	 * ----------------
	 */
	ExecutorRun(queryDesc, PortalGetState(portal), feature, (long) count);

	if (dest == None)			/* MOVE */
		pfree(queryDesc);

	/* ----------------
	 * Switch back to old context.
	 * ----------------
	 */
	MemoryContextSwitchTo(oldcontext);

	/* ----------------
	 * Note: the "end-of-command" tag is returned by higher-level
	 *		 utility code
	 * ----------------
	 */
}

/* --------------------------------
 *		PerformPortalClose
 * --------------------------------
 */
void
PerformPortalClose(char *name, CommandDest dest)
{
	Portal		portal;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	if (name == NULL)
	{
		elog(NOTICE, "PerformPortalClose: missing portal name");
		return;
	}

	/* ----------------
	 *	get the portal from the portal name
	 * ----------------
	 */
	portal = GetPortalByName(name);
	if (!PortalIsValid(portal))
	{
		elog(NOTICE, "PerformPortalClose: portal \"%s\" not found",
			 name);
		return;
	}

	/* ----------------
	 *	Note: PortalCleanup is called as a side-effect
	 * ----------------
	 */
	PortalDrop(&portal);
}

/* ----------------
 *		AlterTableAddColumn
 *		(formerly known as PerformAddAttribute)
 *
 *		adds an additional attribute to a relation
 *
 *		Adds attribute field(s) to a relation.	Each new attribute
 *		is given attnums in sequential order and is added to the
 *		ATTRIBUTE relation.  If the AMI fails, defunct tuples will
 *		remain in the ATTRIBUTE relation for later vacuuming.
 *		Later, there may be some reserved attribute names???
 *
 *		(If needed, can instead use elog to handle exceptions.)
 *
 *		Note:
 *				Initial idea of ordering the tuple attributes so that all
 *		the variable length domains occured last was scratched.  Doing
 *		so would not speed access too much (in general) and would create
 *		many complications in formtuple, amgetattr, and addattribute.
 *
 *		scan attribute catalog for name conflict (within rel)
 *		scan type catalog for absence of data type (if not arg)
 *		create attnum magically???
 *		create attribute tuple
 *		insert attribute in attribute catalog
 *		modify reldesc
 *		create new relation tuple
 *		insert new relation in relation catalog
 *		delete original relation from relation catalog
 * ----------------
 */
void
AlterTableAddColumn(const char *relationName,
					bool inherits,
					ColumnDef *colDef)
{
	Relation	rel,
				attrdesc;
	Oid			myrelid;
	HeapTuple	reltup;
	HeapTuple	newreltup;
	HeapTuple	attributeTuple;
	Form_pg_attribute attribute;
	FormData_pg_attribute attributeD;
	int			i;
	int			minattnum,
				maxatts;
	Relation	idescs[Num_pg_attr_indices];
	Relation	ridescs[Num_pg_class_indices];
	bool		hasindex;

	/*
	 * permissions checking.  this would normally be done in utility.c,
	 * but this particular routine is recursive.
	 *
	 * normally, only the owner of a class can change its schema.
	 */
	if (!allowSystemTableMods && IsSystemRelationName(relationName))
		elog(ERROR, "ALTER TABLE: relation \"%s\" is a system catalog",
			 relationName);
	if (!pg_ownercheck(GetUserId(), relationName, RELNAME))
		elog(ERROR, "ALTER TABLE: permission denied");

	/*
	 * Grab an exclusive lock on the target table, which we will NOT
	 * release until end of transaction.
	 */
	rel = heap_openr(relationName, AccessExclusiveLock);

	if (rel->rd_rel->relkind != RELKIND_RELATION)
		elog(ERROR, "ALTER TABLE: relation \"%s\" is not a table",
			 relationName);

	myrelid = RelationGetRelid(rel);
	heap_close(rel, NoLock);	/* close rel but keep lock! */

	/*
	 * we can't add a not null attribute
	 */
	if (colDef->is_not_null)
		elog(ERROR, "Can't add a NOT NULL attribute to an existing relation");

	if (colDef->raw_default || colDef->cooked_default)
		elog(ERROR, "Adding columns with defaults is not implemented.");


	/*
	 * if the first element in the 'schema' list is a "*" then we are
	 * supposed to add this attribute to all classes that inherit from
	 * 'relationName' (as well as to 'relationName').
	 *
	 * any permissions or problems with duplicate attributes will cause the
	 * whole transaction to abort, which is what we want -- all or
	 * nothing.
	 */
	if (colDef != NULL)
	{
		if (inherits)
		{
			List	   *child,
					   *children;

			/* this routine is actually in the planner */
			children = find_all_inheritors(myrelid);

			/*
			 * find_all_inheritors does the recursive search of the
			 * inheritance hierarchy, so all we have to do is process all
			 * of the relids in the list that it returns.
			 */
			foreach(child, children)
			{
				Oid			childrelid = lfirsti(child);
				char	   *childrelname;

				if (childrelid == myrelid)
					continue;
				rel = heap_open(childrelid, AccessExclusiveLock);
				childrelname = pstrdup(RelationGetRelationName(rel));
				heap_close(rel, AccessExclusiveLock);

				AlterTableAddColumn(childrelname, false, colDef);

				pfree(childrelname);
			}
		}
	}

	rel = heap_openr(RelationRelationName, RowExclusiveLock);

	reltup = SearchSysCache(RELNAME,
							PointerGetDatum(relationName),
							0, 0, 0);

	if (!HeapTupleIsValid(reltup))
		elog(ERROR, "ALTER TABLE: relation \"%s\" not found",
			 relationName);

	minattnum = ((Form_pg_class) GETSTRUCT(reltup))->relnatts;
	maxatts = minattnum + 1;
	if (maxatts > MaxHeapAttributeNumber)
		elog(ERROR, "ALTER TABLE: relations limited to %d columns",
			 MaxHeapAttributeNumber);

	attrdesc = heap_openr(AttributeRelationName, RowExclusiveLock);

	/*
	 * Open all (if any) pg_attribute indices
	 */
	hasindex = RelationGetForm(attrdesc)->relhasindex;
	if (hasindex)
		CatalogOpenIndices(Num_pg_attr_indices, Name_pg_attr_indices, idescs);

	attributeD.attrelid = reltup->t_data->t_oid;

	attributeTuple = heap_addheader(Natts_pg_attribute,
									sizeof attributeD,
									(char *) &attributeD);

	attribute = (Form_pg_attribute) GETSTRUCT(attributeTuple);

	i = 1 + minattnum;

	{
		HeapTuple	typeTuple;
		Form_pg_type tform;
		char	   *typename;
		int			attnelems;

		if (SearchSysCacheExists(ATTNAME,
								 ObjectIdGetDatum(reltup->t_data->t_oid),
								 PointerGetDatum(colDef->colname),
								 0, 0))
			elog(ERROR, "ALTER TABLE: column name \"%s\" already exists in table \"%s\"",
				 colDef->colname, relationName);

		/*
		 * check to see if it is an array attribute.
		 */

		typename = colDef->typename->name;

		if (colDef->typename->arrayBounds)
		{
			attnelems = length(colDef->typename->arrayBounds);
			typename = makeArrayTypeName(colDef->typename->name);
		}
		else
			attnelems = 0;

		typeTuple = SearchSysCache(TYPENAME,
								   PointerGetDatum(typename),
								   0, 0, 0);
		if (!HeapTupleIsValid(typeTuple))
			elog(ERROR, "ALTER TABLE: type \"%s\" does not exist", typename);
		tform = (Form_pg_type) GETSTRUCT(typeTuple);

		namestrcpy(&(attribute->attname), colDef->colname);
		attribute->atttypid = typeTuple->t_data->t_oid;
		attribute->attlen = tform->typlen;
		attribute->attdispersion = 0;
		attribute->attcacheoff = -1;
		attribute->atttypmod = colDef->typename->typmod;
		attribute->attnum = i;
		attribute->attbyval = tform->typbyval;
		attribute->attnelems = attnelems;
		attribute->attisset = (bool) (tform->typtype == 'c');
		attribute->attstorage = tform->typstorage;
		attribute->attalign = tform->typalign;
		attribute->attnotnull = false;
		attribute->atthasdef = (colDef->raw_default != NULL ||
								colDef->cooked_default != NULL);

		ReleaseSysCache(typeTuple);

		heap_insert(attrdesc, attributeTuple);
		if (hasindex)
			CatalogIndexInsert(idescs,
							   Num_pg_attr_indices,
							   attrdesc,
							   attributeTuple);
	}

	if (hasindex)
		CatalogCloseIndices(Num_pg_attr_indices, idescs);

	heap_close(attrdesc, RowExclusiveLock);

	/*
	 * Update number of attributes in pg_class tuple
	 */
	newreltup = heap_copytuple(reltup);

	((Form_pg_class) GETSTRUCT(newreltup))->relnatts = maxatts;
	heap_update(rel, &newreltup->t_self, newreltup, NULL);

	/* keep catalog indices current */
	CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, ridescs);
	CatalogIndexInsert(ridescs, Num_pg_class_indices, rel, newreltup);
	CatalogCloseIndices(Num_pg_class_indices, ridescs);

	heap_freetuple(newreltup);
	ReleaseSysCache(reltup);

	heap_close(rel, NoLock);

	/*
	 * Automatically create the secondary relation for TOAST
	 * if it formerly had no such but now has toastable attributes.
	 */
	CommandCounterIncrement();
	AlterTableCreateToastTable(relationName, true);
}



static void drop_default(Oid relid, int16 attnum);


/*
 * ALTER TABLE ALTER COLUMN SET/DROP DEFAULT
 */
void
AlterTableAlterColumn(const char *relationName,
					  bool inh, const char *colName,
					  Node *newDefault)
{
	Relation	rel;
	HeapTuple	tuple;
	int16		attnum;
	Oid			myrelid;

	if (!allowSystemTableMods && IsSystemRelationName(relationName))
		elog(ERROR, "ALTER TABLE: relation \"%s\" is a system catalog",
			 relationName);
#ifndef NO_SECURITY
	if (!pg_ownercheck(GetUserId(), relationName, RELNAME))
		elog(ERROR, "ALTER TABLE: permission denied");
#endif

	rel = heap_openr(relationName, AccessExclusiveLock);
	if (rel->rd_rel->relkind != RELKIND_RELATION)
		elog(ERROR, "ALTER TABLE: relation \"%s\" is not a table",
			 relationName);
	myrelid = RelationGetRelid(rel);
	heap_close(rel, NoLock);

	/*
	 * Propagate to children if desired
	 */
	if (inh)
	{
		List	   *child,
				   *children;

		/* this routine is actually in the planner */
		children = find_all_inheritors(myrelid);

		/*
		 * find_all_inheritors does the recursive search of the
		 * inheritance hierarchy, so all we have to do is process all of
		 * the relids in the list that it returns.
		 */
		foreach(child, children)
		{
			Oid			childrelid = lfirsti(child);

			if (childrelid == myrelid)
				continue;
			rel = heap_open(childrelid, AccessExclusiveLock);
			AlterTableAlterColumn(RelationGetRelationName(rel),
								  false, colName, newDefault);
			heap_close(rel, AccessExclusiveLock);
		}
	}

	/* -= now do the thing on this relation =- */

	/* reopen the business */
	rel = heap_openr((char *) relationName, AccessExclusiveLock);

	/*
	 * get the number of the attribute
	 */
	tuple = SearchSysCache(ATTNAME,
						   ObjectIdGetDatum(myrelid),
						   PointerGetDatum(colName),
						   0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		heap_close(rel, AccessExclusiveLock);
		elog(ERROR, "ALTER TABLE: relation \"%s\" has no column \"%s\"",
			 relationName, colName);
	}

	attnum = ((Form_pg_attribute) GETSTRUCT(tuple))->attnum;
	ReleaseSysCache(tuple);

	if (newDefault)				/* SET DEFAULT */
	{
		List	   *rawDefaults = NIL;
		RawColumnDefault *rawEnt;

		/* Get rid of the old one first */
		drop_default(myrelid, attnum);

		rawEnt = (RawColumnDefault *) palloc(sizeof(RawColumnDefault));
		rawEnt->attnum = attnum;
		rawEnt->raw_default = newDefault;
		rawDefaults = lappend(rawDefaults, rawEnt);

		/*
		 * This function is intended for CREATE TABLE, so it processes a
		 * _list_ of defaults, but we just do one.
		 */
		AddRelationRawConstraints(rel, rawDefaults, NIL);
	}

	else
/* DROP DEFAULT */
	{
		Relation	attr_rel;
		ScanKeyData scankeys[3];
		HeapScanDesc scan;

		attr_rel = heap_openr(AttributeRelationName, AccessExclusiveLock);
		ScanKeyEntryInitialize(&scankeys[0], 0x0,
							   Anum_pg_attribute_attrelid, F_OIDEQ,
							   ObjectIdGetDatum(myrelid));
		ScanKeyEntryInitialize(&scankeys[1], 0x0,
							   Anum_pg_attribute_attnum, F_INT2EQ,
							   Int16GetDatum(attnum));
		ScanKeyEntryInitialize(&scankeys[2], 0x0,
							   Anum_pg_attribute_atthasdef, F_BOOLEQ,
							   BoolGetDatum(true));

		scan = heap_beginscan(attr_rel, false, SnapshotNow, 3, scankeys);
		AssertState(scan != NULL);

		if (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
		{
			HeapTuple	newtuple;
			Relation	irelations[Num_pg_attr_indices];

			/* update to false */
			newtuple = heap_copytuple(tuple);
			((Form_pg_attribute) GETSTRUCT(newtuple))->atthasdef = FALSE;
			heap_update(attr_rel, &tuple->t_self, newtuple, NULL);

			/* keep the system catalog indices current */
			CatalogOpenIndices(Num_pg_attr_indices, Name_pg_attr_indices, irelations);
			CatalogIndexInsert(irelations, Num_pg_attr_indices, attr_rel, newtuple);
			CatalogCloseIndices(Num_pg_attr_indices, irelations);

			/* get rid of actual default definition */
			drop_default(myrelid, attnum);
		}

		heap_endscan(scan);
		heap_close(attr_rel, NoLock);
	}

	heap_close(rel, NoLock);
}



static void
drop_default(Oid relid, int16 attnum)
{
	ScanKeyData scankeys[2];
	HeapScanDesc scan;
	Relation	attrdef_rel;
	HeapTuple	tuple;

	attrdef_rel = heap_openr(AttrDefaultRelationName, AccessExclusiveLock);
	ScanKeyEntryInitialize(&scankeys[0], 0x0,
						   Anum_pg_attrdef_adrelid, F_OIDEQ,
						   ObjectIdGetDatum(relid));
	ScanKeyEntryInitialize(&scankeys[1], 0x0,
						   Anum_pg_attrdef_adnum, F_INT2EQ,
						   Int16GetDatum(attnum));

	scan = heap_beginscan(attrdef_rel, false, SnapshotNow, 2, scankeys);
	AssertState(scan != NULL);

	if (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
		heap_delete(attrdef_rel, &tuple->t_self, NULL);

	heap_endscan(scan);

	heap_close(attrdef_rel, NoLock);
}


#ifdef	_DROP_COLUMN_HACK__
/*
 *	ALTER TABLE DROP COLUMN trial implementation
 *
 */

/*
 *	system table scan(index scan/sequential scan)
 */
typedef struct SysScanDescData
{
	Relation	heap_rel;
	Relation	irel;
	HeapScanDesc scan;
	IndexScanDesc iscan;
	HeapTupleData tuple;
	Buffer		buffer;
}			SysScanDescData, *SysScanDesc;

static void *
systable_beginscan(Relation rel, const char *indexRelname, int nkeys, ScanKey entry)
{
	bool		hasindex = (rel->rd_rel->relhasindex && !IsIgnoringSystemIndexes());
	SysScanDesc sysscan;

	sysscan = (SysScanDesc) palloc(sizeof(SysScanDescData));
	sysscan->heap_rel = rel;
	sysscan->irel = (Relation) NULL;
	sysscan->tuple.t_datamcxt = NULL;
	sysscan->tuple.t_data = NULL;
	sysscan->buffer = InvalidBuffer;
	if (hasindex)
	{
		sysscan->irel = index_openr((char *) indexRelname);
		sysscan->iscan = index_beginscan(sysscan->irel, false, nkeys, entry);
	}
	else
		sysscan->scan = heap_beginscan(rel, false, SnapshotNow, nkeys, entry);
	return (void *) sysscan;
}

static void
systable_endscan(void *scan)
{
	SysScanDesc sysscan = (SysScanDesc) scan;

	if (sysscan->irel)
	{
		if (BufferIsValid(sysscan->buffer))
			ReleaseBuffer(sysscan->buffer);
		index_endscan(sysscan->iscan);
		index_close(sysscan->irel);
	}
	else
		heap_endscan(sysscan->scan);
	pfree(scan);
}

static HeapTuple
systable_getnext(void *scan)
{
	SysScanDesc sysscan = (SysScanDesc) scan;
	HeapTuple	htup = (HeapTuple) NULL;
	RetrieveIndexResult indexRes;

	if (sysscan->irel)
	{
		if (BufferIsValid(sysscan->buffer))
		{
			ReleaseBuffer(sysscan->buffer);
			sysscan->buffer = InvalidBuffer;
		}
		while (indexRes = index_getnext(sysscan->iscan, ForwardScanDirection), indexRes != NULL)
		{
			sysscan->tuple.t_self = indexRes->heap_iptr;
			heap_fetch(sysscan->heap_rel, SnapshotNow, &sysscan->tuple, &(sysscan->buffer));
			pfree(indexRes);
			if (sysscan->tuple.t_data != NULL)
			{
				htup = &sysscan->tuple;
				break;
			}
		}
	}
	else
		htup = heap_getnext(sysscan->scan, 0);
	return htup;
}

/*
 *	find a specified attribute in a node entry
 */
static bool
find_attribute_walker(Node *node, int *attnump)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;

		if (var->varlevelsup == 0 && var->varno == 1 &&
			var->varattno == *attnump)
			return true;
	}
	return expression_tree_walker(node, find_attribute_walker,
								  (void *) attnump);
}

static bool
find_attribute_in_node(Node *node, int attnum)
{
	return find_attribute_walker(node, &attnum);
}

/*
 *	Remove/check references for the column
 */
static bool
RemoveColumnReferences(Oid reloid, int attnum, bool checkonly, HeapTuple reltup)
{
	Relation	indexRelation,
				rcrel;
	ScanKeyData entry;
	HeapScanDesc scan;
	void	   *sysscan;
	HeapTuple	htup,
				indexTuple;
	Form_pg_index index;
	Form_pg_class pgcform = (Form_pg_class) NULL;
	int			i;
	bool		checkok = true;


	if (!checkonly)
		pgcform = (Form_pg_class) GETSTRUCT(reltup);

	/*
	 * Remove/check constraints here
	 */
	ScanKeyEntryInitialize(&entry, (bits16) 0x0, Anum_pg_relcheck_rcrelid,
					   (RegProcedure) F_OIDEQ, ObjectIdGetDatum(reloid));
	rcrel = heap_openr(RelCheckRelationName, RowExclusiveLock);
	sysscan = systable_beginscan(rcrel, RelCheckIndex, 1, &entry);

	while (HeapTupleIsValid(htup = systable_getnext(sysscan)))
	{
		Form_pg_relcheck relcheck;
		char	   *ccbin;
		Node	   *node;

		relcheck = (Form_pg_relcheck) GETSTRUCT(htup);
		ccbin = DatumGetCString(DirectFunctionCall1(textout,
										PointerGetDatum(&relcheck->rcbin)));
		node = stringToNode(ccbin);
		pfree(ccbin);
		if (find_attribute_in_node(node, attnum))
		{
			if (checkonly)
			{
				checkok = false;
				elog(ERROR, "target column is used in a constraint");
			}
			else
			{
				heap_delete(rcrel, &htup->t_self, NULL);
				pgcform->relchecks--;
			}
		}
	}
	systable_endscan(sysscan);
	heap_close(rcrel, NoLock);

	/*
	 * What to do with triggers/rules/views/procedues ?
	 */

	/*
	 * Remove/check indexes
	 */
	indexRelation = heap_openr(IndexRelationName, RowExclusiveLock);
	ScanKeyEntryInitialize(&entry, 0, Anum_pg_index_indrelid, F_OIDEQ,
						   ObjectIdGetDatum(reloid));
	scan = heap_beginscan(indexRelation, false, SnapshotNow, 1, &entry);
	while (HeapTupleIsValid(indexTuple = heap_getnext(scan, 0)))
	{
		index = (Form_pg_index) GETSTRUCT(indexTuple);
		for (i = 0; i < INDEX_MAX_KEYS; i++)
		{
			if (index->indkey[i] == InvalidAttrNumber)
				break;
			else if (index->indkey[i] == attnum)
			{
				if (checkonly)
				{
					checkok = false;
					elog(ERROR, "target column is used in an index");
				}
				else
				{
					htup = SearchSysCache(RELOID,
										  ObjectIdGetDatum(index->indexrelid),
										  0, 0, 0);
					RemoveIndex(NameStr(((Form_pg_class) GETSTRUCT(htup))->relname));
					ReleaseSysCache(htup);
				}
				break;
			}
		}
	}
	heap_endscan(scan);
	heap_close(indexRelation, NoLock);

	return checkok;
}

#endif	 /* _DROP_COLUMN_HACK__ */

/*
 * ALTER TABLE DROP COLUMN
 */
void
AlterTableDropColumn(const char *relationName,
					 bool inh, const char *colName,
					 int behavior)
{
#ifdef	_DROP_COLUMN_HACK__
	Relation	rel,
				attrdesc,
				adrel;
	Oid			myrelid,
				attoid;
	HeapTuple	reltup;
	HeapTupleData classtuple;
	Buffer		buffer;
	Form_pg_attribute attribute;
	HeapTuple	tup;
	Relation	idescs[Num_pg_attr_indices];
	int			attnum;
	bool		hasindex;
	char		dropColname[32];
	void	   *sysscan;
	ScanKeyData scankeys[2];

	if (inh)
		elog(ERROR, "ALTER TABLE / DROP COLUMN with inherit option is not supported yet");

	/*
	 * permissions checking.  this would normally be done in utility.c,
	 * but this particular routine is recursive.
	 *
	 * normally, only the owner of a class can change its schema.
	 */
	if (!allowSystemTableMods && IsSystemRelationName(relationName))
		elog(ERROR, "ALTER TABLE: relation \"%s\" is a system catalog",
			 relationName);
#ifndef NO_SECURITY
	if (!pg_ownercheck(GetUserId(), relationName, RELNAME))
		elog(ERROR, "ALTER TABLE: permission denied");
#endif

	/*
	 * Grab an exclusive lock on the target table, which we will NOT
	 * release until end of transaction.
	 */
	rel = heap_openr(relationName, AccessExclusiveLock);

	if (rel->rd_rel->relkind != RELKIND_RELATION)
		elog(ERROR, "ALTER TABLE: relation \"%s\" is not a table",
			 relationName);

	myrelid = RelationGetRelid(rel);
	heap_close(rel, NoLock);	/* close rel but keep lock! */

	/*
	 * What to do when rel has inheritors ?
	 */
	if (length(find_all_inheritors(myrelid)) > 1)
		elog(ERROR, "ALTER TABLE: cannot drop a column on table that is inherited from");

	/*
	 * lock the pg_class tuple for update
	 */
	rel = heap_openr(RelationRelationName, RowExclusiveLock);
	reltup = SearchSysCache(RELNAME,
							PointerGetDatum(relationName),
							0, 0, 0);
	if (!HeapTupleIsValid(reltup))
		elog(ERROR, "ALTER TABLE: relation \"%s\" not found",
			 relationName);
	classtuple.t_self = reltup->t_self;
	ReleaseSysCache(reltup);

	switch (heap_mark4update(rel, &classtuple, &buffer))
	{
		case HeapTupleSelfUpdated:
		case HeapTupleMayBeUpdated:
			break;
		default:
			elog(ERROR, "couldn't lock pg_class tuple");
	}
	reltup = heap_copytuple(&classtuple);
	ReleaseBuffer(buffer);

	attrdesc = heap_openr(AttributeRelationName, RowExclusiveLock);

	/*
	 * Get the target pg_attribute tuple and make a modifiable copy
	 */
	tup = SearchSysCacheCopy(ATTNAME,
							 ObjectIdGetDatum(reltup->t_data->t_oid),
							 PointerGetDatum(colName),
							 0, 0);
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "ALTER TABLE: column name \"%s\" doesn't exist in table \"%s\"",
			 colName, relationName);

	attribute = (Form_pg_attribute) GETSTRUCT(tup);
	attnum = attribute->attnum;
	if (attnum <= 0)
		elog(ERROR, "ALTER TABLE: column name \"%s\" was already dropped",
			 colName);
	attoid = tup->t_data->t_oid;

	/*
	 * Check constraints/indices etc here
	 */
	if (behavior != CASCADE)
	{
		if (!RemoveColumnReferences(myrelid, attnum, true, NULL))
			elog(ERROR, "the column is referenced");
	}

	/*
	 * change the target pg_attribute tuple
	 */
	sprintf(dropColname, "*already Dropped*%d", attnum);
	namestrcpy(&(attribute->attname), dropColname);
	ATTRIBUTE_DROP_COLUMN(attribute);

	heap_update(attrdesc, &tup->t_self, tup, NULL);
	hasindex = (!IsIgnoringSystemIndexes() && RelationGetForm(attrdesc)->relhasindex);
	if (hasindex)
	{
		CatalogOpenIndices(Num_pg_attr_indices, Name_pg_attr_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_attr_indices,
						   attrdesc, tup);
		CatalogCloseIndices(Num_pg_attr_indices, idescs);
	}
	heap_close(attrdesc, NoLock);
	heap_freetuple(tup);

	/* delete comments */
	DeleteComments(attoid);
	/* delete attrdef */
	adrel = heap_openr(AttrDefaultRelationName, RowExclusiveLock);
	ScanKeyEntryInitialize(&scankeys[0], 0x0, Anum_pg_attrdef_adrelid,
						   F_OIDEQ, ObjectIdGetDatum(myrelid));

	/*
	 * Oops pg_attrdef doesn't have (adrelid,adnum) index
	 * ScanKeyEntryInitialize(&scankeys[1], 0x0, Anum_pg_attrdef_adnum,
	 * F_INT2EQ, Int16GetDatum(attnum)); sysscan =
	 * systable_beginscan(adrel, AttrDefaultIndex, 2, scankeys);
	 */
	sysscan = systable_beginscan(adrel, AttrDefaultIndex, 1, scankeys);
	while (HeapTupleIsValid(tup = systable_getnext(sysscan)))
	{
		if (((Form_pg_attrdef) GETSTRUCT(tup))->adnum == attnum)
		{
			heap_delete(adrel, &tup->t_self, NULL);
			break;
		}
	}
	systable_endscan(sysscan);
	heap_close(adrel, NoLock);

	/*
	 * Remove objects which reference this column
	 */
	if (behavior == CASCADE)
	{
		Relation	ridescs[Num_pg_class_indices];

		RemoveColumnReferences(myrelid, attnum, false, reltup);
		/* update pg_class tuple */
		heap_update(rel, &reltup->t_self, reltup, NULL);
		CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, ridescs);
		CatalogIndexInsert(ridescs, Num_pg_class_indices, rel, reltup);
		CatalogCloseIndices(Num_pg_class_indices, ridescs);
	}

	heap_freetuple(reltup);
	heap_close(rel, NoLock);
#else
				elog(ERROR, "ALTER TABLE / DROP COLUMN is not implemented");
#endif	 /* _DROP_COLUMN_HACK__ */
}



/*
 * ALTER TABLE ADD CONSTRAINT
 */
void
AlterTableAddConstraint(char *relationName,
						bool inh, Node *newConstraint)
{
	if (newConstraint == NULL)
		elog(ERROR, "ALTER TABLE / ADD CONSTRAINT passed invalid constraint.");

#ifndef NO_SECURITY
	if (!pg_ownercheck(GetUserId(), relationName, RELNAME))
		elog(ERROR, "ALTER TABLE: permission denied");
#endif

	/* Disallow ADD CONSTRAINT on views, indexes, sequences, etc */
	if (! is_relation(relationName))
		elog(ERROR, "ALTER TABLE ADD CONSTRAINT: %s is not a table",
			 relationName);

	switch (nodeTag(newConstraint))
	{
		case T_Constraint:
		{
			Constraint *constr = (Constraint *) newConstraint;

			switch (constr->contype)
			{
				case CONSTR_CHECK:
				{
					ParseState *pstate;
					bool successful = true;
					HeapScanDesc scan;
					ExprContext *econtext;
					TupleTableSlot *slot = makeNode(TupleTableSlot);
					HeapTuple tuple;
					RangeTblEntry *rte;
					List       *qual;
					List       *constlist;
					Relation	rel;
					Node *expr;
					char *name;

					if (constr->name)
						name = constr->name;
					else
						name = "<unnamed>";

					constlist = makeList1(constr);

					rel = heap_openr(relationName, AccessExclusiveLock);

					/* make sure it is not a view */
					if (rel->rd_rel->relkind == RELKIND_VIEW)
						elog(ERROR, "ALTER TABLE: cannot add constraint to a view");

					/*
					 * Scan all of the rows, looking for a false match
					 */
					scan = heap_beginscan(rel, false, SnapshotNow, 0, NULL);
					AssertState(scan != NULL);

					/*
					 * We need to make a parse state and range table to allow
					 * us to transformExpr and fix_opids to get a version of
					 * the expression we can pass to ExecQual
					 */
					pstate = make_parsestate(NULL);
					makeRangeTable(pstate, NULL);
					rte = addRangeTableEntry(pstate, relationName, NULL,
											 false, true);
					addRTEtoJoinList(pstate, rte);

					/* Convert the A_EXPR in raw_expr into an EXPR */
					expr = transformExpr(pstate, constr->raw_expr,
										 EXPR_COLUMN_FIRST);

					/*
					 * Make sure it yields a boolean result.
					 */
					if (exprType(expr) != BOOLOID)
						elog(ERROR, "CHECK '%s' does not yield boolean result",
							 name);

					/*
					 * Make sure no outside relations are referred to.
					 */
					if (length(pstate->p_rtable) != 1)
						elog(ERROR, "Only relation '%s' can be referenced in CHECK",
							 relationName);

					/*
					 * Might as well try to reduce any constant expressions.
					 */
					expr = eval_const_expressions(expr);

					/* And fix the opids */
					fix_opids(expr);

					qual = makeList1(expr);

					/*
					 * Scan through the rows now, making the necessary things
					 * for ExecQual, and then call it to evaluate the
					 * expression.
					 */
					while (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
					{
						slot->val = tuple;
						slot->ttc_shouldFree = false;
						slot->ttc_descIsNew = true;
						slot->ttc_tupleDescriptor = rel->rd_att;
						slot->ttc_buffer = InvalidBuffer;

						econtext = MakeExprContext(slot, CurrentMemoryContext);
						if (!ExecQual(qual, econtext, true))
						{
							successful=false;
							break;
						}
						FreeExprContext(econtext);
					}

					pfree(slot);

					heap_endscan(scan);

					if (!successful)
					{
						heap_close(rel, NoLock);
						elog(ERROR, "AlterTableAddConstraint: rejected due to CHECK constraint %s", name);
					}
					/*
					 * Call AddRelationRawConstraints to do the real adding --
					 * It duplicates some of the above, but does not check the
					 * validity of the constraint against tuples already in
					 * the table.
					 */
					AddRelationRawConstraints(rel, NIL, constlist);
					heap_close(rel, NoLock);
					pfree(constlist);

					break;
				}
				default:
					elog(ERROR, "ALTER TABLE / ADD CONSTRAINT is not implemented for that constraint type.");
			}
			break;
		}
		case T_FkConstraint:
		{
			FkConstraint *fkconstraint = (FkConstraint *) newConstraint;
			Relation	rel, pkrel;
			HeapScanDesc scan;
			HeapTuple	tuple;
			Trigger		trig;
			List	   *list;
			int			count;
			List       *indexoidlist,
				*indexoidscan;
			Form_pg_attribute *rel_attrs = NULL;
			int			i;
			bool		found = false;

			if (is_temp_rel_name(fkconstraint->pktable_name) &&
				!is_temp_rel_name(relationName))
				elog(ERROR, "ALTER TABLE / ADD CONSTRAINT: Unable to reference temporary table from permanent table constraint.");

			/*
			 * Grab an exclusive lock on the pk table, so that someone
			 * doesn't delete rows out from under us.
			 */

			pkrel = heap_openr(fkconstraint->pktable_name, AccessExclusiveLock);
			if (pkrel->rd_rel->relkind != RELKIND_RELATION)
				elog(ERROR, "referenced table \"%s\" not a relation",
					 fkconstraint->pktable_name);

			/*
			 * Grab an exclusive lock on the fk table, and then scan
			 * through each tuple, calling the RI_FKey_Match_Ins
			 * (insert trigger) as if that tuple had just been
			 * inserted.  If any of those fail, it should elog(ERROR)
			 * and that's that.
			 */
			rel = heap_openr(relationName, AccessExclusiveLock);
			if (rel->rd_rel->relkind != RELKIND_RELATION)
				elog(ERROR, "referencing table \"%s\" not a relation",
					 relationName);

			/* First we check for limited correctness of the constraint */

			rel_attrs = pkrel->rd_att->attrs;
			indexoidlist = RelationGetIndexList(pkrel);

			foreach(indexoidscan, indexoidlist)
			{
				Oid             indexoid = lfirsti(indexoidscan);
				HeapTuple       indexTuple;
				Form_pg_index	indexStruct;

				indexTuple = SearchSysCache(INDEXRELID,
											ObjectIdGetDatum(indexoid),
											0, 0, 0);
				if (!HeapTupleIsValid(indexTuple))
					elog(ERROR, "transformFkeyGetPrimaryKey: index %u not found",
						 indexoid);
				indexStruct = (Form_pg_index) GETSTRUCT(indexTuple);

				if (indexStruct->indisunique)
				{
					List	   *attrl;

					/* Make sure this index has the same number of keys -- It obviously
					 * won't match otherwise. */
					for (i = 0; i < INDEX_MAX_KEYS && indexStruct->indkey[i] != 0; i++);
					if (i!=length(fkconstraint->pk_attrs))
						found=false;
					else {
						/* go through the fkconstraint->pk_attrs list */
						foreach(attrl, fkconstraint->pk_attrs)
						{
							Ident *attr=lfirst(attrl);
							found = false;
							for (i = 0; i < INDEX_MAX_KEYS && indexStruct->indkey[i] != 0; i++)
							{
								int pkattno = indexStruct->indkey[i];
								if (pkattno>0)
								{
									char *name = NameStr(rel_attrs[pkattno-1]->attname);
									if (strcmp(name, attr->name)==0)
									{
										found = true;
										break;
									}
								}
							}
							if (!found)
								break;
						}
					}
				}
				ReleaseSysCache(indexTuple);
				if (found)
					break;
			}

			if (!found)
				elog(ERROR, "UNIQUE constraint matching given keys for referenced table \"%s\" not found",
					 fkconstraint->pktable_name);

			freeList(indexoidlist);
			heap_close(pkrel, NoLock);

			rel_attrs = rel->rd_att->attrs;
			if (fkconstraint->fk_attrs!=NIL) {
				List *fkattrs;
				Ident *fkattr;

				found = false;
				foreach(fkattrs, fkconstraint->fk_attrs) {
					int count;
					found = false;
					fkattr=lfirst(fkattrs);
					for (count = 0; count < rel->rd_att->natts; count++) {
						char *name = NameStr(rel->rd_att->attrs[count]->attname);
						if (strcmp(name, fkattr->name)==0) {
							found = true;
							break;
						}
					}
					if (!found)
						break;
				}
				if (!found)
					elog(ERROR, "columns referenced in foreign key constraint not found.");
			}

			trig.tgoid = 0;
			if (fkconstraint->constr_name)
				trig.tgname = fkconstraint->constr_name;
			else
				trig.tgname = "<unknown>";
			trig.tgfoid = 0;
			trig.tgtype = 0;
			trig.tgenabled = TRUE;
			trig.tgisconstraint = TRUE;
			trig.tginitdeferred = FALSE;
			trig.tgdeferrable = FALSE;

			trig.tgargs = (char **) palloc(
				sizeof(char *) * (4 + length(fkconstraint->fk_attrs)
								  + length(fkconstraint->pk_attrs)));

			if (fkconstraint->constr_name)
				trig.tgargs[0] = fkconstraint->constr_name;
			else
				trig.tgargs[0] = "<unknown>";
			trig.tgargs[1] = (char *) relationName;
			trig.tgargs[2] = fkconstraint->pktable_name;
			trig.tgargs[3] = fkconstraint->match_type;
			count = 4;
			foreach(list, fkconstraint->fk_attrs)
				{
					Ident	   *fk_at = lfirst(list);

					trig.tgargs[count] = fk_at->name;
					count+=2;
				}
			count = 5;
			foreach(list, fkconstraint->pk_attrs)
				{
					Ident	   *pk_at = lfirst(list);

					trig.tgargs[count] = pk_at->name;
					count+=2;
				}
			trig.tgnargs = count-1;

			scan = heap_beginscan(rel, false, SnapshotNow, 0, NULL);
			AssertState(scan != NULL);

			while (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
			{
				/* Make a call to the check function */
				/* No parameters are passed, but we do set a context */
				FunctionCallInfoData	fcinfo;
				TriggerData				trigdata;

				MemSet(&fcinfo, 0, sizeof(fcinfo));
				/* We assume RI_FKey_check_ins won't look at flinfo... */

				trigdata.type = T_TriggerData;
				trigdata.tg_event = TRIGGER_EVENT_INSERT | TRIGGER_EVENT_ROW;
				trigdata.tg_relation = rel;
				trigdata.tg_trigtuple = tuple;
				trigdata.tg_newtuple = NULL;
				trigdata.tg_trigger = &trig;

				fcinfo.context = (Node *) &trigdata;

				RI_FKey_check_ins(&fcinfo);
			}
			heap_endscan(scan);
			heap_close(rel, NoLock);		/* close rel but keep
											 * lock! */

			pfree(trig.tgargs);
			break;
		}
		default:
			elog(ERROR, "ALTER TABLE / ADD CONSTRAINT unable to determine type of constraint passed");
	}
}



/*
 * ALTER TABLE DROP CONSTRAINT
 */
void
AlterTableDropConstraint(const char *relationName,
						 bool inh, const char *constrName,
						 int behavior)
{
	elog(ERROR, "ALTER TABLE / DROP CONSTRAINT is not implemented");
}


/*
 * ALTER TABLE OWNER
 */
void
AlterTableOwner(const char *relationName, const char *newOwnerName)
{
	Relation 	class_rel;
	HeapTuple 	tuple;
	int32		newOwnerSysid;
	Relation	idescs[Num_pg_class_indices];

	/*
	 * first check that we are a superuser
	 */
	if (! superuser())
		elog(ERROR, "ALTER TABLE: permission denied");

	/*
	 * look up the new owner in pg_shadow and get the sysid
	 */
	tuple = SearchSysCache(SHADOWNAME,
						   PointerGetDatum(newOwnerName),
						   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "ALTER TABLE: user \"%s\" not found", newOwnerName);
	newOwnerSysid = ((Form_pg_shadow) GETSTRUCT(tuple))->usesysid;
	ReleaseSysCache(tuple);

	/*
	 * find the table's entry in pg_class and make a modifiable copy
	 */
	class_rel = heap_openr(RelationRelationName, RowExclusiveLock);

	tuple = SearchSysCacheCopy(RELNAME,
							   PointerGetDatum(relationName),
							   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "ALTER TABLE: relation \"%s\" not found",
			 relationName);

	switch (((Form_pg_class) GETSTRUCT(tuple))->relkind)
	{
		case RELKIND_RELATION:
		case RELKIND_INDEX:
		case RELKIND_VIEW:
		case RELKIND_SEQUENCE:
			/* ok to change owner */
			break;
		default:
			elog(ERROR, "ALTER TABLE: relation \"%s\" is not a table, index, view, or sequence",
				 relationName);
	}

	/*
	 * modify the table's entry and write to the heap
	 */
	((Form_pg_class) GETSTRUCT(tuple))->relowner = newOwnerSysid;

	heap_update(class_rel, &tuple->t_self, tuple, NULL);

	/* Keep the catalog indices up to date */
	CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, idescs);
	CatalogIndexInsert(idescs, Num_pg_class_indices, class_rel, tuple);
	CatalogCloseIndices(Num_pg_class_indices, idescs);

	/*
	 * unlock everything and return
	 */
	heap_freetuple(tuple);
	heap_close(class_rel, RowExclusiveLock);
}


/*
 * ALTER TABLE CREATE TOAST TABLE
 */
void
AlterTableCreateToastTable(const char *relationName, bool silent)
{
	Relation			rel;
	Oid					myrelid;
	HeapTuple			reltup;
	HeapTupleData		classtuple;
	TupleDesc			tupdesc;
	Relation			class_rel;
	Buffer				buffer;
	Relation			ridescs[Num_pg_class_indices];
	Oid					toast_relid;
	Oid					toast_idxid;
	char				toast_relname[NAMEDATALEN + 1];
	char				toast_idxname[NAMEDATALEN + 1];
	Relation			toast_idxrel;
	IndexInfo		   *indexInfo;
	Oid					classObjectId[1];

	/*
	 * permissions checking.  XXX exactly what is appropriate here?
	 */
#ifndef NO_SECURITY
	if (!pg_ownercheck(GetUserId(), relationName, RELNAME))
		elog(ERROR, "ALTER TABLE: permission denied");
#endif

	/*
	 * Grab an exclusive lock on the target table, which we will NOT
	 * release until end of transaction.
	 */
	rel = heap_openr(relationName, AccessExclusiveLock);

	if (rel->rd_rel->relkind != RELKIND_RELATION)
		elog(ERROR, "ALTER TABLE: relation \"%s\" is not a table",
			 relationName);

	myrelid = RelationGetRelid(rel);

	/*
	 * lock the pg_class tuple for update (is that really needed?)
	 */
	class_rel = heap_openr(RelationRelationName, RowExclusiveLock);

	reltup = SearchSysCache(RELNAME,
							PointerGetDatum(relationName),
							0, 0, 0);
	if (!HeapTupleIsValid(reltup))
		elog(ERROR, "ALTER TABLE: relation \"%s\" not found",
			 relationName);
	classtuple.t_self = reltup->t_self;
	ReleaseSysCache(reltup);

	switch (heap_mark4update(class_rel, &classtuple, &buffer))
	{
		case HeapTupleSelfUpdated:
		case HeapTupleMayBeUpdated:
			break;
		default:
			elog(ERROR, "couldn't lock pg_class tuple");
	}
	reltup = heap_copytuple(&classtuple);
	ReleaseBuffer(buffer);

	/*
	 * Is it already toasted?
	 */
	if (((Form_pg_class) GETSTRUCT(reltup))->reltoastrelid != InvalidOid)
	{
		if (silent)
		{
			heap_close(rel, NoLock);
			heap_close(class_rel, NoLock);
			heap_freetuple(reltup);
			return;
		}

		elog(ERROR, "ALTER TABLE: relation \"%s\" already has a toast table",
			 relationName);
	}

	/*
	 * Check to see whether the table actually needs a TOAST table.
	 */
	if (! needs_toast_table(rel))
	{
		if (silent)
		{
			heap_close(rel, NoLock);
			heap_close(class_rel, NoLock);
			heap_freetuple(reltup);
			return;
		}

		elog(ERROR, "ALTER TABLE: relation \"%s\" does not need a toast table",
			 relationName);
	}

	/*
	 * Create the toast table and its index
	 */
	sprintf(toast_relname, "pg_toast_%u", myrelid);
	sprintf(toast_idxname, "pg_toast_%u_idx", myrelid);

	/* this is pretty painful...  need a tuple descriptor */
	tupdesc = CreateTemplateTupleDesc(3);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1,
					   "chunk_id",
					   OIDOID,
					   -1, 0, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2,
					   "chunk_seq",
					   INT4OID,
					   -1, 0, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3,
					   "chunk_data",
					   BYTEAOID,
					   -1, 0, false);
	/*
	 * Ensure that the toast table doesn't itself get toasted,
	 * or we'll be toast :-(.  This is essential for chunk_data because
	 * type bytea is toastable; hit the other two just to be sure.
	 */
	tupdesc->attrs[0]->attstorage = 'p';
	tupdesc->attrs[1]->attstorage = 'p';
	tupdesc->attrs[2]->attstorage = 'p';

	/*
	 * Note: the toast relation is considered a "normal" relation even if
	 * its master relation is a temp table.  There cannot be any naming
	 * collision, and the toast rel will be destroyed when its master is,
	 * so there's no need to handle the toast rel as temp.
	 */
	toast_relid = heap_create_with_catalog(toast_relname, tupdesc,
										   RELKIND_TOASTVALUE,
										   false, true);

	/* make the toast relation visible, else index creation will fail */
	CommandCounterIncrement();

	/* create index on chunk_id */

	indexInfo = makeNode(IndexInfo);
	indexInfo->ii_NumIndexAttrs = 1;
	indexInfo->ii_NumKeyAttrs = 1;
	indexInfo->ii_KeyAttrNumbers[0] = 1;
	indexInfo->ii_Predicate = NULL;
	indexInfo->ii_FuncOid = InvalidOid;
	indexInfo->ii_Unique = false;

	classObjectId[0] = OID_OPS_OID;

	index_create(toast_relname, toast_idxname, indexInfo,
				 BTREE_AM_OID, classObjectId,
				 false, false, true);

	/*
	 * Update toast rel's pg_class entry to show that it has an index.
	 */
	setRelhasindex(toast_relid, true);

	/*
	 * Make index visible
	 */
	CommandCounterIncrement();

	/*
	 * Get the OID of the newly created index
	 */
	toast_idxrel = index_openr(toast_idxname);
	toast_idxid = RelationGetRelid(toast_idxrel);
	index_close(toast_idxrel);

	/*
	 * Store the toast table- and index-Oid's in the relation tuple
	 */
	((Form_pg_class) GETSTRUCT(reltup))->reltoastrelid = toast_relid;
	((Form_pg_class) GETSTRUCT(reltup))->reltoastidxid = toast_idxid;
	heap_update(class_rel, &reltup->t_self, reltup, NULL);

	/*
	 * Keep catalog indices current
	 */
	CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, ridescs);
	CatalogIndexInsert(ridescs, Num_pg_class_indices, class_rel, reltup);
	CatalogCloseIndices(Num_pg_class_indices, ridescs);

	heap_freetuple(reltup);

	/*
	 * Close relations and make changes visible
	 */
	heap_close(class_rel, NoLock);
	heap_close(rel, NoLock);

	CommandCounterIncrement();
}

/*
 * Check to see whether the table needs a TOAST table.  It does only if
 * (1) there are any toastable attributes, and (2) the maximum length
 * of a tuple could exceed TOAST_TUPLE_THRESHOLD.  (We don't want to
 * create a toast table for something like "f1 varchar(20)".)
 */
static bool
needs_toast_table(Relation rel)
{
	int32		data_length = 0;
	bool		maxlength_unknown = false;
	bool		has_toastable_attrs = false;
	TupleDesc	tupdesc;
	Form_pg_attribute  *att;
	int32		tuple_length;
	int			i;

	tupdesc = rel->rd_att;
	att = tupdesc->attrs;

	for (i = 0; i < tupdesc->natts; i++)
	{
		data_length = att_align(data_length, att[i]->attlen, att[i]->attalign);
		if (att[i]->attlen >= 0)
		{
			/* Fixed-length types are never toastable */
			data_length += att[i]->attlen;
		}
		else
		{
			int32	maxlen = type_maximum_size(att[i]->atttypid,
											   att[i]->atttypmod);

			if (maxlen < 0)
				maxlength_unknown = true;
			else
				data_length += maxlen;
			if (att[i]->attstorage != 'p')
				has_toastable_attrs = true;
		}
	}
	if (!has_toastable_attrs)
		return false;			/* nothing to toast? */
	if (maxlength_unknown)
		return true;			/* any unlimited-length attrs? */
	tuple_length = MAXALIGN(offsetof(HeapTupleHeaderData, t_bits) +
							BITMAPLEN(tupdesc->natts)) +
		MAXALIGN(data_length);
	return (tuple_length > TOAST_TUPLE_THRESHOLD);
}


/*
 *
 * LOCK TABLE
 *
 */
void
LockTableCommand(LockStmt *lockstmt)
{
	Relation	rel;
	int			aclresult;

	rel = heap_openr(lockstmt->relname, NoLock);

	if (rel->rd_rel->relkind != RELKIND_RELATION)
			elog(ERROR, "LOCK TABLE: %s is not a table", lockstmt->relname);

	if (lockstmt->mode == AccessShareLock)
		aclresult = pg_aclcheck(lockstmt->relname, GetUserId(), ACL_RD);
	else
		aclresult = pg_aclcheck(lockstmt->relname, GetUserId(), ACL_WR);

	if (aclresult != ACLCHECK_OK)
		elog(ERROR, "LOCK TABLE: permission denied");

	LockRelation(rel, lockstmt->mode);

	heap_close(rel, NoLock);	/* close rel, keep lock */
}


static bool
is_relation(char *name)
{
	Relation rel = heap_openr(name, NoLock);

	bool retval = (rel->rd_rel->relkind == RELKIND_RELATION);

	heap_close(rel, NoLock);

	return retval;
}
