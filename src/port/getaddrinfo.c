/*-------------------------------------------------------------------------
 *
 * getaddrinfo.c
 *	  Support getaddrinfo() on platforms that don't have it.
 *
 *
 * Copyright (c) 2003, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  $Header: /home/rubik/work/pgcvs/CVSROOT/pgsql/src/port/getaddrinfo.c,v 1.7 2003-06-12 08:15:29 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

/* This is intended to be used in both frontend and backend, so use c.h */
#include "c.h"

#if !defined(_MSC_VER) && !defined(__BORLANDC__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef	HAVE_UNIX_SOCKETS
#include <sys/un.h>
#endif
#endif

#include "getaddrinfo.h"

/*
 * get address info for ipv4 sockets.
 *
 *	Bugs:	- only one addrinfo is set even though hintp is NULL or
 *		  ai_socktype is 0
 *		- AI_CANONNAME is not supported.
 *		- servname can only be a number, not text.
 */
int
getaddrinfo(const char *node, const char *service,
			const struct addrinfo *hintp,
			struct addrinfo **res)
{
	struct addrinfo		*ai;
	struct sockaddr_in	sin, *psin;
	struct addrinfo		hints;

	if (hintp == NULL)	
	{
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
	}
	else
	{
		memcpy(&hints, hintp, sizeof(hints));
	}

	if (hints.ai_family != AF_INET && hints.ai_family != AF_UNSPEC)
		return EAI_FAMILY;

	if (hints.ai_socktype == 0)
		hints.ai_socktype = SOCK_STREAM;

	if (!node && !service)
		return EAI_NONAME;

	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;

	if (node)
	{
		if (node[0] == '\0')
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
		else if (hints.ai_flags & AI_NUMERICHOST)
		{
			if (!inet_aton(node, &sin.sin_addr))
			{
				return EAI_FAIL;
			}
		}
		else
		{
			struct hostent *hp;

			hp = gethostbyname(node);
			if (hp == NULL)
			{
				switch (h_errno)
				{
					case HOST_NOT_FOUND:
					case NO_DATA:
						return EAI_NONAME;
					case TRY_AGAIN:
						return EAI_AGAIN;
					case NO_RECOVERY:
					default:
						return EAI_FAIL;
				}
			}
			if (hp->h_addrtype != AF_INET)
				return EAI_FAIL;

			memcpy(&(sin.sin_addr), hp->h_addr, hp->h_length);
		}
	}
	else
	{
		if (hints.ai_flags & AI_PASSIVE)
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
		else
			sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}

	if (service)
		sin.sin_port = htons((unsigned short) atoi(service));
#if SALEN
        sin.sin_len = sizeof(sin);
#endif

	ai = malloc(sizeof(*ai));
	if (!ai)
	{
		return EAI_MEMORY;
	}

	psin = malloc(sizeof(*psin));
	if (!psin)
	{
		free(ai);
		return EAI_MEMORY;
	}

	memcpy(psin, &sin, sizeof(*psin));

	ai->ai_flags = 0;
	ai->ai_family = AF_INET;
	ai->ai_socktype = hints.ai_socktype;
	ai->ai_protocol = hints.ai_protocol;
	ai->ai_addrlen = sizeof(*psin);
	ai->ai_addr = (struct sockaddr *) psin;
	ai->ai_canonname = NULL;
	ai->ai_next = NULL;

	*res = ai;

	return 0;
}


void
freeaddrinfo(struct addrinfo *res)
{
	if (res)
	{
		if (res->ai_addr)
			free(res->ai_addr);
		free(res);
	}
}


const char *
gai_strerror(int errcode)
{
#ifdef HAVE_HSTRERROR
	int hcode;

	switch (errcode)
	{
		case EAI_NONAME:
			hcode = HOST_NOT_FOUND;
			break;
		case EAI_AGAIN:
			hcode = TRY_AGAIN;
			break;
		case EAI_FAIL:
		default:
			hcode = NO_RECOVERY;
			break;
	}

	return hstrerror(hcode);

#else /* !HAVE_HSTRERROR */

	switch (errcode)
	{
		case EAI_NONAME:
			return "Unknown host";
		case EAI_AGAIN:
			return "Host name lookup failure";
		case EAI_FAIL:
		default:
			return "Unknown server error";
	}

#endif /* HAVE_HSTRERROR */
}

/*
 * Convert an address to a hostname.
 * 
 * Bugs:	- Only supports NI_NUMERICHOST and NI_NUMERICSERV
 *		  It will never resolv a hostname.
 *		- No IPv6 support.
 */
int
getnameinfo(const struct sockaddr *sa, int salen,
		char *node, int nodelen,
		char *service, int servicelen, int flags)
{
	sa_family_t	family;
	int		ret = -1;

	/* Invalid arguments. */
	if (sa == NULL || (node == NULL && service == NULL))
	{
		return EAI_FAIL;
	}

	/* We don't support those. */
	if ((node && !(flags & NI_NUMERICHOST))
		|| (service && !(flags & NI_NUMERICSERV)))
	{
		return EAI_FAIL;
	}

	family = sa->sa_family;
#ifdef	HAVE_IPV6
	if (family == AF_INET6)
	{
		return	EAI_FAMILY;
	}
#endif

	if (service)
	{
		if (family == AF_INET)
		{
			ret = snprintf(service, servicelen, "%d",
				ntohs(((struct sockaddr_in *)sa)->sin_port));
		}
#ifdef	HAVE_UNIX_SOCKETS
		else if (family == AF_UNIX)
		{
			ret = snprintf(service, servicelen, "%s",
				((struct sockaddr_un *)sa)->sun_path);
		}
#endif
		if (ret == -1 || ret > servicelen)
		{
			return EAI_MEMORY;
		}
	}

	if (node)
	{
		if (family == AF_INET)
		{
			char	*p;
			p = inet_ntoa(((struct sockaddr_in *)sa)->sin_addr);
			ret = snprintf(node, nodelen, "%s", p);
		}
#ifdef	HAVE_UNIX_SOCKETS
		else if (family == AF_UNIX)
		{
			ret = snprintf(node, nodelen, "%s", "localhost");
		}
#endif
		if (ret == -1 || ret > nodelen)
		{
			return EAI_MEMORY;
		}
	}

	return 0;
}
