/*-------------------------------------------------------------------------
 *
 * heap.c--
 *    code to create and destroy POSTGRES heap relations
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/catalog/heap.c,v 1.8 1996-11-10 02:59:25 momjian Exp $
 *
 * INTERFACE ROUTINES
 *	heap_creatr()		- Create an uncataloged heap relation
 *	heap_create()		- Create a cataloged relation
 *	heap_destroy()		- Removes named relation from catalogs
 *
 * NOTES
 *    this code taken from access/heap/create.c, which contains
 *    the old heap_creater, amcreate, and amdestroy.  those routines
 *    will soon call these routines using the function manager,
 *    just like the poorly named "NewXXX" routines do.  The
 *    "New" routines are all going to die soon, once and for all!
 *	-cim 1/13/91
 *
 *-------------------------------------------------------------------------
 */
#include <postgres.h>

#include <miscadmin.h>
#include <fmgr.h>
#include <access/heapam.h>
#include <catalog/catalog.h>
#include <catalog/catname.h>
#include <catalog/heap.h>
#include <catalog/index.h>
#include <catalog/indexing.h>
#include <catalog/pg_ipl.h>
#include <catalog/pg_inherits.h>
#include <catalog/pg_proc.h>
#include <catalog/pg_index.h>
#include <catalog/pg_type.h>
#include <storage/bufmgr.h>
#include <storage/lmgr.h>
#include <storage/smgr.h>
#include <parser/catalog_utils.h>
#include <rewrite/rewriteRemove.h>
#include <utils/builtins.h>
#include <utils/mcxt.h>
#include <utils/relcache.h>
#ifndef HAVE_MEMMOVE
# include <regex/utils.h>
#else
# include <string.h>
#endif

/* ----------------------------------------------------------------
 *		XXX UGLY HARD CODED BADNESS FOLLOWS XXX
 *
 *	these should all be moved to someplace in the lib/catalog
 *	module, if not obliterated first.
 * ----------------------------------------------------------------
 */


/*
 * Note:
 *	Should the executor special case these attributes in the future?
 *	Advantage:  consume 1/2 the space in the ATTRIBUTE relation.
 *	Disadvantage:  having rules to compute values in these tuples may
 *		be more difficult if not impossible.
 */

static	FormData_pg_attribute a1 = {
    0xffffffff, {"ctid"}, 27l, 0l, 0l, 0l, sizeof (ItemPointerData),
    SelfItemPointerAttributeNumber, 0, '\0', '\001', 0l, 'i'
};

static	FormData_pg_attribute a2 = {
    0xffffffff, {"oid"}, 26l, 0l, 0l, 0l, sizeof(Oid),
    ObjectIdAttributeNumber, 0, '\001', '\001', 0l, 'i'
};

static	FormData_pg_attribute a3 = {
    0xffffffff, {"xmin"}, 28l, 0l, 0l, 0l, sizeof (TransactionId),
    MinTransactionIdAttributeNumber, 0, '\0', '\001', 0l, 'i',
};

static	FormData_pg_attribute a4 = {
    0xffffffff, {"cmin"}, 29l, 0l, 0l, 0l, sizeof (CommandId),
    MinCommandIdAttributeNumber, 0, '\001', '\001', 0l, 's'
};

static	FormData_pg_attribute a5 = {
    0xffffffff, {"xmax"}, 28l, 0l, 0l, 0l, sizeof (TransactionId),
    MaxTransactionIdAttributeNumber, 0, '\0', '\001', 0l, 'i'
};

static	FormData_pg_attribute a6 = {
    0xffffffff, {"cmax"}, 29l, 0l, 0l, 0l, sizeof (CommandId),
    MaxCommandIdAttributeNumber, 0, '\001', '\001', 0l, 's'
};

static	FormData_pg_attribute a7 = {
    0xffffffff, {"chain"}, 27l, 0l, 0l, 0l, sizeof (ItemPointerData),
    ChainItemPointerAttributeNumber, 0, '\0', '\001', 0l, 'i',
};

static	FormData_pg_attribute a8 = {
    0xffffffff, {"anchor"}, 27l, 0l, 0l, 0l, sizeof (ItemPointerData),
    AnchorItemPointerAttributeNumber, 0, '\0', '\001', 0l, 'i'
};

static	FormData_pg_attribute a9 = {
    0xffffffff, {"tmin"}, 20l, 0l, 0l, 0l, sizeof (AbsoluteTime),
    MinAbsoluteTimeAttributeNumber, 0, '\001', '\001', 0l, 'i'
};

static	FormData_pg_attribute a10 = {
    0xffffffff, {"tmax"}, 20l, 0l, 0l, 0l, sizeof (AbsoluteTime),
    MaxAbsoluteTimeAttributeNumber, 0, '\001', '\001', 0l, 'i'
};

static	FormData_pg_attribute a11 = {
    0xffffffff, {"vtype"}, 18l, 0l, 0l, 0l, sizeof (char),
    VersionTypeAttributeNumber, 0, '\001', '\001', 0l, 'c'
};

static	AttributeTupleForm HeapAtt[] =
{ &a1, &a2, &a3, &a4, &a5, &a6, &a7, &a8, &a9, &a10, &a11 };

/* ----------------------------------------------------------------
 *		XXX END OF UGLY HARD CODED BADNESS XXX
 * ----------------------------------------------------------------
 */

/* the tempRelList holds
   the list of temporary uncatalogued relations that are created.
   these relations should be destroyed at the end of transactions
*/
typedef struct tempRelList {
    Relation *rels; /* array of relation descriptors */
    int  num; /* number of temporary relations */
    int size; /* size of space allocated for the rels array */
} TempRelList;

#define TEMP_REL_LIST_SIZE  32

static TempRelList *tempRels = NULL;   


/* ----------------------------------------------------------------
 *	heap_creatr	- Create an uncataloged heap relation
 *
 *	Fields relpages, reltuples, reltuples, relkeys, relhistory,
 *	relisindexed, and relkind of rdesc->rd_rel are initialized
 *	to all zeros, as are rd_last and rd_hook.  Rd_refcnt is set to 1.
 *
 *	Remove the system relation specific code to elsewhere eventually.
 *
 *	Eventually, must place information about this temporary relation
 *	into the transaction context block.
 *
 *  
 * if heap_creatr is called with "" as the name, then heap_creatr will create a
 * temporary name   "temp_$RELOID" for the relation
 * ----------------------------------------------------------------
 */
Relation
heap_creatr(char *name, 
	    unsigned smgr,
	    TupleDesc tupDesc) 
{
    register unsigned	i;
    Oid		relid;
    Relation		rdesc;
    int			len;
    bool		nailme = false;
    char*               relname = name;
    char                tempname[40];
    int isTemp = 0;
    int natts = tupDesc->natts;
/*    AttributeTupleForm *att = tupDesc->attrs; */
    
    extern GlobalMemory	CacheCxt;
    MemoryContext	oldcxt;

    /* ----------------
     *	sanity checks
     * ----------------
     */
    AssertArg(natts > 0);

    if (IsSystemRelationName(relname) && IsNormalProcessingMode())
	{
	    elog(WARN, 
		 "Illegal class name: %s -- pg_ is reserved for system catalogs",
		 relname);
	}
    
    /* ----------------
     *	switch to the cache context so that we don't lose
     *  allocations at the end of this transaction, I guess.
     *  -cim 6/14/90
     * ----------------
     */
    if (!CacheCxt)
	CacheCxt = CreateGlobalMemory("Cache");
    
    oldcxt = MemoryContextSwitchTo((MemoryContext)CacheCxt);
    
    /* ----------------
     *	real ugly stuff to assign the proper relid in the relation
     *  descriptor follows.
     * ----------------
     */
    if (! strcmp(RelationRelationName,relname))
	{
	    relid = RelOid_pg_class;
	    nailme = true;
	}
    else if (! strcmp(AttributeRelationName,relname))
	{
	    relid = RelOid_pg_attribute;
	    nailme = true;
	}
    else if (! strcmp(ProcedureRelationName, relname))
	{
	    relid = RelOid_pg_proc;
	    nailme = true;
	}
    else if (! strcmp(TypeRelationName,relname))
	{
	    relid = RelOid_pg_type;
	    nailme = true;
	}
    else
      {
	relid = newoid();
    
	if (name[0] == '\0')
	  {
	    sprintf(tempname, "temp_%d", relid);
	    relname = tempname;
	    isTemp = 1;
	  };
      }

    /* ----------------
     *	allocate a new relation descriptor.
     *
     * 	XXX the length computation may be incorrect, handle elsewhere
     * ----------------
     */
    len = sizeof(RelationData);
  
    rdesc = (Relation) palloc(len);
    memset((char *)rdesc, 0,len);
    
    /* ----------
       create a new tuple descriptor from the one passed in
    */
    rdesc->rd_att = CreateTupleDescCopy(tupDesc);
    
    /* ----------------
     *	initialize the fields of our new relation descriptor
     * ----------------
     */
    
    /* ----------------
     *  nail the reldesc if this is a bootstrap create reln and
     *  we may need it in the cache later on in the bootstrap
     *  process so we don't ever want it kicked out.  e.g. pg_attribute!!!
     * ----------------
     */
    if (nailme)
	rdesc->rd_isnailed = true;
    
    RelationSetReferenceCount(rdesc, 1);
    
    rdesc->rd_rel = (Form_pg_class)palloc(sizeof *rdesc->rd_rel);
    
    memset((char *)rdesc->rd_rel, 0,
	   sizeof *rdesc->rd_rel);
    namestrcpy(&(rdesc->rd_rel->relname), relname); 
    rdesc->rd_rel->relkind = RELKIND_UNCATALOGED;
    rdesc->rd_rel->relnatts = natts;
    rdesc->rd_rel->relsmgr = smgr;
    
    for (i = 0; i < natts; i++) {
	rdesc->rd_att->attrs[i]->attrelid = relid;
    }
    
    rdesc->rd_id = relid;
    
     if (nailme) {
 	/* for system relations, set the reltype field here */
 	rdesc->rd_rel->reltype = relid;
     }

    /* ----------------
     *  remember if this is a temp relation
     * ----------------
     */

    rdesc->rd_istemp = isTemp;

    /* ----------------
     *	have the storage manager create the relation.
     * ----------------
     */
    
    rdesc->rd_fd = (File)smgrcreate(smgr, rdesc);
    
    RelationRegisterRelation(rdesc);
    
    MemoryContextSwitchTo(oldcxt);
    
    /* add all temporary relations to the tempRels list
       so they can be properly disposed of at the end of transaction
    */
    if (isTemp)
	AddToTempRelList(rdesc);

    return (rdesc);
}


/* ----------------------------------------------------------------
 *	heap_create	- Create a cataloged relation
 *
 *	this is done in 6 steps:
 *
 *	1) CheckAttributeNames() is used to make certain the tuple
 *	   descriptor contains a valid set of attribute names
 *
 *	2) pg_class is opened and RelationAlreadyExists()
 *	   preforms a scan to ensure that no relation with the
 *         same name already exists.
 *
 *	3) heap_creater() is called to create the new relation on
 *	   disk.
 *
 *	4) TypeDefine() is called to define a new type corresponding
 *	   to the new relation.
 *
 *	5) AddNewAttributeTuples() is called to register the
 *	   new relation's schema in pg_attribute.
 *
 *	6) AddPgRelationTuple() is called to register the
 *	   relation itself in the catalogs.
 *
 *	7) the relations are closed and the new relation's oid
 *	   is returned.
 *
 * old comments:
 *	A new relation is inserted into the RELATION relation
 *	with the specified attribute(s) (newly inserted into
 *	the ATTRIBUTE relation).  How does concurrency control
 *	work?  Is it automatic now?  Expects the caller to have
 *	attname, atttypid, atttyparg, attproc, and attlen domains filled.
 *	Create fills the attnum domains sequentually from zero,
 *	fills the attnvals domains with zeros, and fills the
 *	attrelid fields with the relid.
 *
 *	scan relation catalog for name conflict
 *	scan type catalog for typids (if not arg)
 *	create and insert attribute(s) into attribute catalog
 *	create new relation
 *	insert new relation into attribute catalog
 *
 *	Should coordinate with heap_creater().  Either it should
 *	not be called or there should be a way to prevent
 *	the relation from being removed at the end of the
 *	transaction if it is successful ('u'/'r' may be enough).
 *	Also, if the transaction does not commit, then the
 *	relation should be removed.
 *
 *	XXX amcreate ignores "off" when inserting (for now).
 *	XXX amcreate (like the other utilities) needs to understand indexes.
 *	
 * ----------------------------------------------------------------
 */

/* --------------------------------
 *	CheckAttributeNames
 *
 *	this is used to make certain the tuple descriptor contains a
 *	valid set of attribute names.  a problem simply generates
 *	elog(WARN) which aborts the current transaction.
 * --------------------------------
 */
static void
CheckAttributeNames(TupleDesc tupdesc)
{
    unsigned	i;
    unsigned	j;
    int natts = tupdesc->natts;

    /* ----------------
     *	first check for collision with system attribute names
     * ----------------
     *
     *   also, warn user if attribute to be created has
     *   an unknown typid  (usually as a result of a 'retrieve into'
     *    - jolly
     */
    for (i = 0; i < natts; i += 1) {
	for (j = 0; j < sizeof HeapAtt / sizeof HeapAtt[0]; j += 1) {
	    if (nameeq(&(HeapAtt[j]->attname),
			    &(tupdesc->attrs[i]->attname))) {
		elog(WARN,
		     "create: system attribute named \"%s\"",
		     HeapAtt[j]->attname.data);
	    }
	}
	if (tupdesc->attrs[i]->atttypid == UNKNOWNOID)
	    {
		elog(NOTICE,
		     "create: attribute named \"%s\" has an unknown type",
		      tupdesc->attrs[i]->attname.data);
	    }
    }
    
    /* ----------------
     *	next check for repeated attribute names
     * ----------------
     */
    for (i = 1; i < natts; i += 1) {
	for (j = 0; j < i; j += 1) {
	    if (nameeq(&(tupdesc->attrs[j]->attname),
			    &(tupdesc->attrs[i]->attname))) {
		elog(WARN,
		     "create: repeated attribute \"%s\"",
		     tupdesc->attrs[j]->attname.data);
	    }
	}
    }
}

/* --------------------------------
 *	RelationAlreadyExists
 *
 *	this preforms a scan of pg_class to ensure that
 *	no relation with the same name already exists.  The caller
 *	has to open pg_class and pass an open descriptor.
 * --------------------------------
 */
int
RelationAlreadyExists(Relation pg_class_desc, char relname[])
{
    ScanKeyData	        key;
    HeapScanDesc	pg_class_scan;
    HeapTuple		tup;
    
    /*
     *  If this is not bootstrap (initdb) time, use the catalog index
     *  on pg_class.
     */
    
    if (!IsBootstrapProcessingMode()) {
	tup = ClassNameIndexScan(pg_class_desc, relname);
	if (HeapTupleIsValid(tup)) {
	    pfree(tup);
	    return ((int) true);
	} else
	    return ((int) false);
    }
    
    /* ----------------
     *  At bootstrap time, we have to do this the hard way.  Form the
     *	scan key.
     * ----------------
     */
    ScanKeyEntryInitialize(&key,
			   0,
			   (AttrNumber)Anum_pg_class_relname,
			   (RegProcedure)NameEqualRegProcedure,
			   (Datum) relname);
    
    /* ----------------
     *	begin the scan
     * ----------------
     */
    pg_class_scan = heap_beginscan(pg_class_desc,
				      0,
				      NowTimeQual,
				      1,
				      &key);
    
    /* ----------------
     *	get a tuple.  if the tuple is NULL then it means we
     *  didn't find an existing relation.
     * ----------------
     */
    tup = heap_getnext(pg_class_scan, 0, (Buffer *)NULL);
    
    /* ----------------
     *	end the scan and return existance of relation.
     * ----------------
     */
    heap_endscan(pg_class_scan);
    
    return
	(PointerIsValid(tup) == true);
}

/* --------------------------------
 *	AddNewAttributeTuples
 *
 *	this registers the new relation's schema by adding
 *	tuples to pg_attribute.
 * --------------------------------
 */
static void
AddNewAttributeTuples(Oid new_rel_oid,
		      TupleDesc tupdesc)
{
    AttributeTupleForm *dpp;		
    unsigned 	i;
    HeapTuple	tup;
    Relation	rdesc;
    bool	hasindex;
    Relation	idescs[Num_pg_attr_indices];
    int natts = tupdesc->natts;
    
    /* ----------------
     *	open pg_attribute
     * ----------------
     */
    rdesc = heap_openr(AttributeRelationName);
    
    /* -----------------
     * Check if we have any indices defined on pg_attribute.
     * -----------------
     */
    Assert(rdesc);
    Assert(rdesc->rd_rel);
    hasindex = RelationGetRelationTupleForm(rdesc)->relhasindex;
    if (hasindex)
	CatalogOpenIndices(Num_pg_attr_indices, Name_pg_attr_indices, idescs);
    
    /* ----------------
     *	initialize tuple descriptor.  Note we use setheapoverride()
     *  so that we can see the effects of our TypeDefine() done
     *  previously.
     * ----------------
     */
    setheapoverride(true);
    fillatt(tupdesc);
    setheapoverride(false);
    
    /* ----------------
     *  first we add the user attributes..
     * ----------------
     */
    dpp = tupdesc->attrs;
    for (i = 0; i < natts; i++) {
	(*dpp)->attrelid = new_rel_oid;
	(*dpp)->attnvals = 0l;
	
	tup = heap_addheader(Natts_pg_attribute,
			     ATTRIBUTE_TUPLE_SIZE,
			     (char *) *dpp);
	
	heap_insert(rdesc, tup);
	
	if (hasindex)
	    CatalogIndexInsert(idescs, Num_pg_attr_indices, rdesc, tup);
	
	pfree(tup);
	dpp++;
    }
    
    /* ----------------
     *	next we add the system attributes..
     * ----------------
     */
    dpp = HeapAtt;
    for (i = 0; i < -1 - FirstLowInvalidHeapAttributeNumber; i++) {
	(*dpp)->attrelid = new_rel_oid;
	/*	(*dpp)->attnvals = 0l;	unneeded */
	
	tup = heap_addheader(Natts_pg_attribute,
			     ATTRIBUTE_TUPLE_SIZE,
			     (char *)*dpp);
	
	heap_insert(rdesc, tup);
	
	if (hasindex)
	    CatalogIndexInsert(idescs, Num_pg_attr_indices, rdesc, tup);
	
	pfree(tup);
	dpp++;
    }
    
    heap_close(rdesc);

    /*
     * close pg_attribute indices
     */
    if (hasindex)
	CatalogCloseIndices(Num_pg_attr_indices, idescs);
}

/* --------------------------------
 *	AddPgRelationTuple
 *
 *	this registers the new relation in the catalogs by
 *	adding a tuple to pg_class.
 * --------------------------------
 */
void
AddPgRelationTuple(Relation pg_class_desc,
		   Relation new_rel_desc,
		   Oid new_rel_oid,
		   int arch,
		   unsigned natts)
{
    Form_pg_class	new_rel_reltup;
    HeapTuple		tup;
    Relation		idescs[Num_pg_class_indices];
    bool		isBootstrap;
    
    /* ----------------
     *	first we munge some of the information in our
     *  uncataloged relation's relation descriptor.
     * ----------------
     */
    new_rel_reltup = new_rel_desc->rd_rel;
    
    /* CHECK should get new_rel_oid first via an insert then use XXX */
    /*   new_rel_reltup->reltuples = 1; */ /* XXX */
    
    new_rel_reltup->relowner = GetUserId();
    new_rel_reltup->relkind = RELKIND_RELATION;
    new_rel_reltup->relarch = arch;
    new_rel_reltup->relnatts = natts;
    
    /* ----------------
     *	now form a tuple to add to pg_class
     *  XXX Natts_pg_class_fixed is a hack - see pg_class.h
     * ----------------
     */
    tup = heap_addheader(Natts_pg_class_fixed,
			 CLASS_TUPLE_SIZE,
			 (char *) new_rel_reltup);
    tup->t_oid = new_rel_oid;
    
    /* ----------------
     *  finally insert the new tuple and free it.
     *
     *  Note: I have no idea why we do a
     *		SetProcessingMode(BootstrapProcessing);
     *        here -cim 6/14/90
     * ----------------
     */
    isBootstrap = IsBootstrapProcessingMode() ? true : false;
    
    SetProcessingMode(BootstrapProcessing);
    
    heap_insert(pg_class_desc, tup);
    
    if (! isBootstrap) {
	/*
	 *  First, open the catalog indices and insert index tuples for
	 *  the new relation.
	 */
	
	CatalogOpenIndices(Num_pg_class_indices, Name_pg_class_indices, idescs);
	CatalogIndexInsert(idescs, Num_pg_class_indices, pg_class_desc, tup);
	CatalogCloseIndices(Num_pg_class_indices, idescs);
	
	/* now restore processing mode */
	SetProcessingMode(NormalProcessing);
    }
    
    pfree(tup);
}


/* --------------------------------
 *	addNewRelationType -
 *
 *	define a complex type corresponding to the new relation
 * --------------------------------
 */
void
addNewRelationType(char *typeName, Oid new_rel_oid)
{
    Oid 		new_type_oid;

    /* The sizes are set to oid size because it makes implementing sets MUCH
     * easier, and no one (we hope) uses these fields to figure out
     * how much space to allocate for the type. 
     * An oid is the type used for a set definition.  When a user
     * requests a set, what they actually get is the oid of a tuple in
     * the pg_proc catalog, so the size of the "set" is the size
     * of an oid.
     * Similarly, byval being true makes sets much easier, and 
     * it isn't used by anything else.
     * Note the assumption that OIDs are the same size as int4s.
     */
    new_type_oid = TypeCreate(typeName,			/* type name */
			      new_rel_oid,      	/* relation oid */
			      tlen(type("oid")),	/* internal size */
			      tlen(type("oid")),	/* external size */
			      'c', 		/* type-type (catalog) */
			      ',',		/* default array delimiter */
			      "int4in",	/* input procedure */
			      "int4out",	/* output procedure */
			      "int4in",  /* send procedure */
			      "int4out",	/* receive procedure */
			      NULL,     /* array element type - irrelevent */
			      "-",		/* default type value */
			      (bool) 1,	/* passed by value */
			      'i');	/* default alignment */
}

/* --------------------------------
 *	heap_create	
 *
 *	creates a new cataloged relation.  see comments above.
 * --------------------------------
 */
Oid
heap_create(char relname[],
	    char *typename, /* not used currently */
	    int arch,
	    unsigned smgr,
	    TupleDesc tupdesc)
{
    Relation		pg_class_desc;
    Relation		new_rel_desc;
    Oid			new_rel_oid;
/*    NameData            typeNameData; */
    int natts = tupdesc->natts;

    /* ----------------
     *	sanity checks
     * ----------------
     */
    AssertState(IsNormalProcessingMode() || IsBootstrapProcessingMode());
    if (natts == 0 || natts > MaxHeapAttributeNumber)
	elog(WARN, "amcreate: from 1 to %d attributes must be specified",
	     MaxHeapAttributeNumber);
    
    CheckAttributeNames(tupdesc);
    
    /* ----------------
     *	open pg_class and see that the relation doesn't
     *  already exist.
     * ----------------
     */
    pg_class_desc = heap_openr(RelationRelationName);
    
    if (RelationAlreadyExists(pg_class_desc, relname)) {
	heap_close(pg_class_desc);
	elog(WARN, "amcreate: %s relation already exists", relname);
    }
    
    /* ----------------
     *  ok, relation does not already exist so now we
     *	create an uncataloged relation and pull its relation oid
     *  from the newly formed relation descriptor.
     *
     *  Note: The call to heap_creatr() does all the "real" work
     *  of creating the disk file for the relation.
     * ----------------
     */
    new_rel_desc = heap_creatr(relname, smgr, tupdesc);
    new_rel_oid  = new_rel_desc->rd_att->attrs[0]->attrelid;
    
    /* ----------------
     *  since defining a relation also defines a complex type,
     *	we add a new system type corresponding to the new relation.
     * ----------------
     */
/*    namestrcpy(&typeNameData, relname);*/
/*    addNewRelationType(&typeNameData, new_rel_oid);*/
    addNewRelationType(relname, new_rel_oid);

    /* ----------------
     *	now add tuples to pg_attribute for the attributes in
     *  our new relation.
     * ----------------
     */
    AddNewAttributeTuples(new_rel_oid, tupdesc);
    
    /* ----------------
     *	now update the information in pg_class.
     * ----------------
     */
    AddPgRelationTuple(pg_class_desc,
		       new_rel_desc,
		       new_rel_oid,
		       arch,
		       natts);
    
    /* ----------------
     *	ok, the relation has been cataloged, so close our relations
     *  and return the oid of the newly created relation.
     *
     *	SOMEDAY: fill the STATISTIC relation properly.
     * ----------------
     */
    heap_close(new_rel_desc);
    heap_close(pg_class_desc);
    
    return new_rel_oid;
}


/* ----------------------------------------------------------------
 *	heap_destroy	- removes all record of named relation from catalogs
 *
 *	1)  open relation, check for existence, etc.
 *	2)  remove inheritance information
 *	3)  remove indexes
 *	4)  remove pg_class tuple
 *	5)  remove pg_attribute tuples
 *	6)  remove pg_type tuples
 *	7)  unlink relation
 *
 * old comments
 *	Except for vital relations, removes relation from
 *	relation catalog, and related attributes from
 *	attribute catalog (needed?).  (Anything else?)
 *
 *	get proper relation from relation catalog (if not arg)
 *	check if relation is vital (strcmp()/reltype?)
 *	scan attribute catalog deleting attributes of reldesc
 *		(necessary?)
 *	delete relation from relation catalog
 *	(How are the tuples of the relation discarded?)
 *
 *	XXX Must fix to work with indexes.
 *	There may be a better order for doing things.
 *	Problems with destroying a deleted database--cannot create
 *	a struct reldesc without having an open file descriptor.
 * ----------------------------------------------------------------
 */

/* --------------------------------
 *	RelationRemoveInheritance
 *
 * 	Note: for now, we cause an exception if relation is a
 *	superclass.  Someday, we may want to allow this and merge
 *	the type info into subclass procedures....  this seems like
 *	lots of work.
 * --------------------------------
 */
void
RelationRemoveInheritance(Relation relation)
{
    Relation		catalogRelation;
    HeapTuple		tuple;
    HeapScanDesc	scan;
    ScanKeyData	        entry;
    
    /* ----------------
     *	open pg_inherits
     * ----------------
     */
    catalogRelation = heap_openr(InheritsRelationName);
    
    /* ----------------
     *	form a scan key for the subclasses of this class
     *  and begin scanning
     * ----------------
     */
    ScanKeyEntryInitialize(&entry, 0x0, Anum_pg_inherits_inhparent,
			   ObjectIdEqualRegProcedure,
			   ObjectIdGetDatum(RelationGetRelationId(relation)));
    
    scan = heap_beginscan(catalogRelation,
			  false,
			  NowTimeQual,
			  1,
			  &entry);
    
    /* ----------------
     *	if any subclasses exist, then we disallow the deletion.
     * ----------------
     */
    tuple = heap_getnext(scan, 0, (Buffer *)NULL);
    if (HeapTupleIsValid(tuple)) {
	heap_endscan(scan);
	heap_close(catalogRelation);
	
	elog(WARN, "relation <%d> inherits \"%s\"",
	     ((InheritsTupleForm) GETSTRUCT(tuple))->inhrel,
	     RelationGetRelationName(relation));
    }
    
    /* ----------------
     *	If we get here, it means the relation has no subclasses
     *  so we can trash it.  First we remove dead INHERITS tuples.
     * ----------------
     */
    entry.sk_attno = Anum_pg_inherits_inhrel;
    
    scan = heap_beginscan(catalogRelation,
			  false,
			  NowTimeQual,
			  1,
			  &entry);
    
    for (;;) {
	tuple = heap_getnext(scan, 0, (Buffer *)NULL);
	if (!HeapTupleIsValid(tuple)) {
	    break;
	}
	heap_delete(catalogRelation, &tuple->t_ctid);
    }
    
    heap_endscan(scan);
    heap_close(catalogRelation);
    
    /* ----------------
     *	now remove dead IPL tuples
     * ----------------
     */
    catalogRelation =
	heap_openr(InheritancePrecidenceListRelationName);
    
    entry.sk_attno = Anum_pg_ipl_iplrel;
    
    scan = heap_beginscan(catalogRelation,
			  false,
			  NowTimeQual,
			  1,
			  &entry);
    
    for (;;) {
	tuple = heap_getnext(scan, 0, (Buffer *)NULL);
	if (!HeapTupleIsValid(tuple)) {
	    break;
	}
	heap_delete(catalogRelation, &tuple->t_ctid);
    }
    
    heap_endscan(scan);
    heap_close(catalogRelation);
}

/* --------------------------------
 *	RelationRemoveIndexes
 *	
 * --------------------------------
 */
void
RelationRemoveIndexes(Relation relation)
{
    Relation		indexRelation;
    HeapTuple		tuple;
    HeapScanDesc	scan;
    ScanKeyData  	entry;
    
    indexRelation = heap_openr(IndexRelationName);
    
    ScanKeyEntryInitialize(&entry, 0x0, Anum_pg_index_indrelid,
			   ObjectIdEqualRegProcedure,
			   ObjectIdGetDatum(RelationGetRelationId(relation)));
    
    scan = heap_beginscan(indexRelation,
			  false,
			  NowTimeQual,
			  1,
			  &entry);
    
    for (;;) {
	tuple = heap_getnext(scan, 0, (Buffer *)NULL);
	if (!HeapTupleIsValid(tuple)) {
	    break;
	}
	
	index_destroy(((IndexTupleForm)GETSTRUCT(tuple))->indexrelid);
    }
    
    heap_endscan(scan);
    heap_close(indexRelation);
}

/* --------------------------------
 *	DeletePgRelationTuple
 *
 * --------------------------------
 */
void
DeletePgRelationTuple(Relation rdesc)
{
    Relation		pg_class_desc;
    HeapScanDesc	pg_class_scan;
    ScanKeyData    	key;
    HeapTuple		tup;
    
    /* ----------------
     *	open pg_class
     * ----------------
     */
    pg_class_desc = heap_openr(RelationRelationName);
    
    /* ----------------
     *	create a scan key to locate the relation oid of the
     *  relation to delete
     * ----------------
     */
    ScanKeyEntryInitialize(&key, 0, ObjectIdAttributeNumber,
			   F_INT4EQ, rdesc->rd_att->attrs[0]->attrelid);
    
    pg_class_scan =  heap_beginscan(pg_class_desc,
				    0,
				    NowTimeQual,
				    1,
				    &key);
    
    /* ----------------
     *	use heap_getnext() to fetch the pg_class tuple.  If this
     *  tuple is not valid then something's wrong.
     * ----------------
     */
    tup = heap_getnext(pg_class_scan, 0, (Buffer *) NULL);
    
    if (! PointerIsValid(tup)) {
	heap_endscan(pg_class_scan);
	heap_close(pg_class_desc);
	elog(WARN, "DeletePgRelationTuple: %s relation nonexistent",
	     &rdesc->rd_rel->relname);
    }
    
    /* ----------------
     *	delete the relation tuple from pg_class, and finish up.
     * ----------------
     */
    heap_endscan(pg_class_scan);
    heap_delete(pg_class_desc, &tup->t_ctid);
    
    heap_close(pg_class_desc);
}

/* --------------------------------
 *	DeletePgAttributeTuples
 *
 * --------------------------------
 */
void
DeletePgAttributeTuples(Relation rdesc)
{
    Relation		pg_attribute_desc;
    HeapScanDesc	pg_attribute_scan;
    ScanKeyData	        key;
    HeapTuple		tup;
    
    /* ----------------
     *	open pg_attribute
     * ----------------
     */
    pg_attribute_desc = heap_openr(AttributeRelationName);
    
    /* ----------------
     *	create a scan key to locate the attribute tuples to delete
     *  and begin the scan.
     * ----------------
     */
    ScanKeyEntryInitialize(&key, 0, Anum_pg_attribute_attrelid,
			   F_INT4EQ, rdesc->rd_att->attrs[0]->attrelid);
    
    /* -----------------
     * Get a write lock _before_ getting the read lock in the scan
     * ----------------
     */
    RelationSetLockForWrite(pg_attribute_desc);
    
    pg_attribute_scan = heap_beginscan(pg_attribute_desc,
				       0,
				       NowTimeQual,
				       1,
				       &key);
    
    /* ----------------
     *	use heap_getnext() / amdelete() until all attribute tuples
     *  have been deleted.
     * ----------------
     */
    while (tup = heap_getnext(pg_attribute_scan, 0, (Buffer *)NULL),
	   PointerIsValid(tup)) {
	
	heap_delete(pg_attribute_desc, &tup->t_ctid);
    }
    
    /* ----------------
     *	finish up.
     * ----------------
     */
    heap_endscan(pg_attribute_scan);
    
    /* ----------------
     * Release the write lock 
     * ----------------
     */
    RelationUnsetLockForWrite(pg_attribute_desc);
    heap_close(pg_attribute_desc);
}


/* --------------------------------
 *	DeletePgTypeTuple
 *
 *	If the user attempts to destroy a relation and there
 *	exists attributes in other relations of type
 *	"relation we are deleting", then we have to do something
 *	special.  presently we disallow the destroy.
 * --------------------------------
 */
void
DeletePgTypeTuple(Relation rdesc)
{
    Relation		pg_type_desc;
    HeapScanDesc	pg_type_scan;
    Relation		pg_attribute_desc;
    HeapScanDesc	pg_attribute_scan;
    ScanKeyData	        key;
    ScanKeyData	        attkey;
    HeapTuple		tup;
    HeapTuple		atttup;
    Oid		typoid;
    
    /* ----------------
     *	open pg_type
     * ----------------
     */
    pg_type_desc = heap_openr(TypeRelationName);
    
    /* ----------------
     *	create a scan key to locate the type tuple corresponding
     *  to this relation.
     * ----------------
     */
    ScanKeyEntryInitialize(&key, 0, Anum_pg_type_typrelid, F_INT4EQ,
			   rdesc->rd_att->attrs[0]->attrelid);
    
    pg_type_scan =  heap_beginscan(pg_type_desc,
				   0,
				   NowTimeQual,
				   1,
				   &key);
    
    /* ----------------
     *	use heap_getnext() to fetch the pg_type tuple.  If this
     *  tuple is not valid then something's wrong.
     * ----------------
     */
    tup = heap_getnext(pg_type_scan, 0, (Buffer *)NULL);
    
    if (! PointerIsValid(tup)) {
	heap_endscan(pg_type_scan);
	heap_close(pg_type_desc);
	elog(WARN, "DeletePgTypeTuple: %s type nonexistent",
	     &rdesc->rd_rel->relname);
    }
    
    /* ----------------
     *	now scan pg_attribute.  if any other relations have
     *  attributes of the type of the relation we are deleteing
     *  then we have to disallow the deletion.  should talk to
     *  stonebraker about this.  -cim 6/19/90
     * ----------------
     */
    typoid = tup->t_oid;
    
    pg_attribute_desc = heap_openr(AttributeRelationName);
    
    ScanKeyEntryInitialize(&attkey,
			   0, Anum_pg_attribute_atttypid, F_INT4EQ,
			   typoid);
    
    pg_attribute_scan = heap_beginscan(pg_attribute_desc,
				       0,
				       NowTimeQual,
				       1,
				       &attkey);
    
    /* ----------------
     *	try and get a pg_attribute tuple.  if we succeed it means
     *  we cant delete the relation because something depends on
     *  the schema.
     * ----------------
     */
    atttup = heap_getnext(pg_attribute_scan, 0, (Buffer *)NULL);
    
    if (PointerIsValid(atttup)) {
	Oid relid = ((AttributeTupleForm) GETSTRUCT(atttup))->attrelid;
	
	heap_endscan(pg_type_scan);
	heap_close(pg_type_desc);
	heap_endscan(pg_attribute_scan);
	heap_close(pg_attribute_desc);
	
	elog(WARN, "DeletePgTypeTuple: att of type %s exists in relation %d",
	     &rdesc->rd_rel->relname, relid);	
    }
    heap_endscan(pg_attribute_scan);
    heap_close(pg_attribute_desc);
    
    /* ----------------
     *  Ok, it's safe so we delete the relation tuple
     *  from pg_type and finish up.  But first end the scan so that
     *  we release the read lock on pg_type.  -mer 13 Aug 1991
     * ----------------
     */
    heap_endscan(pg_type_scan);
    heap_delete(pg_type_desc, &tup->t_ctid);
    
    heap_close(pg_type_desc);
}

/* --------------------------------
 *	heap_destroy
 *
 * --------------------------------
 */
void
heap_destroy(char *relname)
{
    Relation	rdesc;
    
    /* ----------------
     *	first open the relation.  if the relation does exist,
     *  heap_openr() returns NULL.
     * ----------------
     */
    rdesc = heap_openr(relname);
    if (rdesc == NULL)
	elog(WARN,"Relation %s Does Not Exist!", relname);
    
    /* ----------------
     *	prevent deletion of system relations
     * ----------------
     */
    if (IsSystemRelationName(RelationGetRelationName(rdesc)->data))
	elog(WARN, "amdestroy: cannot destroy %s relation",
	     &rdesc->rd_rel->relname);
    
    /* ----------------
     *	remove inheritance information
     * ----------------
     */
    RelationRemoveInheritance(rdesc);
    
    /* ----------------
     *	remove indexes if necessary
     * ----------------
     */
    if (rdesc->rd_rel->relhasindex) {
	RelationRemoveIndexes(rdesc);
    }

    /* ----------------
     *	remove rules if necessary
     * ----------------
     */
    if (rdesc->rd_rules != NULL) {
	RelationRemoveRules(rdesc->rd_id);
    }
    
    /* ----------------
     *	delete attribute tuples
     * ----------------
     */
    DeletePgAttributeTuples(rdesc);
    
    /* ----------------
     *	delete type tuple.  here we want to see the effects
     *  of the deletions we just did, so we use setheapoverride().
     * ----------------
     */
    setheapoverride(true);
    DeletePgTypeTuple(rdesc);
    setheapoverride(false);
    
    /* ----------------
     *	delete relation tuple
     * ----------------
     */
    DeletePgRelationTuple(rdesc);
    
    /* ----------------
     *	flush the relation from the relcache
     * ----------------
     */
    RelationIdInvalidateRelationCacheByRelationId(rdesc->rd_id);

    /* ----------------
     *	unlink the relation and finish up.
     * ----------------
     */
    (void) smgrunlink(rdesc->rd_rel->relsmgr, rdesc);
    if(rdesc->rd_istemp) {
        rdesc->rd_tmpunlinked = TRUE;
    }
    heap_close(rdesc);
}

/*
 * heap_destroyr
 *    destroy and close temporary relations
 *
 */

void 
heap_destroyr(Relation rdesc)
{
    ReleaseTmpRelBuffers(rdesc);
    (void) smgrunlink(rdesc->rd_rel->relsmgr, rdesc);
    if(rdesc->rd_istemp) {
        rdesc->rd_tmpunlinked = TRUE;
    }
    heap_close(rdesc);
    RemoveFromTempRelList(rdesc);
}


/**************************************************************
  functions to deal with the list of temporary relations 
**************************************************************/

/* --------------
   InitTempRellist():

   initialize temporary relations list
   the tempRelList is a list of temporary relations that
   are created in the course of the transactions
   they need to be destroyed properly at the end of the transactions

   MODIFIES the global variable tempRels

 >> NOTE <<

   malloc is used instead of palloc because we KNOW when we are
   going to free these things.  Keeps us away from the memory context
   hairyness

*/
void
InitTempRelList(void)
{
    if (tempRels) {
	free(tempRels->rels);
	free(tempRels);
    };

    tempRels = (TempRelList*)malloc(sizeof(TempRelList));
    tempRels->size = TEMP_REL_LIST_SIZE;
    tempRels->rels = (Relation*)malloc(sizeof(Relation) * tempRels->size);
    memset(tempRels->rels, 0, sizeof(Relation) * tempRels->size);
    tempRels->num = 0;
}

/*
   removes a relation from the TempRelList

   MODIFIES the global variable tempRels
      we don't really remove it, just mark it as NULL
      and DestroyTempRels will look for NULLs
*/
void
RemoveFromTempRelList(Relation r)
{
    int i;

    if (!tempRels)
	return;

    for (i=0; i<tempRels->num; i++) {
	if (tempRels->rels[i] == r) {
	    tempRels->rels[i] = NULL;
	    break;
	}
    }
}

/*
   add a temporary relation to the TempRelList

   MODIFIES the global variable tempRels
*/
void
AddToTempRelList(Relation r)
{
    if (!tempRels)
	return;

    if (tempRels->num == tempRels->size) {
	tempRels->size += TEMP_REL_LIST_SIZE;
	tempRels->rels = realloc(tempRels->rels, tempRels->size);
    }
    tempRels->rels[tempRels->num] = r;
    tempRels->num++;
}

/*
   go through the tempRels list and destroy each of the relations
*/
void
DestroyTempRels(void)
{
    int i;
    Relation rdesc;

    if (!tempRels)
	return;

    for (i=0;i<tempRels->num;i++) {
	rdesc = tempRels->rels[i];
	/* rdesc may be NULL if it has been removed from the list already */
	if (rdesc)
	    heap_destroyr(rdesc);
    }
    free(tempRels->rels);
    free(tempRels);
    tempRels = NULL;
}

