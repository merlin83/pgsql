/* $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/include/port/cygwin.h,v 1.3 2003-05-22 17:20:28 petere Exp $ */

#define HAS_TEST_AND_SET
typedef unsigned char slock_t;

#include <cygwin/version.h>

/*
 * Check for b20.1 and disable AF_UNIX family socket support.
 */
#if CYGWIN_VERSION_DLL_MAJOR < 1001
#undef HAVE_UNIX_SOCKETS
#endif

#if __GNUC__ && ! defined (__declspec)
#error You need egcs 1.1 or newer for compiling!
#endif

#ifdef BUILDING_DLL
#define DLLIMPORT __declspec (dllexport)
#else
#define DLLIMPORT __declspec (dllimport)
#endif
