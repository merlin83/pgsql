/*-------------------------------------------------------------------------
 *
 * fd.h
 *	  Virtual file descriptor definitions.
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: fd.h,v 1.23 2000-11-10 03:53:45 vadim Exp $
 *
 *-------------------------------------------------------------------------
 */

/*
 * calls:
 *
 *	File {Close, Read, Write, Seek, Tell, MarkDirty, Sync}
 *	{File Name Open, Allocate, Free} File
 *
 * These are NOT JUST RENAMINGS OF THE UNIX ROUTINES.
 * Use them for all file activity...
 *
 *	File fd;
 *	fd = FilePathOpenFile("foo", O_RDONLY, 0600);
 *
 *	AllocateFile();
 *	FreeFile();
 *
 * Use AllocateFile, not fopen, if you need a stdio file (FILE*); then
 * use FreeFile, not fclose, to close it.  AVOID using stdio for files
 * that you intend to hold open for any length of time, since there is
 * no way for them to share kernel file descriptors with other files.
 */
#ifndef FD_H
#define FD_H

/*
 * FileSeek uses the standard UNIX lseek(2) flags.
 */

typedef char *FileName;

typedef int File;

/*
 * prototypes for functions in fd.c
 */

/* Operations on virtual Files --- equivalent to Unix kernel file ops */
extern File FileNameOpenFile(FileName fileName, int fileFlags, int fileMode);
extern File PathNameOpenFile(FileName fileName, int fileFlags, int fileMode);
extern File OpenTemporaryFile(void);
extern void FileClose(File file);
extern void FileUnlink(File file);
extern int	FileRead(File file, char *buffer, int amount);
extern int	FileWrite(File file, char *buffer, int amount);
extern long FileSeek(File file, long offset, int whence);
extern int	FileTruncate(File file, long offset);
extern int	FileSync(File file);
extern void FileMarkDirty(File file);

/* Operations that allow use of regular stdio --- USE WITH CAUTION */
extern FILE *AllocateFile(char *name, char *mode);
extern void FreeFile(FILE *);

/* If you've really really gotta have a plain kernel FD, use this */
extern int	BasicOpenFile(FileName fileName, int fileFlags, int fileMode);

/* Miscellaneous support routines */
extern void closeAllVfds(void);
extern void AtEOXact_Files(void);

#ifdef XLOG
#define pg_fsync(fd)	fsync(fd)
#else
extern int	pg_fsync(int fd);
#endif

#endif	 /* FD_H */
