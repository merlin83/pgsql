/*-------------------------------------------------------------------------
 *
 * parse_func.c
 *		handle function calls in parser
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/parser/parse_func.c,v 1.81 2000-05-30 00:49:50 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/indexing.h"
#include "catalog/pg_aggregate.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_proc.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_agg.h"
#include "parser/parse_coerce.h"
#include "parser/parse_expr.h"
#include "parser/parse_func.h"
#include "parser/parse_relation.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

static Node *ParseComplexProjection(ParseState *pstate,
					   char *funcname,
					   Node *first_arg,
					   bool *attisset);
static Oid **argtype_inherit(int nargs, Oid *oid_array);

static int	find_inheritors(Oid relid, Oid **supervec);
static CandidateList func_get_candidates(char *funcname, int nargs);
static bool
func_get_detail(char *funcname,
				int nargs,
				Oid *oid_array,
				Oid *funcid,	/* return value */
				Oid *rettype,	/* return value */
				bool *retset,	/* return value */
				Oid **true_typeids);
static Oid **gen_cross_product(InhPaths *arginh, int nargs);
static void make_arguments(ParseState *pstate,
			   int nargs,
			   List *fargs,
			   Oid *input_typeids,
			   Oid *function_typeids);
static int match_argtypes(int nargs,
			   Oid *input_typeids,
			   CandidateList function_typeids,
			   CandidateList *candidates);
static List *setup_tlist(char *attname, Oid relid);
static Oid *func_select_candidate(int nargs, Oid *input_typeids,
					  CandidateList candidates);
static int	agg_get_candidates(char *aggname, Oid typeId, CandidateList *candidates);
static Oid	agg_select_candidate(Oid typeid, CandidateList candidates);


/*
 ** ParseNestedFuncOrColumn
 **    Given a nested dot expression (i.e. (relation func ... attr), build up
 ** a tree with of Iter and Func nodes.
 */
Node *
ParseNestedFuncOrColumn(ParseState *pstate, Attr *attr, int *curr_resno, int precedence)
{
	List	   *mutator_iter;
	Node	   *retval = NULL;

	if (attr->paramNo != NULL)
	{
		Param	   *param = (Param *) transformExpr(pstate, (Node *) attr->paramNo, EXPR_RELATION_FIRST);

		retval = ParseFuncOrColumn(pstate, strVal(lfirst(attr->attrs)),
								   lcons(param, NIL),
								   false, false,
								   curr_resno,
								   precedence);
	}
	else
	{
		Ident	   *ident = makeNode(Ident);

		ident->name = attr->relname;
		ident->isRel = TRUE;
		retval = ParseFuncOrColumn(pstate, strVal(lfirst(attr->attrs)),
								   lcons(ident, NIL),
								   false, false,
								   curr_resno,
								   precedence);
	}

	/* Do more attributes follow this one? */
	foreach(mutator_iter, lnext(attr->attrs))
	{
		retval = ParseFuncOrColumn(pstate, strVal(lfirst(mutator_iter)),
								   lcons(retval, NIL),
								   false, false,
								   curr_resno,
								   precedence);
	}

	return retval;
}

static int
agg_get_candidates(char *aggname,
				   Oid typeId,
				   CandidateList *candidates)
{
	CandidateList current_candidate;
	Relation	pg_aggregate_desc;
	HeapScanDesc pg_aggregate_scan;
	HeapTuple	tup;
	Form_pg_aggregate agg;
	int			ncandidates = 0;
	ScanKeyData aggKey[1];

	*candidates = NULL;

	ScanKeyEntryInitialize(&aggKey[0], 0,
						   Anum_pg_aggregate_aggname,
						   F_NAMEEQ,
						   NameGetDatum(aggname));

	pg_aggregate_desc = heap_openr(AggregateRelationName, AccessShareLock);
	pg_aggregate_scan = heap_beginscan(pg_aggregate_desc,
									   0,
									   SnapshotSelf,	/* ??? */
									   1,
									   aggKey);

	while (HeapTupleIsValid(tup = heap_getnext(pg_aggregate_scan, 0)))
	{
		agg = (Form_pg_aggregate) GETSTRUCT(tup);

		current_candidate = (CandidateList) palloc(sizeof(struct _CandidateList));
		current_candidate->args = (Oid *) palloc(sizeof(Oid));

		current_candidate->args[0] = agg->aggbasetype;
		current_candidate->next = *candidates;
		*candidates = current_candidate;
		ncandidates++;
	}

	heap_endscan(pg_aggregate_scan);
	heap_close(pg_aggregate_desc, AccessShareLock);

	return ncandidates;
}	/* agg_get_candidates() */

/* agg_select_candidate()
 *
 * Try to choose only one candidate aggregate function from a list of
 * possible matches.  Return value is Oid of input type of aggregate
 * if successful, else InvalidOid.
 */
static Oid
agg_select_candidate(Oid typeid, CandidateList candidates)
{
	CandidateList current_candidate;
	CandidateList last_candidate;
	Oid			current_typeid;
	int			ncandidates;
	CATEGORY	category,
				current_category;

	/*
	 * First look for exact matches or binary compatible matches. (Of
	 * course exact matches shouldn't even get here, but anyway.)
	 */
	ncandidates = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeid = current_candidate->args[0];

		if (current_typeid == typeid
			|| IS_BINARY_COMPATIBLE(current_typeid, typeid))
		{
			last_candidate = current_candidate;
			ncandidates++;
		}
	}
	if (ncandidates == 1)
		return last_candidate->args[0];

	/*
	 * If no luck that way, look for candidates which allow coercion and
	 * have a preferred type. Keep all candidates if none match.
	 */
	category = TypeCategory(typeid);
	ncandidates = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeid = current_candidate->args[0];
		current_category = TypeCategory(current_typeid);

		if (current_category == category
			&& IsPreferredType(current_category, current_typeid)
			&& can_coerce_type(1, &typeid, &current_typeid))
		{
			/* only one so far? then keep it... */
			if (last_candidate == NULL)
			{
				candidates = current_candidate;
				last_candidate = current_candidate;
				ncandidates = 1;
			}
			/* otherwise, keep this one too... */
			else
			{
				last_candidate->next = current_candidate;
				last_candidate = current_candidate;
				ncandidates++;
			}
		}
		/* otherwise, don't bother keeping this one around... */
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates->args[0];

	return InvalidOid;
}	/* agg_select_candidate() */


/*
 * parse function
 */
Node *
ParseFuncOrColumn(ParseState *pstate, char *funcname, List *fargs,
				  bool agg_star, bool agg_distinct,
				  int *curr_resno, int precedence)
{
	Oid			rettype = InvalidOid;
	Oid			argrelid = InvalidOid;
	Oid			funcid = InvalidOid;
	List	   *i = NIL;
	Node	   *first_arg = NULL;
	char	   *relname = NULL;
	char	   *refname = NULL;
	Relation	rd;
	Oid			relid;
	int			nargs = length(fargs);
	Func	   *funcnode;
	Oid			oid_array[FUNC_MAX_ARGS];
	Oid		   *true_oid_array;
	Node	   *retval;
	bool		retset;
	bool		must_be_agg = agg_star || agg_distinct;
	bool		could_be_agg;
	bool		attisset = false;
	Oid			toid = InvalidOid;
	Expr	   *expr;

	if (fargs)
	{
		first_arg = lfirst(fargs);
		if (first_arg == NULL)
			elog(ERROR, "Function '%s' does not allow NULL input", funcname);
	}

	/*
	 * check for projection methods: if function takes one argument, and
	 * that argument is a relation, param, or PQ function returning a
	 * complex * type, then the function could be a projection.
	 */
	/* We only have one parameter, and it's not got aggregate decoration */
	if (nargs == 1 && !must_be_agg)
	{
		/* Is it a plain Relation name from the parser? */
		if (IsA(first_arg, Ident) && ((Ident *) first_arg)->isRel)
		{
			Ident	   *ident = (Ident *) first_arg;
			RangeTblEntry *rte;
			AttrNumber	attnum;

			/*
			 * first arg is a relation. This could be a projection.
			 */
			refname = ident->name;

			rte = refnameRangeTableEntry(pstate, refname);
			if (rte == NULL)
			{
				rte = addRangeTableEntry(pstate, refname,
										 makeAttr(refname, NULL),
										 FALSE, FALSE, TRUE);
#ifdef WARN_FROM
				elog(NOTICE, "Adding missing FROM-clause entry%s for table %s",
				  pstate->parentParseState != NULL ? " in subquery" : "",
					 refname);
#endif
			}

			relname = rte->relname;
			relid = rte->relid;
			attnum = InvalidAttrNumber;

			/*
			 * If the attr isn't a set, just make a var for it.  If it is
			 * a set, treat it like a function and drop through. Look
			 * through the explicit column list first, since we now allow
			 * column aliases. - thomas 2000-02-07
			 */
			if (rte->eref->attrs != NULL)
			{
				List	   *c;

				/*
				 * start counting attributes/columns from one. zero is
				 * reserved for InvalidAttrNumber. - thomas 2000-01-27
				 */
				int			i = 1;

				foreach(c, rte->eref->attrs)
				{
					char	   *colname = strVal(lfirst(c));

					/* found a match? */
					if (strcmp(colname, funcname) == 0)
					{
						char	   *basename = get_attname(relid, i);

						if (basename != NULL)
						{
							funcname = basename;
							attnum = i;
						}

						/*
						 * attnum was initialized to InvalidAttrNumber
						 * earlier, so no need to reset it if the above
						 * test fails. - thomas 2000-02-07
						 */
						break;
					}
					i++;
				}
				if (attnum == InvalidAttrNumber)
					attnum = specialAttNum(funcname);
			}
			else
				attnum = get_attnum(relid, funcname);

			if (attnum != InvalidAttrNumber)
			{
				return (Node *) make_var(pstate,
										 relid,
										 refname,
										 funcname);
			}
			/* else drop through - attr is a set */
		}
		else if (ISCOMPLEX(exprType(first_arg)))
		{

			/*
			 * Attempt to handle projection of a complex argument. If
			 * ParseComplexProjection can't handle the projection, we have
			 * to keep going.
			 */
			retval = ParseComplexProjection(pstate,
											funcname,
											first_arg,
											&attisset);
			if (attisset)
			{
				toid = exprType(first_arg);
				rd = heap_openr(typeidTypeName(toid), NoLock);
				if (RelationIsValid(rd))
				{
					relname = RelationGetRelationName(rd);
					heap_close(rd, NoLock);
				}
				else
					elog(ERROR, "Type '%s' is not a relation type",
						 typeidTypeName(toid));
				argrelid = typeidTypeRelid(toid);

				/*
				 * A projection contains either an attribute name or "*".
				 */
				if ((get_attnum(argrelid, funcname) == InvalidAttrNumber)
					&& strcmp(funcname, "*"))
					elog(ERROR, "Functions on sets are not yet supported");
			}

			if (retval)
				return retval;
		}
	}

	/*
	 * See if it's an aggregate.
	 */
	if (must_be_agg)
	{
		/* We don't presently cope with, eg, foo(DISTINCT x,y) */
		if (nargs != 1)
			elog(ERROR, "Aggregate functions may only have one parameter");
		/* Agg's argument can't be a relation name, either */
		if (IsA(first_arg, Ident) && ((Ident *) first_arg)->isRel)
			elog(ERROR, "Aggregate functions cannot be applied to relation names");
		could_be_agg = true;
	}
	else
	{
		/* Try to parse as an aggregate if above-mentioned checks are OK */
		could_be_agg = (nargs == 1) &&
			!(IsA(first_arg, Ident) && ((Ident *) first_arg)->isRel);
	}

	if (could_be_agg)
	{
		Oid			basetype = exprType(lfirst(fargs));
		int			ncandidates;
		CandidateList candidates;

		/* try for exact match first... */
		if (SearchSysCacheTuple(AGGNAME,
								PointerGetDatum(funcname),
								ObjectIdGetDatum(basetype),
								0, 0))
			return (Node *) ParseAgg(pstate, funcname, basetype,
									 fargs, agg_star, agg_distinct,
									 precedence);

		/* check for aggregate-that-accepts-any-type (eg, COUNT) */
		if (SearchSysCacheTuple(AGGNAME,
								PointerGetDatum(funcname),
								ObjectIdGetDatum(0),
								0, 0))
			return (Node *) ParseAgg(pstate, funcname, 0,
									 fargs, agg_star, agg_distinct,
									 precedence);

		/*
		 * No exact match yet, so see if there is another entry in the
		 * aggregate table that is compatible. - thomas 1998-12-05
		 */
		ncandidates = agg_get_candidates(funcname, basetype, &candidates);
		if (ncandidates > 0)
		{
			Oid			type;

			type = agg_select_candidate(basetype, candidates);
			if (OidIsValid(type))
			{
				lfirst(fargs) = coerce_type(pstate, lfirst(fargs),
											basetype, type, -1);
				basetype = type;
				return (Node *) ParseAgg(pstate, funcname, basetype,
										 fargs, agg_star, agg_distinct,
										 precedence);
			}
			else
			{
				/* Multiple possible matches --- give up */
				elog(ERROR, "Unable to select an aggregate function %s(%s)",
					 funcname, typeidTypeName(basetype));
			}
		}

		if (must_be_agg)
		{

			/*
			 * No matching agg, but we had '*' or DISTINCT, so a plain
			 * function could not have been meant.
			 */
			elog(ERROR, "There is no aggregate function %s(%s)",
				 funcname, typeidTypeName(basetype));
		}
	}

	/*
	 * If we dropped through to here it's really a function (or a set,
	 * which is implemented as a function). Extract arg type info and
	 * transform relation name arguments into varnodes of the appropriate
	 * form.
	 */
	MemSet(oid_array, 0, FUNC_MAX_ARGS * sizeof(Oid));

	nargs = 0;
	foreach(i, fargs)
	{
		Node	   *arg = lfirst(i);

		if (IsA(arg, Ident) && ((Ident *) arg)->isRel)
		{
			RangeTblEntry *rte;
			int			vnum;
			int			sublevels_up;

			/*
			 * a relation
			 */
			refname = ((Ident *) arg)->name;

			rte = refnameRangeTableEntry(pstate, refname);
			if (rte == NULL)
			{
				rte = addRangeTableEntry(pstate, refname,
										 makeAttr(refname, NULL),
										 FALSE, FALSE, TRUE);
#ifdef WARN_FROM
				elog(NOTICE, "Adding missing FROM-clause entry%s for table %s",
				  pstate->parentParseState != NULL ? " in subquery" : "",
					 refname);
#endif
			}

			relname = rte->relname;

			vnum = refnameRangeTablePosn(pstate, rte->eref->relname,
										 &sublevels_up);

			/*
			 * for func(relname), the param to the function is the tuple
			 * under consideration.  we build a special VarNode to reflect
			 * this -- it has varno set to the correct range table entry,
			 * but has varattno == 0 to signal that the whole tuple is the
			 * argument.
			 */
			toid = typeTypeId(typenameType(relname));
			/* replace it in the arg list */
			lfirst(i) = makeVar(vnum, 0, toid, -1, sublevels_up);
		}
		else if (!attisset)
			toid = exprType(arg);
		else
		{
			/* if attisset is true, we already set toid for the single arg */
		}

		/*
		 * Most of the rest of the parser just assumes that functions do
		 * not have more than FUNC_MAX_ARGS parameters.  We have to test
		 * here to protect against array overruns, etc.
		 */
		if (nargs >= FUNC_MAX_ARGS)
			elog(ERROR, "Cannot pass more than %d arguments to a function",
				 FUNC_MAX_ARGS);

		oid_array[nargs++] = toid;
	}

	/*
	 * func_get_detail looks up the function in the catalogs, does
	 * disambiguation for polymorphic functions, handles inheritance, and
	 * returns the funcid and type and set or singleton status of the
	 * function's return value.  it also returns the true argument types
	 * to the function.  if func_get_detail returns true, the function
	 * exists.	otherwise, there was an error.
	 */
	if (attisset)
	{							/* we know all of these fields already */

		/*
		 * We create a funcnode with a placeholder function SetEval.
		 * SetEval() never actually gets executed.	When the function
		 * evaluation routines see it, they use the funcid projected out
		 * from the relation as the actual function to call. Example:
		 * retrieve (emp.mgr.name) The plan for this will scan the emp
		 * relation, projecting out the mgr attribute, which is a funcid.
		 * This function is then called (instead of SetEval) and "name" is
		 * projected from its result.
		 */
		funcid = F_SETEVAL;
		rettype = toid;
		retset = true;
		true_oid_array = oid_array;
	}
	else
	{
		bool		exists;

		exists = func_get_detail(funcname, nargs, oid_array, &funcid,
								 &rettype, &retset, &true_oid_array);
		if (!exists)
		{

			/*
			 * If we can't find a function (or can't find a unique
			 * function), see if this is really a type-coercion request:
			 * single-argument function call where the function name is a
			 * type name.  If so, and if we can do the coercion trivially,
			 * just go ahead and do it without requiring there to be a
			 * real function for it.
			 *
			 * "Trivial" coercions are ones that involve binary-compatible
			 * types and ones that are coercing a previously-unknown-type
			 * literal constant to a specific type.
			 *
			 * DO NOT try to generalize this code to nontrivial coercions,
			 * because you'll just set up an infinite recursion between
			 * this routine and coerce_type!  We have already failed to
			 * find a suitable "real" coercion function, so we have to
			 * fail unless this is a coercion that coerce_type can handle
			 * by itself. Make sure this code stays in sync with what
			 * coerce_type does!
			 */
			if (nargs == 1)
			{
				Type		tp;

				tp = SearchSysCacheTuple(TYPENAME,
										 PointerGetDatum(funcname),
										 0, 0, 0);
				if (HeapTupleIsValid(tp))
				{
					Oid			sourceType = oid_array[0];
					Oid			targetType = typeTypeId(tp);
					Node	   *arg1 = lfirst(fargs);

					if ((sourceType == UNKNOWNOID && IsA(arg1, Const)) ||
						sourceType == targetType ||
						IS_BINARY_COMPATIBLE(sourceType, targetType))
					{

						/*
						 * Ah-hah, we can do it as a trivial coercion.
						 * coerce_type can handle these cases, so why
						 * duplicate code...
						 */
						return coerce_type(pstate, arg1,
										   sourceType, targetType, -1);
					}
				}
			}

			/*
			 * Oops.  Time to die.
			 *
			 * If there is a single argument of complex type, we might be
			 * dealing with the PostQuel notation rel.function instead of
			 * the more usual function(rel).  Give a nonspecific error
			 * message that will cover both cases.
			 */
			if (nargs == 1)
			{
				Type		tp = typeidType(oid_array[0]);

				if (typeTypeFlag(tp) == 'c')
					elog(ERROR, "No such attribute or function '%s'",
						 funcname);
			}

			/* Else generate a detailed complaint */
			func_error(NULL, funcname, nargs, oid_array,
					   "Unable to identify a function that satisfies the "
					   "given argument types"
					   "\n\tYou may need to add explicit typecasts");
		}
	}

	/* got it */
	funcnode = makeNode(Func);
	funcnode->funcid = funcid;
	funcnode->functype = rettype;
	funcnode->funcisindex = false;
	funcnode->funcsize = 0;
	funcnode->func_fcache = NULL;
	funcnode->func_tlist = NIL;
	funcnode->func_planlist = NIL;

	/* perform the necessary typecasting */
	make_arguments(pstate, nargs, fargs, oid_array, true_oid_array);

	/*
	 * for functions returning base types, we want to project out the
	 * return value.  set up a target list to do that.	the executor will
	 * ignore these for c functions, and do the right thing for postquel
	 * functions.
	 */

	if (typeidTypeRelid(rettype) == InvalidOid)
		funcnode->func_tlist = setup_base_tlist(rettype);

	/*
	 * For sets, we want to make a targetlist to project out this
	 * attribute of the set tuples.
	 */
	if (attisset)
	{
		if (!strcmp(funcname, "*"))
			funcnode->func_tlist = expandAll(pstate, relname,
											 makeAttr(refname, NULL),
											 curr_resno);
		else
		{
			funcnode->func_tlist = setup_tlist(funcname, argrelid);
			rettype = get_atttype(argrelid, get_attnum(argrelid, funcname));
		}
	}

	/*
	 * Sequence handling.
	 */
	if (funcid == F_NEXTVAL ||
		funcid == F_CURRVAL ||
		funcid == F_SETVAL)
	{
		Const	   *seq;
		char	   *seqrel;
		text	   *seqname;
		int32		aclcheck_result = -1;

		Assert(nargs == ((funcid == F_SETVAL) ? 2 : 1));
		seq = (Const *) lfirst(fargs);
		if (!IsA((Node *) seq, Const))
			elog(ERROR, "Only constant sequence names are acceptable for function '%s'", funcname);

		seqrel = textout((text *) DatumGetPointer(seq->constvalue));
		/* Do we have nextval('"Aa"')? */
		if (strlen(seqrel) >= 2 &&
			seqrel[0] == '\"' && seqrel[strlen(seqrel) - 1] == '\"')
		{
			/* strip off quotes, keep case */
			seqrel = pstrdup(seqrel + 1);
			seqrel[strlen(seqrel) - 1] = '\0';
			pfree(DatumGetPointer(seq->constvalue));
			seq->constvalue = (Datum) textin(seqrel);
		}
		else
		{
			pfree(seqrel);
			seqname = lower((text *) DatumGetPointer(seq->constvalue));
			pfree(DatumGetPointer(seq->constvalue));
			seq->constvalue = PointerGetDatum(seqname);
			seqrel = textout(seqname);
		}

		if ((aclcheck_result = pg_aclcheck(seqrel, GetPgUserName(),
					   (((funcid == F_NEXTVAL) || (funcid == F_SETVAL)) ?
						ACL_WR : ACL_RD)))
			!= ACLCHECK_OK)
			elog(ERROR, "%s.%s: %s",
			  seqrel, funcname, aclcheck_error_strings[aclcheck_result]);

		pfree(seqrel);

		if (funcid == F_NEXTVAL && pstate->p_in_where_clause)
			elog(ERROR, "Sequence function nextval is not allowed in WHERE clauses");
		if (funcid == F_SETVAL && pstate->p_in_where_clause)
			elog(ERROR, "Sequence function setval is not allowed in WHERE clauses");
	}

	expr = makeNode(Expr);
	expr->typeOid = rettype;
	expr->opType = FUNC_EXPR;
	expr->oper = (Node *) funcnode;
	expr->args = fargs;
	retval = (Node *) expr;

	/*
	 * if the function returns a set of values, then we need to iterate
	 * over all the returned values in the executor, so we stick an iter
	 * node here.  if it returns a singleton, then we don't need the iter
	 * node.
	 */

	if (retset)
	{
		Iter	   *iter = makeNode(Iter);

		iter->itertype = rettype;
		iter->iterexpr = retval;
		retval = (Node *) iter;
	}

	return retval;
}


/* func_get_candidates()
 * get a list of all argument type vectors for which a function named
 * funcname taking nargs arguments exists
 */
static CandidateList
func_get_candidates(char *funcname, int nargs)
{
	Relation	heapRelation;
	Relation	idesc;
	ScanKeyData skey;
	HeapTupleData tuple;
	IndexScanDesc sd;
	RetrieveIndexResult indexRes;
	Form_pg_proc pgProcP;
	CandidateList candidates = NULL;
	CandidateList current_candidate;
	int			i;

	heapRelation = heap_openr(ProcedureRelationName, AccessShareLock);
	ScanKeyEntryInitialize(&skey,
						   (bits16) 0x0,
						   (AttrNumber) Anum_pg_proc_proname,
						   (RegProcedure) F_NAMEEQ,
						   (Datum) funcname);

	idesc = index_openr(ProcedureNameIndex);

	sd = index_beginscan(idesc, false, 1, &skey);

	do
	{
		indexRes = index_getnext(sd, ForwardScanDirection);
		if (indexRes)
		{
			Buffer		buffer;

			tuple.t_datamcxt = NULL;
			tuple.t_data = NULL;
			tuple.t_self = indexRes->heap_iptr;
			heap_fetch(heapRelation, SnapshotNow, &tuple, &buffer);
			pfree(indexRes);
			if (tuple.t_data != NULL)
			{
				pgProcP = (Form_pg_proc) GETSTRUCT(&tuple);
				if (pgProcP->pronargs == nargs)
				{
					current_candidate = (CandidateList)
						palloc(sizeof(struct _CandidateList));
					current_candidate->args = (Oid *)
						palloc(FUNC_MAX_ARGS * sizeof(Oid));
					MemSet(current_candidate->args, 0, FUNC_MAX_ARGS * sizeof(Oid));
					for (i = 0; i < nargs; i++)
						current_candidate->args[i] = pgProcP->proargtypes[i];

					current_candidate->next = candidates;
					candidates = current_candidate;
				}
				ReleaseBuffer(buffer);
			}
		}
	} while (indexRes);

	index_endscan(sd);
	index_close(idesc);
	heap_close(heapRelation, AccessShareLock);

	return candidates;
}


/* match_argtypes()
 * Given a list of possible typeid arrays to a function and an array of
 * input typeids, produce a shortlist of those function typeid arrays
 * that match the input typeids (either exactly or by coercion), and
 * return the number of such arrays
 */
static int
match_argtypes(int nargs,
			   Oid *input_typeids,
			   CandidateList function_typeids,
			   CandidateList *candidates)		/* return value */
{
	CandidateList current_candidate;
	CandidateList matching_candidate;
	Oid		   *current_typeids;
	int			ncandidates = 0;

	*candidates = NULL;

	for (current_candidate = function_typeids;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		if (can_coerce_type(nargs, input_typeids, current_typeids))
		{
			matching_candidate = (CandidateList)
				palloc(sizeof(struct _CandidateList));
			matching_candidate->args = current_typeids;
			matching_candidate->next = *candidates;
			*candidates = matching_candidate;
			ncandidates++;
		}
	}

	return ncandidates;
}	/* match_argtypes() */


/* func_select_candidate()
 * Given the input argtype array and more than one candidate
 * for the function argtype array, attempt to resolve the conflict.
 * Returns the selected argtype array if the conflict can be resolved,
 * otherwise returns NULL.
 *
 * By design, this is pretty similar to oper_select_candidate in parse_oper.c.
 * However, the calling convention is a little different: we assume the caller
 * already pruned away "candidates" that aren't actually coercion-compatible
 * with the input types, whereas oper_select_candidate must do that itself.
 */
static Oid *
func_select_candidate(int nargs,
					  Oid *input_typeids,
					  CandidateList candidates)
{
	CandidateList current_candidate;
	CandidateList last_candidate;
	Oid		   *current_typeids;
	int			i;
	int			ncandidates;
	int			nbestMatch,
				nmatch;
	CATEGORY	slot_category,
				current_category;
	Oid			slot_type,
				current_type;

	/*
	 * Run through all candidates and keep those with the most matches on
	 * exact types. Keep all candidates if none match.
	 */
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_typeids[i] != UNKNOWNOID &&
				current_typeids[i] == input_typeids[i])
				nmatch++;
		}

		/* take this one as the best choice so far? */
		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		/* no worse than the last choice, so keep this one too? */
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
		/* otherwise, don't bother keeping this one... */
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates->args;

	/*
	 * Still too many candidates? Run through all candidates and keep
	 * those with the most matches on exact types + binary-compatible
	 * types. Keep all candidates if none match.
	 */
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_typeids[i] != UNKNOWNOID)
			{
				if (current_typeids[i] == input_typeids[i] ||
					IS_BINARY_COMPATIBLE(current_typeids[i],
										 input_typeids[i]))
					nmatch++;
			}
		}

		/* take this one as the best choice so far? */
		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		/* no worse than the last choice, so keep this one too? */
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
		/* otherwise, don't bother keeping this one... */
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates->args;

	/*
	 * Still too many candidates? Now look for candidates which are
	 * preferred types at the args that will require coercion. Keep all
	 * candidates if none match.
	 */
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_typeids[i] != UNKNOWNOID)
			{
				current_category = TypeCategory(current_typeids[i]);
				if (current_typeids[i] == input_typeids[i] ||
					IsPreferredType(current_category, current_typeids[i]))
					nmatch++;
			}
		}

		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates->args;

	/*
	 * Still too many candidates? Try assigning types for the unknown
	 * columns.
	 *
	 * We do this by examining each unknown argument position to see if all
	 * the candidates agree on the type category of that slot.	If so, and
	 * if some candidates accept the preferred type in that category,
	 * eliminate the candidates with other input types.  If we are down to
	 * one candidate at the end, we win.
	 *
	 * XXX It's kinda bogus to do this left-to-right, isn't it?  If we
	 * eliminate some candidates because they are non-preferred at the
	 * first slot, we won't notice that they didn't have the same type
	 * category for a later slot.
	 */
	for (i = 0; i < nargs; i++)
	{
		if (input_typeids[i] == UNKNOWNOID)
		{
			slot_category = INVALID_TYPE;
			slot_type = InvalidOid;
			last_candidate = NULL;
			for (current_candidate = candidates;
				 current_candidate != NULL;
				 current_candidate = current_candidate->next)
			{
				current_typeids = current_candidate->args;
				current_type = current_typeids[i];
				current_category = TypeCategory(current_type);
				if (slot_category == INVALID_TYPE)
				{
					slot_category = current_category;
					slot_type = current_type;
					last_candidate = current_candidate;
				}
				else if (current_category != slot_category)
				{
					/* punt if more than one category for this slot */
					return NULL;
				}
				else if (current_type != slot_type)
				{
					if (IsPreferredType(slot_category, current_type))
					{
						slot_type = current_type;
						/* forget all previous candidates */
						candidates = current_candidate;
						last_candidate = current_candidate;
					}
					else if (IsPreferredType(slot_category, slot_type))
					{
						/* forget this candidate */
						if (last_candidate)
							last_candidate->next = current_candidate->next;
						else
							candidates = current_candidate->next;
					}
					else
						last_candidate = current_candidate;
				}
				else
				{
					/* keep this candidate */
					last_candidate = current_candidate;
				}
			}
			if (last_candidate) /* terminate rebuilt list */
				last_candidate->next = NULL;
		}
	}

	if (candidates == NULL)
		return NULL;			/* no remaining candidates */
	if (candidates->next != NULL)
		return NULL;			/* more than one remaining candidate */
	return candidates->args;
}	/* func_select_candidate() */


/* func_get_detail()
 * Find the named function in the system catalogs.
 *
 * Attempt to find the named function in the system catalogs with
 *	arguments exactly as specified, so that the normal case
 *	(exact match) is as quick as possible.
 *
 * If an exact match isn't found:
 *	1) get a vector of all possible input arg type arrays constructed
 *	   from the superclasses of the original input arg types
 *	2) get a list of all possible argument type arrays to the function
 *	   with given name and number of arguments
 *	3) for each input arg type array from vector #1:
 *	 a) find how many of the function arg type arrays from list #2
 *		it can be coerced to
 *	 b) if the answer is one, we have our function
 *	 c) if the answer is more than one, attempt to resolve the conflict
 *	 d) if the answer is zero, try the next array from vector #1
 */
static bool
func_get_detail(char *funcname,
				int nargs,
				Oid *oid_array,
				Oid *funcid,	/* return value */
				Oid *rettype,	/* return value */
				bool *retset,	/* return value */
				Oid **true_typeids)		/* return value */
{
	HeapTuple	ftup;

	/* attempt to find with arguments exactly as specified... */
	ftup = SearchSysCacheTuple(PROCNAME,
							   PointerGetDatum(funcname),
							   Int32GetDatum(nargs),
							   PointerGetDatum(oid_array),
							   0);

	if (HeapTupleIsValid(ftup))
	{
		/* given argument types are the right ones */
		*true_typeids = oid_array;
	}
	else
	{

		/*
		 * didn't find an exact match, so now try to match up
		 * candidates...
		 */
		CandidateList function_typeids;

		function_typeids = func_get_candidates(funcname, nargs);

		/* found something, so let's look through them... */
		if (function_typeids != NULL)
		{
			Oid		  **input_typeid_vector = NULL;
			Oid		   *current_input_typeids;

			/*
			 * First we will search with the given oid_array, then with
			 * variants based on replacing complex types with their
			 * inheritance ancestors.  Stop as soon as any match is found.
			 */
			current_input_typeids = oid_array;

			do
			{
				CandidateList current_function_typeids;
				int			ncandidates;

				ncandidates = match_argtypes(nargs, current_input_typeids,
											 function_typeids,
											 &current_function_typeids);

				/* one match only? then run with it... */
				if (ncandidates == 1)
				{
					*true_typeids = current_function_typeids->args;
					ftup = SearchSysCacheTuple(PROCNAME,
											   PointerGetDatum(funcname),
											   Int32GetDatum(nargs),
										  PointerGetDatum(*true_typeids),
											   0);
					Assert(HeapTupleIsValid(ftup));
					break;
				}

				/*
				 * multiple candidates? then better decide or throw an
				 * error...
				 */
				if (ncandidates > 1)
				{
					*true_typeids = func_select_candidate(nargs,
												   current_input_typeids,
											   current_function_typeids);

					if (*true_typeids != NULL)
					{
						/* was able to choose a best candidate */
						ftup = SearchSysCacheTuple(PROCNAME,
											   PointerGetDatum(funcname),
												   Int32GetDatum(nargs),
										  PointerGetDatum(*true_typeids),
												   0);
						Assert(HeapTupleIsValid(ftup));
					}

					/*
					 * otherwise, ambiguous function call, so fail by
					 * exiting loop with ftup still NULL.
					 */
					break;
				}

				/*
				 * No match here, so try the next inherited type vector.
				 * First time through, we need to compute the list of
				 * vectors.
				 */
				if (input_typeid_vector == NULL)
					input_typeid_vector = argtype_inherit(nargs, oid_array);

				current_input_typeids = *input_typeid_vector++;
			}
			while (current_input_typeids != NULL);
		}
	}

	if (HeapTupleIsValid(ftup))
	{
		Form_pg_proc pform = (Form_pg_proc) GETSTRUCT(ftup);

		*funcid = ftup->t_data->t_oid;
		*rettype = pform->prorettype;
		*retset = pform->proretset;
		return true;
	}
	return false;
}	/* func_get_detail() */

/*
 *	argtype_inherit() -- Construct an argtype vector reflecting the
 *						 inheritance properties of the supplied argv.
 *
 *		This function is used to disambiguate among functions with the
 *		same name but different signatures.  It takes an array of input
 *		type ids.  For each type id in the array that's a complex type
 *		(a class), it walks up the inheritance tree, finding all
 *		superclasses of that type.	A vector of new Oid type arrays
 *		is returned to the caller, reflecting the structure of the
 *		inheritance tree above the supplied arguments.
 *
 *		The order of this vector is as follows:  all superclasses of the
 *		rightmost complex class are explored first.  The exploration
 *		continues from right to left.  This policy means that we favor
 *		keeping the leftmost argument type as low in the inheritance tree
 *		as possible.  This is intentional; it is exactly what we need to
 *		do for method dispatch.  The last type array we return is all
 *		zeroes.  This will match any functions for which return types are
 *		not defined.  There are lots of these (mostly builtins) in the
 *		catalogs.
 */
static Oid **
argtype_inherit(int nargs, Oid *oid_array)
{
	Oid			relid;
	int			i;
	InhPaths	arginh[FUNC_MAX_ARGS];

	for (i = 0; i < FUNC_MAX_ARGS; i++)
	{
		if (i < nargs)
		{
			arginh[i].self = oid_array[i];
			if ((relid = typeidTypeRelid(oid_array[i])) != InvalidOid)
				arginh[i].nsupers = find_inheritors(relid, &(arginh[i].supervec));
			else
			{
				arginh[i].nsupers = 0;
				arginh[i].supervec = (Oid *) NULL;
			}
		}
		else
		{
			arginh[i].self = InvalidOid;
			arginh[i].nsupers = 0;
			arginh[i].supervec = (Oid *) NULL;
		}
	}

	/* return an ordered cross-product of the classes involved */
	return gen_cross_product(arginh, nargs);
}

static int
find_inheritors(Oid relid, Oid **supervec)
{
	Relation	inhrel;
	HeapScanDesc inhscan;
	ScanKeyData skey;
	HeapTuple	inhtup;
	Oid		   *relidvec;
	int			nvisited;
	List	   *visited,
			   *queue;
	List	   *elt;
	bool		newrelid;

	nvisited = 0;
	queue = NIL;
	visited = NIL;

	inhrel = heap_openr(InheritsRelationName, AccessShareLock);

	/*
	 * Use queue to do a breadth-first traversal of the inheritance graph
	 * from the relid supplied up to the root.	At the top of the loop,
	 * relid is the OID of the reltype to check next, queue is the list of
	 * pending rels to check after this one, and visited is the list of
	 * relids we need to output.
	 */
	do
	{
		/* find all types this relid inherits from, and add them to queue */

		ScanKeyEntryInitialize(&skey, 0x0, Anum_pg_inherits_inhrelid,
							   F_OIDEQ,
							   ObjectIdGetDatum(relid));

		inhscan = heap_beginscan(inhrel, 0, SnapshotNow, 1, &skey);

		while (HeapTupleIsValid(inhtup = heap_getnext(inhscan, 0)))
		{
			Form_pg_inherits inh = (Form_pg_inherits) GETSTRUCT(inhtup);

			queue = lappendi(queue, inh->inhparent);
		}

		heap_endscan(inhscan);

		/* pull next unvisited relid off the queue */

		newrelid = false;
		while (queue != NIL)
		{
			relid = lfirsti(queue);
			queue = lnext(queue);
			if (!intMember(relid, visited))
			{
				newrelid = true;
				break;
			}
		}

		if (newrelid)
		{
			visited = lappendi(visited, relid);
			nvisited++;
		}
	} while (newrelid);

	heap_close(inhrel, AccessShareLock);

	if (nvisited > 0)
	{
		relidvec = (Oid *) palloc(nvisited * sizeof(Oid));
		*supervec = relidvec;

		foreach(elt, visited)
		{
			/* return the type id, rather than the relation id */
			Relation	rd;
			Oid			trelid;

			relid = lfirsti(elt);
			rd = heap_open(relid, NoLock);
			if (!RelationIsValid(rd))
				elog(ERROR, "Relid %u does not exist", relid);
			trelid = typeTypeId(typenameType(RelationGetRelationName(rd)));
			heap_close(rd, NoLock);
			*relidvec++ = trelid;
		}
	}
	else
		*supervec = (Oid *) NULL;

	freeList(visited);

	/*
	 * there doesn't seem to be any equally easy way to release the queue
	 * list cells, but since they're palloc'd space it's not critical.
	 */

	return nvisited;
}

static Oid **
gen_cross_product(InhPaths *arginh, int nargs)
{
	int			nanswers;
	Oid		  **result,
			  **iter;
	Oid		   *oneres;
	int			i,
				j;
	int			cur[FUNC_MAX_ARGS];

	nanswers = 1;
	for (i = 0; i < nargs; i++)
	{
		nanswers *= (arginh[i].nsupers + 2);
		cur[i] = 0;
	}

	iter = result = (Oid **) palloc(sizeof(Oid *) * nanswers);

	/* compute the cross product from right to left */
	for (;;)
	{
		oneres = (Oid *) palloc(FUNC_MAX_ARGS * sizeof(Oid));
		MemSet(oneres, 0, FUNC_MAX_ARGS * sizeof(Oid));

		for (i = nargs - 1; i >= 0 && cur[i] > arginh[i].nsupers; i--)
			continue;

		/* if we're done, terminate with NULL pointer */
		if (i < 0)
		{
			*iter = NULL;
			return result;
		}

		/* no, increment this column and zero the ones after it */
		cur[i] = cur[i] + 1;
		for (j = nargs - 1; j > i; j--)
			cur[j] = 0;

		for (i = 0; i < nargs; i++)
		{
			if (cur[i] == 0)
				oneres[i] = arginh[i].self;
			else if (cur[i] > arginh[i].nsupers)
				oneres[i] = 0;	/* wild card */
			else
				oneres[i] = arginh[i].supervec[cur[i] - 1];
		}

		*iter++ = oneres;
	}
}


/*
 * Given two type OIDs, determine whether the first is a complex type
 * (class type) that inherits from the second.
 */
bool
typeInheritsFrom(Oid subclassTypeId, Oid superclassTypeId)
{
	Oid			relid;
	Oid		   *supervec;
	int			nsupers,
				i;
	bool		result;

	if (!ISCOMPLEX(subclassTypeId) || !ISCOMPLEX(superclassTypeId))
		return false;
	relid = typeidTypeRelid(subclassTypeId);
	if (relid == InvalidOid)
		return false;
	nsupers = find_inheritors(relid, &supervec);
	result = false;
	for (i = 0; i < nsupers; i++)
	{
		if (supervec[i] == superclassTypeId)
		{
			result = true;
			break;
		}
	}
	if (supervec)
		pfree(supervec);
	return result;
}


/* make_arguments()
 * Given the number and types of arguments to a function, and the
 *	actual arguments and argument types, do the necessary typecasting.
 *
 * There are two ways an input typeid can differ from a function typeid:
 *	1) the input type inherits the function type, so no typecasting required
 *	2) the input type can be typecast into the function type
 * Right now, we only typecast unknowns, and that is all we check for.
 *
 * func_get_detail() now can find coercions for function arguments which
 *	will make this function executable. So, we need to recover these
 *	results here too.
 * - thomas 1998-03-25
 */
static void
make_arguments(ParseState *pstate,
			   int nargs,
			   List *fargs,
			   Oid *input_typeids,
			   Oid *function_typeids)
{
	List	   *current_fargs;
	int			i;

	for (i = 0, current_fargs = fargs;
		 i < nargs;
		 i++, current_fargs = lnext(current_fargs))
	{
		/* types don't match? then force coercion using a function call... */
		if (input_typeids[i] != function_typeids[i])
		{
			lfirst(current_fargs) = coerce_type(pstate,
												lfirst(current_fargs),
												input_typeids[i],
												function_typeids[i], -1);
		}
	}
}

/*
 ** setup_tlist
 **		Build a tlist that says which attribute to project to.
 **		This routine is called by ParseFuncOrColumn() to set up a target list
 **		on a tuple parameter or return value.  Due to a bug in 4.0,
 **		it's not possible to refer to system attributes in this case.
 */
static List *
setup_tlist(char *attname, Oid relid)
{
	TargetEntry *tle;
	Resdom	   *resnode;
	Var		   *varnode;
	Oid			typeid;
	int32		type_mod;
	int			attno;

	attno = get_attnum(relid, attname);
	if (attno < 0)
		elog(ERROR, "Cannot reference attribute '%s'"
			 " of tuple params/return values for functions", attname);

	typeid = get_atttype(relid, attno);
	type_mod = get_atttypmod(relid, attno);

	resnode = makeResdom(1,
						 typeid,
						 type_mod,
						 get_attname(relid, attno),
						 0,
						 InvalidOid,
						 false);
	varnode = makeVar(-1, attno, typeid, type_mod, 0);

	tle = makeTargetEntry(resnode, (Node *) varnode);
	return lcons(tle, NIL);
}

/*
 ** setup_base_tlist
 **		Build a tlist that extracts a base type from the tuple
 **		returned by the executor.
 */
List *
setup_base_tlist(Oid typeid)
{
	TargetEntry *tle;
	Resdom	   *resnode;
	Var		   *varnode;

	resnode = makeResdom(1,
						 typeid,
						 -1,
						 "<noname>",
						 0,
						 InvalidOid,
						 false);
	varnode = makeVar(-1, 1, typeid, -1, 0);
	tle = makeTargetEntry(resnode, (Node *) varnode);

	return lcons(tle, NIL);
}

/*
 * ParseComplexProjection -
 *	  handles function calls with a single argument that is of complex type.
 *	  This routine returns NULL if it can't handle the projection (eg. sets).
 */
static Node *
ParseComplexProjection(ParseState *pstate,
					   char *funcname,
					   Node *first_arg,
					   bool *attisset)
{
	Oid			argtype;
	Oid			argrelid;
	Relation	rd;
	Oid			relid;
	int			attnum;

	switch (nodeTag(first_arg))
	{
		case T_Iter:
			{
				Func	   *func;
				Iter	   *iter;

				iter = (Iter *) first_arg;
				func = (Func *) ((Expr *) iter->iterexpr)->oper;
				argtype = get_func_rettype(func->funcid);
				argrelid = typeidTypeRelid(argtype);
				if (argrelid &&
					((attnum = get_attnum(argrelid, funcname))
					 != InvalidAttrNumber))
				{

					/*
					 * the argument is a function returning a tuple, so
					 * funcname may be a projection
					 */

					/* add a tlist to the func node and return the Iter */
					rd = heap_openr(typeidTypeName(argtype), NoLock);
					if (RelationIsValid(rd))
					{
						relid = RelationGetRelid(rd);
						func->func_tlist = setup_tlist(funcname, argrelid);
						iter->itertype = attnumTypeId(rd, attnum);
						heap_close(rd, NoLock);
						return (Node *) iter;
					}
					else
					{
						elog(ERROR, "Function '%s' has bad return type %d",
							 funcname, argtype);
					}
				}
				else
				{
					/* drop through */
					;
				}
				break;
			}
		case T_Var:
			{

				/*
				 * The argument is a set, so this is either a projection
				 * or a function call on this set.
				 */
				*attisset = true;
				break;
			}
		case T_Expr:
			{
				Expr	   *expr = (Expr *) first_arg;
				Func	   *funcnode;

				if (expr->opType != FUNC_EXPR)
					break;

				funcnode = (Func *) expr->oper;
				argtype = get_func_rettype(funcnode->funcid);
				argrelid = typeidTypeRelid(argtype);

				/*
				 * the argument is a function returning a tuple, so
				 * funcname may be a projection
				 */
				if (argrelid &&
					(attnum = get_attnum(argrelid, funcname))
					!= InvalidAttrNumber)
				{

					/* add a tlist to the func node */
					rd = heap_openr(typeidTypeName(argtype), NoLock);
					if (RelationIsValid(rd))
					{
						Expr	   *newexpr;

						relid = RelationGetRelid(rd);
						funcnode->func_tlist = setup_tlist(funcname, argrelid);
						funcnode->functype = attnumTypeId(rd, attnum);

						newexpr = makeNode(Expr);
						newexpr->typeOid = funcnode->functype;
						newexpr->opType = FUNC_EXPR;
						newexpr->oper = (Node *) funcnode;
						newexpr->args = expr->args;

						heap_close(rd, NoLock);

						return (Node *) newexpr;
					}
					/* XXX why not an error condition if it's not there? */

				}

				break;
			}
		case T_Param:
			{
				Param	   *param = (Param *) first_arg;

				/*
				 * If the Param is a complex type, this could be a
				 * projection
				 */
				rd = heap_openr(typeidTypeName(param->paramtype), NoLock);
				if (RelationIsValid(rd))
				{
					relid = RelationGetRelid(rd);
					if ((attnum = get_attnum(relid, funcname))
						!= InvalidAttrNumber)
					{
						param->paramtype = attnumTypeId(rd, attnum);
						param->param_tlist = setup_tlist(funcname, relid);
						heap_close(rd, NoLock);
						return (Node *) param;
					}
					heap_close(rd, NoLock);
				}
				break;
			}
		default:
			break;
	}

	return NULL;
}

/*
 * Error message when function lookup fails that gives details of the
 * argument types
 */
void
func_error(char *caller, char *funcname, int nargs, Oid *argtypes, char *msg)
{
	char		p[(NAMEDATALEN + 2) * FUNC_MAX_ARGS],
			   *ptr;
	int			i;

	ptr = p;
	*ptr = '\0';
	for (i = 0; i < nargs; i++)
	{
		if (i)
		{
			*ptr++ = ',';
			*ptr++ = ' ';
		}
		if (argtypes[i] != 0)
		{
			strcpy(ptr, typeidTypeName(argtypes[i]));
			*(ptr + NAMEDATALEN) = '\0';
		}
		else
			strcpy(ptr, "opaque");
		ptr += strlen(ptr);
	}

	if (caller == NULL)
	{
		elog(ERROR, "Function '%s(%s)' does not exist%s%s",
			 funcname, p,
			 ((msg != NULL) ? "\n\t" : ""), ((msg != NULL) ? msg : ""));
	}
	else
	{
		elog(ERROR, "%s: function '%s(%s)' does not exist%s%s",
			 caller, funcname, p,
			 ((msg != NULL) ? "\n\t" : ""), ((msg != NULL) ? msg : ""));
	}
}
