/*-------------------------------------------------------------------------
 *
 * be-fsstubs.h
 *
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: be-fsstubs.h,v 1.10 2000-01-26 05:58:12 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef BE_FSSTUBS_H
#define BE_FSSTUBS_H

/* Redefine names LOread() and LOwrite() to be lowercase to allow calling
 *	using the new v6.1 case-insensitive SQL parser. Define macros to allow
 *	the existing code to stay the same. - tgl 97/05/03
 */

#define LOread(f,l) loread(f,l)
#define LOwrite(f,b) lowrite(f,b)

extern Oid	lo_import(text *filename);
extern int4 lo_export(Oid lobjId, text *filename);

extern Oid	lo_creat(int mode);

extern int	lo_open(Oid lobjId, int mode);
extern int	lo_close(int fd);
extern int	lo_read(int fd, char *buf, int len);
extern int	lo_write(int fd, char *buf, int len);
extern int	lo_lseek(int fd, int offset, int whence);
extern int	lo_tell(int fd);
extern int	lo_unlink(Oid lobjId);

extern struct varlena *loread(int fd, int len);
extern int	lowrite(int fd, struct varlena * wbuf);

/*
 * Added for buffer leak prevention [ Pascal Andr� <andre@via.ecp.fr> ]
 */
extern void lo_commit(bool isCommit);

#endif	 /* BE_FSSTUBS_H */
