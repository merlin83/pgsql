/*-------------------------------------------------------------------------
 *
 * user.c
 *	  Commands for manipulating users and groups.
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/commands/user.c,v 1.101 2002-05-17 01:19:17 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_database.h"
#include "catalog/pg_shadow.h"
#include "catalog/pg_group.h"
#include "catalog/indexing.h"
#include "commands/user.h"
#include "libpq/crypt.h"
#include "miscadmin.h"
#include "storage/pmsignal.h"
#include "utils/acl.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


extern bool Password_encryption;

static void CheckPgUserAclNotNull(void);
static void UpdateGroupMembership(Relation group_rel, HeapTuple group_tuple,
								  List *members);
static IdList *IdListToArray(List *members);
static List *IdArrayToList(IdList *oldarray);


/*
 *	fputs_quote
 *
 *	Outputs string in quotes, with double-quotes duplicated.
 *	We could use quote_ident(), but that expects varlena.
 */
static void fputs_quote(char *str, FILE *fp)
{
	fputc('"', fp);
	while (*str)
	{
		fputc(*str, fp);
		if (*str == '"')
			fputc('"', fp);
		str++;
	}
	fputc('"', fp);
}



/*
 * group_getfilename --- get full pathname of group file
 *
 * Note that result string is palloc'd, and should be freed by the caller.
 */
char *
group_getfilename(void)
{
	int			bufsize;
	char	   *pfnam;

	bufsize = strlen(DataDir) + strlen("/global/") +
			  strlen(USER_GROUP_FILE) + 1;
	pfnam = (char *) palloc(bufsize);
	snprintf(pfnam, bufsize, "%s/global/%s", DataDir, USER_GROUP_FILE);

	return pfnam;
}



/*
 * Get full pathname of password file.
 * Note that result string is palloc'd, and should be freed by the caller.
 */
char *
user_getfilename(void)
{
	int			bufsize;
	char	   *pfnam;

	bufsize = strlen(DataDir) + strlen("/global/") +
			  strlen(PWD_FILE) + 1;
	pfnam = (char *) palloc(bufsize);
	snprintf(pfnam, bufsize, "%s/global/%s", DataDir, PWD_FILE);

	return pfnam;
}



/*
 * write_group_file for trigger update_pg_pwd_and_pg_group
 */
static void
write_group_file(Relation urel, Relation grel)
{
	char	   *filename,
			   *tempname;
	int			bufsize;
	FILE	   *fp;
	mode_t		oumask;
	HeapScanDesc scan;
	HeapTuple	tuple;
	TupleDesc	dsc = RelationGetDescr(grel);

	/*
	 * Create a temporary filename to be renamed later.  This prevents the
	 * backend from clobbering the pg_group file while the postmaster might
	 * be reading from it.
	 */
	filename = group_getfilename();
	bufsize = strlen(filename) + 12;
	tempname = (char *) palloc(bufsize);

	snprintf(tempname, bufsize, "%s.%d", filename, MyProcPid);
	oumask = umask((mode_t) 077);
	fp = AllocateFile(tempname, "w");
	umask(oumask);
	if (fp == NULL)
		elog(ERROR, "write_group_file: unable to write %s: %m", tempname);

	/* read table */
	scan = heap_beginscan(grel, false, SnapshotSelf, 0, NULL);
	while (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
	{
		Datum		datum, grolist_datum;
		bool		isnull;
		char	   *groname;
		IdList	   *grolist_p;
		AclId	   *aidp;
		int			i, j,
					num;
		char 	   *usename;
		bool		first_user = true;

		datum = heap_getattr(tuple, Anum_pg_group_groname, dsc, &isnull);
		/* ignore NULL groupnames --- shouldn't happen */
		if (isnull)
			continue;
		groname = NameStr(*DatumGetName(datum));

		/*
		 * Check for illegal characters in the group name.
		 */
		i = strcspn(groname, "\n");
		if (groname[i] != '\0')
		{
			elog(LOG, "Invalid group name '%s'", groname);
			continue;
		}

		grolist_datum = heap_getattr(tuple, Anum_pg_group_grolist, dsc, &isnull);
		/* Ignore NULL group lists */
		if (isnull)
			continue;

		/* be sure the IdList is not toasted */
		grolist_p = DatumGetIdListP(grolist_datum);

		/* scan grolist */
		num = IDLIST_NUM(grolist_p);
		aidp = IDLIST_DAT(grolist_p);
		for (i = 0; i < num; ++i)
		{
			tuple = SearchSysCache(SHADOWSYSID,
								   PointerGetDatum(aidp[i]),
								   0, 0, 0);
			if (HeapTupleIsValid(tuple))
			{
				usename = NameStr(((Form_pg_shadow) GETSTRUCT(tuple))->usename);

				/*
				 * Check for illegal characters in the user name.
				 */
				j = strcspn(usename, "\n");
				if (usename[j] != '\0')
				{
					elog(LOG, "Invalid user name '%s'", usename);
					continue;
				}

				/* File format is:
				 *		"dbname"	"user1" "user2" "user3"
				 */
				if (first_user)
				{
					fputs_quote(groname, fp);
					fputs("\t", fp);
				}
				else
					fputs(" ", fp);

				first_user = false;
				fputs_quote(usename, fp);

				ReleaseSysCache(tuple);
			}
		}
		if (!first_user)
			fputs("\n", fp);
		/* if IdList was toasted, free detoasted copy */
		if ((Pointer) grolist_p != DatumGetPointer(grolist_datum))
			pfree(grolist_p);
	}
	heap_endscan(scan);

	fflush(fp);
	if (ferror(fp))
		elog(ERROR, "%s: %m", tempname);
	FreeFile(fp);

	/*
	 * Rename the temp file to its final name, deleting the old pg_pwd. We
	 * expect that rename(2) is an atomic action.
	 */
	if (rename(tempname, filename))
		elog(ERROR, "rename %s to %s: %m", tempname, filename);

	pfree((void *) tempname);
	pfree((void *) filename);
}



/*
 * write_password_file for trigger update_pg_pwd_and_pg_group
 *
 * copy the modified contents of pg_shadow to a file used by the postmaster
 * for user authentication.  The file is stored as $PGDATA/global/pg_pwd.
 *
 * This function set is both a trigger function for direct updates to pg_shadow
 * as well as being called directly from create/alter/drop user.
 *
 * We raise an error to force transaction rollback if we detect an illegal
 * username or password --- illegal being defined as values that would
 * mess up the pg_pwd parser.
 */
static void
write_user_file(Relation urel)
{
	char	   *filename,
			   *tempname;
	int			bufsize;
	FILE	   *fp;
	mode_t		oumask;
	HeapScanDesc scan;
	HeapTuple	tuple;
	TupleDesc	dsc = RelationGetDescr(urel);

	/*
	 * Create a temporary filename to be renamed later.  This prevents the
	 * backend from clobbering the pg_pwd file while the postmaster might
	 * be reading from it.
	 */
	filename = user_getfilename();
	bufsize = strlen(filename) + 12;
	tempname = (char *) palloc(bufsize);

	snprintf(tempname, bufsize, "%s.%d", filename, MyProcPid);
	oumask = umask((mode_t) 077);
	fp = AllocateFile(tempname, "w");
	umask(oumask);
	if (fp == NULL)
		elog(ERROR, "write_password_file: unable to write %s: %m", tempname);

	/* read table */
	scan = heap_beginscan(urel, false, SnapshotSelf, 0, NULL);
	while (HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
	{
		Datum		datum;
		bool		isnull;
		char	   *usename,
				   *passwd,
				   *valuntil;
		int			i;

		datum = heap_getattr(tuple, Anum_pg_shadow_usename, dsc, &isnull);
		/* ignore NULL usernames (shouldn't happen) */
		if (isnull)
			continue;
		usename = NameStr(*DatumGetName(datum));

		datum = heap_getattr(tuple, Anum_pg_shadow_passwd, dsc, &isnull);

		/*
		 * It can be argued that people having a null password shouldn't
		 * be allowed to connect under password authentication, because
		 * they need to have a password set up first. If you think
		 * assuming an empty password in that case is better, change this
		 * logic to look something like the code for valuntil.
		 */
		if (isnull)
			continue;

		passwd = DatumGetCString(DirectFunctionCall1(textout, datum));

		datum = heap_getattr(tuple, Anum_pg_shadow_valuntil, dsc, &isnull);
		if (isnull)
			valuntil = pstrdup("");
		else
			valuntil = DatumGetCString(DirectFunctionCall1(nabstimeout, datum));

		/*
		 * Check for illegal characters in the username and password.
		 */
		i = strcspn(usename, "\n");
		if (usename[i] != '\0')
			elog(ERROR, "Invalid user name '%s'", usename);
		i = strcspn(passwd, "\n");
		if (passwd[i] != '\0')
			elog(ERROR, "Invalid user password '%s'", passwd);

		/*
		 * The extra columns we emit here are not really necessary. To
		 * remove them, the parser in backend/libpq/crypt.c would need to
		 * be adjusted.
		 */
		fputs_quote(usename, fp);
		fputs(" ", fp);
		fputs_quote(passwd, fp);
		fputs(" ", fp);
		fputs_quote(valuntil, fp);
		fputs("\n", fp);

		pfree(passwd);
		pfree(valuntil);
	}
	heap_endscan(scan);

	fflush(fp);
	if (ferror(fp))
		elog(ERROR, "%s: %m", tempname);
	FreeFile(fp);

	/*
	 * Rename the temp file to its final name, deleting the old pg_pwd. We
	 * expect that rename(2) is an atomic action.
	 */
	if (rename(tempname, filename))
		elog(ERROR, "rename %s to %s: %m", tempname, filename);

	pfree((void *) tempname);
	pfree((void *) filename);
}



/* This is the wrapper for triggers. */
Datum
update_pg_pwd_and_pg_group(PG_FUNCTION_ARGS)
{
	/*
	 * ExclusiveLock ensures no one modifies pg_shadow while we read it,
	 * and that only one backend rewrites the flat file at a time.	It's
	 * OK to allow normal reads of pg_shadow in parallel, however.
	 */
	Relation	urel = heap_openr(ShadowRelationName, ExclusiveLock);
	Relation	grel = heap_openr(GroupRelationName, ExclusiveLock);

	write_user_file(urel);
	write_group_file(urel, grel);
	/* OK to release lock, since we did not modify the relation */
	heap_close(grel, ExclusiveLock);
	heap_close(urel, ExclusiveLock);

	/*
	 * Signal the postmaster to reload its password & group-file cache.
	 */
	SendPostmasterSignal(PMSIGNAL_PASSWORD_CHANGE);

	return PointerGetDatum(NULL);
}



/*
 * CREATE USER
 */
void
CreateUser(CreateUserStmt *stmt)
{
	Relation	pg_shadow_rel;
	TupleDesc	pg_shadow_dsc;
	HeapScanDesc scan;
	HeapTuple	tuple;
	Datum		new_record[Natts_pg_shadow];
	char		new_record_nulls[Natts_pg_shadow];
	bool		user_exists = false,
				sysid_exists = false,
				havesysid = false;
	int			max_id;
	List	   *item,
			   *option;
	char	   *password = NULL;	/* PostgreSQL user password */
	bool		encrypt_password = Password_encryption; /* encrypt password? */
	char		encrypted_password[MD5_PASSWD_LEN + 1];
	int			sysid = 0;		/* PgSQL system id (valid if havesysid) */
	bool		createdb = false;		/* Can the user create databases? */
	bool		createuser = false;		/* Can this user create users? */
	List	   *groupElts = NIL;	/* The groups the user is a member of */
	char	   *validUntil = NULL;		/* The time the login is valid
										 * until */
	DefElem    *dpassword = NULL;
	DefElem    *dsysid = NULL;
	DefElem    *dcreatedb = NULL;
	DefElem    *dcreateuser = NULL;
	DefElem    *dgroupElts = NULL;
	DefElem    *dvalidUntil = NULL;

	/* Extract options from the statement node tree */
	foreach(option, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "password") == 0 ||
			strcmp(defel->defname, "encryptedPassword") == 0 ||
			strcmp(defel->defname, "unencryptedPassword") == 0)
		{
			if (dpassword)
				elog(ERROR, "CREATE USER: conflicting options");
			dpassword = defel;
			if (strcmp(defel->defname, "encryptedPassword") == 0)
				encrypt_password = true;
			else if (strcmp(defel->defname, "unencryptedPassword") == 0)
				encrypt_password = false;
		}
		else if (strcmp(defel->defname, "sysid") == 0)
		{
			if (dsysid)
				elog(ERROR, "CREATE USER: conflicting options");
			dsysid = defel;
		}
		else if (strcmp(defel->defname, "createdb") == 0)
		{
			if (dcreatedb)
				elog(ERROR, "CREATE USER: conflicting options");
			dcreatedb = defel;
		}
		else if (strcmp(defel->defname, "createuser") == 0)
		{
			if (dcreateuser)
				elog(ERROR, "CREATE USER: conflicting options");
			dcreateuser = defel;
		}
		else if (strcmp(defel->defname, "groupElts") == 0)
		{
			if (dgroupElts)
				elog(ERROR, "CREATE USER: conflicting options");
			dgroupElts = defel;
		}
		else if (strcmp(defel->defname, "validUntil") == 0)
		{
			if (dvalidUntil)
				elog(ERROR, "CREATE USER: conflicting options");
			dvalidUntil = defel;
		}
		else
			elog(ERROR, "CREATE USER: option \"%s\" not recognized",
				 defel->defname);
	}

	if (dcreatedb)
		createdb = intVal(dcreatedb->arg) != 0;
	if (dcreateuser)
		createuser = intVal(dcreateuser->arg) != 0;
	if (dsysid)
	{
		sysid = intVal(dsysid->arg);
		if (sysid <= 0)
			elog(ERROR, "user id must be positive");
		havesysid = true;
	}
	if (dvalidUntil)
		validUntil = strVal(dvalidUntil->arg);
	if (dpassword)
		password = strVal(dpassword->arg);
	if (dgroupElts)
		groupElts = (List *) dgroupElts->arg;

	/* Check some permissions first */
	if (password)
		CheckPgUserAclNotNull();

	if (!superuser())
		elog(ERROR, "CREATE USER: permission denied");

	if (strcmp(stmt->user, "public") == 0)
		elog(ERROR, "CREATE USER: user name \"%s\" is reserved",
			 stmt->user);

	/*
	 * Scan the pg_shadow relation to be certain the user or id doesn't
	 * already exist.  Note we secure exclusive lock, because we also need
	 * to be sure of what the next usesysid should be, and we need to
	 * protect our update of the flat password file.
	 */
	pg_shadow_rel = heap_openr(ShadowRelationName, ExclusiveLock);
	pg_shadow_dsc = RelationGetDescr(pg_shadow_rel);

	scan = heap_beginscan(pg_shadow_rel, false, SnapshotNow, 0, NULL);
	max_id = 99;				/* start auto-assigned ids at 100 */
	while (!user_exists && !sysid_exists &&
		   HeapTupleIsValid(tuple = heap_getnext(scan, 0)))
	{
		Form_pg_shadow shadow_form = (Form_pg_shadow) GETSTRUCT(tuple);
		int32		this_sysid;

		user_exists = (strcmp(NameStr(shadow_form->usename), stmt->user) == 0);

		this_sysid = shadow_form->usesysid;
		if (havesysid)			/* customized id wanted */
			sysid_exists = (this_sysid == sysid);
		else
		{
			/* pick 1 + max */
			if (this_sysid > max_id)
				max_id = this_sysid;
		}
	}
	heap_endscan(scan);

	if (user_exists)
		elog(ERROR, "CREATE USER: user name \"%s\" already exists",
			 stmt->user);
	if (sysid_exists)
		elog(ERROR, "CREATE USER: sysid %d is already assigned", sysid);

	/* If no sysid given, use max existing id + 1 */
	if (!havesysid)
		sysid = max_id + 1;

	/*
	 * Build a tuple to insert
	 */
	MemSet(new_record, 0, sizeof(new_record));
	MemSet(new_record_nulls, ' ', sizeof(new_record_nulls));

	new_record[Anum_pg_shadow_usename - 1] =
		DirectFunctionCall1(namein, CStringGetDatum(stmt->user));
	new_record[Anum_pg_shadow_usesysid - 1] = Int32GetDatum(sysid);
	AssertState(BoolIsValid(createdb));
	new_record[Anum_pg_shadow_usecreatedb - 1] = BoolGetDatum(createdb);
	new_record[Anum_pg_shadow_usetrace - 1] = BoolGetDatum(false);
	AssertState(BoolIsValid(createuser));
	new_record[Anum_pg_shadow_usesuper - 1] = BoolGetDatum(createuser);
	/* superuser gets catupd right by default */
	new_record[Anum_pg_shadow_usecatupd - 1] = BoolGetDatum(createuser);

	if (password)
	{
		if (!encrypt_password || isMD5(password))
			new_record[Anum_pg_shadow_passwd - 1] =
				DirectFunctionCall1(textin, CStringGetDatum(password));
		else
		{
			if (!EncryptMD5(password, stmt->user, strlen(stmt->user),
							encrypted_password))
				elog(ERROR, "CREATE USER: password encryption failed");
			new_record[Anum_pg_shadow_passwd - 1] =
				DirectFunctionCall1(textin, CStringGetDatum(encrypted_password));
		}
	}
	else
		new_record_nulls[Anum_pg_shadow_passwd - 1] = 'n';

	if (validUntil)
		new_record[Anum_pg_shadow_valuntil - 1] =
			DirectFunctionCall1(nabstimein, CStringGetDatum(validUntil));
	else
		new_record_nulls[Anum_pg_shadow_valuntil - 1] = 'n';

	new_record_nulls[Anum_pg_shadow_useconfig - 1] = 'n';

	tuple = heap_formtuple(pg_shadow_dsc, new_record, new_record_nulls);

	/*
	 * Insert new record in the pg_shadow table
	 */
	heap_insert(pg_shadow_rel, tuple);

	/*
	 * Update indexes
	 */
	if (RelationGetForm(pg_shadow_rel)->relhasindex)
	{
		Relation	idescs[Num_pg_shadow_indices];

		CatalogOpenIndices(Num_pg_shadow_indices,
						   Name_pg_shadow_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_shadow_indices, pg_shadow_rel,
						   tuple);
		CatalogCloseIndices(Num_pg_shadow_indices, idescs);
	}

	/*
	 * Add the user to the groups specified. We'll just call the below
	 * AlterGroup for this.
	 */
	foreach(item, groupElts)
	{
		AlterGroupStmt ags;

		ags.name = strVal(lfirst(item));		/* the group name to add
												 * this in */
		ags.action = +1;
		ags.listUsers = makeList1(makeInteger(sysid));
		AlterGroup(&ags, "CREATE USER");
	}

	/*
	 * Now we can clean up; but keep lock until commit.
	 */
	heap_close(pg_shadow_rel, NoLock);

	/*
	 * Write the updated pg_shadow and pg_group data to the flat file.
	 */
	update_pg_pwd_and_pg_group(NULL);
}



/*
 * ALTER USER
 */
void
AlterUser(AlterUserStmt *stmt)
{
	Datum		new_record[Natts_pg_shadow];
	char		new_record_nulls[Natts_pg_shadow];
	char		new_record_repl[Natts_pg_shadow];
	Relation	pg_shadow_rel;
	TupleDesc	pg_shadow_dsc;
	HeapTuple	tuple,
				new_tuple;
	List	   *option;
	char	   *password = NULL;	/* PostgreSQL user password */
	bool		encrypt_password = Password_encryption; /* encrypt password? */
	char		encrypted_password[MD5_PASSWD_LEN + 1];
	int			createdb = -1;	/* Can the user create databases? */
	int			createuser = -1;	/* Can this user create users? */
	char	   *validUntil = NULL;		/* The time the login is valid
										 * until */
	DefElem    *dpassword = NULL;
	DefElem    *dcreatedb = NULL;
	DefElem    *dcreateuser = NULL;
	DefElem    *dvalidUntil = NULL;

	/* Extract options from the statement node tree */
	foreach(option, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "password") == 0 ||
			strcmp(defel->defname, "encryptedPassword") == 0 ||
			strcmp(defel->defname, "unencryptedPassword") == 0)
		{
			if (dpassword)
				elog(ERROR, "ALTER USER: conflicting options");
			dpassword = defel;
			if (strcmp(defel->defname, "encryptedPassword") == 0)
				encrypt_password = true;
			else if (strcmp(defel->defname, "unencryptedPassword") == 0)
				encrypt_password = false;
		}
		else if (strcmp(defel->defname, "createdb") == 0)
		{
			if (dcreatedb)
				elog(ERROR, "ALTER USER: conflicting options");
			dcreatedb = defel;
		}
		else if (strcmp(defel->defname, "createuser") == 0)
		{
			if (dcreateuser)
				elog(ERROR, "ALTER USER: conflicting options");
			dcreateuser = defel;
		}
		else if (strcmp(defel->defname, "validUntil") == 0)
		{
			if (dvalidUntil)
				elog(ERROR, "ALTER USER: conflicting options");
			dvalidUntil = defel;
		}
		else
			elog(ERROR, "ALTER USER: option \"%s\" not recognized",
				 defel->defname);
	}

	if (dcreatedb)
		createdb = intVal(dcreatedb->arg);
	if (dcreateuser)
		createuser = intVal(dcreateuser->arg);
	if (dvalidUntil)
		validUntil = strVal(dvalidUntil->arg);
	if (dpassword)
		password = strVal(dpassword->arg);

	if (password)
		CheckPgUserAclNotNull();

	/* must be superuser or just want to change your own password */
	if (!superuser() &&
		!(createdb < 0 &&
		  createuser < 0 &&
		  !validUntil &&
		  password &&
		  strcmp(GetUserName(GetUserId()), stmt->user) == 0))
		elog(ERROR, "ALTER USER: permission denied");

	/* changes to the flat password file cannot be rolled back */
	if (IsTransactionBlock() && password)
		elog(NOTICE, "ALTER USER: password changes cannot be rolled back");

	/*
	 * Scan the pg_shadow relation to be certain the user exists. Note we
	 * secure exclusive lock to protect our update of the flat password
	 * file.
	 */
	pg_shadow_rel = heap_openr(ShadowRelationName, ExclusiveLock);
	pg_shadow_dsc = RelationGetDescr(pg_shadow_rel);

	tuple = SearchSysCache(SHADOWNAME,
						   PointerGetDatum(stmt->user),
						   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "ALTER USER: user \"%s\" does not exist", stmt->user);

	/*
	 * Build an updated tuple, perusing the information just obtained
	 */
	MemSet(new_record, 0, sizeof(new_record));
	MemSet(new_record_nulls, ' ', sizeof(new_record_nulls));
	MemSet(new_record_repl, ' ', sizeof(new_record_repl));

	new_record[Anum_pg_shadow_usename - 1] = DirectFunctionCall1(namein,
											CStringGetDatum(stmt->user));
	new_record_repl[Anum_pg_shadow_usename - 1] = 'r';

	/* createdb */
	if (createdb >= 0)
	{
		new_record[Anum_pg_shadow_usecreatedb - 1] = BoolGetDatum(createdb > 0);
		new_record_repl[Anum_pg_shadow_usecreatedb - 1] = 'r';
	}

	/*
	 * createuser (superuser) and catupd
	 *
	 * XXX It's rather unclear how to handle catupd.  It's probably best to
	 * keep it equal to the superuser status, otherwise you could end up
	 * with a situation where no existing superuser can alter the
	 * catalogs, including pg_shadow!
	 */
	if (createuser >= 0)
	{
		new_record[Anum_pg_shadow_usesuper - 1] = BoolGetDatum(createuser > 0);
		new_record_repl[Anum_pg_shadow_usesuper - 1] = 'r';

		new_record[Anum_pg_shadow_usecatupd - 1] = BoolGetDatum(createuser > 0);
		new_record_repl[Anum_pg_shadow_usecatupd - 1] = 'r';
	}

	/* password */
	if (password)
	{
		if (!encrypt_password || isMD5(password))
			new_record[Anum_pg_shadow_passwd - 1] =
				DirectFunctionCall1(textin, CStringGetDatum(password));
		else
		{
			if (!EncryptMD5(password, stmt->user, strlen(stmt->user),
							encrypted_password))
				elog(ERROR, "CREATE USER: password encryption failed");
			new_record[Anum_pg_shadow_passwd - 1] =
				DirectFunctionCall1(textin, CStringGetDatum(encrypted_password));
		}
		new_record_repl[Anum_pg_shadow_passwd - 1] = 'r';
	}

	/* valid until */
	if (validUntil)
	{
		new_record[Anum_pg_shadow_valuntil - 1] =
			DirectFunctionCall1(nabstimein, CStringGetDatum(validUntil));
		new_record_repl[Anum_pg_shadow_valuntil - 1] = 'r';
	}

	new_tuple = heap_modifytuple(tuple, pg_shadow_rel, new_record,
								 new_record_nulls, new_record_repl);
	simple_heap_update(pg_shadow_rel, &tuple->t_self, new_tuple);

	/* Update indexes */
	if (RelationGetForm(pg_shadow_rel)->relhasindex)
	{
		Relation	idescs[Num_pg_shadow_indices];

		CatalogOpenIndices(Num_pg_shadow_indices,
						   Name_pg_shadow_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_shadow_indices, pg_shadow_rel,
						   new_tuple);
		CatalogCloseIndices(Num_pg_shadow_indices, idescs);
	}

	ReleaseSysCache(tuple);
	heap_freetuple(new_tuple);

	/*
	 * Now we can clean up.
	 */
	heap_close(pg_shadow_rel, NoLock);

	/*
	 * Write the updated pg_shadow and pg_group data to the flat file.
	 */
	update_pg_pwd_and_pg_group(NULL);
}


/*
 * ALTER USER ... SET
 */
void
AlterUserSet(AlterUserSetStmt *stmt)
{
	char	   *valuestr;
	HeapTuple	oldtuple,
				newtuple;
	Relation	rel;
	Datum		repl_val[Natts_pg_shadow];
	char		repl_null[Natts_pg_shadow];
	char		repl_repl[Natts_pg_shadow];
	int			i;

	valuestr = flatten_set_variable_args(stmt->variable, stmt->value);

	/*
	 * RowExclusiveLock is sufficient, because we don't need to update
	 * the flat password file.
	 */
	rel = heap_openr(ShadowRelationName, RowExclusiveLock);
	oldtuple = SearchSysCache(SHADOWNAME,
							  PointerGetDatum(stmt->user),
							  0, 0, 0);
	if (!HeapTupleIsValid(oldtuple))
		elog(ERROR, "user \"%s\" does not exist", stmt->user);

	if (!(superuser()
		  || ((Form_pg_shadow) GETSTRUCT(oldtuple))->usesysid == GetUserId()))
		elog(ERROR, "permission denied");

	for (i = 0; i < Natts_pg_shadow; i++)
		repl_repl[i] = ' ';

	repl_repl[Anum_pg_shadow_useconfig-1] = 'r';
	if (strcmp(stmt->variable, "all")==0 && valuestr == NULL)
		/* RESET ALL */
		repl_null[Anum_pg_shadow_useconfig-1] = 'n';
	else
	{
		Datum datum;
		bool isnull;
		ArrayType *array;

		repl_null[Anum_pg_shadow_useconfig-1] = ' ';

		datum = SysCacheGetAttr(SHADOWNAME, oldtuple,
								Anum_pg_shadow_useconfig, &isnull);

		array = isnull ? ((ArrayType *) NULL) : DatumGetArrayTypeP(datum);

		if (valuestr)
			array = GUCArrayAdd(array, stmt->variable, valuestr);
		else
			array = GUCArrayDelete(array, stmt->variable);

		repl_val[Anum_pg_shadow_useconfig-1] = PointerGetDatum(array);
	}

	newtuple = heap_modifytuple(oldtuple, rel, repl_val, repl_null, repl_repl);
	simple_heap_update(rel, &oldtuple->t_self, newtuple);

	{
		Relation	idescs[Num_pg_shadow_indices];

		CatalogOpenIndices(Num_pg_shadow_indices, Name_pg_shadow_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_shadow_indices, rel, newtuple);
		CatalogCloseIndices(Num_pg_shadow_indices, idescs);
	}

	ReleaseSysCache(oldtuple);
	heap_close(rel, RowExclusiveLock);
}



/*
 * DROP USER
 */
void
DropUser(DropUserStmt *stmt)
{
	Relation	pg_shadow_rel;
	TupleDesc	pg_shadow_dsc;
	List	   *item;

	if (!superuser())
		elog(ERROR, "DROP USER: permission denied");

	if (IsTransactionBlock())
		elog(NOTICE, "DROP USER cannot be rolled back completely");

	/*
	 * Scan the pg_shadow relation to find the usesysid of the user to be
	 * deleted.  Note we secure exclusive lock, because we need to protect
	 * our update of the flat password file.
	 */
	pg_shadow_rel = heap_openr(ShadowRelationName, ExclusiveLock);
	pg_shadow_dsc = RelationGetDescr(pg_shadow_rel);

	foreach(item, stmt->users)
	{
		const char *user = strVal(lfirst(item));
		HeapTuple	tuple,
					tmp_tuple;
		Relation	pg_rel;
		TupleDesc	pg_dsc;
		ScanKeyData scankey;
		HeapScanDesc scan;
		int32		usesysid;

		tuple = SearchSysCache(SHADOWNAME,
							   PointerGetDatum(user),
							   0, 0, 0);
		if (!HeapTupleIsValid(tuple))
			elog(ERROR, "DROP USER: user \"%s\" does not exist%s", user,
				 (length(stmt->users) > 1) ? " (no users removed)" : "");

		usesysid = ((Form_pg_shadow) GETSTRUCT(tuple))->usesysid;

		if (usesysid == GetUserId())
			elog(ERROR, "current user cannot be dropped");
		if (usesysid == GetSessionUserId())
			elog(ERROR, "session user cannot be dropped");

		/*
		 * Check if user still owns a database. If so, error out.
		 *
		 * (It used to be that this function would drop the database
		 * automatically. This is not only very dangerous for people that
		 * don't read the manual, it doesn't seem to be the behaviour one
		 * would expect either.) -- petere 2000/01/14)
		 */
		pg_rel = heap_openr(DatabaseRelationName, AccessShareLock);
		pg_dsc = RelationGetDescr(pg_rel);

		ScanKeyEntryInitialize(&scankey, 0x0,
							   Anum_pg_database_datdba, F_INT4EQ,
							   Int32GetDatum(usesysid));

		scan = heap_beginscan(pg_rel, false, SnapshotNow, 1, &scankey);

		if (HeapTupleIsValid(tmp_tuple = heap_getnext(scan, 0)))
		{
			char	   *dbname;

			dbname = NameStr(((Form_pg_database) GETSTRUCT(tmp_tuple))->datname);
			elog(ERROR, "DROP USER: user \"%s\" owns database \"%s\", cannot be removed%s",
				 user, dbname,
				 (length(stmt->users) > 1) ? " (no users removed)" : "");
		}

		heap_endscan(scan);
		heap_close(pg_rel, AccessShareLock);

		/*
		 * Somehow we'd have to check for tables, views, etc. owned by the
		 * user as well, but those could be spread out over all sorts of
		 * databases which we don't have access to (easily).
		 */

		/*
		 * Remove the user from the pg_shadow table
		 */
		simple_heap_delete(pg_shadow_rel, &tuple->t_self);

		ReleaseSysCache(tuple);

		/*
		 * Remove user from groups
		 *
		 * try calling alter group drop user for every group
		 */
		pg_rel = heap_openr(GroupRelationName, ExclusiveLock);
		pg_dsc = RelationGetDescr(pg_rel);
		scan = heap_beginscan(pg_rel, false, SnapshotNow, 0, NULL);
		while (HeapTupleIsValid(tmp_tuple = heap_getnext(scan, 0)))
		{
			AlterGroupStmt ags;

			/* the group name from which to try to drop the user: */
			ags.name = pstrdup(NameStr(((Form_pg_group) GETSTRUCT(tmp_tuple))->groname));
			ags.action = -1;
			ags.listUsers = makeList1(makeInteger(usesysid));
			AlterGroup(&ags, "DROP USER");
		}
		heap_endscan(scan);
		heap_close(pg_rel, ExclusiveLock);

		/*
		 * Advance command counter so that later iterations of this loop
		 * will see the changes already made.  This is essential if, for
		 * example, we are trying to drop two users who are members of the
		 * same group --- the AlterGroup for the second user had better
		 * see the tuple updated from the first one.
		 */
		CommandCounterIncrement();
	}

	/*
	 * Now we can clean up.
	 */
	heap_close(pg_shadow_rel, NoLock);

	/*
	 * Write the updated pg_shadow and pg_group data to the flat file.
	 */
	update_pg_pwd_and_pg_group(NULL);
}



/*
 * CheckPgUserAclNotNull
 *
 * check to see if there is an ACL on pg_shadow
 */
static void
CheckPgUserAclNotNull(void)
{
	HeapTuple	htup;

	htup = SearchSysCache(RELOID,
						  ObjectIdGetDatum(RelOid_pg_shadow),
						  0, 0, 0);
	if (!HeapTupleIsValid(htup))
		elog(ERROR, "CheckPgUserAclNotNull: \"%s\" not found",
			 ShadowRelationName);

	if (heap_attisnull(htup, Anum_pg_class_relacl))
		elog(ERROR,
			 "To use passwords, you have to revoke permissions on %s "
			 "so normal users cannot read the passwords. "
			 "Try 'REVOKE ALL ON \"%s\" FROM PUBLIC'.",
			 ShadowRelationName, ShadowRelationName);

	ReleaseSysCache(htup);
}



/*
 * CREATE GROUP
 */
void
CreateGroup(CreateGroupStmt *stmt)
{
	Relation	pg_group_rel;
	HeapScanDesc scan;
	HeapTuple	tuple;
	TupleDesc	pg_group_dsc;
	bool		group_exists = false,
				sysid_exists = false,
				havesysid = false;
	int			max_id;
	Datum		new_record[Natts_pg_group];
	char		new_record_nulls[Natts_pg_group];
	List	   *item,
			   *option,
			   *newlist = NIL;
	IdList	   *grolist;
	int			sysid = 0;
	List	   *userElts = NIL;
	DefElem    *dsysid = NULL;
	DefElem    *duserElts = NULL;

	foreach(option, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "sysid") == 0)
		{
			if (dsysid)
				elog(ERROR, "CREATE GROUP: conflicting options");
			dsysid = defel;
		}
		else if (strcmp(defel->defname, "userElts") == 0)
		{
			if (duserElts)
				elog(ERROR, "CREATE GROUP: conflicting options");
			duserElts = defel;
		}
		else
			elog(ERROR, "CREATE GROUP: option \"%s\" not recognized",
				 defel->defname);
	}

	if (dsysid)
	{
		sysid = intVal(dsysid->arg);
		if (sysid <= 0)
			elog(ERROR, "group id must be positive");
		havesysid = true;
	}

	if (duserElts)
		userElts = (List *) duserElts->arg;

	/*
	 * Make sure the user can do this.
	 */
	if (!superuser())
		elog(ERROR, "CREATE GROUP: permission denied");

	if (strcmp(stmt->name, "public") == 0)
		elog(ERROR, "CREATE GROUP: group name \"%s\" is reserved",
			 stmt->name);

	pg_group_rel = heap_openr(GroupRelationName, ExclusiveLock);
	pg_group_dsc = RelationGetDescr(pg_group_rel);

	scan = heap_beginscan(pg_group_rel, false, SnapshotNow, 0, NULL);
	max_id = 99;				/* start auto-assigned ids at 100 */
	while (!group_exists && !sysid_exists &&
		   HeapTupleIsValid(tuple = heap_getnext(scan, false)))
	{
		Form_pg_group group_form = (Form_pg_group) GETSTRUCT(tuple);
		int32		this_sysid;

		group_exists = (strcmp(NameStr(group_form->groname), stmt->name) == 0);

		this_sysid = group_form->grosysid;
		if (havesysid)			/* customized id wanted */
			sysid_exists = (this_sysid == sysid);
		else
		{
			/* pick 1 + max */
			if (this_sysid > max_id)
				max_id = this_sysid;
		}
	}
	heap_endscan(scan);

	if (group_exists)
		elog(ERROR, "CREATE GROUP: group name \"%s\" already exists",
			 stmt->name);
	if (sysid_exists)
		elog(ERROR, "CREATE GROUP: group sysid %d is already assigned",
			 sysid);

	if (!havesysid)
		sysid = max_id + 1;

	/*
	 * Translate the given user names to ids
	 */
	foreach(item, userElts)
	{
		const char *groupuser = strVal(lfirst(item));
		int32		userid = get_usesysid(groupuser);

		if (!intMember(userid, newlist))
			newlist = lappendi(newlist, userid);
	}

	/* build an array to insert */
	if (newlist)
		grolist = IdListToArray(newlist);
	else
		grolist = NULL;

	/*
	 * Form a tuple to insert
	 */
	new_record[Anum_pg_group_groname - 1] =
		DirectFunctionCall1(namein, CStringGetDatum(stmt->name));
	new_record[Anum_pg_group_grosysid - 1] = Int32GetDatum(sysid);
	new_record[Anum_pg_group_grolist - 1] = PointerGetDatum(grolist);

	new_record_nulls[Anum_pg_group_groname - 1] = ' ';
	new_record_nulls[Anum_pg_group_grosysid - 1] = ' ';
	new_record_nulls[Anum_pg_group_grolist - 1] = grolist ? ' ' : 'n';

	tuple = heap_formtuple(pg_group_dsc, new_record, new_record_nulls);

	/*
	 * Insert a new record in the pg_group_table
	 */
	heap_insert(pg_group_rel, tuple);

	/*
	 * Update indexes
	 */
	if (RelationGetForm(pg_group_rel)->relhasindex)
	{
		Relation	idescs[Num_pg_group_indices];

		CatalogOpenIndices(Num_pg_group_indices,
						   Name_pg_group_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_group_indices, pg_group_rel,
						   tuple);
		CatalogCloseIndices(Num_pg_group_indices, idescs);
	}

	heap_close(pg_group_rel, NoLock);

	/*
	 * Write the updated pg_shadow and pg_group data to the flat file.
	 */
	update_pg_pwd_and_pg_group(NULL);
}



/*
 * ALTER GROUP
 */
void
AlterGroup(AlterGroupStmt *stmt, const char *tag)
{
	Relation	pg_group_rel;
	TupleDesc	pg_group_dsc;
	HeapTuple	group_tuple;
	IdList	   *oldarray;
	Datum		datum;
	bool		null;
	List	   *newlist,
			   *item;

	/*
	 * Make sure the user can do this.
	 */
	if (!superuser())
		elog(ERROR, "%s: permission denied", tag);

	pg_group_rel = heap_openr(GroupRelationName, ExclusiveLock);
	pg_group_dsc = RelationGetDescr(pg_group_rel);

	/*
	 * Fetch existing tuple for group.
	 */
	group_tuple = SearchSysCache(GRONAME,
								 PointerGetDatum(stmt->name),
								 0, 0, 0);
	if (!HeapTupleIsValid(group_tuple))
		elog(ERROR, "%s: group \"%s\" does not exist", tag, stmt->name);

	/* Fetch old group membership. */
	datum = heap_getattr(group_tuple, Anum_pg_group_grolist,
						 pg_group_dsc, &null);
	oldarray = null ? ((IdList *) NULL) : DatumGetIdListP(datum);

	/* initialize list with old array contents */
	newlist = IdArrayToList(oldarray);

	/*
	 * Now decide what to do.
	 */
	AssertState(stmt->action == +1 || stmt->action == -1);

	if (stmt->action == +1)		/* add users, might also be invoked by
								 * create user */
	{
		/*
		 * convert the to be added usernames to sysids and add them to
		 * the list
		 */
		foreach(item, stmt->listUsers)
		{
			int32	sysid;

			if (strcmp(tag, "ALTER GROUP") == 0)
			{
				/* Get the uid of the proposed user to add. */
				sysid = get_usesysid(strVal(lfirst(item)));
			}
			else if (strcmp(tag, "CREATE USER") == 0)
			{
				/*
				 * in this case we already know the uid and it wouldn't be
				 * in the cache anyway yet
				 */
				sysid = intVal(lfirst(item));
			}
			else
			{
				elog(ERROR, "AlterGroup: unknown tag %s", tag);
				sysid = 0;		/* keep compiler quiet */
			}

			if (!intMember(sysid, newlist))
				newlist = lappendi(newlist, sysid);
			else
				/*
				 * we silently assume here that this error will only come
				 * up in a ALTER GROUP statement
				 */
				elog(WARNING, "%s: user \"%s\" is already in group \"%s\"",
					 tag, strVal(lfirst(item)), stmt->name);
		}

		/* Do the update */
		UpdateGroupMembership(pg_group_rel, group_tuple, newlist);
	}							/* endif alter group add user */

	else if (stmt->action == -1)	/* drop users from group */
	{
		bool		is_dropuser = strcmp(tag, "DROP USER") == 0;

		if (newlist == NIL)
		{
			if (!is_dropuser)
				elog(WARNING, "ALTER GROUP: group \"%s\" does not have any members", stmt->name);
		}
		else
		{
			/*
			 * convert the to be dropped usernames to sysids and
			 * remove them from the list
			 */
			foreach(item, stmt->listUsers)
			{
				int32		sysid;

				if (!is_dropuser)
				{
					/* Get the uid of the proposed user to drop. */
					sysid = get_usesysid(strVal(lfirst(item)));
				}
				else
				{
					/* for dropuser we already know the uid */
					sysid = intVal(lfirst(item));
				}
				if (intMember(sysid, newlist))
					newlist = lremovei(sysid, newlist);
				else if (!is_dropuser)
					elog(WARNING, "ALTER GROUP: user \"%s\" is not in group \"%s\"", strVal(lfirst(item)), stmt->name);
			}

			/* Do the update */
			UpdateGroupMembership(pg_group_rel, group_tuple, newlist);
		}						/* endif group not null */
	}							/* endif alter group drop user */

	ReleaseSysCache(group_tuple);

	/*
	 * Write the updated pg_shadow and pg_group data to the flat files.
	 */
	heap_close(pg_group_rel, NoLock);

	/*
	 * Write the updated pg_shadow and pg_group data to the flat file.
	 */
	update_pg_pwd_and_pg_group(NULL);
}

/*
 * Subroutine for AlterGroup: given a pg_group tuple and a desired new
 * membership (expressed as an integer list), form and write an updated tuple.
 * The pg_group relation must be open and locked already.
 */
static void
UpdateGroupMembership(Relation group_rel, HeapTuple group_tuple,
					  List *members)
{
	IdList	   *newarray;
	Datum		new_record[Natts_pg_group];
	char		new_record_nulls[Natts_pg_group];
	char		new_record_repl[Natts_pg_group];
	HeapTuple	tuple;

	newarray = IdListToArray(members);

	/*
	 * Form an updated tuple with the new array and write it back.
	 */
	MemSet(new_record, 0, sizeof(new_record));
	MemSet(new_record_nulls, ' ', sizeof(new_record_nulls));
	MemSet(new_record_repl, ' ', sizeof(new_record_repl));

	new_record[Anum_pg_group_grolist - 1] = PointerGetDatum(newarray);
	new_record_repl[Anum_pg_group_grolist - 1] = 'r';

	tuple = heap_modifytuple(group_tuple, group_rel,
							 new_record, new_record_nulls, new_record_repl);

	simple_heap_update(group_rel, &group_tuple->t_self, tuple);

	/* Update indexes */
	if (RelationGetForm(group_rel)->relhasindex)
	{
		Relation	idescs[Num_pg_group_indices];

		CatalogOpenIndices(Num_pg_group_indices,
						   Name_pg_group_indices, idescs);
		CatalogIndexInsert(idescs, Num_pg_group_indices, group_rel,
						   tuple);
		CatalogCloseIndices(Num_pg_group_indices, idescs);
	}
}


/*
 * Convert an integer list of sysids to an array.
 */
static IdList *
IdListToArray(List *members)
{
	int			nmembers = length(members);
	IdList	   *newarray;
	List	   *item;
	int			i;

	newarray = palloc(ARR_OVERHEAD(1) + nmembers * sizeof(int32));
	newarray->size = ARR_OVERHEAD(1) + nmembers * sizeof(int32);
	newarray->flags = 0;
	ARR_NDIM(newarray) = 1;		/* one dimensional array */
	ARR_LBOUND(newarray)[0] = 1;	/* axis starts at one */
	ARR_DIMS(newarray)[0] = nmembers; /* axis is this long */
	i = 0;
	foreach(item, members)
	{
		((int *) ARR_DATA_PTR(newarray))[i++] = lfirsti(item);
	}

	return newarray;
}

/*
 * Convert an array of sysids to an integer list.
 */
static List *
IdArrayToList(IdList *oldarray)
{
	List	   *newlist = NIL;
	int			hibound,
				i;

	if (oldarray == NULL)
		return NIL;

	Assert(ARR_NDIM(oldarray) == 1);

	hibound = ARR_DIMS(oldarray)[0];

	for (i = 0; i < hibound; i++)
	{
		int32		sysid;

		sysid = ((int *) ARR_DATA_PTR(oldarray))[i];
		/* filter out any duplicates --- probably a waste of time */
		if (!intMember(sysid, newlist))
			newlist = lappendi(newlist, sysid);
	}

	return newlist;
}


/*
 * DROP GROUP
 */
void
DropGroup(DropGroupStmt *stmt)
{
	Relation	pg_group_rel;
	HeapTuple	tuple;

	/*
	 * Make sure the user can do this.
	 */
	if (!superuser())
		elog(ERROR, "DROP GROUP: permission denied");

	/*
	 * Drop the group.
	 */
	pg_group_rel = heap_openr(GroupRelationName, ExclusiveLock);

	tuple = SearchSysCacheCopy(GRONAME,
							   PointerGetDatum(stmt->name),
							   0, 0, 0);
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "DROP GROUP: group \"%s\" does not exist", stmt->name);

	simple_heap_delete(pg_group_rel, &tuple->t_self);

	heap_close(pg_group_rel, NoLock);

	/*
	 * Write the updated pg_shadow and pg_group data to the flat file.
	 */
	update_pg_pwd_and_pg_group(NULL);
}
