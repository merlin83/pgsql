/*-------------------------------------------------------------------------
 *
 * keys.c--
 *	  Key manipulation routines
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/Attic/keys.c,v 1.16 1999-02-11 17:00:48 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "nodes/pg_list.h"
#include "nodes/nodes.h"
#include "nodes/relation.h"
#include "utils/elog.h"

#include "optimizer/internal.h"
#include "optimizer/keys.h"
#include "optimizer/tlist.h"


static Expr *matching2_tlvar(int var, List *tlist, bool (*test) ());
static bool equal_indexkey_var(int index_key, Var *var);

/*
 * 1. index key
 *		one of:
 *				attnum
 *				(attnum arrayindex)
 * 2. path key
 *		(subkey1 ... subkeyN)
 *		where subkeyI is a var node
 *		note that the 'Keys field is a list of these
 * 3. join key
 *		(outer-subkey inner-subkey)
 *				where each subkey is a var node
 * 4. sort key
 *		one of:
 *				SortKey node
 *				number
 *				nil
 *		(may also refer to the 'SortKey field of a SortKey node,
 *		 which looks exactly like an index key)
 *
 */

/*
 * match-indexkey-operand--
 *	  Returns t iff an index key 'index-key' matches the given clause
 *	  operand.
 *
 */
bool
match_indexkey_operand(int indexkey, Var *operand, RelOptInfo *rel)
{
	if (IsA(operand, Var) &&
		(lfirsti(rel->relids) == operand->varno) &&
		equal_indexkey_var(indexkey, operand))
		return true;
	else
		return false;
}

/*
 * equal_indexkey_var--
 *	  Returns t iff an index key 'index-key' matches the corresponding
 *	  fields of var node 'var'.
 *
 */
static bool
equal_indexkey_var(int index_key, Var *var)
{
	if (index_key == var->varattno)
		return true;
	else
		return false;
}

/*
 * extract-join-subkey--
 *	 Returns the subkey in a join key corresponding to the outer or inner
 *	 relation.
 *
 */
Var *
extract_join_subkey(JoinKey *jk, int which_subkey)
{
	Var		   *retval;

	switch (which_subkey)
	{
		case OUTER:
			retval = jk->outer;
			break;
		case INNER:
			retval = jk->inner;
			break;
		default:				/* do nothing */
			elog(DEBUG, "extract_join_subkey with neither INNER or OUTER");
			retval = NULL;
	}
	return retval;
}

/*
 * pathkeys_match--
 *	  Returns t iff two sets of path keys are equivalent.  They are
 *	  equivalent if the first Var nodes match the second Var nodes.
 *
 *	  XXX		It isn't necessary to check that each sublist exactly contain
 *				the same elements because if the routine that built these
 *				sublists together is correct, having one element in common
 *				implies having all elements in common.
 *		Huh? bjm
 *
 */
bool
pathkeys_match(List *keys1, List *keys2, int *better_key)
{
	List	   *key1,
			   *key2,
			   *key1a,
			   *key2a;

	for (key1 = keys1, key2 = keys2;
		 key1 != NIL && key2 != NIL;
		 key1 = lnext(key1), key2 = lnext(key2))
	{
		for (key1a = lfirst(key1), key2a = lfirst(key2);
			 key1a != NIL && key2a != NIL;
			 key1a = lnext(key1a), key2a = lnext(key2a))
			if (!equal(lfirst(key1a), lfirst(key2a)))
			{
				*better_key = 0;
				return false;
			}
		if (key1a != NIL && key2a == NIL)
		{
			*better_key = 1;
			return true;
		}
		if (key1a == NIL && key2a != NIL)
		{
			*better_key = 2;
			return true;
		}
	}

	/* Now the result should be true if list keys2 has at least as many
	 * entries as keys1, ie, we did not fall off the end of keys2 first.
	 * If key1 is now NIL then we hit the end of keys1 before or at the
	 * same time as the end of keys2.
	 */
	if (key1 != NIL && key2 == NIL)
	{
		*better_key = 1;
		return true;
	}
	if (key1 == NIL && key2 != NIL)
	{
		*better_key = 2;
		return true;
	}
	*better_key = 0;
	return true;
}

/*
 * collect-index-pathkeys--
 *	  Creates a list of subkeys by retrieving var nodes corresponding to
 *	  each index key in 'index-keys' from the relation's target list
 *	  'tlist'.	If the key is not in the target list, the key is irrelevant
 *	  and is thrown away.  The returned subkey list is of the form:
 *				((var1) (var2) ... (varn))
 *
 * 'index-keys' is a list of index keys
 * 'tlist' is a relation target list
 *
 * Returns the list of cons'd subkeys.
 *
 */
/* This function is identical to matching_tlvar and tlistentry_member.
 * They should be merged.
 */
static Expr *
matching2_tlvar(int var, List *tlist, bool (*test) ())
{
	TargetEntry *tlentry = NULL;

	if (var)
	{
		List	   *temp;

		foreach(temp, tlist)
		{
			if ((*test) (var, get_expr(lfirst(temp))))
			{
				tlentry = lfirst(temp);
				break;
			}
		}
	}

	if (tlentry)
		return (Expr *) get_expr(tlentry);
	else
		return (Expr *) NULL;
}


List *
collect_index_pathkeys(int *index_keys, List *tlist)
{
	List	   *retval = NIL;

	Assert(index_keys != NULL);

	while (index_keys[0] != 0)
	{
		Expr	   *mvar;

		mvar = matching2_tlvar(index_keys[0],
							   tlist,
							   equal_indexkey_var);
		if (mvar)
			retval = lappend(retval, lcons(mvar, NIL));
		index_keys++;
	}
	return retval;
}
