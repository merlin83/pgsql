/*-------------------------------------------------------------------------
 *
 * rewriteSupport.c
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/rewrite/rewriteSupport.c,v 1.53 2002-07-12 18:43:17 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "rewrite/rewriteSupport.h"
#include "utils/inval.h"
#include "utils/syscache.h"


/*
 * Is there a rule by the given name?
 */
bool
IsDefinedRewriteRule(Oid owningRel, const char *ruleName)
{
	return SearchSysCacheExists(RULERELNAME,
								ObjectIdGetDatum(owningRel),
								PointerGetDatum(ruleName),
								0, 0);
}


/*
 * SetRelationRuleStatus
 *		Set the value of the relation's relhasrules field in pg_class;
 *		if the relation is becoming a view, also adjust its relkind.
 *
 * NOTE: caller must be holding an appropriate lock on the relation.
 *
 * NOTE: an important side-effect of this operation is that an SI invalidation
 * message is sent out to all backends --- including me --- causing relcache
 * entries to be flushed or updated with the new set of rules for the table.
 * This must happen even if we find that no change is needed in the pg_class
 * row.
 */
void
SetRelationRuleStatus(Oid relationId, bool relHasRules,
					  bool relIsBecomingView)
{
	Relation	relationRelation;
	HeapTuple	tuple;
	Form_pg_class classForm;
	Relation	idescs[Num_pg_class_indices];

	/*
	 * Find the tuple to update in pg_class, using syscache for the
	 * lookup.
	 */
	relationRelation = heap_openr(RelationRelationName, RowExclusiveLock);
	tuple = SearchSysCacheCopy(RELOID,
							   ObjectIdGetDatum(relationId),
							   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "SetRelationRuleStatus: cache lookup failed for relation %u", relationId);
	classForm = (Form_pg_class) GETSTRUCT(tuple);

	if (classForm->relhasrules != relHasRules ||
		(relIsBecomingView && classForm->relkind != RELKIND_VIEW))
	{
		/* Do the update */
		classForm->relhasrules = relHasRules;
		if (relIsBecomingView)
			classForm->relkind = RELKIND_VIEW;

		simple_heap_update(relationRelation, &tuple->t_self, tuple);

		/* Keep the catalog indices up to date */
		CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_class_indices, relationRelation, tuple);
		CatalogCloseIndices(Num_pg_class_indices, idescs);
	}
	else
	{
		/* no need to change tuple, but force relcache rebuild anyway */
		CacheInvalidateRelcache(relationId);
	}

	heap_freetuple(tuple);
	heap_close(relationRelation, RowExclusiveLock);
}
