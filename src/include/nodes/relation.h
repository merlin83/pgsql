/*-------------------------------------------------------------------------
 *
 * relation.h
 *	  Definitions for internal planner nodes.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: relation.h,v 1.34 1999-07-15 23:03:56 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef RELATION_H
#define RELATION_H

#include "nodes/parsenodes.h"

/*
 * Relids
 *		List of relation identifiers (indexes into the rangetable).
 */

typedef List *Relids;

/*
 * RelOptInfo
 *		Per-base-relation information
 *
 *		Parts of this data structure are specific to various scan and join
 *		mechanisms.  It didn't seem worth creating new node types for them.
 *
 *		relids - List of relation indentifiers
 *		indexed - true if the relation has secondary indices
 *		pages - number of pages in the relation
 *		tuples - number of tuples in the relation
 *		size - number of tuples in the relation after restrictions clauses
 *			   have been applied
 *		width - number of bytes per tuple in the relation after the
 *				appropriate projections have been done
 *		targetlist - List of TargetList nodes
 *		pathlist - List of Path nodes, one for each possible method of
 *				   generating the relation
 *		cheapestpath -	least expensive Path (regardless of final order)
 *		pruneable - flag to let the planner know whether it can prune the plan
 *					space of this RelOptInfo or not.
 *
 *	 * If the relation is a (secondary) index it will have the following
 *		three fields:
 *
 *		classlist - List of PG_AMOPCLASS OIDs for the index
 *		indexkeys - List of base-relation attribute numbers that are index keys
 *		ordering - List of PG_OPERATOR OIDs which order the indexscan result
 *		relam	  - the OID of the pg_am of the index
 *
 *	 * The presence of the remaining fields depends on the restrictions
 *		and joins which the relation participates in:
 *
 *		restrictinfo - List of RestrictInfo nodes, containing info about each
 *					 qualification clause in which this relation participates
 *		joininfo  - List of JoinInfo nodes, containing info about each join
 *					clause in which this relation participates
 *		innerjoin - List of Path nodes that represent indices that may be used
 *					as inner paths of nestloop joins
 *
 * NB. the last element of the arrays classlist, indexkeys and ordering
 *	   is always 0.								2/95 - ay
 */

typedef struct RelOptInfo
{
	NodeTag		type;

	/* all relations: */
	Relids		relids;			/* integer list of base relids involved */

	/* catalog statistics information */
	bool		indexed;
	int			pages;
	int			tuples;
	int			size;
	int			width;

	/* materialization information */
	List	   *targetlist;
	List	   *pathlist;		/* Path structures */
	struct Path *cheapestpath;
	bool		pruneable;

	/* used solely by indices: */
	Oid		   *classlist;		/* classes of AM operators */
	int		   *indexkeys;		/* keys over which we're indexing */
	Oid			relam;			/* OID of the access method (in pg_am) */

	Oid			indproc;
	List	   *indpred;

	/* used by various scans and joins: */
	Oid		   *ordering;		/* OID of operators in sort order */
	List	   *restrictinfo;	/* RestrictInfo structures */
	List	   *joininfo;		/* JoinInfo structures */
	List	   *innerjoin;
} RelOptInfo;

extern Var *get_expr(TargetEntry *foo);
extern Var *get_groupclause_expr(GroupClause *groupClause, List *targetList);

typedef struct MergeOrder
{
	NodeTag		type;
	Oid			join_operator;
	Oid			left_operator;
	Oid			right_operator;
	Oid			left_type;
	Oid			right_type;
} MergeOrder;

typedef enum OrderType
{
	MERGE_ORDER, SORTOP_ORDER
} OrderType;

typedef struct PathOrder
{
	NodeTag		type;

	OrderType	ordtype;
	union
	{
		Oid		   *sortop;
		MergeOrder *merge;
	}			ord;
} PathOrder;

typedef struct Path
{
	NodeTag		type;

	RelOptInfo *parent;
	Cost		path_cost;

	NodeTag		pathtype;

	PathOrder  *pathorder;

	List	   *pathkeys;		/* This is a List of List of Var nodes.
								 * See the top of
								 * optimizer/path/pathkeys.c for more
								 * information. */
	Cost		outerjoincost;
	Relids		joinid;
	List	   *loc_restrictinfo;
} Path;

typedef struct IndexPath
{
	Path		path;
	List	   *indexid;
	List	   *indexqual;
	int		   *indexkeys;		/* to transform heap attnos into index
								 * ones */
} IndexPath;

typedef struct NestPath
{
	Path		path;
	List	   *pathinfo;
	Path	   *outerjoinpath;
	Path	   *innerjoinpath;
} NestPath;

typedef NestPath JoinPath;

typedef struct MergePath
{
	JoinPath	jpath;
	List	   *path_mergeclauses;
	List	   *outersortkeys;
	List	   *innersortkeys;
} MergePath;

typedef struct HashPath
{
	JoinPath	jpath;
	List	   *path_hashclauses;
	List	   *outerhashkeys;
	List	   *innerhashkeys;
} HashPath;

/*
 * Keys
 */

typedef struct OrderKey
{
	NodeTag		type;
	int			attribute_number;
	Index		array_index;
} OrderKey;

typedef struct JoinKey
{
	NodeTag		type;
	Var		   *outer;
	Var		   *inner;
} JoinKey;

/*
 * clause info
 */

typedef struct RestrictInfo
{
	NodeTag		type;
	Expr	   *clause;			/* should be an OP clause */
	Cost		selectivity;
	bool		notclause;
	List	   *indexids;

	/* mergejoin only */
	MergeOrder *mergejoinorder;

	/* hashjoin only */
	Oid			hashjoinoperator;
	Relids		restrictinfojoinid;
} RestrictInfo;

typedef struct JoinMethod
{
	NodeTag		type;
	List	   *jmkeys;
	List	   *clauses;
} JoinMethod;

typedef struct HashInfo
{
	JoinMethod	jmethod;
	Oid			hashop;
} HashInfo;

typedef struct MergeInfo
{
	JoinMethod	jmethod;
	MergeOrder *m_ordering;
} MergeInfo;

typedef struct JoinInfo
{
	NodeTag		type;
	Relids		unjoined_relids;
	List	   *jinfo_restrictinfo;
	bool		mergejoinable;
	bool		hashjoinable;
} JoinInfo;

typedef struct Iter
{
	NodeTag		type;
	Node	   *iterexpr;
	Oid			itertype;		/* type of the iter expr (use for type
								 * checking) */
} Iter;

/*
 *	Stream:
 *	A stream represents a root-to-leaf path in a plan tree (i.e. a tree of
 *	JoinPaths and Paths).  The stream includes pointers to all Path nodes,
 *	as well as to any clauses that reside above Path nodes. This structure
 *	is used to make Path nodes and clauses look similar, so that Predicate
 *	Migration can run.
 *
 *	pathptr -- pointer to the current path node
 *	cinfo -- if NULL, this stream node referes to the path node.
 *			  Otherwise this is a pointer to the current clause.
 *	clausetype -- whether cinfo is in loc_restrictinfo or pathinfo in the
 *			  path node
 *	upstream -- linked list pointer upwards
 *	downstream -- ditto, downwards
 *	groupup -- whether or not this node is in a group with the node upstream
 *	groupcost -- total cost of the group that node is in
 *	groupsel -- total selectivity of the group that node is in
 */
typedef struct Stream *StreamPtr;

typedef struct Stream
{
	NodeTag		type;
	Path	   *pathptr;
	RestrictInfo *cinfo;
	int		   *clausetype;
	struct Stream *upstream;
	struct Stream *downstream;
	bool		groupup;
	Cost		groupcost;
	Cost		groupsel;
} Stream;

#endif	 /* RELATION_H */
