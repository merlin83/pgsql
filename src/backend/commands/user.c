/*-------------------------------------------------------------------------
 *
 * user.c--
 *	  use pg_exec_query to create a new user in the catalog
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>				/* for sprintf() */
#include <string.h>

#include <postgres.h>

#include <miscadmin.h>
#include <catalog/catname.h>
#include <catalog/pg_database.h>
#include <catalog/pg_user.h>
#include <libpq/crypt.h>
#include <access/heapam.h>
#include <access/xact.h>
#include <storage/bufmgr.h>
#include <storage/lmgr.h>
#include <tcop/tcopprot.h>
#include <utils/acl.h>
#include <utils/palloc.h>
#include <utils/rel.h>
#include <commands/user.h>

/*---------------------------------------------------------------------
 * UpdatePgPwdFile
 *
 * copy the modified contents of pg_user to a file used by the postmaster
 * for user authentication.  The file is stored as $PGDATA/pg_pwd.
 *---------------------------------------------------------------------
 */
static
void UpdatePgPwdFile(char* sql) {

  char*     filename;

  filename = crypt_getpwdfilename();
  sprintf(sql, "copy %s to '%s' using delimiters '#'", UserRelationName, filename);
  pg_exec_query(sql, (char**)NULL, (Oid*)NULL, 0);
}

/*---------------------------------------------------------------------
 * DefineUser
 *
 * Add the user to the pg_user relation, and if specified make sure the
 * user is specified in the desired groups of defined in pg_group.
 *---------------------------------------------------------------------
 */
void DefineUser(CreateUserStmt *stmt) {

  char*            pg_user;
  Relation         pg_user_rel;
  TupleDesc        pg_user_dsc;
  HeapScanDesc     scan;
  HeapTuple        tuple;
  Datum            datum;
  Buffer           buffer;
  char             sql[512];
  char*            sql_end;
  bool             exists = false,
                   n,
                   inblock;
  int              max_id = -1;

  if (!(inblock = IsTransactionBlock()))
    BeginTransactionBlock();

  /* Make sure the user attempting to create a user can insert into the pg_user
   * relation.
   */
  pg_user = GetPgUserName();
  if (pg_aclcheck(UserRelationName, pg_user, ACL_RD | ACL_WR | ACL_AP) != ACLCHECK_OK) {
    UserAbortTransactionBlock();
    elog(WARN, "defineUser: user \"%s\" does not have SELECT and INSERT privilege for \"%s\"",
               pg_user, UserRelationName);
    return;
  }

  /* Scan the pg_user relation to be certain the user doesn't already exist.
   */
  pg_user_rel = heap_openr(UserRelationName);
  pg_user_dsc = RelationGetTupleDescriptor(pg_user_rel);
  /* Secure a write lock on pg_user so we can be sure of what the next usesysid
   * should be.
   */
  RelationSetLockForWrite(pg_user_rel);

  scan = heap_beginscan(pg_user_rel, false, false, 0, NULL);
  while (HeapTupleIsValid(tuple = heap_getnext(scan, 0, &buffer))) {
    datum = heap_getattr(tuple, buffer, Anum_pg_user_usename, pg_user_dsc, &n);

    if (!exists && !strncmp((char*)datum, stmt->user, strlen(stmt->user)))
      exists = true;

    datum = heap_getattr(tuple, buffer, Anum_pg_user_usesysid, pg_user_dsc, &n);
    if ((int)datum > max_id)
      max_id = (int)datum;

    ReleaseBuffer(buffer);
  }
  heap_endscan(scan);

  if (exists) {
    RelationUnsetLockForWrite(pg_user_rel);
    heap_close(pg_user_rel);
    UserAbortTransactionBlock();
    elog(WARN, "defineUser: user \"%s\" has already been created", stmt->user);
    return;
  }

  /* Build the insert statment to be executed.
   */
  sprintf(sql, "insert into %s(usename,usesysid,usecreatedb,usetrace,usesuper,usecatupd,passwd", UserRelationName);
/*  if (stmt->password)
    strcat(sql, ",passwd"); -- removed so that insert empty string when no password */
  if (stmt->validUntil)
    strcat(sql, ",valuntil");

  sql_end = sql + strlen(sql);
  sprintf(sql_end, ") values('%s',%d", stmt->user, max_id + 1);
  if (stmt->createdb && *stmt->createdb)
    strcat(sql_end, ",'t','t'");
  else
    strcat(sql_end, ",'f','t'");
  if (stmt->createuser && *stmt->createuser)
    strcat(sql_end, ",'t','t'");
  else
    strcat(sql_end, ",'f','t'");
  sql_end += strlen(sql_end);
  if (stmt->password) {
    sprintf(sql_end, ",'%s'", stmt->password);
    sql_end += strlen(sql_end);
  } else {
    strcpy(sql_end, ",''");
    sql_end += strlen(sql_end);
  }
  if (stmt->validUntil) {
    sprintf(sql_end, ",'%s'", stmt->validUntil);
    sql_end += strlen(sql_end);
  }
  strcat(sql_end, ")");

  pg_exec_query(sql, (char**)NULL, (Oid*)NULL, 0);

  /* Add the stuff here for groups.
   */

  RelationUnsetLockForWrite(pg_user_rel);
  heap_close(pg_user_rel);

  UpdatePgPwdFile(sql);

  if (IsTransactionBlock() && !inblock)
    EndTransactionBlock();
}


extern void AlterUser(AlterUserStmt *stmt) {

  char*            pg_user;
  Relation         pg_user_rel;
  TupleDesc        pg_user_dsc;
  HeapScanDesc     scan;
  HeapTuple        tuple;
  Datum            datum;
  Buffer           buffer;
  char             sql[512];
  char*            sql_end;
  bool             exists = false,
                   n,
                   inblock;

  if (!(inblock = IsTransactionBlock()))
    BeginTransactionBlock();

  /* Make sure the user attempting to create a user can insert into the pg_user
   * relation.
   */
  pg_user = GetPgUserName();
  if (pg_aclcheck(UserRelationName, pg_user, ACL_RD | ACL_WR) != ACLCHECK_OK) {
    UserAbortTransactionBlock();
    elog(WARN, "alterUser: user \"%s\" does not have SELECT and UPDATE privilege for \"%s\"",
               pg_user, UserRelationName);
    return;
  }

  /* Scan the pg_user relation to be certain the user exists.
   */
  pg_user_rel = heap_openr(UserRelationName);
  pg_user_dsc = RelationGetTupleDescriptor(pg_user_rel);

  scan = heap_beginscan(pg_user_rel, false, false, 0, NULL);
  while (HeapTupleIsValid(tuple = heap_getnext(scan, 0, &buffer))) {
    datum = heap_getattr(tuple, buffer, Anum_pg_user_usename, pg_user_dsc, &n);

    if (!strncmp((char*)datum, stmt->user, strlen(stmt->user))) {
      exists = true;
      ReleaseBuffer(buffer);
      break;
    }
  }
  heap_endscan(scan);
  heap_close(pg_user_rel);

  if (!exists) {
    UserAbortTransactionBlock();
    elog(WARN, "alterUser: user \"%s\" does not exist", stmt->user);
    return;
  }

  /* Create the update statement to modify the user.
   */
  sprintf(sql, "update %s set", UserRelationName);
  sql_end = sql;
  if (stmt->password) {
    sql_end += strlen(sql_end);
    sprintf(sql_end, " passwd = '%s'", stmt->password);
  }
  if (stmt->createdb) {
    if (sql_end != sql)
      strcat(sql_end, ",");
    sql_end += strlen(sql_end);
    if (*stmt->createdb)
      strcat(sql_end, " usecreatedb = 't'");
    else
      strcat(sql_end, " usecreatedb = 'f'");
  }
  if (stmt->createuser) {
    if (sql_end != sql)
      strcat(sql_end, ",");
    sql_end += strlen(sql_end);
    if (*stmt->createuser)
      strcat(sql_end, " usesuper = 't'");
    else
      strcat(sql_end, " usesuper = 'f'");
  }
  if (stmt->validUntil) {
    if (sql_end != sql)
      strcat(sql_end, ",");
    sql_end += strlen(sql_end);
    sprintf(sql_end, " valuntil = '%s'", stmt->validUntil);
  }
  if (sql_end != sql) {
    sql_end += strlen(sql_end);
    sprintf(sql_end, " where usename = '%s'", stmt->user);
    pg_exec_query(sql, (char**)NULL, (Oid*)NULL, 0);
  }

  /* do the pg_group stuff here */

  UpdatePgPwdFile(sql);

  if (IsTransactionBlock() && !inblock)
    EndTransactionBlock();
}


extern void RemoveUser(char* user) {

  char*            pg_user;
  Relation         pg_rel;
  TupleDesc        pg_dsc;
  HeapScanDesc     scan;
  HeapTuple        tuple;
  Datum            datum;
  Buffer           buffer;
  char             sql[256];
  bool             n,
                   inblock;
  int              usesysid = -1,
                   ndbase = 0;
  char**           dbase = NULL;

  if (!(inblock = IsTransactionBlock()))
    BeginTransactionBlock();

  /* Make sure the user attempting to create a user can delete from the pg_user
   * relation.
   */
  pg_user = GetPgUserName();
  if (pg_aclcheck(UserRelationName, pg_user, ACL_RD | ACL_WR) != ACLCHECK_OK) {
    UserAbortTransactionBlock();
    elog(WARN, "removeUser: user \"%s\" does not have SELECT and DELETE privilege for \"%s\"",
               pg_user, UserRelationName);
    return;
  }

  /* Perform a scan of the pg_user relation to find the usesysid of the user to
   * be deleted.  If it is not found, then return a warning message.
   */
  pg_rel = heap_openr(UserRelationName);
  pg_dsc = RelationGetTupleDescriptor(pg_rel);

  scan = heap_beginscan(pg_rel, false, false, 0, NULL);
  while (HeapTupleIsValid(tuple = heap_getnext(scan, 0, &buffer))) {
    datum = heap_getattr(tuple, buffer, Anum_pg_user_usename, pg_dsc, &n);

    if (!strncmp((char*)datum, user, strlen(user))) {
      usesysid = (int)heap_getattr(tuple, buffer, Anum_pg_user_usesysid, pg_dsc, &n);
      ReleaseBuffer(buffer);
      break;
    }
    ReleaseBuffer(buffer);
  }
  heap_endscan(scan);
  heap_close(pg_rel);

  if (usesysid == -1) {
    UserAbortTransactionBlock();
    elog(WARN, "removeUser: user \"%s\" does not exist", user);
    return;
  }

  /* Perform a scan of the pg_database relation to find the databases owned by
   * usesysid.  Then drop them.
   */
  pg_rel = heap_openr(DatabaseRelationName);
  pg_dsc = RelationGetTupleDescriptor(pg_rel);

  scan = heap_beginscan(pg_rel, false, false, 0, NULL);
  while (HeapTupleIsValid(tuple = heap_getnext(scan, 0, &buffer))) {
    datum = heap_getattr(tuple, buffer, Anum_pg_database_datdba, pg_dsc, &n);

    if ((int)datum == usesysid) {
      datum = heap_getattr(tuple, buffer, Anum_pg_database_datname, pg_dsc, &n);
      if (memcmp((void*)datum, "template1", 9)) {
        dbase = (char**)repalloc((void*)dbase, sizeof(char*) * (ndbase + 1));
        dbase[ndbase] = (char*)palloc(NAMEDATALEN + 1);
        memcpy((void*)dbase[ndbase], (void*)datum, NAMEDATALEN);
        dbase[ndbase++][NAMEDATALEN] = '\0';
      }
    }
    ReleaseBuffer(buffer);
  }
  heap_endscan(scan);
  heap_close(pg_rel);

  while (ndbase--) {
    elog(NOTICE, "Dropping database %s", dbase[ndbase]);
    sprintf(sql, "drop database %s", dbase[ndbase]);
    pfree((void*)dbase[ndbase]);
    pg_exec_query(sql, (char**)NULL, (Oid*)NULL, 0);
  }
  if (dbase)
    pfree((void*)dbase);

  /* Since pg_user is global over all databases, one of two things must be done
   * to insure complete consistency.  First, pg_user could be made non-global.
   * This would elminate the code above for deleting database and would require
   * the addition of code to delete tables, views, etc owned by the user.
   *
   * The second option would be to create a means of deleting tables, view,
   * etc. owned by the user from other databases.  Pg_user is global and so
   * this must be done at some point.
   *
   * Let us not forget that the user should be removed from the pg_groups also.
   *
   * Todd A. Brandys 11/18/1997
   *
   */

  /* Remove the user from the pg_user table
   */
  sprintf(sql, "delete from %s where usename = '%s'", UserRelationName, user);
  pg_exec_query(sql, (char**)NULL, (Oid*)NULL, 0);

  UpdatePgPwdFile(sql);

  if (IsTransactionBlock() && !inblock)
    EndTransactionBlock();
}
