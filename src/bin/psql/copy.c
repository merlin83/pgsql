/*
 * psql - the PostgreSQL interactive terminal
 *
 * Copyright 2000 by PostgreSQL Global Development Group
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/psql/copy.c,v 1.27 2002-10-15 02:24:16 tgl Exp $
 */
#include "postgres_fe.h"
#include "copy.h"

#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>				/* for isatty */
#else
#include <io.h>					/* I think */
#endif

#include "libpq-fe.h"
#include "pqexpbuffer.h"
#include "pqsignal.h"

#include "settings.h"
#include "common.h"
#include "stringutils.h"

#ifdef WIN32
#define strcasecmp(x,y) stricmp(x,y)
#define	__S_ISTYPE(mode, mask)	(((mode) & S_IFMT) == (mask))
#define	S_ISDIR(mode)	 __S_ISTYPE((mode), S_IFDIR)
#endif

bool		copy_in_state;

/*
 * parse_slash_copy
 * -- parses \copy command line
 *
 * Accepted syntax: \copy table|"table" [with oids] from|to filename|'filename' [with ] [ oids ] [ delimiter '<char>'] [ null as 'string' ]
 * (binary is not here yet)
 *
 * Old syntax for backward compatibility: (2002-06-19):
 * \copy table|"table" [with oids] from|to filename|'filename' [ using delimiters '<char>'] [ with null as 'string' ]
 *
 * returns a malloc'ed structure with the options, or NULL on parsing error
 */

struct copy_options
{
	char	   *table;
	char	   *file;			/* NULL = stdin/stdout */
	bool		from;
	bool		binary;
	bool		oids;
	char	   *delim;
	char	   *null;
};


static void
free_copy_options(struct copy_options * ptr)
{
	if (!ptr)
		return;
	free(ptr->table);
	free(ptr->file);
	free(ptr->delim);
	free(ptr->null);
	free(ptr);
}


static struct copy_options *
parse_slash_copy(const char *args)
{
	struct copy_options *result;
	char	   *line;
	char	   *token;
	bool		error = false;
	char		quote;

	if (args)
		line = xstrdup(args);
	else
	{
		psql_error("\\copy: arguments required\n");
		return NULL;
	}

	if (!(result = calloc(1, sizeof(struct copy_options))))
	{
		psql_error("out of memory\n");
		exit(EXIT_FAILURE);
	}

	token = strtokx(line, " \t\n\r", "\"", '\\', &quote, NULL, pset.encoding);
	if (!token)
		error = true;
	else
	{
#ifdef NOT_USED
		/* this is not implemented yet */
		if (!quote && strcasecmp(token, "binary") == 0)
		{
			result->binary = true;
			token = strtokx(NULL, " \t\n\r", "\"", '\\', &quote, NULL, pset.encoding);
			if (!token)
				error = true;
		}
		if (token)
#endif
			result->table = xstrdup(token);
	}

#ifdef USE_ASSERT_CHECKING
	assert(error || result->table);
#endif

	if (!error)
	{
		token = strtokx(NULL, " \t\n\r", NULL, '\\', NULL, NULL, pset.encoding);
		if (!token)
			error = true;
		else
		{
			/*
			 * Allows old COPY syntax for backward compatibility
			 * 2002-06-19
			 */
			if (strcasecmp(token, "with") == 0)
			{
				token = strtokx(NULL, " \t\n\r", NULL, '\\', NULL, NULL, pset.encoding);
				if (!token || strcasecmp(token, "oids") != 0)
					error = true;
				else
					result->oids = true;

				if (!error)
				{
					token = strtokx(NULL, " \t\n\r", NULL, '\\', NULL, NULL, pset.encoding);
					if (!token)
						error = true;
				}
			}

			if (!error && strcasecmp(token, "from") == 0)
				result->from = true;
			else if (!error && strcasecmp(token, "to") == 0)
				result->from = false;
			else
				error = true;
		}
	}

	if (!error)
	{
		token = strtokx(NULL, " \t\n\r", "'", '\\', &quote, NULL, pset.encoding);
		if (!token)
			error = true;
		else if (!quote && (strcasecmp(token, "stdin") == 0 || strcasecmp(token, "stdout") == 0))
			result->file = NULL;
		else
			result->file = xstrdup(token);
	}

	if (!error)
	{
		token = strtokx(NULL, " \t\n\r", NULL, '\\', NULL, NULL, pset.encoding);
		if (token)
		{
			/*
			 * Allows old COPY syntax for backward compatibility
			 * 2002-06-19
			 */
			if (strcasecmp(token, "using") == 0)
			{
				token = strtokx(NULL, " \t\n\r", NULL, '\\', NULL, NULL, pset.encoding);
				if (token && strcasecmp(token, "delimiters") == 0)
				{
					token = strtokx(NULL, " \t\n\r", "'", '\\', NULL, NULL, pset.encoding);
					if (token)
					{
						result->delim = xstrdup(token);
						token = strtokx(NULL, " \t\n\r", NULL, '\\', NULL, NULL, pset.encoding);
					}
					else
						error = true;
				}
				else
					error = true;
			}
		}
	}

	if (!error && token)
	{
		if (strcasecmp(token, "with") == 0)
		{
			while (!error && (token = strtokx(NULL, " \t\n\r", NULL, '\\', NULL, NULL, pset.encoding)))
			{
				if (strcasecmp(token, "delimiter") == 0)
				{
					token = strtokx(NULL, " \t\n\r", "'", '\\', NULL, NULL, pset.encoding);
					if (token && strcasecmp(token, "as") == 0)
						token = strtokx(NULL, " \t\n\r", "'", '\\', NULL, NULL, pset.encoding);
					if (token)
						result->delim = xstrdup(token);
					else
						error = true;
				}
				else if (strcasecmp(token, "null") == 0)
				{
					token = strtokx(NULL, " \t\n\r", "'", '\\', NULL, NULL, pset.encoding);
					if (token && strcasecmp(token, "as") == 0)
						token = strtokx(NULL, " \t\n\r", "'", '\\', NULL, NULL, pset.encoding);
					if (token)
						result->null = xstrdup(token);
					else
						error = true;
				}
				else
					error = true;
			}
		}
		else
			error = true;
	}

	free(line);

	if (error)
	{
		if (token)
			psql_error("\\copy: parse error at '%s'\n", token);
		else
			psql_error("\\copy: parse error at end of line\n");
		free_copy_options(result);
		return NULL;
	}
	else
		return result;
}



/*
 * Execute a \copy command (frontend copy). We have to open a file, then
 * submit a COPY query to the backend and either feed it data from the
 * file or route its response into the file.
 */
bool
do_copy(const char *args)
{
	PQExpBufferData query;
	FILE	   *copystream;
	struct copy_options *options;
	PGresult   *result;
	bool		success;
	struct stat st;

	/* parse options */
	options = parse_slash_copy(args);

	if (!options)
		return false;

	initPQExpBuffer(&query);

	printfPQExpBuffer(&query, "COPY ");
	if (options->binary)
		appendPQExpBuffer(&query, "BINARY ");

	appendPQExpBuffer(&query, "\"%s\" ", options->table);
	/* Uses old COPY syntax for backward compatibility 2002-06-19 */
	if (options->oids)
		appendPQExpBuffer(&query, "WITH OIDS ");

	if (options->from)
		appendPQExpBuffer(&query, "FROM STDIN");
	else
		appendPQExpBuffer(&query, "TO STDOUT");


	/* Uses old COPY syntax for backward compatibility 2002-06-19 */
	if (options->delim)
		appendPQExpBuffer(&query, " USING DELIMITERS '%s'", options->delim);

	if (options->null)
		appendPQExpBuffer(&query, " WITH NULL AS '%s'", options->null);

	if (options->from)
	{
		if (options->file)
			copystream = fopen(options->file, "r");
		else
			copystream = stdin;
	}
	else
	{
		if (options->file)
			copystream = fopen(options->file, "w");
		else
			copystream = stdout;
	}

	if (!copystream)
	{
		psql_error("%s: %s\n",
				   options->file, strerror(errno));
		free_copy_options(options);
		return false;
	}

	/* make sure the specified file is not a directory */
	fstat(fileno(copystream), &st);
	if (S_ISDIR(st.st_mode))
	{
		fclose(copystream);
		psql_error("%s: cannot copy from/to a directory\n",
				   options->file);
		free_copy_options(options);
		return false;
	}

	result = PSQLexec(query.data, false);
	termPQExpBuffer(&query);

	switch (PQresultStatus(result))
	{
		case PGRES_COPY_OUT:
			success = handleCopyOut(pset.db, copystream);
			break;
		case PGRES_COPY_IN:
			success = handleCopyIn(pset.db, copystream, NULL);
			break;
		case PGRES_NONFATAL_ERROR:
		case PGRES_FATAL_ERROR:
		case PGRES_BAD_RESPONSE:
			success = false;
			psql_error("\\copy: %s", PQerrorMessage(pset.db));
			break;
		default:
			success = false;
			psql_error("\\copy: unexpected response (%d)\n", PQresultStatus(result));
	}

	PQclear(result);

	if (copystream != stdout && copystream != stdin)
		fclose(copystream);
	free_copy_options(options);
	return success;
}


#define COPYBUFSIZ 8192			/* size doesn't matter */


/*
 * handleCopyOut
 * receives data as a result of a COPY ... TO stdout command
 *
 * If you want to use COPY TO in your application, this is the code to steal :)
 *
 * conn should be a database connection that you just called COPY TO on
 * (and which gave you PGRES_COPY_OUT back);
 * copystream is the file stream you want the output to go to
 */
bool
handleCopyOut(PGconn *conn, FILE *copystream)
{
	bool		copydone = false;		/* haven't started yet */
	char		copybuf[COPYBUFSIZ];
	int			ret;

	assert(cancelConn);

	while (!copydone)
	{
		ret = PQgetline(conn, copybuf, COPYBUFSIZ);

		if (copybuf[0] == '\\' &&
			copybuf[1] == '.' &&
			copybuf[2] == '\0')
		{
			copydone = true;	/* we're at the end */
		}
		else
		{
			fputs(copybuf, copystream);
			switch (ret)
			{
				case EOF:
					copydone = true;
					/* FALLTHROUGH */
				case 0:
					fputc('\n', copystream);
					break;
				case 1:
					break;
			}
		}
	}
	fflush(copystream);
	ret = !PQendcopy(conn);
	cancelConn = NULL;
	return ret;
}



/*
 * handleCopyIn
 * receives data as a result of a COPY ... FROM stdin command
 *
 * Again, if you want to use COPY FROM in your application, copy this.
 *
 * conn should be a database connection that you just called COPY FROM on
 * (and which gave you PGRES_COPY_IN back);
 * copystream is the file stream you want the input to come from
 * prompt is something to display to request user input (only makes sense
 *	 if stdin is an interactive tty)
 */

bool
handleCopyIn(PGconn *conn, FILE *copystream, const char *prompt)
{
	bool		copydone = false;
	bool		firstload;
	bool		linedone;
	char		copybuf[COPYBUFSIZ];
	char	   *s;
	int			bufleft;
	int			c = 0;
	int			ret;
	unsigned int linecount = 0;

#ifdef USE_ASSERT_CHECKING
	assert(copy_in_state);
#endif

	if (prompt)					/* disable prompt if not interactive */
	{
		if (!isatty(fileno(copystream)))
			prompt = NULL;
	}

	while (!copydone)
	{							/* for each input line ... */
		if (prompt)
		{
			fputs(prompt, stdout);
			fflush(stdout);
		}
		firstload = true;
		linedone = false;

		while (!linedone)
		{						/* for each bufferload in line ... */
			s = copybuf;
			for (bufleft = COPYBUFSIZ - 1; bufleft > 0; bufleft--)
			{
				c = getc(copystream);
				if (c == '\n' || c == EOF)
				{
					linedone = true;
					break;
				}
				*s++ = c;
			}
			*s = '\0';
			if (c == EOF && s == copybuf && firstload)
			{
				PQputline(conn, "\\.");
				copydone = true;
				if (pset.cur_cmd_interactive)
					puts("\\.");
				break;
			}
			PQputline(conn, copybuf);
			if (firstload)
			{
				if (!strcmp(copybuf, "\\."))
				{
					copydone = true;
					break;
				}
				firstload = false;
			}
		}
		PQputline(conn, "\n");
		linecount++;
	}
	ret = !PQendcopy(conn);
	copy_in_state = false;
	pset.lineno += linecount;
	return ret;
}
