/*-------------------------------------------------------------------------
 *
 * parse_node.h
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: parse_node.h,v 1.14 1999-07-15 23:04:02 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_NODE_H
#define PARSE_NODE_H

#include "nodes/parsenodes.h"
#include "utils/rel.h"

/* state information used during parse analysis */
typedef struct ParseState
{
	int			p_last_resno;
	List	   *p_rtable;
	List	   *p_insert_columns;
	struct ParseState *parentParseState;
	bool		p_hasAggs;
	bool		p_hasSubLinks;
	bool		p_is_insert;
	bool		p_is_update;
	bool		p_is_rule;
	bool		p_in_where_clause;
	Relation	p_target_relation;
	RangeTblEntry *p_target_rangetblentry;
} ParseState;

extern ParseState *make_parsestate(ParseState *parentParseState);
extern Expr *make_op(char *opname, Node *ltree, Node *rtree);
extern Var *make_var(ParseState *pstate, Oid relid, char *refname,
		 char *attrname);
extern ArrayRef *make_array_ref(Node *expr,
			   List *indirection);
extern ArrayRef *make_array_set(Expr *target_expr,
			   List *upperIndexpr,
			   List *lowerIndexpr,
			   Expr *expr);
extern Const *make_const(Value *value);

#endif	 /* PARSE_NODE_H */
