/*-------------------------------------------------------------------------
 *
 * ip.h
 *	  Definitions for IPv6-aware network access.
 *
 * Copyright (c) 2003, PostgreSQL Global Development Group
 *
 * $Id: ip.h,v 1.9 2003-07-23 23:30:41 tgl Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef IP_H
#define IP_H

#include "getaddrinfo.h"
#include "libpq/pqcomm.h"


extern int   getaddrinfo_all(const char *hostname, const char *servname,
							 const struct addrinfo *hintp,
							 struct addrinfo **result);
extern void  freeaddrinfo_all(int hint_ai_family, struct addrinfo *ai);

extern int getnameinfo_all(const struct sockaddr_storage *addr, int salen,
						   char *node, int nodelen,
						   char *service, int servicelen,
						   int flags);

extern int   rangeSockAddr(const struct sockaddr_storage *addr,
			const struct sockaddr_storage *netaddr,
			const struct sockaddr_storage *netmask);

extern int SockAddr_cidr_mask(struct sockaddr_storage **mask,
				char *numbits, int family);

#ifdef	HAVE_UNIX_SOCKETS
#define	IS_AF_UNIX(fam)	((fam) == AF_UNIX)
#else
#define	IS_AF_UNIX(fam) (0)
#endif

#endif /* IP_H */
