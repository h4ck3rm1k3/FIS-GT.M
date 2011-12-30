/****************************************************************
 *								*
 *	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "gt_timer.h"

/* pointers for tcp/ip routines */
typedef struct
{
        int               (*aa_accept)(int socket, struct sockaddr *address, size_t *address_len);
        int               (*aa_bind)(int __fd, __const struct sockaddr * __addr, socklen_t __len);
        int               (*aa_close)(int sockfd );
        int               (*aa_connect)(int socket, struct sockaddr *address, size_t address_len);
        int               (*aa_getsockopt)(int __fd, int __level, int __optname,         void * __optval,         socklen_t * __optlen);
	int               (*aa_getsockname)(int __fd, struct sockaddr * __addr,   socklen_t * __len);
        unsigned short    (*aa_htons)(in_port_t);
/* smw 1999/12/15 STDC is not a good flag to use so why is it here
		  perhaps should define in_addr_t somewhere if needed. */
//#if !defined(__STDC__) || defined(__linux__)
//	uint4             (*aa_inet_addr)();
//#else
//	in_addr_t         (*aa_inet_addr)(const char *);
//#endif
  unsigned int      (*aa_inet_addr)(const char*)throw ();
  char           *  (*aa_inet_ntoa)(in_addr)throw ();
  unsigned short    (*aa_ntohs)(in_port_t);
  int               (*aa_listen)(int __fd, int __n) throw ();
  int               (*aa_recv)(int __fd, void *__buf, size_t __n, int __flags);
  //  int               (*aa_select)(int __nfds, fd_set * __readfds, fd_set * __writefds, fd_set * __exceptfds,    ABS_TIME* __timeout      );
  int               (*aa_select)(int       , fd_set*           , fd_set*            , fd_set*             ,    timeval*);

  int               (*aa_send)(int socket, void *buffer, size_t length, int flags);
  int               (*aa_setsockopt)(int sockfd, int level, int optname, const void *optval, socklen_t optlen   );
        int               (*aa_shutdown)();
        int               (*aa_socket)( int __domain, int __type, int __protocol) throw ();

        bool               using_tcpware;      /* use tcpware(1) or ucx(0) */
}tcp_library_struct;

