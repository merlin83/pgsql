/*-------------------------------------------------------------------------
 *
 * command.c
 *	  random postgres portal and utility support code
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/Attic/command.c,v 1.63 2000-01-16 20:04:55 petere Exp $
 *
 * NOTES
 *	  The PortalExecutorHeapMemory crap needs to be eliminated
 *	  by designing a better executor / portal processing memory
 *	  interface.
 *
 *	  The PerformAddAttribute() code, like most of the relation
 *	  manipulating code in the commands/ directory, should go
 *	  someplace closer to the lib/catalog code.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "access/skey.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_attrdef.h"
#include "catalog/pg_type.h"
#include "commands/command.h"
#include "executor/execdefs.h"
#include "executor/executor.h"
#include "catalog/heap.h"
#include "miscadmin.h"
#include "optimizer/prep.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/syscache.h"
#include "utils/temprel.h"


/* ----------------
 *		PortalExecutorHeapMemory stuff
 *
 *		This is where the XXXSuperDuperHacky code was. -cim 3/15/90
 * ----------------
 */
MemoryContext PortalExecutorHeapMemory = NULL;

/* --------------------------------
 *		PortalCleanup
 * --------------------------------
 */
void
PortalCleanup(Portal portal)
{
	MemoryContext context;

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
	context = MemoryContextSwitchTo((MemoryContext) PortalGetHeapMemory(portal));
	PortalExecutorHeapMemory = (MemoryContext) PortalGetHeapMemory(portal);

	/* ----------------
	 *	tell the executor to shutdown the query
	 * ----------------
	 */
	ExecutorEnd(PortalGetQueryDesc(portal), PortalGetState(portal));

	/* ----------------
	 *	switch back to previous context
	 * ----------------
	 */
	MemoryContextSwitchTo(context);
	PortalExecutorHeapMemory = (MemoryContext) NULL;
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
	MemoryContext context;
	Const		limcount;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	if (name == NULL)
	{
		elog(NOTICE, "PerformPortalFetch: blank portal unsupported");
		return;
	}

	/* ----------------
	 *	Create a const node from the given count value
	 * ----------------
	 */
	memset(&limcount, 0, sizeof(limcount));
	limcount.type = T_Const;
	limcount.consttype = INT4OID;
	limcount.constlen = sizeof(int4);
	limcount.constvalue = (Datum) count;
	limcount.constisnull = FALSE;
	limcount.constbyval = TRUE;
	limcount.constisset = FALSE;
	limcount.constiscast = FALSE;


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
	context = MemoryContextSwitchTo((MemoryContext) PortalGetHeapMemory(portal));

	AssertState(context == (MemoryContext) PortalGetHeapMemory(GetPortalByName(NULL)));

	/* ----------------
	 *	setup "feature" to tell the executor what direction and
	 *	how many tuples to fetch.
	 * ----------------
	 */
	if (forward)
		feature = EXEC_FOR;
	else
		feature = EXEC_BACK;

	/* ----------------
	 *	tell the destination to prepare to recieve some tuples
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
				 portal->attinfo,		/* QueryDescGetTypeInfo(queryDesc),
										 * */
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
	PortalExecutorHeapMemory = (MemoryContext) PortalGetHeapMemory(portal);

	ExecutorRun(queryDesc, PortalGetState(portal), feature,
				(Node *) NULL, (Node *) &limcount);

	if (dest == None)			/* MOVE */
		pfree(queryDesc);

	/* ----------------
	 * Note: the "end-of-command" tag is returned by higher-level
	 *		 utility code
	 *
	 * Return blank portal for now.
	 * Otherwise, this named portal will be cleaned.
	 * Note: portals will only be supported within a BEGIN...END
	 * block in the near future.  Later, someone will fix it to
	 * do what is possible across transaction boundries.
	 * ----------------
	 */
	MemoryContextSwitchTo(
			 (MemoryContext) PortalGetHeapMemory(GetPortalByName(NULL)));
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
		elog(NOTICE, "PerformPortalClose: blank portal unsupported");
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
 *      AlterTableAddColumn
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
	HeapTuple	attributeTuple;
	Form_pg_attribute attribute;
	FormData_pg_attribute attributeD;
	int			i;
	int			minattnum,
				maxatts;
	HeapTuple	tup;
	Relation	idescs[Num_pg_attr_indices];
	Relation	ridescs[Num_pg_class_indices];
	bool		hasindex;
    List       *rawDefaults = NIL;

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
	if (!pg_ownercheck(UserName, relationName, RELNAME))
		elog(ERROR, "ALTER TABLE: permission denied");
#endif

	/*
	 * Grab an exclusive lock on the target table, which we will NOT release
	 * until end of transaction.
	 */
	rel = heap_openr((char *)relationName, AccessExclusiveLock);
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
				Oid		childrelid = lfirsti(child);

				if (childrelid == myrelid)
					continue;
				rel = heap_open(childrelid, AccessExclusiveLock);
				AlterTableAddColumn(RelationGetRelationName(rel),
									false, colDef);
				heap_close(rel, AccessExclusiveLock);
			}
		}
	}

	rel = heap_openr(RelationRelationName, RowExclusiveLock);

	reltup = SearchSysCacheTupleCopy(RELNAME,
									 PointerGetDatum(relationName),
									 0, 0, 0);

	if (!HeapTupleIsValid(reltup))
		elog(ERROR, "ALTER TABLE: relation \"%s\" not found",
			 relationName);

	/*
	 * XXX is the following check sufficient?
	 */
	if (((Form_pg_class) GETSTRUCT(reltup))->relkind == RELKIND_INDEX)
	{
		elog(ERROR, "ALTER TABLE: index relation \"%s\" not changed",
			 relationName);
	}

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

		tup = SearchSysCacheTuple(ATTNAME,
								  ObjectIdGetDatum(reltup->t_data->t_oid),
								  PointerGetDatum(colDef->colname),
								  0, 0);

		if (HeapTupleIsValid(tup))
			elog(ERROR, "ALTER TABLE: column name \"%s\" already exists in relation \"%s\"",
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

		typeTuple = SearchSysCacheTuple(TYPENAME,
										PointerGetDatum(typename),
										0, 0, 0);
		tform = (Form_pg_type) GETSTRUCT(typeTuple);

		if (!HeapTupleIsValid(typeTuple))
			elog(ERROR, "ALTER TABLE: type \"%s\" does not exist", typename);
		namestrcpy(&(attribute->attname), colDef->colname);
		attribute->atttypid = typeTuple->t_data->t_oid;
		attribute->attlen = tform->typlen;
		attribute->attdisbursion = 0;
		attribute->attcacheoff = -1;
		attribute->atttypmod = colDef->typename->typmod;
		attribute->attnum = i;
		attribute->attbyval = tform->typbyval;
		attribute->attnelems = attnelems;
		attribute->attisset = (bool) (tform->typtype == 'c');
		attribute->attstorage = 'p';
		attribute->attalign = tform->typalign;
		attribute->attnotnull = false;
		attribute->atthasdef = (colDef->raw_default != NULL ||
								colDef->cooked_default != NULL);

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

	((Form_pg_class) GETSTRUCT(reltup))->relnatts = maxatts;
	heap_update(rel, &reltup->t_self, reltup, NULL);

	/* keep catalog indices current */
	CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, ridescs);
	CatalogIndexInsert(ridescs, Num_pg_class_indices, rel, reltup);
	CatalogCloseIndices(Num_pg_class_indices, ridescs);

	heap_freetuple(reltup);

	heap_close(rel, NoLock);
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
    Relation rel;
    HeapTuple tuple;
    int16 attnum;
    Oid myrelid;

	if (!allowSystemTableMods && IsSystemRelationName(relationName))
		elog(ERROR, "ALTER TABLE: relation \"%s\" is a system catalog",
			 relationName);
#ifndef NO_SECURITY
	if (!pg_ownercheck(UserName, relationName, RELNAME))
		elog(ERROR, "ALTER TABLE: permission denied");
#endif

    /* XXX should heap_openr take const char * ? */
    rel = heap_openr((char *)relationName, AccessExclusiveLock);
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
         * inheritance hierarchy, so all we have to do is process all
         * of the relids in the list that it returns.
         */
        foreach(child, children)
		{
            Oid		childrelid = lfirsti(child);

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
    rel = heap_openr((char *)relationName, AccessExclusiveLock);

    /*
     * get the number of the attribute
     */
    tuple = SearchSysCacheTuple(ATTNAME,
                                ObjectIdGetDatum(myrelid),
                                NameGetDatum(namein((char *)colName)),
                                0, 0);

    if (!HeapTupleIsValid(tuple))
    {
        heap_close(rel, AccessExclusiveLock);
        elog(ERROR, "ALTER TABLE: relation \"%s\" has no column \"%s\"",
             relationName, colName);
    }

    attnum = ((Form_pg_attribute) GETSTRUCT(tuple))->attnum;

    if (newDefault) /* SET DEFAULT */
    {
        List* rawDefaults = NIL;
   		RawColumnDefault *rawEnt;

        /* Get rid of the old one first */
        drop_default(myrelid, attnum);

		rawEnt = (RawColumnDefault *) palloc(sizeof(RawColumnDefault));
		rawEnt->attnum = attnum;
        rawEnt->raw_default = newDefault;
		rawDefaults = lappend(rawDefaults, rawEnt);

        /*
         * This function is intended for CREATE TABLE,
         * so it processes a _list_ of defaults, but we just do one.
         */
        AddRelationRawConstraints(rel, rawDefaults, NIL);
    }

    else /* DROP DEFAULT */
    {
        Relation attr_rel;
        ScanKeyData scankeys[3];
        HeapScanDesc scan;
        HeapTuple tuple;

        attr_rel = heap_openr(AttributeRelationName, AccessExclusiveLock);
        ScanKeyEntryInitialize(&scankeys[0], 0x0, Anum_pg_attribute_attrelid, F_OIDEQ,
                               ObjectIdGetDatum(myrelid));
        ScanKeyEntryInitialize(&scankeys[1], 0x0, Anum_pg_attribute_attnum, F_INT2EQ,
                               Int16GetDatum(attnum));
        ScanKeyEntryInitialize(&scankeys[2], 0x0, Anum_pg_attribute_atthasdef, F_BOOLEQ,
                               TRUE);

        scan = heap_beginscan(attr_rel, false, SnapshotNow, 3, scankeys);
        AssertState(scan!=NULL);

        if (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
        {
            HeapTuple newtuple;
            Relation	irelations[Num_pg_attr_indices];

            /* update to false */
            newtuple = heap_copytuple(tuple);
            ((Form_pg_attribute) GETSTRUCT(newtuple))->atthasdef = FALSE;
            heap_update(attr_rel, &tuple->t_self, newtuple, NULL);

            /* keep the system catalog indices current */
            CatalogOpenIndices(Num_pg_attr_indices, Name_pg_attr_indices, irelations);
            CatalogIndexInsert(irelations, Num_pg_attr_indices, attr_rel, newtuple);
            CatalogCloseIndices(Num_pg_class_indices, irelations);

            /* get rid of actual default definition */
            drop_default(myrelid, attnum);
        }
        else
            elog(NOTICE, "ALTER TABLE: there was no default on column \"%s\" of relation \"%s\"",
                 colName, relationName);
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
    Relation attrdef_rel;
    HeapTuple tuple;

    attrdef_rel = heap_openr(AttrDefaultRelationName, AccessExclusiveLock);
    ScanKeyEntryInitialize(&scankeys[0], 0x0, Anum_pg_attrdef_adrelid, F_OIDEQ,
                           ObjectIdGetDatum(relid));
    ScanKeyEntryInitialize(&scankeys[1], 0x0, Anum_pg_attrdef_adnum, F_INT2EQ,
                           Int16GetDatum(attnum));

    scan = heap_beginscan(attrdef_rel, false, SnapshotNow, 2, scankeys);
    AssertState(scan!=NULL);

    if (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
        heap_delete(attrdef_rel, &tuple->t_self, NULL);

    heap_endscan(scan);

    heap_close(attrdef_rel, NoLock);
}



/*
 * ALTER TABLE DROP COLUMN
 */
void
AlterTableDropColumn(const char *relationName,
                     bool inh, const char *colName,
                     int behavior)
{
    elog(NOTICE, "ALTER TABLE / DROP COLUMN is not implemented");
}



void
AlterTableAddConstraint(const char *relationName,
                        bool inh, Node *newConstraint)
{
    elog(NOTICE, "ALTER TABLE / ADD CONSTRAINT is not implemented");
}



void AlterTableDropConstraint(const char *relationName,
                              bool inh, const char *constrName,
                              int behavior)
{
    elog(NOTICE, "ALTER TABLE / DROP CONSTRAINT is not implemented");
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
	if (! RelationIsValid(rel))
		elog(ERROR, "Relation '%s' does not exist", lockstmt->relname);

	if (lockstmt->mode == AccessShareLock)
		aclresult = pg_aclcheck(lockstmt->relname, GetPgUserName(), ACL_RD);
	else
		aclresult = pg_aclcheck(lockstmt->relname, GetPgUserName(), ACL_WR);

	if (aclresult != ACLCHECK_OK)
		elog(ERROR, "LOCK TABLE: permission denied");

	LockRelation(rel, lockstmt->mode);

	heap_close(rel, NoLock);	/* close rel, keep lock */
}
