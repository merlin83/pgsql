/*-------------------------------------------------------------------------
 *
 * indexnode.c
 *	  Routines to find all indices on a relation
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/Attic/indexnode.c,v 1.15 1999-05-25 16:09:53 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>

#include "postgres.h"

#include "nodes/plannodes.h"
#include "nodes/parsenodes.h"
#include "nodes/relation.h"

#include "optimizer/internal.h"
#include "optimizer/plancat.h"
#include "optimizer/pathnode.h" /* where the decls go */


static List *find_secondary_index(Query *root, Oid relid);

/*
 * find_relation_indices
 *	  Returns a list of index nodes containing appropriate information for
 *	  each (secondary) index defined on a relation.
 *
 */
List *
find_relation_indices(Query *root, RelOptInfo * rel)
{
	if (rel->indexed)
		return find_secondary_index(root, lfirsti(rel->relids));
	else
		return NIL;
}

/*
 * find_secondary_index
 *	  Creates a list of index path nodes containing information for each
 *	  secondary index defined on a relation by searching through the index
 *	  catalog.
 *
 * 'relid' is the OID of the relation for which indices are being located
 *
 * Returns a list of new index nodes.
 *
 */
static List *
find_secondary_index(Query *root, Oid relid)
{
	IdxInfoRetval indexinfo;
	List	   *indexes = NIL;
	bool		first = TRUE;

	while (index_info(root, first, relid, &indexinfo))
	{
		RelOptInfo *indexnode = makeNode(RelOptInfo);

		indexnode->relids = lconsi(indexinfo.relid, NIL);
		indexnode->relam = indexinfo.relam;
		indexnode->pages = indexinfo.pages;
		indexnode->tuples = indexinfo.tuples;
		indexnode->indexkeys = indexinfo.indexkeys;
		indexnode->ordering = indexinfo.orderOprs;
		indexnode->classlist = indexinfo.classlist;
		indexnode->indproc = indexinfo.indproc;
		indexnode->indpred = (List *) indexinfo.indpred;

		indexnode->indexed = false;		/* not indexed itself */
		indexnode->size = 0;
		indexnode->width = 0;
		indexnode->targetlist = NIL;
		indexnode->pathlist = NIL;
		indexnode->cheapestpath = NULL;
		indexnode->pruneable = true;
		indexnode->restrictinfo = NIL;
		indexnode->joininfo = NIL;
		indexnode->innerjoin = NIL;

		indexes = lcons(indexnode, indexes);
		first = FALSE;
	}

	return indexes;
}
