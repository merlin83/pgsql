/*-------------------------------------------------------------------------
 *
 * hba.h
 *	  Interface to hba.c
 *
 *
 * $Id: hba.h,v 1.23 2001-08-15 18:42:15 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef HBA_H
#define HBA_H

#include <netinet/in.h>

#define CONF_FILE "pg_hba.conf"
 /* Name of the config file  */

#define USERMAP_FILE "pg_ident.conf"
 /* Name of the usermap file */

#define OLD_CONF_FILE "pg_hba"
 /* Name of the config file in prior releases of Postgres. */

#define IDENT_PORT 113
 /* Standard TCP port number for Ident service.  Assigned by IANA */

#define MAX_AUTH_ARG	80		/* Max size of an authentication arg */

typedef enum UserAuth
{
	uaReject,
	uaKrb4,
	uaKrb5,
	uaTrust,
	uaIdent,
	uaPassword,
	uaCrypt,
	uaMD5		/* 	This starts as uaCrypt from pg_hba.conf, but gets 
					overridden if the client supports MD5 */
} UserAuth;

typedef struct Port hbaPort;

extern int hba_getauthmethod(hbaPort *port);
extern int authident(hbaPort *port);
extern void load_hba_and_ident(void);

#endif
