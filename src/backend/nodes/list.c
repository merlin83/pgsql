/*-------------------------------------------------------------------------
 *
 * list.c
 *	  various list handling routines
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/nodes/list.c,v 1.25 1999-07-17 20:17:06 momjian Exp $
 *
 * NOTES
 *	  XXX a few of the following functions are duplicated to handle
 *		  List of pointers and List of integers separately. Some day,
 *		  someone should unify them.			- ay 11/2/94
 *	  This file needs cleanup.
 *
 * HISTORY
 *	  AUTHOR			DATE			MAJOR EVENT
 *	  Andrew Yu			Oct, 1994		file creation
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "nodes/parsenodes.h"

/*
 *	makeList
 *
 *	Take varargs, terminated by -1, and make a List
 */
List *
makeList(void *elem,...)
{
	va_list		args;
	List	   *retval = NIL;
	List	   *temp = NIL;
	List	   *tempcons = NIL;

	va_start(args, elem);

	temp = elem;
	while (temp != (void *) -1)
	{
		temp = lcons(temp, NIL);
		if (tempcons == NIL)
			retval = temp;
		else
			lnext(tempcons) = temp;
		tempcons = temp;

		temp = va_arg(args, void *);
	}

	va_end(args);

	return retval;
}

/*
 *	lcons
 *
 *	Add obj to the front of list, or make a new list if 'list' is NIL
 */
List *
lcons(void *obj, List *list)
{
	List	   *l = makeNode(List);

	lfirst(l) = obj;
	lnext(l) = list;
	return l;
}

/*
 *	lconsi
 *
 *	Same as lcons, but for integer data
 */
List *
lconsi(int datum, List *list)
{
	List	   *l = makeNode(List);

	lfirsti(l) = datum;
	lnext(l) = list;
	return l;
}

/*
 *	lappend
 *
 *	Add obj to the end of list, or make a new list if 'list' is NIL
 *
 * MORE EXPENSIVE THAN lcons
 */
List *
lappend(List *list, void *obj)
{
	return nconc(list, lcons(obj, NIL));
}

/*
 *	lappendi
 *
 *	Same as lappend, but for integers
 */
List *
lappendi(List *list, int datum)
{
	return nconc(list, lconsi(datum, NIL));
}

/*
 *	nconc
 *
 *	Concat l2 on to the end of l1
 */
List *
nconc(List *l1, List *l2)
{
	List	   *temp;

	if (l1 == NIL)
		return l2;
	if (l2 == NIL)
		return l1;
	if (l1 == l2)
		elog(ERROR, "tryout to nconc a list to itself");

	for (temp = l1; lnext(temp) != NULL; temp = lnext(temp))
		;

	lnext(temp) = l2;
	return l1;					/* list1 is now list1+list2  */
}


#ifdef NOT_USED
List *
nreverse(List *list)
{
	List	   *rlist = NIL;
	List	   *p = NIL;

	if (list == NULL)
		return NIL;

	if (length(list) == 1)
		return list;

	for (p = list; p != NULL; p = lnext(p))
		rlist = lcons(lfirst(p), rlist);

	lfirst(list) = lfirst(rlist);
	lnext(list) = lnext(rlist);
	return list;
}

#endif

/*
 *	makeInteger
 */
Value *
makeInteger(long i)
{
	Value	   *v = makeNode(Value);

	v->type = T_Integer;
	v->val.ival = i;
	return v;
}

/*
 *	makeFloat
 */
Value *
makeFloat(double d)
{
	Value	   *v = makeNode(Value);

	v->type = T_Float;
	v->val.dval = d;
	return v;
}

/*
 *	makeString
 */
Value *
makeString(char *str)
{
	Value	   *v = makeNode(Value);

	v->type = T_String;
	v->val.str = str;
	return v;
}

/*
 *	nth
 *
 *	Get the n'th element of the list.  First element is 0th.
 */
void *
nth(int n, List *l)
{
	/* XXX assume list is long enough */
	while (n > 0)
	{
		l = lnext(l);
		n--;
	}
	return lfirst(l);
}

/*
 *	nthi
 *
 *	Same as nthi, but for integers
 */
int
nthi(int n, List *l)
{
	/* XXX assume list is long enough */
	while (n > 0)
	{
		l = lnext(l);
		n--;
	}
	return lfirsti(l);
}

/* this is here solely for rt_store. Get rid of me some day! */
void
set_nth(List *l, int n, void *elem)
{
	/* XXX assume list is long enough */
	while (n > 0)
	{
		l = lnext(l);
		n--;
	}
	lfirst(l) = elem;
	return;
}

/*
 *	length
 *
 *	Get the length of l
 */
int
length(List *l)
{
	int			i = 0;

	while (l != NIL)
	{
		l = lnext(l);
		i++;
	}
	return i;
}

/*
 *	freeList
 *
 *	Free the List nodes of a list
 */
void
freeList(List *list)
{
	while (list != NIL)
	{
		List	   *l = list;

		list = lnext(list);
		pfree(l);
	}
}

#ifdef NOT_USED
/*
 * below are for backwards compatibility
 */
List *
append(List *l1, List *l2)
{
	List	   *newlist,
			   *newlist2,
			   *p;

	if (l1 == NIL)
		return copyObject(l2);

	newlist = copyObject(l1);
	newlist2 = copyObject(l2);

	for (p = newlist; lnext(p) != NIL; p = lnext(p))
		;
	lnext(p) = newlist2;
	return newlist;
}

#endif

#ifdef NOT_USED
/*
 * below are for backwards compatibility
 */
List *
intAppend(List *l1, List *l2)
{
	List	   *newlist,
			   *newlist2,
			   *p;

	if (l1 == NIL)
		return listCopy(l2);

	newlist = listCopy(l1);
	newlist2 = listCopy(l2);

	for (p = newlist; lnext(p) != NIL; p = lnext(p))
		;
	lnext(p) = newlist2;
	return newlist;
}

#endif

/*
 *		same
 *
 *		Returns t if two lists contain the same elements.
 *		 now defined in lispdep.c
 *
 * XXX only good for IntList	-ay
 */
bool
same(List *l1, List *l2)
{
	List	   *temp = NIL;

	if (l1 == NULL)
		return l2 == NULL;
	if (l2 == NULL)
		return l1 == NULL;
	if (length(l1) == length(l2))
	{
		foreach(temp, l1)
		{
			if (!intMember(lfirsti(temp), l2))
				return false;
		}
		return true;
	}
	return false;

}

List *
LispUnion(List *l1, List *l2)
{
	List	   *retval = NIL;
	List	   *i = NIL;
	List	   *j = NIL;

	if (l1 == NIL)
		return l2;				/* XXX - should be copy of l2 */

	if (l2 == NIL)
		return l1;				/* XXX - should be copy of l1 */

	foreach(i, l1)
	{
		foreach(j, l2)
		{
			if (!equal(lfirst(i), lfirst(j)))
			{
				retval = lappend(retval, lfirst(i));
				break;
			}
		}
	}
	foreach(i, l2)
		retval = lappend(retval, lfirst(i));

	return retval;
}

List *
LispUnioni(List *l1, List *l2)
{
	List	   *retval = NIL;
	List	   *i = NIL;
	List	   *j = NIL;

	if (l1 == NIL)
		return l2;				/* XXX - should be copy of l2 */

	if (l2 == NIL)
		return l1;				/* XXX - should be copy of l1 */

	foreach(i, l1)
	{
		foreach(j, l2)
		{
			if (lfirsti(i) != lfirsti(j))
			{
				retval = lappendi(retval, lfirsti(i));
				break;
			}
		}
	}
	foreach(i, l2)
		retval = lappendi(retval, lfirsti(i));

	return retval;
}

/*
 * member()
 * - nondestructive, returns t iff l1 is a member of the list
 *	 l2
 */
bool
member(void *l1, List *l2)
{
	List	   *i;

	foreach(i, l2)
		if (equal((Node *) l1, (Node *) lfirst(i)))
		return true;
	return false;
}

bool
intMember(int l1, List *l2)
{
	List	   *i;

	foreach(i, l2)
		if (l1 == lfirsti(i))
		return true;
	return false;
}

/*
 * lremove -
 *	  only does pointer comparisons. Removes 'elem' from the the linked list.
 */
List *
lremove(void *elem, List *list)
{
	List	   *l;
	List	   *prev = NIL;
	List	   *result = list;

	foreach(l, list)
	{
		if (elem == lfirst(l))
			break;
		prev = l;
	}
	if (l != NULL)
	{
		if (prev == NIL)
			result = lnext(list);
		else
			lnext(prev) = lnext(l);
	}
	return result;
}

/*
 *	LispRemove
 */
List *
LispRemove(void *elem, List *list)
{
	List	   *temp = NIL;
	List	   *prev = NIL;

	if (equal(elem, lfirst(list)))
		return lnext(list);

	temp = lnext(list);
	prev = list;
	while (temp != NIL)
	{
		if (equal(elem, lfirst(temp)))
		{
			lnext(prev) = lnext(temp);
			break;
		}
		temp = lnext(temp);
		prev = lnext(prev);
	}
	return list;
}

#ifdef NOT_USED
List *
intLispRemove(int elem, List *list)
{
	List	   *temp = NIL;
	List	   *prev = NIL;

	if (elem == lfirsti(list))
		return lnext(list);

	temp = lnext(list);
	prev = list;
	while (temp != NIL)
	{
		if (elem == lfirsti(temp))
		{
			lnext(prev) = lnext(temp);
			break;
		}
		temp = lnext(temp);
		prev = lnext(prev);
	}
	return list;
}

#endif

/*
 *	set_difference
 *
 *	Return l1 without the elements in l2.
 */
List *
set_difference(List *l1, List *l2)
{
	List	   *temp1 = NIL;
	List	   *result = NIL;

	if (l2 == NIL)
		return l1;

	foreach(temp1, l1)
	{
		if (!member(lfirst(temp1), l2))
			result = lappend(result, lfirst(temp1));
	}
	return result;
}

/*
 *	set_differencei
 *
 *	Same as set_difference, but for integers
 */
List *
set_differencei(List *l1, List *l2)
{
	List	   *temp1 = NIL;
	List	   *result = NIL;

	if (l2 == NIL)
		return l1;

	foreach(temp1, l1)
	{
		if (!intMember(lfirsti(temp1), l2))
			result = lappendi(result, lfirsti(temp1));
	}
	return result;
}
