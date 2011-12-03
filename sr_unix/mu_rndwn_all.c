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

#include "gtm_ipc.h"
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include "gtm_unistd.h"
#include <arpa/inet.h>
#include "gtm_stdlib.h"
#include "gtm_string.h"
#include <sys/sem.h>
#include "gtm_sem.h"
#include "gtm_stat.h"
#include "gtm_stdio.h"
#include "gtmio.h"

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "iosp.h"
#include "mutex.h"
#include "jnl.h"
#include "repl_sem.h"
#include "eintr_wrappers.h"
#include "mu_rndwn_file.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "gtmrecv.h"
#include "gtm_logicals.h"
#include "min_max.h"
#include "util.h"
#include "mu_rndwn_replpool.h"
#include "mu_rndwn_all.h"
#include "mu_gv_cur_reg_init.h"
#include "gtmmsg.h"
#include "cliif.h"
#include "mu_rndwn_repl_instance.h"
#include "send_msg.h"
#include "do_shmat.h"	/* for do_shmat() prototype */

GBLREF gd_region        *gv_cur_region;

LITREF char             gtm_release_name[];
LITREF int4             gtm_release_name_len;

#define	TMP_BUF_LEN	50

boolean_t validate_db_shm_entry(char *, char *, int *);
boolean_t validate_replpool_shm_entry(char *, replpool_id_ptr_t, int *, int *);
shm_parms *get_shm_parm(char *entry);
char *parse_shm_entry(char *entry, int which_field);

int mu_rndwn_all(void)
{
	int 			save_errno, fname_len, exit_status = SS_NORMAL, shmid, tmp_exit_status;
	char			entry[MAX_ENTRY_LEN];
	FILE			*pf;
	char			*fname, *fgets_res, shmid_buff[TMP_BUF_LEN];
	boolean_t 		ret_status;
	replpool_identifier	replpool_id;
	uchar_ptr_t		ret_ptr;

	error_def(ERR_MUNOTALLSEC);
	error_def(ERR_MUFILRNDWNSUC);
	error_def(ERR_MUJPOOLRNDWNSUC);
	error_def(ERR_MURPOOLRNDWNSUC);
	error_def(ERR_MUJPOOLRNDWNFL);
	error_def(ERR_MURPOOLRNDWNFL);
	error_def(ERR_REPLACCSEM);
	error_def(ERR_SYSCALL);
	error_def(ERR_TEXT);

	if (NULL == (pf = POPEN(IPCS_CMD_STR ,"r")))
        {
		save_errno = errno;
		gtm_putmsg(VARLSTCNT(8) ERR_SYSCALL, 5, RTS_ERROR_LITERAL("POPEN()"), CALLFROM, save_errno);
                return ERR_MUNOTALLSEC;
        }
	fname = (char *)malloc(MAX_FN_LEN + 1);
	while (NULL != (FGETS(entry, sizeof(entry), pf, fgets_res)) && entry[0] != '\n')
	{
		tmp_exit_status = SS_NORMAL;
		if (validate_db_shm_entry(entry, fname, &tmp_exit_status))
		{
			mu_gv_cur_reg_init();
			strcpy((char *)gv_cur_region->dyn.addr->fname, fname);
			gv_cur_region->dyn.addr->fname_len = strlen(fname);
			if (mu_rndwn_file(gv_cur_region, FALSE))
			{
				gtm_putmsg(VARLSTCNT(4) ERR_MUFILRNDWNSUC, 2, gv_cur_region->dyn.addr->fname_len,
						gv_cur_region->dyn.addr->fname);
			} else
				exit_status = ERR_MUNOTALLSEC;
			mu_gv_cur_reg_free();
		} else if (tmp_exit_status == SS_NORMAL &&
				validate_replpool_shm_entry(entry, (replpool_id_ptr_t)&replpool_id, &tmp_exit_status, &shmid))
		{
			assert(JNLPOOL_SEGMENT == replpool_id.pool_type || RECVPOOL_SEGMENT == replpool_id.pool_type);
			ret_status = mu_rndwn_repl_instance(&replpool_id, TRUE);
			ret_ptr = i2asc((uchar_ptr_t)shmid_buff, shmid);
			*ret_ptr = '\0';
			gtm_putmsg(VARLSTCNT(6) (JNLPOOL_SEGMENT == replpool_id.pool_type) ?
				(ret_status ? ERR_MUJPOOLRNDWNSUC : ERR_MUJPOOLRNDWNFL) :
				(ret_status ? ERR_MURPOOLRNDWNSUC : ERR_MURPOOLRNDWNFL),
				4, LEN_AND_STR(shmid_buff), LEN_AND_STR(replpool_id.instname));
			if (!ret_status)
				exit_status = ERR_MUNOTALLSEC;
		}
		if (SS_NORMAL == exit_status && SS_NORMAL != tmp_exit_status)
			exit_status = tmp_exit_status;
	}
	pclose(pf);
	free(fname);
	return exit_status;
}

/* Takes an entry from 'ipcs -m' and checks for its validity to be a GT.M db segment.
 * Returns TRUE if the shared memory segment is a valid GT.M db segment
 * (based on a check on some fields in the shared memory) else FALSE.
 * If the segment belongs to GT.M it returns the database file name by
 * the second argument.
 * Sets exit_stat to ERR_MUNOTALLSEC if appropriate.
 */

boolean_t validate_db_shm_entry(char *entry, char *fname, int *exit_stat)
{
	int			fname_len;
	struct stat		st_buff;
	node_local_ptr_t	nl_addr;
	sm_uc_ptr_t		start_addr;
	shm_parms		*parm_buff;

	error_def(ERR_MUNOTALLSEC);

	parm_buff = get_shm_parm(entry);
	if (parm_buff)
	{
		/* check for the bare minimum size of the shared memory segment that we expect
		 * (with no fileheader related information at hand) */
		if (NODE_LOCAL_SPACE + BACKUP_BUFFER_SIZE > parm_buff->sgmnt_siz)
		{
			free(parm_buff);
			return FALSE;
		}
		if (IPC_PRIVATE != parm_buff->key)
		{
			free(parm_buff);
			return FALSE;
		}
		/* we do not need to lock the shm for reading the rundown information as
		 * the other rundowns (if any) can also be allowed to share reading the
		 * same info concurrently.
		 */
		if (-1 == (sm_long_t)(start_addr = (sm_uc_ptr_t) do_shmat(parm_buff->shmid, 0, SHM_RND)))
		{
			free(parm_buff);
			return FALSE;
		}
		nl_addr = (node_local_ptr_t)start_addr;
		memcpy(fname, nl_addr->fname, MAX_FN_LEN + 1);
		fname[MAX_FN_LEN] = '\0';			/* make sure the fname is null terminated */
		fname_len = strlen(fname);
		if (-1 == Stat(fname, &st_buff))		/* check if there is any such file */
		{
			shmdt((void *)start_addr);
			free(parm_buff);
			return FALSE;
		}
		if (memcmp(nl_addr->label, GDS_LABEL, GDS_LABEL_SZ - 1))
		{
			if (!memcmp(nl_addr->label, GDS_LABEL, GDS_LABEL_SZ - 3))
			{
				util_out_print("!AD -> Incorrect database version.", TRUE, fname_len, fname);
				*exit_stat = ERR_MUNOTALLSEC;
			}
			shmdt((void *)start_addr);
			free(parm_buff);
			return FALSE;
		}
		if (memcmp(nl_addr->now_running, gtm_release_name, gtm_release_name_len + 1))
		{
			util_out_print("!AD -> Attempt to access with version !AD, while already using !AD.",
					TRUE, fname_len, fname, gtm_release_name_len, gtm_release_name,
					LEN_AND_STR(nl_addr->now_running));
			shmdt((void *)start_addr);
			free(parm_buff);
			*exit_stat = ERR_MUNOTALLSEC;
			return FALSE;
		}
	}else
		return FALSE;
	shmdt((void *)start_addr);
	free(parm_buff);
	return TRUE;
}

/* Takes an entry from 'ipcs -am' and checks for its validity to be a GT.M replication segment.
 * Returns TRUE if the shared memory segment is a valid GT.M replication segment
 * (based on a check on some fields in the shared memory) else FALSE.
 * If the segment belongs to GT.M, it returns the replication id of the segment
 * by the second argument.
 * Sets exit_stat to ERR_MUNOTALLSEC if appropriate.
 */
boolean_t validate_replpool_shm_entry(char *entry, replpool_id_ptr_t replpool_id, int *exit_stat, int *shmid)
{
	sm_uc_ptr_t		start_addr;
	shm_parms		*parm_buff;
	int			fd;
	struct stat		st_buff;

	error_def(ERR_MUNOTALLSEC);
	error_def(ERR_REPLACCSEM);
	error_def(ERR_TEXT);

	parm_buff = get_shm_parm(entry);
	if (parm_buff)
	{
		*shmid = parm_buff->shmid;
		/* Check for the bare minimum size of the replic shared segment that we expect */
		/* if (parm_buff->sgmnt_siz < (sizeof(replpool_identifier) + MIN(MIN_JNLPOOL_SIZE, MIN_RECVPOOL_SIZE))) */
		if (parm_buff->sgmnt_siz < MIN(MIN_JNLPOOL_SIZE, MIN_RECVPOOL_SIZE))
		{
			free(parm_buff);
			return FALSE;
		}
		if (IPC_PRIVATE != parm_buff->key)
		{
			free(parm_buff);
			return FALSE;
		}
		/* we do not need to lock the shm for reading the rundown information as
		 * the other rundowns (if any) can also be allowed to share reading the
		 * same info concurrently.
		 */
		if (-1 == (sm_long_t)(start_addr = (sm_uc_ptr_t) do_shmat(parm_buff->shmid, 0, SHM_RND)))
		{
			free(parm_buff);
			return FALSE;
		}
		memcpy((void *)replpool_id, (void *)start_addr, sizeof(replpool_identifier));
		if (memcmp(replpool_id->label, GDS_RPL_LABEL, GDS_LABEL_SZ - 1))
		{
			if (!memcmp(replpool_id->label, GDS_RPL_LABEL, GDS_LABEL_SZ - 3))
			{
				util_out_print("Incorrect replpool version (shm id = !UL).", TRUE, parm_buff->shmid);
				*exit_stat = ERR_MUNOTALLSEC;
			}
			shmdt((void *)start_addr);
			free(parm_buff);
			return FALSE;
		}
		assert(JNLPOOL_SEGMENT == replpool_id->pool_type || RECVPOOL_SEGMENT == replpool_id->pool_type);
		if(JNLPOOL_SEGMENT != replpool_id->pool_type && RECVPOOL_SEGMENT != replpool_id->pool_type)
		{
			shmdt((void *)start_addr);
			free(parm_buff);
			return FALSE;
		}
		/*
		 * If we can open instance file, we can use it to rundown the replication instance.
		 * We do this checking here, because later repl_inst_get() will issue rts_eror(), if it cannot open.
		 * We do not want to change repl_inst_get(), which is used by others too.
		 * As a result currently, mupip rundown will not continue to next shared memory segment
		 * for this kind of transient error.
		 * We need to change all rts_error() to gtm_putmsg() in the code path of mupip rundown
		 * for future enhancement. - Layek - 5/1/1.
		 */
		OPENFILE(replpool_id->instname, O_RDONLY, fd);	/* check if we can open it */
		if (-1 == fd)
		{
			shmdt((void *)start_addr);
			free(parm_buff);
			return FALSE;
		}
		close(fd);
		shmdt((void *)start_addr);
		free(parm_buff);
		return TRUE;
	}
	return FALSE;
}

/* Gets all the required fields in shm_parms struct for a given shm entry */

shm_parms *get_shm_parm(char *entry)
{
	char 		*parm;
	shm_parms	*parm_buff;
	struct shmid_ds	shm_buf;

	parm_buff = (shm_parms *)malloc(sizeof(shm_parms));

	parm = parse_shm_entry(entry, SHMID);
	CONVERT_TO_NUM(shmid);
	parm = parse_shm_entry(entry, KEY);
	CONVERT_TO_NUM(key);

	/* get shm segment size directly from shmid_ds instead of parsing ipcs
	 * output (thus avoiding the -a option for ipcs in mu_rndwn_all()
	 */
	if (-1 == shmctl(parm_buff->shmid, IPC_STAT, &shm_buf))
	{
		free(parm_buff);
		return NULL;
	}
	parm_buff->sgmnt_siz = shm_buf.shm_segsz;
	return parm_buff;
}

/* Parses the output of 'IPCS_CMD_STR' command. Returns the value of the
 * specified field.
 */

/* NOTE : Even though the standard says that every column in the output
 * of 'ipcs' command should be separated by atleast one space, we have
 * observed a case where there is no space between the first (T) and
 * second (ID) fields on AIX under certain conditions.
 * The workaround is to always insert a blank space for all UNIX platforms
 * after the first (T field) character assuming the entry always starts with a
 * character describing the type of the ipc resource ('m' for shared memory).
 * On linux, the ipcs output starts with a KEY field (a hexadecimal number).
 * See the definition of IPCS_CMD_STR for this handling
 */
char *parse_shm_entry(char *entry, int which_field)
{
	char 	*parm;
	int 	iter, indx1 = 0, indx2 = 0;

	for(iter = 1; iter < which_field; iter++)
	{
		while(entry[indx1] == ' ')
			indx1++;
		while(entry[indx1] && entry[indx1] != ' ')
			indx1++;
	}
	while(entry[indx1] == ' ')
		indx1++;
	if ('\0' == entry[indx1])
	{
		assert(FALSE);
		return NULL;
	}
	parm = (char *)malloc(MAX_PARM_LEN);
	memset(parm, 0, MAX_PARM_LEN);
	while(entry[indx1] && entry[indx1] != ' ')
		parm[indx2++] = entry[indx1++];
	parm[indx2] = '\0';

	return parm;
}

int parse_sem_id(char *entry)
{
	char 	*parm;
	int 	iter, indx1 = 0, indx2;

	while(entry[indx1] == ' ')
		indx1++;
	while(entry[indx1] && entry[indx1] != ' ')
		indx1++;
	while(entry[indx1] == ' ')
		indx1++;
	if ('\0' == entry[indx1])
	{
		assert(FALSE);
		return -1;
	}
	indx2 = indx1;
	parm = &entry[indx1];
	while(entry[indx2] && entry[indx2] != ' ')
		indx2++;
	entry[indx2] = '\0';
	if (cli_is_dcm(parm))
		return STRTOUL(parm, NULL, 10);
	else if (cli_is_hex(parm + 2))
		return STRTOUL(parm, NULL, 16);
	else
	{
		assert(FALSE);
		return -1;
	}
}

int mu_rndwn_sem_all(void)
{
	int 			save_errno, exit_status = SS_NORMAL, semid;
	char			entry[MAX_ENTRY_LEN];
	FILE			*pf;
	char			fname[MAX_FN_LEN + 1], *fgets_res;
	boolean_t 		rem_sem;
	shm_parms		*parm_buff;

	error_def(ERR_MUNOTALLSEC);
	error_def(ERR_SEMREMOVED);
	error_def(ERR_TEXT);
	error_def(ERR_SYSCALL);

	if (NULL == (pf = POPEN(IPCS_SEM_CMD_STR ,"r")))
        {
		save_errno = errno;
		gtm_putmsg(VARLSTCNT(8) ERR_SYSCALL, 5, RTS_ERROR_LITERAL("POPEN()"), CALLFROM, save_errno);
                return ERR_MUNOTALLSEC;
        }
	while (NULL != (FGETS(entry, sizeof(entry), pf, fgets_res)) && entry[0] != '\n')
	{
		if (-1 != (semid = parse_sem_id(entry)))
		{
			if (is_orphaned_gtm_semaphore(semid))
			{
				if (-1 != semctl(semid, 0, IPC_RMID))
				{
					gtm_putmsg(VARLSTCNT(3) ERR_SEMREMOVED, 1, semid);
					send_msg(VARLSTCNT(3) ERR_SEMREMOVED, 1, semid);
				}
			}
		}
	}
	pclose(pf);
	return exit_status;
}
boolean_t is_orphaned_gtm_semaphore(int semid)
{
	int			semno, semval;
	struct semid_ds		semstat;
	union semun		semarg;

	semarg.buf = &semstat;
	if (-1 != semctl(semid, 0, IPC_STAT, semarg))
	{
		if (-1 == (semval = semctl(semid, semarg.buf->sem_nsems - 1, GETVAL)) || GTM_ID != semval)
			return FALSE;
		else
		{
			/* Make sure all has value = 0 */
			for (semno = 0; semno < semarg.buf->sem_nsems - 1; semno++)
				if (-1 == (semval = semctl(semid, semno, GETVAL)) || semval)
					return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
