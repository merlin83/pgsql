/*-------------------------------------------------------------------------
 *
 * cluster.c
 *	  Paul Brown's implementation of cluster index.
 *
 *	  I am going to use the rename function as a model for this in the
 *	  parser and executor, and the vacuum code as an example in this
 *	  file. As I go - in contrast to the rest of postgres - there will
 *	  be BUCKETS of comments. This is to allow reviewers to understand
 *	  my (probably bogus) assumptions about the way this works.
 *														[pbrown '94]
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994-5, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/cluster.c,v 1.57 2000-07-04 06:11:27 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/pg_index.h"
#include "catalog/pg_proc.h"
#include "commands/cluster.h"
#include "commands/rename.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/syscache.h"

static Relation copy_heap(Oid OIDOldHeap);
static void copy_index(Oid OIDOldIndex, Oid OIDNewHeap);
static void rebuildheap(Oid OIDNewHeap, Oid OIDOldHeap, Oid OIDOldIndex);

/*
 * cluster
 *
 *	 Check that the relation is a relation in the appropriate user
 *	 ACL. I will use the same security that limits users on the
 *	 renamerel() function.
 *
 *	 Check that the index specified is appropriate for the task
 *	 ( ie it's an index over this relation ). This is trickier.
 *
 *	 Create a list of all the other indicies on this relation. Because
 *	 the cluster will wreck all the tids, I'll need to destroy bogus
 *	 indicies. The user will have to re-create them. Not nice, but
 *	 I'm not a nice guy. The alternative is to try some kind of post
 *	 destroy re-build. This may be possible. I'll check out what the
 *	 index create functiond want in the way of paramaters. On the other
 *	 hand, re-creating n indicies may blow out the space.
 *
 *	 Create new (temporary) relations for the base heap and the new
 *	 index.
 *
 *	 Exclusively lock the relations.
 *
 *	 Create new clustered index and base heap relation.
 *
 */
void
cluster(char *oldrelname, char *oldindexname)
{
	Oid			OIDOldHeap,
				OIDOldIndex,
				OIDNewHeap;

	Relation	OldHeap,
				OldIndex;
	Relation	NewHeap;

	char		NewIndexName[NAMEDATALEN];
	char		NewHeapName[NAMEDATALEN];
	char		saveoldrelname[NAMEDATALEN];
	char		saveoldindexname[NAMEDATALEN];

	/*
	 * Copy the arguments into local storage, because they are probably
	 * in palloc'd storage that will go away when we commit a transaction.
	 */
	strcpy(saveoldrelname, oldrelname);
	strcpy(saveoldindexname, oldindexname);

	/*
	 * Like vacuum, cluster spans transactions, so I'm going to handle it
	 * in the same way: commit and restart transactions where needed.
	 *
	 * We grab exclusive access to the target rel and index for the duration
	 * of the initial transaction.
	 */

	OldHeap = heap_openr(saveoldrelname, AccessExclusiveLock);
	OIDOldHeap = RelationGetRelid(OldHeap);

	OldIndex = index_openr(saveoldindexname); /* Open old index relation	*/
	LockRelation(OldIndex, AccessExclusiveLock);
	OIDOldIndex = RelationGetRelid(OldIndex);

	/*
	 * XXX Should check that index is in fact an index on this relation?
	 */

	heap_close(OldHeap, NoLock);/* do NOT give up the locks */
	index_close(OldIndex);

	/*
	 * I need to build the copies of the heap and the index. The Commit()
	 * between here is *very* bogus. If someone is appending stuff, they
	 * will get the lock after being blocked and add rows which won't be
	 * present in the new table. Bleagh! I'd be best to try and ensure
	 * that no-one's in the tables for the entire duration of this process
	 * with a pg_vlock.  XXX Isn't the above comment now invalid?
	 */
	NewHeap = copy_heap(OIDOldHeap);
	OIDNewHeap = RelationGetRelid(NewHeap);
	strcpy(NewHeapName, RelationGetRelationName(NewHeap));

	/* To make the new heap visible (which is until now empty). */
	CommandCounterIncrement();

	rebuildheap(OIDNewHeap, OIDOldHeap, OIDOldIndex);

	/* To flush the filled new heap (and the statistics about it). */
	CommandCounterIncrement();

	/* Create new index over the tuples of the new heap. */
	copy_index(OIDOldIndex, OIDNewHeap);
	snprintf(NewIndexName, NAMEDATALEN, "temp_%x", OIDOldIndex);

	/*
	 * make this really happen. Flush all the buffers. (Believe me, it is
	 * necessary ... ended up in a mess without it.)
	 */
	CommitTransactionCommand();
	StartTransactionCommand();

	/* Destroy old heap (along with its index) and rename new. */
	heap_drop_with_catalog(saveoldrelname, allowSystemTableMods);

	CommitTransactionCommand();
	StartTransactionCommand();

	renamerel(NewHeapName, saveoldrelname);
	renamerel(NewIndexName, saveoldindexname);
}

static Relation
copy_heap(Oid OIDOldHeap)
{
	char		NewName[NAMEDATALEN];
	TupleDesc	OldHeapDesc,
				tupdesc;
	Oid			OIDNewHeap;
	Relation	NewHeap,
				OldHeap;

	/*
	 * Create a new heap relation with a temporary name, which has the
	 * same tuple description as the old one.
	 */
	snprintf(NewName, NAMEDATALEN, "temp_%x", OIDOldHeap);

	OldHeap = heap_open(OIDOldHeap, AccessExclusiveLock);
	OldHeapDesc = RelationGetDescr(OldHeap);

	/*
	 * Need to make a copy of the tuple descriptor,
	 * heap_create_with_catalog modifies it.
	 */

	tupdesc = CreateTupleDescCopy(OldHeapDesc);

	OIDNewHeap = heap_create_with_catalog(NewName, tupdesc,
										  RELKIND_RELATION, false,
										  allowSystemTableMods);

	if (!OidIsValid(OIDNewHeap))
		elog(ERROR, "clusterheap: cannot create temporary heap relation\n");

	/* XXX why are we bothering to do this: */
	NewHeap = heap_open(OIDNewHeap, AccessExclusiveLock);

	heap_close(NewHeap, AccessExclusiveLock);
	heap_close(OldHeap, AccessExclusiveLock);

	return NewHeap;
}

static void
copy_index(Oid OIDOldIndex, Oid OIDNewHeap)
{
	Relation	OldIndex,
				NewHeap;
	HeapTuple	Old_pg_index_Tuple,
				Old_pg_index_relation_Tuple,
				pg_proc_Tuple;
	Form_pg_index Old_pg_index_Form;
	Form_pg_class Old_pg_index_relation_Form;
	Form_pg_proc pg_proc_Form;
	char	   *NewIndexName;
	AttrNumber *attnumP;
	int			natts;
	FuncIndexInfo *finfo;

	NewHeap = heap_open(OIDNewHeap, AccessExclusiveLock);
	OldIndex = index_open(OIDOldIndex);

	/*
	 * OK. Create a new (temporary) index for the one that's already here.
	 * To do this I get the info from pg_index, re-build the FunctInfo if
	 * I have to, and add a new index with a temporary name.
	 */
	Old_pg_index_Tuple = SearchSysCacheTuple(INDEXRELID,
							ObjectIdGetDatum(RelationGetRelid(OldIndex)),
											 0, 0, 0);

	Assert(Old_pg_index_Tuple);
	Old_pg_index_Form = (Form_pg_index) GETSTRUCT(Old_pg_index_Tuple);

	Old_pg_index_relation_Tuple = SearchSysCacheTuple(RELOID,
							ObjectIdGetDatum(RelationGetRelid(OldIndex)),
													  0, 0, 0);

	Assert(Old_pg_index_relation_Tuple);
	Old_pg_index_relation_Form = (Form_pg_class) GETSTRUCT(Old_pg_index_relation_Tuple);

	/* Set the name. */
	NewIndexName = palloc(NAMEDATALEN); /* XXX */
	snprintf(NewIndexName, NAMEDATALEN, "temp_%x", OIDOldIndex);

	/*
	 * Ugly as it is, the only way I have of working out the number of
	 * attribues is to count them. Mostly there'll be just one but I've
	 * got to be sure.
	 */
	for (attnumP = &(Old_pg_index_Form->indkey[0]), natts = 0;
		 natts < INDEX_MAX_KEYS && *attnumP != InvalidAttrNumber;
		 attnumP++, natts++);

	/*
	 * If this is a functional index, I need to rebuild the functional
	 * component to pass it to the defining procedure.
	 */
	if (Old_pg_index_Form->indproc != InvalidOid)
	{
		finfo = (FuncIndexInfo *) palloc(sizeof(FuncIndexInfo));
		FIgetnArgs(finfo) = natts;
		FIgetProcOid(finfo) = Old_pg_index_Form->indproc;

		pg_proc_Tuple = SearchSysCacheTuple(PROCOID,
							ObjectIdGetDatum(Old_pg_index_Form->indproc),
											0, 0, 0);

		Assert(pg_proc_Tuple);
		pg_proc_Form = (Form_pg_proc) GETSTRUCT(pg_proc_Tuple);
		namecpy(&(finfo->funcName), &(pg_proc_Form->proname));
		natts = 1;				/* function result is a single column */
	}
	else
	{
		finfo = (FuncIndexInfo *) NULL;
	}

	index_create(RelationGetRelationName(NewHeap),
				 NewIndexName,
				 finfo,
				 NULL,			/* type info is in the old index */
				 Old_pg_index_relation_Form->relam,
				 natts,
				 Old_pg_index_Form->indkey,
				 Old_pg_index_Form->indclass,
				 (Node *) NULL,	/* XXX where's the predicate? */
				 Old_pg_index_Form->indislossy,
				 Old_pg_index_Form->indisunique,
				 Old_pg_index_Form->indisprimary,
				 allowSystemTableMods);

	setRelhasindexInplace(OIDNewHeap, true, false);

	index_close(OldIndex);
	heap_close(NewHeap, AccessExclusiveLock);
}


static void
rebuildheap(Oid OIDNewHeap, Oid OIDOldHeap, Oid OIDOldIndex)
{
	Relation	LocalNewHeap,
				LocalOldHeap,
				LocalOldIndex;
	IndexScanDesc ScanDesc;
	RetrieveIndexResult ScanResult;

	/*
	 * Open the relations I need. Scan through the OldHeap on the OldIndex
	 * and insert each tuple into the NewHeap.
	 */
	LocalNewHeap = heap_open(OIDNewHeap, AccessExclusiveLock);
	LocalOldHeap = heap_open(OIDOldHeap, AccessExclusiveLock);
	LocalOldIndex = index_open(OIDOldIndex);

	ScanDesc = index_beginscan(LocalOldIndex, false, 0, (ScanKey) NULL);

	while ((ScanResult = index_getnext(ScanDesc, ForwardScanDirection)) != NULL)
	{
		HeapTupleData LocalHeapTuple;
		Buffer		LocalBuffer;

		LocalHeapTuple.t_self = ScanResult->heap_iptr;
		LocalHeapTuple.t_datamcxt = NULL;
		LocalHeapTuple.t_data = NULL;
		heap_fetch(LocalOldHeap, SnapshotNow, &LocalHeapTuple, &LocalBuffer);
		if (LocalHeapTuple.t_data != NULL) {
			/*
			 * We must copy the tuple because heap_insert() will overwrite
			 * the commit-status fields of the tuple it's handed, and the
			 * retrieved tuple will actually be in a disk buffer!  Thus,
			 * the source relation would get trashed, which is bad news
			 * if we abort later on.  (This was a bug in releases thru 7.0)
			 */
			HeapTuple	copiedTuple = heap_copytuple(&LocalHeapTuple);

			ReleaseBuffer(LocalBuffer);
			heap_insert(LocalNewHeap, copiedTuple);
			heap_freetuple(copiedTuple);
		}
		pfree(ScanResult);
	}

	index_endscan(ScanDesc);

	index_close(LocalOldIndex);
	heap_close(LocalOldHeap, AccessExclusiveLock);
	heap_close(LocalNewHeap, AccessExclusiveLock);
}
