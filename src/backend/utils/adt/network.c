/*
 *	PostgreSQL type definitions for the INET type.	This
 *	is for IP V4 CIDR notation, but prepared for V6: just
 *	add the necessary bits where the comments indicate.
 *
 *	$Id: network.c,v 1.5 1999-01-01 04:17:13 momjian Exp $
 *	Jon Postel RIP 16 Oct 1998
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <postgres.h>
#include <utils/palloc.h>
#include <utils/builtins.h>
#include <utils/inet.h>

static int	v4bitncmp(unsigned int a1, unsigned int a2, int bits);

/*
 *	Access macros.	Add IPV6 support.
 */

#define ip_addrsize(inetptr) \
	(((inet_struct *)VARDATA(inetptr))->family == AF_INET ? 4 : -1)

#define ip_family(inetptr) \
	(((inet_struct *)VARDATA(inetptr))->family)

#define ip_bits(inetptr) \
	(((inet_struct *)VARDATA(inetptr))->bits)

#define ip_type(inetptr) \
	(((inet_struct *)VARDATA(inetptr))->type)

#define ip_v4addr(inetptr) \
	(((inet_struct *)VARDATA(inetptr))->addr.ipv4_addr)

/* Common input routine */
static inet *
network_in(char *src, int type)
{
	int			bits;
	inet	   *dst;

	dst = palloc(VARHDRSZ + sizeof(inet_struct));
	if (dst == NULL)
	{
		elog(ERROR, "unable to allocate memory in network_in()");
		return NULL;
	}
	/* First, try for an IP V4 address: */
	ip_family(dst) = AF_INET;
	bits = inet_net_pton(ip_family(dst), src, &ip_v4addr(dst),
			type ? ip_addrsize(dst) : -1);
	if ((bits < 0) || (bits > 32))
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "could not parse \"%s\"", src);
		pfree(dst);
		return NULL;
	}
	VARSIZE(dst) = VARHDRSZ
		+ ((char *) &ip_v4addr(dst) - (char *) VARDATA(dst))
		+ ip_addrsize(dst);
	ip_bits(dst) = bits;
	ip_type(dst) = type;
	return dst;
}

/* INET address reader.  */
inet *
inet_in(char *src)
{
	return network_in(src, 0);
}

/* CIDR address reader.  */
inet *
cidr_in(char *src)
{
	return network_in(src, 1);
}

/*
 *	INET address output function.
 */

char *
inet_out(inet *src)
{
	char	   *dst,
				tmp[sizeof("255.255.255.255/32")];

	if (ip_family(src) == AF_INET)
	{
		/* It's an IP V4 address: */
		if (ip_type(src))
			dst = inet_cidr_ntop(AF_INET, &ip_v4addr(src), ip_bits(src),
						  tmp, sizeof(tmp));
		else
			dst = inet_net_ntop(AF_INET, &ip_v4addr(src), ip_bits(src),
						  tmp, sizeof(tmp));

		if (dst == NULL)
		{
			elog(ERROR, "unable to print address (%s)", strerror(errno));
			return (NULL);
		}
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "unknown address family (%d)", ip_family(src));
		return NULL;
	}
	dst = palloc(strlen(tmp) + 1);
	if (dst == NULL)
	{
		elog(ERROR, "unable to allocate memory in inet_out()");
		return NULL;
	}
	strcpy(dst, tmp);
	return dst;
}


/* just a stub */
char *
cidr_out(inet *src)
{
	return inet_out(src);
}

/*
 *	Boolean tests for magnitude.  Add V4/V6 testing!
 */

bool
network_lt(inet *a1, inet *a2)
{
	if ((ip_family(a1) == AF_INET) && (ip_family(a2) == AF_INET))
	{
		int			order = v4bitncmp(ip_v4addr(a1), ip_v4addr(a2), ip_bits(a2));

		return ((order < 0) || ((order == 0) && (ip_bits(a1) < ip_bits(a2))));
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "cannot compare address families %d and %d",
			 ip_family(a1), ip_family(a2));
		return (FALSE);
	}
}

bool
network_le(inet *a1, inet *a2)
{
	return (network_lt(a1, a2) || network_eq(a1, a2));
}

bool
network_eq(inet *a1, inet *a2)
{
	if ((ip_family(a1) == AF_INET) && (ip_family(a2) == AF_INET))
	{
		return ((ip_bits(a1) == ip_bits(a2))
		 && (v4bitncmp(ip_v4addr(a1), ip_v4addr(a2), ip_bits(a1)) == 0));
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "cannot compare address families %d and %d",
			 ip_family(a1), ip_family(a2));
		return (FALSE);
	}
}

bool
network_ge(inet *a1, inet *a2)
{
	return (network_gt(a1, a2) || network_eq(a1, a2));
}

bool
network_gt(inet *a1, inet *a2)
{
	if ((ip_family(a1) == AF_INET) && (ip_family(a2) == AF_INET))
	{
		int			order = v4bitncmp(ip_v4addr(a1), ip_v4addr(a2), ip_bits(a2));

		return ((order > 0) || ((order == 0) && (ip_bits(a1) > ip_bits(a2))));
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "cannot compare address families %d and %d",
			 ip_family(a1), ip_family(a2));
		return (FALSE);
	}
}

bool
network_ne(inet *a1, inet *a2)
{
	return (!network_eq(a1, a2));
}

bool
network_sub(inet *a1, inet *a2)
{
	if ((ip_family(a1) == AF_INET) && (ip_family(a2) == AF_INET))
	{
		return ((ip_bits(a1) > ip_bits(a2))
		 && (v4bitncmp(ip_v4addr(a1), ip_v4addr(a2), ip_bits(a2)) == 0));
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "cannot compare address families %d and %d",
			 ip_family(a1), ip_family(a2));
		return (FALSE);
	}
}

bool
network_subeq(inet *a1, inet *a2)
{
	if ((ip_family(a1) == AF_INET) && (ip_family(a2) == AF_INET))
	{
		return ((ip_bits(a1) >= ip_bits(a2))
		 && (v4bitncmp(ip_v4addr(a1), ip_v4addr(a2), ip_bits(a2)) == 0));
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "cannot compare address families %d and %d",
			 ip_family(a1), ip_family(a2));
		return (FALSE);
	}
}

bool
network_sup(inet *a1, inet *a2)
{
	if ((ip_family(a1) == AF_INET) && (ip_family(a2) == AF_INET))
	{
		return ((ip_bits(a1) < ip_bits(a2))
		 && (v4bitncmp(ip_v4addr(a1), ip_v4addr(a2), ip_bits(a1)) == 0));
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "cannot compare address families %d and %d",
			 ip_family(a1), ip_family(a2));
		return (FALSE);
	}
}

bool
network_supeq(inet *a1, inet *a2)
{
	if ((ip_family(a1) == AF_INET) && (ip_family(a2) == AF_INET))
	{
		return ((ip_bits(a1) <= ip_bits(a2))
		 && (v4bitncmp(ip_v4addr(a1), ip_v4addr(a2), ip_bits(a1)) == 0));
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "cannot compare address families %d and %d",
			 ip_family(a1), ip_family(a2));
		return (FALSE);
	}
}

/*
 *	Comparison function for sorting.  Add V4/V6 testing!
 */

int4
network_cmp(inet *a1, inet *a2)
{
	if (ntohl(ip_v4addr(a1)) < ntohl(ip_v4addr(a2)))
		return (-1);
	else if (ntohl(ip_v4addr(a1)) > ntohl(ip_v4addr(a2)))
		return (1);
	return 0;
}

text *
network_host(inet *ip)
{
	text	   *ret;
	int			len;
	char	   *ptr,
				tmp[sizeof("255.255.255.255/32")];

	if (ip_type(ip))
	{
		elog(ERROR, "CIDR type has no host part");
		return NULL;
	}

	if (ip_family(ip) == AF_INET)
	{
		/* It's an IP V4 address: */
		if (inet_net_ntop(AF_INET, &ip_v4addr(ip), 32, tmp, sizeof(tmp)) == NULL)
		{
			elog(ERROR, "unable to print host (%s)", strerror(errno));
			return (NULL);
		}
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "unknown address family (%d)", ip_family(ip));
		return (NULL);
	}
	if ((ptr = strchr(tmp, '/')) != NULL)
		*ptr = 0;
	len = VARHDRSZ + strlen(tmp) + 1;
	ret = palloc(len);
	if (ret == NULL)
	{
		elog(ERROR, "unable to allocate memory in network_host()");
		return (NULL);
	}
	VARSIZE(ret) = len;
	strcpy(VARDATA(ret), tmp);
	return (ret);
}

int4
network_masklen(inet *ip)
{
	return ip_bits(ip);
}

text *
network_broadcast(inet *ip)
{
	text	   *ret;
	int			len;
	char	   *ptr,
				tmp[sizeof("255.255.255.255/32")];

	if (ip_family(ip) == AF_INET)
	{
		/* It's an IP V4 address: */
		int	addr = htonl(ntohl(ip_v4addr(ip)) | (0xffffffff >> ip_bits(ip)));

		if (inet_net_ntop(AF_INET, &addr, 32, tmp, sizeof(tmp)) == NULL)
		{
			elog(ERROR, "unable to print address (%s)", strerror(errno));
			return (NULL);
		}
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "unknown address family (%d)", ip_family(ip));
		return (NULL);
	}
	if ((ptr = strchr(tmp, '/')) != NULL)
		*ptr = 0;
	len = VARHDRSZ + strlen(tmp) + 1;
	ret = palloc(len);
	if (ret == NULL)
	{
		elog(ERROR, "unable to allocate memory in network_broadcast()");
		return (NULL);
	}
	VARSIZE(ret) = len;
	strcpy(VARDATA(ret), tmp);
	return (ret);
}

text *
network_network(inet *ip)
{
	text	   *ret;
	int			len;
	char	   	tmp[sizeof("255.255.255.255/32")];

	if (ip_family(ip) == AF_INET)
	{
		/* It's an IP V4 address: */
		int	addr = htonl(ntohl(ip_v4addr(ip)) & (0xffffffff << (32 - ip_bits(ip))));
  
		if (inet_cidr_ntop(AF_INET, &addr, ip_bits(ip), tmp, sizeof(tmp)) == NULL)
		{
			elog(ERROR, "unable to print network (%s)", strerror(errno));
			return (NULL);
		}
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "unknown address family (%d)", ip_family(ip));
		return (NULL);
	}
	len = VARHDRSZ + strlen(tmp) + 1;
	ret = palloc(len);
	if (ret == NULL)
	{
		elog(ERROR, "unable to allocate memory in network_network()");
		return (NULL);
	}
	VARSIZE(ret) = len;
	strcpy(VARDATA(ret), tmp);
	return (ret);
}

text *
network_netmask(inet *ip)
{
	text	   *ret;
	int			len;
	char	   *ptr,
				tmp[sizeof("255.255.255.255/32")];

	if (ip_family(ip) == AF_INET)
	{
		/* It's an IP V4 address: */
		int			addr = htonl((-1 << (32 - ip_bits(ip))) & 0xffffffff);

		if (inet_net_ntop(AF_INET, &addr, 32, tmp, sizeof(tmp)) == NULL)
		{
			elog(ERROR, "unable to print netmask (%s)", strerror(errno));
			return (NULL);
		}
	}
	else
	{
		/* Go for an IPV6 address here, before faulting out: */
		elog(ERROR, "unknown address family (%d)", ip_family(ip));
		return (NULL);
	}
	if ((ptr = strchr(tmp, '/')) != NULL)
		*ptr = 0;
	len = VARHDRSZ + strlen(tmp) + 1;
	ret = palloc(len);
	if (ret == NULL)
	{
		elog(ERROR, "unable to allocate memory in network_netmask()");
		return (NULL);
	}
	VARSIZE(ret) = len;
	strcpy(VARDATA(ret), tmp);
	return (ret);
}

/*
 *	Bitwise comparison for V4 addresses.  Add V6 implementation!
 */

static int
v4bitncmp(unsigned int a1, unsigned int a2, int bits)
{
	unsigned long mask = 0;
	int			i;

	for (i = 0; i < bits; i++)
		mask = (mask >> 1) | 0x80000000;
	a1 = ntohl(a1);
	a2 = ntohl(a2);
	if ((a1 & mask) < (a2 & mask))
		return (-1);
	else if ((a1 & mask) > (a2 & mask))
		return (1);
	return (0);
}
