/*-------------------------------------------------------------------------
 *
 * fe-auth.c
 *	   The front-end (client) authorization routines
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * NOTE: the error message strings returned by this module must not
 * exceed INITIAL_EXPBUFFER_SIZE (currently 256 bytes).
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/interfaces/libpq/fe-auth.c,v 1.129 2007/07/23 10:57:36 mha Exp $
 *
 *-------------------------------------------------------------------------
 */

/*
 * INTERFACE ROUTINES
 *	   frontend (client) routines:
 *		pg_fe_sendauth			send authentication information
 *		pg_fe_getauthname		get user's name according to the client side
 *								of the authentication system
 */

#include "postgres_fe.h"

#ifdef WIN32
#include "win32.h"
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>			/* for MAXHOSTNAMELEN on most */
#include <sys/socket.h>
#if defined(HAVE_STRUCT_CMSGCRED) || defined(HAVE_STRUCT_FCRED) || defined(HAVE_STRUCT_SOCKCRED)
#include <sys/uio.h>
#include <sys/ucred.h>
#endif
#ifndef  MAXHOSTNAMELEN
#include <netdb.h>				/* for MAXHOSTNAMELEN on some */
#endif
#include <pwd.h>
#endif

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#include "libpq-fe.h"
#include "fe-auth.h"
#include "libpq/md5.h"


#ifdef KRB5
/*
 * MIT Kerberos authentication system - protocol version 5
 */

#include <krb5.h>
/* Some old versions of Kerberos do not include <com_err.h> in <krb5.h> */
#if !defined(__COM_ERR_H) && !defined(__COM_ERR_H__)
#include <com_err.h>
#endif

/*
 * Heimdal doesn't have a free function for unparsed names. Just pass it to
 * standard free() which should work in these cases.
 */
#ifndef HAVE_KRB5_FREE_UNPARSED_NAME
static void
krb5_free_unparsed_name(krb5_context context, char *val)
{
	free(val);
}
#endif

/*
 * pg_an_to_ln -- return the local name corresponding to an authentication
 *				  name
 *
 * XXX Assumes that the first aname component is the user name.  This is NOT
 *	   necessarily so, since an aname can actually be something out of your
 *	   worst X.400 nightmare, like
 *		  ORGANIZATION=U. C. Berkeley/NAME=Paul M. Aoki@CS.BERKELEY.EDU
 *	   Note that the MIT an_to_ln code does the same thing if you don't
 *	   provide an aname mapping database...it may be a better idea to use
 *	   krb5_an_to_ln, except that it punts if multiple components are found,
 *	   and we can't afford to punt.
 *
 * For WIN32, convert username to lowercase because the Win32 kerberos library
 * generates tickets with the username as the user entered it instead of as
 * it is entered in the directory.
 */
static char *
pg_an_to_ln(char *aname)
{
	char	   *p;

	if ((p = strchr(aname, '/')) || (p = strchr(aname, '@')))
		*p = '\0';
#ifdef WIN32
	for (p = aname; *p; p++)
		*p = pg_tolower((unsigned char) *p);
#endif

	return aname;
}


/*
 * Various krb5 state which is not connection specific, and a flag to
 * indicate whether we have initialised it yet.
 */
/*
static int	pg_krb5_initialised;
static krb5_context pg_krb5_context;
static krb5_ccache pg_krb5_ccache;
static krb5_principal pg_krb5_client;
static char *pg_krb5_name;
*/

struct krb5_info
{
	int			pg_krb5_initialised;
	krb5_context pg_krb5_context;
	krb5_ccache pg_krb5_ccache;
	krb5_principal pg_krb5_client;
	char	   *pg_krb5_name;
};


static int
pg_krb5_init(char *PQerrormsg, struct krb5_info * info)
{
	krb5_error_code retval;

	if (info->pg_krb5_initialised)
		return STATUS_OK;

	retval = krb5_init_context(&(info->pg_krb5_context));
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_init: krb5_init_context: %s\n",
				 error_message(retval));
		return STATUS_ERROR;
	}

	retval = krb5_cc_default(info->pg_krb5_context, &(info->pg_krb5_ccache));
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_init: krb5_cc_default: %s\n",
				 error_message(retval));
		krb5_free_context(info->pg_krb5_context);
		return STATUS_ERROR;
	}

	retval = krb5_cc_get_principal(info->pg_krb5_context, info->pg_krb5_ccache,
								   &(info->pg_krb5_client));
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_init: krb5_cc_get_principal: %s\n",
				 error_message(retval));
		krb5_cc_close(info->pg_krb5_context, info->pg_krb5_ccache);
		krb5_free_context(info->pg_krb5_context);
		return STATUS_ERROR;
	}

	retval = krb5_unparse_name(info->pg_krb5_context, info->pg_krb5_client, &(info->pg_krb5_name));
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_init: krb5_unparse_name: %s\n",
				 error_message(retval));
		krb5_free_principal(info->pg_krb5_context, info->pg_krb5_client);
		krb5_cc_close(info->pg_krb5_context, info->pg_krb5_ccache);
		krb5_free_context(info->pg_krb5_context);
		return STATUS_ERROR;
	}

	info->pg_krb5_name = pg_an_to_ln(info->pg_krb5_name);

	info->pg_krb5_initialised = 1;
	return STATUS_OK;
}

static void
pg_krb5_destroy(struct krb5_info * info)
{
	krb5_free_principal(info->pg_krb5_context, info->pg_krb5_client);
	krb5_cc_close(info->pg_krb5_context, info->pg_krb5_ccache);
	krb5_free_unparsed_name(info->pg_krb5_context, info->pg_krb5_name);
	krb5_free_context(info->pg_krb5_context);
}



/*
 * pg_krb5_authname -- returns a copy of whatever name the user
 *					   has authenticated to the system, or NULL
 */
static char *
pg_krb5_authname(char *PQerrormsg)
{
	char	   *tmp_name;
	struct krb5_info info;

	info.pg_krb5_initialised = 0;

	if (pg_krb5_init(PQerrormsg, &info) != STATUS_OK)
		return NULL;
	tmp_name = strdup(info.pg_krb5_name);
	pg_krb5_destroy(&info);

	return tmp_name;
}


/*
 * pg_krb5_sendauth -- client routine to send authentication information to
 *					   the server
 */
static int
pg_krb5_sendauth(char *PQerrormsg, int sock, const char *hostname, const char *servicename)
{
	krb5_error_code retval;
	int			ret;
	krb5_principal server;
	krb5_auth_context auth_context = NULL;
	krb5_error *err_ret = NULL;
	struct krb5_info info;

	info.pg_krb5_initialised = 0;

	if (!hostname)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_sendauth: hostname must be specified for Kerberos authentication\n");
		return STATUS_ERROR;
	}

	ret = pg_krb5_init(PQerrormsg, &info);
	if (ret != STATUS_OK)
		return ret;

	retval = krb5_sname_to_principal(info.pg_krb5_context, hostname, servicename,
									 KRB5_NT_SRV_HST, &server);
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_sendauth: krb5_sname_to_principal: %s\n",
				 error_message(retval));
		pg_krb5_destroy(&info);
		return STATUS_ERROR;
	}

	/*
	 * libpq uses a non-blocking socket. But kerberos needs a blocking socket,
	 * and we have to block somehow to do mutual authentication anyway. So we
	 * temporarily make it blocking.
	 */
	if (!pg_set_block(sock))
	{
		char		sebuf[256];

		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 libpq_gettext("could not set socket to blocking mode: %s\n"), pqStrerror(errno, sebuf, sizeof(sebuf)));
		krb5_free_principal(info.pg_krb5_context, server);
		pg_krb5_destroy(&info);
		return STATUS_ERROR;
	}

	retval = krb5_sendauth(info.pg_krb5_context, &auth_context,
						   (krb5_pointer) & sock, (char *) servicename,
						   info.pg_krb5_client, server,
						   AP_OPTS_MUTUAL_REQUIRED,
						   NULL, 0,		/* no creds, use ccache instead */
						   info.pg_krb5_ccache, &err_ret, NULL, NULL);
	if (retval)
	{
		if (retval == KRB5_SENDAUTH_REJECTED && err_ret)
		{
#if defined(HAVE_KRB5_ERROR_TEXT_DATA)
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				  libpq_gettext("Kerberos 5 authentication rejected: %*s\n"),
					 (int) err_ret->text.length, err_ret->text.data);
#elif defined(HAVE_KRB5_ERROR_E_DATA)
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				  libpq_gettext("Kerberos 5 authentication rejected: %*s\n"),
					 (int) err_ret->e_data->length,
					 (const char *) err_ret->e_data->data);
#else
#error "bogus configuration"
#endif
		}
		else
		{
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
					 "krb5_sendauth: %s\n", error_message(retval));
		}

		if (err_ret)
			krb5_free_error(info.pg_krb5_context, err_ret);

		ret = STATUS_ERROR;
	}

	krb5_free_principal(info.pg_krb5_context, server);

	if (!pg_set_noblock(sock))
	{
		char		sebuf[256];

		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
		libpq_gettext("could not restore non-blocking mode on socket: %s\n"),
				 pqStrerror(errno, sebuf, sizeof(sebuf)));
		ret = STATUS_ERROR;
	}
	pg_krb5_destroy(&info);

	return ret;
}
#endif   /* KRB5 */

#ifdef ENABLE_GSS
/*
 * GSSAPI authentication system.
 */

#if defined(WIN32) && !defined(WIN32_ONLY_COMPILER)
/*
 * MIT Kerberos GSSAPI DLL doesn't properly export the symbols for MingW
 * that contain the OIDs required. Redefine here, values copied
 * from src/athena/auth/krb5/src/lib/gssapi/generic/gssapi_generic.c
 */
static const gss_OID_desc GSS_C_NT_HOSTBASED_SERVICE_desc =
 {10, (void *)"\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x04"};
static GSS_DLLIMP gss_OID GSS_C_NT_HOSTBASED_SERVICE = &GSS_C_NT_HOSTBASED_SERVICE_desc;
#endif

/*
 * Fetch all errors of a specific type that fit into a buffer
 * and append them.
 */
static void
pg_GSS_error_int(char *mprefix, char *msg, int msglen,
                 OM_uint32 stat, int type)
{
	int				curlen = 0;
	OM_uint32		lmaj_s, lmin_s;
	gss_buffer_desc	lmsg;
	OM_uint32		msg_ctx = 0;

	do 
	{
		lmaj_s = gss_display_status(&lmin_s, stat, type, 
				GSS_C_NO_OID, &msg_ctx, &lmsg);

		if (curlen < msglen)
		{
			snprintf(msg + curlen, msglen - curlen, "%s: %s\n",
					mprefix, (char *)lmsg.value);
			curlen += lmsg.length;
		}
		gss_release_buffer(&lmin_s, &lmsg);
	} while (msg_ctx);
}

/*
 * GSSAPI errors contains two parts. Put as much as possible of
 * both parts into the string.
 */
static void
pg_GSS_error(char *mprefix, char *msg, int msglen,
	OM_uint32 maj_stat, OM_uint32 min_stat)
{
	int mlen;

	/* Fetch major error codes */
	pg_GSS_error_int(mprefix, msg, msglen, maj_stat, GSS_C_GSS_CODE);
	mlen = strlen(msg);

	/* If there is room left, try to add the minor codes as well */
	if (mlen < msglen-1)
		pg_GSS_error_int(mprefix, msg + mlen, msglen - mlen,
				min_stat, GSS_C_MECH_CODE);
}

/* 
 * Continue GSS authentication with next token as needed.
 */
static int
pg_GSS_continue(char *PQerrormsg, PGconn *conn)
{
	OM_uint32	maj_stat, min_stat, lmin_s;

	maj_stat = gss_init_sec_context(&min_stat,
			GSS_C_NO_CREDENTIAL,
			&conn->gctx,
			conn->gtarg_nam,
			GSS_C_NO_OID,
			GSS_C_MUTUAL_FLAG,
			0,
			GSS_C_NO_CHANNEL_BINDINGS,
			(conn->gctx==GSS_C_NO_CONTEXT)?GSS_C_NO_BUFFER:&conn->ginbuf,
			NULL,
			&conn->goutbuf,
			NULL,
			NULL);

	if (conn->gctx != GSS_C_NO_CONTEXT)
	{
		free(conn->ginbuf.value);
		conn->ginbuf.value = NULL;
		conn->ginbuf.length = 0;
	}

	if (conn->goutbuf.length != 0)
	{
		/*
		 * GSS generated data to send to the server. We don't care if it's
		 * the first or subsequent packet, just send the same kind of
		 * password packet.
		 */
		if (pqPacketSend(conn, 'p',
					conn->goutbuf.value, conn->goutbuf.length)
				!= STATUS_OK)
		{
			gss_release_buffer(&lmin_s, &conn->goutbuf);
			return STATUS_ERROR;
		}
	}
	gss_release_buffer(&lmin_s, &conn->goutbuf);

	if (maj_stat != GSS_S_COMPLETE && maj_stat != GSS_S_CONTINUE_NEEDED)
	{
		pg_GSS_error(libpq_gettext("GSSAPI continuation error"),
				PQerrormsg, PQERRORMSG_LENGTH,
				maj_stat, min_stat);
		gss_release_name(&lmin_s, &conn->gtarg_nam);
		if (conn->gctx)
			gss_delete_sec_context(&lmin_s, &conn->gctx, GSS_C_NO_BUFFER);
		return STATUS_ERROR;
	}

	if (maj_stat == GSS_S_COMPLETE)
		gss_release_name(&lmin_s, &conn->gtarg_nam);

	return STATUS_OK;
}

/* 
 * Send initial GSS authentication token
 */
static int
pg_GSS_startup(char *PQerrormsg, PGconn *conn)
{
	OM_uint32	maj_stat, min_stat;
	int			maxlen;
	gss_buffer_desc	temp_gbuf;

	if (conn->gctx)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				libpq_gettext("duplicate GSS auth request\n"));
		return STATUS_ERROR;
	}

	/*
	 * Import service principal name so the proper ticket can be
	 * acquired by the GSSAPI system.
	 */
	maxlen = NI_MAXHOST + strlen(conn->krbsrvname) + 2;
	temp_gbuf.value = (char*)malloc(maxlen);
	snprintf(temp_gbuf.value, maxlen, "%s@%s", 
			conn->krbsrvname, conn->pghost);
	temp_gbuf.length = strlen(temp_gbuf.value);

	maj_stat = gss_import_name(&min_stat, &temp_gbuf,
			GSS_C_NT_HOSTBASED_SERVICE, &conn->gtarg_nam);
	free(temp_gbuf.value);

	if (maj_stat != GSS_S_COMPLETE)
	{
		pg_GSS_error(libpq_gettext("GSSAPI name import error"), 
				PQerrormsg, PQERRORMSG_LENGTH,
				maj_stat, min_stat);
		return STATUS_ERROR;
	}

	/*
	 * Initial packet is the same as a continuation packet with
	 * no initial context.
	 */
	conn->gctx = GSS_C_NO_CONTEXT;

	return pg_GSS_continue(PQerrormsg, conn);
}
#endif /* ENABLE_GSS */


#ifdef ENABLE_SSPI
/*
 * SSPI authentication system (Windows only)
 */

static void
pg_SSPI_error(char *mprefix, char *msg, int msglen, SECURITY_STATUS r)
{
	char sysmsg[256];

	if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, r, 0, sysmsg, sizeof(sysmsg), NULL) == 0)
		snprintf(msg, msglen, "%s: sspi error %x", mprefix, r);
	else
		snprintf(msg, msglen, "%s: %s (%x)", mprefix, sysmsg, r);
}

/* 
 * Continue SSPI authentication with next token as needed.
 */
static int
pg_SSPI_continue(char *PQerrormsg, PGconn *conn)
{
	SECURITY_STATUS	r;
	CtxtHandle		newContext;
	ULONG			contextAttr;
	SecBufferDesc	inbuf;
	SecBufferDesc	outbuf;
	SecBuffer		OutBuffers[1];
	SecBuffer		InBuffers[1];

	if (conn->sspictx != NULL)
	{
		/*
		 * On runs other than the first we have some data to send. Put this
		 * data in a SecBuffer type structure.
		 */
		inbuf.ulVersion = SECBUFFER_VERSION;
		inbuf.cBuffers = 1;
		inbuf.pBuffers = InBuffers;
		InBuffers[0].pvBuffer = conn->ginbuf.value;
		InBuffers[0].cbBuffer = conn->ginbuf.length;
		InBuffers[0].BufferType = SECBUFFER_TOKEN;
	}

	OutBuffers[0].pvBuffer = NULL;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = 0;
	outbuf.cBuffers = 1;
	outbuf.pBuffers = OutBuffers;
	outbuf.ulVersion = SECBUFFER_VERSION;

	r = InitializeSecurityContext(conn->sspicred,
		conn->sspictx,
		conn->sspitarget,
		ISC_REQ_ALLOCATE_MEMORY,
		0,
		SECURITY_NETWORK_DREP,
		(conn->sspictx == NULL)?NULL:&inbuf,
		0,
		&newContext,
		&outbuf,
		&contextAttr,
		NULL);
	
	if (r != SEC_E_OK && r != SEC_I_CONTINUE_NEEDED)
	{
		pg_SSPI_error(libpq_gettext("SSPI continuation error"),
			PQerrormsg, PQERRORMSG_LENGTH, r);

		return STATUS_ERROR;
	}

	if (conn->sspictx == NULL)
	{
		/* On first run, transfer retreived context handle */
		conn->sspictx = malloc(sizeof(CtxtHandle));
		if (conn->sspictx == NULL)
		{
			strncpy(PQerrormsg, libpq_gettext("out of memory\n"), PQERRORMSG_LENGTH);
			return STATUS_ERROR;
		}
		memcpy(conn->sspictx, &newContext, sizeof(CtxtHandle));
	}
	else
	{
		/*
		 * On subsequent runs when we had data to send, free buffers that contained
		 * this data.
		 */
		free(conn->ginbuf.value);
		conn->ginbuf.value = NULL;
		conn->ginbuf.length = 0;
	}

	/*
	 * If SSPI returned any data to be sent to the server (as it normally would),
	 * send this data as a password packet.
	 */
	if (outbuf.cBuffers > 0)
	{
		if (outbuf.cBuffers != 1)
		{
			/*
			 * This should never happen, at least not for Kerberos authentication. Keep check
			 * in case it shows up with other authentication methods later.
			 */
			strncpy(PQerrormsg, "SSPI returned invalid number of output buffers\n", PQERRORMSG_LENGTH);
			return STATUS_ERROR;
		}

		if (pqPacketSend(conn, 'p',
			outbuf.pBuffers[0].pvBuffer, outbuf.pBuffers[0].cbBuffer))
		{
			FreeContextBuffer(outbuf.pBuffers[0].pvBuffer);
			return STATUS_ERROR;
		}
		FreeContextBuffer(outbuf.pBuffers[0].pvBuffer);
	}

	/* Cleanup is handled by the code in freePGconn() */
	return STATUS_OK;
}

/* 
 * Send initial SSPI authentication token.
 * If use_negotiate is 0, use kerberos authentication package which is
 * compatible with Unix. If use_negotiate is 1, use the negotiate package
 * which supports both kerberos and NTLM, but is not compatible with Unix.
 */
static int
pg_SSPI_startup(char *PQerrormsg, PGconn *conn, int use_negotiate)
{
	SECURITY_STATUS	r;
	TimeStamp		expire;

	conn->sspictx = NULL;

	/*
	 * Retreive credentials handle
	 */
	conn->sspicred = malloc(sizeof(CredHandle));
	if (conn->sspicred == NULL)
	{
		strncpy(PQerrormsg, libpq_gettext("out of memory\n"), PQERRORMSG_LENGTH);
		return STATUS_ERROR;
	}

	r = AcquireCredentialsHandle(NULL, use_negotiate?"negotiate":"kerberos", SECPKG_CRED_OUTBOUND, NULL, NULL, NULL, NULL, conn->sspicred, &expire);
	if (r != SEC_E_OK)
	{
		pg_SSPI_error("acquire credentials failed", PQerrormsg, PQERRORMSG_LENGTH, r);
		free(conn->sspicred);
		conn->sspicred = NULL;
		return STATUS_ERROR;
	}

	/*
	 * Compute target principal name. SSPI has a different format from GSSAPI, but
	 * not more complex. We can skip the @REALM part, because Windows will fill that
	 * in for us automatically.
	 */
	if (conn->pghost == NULL)
	{
		strncpy(PQerrormsg, libpq_gettext("hostname must be specified\n"), PQERRORMSG_LENGTH);
		return STATUS_ERROR;
	}
	conn->sspitarget = malloc(strlen(conn->krbsrvname)+strlen(conn->pghost)+2);
	if (!conn->sspitarget)
	{
		strncpy(PQerrormsg, libpq_gettext("out of memory\n"), PQERRORMSG_LENGTH);
		return STATUS_ERROR;
	}
	sprintf(conn->sspitarget, "%s/%s", conn->krbsrvname, conn->pghost);

	/*
	 * Indicate that we're in SSPI authentication mode to make sure that
	 * pg_SSPI_continue is called next time in the negotiation.
	 */
	conn->usesspi = 1;

	return pg_SSPI_continue(PQerrormsg, conn);
}
#endif /* ENABLE_SSPI */

/*
 * Respond to AUTH_REQ_SCM_CREDS challenge.
 *
 * Note: current backends will not use this challenge if HAVE_GETPEEREID
 * or SO_PEERCRED is defined, but pre-7.4 backends might, so compile the
 * code anyway.
 */
static int
pg_local_sendauth(char *PQerrormsg, PGconn *conn)
{
#if defined(HAVE_STRUCT_CMSGCRED) || defined(HAVE_STRUCT_FCRED) || \
	(defined(HAVE_STRUCT_SOCKCRED) && defined(LOCAL_CREDS))
	char		buf;
	struct iovec iov;
	struct msghdr msg;

#ifdef HAVE_STRUCT_CMSGCRED
	/* Prevent padding */
	char		cmsgmem[sizeof(struct cmsghdr) + sizeof(struct cmsgcred)];

	/* Point to start of first structure */
	struct cmsghdr *cmsg = (struct cmsghdr *) cmsgmem;
#endif

	/*
	 * The backend doesn't care what we send here, but it wants exactly one
	 * character to force recvmsg() to block and wait for us.
	 */
	buf = '\0';
	iov.iov_base = &buf;
	iov.iov_len = 1;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

#ifdef HAVE_STRUCT_CMSGCRED
	/* Create control header, FreeBSD */
	msg.msg_control = cmsg;
	msg.msg_controllen = sizeof(cmsgmem);
	memset(cmsg, 0, sizeof(cmsgmem));
	cmsg->cmsg_len = sizeof(cmsgmem);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_CREDS;
#endif

	if (sendmsg(conn->sock, &msg, 0) == -1)
	{
		char		sebuf[256];

		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_local_sendauth: sendmsg: %s\n",
				 pqStrerror(errno, sebuf, sizeof(sebuf)));
		return STATUS_ERROR;
	}
	return STATUS_OK;
#else
	snprintf(PQerrormsg, PQERRORMSG_LENGTH,
			 libpq_gettext("SCM_CRED authentication method not supported\n"));
	return STATUS_ERROR;
#endif
}

static int
pg_password_sendauth(PGconn *conn, const char *password, AuthRequest areq)
{
	int			ret;
	char	   *crypt_pwd;

	/* Encrypt the password if needed. */

	switch (areq)
	{
		case AUTH_REQ_MD5:
			{
				char	   *crypt_pwd2;

				/* Allocate enough space for two MD5 hashes */
				crypt_pwd = malloc(2 * (MD5_PASSWD_LEN + 1));
				if (!crypt_pwd)
				{
					fprintf(stderr, libpq_gettext("out of memory\n"));
					return STATUS_ERROR;
				}

				crypt_pwd2 = crypt_pwd + MD5_PASSWD_LEN + 1;
				if (!pg_md5_encrypt(password, conn->pguser,
									strlen(conn->pguser), crypt_pwd2))
				{
					free(crypt_pwd);
					return STATUS_ERROR;
				}
				if (!pg_md5_encrypt(crypt_pwd2 + strlen("md5"), conn->md5Salt,
									sizeof(conn->md5Salt), crypt_pwd))
				{
					free(crypt_pwd);
					return STATUS_ERROR;
				}
				break;
			}
		case AUTH_REQ_CRYPT:
			{
				char		salt[3];

				strlcpy(salt, conn->cryptSalt, sizeof(salt));
				crypt_pwd = crypt(password, salt);
				break;
			}
		case AUTH_REQ_PASSWORD:
			/* discard const so we can assign it */
			crypt_pwd = (char *) password;
			break;
		default:
			return STATUS_ERROR;
	}
	/* Packet has a message type as of protocol 3.0 */
	if (PG_PROTOCOL_MAJOR(conn->pversion) >= 3)
		ret = pqPacketSend(conn, 'p', crypt_pwd, strlen(crypt_pwd) + 1);
	else
		ret = pqPacketSend(conn, 0, crypt_pwd, strlen(crypt_pwd) + 1);
	if (areq == AUTH_REQ_MD5)
		free(crypt_pwd);
	return ret;
}

/*
 * pg_fe_sendauth
 *		client demux routine for outgoing authentication information
 */
int
pg_fe_sendauth(AuthRequest areq, PGconn *conn, const char *hostname,
			   const char *password, char *PQerrormsg)
{
#ifndef KRB5
	(void) hostname;			/* not used */
#endif

	switch (areq)
	{
		case AUTH_REQ_OK:
			break;

		case AUTH_REQ_KRB4:
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 libpq_gettext("Kerberos 4 authentication not supported\n"));
			return STATUS_ERROR;

		case AUTH_REQ_KRB5:
#ifdef KRB5
			pglock_thread();
			if (pg_krb5_sendauth(PQerrormsg, conn->sock,
								 hostname, conn->krbsrvname) != STATUS_OK)
			{
				/* PQerrormsg already filled in */
				pgunlock_thread();
				return STATUS_ERROR;
			}
			pgunlock_thread();
			break;
#else
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 libpq_gettext("Kerberos 5 authentication not supported\n"));
			return STATUS_ERROR;
#endif

#if defined(ENABLE_GSS) || defined(ENABLE_SSPI)
		case AUTH_REQ_GSS:
			{
				int r;
				pglock_thread();
				/*
				 * If we have both GSS and SSPI support compiled in, use SSPI
				 * support by default. This is overridable by a connection string parameter.
				 * Note that when using SSPI we still leave the negotiate parameter off,
				 * since we want SSPI to use the GSSAPI kerberos protocol. For actual
				 * SSPI negotiate protocol, we use AUTH_REQ_SSPI.
				 */
#if defined(ENABLE_GSS) && defined(ENABLE_SSPI)
				if (conn->gsslib && (pg_strcasecmp(conn->gsslib, "gssapi") == 0))
					r = pg_GSS_startup(PQerrormsg, conn);
				else
					r = pg_SSPI_startup(PQerrormsg, conn, 0);
#elif defined(ENABLE_GSS) && !defined(ENABLE_SSPI)
				r = pg_GSS_startup(PQerrormsg, conn);
#elif !defined(ENABLE_GSS) && defined(ENABLE_SSPI)
				r = pg_SSPI_startup(PQerrormsg, conn, 0);
#endif
				if (r != STATUS_OK)
				{
					/* PQerrormsg already filled in. */
					pgunlock_thread();
					return STATUS_ERROR;
				}
				pgunlock_thread();
			}
			break;

		case AUTH_REQ_GSS_CONT:
			{
				int r;
				pglock_thread();
#if defined(ENABLE_GSS) && defined(ENABLE_SSPI)
				if (conn->usesspi)
					r = pg_SSPI_continue(PQerrormsg, conn);
				else
					r = pg_GSS_continue(PQerrormsg, conn);
#elif defined(ENABLE_GSS) && !defined(ENABLE_SSPI)
				r = pg_GSS_continue(PQerrormsg, conn);
#elif !defined(ENABLE_GSS) && defined(ENABLE_SSPI)
				r = pg_SSPI_continue(PQerrormsg, conn);
#endif
				if (r != STATUS_OK)
				{
					/* PQerrormsg already filled in. */
					pgunlock_thread();
					return STATUS_ERROR;
				}
				pgunlock_thread();
			}
			break;

#else
		case AUTH_REQ_GSS:
		case AUTH_REQ_GSS_CONT:
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
					libpq_gettext("GSSAPI authentication not supported\n"));
			return STATUS_ERROR;
#endif

#ifdef ENABLE_SSPI
		case AUTH_REQ_SSPI:
			/* 
			 * SSPI has it's own startup message so libpq can decide which
			 * method to use. Indicate to pg_SSPI_startup that we want
			 * SSPI negotiation instead of Kerberos.
			 */
			pglock_thread();
			if (pg_SSPI_startup(PQerrormsg, conn, 1) != STATUS_OK)
			{
				/* PQerrormsg already filled in. */
				pgunlock_thread();
				return STATUS_ERROR;
			}
			pgunlock_thread();
			break;
#else
		case AUTH_REQ_SSPI:
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
					libpq_gettext("SSPI authentication not supported\n"));
			return STATUS_ERROR;
#endif


		case AUTH_REQ_MD5:
		case AUTH_REQ_CRYPT:
		case AUTH_REQ_PASSWORD:
			if (password == NULL || *password == '\0')
			{
				(void) snprintf(PQerrormsg, PQERRORMSG_LENGTH,
								PQnoPasswordSupplied);
				return STATUS_ERROR;
			}
			if (pg_password_sendauth(conn, password, areq) != STATUS_OK)
			{
				(void) snprintf(PQerrormsg, PQERRORMSG_LENGTH,
					 "fe_sendauth: error sending password authentication\n");
				return STATUS_ERROR;
			}
			break;

		case AUTH_REQ_SCM_CREDS:
			if (pg_local_sendauth(PQerrormsg, conn) != STATUS_OK)
				return STATUS_ERROR;
			break;

		default:
			snprintf(PQerrormsg, PQERRORMSG_LENGTH,
			libpq_gettext("authentication method %u not supported\n"), areq);
			return STATUS_ERROR;
	}

	return STATUS_OK;
}


/*
 * pg_fe_getauthname -- returns a pointer to dynamic space containing whatever
 *					 name the user has authenticated to the system
 *
 * if there is an error, return NULL with an error message in PQerrormsg
 */
char *
pg_fe_getauthname(char *PQerrormsg)
{
#ifdef KRB5
	char	   *krb5_name = NULL;
#endif
	const char *name = NULL;
	char	   *authn;

#ifdef WIN32
	char		username[128];
	DWORD		namesize = sizeof(username) - 1;
#else
	char		pwdbuf[BUFSIZ];
	struct passwd pwdstr;
	struct passwd *pw = NULL;
#endif

	/*
	 * pglock_thread() really only needs to be called around
	 * pg_krb5_authname(), but some users are using configure
	 * --enable-thread-safety-force, so we might as well do the locking within
	 * our library to protect pqGetpwuid(). In fact, application developers
	 * can use getpwuid() in their application if they use the locking call we
	 * provide, or install their own locking function using
	 * PQregisterThreadLock().
	 */
	pglock_thread();

#ifdef KRB5

	/*
	 * pg_krb5_authname gives us a strdup'd value that we need to free later,
	 * however, we don't want to free 'name' directly in case it's *not* a
	 * Kerberos login and we fall through to name = pw->pw_name;
	 */
	krb5_name = pg_krb5_authname(PQerrormsg);
	name = krb5_name;
#endif

	if (!name)
	{
#ifdef WIN32
		if (GetUserName(username, &namesize))
			name = username;
#else
		if (pqGetpwuid(geteuid(), &pwdstr, pwdbuf, sizeof(pwdbuf), &pw) == 0)
			name = pw->pw_name;
#endif
	}

	authn = name ? strdup(name) : NULL;

#ifdef KRB5
	/* Free the strdup'd string from pg_krb5_authname, if we got one */
	if (krb5_name)
		free(krb5_name);
#endif

	pgunlock_thread();

	return authn;
}


/*
 * PQencryptPassword -- exported routine to encrypt a password
 *
 * This is intended to be used by client applications that wish to send
 * commands like ALTER USER joe PASSWORD 'pwd'.  The password need not
 * be sent in cleartext if it is encrypted on the client side.	This is
 * good because it ensures the cleartext password won't end up in logs,
 * pg_stat displays, etc.  We export the function so that clients won't
 * be dependent on low-level details like whether the enceyption is MD5
 * or something else.
 *
 * Arguments are the cleartext password, and the SQL name of the user it
 * is for.
 *
 * Return value is a malloc'd string, or NULL if out-of-memory.  The client
 * may assume the string doesn't contain any special characters that would
 * require escaping.
 */
char *
PQencryptPassword(const char *passwd, const char *user)
{
	char	   *crypt_pwd;

	crypt_pwd = malloc(MD5_PASSWD_LEN + 1);
	if (!crypt_pwd)
		return NULL;

	if (!pg_md5_encrypt(passwd, user, strlen(user), crypt_pwd))
	{
		free(crypt_pwd);
		return NULL;
	}

	return crypt_pwd;
}
