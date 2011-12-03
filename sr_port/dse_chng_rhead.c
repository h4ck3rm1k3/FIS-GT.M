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

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdsblk.h"
#include "min_max.h"		/* needed for gdsblkops.h */
#include "gdsblkops.h"
#include "gdscc.h"
#include "cli.h"
#include "copy.h"
#include "filestruct.h"
#include "jnl.h"
#include "skan_offset.h"
#include "skan_rnum.h"
#include "dse.h"

/* Include prototypes */
#include "t_qread.h"
#include "t_write.h"
#include "t_end.h"
#include "t_begin_crit.h"
#include "gvcst_blk_build.h"
#include "util.h"
#include "t_abort.h"

GBLREF char		*update_array, *update_array_ptr;
GBLREF gd_region        *gv_cur_region;
GBLREF uint4		update_array_size;
GBLREF srch_hist	dummy_hist;
GBLREF block_id		patch_curr_blk;
GBLREF unsigned char	patch_comp_count;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF sgmnt_data_ptr_t cs_data;
GBLREF gd_addr		*gd_header;
GBLREF cw_set_element   cw_set[];
GBLREF unsigned char    *non_tp_jfb_buff_ptr;

void dse_chng_rhead(void)
{
	block_id	blk;
	sm_uc_ptr_t	bp, b_top, cp, rp;
	bool		chng_rec;
	rec_hdr		new_rec;
	int4		x;
	blk_segment	*bs1, *bs_ptr;
	cw_set_element  *cse;
	int4		blk_seg_cnt, blk_size;
	error_def(ERR_DSEBLKRDFAIL);
	error_def(ERR_DSEFAIL);
	error_def(ERR_DBRDONLY);

        if (gv_cur_region->read_only)
                rts_error(VARLSTCNT(4) ERR_DBRDONLY, 2, DB_LEN_STR(gv_cur_region));
	assert(update_array);
	/* reset new block mechanism */
	update_array_ptr = update_array;

	if (cli_present("BLOCK") == CLI_PRESENT)
	{
		if(!cli_get_hex("BLOCK", &blk))
			return;
		patch_curr_blk = blk;
	}
	if (patch_curr_blk < 0 || patch_curr_blk >= cs_addrs->ti->total_blks || !(patch_curr_blk % cs_addrs->hdr->bplmap))
	{
		util_out_print("Error: invalid block number.", TRUE);
		return;
	}

	t_begin_crit(ERR_DSEFAIL);
	if(!(bp = t_qread(patch_curr_blk, &dummy_hist.h[0].cycle, &dummy_hist.h[0].cr)))
		rts_error(VARLSTCNT(1) ERR_DSEBLKRDFAIL);

	blk_size = cs_addrs->hdr->blk_size;
	chng_rec = FALSE;
	b_top = bp + ((blk_hdr_ptr_t)bp)->bsiz;
	if (((blk_hdr_ptr_t)bp)->bsiz > blk_size || ((blk_hdr_ptr_t)bp)->bsiz < sizeof(blk_hdr))
		chng_rec = TRUE;	/* force rewrite to correct size */

	if (cli_present("RECORD") == CLI_PRESENT)
	{
		if (!(rp = skan_rnum(bp, FALSE)))
		{
			t_abort(gv_cur_region, cs_addrs);
			return;
		}
	} else if (!(rp = skan_offset (bp, FALSE)))
	{
		t_abort(gv_cur_region, cs_addrs);
		return;
	}
	GET_SHORT(new_rec.rsiz, &((rec_hdr_ptr_t)rp)->rsiz);
	new_rec.cmpc = ((rec_hdr_ptr_t)rp)->cmpc;
	if (cli_present("CMPC") == CLI_PRESENT)
	{
		if (!cli_get_hex("CMPC", &x))
		{
			t_abort(gv_cur_region, cs_addrs);
			return;
		}
		if (x < 0 || x > 0x7f)
		{
			util_out_print("Error: invalid cmpc.",TRUE);
			t_abort(gv_cur_region, cs_addrs);
			return;
		}
		if (x > patch_comp_count)
			util_out_print("Warning:  specified compression count is larger than the current expanded key size.", TRUE);
		new_rec.cmpc = x;
		chng_rec = TRUE;
	}
	if (cli_present("RSIZ") == CLI_PRESENT)
	{
		if (!cli_get_hex("RSIZ", &x))
		{
			t_abort(gv_cur_region, cs_addrs);
			return;
		}
		if (x < sizeof(rec_hdr) || x > blk_size)
		{
			util_out_print("Error: invalid rsiz.", TRUE);
			t_abort(gv_cur_region, cs_addrs);
			return;
		}
		new_rec.rsiz = x;
		chng_rec = TRUE;
	}
	if (chng_rec)
	{
		BLK_INIT(bs_ptr, bs1);
		cp = bp;
		cp += sizeof(blk_hdr);
		if (chng_rec)
		{
			BLK_SEG(bs_ptr, cp, rp - cp);
			BLK_SEG(bs_ptr, (uchar_ptr_t)&new_rec, sizeof(rec_hdr));
			cp = rp + sizeof(rec_hdr);
		}
		if (b_top - cp)
			BLK_SEG(bs_ptr, cp, b_top - cp);
		if (!BLK_FINI(bs_ptr, bs1))
		{
			util_out_print("Error: bad blk build.", TRUE);
			t_abort(gv_cur_region, cs_addrs);
			return;
		}
		t_write(patch_curr_blk, (unsigned char *)bs1, 0, 0, bp, ((blk_hdr_ptr_t)bp)->levl, TRUE, FALSE);
		BUILD_AIMG_IF_JNL_ENABLED(cs_addrs, cs_data, non_tp_jfb_buff_ptr, cse);
		t_end(&dummy_hist, 0);
	}
	return;
}
