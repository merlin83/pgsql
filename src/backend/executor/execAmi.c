/*-------------------------------------------------------------------------
 *
 * execAmi.c
 *	  miscellaneous executor access method routines
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$PostgreSQL: pgsql/src/backend/executor/execAmi.c,v 1.77 2003/12/18 20:21:37 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/heap.h"
#include "executor/execdebug.h"
#include "executor/instrument.h"
#include "executor/nodeAgg.h"
#include "executor/nodeAppend.h"
#include "executor/nodeFunctionscan.h"
#include "executor/nodeGroup.h"
#include "executor/nodeGroup.h"
#include "executor/nodeHash.h"
#include "executor/nodeHashjoin.h"
#include "executor/nodeIndexscan.h"
#include "executor/nodeLimit.h"
#include "executor/nodeMaterial.h"
#include "executor/nodeMergejoin.h"
#include "executor/nodeNestloop.h"
#include "executor/nodeResult.h"
#include "executor/nodeSeqscan.h"
#include "executor/nodeSetOp.h"
#include "executor/nodeSort.h"
#include "executor/nodeSubplan.h"
#include "executor/nodeSubqueryscan.h"
#include "executor/nodeTidscan.h"
#include "executor/nodeUnique.h"


/*
 * ExecReScan
 *		Reset a plan node so that its output can be re-scanned.
 *
 * Note that if the plan node has parameters that have changed value,
 * the output might be different from last time.
 *
 * The second parameter is currently only used to pass a NestLoop plan's
 * econtext down to its inner child plan, in case that is an indexscan that
 * needs access to variables of the current outer tuple.  (The handling of
 * this parameter is currently pretty inconsistent: some callers pass NULL
 * and some pass down their parent's value; so don't rely on it in other
 * situations.  It'd probably be better to remove the whole thing and use
 * the generalized parameter mechanism instead.)
 */
void
ExecReScan(PlanState *node, ExprContext *exprCtxt)
{
	/* If collecting timing stats, update them */
	if (node->instrument)
		InstrEndLoop(node->instrument);

	/* If we have changed parameters, propagate that info */
	if (node->chgParam != NULL)
	{
		List	   *lst;

		foreach(lst, node->initPlan)
		{
			SubPlanState *sstate = (SubPlanState *) lfirst(lst);
			PlanState  *splan = sstate->planstate;

			if (splan->plan->extParam != NULL)	/* don't care about child
												 * local Params */
				UpdateChangedParamSet(splan, node->chgParam);
			if (splan->chgParam != NULL)
				ExecReScanSetParamPlan(sstate, node);
		}
		foreach(lst, node->subPlan)
		{
			SubPlanState *sstate = (SubPlanState *) lfirst(lst);
			PlanState  *splan = sstate->planstate;

			if (splan->plan->extParam != NULL)
				UpdateChangedParamSet(splan, node->chgParam);
		}
		/* Well. Now set chgParam for left/right trees. */
		if (node->lefttree != NULL)
			UpdateChangedParamSet(node->lefttree, node->chgParam);
		if (node->righttree != NULL)
			UpdateChangedParamSet(node->righttree, node->chgParam);
	}

	/* Shut down any SRFs in the plan node's targetlist */
	if (node->ps_ExprContext)
		ReScanExprContext(node->ps_ExprContext);

	/* And do node-type-specific processing */
	switch (nodeTag(node))
	{
		case T_ResultState:
			ExecReScanResult((ResultState *) node, exprCtxt);
			break;

		case T_AppendState:
			ExecReScanAppend((AppendState *) node, exprCtxt);
			break;

		case T_SeqScanState:
			ExecSeqReScan((SeqScanState *) node, exprCtxt);
			break;

		case T_IndexScanState:
			ExecIndexReScan((IndexScanState *) node, exprCtxt);
			break;

		case T_TidScanState:
			ExecTidReScan((TidScanState *) node, exprCtxt);
			break;

		case T_SubqueryScanState:
			ExecSubqueryReScan((SubqueryScanState *) node, exprCtxt);
			break;

		case T_FunctionScanState:
			ExecFunctionReScan((FunctionScanState *) node, exprCtxt);
			break;

		case T_NestLoopState:
			ExecReScanNestLoop((NestLoopState *) node, exprCtxt);
			break;

		case T_MergeJoinState:
			ExecReScanMergeJoin((MergeJoinState *) node, exprCtxt);
			break;

		case T_HashJoinState:
			ExecReScanHashJoin((HashJoinState *) node, exprCtxt);
			break;

		case T_MaterialState:
			ExecMaterialReScan((MaterialState *) node, exprCtxt);
			break;

		case T_SortState:
			ExecReScanSort((SortState *) node, exprCtxt);
			break;

		case T_GroupState:
			ExecReScanGroup((GroupState *) node, exprCtxt);
			break;

		case T_AggState:
			ExecReScanAgg((AggState *) node, exprCtxt);
			break;

		case T_UniqueState:
			ExecReScanUnique((UniqueState *) node, exprCtxt);
			break;

		case T_HashState:
			ExecReScanHash((HashState *) node, exprCtxt);
			break;

		case T_SetOpState:
			ExecReScanSetOp((SetOpState *) node, exprCtxt);
			break;

		case T_LimitState:
			ExecReScanLimit((LimitState *) node, exprCtxt);
			break;

		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}

	if (node->chgParam != NULL)
	{
		bms_free(node->chgParam);
		node->chgParam = NULL;
	}
}

/*
 * ExecMarkPos
 *
 * Marks the current scan position.
 */
void
ExecMarkPos(PlanState *node)
{
	switch (nodeTag(node))
	{
		case T_SeqScanState:
			ExecSeqMarkPos((SeqScanState *) node);
			break;

		case T_IndexScanState:
			ExecIndexMarkPos((IndexScanState *) node);
			break;

		case T_TidScanState:
			ExecTidMarkPos((TidScanState *) node);
			break;

		case T_FunctionScanState:
			ExecFunctionMarkPos((FunctionScanState *) node);
			break;

		case T_MaterialState:
			ExecMaterialMarkPos((MaterialState *) node);
			break;

		case T_SortState:
			ExecSortMarkPos((SortState *) node);
			break;

		default:
			/* don't make hard error unless caller asks to restore... */
			elog(DEBUG2, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}
}

/*
 * ExecRestrPos
 *
 * restores the scan position previously saved with ExecMarkPos()
 */
void
ExecRestrPos(PlanState *node)
{
	switch (nodeTag(node))
	{
		case T_SeqScanState:
			ExecSeqRestrPos((SeqScanState *) node);
			break;

		case T_IndexScanState:
			ExecIndexRestrPos((IndexScanState *) node);
			break;

		case T_TidScanState:
			ExecTidRestrPos((TidScanState *) node);
			break;

		case T_FunctionScanState:
			ExecFunctionRestrPos((FunctionScanState *) node);
			break;

		case T_MaterialState:
			ExecMaterialRestrPos((MaterialState *) node);
			break;

		case T_SortState:
			ExecSortRestrPos((SortState *) node);
			break;

		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(node));
			break;
	}
}

/*
 * ExecSupportsMarkRestore - does a plan type support mark/restore?
 *
 * XXX Ideally, all plan node types would support mark/restore, and this
 * wouldn't be needed.  For now, this had better match the routines above.
 * But note the test is on Plan nodetype, not PlanState nodetype.
 */
bool
ExecSupportsMarkRestore(NodeTag plantype)
{
	switch (plantype)
	{
		case T_SeqScan:
		case T_IndexScan:
		case T_TidScan:
		case T_FunctionScan:
		case T_Material:
		case T_Sort:
			return true;

		default:
			break;
	}

	return false;
}

/*
 * ExecSupportsBackwardScan - does a plan type support backwards scanning?
 *
 * Ideally, all plan types would support backwards scan, but that seems
 * unlikely to happen soon.  In some cases, a plan node passes the backwards
 * scan down to its children, and so supports backwards scan only if its
 * children do.  Therefore, this routine must be passed a complete plan tree.
 */
bool
ExecSupportsBackwardScan(Plan *node)
{
	if (node == NULL)
		return false;

	switch (nodeTag(node))
	{
		case T_Result:
			if (outerPlan(node) != NULL)
				return ExecSupportsBackwardScan(outerPlan(node));
			else
				return false;

		case T_Append:
			{
				List	   *l;

				foreach(l, ((Append *) node)->appendplans)
				{
					if (!ExecSupportsBackwardScan((Plan *) lfirst(l)))
						return false;
				}
				return true;
			}

		case T_SeqScan:
		case T_IndexScan:
		case T_TidScan:
		case T_FunctionScan:
			return true;

		case T_SubqueryScan:
			return ExecSupportsBackwardScan(((SubqueryScan *) node)->subplan);

		case T_Material:
		case T_Sort:
			return true;

		case T_Unique:
			return ExecSupportsBackwardScan(outerPlan(node));

		case T_Limit:
			return ExecSupportsBackwardScan(outerPlan(node));

		default:
			return false;
	}
}
