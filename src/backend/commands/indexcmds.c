/*-------------------------------------------------------------------------
 *
 * indexcmds.c
 *	  POSTGRES define, extend and remove index code.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/indexcmds.c,v 1.20 2000-01-26 05:56:13 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/pg_index.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/defrem.h"
#include "optimizer/clauses.h"
#include "optimizer/planmain.h"
#include "optimizer/prep.h"
#include "parser/parsetree.h"
#include "utils/builtins.h"
#include "utils/syscache.h"

#define IsFuncIndex(ATTR_LIST) (((IndexElem*)lfirst(ATTR_LIST))->args!=NULL)

/* non-export function prototypes */
static void CheckPredicate(List *predList, List *rangeTable, Oid baseRelOid);
static void CheckPredExpr(Node *predicate, List *rangeTable,
			  Oid baseRelOid);
static void
			CheckPredClause(Expr *predicate, List *rangeTable, Oid baseRelOid);
static void FuncIndexArgs(IndexElem *funcIndex, AttrNumber *attNumP,
			  Oid *argTypes, Oid *opOidP, Oid relId);
static void NormIndexAttrs(List *attList, AttrNumber *attNumP,
			   Oid *opOidP, Oid relId);
static char *GetDefaultOpClass(Oid atttypid);

/*
 * DefineIndex
 *		Creates a new index.
 *
 * 'attributeList' is a list of IndexElem specifying either a functional
 *		index or a list of attributes to index on.
 * 'parameterList' is a list of DefElem specified in the with clause.
 * 'predicate' is the qual specified in the where clause.
 * 'rangetable' is for the predicate
 *
 * Exceptions:
 *		XXX
 */
void
DefineIndex(char *heapRelationName,
			char *indexRelationName,
			char *accessMethodName,
			List *attributeList,
			List *parameterList,
			bool unique,
			bool primary,
			Expr *predicate,
			List *rangetable)
{
	Oid		   *classObjectId;
	Oid			accessMethodId;
	Oid			relationId;
	int			numberOfAttributes;
	AttrNumber *attributeNumberA;
	HeapTuple	tuple;
	uint16		parameterCount = 0;
	Datum	   *parameterA = NULL;
	FuncIndexInfo fInfo;
	List	   *cnfPred = NULL;
	bool		lossy = FALSE;
	List	   *pl;

	/*
	 * Handle attributes
	 */
	numberOfAttributes = length(attributeList);
	if (numberOfAttributes <= 0)
		elog(ERROR, "DefineIndex: must specify at least one attribute");
	if (numberOfAttributes > INDEX_MAX_KEYS)
		elog(ERROR, "Cannot use more than %d attributes in an index",
			 INDEX_MAX_KEYS);

	/*
	 * compute heap relation id
	 */
	if ((relationId = RelnameFindRelid(heapRelationName)) == InvalidOid)
	{
		elog(ERROR, "DefineIndex: %s relation not found",
			 heapRelationName);
	}

	if (unique && strcmp(accessMethodName, "btree") != 0)
		elog(ERROR, "DefineIndex: unique indices are only available with the btree access method");

	if (numberOfAttributes > 1 && strcmp(accessMethodName, "btree") != 0)
		elog(ERROR, "DefineIndex: multi-column indices are only available with the btree access method");

	/*
	 * compute access method id
	 */
	tuple = SearchSysCacheTuple(AMNAME,
								PointerGetDatum(accessMethodName),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		elog(ERROR, "DefineIndex: %s access method not found",
			 accessMethodName);
	}
	accessMethodId = tuple->t_data->t_oid;

	/*
	 * WITH clause reinstated to handle lossy indices. -- JMH, 7/22/96
	 */
	foreach(pl, parameterList)
	{
		DefElem *param = (DefElem *) lfirst(pl);

		if (!strcasecmp(param->defname, "islossy"))
			lossy = TRUE;
		else
			elog(NOTICE, "Unrecognized index attribute '%s' ignored",
				 param->defname);
	}

	/*
	 * Convert the partial-index predicate from parsetree form to plan
	 * form, so it can be readily evaluated during index creation. Note:
	 * "predicate" comes in as a list containing (1) the predicate itself
	 * (a where_clause), and (2) a corresponding range table.
	 *
	 * [(1) is 'predicate' and (2) is 'rangetable' now. - ay 10/94]
	 */
	if (predicate != NULL && rangetable != NIL)
	{
		cnfPred = cnfify((Expr *) copyObject(predicate), true);
		fix_opids((Node *) cnfPred);
		CheckPredicate(cnfPred, rangetable, relationId);
	}

	if (IsFuncIndex(attributeList))
	{
		IndexElem  *funcIndex = lfirst(attributeList);
		int			nargs;

		nargs = length(funcIndex->args);
		if (nargs > INDEX_MAX_KEYS)
			elog(ERROR, "Index function can take at most %d arguments",
				 INDEX_MAX_KEYS);

		FIsetnArgs(&fInfo, nargs);

		strcpy(FIgetname(&fInfo), funcIndex->name);

		attributeNumberA = (AttrNumber *) palloc(nargs * sizeof attributeNumberA[0]);

		classObjectId = (Oid *) palloc(sizeof classObjectId[0]);


		FuncIndexArgs(funcIndex, attributeNumberA,
					  &(FIgetArg(&fInfo, 0)),
					  classObjectId, relationId);

		index_create(heapRelationName,
					 indexRelationName,
					 &fInfo, NULL, accessMethodId,
					 numberOfAttributes, attributeNumberA,
			 classObjectId, parameterCount, parameterA, (Node *) cnfPred,
					 lossy, unique, primary);
	}
	else
	{
		attributeNumberA = (AttrNumber *) palloc(numberOfAttributes *
											 sizeof attributeNumberA[0]);

		classObjectId = (Oid *) palloc(numberOfAttributes * sizeof classObjectId[0]);

		NormIndexAttrs(attributeList, attributeNumberA,
					   classObjectId, relationId);

		index_create(heapRelationName, indexRelationName, NULL,
					 attributeList,
					 accessMethodId, numberOfAttributes, attributeNumberA,
			 classObjectId, parameterCount, parameterA, (Node *) cnfPred,
					 lossy, unique, primary);
	}
}


/*
 * ExtendIndex
 *		Extends a partial index.
 *
 * Exceptions:
 *		XXX
 */
void
ExtendIndex(char *indexRelationName, Expr *predicate, List *rangetable)
{
	Oid		   *classObjectId;
	Oid			accessMethodId;
	Oid			indexId,
				relationId;
	Oid			indproc;
	int			numberOfAttributes;
	AttrNumber *attributeNumberA;
	HeapTuple	tuple;
	FuncIndexInfo fInfo;
	FuncIndexInfo *funcInfo = NULL;
	Form_pg_index index;
	Node	   *oldPred = NULL;
	List	   *cnfPred = NULL;
	PredInfo   *predInfo;
	Relation	heapRelation;
	Relation	indexRelation;
	int			i;

	/*
	 * compute index relation id and access method id
	 */
	tuple = SearchSysCacheTuple(RELNAME,
								PointerGetDatum(indexRelationName),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		elog(ERROR, "ExtendIndex: %s index not found",
			 indexRelationName);
	}
	indexId = tuple->t_data->t_oid;
	accessMethodId = ((Form_pg_class) GETSTRUCT(tuple))->relam;

	/*
	 * find pg_index tuple
	 */
	tuple = SearchSysCacheTuple(INDEXRELID,
								ObjectIdGetDatum(indexId),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
	{
		elog(ERROR, "ExtendIndex: %s is not an index",
			 indexRelationName);
	}

	/*
	 * Extract info from the pg_index tuple
	 */
	index = (Form_pg_index) GETSTRUCT(tuple);
	Assert(index->indexrelid == indexId);
	relationId = index->indrelid;
	indproc = index->indproc;

	for (i = 0; i < INDEX_MAX_KEYS; i++)
	{
		if (index->indkey[i] == InvalidAttrNumber)
			break;
	}
	numberOfAttributes = i;

	if (VARSIZE(&index->indpred) != 0)
	{
		char	   *predString;

		predString = fmgr(F_TEXTOUT, &index->indpred);
		oldPred = stringToNode(predString);
		pfree(predString);
	}
	if (oldPred == NULL)
		elog(ERROR, "ExtendIndex: %s is not a partial index",
			 indexRelationName);

	/*
	 * Convert the extension predicate from parsetree form to plan form,
	 * so it can be readily evaluated during index creation. Note:
	 * "predicate" comes in as a list containing (1) the predicate itself
	 * (a where_clause), and (2) a corresponding range table.
	 */
	if (rangetable != NIL)
	{
		cnfPred = cnfify((Expr *) copyObject(predicate), true);
		fix_opids((Node *) cnfPred);
		CheckPredicate(cnfPred, rangetable, relationId);
	}

	/* make predInfo list to pass to index_build */
	predInfo = (PredInfo *) palloc(sizeof(PredInfo));
	predInfo->pred = (Node *) cnfPred;
	predInfo->oldPred = oldPred;

	attributeNumberA = (AttrNumber *) palloc(numberOfAttributes *
											 sizeof attributeNumberA[0]);
	classObjectId = (Oid *) palloc(numberOfAttributes * sizeof classObjectId[0]);


	for (i = 0; i < numberOfAttributes; i++)
	{
		attributeNumberA[i] = index->indkey[i];
		classObjectId[i] = index->indclass[i];
	}

	if (indproc != InvalidOid)
	{
		funcInfo = &fInfo;
/*		FIgetnArgs(funcInfo) = numberOfAttributes; */
		FIsetnArgs(funcInfo, numberOfAttributes);

		tuple = SearchSysCacheTuple(PROCOID,
									ObjectIdGetDatum(indproc),
									0, 0, 0);
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "ExtendIndex: index procedure not found");

		namecpy(&(funcInfo->funcName),
				&(((Form_pg_proc) GETSTRUCT(tuple))->proname));

		FIsetProcOid(funcInfo, tuple->t_data->t_oid);
	}

	heapRelation = heap_open(relationId, ShareLock);
	indexRelation = index_open(indexId);

	InitIndexStrategy(numberOfAttributes, indexRelation, accessMethodId);

	index_build(heapRelation, indexRelation, numberOfAttributes,
				attributeNumberA, 0, NULL, funcInfo, predInfo);

	/* heap and index rels are closed as a side-effect of index_build */
}


/*
 * CheckPredicate
 *		Checks that the given list of partial-index predicates refer
 *		(via the given range table) only to the given base relation oid,
 *		and that they're in a form the planner can handle, i.e.,
 *		boolean combinations of "ATTR OP CONST" (yes, for now, the ATTR
 *		has to be on the left).
 */

static void
CheckPredicate(List *predList, List *rangeTable, Oid baseRelOid)
{
	List	   *item;

	foreach(item, predList)
		CheckPredExpr(lfirst(item), rangeTable, baseRelOid);
}

static void
CheckPredExpr(Node *predicate, List *rangeTable, Oid baseRelOid)
{
	List	   *clauses = NIL,
			   *clause;

	if (is_opclause(predicate))
	{
		CheckPredClause((Expr *) predicate, rangeTable, baseRelOid);
		return;
	}
	else if (or_clause(predicate) || and_clause(predicate))
		clauses = ((Expr *) predicate)->args;
	else
		elog(ERROR, "Unsupported partial-index predicate expression type");

	foreach(clause, clauses)
		CheckPredExpr(lfirst(clause), rangeTable, baseRelOid);
}

static void
CheckPredClause(Expr *predicate, List *rangeTable, Oid baseRelOid)
{
	Var		   *pred_var;
	Const	   *pred_const;

	pred_var = (Var *) get_leftop(predicate);
	pred_const = (Const *) get_rightop(predicate);

	if (!IsA(predicate->oper, Oper) ||
		!IsA(pred_var, Var) ||
		!IsA(pred_const, Const))
		elog(ERROR, "Unsupported partial-index predicate clause type");

	if (getrelid(pred_var->varno, rangeTable) != baseRelOid)
		elog(ERROR,
		 "Partial-index predicates may refer only to the base relation");
}


static void
FuncIndexArgs(IndexElem *funcIndex,
			  AttrNumber *attNumP,
			  Oid *argTypes,
			  Oid *opOidP,
			  Oid relId)
{
	List	   *rest;
	HeapTuple	tuple;
	Form_pg_attribute att;

	tuple = SearchSysCacheTuple(CLANAME,
								PointerGetDatum(funcIndex->class),
								0, 0, 0);

	if (!HeapTupleIsValid(tuple))
	{
		elog(ERROR, "DefineIndex: %s class not found",
			 funcIndex->class);
	}
	*opOidP = tuple->t_data->t_oid;

	MemSet(argTypes, 0, FUNC_MAX_ARGS * sizeof(Oid));

	/*
	 * process the function arguments
	 */
	for (rest = funcIndex->args; rest != NIL; rest = lnext(rest))
	{
		char	   *arg;

		arg = strVal(lfirst(rest));

		tuple = SearchSysCacheTuple(ATTNAME,
									ObjectIdGetDatum(relId),
									PointerGetDatum(arg), 0, 0);

		if (!HeapTupleIsValid(tuple))
		{
			elog(ERROR,
				 "DefineIndex: attribute \"%s\" not found",
				 arg);
		}
		att = (Form_pg_attribute) GETSTRUCT(tuple);
		*attNumP++ = att->attnum;
		*argTypes++ = att->atttypid;
	}
}

static void
NormIndexAttrs(List *attList,	/* list of IndexElem's */
			   AttrNumber *attNumP,
			   Oid *classOidP,
			   Oid relId)
{
	List	   *rest;
	HeapTuple	atttuple,
				tuple;

	/*
	 * process attributeList
	 */

	for (rest = attList; rest != NIL; rest = lnext(rest))
	{
		IndexElem  *attribute;
		Form_pg_attribute attform;

		attribute = lfirst(rest);

		if (attribute->name == NULL)
			elog(ERROR, "missing attribute for define index");

		atttuple = SearchSysCacheTupleCopy(ATTNAME,
										   ObjectIdGetDatum(relId),
										PointerGetDatum(attribute->name),
										   0, 0);
		if (!HeapTupleIsValid(atttuple))
		{
			elog(ERROR,
				 "DefineIndex: attribute \"%s\" not found",
				 attribute->name);
		}

		attform = (Form_pg_attribute) GETSTRUCT(atttuple);
		*attNumP++ = attform->attnum;

		/* we want the type so we can set the proper alignment, etc. */
		if (attribute->typename == NULL)
		{
			tuple = SearchSysCacheTuple(TYPEOID,
									 ObjectIdGetDatum(attform->atttypid),
										0, 0, 0);
			if (!HeapTupleIsValid(tuple))
				elog(ERROR, "create index: type for attribute '%s' undefined",
					 attribute->name);
			/* we just set the type name because that is all we need */
			attribute->typename = makeNode(TypeName);
			attribute->typename->name = nameout(&((Form_pg_type) GETSTRUCT(tuple))->typname);

			/* we all need the typmod for the char and varchar types. */
			attribute->typename->typmod = attform->atttypmod;
		}

		if (attribute->class == NULL)
		{
			/* no operator class specified, so find the default */
			attribute->class = GetDefaultOpClass(attform->atttypid);
			if (attribute->class == NULL)
			{
				elog(ERROR,
					 "Can't find a default operator class for type %u.",
					 attform->atttypid);
			}
		}

		tuple = SearchSysCacheTuple(CLANAME,
									PointerGetDatum(attribute->class),
									0, 0, 0);

		if (!HeapTupleIsValid(tuple))
		{
			elog(ERROR, "DefineIndex: %s class not found",
				 attribute->class);
		}
		*classOidP++ = tuple->t_data->t_oid;
		heap_freetuple(atttuple);
	}
}

static char *
GetDefaultOpClass(Oid atttypid)
{
	HeapTuple	tuple;

	tuple = SearchSysCacheTuple(CLADEFTYPE,
								ObjectIdGetDatum(atttypid),
								0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		return 0;

	return nameout(&((Form_pg_opclass) GETSTRUCT(tuple))->opcname);
}

/*
 * RemoveIndex
 *		Deletes an index.
 *
 * Exceptions:
 *		BadArg if name is invalid.
 *		"WARN" if index nonexistent.
 *		...
 */
void
RemoveIndex(char *name)
{
	HeapTuple	tuple;

	tuple = SearchSysCacheTuple(RELNAME,
								PointerGetDatum(name),
								0, 0, 0);

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "index \"%s\" nonexistent", name);

	if (((Form_pg_class) GETSTRUCT(tuple))->relkind != RELKIND_INDEX)
	{
		elog(ERROR, "relation \"%s\" is of type \"%c\"",
			 name,
			 ((Form_pg_class) GETSTRUCT(tuple))->relkind);
	}

	index_drop(tuple->t_data->t_oid);
}
