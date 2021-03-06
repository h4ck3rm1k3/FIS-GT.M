/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include <signal.h>

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdsblk.h"
#include "cli.h"
#include "dse.h"
#include "util.h"
#include "skan_offset.h"
#include "skan_rnum.h"

/* Include prototypes */
#include "t_qread.h"

GBLREF block_id		patch_curr_blk;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF gd_region	*gv_cur_region;
GBLREF int		patch_is_fdmp;
GBLREF int		patch_fdmp_recs;
GBLREF int		patch_rec_counter;
GBLREF VSIG_ATOMIC_T	util_interrupt;

boolean_t dse_r_dmp(void)
{
	block_id	blk;
	sm_uc_ptr_t	bp, b_top, rp;
	int4		count;
	int4		dummy_int;
	cache_rec_ptr_t	dummy_cr;
	short 		record,size;
	bool		was_crit;
	int4		nocrit_present;
	error_def(ERR_DSEBLKRDFAIL);
	error_def(ERR_CTRLC);

	if (cli_present("BLOCK") == CLI_PRESENT)
	{
		int4 tmp_blk;

		if(!cli_get_hex("BLOCK", &tmp_blk))
			return FALSE;
		blk = (block_id)tmp_blk;
		if (blk < 0 || blk >= cs_addrs->ti->total_blks || !(blk % cs_addrs->hdr->bplmap))
		{
			util_out_print("Error: invalid block number.", TRUE);
			return FALSE;
		}
		patch_curr_blk = blk;
	}
	if (cli_present("COUNT") == CLI_PRESENT)
	{
		if (!cli_get_hex("COUNT", &count))
			return FALSE;
	} else
		count = 1;
	was_crit = cs_addrs->now_crit;
	nocrit_present = (CLI_NEGATED == cli_present("CRIT"));

	if (!was_crit)
	{
		if (nocrit_present)
			cs_addrs->now_crit = TRUE;
		else
			grab_crit(gv_cur_region);
	}

	if(!(bp = t_qread(patch_curr_blk, &dummy_int, &dummy_cr)))
		rts_error(VARLSTCNT(1) ERR_DSEBLKRDFAIL);

	if (((blk_hdr_ptr_t) bp)->bsiz > cs_addrs->hdr->blk_size)
		b_top = bp + cs_addrs->hdr->blk_size;
	else if (((blk_hdr_ptr_t) bp)->bsiz < sizeof(blk_hdr))
		b_top = bp + sizeof(blk_hdr);
	else
		b_top = bp + ((blk_hdr_ptr_t) bp)->bsiz;

	if (((blk_hdr_ptr_t) bp)->levl && patch_is_fdmp)
	{
		util_out_print("Error:  cannot perform GLO/ZWR dump on index block.", TRUE);
		if (!was_crit)
		{
			if (nocrit_present)
				cs_addrs->now_crit = FALSE;
			else
				rel_crit(gv_cur_region);
	    	}
		return FALSE;
	}
	if (cli_present("RECORD") == CLI_PRESENT)
	{
		if (!(rp = skan_rnum (bp, FALSE)))
		{
			if (!was_crit)
			{
				if (nocrit_present)
					cs_addrs->now_crit = FALSE;
				else
					rel_crit(gv_cur_region);
			}
			return FALSE;
		}
	}
	else if (!(rp = skan_offset (bp, FALSE)))
	{
		if (!was_crit)
		{
			if (nocrit_present)
				cs_addrs->now_crit = FALSE;
			else
				rel_crit(gv_cur_region);
		}
		return FALSE;
	}
	util_out_print(0, TRUE);
	for ( ; count > 0 ;count--)
	{
		if (util_interrupt || !(rp = dump_record(rp, patch_curr_blk, bp, b_top)))
			break;
		patch_rec_counter += 1;
	}
	if (!was_crit)
	{
		if (nocrit_present)
			cs_addrs->now_crit = FALSE;
		else
			rel_crit(gv_cur_region);
	}
	if (util_interrupt)
	    rts_error(VARLSTCNT(1) ERR_CTRLC);
	else if (cli_present("HEADER") == CLI_NEGATED)
		util_out_print(0, TRUE);
	return TRUE;
}
