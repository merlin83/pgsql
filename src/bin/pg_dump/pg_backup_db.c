/*-------------------------------------------------------------------------
 *
 * pg_backup_db.c
 *
 *	Implements the basic DB functions used by the archiver.
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/pg_dump/pg_backup_db.c,v 1.19 2001-05-17 21:12:48 petere Exp $
 *
 * NOTES
 *
 * Modifications - 04-Jan-2001 - pjw@rhyme.com.au
 *
 *	  - Check results of PQ routines more carefully.
 *
 * Modifications - 19-Mar-2001 - pjw@rhyme.com.au
 *
 *	  - Avoid forcing table name to lower case in FixupBlobXrefs!
 *
 *-------------------------------------------------------------------------
 */

#include "pg_dump.h"
#include "pg_backup.h"
#include "pg_backup_archiver.h"
#include "pg_backup_db.h"

#include <unistd.h>				/* for getopt() */
#include <ctype.h>

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "libpq-fe.h"
#include "libpq/libpq-fs.h"
#ifndef HAVE_STRDUP
#include "strdup.h"
#endif

static const char *progname = "Archiver(db)";

static void _check_database_version(ArchiveHandle *AH, bool ignoreVersion);
static PGconn *_connectDB(ArchiveHandle *AH, const char *newdbname, char *newUser);
static int	_executeSqlCommand(ArchiveHandle *AH, PGconn *conn, PQExpBuffer qry, char *desc);


/*
 * simple_prompt
 *
 * Generalized function especially intended for reading in usernames and
 * password interactively. Reads from stdin.
 *
 * prompt:		The prompt to print
 * maxlen:		How many characters to accept
 * echo:		Set to false if you want to hide what is entered (for passwords)
 *
 * Returns a malloc()'ed string with the input (w/o trailing newline).
 */
char *
simple_prompt(const char *prompt, int maxlen, bool echo)
{
	int			length;
	char	   *destination;

#ifdef HAVE_TERMIOS_H
	struct termios t_orig,
				t;

#endif

	destination = (char *) malloc(maxlen + 2);
	if (!destination)
		return NULL;
	if (prompt)
		fputs(prompt, stderr);

#ifdef HAVE_TERMIOS_H
	if (!echo)
	{
		tcgetattr(0, &t);
		t_orig = t;
		t.c_lflag &= ~ECHO;
		tcsetattr(0, TCSADRAIN, &t);
	}
#endif

	if (fgets(destination, maxlen, stdin) == NULL)
		destination[0] = '\0';

#ifdef HAVE_TERMIOS_H
	if (!echo)
	{
		tcsetattr(0, TCSADRAIN, &t_orig);
		fputs("\n", stderr);
	}
#endif

	length = strlen(destination);
	if (length > 0 && destination[length - 1] != '\n')
	{
		/* eat rest of the line */
		char		buf[128];
		int			buflen;

		do
		{
			if (fgets(buf, sizeof(buf), stdin) == NULL)
				break;
			buflen = strlen(buf);
		} while (buflen > 0 && buf[buflen - 1] != '\n');
	}
	if (length > 0 && destination[length - 1] == '\n')
		/* remove trailing newline */
		destination[length - 1] = '\0';

	return destination;
}


static int
_parse_version(ArchiveHandle *AH, const char* versionString)
{
	int			cnt;
	int			vmaj, vmin, vrev;

	cnt = sscanf(versionString, "%d.%d.%d", &vmaj, &vmin, &vrev);

	if (cnt < 2)
	{
		die_horribly(AH, "Unable to parse version string: %s\n", versionString);
	}

	if (cnt == 2)
		vrev = 0;

	return (100 * vmaj + vmin) * 100 + vrev;
}

static void
_check_database_version(ArchiveHandle *AH, bool ignoreVersion)
{
	PGresult   *res;
	int			myversion;
	const char *remoteversion_str;
	int			remoteversion;
	PGconn	   *conn = AH->connection;

	myversion = _parse_version(AH, PG_VERSION);

	res = PQexec(conn, "SELECT version()");
	if (!res ||
		PQresultStatus(res) != PGRES_TUPLES_OK ||
		PQntuples(res) != 1)

		die_horribly(AH, "check_database_version(): command failed.  "
			  "Explanation from backend: '%s'.\n", PQerrorMessage(conn));

	remoteversion_str = PQgetvalue(res, 0, 0);
	remoteversion = _parse_version(AH, remoteversion_str + 11);

	PQclear(res);

	AH->public.remoteVersion = remoteversion;

	if (myversion != remoteversion 
		&& (remoteversion < AH->public.minRemoteVersion || remoteversion > AH->public.maxRemoteVersion) )
	{
		fprintf(stderr, "Database version: %s\n%s version: %s\n",
				remoteversion_str, progname, PG_VERSION);
		if (ignoreVersion)
			fprintf(stderr, "Proceeding despite version mismatch.\n");
		else
			die_horribly(AH, "Aborting because of version mismatch.\n"
						 "Use --ignore-version if you think it's safe to proceed anyway.\n");
	}
}

/*
 * Check if a given user is a superuser.
 */
int
UserIsSuperuser(ArchiveHandle *AH, char *user)
{
	PQExpBuffer qry = createPQExpBuffer();
	PGresult   *res;
	int			i_usesuper;
	int			ntups;
	int			isSuper;

	/* Get the superuser setting */
	appendPQExpBuffer(qry, "select usesuper from pg_user where usename = '%s'", user);
	res = PQexec(AH->connection, qry->data);

	if (!res)
		die_horribly(AH, "%s: null result checking superuser status of %s.\n",
					 progname, user);

	if (PQresultStatus(res) != PGRES_TUPLES_OK)
		die_horribly(AH, "%s: Could not check superuser status of %s. Explanation from backend: %s\n",
					 progname, user, PQerrorMessage(AH->connection));

	ntups = PQntuples(res);

	if (ntups == 0)
		isSuper = 0;
	else
	{
		i_usesuper = PQfnumber(res, "usesuper");
		isSuper = (strcmp(PQgetvalue(res, 0, i_usesuper), "t") == 0);
	}
	PQclear(res);

	return isSuper;
}

int
ConnectedUserIsSuperuser(ArchiveHandle *AH)
{
	return UserIsSuperuser(AH, PQuser(AH->connection));
}

char *
ConnectedUser(ArchiveHandle *AH)
{
	return PQuser(AH->connection);
}

/*
 * Reconnect the DB associated with the archive handle
 */
int
ReconnectDatabase(ArchiveHandle *AH, const char *newdbname, char *newUser)
{
	PGconn	   *newConn;
	char	   *dbname;

	if (!newdbname || (strcmp(newdbname, "-") == 0))
		dbname = PQdb(AH->connection);
	else
		dbname = (char *) newdbname;

	/* Let's see if the request is already satisfied */
	if (strcmp(PQuser(AH->connection), newUser) == 0 && strcmp(newdbname, PQdb(AH->connection)) == 0)
		return 1;

	newConn = _connectDB(AH, dbname, newUser);

	PQfinish(AH->connection);
	AH->connection = newConn;
	free(AH->username);
	AH->username = strdup(newUser);

	return 1;
}

/*
 * Connect to the db again.
 */
static PGconn *
_connectDB(ArchiveHandle *AH, const char *reqdb, char *requser)
{
	int			need_pass;
	PGconn	   *newConn;
	char	   *password = NULL;
	int			badPwd = 0;
	int			noPwd = 0;
	char	   *newdb;
	char	   *newuser;

	if (!reqdb || (strcmp(reqdb, "-") == 0))
		newdb = PQdb(AH->connection);
	else
		newdb = (char *) reqdb;

	if (!requser || (strlen(requser) == 0))
		newuser = PQuser(AH->connection);
	else
		newuser = (char *) requser;

	ahlog(AH, 1, "Connecting to %s as %s\n", newdb, newuser);

	if (AH->requirePassword)
	{
		password = simple_prompt("Password: ", 100, false);
		if (password == NULL)
			die_horribly(AH, "out of memory");
	}

	do
	{
		need_pass = false;
		newConn = PQsetdbLogin(PQhost(AH->connection), PQport(AH->connection),
							   NULL, NULL, newdb,
							   newuser, password);
		if (!newConn)
			die_horribly(AH, "%s: Failed to reconnect (PQsetdbLogin failed).\n", progname);

		if (PQstatus(newConn) == CONNECTION_BAD)
		{
			noPwd = (strcmp(PQerrorMessage(newConn),
							"fe_sendauth: no password supplied\n") == 0);
			badPwd = (strncmp(PQerrorMessage(newConn),
							  "Password authentication failed for user", 39) == 0);

			if (noPwd || badPwd)
			{

				if (badPwd)
					fprintf(stderr, "Password incorrect\n");

				fprintf(stderr, "Connecting to %s as %s\n",
						PQdb(AH->connection), newuser);

				need_pass = true;
				if (password)
					free(password);
				password = simple_prompt("Password: ", 100, false);
			}
			else
				die_horribly(AH, "%s: Could not reconnect. %s\n",
							 progname, PQerrorMessage(newConn));
		}

	} while (need_pass);

	if (password)
		free(password);

	return newConn;
}


/*
 * Make a database connection with the given parameters.  The
 * connection handle is returned, the parameters are stored in AHX.
 * An interactive password prompt is automatically issued if required.
 */
PGconn *
ConnectDatabase(Archive *AHX,
				const char *dbname,
				const char *pghost,
				const char *pgport,
				const char *username,
				const int reqPwd,
				const int ignoreVersion)
{
	ArchiveHandle *AH = (ArchiveHandle *) AHX;
	char	   *password = NULL;
	bool		need_pass = false;

	if (AH->connection)
		die_horribly(AH, "%s: already connected to database\n", progname);

	if (!dbname && !(dbname = getenv("PGDATABASE")))
		die_horribly(AH, "%s: no database name specified\n", progname);

	AH->dbname = strdup(dbname);

	if (pghost != NULL)
		AH->pghost = strdup(pghost);
	else
		AH->pghost = NULL;

	if (pgport != NULL)
		AH->pgport = strdup(pgport);
	else
		AH->pgport = NULL;

	if (username != NULL)
		AH->username = strdup(username);
	else
		AH->username = NULL;

	if (reqPwd)
	{
		password = simple_prompt("Password: ", 100, false);
		if (password == NULL)
			die_horribly(AH, "out of memory");
		AH->requirePassword = true;
	}
	else
		AH->requirePassword = false;

	/*
	 * Start the connection.  Loop until we have a password if
	 * requested by backend.
	 */
	do
	{
		need_pass = false;
		AH->connection = PQsetdbLogin(AH->pghost, AH->pgport, NULL, NULL,
									  AH->dbname, AH->username, password);

		if (!AH->connection)
			die_horribly(AH, "%s: Failed to connect (PQsetdbLogin failed).\n",
						 progname);

		if (PQstatus(AH->connection) == CONNECTION_BAD &&
			strcmp(PQerrorMessage(AH->connection), "fe_sendauth: no password supplied\n") == 0 &&
			!feof(stdin))
		{
			PQfinish(AH->connection);
			need_pass = true;
			free(password);
			password = NULL;
			password = simple_prompt("Password: ", 100, false);
		}
	} while (need_pass);

	if (password)
		free(password);

	/* check to see that the backend connection was successfully made */
	if (PQstatus(AH->connection) == CONNECTION_BAD)
		die_horribly(AH, "Connection to database '%s' failed.\n%s\n",
					 AH->dbname, PQerrorMessage(AH->connection));

	/* check for version mismatch */
	_check_database_version(AH, ignoreVersion);

	/*
	 * AH->currUser = PQuser(AH->connection);
	 *
	 * Removed because it prevented an initial \connect when dumping to SQL
	 * in pg_dump.
	 */

	return AH->connection;
}

/* Public interface */
/* Convenience function to send a query. Monitors result to handle COPY statements */
int
ExecuteSqlCommand(ArchiveHandle *AH, PQExpBuffer qry, char *desc)
{
	return _executeSqlCommand(AH, AH->connection, qry, desc);
}

/*
 * Handle command execution. This is used to execute a command on more than one connection,
 * but the 'pgCopyIn' setting assumes the COPY commands are ONLY executed on the primary
 * setting...an error will be raised otherwise.
 */
static int
_executeSqlCommand(ArchiveHandle *AH, PGconn *conn, PQExpBuffer qry, char *desc)
{
	PGresult   *res;

	/* fprintf(stderr, "Executing: '%s'\n\n", qry->data); */
	res = PQexec(conn, qry->data);
	if (!res)
		die_horribly(AH, "%s: %s. No result from backend.\n", progname, desc);

	if (PQresultStatus(res) != PGRES_COMMAND_OK && PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		if (PQresultStatus(res) == PGRES_COPY_IN)
		{
			if (conn != AH->connection)
				die_horribly(AH, "%s: COPY command execute in non-primary connection.\n", progname);

			AH->pgCopyIn = 1;
		}
		else
			die_horribly(AH, "%s: %s. Code = %d. Explanation from backend: '%s'.\n",
						 progname, desc, PQresultStatus(res), PQerrorMessage(AH->connection));
	}

	PQclear(res);

	return strlen(qry->data);
}

/* Convenience function to send one or more queries. Monitors result to handle COPY statements */
int
ExecuteSqlCommandBuf(ArchiveHandle *AH, void *qryv, int bufLen)
{
	int			loc;
	int			pos = 0;
	int			sPos = 0;
	char	   *qry = (char *) qryv;
	int			isEnd = 0;
	char	   *eos = qry + bufLen;

	/*
	 * fprintf(stderr, "\n\n*****\n
	 * Buffer:\n\n%s\n*******************\n\n", qry);
	 */

	/* If we're in COPY IN mode, then just break it into lines and send... */
	if (AH->pgCopyIn)
	{
		for (;;)
		{

			/* Find a lf */
			loc = strcspn(&qry[pos], "\n") + pos;
			pos = 0;

			/* If no match, then wait */
			if (loc >= (eos - qry))		/* None found */
			{
				appendBinaryPQExpBuffer(AH->pgCopyBuf, qry, (eos - qry));
				break;
			};

			/*
			 * fprintf(stderr, "Found cr at %d, prev char was %c, next was
			 * %c\n", loc, qry[loc-1], qry[loc+1]);
			 */

			/* Count the number of preceding slashes */
			sPos = loc;
			while (sPos > 0 && qry[sPos - 1] == '\\')
				sPos--;

			sPos = loc - sPos;

			/*
			 * If an odd number of preceding slashes, then \n was escaped
			 * so set the next search pos, and restart (if any left).
			 */
			if ((sPos & 1) == 1)
			{
				/* fprintf(stderr, "cr was escaped\n"); */
				pos = loc + 1;
				if (pos >= (eos - qry))
				{
					appendBinaryPQExpBuffer(AH->pgCopyBuf, qry, (eos - qry));
					break;
				}
			}
			else
			{
				/* We got a good cr */
				qry[loc] = '\0';
				appendPQExpBuffer(AH->pgCopyBuf, "%s\n", qry);
				qry += loc + 1;
				isEnd = (strcmp(AH->pgCopyBuf->data, "\\.\n") == 0);

				/*---------
				 * fprintf(stderr, "Sending '%s' via
				 *		COPY (at end = %d)\n\n", AH->pgCopyBuf->data, isEnd);
				 *---------
				 */

				if (PQputline(AH->connection, AH->pgCopyBuf->data) != 0)
					die_horribly(AH, "%s: error returned by PQputline\n", progname);

				resetPQExpBuffer(AH->pgCopyBuf);

				/*
				 * fprintf(stderr, "Buffer is '%s'\n",
				 * AH->pgCopyBuf->data);
				 */

				if (isEnd)
				{
					if (PQendcopy(AH->connection) != 0)
						die_horribly(AH, "%s: error returned by PQendcopy\n", progname);

					AH->pgCopyIn = 0;
					break;
				}

			}

			/* Make sure we're not past the original buffer end */
			if (qry >= eos)
				break;

		}
	}

	/* We may have finished Copy In, and have a non-empty buffer */
	if (!AH->pgCopyIn)
	{

		/*
		 * The following is a mini state machine to assess then of of an
		 * SQL statement. It really only needs to parse good SQL, or at
		 * least that's the theory... End-of-statement is assumed to be an
		 * unquoted, un commented semi-colon.
		 */

		/*
		 * fprintf(stderr, "Buffer at start is: '%s'\n\n",
		 * AH->sqlBuf->data);
		 */

		for (pos = 0; pos < (eos - qry); pos++)
		{
			appendPQExpBufferChar(AH->sqlBuf, qry[pos]);
			/* fprintf(stderr, " %c",qry[pos]); */

			switch (AH->sqlparse.state)
			{

				case SQL_SCAN:	/* Default state == 0, set in _allocAH */

					if (qry[pos] == ';' && AH->sqlparse.braceDepth == 0)
					{
						/* Send It & reset the buffer */

						/*
						 * fprintf(stderr, "    sending: '%s'\n\n",
						 * AH->sqlBuf->data);
						 */
						ExecuteSqlCommand(AH, AH->sqlBuf, "Could not execute query");
						resetPQExpBuffer(AH->sqlBuf);
						AH->sqlparse.lastChar = '\0';
					}
					else
					{
						if (qry[pos] == '"' || qry[pos] == '\'')
						{
							/* fprintf(stderr,"[startquote]\n"); */
							AH->sqlparse.state = SQL_IN_QUOTE;
							AH->sqlparse.quoteChar = qry[pos];
							AH->sqlparse.backSlash = 0;
						}
						else if (qry[pos] == '-' && AH->sqlparse.lastChar == '-')
							AH->sqlparse.state = SQL_IN_SQL_COMMENT;
						else if (qry[pos] == '*' && AH->sqlparse.lastChar == '/')
							AH->sqlparse.state = SQL_IN_EXT_COMMENT;
						else if (qry[pos] == '(')
							AH->sqlparse.braceDepth++;
						else if (qry[pos] == ')')
							AH->sqlparse.braceDepth--;

						AH->sqlparse.lastChar = qry[pos];
					}

					break;

				case SQL_IN_SQL_COMMENT:

					if (qry[pos] == '\n')
						AH->sqlparse.state = SQL_SCAN;
					break;

				case SQL_IN_EXT_COMMENT:

					if (AH->sqlparse.lastChar == '*' && qry[pos] == '/')
						AH->sqlparse.state = SQL_SCAN;
					break;

				case SQL_IN_QUOTE:

					if (!AH->sqlparse.backSlash && AH->sqlparse.quoteChar == qry[pos])
					{
						/* fprintf(stderr,"[endquote]\n"); */
						AH->sqlparse.state = SQL_SCAN;
					}
					else
					{

						if (qry[pos] == '\\')
						{
							if (AH->sqlparse.lastChar == '\\')
								AH->sqlparse.backSlash = !AH->sqlparse.backSlash;
							else
								AH->sqlparse.backSlash = 1;
						}
						else
							AH->sqlparse.backSlash = 0;
					}
					break;

			}
			AH->sqlparse.lastChar = qry[pos];
			/* fprintf(stderr, "\n"); */
		}

	}

	return 1;
}

void
FixupBlobRefs(ArchiveHandle *AH, char *tablename)
{
	PQExpBuffer tblQry = createPQExpBuffer();
	PGresult   *res,
			   *uRes;
	int			i,
				n;
	char	   *attr;

	if (strcmp(tablename, BLOB_XREF_TABLE) == 0)
		return;

	appendPQExpBuffer(tblQry, "SELECT a.attname FROM pg_class c, pg_attribute a, pg_type t "
	 " WHERE a.attnum > 0 AND a.attrelid = c.oid AND a.atttypid = t.oid "
			  " AND t.typname = 'oid' AND c.relname = '%s';", tablename);

	res = PQexec(AH->blobConnection, tblQry->data);
	if (!res)
		die_horribly(AH, "%s: could not find OID attrs of %s. Explanation from backend '%s'\n",
					 progname, tablename, PQerrorMessage(AH->connection));

	if ((n = PQntuples(res)) == 0)
	{
		/* We're done */
		ahlog(AH, 1, "No OID attributes in table %s\n", tablename);
		PQclear(res);
		return;
	}

	for (i = 0; i < n; i++)
	{
		attr = PQgetvalue(res, i, 0);

		ahlog(AH, 1, " - %s.%s\n", tablename, attr);

		resetPQExpBuffer(tblQry);

		/*
		 * We should use coalesce here (rather than 'exists'), but it
		 * seems to be broken in 7.0.2 (weird optimizer strategy)
		 */
		appendPQExpBuffer(tblQry, "UPDATE \"%s\" SET \"%s\" = ", tablename, attr);
		appendPQExpBuffer(tblQry, " (SELECT x.newOid FROM \"%s\" x WHERE x.oldOid = \"%s\".\"%s\")",
						  BLOB_XREF_TABLE, tablename, attr);
		appendPQExpBuffer(tblQry, " where exists"
				  "(select * from %s x where x.oldOid = \"%s\".\"%s\");",
						  BLOB_XREF_TABLE, tablename, attr);

		ahlog(AH, 10, " - sql:\n%s\n", tblQry->data);

		uRes = PQexec(AH->blobConnection, tblQry->data);
		if (!uRes)
			die_horribly(AH, "%s: could not update attr %s of table %s. Explanation from backend '%s'\n",
						 progname, attr, tablename, PQerrorMessage(AH->blobConnection));

		if (PQresultStatus(uRes) != PGRES_COMMAND_OK)
			die_horribly(AH, "%s: error while updating attr %s of table %s (result = %d)."
						 " Explanation from backend '%s'\n",
						 progname, attr, tablename, PQresultStatus(uRes),
						 PQerrorMessage(AH->blobConnection));

		PQclear(uRes);
	}

	PQclear(res);

}

/**********
 *	Convenient SQL calls
 **********/
void
CreateBlobXrefTable(ArchiveHandle *AH)
{
	PQExpBuffer qry = createPQExpBuffer();

	/* IF we don't have a BLOB connection, then create one */
	if (!AH->blobConnection)
		AH->blobConnection = _connectDB(AH, NULL, NULL);

	ahlog(AH, 1, "Creating table for BLOBS xrefs\n");

	appendPQExpBuffer(qry, "Create Temporary Table %s(oldOid oid, newOid oid);", BLOB_XREF_TABLE);

	_executeSqlCommand(AH, AH->blobConnection, qry, "can not create BLOB xref table '" BLOB_XREF_TABLE "'");

	resetPQExpBuffer(qry);

	appendPQExpBuffer(qry, "Create Unique Index %s_ix on %s(oldOid)", BLOB_XREF_TABLE, BLOB_XREF_TABLE);
	_executeSqlCommand(AH, AH->blobConnection, qry, "can not create index on BLOB xref table '" BLOB_XREF_TABLE "'");
}

void
InsertBlobXref(ArchiveHandle *AH, int old, int new)
{
	PQExpBuffer qry = createPQExpBuffer();

	appendPQExpBuffer(qry, "Insert Into %s(oldOid, newOid) Values (%d, %d);", BLOB_XREF_TABLE, old, new);

	_executeSqlCommand(AH, AH->blobConnection, qry, "can not create BLOB xref entry");
}

void
StartTransaction(ArchiveHandle *AH)
{
	PQExpBuffer qry = createPQExpBuffer();

	appendPQExpBuffer(qry, "Begin;");

	ExecuteSqlCommand(AH, qry, "can not start database transaction");
	AH->txActive = true;
}

void
StartTransactionXref(ArchiveHandle *AH)
{
	PQExpBuffer qry = createPQExpBuffer();

	appendPQExpBuffer(qry, "Begin;");

	_executeSqlCommand(AH, AH->blobConnection, qry, "can not start BLOB xref transaction");
	AH->blobTxActive = true;
}

void
CommitTransaction(ArchiveHandle *AH)
{
	PQExpBuffer qry = createPQExpBuffer();

	appendPQExpBuffer(qry, "Commit;");

	ExecuteSqlCommand(AH, qry, "can not commit database transaction");
	AH->txActive = false;
}

void
CommitTransactionXref(ArchiveHandle *AH)
{
	PQExpBuffer qry = createPQExpBuffer();

	appendPQExpBuffer(qry, "Commit;");

	_executeSqlCommand(AH, AH->blobConnection, qry, "can not commit BLOB xref transaction");
	AH->blobTxActive = false;
}
