/*-------------------------------------------------------------------------
 *
 * joininfo.c--
 *	  JoinInfo node manipulation routines
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/joininfo.c,v 1.14 1999-02-10 21:02:40 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "nodes/relation.h"

#include "optimizer/internal.h"
#include "optimizer/joininfo.h"
#include "optimizer/var.h"
#include "optimizer/clauses.h"


/*
 * joininfo-member--
 *	  Determines whether a node has already been created for a join
 *	  between a set of join relations and the relation described by
 *	  'joininfo-list'.
 *
 * 'join-relids' is a list of relids corresponding to the join relation
 * 'joininfo-list' is the list of joininfo nodes against which this is
 *				checked
 *
 * Returns the corresponding node in 'joininfo-list' if such a node
 * exists.
 *
 */
JoinInfo   *
joininfo_member(List *join_relids, List *joininfo_list)
{
	List	   *i = NIL;
	List	   *other_rels = NIL;

	foreach(i, joininfo_list)
	{
		other_rels = lfirst(i);
		if (same(join_relids, ((JoinInfo *) other_rels)->otherrels))
			return (JoinInfo *) other_rels;
	}
	return (JoinInfo *) NULL;
}


/*
 * find-joininfo-node--
 *	  Find the joininfo node within a relation entry corresponding
 *	  to a join between 'this_rel' and the relations in 'join-relids'.	A
 *	  new node is created and added to the relation entry's joininfo
 *	  field if the desired one can't be found.
 *
 * Returns a joininfo node.
 *
 */
JoinInfo   *
find_joininfo_node(RelOptInfo *this_rel, List *join_relids)
{
	JoinInfo   *joininfo = joininfo_member(join_relids,
										   this_rel->joininfo);

	if (joininfo == NULL)
	{
		joininfo = makeNode(JoinInfo);
		joininfo->otherrels = join_relids;
		joininfo->jinfo_restrictinfo = NIL;
		joininfo->mergejoinable = false;
		joininfo->hashjoinable = false;
		joininfo->inactive = false;
		this_rel->joininfo = lcons(joininfo, this_rel->joininfo);
	}
	return joininfo;
}

/*
 * other-join-clause-var--
 *	  Determines whether a var node is contained within a joinclause
 *	  of the form(op var var).
 *
 * Returns the other var node in the joinclause if it is, nil if not.
 *
 */
Var *
other_join_clause_var(Var *var, Expr *clause)
{
	Var		   *retval;
	Var		   *l,
			   *r;

	retval = (Var *) NULL;

	if (var != NULL && is_joinable((Node *) clause))
	{
		l = (Var *) get_leftop(clause);
		r = (Var *) get_rightop(clause);

		if (var_equal(var, l))
			retval = r;
		else if (var_equal(var, r))
			retval = l;
	}

	return retval;
}
