/*-------------------------------------------------------------------------
 *
 * postmaster.c
 *	  This program acts as a clearing house for requests to the
 *	  POSTGRES system.	Frontend programs send a startup message
 *	  to the Postmaster and the postmaster uses the info in the
 *	  message to setup a backend process.
 *
 *	  The postmaster also manages system-wide operations such as
 *	  startup, shutdown, and periodic checkpoints.	The postmaster
 *	  itself doesn't do those operations, mind you --- it just forks
 *	  off a subprocess to do them at the right times.  It also takes
 *	  care of resetting the system if a backend crashes.
 *
 *	  The postmaster process creates the shared memory and semaphore
 *	  pools during startup, but as a rule does not touch them itself.
 *	  In particular, it is not a member of the PROC array of backends
 *	  and so it cannot participate in lock-manager operations.	Keeping
 *	  the postmaster away from shared memory operations makes it simpler
 *	  and more reliable.  The postmaster is almost always able to recover
 *	  from crashes of individual backends by resetting shared memory;
 *	  if it did much with shared memory then it would be prone to crashing
 *	  along with the backends.
 *
 *	  When a request message is received, we now fork() immediately.
 *	  The child process performs authentication of the request, and
 *	  then becomes a backend if successful.  This allows the auth code
 *	  to be written in a simple single-threaded style (as opposed to the
 *	  crufty "poor man's multitasking" code that used to be needed).
 *	  More importantly, it ensures that blockages in non-multithreaded
 *	  libraries like SSL or PAM cannot cause denial of service to other
 *	  clients.
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/postmaster/postmaster.c,v 1.229 2001-06-29 16:05:57 tgl Exp $
 *
 * NOTES
 *
 * Initialization:
 *		The Postmaster sets up a few shared memory data structures
 *		for the backends.  It should at the very least initialize the
 *		lock manager.
 *
 * Synchronization:
 *		The Postmaster shares memory with the backends but should avoid
 *		touching shared memory, so as not to become stuck if a crashing
 *		backend screws up locks or shared memory.  Likewise, the Postmaster
 *		should never block on messages from frontend clients.
 *
 * Garbage Collection:
 *		The Postmaster cleans up after backends if they have an emergency
 *		exit and/or core dump.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/param.h>
/* moved here to prevent double define */
#include <netdb.h>
#include <limits.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "catalog/pg_database.h"
#include "commands/async.h"
#include "lib/dllist.h"
#include "libpq/auth.h"
#include "libpq/crypt.h"
#include "libpq/libpq.h"
#include "libpq/pqcomm.h"
#include "libpq/pqsignal.h"
#include "miscadmin.h"
#include "nodes/nodes.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/proc.h"
#include "access/xlog.h"
#include "tcop/tcopprot.h"
#include "utils/exc.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "bootstrap/bootstrap.h"

#include "pgstat.h"

#define INVALID_SOCK	(-1)
#define ARGV_SIZE	64

#ifdef HAVE_SIGPROCMASK
sigset_t	UnBlockSig,
			BlockSig;
#else
int			UnBlockSig,
			BlockSig;
#endif

/*
 * List of active backends (or child processes anyway; we don't actually
 * know whether a given child has become a backend or is still in the
 * authorization phase).  This is used mainly to keep track of how many
 * children we have and send them appropriate signals when necessary.
 */
typedef struct bkend
{
	pid_t		pid;			/* process id of backend */
	long		cancel_key;		/* cancel key for cancels for this backend */
} Backend;

static Dllist *BackendList;

/* The socket number we are listening for connections on */
int			PostPortNumber;
char	   *UnixSocketDir;
char	   *VirtualHost;

/*
 * MaxBackends is the actual limit on the number of backends we will
 * start. The default is established by configure, but it can be
 * readjusted from 1..MAXBACKENDS with the postmaster -N switch. Note
 * that a larger MaxBackends value will increase the size of the shared
 * memory area as well as cause the postmaster to grab more kernel
 * semaphores, even if you never actually use that many backends.
 */
int			MaxBackends = DEF_MAXBACKENDS;


static char *progname = (char *) NULL;
static char **real_argv;
static int	real_argc;

static time_t tnow;

/* flag to indicate that SIGHUP arrived during server loop */
static volatile bool got_SIGHUP = false;

/*
 * Default Values
 */
static int	ServerSock_INET = INVALID_SOCK;		/* stream socket server */

#ifdef HAVE_UNIX_SOCKETS
static int	ServerSock_UNIX = INVALID_SOCK;		/* stream socket server */
#endif

#ifdef USE_SSL
static SSL_CTX *SSL_context = NULL;		/* Global SSL context */
#endif

/*
 * Set by the -o option
 */
static char ExtraOptions[MAXPGPATH];

/*
 * These globals control the behavior of the postmaster in case some
 * backend dumps core.	Normally, it kills all peers of the dead backend
 * and reinitializes shared memory.  By specifying -s or -n, we can have
 * the postmaster stop (rather than kill) peers and not reinitialize
 * shared data structures.
 */
static bool Reinit = true;
static int	SendStop = false;

/* still more option variables */
bool		NetServer = false;	/* listen on TCP/IP */
bool		EnableSSL = false;
bool		SilentMode = false; /* silent mode (-S) */

int			CheckPointTimeout = 300;

/* Startup/shutdown state */
static pid_t StartupPID = 0,
			ShutdownPID = 0,
			CheckPointPID = 0;
static time_t checkpointed = 0;

#define			NoShutdown		0
#define			SmartShutdown	1
#define			FastShutdown	2

static int	Shutdown = NoShutdown;

static bool FatalError = false; /* T if recovering from backend crash */

/*
 * State for assigning random salts and cancel keys.
 * Also, the global MyCancelKey passes the cancel key assigned to a given
 * backend from the postmaster to that backend (via fork).
 */

static unsigned int random_seed = 0;

extern char *optarg;
extern int	optind,
			opterr;
#ifdef HAVE_INT_OPTRESET
extern int	optreset;
#endif

/*
 * postmaster.c - function prototypes
 */
static void pmdaemonize(int argc, char *argv[]);
static Port *ConnCreate(int serverFd);
static void ConnFree(Port *port);
static void ClosePostmasterPorts(void);
static void reset_shared(unsigned short port);
static void SIGHUP_handler(SIGNAL_ARGS);
static void pmdie(SIGNAL_ARGS);
static void reaper(SIGNAL_ARGS);
static void schedule_checkpoint(SIGNAL_ARGS);
static void CleanupProc(int pid, int exitstatus);
static int	DoBackend(Port *port);
static void ExitPostmaster(int status);
static void usage(const char *);
static int	ServerLoop(void);
static int	BackendStartup(Port *port);
static int	ProcessStartupPacket(Port *port, bool SSLdone);
static void	processCancelRequest(Port *port, void *pkt);
static int	initMasks(fd_set *rmask, fd_set *wmask);
static char *canAcceptConnections(void);
static long PostmasterRandom(void);
static void RandomSalt(char *salt);
static void SignalChildren(int signal);
static int	CountChildren(void);
static bool CreateOptsFile(int argc, char *argv[]);
static pid_t SSDataBase(int xlop);
#ifdef __GNUC__
/* This checks the format string for consistency. */
static void postmaster_error(const char *fmt, ...)
	__attribute__((format(printf, 1, 2)));
#else
static void postmaster_error(const char *fmt, ...);
#endif

#define StartupDataBase()		SSDataBase(BS_XLOG_STARTUP)
#define CheckPointDataBase()	SSDataBase(BS_XLOG_CHECKPOINT)
#define ShutdownDataBase()		SSDataBase(BS_XLOG_SHUTDOWN)

#ifdef USE_SSL
static void InitSSL(void);
#endif


static void
checkDataDir(const char *checkdir)
{
	char		path[MAXPGPATH];
	FILE	   *fp;

	if (checkdir == NULL)
	{
		fprintf(stderr, gettext(
			"%s does not know where to find the database system data.\n"
			"You must specify the directory that contains the database system\n"
			"either by specifying the -D invocation option or by setting the\n"
			"PGDATA environment variable.\n\n"),
				progname);
		ExitPostmaster(2);
	}

	snprintf(path, sizeof(path), "%s/global/pg_control", checkdir);

	fp = AllocateFile(path, PG_BINARY_R);
	if (fp == NULL)
	{
		fprintf(stderr, gettext(
			"%s does not find the database system.\n"
			"Expected to find it in the PGDATA directory \"%s\",\n"
			"but unable to open file \"%s\": %s\n\n"),
				progname, checkdir, path, strerror(errno));
		ExitPostmaster(2);
	}

	FreeFile(fp);

	ValidatePgVersion(checkdir);
}


int
PostmasterMain(int argc, char *argv[])
{
	int			opt;
	int			status;
	char		original_extraoptions[MAXPGPATH];
	char	   *potential_DataDir = NULL;

	IsUnderPostmaster = true;	/* so that backends know this */

	*original_extraoptions = '\0';

	progname = argv[0];
	real_argv = argv;
	real_argc = argc;

	/*
	 * Catch standard options before doing much else.  This even works on
	 * systems without getopt_long.
	 */
	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			usage(progname);
			ExitPostmaster(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("postmaster (PostgreSQL) " PG_VERSION);
			ExitPostmaster(0);
		}
	}


	/*
	 * for security, no dir or file created can be group or other
	 * accessible
	 */
	umask((mode_t) 0077);

	MyProcPid = getpid();

	/*
	 * Fire up essential subsystems: error and memory management
	 */
	EnableExceptionHandling(true);
	MemoryContextInit();

	/*
	 * By default, palloc() requests in the postmaster will be allocated
	 * in the PostmasterContext, which is space that can be recycled by
	 * backends.  Allocated data that needs to be available to backends
	 * should be allocated in TopMemoryContext.
	 */
	PostmasterContext = AllocSetContextCreate(TopMemoryContext,
											  "Postmaster",
											  ALLOCSET_DEFAULT_MINSIZE,
											  ALLOCSET_DEFAULT_INITSIZE,
											  ALLOCSET_DEFAULT_MAXSIZE);
	MemoryContextSwitchTo(PostmasterContext);

	/*
	 * Options setup
	 */
	ResetAllOptions(true);

	/* PGPORT environment variable, if set, overrides GUC setting */
	if (getenv("PGPORT"))
		SetConfigOption("port", getenv("PGPORT"), PGC_POSTMASTER, true);

	potential_DataDir = getenv("PGDATA");		/* default value */

	/*
	 * First we must scan for a -D argument to get the data dir. Then read
	 * the config file. Finally, scan all the other arguments. (Command
	 * line switches override config file.)
	 *
	 * Note: The two lists of options must be exactly the same, even though
	 * perhaps the first one would only have to be "D:" with opterr turned
	 * off. But some versions of getopt (notably GNU) are going to
	 * arbitrarily permute some "non-options" (according to the local
	 * world view) which will result in some switches being associated
	 * with the wrong argument. Death and destruction will occur.
	 */
	opterr = 1;
	while ((opt = getopt(argc, argv, "A:a:B:b:c:D:d:Fh:ik:lm:MN:no:p:Ss-:")) != EOF)
	{
		switch (opt)
		{
			case 'D':
				potential_DataDir = optarg;
				break;

			case '?':
				fprintf(stderr, gettext("Try '%s --help' for more information.\n"), progname);
				ExitPostmaster(1);
		}
	}

	/*
	 * Non-option switch arguments don't exist.
	 */
	if (optind < argc)
	{
		postmaster_error("invalid argument -- %s", argv[optind]);
		fprintf(stderr, gettext("Try '%s --help' for more information.\n"), progname);
		ExitPostmaster(1);
	}

	checkDataDir(potential_DataDir);	/* issues error messages */
	SetDataDir(potential_DataDir);

	ProcessConfigFile(PGC_POSTMASTER);

	IgnoreSystemIndexes(false);

	optind = 1;					/* start over */
#ifdef HAVE_INT_OPTRESET
	optreset = 1;
#endif
	while ((opt = getopt(argc, argv, "A:a:B:b:c:D:d:Fh:ik:lm:MN:no:p:Ss-:")) != EOF)
	{
		switch (opt)
		{
			case 'A':
#ifdef USE_ASSERT_CHECKING
				SetConfigOption("debug_assertions", optarg, PGC_POSTMASTER, true);
#else
				postmaster_error("Assert checking is not compiled in.");
#endif
				break;
			case 'a':
				/* Can no longer set authentication method. */
				break;
			case 'B':
				SetConfigOption("shared_buffers", optarg, PGC_POSTMASTER, true);
				break;
			case 'b':
				/* Can no longer set the backend executable file to use. */
				break;
			case 'D':
				/* already done above */
				break;
			case 'd':
				/*
				 * Turn on debugging for the postmaster and the backend
				 * servers descended from it.
				 */
				SetConfigOption("debug_level", optarg, PGC_POSTMASTER, true);
				break;
			case 'F':
				SetConfigOption("fsync", "false", PGC_POSTMASTER, true);
				break;
			case 'h':
				SetConfigOption("virtual_host", optarg, PGC_POSTMASTER, true);
				break;
			case 'i':
				SetConfigOption("tcpip_socket", "true", PGC_POSTMASTER, true);
				break;
			case 'k':
				SetConfigOption("unix_socket_directory", optarg, PGC_POSTMASTER, true);
				break;
#ifdef USE_SSL
			case 'l':
				SetConfigOption("ssl", "true", PGC_POSTMASTER, true);
				break;
#endif
			case 'm':
				/* Multiplexed backends no longer supported. */
				break;
			case 'M':

				/*
				 * ignore this flag.  This may be passed in because the
				 * program was run as 'postgres -M' instead of
				 * 'postmaster'
				 */
				break;
			case 'N':

				/*
				 * The max number of backends to start. Can't set to less
				 * than 1 or more than compiled-in limit.
				 */
				SetConfigOption("max_connections", optarg, PGC_POSTMASTER, true);
				break;
			case 'n':
				/* Don't reinit shared mem after abnormal exit */
				Reinit = false;
				break;
			case 'o':

				/*
				 * Other options to pass to the backend on the command
				 * line -- useful only for debugging.
				 */
				strcat(ExtraOptions, " ");
				strcat(ExtraOptions, optarg);
				strcpy(original_extraoptions, optarg);
				break;
			case 'p':
				SetConfigOption("port", optarg, PGC_POSTMASTER, true);
				break;
			case 'S':

				/*
				 * Start in 'S'ilent mode (disassociate from controlling
				 * tty). You may also think of this as 'S'ysV mode since
				 * it's most badly needed on SysV-derived systems like
				 * SVR4 and HP-UX.
				 */
				SetConfigOption("silent_mode", "true", PGC_POSTMASTER, true);
				break;
			case 's':

				/*
				 * In the event that some backend dumps core, send
				 * SIGSTOP, rather than SIGQUIT, to all its peers.	This
				 * lets the wily post_hacker collect core dumps from
				 * everyone.
				 */
				SendStop = true;
				break;
			case 'c':
			case '-':
				{
					char	   *name,
							   *value;

					ParseLongOption(optarg, &name, &value);
					if (!value)
					{
						if (opt == '-')
							elog(ERROR, "--%s requires argument", optarg);
						else
							elog(ERROR, "-c %s requires argument", optarg);
					}

					SetConfigOption(name, value, PGC_POSTMASTER, true);
					free(name);
					if (value)
						free(value);
					break;
				}

			default:
				/* shouldn't get here */
				fprintf(stderr, gettext("Try '%s --help' for more information.\n"), progname);
				ExitPostmaster(1);
		}
	}

	/*
	 * Check for invalid combinations of switches
	 */
	if (NBuffers < 2 * MaxBackends || NBuffers < 16)
	{
		/*
		 * Do not accept -B so small that backends are likely to starve
		 * for lack of buffers.  The specific choices here are somewhat
		 * arbitrary.
		 */
		postmaster_error("The number of buffers (-B) must be at least twice the number of allowed connections (-N) and at least 16.");
		ExitPostmaster(1);
	}

	/*
	 * Initialize and startup the statistics collector process
	 */
	if (pgstat_init() < 0)
		ExitPostmaster(1);
	if (pgstat_start() < 0)
		ExitPostmaster(1);

	if (DebugLvl > 2)
	{
		extern char **environ;
		char	  **p;

		fprintf(stderr, "%s: PostmasterMain: initial environ dump:\n",
				progname);
		fprintf(stderr, "-----------------------------------------\n");
		for (p = environ; *p; ++p)
			fprintf(stderr, "\t%s\n", *p);
		fprintf(stderr, "-----------------------------------------\n");
	}

	/*
	 * Initialize SSL library, if specified.
	 */
#ifdef USE_SSL
	if (EnableSSL && !NetServer)
	{
		postmaster_error("For SSL, TCP/IP connections must be enabled.");
		fprintf(stderr, gettext("Try '%s --help' for more information.\n"), progname);
		ExitPostmaster(1);
	}
	if (EnableSSL)
		InitSSL();
#endif

	/*
	 * Fork away from controlling terminal, if -S specified.
	 *
	 * Must do this before we grab any interlock files, else the interlocks
	 * will show the wrong PID.
	 */
	if (SilentMode)
		pmdaemonize(argc, argv);

	/*
	 * Create lockfile for data directory.
	 *
	 * We want to do this before we try to grab the input sockets, because
	 * the data directory interlock is more reliable than the socket-file
	 * interlock (thanks to whoever decided to put socket files in /tmp
	 * :-(). For the same reason, it's best to grab the TCP socket before
	 * the Unix socket.
	 */
	if (!CreateDataDirLockFile(DataDir, true))
		ExitPostmaster(1);

	/*
	 * Remove old temporary files.  At this point there can be no other
	 * Postgres processes running in this directory, so this should be safe.
	 */
	RemovePgTempFiles();

	/*
	 * Establish input sockets.
	 */
	if (NetServer)
	{
		status = StreamServerPort(AF_INET, VirtualHost,
								  (unsigned short) PostPortNumber,
								  UnixSocketDir,
								  &ServerSock_INET);
		if (status != STATUS_OK)
		{
			postmaster_error("cannot create INET stream port");
			ExitPostmaster(1);
		}
	}

#ifdef HAVE_UNIX_SOCKETS
	status = StreamServerPort(AF_UNIX, VirtualHost,
							  (unsigned short) PostPortNumber,
							  UnixSocketDir,
							  &ServerSock_UNIX);
	if (status != STATUS_OK)
	{
		postmaster_error("cannot create UNIX stream port");
		ExitPostmaster(1);
	}
#endif

	XLOGPathInit();

	/*
	 * Set up shared memory and semaphores.
	 */
	reset_shared(PostPortNumber);

	/*
	 * Initialize the list of active backends.
	 */
	BackendList = DLNewList();

	/*
	 * Record postmaster options.  We delay this till now to avoid
	 * recording bogus options (eg, NBuffers too high for available
	 * memory).
	 */
	if (!CreateOptsFile(argc, argv))
		ExitPostmaster(1);

	/*
	 * Set up signal handlers for the postmaster process.
	 */
	pqinitmask();
	PG_SETMASK(&BlockSig);

	pqsignal(SIGHUP, SIGHUP_handler);	/* reread config file and have
										 * children do same */
	pqsignal(SIGINT, pmdie);	/* send SIGTERM and ShutdownDataBase */
	pqsignal(SIGQUIT, pmdie);	/* send SIGQUIT and die */
	pqsignal(SIGTERM, pmdie);	/* wait for children and ShutdownDataBase */
	pqsignal(SIGALRM, SIG_IGN); /* ignored */
	pqsignal(SIGPIPE, SIG_IGN); /* ignored */
	pqsignal(SIGUSR1, schedule_checkpoint);		/* start a background
												 * checkpoint */
	pqsignal(SIGUSR2, pmdie);	/* send SIGUSR2, don't die */
	pqsignal(SIGCHLD, reaper);	/* handle child termination */
	pqsignal(SIGTTIN, SIG_IGN); /* ignored */
	pqsignal(SIGTTOU, SIG_IGN); /* ignored */

	/*
	 * We're ready to rock and roll...
	 */
	StartupPID = StartupDataBase();

	status = ServerLoop();

	/*
	 * ServerLoop probably shouldn't ever return, but if it does, close
	 * down.
	 */
	ExitPostmaster(status != STATUS_OK);

	return 0;					/* not reached */
}

static void
pmdaemonize(int argc, char *argv[])
{
	int			i;
	pid_t		pid;

	pid = fork();
	if (pid == (pid_t) -1)
	{
		postmaster_error("fork failed: %s", strerror(errno));
		ExitPostmaster(1);
		return;					/* not reached */
	}
	else if (pid)
	{							/* parent */
		/* Parent should just exit, without doing any atexit cleanup */
		_exit(0);
	}

	MyProcPid = getpid();		/* reset MyProcPid to child */

/* GH: If there's no setsid(), we hopefully don't need silent mode.
 * Until there's a better solution.
 */
#ifdef HAVE_SETSID
	if (setsid() < 0)
	{
		postmaster_error("cannot disassociate from controlling TTY: %s",
						 strerror(errno));
		ExitPostmaster(1);
	}
#endif
	i = open(NULL_DEV, O_RDWR | PG_BINARY);
	dup2(i, 0);
	dup2(i, 1);
	dup2(i, 2);
	close(i);
}



/*
 * Print out help message
 */
static void
usage(const char *progname)
{
	printf(gettext("%s is the PostgreSQL server.\n\n"), progname);
	printf(gettext("Usage:\n  %s [options...]\n\n"), progname);
	printf(gettext("Options:\n"));
#ifdef USE_ASSERT_CHECKING
	printf(gettext("  -A 1|0          enable/disable run-time assert checking\n"));
#endif
	printf(gettext("  -B NBUFFERS     number of shared buffers (default %d)\n"), DEF_NBUFFERS);
	printf(gettext("  -c NAME=VALUE   set run-time parameter\n"));
	printf(gettext("  -d 1-5          debugging level\n"));
	printf(gettext("  -D DATADIR      database directory\n"));
	printf(gettext("  -F              turn fsync off\n"));
	printf(gettext("  -h HOSTNAME     host name or IP address to listen on\n"));
	printf(gettext("  -i              enable TCP/IP connections\n"));
	printf(gettext("  -k DIRECTORY    Unix-domain socket location\n"));
#ifdef USE_SSL
	printf(gettext("  -l              enable SSL connections\n"));
#endif
	printf(gettext("  -N MAX-CONNECT  maximum number of allowed connections (1..%d, default %d)\n"),
		   MAXBACKENDS, DEF_MAXBACKENDS);
	printf(gettext("  -o OPTIONS      pass 'OPTIONS' to each backend server\n"));
	printf(gettext("  -p PORT         port number to listen on (default %d)\n"), DEF_PGPORT);
	printf(gettext("  -S              silent mode (start in background without logging output)\n"));

	printf(gettext("\nDeveloper options:\n"));
	printf(gettext("  -n              do not reinitialize shared memory after abnormal exit\n"));
	printf(gettext("  -s              send SIGSTOP to all backend servers if one dies\n"));

	printf(gettext("\nPlease read the documentation for the complete list of run-time\n"
				   "configuration settings and how to set them on the command line or in\n"
				   "the configuration file.\n\n"
				   "Report bugs to <pgsql-bugs@postgresql.org>.\n"));
}

static int
ServerLoop(void)
{
	fd_set		readmask,
				writemask;
	int			nSockets;
	struct timeval now,
				later;
	struct timezone tz;

	gettimeofday(&now, &tz);

	nSockets = initMasks(&readmask, &writemask);

	for (;;)
	{
		Port	   *port;
		fd_set		rmask,
					wmask;
		struct timeval *timeout = NULL;
		struct timeval timeout_tv;

		if (CheckPointPID == 0 && checkpointed &&
			Shutdown == NoShutdown && !FatalError)
		{
			time_t		now = time(NULL);

			if (CheckPointTimeout + checkpointed > now)
			{
				/*
				 * Not time for checkpoint yet, so set a timeout for
				 * select
				 */
				timeout_tv.tv_sec = CheckPointTimeout + checkpointed - now;
				timeout_tv.tv_usec = 0;
				timeout = &timeout_tv;
			}
			else
			{
				/* Time to make the checkpoint... */
				CheckPointPID = CheckPointDataBase();

				/*
				 * if fork failed, schedule another try at 0.1 normal
				 * delay
				 */
				if (CheckPointPID == 0)
					checkpointed = now - (9 * CheckPointTimeout) / 10;
			}
		}

		/*
		 * Wait for something to happen.
		 */
		memcpy((char *) &rmask, (char *) &readmask, sizeof(fd_set));
		memcpy((char *) &wmask, (char *) &writemask, sizeof(fd_set));

		PG_SETMASK(&UnBlockSig);

		if (select(nSockets, &rmask, &wmask, (fd_set *) NULL, timeout) < 0)
		{
			PG_SETMASK(&BlockSig);
			if (errno == EINTR || errno == EWOULDBLOCK)
				continue;
			postmaster_error("ServerLoop: select failed: %s", strerror(errno));
			return STATUS_ERROR;
		}

		/*
		 * Block all signals until we wait again
		 */
		PG_SETMASK(&BlockSig);

		/*
		 * Respond to signals, if needed
		 */
		if (got_SIGHUP)
		{
			got_SIGHUP = false;
			ProcessConfigFile(PGC_SIGHUP);
		}

		/*
		 * Select a random seed at the time of first receiving a request.
		 */
		while (random_seed == 0)
		{
			gettimeofday(&later, &tz);

			/*
			 * We are not sure how much precision is in tv_usec, so we
			 * swap the nibbles of 'later' and XOR them with 'now'. On the
			 * off chance that the result is 0, we loop until it isn't.
			 */
			random_seed = now.tv_usec ^
				((later.tv_usec << 16) |
				 ((later.tv_usec >> 16) & 0xffff));
		}

		/*
		 * New connection pending on our well-known port's socket?
		 * If so, fork a child process to deal with it.
		 */

#ifdef HAVE_UNIX_SOCKETS
		if (ServerSock_UNIX != INVALID_SOCK
			&& FD_ISSET(ServerSock_UNIX, &rmask))
		{
			port = ConnCreate(ServerSock_UNIX);
			if (port)
			{
				BackendStartup(port);
				/*
				 * We no longer need the open socket or port structure
				 * in this process
				 */
				StreamClose(port->sock);
				ConnFree(port);
			}
		}
#endif

		if (ServerSock_INET != INVALID_SOCK
			&& FD_ISSET(ServerSock_INET, &rmask))
		{
			port = ConnCreate(ServerSock_INET);
			if (port)
			{
				BackendStartup(port);
				/*
				 * We no longer need the open socket or port structure
				 * in this process
				 */
				StreamClose(port->sock);
				ConnFree(port);
			}
		}
	}
}


/*
 * Initialise the read and write masks for select() for the well-known ports
 * we are listening on.  Return the number of sockets to listen on.
 */

static int
initMasks(fd_set *rmask, fd_set *wmask)
{
	int			nsocks = -1;

	FD_ZERO(rmask);
	FD_ZERO(wmask);

#ifdef HAVE_UNIX_SOCKETS
	if (ServerSock_UNIX != INVALID_SOCK)
	{
		FD_SET(ServerSock_UNIX, rmask);

		if (ServerSock_UNIX > nsocks)
			nsocks = ServerSock_UNIX;
	}
#endif

	if (ServerSock_INET != INVALID_SOCK)
	{
		FD_SET(ServerSock_INET, rmask);
		if (ServerSock_INET > nsocks)
			nsocks = ServerSock_INET;
	}

	return nsocks + 1;
}


/*
 * Read the startup packet and do something according to it.
 *
 * Returns STATUS_OK or STATUS_ERROR, or might call elog(FATAL) and
 * not return at all.
 */
static int
ProcessStartupPacket(Port *port, bool SSLdone)
{
	StartupPacket *packet;
	char	   *rejectMsg;
	int32		len;
	void	   *buf;

	pq_getbytes((char *)&len, 4);
	len = ntohl(len);
	len -= 4;

	if (len < sizeof(len) || len > sizeof(len) + sizeof(StartupPacket))
		elog(FATAL, "invalid length of startup packet");

	buf = palloc(len);
	pq_getbytes(buf, len);
	
	packet = buf;

	/*
	 * The first field is either a protocol version number or a special
	 * request code.
	 */
	port->proto = ntohl(packet->protoVersion);

	if (port->proto == CANCEL_REQUEST_CODE)
	{
		processCancelRequest(port, packet);
		return 127;				/* XXX */
	}

	if (port->proto == NEGOTIATE_SSL_CODE && !SSLdone)
	{
		char		SSLok;

#ifdef USE_SSL
		/* No SSL when disabled or on Unix sockets */
		if (!EnableSSL || port->laddr.sa.sa_family != AF_INET)
			SSLok = 'N';
		else
			SSLok = 'S';		/* Support for SSL */
#else
		SSLok = 'N';			/* No support for SSL */
#endif
		if (send(port->sock, &SSLok, 1, 0) != 1)
		{
			postmaster_error("failed to send SSL negotiation response: %s",
							 strerror(errno));
			return STATUS_ERROR; /* close the connection */
		}

#ifdef USE_SSL
		if (SSLok == 'S')
		{
			if (!(port->ssl = SSL_new(SSL_context)) ||
				!SSL_set_fd(port->ssl, port->sock) ||
				SSL_accept(port->ssl) <= 0)
			{
				postmaster_error("failed to initialize SSL connection: %s, errno: %d (%s)",
								 ERR_reason_error_string(ERR_get_error()), errno, strerror(errno));
				return STATUS_ERROR;
			}
		}
#endif
		/* regular startup packet, cancel, etc packet should follow... */
		/* but not another SSL negotiation request */
		return ProcessStartupPacket(port, true);
	}

	/* Could add additional special packet types here */


	/* Check we can handle the protocol the frontend is using. */

	if (PG_PROTOCOL_MAJOR(port->proto) < PG_PROTOCOL_MAJOR(PG_PROTOCOL_EARLIEST) ||
		PG_PROTOCOL_MAJOR(port->proto) > PG_PROTOCOL_MAJOR(PG_PROTOCOL_LATEST) ||
		(PG_PROTOCOL_MAJOR(port->proto) == PG_PROTOCOL_MAJOR(PG_PROTOCOL_LATEST) &&
		 PG_PROTOCOL_MINOR(port->proto) > PG_PROTOCOL_MINOR(PG_PROTOCOL_LATEST)))
		elog(FATAL, "unsupported frontend protocol");

	/*
	 * Get the parameters from the startup packet as C strings.  The
	 * packet destination was cleared first so a short packet has zeros
	 * silently added and a long packet is silently truncated.
	 */
	StrNCpy(port->database, packet->database, sizeof(port->database));
	StrNCpy(port->user, packet->user, sizeof(port->user));
	StrNCpy(port->options, packet->options, sizeof(port->options));
	StrNCpy(port->tty, packet->tty, sizeof(port->tty));

	/* The database defaults to the user name. */
	if (port->database[0] == '\0')
		StrNCpy(port->database, packet->user, sizeof(port->database));

	/*
	 * Truncate given database and user names to length of a Postgres
	 * name.  This avoids lookup failures when overlength names are
	 * given.
	 */
	if ((int) sizeof(port->database) >= NAMEDATALEN)
		port->database[NAMEDATALEN - 1] = '\0';
	if ((int) sizeof(port->user) >= NAMEDATALEN)
		port->user[NAMEDATALEN - 1] = '\0';

	/* Check a user name was given. */
	if (port->user[0] == '\0')
		elog(FATAL, "no PostgreSQL user name specified in startup packet");

	/*
	 * If we're going to reject the connection due to database state, say
	 * so now instead of wasting cycles on an authentication exchange.
	 * (This also allows a pg_ping utility to be written.)
	 */
	rejectMsg = canAcceptConnections();

	if (rejectMsg != NULL)
		elog(FATAL, "%s", rejectMsg);

	return STATUS_OK;
}


/*
 * The client has sent a cancel request packet, not a normal
 * start-a-new-connection packet.  Perform the necessary processing.
 * Nothing is sent back to the client.
 */
static void
processCancelRequest(Port *port, void *pkt)
{
	CancelRequestPacket *canc = (CancelRequestPacket *) pkt;
	int			backendPID;
	long		cancelAuthCode;
	Dlelem	   *curr;
	Backend    *bp;

	backendPID = (int) ntohl(canc->backendPID);
	cancelAuthCode = (long) ntohl(canc->cancelAuthCode);

	if (backendPID == CheckPointPID)
	{
		if (DebugLvl)
			postmaster_error("processCancelRequest: CheckPointPID in cancel request for process %d", backendPID);
		return;
	}

	/* See if we have a matching backend */

	for (curr = DLGetHead(BackendList); curr; curr = DLGetSucc(curr))
	{
		bp = (Backend *) DLE_VAL(curr);
		if (bp->pid == backendPID)
		{
			if (bp->cancel_key == cancelAuthCode)
			{
				/* Found a match; signal that backend to cancel current op */
				if (DebugLvl)
					elog(DEBUG, "processing cancel request: sending SIGINT to process %d",
						 backendPID);
				kill(bp->pid, SIGINT);
			}
			else
			{
				/* Right PID, wrong key: no way, Jose */
				if (DebugLvl)
					elog(DEBUG, "bad key in cancel request for process %d",
						 backendPID);
			}
			return;
		}
	}

	/* No matching backend */
	if (DebugLvl)
		elog(DEBUG, "bad pid in cancel request for process %d", backendPID);
}

/*
 * canAcceptConnections --- check to see if database state allows connections.
 *
 * If we are open for business, return NULL, otherwise return an error message
 * string suitable for rejecting a connection request.
 */
static char *
canAcceptConnections(void)
{
	/* Can't start backends when in startup/shutdown/recovery state. */
	if (Shutdown > NoShutdown)
		return "The Data Base System is shutting down";
	if (StartupPID)
		return "The Data Base System is starting up";
	if (FatalError)
		return "The Data Base System is in recovery mode";
	/*
	 * Don't start too many children.
	 *
	 * We allow more connections than we can have backends here because
	 * some might still be authenticating; they might fail auth, or some
	 * existing backend might exit before the auth cycle is completed.
	 * The exact MaxBackends limit is enforced when a new backend tries
	 * to join the shared-inval backend array.
	 */
	if (CountChildren() >= 2 * MaxBackends)
		return "Sorry, too many clients already";

	return NULL;
}


/*
 * ConnCreate -- create a local connection data structure
 */
static Port *
ConnCreate(int serverFd)
{
	Port	   *port;

	if (!(port = (Port *) calloc(1, sizeof(Port))))
	{
		postmaster_error("ConnCreate: malloc failed");
		SignalChildren(SIGQUIT);
		ExitPostmaster(1);
	}

	if (StreamConnection(serverFd, port) != STATUS_OK)
	{
		StreamClose(port->sock);
		ConnFree(port);
		port = NULL;
	}
	else
	{
		RandomSalt(port->salt);
		port->pktInfo.state = Idle;
	}

	return port;
}


/*
 * ConnFree -- free a local connection data structure
 */
static void
ConnFree(Port *conn)
{
#ifdef USE_SSL
	if (conn->ssl)
		SSL_free(conn->ssl);
#endif
	free(conn);
}


/*
 * ClosePostmasterPorts -- close all the postmaster's open sockets
 *
 * This is called during child process startup to release file descriptors
 * that are not needed by that child process.  The postmaster still has
 * them open, of course.
 */
static void
ClosePostmasterPorts(void)
{
	/* Close the listen sockets */
	if (NetServer)
		StreamClose(ServerSock_INET);
	ServerSock_INET = INVALID_SOCK;
#ifdef HAVE_UNIX_SOCKETS
	StreamClose(ServerSock_UNIX);
	ServerSock_UNIX = INVALID_SOCK;
#endif
}


/*
 * reset_shared -- reset shared memory and semaphores
 */
static void
reset_shared(unsigned short port)
{

	/*
	 * Reset assignment of shared mem and semaphore IPC keys. Doing this
	 * means that in normal cases we'll assign the same keys on each
	 * "cycle of life", and thereby avoid leaving dead IPC objects
	 * floating around if the postmaster crashes and is restarted.
	 */
	IpcInitKeyAssignment(port);

	/*
	 * Create or re-create shared memory and semaphores.
	 */
	CreateSharedMemoryAndSemaphores(false, MaxBackends);
}


/*
 * Set flag if SIGHUP was detected so config file can be reread in
 * main loop
 */
static void
SIGHUP_handler(SIGNAL_ARGS)
{
	int			save_errno = errno;

	PG_SETMASK(&BlockSig);

	if (Shutdown <= SmartShutdown)
	{
		got_SIGHUP = true;
		SignalChildren(SIGHUP);
	}

	errno = save_errno;
}



/*
 * pmdie -- signal handler for processing various postmaster signals.
 */
static void
pmdie(SIGNAL_ARGS)
{
	int			save_errno = errno;

	PG_SETMASK(&BlockSig);

	if (DebugLvl >= 1)
		elog(DEBUG, "pmdie %d", postgres_signal_arg);

	switch (postgres_signal_arg)
	{
		case SIGUSR2:

			/*
			 * Send SIGUSR2 to all children (AsyncNotifyHandler)
			 */
			if (Shutdown > SmartShutdown)
			{
				errno = save_errno;
				return;
			}
			SignalChildren(SIGUSR2);
			errno = save_errno;
			return;

		case SIGTERM:

			/*
			 * Smart Shutdown:
			 *
			 * Wait for children to end their work and ShutdownDataBase.
			 */
			if (Shutdown >= SmartShutdown)
			{
				errno = save_errno;
				return;
			}
			Shutdown = SmartShutdown;
			tnow = time(NULL);
			fprintf(stderr, gettext("Smart Shutdown request at %s"), ctime(&tnow));
			fflush(stderr);
			if (DLGetHead(BackendList)) /* let reaper() handle this */
			{
				errno = save_errno;
				return;
			}

			/*
			 * No children left. Shutdown data base system.
			 */
			if (StartupPID > 0 || FatalError)	/* let reaper() handle
												 * this */
			{
				errno = save_errno;
				return;
			}
			if (ShutdownPID > 0)
				abort();

			ShutdownPID = ShutdownDataBase();
			errno = save_errno;
			return;

		case SIGINT:

			/*
			 * Fast Shutdown:
			 *
			 * abort all children with SIGTERM (rollback active transactions
			 * and exit) and ShutdownDataBase when they are gone.
			 */
			if (Shutdown >= FastShutdown)
			{
				errno = save_errno;
				return;
			}
			tnow = time(NULL);
			fprintf(stderr, gettext("Fast Shutdown request at %s"), ctime(&tnow));
			fflush(stderr);
			if (DLGetHead(BackendList)) /* let reaper() handle this */
			{
				Shutdown = FastShutdown;
				if (!FatalError)
				{
					fprintf(stderr, gettext("Aborting any active transaction...\n"));
					fflush(stderr);
					SignalChildren(SIGTERM);
				}
				errno = save_errno;
				return;
			}
			if (Shutdown > NoShutdown)
			{
				Shutdown = FastShutdown;
				errno = save_errno;
				return;
			}
			Shutdown = FastShutdown;

			/*
			 * No children left. Shutdown data base system.
			 */
			if (StartupPID > 0 || FatalError)	/* let reaper() handle
												 * this */
			{
				errno = save_errno;
				return;
			}
			if (ShutdownPID > 0)
				abort();

			ShutdownPID = ShutdownDataBase();
			errno = save_errno;
			return;

		case SIGQUIT:

			/*
			 * Immediate Shutdown:
			 *
			 * abort all children with SIGQUIT and exit without attempt to
			 * properly shutdown data base system.
			 */
			tnow = time(NULL);
			fprintf(stderr, gettext("Immediate Shutdown request at %s"), ctime(&tnow));
			fflush(stderr);
			if (ShutdownPID > 0)
				kill(ShutdownPID, SIGQUIT);
			if (StartupPID > 0)
				kill(StartupPID, SIGQUIT);
			if (DLGetHead(BackendList))
				SignalChildren(SIGQUIT);
			break;
	}

	/* exit postmaster */
	ExitPostmaster(0);
}

/*
 * Reaper -- signal handler to cleanup after a backend (child) dies.
 */
static void
reaper(SIGNAL_ARGS)
{
	int			save_errno = errno;
#ifdef HAVE_WAITPID
	int			status;			/* backend exit status */
#else
	union wait	status;			/* backend exit status */
#endif
	int			exitstatus;
	int			pid;			/* process id of dead backend */

	PG_SETMASK(&BlockSig);
	/* It's not really necessary to reset the handler each time is it? */
	pqsignal(SIGCHLD, reaper);

	if (DebugLvl)
		postmaster_error("reaping dead processes");
#ifdef HAVE_WAITPID
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		exitstatus = status;
#else
	while ((pid = wait3(&status, WNOHANG, NULL)) > 0)
	{
		exitstatus = status.w_status;
#endif
		/*
		 * Check if this child was the statistics collector. If
		 * so, start a new one. 
		 */
		if (pgstat_ispgstat(pid))
		{
			fprintf(stderr, "%s: Performance collector exited with status %d\n",
					progname, exitstatus);
			pgstat_start();
			continue;
		}

		if (ShutdownPID > 0)
		{
			if (pid != ShutdownPID)
				abort();
			if (exitstatus != 0)
			{
				postmaster_error("Shutdown proc %d exited with status %d", pid, exitstatus);
				fflush(stderr);
				ExitPostmaster(1);
			}
			ExitPostmaster(0);
		}
		if (StartupPID > 0)
		{
			if (pid != StartupPID)
				abort();
			if (exitstatus != 0)
			{
				postmaster_error("Startup proc %d exited with status %d - abort",
								 pid, exitstatus);
				fflush(stderr);
				ExitPostmaster(1);
			}
			StartupPID = 0;
			FatalError = false; /* done with recovery */
			if (Shutdown > NoShutdown)
			{
				if (ShutdownPID > 0)
					abort();
				ShutdownPID = ShutdownDataBase();
			}

			/*
			 * Startup succeeded - remember its ID and RedoRecPtr
			 */
			SetThisStartUpID();

			/*
			 * Arrange for first checkpoint to occur after standard delay.
			 */
			CheckPointPID = 0;
			checkpointed = time(NULL);

			errno = save_errno;
			return;
		}

		CleanupProc(pid, exitstatus);
	}

	if (FatalError)
	{

		/*
		 * Wait for all children exit, then reset shmem and
		 * StartupDataBase.
		 */
		if (DLGetHead(BackendList) || StartupPID > 0 || ShutdownPID > 0)
		{
			errno = save_errno;
			return;
		}
		tnow = time(NULL);
		fprintf(stderr, gettext("Server processes were terminated at %s"
				"Reinitializing shared memory and semaphores\n"),
				ctime(&tnow));
		fflush(stderr);

		shmem_exit(0);
		reset_shared(PostPortNumber);

		StartupPID = StartupDataBase();
		errno = save_errno;
		return;
	}

	if (Shutdown > NoShutdown)
	{
		if (DLGetHead(BackendList))
		{
			errno = save_errno;
			return;
		}
		if (StartupPID > 0 || ShutdownPID > 0)
		{
			errno = save_errno;
			return;
		}
		ShutdownPID = ShutdownDataBase();
	}

	errno = save_errno;
}

/*
 * CleanupProc -- cleanup after terminated backend.
 *
 * Remove all local state associated with backend.
 *
 * Dillon's note: should log child's exit status in the system log.
 */
static void
CleanupProc(int pid,
			int exitstatus)		/* child's exit status. */
{
	Dlelem	   *curr,
			   *next;
	Backend    *bp;

	if (DebugLvl)
		postmaster_error("CleanupProc: pid %d exited with status %d",
						 pid, exitstatus);

	/*
	 * If a backend dies in an ugly way (i.e. exit status not 0) then we
	 * must signal all other backends to quickdie.	If exit status is zero
	 * we assume everything is hunky dory and simply remove the backend
	 * from the active backend list.
	 */
	if (exitstatus == 0)
	{
		curr = DLGetHead(BackendList);
		while (curr)
		{
			bp = (Backend *) DLE_VAL(curr);
			if (bp->pid == pid)
			{
				DLRemove(curr);
				free(bp);
				DLFreeElem(curr);
				break;
			}
			curr = DLGetSucc(curr);
		}

		if (pid == CheckPointPID)
		{
			CheckPointPID = 0;
			if (!FatalError)
			{
				checkpointed = time(NULL);
				/* Update RedoRecPtr for future child backends */
				GetRedoRecPtr();
			}
		}
		else
			pgstat_beterm(pid);

		return;
	}

	if (!FatalError)
	{
		/* Make log entry unless we did so already */
		tnow = time(NULL);
		fprintf(stderr, gettext("Server process (pid %d) exited with status %d at %s"
								"Terminating any active server processes...\n"),
				pid, exitstatus, ctime(&tnow));
		fflush(stderr);
	}

	curr = DLGetHead(BackendList);
	while (curr)
	{
		next = DLGetSucc(curr);
		bp = (Backend *) DLE_VAL(curr);
		if (bp->pid != pid)
		{
			/*
			 * This backend is still alive.  Unless we did so already,
			 * tell it to commit hara-kiri.
			 *
			 * SIGQUIT is the special signal that says exit without proc_exit
			 * and let the user know what's going on. But if SendStop is
			 * set (-s on command line), then we send SIGSTOP instead, so
			 * that we can get core dumps from all backends by hand.
			 */
			if (!FatalError)
			{
				if (DebugLvl)
					postmaster_error("CleanupProc: sending %s to process %d",
									 (SendStop ? "SIGSTOP" : "SIGQUIT"),
									 bp->pid);
				kill(bp->pid, (SendStop ? SIGSTOP : SIGQUIT));
			}
		}
		else
		{
			/*
			 * Found entry for freshly-dead backend, so remove it.
			 */
			DLRemove(curr);
			free(bp);
			DLFreeElem(curr);
		}
		curr = next;
	}

	if (pid == CheckPointPID)
	{
		CheckPointPID = 0;
		checkpointed = 0;
	}
	else
	{
		/*
		 * Tell the collector about backend termination
		 */
		pgstat_beterm(pid);
	}

	FatalError = true;
}

/*
 * Send a signal to all backend children.
 */
static void
SignalChildren(int signal)
{
	Dlelem	   *curr,
			   *next;
	Backend    *bp;
	int			mypid = getpid();

	curr = DLGetHead(BackendList);
	while (curr)
	{
		next = DLGetSucc(curr);
		bp = (Backend *) DLE_VAL(curr);

		if (bp->pid != mypid)
		{
			if (DebugLvl >= 1)
				elog(DEBUG, "SignalChildren: sending signal %d to process %d",
					 signal, bp->pid);

			kill(bp->pid, signal);
		}

		curr = next;
	}
}

/*
 * BackendStartup -- start backend process
 *
 * returns: STATUS_ERROR if the fork/exec failed, STATUS_OK otherwise.
 */
static int
BackendStartup(Port *port)
{
	Backend    *bn;				/* for backend cleanup */
	pid_t		pid;

	/*
	 * Compute the cancel key that will be assigned to this backend. The
	 * backend will have its own copy in the forked-off process' value of
	 * MyCancelKey, so that it can transmit the key to the frontend.
	 */
	MyCancelKey = PostmasterRandom();

	/*
	 * Flush stdio channels just before fork, to avoid double-output
	 * problems. Ideally we'd use fflush(NULL) here, but there are still a
	 * few non-ANSI stdio libraries out there (like SunOS 4.1.x) that
	 * coredump if we do. Presently stdout and stderr are the only stdio
	 * output channels used by the postmaster, so fflush'ing them should
	 * be sufficient.
	 */
	fflush(stdout);
	fflush(stderr);

#ifdef __BEOS__
	/* Specific beos actions before backend startup */
	beos_before_backend_startup();
#endif

	/*
	 * Make room for backend data structure.  Better before the fork()
	 * so we can handle failure cleanly.
	 */
	bn = (Backend *) malloc(sizeof(Backend));
	if (!bn)
	{
		fprintf(stderr, gettext("%s: BackendStartup: malloc failed\n"),
				progname);
		return STATUS_ERROR;
	}

	pid = fork();

	if (pid == 0)				/* child */
	{
		int		status;

		free(bn);
#ifdef __BEOS__
		/* Specific beos backend startup actions */
		beos_backend_startup();
#endif

		status = DoBackend(port);
		if (status != 0)
		{
			fprintf(stderr, gettext("%s child[%d]: BackendStartup: backend startup failed\n"),
					progname, (int) getpid());
			proc_exit(status);
		}
		else
			proc_exit(0);
	}

	/* in parent, error */
	if (pid < 0)
	{
		free(bn);
#ifdef __BEOS__
		/* Specific beos backend startup actions */
		beos_backend_startup_failed();
#endif
		fprintf(stderr, gettext("%s: BackendStartup: fork failed: %s\n"),
				progname, strerror(errno));
		return STATUS_ERROR;
	}

	/* in parent, normal */
	if (DebugLvl >= 1)
		fprintf(stderr, gettext("%s: BackendStartup: pid=%d user=%s db=%s socket=%d\n"),
				progname, pid, port->user, port->database,
				port->sock);

	/*
	 * Everything's been successful, it's safe to add this backend to our
	 * list of backends.
	 */
	bn->pid = pid;
	bn->cancel_key = MyCancelKey;
	DLAddHead(BackendList, DLNewElem(bn));

	return STATUS_OK;
}


/*
 * split_opts -- split a string of options and append it to an argv array
 *
 * NB: the string is destructively modified!
 *
 * Since no current POSTGRES arguments require any quoting characters,
 * we can use the simple-minded tactic of assuming each set of space-
 * delimited characters is a separate argv element.
 *
 * If you don't like that, well, we *used* to pass the whole option string
 * as ONE argument to execl(), which was even less intelligent...
 */
static void
split_opts(char **argv, int *argcp, char *s)
{
	while (s && *s)
	{
		while (isspace((unsigned char) *s))
			++s;
		if (*s == '\0')
			break;
		argv[(*argcp)++] = s;
		while (*s && !isspace((unsigned char) *s))
			++s;
		if (*s)
			*s++ = '\0';
	}
}

/*
 * DoBackend -- perform authentication, and if successful, set up the
 *		backend's argument list and invoke backend main().
 *
 * This used to perform an execv() but we no longer exec the backend;
 * it's the same executable as the postmaster.
 *
 * returns:
 *		Shouldn't return at all.
 *		If PostgresMain() fails, return status.
 */
static int
DoBackend(Port *port)
{
	char	   *av[ARGV_SIZE * 2];
	int			ac = 0;
	char		debugbuf[ARGV_SIZE];
	char		protobuf[ARGV_SIZE];
	char		dbbuf[ARGV_SIZE];
	char		optbuf[ARGV_SIZE];
	char		ttybuf[ARGV_SIZE];
	int			i;
	int			status;
	struct timeval now;
	struct timezone tz;

	/*
	 * Let's clean up ourselves as the postmaster child
	 */

	/* We don't want the postmaster's proc_exit() handlers */
	on_exit_reset();

	/*
	 * Signal handlers setting is moved to tcop/postgres...
	 */

	/* Close the postmaster's other sockets */
	ClosePostmasterPorts();

	SetProcessingMode(InitProcessing);

	/* Save port etc. for ps status */
	MyProcPort = port;

	/* Reset MyProcPid to new backend's pid */
	MyProcPid = getpid();

	whereToSendOutput = Remote;

	status = ProcessStartupPacket(port, false);
	if (status == 127)
		return 0;				/* cancel request processed */

	/*
	 * Don't want backend to be able to see the postmaster random number
	 * generator state.  We have to clobber the static random_seed *and*
	 * start a new random sequence in the random() library function.
	 */
	random_seed = 0;
	gettimeofday(&now, &tz);
	srandom((unsigned int) now.tv_usec);

	/* ----------------
	 * Now, build the argv vector that will be given to PostgresMain.
	 *
	 * The layout of the command line is
	 *		postgres [secure switches] -p databasename [insecure switches]
	 * where the switches after -p come from the client request.
	 * ----------------
	 */

	av[ac++] = "postgres";

	/*
	 * Pass the requested debugging level along to the backend. Level one
	 * debugging in the postmaster traces postmaster connection activity,
	 * and levels two and higher are passed along to the backend.  This
	 * allows us to watch only the postmaster or the postmaster and the
	 * backend.
	 */
	if (DebugLvl > 1)
	{
		sprintf(debugbuf, "-d%d", DebugLvl);
		av[ac++] = debugbuf;
	}

	/*
	 * Pass any backend switches specified with -o in the postmaster's own
	 * command line.  We assume these are secure. (It's OK to mangle
	 * ExtraOptions since we are now in the child process; this won't
	 * change the postmaster's copy.)
	 */
	split_opts(av, &ac, ExtraOptions);

	/* Tell the backend what protocol the frontend is using. */
	sprintf(protobuf, "-v%u", port->proto);
	av[ac++] = protobuf;

	/*
	 * Tell the backend it is being called from the postmaster, and which
	 * database to use.  -p marks the end of secure switches.
	 */
	av[ac++] = "-p";

	StrNCpy(dbbuf, port->database, ARGV_SIZE);
	av[ac++] = dbbuf;

	/*
	 * Pass the (insecure) option switches from the connection request.
	 */
	StrNCpy(optbuf, port->options, ARGV_SIZE);
	split_opts(av, &ac, optbuf);

	/*
	 * Pass the (insecure) debug output file request.
	 *
	 * NOTE: currently, this is useless code, since the backend will not
	 * honor an insecure -o switch.  I left it here since the backend
	 * could be modified to allow insecure -o, given adequate checking
	 * that the specified filename is something safe to write on.
	 */
	if (port->tty[0])
	{
		StrNCpy(ttybuf, port->tty, ARGV_SIZE);
		av[ac++] = "-o";
		av[ac++] = ttybuf;
	}

	av[ac] = (char *) NULL;

	/*
	 * Release postmaster's working memory context so that backend can
	 * recycle the space.  Note this does not trash *MyProcPort, because
	 * ConnCreate() allocated that space with malloc() ... else we'd need
	 * to copy the Port data here.
	 */
	MemoryContextSwitchTo(TopMemoryContext);
	MemoryContextDelete(PostmasterContext);
	PostmasterContext = NULL;

	/*
	 * Debug: print arguments being passed to backend
	 */
	if (DebugLvl > 1)
	{
		fprintf(stderr, "%s child[%d]: starting with (",
				progname, MyProcPid);
		for (i = 0; i < ac; ++i)
			fprintf(stderr, "%s ", av[i]);
		fprintf(stderr, ")\n");
	}

	return (PostgresMain(ac, av, real_argc, real_argv, port->user));
}

/*
 * ExitPostmaster -- cleanup
 *
 * Do NOT call exit() directly --- always go through here!
 */
static void
ExitPostmaster(int status)
{
	/* should cleanup shared memory and kill all backends */

	/*
	 * Not sure of the semantics here.	When the Postmaster dies, should
	 * the backends all be killed? probably not.
	 *
	 * MUST		-- vadim 05-10-1999
	 */
	if (ServerSock_INET != INVALID_SOCK)
		StreamClose(ServerSock_INET);
	ServerSock_INET = INVALID_SOCK;
#ifdef HAVE_UNIX_SOCKETS
	if (ServerSock_UNIX != INVALID_SOCK)
		StreamClose(ServerSock_UNIX);
	ServerSock_UNIX = INVALID_SOCK;
#endif

	proc_exit(status);
}

/* Request to schedule a checkpoint (no-op if one is currently running) */
static void
schedule_checkpoint(SIGNAL_ARGS)
{
	int			save_errno = errno;

	PG_SETMASK(&BlockSig);

	/* Ignore request if checkpointing is currently disabled */
	if (CheckPointPID == 0 && checkpointed &&
		Shutdown == NoShutdown && !FatalError)
	{
		CheckPointPID = CheckPointDataBase();
		/* note: if fork fails, CheckPointPID stays 0; nothing happens */
	}

	errno = save_errno;
}


/*
 * CharRemap
 */
static char
CharRemap(long int ch)
{

	if (ch < 0)
		ch = -ch;

	ch = ch % 62;
	if (ch < 26)
		return 'A' + ch;

	ch -= 26;
	if (ch < 26)
		return 'a' + ch;

	ch -= 26;
	return '0' + ch;
}

/*
 * RandomSalt
 */
static void
RandomSalt(char *salt)
{
	long		rand = PostmasterRandom();

	*salt = CharRemap(rand % 62);
	*(salt + 1) = CharRemap(rand / 62);
}

/*
 * PostmasterRandom
 */
static long
PostmasterRandom(void)
{
	static bool initialized = false;

	if (!initialized)
	{
		Assert(random_seed != 0);
		srandom(random_seed);
		initialized = true;
	}

	return random() ^ random_seed;
}

/*
 * Count up number of child processes.
 */
static int
CountChildren(void)
{
	Dlelem	   *curr;
	Backend    *bp;
	int			mypid = getpid();
	int			cnt = 0;

	for (curr = DLGetHead(BackendList); curr; curr = DLGetSucc(curr))
	{
		bp = (Backend *) DLE_VAL(curr);
		if (bp->pid != mypid)
			cnt++;
	}
	if (CheckPointPID != 0)
		cnt--;
	return cnt;
}

#ifdef USE_SSL
/*
 * Initialize SSL library and structures
 */
static void
InitSSL(void)
{
	char		fnbuf[2048];

	SSL_load_error_strings();
	SSL_library_init();
	SSL_context = SSL_CTX_new(SSLv23_method());
	if (!SSL_context)
	{
		postmaster_error("failed to create SSL context: %s",
						 ERR_reason_error_string(ERR_get_error()));
		ExitPostmaster(1);
	}
	snprintf(fnbuf, sizeof(fnbuf), "%s/server.crt", DataDir);
	if (!SSL_CTX_use_certificate_file(SSL_context, fnbuf, SSL_FILETYPE_PEM))
	{
		postmaster_error("failed to load server certificate (%s): %s",
						 fnbuf, ERR_reason_error_string(ERR_get_error()));
		ExitPostmaster(1);
	}
	snprintf(fnbuf, sizeof(fnbuf), "%s/server.key", DataDir);
	if (!SSL_CTX_use_PrivateKey_file(SSL_context, fnbuf, SSL_FILETYPE_PEM))
	{
		postmaster_error("failed to load private key file (%s): %s",
						 fnbuf, ERR_reason_error_string(ERR_get_error()));
		ExitPostmaster(1);
	}
	if (!SSL_CTX_check_private_key(SSL_context))
	{
		postmaster_error("check of private key failed: %s",
						 ERR_reason_error_string(ERR_get_error()));
		ExitPostmaster(1);
	}
}

#endif

/*
 * Fire off a subprocess for startup/shutdown/checkpoint.
 *
 * Return value is subprocess' PID, or 0 if failed to start subprocess
 * (0 is returned only for checkpoint case).
 */
static pid_t
SSDataBase(int xlop)
{
	pid_t		pid;
	Backend    *bn;

	fflush(stdout);
	fflush(stderr);

#ifdef __BEOS__
	/* Specific beos actions before backend startup */
	beos_before_backend_startup();
#endif

	if ((pid = fork()) == 0)	/* child */
	{
		char	   *av[ARGV_SIZE * 2];
		int			ac = 0;
		char		nbbuf[ARGV_SIZE];
		char		dbbuf[ARGV_SIZE];
		char		xlbuf[ARGV_SIZE];

#ifdef __BEOS__
		/* Specific beos actions after backend startup */
		beos_backend_startup();
#endif

		/* Lose the postmaster's on-exit routines and port connections */
		on_exit_reset();

		/* Close the postmaster's sockets */
		ClosePostmasterPorts();

		/* Set up command-line arguments for subprocess */
		av[ac++] = "postgres";

		av[ac++] = "-d";

		sprintf(nbbuf, "-B%u", NBuffers);
		av[ac++] = nbbuf;

		sprintf(xlbuf, "-x %d", xlop);
		av[ac++] = xlbuf;

		av[ac++] = "-p";

		StrNCpy(dbbuf, "template1", ARGV_SIZE);
		av[ac++] = dbbuf;

		av[ac] = (char *) NULL;

		optind = 1;

		BootstrapMain(ac, av);
		ExitPostmaster(0);
	}

	/* in parent */
	if (pid < 0)
	{
#ifdef __BEOS__
		/* Specific beos actions before backend startup */
		beos_backend_startup_failed();
#endif

		fprintf(stderr, "%s Data Base: fork failed: %s\n",
				((xlop == BS_XLOG_STARTUP) ? "Startup" :
				 ((xlop == BS_XLOG_CHECKPOINT) ? "CheckPoint" :
				  "Shutdown")),
				strerror(errno));

		/*
		 * fork failure is fatal during startup/shutdown, but there's no
		 * need to choke if a routine checkpoint fails.
		 */
		if (xlop == BS_XLOG_CHECKPOINT)
			return 0;
		ExitPostmaster(1);
	}

	/*
	 * The startup and shutdown processes are not considered normal
	 * backends, but the checkpoint process is.  Checkpoint must be added
	 * to the list of backends.
	 */
	if (xlop == BS_XLOG_CHECKPOINT)
	{
		if (!(bn = (Backend *) calloc(1, sizeof(Backend))))
		{
			postmaster_error("CheckPointDataBase: malloc failed");
			ExitPostmaster(1);
		}

		bn->pid = pid;
		bn->cancel_key = PostmasterRandom();
		DLAddHead(BackendList, DLNewElem(bn));

		/*
		 * Since this code is executed periodically, it's a fine place to
		 * do other actions that should happen every now and then on no
		 * particular schedule.  Such as...
		 */
		TouchSocketLockFile();
	}

	return pid;
}


/*
 * Create the opts file
 */
static bool
CreateOptsFile(int argc, char *argv[])
{
	char		fullprogname[MAXPGPATH];
	char	   *filename;
	FILE	   *fp;
	unsigned	i;

	if (FindExec(fullprogname, argv[0], "postmaster") == -1)
		return false;

	filename = palloc(strlen(DataDir) + 20);
	sprintf(filename, "%s/postmaster.opts", DataDir);

	fp = fopen(filename, "w");
	if (fp == NULL)
	{
		postmaster_error("cannot create file %s: %s",
						 filename, strerror(errno));
		return false;
	}

	fprintf(fp, "%s", fullprogname);
	for (i = 1; i < argc; i++)
		fprintf(fp, " '%s'", argv[i]);
	fputs("\n", fp);

	if (ferror(fp))
	{
		postmaster_error("writing file %s failed", filename);
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}


static void
postmaster_error(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, gettext(fmt), ap);
	va_end(ap);
	fprintf(stderr, "\n");
}
