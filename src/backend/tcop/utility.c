/*-------------------------------------------------------------------------
 *
 * utility.c
 *	  Contains functions which control the execution of the POSTGRES utility
 *	  commands.  At one time acted as an interface between the Lisp and C
 *	  systems.
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/tcop/utility.c,v 1.83 2000-02-18 09:29:31 inoue Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "catalog/catalog.h"
#include "catalog/pg_type.h"
#include "commands/async.h"
#include "commands/cluster.h"
#include "commands/command.h"
#include "commands/comment.h"
#include "commands/copy.h"
#include "commands/creatinh.h"
#include "commands/dbcommands.h"
#include "commands/defrem.h"
#include "commands/explain.h"
#include "commands/proclang.h"
#include "commands/rename.h"
#include "commands/sequence.h"
#include "commands/trigger.h"
#include "commands/user.h"
#include "commands/vacuum.h"
#include "commands/variable.h"
#include "commands/view.h"
#include "miscadmin.h"
#include "parser/parse.h"
#include "rewrite/rewriteDefine.h"
#include "rewrite/rewriteRemove.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/ps_status.h"
#include "utils/syscache.h"


/* ----------------
 *		CHECK_IF_ABORTED() is used to avoid doing unnecessary
 *		processing within an aborted transaction block.
 * ----------------
 */
 /* we have to use IF because of the 'break' */
#define CHECK_IF_ABORTED() \
if (1) \
{ \
	if (IsAbortedTransactionBlockState()) \
	{ \
		elog(NOTICE, "current transaction is aborted, " \
			 "queries ignored until end of transaction block"); \
		commandTag = "*ABORT STATE*"; \
		break; \
	} \
} else

/* ----------------
 *		general utility function invoker
 * ----------------
 */
void
ProcessUtility(Node *parsetree,
			   CommandDest dest)
{
	char	   *commandTag = NULL;
	char	   *relname;
	char	   *relationName;
	char	   *userName;

	userName = GetPgUserName();

	switch (nodeTag(parsetree))
	{

			/*
			 * ******************************** transactions ********************************
			 *
			 */
		case T_TransactionStmt:
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;

				switch (stmt->command)
				{
					case BEGIN_TRANS:
						PS_SET_STATUS(commandTag = "BEGIN");
						CHECK_IF_ABORTED();
						BeginTransactionBlock();
						break;

					case COMMIT:
						PS_SET_STATUS(commandTag = "COMMIT");
						EndTransactionBlock();
						break;

					case ROLLBACK:
						PS_SET_STATUS(commandTag = "ROLLBACK");
						UserAbortTransactionBlock();
						break;
				}
			}
			break;

			/*
			 * ******************************** portal manipulation ********************************
			 *
			 */
		case T_ClosePortalStmt:
			{
				ClosePortalStmt *stmt = (ClosePortalStmt *) parsetree;

				PS_SET_STATUS(commandTag = "CLOSE");
				CHECK_IF_ABORTED();

				PerformPortalClose(stmt->portalname, dest);
			}
			break;

		case T_FetchStmt:
			{
				FetchStmt  *stmt = (FetchStmt *) parsetree;
				char	   *portalName = stmt->portalname;
				bool		forward;
				int			count;

				PS_SET_STATUS(commandTag = (stmt->ismove) ? "MOVE" : "FETCH");
				CHECK_IF_ABORTED();

				forward = (bool) (stmt->direction == FORWARD);

				/*
				 * parser ensures that count is >= 0 and 'fetch ALL' -> 0
				 */

				count = stmt->howMany;
				PerformPortalFetch(portalName, forward, count, commandTag,
								   (stmt->ismove) ? None : dest);		/* /dev/null for MOVE */
			}
			break;

			/*
			 * ******************************** relation and attribute
			 * manipulation ********************************
			 *
			 */
		case T_CreateStmt:
			PS_SET_STATUS(commandTag = "CREATE");
			CHECK_IF_ABORTED();

			DefineRelation((CreateStmt *) parsetree, RELKIND_RELATION);
			break;

		case T_DropStmt:
			{
				DropStmt *stmt = (DropStmt *) parsetree;
				List	   *args = stmt->relNames;
				List	   *arg;

				PS_SET_STATUS(commandTag = "DROP");
				CHECK_IF_ABORTED();

				/* check as much as we can before we start dropping ... */
				foreach(arg, args)
				{
					Relation	rel;

					relname = strVal(lfirst(arg));
					if (!allowSystemTableMods && IsSystemRelationName(relname))
						elog(ERROR, "class \"%s\" is a system catalog",
							 relname);
					rel = heap_openr(relname, AccessExclusiveLock);
					if (stmt->sequence &&
						rel->rd_rel->relkind != RELKIND_SEQUENCE)
						elog(ERROR, "Use DROP TABLE to drop table '%s'",
							 relname);
					if (!(stmt->sequence) &&
						rel->rd_rel->relkind == RELKIND_SEQUENCE)
						elog(ERROR, "Use DROP SEQUENCE to drop sequence '%s'",
							 relname);
					/* close rel, but keep lock until end of xact */
					heap_close(rel, NoLock);
#ifndef NO_SECURITY
					if (!pg_ownercheck(userName, relname, RELNAME))
						elog(ERROR, "you do not own class \"%s\"",
							 relname);
#endif
				}
				/* OK, terminate 'em all */
				foreach(arg, args)
				{
					relname = strVal(lfirst(arg));
					RemoveRelation(relname);
				}
			}
			break;

		case T_TruncateStmt:
			{
				Relation	rel;

				PS_SET_STATUS(commandTag = "TRUNCATE");
				CHECK_IF_ABORTED();

				relname = ((TruncateStmt *) parsetree)->relName;			
				if (!allowSystemTableMods && IsSystemRelationName(relname))
					elog(ERROR, "TRUNCATE cannot be used on system tables. '%s' is a system table",
						 relname);

				/* Grab exclusive lock in preparation for truncate... */
				rel = heap_openr(relname, AccessExclusiveLock);
			    if (rel->rd_rel->relkind == RELKIND_SEQUENCE)
					elog(ERROR, "TRUNCATE cannot be used on sequences. '%s' is a sequence",
						 relname);
			    heap_close(rel, NoLock);

#ifndef NO_SECURITY
				if (!pg_ownercheck(userName, relname, RELNAME))
					elog(ERROR, "you do not own class \"%s\"", relname);
#endif
				TruncateRelation(relname);
			}
			break;

	case T_CommentStmt:
	  {
	    
	    CommentStmt *statement;
	    
	    statement = ((CommentStmt *) parsetree);
	    
	    PS_SET_STATUS(commandTag = "COMMENT");
	    CHECK_IF_ABORTED();
	    CommentObject(statement->objtype, statement->objname,
			  statement->objproperty, statement->objlist,
			  statement->comment);
	  }
	  break;
	    


		case T_CopyStmt:
			{
				CopyStmt   *stmt = (CopyStmt *) parsetree;

				PS_SET_STATUS(commandTag = "COPY");
				CHECK_IF_ABORTED();

				DoCopy(stmt->relname,
					   stmt->binary,
					   stmt->oids,
					   (bool) (stmt->direction == FROM),
					   (bool) (stmt->filename == NULL),

				/*
				 * null filename means copy to/from stdout/stdin, rather
				 * than to/from a file.
				 */
					   stmt->filename,
					   stmt->delimiter,
                       stmt->null_print);
			}
			break;

			/*
			 * schema
			 */
		case T_RenameStmt:
			{
				RenameStmt *stmt = (RenameStmt *) parsetree;

				PS_SET_STATUS(commandTag = "ALTER");
				CHECK_IF_ABORTED();

				relname = stmt->relname;
				if (!allowSystemTableMods && IsSystemRelationName(relname))
					elog(ERROR, "ALTER TABLE: relation \"%s\" is a system catalog",
						 relname);
#ifndef NO_SECURITY
				if (!pg_ownercheck(userName, relname, RELNAME))
					elog(ERROR, "permission denied");
#endif

				/* ----------------
				 *	XXX using len == 3 to tell the difference
				 *		between "rename rel to newrel" and
				 *		"rename att in rel to newatt" will not
				 *		work soon because "rename type/operator/rule"
				 *		stuff is being added. - cim 10/24/90
				 * ----------------
				 * [another piece of amuzing but useless anecdote -- ay]
				 */
				if (stmt->column == NULL)
				{
					/* ----------------
					 *		rename relation
					 *
					 *		Note: we also rename the "type" tuple
					 *		corresponding to the relation.
					 * ----------------
					 */
					renamerel(relname,	/* old name */
							  stmt->newname);	/* new name */
					TypeRename(relname, /* old name */
							   stmt->newname);	/* new name */
				}
				else
				{
					/* ----------------
					 *		rename attribute
					 * ----------------
					 */
					renameatt(relname,	/* relname */
							  stmt->column,		/* old att name */
							  stmt->newname,	/* new att name */
							  userName,
							  stmt->inh);		/* recursive? */
				}
			}
			break;

            /* various Alter Table forms */

		case T_AlterTableStmt:
        {
            AlterTableStmt *stmt = (AlterTableStmt *) parsetree;

            PS_SET_STATUS(commandTag = "ALTER");
            CHECK_IF_ABORTED();

            /*
             * Some or all of these functions are recursive to cover inherited things,
             * so permission checks are done there.
             */
            switch(stmt->subtype)
            {
                case 'A': /* ADD COLUMN */
                    AlterTableAddColumn(stmt->relname, stmt->inh, (ColumnDef *) stmt->def);
                    break;
                case 'T': /* ALTER COLUMN */
                    AlterTableAlterColumn(stmt->relname, stmt->inh, stmt->name, stmt->def);
                    break;
                case 'D': /* ALTER DROP */
                    AlterTableDropColumn(stmt->relname, stmt->inh, stmt->name, stmt->behavior);
                    break;
                case 'C': /* ADD CONSTRAINT */
                    AlterTableAddConstraint(stmt->relname, stmt->inh, stmt->def);
                    break;
                case 'X': /* DROP CONSTRAINT */
                    AlterTableDropConstraint(stmt->relname, stmt->inh, stmt->name, stmt->behavior);
                    break;
                default: /* oops */
                    elog(ERROR, "T_AlterTableStmt: unknown subtype");
                    break;
            }
        }
        break;


		case T_ChangeACLStmt:
			{
				ChangeACLStmt *stmt = (ChangeACLStmt *) parsetree;
				List	   *i;
				AclItem    *aip;
				unsigned	modechg;

				PS_SET_STATUS(commandTag = "CHANGE");
				CHECK_IF_ABORTED();

				aip = stmt->aclitem;

				modechg = stmt->modechg;
				foreach(i, stmt->relNames)
                {
					Relation	rel;

					relname = strVal(lfirst(i));
					rel = heap_openr(relname, AccessExclusiveLock);
					if (rel && rel->rd_rel->relkind == RELKIND_INDEX)
						elog(ERROR, "\"%s\" is an index relation",
							 relname);
					/* close rel, but keep lock until end of xact */
					heap_close(rel, NoLock);
#ifndef NO_SECURITY
					if (!pg_ownercheck(userName, relname, RELNAME))
						elog(ERROR, "you do not own class \"%s\"",
							 relname);
#endif
					ChangeAcl(relname, aip, modechg);
				}
			}
			break;

			/*
			 * ******************************** object creation /
			 * destruction ********************************
			 *
			 */
		case T_DefineStmt:
			{
				DefineStmt *stmt = (DefineStmt *) parsetree;

				PS_SET_STATUS(commandTag = "CREATE");
				CHECK_IF_ABORTED();

				switch (stmt->defType)
				{
					case OPERATOR:
						DefineOperator(stmt->defname,	/* operator name */
									   stmt->definition);		/* rest */
						break;
					case TYPE_P:
						DefineType(stmt->defname, stmt->definition);
						break;
					case AGGREGATE:
						DefineAggregate(stmt->defname,	/* aggregate name */
										stmt->definition);		/* rest */
						break;
				}
			}
			break;

		case T_ViewStmt:		/* CREATE VIEW */
			{
				ViewStmt   *stmt = (ViewStmt *) parsetree;

				PS_SET_STATUS(commandTag = "CREATE");
				CHECK_IF_ABORTED();
				DefineView(stmt->viewname, stmt->query);		/* retrieve parsetree */
			}
			break;

		case T_ProcedureStmt:	/* CREATE FUNCTION */
			PS_SET_STATUS(commandTag = "CREATE");
			CHECK_IF_ABORTED();
			CreateFunction((ProcedureStmt *) parsetree, dest);	/* everything */
			break;

		case T_IndexStmt:		/* CREATE INDEX */
			{
				IndexStmt  *stmt = (IndexStmt *) parsetree;

				PS_SET_STATUS(commandTag = "CREATE");
				CHECK_IF_ABORTED();
				DefineIndex(stmt->relname,		/* relation name */
							stmt->idxname,		/* index name */
							stmt->accessMethod, /* am name */
							stmt->indexParams,	/* parameters */
							stmt->withClause,
							stmt->unique,
							stmt->primary,
							(Expr *) stmt->whereClause,
							stmt->rangetable);
			}
			break;

		case T_RuleStmt:		/* CREATE RULE */
			{
				RuleStmt   *stmt = (RuleStmt *) parsetree;
				int			aclcheck_result;

#ifndef NO_SECURITY
				relname = stmt->object->relname;
				aclcheck_result = pg_aclcheck(relname, userName, ACL_RU);
				if (aclcheck_result != ACLCHECK_OK)
					elog(ERROR, "%s: %s", relname, aclcheck_error_strings[aclcheck_result]);
#endif
				PS_SET_STATUS(commandTag = "CREATE");
				CHECK_IF_ABORTED();
				DefineQueryRewrite(stmt);
			}
			break;

		case T_CreateSeqStmt:
			PS_SET_STATUS(commandTag = "CREATE");
			CHECK_IF_ABORTED();

			DefineSequence((CreateSeqStmt *) parsetree);
			break;

		case T_ExtendStmt:
			{
				ExtendStmt *stmt = (ExtendStmt *) parsetree;

				PS_SET_STATUS(commandTag = "EXTEND");
				CHECK_IF_ABORTED();

				ExtendIndex(stmt->idxname,		/* index name */
							(Expr *) stmt->whereClause, /* where */
							stmt->rangetable);
			}
			break;

		case T_RemoveStmt:
			{
				RemoveStmt *stmt = (RemoveStmt *) parsetree;

				PS_SET_STATUS(commandTag = "DROP");
				CHECK_IF_ABORTED();

				switch (stmt->removeType)
				{
					case INDEX:
						relname = stmt->name;
						if (!allowSystemTableMods && IsSystemRelationName(relname))
							elog(ERROR, "class \"%s\" is a system catalog index",
								 relname);
#ifndef NO_SECURITY
						if (!pg_ownercheck(userName, relname, RELNAME))
							elog(ERROR, "%s: %s", relname, aclcheck_error_strings[ACLCHECK_NOT_OWNER]);
#endif
						RemoveIndex(relname);
						break;
					case RULE:
						{
							char	   *rulename = stmt->name;
							int			aclcheck_result;

#ifndef NO_SECURITY

							relationName = RewriteGetRuleEventRel(rulename);
							aclcheck_result = pg_aclcheck(relationName, userName, ACL_RU);
							if (aclcheck_result != ACLCHECK_OK)
								elog(ERROR, "%s: %s", relationName, aclcheck_error_strings[aclcheck_result]);
#endif
							RemoveRewriteRule(rulename);
						}
						break;
					case TYPE_P:
#ifndef NO_SECURITY
						/* XXX moved to remove.c */
#endif
						RemoveType(stmt->name);
						break;
					case VIEW:
						{
							char	   *viewName = stmt->name;
							char	   *ruleName;

#ifndef NO_SECURITY

							ruleName = MakeRetrieveViewRuleName(viewName);
							relationName = RewriteGetRuleEventRel(ruleName);
							if (!pg_ownercheck(userName, relationName, RELNAME))
								elog(ERROR, "%s: %s", relationName, aclcheck_error_strings[ACLCHECK_NOT_OWNER]);
							pfree(ruleName);
#endif
							RemoveView(viewName);
						}
						break;
				}
				break;
			}
			break;

		case T_RemoveAggrStmt:
			{
				RemoveAggrStmt *stmt = (RemoveAggrStmt *) parsetree;

				PS_SET_STATUS(commandTag = "DROP");
				CHECK_IF_ABORTED();
				RemoveAggregate(stmt->aggname, stmt->aggtype);
			}
			break;

		case T_RemoveFuncStmt:
			{
				RemoveFuncStmt *stmt = (RemoveFuncStmt *) parsetree;

				PS_SET_STATUS(commandTag = "DROP");
				CHECK_IF_ABORTED();
				RemoveFunction(stmt->funcname,
							   length(stmt->args),
							   stmt->args);
			}
			break;

		case T_RemoveOperStmt:
			{
				RemoveOperStmt *stmt = (RemoveOperStmt *) parsetree;
				char	   *type1 = (char *) NULL;
				char	   *type2 = (char *) NULL;

				PS_SET_STATUS(commandTag = "DROP");
				CHECK_IF_ABORTED();

				if (lfirst(stmt->args) != NULL)
					type1 = strVal(lfirst(stmt->args));
				if (lsecond(stmt->args) != NULL)
					type2 = strVal(lsecond(stmt->args));
				RemoveOperator(stmt->opname, type1, type2);
			}
			break;

		case T_VersionStmt:
			elog(ERROR, "CREATE VERSION is not currently implemented");
			break;

		case T_CreatedbStmt:
			{
				CreatedbStmt *stmt = (CreatedbStmt *) parsetree;

				PS_SET_STATUS(commandTag = "CREATE DATABASE");
				CHECK_IF_ABORTED();
				createdb(stmt->dbname, stmt->dbpath, stmt->encoding);
			}
			break;

		case T_DropdbStmt:
			{
				DropdbStmt *stmt = (DropdbStmt *) parsetree;

				PS_SET_STATUS(commandTag = "DROP DATABASE");
				CHECK_IF_ABORTED();
				dropdb(stmt->dbname);
			}
			break;

			/* Query-level asynchronous notification */
		case T_NotifyStmt:
			{
				NotifyStmt *stmt = (NotifyStmt *) parsetree;

				PS_SET_STATUS(commandTag = "NOTIFY");
				CHECK_IF_ABORTED();

				Async_Notify(stmt->relname);
			}
			break;

		case T_ListenStmt:
			{
				ListenStmt *stmt = (ListenStmt *) parsetree;

				PS_SET_STATUS(commandTag = "LISTEN");
				CHECK_IF_ABORTED();

				Async_Listen(stmt->relname, MyProcPid);
			}
			break;

		case T_UnlistenStmt:
			{
				UnlistenStmt *stmt = (UnlistenStmt *) parsetree;

				PS_SET_STATUS(commandTag = "UNLISTEN");
				CHECK_IF_ABORTED();

				Async_Unlisten(stmt->relname, MyProcPid);
			}
			break;

			/*
			 * ******************************** dynamic loader ********************************
			 *
			 */
		case T_LoadStmt:
			{
				LoadStmt   *stmt = (LoadStmt *) parsetree;

				PS_SET_STATUS(commandTag = "LOAD");
				CHECK_IF_ABORTED();

				closeAllVfds(); /* probably not necessary... */
				load_file(stmt->filename);
			}
			break;

		case T_ClusterStmt:
			{
				ClusterStmt *stmt = (ClusterStmt *) parsetree;

				PS_SET_STATUS(commandTag = "CLUSTER");
				CHECK_IF_ABORTED();

				cluster(stmt->relname, stmt->indexname);
			}
			break;

		case T_VacuumStmt:
			PS_SET_STATUS(commandTag = "VACUUM");
			CHECK_IF_ABORTED();
			vacuum(((VacuumStmt *) parsetree)->vacrel,
				   ((VacuumStmt *) parsetree)->verbose,
				   ((VacuumStmt *) parsetree)->analyze,
				   ((VacuumStmt *) parsetree)->va_spec);
			break;

		case T_ExplainStmt:
			{
				ExplainStmt *stmt = (ExplainStmt *) parsetree;

				PS_SET_STATUS(commandTag = "EXPLAIN");
				CHECK_IF_ABORTED();

				ExplainQuery(stmt->query, stmt->verbose, dest);
			}
			break;

#ifdef NOT_USED

			/*
			 * ******************************** Tioga-related statements *******************************
			 */
		case T_RecipeStmt:
			{
				RecipeStmt *stmt = (RecipeStmt *) parsetree;

				PS_SET_STATUS(commandTag = "EXECUTE RECIPE");
				CHECK_IF_ABORTED();
				beginRecipe(stmt);
			}
			break;
#endif

			/*
			 * ******************************** set variable statements *******************************
			 */
		case T_VariableSetStmt:
			{
				VariableSetStmt *n = (VariableSetStmt *) parsetree;

				SetPGVariable(n->name, n->value);
				PS_SET_STATUS(commandTag = "SET VARIABLE");
			}
			break;

		case T_VariableShowStmt:
			{
				VariableShowStmt *n = (VariableShowStmt *) parsetree;

				GetPGVariable(n->name);
				PS_SET_STATUS(commandTag = "SHOW VARIABLE");
			}
			break;

		case T_VariableResetStmt:
			{
				VariableResetStmt *n = (VariableResetStmt *) parsetree;

				ResetPGVariable(n->name);
				PS_SET_STATUS(commandTag = "RESET VARIABLE");
			}
			break;

			/*
			 * ******************************** TRIGGER statements *******************************
			 */
		case T_CreateTrigStmt:
			PS_SET_STATUS(commandTag = "CREATE");
			CHECK_IF_ABORTED();

			CreateTrigger((CreateTrigStmt *) parsetree);
			break;

		case T_DropTrigStmt:
			PS_SET_STATUS(commandTag = "DROP");
			CHECK_IF_ABORTED();

			DropTrigger((DropTrigStmt *) parsetree);
			break;

			/*
			 * ************* PROCEDURAL LANGUAGE statements *****************
			 */
		case T_CreatePLangStmt:
			PS_SET_STATUS(commandTag = "CREATE");
			CHECK_IF_ABORTED();

			CreateProceduralLanguage((CreatePLangStmt *) parsetree);
			break;

		case T_DropPLangStmt:
			PS_SET_STATUS(commandTag = "DROP");
			CHECK_IF_ABORTED();

			DropProceduralLanguage((DropPLangStmt *) parsetree);
			break;

			/*
			 * ******************************** USER statements ****
			 *
			 */
		case T_CreateUserStmt:
			PS_SET_STATUS(commandTag = "CREATE USER");
			CHECK_IF_ABORTED();

			CreateUser((CreateUserStmt *) parsetree);
			break;

		case T_AlterUserStmt:
			PS_SET_STATUS(commandTag = "ALTER USER");
			CHECK_IF_ABORTED();

			AlterUser((AlterUserStmt *) parsetree);
			break;

		case T_DropUserStmt:
			PS_SET_STATUS(commandTag = "DROP USER");
			CHECK_IF_ABORTED();

			DropUser((DropUserStmt *) parsetree);
			break;

		case T_LockStmt:
			PS_SET_STATUS(commandTag = "LOCK TABLE");
			CHECK_IF_ABORTED();

			LockTableCommand((LockStmt *) parsetree);
			break;

		case T_ConstraintsSetStmt:
			PS_SET_STATUS(commandTag = "SET CONSTRAINTS");
			CHECK_IF_ABORTED();

			DeferredTriggerSetState((ConstraintsSetStmt *) parsetree);
			break;

        case T_CreateGroupStmt:
            PS_SET_STATUS(commandTag = "CREATE GROUP");
			CHECK_IF_ABORTED();

            CreateGroup((CreateGroupStmt *) parsetree);
            break;

        case T_AlterGroupStmt:
            PS_SET_STATUS(commandTag = "ALTER GROUP");
			CHECK_IF_ABORTED();

            AlterGroup((AlterGroupStmt *) parsetree, "ALTER GROUP");
            break;

        case T_DropGroupStmt:
            PS_SET_STATUS(commandTag = "DROP GROUP");
			CHECK_IF_ABORTED();

            DropGroup((DropGroupStmt *) parsetree);
            break;

		case T_ReindexStmt:
			{
				ReindexStmt *stmt = (ReindexStmt *) parsetree;

				PS_SET_STATUS(commandTag = "REINDEX");
				CHECK_IF_ABORTED();

				switch (stmt->reindexType)
				{
					case INDEX:
						relname = stmt->name;
						if (IsSystemRelationName(relname))
						{
							if (!allowSystemTableMods && IsSystemRelationName(relname))
							elog(ERROR, "class \"%s\" is a system catalog index",
								 relname);
							if (!IsIgnoringSystemIndexes())
								elog(ERROR, "class \"%s\" is a system catalog index",
								 relname);
						}
#ifndef NO_SECURITY
						if (!pg_ownercheck(userName, relname, RELNAME))
							elog(ERROR, "%s: %s", relname, aclcheck_error_strings[ACLCHECK_NOT_OWNER]);
#endif
						ReindexIndex(relname, stmt->force);
						break;
					case TABLE:
						relname = stmt->name;
						if (IsSystemRelationName(relname))
						{
							if (!allowSystemTableMods && IsSystemRelationName(relname))
							elog(ERROR, "class \"%s\" is a system catalog index",
								 relname);
							if (!IsIgnoringSystemIndexes())
								elog(ERROR, "class \"%s\" is a system catalog index",
								 relname);
						}
#ifndef NO_SECURITY
						if (!pg_ownercheck(userName, relname, RELNAME))
							elog(ERROR, "%s: %s", relname, aclcheck_error_strings[ACLCHECK_NOT_OWNER]);
#endif
						ReindexTable(relname, stmt->force);
						break;
					case DATABASE:
						relname = stmt->name;
						if (!allowSystemTableMods)
							elog(ERROR, "-O option is needed");
						if (!IsIgnoringSystemIndexes())
							elog(ERROR, "-P option is needed");
						ReindexDatabase(relname, stmt->force, false);
						break;
				}
				break;
			}
			break;
			/*
			 * ******************************** default ********************************
			 *
			 */
		default:
			elog(ERROR, "ProcessUtility: command #%d unsupported",
				 nodeTag(parsetree));
			break;
	}

	/* ----------------
	 *	tell fe/be or whatever that we're done.
	 * ----------------
	 */
	EndCommand(commandTag, dest);
}
