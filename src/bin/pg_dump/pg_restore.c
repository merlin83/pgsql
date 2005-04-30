/*-------------------------------------------------------------------------
 *
 * pg_restore.c
 *	pg_restore is an utility extracting postgres database definitions
 *	from a backup archive created by pg_dump using the archiver
 *	interface.
 *
 *	pg_restore will read the backup archive and
 *	dump out a script that reproduces
 *	the schema of the database in terms of
 *		  user-defined types
 *		  user-defined functions
 *		  tables
 *		  indexes
 *		  aggregates
 *		  operators
 *		  ACL - grant/revoke
 *
 * the output script is SQL that is understood by PostgreSQL
 *
 * Basic process in a restore operation is:
 *
 *	Open the Archive and read the TOC.
 *	Set flags in TOC entries, and *maybe* reorder them.
 *	Generate script to stdout
 *	Exit
 *
 * Copyright (c) 2000, Philip Warner
 *		Rights are granted to use this software in any way so long
 *		as this notice is not removed.
 *
 *	The author is not responsible for loss or damages that may
 *	result from its use.
 *
 *
 * IDENTIFICATION
 *		$Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/bin/pg_dump/pg_restore.c,v 1.43.2.1 2005-04-30 08:01:29 neilc Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "pg_backup.h"
#include "pg_backup_archiver.h"
#include "dumputils.h"

#include <ctype.h>

#ifndef HAVE_STRDUP
#include "strdup.h"
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#include <unistd.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef ENABLE_NLS
#include <locale.h>
#endif

#define _(x) gettext((x))

/* Forward decls */
static void usage(const char *progname);
static char *_cleanupName(char *name);
static char *_cleanupFuncName(char *name);

typedef struct option optType;


int
main(int argc, char **argv)
{
	RestoreOptions *opts;
	int			c;
	Archive    *AH;
	char	   *fileSpec = NULL;
	extern int	optind;
	extern char *optarg;
	static int	use_setsessauth = 0;
	static int	disable_triggers = 0;

#ifdef HAVE_GETOPT_LONG
	struct option cmdopts[] = {
		{"clean", 0, NULL, 'c'},
		{"create", 0, NULL, 'C'},
		{"data-only", 0, NULL, 'a'},
		{"dbname", 1, NULL, 'd'},
		{"file", 1, NULL, 'f'},
		{"format", 1, NULL, 'F'},
		{"function", 1, NULL, 'P'},
		{"host", 1, NULL, 'h'},
		{"ignore-version", 0, NULL, 'i'},
		{"index", 1, NULL, 'I'},
		{"list", 0, NULL, 'l'},
		{"no-privileges", 0, NULL, 'x'},
		{"no-acl", 0, NULL, 'x'},
		{"no-owner", 0, NULL, 'O'},
		{"no-reconnect", 0, NULL, 'R'},
		{"port", 1, NULL, 'p'},
		{"oid-order", 0, NULL, 'o'},
		{"orig-order", 0, NULL, 'N'},
		{"password", 0, NULL, 'W'},
		{"rearrange", 0, NULL, 'r'},
		{"schema-only", 0, NULL, 's'},
		{"superuser", 1, NULL, 'S'},
		{"table", 1, NULL, 't'},
		{"trigger", 1, NULL, 'T'},
		{"use-list", 1, NULL, 'L'},
		{"username", 1, NULL, 'U'},
		{"verbose", 0, NULL, 'v'},

		/*
		 * the following options don't have an equivalent short option
		 * letter, but are available as '-X long-name'
		 */
		{"use-set-session-authorization", no_argument, &use_setsessauth, 1},
		{"disable-triggers", no_argument, &disable_triggers, 1},

		{NULL, 0, NULL, 0}
	};
#endif   /* HAVE_GETOPT_LONG */


#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain("pg_dump", LOCALEDIR);
	textdomain("pg_dump");
#endif

	opts = NewRestoreOptions();

	if (!strrchr(argv[0], '/'))
		progname = argv[0];
	else
		progname = strrchr(argv[0], '/') + 1;

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			usage(progname);
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("pg_restore (PostgreSQL) " PG_VERSION);
			exit(0);
		}
	}

#ifdef HAVE_GETOPT_LONG
	while ((c = getopt_long(argc, argv, "acCd:f:F:h:iI:lL:NoOp:P:rRsS:t:T:uU:vWxX:", cmdopts, NULL)) != -1)
#else
	while ((c = getopt(argc, argv, "acCd:f:F:h:iI:lL:NoOp:P:rRsS:t:T:uU:vWxX:")) != -1)
#endif
	{
		switch (c)
		{
			case 'a':			/* Dump data only */
				opts->dataOnly = 1;
				break;
			case 'c':			/* clean (i.e., drop) schema prior to
								 * create */
				opts->dropSchema = 1;
				break;
			case 'C':
				opts->create = 1;
				break;
			case 'd':
				if (strlen(optarg) != 0)
				{
					opts->dbname = strdup(optarg);
					opts->useDB = 1;
				}
				break;
			case 'f':			/* output file name */
				opts->filename = strdup(optarg);
				break;
			case 'F':
				if (strlen(optarg) != 0)
					opts->formatName = strdup(optarg);
				break;
			case 'h':
				if (strlen(optarg) != 0)
					opts->pghost = strdup(optarg);
				break;
			case 'i':
				opts->ignoreVersion = 1;
				break;

			case 'l':			/* Dump the TOC summary */
				opts->tocSummary = 1;
				break;

			case 'L':			/* input TOC summary file name */
				opts->tocFile = strdup(optarg);
				break;

			case 'N':
				opts->origOrder = 1;
				break;
			case 'o':
				opts->oidOrder = 1;
				break;
			case 'O':
				opts->noOwner = 1;
				break;
			case 'p':
				if (strlen(optarg) != 0)
					opts->pgport = strdup(optarg);
				break;
			case 'r':
				opts->rearrange = 1;
				break;
			case 'R':
				opts->noReconnect = 1;
				break;
			case 'P':			/* Function */
				opts->selTypes = 1;
				opts->selFunction = 1;
				opts->functionNames = _cleanupFuncName(optarg);
				break;
			case 'I':			/* Index */
				opts->selTypes = 1;
				opts->selIndex = 1;
				opts->indexNames = _cleanupName(optarg);
				break;
			case 'T':			/* Trigger */
				opts->selTypes = 1;
				opts->selTrigger = 1;
				opts->triggerNames = _cleanupName(optarg);
				break;
			case 's':			/* dump schema only */
				opts->schemaOnly = 1;
				break;
			case 'S':			/* Superuser username */
				if (strlen(optarg) != 0)
					opts->superuser = strdup(optarg);
				break;
			case 't':			/* Dump data for this table only */
				opts->selTypes = 1;
				opts->selTable = 1;
				opts->tableNames = _cleanupName(optarg);
				break;

			case 'u':
				opts->requirePassword = true;
				opts->username = simple_prompt("User name: ", 100, true);
				break;

			case 'U':
				opts->username = optarg;
				break;

			case 'v':			/* verbose */
				opts->verbose = 1;
				break;

			case 'W':
				opts->requirePassword = true;
				break;

			case 'x':			/* skip ACL dump */
				opts->aclsSkip = 1;
				break;

			case 'X':
				if (strcmp(optarg, "use-set-session-authorization") == 0)
					use_setsessauth = 1;
				else if (strcmp(optarg, "disable-triggers") == 0)
					disable_triggers = 1;
				else
				{
					fprintf(stderr,
							_("%s: invalid -X option -- %s\n"),
							progname, optarg);
					fprintf(stderr, _("Try '%s --help' for more information.\n"), progname);
					exit(1);
				}
				break;

#ifdef HAVE_GETOPT_LONG
				/* This covers the long options equivalent to -X xxx. */
			case 0:
				break;
#endif

			default:
				fprintf(stderr, _("Try '%s --help' for more information.\n"), progname);
				exit(1);
		}
	}

	if (optind < argc)
		fileSpec = argv[optind];
	else
		fileSpec = NULL;

	opts->use_setsessauth = use_setsessauth;
	opts->disable_triggers = disable_triggers;

	if (opts->formatName)
	{

		switch (opts->formatName[0])
		{

			case 'c':
			case 'C':
				opts->format = archCustom;
				break;

			case 'f':
			case 'F':
				opts->format = archFiles;
				break;

			case 't':
			case 'T':
				opts->format = archTar;
				break;

			default:
				write_msg(NULL, "unrecognized archive format '%s'; please specify 't' or 'c'\n",
						  opts->formatName);
				exit(1);
		}
	}

	AH = OpenArchive(fileSpec, opts->format);

	/* Let the archiver know how noisy to be */
	AH->verbose = opts->verbose;

	if (opts->tocFile)
		SortTocFromFile(AH, opts);

	if (opts->oidOrder)
		SortTocByOID(AH);
	else if (opts->origOrder)
		SortTocByID(AH);

	if (opts->rearrange)
	{
		MoveToStart(AH, "<Init>");
		MoveToEnd(AH, "TABLE DATA");
		MoveToEnd(AH, "BLOBS");
		MoveToEnd(AH, "INDEX");
		MoveToEnd(AH, "TRIGGER");
		MoveToEnd(AH, "RULE");
		MoveToEnd(AH, "SEQUENCE SET");
	}

	/* Database MUST be at start */
	MoveToStart(AH, "DATABASE");

	if (opts->tocSummary)
		PrintTOCSummary(AH, opts);
	else
		RestoreArchive(AH, opts);

	CloseArchive(AH);

	return 0;
}

static void
usage(const char *progname)
{
	printf(_("%s restores a PostgreSQL database from an archive created by pg_dump.\n\n"), progname);
	printf(_("Usage:\n"));
	printf(_("  %s [OPTION]... [FILE]\n"), progname);

	printf(_("\nGeneral options:\n"));
#ifdef HAVE_GETOPT_LONG
	printf(_("  -d, --dbname=NAME        output database name\n"));
	printf(_("  -f, --file=FILENAME      output file name\n"));
	printf(_("  -F, --format=c|t         specify backup file format\n"));
	printf(_("  -i, --ignore-version     proceed even when server version mismatches\n"));
	printf(_("  -l, --list               print summarized TOC of the archive\n"));
	printf(_("  -v, --verbose            verbose mode\n"));
#else /* not HAVE_GETOPT_LONG */
	printf(_("  -d NAME                  output database name\n"));
	printf(_("  -f FILENAME              output file name\n"));
	printf(_("  -F c|t                   specify backup file format\n"));
	printf(_("  -i                       proceed even when server version mismatches\n"));
	printf(_("  -l                       print summarized TOC of the archive\n"));
	printf(_("  -v                       verbose mode\n"));
#endif /* not HAVE_GETOPT_LONG */
	printf(_("  --help                   show this help, then exit\n"));
	printf(_("  --version                output version information, then exit\n"));

	printf(_("\nOptions controlling the output content:\n"));
#ifdef HAVE_GETOPT_LONG
	printf(_("  -a, --data-only          restore only the data, no schema\n"));
	printf(_("  -c, --clean              clean (drop) schema prior to create\n"));
	printf(_("  -C, --create             issue commands to create the database\n"));
	printf(_("  -I, --index=NAME         restore named index\n"));
	printf(_("  -L, --use-list=FILENAME  use specified table of contents for ordering\n"
			 "                           output from this file\n"));
	printf(_("  -N, --orig-order         restore in original dump order\n"));
	printf(_("  -o, --oid-order          restore in OID order\n"));
	printf(_("  -O, --no-owner           do not reconnect to database to match\n"
			 "                           object owner\n"));
	printf(_("  -P, --function=NAME(args)\n"
			 "                           restore named function\n"));
	printf(_("  -r, --rearrange          rearrange output to put indexes etc. at end\n"));
	printf(_("  -R, --no-reconnect       disallow ALL reconnections to the database\n"));
	printf(_("  -s, --schema-only        restore only the schema, no data\n"));
	printf(_("  -S, --superuser=NAME     specify the superuser user name to use for\n"
			 "                           disabling triggers\n"));
	printf(_("  -t, --table=NAME         restore named table\n"));
	printf(_("  -T, --trigger=NAME       restore named trigger\n"));
	printf(_("  -x, --no-privileges      skip restoration of access privileges (grant/revoke)\n"));
	printf(_("  -X use-set-session-authorization, --use-set-session-authorization\n"
			 "                           use SET SESSION AUTHORIZATION commands instead\n"
			 "                           of reconnecting, if possible\n"));
	printf(_("  -X disable-triggers, --disable-triggers\n"
			 "                           disable triggers during data-only restore\n"));
#else /* not HAVE_GETOPT_LONG */
	printf(_("  -a                       restore only the data, no schema\n"));
	printf(_("  -c                       clean (drop) schema prior to create\n"));
	printf(_("  -C                       issue commands to create the database\n"));
	printf(_("  -I NAME                  restore named index\n"));
	printf(_("  -L FILENAME              use specified table of contents for ordering\n"
			 "                           output from this file\n"));
	printf(_("  -N                       restore in original dump order\n"));
	printf(_("  -o                       restore in OID order\n"));
	printf(_("  -O                       do not reconnect to database to match\n"
			 "                           object owner\n"));
	printf(_("  -P NAME(args)            restore named function\n"));
	printf(_("  -r                       rearrange output to put indexes etc. at end\n"));
	printf(_("  -R                       disallow ALL reconnections to the database\n"));
	printf(_("  -s                       restore only the schema, no data\n"));
	printf(_("  -S NAME                  specify the superuser user name to use for\n"
			 "                           disabling triggers\n"));
	printf(_("  -t NAME                  restore named table\n"));
	printf(_("  -T NAME                  restore named trigger\n"));
	printf(_("  -x                       skip restoration of access privileges (grant/revoke)\n"));
	printf(_("  -X use-set-session-authorization\n"
			 "                           use SET SESSION AUTHORIZATION commands instead\n"
			 "                           of reconnecting, if possible\n"));
	printf(_("  -X disable-triggers      disable triggers during data-only restore\n"));
#endif /* not HAVE_GETOPT_LONG */

	printf(_("\nConnection options:\n"));
#ifdef HAVE_GETOPT_LONG
	printf(_("  -h, --host=HOSTNAME      database server host name\n"));
	printf(_("  -p, --port=PORT          database server port number\n"));
	printf(_("  -U, --username=NAME      connect as specified database user\n"));
	printf(_("  -W, --password           force password prompt (should happen automatically)\n"));
#else /* not HAVE_GETOPT_LONG */
	printf(_("  -h HOSTNAME              database server host name\n"));
	printf(_("  -p PORT                  database server port number\n"));
	printf(_("  -U NAME                  connect as specified database user\n"));
	printf(_("  -W                       force password prompt (should happen automatically)\n"));
#endif /* not HAVE_GETOPT_LONG */

	printf(_("\nIf no input file name is supplied, then standard input is used.\n\n"));
	printf(_("Report bugs to <pgsql-bugs@postgresql.org>.\n"));
}


static char *
_cleanupName(char *name)
{
	int			i;

	if (!name || !name[0])
		return NULL;

	name = strdup(name);

	if (name[0] == '"')
	{
		strcpy(name, &name[1]);
		if (name[0] && *(name + strlen(name) - 1) == '"')
			*(name + strlen(name) - 1) = '\0';
	}
	/* otherwise, convert table name to lowercase... */
	else
	{
		for (i = 0; name[i]; i++)
			if (isupper((unsigned char) name[i]))
				name[i] = tolower((unsigned char) name[i]);
	}
	return name;
}


static char *
_cleanupFuncName(char *name)
{
	int			i;
	char	   *ch;

	if (!name || !name[0])
		return NULL;

	name = strdup(name);

	if (name[0] == '"')
	{
		strcpy(name, &name[1]);
		if (strchr(name, '"') != NULL)
			strcpy(strchr(name, '"'), strchr(name, '"') + 1);
	}
	/* otherwise, convert function name to lowercase... */
	else
	{
		for (i = 0; name[i]; i++)
			if (isupper((unsigned char) name[i]))
				name[i] = tolower((unsigned char) name[i]);
	}

	/* strip out any space before paren */
	ch = strchr(name, '(');
	while (ch && ch > name && *(ch - 1) == ' ')
	{
		strcpy(ch - 1, ch);
		ch--;
	}

	/*
	 * Strip out spaces after commas in parameter list. We can't remove
	 * all spaces because some types, like 'double precision' have spaces.
	 */
	if ((ch = strchr(name, '(')) != NULL)
	{
		while ((ch = strstr(ch, ", ")) != NULL)
			strcpy(ch + 1, ch + 2);
	}

	return name;
}
