/*-------------------------------------------------------------------------
 *
 * nodes.h
 *	  Definitions for tagged nodes.
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: nodes.h,v 1.128 2002-12-06 03:43:28 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODES_H
#define NODES_H

/*
 * The first field of every node is NodeTag. Each node created (with makeNode)
 * will have one of the following tags as the value of its first field.
 *
 * Note that the numbers of the node tags are not contiguous. We left holes
 * here so that we can add more tags without changing the existing enum's.
 * (Since node tag numbers never exist outside backend memory, there's no
 * real harm in renumbering, it just costs a full rebuild ...)
 */
typedef enum NodeTag
{
	T_Invalid = 0,

	/*
	 * TAGS FOR EXECUTOR NODES (execnodes.h)
	 */
	T_IndexInfo = 10,
	T_ResultRelInfo,
	T_TupleTableSlot,
	T_ExprContext,
	T_ProjectionInfo,
	T_JunkFilter,
	T_EState,

	/*
	 * TAGS FOR PLAN NODES (plannodes.h)
	 */
	T_Plan = 100,
	T_Result,
	T_Append,
	T_Scan,
	T_SeqScan,
	T_IndexScan,
	T_TidScan,
	T_SubqueryScan,
	T_FunctionScan,
	T_Join,
	T_NestLoop,
	T_MergeJoin,
	T_HashJoin,
	T_Material,
	T_Sort,
	T_Group,
	T_Agg,
	T_Unique,
	T_Hash,
	T_SetOp,
	T_Limit,
	T_SubPlan,

	/*
	 * TAGS FOR PLAN STATE NODES (execnodes.h)
	 *
	 * These should correspond one-to-one with Plan node types.
	 */
	T_PlanState = 200,
	T_ResultState,
	T_AppendState,
	T_ScanState,
	T_SeqScanState,
	T_IndexScanState,
	T_TidScanState,
	T_SubqueryScanState,
	T_FunctionScanState,
	T_JoinState,
	T_NestLoopState,
	T_MergeJoinState,
	T_HashJoinState,
	T_MaterialState,
	T_SortState,
	T_GroupState,
	T_AggState,
	T_UniqueState,
	T_HashState,
	T_SetOpState,
	T_LimitState,
	T_SubPlanState,

	/*
	 * TAGS FOR PRIMITIVE NODES (primnodes.h)
	 */
	T_Resdom = 300,
	T_Fjoin,
	T_Expr,
	T_Var,
	T_Oper,
	T_Const,
	T_Param,
	T_Aggref,
	T_SubLink,
	T_Func,
	T_FieldSelect,
	T_ArrayRef,
	T_RelabelType,
	T_RangeTblRef,
	T_FromExpr,
	T_JoinExpr,

	/*
	 * TAGS FOR PLANNER NODES (relation.h)
	 */
	T_RelOptInfo = 400,
	T_IndexOptInfo,
	T_Path,
	T_IndexPath,
	T_NestPath,
	T_MergePath,
	T_HashPath,
	T_TidPath,
	T_AppendPath,
	T_ResultPath,
	T_MaterialPath,
	T_PathKeyItem,
	T_RestrictInfo,
	T_JoinInfo,
	T_InnerIndexscanInfo,

	/*
	 * TAGS FOR MEMORY NODES (memnodes.h)
	 */
	T_MemoryContext = 500,
	T_AllocSetContext,

	/*
	 * TAGS FOR VALUE NODES (pg_list.h)
	 */
	T_Value = 600,
	T_List,
	T_Integer,
	T_Float,
	T_String,
	T_BitString,
	T_Null,

	/*
	 * TAGS FOR PARSE TREE NODES (parsenodes.h)
	 */
	T_Query = 700,
	T_InsertStmt,
	T_DeleteStmt,
	T_UpdateStmt,
	T_SelectStmt,
	T_AlterTableStmt,
	T_SetOperationStmt,
	T_GrantStmt,
	T_ClosePortalStmt,
	T_ClusterStmt,
	T_CopyStmt,
	T_CreateStmt,
	T_DefineStmt,
	T_DropStmt,
	T_TruncateStmt,
	T_CommentStmt,
	T_FetchStmt,
	T_IndexStmt,
	T_CreateFunctionStmt,
	T_RemoveAggrStmt,
	T_RemoveFuncStmt,
	T_RemoveOperStmt,
	T_RenameStmt,
	T_RuleStmt,
	T_NotifyStmt,
	T_ListenStmt,
	T_UnlistenStmt,
	T_TransactionStmt,
	T_ViewStmt,
	T_LoadStmt,
	T_CreateDomainStmt,
	T_DomainConstraintValue,
	T_CreatedbStmt,
	T_DropdbStmt,
	T_VacuumStmt,
	T_ExplainStmt,
	T_CreateSeqStmt,
	T_VariableSetStmt,
	T_VariableShowStmt,
	T_VariableResetStmt,
	T_CreateTrigStmt,
	T_DropPropertyStmt,
	T_CreatePLangStmt,
	T_DropPLangStmt,
	T_CreateUserStmt,
	T_AlterUserStmt,
	T_DropUserStmt,
	T_LockStmt,
	T_ConstraintsSetStmt,
	T_CreateGroupStmt,
	T_AlterGroupStmt,
	T_DropGroupStmt,
	T_ReindexStmt,
	T_CheckPointStmt,
	T_CreateSchemaStmt,
	T_AlterDatabaseSetStmt,
	T_AlterUserSetStmt,
	T_CreateConversionStmt,
	T_CreateCastStmt,
	T_DropCastStmt,
	T_CreateOpClassStmt,
	T_RemoveOpClassStmt,
	T_PrepareStmt,
	T_ExecuteStmt,
	T_DeallocateStmt,

	T_A_Expr = 800,
	T_ColumnRef,
	T_ParamRef,
	T_A_Const,
	T_FuncCall,
	T_A_Indices,
	T_ExprFieldSelect,
	T_ResTarget,
	T_TypeCast,
	T_SortGroupBy,
	T_Alias,
	T_RangeVar,
	T_RangeSubselect,
	T_RangeFunction,
	T_TypeName,
	T_IndexElem,
	T_ColumnDef,
	T_Constraint,
	T_DefElem,
	T_TargetEntry,
	T_RangeTblEntry,
	T_SortClause,
	T_GroupClause,
	T_NullTest,
	T_BooleanTest,
	T_ConstraintTest,
	T_ConstraintTestValue,
	T_CaseExpr,
	T_CaseWhen,
	T_FkConstraint,
	T_PrivGrantee,
	T_FuncWithArgs,
	T_PrivTarget,
	T_InsertDefault,
	T_CreateOpClassItem,
	T_CompositeTypeStmt,

	/*
	 * TAGS FOR FUNCTION-CALL CONTEXT AND RESULTINFO NODES (see fmgr.h)
	 */
	T_TriggerData = 900,		/* in commands/trigger.h */
	T_ReturnSetInfo				/* in nodes/execnodes.h */

} NodeTag;

/*
 * The first field of a node of any type is guaranteed to be the NodeTag.
 * Hence the type of any node can be gotten by casting it to Node. Declaring
 * a variable to be of Node * (instead of void *) can also facilitate
 * debugging.
 */
typedef struct Node
{
	NodeTag		type;
} Node;

#define nodeTag(nodeptr)		(((Node*)(nodeptr))->type)

/*
 *	There is no way to dereference the palloc'ed pointer to assign the
 *	tag, and return the pointer itself, so we need a holder variable.
 *	Fortunately, this function isn't recursive so we just define
 *	a global variable for this purpose.
 */
extern Node *newNodeMacroHolder;

#define newNode(size, tag) \
( \
	AssertMacro((size) >= sizeof(Node)),		/* need the tag, at least */ \
\
	newNodeMacroHolder = (Node *) palloc0(size), \
	newNodeMacroHolder->type = (tag), \
	newNodeMacroHolder \
)


#define makeNode(_type_)		((_type_ *) newNode(sizeof(_type_),T_##_type_))
#define NodeSetTag(nodeptr,t)	(((Node*)(nodeptr))->type = (t))

#define IsA(nodeptr,_type_)		(nodeTag(nodeptr) == T_##_type_)

/* ----------------------------------------------------------------
 *					  extern declarations follow
 * ----------------------------------------------------------------
 */

/*
 * nodes/{outfuncs.c,print.c}
 */
extern char *nodeToString(void *obj);

/*
 * nodes/{readfuncs.c,read.c}
 */
extern void *stringToNode(char *str);

/*
 * nodes/copyfuncs.c
 */
extern void *copyObject(void *obj);

/*
 * nodes/equalfuncs.c
 */
extern bool equal(void *a, void *b);


/*
 * Typedefs for identifying qualifier selectivities and plan costs as such.
 * These are just plain "double"s, but declaring a variable as Selectivity
 * or Cost makes the intent more obvious.
 *
 * These could have gone into plannodes.h or some such, but many files
 * depend on them...
 */
typedef double Selectivity;		/* fraction of tuples a qualifier will
								 * pass */
typedef double Cost;			/* execution cost (in page-access units) */


/*
 * CmdType -
 *	  enums for type of operation represented by a Query
 *
 * ??? could have put this in parsenodes.h but many files not in the
 *	  optimizer also need this...
 */
typedef enum CmdType
{
	CMD_UNKNOWN,
	CMD_SELECT,					/* select stmt (formerly retrieve) */
	CMD_UPDATE,					/* update stmt (formerly replace) */
	CMD_INSERT,					/* insert stmt (formerly append) */
	CMD_DELETE,
	CMD_UTILITY,				/* cmds like create, destroy, copy,
								 * vacuum, etc. */
	CMD_NOTHING					/* dummy command for instead nothing rules
								 * with qual */
} CmdType;


/*
 * JoinType -
 *	  enums for types of relation joins
 *
 * JoinType determines the exact semantics of joining two relations using
 * a matching qualification.  For example, it tells what to do with a tuple
 * that has no match in the other relation.
 *
 * This is needed in both parsenodes.h and plannodes.h, so put it here...
 */
typedef enum JoinType
{
	/*
	 * The canonical kinds of joins
	 */
	JOIN_INNER,					/* matching tuple pairs only */
	JOIN_LEFT,					/* pairs + unmatched outer tuples */
	JOIN_FULL,					/* pairs + unmatched outer + unmatched
								 * inner */
	JOIN_RIGHT,					/* pairs + unmatched inner tuples */

	/*
	 * SQL92 considers UNION JOIN to be a kind of join, so list it here
	 * for parser convenience, even though it's not implemented like a
	 * join in the executor.  (The planner must convert it to an Append
	 * plan.)
	 */
	JOIN_UNION

	/*
	 * Eventually we will have some additional join types for efficient
	 * support of queries like WHERE foo IN (SELECT bar FROM ...).
	 */
} JoinType;

#define IS_OUTER_JOIN(jointype) \
	((jointype) == JOIN_LEFT || \
	 (jointype) == JOIN_FULL || \
	 (jointype) == JOIN_RIGHT)

#endif   /* NODES_H */
