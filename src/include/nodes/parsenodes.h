/*-------------------------------------------------------------------------
 *
 * parsenodes.h
 *	  definitions for parse tree nodes
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parsenodes.h,v 1.193 2002-07-18 23:11:32 petere Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSENODES_H
#define PARSENODES_H

#include "nodes/primnodes.h"


/*****************************************************************************
 *	Query Tree
 *****************************************************************************/

/*
 * Query -
 *	  all statments are turned into a Query tree (via transformStmt)
 *	  for further processing by the optimizer
 *	  utility statements (i.e. non-optimizable statements)
 *	  have the *utilityStmt field set.
 *
 * we need the isPortal flag because portal names can be null too; can
 * get rid of it if we support CURSOR as a commandType.
 */
typedef struct Query
{
	NodeTag		type;

	CmdType		commandType;	/* select|insert|update|delete|utility */

	Node	   *utilityStmt;	/* non-null if this is a non-optimizable
								 * statement */

	int			resultRelation; /* target relation (index into rtable) */
	RangeVar   *into;			/* target relation or portal (cursor) 
								 * for portal just name is meaningful */
	bool		isPortal;		/* is this a retrieve into portal? */
	bool		isBinary;		/* binary portal? */

	bool		hasAggs;		/* has aggregates in tlist or havingQual */
	bool		hasSubLinks;	/* has subquery SubLink */

	bool		originalQuery;	/* marks original query through rewriting */

	List	   *rtable;			/* list of range table entries */
	FromExpr   *jointree;		/* table join tree (FROM and WHERE
								 * clauses) */

	List	   *rowMarks;		/* integer list of RT indexes of relations
								 * that are selected FOR UPDATE */

	List	   *targetList;		/* target list (of TargetEntry) */

	List	   *groupClause;	/* a list of GroupClause's */

	Node	   *havingQual;		/* qualifications applied to groups */

	List	   *distinctClause; /* a list of SortClause's */

	List	   *sortClause;		/* a list of SortClause's */

	Node	   *limitOffset;	/* # of result tuples to skip */
	Node	   *limitCount;		/* # of result tuples to return */

	Node	   *setOperations;	/* set-operation tree if this is top level
								 * of a UNION/INTERSECT/EXCEPT query */

	/*
	 * If the resultRelation turns out to be the parent of an inheritance
	 * tree, the planner will add all the child tables to the rtable and
	 * store a list of the rtindexes of all the result relations here.
	 * This is done at plan time, not parse time, since we don't want to
	 * commit to the exact set of child tables at parse time.  This field
	 * ought to go in some sort of TopPlan plan node, not in the Query.
	 */
	List	   *resultRelations;	/* integer list of RT indexes, or NIL */

	/* internal to planner */
	List	   *base_rel_list;	/* list of base-relation RelOptInfos */
	List	   *other_rel_list; /* list of other 1-relation RelOptInfos */
	List	   *join_rel_list;	/* list of join-relation RelOptInfos */
	List	   *equi_key_list;	/* list of lists of equijoined
								 * PathKeyItems */
	List	   *query_pathkeys; /* pathkeys for query_planner()'s result */
} Query;


/****************************************************************************
 *	Supporting data structures for Parse Trees
 *
 *	Most of these node types appear in raw parsetrees output by the grammar,
 *	and get transformed to something else by the analyzer.	A few of them
 *	are used as-is in transformed querytrees.
 ****************************************************************************/

/*
 * TypeName - specifies a type in definitions
 *
 * For TypeName structures generated internally, it is often easier to
 * specify the type by OID than by name.  If "names" is NIL then the
 * actual type OID is given by typeid, otherwise typeid is unused.
 *
 * If pct_type is TRUE, then names is actually a field name and we look up
 * the type of that field.  Otherwise (the normal case), names is a type
 * name possibly qualified with schema and database name.
 */
typedef struct TypeName
{
	NodeTag		type;
	List	   *names;			/* qualified name (list of Value strings) */
	Oid			typeid;			/* type identified by OID */
	bool		timezone;		/* timezone specified? */
	bool		setof;			/* is a set? */
	bool		pct_type;		/* %TYPE specified? */
	int32		typmod;			/* type modifier */
	List	   *arrayBounds;	/* array bounds */
} TypeName;

/*
 * ColumnRef - specifies a reference to a column, or possibly a whole tuple
 *
 *		The "fields" list must be nonempty; its last component may be "*"
 *		instead of a field name.  Subscripts are optional.
 */
typedef struct ColumnRef
{
	NodeTag		type;
	List	   *fields;			/* field names (list of Value strings) */
	List	   *indirection;	/* subscripts (list of A_Indices) */
} ColumnRef;

/*
 * ParamRef - specifies a parameter reference
 *
 *		The parameter could be qualified with field names and/or subscripts
 */
typedef struct ParamRef
{
	NodeTag		type;
	int			number;			/* the number of the parameter */
	List	   *fields;			/* field names (list of Value strings) */
	List	   *indirection;	/* subscripts (list of A_Indices) */
} ParamRef;

/*
 * A_Expr - binary expressions
 */
typedef struct A_Expr
{
	NodeTag		type;
	int			oper;			/* type of operation (OP,OR,AND,NOT) */
	List	   *name;			/* possibly-qualified name of operator */
	Node	   *lexpr;			/* left argument */
	Node	   *rexpr;			/* right argument */
} A_Expr;

/*
 * A_Const - a constant expression
 */
typedef struct A_Const
{
	NodeTag		type;
	Value		val;			/* the value (with the tag) */
	TypeName   *typename;		/* typecast */
} A_Const;

/*
 * TypeCast - a CAST expression
 *
 * NOTE: for mostly historical reasons, A_Const parsenodes contain
 * room for a TypeName; we only generate a separate TypeCast node if the
 * argument to be casted is not a constant.  In theory either representation
 * would work, but it is convenient to have the target type immediately
 * available while resolving a constant's datatype.
 */
typedef struct TypeCast
{
	NodeTag		type;
	Node	   *arg;			/* the expression being casted */
	TypeName   *typename;		/* the target type */
} TypeCast;

/*
 * CaseExpr - a CASE expression
 */
typedef struct CaseExpr
{
	NodeTag		type;
	Oid			casetype;
	Node	   *arg;			/* implicit equality comparison argument */
	List	   *args;			/* the arguments (list of WHEN clauses) */
	Node	   *defresult;		/* the default result (ELSE clause) */
} CaseExpr;

/*
 * CaseWhen - an argument to a CASE expression
 */
typedef struct CaseWhen
{
	NodeTag		type;
	Node	   *expr;			/* comparison expression */
	Node	   *result;			/* substitution result */
} CaseWhen;

/* ----------------
 * NullTest
 *
 * NullTest represents the operation of testing a value for NULLness.
 * Currently, we only support scalar input values, but eventually a
 * row-constructor input should be supported.
 * The appropriate test is performed and returned as a boolean Datum.
 * ----------------
 */

typedef enum NullTestType
{
	IS_NULL, IS_NOT_NULL
} NullTestType;

typedef struct NullTest
{
	NodeTag		type;
	Node	   *arg;			/* input expression */
	NullTestType nulltesttype;	/* IS NULL, IS NOT NULL */
} NullTest;

/* ----------------
 * BooleanTest
 *
 * BooleanTest represents the operation of determining whether a boolean
 * is TRUE, FALSE, or UNKNOWN (ie, NULL).  All six meaningful combinations
 * are supported.  Note that a NULL input does *not* cause a NULL result.
 * The appropriate test is performed and returned as a boolean Datum.
 * ----------------
 */

typedef enum BoolTestType
{
	IS_TRUE, IS_NOT_TRUE, IS_FALSE, IS_NOT_FALSE, IS_UNKNOWN, IS_NOT_UNKNOWN
} BoolTestType;

typedef struct BooleanTest
{
	NodeTag		type;
	Node	   *arg;			/* input expression */
	BoolTestType booltesttype;	/* test type */
} BooleanTest;

/*
 * ColumnDef - column definition (used in various creates)
 *
 * If the column has a default value, we may have the value expression
 * in either "raw" form (an untransformed parse tree) or "cooked" form
 * (the nodeToString representation of an executable expression tree),
 * depending on how this ColumnDef node was created (by parsing, or by
 * inheritance from an existing relation).	We should never have both
 * in the same node!
 *
 * The constraints list may contain a CONSTR_DEFAULT item in a raw
 * parsetree produced by gram.y, but transformCreateStmt will remove
 * the item and set raw_default instead.  CONSTR_DEFAULT items
 * should not appear in any subsequent processing.
 *
 * The "support" field, if not null, denotes a supporting relation that
 * should be linked by an internal dependency to the column.  Currently
 * this is only used to link a SERIAL column's sequence to the column.
 */
typedef struct ColumnDef
{
	NodeTag		type;
	char	   *colname;		/* name of column */
	TypeName   *typename;		/* type of column */
	bool		is_not_null;	/* NOT NULL constraint specified? */
	Node	   *raw_default;	/* default value (untransformed parse
								 * tree) */
	char	   *cooked_default; /* nodeToString representation */
	List	   *constraints;	/* other constraints on column */
	RangeVar   *support;		/* supporting relation, if any */
} ColumnDef;

/*
 * Ident -
 *	  an unqualified identifier.  This is currently used only in the context
 *	  of column name lists.
 */
typedef struct Ident
{
	NodeTag		type;
	char	   *name;			/* its name */
} Ident;

/*
 * FuncCall - a function or aggregate invocation
 *
 * agg_star indicates we saw a 'foo(*)' construct, while agg_distinct
 * indicates we saw 'foo(DISTINCT ...)'.  In either case, the construct
 * *must* be an aggregate call.  Otherwise, it might be either an
 * aggregate or some other kind of function.
 */
typedef struct FuncCall
{
	NodeTag		type;
	List	   *funcname;		/* qualified name of function */
	List	   *args;			/* the arguments (list of exprs) */
	bool		agg_star;		/* argument was really '*' */
	bool		agg_distinct;	/* arguments were labeled DISTINCT */
} FuncCall;

/*
 * A_Indices - array reference or bounds ([lidx:uidx] or [uidx])
 */
typedef struct A_Indices
{
	NodeTag		type;
	Node	   *lidx;			/* could be NULL */
	Node	   *uidx;
} A_Indices;

/*
 * ExprFieldSelect - select a field and/or array element from an expression
 *
 *		This is used in the raw parsetree to represent selection from an
 *		arbitrary expression (not a column or param reference).  Either
 *		fields or indirection may be NIL if not used.
 */
typedef struct ExprFieldSelect
{
	NodeTag		type;
	Node	   *arg;			/* the thing being selected from */
	List	   *fields;			/* field names (list of Value strings) */
	List	   *indirection;	/* subscripts (list of A_Indices) */
} ExprFieldSelect;

/*
 * ResTarget -
 *	  result target (used in target list of pre-transformed Parse trees)
 *
 * In a SELECT or INSERT target list, 'name' is either NULL or
 * the column name assigned to the value.  (If there is an 'AS ColumnLabel'
 * clause, the grammar sets 'name' from it; otherwise 'name' is initially NULL
 * and is filled in during the parse analysis phase.)
 * The 'indirection' field is not used at all.
 *
 * In an UPDATE target list, 'name' is the name of the destination column,
 * and 'indirection' stores any subscripts attached to the destination.
 * That is, our representation is UPDATE table SET name [indirection] = val.
 */
typedef struct ResTarget
{
	NodeTag		type;
	char	   *name;			/* column name or NULL */
	List	   *indirection;	/* subscripts for destination column, or
								 * NIL */
	Node	   *val;			/* the value expression to compute or
								 * assign */
} ResTarget;

/*
 * Empty node used as a marker for Default Columns
 */
typedef struct InsertDefault
{
	NodeTag		type;
} InsertDefault;

/*
 * SortGroupBy - for ORDER BY clause
 */
typedef struct SortGroupBy
{
	NodeTag		type;
	List	   *useOp;			/* operator to use */
	Node	   *node;			/* Expression  */
} SortGroupBy;

/*
 * RangeSubselect - subquery appearing in a FROM clause
 */
typedef struct RangeSubselect
{
	NodeTag		type;
	Node	   *subquery;		/* the untransformed sub-select clause */
	Alias	   *alias;			/* table alias & optional column aliases */
} RangeSubselect;

/*
 * RangeFunction - function call appearing in a FROM clause
 */
typedef struct RangeFunction
{
	NodeTag		type;
	Node	   *funccallnode;	/* untransformed function call tree */
	Alias	   *alias;			/* table alias & optional column aliases */
} RangeFunction;

/*
 * IndexElem - index parameters (used in CREATE INDEX)
 *
 * For a plain index, each 'name' is an attribute name in the heap relation;
 * 'funcname' and 'args' are NIL.  For a functional index, only one IndexElem
 * is allowed.  It has name = NULL, funcname = name of function and args =
 * list of attribute names that are the function's arguments.
 */
typedef struct IndexElem
{
	NodeTag		type;
	char	   *name;			/* name of attribute to index, or NULL */
	List	   *funcname;		/* qualified name of function */
	List	   *args;			/* list of names of function arguments */
	List	   *opclass;		/* name of desired opclass; NIL = default */
} IndexElem;

/*
 * DefElem -
 *	  a definition (used in definition lists in the form of defname = arg)
 */
typedef struct DefElem
{
	NodeTag		type;
	char	   *defname;
	Node	   *arg;			/* a (Value *) or a (TypeName *) */
} DefElem;


/****************************************************************************
 *	Nodes for a Query tree
 ****************************************************************************/

/*
 * TargetEntry -
 *	   a target  entry (used in the transformed target list)
 *
 * one of resdom or fjoin is not NULL. a target list is
 *		((<resdom | fjoin> expr) (<resdom | fjoin> expr) ...)
 */
typedef struct TargetEntry
{
	NodeTag		type;
	Resdom	   *resdom;			/* fjoin overload this to be a list?? */
	Fjoin	   *fjoin;
	Node	   *expr;
} TargetEntry;

/*--------------------
 * RangeTblEntry -
 *	  A range table is a List of RangeTblEntry nodes.
 *
 *	  A range table entry may represent a plain relation, a sub-select in
 *	  FROM, or the result of a JOIN clause.  (Only explicit JOIN syntax
 *	  produces an RTE, not the implicit join resulting from multiple FROM
 *	  items.  This is because we only need the RTE to deal with SQL features
 *	  like outer joins and join-output-column aliasing.)  Other special
 *	  RTE types also exist, as indicated by RTEKind.
 *
 *	  alias is an Alias node representing the AS alias-clause attached to the
 *	  FROM expression, or NULL if no clause.
 *
 *	  eref is the table reference name and column reference names (either
 *	  real or aliases).  Note that system columns (OID etc) are not included
 *	  in the column list.
 *	  eref->aliasname is required to be present, and should generally be used
 *	  to identify the RTE for error messages etc.
 *
 *	  inh is TRUE for relation references that should be expanded to include
 *	  inheritance children, if the rel has any.  This *must* be FALSE for
 *	  RTEs other than RTE_RELATION entries.
 *
 *	  inFromCl marks those range variables that are listed in the FROM clause.
 *	  In SQL, the query can only refer to range variables listed in the
 *	  FROM clause, but POSTQUEL allows you to refer to tables not listed,
 *	  in which case a range table entry will be generated.	We still support
 *	  this POSTQUEL feature, although there is some doubt whether it's
 *	  convenient or merely confusing.  The flag is needed since an
 *	  implicitly-added RTE shouldn't change the namespace for unqualified
 *	  column names processed later, and it also shouldn't affect the
 *	  expansion of '*'.
 *
 *	  checkForRead, checkForWrite, and checkAsUser control run-time access
 *	  permissions checks.  A rel will be checked for read or write access
 *	  (or both, or neither) per checkForRead and checkForWrite.  If
 *	  checkAsUser is not InvalidOid, then do the permissions checks using
 *	  the access rights of that user, not the current effective user ID.
 *	  (This allows rules to act as setuid gateways.)
 *--------------------
 */
typedef enum RTEKind
{
	RTE_RELATION,				/* ordinary relation reference */
	RTE_SUBQUERY,				/* subquery in FROM */
	RTE_JOIN,					/* join */
	RTE_SPECIAL,				/* special rule relation (NEW or OLD) */
	RTE_FUNCTION				/* function in FROM */
} RTEKind;

typedef struct RangeTblEntry
{
	NodeTag		type;

	RTEKind		rtekind;		/* see above */

	/*
	 * XXX the fields applicable to only some rte kinds should be merged
	 * into a union.  I didn't do this yet because the diffs would impact
	 * a lot of code that is being actively worked on.  FIXME later.
	 */

	/*
	 * Fields valid for a plain relation RTE (else zero):
	 */
	Oid			relid;			/* OID of the relation */

	/*
	 * Fields valid for a subquery RTE (else NULL):
	 */
	Query	   *subquery;		/* the sub-query */

	/*
	 * Fields valid for a function RTE (else NULL):
	 */
	Node	   *funcexpr;		/* expression tree for func call */

	/*
	 * Fields valid for a join RTE (else NULL/zero):
	 *
	 * joinaliasvars is a list of Vars or COALESCE expressions corresponding
	 * to the columns of the join result.  An alias Var referencing column
	 * K of the join result can be replaced by the K'th element of
	 * joinaliasvars --- but to simplify the task of reverse-listing aliases
	 * correctly, we do not do that until planning time.
	 */
	JoinType	jointype;		/* type of join */
	List	   *joinaliasvars;	/* list of alias-var expansions */

	/*
	 * Fields valid in all RTEs:
	 */
	Alias	   *alias;			/* user-written alias clause, if any */
	Alias	   *eref;			/* expanded reference names */
	bool		inh;			/* inheritance requested? */
	bool		inFromCl;		/* present in FROM clause */
	bool		checkForRead;	/* check rel for read access */
	bool		checkForWrite;	/* check rel for write access */
	Oid			checkAsUser;	/* if not zero, check access as this user */
} RangeTblEntry;

/*
 * SortClause -
 *	   representation of ORDER BY clauses
 *
 * tleSortGroupRef must match ressortgroupref of exactly one Resdom of the
 * associated targetlist; that is the expression to be sorted (or grouped) by.
 * sortop is the OID of the ordering operator.
 *
 * SortClauses are also used to identify Resdoms that we will do a "Unique"
 * filter step on (for SELECT DISTINCT and SELECT DISTINCT ON).  The
 * distinctClause list is simply a copy of the relevant members of the
 * sortClause list.  Note that distinctClause can be a subset of sortClause,
 * but cannot have members not present in sortClause; and the members that
 * do appear must be in the same order as in sortClause.
 */
typedef struct SortClause
{
	NodeTag		type;
	Index		tleSortGroupRef;	/* reference into targetlist */
	Oid			sortop;			/* the sort operator to use */
} SortClause;

/*
 * GroupClause -
 *	   representation of GROUP BY clauses
 *
 * GroupClause is exactly like SortClause except for the nodetag value
 * (it's probably not even really necessary to have two different
 * nodetags...).  We have routines that operate interchangeably on both.
 */
typedef SortClause GroupClause;


/*****************************************************************************
 *		Optimizable Statements
 *****************************************************************************/

/* ----------------------
 *		Insert Statement
 * ----------------------
 */
typedef struct InsertStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* relation to insert into */
	List	   *cols;			/* optional: names of the target columns */

	/*
	 * An INSERT statement has *either* VALUES or SELECT, never both. If
	 * VALUES, a targetList is supplied (empty for DEFAULT VALUES). If
	 * SELECT, a complete SelectStmt (or set-operation tree) is supplied.
	 */
	List	   *targetList;		/* the target list (of ResTarget) */
	Node	   *selectStmt;		/* the source SELECT */
} InsertStmt;

/* ----------------------
 *		Delete Statement
 * ----------------------
 */
typedef struct DeleteStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* relation to delete from */
	Node	   *whereClause;	/* qualifications */
} DeleteStmt;

/* ----------------------
 *		Update Statement
 * ----------------------
 */
typedef struct UpdateStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* relation to update */
	List	   *targetList;		/* the target list (of ResTarget) */
	Node	   *whereClause;	/* qualifications */
	List	   *fromClause;		/* optional from clause for more tables */
} UpdateStmt;

/* ----------------------
 *		Select Statement
 *
 * A "simple" SELECT is represented in the output of gram.y by a single
 * SelectStmt node.  A SELECT construct containing set operators (UNION,
 * INTERSECT, EXCEPT) is represented by a tree of SelectStmt nodes, in
 * which the leaf nodes are component SELECTs and the internal nodes
 * represent UNION, INTERSECT, or EXCEPT operators.  Using the same node
 * type for both leaf and internal nodes allows gram.y to stick ORDER BY,
 * LIMIT, etc, clause values into a SELECT statement without worrying
 * whether it is a simple or compound SELECT.
 * ----------------------
 */
typedef enum SetOperation
{
	SETOP_NONE = 0,
	SETOP_UNION,
	SETOP_INTERSECT,
	SETOP_EXCEPT
} SetOperation;

typedef struct SelectStmt
{
	NodeTag		type;

	/*
	 * These fields are used only in "leaf" SelectStmts.
	 */
	List	   *distinctClause; /* NULL, list of DISTINCT ON exprs, or
								 * lcons(NIL,NIL) for all (SELECT
								 * DISTINCT) */
	RangeVar   *into;			/* target table (for select into table) */
	List	   *intoColNames;	/* column names for into table */
	List	   *targetList;		/* the target list (of ResTarget) */
	List	   *fromClause;		/* the FROM clause */
	Node	   *whereClause;	/* WHERE qualification */
	List	   *groupClause;	/* GROUP BY clauses */
	Node	   *havingClause;	/* HAVING conditional-expression */

	/*
	 * These fields are used in both "leaf" SelectStmts and upper-level
	 * SelectStmts.  portalname/binary may only be set at the top level.
	 */
	List	   *sortClause;		/* sort clause (a list of SortGroupBy's) */
	char	   *portalname;		/* the portal (cursor) to create */
	bool		binary;			/* a binary (internal) portal? */
	Node	   *limitOffset;	/* # of result tuples to skip */
	Node	   *limitCount;		/* # of result tuples to return */
	List	   *forUpdate;		/* FOR UPDATE clause */

	/*
	 * These fields are used only in upper-level SelectStmts.
	 */
	SetOperation op;			/* type of set op */
	bool		all;			/* ALL specified? */
	struct SelectStmt *larg;	/* left child */
	struct SelectStmt *rarg;	/* right child */
	/* Eventually add fields for CORRESPONDING spec here */
} SelectStmt;

/* ----------------------
 *		Set Operation node for post-analysis query trees
 *
 * After parse analysis, a SELECT with set operations is represented by a
 * top-level Query node containing the leaf SELECTs as subqueries in its
 * range table.  Its setOperations field shows the tree of set operations,
 * with leaf SelectStmt nodes replaced by RangeTblRef nodes, and internal
 * nodes replaced by SetOperationStmt nodes.
 * ----------------------
 */
typedef struct SetOperationStmt
{
	NodeTag		type;
	SetOperation op;			/* type of set op */
	bool		all;			/* ALL specified? */
	Node	   *larg;			/* left child */
	Node	   *rarg;			/* right child */
	/* Eventually add fields for CORRESPONDING spec here */

	/* Fields derived during parse analysis: */
	List	   *colTypes;		/* integer list of OIDs of output column
								 * types */
} SetOperationStmt;


/*****************************************************************************
 *		Other Statements (no optimizations required)
 *
 *		Some of them require a little bit of transformation (which is also
 *		done by transformStmt). The whole structure is then passed on to
 *		ProcessUtility (by-passing the optimization step) as the utilityStmt
 *		field in Query.
 *****************************************************************************/

/* ----------------------
 *		Create Schema Statement
 *
 * NOTE: the schemaElts list contains raw parsetrees for component statements
 * of the schema, such as CREATE TABLE, GRANT, etc.  These are analyzed and
 * executed after the schema itself is created.
 * ----------------------
 */
typedef struct CreateSchemaStmt
{
	NodeTag		type;
	char	   *schemaname;		/* the name of the schema to create */
	char	   *authid;			/* the owner of the created schema */
	List	   *schemaElts;		/* schema components (list of parsenodes) */
} CreateSchemaStmt;

typedef enum DropBehavior
{
	DROP_RESTRICT,				/* drop fails if any dependent objects */
	DROP_CASCADE				/* remove dependent objects too */
} DropBehavior;

/* ----------------------
 *	Alter Table
 *
 * The fields are used in different ways by the different variants of
 * this command.
 * ----------------------
 */
typedef struct AlterTableStmt
{
	NodeTag		type;
	char		subtype;		/*------------
								 *	A = add column
								 *	T = alter column default
								 *	N = alter column drop not null
								 *	O = alter column set not null
								 *	S = alter column statistics
								 *  M = alter column storage
								 *	D = drop column
								 *	C = add constraint
								 *	c = pre-processed add constraint
								 *		(local in parser/analyze.c)
								 *	X = drop constraint
								 *	E = create toast table
								 *	U = change owner
								 *------------
								 */
	RangeVar   *relation;		/* table to work on */
	char	   *name;			/* column or constraint name to act on, or
								 * new owner */
	Node	   *def;			/* definition of new column or constraint */
	DropBehavior behavior;		/* RESTRICT or CASCADE for DROP cases */
} AlterTableStmt;

/* ----------------------
 *		Grant|Revoke Statement
 * ----------------------
 */
typedef enum GrantObjectType
{
	ACL_OBJECT_RELATION,		/* table, view, sequence */
	ACL_OBJECT_DATABASE,		/* database */
	ACL_OBJECT_FUNCTION,		/* function */
	ACL_OBJECT_LANGUAGE,		/* procedural language */
	ACL_OBJECT_NAMESPACE		/* namespace */
} GrantObjectType;

/*
 * Grantable rights are encoded so that we can OR them together in a bitmask.
 * The present representation of AclItem limits us to 30 distinct rights.
 * Caution: changing these codes breaks stored ACLs, hence forces initdb.
 */
#define ACL_INSERT		(1<<0)	/* for relations */
#define ACL_SELECT		(1<<1)
#define ACL_UPDATE		(1<<2)
#define ACL_DELETE		(1<<3)
#define ACL_RULE		(1<<4)
#define ACL_REFERENCES	(1<<5)
#define ACL_TRIGGER		(1<<6)
#define ACL_EXECUTE		(1<<7)	/* for functions */
#define ACL_USAGE		(1<<8)	/* for languages and namespaces */
#define ACL_CREATE		(1<<9)	/* for namespaces and databases */
#define ACL_CREATE_TEMP	(1<<10)	/* for databases */
#define N_ACL_RIGHTS	11		/* 1 plus the last 1<<x */
#define ACL_ALL_RIGHTS	(-1)	/* all-privileges marker in GRANT list */
#define ACL_NO_RIGHTS	0

typedef struct GrantStmt
{
	NodeTag		type;
	bool		is_grant;		/* true = GRANT, false = REVOKE */
	GrantObjectType objtype;	/* kind of object being operated on */
	List	   *objects;		/* list of RangeVar nodes, FuncWithArgs nodes,
								 * or plain names (as Value strings) */
	List	   *privileges;		/* integer list of privilege codes */
	List	   *grantees;		/* list of PrivGrantee nodes */
} GrantStmt;

typedef struct PrivGrantee
{
	NodeTag		type;
	char	   *username;		/* if both are NULL then PUBLIC */
	char	   *groupname;
} PrivGrantee;

typedef struct FuncWithArgs
{
	NodeTag		type;
	List	   *funcname;		/* qualified name of function */
	List	   *funcargs;		/* list of Typename nodes */
} FuncWithArgs;

/* This is only used internally in gram.y. */
typedef struct PrivTarget
{
	NodeTag		type;
	GrantObjectType objtype;
	List	   *objs;
} PrivTarget;

/* ----------------------
 *		Close Portal Statement
 * ----------------------
 */
typedef struct ClosePortalStmt
{
	NodeTag		type;
	char	   *portalname;		/* name of the portal (cursor) */
} ClosePortalStmt;

/* ----------------------
 *		Copy Statement
 * ----------------------
 */
typedef struct CopyStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* the relation to copy */
	List *attlist;
	bool		is_from;		/* TO or FROM */
	char	   *filename;		/* if NULL, use stdin/stdout */
	List	   *options;		/* List of DefElem nodes */
} CopyStmt;

/* ----------------------
 *		Create Table Statement
 *
 * NOTE: in the raw gram.y output, ColumnDef, Constraint, and FkConstraint
 * nodes are intermixed in tableElts, and constraints is NIL.  After parse
 * analysis, tableElts contains just ColumnDefs, and constraints contains
 * just Constraint nodes (in fact, only CONSTR_CHECK nodes, in the present
 * implementation).
 * ----------------------
 */
typedef struct CreateStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* relation to create */
	List	   *tableElts;		/* column definitions (list of ColumnDef) */
	List	   *inhRelations;	/* relations to inherit from */
	List	   *constraints;	/* constraints (list of Constraint nodes) */
	bool		hasoids;		/* should it have OIDs? */
} CreateStmt;

/* ----------
 * Definitions for plain (non-FOREIGN KEY) constraints in CreateStmt
 *
 * XXX probably these ought to be unified with FkConstraints at some point?
 *
 * For constraints that use expressions (CONSTR_DEFAULT, CONSTR_CHECK)
 * we may have the expression in either "raw" form (an untransformed
 * parse tree) or "cooked" form (the nodeToString representation of
 * an executable expression tree), depending on how this Constraint
 * node was created (by parsing, or by inheritance from an existing
 * relation).  We should never have both in the same node!
 *
 * Constraint attributes (DEFERRABLE etc) are initially represented as
 * separate Constraint nodes for simplicity of parsing.  analyze.c makes
 * a pass through the constraints list to attach the info to the appropriate
 * FkConstraint node (and, perhaps, someday to other kinds of constraints).
 * ----------
 */

typedef enum ConstrType			/* types of constraints */
{
	CONSTR_NULL,				/* not SQL92, but a lot of people expect
								 * it */
	CONSTR_NOTNULL,
	CONSTR_DEFAULT,
	CONSTR_CHECK,
	CONSTR_PRIMARY,
	CONSTR_UNIQUE,
	CONSTR_ATTR_DEFERRABLE,		/* attributes for previous constraint node */
	CONSTR_ATTR_NOT_DEFERRABLE,
	CONSTR_ATTR_DEFERRED,
	CONSTR_ATTR_IMMEDIATE
} ConstrType;

typedef struct Constraint
{
	NodeTag		type;
	ConstrType	contype;
	char	   *name;			/* name, or NULL if unnamed */
	Node	   *raw_expr;		/* expr, as untransformed parse tree */
	char	   *cooked_expr;	/* expr, as nodeToString representation */
	List	   *keys;			/* Ident nodes naming referenced column(s) */
} Constraint;

/* ----------
 * Definitions for FOREIGN KEY constraints in CreateStmt
 *
 * Note: FKCONSTR_ACTION_xxx values are stored into pg_constraint.confupdtype
 * and pg_constraint.confdeltype columns; FKCONSTR_MATCH_xxx values are
 * stored into pg_constraint.confmatchtype.  Changing the code values may
 * require an initdb!
 *
 * If skip_validation is true then we skip checking that the existing rows
 * in the table satisfy the constraint, and just install the catalog entries
 * for the constraint.  This is currently used only during CREATE TABLE
 * (when we know the table must be empty).
 * ----------
 */
#define FKCONSTR_ACTION_NOACTION	'a'
#define FKCONSTR_ACTION_RESTRICT	'r'
#define FKCONSTR_ACTION_CASCADE		'c'
#define FKCONSTR_ACTION_SETNULL		'n'
#define FKCONSTR_ACTION_SETDEFAULT	'd'

#define FKCONSTR_MATCH_FULL			'f'
#define FKCONSTR_MATCH_PARTIAL		'p'
#define FKCONSTR_MATCH_UNSPECIFIED	'u'

typedef struct FkConstraint
{
	NodeTag		type;
	char	   *constr_name;	/* Constraint name, or NULL if unnamed */
	RangeVar   *pktable;		/* Primary key table */
	List	   *fk_attrs;		/* Attributes of foreign key */
	List	   *pk_attrs;		/* Corresponding attrs in PK table */
	char		fk_matchtype;	/* FULL, PARTIAL, UNSPECIFIED */
	char		fk_upd_action;	/* ON UPDATE action */
	char		fk_del_action;	/* ON DELETE action */
	bool		deferrable;		/* DEFERRABLE */
	bool		initdeferred;	/* INITIALLY DEFERRED */
	bool		skip_validation; /* skip validation of existing rows? */
} FkConstraint;

/* ----------------------
 *		Create/Drop TRIGGER Statements
 * ----------------------
 */

typedef struct CreateTrigStmt
{
	NodeTag		type;
	char	   *trigname;		/* TRIGGER's name */
	RangeVar   *relation;		/* relation trigger is on */
	List	   *funcname;		/* qual. name of function to call */
	List	   *args;			/* list of (T_String) Values or NIL */
	bool		before;			/* BEFORE/AFTER */
	bool		row;			/* ROW/STATEMENT */
	char		actions[4];		/* Insert, Update, Delete */
	char	   *lang;			/* currently not used, always NULL */
	char	   *text;			/* AS 'text' */
	List	   *attr;			/* UPDATE OF a, b,... (NI) or NULL */
	char	   *when;			/* WHEN 'a > 10 ...' (NI) or NULL */

	/* The following are used for referential */
	/* integrity constraint triggers */
	bool		isconstraint;	/* This is an RI trigger */
	bool		deferrable;		/* [NOT] DEFERRABLE */
	bool		initdeferred;	/* INITIALLY {DEFERRED|IMMEDIATE} */
	RangeVar   *constrrel;		/* opposite relation */
} CreateTrigStmt;

/* ----------------------
 *		Create/Drop PROCEDURAL LANGUAGE Statement
 * ----------------------
 */
typedef struct CreatePLangStmt
{
	NodeTag		type;
	char	   *plname;			/* PL name */
	List	   *plhandler;		/* PL call handler function (qual. name) */
	List	   *plvalidator;	/* optional validator function (qual. name) */
	char	   *plcompiler;		/* lancompiler text */
	bool		pltrusted;		/* PL is trusted */
} CreatePLangStmt;

typedef struct DropPLangStmt
{
	NodeTag		type;
	char	   *plname;			/* PL name */
	DropBehavior behavior;		/* RESTRICT or CASCADE behavior */
} DropPLangStmt;

/* ----------------------
 *	Create/Alter/Drop User Statements
 * ----------------------
 */
typedef struct CreateUserStmt
{
	NodeTag		type;
	char	   *user;			/* PostgreSQL user login name */
	List	   *options;		/* List of DefElem nodes */
} CreateUserStmt;

typedef struct AlterUserStmt
{
	NodeTag		type;
	char	   *user;			/* PostgreSQL user login name */
	List	   *options;		/* List of DefElem nodes */
} AlterUserStmt;

typedef struct AlterUserSetStmt
{
	NodeTag		type;
	char	   *user;
	char	   *variable;
	List	   *value;
} AlterUserSetStmt;

typedef struct DropUserStmt
{
	NodeTag		type;
	List	   *users;			/* List of users to remove */
} DropUserStmt;

/* ----------------------
 *		Create/Alter/Drop Group Statements
 * ----------------------
 */
typedef struct CreateGroupStmt
{
	NodeTag		type;
	char	   *name;			/* name of the new group */
	List	   *options;		/* List of DefElem nodes */
} CreateGroupStmt;

typedef struct AlterGroupStmt
{
	NodeTag		type;
	char	   *name;			/* name of group to alter */
	int			action;			/* +1 = add, -1 = drop user */
	List	   *listUsers;		/* list of users to add/drop */
} AlterGroupStmt;

typedef struct DropGroupStmt
{
	NodeTag		type;
	char	   *name;
} DropGroupStmt;

/* ----------------------
 *		Create SEQUENCE Statement
 * ----------------------
 */

typedef struct CreateSeqStmt
{
	NodeTag		type;
	RangeVar   *sequence;		/* the sequence to create */
	List	   *options;
} CreateSeqStmt;

/* ----------------------
 *		Create {Operator|Type|Aggregate} Statement
 * ----------------------
 */
typedef struct DefineStmt
{
	NodeTag		type;
	int			defType;		/* OPERATOR|TYPE_P|AGGREGATE */
	List	   *defnames;		/* qualified name (list of Value strings) */
	List	   *definition;		/* a list of DefElem */
} DefineStmt;

/* ----------------------
 *		Create Domain Statement
 * ----------------------
 */
typedef struct CreateDomainStmt
{
	NodeTag		type;
	List	   *domainname;		/* qualified name (list of Value strings) */
	TypeName   *typename;		/* the base type */
	List	   *constraints;	/* constraints (list of Constraint nodes) */
} CreateDomainStmt;

/* ----------------------
 *		Drop Table|Sequence|View|Index|Type|Domain|Conversion|Schema Statement
 * ----------------------
 */

#define DROP_TABLE	  1
#define DROP_SEQUENCE 2
#define DROP_VIEW	  3
#define DROP_INDEX	  4
#define DROP_TYPE     5
#define DROP_DOMAIN	  6
#define DROP_CONVERSION	  7
#define DROP_SCHEMA	  8

typedef struct DropStmt
{
	NodeTag		type;
	List	   *objects;		/* list of sublists of names (as Values) */
	int			removeType;		/* see #defines above */
	DropBehavior behavior;		/* RESTRICT or CASCADE behavior */
} DropStmt;

/* ----------------------
 *		Drop Rule|Trigger Statement
 *
 * In general this may be used for dropping any property of a relation;
 * for example, someday soon we may have DROP ATTRIBUTE.
 * ----------------------
 */

#define DROP_RULE	  100
#define DROP_TRIGGER  101

typedef struct DropPropertyStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* owning relation */
	char	   *property;		/* name of rule, trigger, etc */
	int			removeType;		/* see #defines above */
	DropBehavior behavior;		/* RESTRICT or CASCADE behavior */
} DropPropertyStmt;

/* ----------------------
 *				Truncate Table Statement
 * ----------------------
 */
typedef struct TruncateStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* relation to be truncated */
} TruncateStmt;

/* ----------------------
 *				Comment On Statement
 * ----------------------
 */
#define COMMENT_ON_AGGREGATE	100
#define COMMENT_ON_COLUMN		101
#define COMMENT_ON_CONSTRAINT	102
#define COMMENT_ON_DATABASE		103
#define COMMENT_ON_FUNCTION		104
#define COMMENT_ON_INDEX		105
#define COMMENT_ON_OPERATOR		106
#define COMMENT_ON_RULE			107
#define COMMENT_ON_SCHEMA		108
#define COMMENT_ON_SEQUENCE		109
#define COMMENT_ON_TABLE		110
#define COMMENT_ON_TRIGGER		111
#define COMMENT_ON_TYPE			112
#define COMMENT_ON_VIEW			113

typedef struct CommentStmt
{
	NodeTag		type;
	int			objtype;		/* Object's type, see codes above */
	List	   *objname;		/* Qualified name of the object */
	List	   *objargs;		/* Arguments if needed (eg, for functions) */
	char	   *comment;		/* Comment to insert, or NULL to remove */
} CommentStmt;

/* ----------------------
 *		Begin Recipe Statement
 * ----------------------
 */
typedef struct RecipeStmt
{
	NodeTag		type;
	char	   *recipeName;		/* name of the recipe */
} RecipeStmt;

/* ----------------------
 *		Fetch Statement
 * ----------------------
 */
typedef struct FetchStmt
{
	NodeTag		type;
	int			direction;		/* FORWARD or BACKWARD */
	int			howMany;		/* amount to fetch ("ALL" --> 0) */
	char	   *portalname;		/* name of portal (cursor) */
	bool		ismove;			/* TRUE if MOVE */
} FetchStmt;

/* ----------------------
 *		Create Index Statement
 * ----------------------
 */
typedef struct IndexStmt
{
	NodeTag		type;
	char	   *idxname;		/* name of the index */
	RangeVar   *relation;		/* relation to build index on */
	char	   *accessMethod;	/* name of access method (eg. btree) */
	List	   *indexParams;	/* a list of IndexElem */
	Node	   *whereClause;	/* qualification (partial-index predicate) */
	List	   *rangetable;		/* range table for qual, filled in by
								 * transformStmt() */
	bool		unique;			/* is index unique? */
	bool		primary;		/* is index on primary key? */
	bool		isconstraint;	/* is it from a CONSTRAINT clause? */
} IndexStmt;

/* ----------------------
 *		Create Function Statement
 * ----------------------
 */
typedef struct CreateFunctionStmt
{
	NodeTag		type;
	bool		replace;		/* T => replace if already exists */
	List	   *funcname;		/* qualified name of function to create */
	List	   *argTypes;		/* list of argument types (TypeName nodes) */
	TypeName   *returnType;		/* the return type */
	List	   *options;		/* a list of DefElem */
	List	   *withClause;		/* a list of DefElem */
} CreateFunctionStmt;

/* ----------------------
 *		Drop Aggregate Statement
 * ----------------------
 */
typedef struct RemoveAggrStmt
{
	NodeTag		type;
	List	   *aggname;		/* aggregate to drop */
	TypeName   *aggtype;		/* TypeName for input datatype, or NULL */
	DropBehavior behavior;		/* RESTRICT or CASCADE behavior */
} RemoveAggrStmt;

/* ----------------------
 *		Drop Function Statement
 * ----------------------
 */
typedef struct RemoveFuncStmt
{
	NodeTag		type;
	List	   *funcname;		/* function to drop */
	List	   *args;			/* types of the arguments */
	DropBehavior behavior;		/* RESTRICT or CASCADE behavior */
} RemoveFuncStmt;

/* ----------------------
 *		Drop Operator Statement
 * ----------------------
 */
typedef struct RemoveOperStmt
{
	NodeTag		type;
	List	   *opname;			/* operator to drop */
	List	   *args;			/* types of the arguments */
	DropBehavior behavior;		/* RESTRICT or CASCADE behavior */
} RemoveOperStmt;

/* ----------------------
 *		Alter Object Rename Statement
 * ----------------------
 * Currently supports renaming tables, table columns, and triggers.
 * If renaming a table, oldname is ignored.
 */
#define RENAME_TABLE	110
#define RENAME_COLUMN	111
#define RENAME_TRIGGER	112
#define RENAME_RULE		113

typedef struct RenameStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* owning relation */
	char	   *oldname;		/* name of rule, trigger, etc */
	char	   *newname;		/* the new name */
	int			renameType;		/* RENAME_TABLE, RENAME_COLUMN, etc */
} RenameStmt;

/* ----------------------
 *		Create Rule Statement
 * ----------------------
 */
typedef struct RuleStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* relation the rule is for */
	char	   *rulename;		/* name of the rule */
	Node	   *whereClause;	/* qualifications */
	CmdType		event;			/* SELECT, INSERT, etc */
	bool		instead;		/* is a 'do instead'? */
	List	   *actions;		/* the action statements */
} RuleStmt;

/* ----------------------
 *		Notify Statement
 * ----------------------
 */
typedef struct NotifyStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* qualified name to notify */
} NotifyStmt;

/* ----------------------
 *		Listen Statement
 * ----------------------
 */
typedef struct ListenStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* qualified name to listen on */
} ListenStmt;

/* ----------------------
 *		Unlisten Statement
 * ----------------------
 */
typedef struct UnlistenStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* qualified name to unlisten on, or '*' */
} UnlistenStmt;

/* ----------------------
 *		{Begin|Abort|End} Transaction Statement
 * ----------------------
 */
typedef struct TransactionStmt
{
	NodeTag		type;
	int			command;		/* BEGIN|END|ABORT */
} TransactionStmt;

/* ----------------------
 *		Create View Statement
 * ----------------------
 */
typedef struct ViewStmt
{
	NodeTag		type;
	RangeVar   *view;			/* the view to be created */
	List	   *aliases;		/* target column names */
	Query	   *query;			/* the SQL statement */
} ViewStmt;

/* ----------------------
 *		Load Statement
 * ----------------------
 */
typedef struct LoadStmt
{
	NodeTag		type;
	char	   *filename;		/* file to load */
} LoadStmt;

/* ----------------------
 *		Createdb Statement
 * ----------------------
 */
typedef struct CreatedbStmt
{
	NodeTag		type;
	char	   *dbname;			/* name of database to create */
	List	   *options;		/* List of DefElem nodes */
} CreatedbStmt;

/* ----------------------
 *	Alter Database
 * ----------------------
 */
typedef struct AlterDatabaseSetStmt
{
	NodeTag		type;
	char	   *dbname;
	char	   *variable;
	List	   *value;
} AlterDatabaseSetStmt;

/* ----------------------
 *		Dropdb Statement
 * ----------------------
 */
typedef struct DropdbStmt
{
	NodeTag		type;
	char	   *dbname;			/* database to drop */
} DropdbStmt;

/* ----------------------
 *		Cluster Statement (support pbrown's cluster index implementation)
 * ----------------------
 */
typedef struct ClusterStmt
{
	NodeTag		type;
	RangeVar   *relation;		/* relation being indexed */
	char	   *indexname;		/* original index defined */
} ClusterStmt;

/* ----------------------
 *		Vacuum and Analyze Statements
 *
 * Even though these are nominally two statements, it's convenient to use
 * just one node type for both.
 * ----------------------
 */
typedef struct VacuumStmt
{
	NodeTag		type;
	bool		vacuum;			/* do VACUUM step */
	bool		full;			/* do FULL (non-concurrent) vacuum */
	bool		analyze;		/* do ANALYZE step */
	bool		freeze;			/* early-freeze option */
	bool		verbose;		/* print progress info */
	RangeVar   *relation;		/* single table to process, or NULL */
	List	   *va_cols;		/* list of column names, or NIL for all */
} VacuumStmt;

/* ----------------------
 *		Explain Statement
 * ----------------------
 */
typedef struct ExplainStmt
{
	NodeTag		type;
	Query	   *query;			/* the query */
	bool		verbose;		/* print plan info */
	bool		analyze;		/* get statistics by executing plan */
} ExplainStmt;

/* ----------------------
 * Checkpoint Statement
 * ----------------------
 */
typedef struct CheckPointStmt
{
	NodeTag		type;
} CheckPointStmt;

/* ----------------------
 * Set Statement
 * ----------------------
 */

typedef struct VariableSetStmt
{
	NodeTag		type;
	char	   *name;
	List	   *args;
	bool		is_local;		/* SET LOCAL */
} VariableSetStmt;

/* ----------------------
 * Show Statement
 * ----------------------
 */

typedef struct VariableShowStmt
{
	NodeTag		type;
	char	   *name;
} VariableShowStmt;

/* ----------------------
 * Reset Statement
 * ----------------------
 */

typedef struct VariableResetStmt
{
	NodeTag		type;
	char	   *name;
} VariableResetStmt;

/* ----------------------
 *		LOCK Statement
 * ----------------------
 */
typedef struct LockStmt
{
	NodeTag		type;
	List	   *relations;		/* relations to lock */
	int			mode;			/* lock mode */
} LockStmt;

/* ----------------------
 *		SET CONSTRAINTS Statement
 * ----------------------
 */
typedef struct ConstraintsSetStmt
{
	NodeTag		type;
	List	   *constraints;	/* List of names as Value strings */
	bool		deferred;
} ConstraintsSetStmt;

/* ----------------------
 *		REINDEX Statement
 * ----------------------
 */
typedef struct ReindexStmt
{
	NodeTag		type;
	int			reindexType;	/* INDEX|TABLE|DATABASE */
	RangeVar   *relation;		/* Table or index to reindex */
	const char *name;			/* name of database to reindex */
	bool		force;
	bool		all;
} ReindexStmt;

/* ----------------------
 *		CREATE CONVERSION Statement
 * ----------------------
 */
typedef struct CreateConversionStmt
{
	NodeTag		type;
	List		*conversion_name;		/* Name of the conversion */
	char		*for_encoding_name;		/* source encoding name */
	char		*to_encoding_name;		/* destiname encoding name */
	List		*func_name;				/* qualified conversion function name */
	bool		def;				/* is this a default conversion? */
} CreateConversionStmt;

/* ----------------------
 *	CREATE CAST Statement
 * ----------------------
 */
typedef struct CreateCastStmt
{
	NodeTag		type;
	TypeName   *sourcetype;
	TypeName   *targettype;
	FuncWithArgs *func;
	bool		implicit;
} CreateCastStmt;

/* ----------------------
 *	DROP CAST Statement
 * ----------------------
 */
typedef struct DropCastStmt
{
	NodeTag		type;
	TypeName   *sourcetype;
	TypeName   *targettype;
	DropBehavior behavior;
} DropCastStmt;


#endif   /* PARSENODES_H */
