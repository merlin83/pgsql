/*-------------------------------------------------------------------------
 *
 * postgres.c--
 *	  POSTGRES C Backend Interface
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/tcop/postgres.c,v 1.47 1997-09-18 05:19:17 vadim Exp $
 *
 * NOTES
 *	  this is the "main" module of the postgres backend and
 *	  hence the main module of the "traffic cop".
 *
 *-------------------------------------------------------------------------
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/param.h>			/* for MAXHOSTNAMELEN on most */
#ifndef MAXHOSTNAMELEN
#include <netdb.h>				/* for MAXHOSTNAMELEN on some */
#endif
#ifndef MAXHOSTNAMELEN			/* for MAXHOSTNAMELEN under sco3.2v5.0.2 */
#include <sys/socket.h>
#endif
#include <errno.h>
#ifdef aix
#include <sys/select.h>
#endif							/* aix */


#include "postgres.h"
#include "miscadmin.h"
#include "catalog/catname.h"
#include "access/xact.h"

#include "lib/dllist.h"

#include "parser/catalog_utils.h"
#include "parser/parse_query.h" /* for MakeTimeRange() */
#include "commands/async.h"
#include "tcop/tcopprot.h"		/* where declarations for this file go */
#include "optimizer/planner.h"

#include "tcop/tcopprot.h"
#include "tcop/tcopdebug.h"

#include "executor/execdebug.h"
#include "executor/executor.h"
#if FALSE
#include "nodes/relation.h"
#endif
#include "nodes/print.h"

#include "optimizer/cost.h"
#include "optimizer/planner.h"
#if 0
#include "optimizer/xfunc.h"
#endif
#include "optimizer/prep.h"
#if FALSE
#include "nodes/plannodes.h"
#endif

#include "storage/bufmgr.h"
#include "fmgr.h"
#include "utils/palloc.h"
#include "utils/rel.h"

#include "nodes/pg_list.h"
#include "tcop/dest.h"
#if FALSE
#include "nodes/memnodes.h"
#endif
#include "utils/mcxt.h"
#include "tcop/pquery.h"
#include "tcop/utility.h"
#include "tcop/fastpath.h"

#include "libpq/libpq.h"
#include "libpq/pqsignal.h"
#include "rewrite/rewriteHandler.h"		/* for QueryRewrite() */

static void quickdie(SIGNAL_ARGS);

/* ----------------
 *		global variables
 * ----------------
 */
static bool DebugPrintQuery = false;
static bool DebugPrintPlan = false;
static bool DebugPrintParse = false;
static bool DebugPrintRewrittenParsetree = false;

/*static bool	EnableRewrite = true; , never changes why have it*/
CommandDest whereToSendOutput;

#ifdef LOCK_MGR_DEBUG
extern int	lockDebug;

#endif
extern int	lockingOff;
extern int	NBuffers;

int			dontExecute = 0;
static int	ShowStats;
static bool IsEmptyQuery = false;

char		relname[80];		/* current relation name */

#if defined(nextstep)
jmp_buf		Warn_restart;

#define sigsetjmp(x,y)	setjmp(x)
#define siglongjmp longjmp
#else
sigjmp_buf	Warn_restart;

#endif							/* defined(nextstep) */
int			InWarn;

extern int	NBuffers;

static int	EchoQuery = 0;		/* default don't echo */
time_t		tim;
char		pg_pathname[256];
static int	ShowParserStats;
static int	ShowPlannerStats;
int			ShowExecutorStats;
FILE	   *StatFp;

typedef struct frontend
{
	bool		fn_connected;
	Port		fn_port;
	FILE	   *fn_Pfin;		/* the input fd */
	FILE	   *fn_Pfout;		/* the output fd */
	bool		fn_done;		/* set after the frontend closes its
								 * connection */
} FrontEnd;

static Dllist *frontendList;

/* ----------------
 *		people who want to use EOF should #define DONTUSENEWLINE in
 *		tcop/tcopdebug.h
 * ----------------
 */
#ifndef TCOP_DONTUSENEWLINE
int			UseNewLine = 1;		/* Use newlines query delimiters (the
								 * default) */

#else
int			UseNewLine = 0;		/* Use EOF as query delimiters */

#endif							/* TCOP_DONTUSENEWLINE */

/* ----------------
 *		bushy tree plan flag: if true planner will generate bushy-tree
 *		plans
 * ----------------
 */
int			BushyPlanFlag = 0;	/* default to false -- consider only
								 * left-deep trees */

/*
** Flags for expensive function optimization -- JMH 3/9/92
*/
int			XfuncMode = 0;

/*
 * ----------------
 *	 Note: _exec_repeat_ defaults to 1 but may be changed
 *		   by a DEBUG command.	 If you set this to a large
 *		   number N, run a single query, and then set it
 *		   back to 1 and run N queries, you can get an idea
 *		   of how much time is being spent in the parser and
 *		   planner b/c in the first case this overhead only
 *		   happens once.  -cim 6/9/91
 * ----------------
*/
int			_exec_repeat_ = 1;

/* ----------------------------------------------------------------
 *		decls for routines only used in this file
 * ----------------------------------------------------------------
 */
static char InteractiveBackend(char *inBuf);
static char SocketBackend(char *inBuf, bool multiplexedBackend);
static char ReadCommand(char *inBuf, bool multiplexedBackend);


/* ----------------------------------------------------------------
 *		routines to obtain user input
 * ----------------------------------------------------------------
 */

/* ----------------
 *	InteractiveBackend() is called for user interactive connections
 *	the string entered by the user is placed in its parameter inBuf.
 * ----------------
 */

static char
InteractiveBackend(char *inBuf)
{
	char	   *stuff = inBuf;	/* current place in input buffer */
	int			c;				/* character read from getc() */
	bool		end = false;	/* end-of-input flag */
	bool		backslashSeen = false;	/* have we seen a \ ? */

	/* ----------------
	 *	display a prompt and obtain input from the user
	 * ----------------
	 */
	printf("> ");

	for (;;)
	{
		if (UseNewLine)
		{
			/* ----------------
			 *	if we are using \n as a delimiter, then read
			 *	characters until the \n.
			 * ----------------
			 */
			while ((c = getc(stdin)) != EOF)
			{
				if (c == '\n')
				{
					if (backslashSeen)
					{
						stuff--;
						continue;
					}
					else
					{
						/* keep the newline character */
						*stuff++ = '\n';
						*stuff++ = '\0';
						break;
					}
				}
				else if (c == '\\')
					backslashSeen = true;
				else
					backslashSeen = false;

				*stuff++ = (char) c;
			}

			if (c == EOF)
				end = true;
		}
		else
		{
			/* ----------------
			 *	otherwise read characters until EOF.
			 * ----------------
			 */
			while ((c = getc(stdin)) != EOF)
				*stuff++ = (char) c;

			if (stuff == inBuf)
				end = true;
		}

		if (end)
		{
			if (!Quiet)
				puts("EOF");
			IsEmptyQuery = true;
			exitpg(0);
		}

		/* ----------------
		 *	otherwise we have a user query so process it.
		 * ----------------
		 */
		break;
	}

	/* ----------------
	 *	if the query echo flag was given, print the query..
	 * ----------------
	 */
	if (EchoQuery)
		printf("query is: %s\n", inBuf);

	return ('Q');
}

/* ----------------
 *	SocketBackend()		Is called for frontend-backend connections
 *
 *	If the input is a query (case 'Q') then the string entered by
 *	the user is placed in its parameter inBuf.
 *
 *	If the input is a fastpath function call (case 'F') then
 *	the function call is processed in HandleFunctionRequest().
 *	(now called from PostgresMain())
 * ----------------
 */

static char
SocketBackend(char *inBuf, bool multiplexedBackend)
{
	char		qtype[2];
	char		result = '\0';

	/* ----------------
	 *	get input from the frontend
	 * ----------------
	 */
	strcpy(qtype, "?");
	if (pq_getnchar(qtype, 0, 1) == EOF)
	{
		/* ------------
		 *	when front-end applications quits/dies
		 * ------------
		 */
		if (multiplexedBackend)
		{
			return 'X';
		}
		else
			exitpg(0);
	}

	switch (*qtype)
	{
			/* ----------------
			 *	'Q': user entered a query
			 * ----------------
			 */
		case 'Q':
			pq_getstr(inBuf, MAX_PARSE_BUFFER);
			result = 'Q';
			break;

			/* ----------------
			 *	'F':  calling user/system functions
			 * ----------------
			 */
		case 'F':
			pq_getstr(inBuf, MAX_PARSE_BUFFER); /* ignore the rest of the
												 * line */
			result = 'F';
			break;

			/* ----------------
			 *	'X':  frontend is exiting
			 * ----------------
			 */
		case 'X':
			result = 'X';
			break;

			/* ----------------
			 *	otherwise we got garbage from the frontend.
			 *
			 *	XXX are we certain that we want to do an elog(FATAL) here?
			 *		-cim 1/24/90
			 * ----------------
			 */
		default:
			elog(FATAL, "Socket command type %c unknown\n", *qtype);
			break;
	}
	return result;
}

/* ----------------
 *		ReadCommand reads a command from either the frontend or
 *		standard input, places it in inBuf, and returns a char
 *		representing whether the string is a 'Q'uery or a 'F'astpath
 *		call.
 * ----------------
 */
static char
ReadCommand(char *inBuf, bool multiplexedBackend)
{
	if (IsUnderPostmaster || multiplexedBackend)
		return SocketBackend(inBuf, multiplexedBackend);
	else
		return InteractiveBackend(inBuf);
}

List	   *
pg_plan(char *query_string,		/* string to execute */
		Oid *typev,				/* argument types */
		int nargs,				/* number of arguments */
		QueryTreeList **queryListP,		/* pointer to the parse trees */
		CommandDest dest)		/* where results should go */
{
	QueryTreeList *querytree_list;
	int			i;
	List	   *plan_list = NIL;
	Plan	   *plan;
	int			j;
	QueryTreeList *new_list;
	List	   *rewritten = NIL;
	Query	   *querytree;

	/* ----------------
	 *	(1) parse the request string into a list of parse trees
	 * ----------------
	 */
	if (ShowParserStats)
		ResetUsage();

	querytree_list = parser(query_string, typev, nargs);

	if (ShowParserStats)
	{
		fprintf(stderr, "! Parser Stats:\n");
		ShowUsage();
	}

	/* new_list holds the rewritten queries */
	new_list = (QueryTreeList *) malloc(sizeof(QueryTreeList));
	new_list->len = querytree_list->len;
	new_list->qtrees = (Query **) malloc(new_list->len * sizeof(Query *));

	/* ----------------
	 *	(2) rewrite the queries, as necessary
	 * ----------------
	 */
	j = 0;						/* counter for the new_list, new_list can
								 * be longer than old list as a result of
								 * rewrites */
	for (i = 0; i < querytree_list->len; i++)
	{
		querytree = querytree_list->qtrees[i];


		/* don't rewrite utilites */
		if (querytree->commandType == CMD_UTILITY)
		{
			new_list->qtrees[j++] = querytree;
			continue;
		}

		if (DebugPrintQuery == true)
		{
			printf("\n---- \tquery is:\n%s\n", query_string);
			printf("\n");
			fflush(stdout);
		}

		if (DebugPrintParse == true)
		{
			printf("\n---- \tparser outputs :\n");
			nodeDisplay(querytree);
			printf("\n");
		}

		/* rewrite queries (retrieve, append, delete, replace) */
		rewritten = QueryRewrite(querytree);
		if (rewritten != NULL)
		{
			int			len,
						k;

			len = length(rewritten);
			if (len == 1)
				new_list->qtrees[j++] = (Query *) lfirst(rewritten);
			else
			{
				/* rewritten queries are longer than original query */
				/* grow the new_list to accommodate */
				new_list->len += len - 1;		/* - 1 because originally
												 * we allocated one space
												 * for the query */
				new_list->qtrees = realloc(new_list->qtrees,
										new_list->len * sizeof(Query *));
				for (k = 0; k < len; k++)
					new_list->qtrees[j++] = (Query *) nth(k, rewritten);
			}
		}
	}

	/* we're done with the original lists, free it */
	free(querytree_list->qtrees);
	free(querytree_list);

	querytree_list = new_list;

	/* ----------------
	 * Fix time range quals
	 * this _must_ go here, because it must take place after rewrites
	 * ( if they take place ) so that time quals are usable by the executor
	 *
	 * Also, need to frob the range table entries here to plan union
	 * queries for archived relations.
	 * ----------------
	 */
	for (i = 0; i < querytree_list->len; i++)
	{
		List	   *l;
		List	   *rt = NULL;

		querytree = querytree_list->qtrees[i];

		/* ----------------
		 *	utilities don't have time ranges
		 * ----------------
		 */
		if (querytree->commandType == CMD_UTILITY)
			continue;

		rt = querytree->rtable;

		foreach(l, rt)
		{
			RangeTblEntry *rte = lfirst(l);
			TimeRange  *timequal = rte->timeRange;

			if (timequal)
			{
				int			timecode = (rte->timeRange->endDate == NULL) ? 0 : 1;

				rte->timeQual = makeTimeRange(rte->timeRange->startDate,
											  rte->timeRange->endDate,
											  timecode);
			}
			else
			{
				rte->timeQual = NULL;
			}
		}

		/* check for archived relations */
		plan_archive(rt);
	}

	if (DebugPrintRewrittenParsetree == true)
	{
		printf("\n---- \tafter rewriting:\n");

		for (i = 0; i < querytree_list->len; i++)
		{
			print(querytree_list->qtrees[i]);
			printf("\n");
		}
	}

	for (i = 0; i < querytree_list->len; i++)
	{
		querytree = querytree_list->qtrees[i];

		/*
		 * For each query that isn't a utility invocation, generate a
		 * plan.
		 */

		if (querytree->commandType != CMD_UTILITY)
		{

			if (IsAbortedTransactionBlockState())
			{
				/* ----------------
				 *	 the EndCommand() stuff is to tell the frontend
				 *	 that the command ended. -cim 6/1/90
				 * ----------------
				 */
				char	   *tag = "*ABORT STATE*";

				EndCommand(tag, dest);

				elog(NOTICE, "(transaction aborted): %s",
					 "queries ignored until END");

				*queryListP = (QueryTreeList *) NULL;
				return (List *) NULL;
			}

			if (ShowPlannerStats)
				ResetUsage();
			plan = planner(querytree);
			if (ShowPlannerStats)
			{
				fprintf(stderr, "! Planner Stats:\n");
				ShowUsage();
			}
			plan_list = lappend(plan_list, plan);
#ifdef INDEXSCAN_PATCH
			/* ----------------
			 *	Print plan if debugging.
			 *	This has been moved here to get debugging output
			 *	also for queries in functions.	DZ - 27-8-1996
			 * ----------------
			 */
			if (DebugPrintPlan == true)
			{
				printf("\n---- \tplan is :\n");
				nodeDisplay(plan);
				printf("\n");
			}
#endif
		}
#ifdef FUNC_UTIL_PATCH

		/*
		 * If the command is an utility append a null plan. This is needed
		 * to keep the plan_list aligned with the querytree_list or the
		 * function executor will crash.  DZ - 30-8-1996
		 */
		else
		{
			plan_list = lappend(plan_list, NULL);
		}
#endif
	}

	if (queryListP)
		*queryListP = querytree_list;

	return (plan_list);
}

/* ----------------------------------------------------------------
 *		pg_eval()
 *
 *		Takes a querystring, runs the parser/utilities or
 *		parser/planner/executor over it as necessary
 *		Begin Transaction Should have been called before this
 *		and CommitTransaction After this is called
 *		This is strictly because we do not allow for nested xactions.
 *
 *		NON-OBVIOUS-RESTRICTIONS
 *		this function _MUST_ allocate a new "parsetree" each time,
 *		since it may be stored in a named portal and should not
 *		change its value.
 *
 * ----------------------------------------------------------------
 */

void
pg_eval(char *query_string, char **argv, Oid *typev, int nargs)
{
	pg_eval_dest(query_string, argv, typev, nargs, whereToSendOutput);
}

void
pg_eval_dest(char *query_string,/* string to execute */
			 char **argv,		/* arguments */
			 Oid *typev,		/* argument types */
			 int nargs,			/* number of arguments */
			 CommandDest dest)	/* where results should go */
{
	List	   *plan_list;
	Plan	   *plan;
	Query	   *querytree;
	int			i,
				j;
	QueryTreeList *querytree_list;

	/* plan the queries */
	plan_list = pg_plan(query_string, typev, nargs, &querytree_list, dest);

	/* pg_plan could have failed */
	if (querytree_list == NULL)
		return;

	for (i = 0; i < querytree_list->len; i++)
	{
		querytree = querytree_list->qtrees[i];

#ifdef FUNC_UTIL_PATCH

		/*
		 * Advance on the plan_list in every case.	Now the plan_list has
		 * the same length of the querytree_list.  DZ - 30-8-1996
		 */
		plan = (Plan *) lfirst(plan_list);
		plan_list = lnext(plan_list);
#endif
		if (querytree->commandType == CMD_UTILITY)
		{
			/* ----------------
			 *	 process utility functions (create, destroy, etc..)
			 *
			 *	 Note: we do not check for the transaction aborted state
			 *	 because that is done in ProcessUtility.
			 * ----------------
			 */
			if (!Quiet)
			{
				time(&tim);
				printf("\tProcessUtility() at %s\n", ctime(&tim));
			}

			ProcessUtility(querytree->utilityStmt, dest);

		}
		else
		{
#ifndef FUNC_UTIL_PATCH

			/*
			 * Moved before the if.  DZ - 30-8-1996
			 */
			plan = (Plan *) lfirst(plan_list);
			plan_list = lnext(plan_list);
#endif

#ifdef INDEXSCAN_PATCH

			/*
			 * Print moved in pg_plan.	DZ - 27-8-1996
			 */
#else
			/* ----------------
			 *	print plan if debugging
			 * ----------------
			 */
			if (DebugPrintPlan == true)
			{
				printf("\n---- plan is :\n");
				nodeDisplay(plan);
				printf("\n");
			}
#endif

			/* ----------------
			 *	 execute the plan
			 *
			 */
			if (ShowExecutorStats)
				ResetUsage();

			for (j = 0; j < _exec_repeat_; j++)
			{
				if (!Quiet)
				{
					time(&tim);
					printf("\tProcessQuery() at %s\n", ctime(&tim));
				}
				ProcessQuery(querytree, plan, argv, typev, nargs, dest);
			}

			if (ShowExecutorStats)
			{
				fprintf(stderr, "! Executor Stats:\n");
				ShowUsage();
			}
		}

		/*
		 * In a query block, we want to increment the command counter
		 * between queries so that the effects of early queries are
		 * visible to subsequent ones.
		 */

		if (querytree_list)
			CommandCounterIncrement();
	}

	free(querytree_list->qtrees);
	free(querytree_list);
}

/* --------------------------------
 *		signal handler routines used in PostgresMain()
 *
 *		handle_warn() is used to catch kill(getpid(),1) which
 *		occurs when elog(WARN) is called.
 *
 *		quickdie() occurs when signalled by the postmaster, some backend
 *		has bought the farm we need to stop what we're doing and exit.
 *
 *		die() preforms an orderly cleanup via ExitPostgres()
 * --------------------------------
 */

void
handle_warn(SIGNAL_ARGS)
{
	siglongjmp(Warn_restart, 1);
}

static void
quickdie(SIGNAL_ARGS)
{
	elog(NOTICE, "I have been signalled by the postmaster.");
	elog(NOTICE, "Some backend process has died unexpectedly and possibly");
	elog(NOTICE, "corrupted shared memory.  The current transaction was");
	elog(NOTICE, "aborted, and I am going to exit.  Please resend the");
	elog(NOTICE, "last query. -- The postgres backend");

	/*
	 * DO NOT ExitPostgres(0) -- we're here because shared memory may be
	 * corrupted, so we don't want to flush any shared state to stable
	 * storage.  Just nail the windows shut and get out of town.
	 */

	exit(0);
}

void
die(SIGNAL_ARGS)
{
	ExitPostgres(0);
}

/* signal handler for floating point exception */
static void
FloatExceptionHandler(SIGNAL_ARGS)
{
	elog(WARN, "floating point exception! the last floating point operation eit\
her exceeded legal ranges or was a divide by zero");
}


static void
usage(char *progname)
{
	fprintf(stderr,
			"Usage: %s [-B nbufs] [-d lvl] ] [-f plantype] \t[-m portno] [\t -o filename]\n",
			progname);
	fprintf(stderr, "\t[-P portno] [-t tracetype] [-x opttype] [-bCEiLFNopQSs] [dbname]\n");
	fprintf(stderr, "    b: consider bushy plan trees during optimization\n");
	fprintf(stderr, "    B: set number of buffers in buffer pool\n");
	fprintf(stderr, "    C: supress version info\n");
	fprintf(stderr, "    d: set debug level\n");
	fprintf(stderr, "    E: echo query before execution\n");
	fprintf(stderr, "    e  turn on European date format\n");
	fprintf(stderr, "    F: turn off fsync\n");
	fprintf(stderr, "    f: forbid plantype generation\n");
	fprintf(stderr, "    i: don't execute the query, just show the plan tree\n");
#ifdef LOCK_MGR_DEBUG
	fprintf(stderr, "    K: set locking debug level [0|1|2]\n");
#endif
	fprintf(stderr, "    L: turn off locking\n");
	fprintf(stderr, "    m: set up a listening backend at portno to support multiple front-ends\n");
	fprintf(stderr, "    M: start as postmaster\n");
	fprintf(stderr, "    N: don't use newline as query delimiter\n");
	fprintf(stderr, "    o: send stdout and stderr to given filename \n");
	fprintf(stderr, "    p: backend started by postmaster\n");
	fprintf(stderr, "    P: set port file descriptor\n");
	fprintf(stderr, "    Q: suppress informational messages\n");
	fprintf(stderr, "    S: set amount of sort memory available\n");
	fprintf(stderr, "    s: show stats after each query\n");
	fprintf(stderr, "    t: trace component execution times\n");
	fprintf(stderr, "    T: execute all possible plans for each query\n");
	fprintf(stderr, "    x: control expensive function optimization\n");
}

/* ----------------------------------------------------------------
 *		PostgresMain
 *		  postgres main loop
 *		all backends, interactive or otherwise start here
 * ----------------------------------------------------------------
 */
int
PostgresMain(int argc, char *argv[])
{
	int			flagC;
	int			flagQ;
	int			flagE;
	int			flagEu;
	int			flag;

	char	   *DBName = NULL;
	int			errs = 0;

	char		firstchar;
	char		parser_input[MAX_PARSE_BUFFER];
	char	   *userName;

	bool		multiplexedBackend;
	char	   *hostName;		/* the host name of the backend server */
	char		hostbuf[MAXHOSTNAMELEN];
	int			serverSock;
	int			serverPortnum = 0;
	int			nSelected;		/* number of descriptors ready from
								 * select(); */
	int			maxFd = 0;		/* max file descriptor + 1 */
	fd_set		rmask,
				basemask;
	FrontEnd   *newFE,
			   *currentFE = NULL;
	int			numFE = 0;		/* keep track of number of active
								 * frontends */
	Port	   *newPort;
	int			newFd;
	Dlelem	   *curr;
	int			status;

	extern int	optind;
	extern char *optarg;
	extern short DebugLvl;

	/* ----------------
	 *	register signal handlers.
	 * ----------------
	 */
	pqsignal(SIGINT, die);

	pqsignal(SIGHUP, die);
	pqsignal(SIGTERM, die);
	pqsignal(SIGPIPE, die);
	pqsignal(SIGUSR1, quickdie);
	pqsignal(SIGUSR2, Async_NotifyHandler);
	pqsignal(SIGFPE, FloatExceptionHandler);

	/* --------------------
	 *	initialize globals
	 * -------------------
	 */

	MasterPid = getpid();

	/* ----------------
	 *	parse command line arguments
	 * ----------------
	 */
	flagC = flagQ = flagE = flagEu = ShowStats = 0;
	ShowParserStats = ShowPlannerStats = ShowExecutorStats = 0;
#ifdef LOCK_MGR_DEBUG
	lockDebug = 0;
#endif

	/*
	 * get hostname is either the environment variable PGHOST or
	 * 'localhost'
	 */
	if (!(hostName = getenv("PGHOST")))
	{
		if (gethostname(hostbuf, MAXHOSTNAMELEN) < 0)
			strcpy(hostbuf, "localhost");
		hostName = hostbuf;
	}

	DataDir = getenv("PGDATA"); /* default */
	multiplexedBackend = false; /* default */

	while ((flag = getopt(argc, argv, "B:bCD:d:Eef:iK:Lm:MNo:P:pQS:st:x:F"))
		   != EOF)
		switch (flag)
		{

			case 'b':
				/* ----------------
				 *	set BushyPlanFlag to true.
				 * ----------------
				 */
				BushyPlanFlag = 1;
				break;
			case 'B':
				/* ----------------
				 *	specify the size of buffer pool
				 * ----------------
				 */
				NBuffers = atoi(optarg);
				break;

			case 'C':
				/* ----------------
				 *	don't print version string (don't know why this is 'C' --mao)
				 * ----------------
				 */
				flagC = 1;
				break;

			case 'D':			/* PGDATA directory */
				DataDir = optarg;

			case 'd':			/* debug level */
				flagQ = 0;
				DebugLvl = (short) atoi(optarg);
				if (DebugLvl > 1)
					DebugPrintQuery = true;
				if (DebugLvl > 2)
				{
					DebugPrintParse = true;
					DebugPrintPlan = true;
					DebugPrintRewrittenParsetree = true;
				}
				break;

			case 'E':
				/* ----------------
				 *	E - echo the query the user entered
				 * ----------------
				 */
				flagE = 1;
				break;

			case 'e':
				/* --------------------------
				 * Use european date formats.
				 * --------------------------
				 */
				flagEu = 1;
				break;

			case 'F':
				/* --------------------
				 *	turn off fsync
				 * --------------------
				 */
				fsyncOff = 1;
				break;

			case 'f':
				/* -----------------
				 *	  f - forbid generation of certain plans
				 * -----------------
				 */
				switch (optarg[0])
				{
					case 's':	/* seqscan */
						_enable_seqscan_ = false;
						break;
					case 'i':	/* indexscan */
						_enable_indexscan_ = false;
						break;
					case 'n':	/* nestloop */
						_enable_nestloop_ = false;
						break;
					case 'm':	/* mergejoin */
						_enable_mergesort_ = false;
						break;
					case 'h':	/* hashjoin */
						_enable_hashjoin_ = false;
						break;
					default:
						errs++;
				}
				break;

			case 'i':
				dontExecute = 1;
				break;

			case 'K':
#ifdef LOCK_MGR_DEBUG
				lockDebug = atoi(optarg);
#else
				fprintf(stderr, "Lock debug not compiled in\n");
#endif
				break;

			case 'L':
				/* --------------------
				 *	turn off locking
				 * --------------------
				 */
				lockingOff = 1;
				break;

			case 'm':

				/*
				 * start up a listening backend that can respond to
				 * multiple front-ends.  (Note:  all the front-end
				 * connections are still connected to a single-threaded
				 * backend.  Requests are FCFS.  Everything is in one
				 * transaction
				 */
				multiplexedBackend = true;
				serverPortnum = atoi(optarg);
				break;
			case 'M':
				exit(PostmasterMain(argc, argv));
				break;
			case 'N':
				/* ----------------
				 *	N - Don't use newline as a query delimiter
				 * ----------------
				 */
				UseNewLine = 0;
				break;

			case 'o':
				/* ----------------
				 *	o - send output (stdout and stderr) to the given file
				 * ----------------
				 */
				strNcpy(OutputFileName, optarg, MAXPGPATH);
				break;

			case 'p':			/* started by postmaster */
				/* ----------------
				 *	p - special flag passed if backend was forked
				 *		by a postmaster.
				 * ----------------
				 */
				IsUnderPostmaster = true;
				break;

			case 'P':
				/* ----------------
				 *	P - Use the passed file descriptor number as the port
				 *	  on which to communicate with the user.  This is ONLY
				 *	  useful for debugging when fired up by the postmaster.
				 * ----------------
				 */
				Portfd = atoi(optarg);
				break;

			case 'Q':
				/* ----------------
				 *	Q - set Quiet mode (reduce debugging output)
				 * ----------------
				 */
				flagQ = 1;
				break;

			case 'S':
				/* ----------------
				 *	S - amount of sort memory to use in 1k bytes and
				 *		(optional) max number of tuples in leftist tree
				 * ----------------
				 */
				{
					int S;
					char *p = strchr (optarg, ',');
					
					if ( p != NULL )
					{
						*p = 0;
						S = atoi(optarg);
						if ( S >= 4*MAXBLCKSZ/1024 )
							SortMem = S;
						S = atoi (p + 1);
						if ( S >= 32 )
							SortTuplesInTree = S;
						*p = ',';
					}
					else
					{
						S = atoi(optarg);
						if ( S >= 4*MAXBLCKSZ/1024 )
							SortMem = S;
					}
				}
				break;

			case 's':
				/* ----------------
				 *	  s - report usage statistics (timings) after each query
				 * ----------------
				 */
				ShowStats = 1;
				StatFp = stderr;
				break;

			case 't':
				/* ----------------
				 *	tell postgres to report usage statistics (timings) for
				 *	each query
				 *
				 *	-tpa[rser] = print stats for parser time of each query
				 *	-tpl[anner] = print stats for planner time of each query
				 *	-te[xecutor] = print stats for executor time of each query
				 *	caution: -s can not be used together with -t.
				 * ----------------
				 */
				StatFp = stderr;
				switch (optarg[0])
				{
					case 'p':
						if (optarg[1] == 'a')
							ShowParserStats = 1;
						else if (optarg[1] == 'l')
							ShowPlannerStats = 1;
						else
							errs++;
						break;
					case 'e':
						ShowExecutorStats = 1;
						break;
					default:
						errs++;
						break;
				}
				break;

			case 'x':
#if 0							/* planner/xfunc.h */

				/*
				 * control joey hellerstein's expensive function
				 * optimization
				 */
				if (XfuncMode != 0)
				{
					fprintf(stderr, "only one -x flag is allowed\n");
					errs++;
					break;
				}
				if (strcmp(optarg, "off") == 0)
					XfuncMode = XFUNC_OFF;
				else if (strcmp(optarg, "nor") == 0)
					XfuncMode = XFUNC_NOR;
				else if (strcmp(optarg, "nopull") == 0)
					XfuncMode = XFUNC_NOPULL;
				else if (strcmp(optarg, "nopm") == 0)
					XfuncMode = XFUNC_NOPM;
				else if (strcmp(optarg, "pullall") == 0)
					XfuncMode = XFUNC_PULLALL;
				else if (strcmp(optarg, "wait") == 0)
					XfuncMode = XFUNC_WAIT;
				else
				{
					fprintf(stderr, "use -x {off,nor,nopull,nopm,pullall,wait}\n");
					errs++;
				}
#endif
				break;

			default:
				/* ----------------
				 *	default: bad command line option
				 * ----------------
				 */
				errs++;
		}

	/* ----------------
	 *	get user name and pathname and check command line validity
	 * ----------------
	 */
	SetPgUserName();
	userName = GetPgUserName();

	if (FindBackend(pg_pathname, argv[0]) < 0)
		elog(FATAL, "%s: could not locate executable, bailing out...",
			 argv[0]);

	if (errs || argc - optind > 1)
	{
		usage(argv[0]);
		exitpg(1);
	}
	else if (argc - optind == 1)
	{
		DBName = argv[optind];
	}
	else if ((DBName = userName) == NULL)
	{
		fprintf(stderr, "%s: USER undefined and no database specified\n",
				argv[0]);
		exitpg(1);
	}

	if (ShowStats &&
		(ShowParserStats || ShowPlannerStats || ShowExecutorStats))
	{
		fprintf(stderr, "-s can not be used together with -t.\n");
		exitpg(1);
	}

	if (!DataDir)
	{
		fprintf(stderr, "%s does not know where to find the database system "
				"data.  You must specify the directory that contains the "
				"database system either by specifying the -D invocation "
			 "option or by setting the PGDATA environment variable.\n\n",
				argv[0]);
		exitpg(1);
	}

	Noversion = flagC;
	Quiet = flagQ;
	EchoQuery = flagE;
	EuroDates = flagEu;

	/* ----------------
	 *	print flags
	 * ----------------
	 */
	if (!Quiet)
	{
		puts("\t---debug info---");
		printf("\tQuiet =        %c\n", Quiet ? 't' : 'f');
		printf("\tNoversion =    %c\n", Noversion ? 't' : 'f');
		printf("\ttimings   =    %c\n", ShowStats ? 't' : 'f');
		printf("\tdates     =    %s\n", EuroDates ? "European" : "Normal");
		printf("\tbufsize   =    %d\n", NBuffers);
		printf("\tsortmem   =    %d\n", SortMem);

		printf("\tquery echo =   %c\n", EchoQuery ? 't' : 'f');
		printf("\tmultiplexed backend? =  %c\n", multiplexedBackend ? 't' : 'f');
		printf("\tDatabaseName = [%s]\n", DBName);
		puts("\t----------------\n");
	}

	/* ----------------
	 *	initialize portal file descriptors
	 * ----------------
	 */
	if (IsUnderPostmaster == true)
	{
		if (Portfd < 0)
		{
			fprintf(stderr,
					"Postmaster flag set: no port number specified, use /dev/null\n");
			Portfd = open(NULL_DEV, O_RDWR, 0666);
		}
		pq_init(Portfd);
	}

	if (multiplexedBackend)
	{
		if (serverPortnum == 0 ||
			StreamServerPort(hostName, serverPortnum, &serverSock) != STATUS_OK)
		{
			fprintf(stderr, "Postgres: cannot create stream port %d\n", serverPortnum);
			exit(1);
		}
/*
{
	char buf[100];
	sprintf(buf, "stream port %d created, socket = %d\n", serverPortnum, serverSock);
	puts(buf);
}
*/
		FD_ZERO(&rmask);
		FD_ZERO(&basemask);
		FD_SET(serverSock, &basemask);

		frontendList = DLNewList();
		/* add the original FrontEnd to the list */
		if (IsUnderPostmaster == true)
		{
			FrontEnd   *fe = malloc(sizeof(FrontEnd));

			FD_SET(Portfd, &basemask);
			maxFd = Max(serverSock, Portfd) + 1;

			fe->fn_connected = true;
			fe->fn_Pfin = Pfin;
			fe->fn_Pfout = Pfout;
			fe->fn_done = false;
			(fe->fn_port).sock = Portfd;
			DLAddHead(frontendList, DLNewElem(fe));
			numFE++;
		}
		else
		{
			numFE = 1;
			maxFd = serverSock + 1;
		}
	}

	if (IsUnderPostmaster || multiplexedBackend)
		whereToSendOutput = Remote;
	else
		whereToSendOutput = Debug;

	SetProcessingMode(InitProcessing);

	/* initialize */
	if (!Quiet)
	{
		puts("\tInitPostgres()..");
	}

	InitPostgres(DBName);

	/* ----------------
	 *	if an exception is encountered, processing resumes here
	 *	so we abort the current transaction and start a new one.
	 *	This must be done after we initialize the slave backends
	 *	so that the slaves signal the master to abort the transaction
	 *	rather than calling AbortCurrentTransaction() themselves.
	 *
	 *	Note:  elog(WARN) causes a kill(getpid(),1) to occur sending
	 *		   us back here.
	 * ----------------
	 */

	pqsignal(SIGHUP, handle_warn);

	if (sigsetjmp(Warn_restart, 1) != 0)
	{
		InWarn = 1;

		time(&tim);

		if (!Quiet)
			printf("\tAbortCurrentTransaction() at %s\n", ctime(&tim));

		memset(parser_input, 0, MAX_PARSE_BUFFER);

		AbortCurrentTransaction();
	}
	InWarn = 0;

	/* ----------------
	 *	POSTGRES main processing loop begins here
	 * ----------------
	 */
	if (IsUnderPostmaster == false)
	{
		puts("\nPOSTGRES backend interactive interface");
		puts("$Revision: 1.47 $ $Date: 1997-09-18 05:19:17 $");
	}

	/* ----------------
	 * if stable main memory is assumed (-S(old) flag is set), it is necessary
	 * to flush all dirty shared buffers before exit
	 * plai 8/7/90
	 * ----------------
	 */
	if (!TransactionFlushEnabled())
		on_exitpg(FlushBufferPool, (caddr_t) 0);

	for (;;)
	{

		if (multiplexedBackend)
		{
			if (numFE == 0)
				break;

			memmove((char *) &rmask, (char *) &basemask, sizeof(fd_set));
			nSelected = select(maxFd, &rmask, 0, 0, 0);

			if (nSelected < 0)
			{

				if (errno == EINTR)
					continue;
				fprintf(stderr, "postgres: multiplexed backend select failed\n");
				exitpg(1);
			}
			if (FD_ISSET(serverSock, &rmask))
			{
				/* new connection pending on our well-known port's socket */
				newFE = (FrontEnd *) malloc(sizeof(FrontEnd));
				memset(newFE, 0, sizeof(FrontEnd));
				newFE->fn_connected = false;
				newFE->fn_done = false;
				newPort = &(newFE->fn_port);
				if (StreamConnection(serverSock, newPort) != STATUS_OK)
				{
					StreamClose(newPort->sock);
					newFd = -1;
				}
				else
				{
					DLAddHead(frontendList, DLNewElem(newFE));
					numFE++;
					newFd = newPort->sock;
					if (newFd >= maxFd)
						maxFd = newFd + 1;
					FD_SET(newFd, &rmask);
					FD_SET(newFd, &basemask);
					--nSelected;
					FD_CLR(serverSock, &rmask);
				}
				continue;
			}					/* if FD_ISSET(serverSock) */

			/*
			 * if we get here, it means that the serverSocket was not the
			 * one selected.  Instead, one of the front ends was selected.
			 * find which one
			 */
			curr = DLGetHead(frontendList);
			while (curr)
			{
				FrontEnd   *fe = (FrontEnd *) DLE_VAL(curr);
				Port	   *port = &(fe->fn_port);

				/* this is lifted from postmaster.c */
				if (FD_ISSET(port->sock, &rmask))
				{
					if (fe->fn_connected == false)
					{
						/* we have a message from a new frontEnd */
						status = PacketReceive(port, &port->buf, NON_BLOCKING);
						if (status == STATUS_OK)
						{
							fe->fn_connected = true;
							pq_init(port->sock);
							fe->fn_Pfin = Pfin;
							fe->fn_Pfout = Pfout;
						}
						else
							fprintf(stderr, "Multiplexed backend: error in reading packets from %d\n", port->sock);
					}
					else
/* we have a query from an existing,  active FrontEnd */
					{
						Pfin = fe->fn_Pfin;
						Pfout = fe->fn_Pfout;
						currentFE = fe;
					}
					if (fe->fn_done)
					{
						Dlelem	   *c = curr;

						curr = DLGetSucc(curr);
						DLRemove(c);
					}
					break;
				}
				else
					curr = DLGetSucc(curr);
			}
		}
		/* ----------------
		 *	 (1) read a command.
		 * ----------------
		 */
		memset(parser_input, 0, MAX_PARSE_BUFFER);

		firstchar = ReadCommand(parser_input, multiplexedBackend);
		/* process the command */
		switch (firstchar)
		{
				/* ----------------
				 *	'F' indicates a fastpath call.
				 *		XXX HandleFunctionRequest
				 * ----------------
				 */
			case 'F':
				IsEmptyQuery = false;

				/* start an xact for this function invocation */
				if (!Quiet)
				{
					time(&tim);
					printf("\tStartTransactionCommand() at %s\n", ctime(&tim));
				}

				StartTransactionCommand();
				HandleFunctionRequest();
				break;

				/* ----------------
				 *	'Q' indicates a user query
				 * ----------------
				 */
			case 'Q':
				fflush(stdout);

				if (strspn(parser_input, " \t\n") == strlen(parser_input))
				{
					/* ----------------
					 *	if there is nothing in the input buffer, don't bother
					 *	trying to parse and execute anything..
					 * ----------------
					 */
					IsEmptyQuery = true;
				}
				else
				{
					/* ----------------
					 *	otherwise, process the input string.
					 * ----------------
					 */
					IsEmptyQuery = false;
					if (ShowStats)
						ResetUsage();

					/* start an xact for this query */
					if (!Quiet)
					{
						time(&tim);
						printf("\tStartTransactionCommand() at %s\n", ctime(&tim));
					}
					StartTransactionCommand();

					pg_eval(parser_input, (char **) NULL, (Oid *) NULL, 0);

					if (ShowStats)
						ShowUsage();
				}
				break;

				/* ----------------
				 *	'X' means that the frontend is closing down the socket
				 * ----------------
				 */
			case 'X':
				IsEmptyQuery = true;
				if (multiplexedBackend)
				{
					FD_CLR(currentFE->fn_port.sock, &basemask);
					currentFE->fn_done = true;
					numFE--;
				}
				pq_close();
				break;

			default:
				elog(WARN, "unknown frontend message was recieved");
		}

		/* ----------------
		 *	 (3) commit the current transaction
		 *
		 *	 Note: if we had an empty input buffer, then we didn't
		 *	 call pg_eval, so we don't bother to commit this transaction.
		 * ----------------
		 */
		if (!IsEmptyQuery)
		{
			if (!Quiet)
			{
				time(&tim);
				printf("\tCommitTransactionCommand() at %s\n", ctime(&tim));
			}
			CommitTransactionCommand();

		}
		else
		{
			if (IsUnderPostmaster || multiplexedBackend)
				NullCommand(Remote);
		}

	}							/* infinite for-loop */
	exitpg(0);
	return 1;
}

#ifndef HAVE_GETRUSAGE
#include "rusagestub.h"
#else							/* HAVE_GETRUSAGE */
#include <sys/resource.h>
#endif							/* HAVE_GETRUSAGE */

struct rusage Save_r;
struct timeval Save_t;

void
ResetUsage(void)
{
	struct timezone tz;

	getrusage(RUSAGE_SELF, &Save_r);
	gettimeofday(&Save_t, &tz);
	ResetBufferUsage();
/*	  ResetTupleCount(); */
}

void
ShowUsage(void)
{
	struct timeval user,
				sys;
	struct timeval elapse_t;
	struct timezone tz;
	struct rusage r;

	getrusage(RUSAGE_SELF, &r);
	gettimeofday(&elapse_t, &tz);
	memmove((char *) &user, (char *) &r.ru_utime, sizeof(user));
	memmove((char *) &sys, (char *) &r.ru_stime, sizeof(sys));
	if (elapse_t.tv_usec < Save_t.tv_usec)
	{
		elapse_t.tv_sec--;
		elapse_t.tv_usec += 1000000;
	}
	if (r.ru_utime.tv_usec < Save_r.ru_utime.tv_usec)
	{
		r.ru_utime.tv_sec--;
		r.ru_utime.tv_usec += 1000000;
	}
	if (r.ru_stime.tv_usec < Save_r.ru_stime.tv_usec)
	{
		r.ru_stime.tv_sec--;
		r.ru_stime.tv_usec += 1000000;
	}

	/*
	 * the only stats we don't show here are for memory usage -- i can't
	 * figure out how to interpret the relevant fields in the rusage
	 * struct, and they change names across o/s platforms, anyway. if you
	 * can figure out what the entries mean, you can somehow extract
	 * resident set size, shared text size, and unshared data and stack
	 * sizes.
	 */

	fprintf(StatFp, "! system usage stats:\n");
	fprintf(StatFp,
			"!\t%ld.%06ld elapsed %ld.%06ld user %ld.%06ld system sec\n",
			(long int) elapse_t.tv_sec - Save_t.tv_sec,
			(long int) elapse_t.tv_usec - Save_t.tv_usec,
			(long int) r.ru_utime.tv_sec - Save_r.ru_utime.tv_sec,
			(long int) r.ru_utime.tv_usec - Save_r.ru_utime.tv_usec,
			(long int) r.ru_stime.tv_sec - Save_r.ru_stime.tv_sec,
			(long int) r.ru_stime.tv_usec - Save_r.ru_stime.tv_usec);
	fprintf(StatFp,
			"!\t[%ld.%06ld user %ld.%06ld sys total]\n",
			(long int) user.tv_sec,
			(long int) user.tv_usec,
			(long int) sys.tv_sec,
			(long int) sys.tv_usec);
#ifdef HAVE_GETRUSAGE
	fprintf(StatFp,
			"!\t%ld/%ld [%ld/%ld] filesystem blocks in/out\n",
			r.ru_inblock - Save_r.ru_inblock,
	/* they only drink coffee at dec */
			r.ru_oublock - Save_r.ru_oublock,
			r.ru_inblock, r.ru_oublock);
	fprintf(StatFp,
		  "!\t%ld/%ld [%ld/%ld] page faults/reclaims, %ld [%ld] swaps\n",
			r.ru_majflt - Save_r.ru_majflt,
			r.ru_minflt - Save_r.ru_minflt,
			r.ru_majflt, r.ru_minflt,
			r.ru_nswap - Save_r.ru_nswap,
			r.ru_nswap);
	fprintf(StatFp,
	 "!\t%ld [%ld] signals rcvd, %ld/%ld [%ld/%ld] messages rcvd/sent\n",
			r.ru_nsignals - Save_r.ru_nsignals,
			r.ru_nsignals,
			r.ru_msgrcv - Save_r.ru_msgrcv,
			r.ru_msgsnd - Save_r.ru_msgsnd,
			r.ru_msgrcv, r.ru_msgsnd);
	fprintf(StatFp,
		 "!\t%ld/%ld [%ld/%ld] voluntary/involuntary context switches\n",
			r.ru_nvcsw - Save_r.ru_nvcsw,
			r.ru_nivcsw - Save_r.ru_nivcsw,
			r.ru_nvcsw, r.ru_nivcsw);
#endif							/* HAVE_GETRUSAGE */
	fprintf(StatFp, "! postgres usage stats:\n");
	PrintBufferUsage(StatFp);
/*	   DisplayTupleCount(StatFp); */
}
