/*-------------------------------------------------------------------------
 *
 * relation.h
 *	  Definitions for internal planner nodes.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: relation.h,v 1.41 2000-01-22 23:50:25 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef RELATION_H
#define RELATION_H

#include "nodes/parsenodes.h"

/*
 * Relids
 *		List of relation identifiers (indexes into the rangetable).
 *
 *		Note: these are lists of integers, not Nodes.
 */

typedef List *Relids;

/*
 * RelOptInfo
 *		Per-relation information for planning/optimization
 *
 *		Parts of this data structure are specific to various scan and join
 *		mechanisms.  It didn't seem worth creating new node types for them.
 *
 *		relids - List of base-relation identifiers; it is a base relation
 *				if there is just one, a join relation if more than one
 *		rows - estimated number of tuples in the relation after restriction
 *			   clauses have been applied (ie, output rows of a plan for it)
 *		width - avg. number of bytes per tuple in the relation after the
 *				appropriate projections have been done (ie, output width)
 *		targetlist - List of TargetList nodes
 *		pathlist - List of Path nodes, one for each potentially useful
 *				   method of generating the relation
 *		cheapestpath -	least expensive Path (regardless of ordering)
 *		pruneable - flag to let the planner know whether it can prune the
 *					pathlist of this RelOptInfo or not.
 *
 *	 * If the relation is a base relation it will have these fields set:
 *
 *		indexed - true if the relation has secondary indices
 *		pages - number of disk pages in relation
 *		tuples - number of tuples in relation (not considering restrictions)
 *
 *	 * The presence of the remaining fields depends on the restrictions
 *		and joins that the relation participates in:
 *
 *		restrictinfo - List of RestrictInfo nodes, containing info about each
 *					 qualification clause in which this relation participates
 *		joininfo  - List of JoinInfo nodes, containing info about each join
 *					clause in which this relation participates
 *		innerjoin - List of Path nodes that represent indices that may be used
 *					as inner paths of nestloop joins. This field is non-null
 *					only for base rels, since join rels have no indices.
 */

typedef struct RelOptInfo
{
	NodeTag		type;

	/* all relations included in this RelOptInfo */
	Relids		relids;			/* integer list of base relids (RT indexes) */

	/* size estimates generated by planner */
	double		rows;			/* estimated number of result tuples */
	int			width;			/* estimated avg width of result tuples */

	/* materialization information */
	List	   *targetlist;
	List	   *pathlist;		/* Path structures */
	struct Path *cheapestpath;
	bool		pruneable;

	/* statistics from pg_class (only valid if it's a base rel!) */
	bool		indexed;
	long		pages;
	double		tuples;

	/* used by various scans and joins: */
	List	   *restrictinfo;	/* RestrictInfo structures */
	List	   *joininfo;		/* JoinInfo structures */
	List	   *innerjoin;		/* potential indexscans for nestloop joins */
	/* innerjoin indexscans are not in the main pathlist because they are
	 * not usable except in specific join contexts; we have to test before
	 * seeing whether they can be used.
	 */
} RelOptInfo;

/*
 * IndexOptInfo
 *		Per-index information for planning/optimization
 *
 *		Prior to Postgres 7.0, RelOptInfo was used to describe both relations
 *		and indexes, but that created confusion without actually doing anything
 *		useful.  So now we have a separate IndexOptInfo struct for indexes.
 *
 *		indexoid - OID of the index relation itself
 *		pages - number of disk pages in index
 *		tuples - number of index tuples in index
 *		classlist - List of PG_AMOPCLASS OIDs for the index
 *		indexkeys - List of base-relation attribute numbers that are index keys
 *		ordering - List of PG_OPERATOR OIDs which order the indexscan result
 *		relam	  - the OID of the pg_am of the index
 *		amcostestimate - OID of the relam's cost estimator
 *		indproc	  - OID of the function if a functional index, else 0
 *		indpred	  - index predicate if a partial index, else NULL
 *
 *		NB. the last element of the arrays classlist, indexkeys and ordering
 *			is always 0.
 */

typedef struct IndexOptInfo
{
	NodeTag		type;

	Oid			indexoid;		/* OID of the index relation */

	/* statistics from pg_class */
	long		pages;
	double		tuples;

	/* index descriptor information */
	Oid		   *classlist;		/* classes of AM operators */
	int		   *indexkeys;		/* keys over which we're indexing */
	Oid		   *ordering;		/* OIDs of sort operators for each key */
	Oid			relam;			/* OID of the access method (in pg_am) */

	RegProcedure amcostestimate; /* OID of the access method's cost fcn */

	Oid			indproc;		/* if a functional index */
	List	   *indpred;		/* if a partial index */
} IndexOptInfo;

/*
 * PathKeys
 *
 *	The sort ordering of a path is represented by a list of sublists of
 *	PathKeyItem nodes.  An empty list implies no known ordering.  Otherwise
 *	the first sublist represents the primary sort key, the second the
 *	first secondary sort key, etc.  Each sublist contains one or more
 *	PathKeyItem nodes, each of which can be taken as the attribute that
 *	appears at that sort position.  (See the top of optimizer/path/pathkeys.c
 *	for more information.)
 */

typedef struct PathKeyItem
{
	NodeTag		type;

	Node	   *key;			/* the item that is ordered */
	Oid			sortop;			/* the ordering operator ('<' op) */
	/*
	 * key typically points to a Var node, ie a relation attribute,
	 * but it can also point to a Func clause representing the value
	 * indexed by a functional index.  Someday we might allow arbitrary
	 * expressions as path keys, so don't assume more than you must.
	 */
} PathKeyItem;

/*
 * Type "Path" is used as-is for sequential-scan paths.  For other
 * path types it is the first component of a larger struct.
 */

typedef struct Path
{
	NodeTag		type;

	RelOptInfo *parent;			/* the relation this path can build */

	Cost		path_cost;		/* estimated execution cost of path */

	NodeTag		pathtype;		/* tag identifying scan/join method */
	/* XXX why is pathtype separate from the NodeTag? */

	List	   *pathkeys;		/* sort ordering of path's output */
	/* pathkeys is a List of Lists of PathKeyItem nodes; see above */
} Path;

/*----------
 * IndexPath represents an index scan.  Although an indexscan can only read
 * a single relation, it can scan it more than once, potentially using a
 * different index during each scan.  The result is the union (OR) of all the
 * tuples matched during any scan.  (The executor is smart enough not to return
 * the same tuple more than once, even if it is matched in multiple scans.)
 *
 * 'indexid' is a list of index relation OIDs, one per scan to be performed.
 * 'indexqual' is a list of index qualifications, also one per scan.
 * Each entry in 'indexqual' is a sublist of qualification expressions with
 * implicit AND semantics across the sublist items.  Only expressions that
 * are usable as indexquals (as determined by indxpath.c) may appear here.
 *
 * NOTE that the semantics of the top-level list in 'indexqual' is OR
 * combination, while the sublists are implicitly AND combinations!
 *----------
 */

typedef struct IndexPath
{
	Path		path;
	List	   *indexid;
	List	   *indexqual;
	/*
	 * joinrelids is only used in IndexPaths that are constructed for use
	 * as the inner path of a nestloop join.  These paths have indexquals
	 * that refer to values of other rels, so those other rels must be
	 * included in the outer joinrel in order to make a usable join.
	 */
	Relids		joinrelids;			/* other rels mentioned in indexqual */
} IndexPath;

typedef struct TidPath
{
	Path	path;
	List	*tideval;
	Relids	unjoined_relids; /* some rels not yet part of my Path */
} TidPath;  

/*
 * All join-type paths share these fields.
 */

typedef struct JoinPath
{
	Path		path;

	Path	   *outerjoinpath;	/* path for the outer side of the join */
	Path	   *innerjoinpath;	/* path for the inner side of the join */
} JoinPath;

/*
 * A nested-loop path needs no special fields.
 */

typedef JoinPath NestPath;

/*
 * A mergejoin path has these fields.
 *
 * Note that the mergeclauses are a subset of the parent relation's
 * restriction-clause list.  Any join clauses that are not mergejoinable
 * appear only in the parent's restrict list, and must be checked by a
 * qpqual at execution time.
 */

typedef struct MergePath
{
	JoinPath	jpath;
	List	   *path_mergeclauses; /* join clauses used for merge */
	/*
	 * outersortkeys (resp. innersortkeys) is NIL if the outer path
	 * (resp. inner path) is already ordered appropriately for the
	 * mergejoin.  If it is not NIL then it is a PathKeys list describing
	 * the ordering that must be created by an explicit sort step.
	 */
	List	   *outersortkeys;
	List	   *innersortkeys;
} MergePath;

/*
 * A hashjoin path has these fields.
 *
 * The remarks above for mergeclauses apply for hashclauses as well.
 * However, hashjoin does not care what order its inputs appear in,
 * so we have no need for sortkeys.
 */

typedef struct HashPath
{
	JoinPath	jpath;
	List	   *path_hashclauses; /* join clauses used for hashing */
} HashPath;

/*
 * Restriction clause info.
 *
 * We create one of these for each AND sub-clause of a restriction condition
 * (WHERE clause).  Since the restriction clauses are logically ANDed, we
 * can use any one of them or any subset of them to filter out tuples,
 * without having to evaluate the rest.  The RestrictInfo node itself stores
 * data used by the optimizer while choosing the best query plan.
 *
 * A restriction clause will appear in the restrictinfo list of a RelOptInfo
 * that describes exactly the set of base relations referenced by the
 * restriction clause.  It is not possible to apply the clause at any lower
 * nesting level, and there is little point in delaying its evaluation to
 * higher nesting levels.  (The "predicate migration" code was once intended
 * to push restriction clauses up and down the plan tree, but it's dead code
 * and is unlikely to be resurrected in the foreseeable future.)
 *
 * If a restriction clause references more than one base rel, it will also
 * appear in the JoinInfo list of every RelOptInfo that describes a strict
 * subset of the base rels mentioned in the clause.  The JoinInfo lists are
 * used to drive join tree building by selecting plausible join candidates.
 *
 * In general, the referenced clause might be arbitrarily complex.  The
 * kinds of clauses we can handle as indexscan quals, mergejoin clauses,
 * or hashjoin clauses are fairly limited --- the code for each kind of
 * path is responsible for identifying the restrict clauses it can use
 * and ignoring the rest.  Clauses not implemented by an indexscan,
 * mergejoin, or hashjoin will be placed in the qpqual field of the
 * final Plan node, where they will be enforced by general-purpose
 * qual-expression-evaluation code.  (But we are still entitled to count
 * their selectivity when estimating the result tuple count, if we
 * can guess what it is...)
 */

typedef struct RestrictInfo
{
	NodeTag		type;

	Expr	   *clause;			/* the represented clause of WHERE cond */

	/* only used if clause is an OR clause: */
	List	   *subclauseindices;	/* indexes matching subclauses */
	/* subclauseindices is a List of Lists of IndexOptInfos */

	/* valid if clause is mergejoinable, else InvalidOid: */
	Oid			mergejoinoperator;	/* copy of clause operator */
	Oid			left_sortop;	/* leftside sortop needed for mergejoin */
	Oid			right_sortop;	/* rightside sortop needed for mergejoin */

	/* valid if clause is hashjoinable, else InvalidOid: */
	Oid			hashjoinoperator;	/* copy of clause operator */
} RestrictInfo;

/*
 * Join clause info.
 *
 * We make a list of these for each RelOptInfo, containing info about
 * all the join clauses this RelOptInfo participates in.  (For this
 * purpose, a "join clause" is a WHERE clause that mentions both vars
 * belonging to this relation and vars belonging to relations not yet
 * joined to it.)  We group these clauses according to the set of
 * other base relations (unjoined relations) mentioned in them.
 * There is one JoinInfo for each distinct set of unjoined_relids,
 * and its jinfo_restrictinfo lists the clause(s) that use that set
 * of other relations.
 */

typedef struct JoinInfo
{
	NodeTag		type;
	Relids		unjoined_relids; /* some rels not yet part of my RelOptInfo */
	List	   *jinfo_restrictinfo;	/* relevant RestrictInfos */
} JoinInfo;

/*
 *	Stream:
 *	A stream represents a root-to-leaf path in a plan tree (i.e. a tree of
 *	JoinPaths and Paths).  The stream includes pointers to all Path nodes,
 *	as well as to any clauses that reside above Path nodes. This structure
 *	is used to make Path nodes and clauses look similar, so that Predicate
 *	Migration can run.
 *
 *	XXX currently, Predicate Migration is dead code, and so is this node type.
 *	Probably should remove support for it.
 *
 *	pathptr -- pointer to the current path node
 *	cinfo -- if NULL, this stream node referes to the path node.
 *			  Otherwise this is a pointer to the current clause.
 *	clausetype -- whether cinfo is in loc_restrictinfo or pathinfo in the
 *			  path node (XXX this is now used only by dead code, which is
 *			  good because the distinction no longer exists...)
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
	StreamPtr	upstream;
	StreamPtr	downstream;
	bool		groupup;
	Cost		groupcost;
	Selectivity	groupsel;
} Stream;

#endif	 /* RELATION_H */
