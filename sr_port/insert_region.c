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

/* 	insert_region.c
 *
 *	requirement
 *		reg		initialized (gvcst_init)
 *	parameters
 *		reg		the region to be inserted
 *		reg_list	pointer to the pointer to the list head
 *		reg_free_list	pointer to the pointer to the free list
 *		size		size of the structure of each item in the list
 *	return
 *		pointer to the item in the list that is corresponding to the region.
 *		*reg_list and *reg_free_list are also updated if needed.
 *
 *	fid_index field in csa and tp_reg_list is maintained by gvcst_init. Maintaining tp_reg_list is
 *	important, since the regions might be re-sorted in between insert_region() calls (i.e. new
 *	regions opening). All callers of insert_region except for dse_all() either use tp_reg_list or do not
 *	have the regions open.  dse_all() opens the regions before it calls insert_region(), so maintaining
 *	fid_index in tp_reg_list is sufficient.
 *
 */

#include "mdef.h"

#ifdef	VMS
#include <rms>
#endif

#ifdef UNIX
#include "gtm_ipc.h"		/* needed for FTOK */
#endif

#include "gtm_string.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdscc.h"
#include "gdskill.h"
#include "filestruct.h"
#include "jnl.h"
#include "hashtab.h"		/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "iosp.h"
#include "dbfilop.h"
#include "gtmmsg.h"
#include "is_file_identical.h"
#include "tp_grab_crit.h"
#include "t_retry.h"

GBLREF	gd_region	*gv_cur_region;
GBLREF	bool		run_time;
GBLREF	short		dollar_tlevel;
GBLREF	unsigned int	t_tries;

tp_region	*insert_region(	gd_region	*reg,
		   		tp_region	**reg_list,
		   		tp_region	**reg_free_list,
		   		int4		size)
{
	tp_region	*tr, *tr_last, *tr_new;
	unique_file_id	local_id;
#ifdef 	VMS
	char		*local_id_fiptr;
	file_control	*fc;
	uint4		status;
	gd_region	*temp_reg;
#endif
	int4		local_fid_index;
	sgmnt_addrs	*csa;
	int4		prev_index;

	assert(size >= sizeof(tp_region));
	assert(!run_time || dollar_tlevel);
	if (reg->open)
		csa = (sgmnt_addrs *)&FILE_INFO(reg)->s_addrs;
#if defined(VMS)
	if (!reg->open)
	{
		temp_reg = gv_cur_region;
		gv_cur_region = reg;
		local_id_fiptr = &local_id.file_id[0];
		if (!mupfndfil(reg, NULL))
		{
			gv_cur_region = temp_reg;
			return NULL;
		}
	  	if (NULL == reg->dyn.addr->file_cntl)
	    	{
	      		reg->dyn.addr->file_cntl = malloc(sizeof(file_control));
              		memset(reg->dyn.addr->file_cntl, 0, sizeof(file_control));
	    	}
	  	if (NULL == reg->dyn.addr->file_cntl->file_info)
	    	{
	      		reg->dyn.addr->file_cntl->file_info = malloc(sizeof(vms_gds_info));
	      		memset(reg->dyn.addr->file_cntl->file_info, 0, sizeof(vms_gds_info));
	    	}
	  	fc = reg->dyn.addr->file_cntl;
		fc->file_type = reg->dyn.addr->acc_meth;
	  	fc->op = FC_OPEN;
	  	status = dbfilop(fc);
	  	if (status & 1)
	    	{
			local_id_fiptr = &(FILE_INFO(reg)->file_id);
	      		sys$dassgn(FILE_INFO(reg)->fab->fab$l_stv);
	    	} else
	    	{
	      		gtm_putmsg(VARLSTCNT(1) status);
			gv_cur_region = temp_reg;
	      		return NULL;
	    	}
		gv_cur_region = temp_reg;
	} else
		local_fid_index = csa->fid_index;
#elif defined(UNIX)
	if (!reg->open)
	{
		if (!mupfndfil(reg, NULL))
			return NULL;
		if (!filename_to_id(&local_id.uid, (char *)reg->dyn.addr->fname))
			return NULL;
	} else
		local_fid_index = csa->fid_index;
#endif
	/* See if the region is already on the list or if we have to add it */
	for (tr = *reg_list, tr_last = NULL; NULL != tr; tr = tr->fPtr)
	{
		if (reg == tr->reg)			/* Region is found */
		{	/* assert we are not in final retry or we are in TP and have crit on the region already */
			assert((CDB_STAGNATE > t_tries)
				|| ((0 < dollar_tlevel) && reg->open && csa->now_crit));
			return tr;
		}
		if (reg->open)
		{	/* gvcst_init must have sorted them and filled in the fid_index field of node_local */
			assert(tr->reg->open);
			if ((tr->file.fid_index > local_fid_index))
				break;				/* .. we have found our insertion point */
		} else
		{	/* let's sort here */
			if (!tr->reg->open)
			{	/* all regions closed */
				VMS_ONLY(if (0 < memcmp(&(tr->file.file_id), local_id_fiptr, sizeof(gd_id))))
				UNIX_ONLY(if (0 < gdid_cmp(&(tr->file.file_id), &(local_id.uid))))
					break;				/* .. we have found our insertion point */
			} else
			{	/* the other regions are open, i.e. file is pointing to fid_index, use file_id
				 * from node_local */
				VMS_ONLY(if (0 < memcmp(
					&(((sgmnt_addrs *)&FILE_INFO(tr->reg)->s_addrs)->nl->unique_id.file_id),
					local_id_fiptr, sizeof(gd_id))))
				UNIX_ONLY(if (0 < gdid_cmp(
					&(((sgmnt_addrs *)&FILE_INFO(tr->reg)->s_addrs)->nl->unique_id.uid), &(local_id.uid))))
					break;				/* .. we have found our insertion point */
			}
		}
		tr_last = tr;
	}
	if ((NULL != reg_free_list) && (NULL != *reg_free_list))	/* Get a used block off our unused queue */
	{
		tr_new = *reg_free_list;		/* Get element */
		*reg_free_list = tr_new->fPtr;		/* Remove from queue */
	} else						/* get a new one */
	{
		tr_new = (tp_region *)malloc(size);
		if (size > sizeof(tp_region))
			memset(tr_new, 0, size);
	}
	tr_new->reg = reg;				/* Add this region to end of list */
	if (!reg->open)
	{
		VMS_ONLY(memcpy(&(tr_new->file.file_id), local_id_fiptr, sizeof(gd_id));)
		UNIX_ONLY(tr_new->file.file_id = local_id.uid;)
	} else
		tr_new->file.fid_index = local_fid_index;
	if (NULL == tr_last)
	{	/* First element on the list */
		tr_new->fPtr = *reg_list;
		*reg_list = tr_new;
	} else
	{	/* Insert into list */
		tr_new->fPtr = tr_last->fPtr;
		tr_last->fPtr = tr_new;
	}
	if ((CDB_STAGNATE <= t_tries) && (0 < dollar_tlevel) && reg->open && !csa->now_crit)
	{	/* Final retry in TP and this region not locked down. Get crit on it if it is open.
		 * reg->open needs to be checked above to take care of the case where we do an insert_region() from gvcst_init()
		 * 	in the 3rd retry in TP when we have not yet opened the region. In case region is not open,
		 * 	tp_restart() (invoked through t_retry from gvcst_init) will open "reg" as well as get crit on it for us.
		 */
		if (FALSE == tp_grab_crit(reg))		/* Attempt lockdown now */
		{
			t_retry(cdb_sc_needcrit);	/* avoid deadlock -- restart transaction */
			assert(FALSE);			/* should not come here as t_retry() does not return */
		}
		assert(csa->now_crit);	/* ensure we have crit now */
	}
	DEBUG_ONLY(
		prev_index = 0;
		for (tr = *reg_list; NULL != tr; tr = tr->fPtr)
		{
			if (tr->reg->open)
			{
				assert(prev_index < tr->file.fid_index);
				DEBUG_ONLY(prev_index = tr->file.fid_index);
			}
		}
	)
	return tr_new;
}
