/*-------------------------------------------------------------------------
 *
 * nodeIndexscan.c
 *	  Routines to support indexes and indexed scans of relations
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/executor/nodeIndexscan.c,v 1.99 2004/12/31 21:59:45 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 * INTERFACE ROUTINES
 *		ExecIndexScan			scans a relation using indices
 *		ExecIndexNext			using index to retrieve next tuple
 *		ExecInitIndexScan		creates and initializes state info.
 *		ExecIndexReScan			rescans the indexed relation.
 *		ExecEndIndexScan		releases all storage.
 *		ExecIndexMarkPos		marks scan position.
 *		ExecIndexRestrPos		restores scan position.
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "executor/execdebug.h"
#include "executor/nodeIndexscan.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/clauses.h"
#include "parser/parsetree.h"


/*
 * In a multiple-index plan, we must take care to return any given tuple
 * only once, even if it matches conditions of several index scans.  Our
 * preferred way to do this is to record already-returned tuples in a hash
 * table (using the TID as unique identifier).	However, in a very large
 * scan this could conceivably run out of memory.  We limit the hash table
 * to no more than work_mem KB; if it grows past that, we fall back to the
 * pre-7.4 technique: evaluate the prior-scan index quals again for each
 * tuple (which is space-efficient, but slow).
 *
 * When scanning backwards, we use scannum to determine when to emit the
 * tuple --- we have to re-emit a tuple in the same scan as it was first
 * encountered.
 *
 * Note: this code would break if the planner were ever to create a multiple
 * index plan with overall backwards direction, because the hashtable code
 * will emit a tuple the first time it is encountered (which would be the
 * highest scan in which it matches the index), but the evaluate-the-quals
 * code will emit a tuple in the lowest-numbered scan in which it's valid.
 * This could be fixed at need by making the evaluate-the-quals case more
 * complex.  Currently the planner will never create such a plan (since it
 * considers multi-index plans unordered anyway), so there's no need for
 * more complexity.
 */
typedef struct
{
	/* tid is the hash key and so must be first! */
	ItemPointerData tid;		/* TID of a tuple we've returned */
	int			scannum;		/* number of scan we returned it in */
} DupHashTabEntry;


static TupleTableSlot *IndexNext(IndexScanState *node);
static void create_duphash(IndexScanState *node);


/* ----------------------------------------------------------------
 *		IndexNext
 *
 *		Retrieve a tuple from the IndexScan node's currentRelation
 *		using the indices in the IndexScanState information.
 *
 *		note: the old code mentions 'Primary indices'.	to my knowledge
 *		we only support a single secondary index. -cim 9/11/89
 *
 * old comments:
 *		retrieve a tuple from relation using the indices given.
 *		The indices are used in the order they appear in 'indices'.
 *		The indices may be primary or secondary indices:
 *		  * primary index --	scan the relation 'relID' using keys supplied.
 *		  * secondary index --	scan the index relation to get the 'tid' for
 *								a tuple in the relation 'relID'.
 *		If the current index(pointed by 'indexPtr') fails to return a
 *		tuple, the next index in the indices is used.
 *
 *		  bug fix so that it should retrieve on a null scan key.
 * ----------------------------------------------------------------
 */
static TupleTableSlot *
IndexNext(IndexScanState *node)
{
	EState	   *estate;
	ExprContext *econtext;
	ScanDirection direction;
	IndexScanDescPtr scanDescs;
	List	  **lossyQuals;
	IndexScanDesc scandesc;
	List	   *lossyQual;
	Index		scanrelid;
	HeapTuple	tuple;
	TupleTableSlot *slot;
	int			numIndices;
	bool		bBackward;
	int			indexNumber;

	/*
	 * extract necessary information from index scan node
	 */
	estate = node->ss.ps.state;
	direction = estate->es_direction;
	if (ScanDirectionIsBackward(((IndexScan *) node->ss.ps.plan)->indxorderdir))
	{
		if (ScanDirectionIsForward(direction))
			direction = BackwardScanDirection;
		else if (ScanDirectionIsBackward(direction))
			direction = ForwardScanDirection;
	}
	scanDescs = node->iss_ScanDescs;
	lossyQuals = node->iss_LossyQuals;
	numIndices = node->iss_NumIndices;
	econtext = node->ss.ps.ps_ExprContext;
	slot = node->ss.ss_ScanTupleSlot;
	scanrelid = ((IndexScan *) node->ss.ps.plan)->scan.scanrelid;

	/*
	 * Clear any reference to the previously returned tuple.  The idea
	 * here is to not have the tuple slot be the last holder of a pin on
	 * that tuple's buffer; if it is, we'll need a separate visit to the
	 * bufmgr to release the buffer.  By clearing here, we get to have the
	 * release done by ReleaseAndReadBuffer inside index_getnext.
	 */
	ExecClearTuple(slot);

	/*
	 * Check if we are evaluating PlanQual for tuple of this relation.
	 * Additional checking is not good, but no other way for now. We could
	 * introduce new nodes for this case and handle IndexScan --> NewNode
	 * switching in Init/ReScan plan...
	 */
	if (estate->es_evTuple != NULL &&
		estate->es_evTuple[scanrelid - 1] != NULL)
	{
		ListCell   *qual;

		if (estate->es_evTupleNull[scanrelid - 1])
			return slot;		/* return empty slot */

		ExecStoreTuple(estate->es_evTuple[scanrelid - 1],
					   slot, InvalidBuffer, false);

		/* Does the tuple meet any of the OR'd indxqual conditions? */
		econtext->ecxt_scantuple = slot;

		ResetExprContext(econtext);

		foreach(qual, node->indxqualorig)
		{
			if (ExecQual((List *) lfirst(qual), econtext, false))
				break;
		}
		if (qual == NULL)		/* would not be returned by indices */
			slot->val = NULL;

		/* Flag for the next call that no more tuples */
		estate->es_evTupleNull[scanrelid - 1] = true;

		return slot;
	}

	/*
	 * ok, now that we have what we need, fetch an index tuple. if
	 * scanning this index succeeded then return the appropriate heap
	 * tuple.. else return NULL.
	 */
	bBackward = ScanDirectionIsBackward(direction);
	if (bBackward)
	{
		indexNumber = numIndices - node->iss_IndexPtr - 1;
		if (indexNumber < 0)
		{
			indexNumber = 0;
			node->iss_IndexPtr = numIndices - 1;
		}
	}
	else
	{
		if ((indexNumber = node->iss_IndexPtr) < 0)
		{
			indexNumber = 0;
			node->iss_IndexPtr = 0;
		}
	}
	while (indexNumber < numIndices)
	{
		scandesc = scanDescs[node->iss_IndexPtr];
		lossyQual = lossyQuals[node->iss_IndexPtr];

		while ((tuple = index_getnext(scandesc, direction)) != NULL)
		{
			/*
			 * Store the scanned tuple in the scan tuple slot of the scan
			 * state.  Note: we pass 'false' because tuples returned by
			 * amgetnext are pointers onto disk pages and must not be
			 * pfree()'d.
			 */
			ExecStoreTuple(tuple,		/* tuple to store */
						   slot,	/* slot to store in */
						   scandesc->xs_cbuf,	/* buffer containing tuple */
						   false);		/* don't pfree */

			/*
			 * If any of the index operators involved in this scan are
			 * lossy, recheck them by evaluating the original operator
			 * clauses.
			 */
			if (lossyQual)
			{
				econtext->ecxt_scantuple = slot;
				ResetExprContext(econtext);
				if (!ExecQual(lossyQual, econtext, false))
				{
					/*
					 * Fails lossy op, so drop it and loop back for
					 * another
					 */
					ExecClearTuple(slot);
					continue;
				}
			}

			/*
			 * If it's a multiple-index scan, make sure not to
			 * double-report a tuple matched by more than one index.  (See
			 * notes above.)
			 */
			if (numIndices > 1)
			{
				/* First try the hash table */
				if (node->iss_DupHash)
				{
					DupHashTabEntry *entry;
					bool		found;

					entry = (DupHashTabEntry *)
						hash_search(node->iss_DupHash,
									&tuple->t_data->t_ctid,
									HASH_ENTER,
									&found);
					if (entry == NULL ||
					node->iss_DupHash->hctl->nentries > node->iss_MaxHash)
					{
						/* out of memory (either hard or soft limit) */
						/* release hash table and fall thru to old code */
						hash_destroy(node->iss_DupHash);
						node->iss_DupHash = NULL;
					}
					else if (found)
					{
						/* pre-existing entry */

						/*
						 * It's duplicate if first emitted in a different
						 * scan.  If same scan, we must be backing up, so
						 * okay to emit again.
						 */
						if (entry->scannum != node->iss_IndexPtr)
						{
							/* Dup, so drop it and loop back for another */
							ExecClearTuple(slot);
							continue;
						}
					}
					else
					{
						/* new entry, finish filling it in */
						entry->scannum = node->iss_IndexPtr;
					}
				}
				/* If hash table has overflowed, do it the hard way */
				if (node->iss_DupHash == NULL &&
					node->iss_IndexPtr > 0)
				{
					bool		prev_matches = false;
					int			prev_index;
					ListCell   *qual;

					econtext->ecxt_scantuple = slot;
					ResetExprContext(econtext);
					qual = list_head(node->indxqualorig);
					for (prev_index = 0;
						 prev_index < node->iss_IndexPtr;
						 prev_index++)
					{
						if (ExecQual((List *) lfirst(qual), econtext, false))
						{
							prev_matches = true;
							break;
						}
						qual = lnext(qual);
					}
					if (prev_matches)
					{
						/* Dup, so drop it and loop back for another */
						ExecClearTuple(slot);
						continue;
					}
				}
			}

			return slot;		/* OK to return tuple */
		}

		if (indexNumber < numIndices)
		{
			indexNumber++;
			if (bBackward)
				node->iss_IndexPtr--;
			else
				node->iss_IndexPtr++;
		}
	}

	/*
	 * if we get here it means the index scan failed so we are at the end
	 * of the scan..
	 */
	return ExecClearTuple(slot);
}

/* ----------------------------------------------------------------
 *		ExecIndexScan(node)
 *
 * old comments:
 *		Scans the relation using primary or secondary indices and returns
 *		   the next qualifying tuple in the direction specified.
 *		It calls ExecScan() and passes it the access methods which returns
 *		the next tuple using the indices.
 *
 *		Conditions:
 *		  -- the "cursor" maintained by the AMI is positioned at the tuple
 *			 returned previously.
 *
 *		Initial States:
 *		  -- the relation indicated is opened for scanning so that the
 *			 "cursor" is positioned before the first qualifying tuple.
 *		  -- all index realtions are opened for scanning.
 *		  -- indexPtr points to the first index.
 *		  -- state variable ruleFlag = nil.
 * ----------------------------------------------------------------
 */
TupleTableSlot *
ExecIndexScan(IndexScanState *node)
{
	/*
	 * If we have runtime keys and they've not already been set up, do it
	 * now.
	 */
	if (node->iss_RuntimeKeyInfo && !node->iss_RuntimeKeysReady)
		ExecReScan((PlanState *) node, NULL);

	/*
	 * use IndexNext as access method
	 */
	return ExecScan(&node->ss, (ExecScanAccessMtd) IndexNext);
}

/* ----------------------------------------------------------------
 *		ExecIndexReScan(node)
 *
 *		Recalculates the value of the scan keys whose value depends on
 *		information known at runtime and rescans the indexed relation.
 *		Updating the scan key was formerly done separately in
 *		ExecUpdateIndexScanKeys. Integrating it into ReScan makes
 *		rescans of indices and relations/general streams more uniform.
 *
 * ----------------------------------------------------------------
 */
void
ExecIndexReScan(IndexScanState *node, ExprContext *exprCtxt)
{
	EState	   *estate;
	ExprContext *econtext;
	int			numIndices;
	IndexScanDescPtr scanDescs;
	ScanKey    *scanKeys;
	ExprState ***runtimeKeyInfo;
	int		   *numScanKeys;
	Index		scanrelid;
	int			i;
	int			j;

	estate = node->ss.ps.state;
	econtext = node->iss_RuntimeContext;		/* context for runtime
												 * keys */
	numIndices = node->iss_NumIndices;
	scanDescs = node->iss_ScanDescs;
	scanKeys = node->iss_ScanKeys;
	runtimeKeyInfo = node->iss_RuntimeKeyInfo;
	numScanKeys = node->iss_NumScanKeys;
	scanrelid = ((IndexScan *) node->ss.ps.plan)->scan.scanrelid;

	if (econtext)
	{
		/*
		 * If we are being passed an outer tuple, save it for runtime key
		 * calc.  We also need to link it into the "regular" per-tuple
		 * econtext, so it can be used during indexqualorig evaluations.
		 */
		if (exprCtxt != NULL)
		{
			ExprContext *stdecontext;

			econtext->ecxt_outertuple = exprCtxt->ecxt_outertuple;
			stdecontext = node->ss.ps.ps_ExprContext;
			stdecontext->ecxt_outertuple = exprCtxt->ecxt_outertuple;
		}

		/*
		 * Reset the runtime-key context so we don't leak memory as each
		 * outer tuple is scanned.	Note this assumes that we will
		 * recalculate *all* runtime keys on each call.
		 */
		ResetExprContext(econtext);
	}

	/*
	 * If we are doing runtime key calculations (ie, the index keys depend
	 * on data from an outer scan), compute the new key values
	 */
	if (runtimeKeyInfo)
	{
		for (i = 0; i < numIndices; i++)
		{
			int			n_keys;
			ScanKey		scan_keys;
			ExprState **run_keys;

			n_keys = numScanKeys[i];
			scan_keys = scanKeys[i];
			run_keys = runtimeKeyInfo[i];

			for (j = 0; j < n_keys; j++)
			{
				/*
				 * If we have a run-time key, then extract the run-time
				 * expression and evaluate it with respect to the current
				 * outer tuple.  We then stick the result into the scan
				 * key.
				 *
				 * Note: the result of the eval could be a pass-by-ref value
				 * that's stored in the outer scan's tuple, not in
				 * econtext->ecxt_per_tuple_memory.  We assume that the
				 * outer tuple will stay put throughout our scan.  If this
				 * is wrong, we could copy the result into our context
				 * explicitly, but I think that's not necessary...
				 */
				if (run_keys[j] != NULL)
				{
					Datum		scanvalue;
					bool		isNull;

					scanvalue = ExecEvalExprSwitchContext(run_keys[j],
														  econtext,
														  &isNull,
														  NULL);
					scan_keys[j].sk_argument = scanvalue;
					if (isNull)
						scan_keys[j].sk_flags |= SK_ISNULL;
					else
						scan_keys[j].sk_flags &= ~SK_ISNULL;
				}
			}
		}

		node->iss_RuntimeKeysReady = true;
	}

	/* If this is re-scanning of PlanQual ... */
	if (estate->es_evTuple != NULL &&
		estate->es_evTuple[scanrelid - 1] != NULL)
	{
		estate->es_evTupleNull[scanrelid - 1] = false;
		return;
	}

	/* reset hash table */
	if (numIndices > 1)
	{
		if (node->iss_DupHash)
			hash_destroy(node->iss_DupHash);
		create_duphash(node);
	}

	/* reset index scans */
	if (ScanDirectionIsBackward(((IndexScan *) node->ss.ps.plan)->indxorderdir))
		node->iss_IndexPtr = numIndices;
	else
		node->iss_IndexPtr = -1;

	for (i = 0; i < numIndices; i++)
	{
		IndexScanDesc scan = scanDescs[i];
		ScanKey		skey = scanKeys[i];

		index_rescan(scan, skey);
	}
}

/* ----------------------------------------------------------------
 *		ExecEndIndexScan
 * ----------------------------------------------------------------
 */
void
ExecEndIndexScan(IndexScanState *node)
{
	int			numIndices;
	RelationPtr indexRelationDescs;
	IndexScanDescPtr indexScanDescs;
	Relation	relation;
	int			i;

	/*
	 * extract information from the node
	 */
	numIndices = node->iss_NumIndices;
	indexRelationDescs = node->iss_RelationDescs;
	indexScanDescs = node->iss_ScanDescs;
	relation = node->ss.ss_currentRelation;

	/*
	 * Free the exprcontext(s)
	 */
	ExecFreeExprContext(&node->ss.ps);
	if (node->iss_RuntimeContext)
		FreeExprContext(node->iss_RuntimeContext);

	/*
	 * clear out tuple table slots
	 */
	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);
	ExecClearTuple(node->ss.ss_ScanTupleSlot);

	/* drop hash table */
	if (node->iss_DupHash)
		hash_destroy(node->iss_DupHash);

	/*
	 * close the index relations
	 */
	for (i = 0; i < numIndices; i++)
	{
		if (indexScanDescs[i] != NULL)
			index_endscan(indexScanDescs[i]);

		if (indexRelationDescs[i] != NULL)
			index_close(indexRelationDescs[i]);
	}

	/*
	 * close the heap relation.
	 *
	 * Currently, we do not release the AccessShareLock acquired by
	 * ExecInitIndexScan.  This lock should be held till end of
	 * transaction. (There is a faction that considers this too much
	 * locking, however.)
	 */
	heap_close(relation, NoLock);
}

/* ----------------------------------------------------------------
 *		ExecIndexMarkPos
 *
 * old comments
 *		Marks scan position by marking the current index.
 *		Returns nothing.
 * ----------------------------------------------------------------
 */
void
ExecIndexMarkPos(IndexScanState *node)
{
	IndexScanDescPtr indexScanDescs;
	IndexScanDesc scanDesc;
	int			indexPtr;

	indexPtr = node->iss_MarkIndexPtr = node->iss_IndexPtr;
	if (indexPtr >= 0 && indexPtr < node->iss_NumIndices)
	{
		indexScanDescs = node->iss_ScanDescs;
		scanDesc = indexScanDescs[indexPtr];

		index_markpos(scanDesc);
	}
}

/* ----------------------------------------------------------------
 *		ExecIndexRestrPos
 *
 * old comments
 *		Restores scan position by restoring the current index.
 *		Returns nothing.
 * ----------------------------------------------------------------
 */
void
ExecIndexRestrPos(IndexScanState *node)
{
	IndexScanDescPtr indexScanDescs;
	IndexScanDesc scanDesc;
	int			indexPtr;

	indexPtr = node->iss_IndexPtr = node->iss_MarkIndexPtr;
	if (indexPtr >= 0 && indexPtr < node->iss_NumIndices)
	{
		indexScanDescs = node->iss_ScanDescs;
		scanDesc = indexScanDescs[indexPtr];

		index_restrpos(scanDesc);
	}
}

/* ----------------------------------------------------------------
 *		ExecInitIndexScan
 *
 *		Initializes the index scan's state information, creates
 *		scan keys, and opens the base and index relations.
 *
 *		Note: index scans have 2 sets of state information because
 *			  we have to keep track of the base relation and the
 *			  index relations.
 *
 * old comments
 *		Creates the run-time state information for the node and
 *		sets the relation id to contain relevant descriptors.
 *
 *		Parameters:
 *		  node: IndexNode node produced by the planner.
 *		  estate: the execution state initialized in InitPlan.
 * ----------------------------------------------------------------
 */
IndexScanState *
ExecInitIndexScan(IndexScan *node, EState *estate)
{
	IndexScanState *indexstate;
	ListCell   *indxqual;
	ListCell   *indxstrategy;
	ListCell   *indxsubtype;
	ListCell   *indxlossy;
	ListCell   *indxid_item;
	int			i;
	int			numIndices;
	int			indexPtr;
	ScanKey    *scanKeys;
	int		   *numScanKeys;
	RelationPtr indexDescs;
	IndexScanDescPtr scanDescs;
	List	  **lossyQuals;
	ExprState ***runtimeKeyInfo;
	bool		have_runtime_keys;
	RangeTblEntry *rtentry;
	Index		relid;
	Oid			reloid;
	Relation	currentRelation;

	/*
	 * create state structure
	 */
	indexstate = makeNode(IndexScanState);
	indexstate->ss.ps.plan = (Plan *) node;
	indexstate->ss.ps.state = estate;

	/*
	 * Miscellaneous initialization
	 *
	 * create expression context for node
	 */
	ExecAssignExprContext(estate, &indexstate->ss.ps);

	/*
	 * initialize child expressions
	 *
	 * Note: we don't initialize all of the indxqual expression, only the
	 * sub-parts corresponding to runtime keys (see below).  The
	 * indxqualorig expression is always initialized even though it will
	 * only be used in some uncommon cases --- would be nice to improve
	 * that.  (Problem is that any SubPlans present in the expression must
	 * be found now...)
	 */
	indexstate->ss.ps.targetlist = (List *)
		ExecInitExpr((Expr *) node->scan.plan.targetlist,
					 (PlanState *) indexstate);
	indexstate->ss.ps.qual = (List *)
		ExecInitExpr((Expr *) node->scan.plan.qual,
					 (PlanState *) indexstate);
	indexstate->indxqualorig = (List *)
		ExecInitExpr((Expr *) node->indxqualorig,
					 (PlanState *) indexstate);

#define INDEXSCAN_NSLOTS 2

	/*
	 * tuple table initialization
	 */
	ExecInitResultTupleSlot(estate, &indexstate->ss.ps);
	ExecInitScanTupleSlot(estate, &indexstate->ss);

	/*
	 * Initialize index-specific scan state
	 */
	indexstate->iss_NumIndices = 0;
	indexstate->iss_IndexPtr = -1;
	indexstate->iss_ScanKeys = NULL;
	indexstate->iss_NumScanKeys = NULL;
	indexstate->iss_RuntimeKeyInfo = NULL;
	indexstate->iss_RuntimeContext = NULL;
	indexstate->iss_RuntimeKeysReady = false;
	indexstate->iss_RelationDescs = NULL;
	indexstate->iss_ScanDescs = NULL;
	indexstate->iss_LossyQuals = NULL;

	/*
	 * get the index node information
	 */
	indxid_item = list_head(node->indxid);
	numIndices = list_length(node->indxid);
	indexPtr = -1;

	CXT1_printf("ExecInitIndexScan: context is %d\n", CurrentMemoryContext);

	/*
	 * scanKeys is used to keep track of the ScanKey's. This is needed
	 * because a single scan may use several indices and each index has
	 * its own ScanKey.
	 */
	numScanKeys = (int *) palloc(numIndices * sizeof(int));
	scanKeys = (ScanKey *) palloc(numIndices * sizeof(ScanKey));
	indexDescs = (RelationPtr) palloc(numIndices * sizeof(Relation));
	scanDescs = (IndexScanDescPtr) palloc(numIndices * sizeof(IndexScanDesc));
	lossyQuals = (List **) palloc0(numIndices * sizeof(List *));

	/*
	 * initialize space for runtime key info (may not be needed)
	 */
	have_runtime_keys = false;
	runtimeKeyInfo = (ExprState ***) palloc0(numIndices * sizeof(ExprState **));

	/*
	 * build the index scan keys from the index qualification
	 */
	indxqual = list_head(node->indxqual);
	indxstrategy = list_head(node->indxstrategy);
	indxsubtype = list_head(node->indxsubtype);
	indxlossy = list_head(node->indxlossy);
	for (i = 0; i < numIndices; i++)
	{
		List	   *quals;
		List	   *strategies;
		List	   *subtypes;
		List	   *lossyflags;
		ListCell   *qual_cell;
		ListCell   *strategy_cell;
		ListCell   *subtype_cell;
		ListCell   *lossyflag_cell;
		int			n_keys;
		ScanKey		scan_keys;
		ExprState **run_keys;
		int			j;

		quals = (List *) lfirst(indxqual);
		indxqual = lnext(indxqual);
		strategies = (List *) lfirst(indxstrategy);
		indxstrategy = lnext(indxstrategy);
		subtypes = (List *) lfirst(indxsubtype);
		indxsubtype = lnext(indxsubtype);
		lossyflags = (List *) lfirst(indxlossy);
		indxlossy = lnext(indxlossy);
		n_keys = list_length(quals);
		scan_keys = (n_keys <= 0) ? NULL :
			(ScanKey) palloc(n_keys * sizeof(ScanKeyData));
		run_keys = (n_keys <= 0) ? NULL :
			(ExprState **) palloc(n_keys * sizeof(ExprState *));

		/*
		 * for each opclause in the given qual, convert each qual's
		 * opclause into a single scan key
		 */
		qual_cell = list_head(quals);
		strategy_cell = list_head(strategies);
		subtype_cell = list_head(subtypes);
		lossyflag_cell = list_head(lossyflags);
		for (j = 0; j < n_keys; j++)
		{
			OpExpr	   *clause; /* one clause of index qual */
			Expr	   *leftop; /* expr on lhs of operator */
			Expr	   *rightop;	/* expr on rhs ... */
			int			flags = 0;
			AttrNumber	varattno;		/* att number used in scan */
			StrategyNumber strategy;	/* op's strategy number */
			Oid			subtype;	/* op's strategy subtype */
			int			lossy;	/* op's recheck flag */
			RegProcedure opfuncid;		/* operator proc id used in scan */
			Datum		scanvalue;		/* value used in scan (if const) */

			/*
			 * extract clause information from the qualification
			 */
			clause = (OpExpr *) lfirst(qual_cell);
			qual_cell = lnext(qual_cell);
			strategy = lfirst_int(strategy_cell);
			strategy_cell = lnext(strategy_cell);
			subtype = lfirst_oid(subtype_cell);
			subtype_cell = lnext(subtype_cell);
			lossy = lfirst_int(lossyflag_cell);
			lossyflag_cell = lnext(lossyflag_cell);

			if (!IsA(clause, OpExpr))
				elog(ERROR, "indxqual is not an OpExpr");

			opfuncid = clause->opfuncid;

			/*
			 * Here we figure out the contents of the index qual. The
			 * usual case is (var op const) which means we form a scan key
			 * for the attribute listed in the var node and use the value
			 * of the const as comparison data.
			 *
			 * If we don't have a const node, it means our scan key is a
			 * function of information obtained during the execution of
			 * the plan, in which case we need to recalculate the index
			 * scan key at run time.  Hence, we set have_runtime_keys to
			 * true and place the appropriate subexpression in run_keys.
			 * The corresponding scan key values are recomputed at run
			 * time.
			 */
			run_keys[j] = NULL;

			/*
			 * determine information in leftop
			 */
			leftop = (Expr *) get_leftop((Expr *) clause);

			if (leftop && IsA(leftop, RelabelType))
				leftop = ((RelabelType *) leftop)->arg;

			Assert(leftop != NULL);

			if (!(IsA(leftop, Var) &&
				  var_is_rel((Var *) leftop)))
				elog(ERROR, "indxqual doesn't have key on left side");

			varattno = ((Var *) leftop)->varattno;

			/*
			 * now determine information in rightop
			 */
			rightop = (Expr *) get_rightop((Expr *) clause);

			if (rightop && IsA(rightop, RelabelType))
				rightop = ((RelabelType *) rightop)->arg;

			Assert(rightop != NULL);

			if (IsA(rightop, Const))
			{
				/*
				 * if the rightop is a const node then it means it
				 * identifies the value to place in our scan key.
				 */
				scanvalue = ((Const *) rightop)->constvalue;
				if (((Const *) rightop)->constisnull)
					flags |= SK_ISNULL;
			}
			else
			{
				/*
				 * otherwise, the rightop contains an expression evaluable
				 * at runtime to figure out the value to place in our scan
				 * key.
				 */
				have_runtime_keys = true;
				run_keys[j] = ExecInitExpr(rightop, (PlanState *) indexstate);
				scanvalue = (Datum) 0;
			}

			/*
			 * initialize the scan key's fields appropriately
			 */
			ScanKeyEntryInitialize(&scan_keys[j],
								   flags,
								   varattno,	/* attribute number to
												 * scan */
								   strategy,	/* op's strategy */
								   subtype,		/* strategy subtype */
								   opfuncid,	/* reg proc to use */
								   scanvalue);	/* constant */

			/*
			 * If this operator is lossy, add its indxqualorig expression
			 * to the list of quals to recheck.  The list_nth() calls here
			 * could be avoided by chasing the lists in parallel to all
			 * the other lists, but since lossy operators are very
			 * uncommon, it's probably a waste of time to do so.
			 */
			if (lossy)
			{
				List	   *qualOrig = indexstate->indxqualorig;

				lossyQuals[i] = lappend(lossyQuals[i],
							list_nth((List *) list_nth(qualOrig, i), j));
			}
		}

		/*
		 * store the key information into our arrays.
		 */
		numScanKeys[i] = n_keys;
		scanKeys[i] = scan_keys;
		runtimeKeyInfo[i] = run_keys;
	}

	indexstate->iss_NumIndices = numIndices;
	if (ScanDirectionIsBackward(node->indxorderdir))
		indexPtr = numIndices;
	indexstate->iss_IndexPtr = indexPtr;
	indexstate->iss_ScanKeys = scanKeys;
	indexstate->iss_NumScanKeys = numScanKeys;

	/*
	 * If all of our keys have the form (var op const), then we have no
	 * runtime keys so we store NULL in the runtime key info. Otherwise
	 * runtime key info contains an array of pointers (one for each index)
	 * to arrays of flags (one for each key) which indicate that the qual
	 * needs to be evaluated at runtime. -cim 10/24/89
	 *
	 * If we do have runtime keys, we need an ExprContext to evaluate them;
	 * the node's standard context won't do because we want to reset that
	 * context for every tuple.  So, build another context just like the
	 * other one... -tgl 7/11/00
	 */
	if (have_runtime_keys)
	{
		ExprContext *stdecontext = indexstate->ss.ps.ps_ExprContext;

		ExecAssignExprContext(estate, &indexstate->ss.ps);
		indexstate->iss_RuntimeKeyInfo = runtimeKeyInfo;
		indexstate->iss_RuntimeContext = indexstate->ss.ps.ps_ExprContext;
		indexstate->ss.ps.ps_ExprContext = stdecontext;
	}
	else
	{
		indexstate->iss_RuntimeKeyInfo = NULL;
		indexstate->iss_RuntimeContext = NULL;
		/* Get rid of the speculatively-allocated flag arrays, too */
		for (i = 0; i < numIndices; i++)
		{
			if (runtimeKeyInfo[i] != NULL)
				pfree(runtimeKeyInfo[i]);
		}
		pfree(runtimeKeyInfo);
	}

	/*
	 * open the base relation and acquire AccessShareLock on it.
	 */
	relid = node->scan.scanrelid;
	rtentry = rt_fetch(relid, estate->es_range_table);
	reloid = rtentry->relid;

	currentRelation = heap_open(reloid, AccessShareLock);

	indexstate->ss.ss_currentRelation = currentRelation;
	indexstate->ss.ss_currentScanDesc = NULL;	/* no heap scan here */

	/*
	 * get the scan type from the relation descriptor.
	 */
	ExecAssignScanType(&indexstate->ss, RelationGetDescr(currentRelation), false);

	/*
	 * open the index relations and initialize relation and scan
	 * descriptors.  Note we acquire no locks here; the index machinery
	 * does its own locks and unlocks.	(We rely on having AccessShareLock
	 * on the parent table to ensure the index won't go away!)
	 */
	for (i = 0; i < numIndices; i++)
	{
		Oid			indexOid = lfirst_oid(indxid_item);

		indexDescs[i] = index_open(indexOid);
		scanDescs[i] = index_beginscan(currentRelation,
									   indexDescs[i],
									   estate->es_snapshot,
									   numScanKeys[i],
									   scanKeys[i]);
		indxid_item = lnext(indxid_item);
	}

	indexstate->iss_RelationDescs = indexDescs;
	indexstate->iss_ScanDescs = scanDescs;
	indexstate->iss_LossyQuals = lossyQuals;

	/*
	 * Initialize result tuple type and projection info.
	 */
	ExecAssignResultTypeFromTL(&indexstate->ss.ps);
	ExecAssignScanProjectionInfo(&indexstate->ss);

	/*
	 * Initialize hash table if needed.
	 */
	if (numIndices > 1)
		create_duphash(indexstate);
	else
		indexstate->iss_DupHash = NULL;

	/*
	 * all done.
	 */
	return indexstate;
}

static void
create_duphash(IndexScanState *node)
{
	HASHCTL		hash_ctl;
	long		nbuckets;

	node->iss_MaxHash = (work_mem * 1024L) /
		(MAXALIGN(sizeof(HASHELEMENT)) + MAXALIGN(sizeof(DupHashTabEntry)));
	MemSet(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = SizeOfIptrData;
	hash_ctl.entrysize = sizeof(DupHashTabEntry);
	hash_ctl.hash = tag_hash;
	hash_ctl.hcxt = CurrentMemoryContext;
	nbuckets = (long) ceil(node->ss.ps.plan->plan_rows);
	if (nbuckets < 1)
		nbuckets = 1;
	if (nbuckets > node->iss_MaxHash)
		nbuckets = node->iss_MaxHash;
	node->iss_DupHash = hash_create("DupHashTable",
									nbuckets,
									&hash_ctl,
							   HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);
}

int
ExecCountSlotsIndexScan(IndexScan *node)
{
	return ExecCountSlotsNode(outerPlan((Plan *) node)) +
		ExecCountSlotsNode(innerPlan((Plan *) node)) + INDEXSCAN_NSLOTS;
}
