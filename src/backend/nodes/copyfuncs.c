/*-------------------------------------------------------------------------
 *
 * copyfuncs.c
 *	  Copy functions for Postgres tree nodes.
 *
 * NOTE: a general convention when copying or comparing plan nodes is
 * that we ignore the executor state subnode.  We do not need to look
 * at it because no current uses of copyObject() or equal() need to
 * deal with already-executing plan trees.	By leaving the state subnodes
 * out, we avoid needing to write copy/compare routines for all the
 * different executor state node types.
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/nodes/copyfuncs.c,v 1.203 2002-08-19 00:40:14 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "optimizer/clauses.h"
#include "optimizer/planmain.h"


/*
 * Node_Copy
 *	  a macro to simplify calling of copyObject on the specified field
 */
#define Node_Copy(from, newnode, field) \
	((newnode)->field = copyObject((from)->field))


/*
 * listCopy
 *	  This copy function only copies the "cons-cells" of the list, not the
 *	  pointed-to objects.  (Use copyObject if you want a "deep" copy.)
 *
 *	  We also use this function for copying lists of integers, which is
 *	  grotty but unlikely to break --- it could fail if sizeof(pointer)
 *	  is less than sizeof(int), but I don't know any such machines...
 *
 *	  Note that copyObject will surely coredump if applied to a list
 *	  of integers!
 */
List *
listCopy(List *list)
{
	List	   *newlist,
			   *l,
			   *nl;

	/* rather ugly coding for speed... */
	if (list == NIL)
		return NIL;

	newlist = nl = makeList1(lfirst(list));

	foreach(l, lnext(list))
	{
		lnext(nl) = makeList1(lfirst(l));
		nl = lnext(nl);
	}
	return newlist;
}

/* ****************************************************************
 *					 plannodes.h copy functions
 * ****************************************************************
 */

/* ----------------
 *		CopyPlanFields
 *
 *		This function copies the fields of the Plan node.  It is used by
 *		all the copy functions for classes which inherit from Plan.
 * ----------------
 */
static void
CopyPlanFields(Plan *from, Plan *newnode)
{
	newnode->startup_cost = from->startup_cost;
	newnode->total_cost = from->total_cost;
	newnode->plan_rows = from->plan_rows;
	newnode->plan_width = from->plan_width;
	/* state is NOT copied */
	Node_Copy(from, newnode, targetlist);
	Node_Copy(from, newnode, qual);
	Node_Copy(from, newnode, lefttree);
	Node_Copy(from, newnode, righttree);
	newnode->extParam = listCopy(from->extParam);
	newnode->locParam = listCopy(from->locParam);
	newnode->chgParam = listCopy(from->chgParam);
	Node_Copy(from, newnode, initPlan);
	/* subPlan list must point to subplans in the new subtree, not the old */
	if (from->subPlan != NIL)
		newnode->subPlan = nconc(pull_subplans((Node *) newnode->targetlist),
								 pull_subplans((Node *) newnode->qual));
	else
		newnode->subPlan = NIL;
	newnode->nParamExec = from->nParamExec;
}

/* ----------------
 *		_copyPlan
 * ----------------
 */
static Plan *
_copyPlan(Plan *from)
{
	Plan	   *newnode = makeNode(Plan);

	/*
	 * copy the node superclass fields
	 */
	CopyPlanFields(from, newnode);

	return newnode;
}


/* ----------------
 *		_copyResult
 * ----------------
 */
static Result *
_copyResult(Result *from)
{
	Result	   *newnode = makeNode(Result);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, resconstantqual);

	/*
	 * We must add subplans in resconstantqual to the new plan's subPlan
	 * list
	 */
	if (from->plan.subPlan != NIL)
		newnode->plan.subPlan = nconc(newnode->plan.subPlan,
								pull_subplans(newnode->resconstantqual));

	return newnode;
}

/* ----------------
 *		_copyAppend
 * ----------------
 */
static Append *
_copyAppend(Append *from)
{
	Append	   *newnode = makeNode(Append);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, appendplans);
	newnode->isTarget = from->isTarget;

	return newnode;
}


/* ----------------
 *		CopyScanFields
 *
 *		This function copies the fields of the Scan node.  It is used by
 *		all the copy functions for classes which inherit from Scan.
 * ----------------
 */
static void
CopyScanFields(Scan *from, Scan *newnode)
{
	newnode->scanrelid = from->scanrelid;
	return;
}

/* ----------------
 *		_copyScan
 * ----------------
 */
static Scan *
_copyScan(Scan *from)
{
	Scan	   *newnode = makeNode(Scan);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyScanFields((Scan *) from, (Scan *) newnode);

	return newnode;
}

/* ----------------
 *		_copySeqScan
 * ----------------
 */
static SeqScan *
_copySeqScan(SeqScan *from)
{
	SeqScan    *newnode = makeNode(SeqScan);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyScanFields((Scan *) from, (Scan *) newnode);

	return newnode;
}

/* ----------------
 *		_copyIndexScan
 * ----------------
 */
static IndexScan *
_copyIndexScan(IndexScan *from)
{
	IndexScan  *newnode = makeNode(IndexScan);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyScanFields((Scan *) from, (Scan *) newnode);

	/*
	 * copy remainder of node
	 */
	newnode->indxid = listCopy(from->indxid);
	Node_Copy(from, newnode, indxqual);
	Node_Copy(from, newnode, indxqualorig);
	newnode->indxorderdir = from->indxorderdir;

	/*
	 * We must add subplans in index quals to the new plan's subPlan list
	 */
	if (from->scan.plan.subPlan != NIL)
	{
		newnode->scan.plan.subPlan = nconc(newnode->scan.plan.subPlan,
							  pull_subplans((Node *) newnode->indxqual));
		newnode->scan.plan.subPlan = nconc(newnode->scan.plan.subPlan,
						  pull_subplans((Node *) newnode->indxqualorig));
	}

	return newnode;
}

/* ----------------
 *				_copyTidScan
 * ----------------
 */
static TidScan *
_copyTidScan(TidScan *from)
{
	TidScan    *newnode = makeNode(TidScan);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyScanFields((Scan *) from, (Scan *) newnode);

	/*
	 * copy remainder of node
	 */
	newnode->needRescan = from->needRescan;
	Node_Copy(from, newnode, tideval);

	return newnode;
}

/* ----------------
 *		_copySubqueryScan
 * ----------------
 */
static SubqueryScan *
_copySubqueryScan(SubqueryScan *from)
{
	SubqueryScan *newnode = makeNode(SubqueryScan);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyScanFields((Scan *) from, (Scan *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, subplan);

	return newnode;
}

/* ----------------
 *		_copyFunctionScan
 * ----------------
 */
static FunctionScan *
_copyFunctionScan(FunctionScan *from)
{
	FunctionScan *newnode = makeNode(FunctionScan);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyScanFields((Scan *) from, (Scan *) newnode);

	return newnode;
}

/* ----------------
 *		CopyJoinFields
 *
 *		This function copies the fields of the Join node.  It is used by
 *		all the copy functions for classes which inherit from Join.
 * ----------------
 */
static void
CopyJoinFields(Join *from, Join *newnode)
{
	newnode->jointype = from->jointype;
	Node_Copy(from, newnode, joinqual);
	/* subPlan list must point to subplans in the new subtree, not the old */
	if (from->plan.subPlan != NIL)
		newnode->plan.subPlan = nconc(newnode->plan.subPlan,
							  pull_subplans((Node *) newnode->joinqual));
}


/* ----------------
 *		_copyJoin
 * ----------------
 */
static Join *
_copyJoin(Join *from)
{
	Join	   *newnode = makeNode(Join);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyJoinFields(from, newnode);

	return newnode;
}


/* ----------------
 *		_copyNestLoop
 * ----------------
 */
static NestLoop *
_copyNestLoop(NestLoop *from)
{
	NestLoop   *newnode = makeNode(NestLoop);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyJoinFields((Join *) from, (Join *) newnode);

	return newnode;
}


/* ----------------
 *		_copyMergeJoin
 * ----------------
 */
static MergeJoin *
_copyMergeJoin(MergeJoin *from)
{
	MergeJoin  *newnode = makeNode(MergeJoin);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyJoinFields((Join *) from, (Join *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, mergeclauses);

	/*
	 * We must add subplans in mergeclauses to the new plan's subPlan list
	 */
	if (from->join.plan.subPlan != NIL)
		newnode->join.plan.subPlan = nconc(newnode->join.plan.subPlan,
						  pull_subplans((Node *) newnode->mergeclauses));

	return newnode;
}

/* ----------------
 *		_copyHashJoin
 * ----------------
 */
static HashJoin *
_copyHashJoin(HashJoin *from)
{
	HashJoin   *newnode = makeNode(HashJoin);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);
	CopyJoinFields((Join *) from, (Join *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, hashclauses);
	newnode->hashjoinop = from->hashjoinop;

	/*
	 * We must add subplans in hashclauses to the new plan's subPlan list
	 */
	if (from->join.plan.subPlan != NIL)
		newnode->join.plan.subPlan = nconc(newnode->join.plan.subPlan,
						   pull_subplans((Node *) newnode->hashclauses));

	return newnode;
}


/* ----------------
 *		_copyMaterial
 * ----------------
 */
static Material *
_copyMaterial(Material *from)
{
	Material   *newnode = makeNode(Material);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	return newnode;
}


/* ----------------
 *		_copySort
 * ----------------
 */
static Sort *
_copySort(Sort *from)
{
	Sort	   *newnode = makeNode(Sort);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	newnode->keycount = from->keycount;

	return newnode;
}


/* ----------------
 *		_copyGroup
 * ----------------
 */
static Group *
_copyGroup(Group *from)
{
	Group	   *newnode = makeNode(Group);

	CopyPlanFields((Plan *) from, (Plan *) newnode);

	newnode->tuplePerGroup = from->tuplePerGroup;
	newnode->numCols = from->numCols;
	newnode->grpColIdx = palloc(from->numCols * sizeof(AttrNumber));
	memcpy(newnode->grpColIdx, from->grpColIdx, from->numCols * sizeof(AttrNumber));

	return newnode;
}

/* ---------------
 *	_copyAgg
 * --------------
 */
static Agg *
_copyAgg(Agg *from)
{
	Agg		   *newnode = makeNode(Agg);

	CopyPlanFields((Plan *) from, (Plan *) newnode);

	return newnode;
}

/* ---------------
 *	_copyGroupClause
 * --------------
 */
static GroupClause *
_copyGroupClause(GroupClause *from)
{
	GroupClause *newnode = makeNode(GroupClause);

	newnode->tleSortGroupRef = from->tleSortGroupRef;
	newnode->sortop = from->sortop;

	return newnode;
}

/* ----------------
 *		_copyUnique
 * ----------------
 */
static Unique *
_copyUnique(Unique *from)
{
	Unique	   *newnode = makeNode(Unique);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	/*
	 * copy remainder of node
	 */
	newnode->numCols = from->numCols;
	newnode->uniqColIdx = palloc(from->numCols * sizeof(AttrNumber));
	memcpy(newnode->uniqColIdx, from->uniqColIdx, from->numCols * sizeof(AttrNumber));

	return newnode;
}

/* ----------------
 *		_copySetOp
 * ----------------
 */
static SetOp *
_copySetOp(SetOp *from)
{
	SetOp	   *newnode = makeNode(SetOp);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	/*
	 * copy remainder of node
	 */
	newnode->cmd = from->cmd;
	newnode->numCols = from->numCols;
	newnode->dupColIdx = palloc(from->numCols * sizeof(AttrNumber));
	memcpy(newnode->dupColIdx, from->dupColIdx, from->numCols * sizeof(AttrNumber));
	newnode->flagColIdx = from->flagColIdx;

	return newnode;
}

/* ----------------
 *		_copyLimit
 * ----------------
 */
static Limit *
_copyLimit(Limit *from)
{
	Limit	   *newnode = makeNode(Limit);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, limitOffset);
	Node_Copy(from, newnode, limitCount);

	return newnode;
}

/* ----------------
 *		_copyHash
 * ----------------
 */
static Hash *
_copyHash(Hash *from)
{
	Hash	   *newnode = makeNode(Hash);

	/*
	 * copy node superclass fields
	 */
	CopyPlanFields((Plan *) from, (Plan *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, hashkey);

	return newnode;
}

static SubPlan *
_copySubPlan(SubPlan *from)
{
	SubPlan    *newnode = makeNode(SubPlan);

	Node_Copy(from, newnode, plan);
	newnode->plan_id = from->plan_id;
	Node_Copy(from, newnode, rtable);
	newnode->setParam = listCopy(from->setParam);
	newnode->parParam = listCopy(from->parParam);
	Node_Copy(from, newnode, sublink);

	/* do not copy execution state */
	newnode->needShutdown = false;
	newnode->curTuple = NULL;

	return newnode;
}

/* ****************************************************************
 *					   primnodes.h copy functions
 * ****************************************************************
 */

/* ----------------
 *		_copyResdom
 * ----------------
 */
static Resdom *
_copyResdom(Resdom *from)
{
	Resdom	   *newnode = makeNode(Resdom);

	newnode->resno = from->resno;
	newnode->restype = from->restype;
	newnode->restypmod = from->restypmod;
	if (from->resname != NULL)
		newnode->resname = pstrdup(from->resname);
	newnode->ressortgroupref = from->ressortgroupref;
	newnode->reskey = from->reskey;
	newnode->reskeyop = from->reskeyop;
	newnode->resjunk = from->resjunk;

	return newnode;
}

static Fjoin *
_copyFjoin(Fjoin *from)
{
	Fjoin	   *newnode = makeNode(Fjoin);

	/*
	 * copy node superclass fields
	 */

	newnode->fj_initialized = from->fj_initialized;
	newnode->fj_nNodes = from->fj_nNodes;

	Node_Copy(from, newnode, fj_innerNode);

	newnode->fj_results = (DatumPtr)
		palloc((from->fj_nNodes) * sizeof(Datum));
	memmove(from->fj_results,
			newnode->fj_results,
			(from->fj_nNodes) * sizeof(Datum));

	newnode->fj_alwaysDone = (BoolPtr)
		palloc((from->fj_nNodes) * sizeof(bool));
	memmove(from->fj_alwaysDone,
			newnode->fj_alwaysDone,
			(from->fj_nNodes) * sizeof(bool));


	return newnode;
}

/* ----------------
 *		_copyExpr
 * ----------------
 */
static Expr *
_copyExpr(Expr *from)
{
	Expr	   *newnode = makeNode(Expr);

	/*
	 * copy node superclass fields
	 */
	newnode->typeOid = from->typeOid;
	newnode->opType = from->opType;

	Node_Copy(from, newnode, oper);
	Node_Copy(from, newnode, args);

	return newnode;
}

/* ----------------
 *		_copyVar
 * ----------------
 */
static Var *
_copyVar(Var *from)
{
	Var		   *newnode = makeNode(Var);

	/*
	 * copy remainder of node
	 */
	newnode->varno = from->varno;
	newnode->varattno = from->varattno;
	newnode->vartype = from->vartype;
	newnode->vartypmod = from->vartypmod;
	newnode->varlevelsup = from->varlevelsup;

	newnode->varnoold = from->varnoold;
	newnode->varoattno = from->varoattno;

	return newnode;
}

/* ----------------
 *		_copyOper
 * ----------------
 */
static Oper *
_copyOper(Oper *from)
{
	Oper	   *newnode = makeNode(Oper);

	/*
	 * copy remainder of node
	 */
	newnode->opno = from->opno;
	newnode->opid = from->opid;
	newnode->opresulttype = from->opresulttype;
	newnode->opretset = from->opretset;
	/* Do not copy the run-time state, if any */
	newnode->op_fcache = NULL;

	return newnode;
}

/* ----------------
 *		_copyConst
 * ----------------
 */
static Const *
_copyConst(Const *from)
{
	Const	   *newnode = makeNode(Const);

	/*
	 * copy remainder of node
	 */
	newnode->consttype = from->consttype;
	newnode->constlen = from->constlen;

	if (from->constbyval || from->constisnull)
	{
		/*
		 * passed by value so just copy the datum. Also, don't try to copy
		 * struct when value is null!
		 *
		 */
		newnode->constvalue = from->constvalue;
	}
	else
	{
		/*
		 * not passed by value. datum contains a pointer.
		 */
		int			length = from->constlen;

		if (length == -1)		/* variable-length type? */
			length = VARSIZE(from->constvalue);
		newnode->constvalue = PointerGetDatum(palloc(length));
		memcpy(DatumGetPointer(newnode->constvalue),
			   DatumGetPointer(from->constvalue),
			   length);
	}

	newnode->constisnull = from->constisnull;
	newnode->constbyval = from->constbyval;
	newnode->constisset = from->constisset;
	newnode->constiscast = from->constiscast;

	return newnode;
}

/* ----------------
 *		_copyParam
 * ----------------
 */
static Param *
_copyParam(Param *from)
{
	Param	   *newnode = makeNode(Param);

	/*
	 * copy remainder of node
	 */
	newnode->paramkind = from->paramkind;
	newnode->paramid = from->paramid;

	if (from->paramname != NULL)
		newnode->paramname = pstrdup(from->paramname);
	newnode->paramtype = from->paramtype;

	return newnode;
}

/* ----------------
 *		_copyFunc
 * ----------------
 */
static Func *
_copyFunc(Func *from)
{
	Func	   *newnode = makeNode(Func);

	/*
	 * copy remainder of node
	 */
	newnode->funcid = from->funcid;
	newnode->funcresulttype = from->funcresulttype;
	newnode->funcretset = from->funcretset;
	/* Do not copy the run-time state, if any */
	newnode->func_fcache = NULL;

	return newnode;
}

/* ----------------
 *		_copyAggref
 * ----------------
 */
static Aggref *
_copyAggref(Aggref *from)
{
	Aggref	   *newnode = makeNode(Aggref);

	newnode->aggfnoid = from->aggfnoid;
	newnode->aggtype = from->aggtype;
	Node_Copy(from, newnode, target);
	newnode->aggstar = from->aggstar;
	newnode->aggdistinct = from->aggdistinct;
	newnode->aggno = from->aggno;		/* probably not needed */

	return newnode;
}

/* ----------------
 *		_copySubLink
 * ----------------
 */
static SubLink *
_copySubLink(SubLink *from)
{
	SubLink    *newnode = makeNode(SubLink);

	/*
	 * copy remainder of node
	 */
	newnode->subLinkType = from->subLinkType;
	newnode->useor = from->useor;
	Node_Copy(from, newnode, lefthand);
	Node_Copy(from, newnode, oper);
	Node_Copy(from, newnode, subselect);

	return newnode;
}

/* ----------------
 *		_copyFieldSelect
 * ----------------
 */
static FieldSelect *
_copyFieldSelect(FieldSelect *from)
{
	FieldSelect *newnode = makeNode(FieldSelect);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, arg);
	newnode->fieldnum = from->fieldnum;
	newnode->resulttype = from->resulttype;
	newnode->resulttypmod = from->resulttypmod;

	return newnode;
}

/* ----------------
 *		_copyRelabelType
 * ----------------
 */
static RelabelType *
_copyRelabelType(RelabelType *from)
{
	RelabelType *newnode = makeNode(RelabelType);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, arg);
	newnode->resulttype = from->resulttype;
	newnode->resulttypmod = from->resulttypmod;

	return newnode;
}

static RangeTblRef *
_copyRangeTblRef(RangeTblRef *from)
{
	RangeTblRef *newnode = makeNode(RangeTblRef);

	newnode->rtindex = from->rtindex;

	return newnode;
}

static FromExpr *
_copyFromExpr(FromExpr *from)
{
	FromExpr   *newnode = makeNode(FromExpr);

	Node_Copy(from, newnode, fromlist);
	Node_Copy(from, newnode, quals);

	return newnode;
}

static JoinExpr *
_copyJoinExpr(JoinExpr *from)
{
	JoinExpr   *newnode = makeNode(JoinExpr);

	newnode->jointype = from->jointype;
	newnode->isNatural = from->isNatural;
	Node_Copy(from, newnode, larg);
	Node_Copy(from, newnode, rarg);
	Node_Copy(from, newnode, using);
	Node_Copy(from, newnode, quals);
	Node_Copy(from, newnode, alias);
	newnode->rtindex = from->rtindex;

	return newnode;
}

/* ----------------
 *		_copyCaseExpr
 * ----------------
 */
static CaseExpr *
_copyCaseExpr(CaseExpr *from)
{
	CaseExpr   *newnode = makeNode(CaseExpr);

	/*
	 * copy remainder of node
	 */
	newnode->casetype = from->casetype;

	Node_Copy(from, newnode, arg);
	Node_Copy(from, newnode, args);
	Node_Copy(from, newnode, defresult);

	return newnode;
}

/* ----------------
 *		_copyCaseWhen
 * ----------------
 */
static CaseWhen *
_copyCaseWhen(CaseWhen *from)
{
	CaseWhen   *newnode = makeNode(CaseWhen);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, expr);
	Node_Copy(from, newnode, result);

	return newnode;
}

/* ----------------
 *		_copyNullTest
 * ----------------
 */
static NullTest *
_copyNullTest(NullTest *from)
{
	NullTest   *newnode = makeNode(NullTest);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, arg);
	newnode->nulltesttype = from->nulltesttype;

	return newnode;
}

/* ----------------
 *		_copyBooleanTest
 * ----------------
 */
static BooleanTest *
_copyBooleanTest(BooleanTest *from)
{
	BooleanTest *newnode = makeNode(BooleanTest);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, arg);
	newnode->booltesttype = from->booltesttype;

	return newnode;
}

static ArrayRef *
_copyArrayRef(ArrayRef *from)
{
	ArrayRef   *newnode = makeNode(ArrayRef);

	/*
	 * copy remainder of node
	 */
	newnode->refattrlength = from->refattrlength;
	newnode->refelemlength = from->refelemlength;
	newnode->refelemtype = from->refelemtype;
	newnode->refelembyval = from->refelembyval;

	Node_Copy(from, newnode, refupperindexpr);
	Node_Copy(from, newnode, reflowerindexpr);
	Node_Copy(from, newnode, refexpr);
	Node_Copy(from, newnode, refassgnexpr);

	return newnode;
}

/* ****************************************************************
 *						relation.h copy functions
 * ****************************************************************
 */

/* ----------------
 *		_copyRelOptInfo
 * ----------------
 */
static RelOptInfo *
_copyRelOptInfo(RelOptInfo *from)
{
	RelOptInfo *newnode = makeNode(RelOptInfo);

	newnode->reloptkind = from->reloptkind;

	newnode->relids = listCopy(from->relids);

	newnode->rows = from->rows;
	newnode->width = from->width;

	Node_Copy(from, newnode, targetlist);
	Node_Copy(from, newnode, pathlist);
	/* XXX cheapest-path fields should point to members of pathlist? */
	Node_Copy(from, newnode, cheapest_startup_path);
	Node_Copy(from, newnode, cheapest_total_path);
	newnode->pruneable = from->pruneable;

	newnode->rtekind = from->rtekind;
	Node_Copy(from, newnode, indexlist);
	newnode->pages = from->pages;
	newnode->tuples = from->tuples;
	Node_Copy(from, newnode, subplan);

	newnode->joinrti = from->joinrti;
	newnode->joinrteids = listCopy(from->joinrteids);

	Node_Copy(from, newnode, baserestrictinfo);
	newnode->baserestrictcost = from->baserestrictcost;
	newnode->outerjoinset = listCopy(from->outerjoinset);
	Node_Copy(from, newnode, joininfo);
	Node_Copy(from, newnode, innerjoin);

	return newnode;
}

/* ----------------
 *		_copyIndexOptInfo
 * ----------------
 */
static IndexOptInfo *
_copyIndexOptInfo(IndexOptInfo *from)
{
	IndexOptInfo *newnode = makeNode(IndexOptInfo);
	Size		len;

	newnode->indexoid = from->indexoid;
	newnode->pages = from->pages;
	newnode->tuples = from->tuples;

	newnode->ncolumns = from->ncolumns;
	newnode->nkeys = from->nkeys;

	if (from->classlist)
	{
		/* copy the trailing zero too */
		len = (from->ncolumns + 1) * sizeof(Oid);
		newnode->classlist = (Oid *) palloc(len);
		memcpy(newnode->classlist, from->classlist, len);
	}

	if (from->indexkeys)
	{
		/* copy the trailing zero too */
		len = (from->nkeys + 1) * sizeof(int);
		newnode->indexkeys = (int *) palloc(len);
		memcpy(newnode->indexkeys, from->indexkeys, len);
	}

	if (from->ordering)
	{
		/* copy the trailing zero too */
		len = (from->ncolumns + 1) * sizeof(Oid);
		newnode->ordering = (Oid *) palloc(len);
		memcpy(newnode->ordering, from->ordering, len);
	}

	newnode->relam = from->relam;
	newnode->amcostestimate = from->amcostestimate;
	newnode->indproc = from->indproc;
	Node_Copy(from, newnode, indpred);
	newnode->unique = from->unique;

	return newnode;
}

/* ----------------
 *		CopyPathFields
 *
 *		This function copies the fields of the Path node.  It is used by
 *		all the copy functions for classes which inherit from Path.
 * ----------------
 */
static void
CopyPathFields(Path *from, Path *newnode)
{
	/*
	 * Modify the next line, since it causes the copying to cycle (i.e.
	 * the parent points right back here! -- JMH, 7/7/92. Old version:
	 * Node_Copy(from, newnode, parent);
	 */
	newnode->parent = from->parent;

	newnode->startup_cost = from->startup_cost;
	newnode->total_cost = from->total_cost;

	newnode->pathtype = from->pathtype;

	Node_Copy(from, newnode, pathkeys);
}

/* ----------------
 *		_copyPath
 * ----------------
 */
static Path *
_copyPath(Path *from)
{
	Path	   *newnode = makeNode(Path);

	CopyPathFields(from, newnode);

	return newnode;
}

/* ----------------
 *		_copyIndexPath
 * ----------------
 */
static IndexPath *
_copyIndexPath(IndexPath *from)
{
	IndexPath  *newnode = makeNode(IndexPath);

	/*
	 * copy the node superclass fields
	 */
	CopyPathFields((Path *) from, (Path *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, indexinfo);
	Node_Copy(from, newnode, indexqual);
	newnode->indexscandir = from->indexscandir;
	newnode->joinrelids = listCopy(from->joinrelids);
	newnode->alljoinquals = from->alljoinquals;
	newnode->rows = from->rows;

	return newnode;
}

/* ----------------
 *				_copyTidPath
 * ----------------
 */
static TidPath *
_copyTidPath(TidPath *from)
{
	TidPath    *newnode = makeNode(TidPath);

	/*
	 * copy the node superclass fields
	 */
	CopyPathFields((Path *) from, (Path *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, tideval);
	newnode->unjoined_relids = listCopy(from->unjoined_relids);

	return newnode;
}

/* ----------------
 *				_copyAppendPath
 * ----------------
 */
static AppendPath *
_copyAppendPath(AppendPath *from)
{
	AppendPath *newnode = makeNode(AppendPath);

	/*
	 * copy the node superclass fields
	 */
	CopyPathFields((Path *) from, (Path *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, subpaths);

	return newnode;
}

/* ----------------
 *		CopyJoinPathFields
 *
 *		This function copies the fields of the JoinPath node.  It is used by
 *		all the copy functions for classes which inherit from JoinPath.
 * ----------------
 */
static void
CopyJoinPathFields(JoinPath *from, JoinPath *newnode)
{
	newnode->jointype = from->jointype;
	Node_Copy(from, newnode, outerjoinpath);
	Node_Copy(from, newnode, innerjoinpath);
	Node_Copy(from, newnode, joinrestrictinfo);
}

/* ----------------
 *		_copyNestPath
 * ----------------
 */
static NestPath *
_copyNestPath(NestPath *from)
{
	NestPath   *newnode = makeNode(NestPath);

	/*
	 * copy the node superclass fields
	 */
	CopyPathFields((Path *) from, (Path *) newnode);
	CopyJoinPathFields((JoinPath *) from, (JoinPath *) newnode);

	return newnode;
}

/* ----------------
 *		_copyMergePath
 * ----------------
 */
static MergePath *
_copyMergePath(MergePath *from)
{
	MergePath  *newnode = makeNode(MergePath);

	/*
	 * copy the node superclass fields
	 */
	CopyPathFields((Path *) from, (Path *) newnode);
	CopyJoinPathFields((JoinPath *) from, (JoinPath *) newnode);

	/*
	 * copy the remainder of the node
	 */
	Node_Copy(from, newnode, path_mergeclauses);
	Node_Copy(from, newnode, outersortkeys);
	Node_Copy(from, newnode, innersortkeys);

	return newnode;
}

/* ----------------
 *		_copyHashPath
 * ----------------
 */
static HashPath *
_copyHashPath(HashPath *from)
{
	HashPath   *newnode = makeNode(HashPath);

	/*
	 * copy the node superclass fields
	 */
	CopyPathFields((Path *) from, (Path *) newnode);
	CopyJoinPathFields((JoinPath *) from, (JoinPath *) newnode);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, path_hashclauses);

	return newnode;
}

/* ----------------
 *		_copyPathKeyItem
 * ----------------
 */
static PathKeyItem *
_copyPathKeyItem(PathKeyItem *from)
{
	PathKeyItem *newnode = makeNode(PathKeyItem);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, key);
	newnode->sortop = from->sortop;

	return newnode;
}

/* ----------------
 *		_copyRestrictInfo
 * ----------------
 */
static RestrictInfo *
_copyRestrictInfo(RestrictInfo *from)
{
	RestrictInfo *newnode = makeNode(RestrictInfo);

	/*
	 * copy remainder of node
	 */
	Node_Copy(from, newnode, clause);
	newnode->ispusheddown = from->ispusheddown;
	Node_Copy(from, newnode, subclauseindices);
	newnode->eval_cost = from->eval_cost;
	newnode->this_selec = from->this_selec;
	newnode->mergejoinoperator = from->mergejoinoperator;
	newnode->left_sortop = from->left_sortop;
	newnode->right_sortop = from->right_sortop;

	/*
	 * Do not copy pathkeys, since they'd not be canonical in a copied
	 * query
	 */
	newnode->left_pathkey = NIL;
	newnode->right_pathkey = NIL;
	newnode->left_mergescansel = from->left_mergescansel;
	newnode->right_mergescansel = from->right_mergescansel;
	newnode->hashjoinoperator = from->hashjoinoperator;
	newnode->left_bucketsize = from->left_bucketsize;
	newnode->right_bucketsize = from->right_bucketsize;

	return newnode;
}

/* ----------------
 *		_copyJoinInfo
 * ----------------
 */
static JoinInfo *
_copyJoinInfo(JoinInfo *from)
{
	JoinInfo   *newnode = makeNode(JoinInfo);

	/*
	 * copy remainder of node
	 */
	newnode->unjoined_relids = listCopy(from->unjoined_relids);
	Node_Copy(from, newnode, jinfo_restrictinfo);

	return newnode;
}

static Stream *
_copyStream(Stream *from)
{
	Stream	   *newnode = makeNode(Stream);

	newnode->pathptr = from->pathptr;
	newnode->cinfo = from->cinfo;
	newnode->clausetype = from->clausetype;

	newnode->upstream = (StreamPtr) NULL;		/* only copy nodes
												 * downwards! */
	Node_Copy(from, newnode, downstream);
	if (newnode->downstream)
		((Stream *) newnode->downstream)->upstream = (Stream *) newnode;

	newnode->groupup = from->groupup;
	newnode->groupcost = from->groupcost;
	newnode->groupsel = from->groupsel;

	return newnode;
}

/* ****************************************************************
 *					parsenodes.h copy functions
 * ****************************************************************
 */

static TargetEntry *
_copyTargetEntry(TargetEntry *from)
{
	TargetEntry *newnode = makeNode(TargetEntry);

	Node_Copy(from, newnode, resdom);
	Node_Copy(from, newnode, fjoin);
	Node_Copy(from, newnode, expr);
	return newnode;
}

static RangeTblEntry *
_copyRangeTblEntry(RangeTblEntry *from)
{
	RangeTblEntry *newnode = makeNode(RangeTblEntry);

	newnode->rtekind = from->rtekind;
	newnode->relid = from->relid;
	Node_Copy(from, newnode, subquery);
	Node_Copy(from, newnode, funcexpr);
	Node_Copy(from, newnode, coldeflist);
	newnode->jointype = from->jointype;
	Node_Copy(from, newnode, joinaliasvars);
	Node_Copy(from, newnode, alias);
	Node_Copy(from, newnode, eref);
	newnode->inh = from->inh;
	newnode->inFromCl = from->inFromCl;
	newnode->checkForRead = from->checkForRead;
	newnode->checkForWrite = from->checkForWrite;
	newnode->checkAsUser = from->checkAsUser;

	return newnode;
}

static FkConstraint *
_copyFkConstraint(FkConstraint *from)
{
	FkConstraint *newnode = makeNode(FkConstraint);

	if (from->constr_name)
		newnode->constr_name = pstrdup(from->constr_name);
	Node_Copy(from, newnode, pktable);
	Node_Copy(from, newnode, fk_attrs);
	Node_Copy(from, newnode, pk_attrs);
	newnode->fk_matchtype = from->fk_matchtype;
	newnode->fk_upd_action = from->fk_upd_action;
	newnode->fk_del_action = from->fk_del_action;
	newnode->deferrable = from->deferrable;
	newnode->initdeferred = from->initdeferred;
	newnode->skip_validation = from->skip_validation;

	return newnode;
}

static SortClause *
_copySortClause(SortClause *from)
{
	SortClause *newnode = makeNode(SortClause);

	newnode->tleSortGroupRef = from->tleSortGroupRef;
	newnode->sortop = from->sortop;

	return newnode;
}

static A_Expr *
_copyAExpr(A_Expr *from)
{
	A_Expr	   *newnode = makeNode(A_Expr);

	newnode->oper = from->oper;
	Node_Copy(from, newnode, name);
	Node_Copy(from, newnode, lexpr);
	Node_Copy(from, newnode, rexpr);

	return newnode;
}

static ColumnRef *
_copyColumnRef(ColumnRef *from)
{
	ColumnRef	   *newnode = makeNode(ColumnRef);

	Node_Copy(from, newnode, fields);
	Node_Copy(from, newnode, indirection);

	return newnode;
}

static ParamRef *
_copyParamRef(ParamRef *from)
{
	ParamRef    *newnode = makeNode(ParamRef);

	newnode->number = from->number;
	Node_Copy(from, newnode, fields);
	Node_Copy(from, newnode, indirection);

	return newnode;
}

static A_Const *
_copyAConst(A_Const *from)
{
	A_Const    *newnode = makeNode(A_Const);

	/* This part must duplicate _copyValue */
	newnode->val.type = from->val.type;
	switch (from->val.type)
	{
		case T_Integer:
			newnode->val.val.ival = from->val.val.ival;
			break;
		case T_Float:
		case T_String:
		case T_BitString:
			newnode->val.val.str = pstrdup(from->val.val.str);
			break;
		case T_Null:
			/* nothing to do */
			break;
		default:
			elog(ERROR, "_copyAConst: unknown node type %d", from->val.type);
			break;
	}

	Node_Copy(from, newnode, typename);

	return newnode;
}

static Ident *
_copyIdent(Ident *from)
{
	Ident	   *newnode = makeNode(Ident);

	newnode->name = pstrdup(from->name);

	return newnode;
}

static FuncCall *
_copyFuncCall(FuncCall *from)
{
	FuncCall   *newnode = makeNode(FuncCall);

	Node_Copy(from, newnode, funcname);
	Node_Copy(from, newnode, args);
	newnode->agg_star = from->agg_star;
	newnode->agg_distinct = from->agg_distinct;

	return newnode;
}

static A_Indices *
_copyAIndices(A_Indices *from)
{
	A_Indices  *newnode = makeNode(A_Indices);

	Node_Copy(from, newnode, lidx);
	Node_Copy(from, newnode, uidx);

	return newnode;
}

static ExprFieldSelect *
_copyExprFieldSelect(ExprFieldSelect *from)
{
	ExprFieldSelect	   *newnode = makeNode(ExprFieldSelect);

	Node_Copy(from, newnode, arg);
	Node_Copy(from, newnode, fields);
	Node_Copy(from, newnode, indirection);

	return newnode;
}

static ResTarget *
_copyResTarget(ResTarget *from)
{
	ResTarget  *newnode = makeNode(ResTarget);

	if (from->name)
		newnode->name = pstrdup(from->name);
	Node_Copy(from, newnode, indirection);
	Node_Copy(from, newnode, val);

	return newnode;
}

static TypeName *
_copyTypeName(TypeName *from)
{
	TypeName   *newnode = makeNode(TypeName);

	Node_Copy(from, newnode, names);
	newnode->typeid = from->typeid;
	newnode->timezone = from->timezone;
	newnode->setof = from->setof;
	newnode->pct_type = from->pct_type;
	newnode->typmod = from->typmod;
	Node_Copy(from, newnode, arrayBounds);

	return newnode;
}

static SortGroupBy *
_copySortGroupBy(SortGroupBy *from)
{
	SortGroupBy *newnode = makeNode(SortGroupBy);

	Node_Copy(from, newnode, useOp);
	Node_Copy(from, newnode, node);

	return newnode;
}

static Alias *
_copyAlias(Alias *from)
{
	Alias	   *newnode = makeNode(Alias);

	if (from->aliasname)
		newnode->aliasname = pstrdup(from->aliasname);
	Node_Copy(from, newnode, colnames);

	return newnode;
}

static RangeVar *
_copyRangeVar(RangeVar *from)
{
	RangeVar   *newnode = makeNode(RangeVar);

	if (from->catalogname)
		newnode->catalogname = pstrdup(from->catalogname);
	if (from->schemaname)
		newnode->schemaname = pstrdup(from->schemaname);
	if (from->relname)
		newnode->relname = pstrdup(from->relname);
	newnode->inhOpt = from->inhOpt;
	newnode->istemp = from->istemp;
	Node_Copy(from, newnode, alias);

	return newnode;
}

static RangeSubselect *
_copyRangeSubselect(RangeSubselect *from)
{
	RangeSubselect *newnode = makeNode(RangeSubselect);

	Node_Copy(from, newnode, subquery);
	Node_Copy(from, newnode, alias);

	return newnode;
}

static RangeFunction *
_copyRangeFunction(RangeFunction *from)
{
	RangeFunction   *newnode = makeNode(RangeFunction);

	Node_Copy(from, newnode, funccallnode);
	Node_Copy(from, newnode, alias);
	Node_Copy(from, newnode, coldeflist);

	return newnode;
}

static TypeCast *
_copyTypeCast(TypeCast *from)
{
	TypeCast   *newnode = makeNode(TypeCast);

	Node_Copy(from, newnode, arg);
	Node_Copy(from, newnode, typename);

	return newnode;
}

static IndexElem *
_copyIndexElem(IndexElem *from)
{
	IndexElem  *newnode = makeNode(IndexElem);

	if (from->name)
		newnode->name = pstrdup(from->name);
	Node_Copy(from, newnode, funcname);
	Node_Copy(from, newnode, args);
	Node_Copy(from, newnode, opclass);

	return newnode;
}

static ColumnDef *
_copyColumnDef(ColumnDef *from)
{
	ColumnDef  *newnode = makeNode(ColumnDef);

	if (from->colname)
		newnode->colname = pstrdup(from->colname);
	Node_Copy(from, newnode, typename);
	newnode->is_not_null = from->is_not_null;
	Node_Copy(from, newnode, raw_default);
	if (from->cooked_default)
		newnode->cooked_default = pstrdup(from->cooked_default);
	Node_Copy(from, newnode, constraints);
	Node_Copy(from, newnode, support);

	return newnode;
}

static Constraint *
_copyConstraint(Constraint *from)
{
	Constraint *newnode = makeNode(Constraint);

	newnode->contype = from->contype;
	if (from->name)
		newnode->name = pstrdup(from->name);
	Node_Copy(from, newnode, raw_expr);
	if (from->cooked_expr)
		newnode->cooked_expr = pstrdup(from->cooked_expr);
	Node_Copy(from, newnode, keys);

	return newnode;
}

static DefElem *
_copyDefElem(DefElem *from)
{
	DefElem    *newnode = makeNode(DefElem);

	if (from->defname)
		newnode->defname = pstrdup(from->defname);
	Node_Copy(from, newnode, arg);

	return newnode;
}

static Query *
_copyQuery(Query *from)
{
	Query	   *newnode = makeNode(Query);

	newnode->commandType = from->commandType;
	Node_Copy(from, newnode, utilityStmt);
	newnode->resultRelation = from->resultRelation;
	Node_Copy(from, newnode, into);
	newnode->isPortal = from->isPortal;
	newnode->isBinary = from->isBinary;
	newnode->hasAggs = from->hasAggs;
	newnode->hasSubLinks = from->hasSubLinks;
	newnode->originalQuery = from->originalQuery;

	Node_Copy(from, newnode, rtable);
	Node_Copy(from, newnode, jointree);

	newnode->rowMarks = listCopy(from->rowMarks);

	Node_Copy(from, newnode, targetList);

	Node_Copy(from, newnode, groupClause);
	Node_Copy(from, newnode, havingQual);
	Node_Copy(from, newnode, distinctClause);
	Node_Copy(from, newnode, sortClause);

	Node_Copy(from, newnode, limitOffset);
	Node_Copy(from, newnode, limitCount);

	Node_Copy(from, newnode, setOperations);

	newnode->resultRelations = listCopy(from->resultRelations);

	/*
	 * We do not copy the planner internal fields: base_rel_list,
	 * other_rel_list, join_rel_list, equi_key_list, query_pathkeys. Not
	 * entirely clear if this is right?
	 */

	return newnode;
}

static InsertStmt *
_copyInsertStmt(InsertStmt *from)
{
	InsertStmt *newnode = makeNode(InsertStmt);

	Node_Copy(from, newnode, relation);
	Node_Copy(from, newnode, cols);
	Node_Copy(from, newnode, targetList);
	Node_Copy(from, newnode, selectStmt);

	return newnode;
}

static DeleteStmt *
_copyDeleteStmt(DeleteStmt *from)
{
	DeleteStmt *newnode = makeNode(DeleteStmt);

	Node_Copy(from, newnode, relation);
	Node_Copy(from, newnode, whereClause);

	return newnode;
}

static UpdateStmt *
_copyUpdateStmt(UpdateStmt *from)
{
	UpdateStmt *newnode = makeNode(UpdateStmt);

	Node_Copy(from, newnode, relation);
	Node_Copy(from, newnode, targetList);
	Node_Copy(from, newnode, whereClause);
	Node_Copy(from, newnode, fromClause);

	return newnode;
}

static SelectStmt *
_copySelectStmt(SelectStmt *from)
{
	SelectStmt *newnode = makeNode(SelectStmt);

	Node_Copy(from, newnode, distinctClause);
	Node_Copy(from, newnode, into);
	Node_Copy(from, newnode, intoColNames);
	Node_Copy(from, newnode, targetList);
	Node_Copy(from, newnode, fromClause);
	Node_Copy(from, newnode, whereClause);
	Node_Copy(from, newnode, groupClause);
	Node_Copy(from, newnode, havingClause);
	Node_Copy(from, newnode, sortClause);
	if (from->portalname)
		newnode->portalname = pstrdup(from->portalname);
	newnode->binary = from->binary;
	Node_Copy(from, newnode, limitOffset);
	Node_Copy(from, newnode, limitCount);
	Node_Copy(from, newnode, forUpdate);
	newnode->op = from->op;
	newnode->all = from->all;
	Node_Copy(from, newnode, larg);
	Node_Copy(from, newnode, rarg);

	return newnode;
}

static SetOperationStmt *
_copySetOperationStmt(SetOperationStmt *from)
{
	SetOperationStmt *newnode = makeNode(SetOperationStmt);

	newnode->op = from->op;
	newnode->all = from->all;
	Node_Copy(from, newnode, larg);
	Node_Copy(from, newnode, rarg);
	newnode->colTypes = listCopy(from->colTypes);

	return newnode;
}

static AlterTableStmt *
_copyAlterTableStmt(AlterTableStmt *from)
{
	AlterTableStmt *newnode = makeNode(AlterTableStmt);

	newnode->subtype = from->subtype;
	Node_Copy(from, newnode, relation);
	if (from->name)
		newnode->name = pstrdup(from->name);
	Node_Copy(from, newnode, def);
	newnode->behavior = from->behavior;

	return newnode;
}

static GrantStmt *
_copyGrantStmt(GrantStmt *from)
{
	GrantStmt  *newnode = makeNode(GrantStmt);

	newnode->is_grant = from->is_grant;
	newnode->objtype = from->objtype;
	Node_Copy(from, newnode, objects);
	newnode->privileges = listCopy(from->privileges);
	Node_Copy(from, newnode, grantees);

	return newnode;
}

static PrivGrantee *
_copyPrivGrantee(PrivGrantee *from)
{
	PrivGrantee *newnode = makeNode(PrivGrantee);

	if (from->username)
		newnode->username = pstrdup(from->username);
	if (from->groupname)
		newnode->groupname = pstrdup(from->groupname);

	return newnode;
}

static FuncWithArgs *
_copyFuncWithArgs(FuncWithArgs *from)
{
	FuncWithArgs *newnode = makeNode(FuncWithArgs);

	Node_Copy(from, newnode, funcname);
	Node_Copy(from, newnode, funcargs);

	return newnode;
}

static InsertDefault *
_copyInsertDefault(InsertDefault *from)
{
	InsertDefault *newnode = makeNode(InsertDefault);

	return newnode;
}


static ClosePortalStmt *
_copyClosePortalStmt(ClosePortalStmt *from)
{
	ClosePortalStmt *newnode = makeNode(ClosePortalStmt);

	if (from->portalname)
		newnode->portalname = pstrdup(from->portalname);

	return newnode;
}

static ClusterStmt *
_copyClusterStmt(ClusterStmt *from)
{
	ClusterStmt *newnode = makeNode(ClusterStmt);

	Node_Copy(from, newnode, relation);
	if (from->indexname)
		newnode->indexname = pstrdup(from->indexname);

	return newnode;
}

static CopyStmt *
_copyCopyStmt(CopyStmt *from)
{
	CopyStmt   *newnode = makeNode(CopyStmt);

	Node_Copy(from, newnode, relation);
	Node_Copy(from, newnode, attlist);
	newnode->is_from = from->is_from;
	if (from->filename)
		newnode->filename = pstrdup(from->filename);
	Node_Copy(from, newnode, options);

	return newnode;
}

static CreateStmt *
_copyCreateStmt(CreateStmt *from)
{
	CreateStmt *newnode = makeNode(CreateStmt);

	Node_Copy(from, newnode, relation);
	Node_Copy(from, newnode, tableElts);
	Node_Copy(from, newnode, inhRelations);
	Node_Copy(from, newnode, constraints);
	newnode->hasoids = from->hasoids;

	return newnode;
}

static DefineStmt *
_copyDefineStmt(DefineStmt *from)
{
	DefineStmt *newnode = makeNode(DefineStmt);

	newnode->defType = from->defType;
	Node_Copy(from, newnode, defnames);
	Node_Copy(from, newnode, definition);

	return newnode;
}

static DropStmt *
_copyDropStmt(DropStmt *from)
{
	DropStmt   *newnode = makeNode(DropStmt);

	Node_Copy(from, newnode, objects);
	newnode->removeType = from->removeType;
	newnode->behavior = from->behavior;

	return newnode;
}

static TruncateStmt *
_copyTruncateStmt(TruncateStmt *from)
{
	TruncateStmt *newnode = makeNode(TruncateStmt);

	Node_Copy(from, newnode, relation);

	return newnode;
}

static CommentStmt *
_copyCommentStmt(CommentStmt *from)
{
	CommentStmt *newnode = makeNode(CommentStmt);

	newnode->objtype = from->objtype;
	Node_Copy(from, newnode, objname);
	Node_Copy(from, newnode, objargs);
	if (from->comment)
		newnode->comment = pstrdup(from->comment);

	return newnode;
}

static FetchStmt *
_copyFetchStmt(FetchStmt *from)
{
	FetchStmt  *newnode = makeNode(FetchStmt);

	newnode->direction = from->direction;
	newnode->howMany = from->howMany;
	newnode->portalname = pstrdup(from->portalname);
	newnode->ismove = from->ismove;

	return newnode;
}

static IndexStmt *
_copyIndexStmt(IndexStmt *from)
{
	IndexStmt  *newnode = makeNode(IndexStmt);

	newnode->idxname = pstrdup(from->idxname);
	Node_Copy(from, newnode, relation);
	newnode->accessMethod = pstrdup(from->accessMethod);
	Node_Copy(from, newnode, indexParams);
	Node_Copy(from, newnode, whereClause);
	Node_Copy(from, newnode, rangetable);
	newnode->unique = from->unique;
	newnode->primary = from->primary;
	newnode->isconstraint = from->isconstraint;

	return newnode;
}

static CreateFunctionStmt *
_copyCreateFunctionStmt(CreateFunctionStmt *from)
{
	CreateFunctionStmt *newnode = makeNode(CreateFunctionStmt);

	newnode->replace = from->replace;
	Node_Copy(from, newnode, funcname);
	Node_Copy(from, newnode, argTypes);
	Node_Copy(from, newnode, returnType);
	Node_Copy(from, newnode, options);
	Node_Copy(from, newnode, withClause);

	return newnode;
}

static RemoveAggrStmt *
_copyRemoveAggrStmt(RemoveAggrStmt *from)
{
	RemoveAggrStmt *newnode = makeNode(RemoveAggrStmt);

	Node_Copy(from, newnode, aggname);
	Node_Copy(from, newnode, aggtype);
	newnode->behavior = from->behavior;

	return newnode;
}

static RemoveFuncStmt *
_copyRemoveFuncStmt(RemoveFuncStmt *from)
{
	RemoveFuncStmt *newnode = makeNode(RemoveFuncStmt);

	Node_Copy(from, newnode, funcname);
	Node_Copy(from, newnode, args);
	newnode->behavior = from->behavior;

	return newnode;
}

static RemoveOperStmt *
_copyRemoveOperStmt(RemoveOperStmt *from)
{
	RemoveOperStmt *newnode = makeNode(RemoveOperStmt);

	Node_Copy(from, newnode, opname);
	Node_Copy(from, newnode, args);
	newnode->behavior = from->behavior;

	return newnode;
}

static RemoveOpClassStmt *
_copyRemoveOpClassStmt(RemoveOpClassStmt *from)
{
	RemoveOpClassStmt *newnode = makeNode(RemoveOpClassStmt);

	Node_Copy(from, newnode, opclassname);
	if (from->amname)
		newnode->amname = pstrdup(from->amname);
	newnode->behavior = from->behavior;

	return newnode;
}

static RenameStmt *
_copyRenameStmt(RenameStmt *from)
{
	RenameStmt *newnode = makeNode(RenameStmt);

	Node_Copy(from, newnode, relation);
	if (from->oldname)
		newnode->oldname = pstrdup(from->oldname);
	if (from->newname)
		newnode->newname = pstrdup(from->newname);
	newnode->renameType = from->renameType;

	return newnode;
}

static RuleStmt *
_copyRuleStmt(RuleStmt *from)
{
	RuleStmt   *newnode = makeNode(RuleStmt);

	Node_Copy(from, newnode, relation);
	newnode->rulename = pstrdup(from->rulename);
	Node_Copy(from, newnode, whereClause);
	newnode->event = from->event;
	newnode->instead = from->instead;
	Node_Copy(from, newnode, actions);

	return newnode;
}

static NotifyStmt *
_copyNotifyStmt(NotifyStmt *from)
{
	NotifyStmt *newnode = makeNode(NotifyStmt);

	Node_Copy(from, newnode, relation);

	return newnode;
}

static ListenStmt *
_copyListenStmt(ListenStmt *from)
{
	ListenStmt *newnode = makeNode(ListenStmt);

	Node_Copy(from, newnode, relation);

	return newnode;
}

static UnlistenStmt *
_copyUnlistenStmt(UnlistenStmt *from)
{
	UnlistenStmt *newnode = makeNode(UnlistenStmt);

	Node_Copy(from, newnode, relation);

	return newnode;
}

static TransactionStmt *
_copyTransactionStmt(TransactionStmt *from)
{
	TransactionStmt *newnode = makeNode(TransactionStmt);

	newnode->command = from->command;
	Node_Copy(from, newnode, options);

	return newnode;
}

static CompositeTypeStmt *
_copyCompositeTypeStmt(CompositeTypeStmt *from)
{
	CompositeTypeStmt   *newnode = makeNode(CompositeTypeStmt);

	Node_Copy(from, newnode, typevar);
	Node_Copy(from, newnode, coldeflist);

	return newnode;
}

static ViewStmt *
_copyViewStmt(ViewStmt *from)
{
	ViewStmt   *newnode = makeNode(ViewStmt);

	Node_Copy(from, newnode, view);
	Node_Copy(from, newnode, aliases);
	Node_Copy(from, newnode, query);

	return newnode;
}

static LoadStmt *
_copyLoadStmt(LoadStmt *from)
{
	LoadStmt   *newnode = makeNode(LoadStmt);

	if (from->filename)
		newnode->filename = pstrdup(from->filename);

	return newnode;
}

static CreateDomainStmt *
_copyCreateDomainStmt(CreateDomainStmt *from)
{
	CreateDomainStmt *newnode = makeNode(CreateDomainStmt);

	Node_Copy(from, newnode, domainname);
	Node_Copy(from, newnode, typename);
	Node_Copy(from, newnode, constraints);

	return newnode;
}

static CreateOpClassStmt *
_copyCreateOpClassStmt(CreateOpClassStmt *from)
{
	CreateOpClassStmt *newnode = makeNode(CreateOpClassStmt);

	Node_Copy(from, newnode, opclassname);
	if (from->amname)
		newnode->amname = pstrdup(from->amname);
	Node_Copy(from, newnode, datatype);
	Node_Copy(from, newnode, items);
	newnode->isDefault = from->isDefault;

	return newnode;
}

static CreateOpClassItem *
_copyCreateOpClassItem(CreateOpClassItem *from)
{
	CreateOpClassItem *newnode = makeNode(CreateOpClassItem);

	newnode->itemtype = from->itemtype;
	Node_Copy(from, newnode, name);
	Node_Copy(from, newnode, args);
	newnode->number = from->number;
	newnode->recheck = from->recheck;
	Node_Copy(from, newnode, storedtype);

	return newnode;
}

static CreatedbStmt *
_copyCreatedbStmt(CreatedbStmt *from)
{
	CreatedbStmt *newnode = makeNode(CreatedbStmt);

	if (from->dbname)
		newnode->dbname = pstrdup(from->dbname);
	Node_Copy(from, newnode, options);

	return newnode;
}

static AlterDatabaseSetStmt *
_copyAlterDatabaseSetStmt(AlterDatabaseSetStmt *from)
{
	AlterDatabaseSetStmt *newnode = makeNode(AlterDatabaseSetStmt);

	if (from->dbname)
		newnode->dbname = pstrdup(from->dbname);
	if (from->variable)
		newnode->variable = pstrdup(from->variable);
	Node_Copy(from, newnode, value);

	return newnode;
}

static DropdbStmt *
_copyDropdbStmt(DropdbStmt *from)
{
	DropdbStmt *newnode = makeNode(DropdbStmt);

	if (from->dbname)
		newnode->dbname = pstrdup(from->dbname);

	return newnode;
}

static VacuumStmt *
_copyVacuumStmt(VacuumStmt *from)
{
	VacuumStmt *newnode = makeNode(VacuumStmt);

	newnode->vacuum = from->vacuum;
	newnode->full = from->full;
	newnode->analyze = from->analyze;
	newnode->freeze = from->freeze;
	newnode->verbose = from->verbose;
	Node_Copy(from, newnode, relation);
	Node_Copy(from, newnode, va_cols);

	return newnode;
}

static ExplainStmt *
_copyExplainStmt(ExplainStmt *from)
{
	ExplainStmt *newnode = makeNode(ExplainStmt);

	Node_Copy(from, newnode, query);
	newnode->verbose = from->verbose;
	newnode->analyze = from->analyze;

	return newnode;
}

static CreateSeqStmt *
_copyCreateSeqStmt(CreateSeqStmt *from)
{
	CreateSeqStmt *newnode = makeNode(CreateSeqStmt);

	Node_Copy(from, newnode, sequence);
	Node_Copy(from, newnode, options);

	return newnode;
}

static VariableSetStmt *
_copyVariableSetStmt(VariableSetStmt *from)
{
	VariableSetStmt *newnode = makeNode(VariableSetStmt);

	if (from->name)
		newnode->name = pstrdup(from->name);
	Node_Copy(from, newnode, args);
	newnode->is_local = from->is_local;

	return newnode;
}

static VariableShowStmt *
_copyVariableShowStmt(VariableShowStmt *from)
{
	VariableShowStmt *newnode = makeNode(VariableShowStmt);

	if (from->name)
		newnode->name = pstrdup(from->name);

	return newnode;
}

static VariableResetStmt *
_copyVariableResetStmt(VariableResetStmt *from)
{
	VariableResetStmt *newnode = makeNode(VariableResetStmt);

	if (from->name)
		newnode->name = pstrdup(from->name);

	return newnode;
}

static CreateTrigStmt *
_copyCreateTrigStmt(CreateTrigStmt *from)
{
	CreateTrigStmt *newnode = makeNode(CreateTrigStmt);

	if (from->trigname)
		newnode->trigname = pstrdup(from->trigname);
	Node_Copy(from, newnode, relation);
	Node_Copy(from, newnode, funcname);
	Node_Copy(from, newnode, args);
	newnode->before = from->before;
	newnode->row = from->row;
	memcpy(newnode->actions, from->actions, sizeof(from->actions));
	if (from->lang)
		newnode->lang = pstrdup(from->lang);
	if (from->text)
		newnode->text = pstrdup(from->text);

	Node_Copy(from, newnode, attr);
	if (from->when)
		newnode->when = pstrdup(from->when);
	newnode->isconstraint = from->isconstraint;
	newnode->deferrable = from->deferrable;
	newnode->initdeferred = from->initdeferred;
	Node_Copy(from, newnode, constrrel);

	return newnode;
}

static DropPropertyStmt *
_copyDropPropertyStmt(DropPropertyStmt *from)
{
	DropPropertyStmt *newnode = makeNode(DropPropertyStmt);

	Node_Copy(from, newnode, relation);
	if (from->property)
		newnode->property = pstrdup(from->property);
	newnode->removeType = from->removeType;
	newnode->behavior = from->behavior;

	return newnode;
}

static CreatePLangStmt *
_copyCreatePLangStmt(CreatePLangStmt *from)
{
	CreatePLangStmt *newnode = makeNode(CreatePLangStmt);

	if (from->plname)
		newnode->plname = pstrdup(from->plname);
	Node_Copy(from, newnode, plhandler);
	Node_Copy(from, newnode, plvalidator);
	newnode->pltrusted = from->pltrusted;

	return newnode;
}

static DropPLangStmt *
_copyDropPLangStmt(DropPLangStmt *from)
{
	DropPLangStmt *newnode = makeNode(DropPLangStmt);

	if (from->plname)
		newnode->plname = pstrdup(from->plname);
	newnode->behavior = from->behavior;

	return newnode;
}

static CreateUserStmt *
_copyCreateUserStmt(CreateUserStmt *from)
{
	CreateUserStmt *newnode = makeNode(CreateUserStmt);

	if (from->user)
		newnode->user = pstrdup(from->user);
	Node_Copy(from, newnode, options);

	return newnode;
}

static AlterUserStmt *
_copyAlterUserStmt(AlterUserStmt *from)
{
	AlterUserStmt *newnode = makeNode(AlterUserStmt);

	if (from->user)
		newnode->user = pstrdup(from->user);
	Node_Copy(from, newnode, options);

	return newnode;
}

static AlterUserSetStmt *
_copyAlterUserSetStmt(AlterUserSetStmt *from)
{
	AlterUserSetStmt *newnode = makeNode(AlterUserSetStmt);

	if (from->user)
		newnode->user = pstrdup(from->user);
	if (from->variable)
		newnode->variable = pstrdup(from->variable);
	Node_Copy(from, newnode, value);

	return newnode;
}

static DropUserStmt *
_copyDropUserStmt(DropUserStmt *from)
{
	DropUserStmt *newnode = makeNode(DropUserStmt);

	Node_Copy(from, newnode, users);

	return newnode;
}

static LockStmt *
_copyLockStmt(LockStmt *from)
{
	LockStmt   *newnode = makeNode(LockStmt);

	Node_Copy(from, newnode, relations);

	newnode->mode = from->mode;

	return newnode;
}

static ConstraintsSetStmt *
_copyConstraintsSetStmt(ConstraintsSetStmt *from)
{
	ConstraintsSetStmt *newnode = makeNode(ConstraintsSetStmt);

	Node_Copy(from, newnode, constraints);
	newnode->deferred = from->deferred;

	return newnode;
}

static CreateGroupStmt *
_copyCreateGroupStmt(CreateGroupStmt *from)
{
	CreateGroupStmt *newnode = makeNode(CreateGroupStmt);

	if (from->name)
		newnode->name = pstrdup(from->name);
	Node_Copy(from, newnode, options);

	return newnode;
}

static AlterGroupStmt *
_copyAlterGroupStmt(AlterGroupStmt *from)
{
	AlterGroupStmt *newnode = makeNode(AlterGroupStmt);

	if (from->name)
		newnode->name = pstrdup(from->name);
	newnode->action = from->action;
	Node_Copy(from, newnode, listUsers);

	return newnode;
}

static DropGroupStmt *
_copyDropGroupStmt(DropGroupStmt *from)
{
	DropGroupStmt *newnode = makeNode(DropGroupStmt);

	if (from->name)
		newnode->name = pstrdup(from->name);

	return newnode;
}

static ReindexStmt *
_copyReindexStmt(ReindexStmt *from)
{
	ReindexStmt *newnode = makeNode(ReindexStmt);

	newnode->reindexType = from->reindexType;
	Node_Copy(from, newnode, relation);
	if (from->name)
		newnode->name = pstrdup(from->name);
	newnode->force = from->force;
	newnode->all = from->all;

	return newnode;
}

static CreateSchemaStmt *
_copyCreateSchemaStmt(CreateSchemaStmt *from)
{
	CreateSchemaStmt *newnode = makeNode(CreateSchemaStmt);

	newnode->schemaname = pstrdup(from->schemaname);
	if (from->authid)
		newnode->authid = pstrdup(from->authid);
	Node_Copy(from, newnode, schemaElts);

	return newnode;
}

static CreateConversionStmt *
_copyCreateConversionStmt(CreateConversionStmt *from)
{
	CreateConversionStmt *newnode = makeNode(CreateConversionStmt);

	Node_Copy(from, newnode, conversion_name);
	newnode->for_encoding_name = pstrdup(from->for_encoding_name);
	newnode->to_encoding_name = pstrdup(from->to_encoding_name);
	Node_Copy(from, newnode, func_name);
	newnode->def = from->def;

	return newnode;
}

static CreateCastStmt *
_copyCreateCastStmt(CreateCastStmt *from)
{
	CreateCastStmt *newnode = makeNode(CreateCastStmt);

	Node_Copy(from, newnode, sourcetype);
	Node_Copy(from, newnode, targettype);
	Node_Copy(from, newnode, func);
	newnode->implicit = from->implicit;

	return newnode;
}

static DropCastStmt *
_copyDropCastStmt(DropCastStmt *from)
{
	DropCastStmt *newnode = makeNode(DropCastStmt);

	Node_Copy(from, newnode, sourcetype);
	Node_Copy(from, newnode, targettype);
	newnode->behavior = from->behavior;

	return newnode;
}


/* ****************************************************************
 *					pg_list.h copy functions
 * ****************************************************************
 */

static Value *
_copyValue(Value *from)
{
	Value	   *newnode = makeNode(Value);

	/* See also _copyAConst when changing this code! */

	newnode->type = from->type;
	switch (from->type)
	{
		case T_Integer:
			newnode->val.ival = from->val.ival;
			break;
		case T_Float:
		case T_String:
		case T_BitString:
			newnode->val.str = pstrdup(from->val.str);
			break;
		case T_Null:
			/* nothing to do */
			break;
		default:
			elog(ERROR, "_copyValue: unknown node type %d", from->type);
			break;
	}
	return newnode;
}

/* ----------------
 *		copyObject returns a copy of the node or list. If it is a list, it
 *		recursively copies its items.
 * ----------------
 */
void *
copyObject(void *from)
{
	void	   *retval;

	if (from == NULL)
		return NULL;

	switch (nodeTag(from))
	{
			/*
			 * PLAN NODES
			 */
		case T_Plan:
			retval = _copyPlan(from);
			break;
		case T_Result:
			retval = _copyResult(from);
			break;
		case T_Append:
			retval = _copyAppend(from);
			break;
		case T_Scan:
			retval = _copyScan(from);
			break;
		case T_SeqScan:
			retval = _copySeqScan(from);
			break;
		case T_IndexScan:
			retval = _copyIndexScan(from);
			break;
		case T_TidScan:
			retval = _copyTidScan(from);
			break;
		case T_SubqueryScan:
			retval = _copySubqueryScan(from);
			break;
		case T_FunctionScan:
			retval = _copyFunctionScan(from);
			break;
		case T_Join:
			retval = _copyJoin(from);
			break;
		case T_NestLoop:
			retval = _copyNestLoop(from);
			break;
		case T_MergeJoin:
			retval = _copyMergeJoin(from);
			break;
		case T_HashJoin:
			retval = _copyHashJoin(from);
			break;
		case T_Material:
			retval = _copyMaterial(from);
			break;
		case T_Sort:
			retval = _copySort(from);
			break;
		case T_Group:
			retval = _copyGroup(from);
			break;
		case T_Agg:
			retval = _copyAgg(from);
			break;
		case T_Unique:
			retval = _copyUnique(from);
			break;
		case T_SetOp:
			retval = _copySetOp(from);
			break;
		case T_Limit:
			retval = _copyLimit(from);
			break;
		case T_Hash:
			retval = _copyHash(from);
			break;
		case T_SubPlan:
			retval = _copySubPlan(from);
			break;

			/*
			 * PRIMITIVE NODES
			 */
		case T_Resdom:
			retval = _copyResdom(from);
			break;
		case T_Fjoin:
			retval = _copyFjoin(from);
			break;
		case T_Expr:
			retval = _copyExpr(from);
			break;
		case T_Var:
			retval = _copyVar(from);
			break;
		case T_Oper:
			retval = _copyOper(from);
			break;
		case T_Const:
			retval = _copyConst(from);
			break;
		case T_Param:
			retval = _copyParam(from);
			break;
		case T_Aggref:
			retval = _copyAggref(from);
			break;
		case T_SubLink:
			retval = _copySubLink(from);
			break;
		case T_Func:
			retval = _copyFunc(from);
			break;
		case T_ArrayRef:
			retval = _copyArrayRef(from);
			break;
		case T_FieldSelect:
			retval = _copyFieldSelect(from);
			break;
		case T_RelabelType:
			retval = _copyRelabelType(from);
			break;
		case T_RangeTblRef:
			retval = _copyRangeTblRef(from);
			break;
		case T_FromExpr:
			retval = _copyFromExpr(from);
			break;
		case T_JoinExpr:
			retval = _copyJoinExpr(from);
			break;

			/*
			 * RELATION NODES
			 */
		case T_RelOptInfo:
			retval = _copyRelOptInfo(from);
			break;
		case T_Path:
			retval = _copyPath(from);
			break;
		case T_IndexPath:
			retval = _copyIndexPath(from);
			break;
		case T_TidPath:
			retval = _copyTidPath(from);
			break;
		case T_AppendPath:
			retval = _copyAppendPath(from);
			break;
		case T_NestPath:
			retval = _copyNestPath(from);
			break;
		case T_MergePath:
			retval = _copyMergePath(from);
			break;
		case T_HashPath:
			retval = _copyHashPath(from);
			break;
		case T_PathKeyItem:
			retval = _copyPathKeyItem(from);
			break;
		case T_RestrictInfo:
			retval = _copyRestrictInfo(from);
			break;
		case T_JoinInfo:
			retval = _copyJoinInfo(from);
			break;
		case T_Stream:
			retval = _copyStream(from);
			break;
		case T_IndexOptInfo:
			retval = _copyIndexOptInfo(from);
			break;

			/*
			 * VALUE NODES
			 */
		case T_Integer:
		case T_Float:
		case T_String:
		case T_BitString:
		case T_Null:
			retval = _copyValue(from);
			break;
		case T_List:
			{
				List	   *list = from,
						   *l,
						   *nl;

				/* rather ugly coding for speed... */
				/* Note the input list cannot be NIL if we got here. */
				nl = makeList1(copyObject(lfirst(list)));
				retval = nl;

				foreach(l, lnext(list))
				{
					lnext(nl) = makeList1(copyObject(lfirst(l)));
					nl = lnext(nl);
				}
			}
			break;

			/*
			 * PARSE NODES
			 */
		case T_Query:
			retval = _copyQuery(from);
			break;
		case T_InsertStmt:
			retval = _copyInsertStmt(from);
			break;
		case T_DeleteStmt:
			retval = _copyDeleteStmt(from);
			break;
		case T_UpdateStmt:
			retval = _copyUpdateStmt(from);
			break;
		case T_SelectStmt:
			retval = _copySelectStmt(from);
			break;
		case T_SetOperationStmt:
			retval = _copySetOperationStmt(from);
			break;
		case T_AlterTableStmt:
			retval = _copyAlterTableStmt(from);
			break;
		case T_GrantStmt:
			retval = _copyGrantStmt(from);
			break;
		case T_ClosePortalStmt:
			retval = _copyClosePortalStmt(from);
			break;
		case T_ClusterStmt:
			retval = _copyClusterStmt(from);
			break;
		case T_CopyStmt:
			retval = _copyCopyStmt(from);
			break;
		case T_CreateStmt:
			retval = _copyCreateStmt(from);
			break;
		case T_DefineStmt:
			retval = _copyDefineStmt(from);
			break;
		case T_DropStmt:
			retval = _copyDropStmt(from);
			break;
		case T_TruncateStmt:
			retval = _copyTruncateStmt(from);
			break;
		case T_CommentStmt:
			retval = _copyCommentStmt(from);
			break;
		case T_FetchStmt:
			retval = _copyFetchStmt(from);
			break;
		case T_IndexStmt:
			retval = _copyIndexStmt(from);
			break;
		case T_CreateFunctionStmt:
			retval = _copyCreateFunctionStmt(from);
			break;
		case T_RemoveAggrStmt:
			retval = _copyRemoveAggrStmt(from);
			break;
		case T_RemoveFuncStmt:
			retval = _copyRemoveFuncStmt(from);
			break;
		case T_RemoveOperStmt:
			retval = _copyRemoveOperStmt(from);
			break;
		case T_RemoveOpClassStmt:
			retval = _copyRemoveOpClassStmt(from);
			break;
		case T_RenameStmt:
			retval = _copyRenameStmt(from);
			break;
		case T_RuleStmt:
			retval = _copyRuleStmt(from);
			break;
		case T_NotifyStmt:
			retval = _copyNotifyStmt(from);
			break;
		case T_ListenStmt:
			retval = _copyListenStmt(from);
			break;
		case T_UnlistenStmt:
			retval = _copyUnlistenStmt(from);
			break;
		case T_TransactionStmt:
			retval = _copyTransactionStmt(from);
			break;
		case T_CompositeTypeStmt:
			retval = _copyCompositeTypeStmt(from);
			break;
		case T_ViewStmt:
			retval = _copyViewStmt(from);
			break;
		case T_LoadStmt:
			retval = _copyLoadStmt(from);
			break;
		case T_CreateDomainStmt:
			retval = _copyCreateDomainStmt(from);
			break;
		case T_CreateOpClassStmt:
			retval = _copyCreateOpClassStmt(from);
			break;
		case T_CreateOpClassItem:
			retval = _copyCreateOpClassItem(from);
			break;
		case T_CreatedbStmt:
			retval = _copyCreatedbStmt(from);
			break;
		case T_AlterDatabaseSetStmt:
			retval = _copyAlterDatabaseSetStmt(from);
			break;
		case T_DropdbStmt:
			retval = _copyDropdbStmt(from);
			break;
		case T_VacuumStmt:
			retval = _copyVacuumStmt(from);
			break;
		case T_ExplainStmt:
			retval = _copyExplainStmt(from);
			break;
		case T_CreateSeqStmt:
			retval = _copyCreateSeqStmt(from);
			break;
		case T_VariableSetStmt:
			retval = _copyVariableSetStmt(from);
			break;
		case T_VariableShowStmt:
			retval = _copyVariableShowStmt(from);
			break;
		case T_VariableResetStmt:
			retval = _copyVariableResetStmt(from);
			break;
		case T_CreateTrigStmt:
			retval = _copyCreateTrigStmt(from);
			break;
		case T_DropPropertyStmt:
			retval = _copyDropPropertyStmt(from);
			break;
		case T_CreatePLangStmt:
			retval = _copyCreatePLangStmt(from);
			break;
		case T_DropPLangStmt:
			retval = _copyDropPLangStmt(from);
			break;
		case T_CreateUserStmt:
			retval = _copyCreateUserStmt(from);
			break;
		case T_AlterUserStmt:
			retval = _copyAlterUserStmt(from);
			break;
		case T_AlterUserSetStmt:
			retval = _copyAlterUserSetStmt(from);
			break;
		case T_DropUserStmt:
			retval = _copyDropUserStmt(from);
			break;
		case T_LockStmt:
			retval = _copyLockStmt(from);
			break;
		case T_ConstraintsSetStmt:
			retval = _copyConstraintsSetStmt(from);
			break;
		case T_CreateGroupStmt:
			retval = _copyCreateGroupStmt(from);
			break;
		case T_AlterGroupStmt:
			retval = _copyAlterGroupStmt(from);
			break;
		case T_DropGroupStmt:
			retval = _copyDropGroupStmt(from);
			break;
		case T_ReindexStmt:
			retval = _copyReindexStmt(from);
			break;
		case T_CheckPointStmt:
			retval = (void *) makeNode(CheckPointStmt);
			break;
		case T_CreateSchemaStmt:
			retval = _copyCreateSchemaStmt(from);
			break;
		case T_CreateConversionStmt:
			retval = _copyCreateConversionStmt(from);
			break;
		case T_CreateCastStmt:
			retval = _copyCreateCastStmt(from);
			break;
		case T_DropCastStmt:
			retval = _copyDropCastStmt(from);
			break;

		case T_A_Expr:
			retval = _copyAExpr(from);
			break;
		case T_ColumnRef:
			retval = _copyColumnRef(from);
			break;
		case T_ParamRef:
			retval = _copyParamRef(from);
			break;
		case T_A_Const:
			retval = _copyAConst(from);
			break;
		case T_Ident:
			retval = _copyIdent(from);
			break;
		case T_FuncCall:
			retval = _copyFuncCall(from);
			break;
		case T_A_Indices:
			retval = _copyAIndices(from);
			break;
		case T_ExprFieldSelect:
			retval = _copyExprFieldSelect(from);
			break;
		case T_ResTarget:
			retval = _copyResTarget(from);
			break;
		case T_TypeCast:
			retval = _copyTypeCast(from);
			break;
		case T_SortGroupBy:
			retval = _copySortGroupBy(from);
			break;
		case T_Alias:
			retval = _copyAlias(from);
			break;
		case T_RangeVar:
			retval = _copyRangeVar(from);
			break;
		case T_RangeSubselect:
			retval = _copyRangeSubselect(from);
			break;
		case T_RangeFunction:
			retval = _copyRangeFunction(from);
			break;
		case T_TypeName:
			retval = _copyTypeName(from);
			break;
		case T_IndexElem:
			retval = _copyIndexElem(from);
			break;
		case T_ColumnDef:
			retval = _copyColumnDef(from);
			break;
		case T_Constraint:
			retval = _copyConstraint(from);
			break;
		case T_DefElem:
			retval = _copyDefElem(from);
			break;
		case T_TargetEntry:
			retval = _copyTargetEntry(from);
			break;
		case T_RangeTblEntry:
			retval = _copyRangeTblEntry(from);
			break;
		case T_SortClause:
			retval = _copySortClause(from);
			break;
		case T_GroupClause:
			retval = _copyGroupClause(from);
			break;
		case T_CaseExpr:
			retval = _copyCaseExpr(from);
			break;
		case T_CaseWhen:
			retval = _copyCaseWhen(from);
			break;
		case T_NullTest:
			retval = _copyNullTest(from);
			break;
		case T_BooleanTest:
			retval = _copyBooleanTest(from);
			break;
		case T_FkConstraint:
			retval = _copyFkConstraint(from);
			break;
		case T_PrivGrantee:
			retval = _copyPrivGrantee(from);
			break;
		case T_FuncWithArgs:
			retval = _copyFuncWithArgs(from);
			break;
		case T_InsertDefault:
			retval = _copyInsertDefault(from);
			break;

		default:
			elog(ERROR, "copyObject: don't know how to copy node type %d",
				 nodeTag(from));
			retval = from;		/* keep compiler quiet */
			break;
	}
	return retval;
}
