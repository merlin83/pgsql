/*-------------------------------------------------------------------------
 *
 * libpq-int.h
 *	  This file contains internal definitions meant to be used only by
 *	  the frontend libpq library, not by applications that call it.
 *
 *	  An application can include this file if it wants to bypass the
 *	  official API defined by libpq-fe.h, but code that does so is much
 *	  more likely to break across PostgreSQL releases than code that uses
 *	  only the official API.
 *
 * Portions Copyright (c) 1996-2002, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: libpq-int.h,v 1.64 2003-04-24 21:16:44 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */

#ifndef LIBPQ_INT_H
#define LIBPQ_INT_H

#include <time.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/time.h>
#endif

#if defined(WIN32) && (!defined(ssize_t))
typedef int ssize_t;			/* ssize_t doesn't exist in VC (atleast
								 * not VC6) */
#endif

/* We assume libpq-fe.h has already been included. */
#include "postgres_fe.h"

/* include stuff common to fe and be */
#include "libpq/pqcomm.h"
#include "lib/dllist.h"
/* include stuff found in fe only */
#include "pqexpbuffer.h"

#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

/* libpq supports this version of the frontend/backend protocol.
 *
 * NB: we used to use PG_PROTOCOL_LATEST from the backend pqcomm.h file,
 * but that's not really the right thing: just recompiling libpq
 * against a more recent backend isn't going to magically update it
 * for most sorts of protocol changes.	So, when you change libpq
 * to support a different protocol revision, you have to change this
 * constant too.  PG_PROTOCOL_EARLIEST and PG_PROTOCOL_LATEST in
 * pqcomm.h describe what the backend knows, not what libpq knows.
 */

#define PG_PROTOCOL_LIBPQ	PG_PROTOCOL(3,103) /* XXX temporary value */

/*
 * POSTGRES backend dependent Constants.
 */

#define PQERRORMSG_LENGTH 1024
#define CMDSTATUS_LEN 40

/*
 * PGresult and the subsidiary types PGresAttDesc, PGresAttValue
 * represent the result of a query (or more precisely, of a single SQL
 * command --- a query string given to PQexec can contain multiple commands).
 * Note we assume that a single command can return at most one tuple group,
 * hence there is no need for multiple descriptor sets.
 */

/* Subsidiary-storage management structure for PGresult.
 * See space management routines in fe-exec.c for details.
 * Note that space[k] refers to the k'th byte starting from the physical
 * head of the block --- it's a union, not a struct!
 */
typedef union pgresult_data PGresult_data;

union pgresult_data
{
	PGresult_data *next;		/* link to next block, or NULL */
	char		space[1];		/* dummy for accessing block as bytes */
};

/* Data about a single attribute (column) of a query result */

typedef struct pgresAttDesc
{
	char	   *name;			/* type name */
	Oid			typid;			/* type id */
	int			typlen;			/* type size */
	int			atttypmod;		/* type-specific modifier info */
}	PGresAttDesc;

/* Data for a single attribute of a single tuple */

/* We use char* for Attribute values.
   The value pointer always points to a null-terminated area; we add a
   null (zero) byte after whatever the backend sends us.  This is only
   particularly useful for text tuples ... with a binary value, the
   value might have embedded nulls, so the application can't use C string
   operators on it.  But we add a null anyway for consistency.
   Note that the value itself does not contain a length word.

   A NULL attribute is a special case in two ways: its len field is NULL_LEN
   and its value field points to null_field in the owning PGresult.  All the
   NULL attributes in a query result point to the same place (there's no need
   to store a null string separately for each one).
 */

#define NULL_LEN		(-1)	/* pg_result len for NULL value */

typedef struct pgresAttValue
{
	int			len;			/* length in bytes of the value */
	char	   *value;			/* actual value, plus terminating zero
								 * byte */
}	PGresAttValue;

struct pg_result
{
	int			ntups;
	int			numAttributes;
	PGresAttDesc *attDescs;
	PGresAttValue **tuples;		/* each PGresTuple is an array of
								 * PGresAttValue's */
	int			tupArrSize;		/* size of tuples array allocated */
	ExecStatusType resultStatus;
	char		cmdStatus[CMDSTATUS_LEN];		/* cmd status from the
												 * last query */
	int			binary;			/* binary tuple values if binary == 1,
								 * otherwise text */

	/*
	 * The conn link in PGresult is no longer used by any libpq code. It
	 * should be removed entirely, because it could be a dangling link
	 * (the application could keep the PGresult around longer than it
	 * keeps the PGconn!)  But there may be apps out there that depend on
	 * it, so we will leave it here at least for a release or so.
	 */
	PGconn	   *xconn;			/* connection we did the query on, if any */

	/*
	 * These fields are copied from the originating PGconn, so that
	 * operations on the PGresult don't have to reference the PGconn.
	 */
	PQnoticeProcessor noticeHook;		/* notice/error message processor */
	void	   *noticeArg;
	int			client_encoding;	/* encoding id */

	/*
	 * Error information (all NULL if not an error result).  errMsg is the
	 * "overall" error message returned by PQresultErrorMessage.  If we
	 * got a field-ized error from the server then the additional fields
	 * may be set.
	 */
	char	   *errMsg;			/* error message, or NULL if no error */

	char	   *errSeverity;	/* severity code */
	char	   *errCode;		/* SQLSTATE code */
	char	   *errPrimary;		/* primary message text */
	char	   *errDetail;		/* detail text */
	char	   *errHint;		/* hint text */
	char	   *errPosition;	/* cursor position */
	char	   *errContext;		/* location information */
	char	   *errFilename;	/* source-code file name */
	char	   *errLineno;		/* source-code line number */
	char	   *errFuncname;	/* source-code function name */

	/* All NULL attributes in the query result point to this null string */
	char		null_field[1];

	/*
	 * Space management information.  Note that attDescs and errMsg, if
	 * not null, point into allocated blocks.  But tuples points to a
	 * separately malloc'd block, so that we can realloc it.
	 */
	PGresult_data *curBlock;	/* most recently allocated block */
	int			curOffset;		/* start offset of free space in block */
	int			spaceLeft;		/* number of free bytes remaining in block */
};

/* PGAsyncStatusType defines the state of the query-execution state machine */
typedef enum
{
	PGASYNC_IDLE,				/* nothing's happening, dude */
	PGASYNC_BUSY,				/* query in progress */
	PGASYNC_READY,				/* result ready for PQgetResult */
	PGASYNC_COPY_IN,			/* Copy In data transfer in progress */
	PGASYNC_COPY_OUT			/* Copy Out data transfer in progress */
}	PGAsyncStatusType;

/* PGSetenvStatusType defines the state of the PQSetenv state machine */
typedef enum
{
	SETENV_STATE_ENCODINGS_SEND,	/* About to send an "encodings" query */
	SETENV_STATE_ENCODINGS_WAIT,	/* Waiting for query to complete	  */
	SETENV_STATE_IDLE
}	PGSetenvStatusType;

/* large-object-access data ... allocated only if large-object code is used. */
typedef struct pgLobjfuncs
{
	Oid			fn_lo_open;		/* OID of backend function lo_open		*/
	Oid			fn_lo_close;	/* OID of backend function lo_close		*/
	Oid			fn_lo_creat;	/* OID of backend function lo_creat		*/
	Oid			fn_lo_unlink;	/* OID of backend function lo_unlink	*/
	Oid			fn_lo_lseek;	/* OID of backend function lo_lseek		*/
	Oid			fn_lo_tell;		/* OID of backend function lo_tell		*/
	Oid			fn_lo_read;		/* OID of backend function LOread		*/
	Oid			fn_lo_write;	/* OID of backend function LOwrite		*/
}	PGlobjfuncs;

/* PGconn stores all the state data associated with a single connection
 * to a backend.
 */
struct pg_conn
{
	/* Saved values of connection options */
	char	   *pghost;			/* the machine on which the server is
								 * running */
	char	   *pghostaddr;		/* the IPv4 address of the machine on
								 * which the server is running, in IPv4
								 * numbers-and-dots notation. Takes
								 * precedence over above. */
	char	   *pgport;			/* the server's communication port */
	char	   *pgunixsocket;	/* the Unix-domain socket that the server
								 * is listening on; if NULL, uses a
								 * default constructed from pgport */
	char	   *pgtty;			/* tty on which the backend messages is
								 * displayed (OBSOLETE, NOT USED) */
	char	   *connect_timeout; /* connection timeout (numeric string) */
	char	   *pgoptions;		/* options to start the backend with */
	char	   *dbName;			/* database name */
	char	   *pguser;			/* Postgres username and password, if any */
	char	   *pgpass;

	/* Optional file to write trace info to */
	FILE	   *Pfdebug;

	/* Callback procedure for notice/error message processing */
	PQnoticeProcessor noticeHook;
	void	   *noticeArg;

	/* Status indicators */
	ConnStatusType status;
	PGAsyncStatusType asyncStatus;
	char		copy_is_binary;	/* 1 = copy binary, 0 = copy text */
	int			copy_already_done; /* # bytes already returned in COPY OUT */
	int			nonblocking;	/* whether this connection is using a
								 * blocking socket to the backend or not */
	Dllist	   *notifyList;		/* Notify msgs not yet handed to
								 * application */

	/* Connection data */
	int			sock;			/* Unix FD for socket, -1 if not connected */
	SockAddr	laddr;			/* Local address */
	SockAddr	raddr;			/* Remote address */
	int			raddr_len;		/* Length of remote address */

	/* Miscellaneous stuff */
	int			be_pid;			/* PID of backend --- needed for cancels */
	int			be_key;			/* key of backend --- needed for cancels */
	char		md5Salt[4];		/* password salt received from backend */
	char		cryptSalt[2];	/* password salt received from backend */
	int			client_encoding; /* encoding id */
	PGlobjfuncs *lobjfuncs;		/* private state for large-object access
								 * fns */

	/* Buffer for data received from backend and not yet processed */
	char	   *inBuffer;		/* currently allocated buffer */
	int			inBufSize;		/* allocated size of buffer */
	int			inStart;		/* offset to first unconsumed data in
								 * buffer */
	int			inCursor;		/* next byte to tentatively consume */
	int			inEnd;			/* offset to first position after avail
								 * data */

	/* Buffer for data not yet sent to backend */
	char	   *outBuffer;		/* currently allocated buffer */
	int			outBufSize;		/* allocated size of buffer */
	int			outCount;		/* number of chars waiting in buffer */

	/* State for constructing messages in outBuffer */
	int			outMsgStart;	/* offset to msg start (length word) */
	int			outMsgEnd;		/* offset to msg end (so far) */

	/* Status for asynchronous result construction */
	PGresult   *result;			/* result being constructed */
	PGresAttValue *curTuple;	/* tuple currently being read */

	/* Status for sending environment info.  Used during PQSetenv only. */
	PGSetenvStatusType setenv_state;

#ifdef USE_SSL
	bool		allow_ssl_try;	/* Allowed to try SSL negotiation */
	bool		require_ssl;	/* Require SSL to make connection */
	SSL		   *ssl;			/* SSL status, if have SSL connection */
	X509	   *peer;			/* X509 cert of server */
	char		peer_dn[256 + 1];		/* peer distinguished name */
	char		peer_cn[SM_USER + 1];	/* peer common name */
#endif

	/* Buffer for current error message */
	PQExpBufferData errorMessage;		/* expansible string */

	/* Buffer for receiving various parts of messages */
	PQExpBufferData workBuffer; /* expansible string */
};

/* String descriptions of the ExecStatusTypes.
 * direct use of this array is deprecated; call PQresStatus() instead.
 */
extern char *const pgresStatus[];

/* ----------------
 * Internal functions of libpq
 * Functions declared here need to be visible across files of libpq,
 * but are not intended to be called by applications.  We use the
 * convention "pqXXX" for internal functions, vs. the "PQxxx" names
 * used for application-visible routines.
 * ----------------
 */

/* === in fe-connect.c === */

extern int	pqPacketSend(PGconn *conn, char pack_type,
						 const void *buf, size_t buf_len);

/* === in fe-exec.c === */

extern void pqSetResultError(PGresult *res, const char *msg);
extern void *pqResultAlloc(PGresult *res, size_t nBytes, bool isBinary);
extern char *pqResultStrdup(PGresult *res, const char *str);
extern void pqClearAsyncResult(PGconn *conn);
extern int	pqGetErrorNotice(PGconn *conn, bool isError);

/* === in fe-misc.c === */

 /*
  * "Get" and "Put" routines return 0 if successful, EOF if not. Note that
  * for Get, EOF merely means the buffer is exhausted, not that there is
  * necessarily any error.
  */
extern int	pqCheckInBufferSpace(int bytes_needed, PGconn *conn);
extern int	pqGetc(char *result, PGconn *conn);
extern int	pqPutc(char c, PGconn *conn);
extern int	pqGets(PQExpBuffer buf, PGconn *conn);
extern int	pqPuts(const char *s, PGconn *conn);
extern int	pqGetnchar(char *s, size_t len, PGconn *conn);
extern int	pqPutnchar(const char *s, size_t len, PGconn *conn);
extern int	pqGetInt(int *result, size_t bytes, PGconn *conn);
extern int	pqPutInt(int value, size_t bytes, PGconn *conn);
extern int	pqPutMsgStart(char msg_type, PGconn *conn);
extern int	pqPutMsgEnd(PGconn *conn);
extern int	pqReadData(PGconn *conn);
extern int	pqFlush(PGconn *conn);
extern int	pqWait(int forRead, int forWrite, PGconn *conn);
extern int	pqWaitTimed(int forRead, int forWrite, PGconn *conn, 
						time_t finish_time);
extern int	pqReadReady(PGconn *conn);
extern int	pqWriteReady(PGconn *conn);

/* === in fe-secure.c === */

extern int	pqsecure_initialize(PGconn *);
extern void pqsecure_destroy(void);
extern int	pqsecure_open_client(PGconn *);
extern void pqsecure_close(PGconn *);
extern ssize_t pqsecure_read(PGconn *, void *ptr, size_t len);
extern ssize_t pqsecure_write(PGconn *, const void *ptr, size_t len);

/* bits in a byte */
#define BYTELEN 8

/* fall back options if they are not specified by arguments or defined
   by environment variables */
#define DefaultHost		"localhost"
#define DefaultTty		""
#define DefaultOption	""
#define DefaultAuthtype		  ""
#define DefaultPassword		  ""

/*
 * this is so that we can check is a connection is non-blocking internally
 * without the overhead of a function call
 */
#define pqIsnonblocking(conn)	((conn)->nonblocking)

#ifdef ENABLE_NLS
extern char *
libpq_gettext(const char *msgid)
__attribute__((format_arg(1)));

#else
#define libpq_gettext(x) (x)
#endif

/*
 * These macros are needed to let error-handling code be portable between
 * Unix and Windows.  (ugh)
 */
#ifdef WIN32
#define SOCK_ERRNO (WSAGetLastError())
#define SOCK_STRERROR winsock_strerror
#else
#define SOCK_ERRNO errno
#define SOCK_STRERROR strerror
#endif

#endif   /* LIBPQ_INT_H */
