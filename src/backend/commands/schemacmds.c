/*-------------------------------------------------------------------------
 *
 * schemacmds.c
 *	  schema creation/manipulation commands
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/schemacmds.c,v 1.9 2003-05-06 20:26:26 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catalog.h"
#include "catalog/catname.h"
#include "catalog/dependency.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "commands/schemacmds.h"
#include "miscadmin.h"
#include "parser/analyze.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


/*
 * CREATE SCHEMA
 */
void
CreateSchemaCommand(CreateSchemaStmt *stmt)
{
	const char *schemaName = stmt->schemaname;
	const char *authId = stmt->authid;
	Oid			namespaceId;
	List	   *parsetree_list;
	List	   *parsetree_item;
	const char *owner_name;
	AclId		owner_userid;
	AclId		saved_userid;
	AclResult	aclresult;

	saved_userid = GetUserId();

	/*
	 * Figure out user identities.
	 */

	if (!authId)
	{
		owner_userid = saved_userid;
		owner_name = GetUserNameFromId(owner_userid);
	}
	else if (superuser())
	{
		owner_name = authId;
		/* The following will error out if user does not exist */
		owner_userid = get_usesysid(owner_name);

		/*
		 * Set the current user to the requested authorization so that
		 * objects created in the statement have the requested owner.
		 * (This will revert to session user on error or at the end of
		 * this routine.)
		 */
		SetUserId(owner_userid);
	}
	else
/* not superuser */
	{
		owner_userid = saved_userid;
		owner_name = GetUserNameFromId(owner_userid);
		if (strcmp(authId, owner_name) != 0)
			elog(ERROR, "CREATE SCHEMA: permission denied"
				 "\n\t\"%s\" is not a superuser, so cannot create a schema for \"%s\"",
				 owner_name, authId);
	}

	/*
	 * Permissions checks.
	 */
	aclresult = pg_database_aclcheck(MyDatabaseId, saved_userid, ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, DatabaseName);

	if (!allowSystemTableMods && IsReservedName(schemaName))
		elog(ERROR, "CREATE SCHEMA: Illegal schema name: \"%s\" -- pg_ is reserved for system schemas",
			 schemaName);

	/* Create the schema's namespace */
	namespaceId = NamespaceCreate(schemaName, owner_userid);

	/* Advance cmd counter to make the namespace visible */
	CommandCounterIncrement();

	/*
	 * Temporarily make the new namespace be the front of the search path,
	 * as well as the default creation target namespace.  This will be
	 * undone at the end of this routine, or upon error.
	 */
	PushSpecialNamespace(namespaceId);

	/*
	 * Examine the list of commands embedded in the CREATE SCHEMA command,
	 * and reorganize them into a sequentially executable order with no
	 * forward references.	Note that the result is still a list of raw
	 * parsetrees in need of parse analysis --- we cannot, in general, run
	 * analyze.c on one statement until we have actually executed the
	 * prior ones.
	 */
	parsetree_list = analyzeCreateSchemaStmt(stmt);

	/*
	 * Analyze and execute each command contained in the CREATE SCHEMA
	 */
	foreach(parsetree_item, parsetree_list)
	{
		Node	   *parsetree = (Node *) lfirst(parsetree_item);
		List	   *querytree_list,
				   *querytree_item;

		querytree_list = parse_analyze(parsetree, NULL, 0);

		foreach(querytree_item, querytree_list)
		{
			Query	   *querytree = (Query *) lfirst(querytree_item);

			/* schemas should contain only utility stmts */
			Assert(querytree->commandType == CMD_UTILITY);
			/* do this step */
			ProcessUtility(querytree->utilityStmt, None_Receiver, NULL);
			/* make sure later steps can see the object created here */
			CommandCounterIncrement();
		}
	}

	/* Reset search path to normal state */
	PopSpecialNamespace(namespaceId);

	/* Reset current user */
	SetUserId(saved_userid);
}


/*
 *	RemoveSchema
 *		Removes a schema.
 */
void
RemoveSchema(List *names, DropBehavior behavior)
{
	char	   *namespaceName;
	Oid			namespaceId;
	ObjectAddress object;

	if (length(names) != 1)
		elog(ERROR, "Schema name may not be qualified");
	namespaceName = strVal(lfirst(names));

	namespaceId = GetSysCacheOid(NAMESPACENAME,
								 CStringGetDatum(namespaceName),
								 0, 0, 0);
	if (!OidIsValid(namespaceId))
		elog(ERROR, "Schema \"%s\" does not exist", namespaceName);

	/* Permission check */
	if (!pg_namespace_ownercheck(namespaceId, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, namespaceName);

	/*
	 * Do the deletion.  Objects contained in the schema are removed by
	 * means of their dependency links to the schema.
	 *
	 * XXX currently, index opclasses don't have creation/deletion commands,
	 * so they will not get removed when the containing schema is removed.
	 * This is annoying but not fatal.
	 */
	object.classId = get_system_catalog_relid(NamespaceRelationName);
	object.objectId = namespaceId;
	object.objectSubId = 0;

	performDeletion(&object, behavior);
}


/*
 * Guts of schema deletion.
 */
void
RemoveSchemaById(Oid schemaOid)
{
	Relation	relation;
	HeapTuple	tup;

	relation = heap_openr(NamespaceRelationName, RowExclusiveLock);

	tup = SearchSysCache(NAMESPACEOID,
						 ObjectIdGetDatum(schemaOid),
						 0, 0, 0);
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "RemoveSchemaById: schema %u not found",
			 schemaOid);

	simple_heap_delete(relation, &tup->t_self);

	ReleaseSysCache(tup);

	heap_close(relation, RowExclusiveLock);
}
