/*-------------------------------------------------------------------------
 *
 * primnodes.h
 *	  Definitions for parse tree/query tree ("primitive") nodes.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: primnodes.h,v 1.34 1999-08-21 03:49:09 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PRIMNODES_H
#define PRIMNODES_H

#include "access/attnum.h"
#include "nodes/pg_list.h"
#include "utils/fcache.h"

/* ----------------------------------------------------------------
 *						node definitions
 * ----------------------------------------------------------------
 */

/* ----------------
 * Resdom (Result Domain)
 *		resno			- attribute number
 *		restype			- type of the value
 *		restypmod		- type-specific modifier of the value
 *		resname			- name of the resdom (could be NULL)
 *		ressortgroupref	- nonzero if referenced by a sort/group clause
 *		reskey			- order of key in a sort (for those > 0)
 *		reskeyop		- sort operator's regproc Oid
 *		resjunk			- set to true to eliminate the attribute
 *						  from final target list
 *
 * Notes:
 * ressortgroupref is the parse/plan-time representation of ORDER BY and
 * GROUP BY items.  Targetlist entries with ressortgroupref=0 are not
 * sort/group items.  If ressortgroupref>0, then this item is an ORDER BY or
 * GROUP BY value.  No two entries in a targetlist may have the same nonzero
 * ressortgroupref --- but there is no particular meaning to the nonzero
 * values, except as tags.  (For example, one must not assume that lower
 * ressortgroupref means a more significant sort key.)  The order of the
 * associated SortClause or GroupClause lists determine the semantics.
 *
 * reskey and reskeyop are the execution-time representation of sorting.
 * reskey must be zero in any non-sort-key item.  The reskey of sort key
 * targetlist items for a sort plan node is 1,2,...,n for the n sort keys.
 * The reskeyop of each such targetlist item is the sort operator's
 * regproc OID.  reskeyop will be zero in non-sort-key items.
 *
 * Both reskey and reskeyop are typically zero during parse/plan stages.
 * The executor does not pay any attention to ressortgroupref.
 * ----------------
 */
typedef struct Resdom
{
	NodeTag		type;
	AttrNumber	resno;
	Oid			restype;
	int32		restypmod;
	char	   *resname;
	Index		ressortgroupref;
	Index		reskey;
	Oid			reskeyop;
	bool		resjunk;
} Resdom;

/* -------------
 * Fjoin
 *		initialized		- true if the Fjoin has already been initialized for
 *						  the current target list evaluation
 *		nNodes			- The number of Iter nodes returning sets that the
 *						  node will flatten
 *		outerList		- 1 or more Iter nodes
 *		inner			- exactly one Iter node.  We eval every node in the
 *						  outerList once then eval the inner node to completion
 *						  pair the outerList result vector with each inner
 *						  result to form the full result.  When the inner has
 *						  been exhausted, we get the next outer result vector
 *						  and reset the inner.
 *		results			- The complete (flattened) result vector
 *		alwaysNull		- a null vector to indicate sets with a cardinality of
 *						  0, we treat them as the set {NULL}.
 */
typedef struct Fjoin
{
	NodeTag		type;
	bool		fj_initialized;
	int			fj_nNodes;
	List	   *fj_innerNode;
	DatumPtr	fj_results;
	BoolPtr		fj_alwaysDone;
} Fjoin;

/* ----------------
 * Expr
 *		typeOid			- oid of the type of this expression
 *		opType			- type of this expression
 *		oper			- the Oper node if it is an OPER_EXPR or the
 *						  Func node if it is a FUNC_EXPR
 *		args			- arguments to this expression
 * ----------------
 */
typedef enum OpType
{
	OP_EXPR, FUNC_EXPR, OR_EXPR, AND_EXPR, NOT_EXPR, SUBPLAN_EXPR
} OpType;

typedef struct Expr
{
	NodeTag		type;
	Oid			typeOid;		/* oid of the type of this expr */
	OpType		opType;			/* type of the op */
	Node	   *oper;			/* could be Oper or Func or SubPlan */
	List	   *args;			/* list of argument nodes */
} Expr;

/* ----------------
 * Var
 *		varno			- index of this var's relation in the range table
 *						  (could be INNER or OUTER)
 *		varattno		- attribute number of this var, or zero for all
 *		vartype			- pg_type tuple oid for the type of this var
 *		vartypmod		- pg_attribute typmod value
 *		varlevelsup		- for subquery variables referencing outer relations
 *		varnoold		- keep varno around in case it got changed to INNER/
 *						  OUTER (see match_varid)
 *		varoattno		- attribute number of this var
 *						  [ '(varnoold varoattno) was varid   -ay 2/95]
 * ----------------
 */
#define    INNER		65000
#define    OUTER		65001

#define    PRS2_CURRENT_VARNO			1
#define    PRS2_NEW_VARNO				2

typedef struct Var
{
	NodeTag		type;
	Index		varno;
	AttrNumber	varattno;
	Oid			vartype;
	int32		vartypmod;
	Index		varlevelsup;	/* erased by upper optimizer */
	Index		varnoold;		/* only used by optimizer */
	AttrNumber	varoattno;		/* only used by optimizer */
} Var;

/* ----------------
 * Oper
 *		opno			- PG_OPERATOR OID of the operator
 *		opid			- PG_PROC OID for the operator
 *		opresulttype	- PG_TYPE OID of the operator's return value
 *		opsize			- size of return result (cached by executor)
 *		op_fcache		- XXX comment me.
 *
 * ----
 * NOTE: in the good old days 'opno' used to be both (or either, or
 * neither) the pg_operator oid, and/or the pg_proc oid depending
 * on the postgres module in question (parser->pg_operator,
 * executor->pg_proc, planner->both), the mood of the programmer,
 * and the phase of the moon (rumors that it was also depending on the day
 * of the week are probably false). To make things even more postgres-like
 * (i.e. a mess) some comments were referring to 'opno' using the name
 * 'opid'. Anyway, now we have two separate fields, and of course that
 * immediately removes all bugs from the code...		[ sp :-) ].
 * ----------------
 */
typedef struct Oper
{
	NodeTag		type;
	Oid			opno;
	Oid			opid;
	Oid			opresulttype;
	int			opsize;
	FunctionCachePtr op_fcache;
} Oper;


/* ----------------
 * Const
 *		consttype - PG_TYPE OID of the constant's value
 *		constlen - length in bytes of the constant's value
 *		constvalue - the constant's value
 *		constisnull - whether the constant is null
 *				(if true, the other fields are undefined)
 *		constbyval - whether the information in constvalue
 *				if passed by value.  If true, then all the information
 *				is stored in the datum. If false, then the datum
 *				contains a pointer to the information.
 *		constisset - whether the const represents a set.  The const
 *				value corresponding will be the query that defines
 *				the set.
 * ----------------
 */
typedef struct Const
{
	NodeTag		type;
	Oid			consttype;
	int			constlen;
	Datum		constvalue;
	bool		constisnull;
	bool		constbyval;
	bool		constisset;
	bool		constiscast;
} Const;

/* ----------------
 * Param
 *		paramkind - specifies the kind of parameter. The possible values
 *		for this field are specified in "params.h", and they are:
 *
 *		PARAM_NAMED: The parameter has a name, i.e. something
 *				like `$.salary' or `$.foobar'.
 *				In this case field `paramname' must be a valid Name.
 *
 *		PARAM_NUM:	 The parameter has only a numeric identifier,
 *				i.e. something like `$1', `$2' etc.
 *				The number is contained in the `paramid' field.
 *
 *		PARAM_NEW:	 Used in PRS2 rule, similar to PARAM_NAMED.
 *					 The `paramname' and `paramid' refer to the "NEW" tuple
 *					 The `pramname' is the attribute name and `paramid'
 *					 is the attribute number.
 *
 *		PARAM_OLD:	 Same as PARAM_NEW, but in this case we refer to
 *				the "OLD" tuple.
 *
 *		paramid - numeric identifier for literal-constant parameters ("$1")
 *		paramname - attribute name for tuple-substitution parameters ("$.foo")
 *		paramtype - PG_TYPE OID of the parameter's value
 *		param_tlist - allows for projection in a param node.
 * ----------------
 */
typedef struct Param
{
	NodeTag		type;
	int			paramkind;
	AttrNumber	paramid;
	char	   *paramname;
	Oid			paramtype;
	List	   *param_tlist;
} Param;


/* ----------------
 * Func
 *		funcid			- PG_FUNCTION OID of the function
 *		functype		- PG_TYPE OID of the function's return value
 *		funcisindex		- the function can be evaluated by scanning an index
 *						  (set during query optimization)
 *		funcsize		- size of return result (cached by executor)
 *		func_fcache		- runtime state while running this function.  Where
 *						  we are in the execution of the function if it
 *						  returns more than one value, etc.
 *						  See utils/fcache.h
 *		func_tlist		- projection of functions returning tuples
 *		func_planlist	- result of planning this func, if it's a PQ func
 * ----------------
 */
typedef struct Func
{
	NodeTag		type;
	Oid			funcid;
	Oid			functype;
	bool		funcisindex;
	int			funcsize;
	FunctionCachePtr func_fcache;
	List	   *func_tlist;
	List	   *func_planlist;
} Func;

/* ----------------
 * Iter
 *		can anyone explain what this is for?  Seems to have something to do
 *		with evaluation of functions that return sets...
 * ----------------
 */
typedef struct Iter
{
	NodeTag		type;
	Node	   *iterexpr;
	Oid			itertype;		/* type of the iter expr (use for type
								 * checking) */
} Iter;

/* ----------------
 * Aggref
 *		aggname			- name of the aggregate
 *		basetype		- base type Oid of the aggregate
 *		aggtype			- type Oid of final result of the aggregate
 *		target			- attribute or expression we are aggregating on
 *		usenulls		- TRUE to accept null values as inputs
 *		aggno			- workspace for nodeAgg.c executor
 * ----------------
 */
typedef struct Aggref
{
	NodeTag		type;
	char	   *aggname;
	Oid			basetype;
	Oid			aggtype;
	Node	   *target;
	bool		usenulls;
	int			aggno;
} Aggref;

/* ----------------
 * SubLink
 *		subLinkType		- EXISTS, ALL, ANY, EXPR
 *		useor			- TRUE for <>
 *		lefthand		- list of Var/Const nodes on the left
 *		oper			- list of Oper nodes
 *		subselect		- subselect as Query* or parsetree
 * ----------------
 */
typedef enum SubLinkType
{
	EXISTS_SUBLINK, ALL_SUBLINK, ANY_SUBLINK, EXPR_SUBLINK
} SubLinkType;


typedef struct SubLink
{
	NodeTag		type;
	SubLinkType subLinkType;
	bool		useor;
	List	   *lefthand;
	List	   *oper;
	Node	   *subselect;
} SubLink;

/* ----------------
 * Array
 *		arrayelemtype	- type of the array's elements (homogenous!)
 *		arrayelemlength - length of that type
 *		arrayelembyval	- is the element type pass-by-value?
 *		arrayndim		- number of dimensions of the array
 *		arraylow		- base for array indexing
 *		arrayhigh		- limit for array indexing
 *		arraylen		- total length of array object
 * ----------------
 *
 *	memo from mao:	the array support we inherited from 3.1 is just
 *	wrong.	when time exists, we should redesign this stuff to get
 *	around a bunch of unfortunate implementation decisions made there.
 */
typedef struct Array
{
	NodeTag		type;
	Oid			arrayelemtype;
	int			arrayelemlength;
	bool		arrayelembyval;
	int			arrayndim;
	IntArray	arraylow;
	IntArray	arrayhigh;
	int			arraylen;
} Array;

/* ----------------
 *	ArrayRef: describes an array subscripting operation
 *
 * An ArrayRef can describe fetching a single element from an array,
 * fetching a subarray (array slice), storing a single element into
 * an array, or storing a slice.  The "store" cases work with an
 * initial array value and a source value that is inserted into the
 * appropriate part of the array.
 *
 *		refattrlength	- total length of array object
 *		refelemtype		- type of the result of the subscript operation
 *		refelemlength	- length of the array element type
 *		refelembyval	- is the element type pass-by-value?
 *		refupperindexpr - expressions that evaluate to upper array indexes
 *		reflowerindexpr	- expressions that evaluate to lower array indexes
 *		refexpr			- the expression that evaluates to an array value
 *		refassgnexpr	- expression for the source value, or NULL if fetch
 *
 * If reflowerindexpr = NIL, then we are fetching or storing a single array
 * element at the subscripts given by refupperindexpr.  Otherwise we are
 * fetching or storing an array slice, that is a rectangular subarray
 * with lower and upper bounds given by the index expressions.
 * reflowerindexpr must be the same length as refupperindexpr when it
 * is not NIL.
 *
 * Note: array types can be fixed-length (refattrlength > 0), but only
 * when the element type is itself fixed-length.  Otherwise they are
 * varlena structures and have refattrlength = -1.  In any case,
 * an array type is never pass-by-value.
 *
 * Note: currently, refelemtype is NOT the element type, but the array type,
 * when doing subarray fetch or either type of store.  It would be cleaner
 * to add more fields so we can distinguish the array element type from the
 * result type of the subscript operator...
 * ----------------
 */
typedef struct ArrayRef
{
	NodeTag		type;
	int			refattrlength;
	int			refelemlength;
	Oid			refelemtype;
	bool		refelembyval;
	List	   *refupperindexpr;
	List	   *reflowerindexpr;
	Node	   *refexpr;
	Node	   *refassgnexpr;
} ArrayRef;

#endif	 /* PRIMNODES_H */
