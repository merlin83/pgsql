/*-------------------------------------------------------------------------
 *
 * Miscellaneous shared code
 *
 * Portions Copyright (c) 1996-2003, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/scripts/common.c,v 1.1 2003-03-18 22:19:46 petere Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "common.h"

#include <pwd.h>
#include <unistd.h>


/*
 * Returns the current user name.
 */
const char *
get_user_name(const char *progname)
{
	struct passwd *pw;

	pw = getpwuid(getuid());
	if (!pw)
	{
		perror(progname);
		exit(1);
	}
	return pw->pw_name;
}


/*
 * Extracts the actual name of the program as called.
 */
char *
get_progname(char *argv0)
{
	if (!strrchr(argv0, '/'))
		return argv0;
	else
		return strrchr(argv0, '/') + 1;
}


/*
 * Initialized NLS if enabled.
 */
void
init_nls(void)
{
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain("pgscripts", LOCALEDIR);
	textdomain("pgscripts");
#endif
}


/*
 * Provide strictly harmonized handling of --help and --version
 * options.
 */
void
handle_help_version_opts(int argc, char *argv[], const char *fixed_progname, help_handler hlp)
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			hlp(get_progname(argv[0]));
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			printf("%s (PostgreSQL) " PG_VERSION "\n", fixed_progname);
			exit(0);
		}
	}
}


/*
 * Make a database connection with the given parameters.  An
 * interactive password prompt is automatically issued if required.
 */
PGconn *
connectDatabase(const char *dbname, const char *pghost, const char *pgport,
				const char *pguser, bool require_password, const char *progname)
{
	PGconn	   *conn;
	char	   *password = NULL;
	bool		need_pass = false;

	if (require_password)
		password = simple_prompt("Password: ", 100, false);

	/*
	 * Start the connection.  Loop until we have a password if requested
	 * by backend.
	 */
	do
	{
		need_pass = false;
		conn = PQsetdbLogin(pghost, pgport, NULL, NULL, dbname, pguser, password);

		if (!conn)
		{
			fprintf(stderr, _("%s: could not connect to database %s\n"),
					progname, dbname);
			exit(1);
		}

		if (PQstatus(conn) == CONNECTION_BAD &&
			strcmp(PQerrorMessage(conn), "fe_sendauth: no password supplied\n") == 0 &&
			!feof(stdin))
		{
			PQfinish(conn);
			need_pass = true;
			free(password);
			password = NULL;
			password = simple_prompt("Password: ", 100, false);
		}
	} while (need_pass);

	if (password)
		free(password);

	/* check to see that the backend connection was successfully made */
	if (PQstatus(conn) == CONNECTION_BAD)
	{
		fprintf(stderr, _("%s: could not connect to database %s: %s"),
				progname, dbname, PQerrorMessage(conn));
		exit(1);
	}

	return conn;
}


/*
 * Run a query, return the results, exit program on failure.
 */
PGresult *
executeQuery(PGconn *conn, const char *query, const char *progname, bool echo)
{
	PGresult   *res;

	if (echo)
		printf("%s\n", query);

	res = PQexec(conn, query);
	if (!res ||
		PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, _("%s: query failed: %s"), progname, PQerrorMessage(conn));
		fprintf(stderr, _("%s: query was: %s\n"), progname, query);
		PQfinish(conn);
		exit(1);
	}

	return res;
}
