/*-------------------------------------------------------------------------
 *
 * port_protos.h
 *	  port-specific prototypes for SunOS 4
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: solaris_sparc.h,v 1.3 1999-02-13 23:17:26 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PORT_PROTOS_H
#define PORT_PROTOS_H

#include <netinet/in.h>			/* For struct in_addr */
#include <arpa/inet.h>

#include <dlfcn.h>

#include "fmgr.h"				/* for func_ptr */
#include "utils/dynamic_loader.h"

/* dynloader.c */
/*
 * Dynamic Loader on SunOS 4.
 *
 * this dynamic loader uses the system dynamic loading interface for shared
 * libraries (ie. dlopen/dlsym/dlclose). The user must specify a shared
 * library as the file to be dynamically loaded.
 *
 */
#define pg_dlopen(f)	dlopen(f,1)
#define pg_dlsym		dlsym
#define pg_dlclose		dlclose
#define pg_dlerror		dlerror

#endif	 /* PORT_PROTOS_H */
