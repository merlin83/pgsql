/*-------------------------------------------------------------------------
 *
 * auth.c
 *	  Routines to handle network authentication
 *
 * Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/backend/libpq/auth.c,v 1.52 2001-03-22 03:59:30 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
/*
 * INTERFACE ROUTINES
 *
 *	   backend (postmaster) routines:
 *		be_recvauth				receive authentication information
 */
#include <sys/param.h>			/* for MAXHOSTNAMELEN on most */
#ifndef  MAXHOSTNAMELEN
#include <netdb.h>				/* for MAXHOSTNAMELEN on some */
#endif
#include <pwd.h>
#include <ctype.h>

#include <sys/types.h>			/* needed by in.h on Ultrix */
#include <netinet/in.h>
#include <arpa/inet.h>

#include "postgres.h"

#include "libpq/auth.h"
#include "libpq/crypt.h"
#include "libpq/hba.h"
#include "libpq/libpq.h"
#include "libpq/password.h"
#include "miscadmin.h"

static void sendAuthRequest(Port *port, AuthRequest areq, PacketDoneProc handler);
static int	handle_done_auth(void *arg, PacketLen len, void *pkt);
static int	handle_krb4_auth(void *arg, PacketLen len, void *pkt);
static int	handle_krb5_auth(void *arg, PacketLen len, void *pkt);
static int	handle_password_auth(void *arg, PacketLen len, void *pkt);
static int	readPasswordPacket(void *arg, PacketLen len, void *pkt);
static int	pg_passwordv0_recvauth(void *arg, PacketLen len, void *pkt);
static int	checkPassword(Port *port, char *user, char *password);
static int	old_be_recvauth(Port *port);
static int	map_old_to_new(Port *port, UserAuth old, int status);
static void auth_failed(Port *port);


char	   *pg_krb_server_keyfile;


#ifdef KRB4
/*----------------------------------------------------------------
 * MIT Kerberos authentication system - protocol version 4
 *----------------------------------------------------------------
 */

#include "krb.h"

/*
 * pg_krb4_recvauth -- server routine to receive authentication information
 *					   from the client
 *
 * Nothing unusual here, except that we compare the username obtained from
 * the client's setup packet to the authenticated name.  (We have to retain
 * the name in the setup packet since we have to retain the ability to handle
 * unauthenticated connections.)
 */
static int
pg_krb4_recvauth(Port *port)
{
	long		krbopts = 0;	/* one-way authentication */
	KTEXT_ST	clttkt;
	char		instance[INST_SZ + 1],
				version[KRB_SENDAUTH_VLEN + 1];
	AUTH_DAT	auth_data;
	Key_schedule key_sched;
	int			status;

	strcpy(instance, "*");		/* don't care, but arg gets expanded
								 * anyway */
	status = krb_recvauth(krbopts,
						  port->sock,
						  &clttkt,
						  PG_KRB_SRVNAM,
						  instance,
						  &port->raddr.in,
						  &port->laddr.in,
						  &auth_data,
						  pg_krb_server_keyfile,
						  key_sched,
						  version);
	if (status != KSUCCESS)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb4_recvauth: kerberos error: %s\n",
				 krb_err_txt[status]);
		fputs(PQerrormsg, stderr);
		pqdebug("%s", PQerrormsg);
		return STATUS_ERROR;
	}
	if (strncmp(version, PG_KRB4_VERSION, KRB_SENDAUTH_VLEN))
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb4_recvauth: protocol version != \"%s\"\n",
				 PG_KRB4_VERSION);
		fputs(PQerrormsg, stderr);
		pqdebug("%s", PQerrormsg);
		return STATUS_ERROR;
	}
	if (strncmp(port->user, auth_data.pname, SM_USER))
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb4_recvauth: name \"%s\" != \"%s\"\n",
				 port->user, auth_data.pname);
		fputs(PQerrormsg, stderr);
		pqdebug("%s", PQerrormsg);
		return STATUS_ERROR;
	}
	return STATUS_OK;
}

#else
static int
pg_krb4_recvauth(Port *port)
{
	snprintf(PQerrormsg, PQERRORMSG_LENGTH,
		 "pg_krb4_recvauth: Kerberos not implemented on this server.\n");
	fputs(PQerrormsg, stderr);
	pqdebug("%s", PQerrormsg);

	return STATUS_ERROR;
}

#endif	 /* KRB4 */


#ifdef KRB5
/*----------------------------------------------------------------
 * MIT Kerberos authentication system - protocol version 5
 *----------------------------------------------------------------
 */

#include <krb5.h>
#include <com_err.h>

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
 */
static char *
pg_an_to_ln(char *aname)
{
	char	   *p;

	if ((p = strchr(aname, '/')) || (p = strchr(aname, '@')))
		*p = '\0';
	return aname;
}


/*
 * Various krb5 state which is not connection specfic, and a flag to
 * indicate whether we have initialised it yet.
 */
static int	pg_krb5_initialised;
static krb5_context pg_krb5_context;
static krb5_keytab pg_krb5_keytab;
static krb5_principal pg_krb5_server;


static int
pg_krb5_init(void)
{
	krb5_error_code retval;

	if (pg_krb5_initialised)
		return STATUS_OK;

	retval = krb5_init_context(&pg_krb5_context);
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_init: krb5_init_context returned"
				 " Kerberos error %d\n", retval);
		com_err("postgres", retval, "while initializing krb5");
		return STATUS_ERROR;
	}

	retval = krb5_kt_resolve(pg_krb5_context, pg_krb_server_keyfile, &pg_krb5_keytab);
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_init: krb5_kt_resolve returned"
				 " Kerberos error %d\n", retval);
		com_err("postgres", retval, "while resolving keytab file %s",
				pg_krb_server_keyfile);
		krb5_free_context(pg_krb5_context);
		return STATUS_ERROR;
	}

	retval = krb5_sname_to_principal(pg_krb5_context, NULL, PG_KRB_SRVNAM,
									 KRB5_NT_SRV_HST, &pg_krb5_server);
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_init: krb5_sname_to_principal returned"
				 " Kerberos error %d\n", retval);
		com_err("postgres", retval,
				"while getting server principal for service %s",
				pg_krb_server_keyfile);
		krb5_kt_close(pg_krb5_context, pg_krb5_keytab);
		krb5_free_context(pg_krb5_context);
		return STATUS_ERROR;
	}

	pg_krb5_initialised = 1;
	return STATUS_OK;
}


/*
 * pg_krb5_recvauth -- server routine to receive authentication information
 *					   from the client
 *
 * We still need to compare the username obtained from the client's setup
 * packet to the authenticated name, as described in pg_krb4_recvauth.	This
 * is a bit more problematic in v5, as described above in pg_an_to_ln.
 *
 * We have our own keytab file because postgres is unlikely to run as root,
 * and so cannot read the default keytab.
 */
static int
pg_krb5_recvauth(Port *port)
{
	krb5_error_code retval;
	int			ret;
	krb5_auth_context auth_context = NULL;
	krb5_ticket *ticket;
	char	   *kusername;

	ret = pg_krb5_init();
	if (ret != STATUS_OK)
		return ret;

	retval = krb5_recvauth(pg_krb5_context, &auth_context,
						   (krb5_pointer) & port->sock, PG_KRB_SRVNAM,
						   pg_krb5_server, 0, pg_krb5_keytab, &ticket);
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_recvauth: krb5_recvauth returned"
				 " Kerberos error %d\n", retval);
		com_err("postgres", retval, "from krb5_recvauth");
		return STATUS_ERROR;
	}

	/*
	 * The "client" structure comes out of the ticket and is therefore
	 * authenticated.  Use it to check the username obtained from the
	 * postmaster startup packet.
	 *
	 * I have no idea why this is considered necessary.
	 */
	retval = krb5_unparse_name(pg_krb5_context,
							   ticket->enc_part2->client, &kusername);
	if (retval)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_krb5_recvauth: krb5_unparse_name returned"
				 " Kerberos error %d\n", retval);
		com_err("postgres", retval, "while unparsing client name");
		krb5_free_ticket(pg_krb5_context, ticket);
		krb5_auth_con_free(pg_krb5_context, auth_context);
		return STATUS_ERROR;
	}

	kusername = pg_an_to_ln(kusername);
	if (strncmp(port->user, kusername, SM_USER))
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
			  "pg_krb5_recvauth: user name \"%s\" != krb5 name \"%s\"\n",
				 port->user, kusername);
		ret = STATUS_ERROR;
	}
	else
		ret = STATUS_OK;

	krb5_free_ticket(pg_krb5_context, ticket);
	krb5_auth_con_free(pg_krb5_context, auth_context);
	free(kusername);

	return ret;
}

#else
static int
pg_krb5_recvauth(Port *port)
{
	snprintf(PQerrormsg, PQERRORMSG_LENGTH,
		 "pg_krb5_recvauth: Kerberos not implemented on this server.\n");
	fputs(PQerrormsg, stderr);
	pqdebug("%s", PQerrormsg);

	return STATUS_ERROR;
}

#endif	 /* KRB5 */


/*
 * Handle a v0 password packet.
 */

static int
pg_passwordv0_recvauth(void *arg, PacketLen len, void *pkt)
{
	Port	   *port;
	PasswordPacketV0 *pp;
	char	   *user,
			   *password,
			   *cp,
			   *start;

	port = (Port *) arg;
	pp = (PasswordPacketV0 *) pkt;

	/*
	 * The packet is supposed to comprise the user name and the password
	 * as C strings.  Be careful the check that this is the case.
	 */

	user = password = NULL;

	len -= sizeof(pp->unused);

	cp = start = pp->data;

	while (len-- > 0)
		if (*cp++ == '\0')
		{
			if (user == NULL)
				user = start;
			else
			{
				password = start;
				break;
			}

			start = cp;
		}

	if (user == NULL || password == NULL)
	{
		snprintf(PQerrormsg, PQERRORMSG_LENGTH,
				 "pg_password_recvauth: badly formed password packet.\n");
		fputs(PQerrormsg, stderr);
		pqdebug("%s", PQerrormsg);

		auth_failed(port);
	}
	else
	{
		int			status;
		UserAuth	saved;

		/* Check the password. */

		saved = port->auth_method;
		port->auth_method = uaPassword;

		status = checkPassword(port, user, password);

		port->auth_method = saved;

		/* Adjust the result if necessary. */

		if (map_old_to_new(port, uaPassword, status) != STATUS_OK)
			auth_failed(port);
	}

	return STATUS_OK;			/* don't close the connection yet */
}


/*
 * Tell the user the authentication failed, but not (much about) why.
 *
 * There is a tradeoff here between security concerns and making life
 * unnecessarily difficult for legitimate users.  We would not, for example,
 * want to report the password we were expecting to receive...
 * But it seems useful to report the username and authorization method
 * in use, and these are items that must be presumed known to an attacker
 * anyway.
 * Note that many sorts of failure report additional information in the
 * postmaster log, which we hope is only readable by good guys.
 */

static void
auth_failed(Port *port)
{
	char		buffer[512];
	const char *authmethod = "Unknown auth method:";

	switch (port->auth_method)
	{
		case uaReject:
			authmethod = "Rejected host:";
			break;
		case uaKrb4:
			authmethod = "Kerberos4";
			break;
		case uaKrb5:
			authmethod = "Kerberos5";
			break;
		case uaTrust:
			authmethod = "Trusted";
			break;
		case uaIdent:
			authmethod = "IDENT";
			break;
		case uaPassword:
			authmethod = "Password";
			break;
		case uaCrypt:
			authmethod = "Password";
			break;
	}

	sprintf(buffer, "%s authentication failed for user '%s'",
			authmethod, port->user);

	PacketSendError(&port->pktInfo, buffer);
}


/*
 * be_recvauth -- server demux routine for incoming authentication information
 */
void
be_recvauth(Port *port)
{

	/*
	 * Get the authentication method to use for this frontend/database
	 * combination.  Note: a failure return indicates a problem with the
	 * hba config file, not with the request.  hba.c should have dropped
	 * an error message into the postmaster logfile if it failed.
	 */

	if (hba_getauthmethod(port) != STATUS_OK)
		PacketSendError(&port->pktInfo,
						"Missing or erroneous pg_hba.conf file, see postmaster log for details");

	else if (PG_PROTOCOL_MAJOR(port->proto) == 0)
	{
		/* Handle old style authentication. */

		if (old_be_recvauth(port) != STATUS_OK)
			auth_failed(port);
	}
	else
	{
		/* Handle new style authentication. */

		AuthRequest areq = AUTH_REQ_OK;
		PacketDoneProc auth_handler = NULL;

		switch (port->auth_method)
		{
			case uaReject:

				/*
				 * This could have come from an explicit "reject" entry in
				 * pg_hba.conf, but more likely it means there was no
				 * matching entry.	Take pity on the poor user and issue a
				 * helpful error message.  NOTE: this is not a security
				 * breach, because all the info reported here is known at
				 * the frontend and must be assumed known to bad guys.
				 * We're merely helping out the less clueful good guys.
				 * NOTE 2: libpq-be.h defines the maximum error message
				 * length as 99 characters.  It probably wouldn't hurt
				 * anything to increase it, but there might be some client
				 * out there that will fail.  So, be terse.
				 */
				{
					char		buffer[512];
					const char *hostinfo = "localhost";

					if (port->raddr.sa.sa_family == AF_INET)
						hostinfo = inet_ntoa(port->raddr.in.sin_addr);
					sprintf(buffer,
							"No pg_hba.conf entry for host %s, user %s, database %s",
							hostinfo, port->user, port->database);
					PacketSendError(&port->pktInfo, buffer);
					return;
				}
				break;

			case uaKrb4:
				areq = AUTH_REQ_KRB4;
				auth_handler = handle_krb4_auth;
				break;

			case uaKrb5:
				areq = AUTH_REQ_KRB5;
				auth_handler = handle_krb5_auth;
				break;

			case uaTrust:
				areq = AUTH_REQ_OK;
				auth_handler = handle_done_auth;
				break;

			case uaIdent:
				if (authident(&port->raddr.in, &port->laddr.in,
							  port->user, port->auth_arg) == STATUS_OK)
				{
					areq = AUTH_REQ_OK;
					auth_handler = handle_done_auth;
				}

				break;

			case uaPassword:
				areq = AUTH_REQ_PASSWORD;
				auth_handler = handle_password_auth;
				break;

			case uaCrypt:
				areq = AUTH_REQ_CRYPT;
				auth_handler = handle_password_auth;
				break;
		}

		/* Tell the frontend what we want next. */

		if (auth_handler != NULL)
			sendAuthRequest(port, areq, auth_handler);
		else
			auth_failed(port);
	}
}


/*
 * Send an authentication request packet to the frontend.
 */

static void
sendAuthRequest(Port *port, AuthRequest areq, PacketDoneProc handler)
{
	char	   *dp,
			   *sp;
	int			i;
	uint32		net_areq;

	/* Convert to a byte stream. */

	net_areq = htonl(areq);

	dp = port->pktInfo.pkt.ar.data;
	sp = (char *) &net_areq;

	*dp++ = 'R';

	for (i = 1; i <= 4; ++i)
		*dp++ = *sp++;

	/* Add the salt for encrypted passwords. */

	if (areq == AUTH_REQ_CRYPT)
	{
		*dp++ = port->salt[0];
		*dp++ = port->salt[1];
		i += 2;
	}

	PacketSendSetup(&port->pktInfo, i, handler, (void *) port);
}


/*
 * Called when we have told the front end that it is authorised.
 */

static int
handle_done_auth(void *arg, PacketLen len, void *pkt)
{

	/*
	 * Don't generate any more traffic.  This will cause the backend to
	 * start.
	 */

	return STATUS_OK;
}


/*
 * Called when we have told the front end that it should use Kerberos V4
 * authentication.
 */

static int
handle_krb4_auth(void *arg, PacketLen len, void *pkt)
{
	Port	   *port = (Port *) arg;

	if (pg_krb4_recvauth(port) != STATUS_OK)
		auth_failed(port);
	else
		sendAuthRequest(port, AUTH_REQ_OK, handle_done_auth);

	return STATUS_OK;
}


/*
 * Called when we have told the front end that it should use Kerberos V5
 * authentication.
 */

static int
handle_krb5_auth(void *arg, PacketLen len, void *pkt)
{
	Port	   *port = (Port *) arg;

	if (pg_krb5_recvauth(port) != STATUS_OK)
		auth_failed(port);
	else
		sendAuthRequest(port, AUTH_REQ_OK, handle_done_auth);

	return STATUS_OK;
}


/*
 * Called when we have told the front end that it should use password
 * authentication.
 */

static int
handle_password_auth(void *arg, PacketLen len, void *pkt)
{
	Port	   *port = (Port *) arg;

	/* Set up the read of the password packet. */

	PacketReceiveSetup(&port->pktInfo, readPasswordPacket, (void *) port);

	return STATUS_OK;
}


/*
 * Called when we have received the password packet.
 */

static int
readPasswordPacket(void *arg, PacketLen len, void *pkt)
{
	char		password[sizeof(PasswordPacket) + 1];
	Port	   *port = (Port *) arg;

	/* Silently truncate a password that is too big. */

	if (len > sizeof(PasswordPacket))
		len = sizeof(PasswordPacket);

	StrNCpy(password, ((PasswordPacket *) pkt)->passwd, len);

	if (checkPassword(port, port->user, password) != STATUS_OK)
		auth_failed(port);
	else
		sendAuthRequest(port, AUTH_REQ_OK, handle_done_auth);

	return STATUS_OK;			/* don't close the connection yet */
}


/*
 * Handle `password' and `crypt' records. If an auth argument was
 * specified, use the respective file. Else use pg_shadow passwords.
 */
static int
checkPassword(Port *port, char *user, char *password)
{
	if (port->auth_arg[0] != '\0')
		return verify_password(port, user, password);

	return crypt_verify(port, user, password);
}


/*
 * Server demux routine for incoming authentication information for protocol
 * version 0.
 */
static int
old_be_recvauth(Port *port)
{
	int			status;
	MsgType		msgtype = (MsgType) port->proto;

	/* Handle the authentication that's offered. */

	switch (msgtype)
	{
		case STARTUP_KRB4_MSG:
			status = map_old_to_new(port, uaKrb4, pg_krb4_recvauth(port));
			break;

		case STARTUP_KRB5_MSG:
			status = map_old_to_new(port, uaKrb5, pg_krb5_recvauth(port));
			break;

		case STARTUP_MSG:
			status = map_old_to_new(port, uaTrust, STATUS_OK);
			break;

		case STARTUP_PASSWORD_MSG:
			PacketReceiveSetup(&port->pktInfo, pg_passwordv0_recvauth,
							   (void *) port);

			return STATUS_OK;

		default:
			fprintf(stderr, "Invalid startup message type: %u\n", msgtype);

			return STATUS_OK;
	}

	return status;
}


/*
 * The old style authentication has been done.	Modify the result of this (eg.
 * allow the connection anyway, disallow it anyway, or use the result)
 * depending on what authentication we really want to use.
 */

static int
map_old_to_new(Port *port, UserAuth old, int status)
{
	switch (port->auth_method)
	{
			case uaCrypt:
			case uaReject:
			status = STATUS_ERROR;
			break;

		case uaKrb4:
			if (old != uaKrb4)
				status = STATUS_ERROR;
			break;

		case uaKrb5:
			if (old != uaKrb5)
				status = STATUS_ERROR;
			break;

		case uaTrust:
			status = STATUS_OK;
			break;

		case uaIdent:
			status = authident(&port->raddr.in, &port->laddr.in,
							   port->user, port->auth_arg);
			break;

		case uaPassword:
			if (old != uaPassword)
				status = STATUS_ERROR;

			break;
	}

	return status;
}
