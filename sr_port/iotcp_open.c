/****************************************************************
 *								*
 *	Copyright 2001, 2004 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

/* iotcp_open.c - open a TCP/IP connection
 *  Parameters-
 *	dev		- the logical name associated with this socket (ignored by this routine).
 *	pp->str.addr 	- device parameters.  The "stream" parameter is required.
 *	file_des	- unused. (UNIX only)
 *	mspace		- unused.
 *	t		- maximum time to wait for a connection (in ms).
 *
 *  Returns-
 *	non-zero	- socket successfully opened and ready for I/O
 *	zero		- open operation failed or timed out.
 */
#include "mdef.h"

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "gtm_inet.h"
#include "gtm_ctype.h"
#include "gtm_string.h"
#include "gtm_stdio.h"

#include "copy.h"
#include "gt_timer.h"
#include "io.h"
#include "iotimer.h"
#include "iotcp_select.h"
#include "iotcpdef.h"
#include "iotcpdefsp.h"
#include "iotcproutine.h"
#include "io_params.h"
#include "stringpool.h"
#include "outofband.h"
#include "wake_alarm.h"

GBLREF tcp_library_struct	tcp_routines;
GBLREF bool			out_of_time;
GBLREF volatile int4		outofband;
LITREF unsigned char		io_params_size[];

short	iotcp_open(io_log_name *dev, mval *pp, int file_des, mval *mspace, int4 timeout)
{
	boolean_t		no_time_left = FALSE, timed;
	char			addr[SA_MAXLEN+1], *errptr, sockaddr[SA_MAXLEN+1],
				temp_addr[SA_MAXLEN+1], temp_ch;
	unsigned char		ch, len;
	int4			length, width;
	unsigned short		port;
	int4			errlen, msec_timeout;
	int			ii, status, size,
				on = 1,
				p_offset = 0,
				temp_1 = -2;
	TID			timer_id;
	ABS_TIME		cur_time, end_time, time_for_read, save_time_for_read;
	d_tcp_struct		*tcpptr, newtcp;
	io_desc			*ioptr;
	struct sockaddr_in	peer;		/* socket address + port */
	fd_set			tcp_fd;
	int			lsock;

	error_def(ERR_DEVPARMNEG);
	error_def(ERR_INVADDRSPEC);
	error_def(ERR_INVPORTSPEC);
	error_def(ERR_IPADDRREQ);
	error_def(ERR_OPENCONN);
	error_def(ERR_SOCKACPT);
	error_def(ERR_SOCKINIT);
	error_def(ERR_SOCKPARMREQ);
	error_def(ERR_SOCKWAIT);
	error_def(ERR_TEXT);

#ifdef	DEBUG_TCP
	PRINTF("iotcp_open.c >>>   tt = %d\n", t);
#endif
	ioptr = dev->iod;
	assert((params) *(pp->str.addr + p_offset) < (unsigned char)n_iops);
	assert(0 != ioptr);
	assert(ioptr->state >= 0 && ioptr->state < n_io_dev_states);
	assert(tcp == ioptr->type);
	if (dev_never_opened == ioptr->state)
	{
		ioptr->dev_sp = (void *)malloc(sizeof(d_tcp_struct));
		memset(ioptr->dev_sp, 0, sizeof(d_tcp_struct));
	}
	tcpptr = (d_tcp_struct *)ioptr->dev_sp;
	if (dev_never_opened == ioptr->state)
	{
		ioptr->state	= dev_closed;
		ioptr->width	= TCPDEF_WIDTH;
		ioptr->length	= TCPDEF_LENGTH;
		ioptr->wrap	= TRUE;
		if (-1 == iotcp_fillroutine())
			assert(FALSE);
	}
	ioptr->dollar.zeof = FALSE;
	newtcp = *tcpptr;
	memcpy(newtcp.dollar_device, LITZERO, sizeof(LITZERO));
	newtcp.passive = FALSE;
	while (iop_eol != *(pp->str.addr + p_offset))
	{
		switch	(ch = *(pp->str.addr + p_offset++))
		{
		case	iop_width:
			GET_LONG(width, pp->str.addr + p_offset);
			if (0 == width)
				newtcp.width = TCPDEF_WIDTH;
			else if (width > 0)
				newtcp.width = width;
			else
				rts_error(VARLSTCNT(1)	ERR_DEVPARMNEG);
			break;
		case	iop_length:
			GET_LONG(length, pp->str.addr + p_offset);
			if (0 == length)
				newtcp.length = TCPDEF_LENGTH;
			else if (length > 0)
				newtcp.length = length;
			else
				rts_error(VARLSTCNT(1) ERR_DEVPARMNEG);
			break;
		case	iop_listen:
			newtcp.passive = TRUE;
			break;
		case	iop_socket:
			len = *(pp->str.addr + p_offset);
			memset(sockaddr, 0, SA_MAXLEN+1);
			memcpy(sockaddr, pp->str.addr + p_offset + 1, (len <= SA_MAXLEN) ? len : SA_MAXLEN);
			*temp_addr = '\0';
			*addr = '\0';
			port = 0;
			if (SSCANF(sockaddr, "%[^,], %hu", temp_addr, &port) < 2)
			{
				newtcp.sin.sin_addr.s_addr = INADDR_ANY;
				if (SSCANF(sockaddr, ",%hu", &port) < 1)
				{
					rts_error(VARLSTCNT(1) ERR_INVPORTSPEC);
					return	FALSE;
				}
			} else
			{
				ii = 0;
				temp_ch = temp_addr[0];
				while(ISDIGIT(temp_ch) || ('.' == temp_ch))
				{
					ii++;
					temp_ch = temp_addr[ii];
				}
				if ('\0' != temp_ch)
					SPRINTF(addr, "%s", iotcp_name2ip(temp_addr));
				else
					SPRINTF(addr, "%s", temp_addr);
				if ((in_addr_t)-1 == (newtcp.sin.sin_addr.s_addr = tcp_routines.aa_inet_addr(addr)))
				{
					rts_error(VARLSTCNT(1) ERR_INVADDRSPEC);
					return	FALSE;
				}
			}
			newtcp.sin.sin_port = GTM_HTONS(port);
			newtcp.sin.sin_family = AF_INET;
			break;
		case	iop_exception:
			ioptr->error_handler.len = *(pp->str.addr + p_offset);
			ioptr->error_handler.addr = (char *)(pp->str.addr + p_offset + 1);
			s2pool(&ioptr->error_handler);
			break;
		default:
			break;
		}
		p_offset += ((IOP_VAR_SIZE == io_params_size[ch]) ?
			(unsigned char)*(pp->str.addr + p_offset) + 1 : io_params_size[ch]);
	}
	if ((0 == newtcp.sin.sin_port) && (0 == newtcp.sin.sin_addr.s_addr))
	{
		rts_error(VARLSTCNT(1) ERR_SOCKPARMREQ);
		return	FALSE;
	}
	/* active connection must have a complete address specification */
	if ((INADDR_ANY == newtcp.sin.sin_addr.s_addr) && !newtcp.passive)
	{
		rts_error(VARLSTCNT(1) ERR_IPADDRREQ);
		return FALSE;
	}
	if (dev_closed == ioptr->state)
	{
		if (newtcp.passive)		/* passive connection */
		{
			/* no listening socket for this addr?  make one. */
			memcpy(ioptr->dev_sp, &newtcp, sizeof(d_tcp_struct));
			if (!(lsock = iotcp_getlsock(dev)))
				return	FALSE;	/* could not create listening socket */
			timer_id = (TID)iotcp_open;
			out_of_time = FALSE;
			time_for_read.at_sec = ((NO_M_TIMEOUT == timeout) ? 0 : 1);
			time_for_read.at_usec = 0;
			if (NO_M_TIMEOUT == timeout)
			{
				timed = FALSE;
				msec_timeout = NO_M_TIMEOUT;
			} else
			{
				timed = TRUE;
				msec_timeout = timeout2msec(timeout);
				if (msec_timeout > 0)
				{       /* there is time to wait */
					sys_get_curr_time(&cur_time);
					add_int_to_abs_time(&cur_time, msec_timeout, &end_time);
					start_timer(timer_id, msec_timeout, wake_alarm, 0, NULL);
				} else
					out_of_time = TRUE;
			}
			for (status = 0; 0 == status; )
			{
				FD_ZERO(&tcp_fd);
				FD_SET(lsock, &tcp_fd);
				/*
				 * Note: the check for EINTR from the select below should remain, as aa_select is a
				 * function, and not all callers of aa_select behave the same when EINTR is returned.
				 */
                                save_time_for_read = time_for_read;
				status = tcp_routines.aa_select(lsock + 1, (void *)&tcp_fd, (void *)0, (void *)0, &time_for_read);
                                time_for_read = save_time_for_read;
				if (0 > status)
				{
					if (EINTR == errno && FALSE == out_of_time)
						/* interrupted by a signal which is not OUR timer */
						status = 0;
					else
						break;
				}
				if (outofband)
                        		break;
				if (timed)
				{
					if (msec_timeout > 0)
					{
						sys_get_curr_time(&cur_time);
						cur_time = sub_abs_time(&end_time, &cur_time);
						if (cur_time.at_sec <= 0)
						{
							out_of_time = TRUE;
							cancel_timer(timer_id);
							break;
						}
					} else
						break;
				}
#ifdef __linux__
				time_for_read.at_sec = ((NO_M_TIMEOUT == timeout) ? 0 : 1);
				time_for_read.at_usec = 0;
#endif
			}
			if (timed)
			{
				if (0 != msec_timeout)
				{
					cancel_timer(timer_id);
					if (out_of_time || outofband)
						return FALSE;
					/*if (outofband)
						outofband_action(FALSE);*/
				}
			}
			if (0 > status)
			{
				errptr = (char *)STRERROR(errno);
				errlen = strlen(errptr);
				iotcp_rmlsock((io_desc *)dev->iod);
				rts_error(VARLSTCNT(6) ERR_SOCKWAIT, 0, ERR_TEXT, 2, errlen, errptr);
				return FALSE;
			}
			size = sizeof(struct sockaddr_in);
			status = tcp_routines.aa_accept(lsock, &peer, &size);
			if (-1 == status)
			{
				errptr = (char *)STRERROR(errno);
				errlen = strlen(errptr);
				iotcp_rmlsock((io_desc *)dev->iod);
				rts_error(VARLSTCNT(6) ERR_SOCKACPT, 0, ERR_TEXT, 2, errlen, errptr);
				return FALSE;
			}
			SPRINTF(newtcp.saddr, "%s,%d", tcp_routines.aa_inet_ntoa(peer.sin_addr),
				GTM_NTOHS(newtcp.sin.sin_port));
			newtcp.socket = status;
		} else			/*	active connection */
		{
			if (NO_M_TIMEOUT != timeout)
			{
				msec_timeout = timeout2msec(timeout);
				sys_get_curr_time(&cur_time);
				add_int_to_abs_time(&cur_time, msec_timeout, &end_time);
			}
			no_time_left = FALSE;
			temp_1 = 1;
			do
			{
				if(1 != temp_1)
					tcp_routines.aa_close(newtcp.socket);
				newtcp.socket = tcp_routines.aa_socket(AF_INET, SOCK_STREAM, 0);
				if (-1 == newtcp.socket)
				{
					errptr = (char *)STRERROR(errno);
					errlen = strlen(errptr);
					rts_error(VARLSTCNT(5) ERR_SOCKINIT, 3, errno, errlen, errptr);
					return FALSE;
				}
				/*	allow multiple connections to the same IP address */
				if	(-1 == tcp_routines.aa_setsockopt(newtcp.socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
				{
					(void)tcp_routines.aa_close(newtcp.socket);
					errptr = (char *)STRERROR(errno);
					errlen = strlen(errptr);
					rts_error(VARLSTCNT(5) ERR_SOCKINIT, 3, errno, errlen, errptr);
					return FALSE;
				}
				size=sizeof(newtcp.bufsiz);
				if (-1 == tcp_routines.aa_getsockopt(newtcp.socket, SOL_SOCKET, SO_RCVBUF, &newtcp.bufsiz, &size))
				{
					(void)tcp_routines.aa_close(newtcp.socket);
					errptr = (char *)STRERROR(errno);
					errlen = strlen(errptr);
					rts_error(VARLSTCNT(5) ERR_SOCKINIT, 3, errno, errlen, errptr);
					return FALSE;
				}
				/*
				 * Note: the check for EINTR from the connect need not be converted to an EINTR wrapper macro,
				 * since the connect is not retried on EINTR.
				 */
				temp_1 = tcp_routines.aa_connect(newtcp.socket, (struct sockaddr *)&newtcp.sin, sizeof(newtcp.sin));
				if ((temp_1 < 0) && (ECONNREFUSED != errno) && (EINTR != errno))
				{
					(void)tcp_routines.aa_close(newtcp.socket);
					errptr = (char *)STRERROR(errno);
					errlen = strlen(errptr);
					rts_error(VARLSTCNT(6) ERR_OPENCONN, 0, ERR_TEXT, 2, errlen, errptr);
					return FALSE;
				}
				if ((temp_1 < 0) && (EINTR == errno))
				{
					(void)tcp_routines.aa_close(newtcp.socket);
					return FALSE;
				}
				if ((temp_1 < 0) && (NO_M_TIMEOUT != timeout))
				{
					sys_get_curr_time(&cur_time);
					cur_time = sub_abs_time(&end_time, &cur_time);
					if (cur_time.at_sec <= 0)
						no_time_left = TRUE;
				}
				SHORT_SLEEP(100);		/* Sleep for 100 ms */
			}
			while ((TRUE != no_time_left) && (temp_1 < 0));
			if (temp_1 < 0) /* out of time */
			{
				tcp_routines.aa_close(newtcp.socket);
				return FALSE;
			}
#ifdef	ntohs	/* if it's a macro, use it instead of tcp_routines.aa_ntohs */
			SPRINTF(newtcp.saddr, "%s,%d", tcp_routines.aa_inet_ntoa(newtcp.sin.sin_addr),
				ntohs(newtcp.sin.sin_port));
#else
			SPRINTF(newtcp.saddr, "%s,%d", tcp_routines.aa_inet_ntoa(newtcp.sin.sin_addr),
				tcp_routines.aa_ntohs(newtcp.sin.sin_port));
#endif
		}
		memcpy(ioptr->dev_sp, &newtcp, sizeof(d_tcp_struct));
		ioptr->state = dev_open;
	}
#ifdef	DEBUG_TCP
	PRINTF("%s (%d) <<<\n", __FILE__, tcpptr->socket);
#endif
	return TRUE;
}
