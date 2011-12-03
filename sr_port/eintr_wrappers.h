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

/* Define macros to do system calls and restart as appropriate
 *
 * FCNTL, FCNTL3	Loop until fcntl call succeeds or fails with other than EINTR.
 * TCFLUSH		Loop until tcflush call succeeds or fails with other than EINTR.
 * TCSETATTR		Loop until tcsetattr call succeeds or fails with other than EINTR.
 */

#ifndef EINTR_WRP_Included
#define EINTR_WRP_Included

#include <sys/types.h>
#include <errno.h>

#define ACCEPT_SOCKET(SOCKET, ADDR, LEN, RC)	\
{						\
	do					\
	{					\
	   RC = ACCEPT(SOCKET, ADDR, LEN);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define CHG_OWNER(PATH, OWNER, GRP, RC)		\
{						\
	do					\
	{					\
	   RC = CHOWN(PATH, OWNER, GRP);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define CLOSEDIR(DIR, RC)			\
{						\
	do					\
	{					\
	   RC = closedir(DIR);			\
	} while(-1 == RC && EINTR == errno);	\
}

#define CONNECT_SOCKET(SOCKET, ADDR, LEN, RC)	\
{						\
	do					\
	{					\
	   RC = CONNECT(SOCKET, ADDR, LEN);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define CREATE_FILE(PATHNAME, MODE, RC)		\
{						\
	do					\
	{					\
	   RC = CREAT(PATHNAME, MODE);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define DOREAD_A_NOINT(FD, BUF, SIZE, RC)	\
{						\
	do					\
	{					\
	   RC = DOREAD_A(FD, BUF, SIZE);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define DUP2(FDESC1, FDESC2, RC)		\
{						\
	do					\
	{					\
	   RC = dup2(FDESC1, FDESC2);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define FCLOSE(STREAM, RC)			\
{						\
	do					\
	{					\
	   RC = fclose(STREAM);			\
	} while(-1 == RC && EINTR == errno);	\
}

#define FCNTL2(FDESC, ACTION, RC)		\
{						\
	do					\
	{					\
	   RC = fcntl(FDESC, ACTION);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define FCNTL3(FDESC, ACTION, ARG, RC)		\
{						\
	do					\
	{					\
	   RC = fcntl(FDESC, ACTION, ARG);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define FGETS_FILE(BUF, LEN, FP, RC)		\
{						\
	do					\
	{					\
	   FGETS(BUF, LEN, FP, RC);		\
	} while(NULL == RC && !feof(FP) && ferror(FP) && EINTR == errno);	\
}

#define FSTAT_FILE(FDESC, INFO, RC)		\
{						\
	do					\
	{					\
	   RC = fstat(FDESC, INFO);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define FSTATVFS_FILE(FDESC, FSINFO, RC)	\
{						\
	do					\
	{					\
	   FSTATVFS(FDESC, FSINFO, RC);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define FTRUNCATE(FDESC, LENGTH, RC)		\
{						\
	do					\
	{					\
	   RC = ftruncate(FDESC, LENGTH);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define MSGSND(MSGID, MSGP, MSGSZ, FLG, RC)	\
{						\
	do					\
	{					\
	   RC = msgsnd(MSGID, MSGP, MSGSZ, FLG);\
	} while(-1 == RC && EINTR == errno);	\
}

#define OPEN_PIPE(FDESC, RC)			\
{						\
	do					\
	{					\
	   RC = pipe(FDESC);			\
	} while(-1 == RC && EINTR == errno);	\
}

#define READ_FILE(FD, BUF, SIZE, RC)		\
{						\
	do					\
	{					\
	   RC = read(FD, BUF, SIZE);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define RECVFROM_SOCK(SOCKET, BUF, LEN, FLAGS,	\
		 ADDR, ADDR_LEN, RC)		\
{						\
	do					\
	{					\
	   RC = RECVFROM(SOCKET, BUF, LEN,	\
			 FLAGS, ADDR, ADDR_LEN);\
	} while(-1 == RC && EINTR == errno);	\
}

#define SELECT(FDS, INLIST, OUTLIST, XLIST,	\
		 TIMEOUT, RC)			\
{						\
        struct timeval eintr_select_timeval;	\
	do					\
	{					\
	   eintr_select_timeval = *(TIMEOUT);	\
	   RC = select(FDS, INLIST, OUTLIST,	\
		         XLIST,			\
		         &eintr_select_timeval);\
	} while(-1 == RC && EINTR == errno);	\
}

#define SEMOP(SEMID, SOPS, NSOPS, RC)		\
{						\
	do					\
	{					\
	   RC = semop(SEMID, SOPS, NSOPS);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define SEND(SOCKET, BUF, LEN, FLAGS, RC)	\
{						\
	do					\
	{					\
	   RC = send(SOCKET, BUF, LEN, FLAGS);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define SENDTO_SOCK(SOCKET, BUF, LEN, FLAGS,	\
		 ADDR, ADDR_LEN, RC)		\
{						\
	do					\
	{					\
	   RC = SENDTO(SOCKET, BUF, LEN, FLAGS,	\
			 ADDR, ADDR_LEN);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define STAT_FILE(PATH, INFO, RC)		\
{						\
	do					\
	{					\
	   RC = Stat(PATH, INFO);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define TCFLUSH(FDESC, REQUEST, RC)		\
{						\
	do					\
	{					\
	   RC = tcflush(FDESC, REQUEST);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define Tcsetattr(FDESC, WHEN, TERMPTR, RC)	\
{						\
	do					\
	{					\
	   RC = tcsetattr(FDESC, WHEN, TERMPTR);\
	} while(-1 == RC && EINTR == errno);	\
}

#define TRUNCATE_FILE(PATH, LENGTH, RC)		\
{						\
	do					\
	{					\
	   RC = TRUNCATE(PATH, LENGTH);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define WAIT(STATUS, RC)			\
{						\
	do					\
	{					\
	   RC = wait(STATUS);			\
	} while(-1 == RC && EINTR == errno);	\
}

#define WAITPID(PID, STATUS, OPTS, RC)		\
{						\
	do					\
	{					\
	   RC = waitpid(PID, STATUS, OPTS);	\
	} while(-1 == RC && EINTR == errno);	\
}

#define WRITE_FILE(FD, BUF, SIZE, RC)		\
{						\
	do					\
	{					\
	   RC = write(FD, BUF, SIZE);		\
	} while(-1 == RC && EINTR == errno);	\
}

#define GTM_FSYNC(FD, RC)			\
{						\
	do					\
	{					\
	   RC = fsync(FD);			\
	} while(-1 == RC && EINTR == errno);	\
}

#define SIGPROCMASK(FUNC, NEWSET, OLDSET, RC)		\
{							\
	do						\
	{						\
	  RC = sigprocmask(FUNC, NEWSET, OLDSET);	\
	} while (-1 == RC && EINTR == errno);		\
}

#endif
