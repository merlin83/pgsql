/*-------------------------------------------------------------------------
 *
 * dynloader.h
 *	  port-specific prototypes for Intel x86/Intel SVR4
 *
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: svr4.h,v 1.10 2001-10-28 06:25:47 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef DYNLOADER_H
#define DYNLOADER_H

#include <dlfcn.h>
#include "utils/dynamic_loader.h"

/* dynloader.h */
/*
 * Dynamic Loader on Intel x86/Intel SVR4.
 *
 * this dynamic loader uses the system dynamic loading interface for shared
 * libraries (ie. dlopen/dlsym/dlclose). The user must specify a shared
 * library as the file to be dynamically loaded.
 *
 */
#define pg_dlopen(f)	dlopen((f), RTLD_LAZY | RTLD_GLOBAL)
#define pg_dlsym		dlsym
#define pg_dlclose		dlclose
#define pg_dlerror		dlerror

#endif	 /* DYNLOADER_H */
