/*-------------------------------------------------------------------------
 *
 * clausesel.c
 *	  Routines to compute clause selectivities
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/path/clausesel.c,v 1.28 2000-01-23 02:06:58 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/pg_operator.h"
#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/internal.h"
#include "optimizer/plancat.h"
#include "optimizer/restrictinfo.h"
#include "parser/parsetree.h"
#include "utils/lsyscache.h"


/****************************************************************************
 *		ROUTINES TO COMPUTE SELECTIVITIES
 ****************************************************************************/

/*
 * restrictlist_selectivity -
 *	  Compute the selectivity of an implicitly-ANDed list of RestrictInfo
 *	  clauses.
 *
 * This is the same as clauselist_selectivity except for the representation
 * of the clause list.
 */
Selectivity
restrictlist_selectivity(Query *root,
						 List *restrictinfo_list,
						 int varRelid)
{
	List	   *clauselist = get_actual_clauses(restrictinfo_list);
	Selectivity	result;

	result = clauselist_selectivity(root, clauselist, varRelid);
	freeList(clauselist);
	return result;
}

/*
 * clauselist_selectivity -
 *	  Compute the selectivity of an implicitly-ANDed list of boolean
 *	  expression clauses.  The list can be empty, in which case 1.0
 *	  must be returned.
 *
 * See clause_selectivity() for the meaning of the varRelid parameter.
 */
Selectivity
clauselist_selectivity(Query *root,
					   List *clauses,
					   int varRelid)
{
	Selectivity		s1 = 1.0;
	List		   *clause;

	/* Use the product of the selectivities of the subclauses.
	 * XXX this is too optimistic, since the subclauses
	 * are very likely not independent...
	 */
	foreach(clause, clauses)
	{
		Selectivity	s2 = clause_selectivity(root,
											(Node *) lfirst(clause),
											varRelid);
		s1 = s1 * s2;
	}
	return s1;
}

/*
 * clause_selectivity -
 *	  Compute the selectivity of a general boolean expression clause.
 *
 * varRelid is either 0 or a rangetable index.
 *
 * When varRelid is not 0, only variables belonging to that relation are
 * considered in computing selectivity; other vars are treated as constants
 * of unknown values.  This is appropriate for estimating the selectivity of
 * a join clause that is being used as a restriction clause in a scan of a
 * nestloop join's inner relation --- varRelid should then be the ID of the
 * inner relation.
 *
 * When varRelid is 0, all variables are treated as variables.  This
 * is appropriate for ordinary join clauses and restriction clauses.
 */
Selectivity
clause_selectivity(Query *root,
				   Node *clause,
				   int varRelid)
{
	Selectivity		s1 = 1.0;	/* default for any unhandled clause type */

	if (clause == NULL)
		return s1;
	if (IsA(clause, Var))
	{
		/*
		 * we have a bool Var.	This is exactly equivalent to the clause:
		 * reln.attribute = 't' so we compute the selectivity as if that
		 * is what we have. The magic #define constants are a hack.  I
		 * didn't want to have to do system cache look ups to find out all
		 * of that info.
		 */
		Index	varno = ((Var *) clause)->varno;

		if (varRelid == 0 || varRelid == varno)
			s1 = restriction_selectivity(F_EQSEL,
										 BooleanEqualOperator,
										 getrelid(varno, root->rtable),
										 ((Var *) clause)->varattno,
										 Int8GetDatum(true),
										 SEL_CONSTANT | SEL_RIGHT);
		/* an outer-relation bool var is taken as always true... */
	}
	else if (IsA(clause, Param))
	{
		/* XXX any way to do better? */
		s1 = 1.0;
	}
	else if (IsA(clause, Const))
	{
		/* bool constant is pretty easy... */
		s1 = ((bool) ((Const *) clause)->constvalue) ? 1.0 : 0.0;
	}
	else if (not_clause(clause))
	{
		/* inverse of the selectivity of the underlying clause */
		s1 = 1.0 - clause_selectivity(root,
									  (Node*) get_notclausearg((Expr*) clause),
									  varRelid);
	}
	else if (and_clause(clause))
	{
		/* share code with clauselist_selectivity() */
		s1 = clauselist_selectivity(root,
									((Expr *) clause)->args,
									varRelid);
	}
	else if (or_clause(clause))
	{
		/*
		 * Selectivities for an 'or' clause are computed as s1+s2 - s1*s2
		 * to account for the probable overlap of selected tuple sets.
		 * XXX is this too conservative?
		 */
		List   *arg;
		s1 = 0.0;
		foreach(arg, ((Expr *) clause)->args)
		{
			Selectivity	s2 = clause_selectivity(root,
												(Node *) lfirst(arg),
												varRelid);
			s1 = s1 + s2 - s1 * s2;
		}
	}
	else if (is_opclause(clause))
	{
		Oid			opno = ((Oper *) ((Expr *) clause)->oper)->opno;
		bool		is_join_clause;

		if (varRelid != 0)
		{
			/*
			 * If we are considering a nestloop join then all clauses
			 * are restriction clauses, since we are only interested in
			 * the one relation.
			 */
			is_join_clause = false;
		}
		else
		{
			/*
			 * Otherwise, it's a join if there's more than one relation used.
			 */
			is_join_clause = (NumRelids(clause) > 1);
		}

		if (is_join_clause)
		{
			/* Estimate selectivity for a join clause. */
			RegProcedure oprjoin = get_oprjoin(opno);

			/*
			 * if the oprjoin procedure is missing for whatever reason, use a
			 * selectivity of 0.5
			 */
			if (!oprjoin)
				s1 = (Selectivity) 0.5;
			else
			{
				int			relid1,
							relid2;
				AttrNumber	attno1,
							attno2;
				Oid			reloid1,
							reloid2;

				get_rels_atts(clause, &relid1, &attno1, &relid2, &attno2);
				reloid1 = relid1 ? getrelid(relid1, root->rtable) : InvalidOid;
				reloid2 = relid2 ? getrelid(relid2, root->rtable) : InvalidOid;
				s1 = join_selectivity(oprjoin, opno,
									  reloid1, attno1,
									  reloid2, attno2);
			}
		}
		else
		{
			/* Estimate selectivity for a restriction clause. */
			RegProcedure oprrest = get_oprrest(opno);

			/*
			 * if the oprrest procedure is missing for whatever reason, use a
			 * selectivity of 0.5
			 */
			if (!oprrest)
				s1 = (Selectivity) 0.5;
			else
			{
				int			relidx;
				AttrNumber	attno;
				Datum		constval;
				int			flag;
				Oid			reloid;

				get_relattval(clause, varRelid,
							  &relidx, &attno, &constval, &flag);
				reloid = relidx ? getrelid(relidx, root->rtable) : InvalidOid;
				s1 = restriction_selectivity(oprrest, opno,
											 reloid, attno,
											 constval, flag);
			}
		}
	}
	else if (is_funcclause(clause))
	{
		/*
		 * This is not an operator, so we guess at the selectivity. THIS
		 * IS A HACK TO GET V4 OUT THE DOOR.  FUNCS SHOULD BE ABLE TO HAVE
		 * SELECTIVITIES THEMSELVES.	   -- JMH 7/9/92
		 */
		s1 = (Selectivity) 0.3333333;
	}
	else if (is_subplan(clause))
	{
		/*
		 * Just for the moment! FIX ME! - vadim 02/04/98
		 */
		s1 = 1.0;
	}

	return s1;
}
