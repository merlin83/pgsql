/*-------------------------------------------------------------------------
 *
 * var.c
 *	  Var node manipulation routines
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/var.c,v 1.23 1999-08-22 20:14:54 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>

#include "postgres.h"

#include "optimizer/clauses.h"
#include "optimizer/var.h"


static bool pull_varnos_walker(Node *node, List **listptr);
static bool contain_var_clause_walker(Node *node, void *context);
static bool pull_var_clause_walker(Node *node, List **listptr);


/*
 *		pull_varnos
 *
 *		Create a list of all the distinct varnos present in a parsetree
 *		(tlist or qual).
 */
List *
pull_varnos(Node *node)
{
	List	   *result = NIL;

	pull_varnos_walker(node, &result);
	return result;
}

static bool
pull_varnos_walker(Node *node, List **listptr)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var	   *var = (Var *) node;
		if (!intMember(var->varno, *listptr))
			*listptr = lconsi(var->varno, *listptr);
		return false;
	}
	return expression_tree_walker(node, pull_varnos_walker, (void *) listptr);
}

/*
 * contain_var_clause
 *	  Recursively scan a clause to discover whether it contains any Var nodes.
 *
 *	  Returns true if any varnode found.
 */
bool
contain_var_clause(Node *clause)
{
	return contain_var_clause_walker(clause, NULL);
}

static bool
contain_var_clause_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
		return true;			/* abort the tree traversal and return true */
	return expression_tree_walker(node, contain_var_clause_walker, context);
}

/*
 * pull_var_clause
 *	  Recursively pulls all var nodes from an expression clause.
 *
 *	  Returns list of varnodes found.  Note the varnodes themselves are not
 *	  copied, only referenced.
 */
List *
pull_var_clause(Node *clause)
{
	List	   *result = NIL;

	pull_var_clause_walker(clause, &result);
	return result;
}

static bool
pull_var_clause_walker(Node *node, List **listptr)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		*listptr = lappend(*listptr, node);
		return false;
	}
	return expression_tree_walker(node, pull_var_clause_walker,
								  (void *) listptr);
}
