/*-------------------------------------------------------------------------
 *
 * creatinh.c--
 *	  POSTGRES create/destroy relation with inheritance utility code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/Attic/creatinh.c,v 1.29 1998-04-26 04:06:20 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>

#include <postgres.h>

#include <utils/rel.h>
#include <nodes/parsenodes.h>
#include <catalog/heap.h>
#include <commands/creatinh.h>
#include <access/xact.h>
#include <access/heapam.h>
#include <utils/syscache.h>
#include <catalog/catname.h>
#include <catalog/pg_type.h>
#include <catalog/pg_inherits.h>
#include <catalog/pg_ipl.h>

/* ----------------
 *		local stuff
 * ----------------
 */

static int
checkAttrExists(char *attributeName,
				char *attributeType, List *schema);
static List *MergeAttributes(List *schema, List *supers, List **supconstr);
static void StoreCatalogInheritance(Oid relationId, List *supers);

/* ----------------------------------------------------------------
 *		DefineRelation --
 *				Creates a new relation.
 * ----------------------------------------------------------------
 */
void
DefineRelation(CreateStmt *stmt)
{
	char	   *relname = palloc(NAMEDATALEN);
	List	   *schema = stmt->tableElts;
	int			numberOfAttributes;
	Oid			relationId;
	List	   *inheritList = NULL;
	TupleDesc	descriptor;
	List	   *constraints;

	if (strlen(stmt->relname) >= NAMEDATALEN)
		elog(ERROR, "the relation name %s is >= %d characters long", stmt->relname,
			 NAMEDATALEN);
	StrNCpy(relname, stmt->relname, NAMEDATALEN);		/* make full length for
														 * copy */

	/* ----------------
	 *	Handle parameters
	 *	XXX parameter handling missing below.
	 * ----------------
	 */
	inheritList = stmt->inhRelnames;

	/* ----------------
	 *	generate relation schema, including inherited attributes.
	 * ----------------
	 */
	schema = MergeAttributes(schema, inheritList, &constraints);
	constraints = nconc(constraints, stmt->constraints);

	numberOfAttributes = length(schema);
	if (numberOfAttributes <= 0)
	{
		elog(ERROR, "DefineRelation: %s",
			 "please inherit from a relation or define an attribute");
	}

	/* ----------------
	 *	create a relation descriptor from the relation schema
	 *	and create the relation.
	 * ----------------
	 */
	descriptor = BuildDescForRelation(schema, relname);

	if (constraints != NIL)
	{
		List	   *entry;
		int			nconstr = length(constraints);
		ConstrCheck *check = (ConstrCheck *) palloc(nconstr * sizeof(ConstrCheck));
		int			ncheck = 0;
		int			i;

		foreach(entry, constraints)
		{
			Constraint *cdef = (Constraint *) lfirst(entry);

			if (cdef->contype == CONSTR_CHECK)
			{
				if (cdef->name != NULL)
				{
					for (i = 0; i < ncheck; i++)
					{
						if (strcmp(check[i].ccname, cdef->name) == 0)
							elog(ERROR, "DefineRelation: name (%s) of CHECK constraint duplicated", cdef->name);
					}
					check[ncheck].ccname = cdef->name;
				}
				else
				{
					check[ncheck].ccname = (char *) palloc(NAMEDATALEN);
					sprintf(check[ncheck].ccname, "$%d", ncheck + 1);
				}
				check[ncheck].ccbin = NULL;
				check[ncheck].ccsrc = (char *) cdef->def;
				ncheck++;
			}
		}
		if (ncheck > 0)
		{
			if (ncheck < nconstr)
				check = (ConstrCheck *) repalloc(check, ncheck * sizeof(ConstrCheck));
			if (descriptor->constr == NULL)
			{
				descriptor->constr = (TupleConstr *) palloc(sizeof(TupleConstr));
				descriptor->constr->num_defval = 0;
				descriptor->constr->has_not_null = false;
			}
			descriptor->constr->num_check = ncheck;
			descriptor->constr->check = check;
		}
	}

	relationId = heap_create_with_catalog(relname, descriptor);

	StoreCatalogInheritance(relationId, inheritList);
}

/*
 * RemoveRelation --
 *		Deletes a new relation.
 *
 * Exceptions:
 *		BadArg if name is invalid.
 *
 * Note:
 *		If the relation has indices defined on it, then the index relations
 * themselves will be destroyed, too.
 */
void
RemoveRelation(char *name)
{
	AssertArg(name);
	heap_destroy_with_catalog(name);
}


/*
 * MergeAttributes --
 *		Returns new schema given initial schema and supers.
 *
 *
 * 'schema' is the column/attribute definition for the table. (It's a list
 *		of ColumnDef's.) It is destructively changed.
 * 'inheritList' is the list of inherited relations (a list of Value(str)'s).
 *
 * Notes:
 *	  The order in which the attributes are inherited is very important.
 *	  Intuitively, the inherited attributes should come first. If a table
 *	  inherits from multiple parents, the order of those attributes are
 *	  according to the order of the parents specified in CREATE TABLE.
 *
 *	  Here's an example:
 *
 *		create table person (name text, age int4, location point);
 *		create table emp (salary int4, manager text) inherits(person);
 *		create table student (gpa float8) inherits (person);
 *		create table stud_emp (percent int4) inherits (emp, student);
 *
 *	  the order of the attributes of stud_emp is as follow:
 *
 *
 *							person {1:name, 2:age, 3:location}
 *							/	 \
 *			   {6:gpa}	student   emp {4:salary, 5:manager}
 *							\	 /
 *						   stud_emp {7:percent}
 */
static List *
MergeAttributes(List *schema, List *supers, List **supconstr)
{
	List	   *entry;
	List	   *inhSchema = NIL;
	List	   *constraints = NIL;

	/*
	 * Validates that there are no duplications. Validity checking of
	 * types occurs later.
	 */
	foreach(entry, schema)
	{
		List	   *rest;
		ColumnDef  *coldef = lfirst(entry);

		foreach(rest, lnext(entry))
		{

			/*
			 * check for duplicated relation names
			 */
			ColumnDef  *restdef = lfirst(rest);

			if (!strcmp(coldef->colname, restdef->colname))
			{
				elog(ERROR, "attribute '%s' duplicated",
					 coldef->colname);
			}
		}
	}
	foreach(entry, supers)
	{
		List	   *rest;

		foreach(rest, lnext(entry))
		{
			if (!strcmp(strVal(lfirst(entry)), strVal(lfirst(rest))))
			{
				elog(ERROR, "relation '%s' duplicated",
					 strVal(lfirst(entry)));
			}
		}
	}

	/*
	 * merge the inherited attributes into the schema
	 */
	foreach(entry, supers)
	{
		char	   *name = strVal(lfirst(entry));
		Relation	relation;
		List	   *partialResult = NIL;
		AttrNumber	attrno;
		TupleDesc	tupleDesc;
		TupleConstr *constr;

		relation = heap_openr(name);
		if (relation == NULL)
		{
			elog(ERROR,
				 "MergeAttr: Can't inherit from non-existent superclass '%s'", name);
		}
		if (relation->rd_rel->relkind == 'S')
		{
			elog(ERROR, "MergeAttr: Can't inherit from sequence superclass '%s'", name);
		}
		tupleDesc = RelationGetTupleDescriptor(relation);
		constr = tupleDesc->constr;

		for (attrno = relation->rd_rel->relnatts - 1; attrno >= 0; attrno--)
		{
			AttributeTupleForm attribute = tupleDesc->attrs[attrno];
			char	   *attributeName;
			char	   *attributeType;
			HeapTuple	tuple;
			ColumnDef  *def;
			TypeName   *typename;

			/*
			 * form name, type and constraints
			 */
			attributeName = (attribute->attname).data;
			tuple =
				SearchSysCacheTuple(TYPOID,
									ObjectIdGetDatum(attribute->atttypid),
									0, 0, 0);
			AssertState(HeapTupleIsValid(tuple));
			attributeType =
				(((TypeTupleForm) GETSTRUCT(tuple))->typname).data;

			/*
			 * check validity
			 *
			 */
			if (checkAttrExists(attributeName, attributeType, inhSchema) ||
				checkAttrExists(attributeName, attributeType, schema))
			{

				/*
				 * this entry already exists
				 */
				continue;
			}

			/*
			 * add an entry to the schema
			 */
			def = makeNode(ColumnDef);
			typename = makeNode(TypeName);
			def->colname = pstrdup(attributeName);
			typename->name = pstrdup(attributeType);
			typename->typmod = attribute->atttypmod;
			def->typename = typename;
			def->is_not_null = attribute->attnotnull;
			def->defval = NULL;
			if (attribute->atthasdef)
			{
				AttrDefault *attrdef = constr->defval;
				int			i;

				Assert(constr != NULL && constr->num_defval > 0);

				for (i = 0; i < constr->num_defval; i++)
				{
					if (attrdef[i].adnum != attrno + 1)
						continue;
					def->defval = pstrdup(attrdef[i].adsrc);
					break;
				}
				Assert(def->defval != NULL);
			}
			partialResult = lcons(def, partialResult);
		}

		if (constr && constr->num_check > 0)
		{
			ConstrCheck *check = constr->check;
			int			i;

			for (i = 0; i < constr->num_check; i++)
			{
				Constraint *cdef = (Constraint *) makeNode(Constraint); /* palloc(sizeof(Constrai
																		 * nt)); */

				cdef->contype = CONSTR_CHECK;
				if (check[i].ccname[0] == '$')
					cdef->name = NULL;
				else
					cdef->name = pstrdup(check[i].ccname);
				cdef->def = (void *) pstrdup(check[i].ccsrc);
				constraints = lappend(constraints, cdef);
			}
		}

		/*
		 * iteration cleanup and result collection
		 */
		heap_close(relation);

		/*
		 * wants the inherited schema to appear in the order they are
		 * specified in CREATE TABLE
		 */
		inhSchema = nconc(inhSchema, partialResult);
	}

	/*
	 * put the inherited schema before our the schema for this table
	 */
	schema = nconc(inhSchema, schema);
	*supconstr = constraints;
	return (schema);
}

/*
 * StoreCatalogInheritance --
 *		Updates the system catalogs with proper inheritance information.
 */
static void
StoreCatalogInheritance(Oid relationId, List *supers)
{
	Relation	relation;
	TupleDesc	desc;
	int16		seqNumber;
	List	   *entry;
	List	   *idList;
	HeapTuple	tuple;

	/* ----------------
	 *	sanity checks
	 * ----------------
	 */
	AssertArg(OidIsValid(relationId));

	if (supers == NIL)
		return;

	/* ----------------
	 * Catalog INHERITS information.
	 * ----------------
	 */
	relation = heap_openr(InheritsRelationName);
	desc = RelationGetTupleDescriptor(relation);

	seqNumber = 1;
	idList = NIL;
	foreach(entry, supers)
	{
		Datum		datum[Natts_pg_inherits];
		char		nullarr[Natts_pg_inherits];

		tuple = SearchSysCacheTuple(RELNAME,
								  PointerGetDatum(strVal(lfirst(entry))),
									0, 0, 0);
		AssertArg(HeapTupleIsValid(tuple));

		/*
		 * build idList for use below
		 */
		idList = lappendi(idList, tuple->t_oid);

		datum[0] = ObjectIdGetDatum(relationId);		/* inhrel */
		datum[1] = ObjectIdGetDatum(tuple->t_oid);		/* inhparent */
		datum[2] = Int16GetDatum(seqNumber);	/* inhseqno */

		nullarr[0] = ' ';
		nullarr[1] = ' ';
		nullarr[2] = ' ';

		tuple = heap_formtuple(desc, datum, nullarr);

		heap_insert(relation, tuple);
		pfree(tuple);

		seqNumber += 1;
	}

	heap_close(relation);

	/* ----------------
	 * Catalog IPL information.
	 *
	 * Algorithm:
	 *	0. list superclasses (by Oid) in order given (see idList).
	 *	1. append after each relationId, its superclasses, recursively.
	 *	3. remove all but last of duplicates.
	 *	4. store result.
	 * ----------------
	 */

	/* ----------------
	 *	1.
	 * ----------------
	 */
	foreach(entry, idList)
	{
		HeapTuple	tuple;
		Oid			id;
		int16		number;
		List	   *next;
		List	   *current;

		id = (Oid) lfirsti(entry);
		current = entry;
		next = lnext(entry);

		for (number = 1;; number += 1)
		{
			tuple = SearchSysCacheTuple(INHRELID,
										ObjectIdGetDatum(id),
										Int16GetDatum(number),
										0, 0);

			if (!HeapTupleIsValid(tuple))
				break;

			lnext(current) =
				lconsi(((InheritsTupleForm)
						GETSTRUCT(tuple))->inhparent,
					   NIL);

			current = lnext(current);
		}
		lnext(current) = next;
	}

	/* ----------------
	 *	2.
	 * ----------------
	 */
	foreach(entry, idList)
	{
		Oid			name;
		List	   *rest;
		bool		found = false;

again:
		name = lfirsti(entry);
		foreach(rest, lnext(entry))
		{
			if (name == lfirsti(rest))
			{
				found = true;
				break;
			}
		}
		if (found)
		{

			/*
			 * entry list must be of length >= 2 or else no match
			 *
			 * so, remove this entry.
			 */
			lfirst(entry) = lfirst(lnext(entry));
			lnext(entry) = lnext(lnext(entry));

			found = false;
			goto again;
		}
	}

	/* ----------------
	 *	3.
	 * ----------------
	 */
	relation = heap_openr(InheritancePrecidenceListRelationName);
	desc = RelationGetTupleDescriptor(relation);

	seqNumber = 1;

	foreach(entry, idList)
	{
		Datum		datum[Natts_pg_ipl];
		char		nullarr[Natts_pg_ipl];

		datum[0] = ObjectIdGetDatum(relationId);		/* iplrel */
		datum[1] = ObjectIdGetDatum(lfirsti(entry));
		/* iplinherits */
		datum[2] = Int16GetDatum(seqNumber);	/* iplseqno */

		nullarr[0] = ' ';
		nullarr[1] = ' ';
		nullarr[2] = ' ';

		tuple = heap_formtuple(desc, datum, nullarr);

		heap_insert(relation, tuple);
		pfree(tuple);

		seqNumber += 1;
	}

	heap_close(relation);
}

/*
 * returns 1 if attribute already exists in schema, 0 otherwise.
 */
static int
checkAttrExists(char *attributeName, char *attributeType, List *schema)
{
	List	   *s;

	foreach(s, schema)
	{
		ColumnDef  *def = lfirst(s);

		if (!strcmp(attributeName, def->colname))
		{

			/*
			 * attribute exists. Make sure the types are the same.
			 */
			if (strcmp(attributeType, def->typename->name) != 0)
			{
				elog(ERROR, "%s and %s conflict for %s",
					 attributeType, def->typename->name, attributeName);
			}
			return 1;
		}
	}
	return 0;
}
