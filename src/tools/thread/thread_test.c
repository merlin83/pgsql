/*-------------------------------------------------------------------------
 *
 * test_thread_funcs.c
 *		libc thread test program
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *	$PostgreSQL: pgsql/src/tools/thread/thread_test.c,v 1.36 2004/12/31 22:04:02 pgsql Exp $
 *
 *	This program tests to see if your standard libc functions use
 *	pthread_setspecific()/pthread_getspecific() to be thread-safe.
 *	See src/port/thread.c for more details.
 *
 *	This program first tests to see if each function returns a constant
 *	memory pointer within the same thread, then, assuming it does, tests
 *	to see if the pointers are different for different threads.  If they
 *	are, the function is thread-safe.
 *
 *-------------------------------------------------------------------------
 */

#ifndef IN_CONFIGURE
#include "postgres.h"
#else
/* From src/include/c.h" */
#ifndef bool
typedef char bool;
#endif

#ifndef true
#define true	((bool) 1)
#endif

#ifndef false
#define false	((bool) 0)
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/* Test for POSIX.1c 2-arg sigwait() and fail on single-arg version */
#include <signal.h>
int sigwait(const sigset_t *set, int *sig);


#if !defined(ENABLE_THREAD_SAFETY) && !defined(IN_CONFIGURE)
int
main(int argc, char *argv[])
{
	fprintf(stderr, "This PostgreSQL build does not support threads.\n");
	fprintf(stderr, "Perhaps rerun 'configure' using '--enable-thread-safety'.\n");
	return 1;
}

#else

/* This must be down here because this is the code that uses threads. */
#include <pthread.h>

void		func_call_1(void);
void		func_call_2(void);

#define		TEMP_FILENAME_1 "/tmp/thread_test.1.XXXXXX"
#define		TEMP_FILENAME_2 "/tmp/thread_test.2.XXXXXX"

char	   *temp_filename_1;
char	   *temp_filename_2;

pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile int thread1_done = 0;
volatile int thread2_done = 0;

volatile int errno1_set = 0;
volatile int errno2_set = 0;

#ifndef HAVE_STRERROR_R
char	   *strerror_p1;
char	   *strerror_p2;
bool		strerror_threadsafe = false;
#endif

#ifndef HAVE_GETPWUID_R
struct passwd *passwd_p1;
struct passwd *passwd_p2;
bool		getpwuid_threadsafe = false;
#endif

#if !defined(HAVE_GETADDRINFO) && !defined(HAVE_GETHOSTBYNAME_R)
struct hostent *hostent_p1;
struct hostent *hostent_p2;
char		myhostname[MAXHOSTNAMELEN];
bool		gethostbyname_threadsafe = false;
#endif

bool		platform_is_threadsafe = true;

int
main(int argc, char *argv[])
{
	pthread_t	thread1,
				thread2;
	int			fd;

	if (argc > 1)
	{
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return 1;
	}

#ifdef IN_CONFIGURE
	/* Send stdout to 'config.log' */
	close(1);
	dup(5);
#endif

	/* Make temp filenames, might not have strdup() */
	temp_filename_1 = malloc(strlen(TEMP_FILENAME_1) + 1);
	strcpy(temp_filename_1, TEMP_FILENAME_1);
	fd = mkstemp(temp_filename_1);
	close(fd);

	temp_filename_2 = malloc(strlen(TEMP_FILENAME_2) + 1);
	strcpy(temp_filename_2, TEMP_FILENAME_2);
	fd = mkstemp(temp_filename_2);
	close(fd);

#if !defined(HAVE_GETADDRINFO) && !defined(HAVE_GETHOSTBYNAME_R)
	if (gethostname(myhostname, MAXHOSTNAMELEN) != 0)
	{
		fprintf(stderr, "Can not get local hostname **\nexiting\n");
		exit(1);
	}
#endif

	/* Hold lock until we are ready for the child threads to exit. */
	pthread_mutex_lock(&init_mutex);

	pthread_create(&thread1, NULL, (void *(*) (void *)) func_call_1, NULL);
	pthread_create(&thread2, NULL, (void *(*) (void *)) func_call_2, NULL);

	while (thread1_done == 0 || thread2_done == 0)
		sched_yield();			/* if this is a portability problem,
								 * remove it */

	printf("Your errno is thread-safe.\n");

#ifndef HAVE_STRERROR_R
	if (strerror_p1 != strerror_p2)
		strerror_threadsafe = true;
#endif

#ifndef HAVE_GETPWUID_R
	if (passwd_p1 != passwd_p2)
		getpwuid_threadsafe = true;
#endif

#if !defined(HAVE_GETADDRINFO) && !defined(HAVE_GETHOSTBYNAME_R)
	if (hostent_p1 != hostent_p2)
		gethostbyname_threadsafe = true;
#endif

	pthread_mutex_unlock(&init_mutex);	/* let children exit  */

	pthread_join(thread1, NULL);	/* clean up children */
	pthread_join(thread2, NULL);

#ifdef HAVE_STRERROR_R
	printf("Your system has sterror_r();  it does not need strerror().\n");
#else
	printf("Your system uses strerror() which is ");
	if (strerror_threadsafe)
		printf("thread-safe.\n");
	else
	{
		printf("not thread-safe. **\n");
		platform_is_threadsafe = false;
	}
#endif

#ifdef HAVE_GETPWUID_R
	printf("Your system has getpwuid_r();  it does not need getpwuid().\n");
#else
	printf("Your system uses getpwuid() which is ");
	if (getpwuid_threadsafe)
		printf("thread-safe.\n");
	else
	{
		printf("not thread-safe. **\n");
		platform_is_threadsafe = false;
	}
#endif

#ifdef HAVE_GETADDRINFO
	printf("Your system has getaddrinfo();  it does not need gethostbyname()\n"
		   "  or gethostbyname_r().\n");
#else
#ifdef HAVE_GETHOSTBYNAME_R
	printf("Your system has gethostbyname_r();  it does not need gethostbyname().\n");
#else
	printf("Your system uses gethostbyname which is ");
	if (gethostbyname_threadsafe)
		printf("thread-safe.\n");
	else
	{
		printf("not thread-safe. **\n");
		platform_is_threadsafe = false;
	}
#endif
#endif

	if (platform_is_threadsafe)
	{
		printf("\nYour platform is thread-safe.\n");
		return 0;
	}
	else
	{
		printf("\n** YOUR PLATFORM IS NOT THREAD-SAFE. **\n");
		return 1;
	}
}

void
func_call_1(void)
{
#if !defined(HAVE_GETPWUID_R) || \
	(!defined(HAVE_GETADDRINFO) && \
	 !defined(HAVE_GETHOSTBYNAME_R))
	void	   *p;
#endif

	unlink(temp_filename_1);
	/* create, then try to fail on exclusive create open */
	if (open(temp_filename_1, O_RDWR | O_CREAT, 0600) < 0 ||
		open(temp_filename_1, O_RDWR | O_CREAT | O_EXCL, 0600) >= 0)
	{
		fprintf(stderr, "Could not create file in /tmp or\n");
		fprintf(stderr, "Could not generate failure for create file in /tmp **\nexiting\n");
		exit(1);
	}

	/*
	 * Wait for other thread to set errno. We can't use thread-specific
	 * locking here because it might affect errno.
	 */
	errno1_set = 1;
	while (errno2_set == 0)
		sched_yield();
	if (errno != EEXIST)
	{
		fprintf(stderr, "errno not thread-safe **\nexiting\n");
		unlink(temp_filename_1);
		exit(1);
	}
	unlink(temp_filename_1);

#ifndef HAVE_STRERROR_R
	strerror_p1 = strerror(EACCES);

	/*
	 * If strerror() uses sys_errlist, the pointer might change for
	 * different errno values, so we don't check to see if it varies
	 * within the thread.
	 */
#endif

#ifndef HAVE_GETPWUID_R
	passwd_p1 = getpwuid(0);
	p = getpwuid(1);
	if (passwd_p1 != p)
	{
		printf("Your getpwuid() changes the static memory area between calls\n");
		passwd_p1 = NULL;		/* force thread-safe failure report */
	}
#endif

#if !defined(HAVE_GETADDRINFO) && !defined(HAVE_GETHOSTBYNAME_R)
	/* threads do this in opposite order */
	hostent_p1 = gethostbyname(myhostname);
	p = gethostbyname("localhost");
	if (hostent_p1 != p)
	{
		printf("Your gethostbyname() changes the static memory area between calls\n");
		hostent_p1 = NULL;		/* force thread-safe failure report */
	}
#endif

	thread1_done = 1;
	pthread_mutex_lock(&init_mutex);	/* wait for parent to test */
	pthread_mutex_unlock(&init_mutex);
}


void
func_call_2(void)
{
#if !defined(HAVE_GETPWUID_R) || \
	(!defined(HAVE_GETADDRINFO) && \
	 !defined(HAVE_GETHOSTBYNAME_R))
	void	   *p;
#endif

	unlink(temp_filename_2);
	/* open non-existant file */
	if (open(temp_filename_2, O_RDONLY, 0600) >= 0)
	{
		fprintf(stderr, "Read-only open succeeded without create **\nexiting\n");
		exit(1);
	}

	/*
	 * Wait for other thread to set errno. We can't use thread-specific
	 * locking here because it might affect errno.
	 */
	errno2_set = 1;
	while (errno1_set == 0)
		sched_yield();
	if (errno != ENOENT)
	{
		fprintf(stderr, "errno not thread-safe **\nexiting\n");
		unlink(temp_filename_2);
		exit(1);
	}
	unlink(temp_filename_2);

#ifndef HAVE_STRERROR_R
	strerror_p2 = strerror(EINVAL);

	/*
	 * If strerror() uses sys_errlist, the pointer might change for
	 * different errno values, so we don't check to see if it varies
	 * within the thread.
	 */
#endif

#ifndef HAVE_GETPWUID_R
	passwd_p2 = getpwuid(2);
	p = getpwuid(3);
	if (passwd_p2 != p)
	{
		printf("Your getpwuid() changes the static memory area between calls\n");
		passwd_p2 = NULL;		/* force thread-safe failure report */
	}
#endif

#if !defined(HAVE_GETADDRINFO) && !defined(HAVE_GETHOSTBYNAME_R)
	/* threads do this in opposite order */
	hostent_p2 = gethostbyname("localhost");
	p = gethostbyname(myhostname);
	if (hostent_p2 != p)
	{
		printf("Your gethostbyname() changes the static memory area between calls\n");
		hostent_p2 = NULL;		/* force thread-safe failure report */
	}
#endif

	thread2_done = 1;
	pthread_mutex_lock(&init_mutex);	/* wait for parent to test */
	pthread_mutex_unlock(&init_mutex);
}

#endif   /* !ENABLE_THREAD_SAFETY && !IN_CONFIGURE */
