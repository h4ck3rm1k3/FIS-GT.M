/****************************************************************
 *								*
 *	Copyright 2001, 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_stdlib.h"		/* for exit() */

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/sem.h>
#include <ctype.h>
#include <stddef.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#include "gtm_socket.h"
#include "gtm_fcntl.h"
#include "gtm_unistd.h"
#include "gtm_stdio.h"
#include "gtm_stat.h"
#include "gt_timer.h"

#include "io.h"
#include "gtmsecshr.h"
#include "iosp.h"
#include "error.h"
#include "eintr_wrappers.h"
#include "util.h"
#include "send_msg.h"
#include "gtmmsg.h"
#include "wcs_backoff.h"
#include "trans_log_name.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"

GBLREF struct sockaddr_un       gtmsecshr_sock_name;
GBLREF key_t                    gtmsecshr_key;
GBLREF int                      gtmsecshr_sockpath_len;
GBLREF int                      gtmsecshr_sockfd;
GBLREF mstr                     gtmsecshr_pathname;
GBLREF int			server_start_tries;
GBLREF boolean_t		gtmsecshr_sock_init_done;
GBLREF uint4			process_id;
GBLREF ipcs_mesg		db_ipcs;

static int			secshr_sem;
static int			gtmsecshr_file_check_done = 0;
static mstr			gtmsecshr_logname;
static char			gtmsecshr_path[MAX_TRANS_NAME_LEN];
static volatile int		client_timer_popped;
static unsigned long		cur_seqno = 0;


const static char readonly secshr_fail_mesg_code[][MAX_GTMSECSHR_FAIL_MESG_LEN] = {
	"",
	"Wake Message Failed",
	"Check process alive failed",
	"Remove Semaphore failed",
	"Remove Shared Memory segment failed",
	"Ping Message failed",
	"Remove File failed",
	"Continue Process failed",
};

const static char readonly secshr_unbl_start_mesg_code[][MAX_GTMSECSHR_FAIL_MESG_LEN] = {
	"",
	"gtmsecshr unable to set-uid to root",
	"gtmsecshr unable to set-gid to root",
	"gtmsecshr unable to open log file",
	"gtmsecshr unable to dup stdout and stderr",
	"The environmental variable gtm_dist is pointing to an invalid path",
	"Unable to exec gtmsecshr",
	"gtmsecshr unable to create a  child process",
	"The environmental variable gtm_log is pointing to an invalid path",
	"Error with gtmsecshr semaphore",
	"Invalid gtm exit message",
};

#define MAX_RETRIES			7
#define CLIENT_ACK_TIMER		5


#define START_SERVER										\
{												\
	if (0 != (create_server_status = create_server()))					\
	{											\
		gtm_putmsg(VARLSTCNT(9) ERR_GTMSECSHRSTARTUP, 3, RTS_ERROR_TEXT("Client"),	\
			process_id, ERR_TEXT, 2,						\
			RTS_ERROR_STRING(secshr_unbl_start_mesg_code[create_server_status]));	\
		if (FATALFAILURE(create_server_status))						\
		{										\
			gtmsecshr_sock_cleanup(CLIENT);						\
			return create_server_status;						\
		}										\
		/* Transient Failures and will continue after printing out message */		\
	}											\
	hiber_start(3000); /* 3000 ms (3 sec) to allow server to come up */			\
}

#define SETUP_FOR_RECV										\
{												\
	recd = 0;										\
	mesg_len = 0;										\
	recv_ptr = (char *)&mesg;								\
	recv_len = sizeof(mesg);								\
	client_timer_popped = 0;								\
	msec_timeout = timeout2msec(CLIENT_ACK_TIMER);						\
	start_timer(timer_id, msec_timeout, client_timer_handler, 0, NULL);			\
}

void client_timer_handler(void);
int send_mesg2gtmsecshr (unsigned int code, unsigned int id, char *path, int path_len);
int create_server (void);


void client_timer_handler(void)
{
	client_timer_popped = 1;
}

int send_mesg2gtmsecshr (unsigned int code, unsigned int id, char *path, int path_len)
{
	int                     client_sockfd, create_server_status, fcntl_res;
	int			req_code, wait_count = 0;
	int			mesg_len;
	int			recv_len, send_len;
	int			recd = 0, sent;
	int			num_chars_recvd, num_chars_sent;
	int 			save_errno, ret_code = 0, init_ret_code = 0;
	int			loop_count = 0;
	boolean_t		retry = FALSE, recv_complete, send_complete;
	size_t			server_proc_len;
	int			semop_res;
	int			selstat;
	char			*recv_ptr, *send_ptr;
	struct sockaddr_un	server_proc;
	struct sembuf		sop[4];
	struct timeval		input_timeval;
	struct timeval		save_input_timeval;
	fd_set			wait_on_fd;
	gtmsecshr_mesg		mesg;
	TID			timer_id;
	int4			msec_timeout;

	error_def(ERR_GTMSECSHR);
	error_def(ERR_GTMSECSHRSTARTUP);
	error_def(ERR_GTMSECSHRSOCKET);
	error_def(ERR_GTMSECSHRSRVFFILE);
	error_def(ERR_GTMSECSHRSRVFID);
	error_def(ERR_GTMSECSHRSRVF);
	error_def(ERR_GTMSECSHRPERM);
	error_def(ERR_TEXT);
	error_def(ERR_SYSCALL);
	timer_id = (TID)send_mesg2gtmsecshr;

	if (!gtmsecshr_file_check_done)
	{
		struct stat stat_buf;

		gtmsecshr_logname.addr = GTMSECSHR_PATH;
                gtmsecshr_logname.len = sizeof(GTMSECSHR_PATH) - 1;
                if (SS_NORMAL != trans_log_name(&gtmsecshr_logname, &gtmsecshr_pathname, gtmsecshr_path))
		{
                        send_msg(VARLSTCNT(9) ERR_GTMSECSHRSTARTUP, 3, RTS_ERROR_TEXT("Client"), process_id,
					ERR_TEXT, 2, RTS_ERROR_STRING(secshr_unbl_start_mesg_code[INVTRANSGTMSECSHR]));
                        rts_error(VARLSTCNT(9) ERR_GTMSECSHRSTARTUP, 3, RTS_ERROR_TEXT("Client"), process_id,
					ERR_TEXT, 2, RTS_ERROR_STRING(secshr_unbl_start_mesg_code[INVTRANSGTMSECSHR]));
		}
		gtmsecshr_pathname.addr[gtmsecshr_pathname.len] = '\0';
		if (-1 == Stat(gtmsecshr_pathname.addr, &stat_buf))
			rts_error(VARLSTCNT(8) ERR_SYSCALL, 5, LEN_AND_LIT("stat"), CALLFROM, errno);
		if ((ROOTUID != stat_buf.st_uid) ||
			!(stat_buf.st_mode & S_ISUID))
			rts_error(VARLSTCNT(1) ERR_GTMSECSHRPERM);
		gtmsecshr_file_check_done = 1;
	}
	if (!gtmsecshr_sock_init_done && ((init_ret_code = gtmsecshr_sock_init(CLIENT)) > 0))
		return init_ret_code;
	while (MAX_RETRIES > loop_count)
	{	/* first, try the sendto */
		req_code = mesg.code = code;
		mesg.len = GTM_MESG_HDR_SIZE;
  		if (REMOVE_FILE == code)
		{
			memcpy(mesg.mesg.path, path, path_len);
			mesg.len += path_len;
		} else if (FLUSH_DB_IPCS_INFO == code)
		{
			memcpy(&mesg.mesg.db_ipcs, &db_ipcs, sizeof(struct ipcs_mesg_struct));
			/* most of the time file length is much smaller than MAX_TRANS_NAME_LEN */
			mesg.len += (sizeof(struct ipcs_mesg_struct) - MAX_TRANS_NAME_LEN);
			mesg.len += mesg.mesg.db_ipcs.fn_len;
		} else
		{
			mesg.mesg.id = id;
			mesg.len += sizeof(mesg.mesg.id);
		}
		mesg.pid = process_id;
		mesg.seqno = ++cur_seqno;
		sent = 0;
		send_ptr = (char *)&mesg;
		send_len = mesg_len = mesg.len;
		for (send_complete = FALSE; !send_complete;)
		{
  			SENDTO_SOCK(gtmsecshr_sockfd, send_ptr, send_len, 0, (struct sockaddr *)&gtmsecshr_sock_name,
				(sssize_t)gtmsecshr_sockpath_len, num_chars_sent);
			if (0 <= num_chars_sent)
			{
				sent += num_chars_sent;
				if (sent == mesg_len)
				{
					send_complete = TRUE;
					break;
				}
				send_ptr += num_chars_sent;
				send_len -= num_chars_sent;
			} else
			{
				/* sendto failed - start server and attempt to resend */
				save_errno = errno;
				if ((EISCONN == save_errno) || (EBADF == save_errno))
				{
					gtmsecshr_sock_cleanup(CLIENT);
					gtmsecshr_sock_init(CLIENT);
					wcs_backoff(loop_count + 1);
				} else
				{
#ifdef SECSHR_DEBUG
					util_out_print("secshr_client starting server due to sendto failure errno = !UL",
						TRUE, save_errno);
#endif
					send_msg(VARLSTCNT(10) ERR_GTMSECSHRSRVF, 3, RTS_ERROR_TEXT("Client"), process_id,
							ERR_TEXT, 2, RTS_ERROR_TEXT("sendto to gtmsecshr failed"), save_errno);
					START_SERVER;
				}
				loop_count++;
				break;
			}
		}
		if (!send_complete)
			continue;
		SETUP_FOR_RECV;
		for (save_errno = 0, recv_complete = FALSE; !recv_complete;)
		{
			num_chars_recvd = recvfrom(gtmsecshr_sockfd, recv_ptr, MAX_MESG, 0, (struct sockaddr *)0, (sssize_t *)0);
			if (0 <= num_chars_recvd)
			{
				recd += num_chars_recvd;
				if ((0 == mesg_len) && (sizeof(int) <= recd))
					mesg_len = mesg.len;
				if (recd == mesg_len)
				{
					if (mesg.seqno == cur_seqno)
					{
						recv_complete = TRUE;
						break;
					}
					else
					{
						cancel_timer(timer_id);
						recv_complete = FALSE;
						save_errno = 0;
						SETUP_FOR_RECV;
						continue;
					}

				}
				recv_ptr += num_chars_recvd;
				recv_len -= num_chars_recvd;
				continue;
			}
			if (client_timer_popped)
				break;
			if (EINTR == errno)
				continue;
			save_errno = errno;
			if (EBADF == errno)
				break;
			send_msg(VARLSTCNT(10) ERR_GTMSECSHRSRVF, 3, RTS_ERROR_TEXT("Client"), process_id,
					ERR_TEXT, 2, RTS_ERROR_TEXT("recvfrom from gtmsecshr failed"), save_errno);
			if ((ECONNRESET == save_errno) || (ENOTCONN == save_errno))
			{
				num_chars_recvd = 0;
				break;
			}
			gtmsecshr_sock_cleanup(CLIENT);
			cancel_timer(timer_id);
			return save_errno;
		}
		if ((client_timer_popped || (EBADF == save_errno)))
		{
			gtmsecshr_sock_cleanup(CLIENT);
			gtmsecshr_sock_init(CLIENT);
			retry = TRUE;
			if (client_timer_popped)
			{
#ifdef SECSHR_DEBUG
				util_out_print("secshr_client starting server due to recvfrom timeout", TRUE);
#endif
				START_SERVER;
			}
			loop_count++;
			continue;
		}
		cancel_timer(timer_id);
		if (ret_code = mesg.code)		/* assign to ret_code, then test value */
		{
			switch(req_code)
			{
				case CHECK_PROCESS_ALIVE:
					send_msg(VARLSTCNT(13) ERR_GTMSECSHRSRVFID, 6, RTS_ERROR_TEXT("Client"), process_id,
							mesg.pid, req_code, mesg.mesg.id, ERR_TEXT, 2,
							RTS_ERROR_STRING(secshr_fail_mesg_code[req_code]),
							mesg.code);
					break;
				case REMOVE_FILE :
					if (retry  && ENOENT == ret_code)
						ret_code = 0;  /* assume first time worked */
					else
						send_msg(VARLSTCNT(14) ERR_GTMSECSHRSRVFFILE, 7, RTS_ERROR_TEXT("Client"),
							process_id, mesg.pid, req_code, RTS_ERROR_TEXT(mesg.mesg.path),
							ERR_TEXT, 2, RTS_ERROR_STRING(secshr_fail_mesg_code[req_code]),
							mesg.code);
					break;
				case REMOVE_SEM:
				case REMOVE_SHMMEM:
					if (retry && (mesg.mesg.id == id))
						ret_code = 0;
					break;
				case FLUSH_DB_IPCS_INFO:
					break;
				default :
					if (EPERM != mesg.code && EACCES != mesg.code)
						send_msg(VARLSTCNT(13) ERR_GTMSECSHRSRVFID, 6, RTS_ERROR_TEXT("Client"),
							process_id, mesg.pid, req_code, mesg.mesg.id, ERR_TEXT, 2,
							RTS_ERROR_STRING(secshr_fail_mesg_code[req_code]),
							mesg.code);
					break;

			}
		}
		if (recv_complete)
			break;
	}
	if (MAX_RETRIES == loop_count)
	{
		ret_code = -1;
		gtm_putmsg(VARLSTCNT(9) ERR_GTMSECSHRSRVF, 3, RTS_ERROR_TEXT("Client"), process_id,
			ERR_TEXT, 2, RTS_ERROR_TEXT("Unable to communicate with gtmsecshr"));
	}
	if (ONETIMESOCKET == init_ret_code)
		gtmsecshr_sock_cleanup(CLIENT);
	return ret_code;
}

int create_server (void)
{
	int		child_pid, done_pid, status = 0;
#ifdef _BSD
	union	wait	chld_status;
#	define CSTAT	chld_status
#else
#	define CSTAT	status
#endif
	int		save_errno;

	error_def(ERR_GTMSECSHRSTARTUP);
	error_def(ERR_GTMSECSHRSRVF);
	error_def(ERR_TEXT);

        if (0 == (child_pid = fork()))
	{
		process_id = getpid();

		/* do exec using gtmsecshr_path, which was initialize in file check code - send_mesg2gtmsecshr */
		status = EXECL(gtmsecshr_path, gtmsecshr_path, 0);
		if (-1 == status)
		{
                        send_msg(VARLSTCNT(9) ERR_GTMSECSHRSTARTUP, 3, RTS_ERROR_TEXT("Client"), process_id,
					ERR_TEXT, 2, RTS_ERROR_STRING(secshr_unbl_start_mesg_code[INVTRANSGTMSECSHR]));
			exit(UNABLETOEXECGTMSECSHR);
		}
        } else
	{
		if (-1 == child_pid)
		{
			status = GNDCHLDFORKFLD;
			gtm_putmsg(VARLSTCNT(10) ERR_GTMSECSHRSTARTUP, 3, RTS_ERROR_TEXT("Client"), process_id,
					ERR_TEXT,  2, RTS_ERROR_TEXT("Failed to fork off gtmsecshr"), errno);
			/* Sleep for a while and hope a subsequent fork will succeed */
			hiber_start(1000);
		}
		for ( ; !status ; )
		{
			/* To prevent a warning message that the compiler issues */
			done_pid = wait(&CSTAT);
			if (done_pid == child_pid)
			{
				status = GETLASTBYTE(WEXITSTATUS(CSTAT));
				break;
			} else if (-1 == done_pid)
			{
				if (ECHILD == errno) /* Assume normal exit status */
					break;
				else if (EINTR != errno)
				{
					status = GNDCHLDFORKFLD;
					gtm_putmsg(VARLSTCNT(10) ERR_GTMSECSHRSTARTUP, 3, RTS_ERROR_TEXT("Client"), process_id,
							ERR_TEXT, 2, RTS_ERROR_TEXT("Error spawning gtmsecshr"), errno);
				}
			}
		}
	}
	return status;
}
