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
#include <sys/sem.h>
#include <errno.h>
#include <fcntl.h>
#include "gtm_unistd.h"
#include <arpa/inet.h>
#include "gtm_string.h"
#include <stddef.h>

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "iosp.h"
#include "gtmrecv.h"
#include "repl_msg.h"
#include "gtmsource.h"
#include "gtm_logicals.h"
#include "jnl.h"
#include "repl_sem.h"
#include "repl_shutdcode.h"
#include "io.h"
#include "trans_log_name.h"
#include "repl_instance.h"
#include "gtmmsg.h"
#include "gtm_sem.h"
#include "mu_rndwn_replpool.h"
#include "ftok_sems.h"

GBLREF	jnlpool_addrs		jnlpool;
GBLREF	recvpool_addrs		recvpool;
GBLREF	gd_region		*gv_cur_region;

/*
 * Description:
 * 	Grab ftok semaphore on replication instance file
 *	Release all replication semaphores for the instance (both jnlpool and recvpool)
 * 	Release ftok semaphore
 * Parameters:
 * Return Value: TRUE, if succsessful
 *	         FALSE, if fails.
 */
boolean_t mu_replpool_remove_sem(boolean_t immediate)
{
	char            *instname;
	gd_region	*replreg;
	unix_db_info	*udi;
	unsigned int	full_len;
	int		save_errno;

	error_def(ERR_REPLFTOKSEM);
	error_def(ERR_REPLACCSEM);

	/*
	 * JNL POOL SEMAPHORES
	 */
	replreg = jnlpool.jnlpool_dummy_reg;
	assert(replreg);
	instname = (char *)replreg->dyn.addr->fname;
	full_len = replreg->dyn.addr->fname_len;
	if (0 == full_len)
		return TRUE;
	if (!ftok_sem_get(replreg, TRUE, REPLPOOL_ID, immediate))
		rts_error(VARLSTCNT(4) ERR_REPLFTOKSEM, 2, full_len, instname);
	if (0 != remove_sem_set(SOURCE))
	{
		save_errno = REPL_SEM_ERRNO;
		if (!ftok_sem_release(replreg, TRUE, TRUE))
			gtm_putmsg(VARLSTCNT(4) ERR_REPLFTOKSEM, 2, full_len, instname);
		udi = FILE_INFO(replreg);
		rts_error(VARLSTCNT(6) ERR_REPLACCSEM, 3, udi->semid, full_len, instname, save_errno);
	}
	repl_inst_jnlpool_reset();
	/*
	 * RECV POOL SEMAPHORES
	 */
	replreg = recvpool.recvpool_dummy_reg;
	assert(replreg);
	if (0 != remove_sem_set(RECV))
	{
		save_errno = REPL_SEM_ERRNO;
		if (!ftok_sem_release(replreg, TRUE, TRUE))
			gtm_putmsg(VARLSTCNT(4) ERR_REPLFTOKSEM, 2, full_len, instname);
		udi = FILE_INFO(replreg);
		rts_error(VARLSTCNT(6) ERR_REPLACCSEM, 3, udi->semid, full_len, instname, save_errno);
	}
	repl_inst_recvpool_reset();
	if (!ftok_sem_release(replreg, TRUE, immediate))
		rts_error(VARLSTCNT(4) ERR_REPLFTOKSEM, 2, full_len, instname);
	return TRUE;
}
