#include <config.h>
#include <c.h>

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#ifdef WIN32
#include <io.h>
#include <window.h>
#else
#include <unistd.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <libpq-fe.h>
#include <pqsignal.h>
#include <version.h>

#include "settings.h"
#include "command.h"
#include "help.h"
#include "mainloop.h"
#include "common.h"
#include "input.h"
#include "variables.h"
#include "print.h"
#include "describe.h"



static void
process_psqlrc(PsqlSettings *pset);

static void
showVersion(void);


/* Structures to pass information between the option parsing routine
 * and the main function
 */
enum _actions
{
	ACT_NOTHING = 0,
	ACT_SINGLE_SLASH,
	ACT_LIST_DB,
	ACT_SINGLE_QUERY,
	ACT_FILE
};

struct adhoc_opts
{
	char	   *dbname;
	char	   *host;
	char	   *port;
	char	   *username;
	enum _actions action;
	char	   *action_string;
	bool		no_readline;
};

static void
parse_options(int argc, char *argv[], PsqlSettings *pset, struct adhoc_opts * options);



/*
 *
 * main()
 *
 */
int
main(int argc, char **argv)
{
	PsqlSettings settings;
	struct adhoc_opts options;
	int			successResult;

	char	   *username = NULL;
	char	   *password = NULL;
	bool		need_pass;

	memset(&settings, 0, sizeof settings);

    if (!strrchr(argv[0], SEP_CHAR))
        settings.progname = argv[0];
    else
        settings.progname = strrchr(argv[0], SEP_CHAR) + 1;

	settings.cur_cmd_source = stdin;
	settings.cur_cmd_interactive = false;

	settings.vars = CreateVariableSpace();
	settings.popt.topt.format = PRINT_ALIGNED;
	settings.queryFout = stdout;
	settings.popt.topt.fieldSep = strdup(DEFAULT_FIELD_SEP);
	settings.popt.topt.border = 1;
	settings.popt.topt.pager = 1;

	SetVariable(settings.vars, "prompt1", DEFAULT_PROMPT1);
	SetVariable(settings.vars, "prompt2", DEFAULT_PROMPT2);
	SetVariable(settings.vars, "prompt3", DEFAULT_PROMPT3);

	settings.notty = (!isatty(fileno(stdin)) || !isatty(fileno(stdout)));

	/* This is obsolete and will be removed very soon. */
#ifdef PSQL_ALWAYS_GET_PASSWORDS
	settings.getPassword = true;
#else
	settings.getPassword = false;
#endif

#ifdef MULTIBYTE
	settings.has_client_encoding = (getenv("PGCLIENTENCODING") != NULL);
#endif

	parse_options(argc, argv, &settings, &options);

	if (options.action == ACT_LIST_DB)
		options.dbname = "template1";

	if (options.username)
	{
		if (strcmp(options.username, "?") == 0)
			username = simple_prompt("Username: ", 100, true);
		else
			username = strdup(options.username);
	}

	if (settings.getPassword)
		password = simple_prompt("Password: ", 100, false);

	/* loop until we have a password if requested by backend */
	do
	{
		need_pass = false;
		settings.db = PQsetdbLogin(options.host, options.port, NULL, NULL, options.dbname, username, password);

		if (PQstatus(settings.db) == CONNECTION_BAD &&
			strcmp(PQerrorMessage(settings.db), "fe_sendauth: no password supplied\n") == 0)
		{
			need_pass = true;
			free(password);
			password = NULL;
			password = simple_prompt("Password: ", 100, false);
		}
	} while (need_pass);

	free(username);
	free(password);

	if (PQstatus(settings.db) == CONNECTION_BAD)
	{
		fprintf(stderr, "%s: connection to database '%s' failed.\n%s",
                settings.progname, PQdb(settings.db),
				PQerrorMessage(settings.db));
		PQfinish(settings.db);
		exit(EXIT_BADCONN);
	}

	if (options.action == ACT_LIST_DB)
	{
		int			success = listAllDbs(&settings, false);

		PQfinish(settings.db);
		exit(!success);
	}


	if (!GetVariable(settings.vars, "quiet") && !settings.notty && !options.action)
	{
		printf("Welcome to %s, the PostgreSQL interactive terminal.\n\n"
               "Type:  \\copyright for distribution terms\n"
               "       \\h for help with SQL commands\n"
               "       \\? for help on internal slash commands\n"
               "       \\g or terminate with semicolon to execute query\n"
               "       \\q to quit\n", settings.progname);
	}

	process_psqlrc(&settings);

	initializeInput(options.no_readline ? 0 : 1, &settings);

	/* Now find something to do */

	/* process file given by -f */
	if (options.action == ACT_FILE)
		successResult = process_file(options.action_string, &settings) ? 0 : 1;
	/* process slash command if one was given to -c */
	else if (options.action == ACT_SINGLE_SLASH)
		successResult = HandleSlashCmds(&settings, options.action_string, NULL, NULL) != CMD_ERROR ? 0 : 1;
	/* If the query given to -c was a normal one, send it */
	else if (options.action == ACT_SINGLE_QUERY)
		successResult = SendQuery(&settings, options.action_string) ? 0 : 1;
	/* or otherwise enter interactive main loop */
	else
		successResult = MainLoop(&settings, stdin);

	/* clean up */
	finishInput();
	PQfinish(settings.db);
	setQFout(NULL, &settings);
	DestroyVariableSpace(settings.vars);

	return successResult;
}



/*
 * Parse command line options
 */

#ifdef WIN32
/* getopt is not in the standard includes on Win32 */
int			getopt(int, char *const[], const char *);

#endif

static void
parse_options(int argc, char *argv[], PsqlSettings *pset, struct adhoc_opts * options)
{
#ifdef HAVE_GETOPT_LONG
	static struct option long_options[] = {
		{"no-align", no_argument, NULL, 'A'},
		{"command", required_argument, NULL, 'c'},
		{"database", required_argument, NULL, 'd'},
		{"dbname", required_argument, NULL, 'd'},
		{"echo", no_argument, NULL, 'e'},
		{"echo-queries", no_argument, NULL, 'e'},
		{"echo-all", no_argument, NULL, 'E'},
		{"echo-all-queries", no_argument, NULL, 'E'},
		{"file", required_argument, NULL, 'f'},
		{"field-separator", required_argument, NULL, 'F'},
		{"host", required_argument, NULL, 'h'},
		{"html", no_argument, NULL, 'H'},
		{"list", no_argument, NULL, 'l'},
		{"no-readline", no_argument, NULL, 'n'},
		{"output", required_argument, NULL, 'o'},
		{"port", required_argument, NULL, 'p'},
		{"pset", required_argument, NULL, 'P'},
		{"quiet", no_argument, NULL, 'q'},
		{"single-step", no_argument, NULL, 's'},
		{"single-line", no_argument, NULL, 'S'},
		{"tuples-only", no_argument, NULL, 't'},
		{"table-attr", required_argument, NULL, 'T'},
		{"username", required_argument, NULL, 'U'},
		{"expanded", no_argument, NULL, 'x'},
		{"set", required_argument, NULL, 'v'},
		{"variable", required_argument, NULL, 'v'},
		{"version", no_argument, NULL, 'V'},
		{"password", no_argument, NULL, 'W'},
		{"help", no_argument, NULL, '?'},
	};

	int			optindex;

#endif

	extern char *optarg;
	extern int	optind;
	int			c;

	memset(options, 0, sizeof *options);

#ifdef HAVE_GETOPT_LONG
	while ((c = getopt_long(argc, argv, "Ac:d:eEf:F:lh:Hno:p:P:qsStT:uU:v:VWx?", long_options, &optindex)) != -1)
#else

	/*
	 * Be sure to leave the '-' in here, so we can catch accidental long
	 * options.
	 */
	while ((c = getopt(argc, argv, "Ac:d:eEf:F:lh:Hno:p:P:qsStT:uU:v:VWx?-")) != -1)
#endif
	{
		switch (c)
		{
			case 'A':
				pset->popt.topt.format = PRINT_UNALIGNED;
				break;
			case 'c':
				options->action_string = optarg;
				if (optarg[0] == '\\')
					options->action = ACT_SINGLE_SLASH;
				else
					options->action = ACT_SINGLE_QUERY;
				break;
			case 'd':
				options->dbname = optarg;
				break;
			case 'e':
				SetVariable(pset->vars, "echo", "");
				break;
			case 'E':
				SetVariable(pset->vars, "echo_secret", "");
				break;
			case 'f':
				options->action = ACT_FILE;
				options->action_string = optarg;
				break;
			case 'F':
				pset->popt.topt.fieldSep = strdup(optarg);
				break;
			case 'h':
				options->host = optarg;
				break;
			case 'H':
				pset->popt.topt.format = PRINT_HTML;
				break;
			case 'l':
				options->action = ACT_LIST_DB;
				break;
			case 'n':
				options->no_readline = true;
				break;
			case 'o':
				setQFout(optarg, pset);
				break;
			case 'p':
				options->port = optarg;
				break;
			case 'P':
				{
					char	   *value;
					char	   *equal_loc;
					bool		result;

					value = xstrdup(optarg);
					equal_loc = strchr(value, '=');
					if (!equal_loc)
						result = do_pset(value, NULL, &pset->popt, true);
					else
					{
						*equal_loc = '\0';
						result = do_pset(value, equal_loc + 1, &pset->popt, true);
					}

					if (!result)
					{
						fprintf(stderr, "Couldn't set printing paramter %s.\n", value);
						exit(EXIT_FAILURE);
					}

					free(value);
					break;
				}
			case 'q':
				SetVariable(pset->vars, "quiet", "");
				break;
			case 's':
				SetVariable(pset->vars, "singlestep", "");
				break;
			case 'S':
				SetVariable(pset->vars, "singleline", "");
				break;
			case 't':
				pset->popt.topt.tuples_only = true;
				break;
			case 'T':
				pset->popt.topt.tableAttr = xstrdup(optarg);
				break;
			case 'u':
				pset->getPassword = true;
				options->username = "?";
				break;
			case 'U':
				options->username = optarg;
				break;
			case 'x':
				pset->popt.topt.expanded = true;
				break;
			case 'v':
				{
					char	   *value;
					char	   *equal_loc;

					value = xstrdup(optarg);
					equal_loc = strchr(value, '=');
					if (!equal_loc)
					{
						if (!DeleteVariable(pset->vars, value))
						{
							fprintf(stderr, "%s: could not delete variable %s\n",
                                    pset->progname, value);
							exit(EXIT_FAILURE);
						}
					}
					else
					{
						*equal_loc = '\0';
						if (!SetVariable(pset->vars, value, equal_loc + 1))
						{
							fprintf(stderr, "%s: Couldn't set variable %s to %s\n",
                                    pset->progname, value, equal_loc);
							exit(EXIT_FAILURE);
						}
					}

					free(value);
					break;
				}
			case 'V':
				showVersion();
				exit(EXIT_SUCCESS);
			case 'W':
				pset->getPassword = true;
				break;
			case '?':
				usage();
				exit(EXIT_SUCCESS);
				break;
#ifndef HAVE_GETOPT_LONG
			case '-':
				fprintf(stderr, "%s was compiled without support for long options.\n"
						"Use -? for help on invocation options.\n", pset->progname);
				exit(EXIT_FAILURE);
				break;
#endif
			default:
				usage();
				exit(EXIT_FAILURE);
				break;
		}
	}

	/*
	 * if we still have arguments, use it as the database name and
	 * username
	 */
	while (argc - optind >= 1)
	{
		if (!options->dbname)
			options->dbname = argv[optind];
		else if (!options->username)
			options->username = argv[optind];
		else
			fprintf(stderr, "%s: warning: extra option %s ignored\n",
                    pset->progname, argv[optind]);

		optind++;
	}
}



/*
 * Load /etc/psqlrc or .psqlrc file, if found.
 */
static void
process_psqlrc(PsqlSettings *pset)
{
	char	   *psqlrc;
	char	   *home;

#ifdef WIN32
#define R_OK 0
#endif

	/* System-wide startup file */
	if (access("/etc/psqlrc-" PG_RELEASE "." PG_VERSION "." PG_SUBVERSION, R_OK) == 0)
		process_file("/etc/psqlrc-" PG_RELEASE "." PG_VERSION "." PG_SUBVERSION, pset);
	else if (access("/etc/psqlrc", R_OK) == 0)
		process_file("/etc/psqlrc", pset);

	/* Look for one in the home dir */
	home = getenv("HOME");

	if (home)
	{
		psqlrc = (char *) malloc(strlen(home) + 20);
		if (!psqlrc)
		{
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		sprintf(psqlrc, "%s/.psqlrc-" PG_RELEASE "." PG_VERSION "." PG_SUBVERSION, home);
		if (access(psqlrc, R_OK) == 0)
			process_file(psqlrc, pset);
		else
		{
			sprintf(psqlrc, "%s/.psqlrc", home);
			if (access(psqlrc, R_OK) == 0)
				process_file(psqlrc, pset);
		}
		free(psqlrc);
	}
}



/* showVersion
 *
 * This output format is intended to match GNU standards.
 */
static void
showVersion(void)
{
    puts("psql (PostgreSQL) " PG_RELEASE "." PG_VERSION "." PG_SUBVERSION);

#if defined(USE_READLINE) || defined (USE_HISTORY) || defined(MULTIBYTE)
    fputs("contains ", stdout);

#ifdef USE_READLINE
    fputs("readline", stdout);
#define _Feature
#endif

#ifdef USE_HISTORY
#ifdef _Feature
    fputs(", ", stdout);
#else
#define _Feature
#endif
    fputs("history", stdout);
#endif

#ifdef MULTIBYTE
#ifdef _Feature
    fputs(", ", stdout);
#else
#define _Feature
#endif
    fputs("multibyte");
#endif
    
#undef _Feature

    puts(" support");
#endif

    puts("Copyright (C) 2000 PostgreSQL Global Development Team");
    puts("Copyright (C) 1996 Regents of the University of California");
    puts("Read the file COPYING or use the command \\copyright to see the");
    puts("usage and distribution terms.");
}
