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

#include "mdef.h"

#include "gtm_string.h"


#include "gtm_stdio.h"
#include "gtm_stdlib.h"		/* for exit() */
#include "gtm_unistd.h"		/* for getopt() and read() */

#include "iosp.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>		/* for msgget() and msgctl() prototype */
#include <sys/sem.h>		/* for semget() and semctl() prototype */

#include "daemon.h"
#include "rc_cpt.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "trans_log_name.h"	/* for trans_log_name() prototype */

int	quiet = 0;

clean_mem(char *name)
{
	mstr	path1, path2;
	int	semid;
	key_t	msg_key;
	char	buff[512];
	int	q_id, s_id, m_id;

	path1.len = strlen(name);
	path1.addr = name;
	if (trans_log_name(&path1, &path2, buff) != SS_NORMAL)
		fprintf(stderr, "Error translating path: %s\n", name);
	else
	{
		path2.addr[path2.len] = '\0';
		if ((msg_key = ftok(path2.addr, GTM_ID)) == -1)
			perror("shmclean: Error with ftok");
		else
		{
			if ((q_id = msgget(msg_key, 0)) != -1)
			{
				if (!quiet)
					printf("shmclean: removing msg queue %d\n", q_id);
				if (msgctl(q_id, IPC_RMID, 0) == -1)
				{
					fprintf(stderr,"shmclean: error removing msg queue %d\n", q_id);
					perror("shmclean");
				}
			}
			if ((semid = semget(msg_key, 2, RWDALL)) != -1)
			{
				if (!quiet)
					printf("shmclean: removing semid %d\n", semid);
				if (semctl(semid, 0, IPC_RMID, 0) == -1)
				{
					fprintf(stderr,"shmclean: error removing semid %d\n", semid);
					perror("shmclean");
				}
			} else
				semid = INVALID_SEMID;
			if ((m_id = shmget(msg_key, 10, RWDALL)) != -1)
			{
				if (!quiet)
					printf("shmclean: removing shmid %d\n", m_id);
				if (shmctl(m_id, IPC_RMID, 0) == -1)
				{
					fprintf(stderr,"shmclean: error removing shmid %d\n", m_id);
					perror("shmclean");
				}
			} else
				m_id = INVALID_SEMID;
		}
	}
}

database_clean(char *path)
{
	key_t	d_key;
	int	shmid;
	int	semid;

	if ((d_key = ftok(path, GTM_ID)) == -1)
	{
		perror("Error with database ftok");
		printf("File: %s\n", path);
		return;
	}
	if ((shmid = shmget(d_key, 10, RWALL)) != -1)
	{
		if (!quiet)
			printf("shmclean: removing shmid %d\n", shmid);
		if (shmctl(shmid, IPC_RMID, 0) == -1)
		{
			fprintf(stderr,"shmclean: error removing shmid %d\n", shmid);
			perror("shmclean");
		}
	} else
		shmid = INVALID_SHMID;
	if ((semid = semget(d_key, 2, 0600)) != -1)
	{
		if (!quiet)
			printf("shmclean: removing semid %d\n", semid);
		if (semctl(semid, 0, IPC_RMID, 0) == -1)
		{
			fprintf(stderr,"shmclean: error removing semid %d\n", semid);
			perror("shmclean");
		}
	} else
		semid = INVALID_SEMID;
}

/* On OSF/1 (Digital Unix), pointers are 64 bits wide; the only exception to this is C programs for which one may
 * specify compiler and link editor options in order to use (and allocate) 32-bit pointers.  However, since C is
 * the only exception and, in particular because the operating system does not support such an exception, the argv
 * array passed to the main program is an array of 64-bit pointers.  Thus the C program needs to declare argv[]
 * as an array of 64-bit pointers and needs to do the same for any pointer it sets to an element of argv[].
 */
int main(int argc, char_ptr_t argv[])
{
	key_t	d_msg_key, key, s_msg_key;
	int	q_id, s_id, m_id;
	mstr	dpath1, dpath2, fpath1, fpath2;
	int	server_sem, daemon_sem;
	char	buff[512];
	int	server = 0;
	int	daemon = 0;
	char	resp;
	int	opt, err;

	err = 0;
	while ((opt = getopt(argc, argv, "qds")) != -1)
	{
		switch (opt)
		{
			case 'q':
				quiet = 1;
				break;
			case 'd':
				daemon = 1;
				break;
			case 's':
				server = 1;
				break;
			/* mupip rundown should be used for databases. */
			/*
			 *   case 'D':
			 *    database_clean(optarg);
			 *    break;
			 */
		}
	}
	if (quiet != 1)
	{
		fprintf(stderr,"If this program is used to remove shared memory from running\n");
		fprintf(stderr,"processes, it will cause the program to fail. Please make\n");
		fprintf(stderr,"sure all GTM processes have been shut down cleanly before running\n");
		fprintf(stderr,"this program.\n\n");
		fprintf(stderr,"Do you want to contine? (y or n)  ");
		read(0, &resp, 1);
		if ((resp != 'y') && (resp != 'Y'))
		{
			exit(0);
		}
	}
	if (daemon == 1)
		clean_mem(DAEMON_PATH);
	if (server == 1 && daemon == 0)
		clean_mem(RC_CPT_PATH);
}
