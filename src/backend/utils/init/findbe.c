/*-------------------------------------------------------------------------
 *
 * findbe.c --
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *    $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/utils/init/Attic/findbe.c,v 1.1.1.1 1996-07-09 06:22:08 scrappy Exp $
 *
 *-------------------------------------------------------------------------
 */
#include <stdio.h>
#ifndef WIN32
#include <grp.h>
#else
#include <windows.h>
#endif /* WIN32 */
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "c.h"
#include "miscadmin.h"	/* for DebugLvl */

#ifndef S_IRUSR			/* XXX [TRH] should be in a header */
#   define S_IRUSR	S_IREAD
#   define S_IWUSR	S_IWRITE
#   define S_IXUSR	S_IEXEC
#   define S_IRGRP	((S_IRUSR)>>3)
#   define S_IWGRP	((S_IWUSR)>>3)
#   define S_IXGRP	((S_IXUSR)>>3)
#   define S_IROTH	((S_IRUSR)>>6)
#   define S_IWOTH	((S_IWUSR)>>6)
#   define S_IXOTH	((S_IXUSR)>>6)
#endif

/*
 * ValidateBackend -- validate "path" as a POSTGRES executable file
 *
 * returns 0 if the file is found and no error is encountered.
 *	  -1 if the regular file "path" does not exist or cannot be executed.
 *	  -2 if the file is otherwise valid but cannot be read.
 */
int
ValidateBackend(char *path)
{
#ifndef WIN32
    struct stat		buf;
    uid_t		euid;
    struct group	*gp;
    struct passwd	*pwp;
    int			i;
    int			is_r = 0;
    int			is_x = 0;
    int			in_grp = 0;
#else
    DWORD file_attributes;
#endif /* WIN32 */
    
    /*
     * Ensure that the file exists and is a regular file.
     *
     * XXX if you have a broken system where stat() looks at the symlink
     *     instead of the underlying file, you lose.
     */
    if (strlen(path) >= MAXPGPATH) {
	if (DebugLvl > 1)
	    fprintf(stderr, "ValidateBackend: pathname \"%s\" is too long\n",
		    path);
	return(-1);
    }

#ifndef WIN32
    if (stat(path, &buf) < 0) {
	if (DebugLvl > 1)
	    fprintf(stderr, "ValidateBackend: can't stat \"%s\"\n",
		    path);
	return(-1);
    }
    if (!(buf.st_mode & S_IFREG)) {
	if (DebugLvl > 1)
	    fprintf(stderr, "ValidateBackend: \"%s\" is not a regular file\n",
		    path);
	return(-1);
    }
    
    /*
     * Ensure that we are using an authorized backend. 
     *
     * XXX I'm open to suggestions here.  I would like to enforce ownership
     *     of backends by user "postgres" but people seem to like to run
     *     as users other than "postgres"...
     */
    
    /*
     * Ensure that the file is both executable and readable (required for
     * dynamic loading).
     *
     * We use the effective uid here because the backend will not have
     * executed setuid() by the time it calls this routine.
     */
    euid = geteuid();
    if (euid == buf.st_uid) {
	is_r = buf.st_mode & S_IRUSR;
	is_x = buf.st_mode & S_IXUSR;
	if (DebugLvl > 1 && !(is_r && is_x))
	    fprintf(stderr, "ValidateBackend: \"%s\" is not user read/execute\n",
		    path);
	return(is_x ? (is_r ? 0 : -2) : -1);
    }
    pwp = getpwuid(euid);
    if (pwp) {
	if (pwp->pw_gid == buf.st_gid) {
	    ++in_grp;
	} else if (pwp->pw_name &&
		   (gp = getgrgid(buf.st_gid))) {
	    for (i = 0; gp->gr_mem[i]; ++i) {
		if (!strcmp(gp->gr_mem[i], pwp->pw_name)) {
		    ++in_grp;
		    break;
		}
	    }
	}
	if (in_grp) {
	    is_r = buf.st_mode & S_IRGRP;
	    is_x = buf.st_mode & S_IXGRP;
	    if (DebugLvl > 1 && !(is_r && is_x))
		fprintf(stderr, "ValidateBackend: \"%s\" is not group read/execute\n",
			path);
	    return(is_x ? (is_r ? 0 : -2) : -1);
	}
    }
    is_r = buf.st_mode & S_IROTH;
    is_x = buf.st_mode & S_IXOTH;
    if (DebugLvl > 1 && !(is_r && is_x))
	fprintf(stderr, "ValidateBackend: \"%s\" is not other read/execute\n",
		path);
    return(is_x ? (is_r ? 0 : -2) : -1);
#else
    file_attributes = GetFileAttributes(path);
    if(file_attributes != 0xFFFFFFFF)
      return(0);
    else
      return(-1);
#endif /* WIN32 */
}

/*
 * FindBackend -- find an absolute path to a valid backend executable
 *
 * The reason we have to work so hard to find an absolute path is that
 * we need to feed the backend server the location of its actual 
 * executable file -- otherwise, we can't do dynamic loading.
 */
int
FindBackend(char *backend, char *argv0)
{
    char	buf[MAXPGPATH + 2];
    char	*p;
    char	*path, *startp, *endp;
    int		pathlen;
    
#ifdef WIN32
    strcpy(backend, argv0);
    return(0);
#endif /* WIN32 */
    
    /*
     * for the postmaster:
     * First try: use the backend that's located in the same directory
     * as the postmaster, if it was invoked with an explicit path.
     * Presumably the user used an explicit path because it wasn't in
     * PATH, and we don't want to use incompatible executables.
     *
     * This has the neat property that it works for installed binaries,
     * old source trees (obj/support/post{master,gres}) and new marc
     * source trees (obj/post{master,gres}) because they all put the 
     * two binaries in the same place.
     *
     * for the backend server:
     * First try: if we're given some kind of path, use it (making sure
     * that a relative path is made absolute before returning it).
     */
    if (argv0 && (p = strrchr(argv0, '/')) && *++p) {
	if (*argv0 == '/' || !getcwd(buf, MAXPGPATH))
	    buf[0] = '\0';
	else
	    (void) strcat(buf, "/");
	(void) strcat(buf, argv0);
	p = strrchr(buf, '/');
	(void) strcpy(++p, "postgres");
	if (!ValidateBackend(buf)) {
	    (void) strncpy(backend, buf, MAXPGPATH);
	    if (DebugLvl)
		fprintf(stderr, "FindBackend: found \"%s\" using argv[0]\n",
			backend);
	    return(0);
	}
	fprintf(stderr, "FindBackend: invalid backend \"%s\"\n",
		buf);
	return(-1);
    }
    
    /*
     * Second try: since no explicit path was supplied, the user must
     * have been relying on PATH.  We'll use the same PATH.
     */
    if ((p = getenv("PATH")) && *p) {
	if (DebugLvl)
	    fprintf(stderr, "FindBackend: searching PATH ...\n");
	pathlen = strlen(p);
	path = malloc(pathlen + 1);
	(void) strcpy(path, p);
	for (startp = path, endp = strchr(path, ':');
	     startp && *startp;
	     startp = endp + 1, endp = strchr(startp, ':')) {
	    if (startp == endp)	/* it's a "::" */
		continue;
	    if (endp)
		*endp = '\0';
	    if (*startp == '/' || !getcwd(buf, MAXPGPATH))
		buf[0] = '\0';
	    (void) strcat(buf, startp);
	    (void) strcat(buf, "/postgres");
	    switch (ValidateBackend(buf)) {
	    case 0:		/* found ok */
		(void) strncpy(backend, buf, MAXPGPATH);
		if (DebugLvl)
		    fprintf(stderr, "FindBackend: found \"%s\" using PATH\n",
			    backend);
		free(path);
		return(0);
	    case -1:		/* wasn't even a candidate, keep looking */
		break;
	    case -2:		/* found but disqualified */
		fprintf(stderr, "FindBackend: could not read backend \"%s\"\n",
			buf);
		free(path);
		return(-1);
	    }
	    if (!endp)		/* last one */
		break;
	}
	free(path);
    }

    fprintf(stderr, "FindBackend: could not find a backend to execute...\n");
    return(-1);
}
