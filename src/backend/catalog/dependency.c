/*-------------------------------------------------------------------------
 *
 * dependency.c
 *	  Routines to support inter-object dependencies.
 *
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/dependency.c,v 1.6 2002-07-25 10:07:10 ishii Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/dependency.h"
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/indexing.h"
#include "catalog/pg_attrdef.h"
#include "catalog/pg_constraint.h"
#include "catalog/pg_conversion.h"
#include "catalog/pg_depend.h"
#include "catalog/pg_language.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_rewrite.h"
#include "catalog/pg_trigger.h"
#include "catalog/pg_type.h"
#include "commands/comment.h"
#include "commands/defrem.h"
#include "commands/proclang.h"
#include "commands/schemacmds.h"
#include "commands/trigger.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
#include "optimizer/clauses.h"
#include "parser/parsetree.h"
#include "rewrite/rewriteRemove.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


/* This enum covers all system catalogs whose OIDs can appear in classid. */
typedef enum ObjectClasses
{
	OCLASS_CAST,				/* pg_cast */
	OCLASS_CLASS,				/* pg_class */
	OCLASS_PROC,				/* pg_proc */
	OCLASS_TYPE,				/* pg_type */
	OCLASS_CONSTRAINT,			/* pg_constraint */
	OCLASS_CONVERSION,			/* pg_conversion */
	OCLASS_DEFAULT,				/* pg_attrdef */
	OCLASS_LANGUAGE,			/* pg_language */
	OCLASS_OPERATOR,			/* pg_operator */
	OCLASS_REWRITE,				/* pg_rewrite */
	OCLASS_TRIGGER,				/* pg_trigger */
	OCLASS_SCHEMA,				/* pg_namespace */
	MAX_OCLASS					/* MUST BE LAST */
} ObjectClasses;

/* expansible list of ObjectAddresses */
typedef struct ObjectAddresses
{
	ObjectAddress  *refs;		/* => palloc'd array */
	int				numrefs;	/* current number of references */
	int				maxrefs;	/* current size of palloc'd array */
	struct ObjectAddresses *link; /* list link for use in recursion */
} ObjectAddresses;

/* for find_expr_references_walker */
typedef struct
{
	ObjectAddresses	addrs;		/* addresses being accumulated */
	List		   *rtables;	/* list of rangetables to resolve Vars */
} find_expr_references_context;


/*
 * Because not all system catalogs have predetermined OIDs, we build a table
 * mapping between ObjectClasses and OIDs.  This is done at most once per
 * backend run, to minimize lookup overhead.
 */
static bool	object_classes_initialized = false;
static Oid	object_classes[MAX_OCLASS];


static bool recursiveDeletion(const ObjectAddress *object,
							  DropBehavior behavior,
							  const ObjectAddress *callingObject,
							  ObjectAddresses *pending,
							  Relation depRel);
static void doDeletion(const ObjectAddress *object);
static bool find_expr_references_walker(Node *node,
										find_expr_references_context *context);
static void eliminate_duplicate_dependencies(ObjectAddresses *addrs);
static int	object_address_comparator(const void *a, const void *b);
static void init_object_addresses(ObjectAddresses *addrs);
static void add_object_address(ObjectClasses oclass, Oid objectId, int32 subId,
							   ObjectAddresses *addrs);
static void add_exact_object_address(const ObjectAddress *object,
									 ObjectAddresses *addrs);
static void del_object_address(const ObjectAddress *object,
							   ObjectAddresses *addrs);
static void del_object_address_by_index(int index, ObjectAddresses *addrs);
static void term_object_addresses(ObjectAddresses *addrs);
static void init_object_classes(void);
static ObjectClasses getObjectClass(const ObjectAddress *object);
static char *getObjectDescription(const ObjectAddress *object);
static void getRelationDescription(StringInfo buffer, Oid relid);


/*
 * performDeletion: attempt to drop the specified object.  If CASCADE
 * behavior is specified, also drop any dependent objects (recursively).
 * If RESTRICT behavior is specified, error out if there are any dependent
 * objects, except for those that should be implicitly dropped anyway
 * according to the dependency type.
 *
 * This is the outer control routine for all forms of DROP that drop objects
 * that can participate in dependencies.
 */
void
performDeletion(const ObjectAddress *object,
				DropBehavior behavior)
{
	char		   *objDescription;
	Relation		depRel;

	/*
	 * Get object description for possible use in failure message.
	 * Must do this before deleting it ...
	 */
	objDescription = getObjectDescription(object);

	/*
	 * We save some cycles by opening pg_depend just once and passing the
	 * Relation pointer down to all the recursive deletion steps.
	 */
	depRel = heap_openr(DependRelationName, RowExclusiveLock);

	if (!recursiveDeletion(object, behavior, NULL, NULL, depRel))
		elog(ERROR, "Cannot drop %s because other objects depend on it"
			 "\n\tUse DROP ... CASCADE to drop the dependent objects too",
			 objDescription);

	heap_close(depRel, RowExclusiveLock);

	pfree(objDescription);
}


/*
 * recursiveDeletion: delete a single object for performDeletion.
 *
 * Returns TRUE if successful, FALSE if not.
 *
 * callingObject is NULL at the outer level, else identifies the object that
 * we recursed from (the reference object that someone else needs to delete).
 * pending is a linked list of objects that outer recursion levels want to
 * delete.  We remove the target object from any outer-level list it may
 * appear in.
 * depRel is the already-open pg_depend relation.
 *
 * In RESTRICT mode, we perform all the deletions anyway, but elog a NOTICE
 * and return FALSE if we find a restriction violation.  performDeletion
 * will then abort the transaction to nullify the deletions.  We have to
 * do it this way to (a) report all the direct and indirect dependencies
 * while (b) not going into infinite recursion if there's a cycle.
 *
 * This is even more complex than one could wish, because it is possible for
 * the same pair of objects to be related by both NORMAL and AUTO (or IMPLICIT)
 * dependencies.  (Since one or both paths might be indirect, it's very hard
 * to prevent this; we must cope instead.)  If there is an AUTO/IMPLICIT
 * deletion path then we should perform the deletion, and not fail because
 * of the NORMAL dependency.  So, when we hit a NORMAL dependency we don't
 * immediately decide we've failed; instead we stick the NORMAL dependent
 * object into a list of pending deletions.  If we find a legal path to delete
 * that object later on, the recursive call will remove it from our pending
 * list.  After we've exhausted all such possibilities, we remove the
 * remaining pending objects anyway, but emit a notice and prepare to return
 * FALSE.  (We have to do it this way because the dependent objects *must* be
 * removed before we can remove the object they depend on.)
 *
 * Note: in the case where the AUTO path is traversed first, we will never
 * see the NORMAL dependency path because of the pg_depend removals done in
 * recursive executions of step 1.  The pending list is necessary essentially
 * just to make the behavior independent of the order in which pg_depend
 * entries are visited.
 */
static bool
recursiveDeletion(const ObjectAddress *object,
				  DropBehavior behavior,
				  const ObjectAddress *callingObject,
				  ObjectAddresses *pending,
				  Relation depRel)
{
	bool			ok = true;
	char		   *objDescription;
	ObjectAddresses	mypending;
	ScanKeyData		key[3];
	int				nkeys;
	SysScanDesc		scan;
	HeapTuple		tup;
	ObjectAddress	otherObject;
	ObjectAddress	owningObject;
	bool			amOwned = false;

	/*
	 * Get object description for possible use in messages.  Must do this
	 * before deleting it ...
	 */
	objDescription = getObjectDescription(object);

	/*
	 * Initialize list of restricted objects, and set up chain link.
	 */
	init_object_addresses(&mypending);
	mypending.link = pending;

	/*
	 * Step 1: find and remove pg_depend records that link from this
	 * object to others.  We have to do this anyway, and doing it first
	 * ensures that we avoid infinite recursion in the case of cycles.
	 * Also, some dependency types require extra processing here.
	 *
	 * When dropping a whole object (subId = 0), remove all pg_depend
	 * records for its sub-objects too.
	 */
	ScanKeyEntryInitialize(&key[0], 0x0,
						   Anum_pg_depend_classid, F_OIDEQ,
						   ObjectIdGetDatum(object->classId));
	ScanKeyEntryInitialize(&key[1], 0x0,
						   Anum_pg_depend_objid, F_OIDEQ,
						   ObjectIdGetDatum(object->objectId));
	if (object->objectSubId != 0)
	{
		ScanKeyEntryInitialize(&key[2], 0x0,
							   Anum_pg_depend_objsubid, F_INT4EQ,
							   Int32GetDatum(object->objectSubId));
		nkeys = 3;
	}
	else
		nkeys = 2;

	scan = systable_beginscan(depRel, DependDependerIndex, true,
							  SnapshotNow, nkeys, key);

	while (HeapTupleIsValid(tup = systable_getnext(scan)))
	{
		Form_pg_depend	foundDep = (Form_pg_depend) GETSTRUCT(tup);

		otherObject.classId = foundDep->refclassid;
		otherObject.objectId = foundDep->refobjid;
		otherObject.objectSubId = foundDep->refobjsubid;

		switch (foundDep->deptype)
		{
			case DEPENDENCY_NORMAL:
			case DEPENDENCY_AUTO:
				/* no problem */
				break;
			case DEPENDENCY_INTERNAL:
				/*
				 * This object is part of the internal implementation
				 * of another object.  We have three cases:
				 *
				 * 1. At the outermost recursion level, disallow the DROP.
				 * (We just elog here, rather than considering this drop
				 * to be pending, since no other dependencies are likely
				 * to be interesting.)
				 */
				if (callingObject == NULL)
				{
					char *otherObjDesc = getObjectDescription(&otherObject);

					elog(ERROR, "Cannot drop %s because %s requires it"
						 "\n\tYou may drop %s instead",
						 objDescription, otherObjDesc, otherObjDesc);
				}
				/*
				 * 2. When recursing from the other end of this dependency,
				 * it's okay to continue with the deletion.  This holds when
				 * recursing from a whole object that includes the nominal
				 * other end as a component, too.
				 */
				if (callingObject->classId == otherObject.classId &&
					callingObject->objectId == otherObject.objectId &&
					(callingObject->objectSubId == otherObject.objectSubId ||
					 callingObject->objectSubId == 0))
					break;
				/*
				 * 3. When recursing from anyplace else, transform this
				 * deletion request into a delete of the other object.
				 * (This will be an error condition iff RESTRICT mode.)
				 * In this case we finish deleting my dependencies except
				 * for the INTERNAL link, which will be needed to cause
				 * the owning object to recurse back to me.
				 */
				if (amOwned)	/* shouldn't happen */
					elog(ERROR, "recursiveDeletion: multiple INTERNAL dependencies for %s",
						 objDescription);
				owningObject = otherObject;
				amOwned = true;
				/* "continue" bypasses the simple_heap_delete call below */
				continue;
			case DEPENDENCY_PIN:
				/*
				 * Should not happen; PIN dependencies should have zeroes
				 * in the depender fields...
				 */
				elog(ERROR, "recursiveDeletion: incorrect use of PIN dependency with %s",
					 objDescription);
				break;
			default:
				elog(ERROR, "recursiveDeletion: unknown dependency type '%c' for %s",
					 foundDep->deptype, objDescription);
				break;
		}

		simple_heap_delete(depRel, &tup->t_self);
	}

	systable_endscan(scan);

	/*
	 * CommandCounterIncrement here to ensure that preceding changes
	 * are all visible; in particular, that the above deletions of pg_depend
	 * entries are visible.  That prevents infinite recursion in case of
	 * a dependency loop (which is perfectly legal).
	 */
	CommandCounterIncrement();

	/*
	 * If we found we are owned by another object, ask it to delete itself
	 * instead of proceeding.
	 */
	if (amOwned)
	{
		if (behavior == DROP_RESTRICT)
		{
			elog(NOTICE, "%s depends on %s",
				 getObjectDescription(&owningObject),
				 objDescription);
			ok = false;
		}
		else
			elog(NOTICE, "Drop cascades to %s",
				 getObjectDescription(&owningObject));

		if (!recursiveDeletion(&owningObject, behavior,
							   object,
							   pending, depRel))
			ok = false;

		pfree(objDescription);
		term_object_addresses(&mypending);

		return ok;
	}

	/*
	 * Step 2: scan pg_depend records that link to this object, showing
	 * the things that depend on it.  Recursively delete those things.
	 * (We don't delete the pg_depend records here, as the recursive call
	 * will do that.)  Note it's important to delete the dependent objects
	 * before the referenced one, since the deletion routines might do
	 * things like try to update the pg_class record when deleting a
	 * check constraint.
	 *
	 * Again, when dropping a whole object (subId = 0), find pg_depend
	 * records for its sub-objects too.
	 *
	 * NOTE: because we are using SnapshotNow, if a recursive call deletes
	 * any pg_depend tuples that our scan hasn't yet visited, we will not see
	 * them as good when we do visit them.  This is essential for correct
	 * behavior if there are multiple dependency paths between two objects
	 * --- else we might try to delete an already-deleted object.
	 */
	ScanKeyEntryInitialize(&key[0], 0x0,
						   Anum_pg_depend_refclassid, F_OIDEQ,
						   ObjectIdGetDatum(object->classId));
	ScanKeyEntryInitialize(&key[1], 0x0,
						   Anum_pg_depend_refobjid, F_OIDEQ,
						   ObjectIdGetDatum(object->objectId));
	if (object->objectSubId != 0)
	{
		ScanKeyEntryInitialize(&key[2], 0x0,
							   Anum_pg_depend_refobjsubid, F_INT4EQ,
							   Int32GetDatum(object->objectSubId));
		nkeys = 3;
	}
	else
		nkeys = 2;

	scan = systable_beginscan(depRel, DependReferenceIndex, true,
							  SnapshotNow, nkeys, key);

	while (HeapTupleIsValid(tup = systable_getnext(scan)))
	{
		Form_pg_depend	foundDep = (Form_pg_depend) GETSTRUCT(tup);

		otherObject.classId = foundDep->classid;
		otherObject.objectId = foundDep->objid;
		otherObject.objectSubId = foundDep->objsubid;

		switch (foundDep->deptype)
		{
			case DEPENDENCY_NORMAL:
				if (behavior == DROP_RESTRICT)
				{
					/*
					 * We've found a restricted object (or at least one
					 * that's not deletable along this path).  Log for later
					 * processing.  (Note it's okay if the same object gets
					 * into mypending multiple times.)
					 */
					add_exact_object_address(&otherObject, &mypending);
				}
				else
				{
					elog(NOTICE, "Drop cascades to %s",
						 getObjectDescription(&otherObject));

					if (!recursiveDeletion(&otherObject, behavior,
										   object,
										   &mypending, depRel))
						ok = false;
				}
				break;
			case DEPENDENCY_AUTO:
			case DEPENDENCY_INTERNAL:
				/*
				 * We propagate the DROP without complaint even in the
				 * RESTRICT case.  (However, normal dependencies on the
				 * component object could still cause failure.)
				 */
				elog(DEBUG1, "Drop auto-cascades to %s",
					 getObjectDescription(&otherObject));

				if (!recursiveDeletion(&otherObject, behavior,
									   object,
									   &mypending, depRel))
					ok = false;
				break;
			case DEPENDENCY_PIN:
				/*
				 * For a PIN dependency we just elog immediately; there
				 * won't be any others to report.
				 */
				elog(ERROR, "Cannot drop %s because it is required by the database system",
					 objDescription);
				break;
			default:
				elog(ERROR, "recursiveDeletion: unknown dependency type '%c' for %s",
					 foundDep->deptype, objDescription);
				break;
		}
	}

	systable_endscan(scan);

	/*
	 * If we found no restricted objects, or got rid of them all via other
	 * paths, we're in good shape.  Otherwise continue step 2 by processing
	 * the remaining restricted objects.
	 */
	if (mypending.numrefs > 0)
	{
		/*
		 * Successively extract and delete each remaining object.
		 * Note that the right things will happen if some of these objects
		 * depend on others: we'll report/delete each one exactly once.
		 */
		while (mypending.numrefs > 0)
		{
			ObjectAddress	otherObject = mypending.refs[0];

			del_object_address_by_index(0, &mypending);

			elog(NOTICE, "%s depends on %s",
				 getObjectDescription(&otherObject),
				 objDescription);
			if (!recursiveDeletion(&otherObject, behavior,
								   object,
								   &mypending, depRel))
				ok = false;
		}

		ok = false;
	}

	/*
	 * We do not need CommandCounterIncrement here, since if step 2 did
	 * anything then each recursive call will have ended with one.
	 */

	/*
	 * Step 3: delete the object itself.
	 */
	doDeletion(object);

	/*
	 * Delete any comments associated with this object.  (This is a convenient
	 * place to do it instead of having every object type know to do it.)
	 */
	DeleteComments(object->objectId, object->classId, object->objectSubId);

	/*
	 * If this object is mentioned in any caller's pending list, remove it.
	 */
	del_object_address(object, pending);

	/*
	 * CommandCounterIncrement here to ensure that preceding changes
	 * are all visible.
	 */
	CommandCounterIncrement();

	/*
	 * And we're done!
	 */
	pfree(objDescription);
	term_object_addresses(&mypending);

	return ok;
}


/*
 * doDeletion: actually delete a single object
 */
static void
doDeletion(const ObjectAddress *object)
{
	switch (getObjectClass(object))
	{
		case OCLASS_CLASS:
		{
			HeapTuple	relTup;
			char		relKind;

			/*
			 * Need the relkind to figure out how to drop.
			 */
			relTup = SearchSysCache(RELOID,
									ObjectIdGetDatum(object->objectId),
									0, 0, 0);
			if (!HeapTupleIsValid(relTup))
				elog(ERROR, "doDeletion: Relation %u does not exist",
					 object->objectId);
			relKind = ((Form_pg_class) GETSTRUCT(relTup))->relkind;
			ReleaseSysCache(relTup);

			if (relKind == RELKIND_INDEX)
			{
				Assert(object->objectSubId == 0);
				index_drop(object->objectId);
			}
			else
			{
				if (object->objectSubId != 0)
					elog(ERROR, "DROP COLUMN not implemented yet");
				else
					heap_drop_with_catalog(object->objectId);
			}
			break;
		}

		case OCLASS_PROC:
			RemoveFunctionById(object->objectId);
			break;

		case OCLASS_TYPE:
			RemoveTypeById(object->objectId);
			break;

		case OCLASS_CONSTRAINT:
			RemoveConstraintById(object->objectId);
			break;

		case OCLASS_CONVERSION:
			RemoveConversionById(object->objectId);
			break;

		case OCLASS_DEFAULT:
			RemoveAttrDefaultById(object->objectId);
			break;

		case OCLASS_LANGUAGE:
			DropProceduralLanguageById(object->objectId);
			break;

		case OCLASS_OPERATOR:
			RemoveOperatorById(object->objectId);
			break;

		case OCLASS_REWRITE:
			RemoveRewriteRuleById(object->objectId);
			break;

		case OCLASS_TRIGGER:
			RemoveTriggerById(object->objectId);
			break;

		case OCLASS_SCHEMA:
			RemoveSchemaById(object->objectId);
			break;

		case OCLASS_CAST:
			DropCastById(object->objectId);
			break;

		default:
			elog(ERROR, "doDeletion: Unsupported object class %u",
				 object->classId);
	}
}

/*
 * recordDependencyOnExpr - find expression dependencies
 *
 * This is used to find the dependencies of rules, constraint expressions,
 * etc.
 *
 * Given an expression or query in node-tree form, find all the objects
 * it refers to (tables, columns, operators, functions, etc).  Record
 * a dependency of the specified type from the given depender object
 * to each object mentioned in the expression.
 *
 * rtable is the rangetable to be used to interpret Vars with varlevelsup=0.
 * It can be NIL if no such variables are expected.
 *
 * XXX is it important to create dependencies on the datatypes mentioned in
 * the expression?  In most cases this would be redundant (eg, a ref to an
 * operator indirectly references its input and output datatypes), but I'm
 * not quite convinced there are no cases where we need it.
 */
void
recordDependencyOnExpr(const ObjectAddress *depender,
					   Node *expr, List *rtable,
					   DependencyType behavior)
{
	find_expr_references_context	context;

	init_object_addresses(&context.addrs);

	/* Set up interpretation for Vars at varlevelsup = 0 */
	context.rtables = makeList1(rtable);

	/* Scan the expression tree for referenceable objects */
	find_expr_references_walker(expr, &context);

	/* Remove any duplicates */
	eliminate_duplicate_dependencies(&context.addrs);

	/* And record 'em */
	recordMultipleDependencies(depender,
							   context.addrs.refs, context.addrs.numrefs,
							   behavior);

	term_object_addresses(&context.addrs);
}

/*
 * Recursively search an expression tree for object references.
 */
static bool
find_expr_references_walker(Node *node,
							find_expr_references_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;
		int			levelsup;
		List	   *rtable,
				   *rtables;
		RangeTblEntry *rte;

		/* Find matching rtable entry, or complain if not found */
		levelsup = var->varlevelsup;
		rtables = context->rtables;
		while (levelsup--)
		{
			if (rtables == NIL)
				break;
			rtables = lnext(rtables);
		}
		if (rtables == NIL)
			elog(ERROR, "find_expr_references_walker: bogus varlevelsup %d",
				 var->varlevelsup);
		rtable = lfirst(rtables);
		if (var->varno <= 0 || var->varno > length(rtable))
			elog(ERROR, "find_expr_references_walker: bogus varno %d",
				 var->varno);
		rte = rt_fetch(var->varno, rtable);
		/* If it's a plain relation, reference this column */
		if (rte->rtekind == RTE_RELATION)
			add_object_address(OCLASS_CLASS, rte->relid, var->varattno,
							   &context->addrs);
		return false;
	}
	if (IsA(node, Expr))
	{
		Expr	   *expr = (Expr *) node;

		if (expr->opType == OP_EXPR)
		{
			Oper	   *oper = (Oper *) expr->oper;

			add_object_address(OCLASS_OPERATOR, oper->opno, 0,
							   &context->addrs);
		}
		else if (expr->opType == FUNC_EXPR)
		{
			Func	   *func = (Func *) expr->oper;

			add_object_address(OCLASS_PROC, func->funcid, 0,
							   &context->addrs);
		}
		/* fall through to examine arguments */
	}
	if (IsA(node, Aggref))
	{
		Aggref	   *aggref = (Aggref *) node;

		add_object_address(OCLASS_PROC, aggref->aggfnoid, 0,
						   &context->addrs);
		/* fall through to examine arguments */
	}
	if (is_subplan(node))
	{
		/* Extra work needed here if we ever need this case */
		elog(ERROR, "find_expr_references_walker: already-planned subqueries not supported");
	}
	if (IsA(node, Query))
	{
		/* Recurse into RTE subquery or not-yet-planned sublink subquery */
		Query	   *query = (Query *) node;
		List	   *rtable;
		bool		result;

		/*
		 * Add whole-relation refs for each plain relation mentioned in the
		 * subquery's rtable.  (Note: query_tree_walker takes care of
		 * recursing into RTE_FUNCTION and RTE_SUBQUERY RTEs, so no need
		 * to do that here.)
		 */
		foreach(rtable, query->rtable)
		{
			RangeTblEntry *rte = (RangeTblEntry *) lfirst(rtable);

			if (rte->rtekind == RTE_RELATION)
				add_object_address(OCLASS_CLASS, rte->relid, 0,
								   &context->addrs);
		}

		/* Examine substructure of query */
		context->rtables = lcons(query->rtable, context->rtables);
		result = query_tree_walker(query,
								   find_expr_references_walker,
								   (void *) context, true);
		context->rtables = lnext(context->rtables);
		return result;
	}
	return expression_tree_walker(node, find_expr_references_walker,
								  (void *) context);
}

/*
 * Given an array of dependency references, eliminate any duplicates.
 */
static void
eliminate_duplicate_dependencies(ObjectAddresses *addrs)
{
	ObjectAddress  *priorobj;
	int			oldref,
				newrefs;

	if (addrs->numrefs <= 1)
		return;					/* nothing to do */

	/* Sort the refs so that duplicates are adjacent */
	qsort((void *) addrs->refs, addrs->numrefs, sizeof(ObjectAddress),
		  object_address_comparator);

	/* Remove dups */
	priorobj = addrs->refs;
	newrefs = 1;
	for (oldref = 1; oldref < addrs->numrefs; oldref++)
	{
		ObjectAddress  *thisobj = addrs->refs + oldref;

		if (priorobj->classId == thisobj->classId &&
			priorobj->objectId == thisobj->objectId)
		{
			if (priorobj->objectSubId == thisobj->objectSubId)
				continue;		/* identical, so drop thisobj */
			/*
			 * If we have a whole-object reference and a reference to a
			 * part of the same object, we don't need the whole-object
			 * reference (for example, we don't need to reference both
			 * table foo and column foo.bar).  The whole-object reference
			 * will always appear first in the sorted list.
			 */
			if (priorobj->objectSubId == 0)
			{
				/* replace whole ref with partial */
				priorobj->objectSubId = thisobj->objectSubId;
				continue;
			}
		}
		/* Not identical, so add thisobj to output set */
		priorobj++;
		priorobj->classId = thisobj->classId;
		priorobj->objectId = thisobj->objectId;
		priorobj->objectSubId = thisobj->objectSubId;
		newrefs++;
	}

	addrs->numrefs = newrefs;
}

/*
 * qsort comparator for ObjectAddress items
 */
static int
object_address_comparator(const void *a, const void *b)
{
	const ObjectAddress *obja = (const ObjectAddress *) a;
	const ObjectAddress *objb = (const ObjectAddress *) b;

	if (obja->classId < objb->classId)
		return -1;
	if (obja->classId > objb->classId)
		return 1;
	if (obja->objectId < objb->objectId)
		return -1;
	if (obja->objectId > objb->objectId)
		return 1;
	/*
	 * We sort the subId as an unsigned int so that 0 will come first.
	 * See logic in eliminate_duplicate_dependencies.
	 */
	if ((unsigned int) obja->objectSubId < (unsigned int) objb->objectSubId)
		return -1;
	if ((unsigned int) obja->objectSubId > (unsigned int) objb->objectSubId)
		return 1;
	return 0;
}

/*
 * Routines for handling an expansible array of ObjectAddress items.
 *
 * init_object_addresses: initialize an ObjectAddresses array.
 */
static void
init_object_addresses(ObjectAddresses *addrs)
{
	/* Initialize array to empty */
	addrs->numrefs = 0;
	addrs->maxrefs = 32;		/* arbitrary initial array size */
	addrs->refs = (ObjectAddress *)
		palloc(addrs->maxrefs * sizeof(ObjectAddress));
	addrs->link = NULL;

	/* Initialize object_classes[] if not done yet */
	/* This will be needed by add_object_address() */
	if (!object_classes_initialized)
		init_object_classes();
}

/*
 * Add an entry to an ObjectAddresses array.
 *
 * It is convenient to specify the class by ObjectClass rather than directly
 * by catalog OID.
 */
static void
add_object_address(ObjectClasses oclass, Oid objectId, int32 subId,
				   ObjectAddresses *addrs)
{
	ObjectAddress  *item;

	/* enlarge array if needed */
	if (addrs->numrefs >= addrs->maxrefs)
	{
		addrs->maxrefs *= 2;
		addrs->refs = (ObjectAddress *)
			repalloc(addrs->refs, addrs->maxrefs * sizeof(ObjectAddress));
	}
	/* record this item */
	item = addrs->refs + addrs->numrefs;
	item->classId = object_classes[oclass];
	item->objectId = objectId;
	item->objectSubId = subId;
	addrs->numrefs++;
}

/*
 * Add an entry to an ObjectAddresses array.
 *
 * As above, but specify entry exactly.
 */
static void
add_exact_object_address(const ObjectAddress *object,
						 ObjectAddresses *addrs)
{
	ObjectAddress  *item;

	/* enlarge array if needed */
	if (addrs->numrefs >= addrs->maxrefs)
	{
		addrs->maxrefs *= 2;
		addrs->refs = (ObjectAddress *)
			repalloc(addrs->refs, addrs->maxrefs * sizeof(ObjectAddress));
	}
	/* record this item */
	item = addrs->refs + addrs->numrefs;
	*item = *object;
	addrs->numrefs++;
}

/*
 * If an ObjectAddresses array contains any matches for the given object,
 * remove it/them.  Also, do the same in any linked ObjectAddresses arrays.
 */
static void
del_object_address(const ObjectAddress *object,
				   ObjectAddresses *addrs)
{
	for (; addrs != NULL; addrs = addrs->link)
	{
		int			i;

		/* Scan backwards to simplify deletion logic. */
		for (i = addrs->numrefs-1; i >= 0; i--)
		{
			ObjectAddress  *thisobj = addrs->refs + i;

			if (object->classId == thisobj->classId &&
				object->objectId == thisobj->objectId)
			{
				/*
				 * Delete if exact match, or if thisobj is a subobject of
				 * the passed-in object.
				 */
				if (object->objectSubId == thisobj->objectSubId ||
					object->objectSubId == 0)
					del_object_address_by_index(i, addrs);
			}
		}
	}
}

/*
 * Remove an entry (specified by array index) from an ObjectAddresses array.
 * The end item in the list is moved down to fill the hole.
 */
static void
del_object_address_by_index(int index, ObjectAddresses *addrs)
{
	Assert(index >= 0 && index < addrs->numrefs);
	addrs->refs[index] = addrs->refs[addrs->numrefs - 1];
	addrs->numrefs--;
}

/*
 * Clean up when done with an ObjectAddresses array.
 */
static void
term_object_addresses(ObjectAddresses *addrs)
{
	pfree(addrs->refs);
}

/*
 * Initialize the object_classes[] table.
 *
 * Although some of these OIDs aren't compile-time constants, they surely
 * shouldn't change during a backend's run.  So, we look them up the
 * first time through and then cache them.
 */
static void
init_object_classes(void)
{
	object_classes[OCLASS_CAST] = get_system_catalog_relid(CastRelationName);
	object_classes[OCLASS_CLASS] = RelOid_pg_class;
	object_classes[OCLASS_PROC] = RelOid_pg_proc;
	object_classes[OCLASS_TYPE] = RelOid_pg_type;
	object_classes[OCLASS_CONSTRAINT] = get_system_catalog_relid(ConstraintRelationName);
	object_classes[OCLASS_CONVERSION] = get_system_catalog_relid(ConversionRelationName);
	object_classes[OCLASS_DEFAULT] = get_system_catalog_relid(AttrDefaultRelationName);
	object_classes[OCLASS_LANGUAGE] = get_system_catalog_relid(LanguageRelationName);
	object_classes[OCLASS_OPERATOR] = get_system_catalog_relid(OperatorRelationName);
	object_classes[OCLASS_REWRITE] = get_system_catalog_relid(RewriteRelationName);
	object_classes[OCLASS_TRIGGER] = get_system_catalog_relid(TriggerRelationName);
	object_classes[OCLASS_SCHEMA] = get_system_catalog_relid(NamespaceRelationName);
	object_classes_initialized = true;
}

/*
 * Determine the class of a given object identified by objectAddress.
 *
 * This function is needed just because some of the system catalogs do
 * not have hardwired-at-compile-time OIDs.
 */
static ObjectClasses
getObjectClass(const ObjectAddress *object)
{
	/* Easy for the bootstrapped catalogs... */
	switch (object->classId)
	{
		case RelOid_pg_class:
			/* caller must check objectSubId */
			return OCLASS_CLASS;

		case RelOid_pg_proc:
			Assert(object->objectSubId == 0);
			return OCLASS_PROC;

		case RelOid_pg_type:
			Assert(object->objectSubId == 0);
			return OCLASS_TYPE;
	}

	/*
	 * Handle cases where catalog's OID is not hardwired.
	 */
	if (!object_classes_initialized)
		init_object_classes();

	if (object->classId == object_classes[OCLASS_CAST])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_CAST;
	}
	if (object->classId == object_classes[OCLASS_CONSTRAINT])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_CONSTRAINT;
	}
	if (object->classId == object_classes[OCLASS_CONVERSION])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_CONVERSION;
	}
	if (object->classId == object_classes[OCLASS_DEFAULT])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_DEFAULT;
	}
	if (object->classId == object_classes[OCLASS_LANGUAGE])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_LANGUAGE;
	}
	if (object->classId == object_classes[OCLASS_OPERATOR])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_OPERATOR;
	}
	if (object->classId == object_classes[OCLASS_REWRITE])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_REWRITE;
	}
	if (object->classId == object_classes[OCLASS_TRIGGER])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_TRIGGER;
	}
	if (object->classId == object_classes[OCLASS_SCHEMA])
	{
		Assert(object->objectSubId == 0);
		return OCLASS_SCHEMA;
	}

	elog(ERROR, "getObjectClass: Unknown object class %u",
		 object->classId);
	return OCLASS_CLASS;		/* keep compiler quiet */
}

/*
 * getObjectDescription: build an object description for messages
 *
 * The result is a palloc'd string.
 */
static char *
getObjectDescription(const ObjectAddress *object)
{
	StringInfoData buffer;

	initStringInfo(&buffer);

	switch (getObjectClass(object))
	{
		case OCLASS_CAST:
			appendStringInfo(&buffer, "cast");
			break;

		case OCLASS_CLASS:
			getRelationDescription(&buffer, object->objectId);
			if (object->objectSubId != 0)
				appendStringInfo(&buffer, " column %s",
								 get_attname(object->objectId,
											 object->objectSubId));
			break;

		case OCLASS_PROC:
			/* XXX could improve on this */
			appendStringInfo(&buffer, "function %s",
							 get_func_name(object->objectId));
			break;

		case OCLASS_TYPE:
		{
			HeapTuple		typeTup;

			typeTup = SearchSysCache(TYPEOID,
									 ObjectIdGetDatum(object->objectId),
									 0, 0, 0);
			if (!HeapTupleIsValid(typeTup))
				elog(ERROR, "getObjectDescription: Type %u does not exist",
					 object->objectId);
			appendStringInfo(&buffer, "type %s",
							 NameStr(((Form_pg_type) GETSTRUCT(typeTup))->typname));
			ReleaseSysCache(typeTup);
			break;
		}

		case OCLASS_CONSTRAINT:
		{
			Relation		conDesc;
			ScanKeyData		skey[1];
			SysScanDesc		rcscan;
			HeapTuple		tup;
			Form_pg_constraint con;

			conDesc = heap_openr(ConstraintRelationName, AccessShareLock);

			ScanKeyEntryInitialize(&skey[0], 0x0,
								   ObjectIdAttributeNumber, F_OIDEQ,
								   ObjectIdGetDatum(object->objectId));

			rcscan = systable_beginscan(conDesc, ConstraintOidIndex, true,
										SnapshotNow, 1, skey);

			tup = systable_getnext(rcscan);

			if (!HeapTupleIsValid(tup))
				elog(ERROR, "getObjectDescription: Constraint %u does not exist",
					 object->objectId);

			con = (Form_pg_constraint) GETSTRUCT(tup);

			if (OidIsValid(con->conrelid))
			{
				appendStringInfo(&buffer, "constraint %s on ",
								 NameStr(con->conname));
				getRelationDescription(&buffer, con->conrelid);
			}
			else
			{
				appendStringInfo(&buffer, "constraint %s",
								 NameStr(con->conname));
			}

			systable_endscan(rcscan);
			heap_close(conDesc, AccessShareLock);
			break;
		}

		case OCLASS_CONVERSION:
		{
			HeapTuple		conTup;

			conTup = SearchSysCache(CONOID,
									 ObjectIdGetDatum(object->objectId),
									 0, 0, 0);
			if (!HeapTupleIsValid(conTup))
				elog(ERROR, "getObjectDescription: Conversion %u does not exist",
					 object->objectId);
			appendStringInfo(&buffer, "conversion %s",
							 NameStr(((Form_pg_conversion) GETSTRUCT(conTup))->conname));
			ReleaseSysCache(conTup);
			break;
		}

		case OCLASS_DEFAULT:
		{
			Relation		attrdefDesc;
			ScanKeyData		skey[1];
			SysScanDesc		adscan;
			HeapTuple		tup;
			Form_pg_attrdef attrdef;
			ObjectAddress	colobject;

			attrdefDesc = heap_openr(AttrDefaultRelationName, AccessShareLock);

			ScanKeyEntryInitialize(&skey[0], 0x0,
								   ObjectIdAttributeNumber, F_OIDEQ,
								   ObjectIdGetDatum(object->objectId));

			adscan = systable_beginscan(attrdefDesc, AttrDefaultOidIndex, true,
										SnapshotNow, 1, skey);

			tup = systable_getnext(adscan);

			if (!HeapTupleIsValid(tup))
				elog(ERROR, "getObjectDescription: Default %u does not exist",
					 object->objectId);

			attrdef = (Form_pg_attrdef) GETSTRUCT(tup);

			colobject.classId = RelOid_pg_class;
			colobject.objectId = attrdef->adrelid;
			colobject.objectSubId = attrdef->adnum;

			appendStringInfo(&buffer, "default for %s",
							 getObjectDescription(&colobject));

			systable_endscan(adscan);
			heap_close(attrdefDesc, AccessShareLock);
			break;
		}

		case OCLASS_LANGUAGE:
		{
			HeapTuple		langTup;

			langTup = SearchSysCache(LANGOID,
									 ObjectIdGetDatum(object->objectId),
									 0, 0, 0);
			if (!HeapTupleIsValid(langTup))
				elog(ERROR, "getObjectDescription: Language %u does not exist",
					 object->objectId);
			appendStringInfo(&buffer, "language %s",
							 NameStr(((Form_pg_language) GETSTRUCT(langTup))->lanname));
			ReleaseSysCache(langTup);
			break;
		}

		case OCLASS_OPERATOR:
			/* XXX could improve on this */
			appendStringInfo(&buffer, "operator %s",
							 get_opname(object->objectId));
			break;

		case OCLASS_REWRITE:
		{
			Relation		ruleDesc;
			ScanKeyData		skey[1];
			SysScanDesc		rcscan;
			HeapTuple		tup;
			Form_pg_rewrite	rule;

			ruleDesc = heap_openr(RewriteRelationName, AccessShareLock);

			ScanKeyEntryInitialize(&skey[0], 0x0,
								   ObjectIdAttributeNumber, F_OIDEQ,
								   ObjectIdGetDatum(object->objectId));

			rcscan = systable_beginscan(ruleDesc, RewriteOidIndex, true,
										SnapshotNow, 1, skey);

			tup = systable_getnext(rcscan);

			if (!HeapTupleIsValid(tup))
				elog(ERROR, "getObjectDescription: Rule %u does not exist",
					 object->objectId);

			rule = (Form_pg_rewrite) GETSTRUCT(tup);

			appendStringInfo(&buffer, "rule %s on ",
							 NameStr(rule->rulename));
			getRelationDescription(&buffer, rule->ev_class);

			systable_endscan(rcscan);
			heap_close(ruleDesc, AccessShareLock);
			break;
		}

		case OCLASS_TRIGGER:
		{
			Relation		trigDesc;
			ScanKeyData		skey[1];
			SysScanDesc		tgscan;
			HeapTuple		tup;
			Form_pg_trigger	trig;

			trigDesc = heap_openr(TriggerRelationName, AccessShareLock);

			ScanKeyEntryInitialize(&skey[0], 0x0,
								   ObjectIdAttributeNumber, F_OIDEQ,
								   ObjectIdGetDatum(object->objectId));

			tgscan = systable_beginscan(trigDesc, TriggerOidIndex, true,
										SnapshotNow, 1, skey);

			tup = systable_getnext(tgscan);

			if (!HeapTupleIsValid(tup))
				elog(ERROR, "getObjectDescription: Trigger %u does not exist",
					 object->objectId);

			trig = (Form_pg_trigger) GETSTRUCT(tup);

			appendStringInfo(&buffer, "trigger %s on ",
							 NameStr(trig->tgname));
			getRelationDescription(&buffer, trig->tgrelid);

			systable_endscan(tgscan);
			heap_close(trigDesc, AccessShareLock);
			break;
		}

		case OCLASS_SCHEMA:
		{
			HeapTuple		schemaTup;

			schemaTup = SearchSysCache(NAMESPACEOID,
									   ObjectIdGetDatum(object->objectId),
									   0, 0, 0);
			if (!HeapTupleIsValid(schemaTup))
				elog(ERROR, "getObjectDescription: Schema %u does not exist",
					 object->objectId);
			appendStringInfo(&buffer, "schema %s",
							 NameStr(((Form_pg_namespace) GETSTRUCT(schemaTup))->nspname));
			ReleaseSysCache(schemaTup);
			break;
		}

		default:
			appendStringInfo(&buffer, "unknown object %u %u %d",
							 object->classId,
							 object->objectId,
							 object->objectSubId);
			break;
	}

	return buffer.data;
}

/*
 * subroutine for getObjectDescription: describe a relation
 */
static void
getRelationDescription(StringInfo buffer, Oid relid)
{
	HeapTuple	relTup;
	Form_pg_class	relForm;

	relTup = SearchSysCache(RELOID,
							ObjectIdGetDatum(relid),
							0, 0, 0);
	if (!HeapTupleIsValid(relTup))
		elog(ERROR, "getObjectDescription: Relation %u does not exist",
			 relid);
	relForm = (Form_pg_class) GETSTRUCT(relTup);

	switch (relForm->relkind)
	{
		case RELKIND_RELATION:
			appendStringInfo(buffer, "table %s",
							 NameStr(relForm->relname));
			break;
		case RELKIND_INDEX:
			appendStringInfo(buffer, "index %s",
							 NameStr(relForm->relname));
			break;
		case RELKIND_SPECIAL:
			appendStringInfo(buffer, "special system relation %s",
							 NameStr(relForm->relname));
			break;
		case RELKIND_SEQUENCE:
			appendStringInfo(buffer, "sequence %s",
							 NameStr(relForm->relname));
			break;
		case RELKIND_UNCATALOGED:
			appendStringInfo(buffer, "uncataloged table %s",
							 NameStr(relForm->relname));
			break;
		case RELKIND_TOASTVALUE:
			appendStringInfo(buffer, "toast table %s",
							 NameStr(relForm->relname));
			break;
		case RELKIND_VIEW:
			appendStringInfo(buffer, "view %s",
							 NameStr(relForm->relname));
			break;
		default:
			/* shouldn't get here */
			appendStringInfo(buffer, "relation %s",
							 NameStr(relForm->relname));
			break;
	}

	ReleaseSysCache(relTup);
}
