/*-------------------------------------------------------------------------
 *
 * vacuumlo.c
 *	  This removes orphaned large objects from a database.
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/contrib/vacuumlo/vacuumlo.c,v 1.20 2003-05-14 03:25:57 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include "libpq-fe.h"
#include "libpq/libpq-fs.h"

#define atooid(x)  ((Oid) strtoul((x), NULL, 10))

#define BUFSIZE			1024

extern char *optarg;
extern int	optind,
			opterr,
			optopt;

struct _param
{
	char	   *pg_user;
	int			pg_prompt;
	char	   *pg_port;
	char	   *pg_host;
	int			verbose;
	int			dry_run;
};

int			vacuumlo(char *, struct _param *);
char	   *simple_prompt(const char *prompt, int, int);
void		usage(void);


/*
 * simple_prompt
 *
 * Generalized function especially intended for reading in usernames and
 * password interactively. Reads from /dev/tty or stdin/stderr.
 *
 * prompt:		The prompt to print
 * maxlen:		How many characters to accept
 * echo:		Set to 0 if you want to hide what is entered (for passwords)
 *
 * Returns a malloc()'ed string with the input (w/o trailing newline).
 */
static int	prompt_state = 0;

char *
simple_prompt(const char *prompt, int maxlen, int echo)
{
	int			length;
	char	   *destination;
	FILE	   *termin,
			   *termout;

#ifdef HAVE_TERMIOS_H
	struct termios t_orig,
				t;
#endif

	destination = (char *) malloc(maxlen + 2);
	if (!destination)
		return NULL;

	prompt_state = 1;			/* disable SIGINT */

	/*
	 * Do not try to collapse these into one "w+" mode file. Doesn't work
	 * on some platforms (eg, HPUX 10.20).
	 */
	termin = fopen("/dev/tty", "r");
	termout = fopen("/dev/tty", "w");
	if (!termin || !termout)
	{
		if (termin)
			fclose(termin);
		if (termout)
			fclose(termout);
		termin = stdin;
		termout = stderr;
	}

#ifdef HAVE_TERMIOS_H
	if (!echo)
	{
		tcgetattr(fileno(termin), &t);
		t_orig = t;
		t.c_lflag &= ~ECHO;
		tcsetattr(fileno(termin), TCSAFLUSH, &t);
	}
#endif

	if (prompt)
	{
		fputs(prompt, termout);
		fflush(termout);
	}

	if (fgets(destination, maxlen, termin) == NULL)
		destination[0] = '\0';

	length = strlen(destination);
	if (length > 0 && destination[length - 1] != '\n')
	{
		/* eat rest of the line */
		char		buf[128];
		int			buflen;

		do
		{
			if (fgets(buf, sizeof(buf), termin) == NULL)
				break;
			buflen = strlen(buf);
		} while (buflen > 0 && buf[buflen - 1] != '\n');
	}

	if (length > 0 && destination[length - 1] == '\n')
		/* remove trailing newline */
		destination[length - 1] = '\0';

#ifdef HAVE_TERMIOS_H
	if (!echo)
	{
		tcsetattr(fileno(termin), TCSAFLUSH, &t_orig);
		fputs("\n", termout);
		fflush(termout);
	}
#endif

	if (termin != stdin)
	{
		fclose(termin);
		fclose(termout);
	}

	prompt_state = 0;			/* SIGINT okay again */

	return destination;
}



/*
 * This vacuums LOs of one database. It returns 0 on success, -1 on failure.
 */
int
vacuumlo(char *database, struct _param * param)
{
	PGconn	   *conn;
	PGresult   *res,
			   *res2;
	char		buf[BUFSIZE];
	int			matched;
	int			deleted;
	int			i;
	char	   *password = NULL;

	if (param->pg_prompt)
	{
		password = simple_prompt("Password: ", 32, 0);
		if (!password)
		{
			fprintf(stderr, "failed to get password\n");
			exit(1);
		}
	}

	conn = PQsetdbLogin(param->pg_host,
						param->pg_port,
						NULL,
						NULL,
						database,
						param->pg_user,
						password
		);

	/* check to see that the backend connection was successfully made */
	if (PQstatus(conn) == CONNECTION_BAD)
	{
		fprintf(stderr, "Connection to database '%s' failed:\n", database);
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
	}

	if (param->verbose)
	{
		fprintf(stdout, "Connected to %s\n", database);
		if (param->dry_run)
			fprintf(stdout, "Test run: no large objects will be removed!\n");
	}

	res = PQexec(conn, "SET search_path = public");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to set search_path on:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	/*
	 * First we create and populate the LO temp table
	 */
	buf[0] = '\0';
	strcat(buf, "SELECT DISTINCT loid AS lo ");
	strcat(buf, "INTO TEMP TABLE vacuum_l ");
	strcat(buf, "FROM pg_largeobject ");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to create temp table:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	/*
	 * Vacuum the temp table so that planner will generate decent plans
	 * for the DELETEs below.
	 */
	buf[0] = '\0';
	strcat(buf, "VACUUM ANALYZE vacuum_l ");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, "Failed to vacuum temp table:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}
	PQclear(res);

	/*
	 * Now find any candidate tables who have columns of type oid.
	 *
	 * NOTE: the temp table formed above is ignored, because its real table
	 * name will be pg_something.  Also, pg_largeobject will be ignored.
	 * If either of these were scanned, obviously we'd end up with nothing
	 * to delete...
	 *
	 * NOTE: the system oid column is ignored, as it has attnum < 1. This
	 * shouldn't matter for correctness, but it saves time.
	 */
	buf[0] = '\0';
	strcat(buf, "SELECT c.relname, a.attname ");
	strcat(buf, "FROM pg_class c, pg_attribute a, pg_type t ");
	strcat(buf, "WHERE a.attnum > 0 ");
	strcat(buf, "      AND a.attrelid = c.oid ");
	strcat(buf, "      AND a.atttypid = t.oid ");
	strcat(buf, "      AND t.typname in ('oid', 'lo') ");
	strcat(buf, "      AND c.relkind = 'r'");
	strcat(buf, "      AND c.relname NOT LIKE 'pg_%'");
	strcat(buf, "      AND c.relname != 'vacuum_l'");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Failed to find OID columns:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}

	for (i = 0; i < PQntuples(res); i++)
	{
		char	   *table,
				   *field;

		table = PQgetvalue(res, i, 0);
		field = PQgetvalue(res, i, 1);

		if (param->verbose)
			fprintf(stdout, "Checking %s in %s\n", field, table);

		/*
		 * We use a DELETE with implicit join for efficiency.  This is a
		 * Postgres-ism and not portable to other DBMSs, but then this
		 * whole program is a Postgres-ism.
		 */
		snprintf(buf, BUFSIZE, "DELETE FROM vacuum_l WHERE lo = \"%s\".\"%s\" ",
				 table, field);
		res2 = PQexec(conn, buf);
		if (PQresultStatus(res2) != PGRES_COMMAND_OK)
		{
			fprintf(stderr, "Failed to check %s in table %s:\n",
					field, table);
			fprintf(stderr, "%s", PQerrorMessage(conn));
			PQclear(res2);
			PQclear(res);
			PQfinish(conn);
			return -1;
		}
		PQclear(res2);
	}
	PQclear(res);

	/*
	 * Run the actual deletes in a single transaction.	Note that this
	 * would be a bad idea in pre-7.1 Postgres releases (since rolling
	 * back a table delete used to cause problems), but it should be safe
	 * now.
	 */
	res = PQexec(conn, "begin");
	PQclear(res);

	/*
	 * Finally, those entries remaining in vacuum_l are orphans.
	 */
	buf[0] = '\0';
	strcat(buf, "SELECT lo ");
	strcat(buf, "FROM vacuum_l");
	res = PQexec(conn, buf);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "Failed to read temp table:\n");
		fprintf(stderr, "%s", PQerrorMessage(conn));
		PQclear(res);
		PQfinish(conn);
		return -1;
	}

	matched = PQntuples(res);
	deleted = 0;
	for (i = 0; i < matched; i++)
	{
		Oid			lo = atooid(PQgetvalue(res, i, 0));

		if (param->verbose)
		{
			fprintf(stdout, "\rRemoving lo %6u   ", lo);
			fflush(stdout);
		}

		if (param->dry_run == 0)
		{
			if (lo_unlink(conn, lo) < 0)
			{
				fprintf(stderr, "\nFailed to remove lo %u: ", lo);
				fprintf(stderr, "%s", PQerrorMessage(conn));
			}
			else
				deleted++;
		}
		else
			deleted++;
	}
	PQclear(res);

	/*
	 * That's all folks!
	 */
	res = PQexec(conn, "end");
	PQclear(res);

	PQfinish(conn);

	if (param->verbose)
		fprintf(stdout, "\r%s %d large objects from %s.\n",
		(param->dry_run ? "Would remove" : "Removed"), deleted, database);

	return 0;
}

void
usage(void)
{
	fprintf(stdout, "vacuumlo removes unreferenced large objects from databases\n\n");
	fprintf(stdout, "Usage:\n  vacuumlo [options] dbname [dbnames...]\n\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "  -v\t\tWrite a lot of output\n");
	fprintf(stdout, "  -n\t\tDon't remove any large object, just show what would be done\n");
	fprintf(stdout, "  -U username\tUsername to connect as\n");
	fprintf(stdout, "  -W\t\tPrompt for password\n");
	fprintf(stdout, "  -h hostname\tDatabase server host\n");
	fprintf(stdout, "  -p port\tDatabase server port\n\n");
}


int
main(int argc, char **argv)
{
	int			rc = 0;
	struct _param param;
	int			c;
	int			port;

	/* Parameter handling */
	param.pg_user = NULL;
	param.pg_prompt = 0;
	param.pg_host = NULL;
	param.pg_port = 0;
	param.verbose = 0;
	param.dry_run = 0;

	while (1)
	{
		c = getopt(argc, argv, "?h:U:p:vnW");
		if (c == -1)
			break;

		switch (c)
		{
			case '?':
				if (optopt == '?')
				{
					usage();
					exit(0);
				}
				exit(1);
			case ':':
				exit(1);
			case 'v':
				param.verbose = 1;
				break;
			case 'n':
				param.dry_run = 1;
				param.verbose = 1;
				break;
			case 'U':
				param.pg_user = strdup(optarg);
				break;
			case 'W':
				param.pg_prompt = 1;
				break;
			case 'p':
				port = strtol(optarg, NULL, 10);
				if ((port < 1) || (port > 65535))
				{
					fprintf(stderr, "[%s]: invalid port number '%s'\n", argv[0], optarg);
					exit(1);
				}
				param.pg_port = strdup(optarg);
				break;
			case 'h':
				param.pg_host = strdup(optarg);
				break;
		}
	}

	/* No database given? Show usage */
	if (optind >= argc)
	{
		fprintf(stderr, "vacuumlo: missing required argument: database name\n");
		fprintf(stderr, "Try 'vacuumlo -?' for help.\n");
		exit(1);
	}

	for (c = optind; c < argc; c++)
	{
		/* Work on selected database */
		rc += (vacuumlo(argv[c], &param) != 0);
	}

	return rc;
}
