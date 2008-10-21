/*-------------------------------------------------------------------------
 *
 * placeholder.c
 *	  PlaceHolderVar and PlaceHolderInfo manipulation routines
 *
 *
 * Portions Copyright (c) 1996-2008, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/optimizer/util/placeholder.c,v 1.1 2008/10/21 20:42:53 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "nodes/nodeFuncs.h"
#include "optimizer/pathnode.h"
#include "optimizer/placeholder.h"
#include "optimizer/planmain.h"
#include "optimizer/var.h"
#include "utils/lsyscache.h"


/*
 * make_placeholder_expr
 *		Make a PlaceHolderVar (and corresponding PlaceHolderInfo)
 *		for the given expression.
 *
 * phrels is the syntactic location (as a set of baserels) to attribute
 * to the expression.
 */
PlaceHolderVar *
make_placeholder_expr(PlannerInfo *root, Expr *expr, Relids phrels)
{
	PlaceHolderVar *phv = makeNode(PlaceHolderVar);
	PlaceHolderInfo *phinfo = makeNode(PlaceHolderInfo);

	phv->phexpr = expr;
	phv->phrels = phrels;
	phv->phid = ++(root->glob->lastPHId);
	phv->phlevelsup = 0;

	phinfo->phid = phv->phid;
	phinfo->ph_var = copyObject(phv);
	phinfo->ph_eval_at = pull_varnos((Node *) phv);
	/* ph_eval_at may change later, see fix_placeholder_eval_levels */
	phinfo->ph_needed = NULL;		/* initially it's unused */
	/* for the moment, estimate width using just the datatype info */
	phinfo->ph_width = get_typavgwidth(exprType((Node *) expr),
									   exprTypmod((Node *) expr));

	root->placeholder_list = lappend(root->placeholder_list, phinfo);

	return phv;
}

/*
 * find_placeholder_info
 *		Fetch the PlaceHolderInfo for the given PHV; error if not found
 */
PlaceHolderInfo *
find_placeholder_info(PlannerInfo *root, PlaceHolderVar *phv)
{
	ListCell   *lc;

	/* if this ever isn't true, we'd need to be able to look in parent lists */
	Assert(phv->phlevelsup == 0);

	foreach(lc, root->placeholder_list)
	{
		PlaceHolderInfo *phinfo = (PlaceHolderInfo *) lfirst(lc);

		if (phinfo->phid == phv->phid)
			return phinfo;
	}
	elog(ERROR, "could not find PlaceHolderInfo with id %u", phv->phid);
	return NULL;				/* keep compiler quiet */
}

/*
 * fix_placeholder_eval_levels
 *		Adjust the target evaluation levels for placeholders
 *
 * The initial eval_at level set by make_placeholder_expr was the set of
 * rels used in the placeholder's expression (or the whole subselect if
 * the expr is variable-free).  If the subselect contains any outer joins
 * that can null any of those rels, we must delay evaluation to above those
 * joins.
 *
 * In future we might want to put additional policy/heuristics here to
 * try to determine an optimal evaluation level.  The current rules will
 * result in evaluation at the lowest possible level.
 */
void
fix_placeholder_eval_levels(PlannerInfo *root)
{
	ListCell   *lc1;

	foreach(lc1, root->placeholder_list)
	{
		PlaceHolderInfo *phinfo = (PlaceHolderInfo *) lfirst(lc1);
		Relids		syn_level = phinfo->ph_var->phrels;
		Relids		eval_at = phinfo->ph_eval_at;
		BMS_Membership eval_membership;
		bool		found_some;
		ListCell   *lc2;

		/*
		 * Ignore unreferenced placeholders.  Note: if a placeholder is
		 * referenced only by some other placeholder's expr, we will do
		 * the right things because the referencing placeholder must appear
		 * earlier in the list.
		 */
		if (bms_is_empty(phinfo->ph_needed))
			continue;

		/*
		 * Check for delays due to lower outer joins.  This is the same logic
		 * as in check_outerjoin_delay in initsplan.c, except that we don't
		 * want to modify the delay_upper_joins flags; that was all handled
		 * already during distribute_qual_to_rels.
		 */
		do
		{
			found_some = false;
			foreach(lc2, root->join_info_list)
			{
				SpecialJoinInfo *sjinfo = (SpecialJoinInfo *) lfirst(lc2);

				/* disregard joins not within the expr's sub-select */
				if (!bms_is_subset(sjinfo->syn_lefthand, syn_level) ||
					!bms_is_subset(sjinfo->syn_righthand, syn_level))
					continue;

				/* do we reference any nullable rels of this OJ? */
				if (bms_overlap(eval_at, sjinfo->min_righthand) ||
					(sjinfo->jointype == JOIN_FULL &&
					 bms_overlap(eval_at, sjinfo->min_lefthand)))
				{
					/* yes; have we included all its rels in eval_at? */
					if (!bms_is_subset(sjinfo->min_lefthand, eval_at) ||
						!bms_is_subset(sjinfo->min_righthand, eval_at))
					{
						/* no, so add them in */
						eval_at = bms_add_members(eval_at,
												  sjinfo->min_lefthand);
						eval_at = bms_add_members(eval_at,
												  sjinfo->min_righthand);
						/* we'll need another iteration */
						found_some = true;
					}
				}
			}
		} while (found_some);

		phinfo->ph_eval_at = eval_at;

		/*
		 * Now that we know where to evaluate the placeholder, make sure that
		 * any vars or placeholders it uses will be available at that join
		 * level.  (Note that this has to be done within this loop to make
		 * sure we don't skip over such placeholders when we get to them.)
		 */
		eval_membership = bms_membership(eval_at);
		if (eval_membership == BMS_MULTIPLE)
		{
			List	   *vars = pull_var_clause((Node *) phinfo->ph_var->phexpr,
											   true);

			add_vars_to_targetlist(root, vars, eval_at);
			list_free(vars);
		}

		/*
		 * Also, if the placeholder can be computed at a base rel and is
		 * needed above it, add it to that rel's targetlist.  (This is
		 * essentially the same logic as in add_placeholders_to_joinrel, but
		 * we can't do that part until joinrels are formed.)
		 */
		if (eval_membership == BMS_SINGLETON)
		{
			int			varno = bms_singleton_member(eval_at);
			RelOptInfo *rel = find_base_rel(root, varno);

			if (bms_nonempty_difference(phinfo->ph_needed, rel->relids))
				rel->reltargetlist = lappend(rel->reltargetlist,
											 copyObject(phinfo->ph_var));
		}
	}
}

/*
 * add_placeholders_to_joinrel
 *		Add any required PlaceHolderVars to a join rel's targetlist.
 *
 * A join rel should emit a PlaceHolderVar if (a) the PHV is needed above
 * this join level and (b) the PHV can be computed at or below this level.
 * At this time we do not need to distinguish whether the PHV will be
 * computed here or copied up from below.
 */
void
add_placeholders_to_joinrel(PlannerInfo *root, RelOptInfo *joinrel)
{
	Relids		relids = joinrel->relids;
	ListCell   *lc;

	foreach(lc, root->placeholder_list)
	{
		PlaceHolderInfo *phinfo = (PlaceHolderInfo *) lfirst(lc);

		/* Is it still needed above this joinrel? */
		if (bms_nonempty_difference(phinfo->ph_needed, relids))
		{
			/* Is it computable here? */
			if (bms_is_subset(phinfo->ph_eval_at, relids))
			{
				/* Yup, add it to the output */
				joinrel->reltargetlist = lappend(joinrel->reltargetlist,
												 phinfo->ph_var);
				joinrel->width += phinfo->ph_width;
			}
		}
	}
}
