/*-------------------------------------------------------------------------
 *
 * parsenodes.h
 *	  definitions for parse tree nodes
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parsenodes.h,v 1.74 1999-05-25 22:42:57 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSENODES_H
#define PARSENODES_H

#include <nodes/primnodes.h>

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
 *
 */
typedef struct Query
{
	NodeTag		type;

	CmdType		commandType;	/* select|insert|update|delete|utility */

	Node	   *utilityStmt;	/* non-null if this is a non-optimizable
								 * statement */

	int			resultRelation; /* target relation (index to rtable) */
	char	   *into;			/* portal (cursor) name */
	bool		isPortal;		/* is this a retrieve into portal? */
	bool		isBinary;		/* binary portal? */
	bool		isTemp;			/* is 'into' a temp table? */
	bool		unionall;		/* union without unique sort */
	bool		hasAggs;		/* has aggregates in target list */
	bool		hasSubLinks;	/* has subquery SubLink */

	char	   *uniqueFlag;		/* NULL, '*', or Unique attribute name */
	List	   *sortClause;		/* a list of SortClause's */

	List	   *rtable;			/* list of range table entries */
	List	   *targetList;		/* target list (of TargetEntry) */
	Node	   *qual;			/* qualifications */
	List	   *rowMark;		/* list of RowMark entries */

	List	   *groupClause;	/* list of columns to specified in GROUP
								 * BY */
	Node	   *havingQual;		/* qualification of each group */

	/***S*I***/
	List	   *intersectClause;

	List	   *unionClause;	/* unions are linked under the previous
								 * query */
	Node	   *limitOffset;	/* # of result tuples to skip */
	Node	   *limitCount;		/* # of result tuples to return */

	/* internal to planner */
	List	   *base_rel_list;	/* base relation list */
	List	   *join_rel_list;	/* list of relation involved in joins */
} Query;


/*****************************************************************************
 *		Other Statements (no optimizations required)
 *
 *		Some of them require a little bit of transformation (which is also
 *		done by transformStmt). The whole structure is then passed on to
 *		ProcessUtility (by-passing the optimization step) as the utilityStmt
 *		field in Query.
 *****************************************************************************/

/* ----------------------
 *		Add Column Statement
 * ----------------------
 */
typedef struct AddAttrStmt
{
	NodeTag		type;
	char	   *relname;		/* the relation to add attr */
	bool		inh;			/* add recursively to children? */
	Node	   *colDef;			/* the attribute definition */
} AddAttrStmt;

/* ----------------------
 *		Change ACL Statement
 * ----------------------
 */
typedef struct ChangeACLStmt
{
	NodeTag		type;
	struct AclItem *aclitem;
	unsigned	modechg;
	List	   *relNames;
} ChangeACLStmt;

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
	bool		binary;			/* is a binary copy? */
	char	   *relname;		/* the relation to copy */
	bool		oids;			/* copy oid's? */
	int			direction;		/* TO or FROM */
	char	   *filename;		/* if NULL, use stdin/stdout */
	char	   *delimiter;		/* delimiter character, \t by default */
} CopyStmt;

/* ----------------------
 *		Create Table Statement
 * ----------------------
 */
typedef struct CreateStmt
{
	NodeTag		type;
	bool		istemp;			/* is this a temp table? */
	char	   *relname;		/* the relation to create */
	List	   *tableElts;		/* column definitions list of Column */
	List	   *inhRelnames;	/* relations to inherit from list of Value
								 * (string) */
	List	   *constraints;	/* list of constraints (ConstaintDef) */
} CreateStmt;

typedef enum ConstrType			/* type of constaints */
{
	CONSTR_NULL, CONSTR_NOTNULL, CONSTR_DEFAULT, CONSTR_CHECK, CONSTR_PRIMARY, CONSTR_UNIQUE
} ConstrType;

typedef struct Constraint
{
	NodeTag		type;
	ConstrType	contype;
	char	   *name;			/* name */
	void	   *def;			/* definition */
	void	   *keys;			/* list of primary keys */
} Constraint;

/* ----------------------
 *		Create/Drop TRIGGER Statements
 * ----------------------
 */

typedef struct CreateTrigStmt
{
	NodeTag		type;
	char	   *trigname;		/* TRIGGER' name */
	char	   *relname;		/* triggered relation */
	char	   *funcname;		/* function to call (or NULL) */
	List	   *args;			/* list of (T_String) Values or NULL */
	bool		before;			/* BEFORE/AFTER */
	bool		row;			/* ROW/STATEMENT */
	char		actions[4];		/* Insert, Update, Delete */
	char	   *lang;			/* NULL (which means Clanguage) */
	char	   *text;			/* AS 'text' */
	List	   *attr;			/* UPDATE OF a, b,... (NI) or NULL */
	char	   *when;			/* WHEN 'a > 10 ...' (NI) or NULL */
} CreateTrigStmt;

typedef struct DropTrigStmt
{
	NodeTag		type;
	char	   *trigname;		/* TRIGGER' name */
	char	   *relname;		/* triggered relation */
} DropTrigStmt;


/* ----------------------
 *		Create/Drop PROCEDURAL LANGUAGE Statement
 * ----------------------
 */
typedef struct CreatePLangStmt
{
	NodeTag		type;
	char	   *plname;			/* PL name */
	char	   *plhandler;		/* PL call handler function */
	char	   *plcompiler;		/* lancompiler text */
	bool		pltrusted;		/* PL is trusted */
} CreatePLangStmt;

typedef struct DropPLangStmt
{
	NodeTag		type;
	char	   *plname;			/* PL name */
} DropPLangStmt;


/* ----------------------
 *				Create/Alter/Drop User Statements
 * ----------------------
 */
typedef struct CreateUserStmt
{
	NodeTag		type;
	char	   *user;			/* PostgreSQL user login			  */
	char	   *password;		/* PostgreSQL user password			  */
	bool	   *createdb;		/* Can the user create databases?	  */
	bool	   *createuser;		/* Can this user create users?		  */
	List	   *groupElts;		/* The groups the user is a member of */
	char	   *validUntil;		/* The time the login is valid until  */
} CreateUserStmt;

typedef CreateUserStmt AlterUserStmt;

typedef struct DropUserStmt
{
	NodeTag		type;
	char	   *user;			/* PostgreSQL user login			  */
} DropUserStmt;


/* ----------------------
 *		Create SEQUENCE Statement
 * ----------------------
 */

typedef struct CreateSeqStmt
{
	NodeTag		type;
	char	   *seqname;		/* the relation to create */
	List	   *options;
} CreateSeqStmt;

/* ----------------------
 *		Create Version Statement
 * ----------------------
 */
typedef struct VersionStmt
{
	NodeTag		type;
	char	   *relname;		/* the new relation */
	int			direction;		/* FORWARD | BACKWARD */
	char	   *fromRelname;	/* relation to create a version */
	char	   *date;			/* date of the snapshot */
} VersionStmt;

/* ----------------------
 *		Create {Operator|Type|Aggregate} Statement
 * ----------------------
 */
typedef struct DefineStmt
{
	NodeTag		type;
	int			defType;		/* OPERATOR|P_TYPE|AGGREGATE */
	char	   *defname;
	List	   *definition;		/* a list of DefElem */
} DefineStmt;

/* ----------------------
 *		Drop Table Statement
 * ----------------------
 */
typedef struct DestroyStmt
{
	NodeTag		type;
	List	   *relNames;		/* relations to be dropped */
	bool		sequence;
} DestroyStmt;

/* ----------------------
 *		Extend Index Statement
 * ----------------------
 */
typedef struct ExtendStmt
{
	NodeTag		type;
	char	   *idxname;		/* name of the index */
	Node	   *whereClause;	/* qualifications */
	List	   *rangetable;		/* range table, filled in by
								 * transformStmt() */
} ExtendStmt;

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
	char	   *relname;		/* name of relation to index on */
	char	   *accessMethod;	/* name of acess methood (eg. btree) */
	List	   *indexParams;	/* a list of IndexElem */
	List	   *withClause;		/* a list of ParamString */
	Node	   *whereClause;	/* qualifications */
	List	   *rangetable;		/* range table, filled in by
								 * transformStmt() */
	bool	   *lossy;			/* is index lossy? */
	bool		unique;			/* is index unique? */
	bool		primary;		/* is index on primary key? */
} IndexStmt;

/* ----------------------
 *		Create Function Statement
 * ----------------------
 */
typedef struct ProcedureStmt
{
	NodeTag		type;
	char	   *funcname;		/* name of function to create */
	List	   *defArgs;		/* list of definitions a list of strings
								 * (as Value *) */
	Node	   *returnType;		/* the return type (as a string or a
								 * TypeName (ie.setof) */
	List	   *withClause;		/* a list of ParamString */
	char	   *as;				/* the SQL statement or filename */
	char	   *language;		/* C or SQL */
} ProcedureStmt;

/* ----------------------
 *		Drop Aggregate Statement
 * ----------------------
 */
typedef struct RemoveAggrStmt
{
	NodeTag		type;
	char	   *aggname;		/* aggregate to drop */
	char	   *aggtype;		/* for this type */
} RemoveAggrStmt;

/* ----------------------
 *		Drop Function Statement
 * ----------------------
 */
typedef struct RemoveFuncStmt
{
	NodeTag		type;
	char	   *funcname;		/* function to drop */
	List	   *args;			/* types of the arguments */
} RemoveFuncStmt;

/* ----------------------
 *		Drop Operator Statement
 * ----------------------
 */
typedef struct RemoveOperStmt
{
	NodeTag		type;
	char	   *opname;			/* operator to drop */
	List	   *args;			/* types of the arguments */
} RemoveOperStmt;

/* ----------------------
 *		Drop {Type|Index|Rule|View} Statement
 * ----------------------
 */
typedef struct RemoveStmt
{
	NodeTag		type;
	int			removeType;		/* P_TYPE|INDEX|RULE|VIEW */
	char	   *name;			/* name to drop */
} RemoveStmt;

/* ----------------------
 *		Alter Table Statement
 * ----------------------
 */
typedef struct RenameStmt
{
	NodeTag		type;
	char	   *relname;		/* relation to be altered */
	bool		inh;			/* recursively alter children? */
	char	   *column;			/* if NULL, rename the relation name to
								 * the new name. Otherwise, rename this
								 * column name. */
	char	   *newname;		/* the new name */
} RenameStmt;

/* ----------------------
 *		Create Rule Statement
 * ----------------------
 */
typedef struct RuleStmt
{
	NodeTag		type;
	char	   *rulename;		/* name of the rule */
	Node	   *whereClause;	/* qualifications */
	CmdType		event;			/* RETRIEVE */
	struct Attr *object;		/* object affected */
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
	char	   *relname;		/* relation to notify */
} NotifyStmt;

/* ----------------------
 *		Listen Statement
 * ----------------------
 */
typedef struct ListenStmt
{
	NodeTag		type;
	char	   *relname;		/* relation to listen on */
} ListenStmt;

/* ----------------------
 *		Unlisten Statement
 * ----------------------
 */
typedef struct UnlistenStmt
{
	NodeTag		type;
	char	   *relname;		/* relation to unlisten on */
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
	char	   *viewname;		/* name of the view */
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
	char	   *dbname;			/* database to create */
	char	   *dbpath;			/* location of database */
	int			encoding;		/* default encoding (see regex/pg_wchar.h) */
} CreatedbStmt;

/* ----------------------
 *		Destroydb Statement
 * ----------------------
 */
typedef struct DestroydbStmt
{
	NodeTag		type;
	char	   *dbname;			/* database to drop */
} DestroydbStmt;

/* ----------------------
 *		Cluster Statement (support pbrown's cluster index implementation)
 * ----------------------
 */
typedef struct ClusterStmt
{
	NodeTag		type;
	char	   *relname;		/* relation being indexed */
	char	   *indexname;		/* original index defined */
} ClusterStmt;

/* ----------------------
 *		Vacuum Statement
 * ----------------------
 */
typedef struct VacuumStmt
{
	NodeTag		type;
	bool		verbose;		/* print status info */
	bool		analyze;		/* analyze data */
	char	   *vacrel;			/* table to vacuum */
	List	   *va_spec;		/* columns to analyse */
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
} ExplainStmt;

/* ----------------------
 * Set Statement
 * ----------------------
 */

typedef struct VariableSetStmt
{
	NodeTag		type;
	char	   *name;
	char	   *value;
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
	char	   *relname;		/* relation to lock */
	int			mode;			/* lock mode */
} LockStmt;

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
	char	   *relname;		/* relation to insert into */
	char	   *unique;			/* NULL, '*', or unique attribute name */
	List	   *cols;			/* names of the columns */
	List	   *targetList;		/* the target list (of ResTarget) */
	List	   *fromClause;		/* the from clause */
	Node	   *whereClause;	/* qualifications */
	List	   *groupClause;	/* group by clause */
	Node	   *havingClause;	/* having conditional-expression */
	List	   *unionClause;	/* union subselect parameters */
	bool		unionall;		/* union without unique sort */
	/***S*I***/
	List	   *intersectClause;
	List	   *forUpdate;		/* FOR UPDATE clause */
} InsertStmt;

/* ----------------------
 *		Delete Statement
 * ----------------------
 */
typedef struct DeleteStmt
{
	NodeTag		type;
	char	   *relname;		/* relation to delete from */
	Node	   *whereClause;	/* qualifications */
} DeleteStmt;

/* ----------------------
 *		Update Statement
 * ----------------------
 */
typedef struct UpdateStmt
{
	NodeTag		type;
	char	   *relname;		/* relation to update */
	List	   *targetList;		/* the target list (of ResTarget) */
	Node	   *whereClause;	/* qualifications */
	List	   *fromClause;		/* the from clause */
} UpdateStmt;

/* ----------------------
 *		Select Statement
 * ----------------------
 */
typedef struct SelectStmt
{
	NodeTag		type;
	char	   *unique;			/* NULL, '*', or unique attribute name */
	char	   *into;			/* name of table (for select into table) */
	List	   *targetList;		/* the target list (of ResTarget) */
	List	   *fromClause;		/* the from clause */
	Node	   *whereClause;	/* qualifications */
	List	   *groupClause;	/* group by clause */
	Node	   *havingClause;	/* having conditional-expression */
	/***S*I***/
	List	   *intersectClause;
	List	   *exceptClause;

	List	   *unionClause;	/* union subselect parameters */
	List	   *sortClause;		/* sort clause (a list of SortGroupBy's) */
	char	   *portalname;		/* the portal (cursor) to create */
	bool		binary;			/* a binary (internal) portal? */
	bool		istemp;			/* into is a temp table */
	bool		unionall;		/* union without unique sort */
	Node	   *limitOffset;	/* # of result tuples to skip */
	Node	   *limitCount;		/* # of result tuples to return */
	List	   *forUpdate;		/* FOR UPDATE clause */
} SelectStmt;

/****************************************************************************
 *	Supporting data structures for Parse Trees
 ****************************************************************************/

/*
 * TypeName - specifies a type in definitions
 */
typedef struct TypeName
{
	NodeTag		type;
	char	   *name;			/* name of the type */
	bool		timezone;		/* timezone specified? */
	bool		setof;			/* is a set? */
	int32		typmod;			/* type modifier */
	List	   *arrayBounds;	/* array bounds */
} TypeName;

/*
 * ParamNo - specifies a parameter reference
 */
typedef struct ParamNo
{
	NodeTag		type;
	int			number;			/* the number of the parameter */
	TypeName   *typename;		/* the typecast */
	List	   *indirection;	/* array references */
} ParamNo;

/*
 * A_Expr - binary expressions
 */
typedef struct A_Expr
{
	NodeTag		type;
	int			oper;			/* type of operation
								 * {OP,OR,AND,NOT,ISNULL,NOTNULL} */
	char	   *opname;			/* name of operator/function */
	Node	   *lexpr;			/* left argument */
	Node	   *rexpr;			/* right argument */
} A_Expr;

/*
 * Attr -
 *	  specifies an Attribute (ie. a Column); could have nested dots or
 *	  array references.
 *
 */
typedef struct Attr
{
	NodeTag		type;
	char	   *relname;		/* name of relation (can be "*") */
	ParamNo    *paramNo;		/* or a parameter */
	List	   *attrs;			/* attributes (possibly nested); list of
								 * Values (strings) */
	List	   *indirection;	/* array refs (list of A_Indices') */
} Attr;

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

/*
 * ColumnDef - column definition (used in various creates)
 */
typedef struct ColumnDef
{
	NodeTag		type;
	char	   *colname;		/* name of column */
	TypeName   *typename;		/* type of column */
	bool		is_not_null;	/* flag to NOT NULL constraint */
	bool		is_sequence;	/* is a sequence? */
	char	   *defval;			/* default value of column */
	List	   *constraints;	/* constraints on column */
} ColumnDef;

/*
 * Ident -
 *	  an identifier (could be an attribute or a relation name). Depending
 *	  on the context at transformStmt time, the identifier is treated as
 *	  either a relation name (in which case, isRel will be set) or an
 *	  attribute (in which case, it will be transformed into an Attr).
 */
typedef struct Ident
{
	NodeTag		type;
	char	   *name;			/* its name */
	List	   *indirection;	/* array references */
	bool		isRel;			/* is a relation - filled in by
								 * transformExpr() */
} Ident;

/*
 * FuncCall - a function/aggregate invocation
 */
typedef struct FuncCall
{
	NodeTag		type;
	char	   *funcname;		/* name of function */
	List	   *args;			/* the arguments (list of exprs) */
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
 * ResTarget -
 *	  result target (used in target list of pre-transformed Parse trees)
 */
typedef struct ResTarget
{
	NodeTag		type;
	char	   *name;			/* name of the result column */
	List	   *indirection;	/* array references */
	Node	   *val;			/* the value of the result (A_Expr or
								 * Attr) (or A_Const) */
} ResTarget;

/*
 * ParamString - used in WITH clauses
 */
typedef struct ParamString
{
	NodeTag		type;
	char	   *name;
	char	   *val;
} ParamString;

/*
 * RelExpr - relation expressions
 */
typedef struct RelExpr
{
	NodeTag		type;
	char	   *relname;		/* the relation name */
	bool		inh;			/* inheritance query */
} RelExpr;

/*
 * SortGroupBy - for ORDER BY clause
 */
typedef struct SortGroupBy
{
	NodeTag		type;
	char	   *useOp;			/* operator to use */
	Node	   *node;			/* Expression  */
} SortGroupBy;

/*
 * RangeVar - range variable, used in FROM clauses
 */
typedef struct RangeVar
{
	NodeTag		type;
	RelExpr    *relExpr;		/* the relation expression */
	char	   *name;			/* the name to be referenced (optional) */
} RangeVar;

/*
 * IndexElem - index parameters (used in CREATE INDEX)
 */
typedef struct IndexElem
{
	NodeTag		type;
	char	   *name;			/* name of index */
	List	   *args;			/* if not NULL, function index */
	char	   *class;
	TypeName   *typename;		/* type of index's keys (optional) */
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

/*
 * JoinExpr - for JOIN expressions
 */
typedef struct JoinExpr
{
	NodeTag		type;
	int			jointype;
	RangeVar   *larg;
	Node	   *rarg;
	List	   *quals;
} JoinExpr;


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
	Node	   *expr;			/* can be a list too */
} TargetEntry;

/*
 * RangeTblEntry -
 *	  used in range tables. Some of the following are only used in one of
 *	  the parsing, optimizing, execution stages.
 *
 *	  inFromCl marks those range variables that are listed in the from clause.
 *	  In SQL, the targetlist can only refer to range variables listed in the
 *	  from clause but POSTQUEL allows you to refer to tables not specified, in
 *	  which case a range table entry will be generated. We use POSTQUEL
 *	  semantics which is more powerful. However, we need SQL semantics in
 *	  some cases (eg. when expanding a '*')
 */
typedef struct RangeTblEntry
{
	NodeTag		type;
	char	   *relname;		/* real name of the relation */
	char	   *refname;		/* the reference name (specified in the
								 * from clause) */
	Oid			relid;
	bool		inh;			/* inheritance? */
	bool		inFromCl;		/* comes from From Clause */
	bool		skipAcl;		/* skip ACL check in executor */
} RangeTblEntry;

/*
 * SortClause -
 *	   used in the sort clause for retrieves and cursors
 */
typedef struct SortClause
{
	NodeTag		type;
	Resdom	   *resdom;			/* attributes in tlist to be sorted */
	Oid			opoid;			/* sort operators */
} SortClause;

/*
 * GroupClause -
 *	   used in the GROUP BY clause
 */
typedef struct GroupClause
{
	NodeTag		type;
	Oid			grpOpoid;		/* the sort operator to use */
	Index		tleGroupref;	/* reference into targetlist */
} GroupClause;

#define ROW_MARK_FOR_UPDATE		(1 << 0)
#define ROW_ACL_FOR_UPDATE		(1 << 1)

typedef struct RowMark
{
	NodeTag		type;
	Index		rti;			/* index in Query->rtable */
	bits8		info;			/* as above */
} RowMark;

#endif	 /* PARSENODES_H */
