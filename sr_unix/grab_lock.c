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

#include "mdef.h"

#include <signal.h>	/* for VSIG_ATOMIC_T type */

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "mutex.h"
#include "deferred_signal_handler.h"
#include "have_crit.h"
#include "caller_id.h"

GBLREF	volatile int4		crit_count;
GBLREF	VSIG_ATOMIC_T		forced_exit;
GBLREF	uint4			process_id;
GBLREF	node_local_ptr_t	locknl;

/* Note about usage of this function : Create dummy gd_region, gd_segment, file_control,
 * unix_db_info, sgmnt_addrs, and allocate mutex_struct (and NUM_CRIT_ENTRY * mutex_que_entry),
 * mutex_spin_parms_struct, and node_local in shared memory. Initialize the fields as in
 * jnlpool_init(). Pass the address of the dummy region as argument to this function.
 */
void	grab_lock(gd_region *reg)
{
	unix_db_info 		*udi;
	sgmnt_addrs  		*csa;
	enum cdb_sc		status;
	mutex_spin_parms_ptr_t	mutex_spin_parms;

	error_def(ERR_DBCCERR);
	error_def(ERR_CRITRESET);

	udi = FILE_INFO(reg);
	csa = &udi->s_addrs;
	if (!csa->now_crit)
	{
		assert(0 == crit_count);
		crit_count++;	/* prevent interrupts */
		DEBUG_ONLY(locknl = csa->nl;)	/* for DEBUG_ONLY LOCK_HIST macro */
		mutex_spin_parms = (mutex_spin_parms_ptr_t)((sm_uc_ptr_t)csa->critical + CRIT_SPACE);
			/* This assumes that mutex_spin_parms_t is located immediately after the crit structures */
		/* As of 10/07/98, crashcnt field in mutex_struct is not changed by any function for the dummy  region */
		status = mutex_lockw(reg, mutex_spin_parms, 0);
		DEBUG_ONLY(locknl = NULL;)	/* restore "locknl" to default value */
		if (status != cdb_sc_normal)
		{
			crit_count = 0;
			switch(status)
			{
				case cdb_sc_critreset: /* As of 10/07/98, this return value is not possible */
					rts_error(VARLSTCNT(4) ERR_CRITRESET, 2, REG_LEN_STR(reg));
				case cdb_sc_dbccerr:
					rts_error(VARLSTCNT(4) ERR_DBCCERR, 2, REG_LEN_STR(reg));
				default:
					if (forced_exit && 0 == have_crit(CRIT_HAVE_ANY_REG))
						deferred_signal_handler();
					GTMASSERT;
			}
			return;
		}
		assert(csa->nl->in_crit == 0);
		csa->nl->in_crit = process_id;
		CRIT_TRACE(crit_ops_gw);		/* see gdsbt.h for comment on placement */
		crit_count = 0;
	}
	return;
}
