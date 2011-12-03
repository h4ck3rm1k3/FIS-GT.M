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

#if defined(VMS)
#include <climsgdef.h>
#include <fab.h>
#include <rms.h>
#include <errno.h>
#include <nam.h>
#include <psldef.h>
#include <rmsdef.h>
#include <descrip.h>
#endif
#include "gtm_unistd.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gdsblk.h"
#include "error.h"
#include "jnl.h"
#include "mupip_set.h"
#include "io.h"
#include "gtmsecshr.h"
#include "gds_rundown.h"
#if defined(UNIX)
#include "ftok_sems.h"
#endif
#include "tp_change_reg.h"
#include "dbfilop.h"

GBLREF	mu_set_rlist	*grlist;
GBLREF	gd_region	*gv_cur_region;

void mupip_set_jnl_cleanup(boolean_t clean_exit)
{
	mu_set_rlist		*rptr;
	file_control		*fc;
	VMS_ONLY(
		GDS_INFO	*gds_info;
		uint4		status;
	)

	for (rptr = grlist; NULL != rptr; rptr = rptr->fPtr)
	{
		if (ALLOCATED != rptr->state)
			continue;
		gv_cur_region = rptr->reg;
		if (rptr->exclusive)
		{
			assert(NULL != rptr->sd);
			if (NULL == rptr->sd)
				continue;
			free(rptr->sd);
			rptr->sd = NULL;
			fc = gv_cur_region->dyn.addr->file_cntl;
			fc->op = FC_CLOSE;
			dbfilop(fc);
			VMS_ONLY(
				gds_info = FILE_INFO(gv_cur_region);
				assert(gds_info->file_cntl_lsb.lockid);
				status = gtm_deq(gds_info->file_cntl_lsb.lockid, NULL, PSL$C_USER, 0);
				assert(SS$_NORMAL == status);
				gds_info->file_cntl_lsb.lockid = 0;
				sys$dassgn(gds_info->fab->fab$l_stv);
			)
			UNIX_ONLY(db_ipcs_reset(gv_cur_region, !clean_exit);)
		} else
		{
 			tp_change_reg();
			assert(NULL != gv_cur_region->dyn.addr->file_cntl && NULL != rptr->sd);
			if (NULL != gv_cur_region->dyn.addr->file_cntl && NULL != rptr->sd)
				gds_rundown();
			/* Note: We did not allocate, so we do not deallocate rptr->sd */
			rptr->sd = NULL;
		}
		rptr->state = DEALLOCATED;
	}
}
