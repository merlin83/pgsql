/*
 * These routines were taken from the Apache source, but were made
 * available with a PostgreSQL-compatible license.  Kudos Wilfredo
 * S�nchez <wsanchez@apple.com>.
 *
 * $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/port/dynloader/darwin.c,v 1.4 2000-12-11 00:49:54 tgl Exp $
 */

#include "postgres.h"
#include <mach-o/dyld.h>
#include "dynloader.h"

void *pg_dlopen(char *filename)
{
	NSObjectFileImage image;

	if (NSCreateObjectFileImageFromFile(filename, &image) !=
		NSObjectFileImageSuccess)
		return NULL;
	return NSLinkModule(image, filename, TRUE);
}

void pg_dlclose(void *handle)
{
	NSUnLinkModule(handle,FALSE);
	return;
}

PGFunction pg_dlsym(void *handle, char *funcname)
{
	NSSymbol symbol;
	char *symname = (char*)malloc(strlen(funcname)+2);

	sprintf(symname, "_%s", funcname);
	if (NSIsSymbolNameDefined(symname))
	{
		symbol = NSLookupAndBindSymbol(symname);
		free(symname);
		return (PGFunction) NSAddressOfSymbol(symbol);
	}
	else
	{
		free(symname);
		return (PGFunction)NULL;
	}
}

char *pg_dlerror(void)
{
	return "no error message available";
}
