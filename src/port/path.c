/*-------------------------------------------------------------------------
 *
 * path.c
 *	  portable path handling routines
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/port/path.c,v 1.50.4.1 2005/01/26 19:24:21 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#include "c.h"

#include <ctype.h>
#include <sys/stat.h>
#ifdef WIN32
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0500
#ifdef near
#undef near
#endif
#define near
#include <shlobj.h>
#else
#include <unistd.h>
#endif

#include "pg_config_paths.h"


#ifndef WIN32
#define IS_DIR_SEP(ch)	((ch) == '/')
#else
#define IS_DIR_SEP(ch)	((ch) == '/' || (ch) == '\\')
#endif

#ifndef WIN32
#define IS_PATH_SEP(ch) ((ch) == ':')
#else
#define IS_PATH_SEP(ch) ((ch) == ';')
#endif

static void make_relative_path(char *ret_path, const char *target_path,
							   const char *bin_path, const char *my_exec_path);
static void trim_directory(char *path);
static void trim_trailing_separator(char *path);


/*
 * skip_drive
 *
 * On Windows, a path may begin with "C:" or "//network/".  Advance over
 * this and point to the effective start of the path.
 */
#ifdef WIN32

static char *
skip_drive(const char *path)
{
	if (IS_DIR_SEP(path[0]) && IS_DIR_SEP(path[1]))
	{
		path += 2;
		while (*path && !IS_DIR_SEP(*path))
			path++;
	}
	else if (isalpha(path[0]) && path[1] == ':')
	{
		path += 2;
	}
	return (char *) path;
}

#else

#define skip_drive(path)	(path)

#endif

/*
 *	first_dir_separator
 *
 * Find the location of the first directory separator, return
 * NULL if not found.
 */
char *
first_dir_separator(const char *filename)
{
	const char *p;

	for (p = skip_drive(filename); *p; p++)
		if (IS_DIR_SEP(*p))
			return (char *) p;
	return NULL;
}

/*
 *	first_path_separator
 *
 * Find the location of the first path separator (i.e. ':' on
 * Unix, ';' on Windows), return NULL if not found.
 */
char *
first_path_separator(const char *pathlist)
{
	const char *p;

	/* skip_drive is not needed */
	for (p = pathlist; *p; p++)
		if (IS_PATH_SEP(*p))
			return (char *) p;
	return NULL;
}

/*
 *	last_dir_separator
 *
 * Find the location of the last directory separator, return
 * NULL if not found.
 */
char *
last_dir_separator(const char *filename)
{
	const char *p,
			   *ret = NULL;

	for (p = skip_drive(filename); *p; p++)
		if (IS_DIR_SEP(*p))
			ret = p;
	return (char *) ret;
}


/*
 *	make_native_path - on WIN32, change / to \ in the path
 *
 *	This effectively undoes canonicalize_path.
 *
 *	This is required because WIN32 COPY is an internal CMD.EXE
 *	command and doesn't process forward slashes in the same way
 *	as external commands.  Quoting the first argument to COPY
 *	does not convert forward to backward slashes, but COPY does
 *	properly process quoted forward slashes in the second argument.
 *
 *	COPY works with quoted forward slashes in the first argument
 *	only if the current directory is the same as the directory
 *	of the first argument.
 */
void
make_native_path(char *filename)
{
#ifdef WIN32
	char	   *p;

	for (p = filename; *p; p++)
		if (*p == '/')
			*p = '\\';
#endif
}


/*
 * join_path_components - join two path components, inserting a slash
 *
 * ret_path is the output area (must be of size MAXPGPATH)
 *
 * ret_path can be the same as head, but not the same as tail.
 */
void
join_path_components(char *ret_path,
					 const char *head, const char *tail)
{
	if (ret_path != head)
		StrNCpy(ret_path, head, MAXPGPATH);
	/*
	 * Remove any leading "." and ".." in the tail component,
	 * adjusting head as needed.
	 */
	for (;;)
	{
		if (tail[0] == '.' && IS_DIR_SEP(tail[1]))
		{
			tail += 2;
		}
		else if (tail[0] == '.' && tail[1] == '\0')
		{
			tail += 1;
			break;
		}
		else if (tail[0] == '.' && tail[1] == '.' && IS_DIR_SEP(tail[2]))
		{
			trim_directory(ret_path);
			tail += 3;
		}
		else if (tail[0] == '.' && tail[1] == '.' && tail[2] == '\0')
		{
			trim_directory(ret_path);
			tail += 2;
			break;
		}
		else
			break;
	}
	if (*tail)
		snprintf(ret_path + strlen(ret_path), MAXPGPATH - strlen(ret_path),
				 "/%s", tail);
}


/*
 *	Clean up path by:
 *		o  make Win32 path use Unix slashes
 *		o  remove trailing quote on Win32
 *		o  remove trailing slash
 *		o  remove duplicate adjacent separators
 *		o  remove trailing '.'
 *		o  process trailing '..' ourselves
 */
void
canonicalize_path(char *path)
{
	char	   *p, *to_p;
	bool		was_sep = false;

#ifdef WIN32
	/*
	 * The Windows command processor will accept suitably quoted paths
	 * with forward slashes, but barfs badly with mixed forward and back
	 * slashes.
	 */
	for (p = path; *p; p++)
	{
		if (*p == '\\')
			*p = '/';
	}

	/*
	 * In Win32, if you do: prog.exe "a b" "\c\d\" the system will pass
	 * \c\d" as argv[2], so trim off trailing quote.
	 */
	if (p > path && *(p - 1) == '"')
		*(p - 1) = '/';
#endif

	/*
	 * Removing the trailing slash on a path means we never get ugly
	 * double trailing slashes.	Also, Win32 can't stat() a directory
	 * with a trailing slash. Don't remove a leading slash, though.
	 */
	trim_trailing_separator(path);

	/*
	 *	Remove duplicate adjacent separators
	 */
	p = path;
#ifdef WIN32
	/* Don't remove leading double-slash on Win32 */
	if (*p)
		p++;
#endif
	to_p = p;
	for (; *p; p++, to_p++)
	{
		/* Handle many adjacent slashes, like "/a///b" */
		while (*p == '/' && was_sep)
			p++;
		if (to_p != p)
			*to_p = *p;
		was_sep = (*p == '/');
	}
	*to_p = '\0';

	/*
	 * Remove any trailing uses of "." and process ".." ourselves
	 */
	for (;;)
	{
		int			len = strlen(path);

		if (len > 2 && strcmp(path + len - 2, "/.") == 0)
			trim_directory(path);
		else if (len > 3 && strcmp(path + len - 3, "/..") == 0)
		{
			trim_directory(path);
			trim_directory(path);	/* remove directory above */
		}
		else
			break;
	}
}


/*
 * Extracts the actual name of the program as called - 
 * stripped of .exe suffix if any
 */
const char *
get_progname(const char *argv0)
{
	const char *nodir_name;

	nodir_name = last_dir_separator(argv0);
	if (nodir_name)
		nodir_name++;
	else
		nodir_name = skip_drive(argv0);

#if defined(__CYGWIN__) || defined(WIN32)
	/* strip .exe suffix, regardless of case */
	if (strlen(nodir_name) > sizeof(EXE) - 1 &&
		pg_strcasecmp(nodir_name + strlen(nodir_name)-(sizeof(EXE)-1), EXE) == 0)
	{
		char *progname;

		progname = strdup(nodir_name);
		if (progname == NULL)
		{
			fprintf(stderr, "%s: out of memory\n", nodir_name);
			exit(1);	/* This could exit the postmaster */
		}
		progname[strlen(progname) - (sizeof(EXE) - 1)] = '\0';
		nodir_name = progname; 
	}
#endif

	return nodir_name;
}


/*
 * make_relative_path - make a path relative to the actual binary location
 *
 * This function exists to support relocation of installation trees.
 *
 *	ret_path is the output area (must be of size MAXPGPATH)
 *	target_path is the compiled-in path to the directory we want to find
 *	bin_path is the compiled-in path to the directory of executables
 *	my_exec_path is the actual location of my executable
 *
 * If target_path matches bin_path up to the last directory component of
 * bin_path, then we build the result as my_exec_path (less the executable
 * name and last directory) joined to the non-matching part of target_path.
 * Otherwise, we return target_path as-is.
 * 
 * For example:
 *		target_path  = '/usr/local/share/postgresql'
 *		bin_path     = '/usr/local/bin'
 *		my_exec_path = '/opt/pgsql/bin/postmaster'
 * Given these inputs we would return '/opt/pgsql/share/postgresql'
 */
static void
make_relative_path(char *ret_path, const char *target_path,
				   const char *bin_path, const char *my_exec_path)
{
	const char *bin_end;
	int			prefix_len;

	bin_end = last_dir_separator(bin_path);
	if (!bin_end)
		goto no_match;
	prefix_len = bin_end - bin_path + 1;
	if (strncmp(target_path, bin_path, prefix_len) != 0)
		goto no_match;

	StrNCpy(ret_path, my_exec_path, MAXPGPATH);
	trim_directory(ret_path);	/* remove my executable name */
	trim_directory(ret_path);	/* remove last directory component (/bin) */
	join_path_components(ret_path, ret_path, target_path + prefix_len);
	canonicalize_path(ret_path);
	return;

no_match:
	StrNCpy(ret_path, target_path, MAXPGPATH);
	canonicalize_path(ret_path);
}


/*
 *	get_share_path
 */
void
get_share_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, PGSHAREDIR, PGBINDIR, my_exec_path);
}

/*
 *	get_etc_path
 */
void
get_etc_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, SYSCONFDIR, PGBINDIR, my_exec_path);
}

/*
 *	get_include_path
 */
void
get_include_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, INCLUDEDIR, PGBINDIR, my_exec_path);
}

/*
 *	get_pkginclude_path
 */
void
get_pkginclude_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, PKGINCLUDEDIR, PGBINDIR, my_exec_path);
}

/*
 *	get_includeserver_path
 */
void
get_includeserver_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, INCLUDEDIRSERVER, PGBINDIR, my_exec_path);
}

/*
 *	get_lib_path
 */
void
get_lib_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, LIBDIR, PGBINDIR, my_exec_path);
}

/*
 *	get_pkglib_path
 */
void
get_pkglib_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, PKGLIBDIR, PGBINDIR, my_exec_path);
}

/*
 *	get_locale_path
 */
void
get_locale_path(const char *my_exec_path, char *ret_path)
{
	make_relative_path(ret_path, LOCALEDIR, PGBINDIR, my_exec_path);
}


/*
 *	get_home_path
 *
 * On Unix, this actually returns the user's home directory.  On Windows
 * it returns the PostgreSQL-specific application data folder.
 */
bool
get_home_path(char *ret_path)
{
#ifndef WIN32
	char		pwdbuf[BUFSIZ];
	struct passwd pwdstr;
	struct passwd *pwd = NULL;

	if (pqGetpwuid(geteuid(), &pwdstr, pwdbuf, sizeof(pwdbuf), &pwd) != 0)
		return false;
	StrNCpy(ret_path, pwd->pw_dir, MAXPGPATH);
	return true;

#else
	char		tmppath[MAX_PATH];

	ZeroMemory(tmppath, sizeof(tmppath));
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, tmppath) != S_OK)
		return false;
	snprintf(ret_path, MAXPGPATH, "%s/postgresql", tmppath);
	return true;
#endif
}


/*
 * get_parent_directory
 *
 * Modify the given string in-place to name the parent directory of the
 * named file.
 */
void
get_parent_directory(char *path)
{
	trim_directory(path);
}


/*
 *	set_pglocale_pgservice
 *
 *	Set application-specific locale and service directory
 *
 *	This function takes an argv[0] rather than a full path.
 */
void
set_pglocale_pgservice(const char *argv0, const char *app)
{
	char		path[MAXPGPATH];
	char		my_exec_path[MAXPGPATH];
	char		env_path[MAXPGPATH + sizeof("PGSYSCONFDIR=")];	/* longer than
																 * PGLOCALEDIR */

	/* don't set LC_ALL in the backend */
	if (strcmp(app, "postgres") != 0)
		setlocale(LC_ALL, "");

	if (find_my_exec(argv0, my_exec_path) < 0)
		return;

#ifdef ENABLE_NLS
	get_locale_path(my_exec_path, path);
	bindtextdomain(app, path);
	textdomain(app);

	if (getenv("PGLOCALEDIR") == NULL)
	{
		/* set for libpq to use */
		snprintf(env_path, sizeof(env_path), "PGLOCALEDIR=%s", path);
		canonicalize_path(env_path + 12);
		putenv(strdup(env_path));
	}
#endif

	if (getenv("PGSYSCONFDIR") == NULL)
	{
		get_etc_path(my_exec_path, path);

		/* set for libpq to use */
		snprintf(env_path, sizeof(env_path), "PGSYSCONFDIR=%s", path);
		canonicalize_path(env_path + 13);
		putenv(strdup(env_path));
	}
}


/*
 *	trim_directory
 *
 *	Trim trailing directory from path, that is, remove any trailing slashes,
 *	the last pathname component, and the slash just ahead of it --- but never
 *	remove a leading slash.
 */
static void
trim_directory(char *path)
{
	char	   *p;

	path = skip_drive(path);

	if (path[0] == '\0')
		return;

	/* back up over trailing slash(es) */
	for (p = path + strlen(path) - 1; IS_DIR_SEP(*p) && p > path; p--)
		;
	/* back up over directory name */
	for (; !IS_DIR_SEP(*p) && p > path; p--)
		;
	/* if multiple slashes before directory name, remove 'em all */
	for (; p > path && IS_DIR_SEP(*(p - 1)); p--)
		;
	/* don't erase a leading slash */
	if (p == path && IS_DIR_SEP(*p))
		p++;
	*p = '\0';
}


/*
 *	trim_trailing_separator
 *
 * trim off trailing slashes, but not a leading slash
 */
static void
trim_trailing_separator(char *path)
{
	char	   *p;

	path = skip_drive(path);
	p = path + strlen(path);
	if (p > path)
		for (p--; p > path && IS_DIR_SEP(*p); p--)
			*p = '\0';
}
