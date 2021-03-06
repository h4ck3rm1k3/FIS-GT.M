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

#include "gtm_string.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdsblk.h"
#include "copy.h"
#include "util.h"
#include "cli.h"
#include "skan_offset.h"

#define MAX_UTIL_LEN	80

GBLREF block_id		patch_curr_blk;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF char		patch_comp_key[256];
GBLREF unsigned char	patch_comp_count;
GBLREF int		patch_rec_counter;
sm_uc_ptr_t skan_offset (sm_uc_ptr_t bp, bool over_run)
{
	sm_uc_ptr_t b_top, rp, r_top, rp_targ, key_top;
	char util_buff[MAX_UTIL_LEN];
	char cc;
	short int size, rec_size;
	int4 offset;

	if (((blk_hdr_ptr_t) bp)->bsiz > cs_addrs->hdr->blk_size)
		b_top = bp + cs_addrs->hdr->blk_size;
	else if (((blk_hdr_ptr_t) bp)->bsiz < sizeof(blk_hdr))
		b_top = bp + sizeof(blk_hdr);
	else
		b_top = bp + ((blk_hdr_ptr_t) bp)->bsiz;

	if (!cli_get_hex("OFFSET",&offset))
		return 0;
	if (offset < sizeof(blk_hdr))
	{	util_out_print("Error: offset less than blk header",TRUE);
		return 0;
	}
	rp_targ = bp + offset;
	if (rp_targ > b_top)
	{
		memcpy(util_buff, "Error: offset greater than blk size of ", 39);
		util_buff[ i2hex_nofill(b_top - bp, (uchar_ptr_t)&util_buff[39], 8) + 39 ] = 0;
		util_out_print(&util_buff[0],TRUE);
		return 0;
	}

	patch_rec_counter = 1;
	patch_comp_key[0] = patch_comp_key[1] = 0;
	for (rp = bp + sizeof(blk_hdr); rp < rp_targ ; )
	{	GET_SHORT(rec_size, &((rec_hdr_ptr_t)rp)->rsiz);
		if (rec_size < sizeof(rec_hdr))
			r_top = rp + sizeof(rec_hdr);
		else
			r_top = rp + rec_size;
		if (r_top >= b_top)
		{	if (!over_run)
				return rp;
			r_top = b_top;
		}
		if (r_top > rp_targ)
			return rp;

		patch_rec_counter++;
		if (((blk_hdr_ptr_t) bp)->levl)
			key_top = r_top - sizeof(block_id);
		else
		{	for (key_top = rp + sizeof(rec_hdr); key_top < r_top ; )
				if (!*key_top++ && !*key_top++)
					break;
		}
		if (((rec_hdr_ptr_t) rp)->cmpc > patch_comp_count)
			cc = patch_comp_count;
		else
			cc = ((rec_hdr_ptr_t) rp)->cmpc;
		size = key_top - rp - sizeof(rec_hdr);
		if (size < 0)
			size = 0;
		else if (size > sizeof(patch_comp_key) - 2)
			size = sizeof(patch_comp_key) - 2;
		memcpy(&patch_comp_key[cc],rp + sizeof(rec_hdr),size);
		patch_comp_count = cc + size;
		rp = r_top;
	}
	return rp;
}
