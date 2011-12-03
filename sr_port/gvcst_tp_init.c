/****************************************************************
 *								*
 *	Copyright 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gdscc.h"
#include "gdskill.h"
#include "jnl.h"
#include "hashtab.h"		/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "tp_timeout.h"
#include "gvcst_tp_init.h"

/* Initialize the TP structures we will be using for the successive TP operations */
void gvcst_tp_init(gd_region *greg)
{
	sgm_info		*si;
	sgmnt_addrs		*csa;

	csa = (sgmnt_addrs *)&FILE_INFO(greg)->s_addrs;
	if (NULL == csa->sgm_info_ptr)
	{
		si = csa->sgm_info_ptr = (sgm_info *)malloc(sizeof(sgm_info));
		assert(32768 > sizeof(sgm_info));
		memset(si, 0, sizeof(sgm_info));
		si->tp_hist_size = TP_MAX_MM_TRANSIZE;
		si->cur_tp_hist_size = INIT_CUR_TP_HIST_SIZE;	/* should be very much less than si->tp_hist_size */
		assert(si->cur_tp_hist_size <= si->tp_hist_size);
		init_hashtab(&si->blks_in_use, BLKS_IN_USE_INIT_ELEMS);
		/* See comment in tp.h about cur_tp_hist_size for details */
		si->first_tp_hist = si->last_tp_hist =
			(srch_blk_status *)malloc(sizeof(srch_blk_status) * si->cur_tp_hist_size);
		si->cw_set_list = (buddy_list *)malloc(sizeof(buddy_list));
		initialize_list(si->cw_set_list, sizeof(cw_set_element), CW_SET_LIST_INIT_ALLOC);
		si->tlvl_cw_set_list = (buddy_list *)malloc(sizeof(buddy_list));
		initialize_list(si->tlvl_cw_set_list, sizeof(cw_set_element), TLVL_CW_SET_LIST_INIT_ALLOC);
		si->tlvl_info_list = (buddy_list *)malloc(sizeof(buddy_list));
		initialize_list(si->tlvl_info_list, sizeof(tlevel_info), TLVL_INFO_LIST_INIT_ALLOC);
		si->new_buff_list = (buddy_list *)malloc(sizeof(buddy_list));
		initialize_list(si->new_buff_list, sizeof(que_ent) + csa->hdr->blk_size, NEW_BUFF_LIST_INIT_ALLOC);
		si->recompute_list = (buddy_list *)malloc(sizeof(buddy_list));
		initialize_list(si->recompute_list, sizeof(key_cum_value), RECOMPUTE_LIST_INIT_ALLOC);
		/* The size of the si->cr_array can go up to TP_MAX_MM_TRANSIZE, but usually is quite less.
		 * Therefore, initially allocate a small array and expand as needed later.
		 */
		if (dba_bg == greg->dyn.addr->acc_meth)
		{
			si->cr_array_size = si->cur_tp_hist_size;
			si->cr_array = (cache_rec_ptr_ptr_t)malloc(sizeof(cache_rec_ptr_t) * si->cr_array_size);
		} else
		{
			si->cr_array_size = 0;
			si->cr_array = NULL;
		}
		si->fresh_start = TRUE;
	} else
		si = csa->sgm_info_ptr;
	si->gv_cur_region = greg;
	si->start_tn = csa->ti->curr_tn;
	if (JNL_ALLOWED(csa))
	{
		si->total_jnl_rec_size = csa->min_total_tpjnl_rec_size;	/* Reinitialize total_jnl_rec_size */
		/* Since the following jnl-mallocs are independent of any dynamically-changeable parameter of the
		 * database, we can as well use the existing malloced jnl structures if at all they exist.
		 */
		if (NULL == si->jnl_tail)
		{
			si->jnl_tail = &si->jnl_head;
			si->jnl_list = (buddy_list *)malloc(sizeof(buddy_list));
			initialize_list(si->jnl_list, sizeof(jnl_format_buffer), JNL_LIST_INIT_ALLOC);
			si->format_buff_list = (buddy_list *)malloc(sizeof(buddy_list));
			/* Minimum value of elemSize is 8 due to alignment requirements of the returned memory location.
			 * Therefore, we request an elemSize of 8 bytes for the format-buffer and will convert as much
			 * bytes as we need into as many 8-byte multiple segments (see code in jnl_format).
			 */
			initialize_list(si->format_buff_list, JFB_ELE_SIZE,
					DIVIDE_ROUND_UP(JNL_FORMAT_BUFF_INIT_ALLOC, JFB_ELE_SIZE));
		}
	} else if (NULL != si->jnl_tail)
	{	/* journaling is currently disallowed although it was allowed (non-zero si->jnl_tail)
		 * during the prior use of this region. Free up unnecessary region-specific structures now.
		 */
		FREEUP_BUDDY_LIST(si->jnl_list);
		FREEUP_BUDDY_LIST(si->format_buff_list);
		si->jnl_tail = NULL;
	}
}
