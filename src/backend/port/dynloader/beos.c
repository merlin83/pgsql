/*-------------------------------------------------------------------------
 *
 * dynloader.c
 *	  Dynamic Loader for Postgres for BeOS
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/port/dynloader/Attic/beos.c,v 1.3 2000-10-07 14:39:11 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "utils/dynamic_loader.h"
#include "utils/elog.h"


void	   *
pg_dlopen(char *filename)
{
	image_id* im; 
	
	/* Handle memory allocation to store the Id of the shared object*/
	im=(image_id*)(malloc(sizeof(image_id)));
	
	/* Add-on loading */
	*im=beos_dl_open(filename);
			
	return im;
}


char	   *
pg_dlerror()
{
	static char errmsg[] = "Load Add-On failed";
	return errmsg;
}

PGFunction 
pg_dlsym(void *handle, char *funcname)
{
	PGFunction fpt;

	/* Checking that "Handle" is valid */
	if ((handle) && ((*(int*)(handle))>=0))
	{
		/* Loading symbol */
		if(get_image_symbol(*((int*)(handle)),funcname,B_SYMBOL_TYPE_TEXT,(void**)&fpt)==B_OK);
		{
			return fpt;
		}
		elog(NOTICE, "loading symbol '%s' failed ",funcname);
	}
	elog(NOTICE, "add-on not loaded correctly");
	return NULL;
}

void 
pg_dlclose(void *handle)
{
	/* Checking that "Handle" is valid */
	if ((handle) && ((*(int*)(handle))>=0))
	{
		if (beos_dl_close(*(image_id*)handle)!=B_OK)
			elog(NOTICE, "error while unloading add-on");
		free(handle);
	}
}