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

#include "mdef.h"

#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>

#include "gtm_string.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsblk.h"
#include "gtmio.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "io.h"
#include "iosp.h"
#include "eintr_wrappers.h"
#include "gtmsecshr.h"
#include "mu_rndwn_file.h"
#include "ftok_sems.h"
#include "ipcrmid.h"
#include "gtmmsg.h"
#include "dbfilop.h"

GBLREF uint4		process_id;
GBLREF ipcs_mesg	db_ipcs;
GBLREF gd_region        *gv_cur_region;
GBLREF gd_region	*standalone_reg;
/*
 * This routine is the counterpart of mu_rndwn_file() for standalone access.
 * In mu_rndwn_file() we got database access control semaphore.
 * We release it here.
 * As mu_rndwn_file it works for one region only.
 * 'immediate' flag is to control of IPC_WAIT flag to use or, not for system calls.
 */
boolean_t db_ipcs_reset(gd_region *reg, boolean_t immediate)
{
	int			status;
	uint4			ustatus;
	sgmnt_data_ptr_t	csd;
	file_control		*fc;
	unix_db_info		*udi;
	gd_region		*temp_region;

	error_def (ERR_TEXT);
	error_def (ERR_CRITSEMFAIL);
	error_def (ERR_DBFILERR);
	error_def (ERR_FILEPARSE);

	assert(reg);
	temp_region = gv_cur_region; 	/* save gv_cur_region wherever there is scope for it to be changed */
	gv_cur_region = reg; /* dbfilop needs gv_cur_region */
	udi = NULL;
	if (NULL != reg->dyn.addr->file_cntl)
		udi = FILE_INFO(reg);
	if ((NULL == udi) || (INVALID_SEMID == udi->ftok_semid) || (INVALID_SEMID == udi->semid))
	{
		assert(!reg->open);
		gv_cur_region = temp_region;
		return FALSE;
	}
	csd = (sgmnt_data_ptr_t)malloc(sizeof(*csd));
	fc = reg->dyn.addr->file_cntl;
	fc->op = FC_OPEN;
	status = dbfilop(fc);
	gv_cur_region = temp_region;
	if (SS_NORMAL != status)
	{
		gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
                return FALSE;
	}
	if (!ftok_sem_lock(reg, FALSE, immediate))
		return FALSE;
	/* Now we have locked the database using ftok_sem. Any other ftok conflicted database will
	 * suspend at this point, because of the lock.
	 * At the end of this routine, we release ftok semaphore.
	 * Now read file header and continue with main operation of this routine.
	 */
	LSEEKREAD(udi->fd, (off_t)0, csd, sizeof(*csd), status);
	if (0 != status)
	{
		gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
		CLOSEFILE(udi->fd, status);
		if (0 != status)
			gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
		return FALSE;
	}
	if (!reg->read_only)
	{
		csd->semid = INVALID_SEMID;
		csd->shmid = INVALID_SHMID;
		csd->sem_ctime.ctime = 0;
		csd->shm_ctime.ctime = 0;
		LSEEKWRITE(udi->fd, (off_t)0, csd, sizeof(*csd), status);
		if (0 != status)
		{
			gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
			CLOSEFILE(udi->fd, status);
			if (0 != status)
				gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
			return FALSE;
		}
	} else
	{
		db_ipcs.semid = INVALID_SEMID;
		db_ipcs.shmid = INVALID_SHMID;
		db_ipcs.sem_ctime = 0;
		db_ipcs.shm_ctime = 0;
		if (!get_full_path((char *)DB_STR_LEN(reg), db_ipcs.fn, (unsigned int *)&db_ipcs.fn_len,
					MAX_TRANS_NAME_LEN, &ustatus))
		{
			gtm_putmsg(VARLSTCNT(5) ERR_FILEPARSE, 2, DB_LEN_STR(reg), ustatus);
			return FALSE;
		}
		db_ipcs.fn[db_ipcs.fn_len] = 0;
		if (0 != (status = send_mesg2gtmsecshr(FLUSH_DB_IPCS_INFO, 0, (char *)NULL, 0)))
		{
			gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
			CLOSEFILE(udi->fd, status);
			if (0 != status)
				gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
			return FALSE;
		}
	}
	CLOSEFILE(udi->fd, status);
	if (0 != status)
		gtm_putmsg(VARLSTCNT(5) ERR_DBFILERR, 2, DB_LEN_STR(reg), status);
	if (0 != sem_rmid(udi->semid))
	{
		gtm_putmsg(VARLSTCNT(8) ERR_CRITSEMFAIL, 2, DB_LEN_STR(reg),
			ERR_TEXT, 2, RTS_ERROR_TEXT("sem_rmid of semid failed"));
		return FALSE;
	}
	if (!ftok_sem_release(reg, TRUE, immediate))
		return FALSE;
	udi->semid = INVALID_SEMID;
	udi->shmid = INVALID_SHMID;
	udi->sem_ctime = 0;
	udi->shm_ctime = 0;
	free(csd);
	assert((reg != standalone_reg) || (NULL == standalone_reg));	/* ftok_sem_release() should have NULLified it */
	standalone_reg = NULL;		/* just in case */
	return TRUE;
}
