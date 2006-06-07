/*
 * Headers for port/dirent.c, win32 native implementation of dirent functions
 *
 * $PostgreSQL: pgsql/src/include/port/win32_msvc/dirent.h,v 1.1 2006/06/07 22:24:45 momjian Exp $
 */

#ifndef _WIN32VC_DIRENT_H
#define _WIN32VC_DIRENT_H
struct dirent {
	long d_ino;
	unsigned short d_reclen;
	unsigned short d_namlen;
	char d_name[MAX_PATH];
};

typedef struct DIR DIR;

DIR* opendir(const char *);
struct dirent* readdir(DIR *);
int closedir(DIR*);
#endif
