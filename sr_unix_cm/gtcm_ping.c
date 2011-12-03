/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* gtcm_ping.c - routines providing the capability of pinging remote
 * 		 machines.
 */

#include "mdef.h"

#include "gtm_stdio.h"
#include "gtm_stdlib.h"		/* for exit() */
#include "gtm_unistd.h"		/* for getpid() */

#include <errno.h>
#if defined(sun) || defined(mips)
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#ifdef SCO
#include <sys/stream.h>
#endif
#ifndef __MVS__
#include <netinet/tcp.h>
#endif

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include "omi.h"

#ifdef BSD_TCP
#include <arpa/inet.h>
#endif /* defined(BSD_TCP) */

static int pingsock = -1, ident;
static char pingrcv[IP_MAXPACKET], pingsend[256];

/* init_ping
 * Set up a raw socket to ping machines
 * Returns the socket id of the "ping" socket.
 */
int init_ping()
{
    struct protoent *proto;

    ident = getpid() & 0xFFFF;
    if (!(proto = getprotobyname("icmp")))
    {
	    (void)fprintf(stderr, "ping: unknown protocol icmp.\n");
	    pingsock = -1;
	    return pingsock;
    }
    if ((pingsock = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0) {
	    perror("ping: socket");
	    if (errno == EACCES)
		    OMI_DBG((omi_debug,"You must run this program as root in order to use the -ping option.\n"));
	    pingsock = -1;
    }
    return pingsock;
}

/* icmp_ping --
 * 	Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 */
int icmp_ping(int conn)
{
	fd_set			fdmask;
	struct sockaddr_in	paddr;
	int			paddr_len = sizeof(struct sockaddr_in);
	struct icmp		*icp;
	struct ip		*ip;
	struct timeval		timeout;
	int			cc;

	if (pingsock < 0)
	{
		fprintf(stderr,"icmp_ping:  no ping socket.\n");
		exit(1);
	}
	if (getpeername(conn, (struct sockaddr *)&paddr, (size_t *)&paddr_len) < 0)
	{
		perror("getpeername");
		return -1;	/* to denote error return */
	}
	icp = (struct icmp *)pingsend;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_seq = conn;
	icp->icmp_id = ident;				/* ID */
	time((time_t *)&pingsend[ICMP_MINLEN]);		/* time stamp */
	/* compute ICMP checksum here */
	icp->icmp_cksum = in_cksum((u_short *)icp, ICMP_MINLEN + sizeof(int));
	while (cc = sendto(pingsock, (char *)pingsend, ICMP_MINLEN + sizeof(int), 0, (struct sockaddr *)&paddr, paddr_len) < 0)
	{
		if (errno == EINTR)
			continue;
		perror("ping: sendto");
		continue;
	}
#ifdef DEBUG_PING
	{
		char *host;
		struct hostent *he;
		char msg[64];
#ifndef SUNOS
		host = inet_ntoa(paddr.sin_addr.s_addr);
		if ((he = gethostbyaddr(paddr.sin_addr.s_addr, sizeof(paddr.sin_addr.s_addr), 0)))
			host = he->h_name;
#else
	 (void) sprintf(msg, "%d.%d.%d.%d",
		   paddr.sin_addr.s_addr >> 24,
		   paddr.sin_addr.s_addr >> 16 & 0xFF,
		   paddr.sin_addr.s_addr >> 8 & 0xFF,
		   paddr.sin_addr.s_addr & 0xFF);
		host = msg;
#endif
		OMI_DBG((omi_debug, "ping: send to %s\n",host));
	}
#endif
}

/* get_ping_rsp
 * Read the ping socket and determine the packet type.
 *
 * Returns:  The sequence field in the incoming ECHO_REPLY packet, or -1
 *           if the packet is not a response to one of our pings.
 */
int get_ping_rsp()
{
	struct sockaddr_in from;
	register int cc;
	int fromlen;
	struct icmp *icp;
	struct ip *ip;

	if (pingsock < 0)
	{
		fprintf(stderr,"icmp_ping:  no ping socket.\n");
		exit(1);
	}
	fromlen = sizeof(from);
	while ((cc = recvfrom(pingsock, (char *)pingrcv, IP_MAXPACKET, 0, (struct sockaddr *)&from, (size_t *)&fromlen)) < 0)
	{
		if (errno == EINTR)
			continue;
		perror("ping: recvfrom");
		continue;
	}
	ip = (struct ip *) pingrcv;
	icp = (struct icmp *)(pingrcv + (ip->ip_hl << 2));
#ifdef DEBUG_PING
	{
		char *host;
		struct hostent *he;
		char msg[64];
#ifndef SUNOS
		host = inet_ntoa(from.sin_addr.s_addr);
		if ((he = gethostbyaddr(from.sin_addr.s_addr,
					sizeof(from.sin_addr.s_addr), 0)))
			host = he->h_name;
#else
	 (void) sprintf(msg, "%d.%d.%d.%d",
		   from.sin_addr.s_addr >> 24,
		   from.sin_addr.s_addr >> 16 & 0xFF,
		   from.sin_addr.s_addr >> 8 & 0xFF,
		   from.sin_addr.s_addr & 0xFF);
		host = msg;
#endif
		OMI_DBG((omi_debug, "ping: response from %s\n",host));
	}
#endif
	/* check to see if it is a reply and if it belongs to us */
	if (icp->icmp_type == ICMP_ECHOREPLY && icp->icmp_id == ident)
		return icp->icmp_seq;
	else
		return -1;
}

/* in_cksum --
 *	Checksum routine for Internet Protocol family headers (C Version)
 */
int in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/* Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}
	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}
	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return(answer);
}
