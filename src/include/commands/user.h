/*-------------------------------------------------------------------------
 *
 * user.h
 *	  Commands for manipulating users and groups.
 *
 *
 * $PostgreSQL: pgsql/src/include/commands/user.h,v 1.22 2003/11/29 22:40:59 pgsql Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef USER_H
#define USER_H

#include "fmgr.h"
#include "nodes/parsenodes.h"


extern char *group_getfilename(void);
extern char *user_getfilename(void);

extern void CreateUser(CreateUserStmt *stmt);
extern void AlterUser(AlterUserStmt *stmt);
extern void AlterUserSet(AlterUserSetStmt *stmt);
extern void DropUser(DropUserStmt *stmt);
extern void RenameUser(const char *oldname, const char *newname);

extern void CreateGroup(CreateGroupStmt *stmt);
extern void AlterGroup(AlterGroupStmt *stmt, const char *tag);
extern void DropGroup(DropGroupStmt *stmt);
extern void RenameGroup(const char *oldname, const char *newname);

extern Datum update_pg_pwd_and_pg_group(PG_FUNCTION_ARGS);

extern void AtEOXact_UpdatePasswordFile(bool isCommit);

#endif   /* USER_H */
