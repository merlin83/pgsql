/*-------------------------------------------------------------------------
 *
 * libpq.h
 *	  POSTGRES LIBPQ buffer structure definitions.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id: libpq.h,v 1.28 1999-04-25 03:19:13 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef LIBPQ_H
#define LIBPQ_H

#include <sys/types.h>

#include <netinet/in.h>

#include "libpq/libpq-be.h"
#include "tcop/dest.h"


/* ----------------
 * PQArgBlock 
 *		Information (pointer to array of this structure) required
 *		for the PQfn() call.
 * ----------------
 */
typedef struct
{
	int			len;
	int			isint;
	union
	{
		int		   *ptr;		/* can't use void (dec compiler barfs)	 */
		int			integer;
	}			u;
} PQArgBlock;

/* ----------------
 * TypeBlock 
 *		Information about an attribute.
 * ----------------
 */
#define NameLength NAMEDATALEN

typedef struct TypeBlock
{
	char		name[NAMEDATALEN];		/* name of the attribute */
	int			typid;			/* typid of the type */
	int			typlen;			/* typlen of the type */
} TypeBlock;

/* ----------------
 * TupleBlock 
 *		Data of a tuple.
 * ----------------
 */
#define TupleBlockSize 100

typedef struct TupleBlock
{
	char	  **values[TupleBlockSize]; /* an array of tuples */
	int		   *lengths[TupleBlockSize];		/* an array of length vec.
												 * foreach tuple */
	struct TupleBlock *next;	/* next tuple block */
	int			tuple_index;	/* current tuple index */
} TupleBlock;

/* ----------------
 * GroupBuffer 
 *		A group of tuples with the same attributes.
 * ----------------
 */
typedef struct GroupBuffer
{
	int			no_tuples;		/* number of tuples in this group */
	int			no_fields;		/* number of attributes */
	TypeBlock  *types;			/* types of the attributes */
	TupleBlock *tuples;			/* tuples in this group */
	struct GroupBuffer *next;	/* next group */
} GroupBuffer;

/* ----------------
 * PortalBuffer 
 *		Data structure of a portal buffer.
 * ----------------
 */
typedef struct PortalBuffer
{
	int			rule_p;			/* 1 if this is an asynchronized portal. */
	int			no_tuples;		/* number of tuples in this portal buffer */
	int			no_groups;		/* number of tuple groups */
	GroupBuffer *groups;		/* linked list of tuple groups */
} PortalBuffer;

/* ----------------
 * PortalEntry 
 *		an entry in the global portal table
 *
 * Note: the portalcxt is only meaningful for PQcalls made from
 *		 within a postgres backend.  frontend apps should ignore it.
 * ----------------
 */
#define PortalNameLength 32

typedef struct PortalEntry
{
	char		name[PortalNameLength]; /* name of this portal */
	PortalBuffer *portal;		/* tuples contained in this portal */
	Pointer		portalcxt;		/* memory context (for backend) */
	Pointer		result;			/* result for PQexec */
} PortalEntry;

#define PORTALS_INITIAL_SIZE 32
#define PORTALS_GROW_BY		 32

/* in portalbuf.c */
extern PortalEntry **portals;
extern size_t portals_array_size;

/*
 * Exceptions.
 */

#define libpq_raise(X, Y) ExcRaise((Exception *)(X), (ExcDetail) (Y),\
								   (ExcData)0, (ExcMessage) 0)

/* in portal.c */
extern Exception MemoryError,
			PortalError,
			PostquelError,
			ProtocolError;

/*
 * POSTGRES backend dependent Constants.
 */

/* ERROR_MSG_LENGTH should really be the same as ELOG_MAXLEN in utils/elog.h*/
#define ERROR_MSG_LENGTH 4096
#define COMMAND_LENGTH 20
#define REMARK_LENGTH 80

extern char PQerrormsg[ERROR_MSG_LENGTH];		/* in portal.c */

/*
 * External functions.
 */

/*
 * prototypes for functions in portal.c
 */
extern void pqdebug(char *target, char *msg);
extern void pqdebug2(char *target, char *msg1, char *msg2);
extern void PQtrace(void);
extern void PQuntrace(void);
extern int	PQnportals(int rule_p);
extern void PQpnames(char **pnames, int rule_p);
extern PortalBuffer *PQparray(char *pname);
extern int	PQrulep(PortalBuffer *portal);
extern int	PQntuples(PortalBuffer *portal);
extern int	PQninstances(PortalBuffer *portal);
extern int	PQngroups(PortalBuffer *portal);
extern int	PQntuplesGroup(PortalBuffer *portal, int group_index);
extern int	PQninstancesGroup(PortalBuffer *portal, int group_index);
extern int	PQnfieldsGroup(PortalBuffer *portal, int group_index);
extern int	PQfnumberGroup(PortalBuffer *portal, int group_index, char *field_name);
extern char *PQfnameGroup(PortalBuffer *portal, int group_index, int field_number);
extern int PQftypeGroup(PortalBuffer *portal, int group_index,
			 int field_number);
extern int PQfsizeGroup(PortalBuffer *portal, int group_index,
			 int field_number);
extern GroupBuffer *PQgroup(PortalBuffer *portal, int tuple_index);
extern int	PQgetgroup(PortalBuffer *portal, int tuple_index);
extern int	PQnfields(PortalBuffer *portal, int tuple_index);
extern int	PQfnumber(PortalBuffer *portal, int tuple_index, char *field_name);
extern char *PQfname(PortalBuffer *portal, int tuple_index, int field_number);
extern int	PQftype(PortalBuffer *portal, int tuple_index, int field_number);
extern int	PQfsize(PortalBuffer *portal, int tuple_index, int field_number);
extern int	PQsametype(PortalBuffer *portal, int tuple_index1, int tuple_index2);
extern char *PQgetvalue(PortalBuffer *portal, int tuple_index, int field_number);
extern char *PQgetAttr(PortalBuffer *portal, int tuple_index, int field_number);
extern int	PQgetlength(PortalBuffer *portal, int tuple_index, int field_number);
extern void PQclear(char *pname);

/*
 * prototypes for functions in portalbuf.c
 */
extern caddr_t pbuf_alloc(size_t size);
extern void pbuf_free(caddr_t pointer);
extern PortalBuffer *pbuf_addPortal(void);
extern GroupBuffer *pbuf_addGroup(PortalBuffer *portal);
extern TypeBlock *pbuf_addTypes(int n);
extern TupleBlock *pbuf_addTuples(void);
extern char **pbuf_addTuple(int n);
extern int *pbuf_addTupleValueLengths(int n);
extern char *pbuf_addValues(int n);
extern PortalEntry *pbuf_addEntry(void);
extern void pbuf_freeEntry(int i);
extern void pbuf_freeTypes(TypeBlock *types);
extern void pbuf_freeTuples(TupleBlock *tuples, int no_tuples, int no_fields);
extern void pbuf_freeGroup(GroupBuffer *group);
extern void pbuf_freePortal(PortalBuffer *portal);
extern int	pbuf_getIndex(char *pname);
extern void pbuf_setportalinfo(PortalEntry *entry, char *pname);
extern PortalEntry *pbuf_setup(char *pname);
extern void pbuf_close(char *pname);
extern GroupBuffer *pbuf_findGroup(PortalBuffer *portal, int group_index);
extern int	pbuf_findFnumber(GroupBuffer *group, char *field_name);
extern void pbuf_checkFnumber(GroupBuffer *group, int field_number);
extern char *pbuf_findFname(GroupBuffer *group, int field_number);

/* in be-dumpdata.c */
extern void be_portalinit(void);
extern void be_portalpush(PortalEntry *entry);
extern PortalEntry *be_portalpop(void);
extern PortalEntry *be_currentportal(void);
extern PortalEntry *be_newportal(void);
extern void be_typeinit(PortalEntry *entry, TupleDesc attrs,
			int natts);
extern void be_printtup(HeapTuple tuple, TupleDesc typeinfo,
						DestReceiver* self);


/* in be-pqexec.c */
extern char *PQfn(int fnid, int *result_buf, int result_len, int result_is_int,
	 PQArgBlock *args, int nargs);
extern char *PQexec(char *query);
extern int	pqtest_PQexec(char *q);
extern int	pqtest_PQfn(char *q);
extern int32 pqtest(struct varlena * vlena);

/*
 * prototypes for functions in pqcomm.c
 */
extern int	StreamServerPort(char *hostName, short portName, int *fdP);
extern int	StreamConnection(int server_fd, Port *port);
extern void StreamClose(int sock);
extern void pq_init(void);
extern int	pq_getport(void);
extern void pq_close(void);
extern int	pq_getbytes(char *s, size_t len);
extern int	pq_getstring(char *s, size_t len);
extern int	pq_peekbyte(void);
extern int	pq_putbytes(const char *s, size_t len);
extern int	pq_flush(void);
extern int	pq_putmessage(char msgtype, const char *s, size_t len);
extern void	pq_startcopyout(void);
extern void	pq_endcopyout(bool errorAbort);

#endif	 /* LIBPQ_H */
