/*-------------------------------------------------------------------------
 *
 * dfmgr.c
 *	  Dynamic function manager code.
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/fmgr/dfmgr.c,v 1.36 1999-11-07 23:08:27 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <sys/types.h>
#include <sys/stat.h>

#include "postgres.h"

#include "utils/dynamic_loader.h"

#include "access/heapam.h"
#include "catalog/catname.h"
#include "catalog/pg_proc.h"
#include "dynloader.h"
#include "utils/builtins.h"
#include "utils/syscache.h"

static DynamicFileList *file_list = (DynamicFileList *) NULL;
static DynamicFileList *file_tail = (DynamicFileList *) NULL;

#define NOT_EQUAL(A, B) (((A).st_ino != (B).inode) \
					  || ((A).st_dev != (B).device))

static Oid	procedureId_save = -1;
static int	pronargs_save;
static func_ptr user_fn_save = (func_ptr) NULL;
static func_ptr handle_load(char *filename, char *funcname);

func_ptr
fmgr_dynamic(Oid procedureId, int *pronargs)
{
	HeapTuple	procedureTuple;
	Form_pg_proc procedureStruct;
	char	   *proname,
			   *linksymbol,
			   *probinstring;
	char	   *prosrcstring = NULL;
	Datum		probinattr;
	Datum		prosrcattr;
	func_ptr	user_fn;
	Relation	rel;
	bool		isnull;

	/* Implement simple one-element cache for function lookups */
	if (procedureId == procedureId_save)
	{
		*pronargs = pronargs_save;
		return user_fn_save;
	}

	/*
	 * The procedure isn't a builtin, so we'll have to do a catalog lookup
	 * to find its pg_proc entry.  Moreover, since probin is varlena, we're
	 * going to have to use heap_getattr, which means we need the reldesc,
	 * which means we need to open the relation.  So we might as well do that
	 * first and get the benefit of SI inval if needed.
	 */
	rel = heap_openr(ProcedureRelationName, AccessShareLock);

	procedureTuple = SearchSysCacheTuple(PROOID,
										 ObjectIdGetDatum(procedureId),
										 0, 0, 0);
	if (!HeapTupleIsValid(procedureTuple))
	{
		elog(ERROR, "fmgr: Cache lookup failed for procedure %u\n",
			 procedureId);
		return (func_ptr) NULL;
	}

	procedureStruct = (Form_pg_proc) GETSTRUCT(procedureTuple);
	proname = NameStr(procedureStruct->proname);
	pronargs_save = *pronargs = procedureStruct->pronargs;
	probinattr = heap_getattr(procedureTuple,
							  Anum_pg_proc_probin,
							  RelationGetDescr(rel), &isnull);
	if (!PointerIsValid(probinattr) /* || isnull */ )
	{
		heap_close(rel, AccessShareLock);
		elog(ERROR, "fmgr: Could not extract probin for %u from %s",
			 procedureId, ProcedureRelationName);
		return (func_ptr) NULL;
	}
	probinstring = textout((struct varlena *) probinattr);

	prosrcattr = heap_getattr(procedureTuple,
							  Anum_pg_proc_prosrc,
							  RelationGetDescr(rel), &isnull);

	if (isnull)
	{							/* Use the proname for the link symbol */
		linksymbol = proname;
	}
	else if (!PointerIsValid(prosrcattr))
	{							/* pg_proc must be messed up! */
		heap_close(rel, AccessShareLock);
		elog(ERROR, "fmgr: Could not extract prosrc for %u from %s",
			 procedureId, ProcedureRelationName);
		return (func_ptr) NULL;
	}
	else
	{							/* The text in prosrcattr is either "-" or
								 * a link symbol */
		prosrcstring = textout((struct varlena *) prosrcattr);
		if (strcmp(prosrcstring, "-") == 0)
			linksymbol = proname;
		else
			linksymbol = prosrcstring;
	}

	heap_close(rel, AccessShareLock);

	user_fn = handle_load(probinstring, linksymbol);

	pfree(probinstring);
	if (prosrcstring) pfree(prosrcstring);

	procedureId_save = procedureId;
	user_fn_save = user_fn;

	return user_fn;
}

static func_ptr
handle_load(char *filename, char *funcname)
{
	DynamicFileList *file_scanner = (DynamicFileList *) NULL;
	func_ptr	retval = (func_ptr) NULL;
	char	   *load_error;
	struct stat stat_buf;

	/*
	 * Do this because loading files may screw up the dynamic function
	 * manager otherwise.
	 */
	procedureId_save = -1;

	/*
	 * Scan the list of loaded FILES to see if the function has been
	 * loaded.
	 */

	if (filename != (char *) NULL)
	{
		for (file_scanner = file_list;
			 file_scanner != (DynamicFileList *) NULL
			 && file_scanner->filename != (char *) NULL
			 && strcmp(filename, file_scanner->filename) != 0;
			 file_scanner = file_scanner->next)
			;
		if (file_scanner == (DynamicFileList *) NULL)
		{
			if (stat(filename, &stat_buf) == -1)
				elog(ERROR, "stat failed on file '%s': %m", filename);

			for (file_scanner = file_list;
				 file_scanner != (DynamicFileList *) NULL
				 && (NOT_EQUAL(stat_buf, *file_scanner));
				 file_scanner = file_scanner->next)
				;

			/*
			 * Same files - different paths (ie, symlink or link)
			 */
			if (file_scanner != (DynamicFileList *) NULL)
				strcpy(file_scanner->filename, filename);

		}
	}
	else
		file_scanner = (DynamicFileList *) NULL;

	/*
	 * File not loaded yet.
	 */

	if (file_scanner == (DynamicFileList *) NULL)
	{
		if (file_list == (DynamicFileList *) NULL)
		{
			file_list = (DynamicFileList *)
				malloc(sizeof(DynamicFileList));
			file_scanner = file_list;
		}
		else
		{
			file_tail->next = (DynamicFileList *)
				malloc(sizeof(DynamicFileList));
			file_scanner = file_tail->next;
		}
		MemSet((char *) file_scanner, 0, sizeof(DynamicFileList));

		strcpy(file_scanner->filename, filename);
		file_scanner->device = stat_buf.st_dev;
		file_scanner->inode = stat_buf.st_ino;
		file_scanner->next = (DynamicFileList *) NULL;

		file_scanner->handle = pg_dlopen(filename);
		if (file_scanner->handle == (void *) NULL)
		{
			load_error = (char *) pg_dlerror();
			if (file_scanner == file_list)
				file_list = (DynamicFileList *) NULL;
			else
				file_tail->next = (DynamicFileList *) NULL;

			free((char *) file_scanner);
			elog(ERROR, "Load of file %s failed: %s", filename, load_error);
		}

		/*
		 * Just load the file - we are done with that so return.
		 */
		file_tail = file_scanner;

		if (funcname == (char *) NULL)
			return (func_ptr) NULL;
	}

	retval = (func_ptr) pg_dlsym(file_scanner->handle, funcname);

	if (retval == (func_ptr) NULL)
		elog(ERROR, "Can't find function %s in file %s", funcname, filename);

	return retval;
}

/*
 * This function loads files by the following:
 *
 * If the file is already loaded:
 * o  Zero out that file's loaded space (so it doesn't screw up linking)
 * o  Free all space associated with that file
 * o  Free that file's descriptor.
 *
 * Now load the file by calling handle_load with a NULL argument as the
 * function.
 */
void
load_file(char *filename)
{
	DynamicFileList *file_scanner,
			   *p;
	struct stat stat_buf;
	int			done = 0;

	/*
	 * We need to do stat() in order to determine whether this is the same
	 * file as a previously loaded file; it's also handy so as to give a
	 * good error message if bogus file name given.
	 */
	if (stat(filename, &stat_buf) == -1)
		elog(ERROR, "LOAD: could not open file '%s': %m", filename);

	if (file_list != (DynamicFileList *) NULL
		&& !NOT_EQUAL(stat_buf, *file_list))
	{
		file_scanner = file_list;
		file_list = file_list->next;
		pg_dlclose(file_scanner->handle);
		free((char *) file_scanner);
	}
	else if (file_list != (DynamicFileList *) NULL)
	{
		file_scanner = file_list;
		while (!done)
		{
			if (file_scanner->next == (DynamicFileList *) NULL)
				done = 1;
			else if (!NOT_EQUAL(stat_buf, *(file_scanner->next)))
				done = 1;
			else
				file_scanner = file_scanner->next;
		}

		if (file_scanner->next != (DynamicFileList *) NULL)
		{
			p = file_scanner->next;
			file_scanner->next = file_scanner->next->next;
			pg_dlclose(file_scanner->handle);
			free((char *) p);
		}
	}
	handle_load(filename, (char *) NULL);
}

/* Is this used? bjm 1998/10/08   No. tgl 1999/02/07 */
#ifdef NOT_USED
func_ptr
trigger_dynamic(char *filename, char *funcname)
{
	func_ptr	trigger_fn;

	trigger_fn = handle_load(filename, funcname);

	return trigger_fn;
}

#endif
