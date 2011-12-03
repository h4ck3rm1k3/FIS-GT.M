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

#include "gtm_string.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "gdsblk.h"
#include "gdsbml.h"
#include "longset.h"
#include "mupint.h"
#include "mu_file_size.h"
#include "gtmmsg.h"

GBLDEF unsigned char		*mu_int_locals;
GBLDEF unsigned char		*mu_int_master;
GBLDEF int4			mu_int_ovrhd;

GBLREF gd_region		*gv_cur_region;
GBLREF sgmnt_data		mu_int_data;
GBLREF uint4			mu_int_errknt;
GBLREF boolean_t		tn_reset_specified;

boolean_t mu_int_fhead(void)
{
	unsigned char	*p1;
	unsigned int	maps, native_size, size, block_factor;
	sgmnt_data_ptr_t mu_data;

	error_def(ERR_MUKILLIP);
	error_def(ERR_MUTNWARN);
	error_def(ERR_DBNOTDB);
	error_def(ERR_DBINCRVER);
	error_def(ERR_DBSVBNMIN);
	error_def(ERR_DBFLCORRP);
	error_def(ERR_DBCREINCOMP);
	error_def(ERR_DBBSIZZRO);
	error_def(ERR_DBSZGT64K);
	error_def(ERR_DBNOTMLTP);
	error_def(ERR_DBBPLMLT512);
	error_def(ERR_DBBPLMGT2K);
	error_def(ERR_DBBPLNOT512);
	error_def(ERR_DBTTLBLK0);
	error_def(ERR_DBTNNEQ);
	error_def(ERR_DBMAXKEYEXC);
	error_def(ERR_DBMXRSEXCMIN);
	error_def(ERR_DBMAXRSEXBL);
	error_def(ERR_DBUNDACCMT);
	error_def(ERR_DBHEADINV);
	error_def(ERR_DBFGTBC);
	error_def(ERR_DBFSTBC);
	error_def(ERR_DBTOTBLK);


	mu_data = &mu_int_data;
	if (MEMCMP_LIT(mu_data->label, GDS_LABEL))
	{
		if (memcmp(mu_data->label, GDS_LABEL, sizeof(GDS_LABEL) - 2))
			mu_int_err(ERR_DBNOTDB, 0, 0, 0, 0, 0, 0, 0);
		else
			mu_int_err(ERR_DBINCRVER, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	if (mu_data->start_vbn < DIVIDE_ROUND_UP(sizeof(sgmnt_data), DISK_BLOCK_SIZE))
	{
		mu_int_err(ERR_DBSVBNMIN, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	if (mu_data->file_corrupt)
		mu_int_err(ERR_DBFLCORRP, 0, 0, 0, 0, 0, 0, 0);
	if (mu_data->createinprogress)
	{
		mu_int_err(ERR_DBCREINCOMP, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	/* CHECK: 0 < blk_size <= 64K; blk_size is a multiple of DISK_BLOCK_SIZE */
	if (0 == mu_data->blk_size)
	{
		mu_int_err(ERR_DBBSIZZRO, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	if (mu_data->blk_size > 1 << 16)
	{
		mu_int_err(ERR_DBSZGT64K, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	if (mu_data->blk_size % DISK_BLOCK_SIZE)
	{	/* these messages should use rts_error and parameters */
		assert(512 == DISK_BLOCK_SIZE);		/* but in lieu of that, check that message is accurate */
		mu_int_err(ERR_DBNOTMLTP, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	/* CHECK: BLKS_PER_LMAP <= bplmap <= 2K; bplmap is a multiple of BLKS_PER_LMAP */
	if (mu_data->bplmap < BLKS_PER_LMAP)
	{
		mu_int_err(ERR_DBBPLMLT512, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	if (mu_data->bplmap > 1 << 11)
	{
		mu_int_err(ERR_DBBPLMGT2K, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	if (BLKS_PER_LMAP != mu_data->bplmap)
	{
		mu_int_err(ERR_DBBPLNOT512, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	/* CHECK: total_blks <> 0 */
	if (0 == mu_data->trans_hist.total_blks)
	{
		if (0 == mu_data->trans_hist.free_blocks)	/* if 0 because old version, both should be 0 */
		{
			mu_data->trans_hist.total_blks = mu_data->total_blks_filler;
			mu_data->trans_hist.free_blocks = mu_data->free_blocks_filler;
		}
		if (0 == mu_data->trans_hist.total_blks)
		{
			mu_int_err(ERR_DBTTLBLK0, 0, 0, 0, 0, 0, 0, 0);
			return FALSE;
		}
	}
	if (mu_data->trans_hist.curr_tn != mu_data->trans_hist.early_tn)
		mu_int_err(ERR_DBTNNEQ, 0, 0, 0, 0, 0, 0, 0);
        if (0 != mu_data->kill_in_prog)
        {
                gtm_putmsg(VARLSTCNT(4) ERR_MUKILLIP, 2, DB_LEN_STR(gv_cur_region));
                mu_int_errknt++;
        }
	if (MAX_KEY_SZ < mu_data->max_key_size)
		mu_int_err(ERR_DBMAXKEYEXC, 0, 0, 0, 0, 0, 0, 0);
	if (sizeof(rec_hdr) + sizeof(block_id) >= mu_data->max_rec_size)
		mu_int_err(ERR_DBMXRSEXCMIN, 0, 0, 0, 0, 0, 0, 0);
	if (mu_data->blk_size - sizeof(blk_hdr) < mu_data->max_rec_size)
		mu_int_err(ERR_DBMAXRSEXBL, 0, 0, 0, 0, 0, 0, 0);
	/* !tn_reset_this_reg should ideally be used here instead of (!tn_reset_specified || gv_cur_region->read_only).
	 * But at this point, tn_reset_this_reg has not yet been set for this region and to avoid taking a risk in
	 *   changing the code flow, we redo the computation ot tn_reset_this_reg here. This is not as much a performance concern.
	 */
	if ((!tn_reset_specified || gv_cur_region->read_only) && (mu_data->trans_hist.curr_tn > WARNING_TN))
	{
		gtm_putmsg(VARLSTCNT(4) ERR_MUTNWARN, 2, DB_LEN_STR(gv_cur_region));
		mu_int_errknt++;
	}
	/* Note - ovrhd is incremented once in order to achieve a zero-based
	 * index of the GDS 'data' blocks (those other than the file header
	 * and the block table). */
	switch (mu_data->acc_meth)
	{
		default:
			mu_int_err(ERR_DBUNDACCMT, 0, 0, 0, 0, 0, 0, 0);
		/*** WARNING: Drop thru ***/
#ifdef VMS
#ifdef GT_CX_DEF
		case dba_bg:	/* necessary to do calculation in this manner to prevent double rounding causing an error */
			if (mu_data->unbacked_cache)
				mu_int_ovrhd = DIVIDE_ROUND_UP(sizeof(sgmnt_data) + mu_data->free_space +
					mu_data->lock_space_size, DISK_BLOCK_SIZE);
			else
				mu_int_ovrhd = DIVIDE_ROUND_UP(sizeof(sgmnt_data) + BT_SIZE(mu_data)
					+ mu_data->free_space + mu_data->lock_space_size, DISK_BLOCK_SIZE);
			break;
		case dba_mm:
			mu_int_ovrhd = DIVIDE_ROUND_UP(sizeof(sgmnt_data) + mu_data->free_space, DISK_BLOCK_SIZE);
			break;
#else
		case dba_bg:
		/*** WARNING: Drop thru ***/
		case dba_mm:
			mu_int_ovrhd = DIVIDE_ROUND_UP(sizeof(sgmnt_data) + mu_data->free_space, DISK_BLOCK_SIZE);
		break;
#endif

#elif defined(UNIX)
		case dba_bg:
		/*** WARNING: Drop thru ***/
		case dba_mm:
			mu_int_ovrhd = DIVIDE_ROUND_UP(sizeof(sgmnt_data) + mu_data->free_space, DISK_BLOCK_SIZE);
#else
#error unsupported platform
#endif
	}
	assert(mu_data->blk_size == ROUND_UP(mu_data->blk_size, DISK_BLOCK_SIZE));
 	block_factor =  mu_data->blk_size / DISK_BLOCK_SIZE;
	mu_int_ovrhd += 1;
	if (mu_int_ovrhd != mu_data->start_vbn)
	{
		mu_int_err(ERR_DBHEADINV, 0, 0, 0, 0, 0, 0, 0);
		return FALSE;
	}
	size = mu_int_ovrhd + block_factor * mu_data->trans_hist.total_blks;
	native_size = mu_file_size(gv_cur_region->dyn.addr->file_cntl);
	/* In the following tests, the EOF block should always be 1 greater
	 * than the actual size of the file.  This is due to the GDS being
	 * allocated in even DISK_BLOCK_SIZE-byte blocks. */
	if (native_size && (DIVIDE_ROUND_DOWN(size - mu_int_ovrhd, block_factor) !=
		DIVIDE_ROUND_DOWN(native_size - mu_int_ovrhd, block_factor)))
	{
		if (size < native_size)
			mu_int_err(ERR_DBFGTBC, 0, 0, 0, 0, 0, 0, 0);
		else
			mu_int_err(ERR_DBFSTBC, 0, 0, 0, 0, 0, 0, 0);
		gtm_putmsg(VARLSTCNT(4) ERR_DBTOTBLK, 2, (native_size - mu_data->start_vbn)
			/ (block_factor), (mu_data->trans_hist.total_blks));
	}
	/* make copy of master bitmap in mu_int_master */
	mu_int_master = (unsigned char *)malloc(MASTER_MAP_SIZE);
	memcpy(mu_int_master, mu_data->master_map, MASTER_MAP_SIZE);
	/* make working space for all local bitmaps */
	maps = (mu_data->trans_hist.total_blks + mu_data->bplmap - 1) / mu_data->bplmap;
	size = BM_SIZE(mu_data->bplmap) - sizeof(blk_hdr);
	size *= maps;
	mu_int_locals = (unsigned char *)malloc(size);
	longset(mu_int_locals, size, FOUR_BLKS_FREE);
	return TRUE;
}
