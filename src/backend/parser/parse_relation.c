/*-------------------------------------------------------------------------
 *
 * parse_relation.c--
 *	  parser support routines dealing with relations
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/parse_relation.c,v 1.15 1998-09-01 03:24:17 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <ctype.h>
#include <string.h>

#include "postgres.h"
#include "access/heapam.h"
#include "access/htup.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "parser/parse_relation.h"
#include "parser/parse_coerce.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"

static void
checkTargetTypes(ParseState *pstate, char *target_colname,
				 char *refname, char *colname);

struct
{
	char	   *field;
	int			code;
}			special_attr[] =

{
	{
		"ctid", SelfItemPointerAttributeNumber
	},
	{
		"oid", ObjectIdAttributeNumber
	},
	{
		"xmin", MinTransactionIdAttributeNumber
	},
	{
		"cmin", MinCommandIdAttributeNumber
	},
	{
		"xmax", MaxTransactionIdAttributeNumber
	},
	{
		"cmax", MaxCommandIdAttributeNumber
	},
};

#define SPECIALS (sizeof(special_attr)/sizeof(*special_attr))

static char *attnum_type[SPECIALS] = {
	"tid",
	"oid",
	"xid",
	"cid",
	"xid",
	"cid",
};

/* given refname, return a pointer to the range table entry */
RangeTblEntry *
refnameRangeTableEntry(ParseState *pstate, char *refname)
{
	List	   *temp;

	while (pstate != NULL)
	{
		foreach(temp, pstate->p_rtable)
		{
			RangeTblEntry *rte = lfirst(temp);

			if (!strcmp(rte->refname, refname))
				return rte;
		}
		/* only allow correlated columns in WHERE clause */
		if (pstate->p_in_where_clause)
			pstate = pstate->parentParseState;
		else
			break;
	}
	return NULL;
}

/* given refname, return id of variable; position starts with 1 */
int
refnameRangeTablePosn(ParseState *pstate, char *refname, int *sublevels_up)
{
	int			index;
	List	   *temp;


	if (sublevels_up)
		*sublevels_up = 0;

	while (pstate != NULL)
	{
		index = 1;
		foreach(temp, pstate->p_rtable)
		{
			RangeTblEntry *rte = lfirst(temp);

			if (!strcmp(rte->refname, refname))
				return index;
			index++;
		}
		/* only allow correlated columns in WHERE clause */
		if (pstate->p_in_where_clause)
		{
			pstate = pstate->parentParseState;
			if (sublevels_up)
				(*sublevels_up)++;
		}
		else
			break;
	}
	return 0;
}

/*
 * returns range entry if found, else NULL
 */
RangeTblEntry *
colnameRangeTableEntry(ParseState *pstate, char *colname)
{
	List	   *et;
	List	   *rtable;
	RangeTblEntry *rte_result;

	rte_result = NULL;
	while (pstate != NULL)
	{
		if (pstate->p_is_rule)
			rtable = lnext(lnext(pstate->p_rtable));
		else
			rtable = pstate->p_rtable;

		foreach(et, rtable)
		{
			RangeTblEntry *rte = lfirst(et);

			/* only entries on outer(non-function?) scope */
			if (!rte->inFromCl && rte != pstate->p_target_rangetblentry)
				continue;

			if (get_attnum(rte->relid, colname) != InvalidAttrNumber)
			{
				if (rte_result != NULL)
				{
					if (!pstate->p_is_insert ||
						rte != pstate->p_target_rangetblentry)
						elog(ERROR, "Column %s is ambiguous", colname);
				}
				else
					rte_result = rte;
			}
		}
		/* only allow correlated columns in WHERE clause */
		if (pstate->p_in_where_clause && rte_result == NULL)
			pstate = pstate->parentParseState;
		else
			break;
	}
	return rte_result;
}

/*
 * put new entry in pstate p_rtable structure, or return pointer
 * if pstate null
*/
RangeTblEntry *
addRangeTableEntry(ParseState *pstate,
				   char *relname,
				   char *refname,
				   bool inh,
				   bool inFromCl)
{
	Relation	relation;
	RangeTblEntry *rte = makeNode(RangeTblEntry);
	int			sublevels_up;

	if (pstate != NULL)
	{
		if (refnameRangeTablePosn(pstate, refname, &sublevels_up) != 0 &&
			(!inFromCl || sublevels_up == 0)) {
			if (!strcmp(refname, "*CURRENT*") || !strcmp(refname, "*NEW*")) {
				int	rt_index = refnameRangeTablePosn(pstate, refname, &sublevels_up);
				return (RangeTblEntry *)nth(rt_index - 1, pstate->p_rtable);
			}
			elog(ERROR, "Table name %s specified more than once", refname);
		}
	}

	rte->relname = pstrdup(relname);
	rte->refname = pstrdup(refname);

	relation = heap_openr(relname);
	if (relation == NULL)
		elog(ERROR, "%s: %s",
			 relname, aclcheck_error_strings[ACLCHECK_NO_CLASS]);

	rte->relid = RelationGetRelid(relation);

	heap_close(relation);

	/*
	 * Flags - zero or more from inheritance,union,version or recursive
	 * (transitive closure) [we don't support them all -- ay 9/94 ]
	 */
	rte->inh = inh;

	/* RelOID */
	rte->inFromCl = inFromCl;

	/*
	 * close the relation we're done with it for now.
	 */
	if (pstate != NULL)
		pstate->p_rtable = lappend(pstate->p_rtable, rte);

	return rte;
}

/*
 * expandAll -
 *	  makes a list of attributes
 */
List *
expandAll(ParseState *pstate, char *relname, char *refname, int *this_resno)
{
	Relation	rel;
	List	   *te_tail = NIL,
			   *te_head = NIL;
	Var		   *varnode;
	int			varattno,
				maxattrs;
	RangeTblEntry *rte;

	rte = refnameRangeTableEntry(pstate, refname);
	if (rte == NULL)
		rte = addRangeTableEntry(pstate, relname, refname, FALSE, FALSE);

	rel = heap_open(rte->relid);

	if (rel == NULL)
		elog(ERROR, "Unable to expand all -- heap_open failed on %s",
			 rte->refname);

	maxattrs = RelationGetNumberOfAttributes(rel);

	for (varattno = 0; varattno <= maxattrs - 1; varattno++)
	{
		char	   *attrname;
		char	   *resname = NULL;
		TargetEntry *te = makeNode(TargetEntry);

		attrname = pstrdup((rel->rd_att->attrs[varattno]->attname).data);
		varnode = (Var *) make_var(pstate, rte->relid, refname, attrname);

		handleTargetColname(pstate, &resname, refname, attrname);
		if (resname != NULL)
			attrname = resname;

		/*
		 * Even if the elements making up a set are complex, the set
		 * itself is not.
		 */

		te->resdom = makeResdom((AttrNumber) (*this_resno)++,
								varnode->vartype,
								varnode->vartypmod,
								attrname,
								(Index) 0,
								(Oid) 0,
								0);
		te->expr = (Node *) varnode;
		if (te_head == NIL)
			te_head = te_tail = lcons(te, NIL);
		else
			te_tail = lappend(te_tail, te);
	}

	heap_close(rel);

	return te_head;
}

/*
 *	given relation and att name, return id of variable
 *
 *	This should only be used if the relation is already
 *	heap_open()'ed.  Use the cache version get_attnum()
 *	for access to non-opened relations.
 */
int
attnameAttNum(Relation rd, char *a)
{
	int			i;

	for (i = 0; i < rd->rd_rel->relnatts; i++)
		if (!namestrcmp(&(rd->rd_att->attrs[i]->attname), a))
			return i + 1;

	for (i = 0; i < SPECIALS; i++)
		if (!strcmp(special_attr[i].field, a))
			return special_attr[i].code;

	/* on failure */
	elog(ERROR, "Relation %s does not have attribute %s",
		 RelationGetRelationName(rd), a);
	return 0;					/* lint */
}

/*
 * Given range variable, return whether attribute of this name
 * is a set.
 * NOTE the ASSUMPTION here that no system attributes are, or ever
 * will be, sets.
 *
 *	This should only be used if the relation is already
 *	heap_open()'ed.  Use the cache version get_attisset()
 *	for access to non-opened relations.
 */
bool
attnameIsSet(Relation rd, char *name)
{
	int			i;

	/* First check if this is a system attribute */
	for (i = 0; i < SPECIALS; i++)
	{
		if (!strcmp(special_attr[i].field, name))
		{
			return false;		/* no sys attr is a set */
		}
	}
	return get_attisset(RelationGetRelid(rd), name);
}

/*
 *	This should only be used if the relation is already
 *	heap_open()'ed.  Use the cache version
 *	for access to non-opened relations.
 */
int
attnumAttNelems(Relation rd, int attid)
{
	return rd->rd_att->attrs[attid - 1]->attnelems;
}

/* given attribute id, return type of that attribute */
/*
 *	This should only be used if the relation is already
 *	heap_open()'ed.  Use the cache version get_atttype()
 *	for access to non-opened relations.
 */
Oid
attnumTypeId(Relation rd, int attid)
{

	if (attid < 0)
		return typeTypeId(typenameType(attnum_type[-attid - 1]));

	/*
	 * -1 because varattno (where attid comes from) returns one more than
	 * index
	 */
	return rd->rd_att->attrs[attid - 1]->atttypid;
}

/* handleTargetColname()
 * Use column names from insert.
 */
void
handleTargetColname(ParseState *pstate, char **resname,
					char *refname, char *colname)
{
	if (pstate->p_is_insert)
	{
		if (pstate->p_insert_columns != NIL)
		{
			Ident	   *id = lfirst(pstate->p_insert_columns);

			*resname = id->name;
			pstate->p_insert_columns = lnext(pstate->p_insert_columns);
		}
		else
			elog(ERROR, "insert: more expressions than target columns");
	}
	if (pstate->p_is_insert || pstate->p_is_update)
		checkTargetTypes(pstate, *resname, refname, colname);
}

/* checkTargetTypes()
 * Checks value and target column types.
 */
static void
checkTargetTypes(ParseState *pstate, char *target_colname,
				 char *refname, char *colname)
{
	Oid			attrtype_id,
				attrtype_target;
	int			resdomno_id,
				resdomno_target;
	RangeTblEntry *rte;

	if (target_colname == NULL || colname == NULL)
		return;

	if (refname != NULL)
		rte = refnameRangeTableEntry(pstate, refname);
	else
	{
		rte = colnameRangeTableEntry(pstate, colname);
		if (rte == (RangeTblEntry *) NULL)
			elog(ERROR, "attribute %s not found", colname);
		refname = rte->refname;
	}

/*
	if (pstate->p_is_insert && rte == pstate->p_target_rangetblentry)
		elog(ERROR, "%s not available in this context", colname);
*/
	resdomno_id = get_attnum(rte->relid, colname);
	attrtype_id = get_atttype(rte->relid, resdomno_id);

	resdomno_target = attnameAttNum(pstate->p_target_relation, target_colname);
	attrtype_target = attnumTypeId(pstate->p_target_relation, resdomno_target);

#if FALSE
	if ((attrtype_id != attrtype_target)
	 || (get_atttypmod(rte->relid, resdomno_id) !=
		 get_atttypmod(pstate->p_target_relation->rd_id, resdomno_target)))
	{
		if (can_coerce_type(1, &attrtype_id, &attrtype_target))
		{
			Node *expr = coerce_type(pstate, expr, attrtype_id, attrtype_target);

			elog(ERROR, "Type %s(%d) can be coerced to match target column %s(%d)",
				 colname, get_atttypmod(rte->relid, resdomno_id),
				 target_colname, get_atttypmod(pstate->p_target_relation->rd_id, resdomno_target));
		}
		else
		{
			elog(ERROR, "Type or size of %s(%d) does not match target column %s(%d)",
				 colname, get_atttypmod(rte->relid, resdomno_id),
				 target_colname, get_atttypmod(pstate->p_target_relation->rd_id, resdomno_target));
		}
	}
#else
	if (attrtype_id != attrtype_target)
		elog(ERROR, "Type of %s does not match target column %s",
			 colname, target_colname);

	if (attrtype_id == BPCHAROID &&
		get_atttypmod(rte->relid, resdomno_id) !=
		get_atttypmod(pstate->p_target_relation->rd_id, resdomno_target))
		elog(ERROR, "Length of %s is not equal to the length of target column %s",
			 colname, target_colname);
	if (attrtype_id == VARCHAROID &&
		get_atttypmod(rte->relid, resdomno_id) >
		get_atttypmod(pstate->p_target_relation->rd_id, resdomno_target))
		elog(ERROR, "Length of %s is longer than length of target column %s",
			 colname, target_colname);
#endif
}
