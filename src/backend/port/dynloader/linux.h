/*-------------------------------------------------------------------------
 *
 * port-protos.h--
 *	  port-specific prototypes for Linux
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: linux.h,v 1.3 1998-02-26 04:34:28 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef PORT_PROTOS_H
#define PORT_PROTOS_H

#include "fmgr.h"				/* for func_ptr */
#include "utils/dynamic_loader.h"
#ifdef __ELF__
#include "dlfcn.h"
#endif

/* dynloader.c */

#ifndef __ELF__
#ifndef HAVE_DLD_H
#define pg_dlsym(handle, funcname)		(NULL)
#define pg_dlclose(handle)			   ({})
#else
#define pg_dlsym(handle, funcname)		((func_ptr) dld_get_func((funcname)))
#define pg_dlclose(handle)			   ({ dld_unlink_by_file(handle, 1); free(handle); })
#endif
#else
/* #define		pg_dlopen(f)	dlopen(f, 1) */
#define pg_dlopen(f)	dlopen(f, 2)
#define pg_dlsym		dlsym
#define pg_dlclose		dlclose
#define pg_dlerror		dlerror
#endif

/* port.c */

#endif							/* PORT_PROTOS_H */
