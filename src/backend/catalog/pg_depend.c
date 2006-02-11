/*-------------------------------------------------------------------------
 *
 * pg_depend.c
 *	  routines to support manipulation of the pg_depend relation
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/catalog/pg_depend.c,v 1.18 2006/02/11 22:17:18 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/indexing.h"
#include "catalog/dependency.h"
#include "catalog/pg_depend.h"
#include "miscadmin.h"
#include "utils/fmgroids.h"


static bool isObjectPinned(const ObjectAddress *object, Relation rel);


/*
 * Record a dependency between 2 objects via their respective objectAddress.
 * The first argument is the dependent object, the second the one it
 * references.
 *
 * This simply creates an entry in pg_depend, without any other processing.
 */
void
recordDependencyOn(const ObjectAddress *depender,
				   const ObjectAddress *referenced,
				   DependencyType behavior)
{
	recordMultipleDependencies(depender, referenced, 1, behavior);
}

/*
 * Record multiple dependencies (of the same kind) for a single dependent
 * object.	This has a little less overhead than recording each separately.
 */
void
recordMultipleDependencies(const ObjectAddress *depender,
						   const ObjectAddress *referenced,
						   int nreferenced,
						   DependencyType behavior)
{
	Relation	dependDesc;
	CatalogIndexState indstate;
	HeapTuple	tup;
	int			i;
	char		nulls[Natts_pg_depend];
	Datum		values[Natts_pg_depend];

	if (nreferenced <= 0)
		return;					/* nothing to do */

	/*
	 * During bootstrap, do nothing since pg_depend may not exist yet. initdb
	 * will fill in appropriate pg_depend entries after bootstrap.
	 */
	if (IsBootstrapProcessingMode())
		return;

	dependDesc = heap_open(DependRelationId, RowExclusiveLock);

	/* Don't open indexes unless we need to make an update */
	indstate = NULL;

	memset(nulls, ' ', sizeof(nulls));

	for (i = 0; i < nreferenced; i++, referenced++)
	{
		/*
		 * If the referenced object is pinned by the system, there's no real
		 * need to record dependencies on it.  This saves lots of space in
		 * pg_depend, so it's worth the time taken to check.
		 */
		if (!isObjectPinned(referenced, dependDesc))
		{
			/*
			 * Record the Dependency.  Note we don't bother to check for
			 * duplicate dependencies; there's no harm in them.
			 */
			values[Anum_pg_depend_classid - 1] = ObjectIdGetDatum(depender->classId);
			values[Anum_pg_depend_objid - 1] = ObjectIdGetDatum(depender->objectId);
			values[Anum_pg_depend_objsubid - 1] = Int32GetDatum(depender->objectSubId);

			values[Anum_pg_depend_refclassid - 1] = ObjectIdGetDatum(referenced->classId);
			values[Anum_pg_depend_refobjid - 1] = ObjectIdGetDatum(referenced->objectId);
			values[Anum_pg_depend_refobjsubid - 1] = Int32GetDatum(referenced->objectSubId);

			values[Anum_pg_depend_deptype - 1] = CharGetDatum((char) behavior);

			tup = heap_formtuple(dependDesc->rd_att, values, nulls);

			simple_heap_insert(dependDesc, tup);

			/* keep indexes current */
			if (indstate == NULL)
				indstate = CatalogOpenIndexes(dependDesc);

			CatalogIndexInsert(indstate, tup);

			heap_freetuple(tup);
		}
	}

	if (indstate != NULL)
		CatalogCloseIndexes(indstate);

	heap_close(dependDesc, RowExclusiveLock);
}

/*
 * deleteDependencyRecordsFor -- delete all records with given depender
 * classId/objectId.  Returns the number of records deleted.
 *
 * This is used when redefining an existing object.  Links leading to the
 * object do not change, and links leading from it will be recreated
 * (possibly with some differences from before).
 */
long
deleteDependencyRecordsFor(Oid classId, Oid objectId)
{
	long		count = 0;
	Relation	depRel;
	ScanKeyData key[2];
	SysScanDesc scan;
	HeapTuple	tup;

	depRel = heap_open(DependRelationId, RowExclusiveLock);

	ScanKeyInit(&key[0],
				Anum_pg_depend_classid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(classId));
	ScanKeyInit(&key[1],
				Anum_pg_depend_objid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(objectId));

	scan = systable_beginscan(depRel, DependDependerIndexId, true,
							  SnapshotNow, 2, key);

	while (HeapTupleIsValid(tup = systable_getnext(scan)))
	{
		simple_heap_delete(depRel, &tup->t_self);
		count++;
	}

	systable_endscan(scan);

	heap_close(depRel, RowExclusiveLock);

	return count;
}

/*
 * objectIsInternalDependency -- return whether the specified object
 * is listed as an internal dependency for some other object.
 *
 * This is used to implement DROP/REASSIGN OWNED.  We cannot invoke
 * performDeletion blindly, because it may try to drop or modify an internal-
 * dependent object before the "main" object, so we need to skip the first
 * object and expect it to be automatically dropped when the main object is
 * dropped.
 */
bool
objectIsInternalDependency(Oid classId, Oid objectId)
{
	Relation	depRel;
	ScanKeyData key[2];
	SysScanDesc scan;
	HeapTuple	tup;
	bool		isdep = false;

	depRel = heap_open(DependRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_pg_depend_classid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(classId));
	ScanKeyInit(&key[1],
				Anum_pg_depend_objid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(objectId));

	scan = systable_beginscan(depRel, DependDependerIndexId, true,
							  SnapshotNow, 2, key);

	while (HeapTupleIsValid(tup = systable_getnext(scan)))
	{
		Form_pg_depend depForm = (Form_pg_depend) GETSTRUCT(tup);

		if (depForm->deptype == DEPENDENCY_INTERNAL)
		{
			/* No need to keep scanning */
			isdep = true;
			break;
		}
	}

	systable_endscan(scan);

	heap_close(depRel, AccessShareLock);

	return isdep;
}

/*
 * Adjust dependency record(s) to point to a different object of the same type
 *
 * classId/objectId specify the referencing object.
 * refClassId/oldRefObjectId specify the old referenced object.
 * newRefObjectId is the new referenced object (must be of class refClassId).
 *
 * Note the lack of objsubid parameters.  If there are subobject references
 * they will all be readjusted.
 *
 * Returns the number of records updated.
 */
long
changeDependencyFor(Oid classId, Oid objectId,
					Oid refClassId, Oid oldRefObjectId,
					Oid newRefObjectId)
{
	long		count = 0;
	Relation	depRel;
	ScanKeyData key[2];
	SysScanDesc scan;
	HeapTuple	tup;
	ObjectAddress objAddr;
	bool		newIsPinned;

	depRel = heap_open(DependRelationId, RowExclusiveLock);

	/*
	 * If oldRefObjectId is pinned, there won't be any dependency entries on
	 * it --- we can't cope in that case.  (This isn't really worth expending
	 * code to fix, in current usage; it just means you can't rename stuff out
	 * of pg_catalog, which would likely be a bad move anyway.)
	 */
	objAddr.classId = refClassId;
	objAddr.objectId = oldRefObjectId;
	objAddr.objectSubId = 0;

	if (isObjectPinned(&objAddr, depRel))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
		errmsg("cannot remove dependency on %s because it is a system object",
			   getObjectDescription(&objAddr))));

	/*
	 * We can handle adding a dependency on something pinned, though, since
	 * that just means deleting the dependency entry.
	 */
	objAddr.objectId = newRefObjectId;

	newIsPinned = isObjectPinned(&objAddr, depRel);

	/* Now search for dependency records */
	ScanKeyInit(&key[0],
				Anum_pg_depend_classid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(classId));
	ScanKeyInit(&key[1],
				Anum_pg_depend_objid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(objectId));

	scan = systable_beginscan(depRel, DependDependerIndexId, true,
							  SnapshotNow, 2, key);

	while (HeapTupleIsValid((tup = systable_getnext(scan))))
	{
		Form_pg_depend depform = (Form_pg_depend) GETSTRUCT(tup);

		if (depform->refclassid == refClassId &&
			depform->refobjid == oldRefObjectId)
		{
			if (newIsPinned)
				simple_heap_delete(depRel, &tup->t_self);
			else
			{
				/* make a modifiable copy */
				tup = heap_copytuple(tup);
				depform = (Form_pg_depend) GETSTRUCT(tup);

				depform->refobjid = newRefObjectId;

				simple_heap_update(depRel, &tup->t_self, tup);
				CatalogUpdateIndexes(depRel, tup);

				heap_freetuple(tup);
			}

			count++;
		}
	}

	systable_endscan(scan);

	heap_close(depRel, RowExclusiveLock);

	return count;
}

/*
 * isObjectPinned()
 *
 * Test if an object is required for basic database functionality.
 * Caller must already have opened pg_depend.
 *
 * The passed subId, if any, is ignored; we assume that only whole objects
 * are pinned (and that this implies pinning their components).
 */
static bool
isObjectPinned(const ObjectAddress *object, Relation rel)
{
	bool		ret = false;
	SysScanDesc scan;
	HeapTuple	tup;
	ScanKeyData key[2];

	ScanKeyInit(&key[0],
				Anum_pg_depend_refclassid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(object->classId));

	ScanKeyInit(&key[1],
				Anum_pg_depend_refobjid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(object->objectId));

	scan = systable_beginscan(rel, DependReferenceIndexId, true,
							  SnapshotNow, 2, key);

	/*
	 * Since we won't generate additional pg_depend entries for pinned
	 * objects, there can be at most one entry referencing a pinned object.
	 * Hence, it's sufficient to look at the first returned tuple; we don't
	 * need to loop.
	 */
	tup = systable_getnext(scan);
	if (HeapTupleIsValid(tup))
	{
		Form_pg_depend foundDep = (Form_pg_depend) GETSTRUCT(tup);

		if (foundDep->deptype == DEPENDENCY_PIN)
			ret = true;
	}

	systable_endscan(scan);

	return ret;
}

List* getReferencingOids(Oid refClassId, Oid refObjId, Oid refObjSubId,
						 Oid classId, DependencyType deptype)
{
	ScanKeyData		key[3];
	SysScanDesc		scan;
	HeapTuple		tup;
	Relation		depRel;
	List		   *list = NIL;

	depRel = heap_open(DependRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_pg_depend_refclassid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(refClassId));

	ScanKeyInit(&key[1],
				Anum_pg_depend_refobjid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(refObjId));

	ScanKeyInit(&key[2],
				Anum_pg_depend_refobjsubid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(refObjSubId));

	scan = systable_beginscan(depRel, DependReferenceIndexId, true,
							  SnapshotNow, 3, key);

	while (HeapTupleIsValid(tup = systable_getnext(scan)))
	{
		Form_pg_depend	depForm = (Form_pg_depend) GETSTRUCT(tup);

		/* check if the class id is what we want */
		if (depForm->classid != classId)
			continue;

		/* check if the DependencyType is what we want */
		if (depForm->deptype != deptype)
			continue;

		/* if we are still here, we have found a match */
		list = lcons_oid(depForm->objid, list);
		break;
	}
	systable_endscan(scan);

	heap_close(depRel, AccessShareLock);
	return list;
}


List* getDependentOids(Oid classId, Oid objId,
					   Oid refClassId, DependencyType deptype)
{
	ScanKeyData		key[2];
	SysScanDesc		scan;
	HeapTuple		tup;
	Relation		depRel;
	List		   *list = NIL;

	depRel = heap_open(DependRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_pg_depend_classid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(classId));

	ScanKeyInit(&key[1],
				Anum_pg_depend_objid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(objId));

	scan = systable_beginscan(depRel, DependDependerIndexId, true,
							  SnapshotNow, 2, key);

	while (HeapTupleIsValid(tup = systable_getnext(scan)))
	{
		Form_pg_depend	depForm = (Form_pg_depend) GETSTRUCT(tup);

		/* check if the DependencyType is what we want */
		if (depForm->deptype != deptype)
			continue;

		/* check if the referenced class id is what we want */
		if (depForm->refclassid != refClassId)
			continue;

		/* if we are still here, we have found a match */
		list = lcons_oid(depForm->refobjid, list);
		break;
	}
	systable_endscan(scan);

	heap_close(depRel, AccessShareLock);
	return list;
}

