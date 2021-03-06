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
#include "cmidef.h"
#include "hashdef.h"
#include "cmmdef.h"
#include "gt_timer.h"
#include "gtcmd.h"
#include "mlkdef.h"
#include "gtcmlkdef.h"
#include "gtcml.h"
#include "gtcmtr_terminate.h"
#include "gtcmtr_terminate_free.h"

GBLREF connection_struct *curr_entry;
GBLREF int4		gtcm_users;
#ifdef VMS
GBLREF short	gtcm_ast_avail;
#endif
GBLREF struct CLB	*proc_to_clb[];

bool gtcmtr_terminate(bool cm_err)
{
#ifdef VMS
	unsigned char	*mbuffer;
#endif
	uint4		status;
	struct CLB	*clb;

	if (curr_entry)
	{
		cancel_timer((TID)curr_entry);
		gtcml_lkrundown();
		gtcmd_rundown(curr_entry, cm_err);
		if (curr_entry->pvec)
		{
			free(curr_entry->pvec);
			curr_entry->pvec = NULL;
		}
		curr_entry->region_root = NULL;	/* make sure you can't access any regions through this entry... just in case */
		curr_entry->maxregnum = 0;
		if (curr_entry->clb_ptr)
		{
			clb = curr_entry->clb_ptr;
			VMS_ONLY(mbuffer = clb->mbf;)
			curr_entry->clb_ptr = NULL;
			cmi_close(clb);
			/*
			 * The freeing of the buffer is automatically
			 * handled by cmi_free_clb on UNIX.
			 */
#ifdef VMS
			if (mbuffer)
				free(mbuffer);
#endif
                        proc_to_clb[curr_entry->procnum] = NULL;
			/*
			 * Unix cmi_close doesn't free the clb.
			 */
			UNIX_ONLY(cmi_free_clb(clb));
		}
		/*
		 * The connection struct is part of the memory allocated by
		 * cmi_init on UNIX.  The cmi_free_clb has already dealt with it.
		 */
		VMS_ONLY(free(curr_entry));
		curr_entry = NULL;
	}
	gtcm_users--;
	VMS_ONLY(gtcm_ast_avail++);
	return CM_NOOP;
}
