/*-------------------------------------------------------------------------
 *
 * ordering.c
 *	  Routines to manipulate and compare merge and path orderings
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/optimizer/util/Attic/ordering.c,v 1.16 1999-05-25 16:09:57 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>

#include "postgres.h"

#include "optimizer/internal.h"
#include "optimizer/ordering.h"

static bool sortops_order_match(Oid *ordering1, Oid *ordering2,
					int *better_sort);

/*
 * equal_path_ordering
 *	  Returns t iff two path orderings are equal.
 *
 */
bool
pathorder_match(PathOrder *path_ordering1,
				PathOrder *path_ordering2,
				int *better_sort)
{

	*better_sort = 0;

	if (path_ordering1 == path_ordering2)
		return true;

	if (!path_ordering2)
	{
		*better_sort = 1;
		return true;
	}

	if (!path_ordering1)
	{
		*better_sort = 2;
		return true;
	}

	if (path_ordering1->ordtype == MERGE_ORDER &&
		path_ordering2->ordtype == MERGE_ORDER)
		return equal(path_ordering1->ord.merge, path_ordering2->ord.merge);
	else if (path_ordering1->ordtype == SORTOP_ORDER &&
			 path_ordering2->ordtype == SORTOP_ORDER)
	{
		return sortops_order_match(path_ordering1->ord.sortop,
								   path_ordering2->ord.sortop,
								   better_sort);
	}
	else if (path_ordering1->ordtype == MERGE_ORDER &&
			 path_ordering2->ordtype == SORTOP_ORDER)
	{
		if (!path_ordering2->ord.sortop)
		{
			*better_sort = 1;
			return true;
		}
		return path_ordering1->ord.merge->left_operator == path_ordering2->ord.sortop[0];
	}
	else
	{
		if (!path_ordering1->ord.sortop)
		{
			*better_sort = 2;
			return true;
		}
		return path_ordering1->ord.sortop[0] == path_ordering2->ord.merge->left_operator;
	}
}

/*
 * equal_path_merge_ordering
 *	  Returns t iff a path ordering is usable for ordering a merge join.
 *
 * XXX	Presently, this means that the first sortop of the path matches
 *		either of the merge sortops.  Is there a "right" and "wrong"
 *		sortop to match?
 *
 */
bool
equal_path_merge_ordering(Oid *path_ordering,
						  MergeOrder *merge_ordering)
{
	if (path_ordering == NULL || merge_ordering == NULL)
		return false;

	if (path_ordering[0] == merge_ordering->left_operator ||
		path_ordering[0] == merge_ordering->right_operator)
		return true;
	else
		return false;
}

/*
 * equal_merge_ordering
 *	  Returns t iff two merge orderings are equal.
 *
 */
bool
equal_merge_ordering(MergeOrder *merge_ordering1,
					 MergeOrder *merge_ordering2)
{
	return equal(merge_ordering1, merge_ordering2);
}


/*
 *	sortops
 *
 */

/*
 * equal_sort_ops_order -
 *	  Returns true iff the sort operators are in the same order.
 */
static bool
sortops_order_match(Oid *ordering1, Oid *ordering2, int *better_sort)
{
	int			i = 0;

	*better_sort = 0;

	if (ordering1 == ordering2)
		return true;

	if (!ordering2)
	{
		*better_sort = 1;
		return true;
	}

	if (!ordering1)
	{
		*better_sort = 2;
		return true;
	}

	while (ordering1[i] != 0 && ordering2[i] != 0)
	{
		if (ordering1[i] != ordering2[i])
			break;
		i++;
	}

	if (ordering1[i] != 0 && ordering2[i] == 0)
	{
		*better_sort = 1;
		return true;
	}

	if (ordering1[i] == 0 && ordering2[i] != 0)
	{
		*better_sort = 2;
		return true;
	}

	return ordering1[i] == 0 && ordering2[i] == 0;
}
