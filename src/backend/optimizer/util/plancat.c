/*-------------------------------------------------------------------------
 *
 * plancat.c
 *	   routines for accessing the system catalogs
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/plancat.c,v 1.52 2000-05-30 00:49:49 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <math.h>

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_amop.h"
#include "catalog/pg_inherits.h"
#include "optimizer/plancat.h"
#include "parser/parsetree.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/syscache.h"
#include "catalog/catalog.h"
#include "miscadmin.h"


/*
 * relation_info -
 *	  Retrieves catalog information for a given relation.
 *	  Given the rangetable index of the relation, return the following info:
 *				whether the relation has secondary indices
 *				number of pages
 *				number of tuples
 */
void
relation_info(Query *root, Index relid,
			  bool *hasindex, long *pages, double *tuples)
{
	Oid			relationObjectId = getrelid(relid, root->rtable);
	HeapTuple	relationTuple;
	Form_pg_class relation;

	relationTuple = SearchSysCacheTuple(RELOID,
									  ObjectIdGetDatum(relationObjectId),
										0, 0, 0);
	if (!HeapTupleIsValid(relationTuple))
		elog(ERROR, "relation_info: Relation %u not found",
			 relationObjectId);
	relation = (Form_pg_class) GETSTRUCT(relationTuple);

	if (IsIgnoringSystemIndexes() && IsSystemRelationName(NameStr(relation->relname)))
		*hasindex = false;
	else
		*hasindex = (relation->relhasindex) ? true : false;
	*pages = relation->relpages;
	*tuples = relation->reltuples;
}

/*
 * find_secondary_indexes
 *	  Creates a list of IndexOptInfo nodes containing information for each
 *	  secondary index defined on the given relation.
 *
 * 'relid' is the RT index of the relation for which indices are being located
 *
 * Returns a list of new IndexOptInfo nodes.
 */
List *
find_secondary_indexes(Query *root, Index relid)
{
	List	   *indexes = NIL;
	Oid			indrelid = getrelid(relid, root->rtable);
	Relation	relation;
	HeapScanDesc scan;
	ScanKeyData indexKey;
	HeapTuple	indexTuple;

	/* Scan pg_index for tuples describing indexes of this rel */
	relation = heap_openr(IndexRelationName, AccessShareLock);

	ScanKeyEntryInitialize(&indexKey, 0,
						   Anum_pg_index_indrelid,
						   F_OIDEQ,
						   ObjectIdGetDatum(indrelid));

	scan = heap_beginscan(relation, 0, SnapshotNow,
						  1, &indexKey);

	while (HeapTupleIsValid(indexTuple = heap_getnext(scan, 0)))
	{
		Form_pg_index index = (Form_pg_index) GETSTRUCT(indexTuple);
		IndexOptInfo *info = makeNode(IndexOptInfo);
		int			i;
		Relation	indexRelation;
		Oid			relam;
		uint16		amorderstrategy;

		/*
		 * Need to make these arrays large enough to be sure there is a
		 * terminating 0 at the end of each one.
		 */
		info->classlist = (Oid *) palloc(sizeof(Oid) * (INDEX_MAX_KEYS + 1));
		info->indexkeys = (int *) palloc(sizeof(int) * (INDEX_MAX_KEYS + 1));
		info->ordering = (Oid *) palloc(sizeof(Oid) * (INDEX_MAX_KEYS + 1));

		/* Extract info from the pg_index tuple */
		info->indexoid = index->indexrelid;
		info->indproc = index->indproc; /* functional index ?? */
		if (VARSIZE(&index->indpred) != 0)		/* partial index ?? */
		{
			char	   *predString = textout(&index->indpred);

			info->indpred = (List *) stringToNode(predString);
			pfree(predString);
		}
		else
			info->indpred = NIL;
		info->lossy = index->indislossy;

		for (i = 0; i < INDEX_MAX_KEYS; i++)
			info->indexkeys[i] = index->indkey[i];
		info->indexkeys[INDEX_MAX_KEYS] = 0;
		for (i = 0; i < INDEX_MAX_KEYS; i++)
			info->classlist[i] = index->indclass[i];
		info->classlist[INDEX_MAX_KEYS] = (Oid) 0;

		/* Extract info from the relation descriptor for the index */
		indexRelation = index_open(index->indexrelid);
		relam = indexRelation->rd_rel->relam;
		info->relam = relam;
		info->pages = indexRelation->rd_rel->relpages;
		info->tuples = indexRelation->rd_rel->reltuples;
		info->amcostestimate = index_cost_estimator(indexRelation);
		amorderstrategy = indexRelation->rd_am->amorderstrategy;
		index_close(indexRelation);

		/*
		 * Fetch the ordering operators associated with the index, if any.
		 */
		MemSet(info->ordering, 0, sizeof(Oid) * (INDEX_MAX_KEYS + 1));
		if (amorderstrategy != 0)
		{
			for (i = 0; i < INDEX_MAX_KEYS && index->indclass[i]; i++)
			{
				HeapTuple	amopTuple;
				Form_pg_amop amop;

				amopTuple =
					SearchSysCacheTuple(AMOPSTRATEGY,
										ObjectIdGetDatum(relam),
									ObjectIdGetDatum(index->indclass[i]),
										UInt16GetDatum(amorderstrategy),
										0);
				if (!HeapTupleIsValid(amopTuple))
					elog(ERROR, "find_secondary_indexes: no amop %u %u %d",
					   relam, index->indclass[i], (int) amorderstrategy);
				amop = (Form_pg_amop) GETSTRUCT(amopTuple);
				info->ordering[i] = amop->amopopr;
			}
		}

		indexes = lcons(info, indexes);
	}

	heap_endscan(scan);
	heap_close(relation, AccessShareLock);

	return indexes;
}

/*
 * restriction_selectivity
 *
 * Returns the selectivity of a specified operator.
 * This code executes registered procedures stored in the
 * operator relation, by calling the function manager.
 *
 * XXX The assumption in the selectivity procedures is that if the
 *		relation OIDs or attribute numbers are 0, then the clause
 *		isn't of the form (op var const).
 */
Selectivity
restriction_selectivity(Oid functionObjectId,
						Oid operatorObjectId,
						Oid relationObjectId,
						AttrNumber attributeNumber,
						Datum constValue,
						int constFlag)
{
	float64		result;

	result = (float64) fmgr(functionObjectId,
							(char *) operatorObjectId,
							(char *) relationObjectId,
							(char *) (int) attributeNumber,
							(char *) constValue,
							(char *) constFlag,
							NULL);
	if (!PointerIsValid(result))
		elog(ERROR, "restriction_selectivity: bad pointer");

	if (*result < 0.0 || *result > 1.0)
		elog(ERROR, "restriction_selectivity: bad value %f", *result);

	return (Selectivity) *result;
}

/*
 * join_selectivity
 *
 * Returns the selectivity of an operator, given the join clause
 * information.
 *
 * XXX The assumption in the selectivity procedures is that if the
 *		relation OIDs or attribute numbers are 0, then the clause
 *		isn't of the form (op var var).
 */
Selectivity
join_selectivity(Oid functionObjectId,
				 Oid operatorObjectId,
				 Oid relationObjectId1,
				 AttrNumber attributeNumber1,
				 Oid relationObjectId2,
				 AttrNumber attributeNumber2)
{
	float64		result;

	result = (float64) fmgr(functionObjectId,
							(char *) operatorObjectId,
							(char *) relationObjectId1,
							(char *) (int) attributeNumber1,
							(char *) relationObjectId2,
							(char *) (int) attributeNumber2,
							NULL);
	if (!PointerIsValid(result))
		elog(ERROR, "join_selectivity: bad pointer");

	if (*result < 0.0 || *result > 1.0)
		elog(ERROR, "join_selectivity: bad value %f", *result);

	return (Selectivity) *result;
}

/*
 * find_inheritance_children
 *
 * Returns an integer list containing the OIDs of all relations which
 * inherit *directly* from the relation with OID 'inhparent'.
 */
List *
find_inheritance_children(Oid inhparent)
{
	static ScanKeyData key[1] = {
		{0, Anum_pg_inherits_inhparent, F_OIDEQ}
	};

	HeapTuple	inheritsTuple;
	Relation	relation;
	HeapScanDesc scan;
	List	   *list = NIL;
	Oid			inhrelid;

	fmgr_info(F_OIDEQ, &key[0].sk_func);
	key[0].sk_nargs = key[0].sk_func.fn_nargs;
	key[0].sk_argument = ObjectIdGetDatum(inhparent);

	relation = heap_openr(InheritsRelationName, AccessShareLock);
	scan = heap_beginscan(relation, 0, SnapshotNow, 1, key);
	while (HeapTupleIsValid(inheritsTuple = heap_getnext(scan, 0)))
	{
		inhrelid = ((Form_pg_inherits) GETSTRUCT(inheritsTuple))->inhrelid;
		list = lappendi(list, inhrelid);
	}
	heap_endscan(scan);
	heap_close(relation, AccessShareLock);
	return list;
}

#ifdef NOT_USED
/*
 * VersionGetParents
 *
 * Returns a LISP list containing the OIDs of all relations which are
 * base relations of the relation with OID 'verrelid'.
 */
List *
VersionGetParents(Oid verrelid)
{
	static ScanKeyData key[1] = {
		{0, Anum_pg_version_verrelid, F_OIDEQ}
	};

	HeapTuple	versionTuple;
	Relation	relation;
	HeapScanDesc scan;
	Oid			verbaseid;
	List	   *list = NIL;

	fmgr_info(F_OIDEQ, &key[0].sk_func);
	key[0].sk_nargs = key[0].sk_func.fn_nargs;
	key[0].sk_argument = ObjectIdGetDatum(verrelid);
	relation = heap_openr(VersionRelationName, AccessShareLock);
	scan = heap_beginscan(relation, 0, SnapshotNow, 1, key);
	while (HeapTupleIsValid(versionTuple = heap_getnext(scan, 0)))
	{
		verbaseid = ((Form_pg_version)
					 GETSTRUCT(versionTuple))->verbaseid;

		list = lconsi(verbaseid, list);

		key[0].sk_argument = ObjectIdGetDatum(verbaseid);
		heap_rescan(scan, 0, key);
	}
	heap_endscan(scan);
	heap_close(relation, AccessShareLock);
	return list;
}

#endif
