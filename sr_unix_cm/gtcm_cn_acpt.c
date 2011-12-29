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

/*  gtcm_cn_acpt.c ---
 *	Accept a client connection.
 */

#ifdef DEBUG
#include "gtm_fcntl.h"
#endif /* defined(DEBUG) */

#include "mdef.h"

#include "gtm_string.h"
#include "gtm_stdio.h"
#include "gtm_unistd.h"		/* for close() */
#include "gtm_time.h"		/* for ctime() and time() */
#include "gtcm.h"
#include "rc_oflow.h"
#include <errno.h>
#include "eintr_wrappers.h"

#ifdef BSD_TCP
#include <arpa/inet.h>
#endif /* defined(BSD_TCP) */

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/fis-gtm/gtm/sr_unix_cm/gtcm_cn_acpt.c,v 1.2 2002/06/13 14:34:16 miazim Exp $";
#endif

#ifndef __linux__

#	ifdef __osf__
#	pragma pointer_size (save)
#	pragma pointer_size (long)
#	endif

extern char	*sys_errlist[];

#	ifdef __osf__
#	pragma pointer_size (restore)
#	endif

#endif

GBLREF	char	*omi_pklog;
GBLREF	int	one_conn_per_inaddr;
GBLREF	int	conn_timeout;

int gtcm_cn_acpt(omi_conn_ll *cll, int now)		/* now --> current time in seconds */
{
	int		i;
	omi_conn	*cptr;
	omi_fd		fd;

#ifdef BSD_TCP
	int			sln;
	struct sockaddr_in	sin;
	int			option, optsize;

	/*  Accept the connection from the network layer */
	sln = sizeof(sin);
	if ((fd = accept(cll->nve, (struct sockaddr *)&sin, (size_t *)&sln)) < 0)
		return -1;
#endif				/* defined(BSD_TCP) */

	/*  Build the client data structure */
	if (!(cptr = (omi_conn *)gtm_malloc_intern(sizeof(omi_conn))) || !(cptr->buff = (char *)gtm_malloc_intern(OMI_BUFSIZ)))
	{
		if (cptr)
			gtm_free_intern(cptr);
		(void) close(fd);
		return -1;
	}
	/*  Initialize the connection structure */
	cptr->next  = (omi_conn *)0;
	cptr->fd    = fd;
	cptr->ping_cnt = 0;
	cptr->timeout = now + conn_timeout;
	cptr->bsiz  = OMI_BUFSIZ;
	cptr->bptr  = cptr->buff;
	cptr->xptr  = (char *)0;
	cptr->blen  = 0;
	cptr->exts  = 0;
	cptr->state = OMI_ST_DISC;
	cptr->ga    = (ga_struct *)0; /* struct gd_addr_struct */
	cptr->of = (oof_struct *) gtm_malloc_intern(sizeof(struct rc_oflow));
	memset(cptr->of, 0, sizeof(struct rc_oflow));
	cptr->pklog = INV_FD;
	/*  Initialize the statistics */
	memcpy(&cptr->stats.sin,&sin,sizeof(sin));
	cptr->stats.bytes_recv = 0;
	cptr->stats.bytes_send = 0;
	cptr->stats.start      = time((time_t *)0);
	for (i = 0; i < OMI_OP_MAX; i++)
		cptr->stats.xact[i] = 0;
	for (i = 0; i < OMI_ER_MAX; i++)
		cptr->stats.errs[i] = 0;

	/* if we only allowing one connection per internet address, close any existing ones with the same addr. */
	if (one_conn_per_inaddr)
	{
		omi_conn	*this, *prev;

		for (prev = NULL, this = cll->head; this; prev = this, this = this->next)
		{
			if (this->stats.sin.sin_addr.s_addr == sin.sin_addr.s_addr)
			{
				if (cll->tail == this)
				    cll->tail = cptr;
				if (prev)
				    prev->next = cptr;
				else
				    cll->head = cptr;
				cptr->next = this->next;
				OMI_DBG_STMP;
				OMI_DBG((omi_debug, "%s: dropping old connection to %s\n",
					SRVR_NAME, gtcm_hname(&cptr->stats.sin)));
				gtcm_cn_disc(this, cll);
				break;
			}
		}
		/* not found - add to the end of the list */
		if (!this)
		{
			if (cll->tail)
			{
				cll->tail->next = cptr;
				cll->tail       = cptr;
			} else
				cll->head = cll->tail = cptr;
		}
	} else
	{
		/*  Insert the client into the list of connections */
		if (cll->tail)
		{
			cll->tail->next = cptr;
			cll->tail       = cptr;
		} else
			cll->head = cll->tail = cptr;
	}
	cptr->stats.id = ++cll->stats.conn;

	DEBUG_ONLY(
		if (omi_pklog)
		{
			char		pklog[1024];

			(void) sprintf(pklog, "%s.%04d", omi_pklog, cptr->stats.id);
			if (INV_FD_P((cptr->pklog = open(pklog, O_WRONLY|O_CREAT|O_APPEND|O_TRUNC, 0644))))
			{
				OMI_DBG_STMP;
				OMI_DBG((omi_debug, "%s: unable to open packet log \"%s\"\n\t%s\n",
					SRVR_NAME, pklog, sys_errlist[errno]));
			}
		}
	)
	OMI_DBG((omi_debug, "%s: connection %d from %s by user <%s> at %s", SRVR_NAME,
		cptr->stats.id, gtcm_hname(&cptr->stats.sin), cptr->ag_name, GTM_CTIME(&cptr->stats.start)));
	option = -1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&option, sizeof(option)) < 0)
	{
		PERROR("setsockopt:");
		return -1;
	}
	return 0;
}
