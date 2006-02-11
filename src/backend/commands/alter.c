/*-------------------------------------------------------------------------
 *
 * alter.c
 *	  Drivers for generic alter commands
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/commands/alter.c,v 1.16 2006/02/11 22:17:18 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/htup.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/namespace.h"
#include "catalog/pg_class.h"
#include "catalog/pg_constraint.h"
#include "commands/alter.h"
#include "commands/conversioncmds.h"
#include "commands/dbcommands.h"
#include "commands/defrem.h"
#include "commands/proclang.h"
#include "commands/schemacmds.h"
#include "commands/tablecmds.h"
#include "commands/tablespace.h"
#include "commands/trigger.h"
#include "commands/typecmds.h"
#include "commands/user.h"
#include "miscadmin.h"
#include "parser/parse_clause.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


/*
 * Executes an ALTER OBJECT / RENAME TO statement.	Based on the object
 * type, the function appropriate to that type is executed.
 */
void
ExecRenameStmt(RenameStmt *stmt)
{
	switch (stmt->renameType)
	{
		case OBJECT_AGGREGATE:
			RenameAggregate(stmt->object,
							(TypeName *) linitial(stmt->objarg),
							stmt->newname);
			break;

		case OBJECT_CONVERSION:
			RenameConversion(stmt->object, stmt->newname);
			break;

		case OBJECT_DATABASE:
			RenameDatabase(stmt->subname, stmt->newname);
			break;

		case OBJECT_FUNCTION:
			RenameFunction(stmt->object, stmt->objarg, stmt->newname);
			break;

		case OBJECT_LANGUAGE:
			RenameLanguage(stmt->subname, stmt->newname);
			break;

		case OBJECT_OPCLASS:
			RenameOpClass(stmt->object, stmt->subname, stmt->newname);
			break;

		case OBJECT_ROLE:
			RenameRole(stmt->subname, stmt->newname);
			break;

		case OBJECT_SCHEMA:
			RenameSchema(stmt->subname, stmt->newname);
			break;

		case OBJECT_TABLESPACE:
			RenameTableSpace(stmt->subname, stmt->newname);
			break;

		case OBJECT_TABLE:
		case OBJECT_INDEX:
		case OBJECT_COLUMN:
		case OBJECT_TRIGGER:
		case OBJECT_CONSTRAINT:
			{
				Oid			relid;

				CheckRelationOwnership(stmt->relation, true);

				relid = RangeVarGetRelid(stmt->relation, false);

				switch (stmt->renameType)
				{
					case OBJECT_TABLE:
					case OBJECT_INDEX:
						{
							/*
							 * RENAME TABLE requires that we (still) hold
							 * CREATE rights on the containing namespace, as
							 * well as ownership of the table.
							 */
							Oid			namespaceId = get_rel_namespace(relid);
							AclResult	aclresult;

							aclresult = pg_namespace_aclcheck(namespaceId,
													GetUserId(), ACL_CREATE);
							if (aclresult != ACLCHECK_OK)
								aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
											get_namespace_name(namespaceId));

							/*
							 *	Do NOT refer to stmt->renameType here because
							 *	you can also rename an index with ALTER TABLE
							 */
							if (get_rel_relkind(relid) == RELKIND_INDEX)
							{
								/* see if we depend on a constraint */
								List* depOids = getDependentOids(
												RelationRelationId, relid,
												ConstraintRelationId,
												DEPENDENCY_INTERNAL);

								/* there should only be one constraint */
								Assert(list_length(depOids) <= 1);
								if (list_length(depOids) == 1)
								{
										Oid conRelId = linitial_oid(depOids);
										/*
										 *	Apply the same name to the
										 *	constraint and tell it that this
										 *	is an implicit rename triggered
										 *	by an "ALTER INDEX" command.
										 */
										RenameConstraint(conRelId,
											stmt->newname, true, "ALTER INDEX");
								}
							}
							renamerel(relid, stmt->newname);
							break;
						}
					case OBJECT_COLUMN:
						renameatt(relid,
								  stmt->subname,		/* old att name */
								  stmt->newname,		/* new att name */
								  interpretInhOption(stmt->relation->inhOpt),	/* recursive? */
								  false);		/* recursing already? */
						break;
					case OBJECT_TRIGGER:
						renametrig(relid,
								   stmt->subname,		/* old att name */
								   stmt->newname);		/* new att name */
						break;
					case OBJECT_CONSTRAINT:
						/* XXX could do extra function renameconstr() - but I
						 * don't know where it should go */
						/* renameconstr(relid,
									 stmt->subname,
									 stmt->newname); */
						{
							List		*depRelOids;
							ListCell	*l;
							Oid conId =
									GetRelationConstraintOid(relid,
															 stmt->subname);
							if (!OidIsValid(conId)) {
								ereport(ERROR,
										(errcode(ERRCODE_UNDEFINED_OBJECT),
										 errmsg("constraint with name \"%s\" "
												"does not exist",
												stmt->subname)));
							}
							RenameConstraint(conId, stmt->newname,
											 false, NULL);
							depRelOids = getReferencingOids(
											ConstraintRelationId, conId, 0,
											RelationRelationId,
											DEPENDENCY_INTERNAL);
							foreach(l, depRelOids)
							{
								Oid		depRelOid;
								Oid		nspOid;
								depRelOid = lfirst_oid(l);
								nspOid = get_rel_namespace(depRelOid);
								if (get_rel_relkind(depRelOid) == RELKIND_INDEX)
								{
									ereport(NOTICE,
											(errmsg("ALTER TABLE / CONSTRAINT will implicitly rename index "
													"\"%s\" to \"%s\" on table \"%s.%s\"",
												get_rel_name(depRelOid),
												stmt->newname,
												get_namespace_name(nspOid),
												get_rel_name(relid))));
									renamerel(depRelOid, stmt->newname);
								}
							}
						}
						break;

					default:
						 /* can't happen */ ;
				}
				break;
			}

		default:
			elog(ERROR, "unrecognized rename stmt type: %d",
				 (int) stmt->renameType);
	}
}

/*
 * Executes an ALTER OBJECT / SET SCHEMA statement.  Based on the object
 * type, the function appropriate to that type is executed.
 */
void
ExecAlterObjectSchemaStmt(AlterObjectSchemaStmt *stmt)
{
	switch (stmt->objectType)
	{
		case OBJECT_AGGREGATE:
		case OBJECT_FUNCTION:
			AlterFunctionNamespace(stmt->object, stmt->objarg,
								   stmt->newschema);
			break;

		case OBJECT_SEQUENCE:
		case OBJECT_TABLE:
			CheckRelationOwnership(stmt->relation, true);
			AlterTableNamespace(stmt->relation, stmt->newschema);
			break;

		case OBJECT_TYPE:
		case OBJECT_DOMAIN:
			AlterTypeNamespace(stmt->object, stmt->newschema);
			break;

		default:
			elog(ERROR, "unrecognized AlterObjectSchemaStmt type: %d",
				 (int) stmt->objectType);
	}
}

/*
 * Executes an ALTER OBJECT / OWNER TO statement.  Based on the object
 * type, the function appropriate to that type is executed.
 */
void
ExecAlterOwnerStmt(AlterOwnerStmt *stmt)
{
	Oid			newowner = get_roleid_checked(stmt->newowner);

	switch (stmt->objectType)
	{
		case OBJECT_AGGREGATE:
			AlterAggregateOwner(stmt->object,
								(TypeName *) linitial(stmt->objarg),
								newowner);
			break;

		case OBJECT_CONVERSION:
			AlterConversionOwner(stmt->object, newowner);
			break;

		case OBJECT_DATABASE:
			AlterDatabaseOwner((char *) linitial(stmt->object), newowner);
			break;

		case OBJECT_FUNCTION:
			AlterFunctionOwner(stmt->object, stmt->objarg, newowner);
			break;

		case OBJECT_OPERATOR:
			AlterOperatorOwner(stmt->object,
							   (TypeName *) linitial(stmt->objarg),
							   (TypeName *) lsecond(stmt->objarg),
							   newowner);
			break;

		case OBJECT_OPCLASS:
			AlterOpClassOwner(stmt->object, stmt->addname, newowner);
			break;

		case OBJECT_SCHEMA:
			AlterSchemaOwner((char *) linitial(stmt->object), newowner);
			break;

		case OBJECT_TABLESPACE:
			AlterTableSpaceOwner((char *) linitial(stmt->object), newowner);
			break;

		case OBJECT_TYPE:
		case OBJECT_DOMAIN:		/* same as TYPE */
			AlterTypeOwner(stmt->object, newowner);
			break;

		default:
			elog(ERROR, "unrecognized AlterOwnerStmt type: %d",
				 (int) stmt->objectType);
	}
}
