/*-------------------------------------------------------------------------
 *
 * preptlist.c
 *	  Routines to preprocess the parse tree target list
 *
 * This module takes care of altering the query targetlist as needed for
 * INSERT, UPDATE, and DELETE queries.	For INSERT and UPDATE queries,
 * the targetlist must contain an entry for each attribute of the target
 * relation in the correct order.  For both UPDATE and DELETE queries,
 * we need a junk targetlist entry holding the CTID attribute --- the
 * executor relies on this to find the tuple to be replaced/deleted.
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/prep/preptlist.c,v 1.53 2002-06-20 20:29:31 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "optimizer/prep.h"
#include "parser/parsetree.h"


static List *expand_targetlist(List *tlist, int command_type,
				  Index result_relation, List *range_table);


/*
 * preprocess_targetlist
 *	  Driver for preprocessing the parse tree targetlist.
 *
 *	  Returns the new targetlist.
 */
List *
preprocess_targetlist(List *tlist,
					  int command_type,
					  Index result_relation,
					  List *range_table)
{
	/*
	 * Sanity check: if there is a result relation, it'd better be a real
	 * relation not a subquery.  Else parser or rewriter messed up.
	 */
	if (result_relation)
	{
		RangeTblEntry *rte = rt_fetch(result_relation, range_table);

		if (rte->subquery != NULL || rte->relid == InvalidOid)
			elog(ERROR, "preprocess_targetlist: subquery cannot be result relation");
	}

	/*
	 * for heap_formtuple to work, the targetlist must match the exact
	 * order of the attributes. We also need to fill in any missing
	 * attributes.							-ay 10/94
	 */
	if (command_type == CMD_INSERT || command_type == CMD_UPDATE)
		tlist = expand_targetlist(tlist, command_type,
								  result_relation, range_table);

	/*
	 * for "update" and "delete" queries, add ctid of the result relation
	 * into the target list so that the ctid will propagate through
	 * execution and ExecutePlan() will be able to identify the right
	 * tuple to replace or delete.	This extra field is marked "junk" so
	 * that it is not stored back into the tuple.
	 */
	if (command_type == CMD_UPDATE || command_type == CMD_DELETE)
	{
		Resdom	   *resdom;
		Var		   *var;

		resdom = makeResdom(length(tlist) + 1,
							TIDOID,
							-1,
							pstrdup("ctid"),
							true);

		var = makeVar(result_relation, SelfItemPointerAttributeNumber,
					  TIDOID, -1, 0);

		/*
		 * For an UPDATE, expand_targetlist already created a fresh tlist.
		 * For DELETE, better do a listCopy so that we don't destructively
		 * modify the original tlist (is this really necessary?).
		 */
		if (command_type == CMD_DELETE)
			tlist = listCopy(tlist);

		tlist = lappend(tlist, makeTargetEntry(resdom, (Node *) var));
	}

	return tlist;
}

/*****************************************************************************
 *
 *		TARGETLIST EXPANSION
 *
 *****************************************************************************/

/*
 * expand_targetlist
 *	  Given a target list as generated by the parser and a result relation,
 *	  add targetlist entries for any missing attributes, and ensure the
 *	  non-junk attributes appear in proper field order.
 *
 * NOTE: if you are tempted to put more processing here, consider whether
 * it shouldn't go in the rewriter's rewriteTargetList() instead.
 */
static List *
expand_targetlist(List *tlist, int command_type,
				  Index result_relation, List *range_table)
{
	List	   *new_tlist = NIL;
	Relation	rel;
	int			attrno,
				numattrs;

	/*
	 * The rewriter should have already ensured that the TLEs are in
	 * correct order; but we have to insert TLEs for any missing attributes.
	 *
	 * Scan the tuple description in the relation's relcache entry to make
	 * sure we have all the user attributes in the right order.
	 */
	rel = heap_open(getrelid(result_relation, range_table), AccessShareLock);

	numattrs = RelationGetNumberOfAttributes(rel);

	for (attrno = 1; attrno <= numattrs; attrno++)
	{
		Form_pg_attribute att_tup = rel->rd_att->attrs[attrno - 1];
		TargetEntry *new_tle = NULL;

		if (tlist != NIL)
		{
			TargetEntry *old_tle = (TargetEntry *) lfirst(tlist);
			Resdom	   *resdom = old_tle->resdom;

			if (!resdom->resjunk && resdom->resno == attrno)
			{
				Assert(strcmp(resdom->resname,
							  NameStr(att_tup->attname)) == 0);
				new_tle = old_tle;
				tlist = lnext(tlist);
			}
		}

		if (new_tle == NULL)
		{
			/*
			 * Didn't find a matching tlist entry, so make one.
			 *
			 * For INSERT, generate a NULL constant.  (We assume the
			 * rewriter would have inserted any available default value.)
			 *
			 * For UPDATE, generate a Var reference to the existing value of
			 * the attribute, so that it gets copied to the new tuple.
			 */
			Oid			atttype = att_tup->atttypid;
			int32		atttypmod = att_tup->atttypmod;
			Node	   *new_expr;

			switch (command_type)
			{
				case CMD_INSERT:
					new_expr = (Node *) makeConst(atttype,
												  att_tup->attlen,
												  (Datum) 0,
												  true,		/* isnull */
												  att_tup->attbyval,
												  false,	/* not a set */
												  false);
					break;
				case CMD_UPDATE:
					new_expr = (Node *) makeVar(result_relation,
												attrno,
												atttype,
												atttypmod,
												0);
					break;
				default:
					elog(ERROR, "expand_targetlist: unexpected command_type");
					new_expr = NULL;	/* keep compiler quiet */
					break;
			}

			new_tle = makeTargetEntry(makeResdom(attrno,
												 atttype,
												 atttypmod,
												 pstrdup(NameStr(att_tup->attname)),
												 false),
									  new_expr);
		}

		new_tlist = lappend(new_tlist, new_tle);
	}

	/*
	 * The remaining tlist entries should be resjunk; append them all to
	 * the end of the new tlist, making sure they have resnos higher than
	 * the last real attribute.  (Note: although the rewriter already did
	 * such renumbering, we have to do it again here in case we are doing
	 * an UPDATE in an inheritance child table with more columns.)
	 */
	while (tlist)
	{
		TargetEntry *old_tle = (TargetEntry *) lfirst(tlist);
		Resdom	   *resdom = old_tle->resdom;

		if (!resdom->resjunk)
			elog(ERROR, "expand_targetlist: targetlist is not sorted correctly");
		/* Get the resno right, but don't copy unnecessarily */
		if (resdom->resno != attrno)
		{
			resdom = (Resdom *) copyObject((Node *) resdom);
			resdom->resno = attrno;
			old_tle = makeTargetEntry(resdom, old_tle->expr);
		}
		new_tlist = lappend(new_tlist, old_tle);
		attrno++;
		tlist = lnext(tlist);
	}

	heap_close(rel, AccessShareLock);

	return new_tlist;
}
