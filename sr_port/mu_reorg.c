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

/***************************************************************************************************************
 mu_reorg.c:
	This program reorgs the database block structure of a particular global variable
	traversing the Global Variable Tree (GVT) in a pre-order manner.
	Globals are specified in SELECT option.  During the reorg it does not affect database
	block structure of globals mentioned in EXCLUDE option.
	Given fill_factor (data density % in a block) of a working block, reorg tries to acheive that fill_factor.
	Then it swaps the working block with another block.
	An off-line reorg will assign block-id sequentially following the pre-order traversal.
	An on-line reorg will assign block-id sequentially while traversing the GVT in an adaptive pre-order traversal.
	mu_reorg() calls mu_split(), if split is needed to achieve fill factor
	mu_reorg() calls mu_clsce(), if coalese is needed with right sibling to achieve fill factor
	mu_reorg() calls mu_swap(), to swap the working block which acheived the fill facotr with
 	some other block which will give better I/O performance.
	Note that split can result in increase of GVT height. Coalesce can help to reduce heigth.
	mu_reduce_level() is called to see if height can be reduced.
****************************************************************************************************************/

#include "mdef.h"

#include "gtm_string.h"

#include "cdb_sc.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "gdsblkops.h"
#include "gdskill.h"
#include "gdscc.h"
#include "copy.h"
#include "interlock.h"
#include "muextr.h"
#include "mu_reorg.h"

/* Include prototypes */
#include "t_write.h"
#include "t_end.h"
#include "t_retry.h"
#include "mupip_reorg.h"
#include "util.h"
#include "t_begin.h"
#include "op.h"
#include "gvcst_rtsib.h"
#include "gvcst_search.h"
#include "gvcst_bmp_mark_free.h"
#include "gvcst_kill_sort.h"
#include "gtmmsg.h"
#include "add_inter.h"
#include "t_abort.h"

GBLREF	bool		mu_ctrlc_occurred;
GBLREF	bool		mu_ctrly_occurred;
GBLREF	sgmnt_data_ptr_t	cs_data;
GBLREF	sgmnt_addrs	*cs_addrs;
GBLREF	gv_key		*gv_currkey_next_reorg;
GBLREF	gd_region	*gv_cur_region;
GBLREF	gv_key		*gv_currkey;
GBLREF	gv_namehead 	*gv_target;
GBLREF	gv_namehead 	*reorg_gv_target;
GBLREF	unsigned char	cw_map_depth;
GBLREF	unsigned char	cw_set_depth;
GBLREF	cw_set_element	cw_set[];
GBLREF	uint4		t_err;
GBLREF	unsigned int	t_tries;
GBLREF	unsigned char	rdfail_detail;
GBLREF	inctn_opcode_t	inctn_opcode;
GBLREF	kill_set	*kill_set_tail;
GBLREF	boolean_t 	kip_incremented;
GBLREF	boolean_t 	need_kip_incr;
GBLREF	int4		update_trans;

void log_detailed_log(char *X, srch_hist *Y, srch_hist *Z, int level, kill_set *kill_set_list, trans_num tn);
void reorg_finish(block_id dest_blk_id, int blks_processed, int blks_killed,
	int blks_reused, int file_extended, int lvls_reduced,
	int blks_coalesced, int blks_split, int blks_swapped);

void log_detailed_log(char *X, srch_hist *Y, srch_hist *Z, int level, kill_set *kill_set_list, trans_num tn)
{
	int 		i;
	block_id	bitmap = 1, temp_bitmap;	/* bitmap is initialized to 1, which is not a bitmap block id */

	assert(NULL != (char *)(Y));
	assert(0 < (Y)->depth);
	assert((NULL == (char *)(Z)) || (0 < (Z)->depth));
	util_out_print("!AD::!UL::", FALSE, LEN_AND_STR(X), tn);
	for (i = 0; i <= (Y)->depth; i++)
		util_out_print("!SL|", FALSE, (Y)->h[i].blk_num);
	if (NULL != (char *)(Z))
	{
		util_out_print("-", FALSE);
		for (i = 0; i <= (Z)->depth; i++)
			util_out_print("!SL|", FALSE, (Z)->h[i].blk_num);
	}
	if (cw_set_depth)
	{
		util_out_print("::", FALSE);
		for (i = 0; i < cw_set_depth; i++)
			util_out_print("!SL|", FALSE, cw_set[i].blk);
	}
	if ((0 == memcmp((X), "SPL", 3))
		|| (0 == memcmp((X), "CLS", 3))
		|| (0 == memcmp((X), "SWA", 3)))
	{
		if (NULL != (char *)(Z))
			util_out_print("::!SL|!SL", TRUE,
				(Y)->h[level].blk_num, (Z)->h[level].blk_num);
		else
			util_out_print("::!SL", TRUE, (Y)->h[level].blk_num);
	}
	else
	{
		if ((0 == memcmp((X), "KIL", 3)) && (NULL != kill_set_list))
		{
			util_out_print("::", FALSE);
			for (i = 0; i < kill_set_list->used; i++)
			{
				temp_bitmap = kill_set_list->blk[i].block & (~(BLKS_PER_LMAP - 1));
				if (bitmap != temp_bitmap)
				{
					if (1 != bitmap)
						util_out_print("]", FALSE);
					bitmap = temp_bitmap;
					util_out_print("[!SL:", FALSE, bitmap);
				}
				util_out_print("!SL,", FALSE, kill_set_list->blk[i].block);
			}
			util_out_print("]", TRUE);
		}
	}
}

/****************************************************************
Input Parameter:
	gn = Global name
	exclude_glist_ptr = list of globals in EXCLUDE option
	index_fill_factor = index blocks' fill factor
	data_fill_factor = data blocks' fill factor
Input/Output Parameter:
	resume = resume flag
	reorg_op = What operations to do (coalesce or, swap or, split) [Default is all]
			[Only for debugging]
 ****************************************************************/
boolean_t mu_reorg(mval *gn, glist *exclude_glist_ptr, boolean_t *resume, int index_fill_factor, int data_fill_factor, int reorg_op)
{
	boolean_t		end_of_tree = FALSE, complete_merge, detailed_log;
	int			rec_size;
	/*
	 *
	 * "level" is the level of the working block.
	 * "pre_order_successor_level" is pre_order successor level except in the case
	 * where we are in a left-most descent of the tree
	 * in which case pre_order_successor_level will be the maximum height of that subtree
	 * until we reach the leaf level block .
	 * In other words, pre_order_successor_level and level variable controls the iterative pre-order traversal.
	 * We start reorg from the (root_level - 1) to 0. That is, level = pre_order_successor_level:-1:0.
	 */
	int			pre_order_successor_level, level;
	static block_id		dest_blk_id = 0;
	int			tkeysize;
	int			blks_killed, blks_processed, blks_reused, blks_coalesced, blks_split, blks_swapped,
				count, file_extended, lvls_reduced;
	int			d_max_fill, i_max_fill, blk_size, cur_blk_size, max_fill, toler, d_toler, i_toler;
	int			cnt1, cnt2;
	kill_set		kill_set_list;
	sm_uc_ptr_t		rPtr1;
	enum cdb_sc		status;
	srch_hist		*rtsib_hist;
	jnl_buffer_ptr_t	jbp;
	trans_num		ret_tn;

	error_def(ERR_MUREORGFAIL);
	error_def(ERR_DBRDONLY);
	error_def(ERR_GBLNOEXIST);
	error_def(ERR_MAXBTLEVEL);

	t_err = ERR_MUREORGFAIL;
	kill_set_tail = &kill_set_list;
	/* Initialization for current global */
	op_gvname(VARLSTCNT(1) gn);
	/* Cannot proceed for read-only data files */
	if (gv_cur_region->read_only)
	{
		gtm_putmsg(VARLSTCNT(4) ERR_DBRDONLY, 2, DB_LEN_STR(gv_cur_region));
		return FALSE;
	}
	dest_blk_id = cs_addrs->reorg_last_dest;
	inctn_opcode = inctn_mu_reorg;

	/* If resume option is present, then reorg_restart_key should be not null.
	 * Skip all globals until we are in the region for that global.
	 * Get the reorg_restart_key and reorg_restart_block from database header and restart from there.
	 */
	if (*resume && 0 != cs_data->reorg_restart_key[0])
	{
		/* resume from last key reorged in GVT */
		GET_KEY_LEN(tkeysize, &cs_data->reorg_restart_key[0]);
		memcpy(gv_currkey->base, cs_data->reorg_restart_key, tkeysize);
		gv_currkey->end = tkeysize - 1;
		dest_blk_id = cs_data->reorg_restart_block;
 		if (0 == memcmp(cs_data->reorg_restart_key, gn->str.addr, gn->str.len))
			/* Going to resume from current global, so it resumed and make it false */
			*resume = FALSE;
	} else
	{
		/* start from the left most leaf */
		memcpy(&gv_currkey->base[0], gn->str.addr, gn->str.len);
		gv_currkey->base[gn->str.len] = gv_currkey->base[gn->str.len + 1] = 0;
		gv_currkey->end = gn->str.len + 1;
	}
	if (*resume)
	{
		util_out_print("REORG cannot be resumed from this point, Skipping this global...", FLUSH);
		memcpy(&gv_currkey->base[0], gn->str.addr, gn->str.len);
		gv_currkey->base[gn->str.len] = gv_currkey->base[gn->str.len + 1] = 0;
		gv_currkey->end = gn->str.len + 1;
		return TRUE;
	}
 	memcpy(&gv_currkey_next_reorg->base[0], &gv_currkey->base[0], gv_currkey->end + 1);
	gv_currkey_next_reorg->end =  gv_currkey->end;
	if (2 > dest_blk_id)
		dest_blk_id = 2; /* we know that first block is bitmap and next one is directory tree root */
	file_extended = cs_data->trans_hist.total_blks;
	blk_size = cs_data->blk_size;
	d_max_fill = (double)data_fill_factor * blk_size / 100.0 - cs_data->reserved_bytes;
	i_max_fill = (double)index_fill_factor * blk_size / 100.0 - cs_data->reserved_bytes;
	d_toler = (double) DATA_FILL_TOLERANCE * blk_size / 100.0;
	i_toler = (double) INDEX_FILL_TOLERANCE * blk_size / 100.0;
	blks_killed = blks_processed = blks_reused = lvls_reduced = blks_coalesced = blks_split = blks_swapped = 0;
	pre_order_successor_level = level = MAX_BT_DEPTH + 1; /* Just some high value to initialize */

	/* --- more detailed debugging information --- */
	if (detailed_log = reorg_op & DETAIL)
		util_out_print("STARTING to work on global ^!AD from region !AD", TRUE,
			gn->str.len, gn->str.addr, REG_LEN_STR(gv_cur_region));

	/* In each iteration of MAIN loop, a working block is processed for a GVT */
	for (; ;)	/* ================ START MAIN LOOP ================ */
	{
		/* If right sibling is completely merged with the working block, do not swap the working block
		 * with its final destination block. Continue trying next right sibling. Swap only at the end.
		 */
		complete_merge = TRUE;
		while(complete_merge)	/* === START WHILE COMPLETE_MERGE === */
		{
			if (mu_ctrlc_occurred || mu_ctrly_occurred)
			{
				cs_data->reorg_restart_block = dest_blk_id;
				memcpy(&cs_data->reorg_restart_key[0], &gv_currkey->base[0], gv_currkey->end + 1);
				return FALSE;
			}
			complete_merge = FALSE;
			blks_processed++;
			t_begin(ERR_MUREORGFAIL, TRUE);
			/* Folllowing for loop is to handle concurrency retry for split/coalesce */
			for (; ;)		/* === SPLIT-COALESCE LOOP STARTS === */
			{
				gv_target->clue.end = 0;
				/* search gv_currkey and get the result in gv_target */
				if ((status = gvcst_search(gv_currkey, NULL)) != cdb_sc_normal)
				{
					assert(CDB_STAGNATE > t_tries);
					t_retry(status);
					continue;
				} else if (gv_currkey->end + 1 != gv_target->hist.h[0].curr_rec.match)
                                {
					if (sizeof(blk_hdr) == ((blk_hdr_ptr_t)gv_target->hist.h[0].buffaddr)->bsiz
						&& 1 == gv_target->hist.depth)
					{
						if (cs_addrs->now_crit)
						{
							t_abort(gv_cur_region, cs_addrs); /* do crit and other cleanup */
							gtm_putmsg(VARLSTCNT(4) ERR_GBLNOEXIST, 2, gn->str.len, gn->str.addr);
							reorg_finish(dest_blk_id, blks_processed, blks_killed, blks_reused,
								file_extended, lvls_reduced,
								blks_coalesced, blks_split, blks_swapped);
							return TRUE; /* It is not an error that global was killed */
						} else
						{
							assert(CDB_STAGNATE > t_tries);
							t_retry(status);
							continue;
						}
					}
                                }
				if (gv_target->hist.depth <= level)
				{
					/* Will come here
					 * 	1) first iteration of the for loop (since level == MAX_BT_DEPTH + 1) or,
					 *	2) tree depth decreased for mu_reduce_level or, M-kill
					 */
					pre_order_successor_level = gv_target->hist.depth - 1;
					if (MAX_BT_DEPTH + 1 != level)
					{
						/* break the loop when tree depth decreased (case 2) */
						level = pre_order_successor_level;
						break;
					}
					level = pre_order_successor_level;
				}
				max_fill = (0 == level)? d_max_fill : i_max_fill;
				toler = (0 == level)? d_toler:i_toler;
				cur_blk_size =  ((blk_hdr_ptr_t)(gv_target->hist.h[level].buffaddr))->bsiz;
				if (cur_blk_size > max_fill + toler && 0 == (reorg_op & NOSPLIT)) /* SPLIT BLOCK */
				{
					cnt1 = cnt2 = 0;
					/* history of current working block is in gv_target */
					status = mu_split(level, i_max_fill, d_max_fill, &cnt1, &cnt2);
					if (cdb_sc_maxlvl == status)
					{
						gtm_putmsg(VARLSTCNT(4) ERR_MAXBTLEVEL, 2, gn->str.len, gn->str.addr);
						reorg_finish(dest_blk_id, blks_processed, blks_killed, blks_reused,
							file_extended, lvls_reduced, blks_coalesced, blks_split, blks_swapped);
						return FALSE;
					} else if (cdb_sc_oprnotneeded != status && cdb_sc_normal != status)
					{
						assert(CDB_STAGNATE > t_tries);
						t_retry(status);
						continue;
					} else if (cdb_sc_normal == status)
					{
						if (!(ret_tn = t_end(&(gv_target->hist), 0)))
						{
							need_kip_incr = FALSE;
							continue;
						}
						if (detailed_log)
							log_detailed_log("SPL", &(gv_target->hist), NULL, level, NULL, ret_tn);
						blks_reused += cnt1;
						lvls_reduced -= cnt2;
						blks_split++;
						break;
					}
					/* else cdb_sc_oprnotneeded == status  <*** DROP THRU ***> */
				} /* end if SPLIT BLOCK */

				/* We are here because, mu_split() was not called or, split was not done or, not required */
				rtsib_hist = gv_target->alt_hist;
				status = gvcst_rtsib(rtsib_hist, level);
				if (cdb_sc_normal != status && cdb_sc_endtree != status)
				{
					assert(CDB_STAGNATE > t_tries);
					t_retry(status);
					continue;
				}
				if (cdb_sc_endtree == status)
				{
					if (0 == level)
						end_of_tree = TRUE;
					break;
				} else if (0 == level)
					pre_order_successor_level = rtsib_hist->depth - 1;
				/* COALESCE WITH RTSIB */
				kill_set_list.used = 0;
				if (cur_blk_size < max_fill - toler && 0 == (reorg_op & NOCOALESCE))
				{
					/* histories are sent in &gv_target->hist and gv_target->alt_hist */
					status = mu_clsce(level, i_max_fill, d_max_fill, &kill_set_list, &complete_merge);
					if (cdb_sc_normal != status && cdb_sc_oprnotneeded != status)
					{
						assert(CDB_STAGNATE > t_tries);
						t_retry(status);
						continue;
					}
					else if (cdb_sc_normal == status)
					{
						if (level) /* delete lower elements of array, t_end might confuse */
						{
							memmove(&rtsib_hist->h[0], &rtsib_hist->h[level],
								sizeof(srch_blk_status)*(rtsib_hist->depth - level + 2));
							rtsib_hist->depth = rtsib_hist->depth - level;
						}
						if (0 < kill_set_list.used)     /* increase kill_in_prog */
							need_kip_incr = TRUE;
						if (!(ret_tn = t_end(&(gv_target->hist), rtsib_hist)))
						{
							need_kip_incr = FALSE;
							assert(!kip_incremented);
							if (level)
							{	/* reinitialize level member in rtsib_hist srch_blk_status' */
								for (count = 0; count < MAX_BT_DEPTH; count++)
									rtsib_hist->h[count].level = count;
							}
							continue;
						}
						if (level)
						{	/* reinitialize level member in rtsib_hist srch_blk_status' */
							for (count = 0; count < MAX_BT_DEPTH; count++)
								rtsib_hist->h[count].level = count;
						}
						if (detailed_log)
							log_detailed_log("CLS", &(gv_target->hist), rtsib_hist, level,
								NULL, ret_tn);
						assert(0 < kill_set_list.used || !kip_incremented);
						if (0 < kill_set_list.used)     /* decrease kill_in_prog */
						{
							gvcst_kill_sort(&kill_set_list);
							GVCST_BMP_MARK_FREE(&kill_set_list, ret_tn, inctn_mu_reorg,
									inctn_bmp_mark_free_mu_reorg, inctn_opcode, cs_addrs)
							DECR_KIP(cs_data, cs_addrs, kip_incremented);
							if (detailed_log)
								log_detailed_log("KIL", &(gv_target->hist), NULL, level,
									&kill_set_list, ret_tn);
							blks_killed += kill_set_list.used;
						}
						blks_coalesced++;
						break;
					}
					/* else cdb_sc_oprnotneeded == status <*** DROP THRU ***> */
				} /* end if try coalesce */
				if (0 == level)
				{
					/* Note: In data block level:
					 *      if split is successful or,
					 *	if coalesce is successful without a complete merge of rtsib,
					 *	then gv_currkey_next_reorg is already set from the called function.
					 *	if split or, coalesce do a retry or,
					 *	if coalesce is successful with a complete merge then
					 *	gv_currkey will not be changed.
					 * If split or, coalesce is not successful or, not needed then
					 *	here gv_currkey_next_reorg will be set from right sibling
					 */
					cw_set_depth = cw_map_depth = 0;
					GET_KEY_LEN(tkeysize, rtsib_hist->h[0].buffaddr + sizeof(blk_hdr) + sizeof(rec_hdr));
					if (2 < tkeysize && MAX_KEY_SZ >= tkeysize)
					{
						memcpy(&(gv_currkey_next_reorg->base[0]), rtsib_hist->h[0].buffaddr
							+ sizeof(blk_hdr) +sizeof(rec_hdr), tkeysize);
						gv_currkey_next_reorg->end = tkeysize - 1;
						inctn_opcode = inctn_invalid_op; /* temporary reset; satisfy an assert in t_end() */
						assert(update_trans);
						update_trans = FALSE; /* tell t_end, this is no longer an update transaction */
						if (!(ret_tn = t_end(rtsib_hist, NULL)))
						{
							need_kip_incr = FALSE;
							inctn_opcode = inctn_mu_reorg;	/* reset inctn_opcode to its default */
							update_trans = TRUE;	/* reset update_trans to its old value */
							assert(!kip_incremented);
							continue;
						}
						/* there is no need to reset update_trans to TRUE in case of a successful t_end()
						 * call. this is because before the next call to t_end() we should have a call to
						 * t_begin() which will reset update_trans anyways.
						 */
						inctn_opcode = inctn_mu_reorg;	/* reset inctn_opcode to its default */
						if (detailed_log)
							log_detailed_log("NOU", rtsib_hist, NULL, level, NULL, ret_tn);
					} else
					{
						assert(CDB_STAGNATE > t_tries);
						t_retry(status);
						continue;
					}
				} /* end if (0 == level) */
				break;
			}/* === SPLIT-COALESCE LOOP END === */
			t_abort(gv_cur_region, cs_addrs);	/* do crit and other cleanup */
		}/* === START WHILE COMPLETE_MERGE === */

		if (mu_ctrlc_occurred || mu_ctrly_occurred)
		{
			cs_data->reorg_restart_block = dest_blk_id;
			memcpy(&cs_data->reorg_restart_key[0], &gv_currkey->base[0], gv_currkey->end+1);
			return FALSE;
		}
		/* Now swap the working block */
		if (0 == (reorg_op & NOSWAP))
		{
			t_begin(ERR_MUREORGFAIL, TRUE);
			/* Following loop is to handle concurrency retry for swap */
			for (; ;)	/* === START OF SWAP LOOP === */
			{
				kill_set_list.used = 0;
				gv_target->clue.end = 0;
				/* search gv_currkey and get the result in gv_target */
				if ((status = gvcst_search(gv_currkey, NULL)) != cdb_sc_normal)
				{
					assert(CDB_STAGNATE > t_tries);
					t_retry(status);
					continue;
				} else if (gv_currkey->end + 1 != gv_target->hist.h[0].curr_rec.match)
                                {
					if (sizeof(blk_hdr) == ((blk_hdr_ptr_t)gv_target->hist.h[0].buffaddr)->bsiz
						&& 1 == gv_target->hist.depth)
					{
						if (cs_addrs->now_crit)
						{
							t_abort(gv_cur_region, cs_addrs); /* do crit and other cleanup */
							gtm_putmsg(VARLSTCNT(4) ERR_GBLNOEXIST, 2, gn->str.len, gn->str.addr);
							reorg_finish(dest_blk_id, blks_processed, blks_killed, blks_reused,
								file_extended, lvls_reduced,
								blks_coalesced, blks_split, blks_swapped);
							return TRUE; /* It is not an error that global was killed */
						} else
						{
							assert(CDB_STAGNATE > t_tries);
							t_retry(status);
							continue;
						}
					}
                                }
				if (gv_target->hist.depth <= level)
					break;
				/* swap working block with appropriate dest_blk_id block.
				   Historys are sent as gv_target->hist and reorg_gv_target->hist */
				status = mu_swap_blk(level, &dest_blk_id, &kill_set_list, exclude_glist_ptr);
				if (cdb_sc_oprnotneeded == status)
				{
					if (cs_data->trans_hist.total_blks <= dest_blk_id)
					{
						util_out_print("REORG may be incomplete for this global.", TRUE);
						reorg_finish(dest_blk_id, blks_processed, blks_killed, blks_reused,
							file_extended, lvls_reduced, blks_coalesced, blks_split, blks_swapped);
						return TRUE;
					}
				} else if (cdb_sc_normal != status)
				{
					assert(CDB_STAGNATE > t_tries);
					t_retry(status);
					continue;
				} else
				{
					if (0 < kill_set_list.used)
					{
						need_kip_incr = TRUE;
						/* second history not needed, because,
						   we are reusing a free block, which does not need history */
						if (!(ret_tn = t_end(&(gv_target->hist), NULL)))
						{
							need_kip_incr = FALSE;
							assert(!kip_incremented);
							DECR_BLK_NUM(dest_blk_id);
							continue;
						}
						if (detailed_log)
							log_detailed_log("SWA", &(gv_target->hist), NULL, level, NULL, ret_tn);
						gvcst_kill_sort(&kill_set_list);
						GVCST_BMP_MARK_FREE(&kill_set_list, ret_tn, inctn_mu_reorg,
								inctn_bmp_mark_free_mu_reorg, inctn_opcode, cs_addrs)
						DECR_KIP(cs_data, cs_addrs, kip_incremented);
						if (detailed_log)
							log_detailed_log("KIL", &(gv_target->hist), NULL, level,
								&kill_set_list, ret_tn);
						blks_reused += kill_set_list.used;
						blks_killed += kill_set_list.used;
					}
					/* gv_target->hist is for working block's history, and
					   reorg_gv_target->hist is for destinition block's history.
					   Note: gv_target and reorg_gv_target can be part of different GVT.  */
					else if (!(ret_tn = t_end(&(gv_target->hist), &(reorg_gv_target->hist))))
					{
						need_kip_incr = FALSE;
						assert(!kip_incremented);
						DECR_BLK_NUM(dest_blk_id);
						continue;
					}
					if ((0 >= kill_set_list.used) && detailed_log)
						log_detailed_log("SWA", &(gv_target->hist), &(reorg_gv_target->hist),
							level, NULL, ret_tn);
					blks_swapped++;
					if (reorg_op & SWAPHIST)
						util_out_print("Dest !SL From !SL", TRUE, dest_blk_id,
							gv_target->hist.h[level].blk_num);
				}
				break;

			}	/* === END OF SWAP LOOP === */
			t_abort(gv_cur_region, cs_addrs);	/* do crit and other cleanup */
		}
		if (mu_ctrlc_occurred || mu_ctrly_occurred)
		{
			cs_data->reorg_restart_block = dest_blk_id;
			memcpy(&cs_data->reorg_restart_key[0], &gv_currkey->base[0], gv_currkey->end + 1);
			return FALSE;
		}
		if (end_of_tree)
			break;
		if (0 < level)
			level--; /* Order of reorg is root towards leaf */
		else
		{
			level = pre_order_successor_level;
			memcpy(&gv_currkey->base[0], &gv_currkey_next_reorg->base[0], gv_currkey_next_reorg->end + 1);
			gv_currkey->end =  gv_currkey_next_reorg->end;
			cs_data->reorg_restart_block = dest_blk_id;
			memcpy(&cs_data->reorg_restart_key[0], &gv_currkey->base[0], gv_currkey->end + 1);
		}
	}		/* ================ END MAIN LOOP ================ */

	/* =========== START REDUCE LEVEL ============== */
	memcpy(&gv_currkey->base[0], gn->str.addr, gn->str.len);
	gv_currkey->base[gn->str.len] = gv_currkey->base[gn->str.len + 1] = 0;
	gv_currkey->end = gn->str.len + 1;
	for (;;)	/* Reduce level continues until it fails to reduce */
	{
		t_begin(ERR_MUREORGFAIL, TRUE);
		cnt1 = 0;
		for (; ;) 	/* main reduce level loop starts */
		{
			kill_set_list.used = 0;
			gv_target->clue.end = 0;
			/* search gv_currkey and get the result in gv_target */
			if ((status = gvcst_search(gv_currkey, NULL)) != cdb_sc_normal)
			{
				assert(CDB_STAGNATE > t_tries);
				t_retry(status);
				continue;
			} else if (gv_currkey->end + 1 != gv_target->hist.h[0].curr_rec.match)
			{
				if (sizeof(blk_hdr) == ((blk_hdr_ptr_t)gv_target->hist.h[0].buffaddr)->bsiz
					&& 1 == gv_target->hist.depth)
				{
					if (cs_addrs->now_crit)
					{
						t_abort(gv_cur_region, cs_addrs);	/* do crit and other cleanup */
						gtm_putmsg(VARLSTCNT(4) ERR_GBLNOEXIST, 2, gn->str.len, gn->str.addr);
						reorg_finish(dest_blk_id, blks_processed, blks_killed, blks_reused,
							file_extended, lvls_reduced, blks_coalesced, blks_split, blks_swapped);
						return TRUE; /* It is not an error that global was killed */
					} else
					{
						assert(CDB_STAGNATE > t_tries);
						t_retry(status);
						continue;
					}
				}
			}
			if (gv_target->hist.depth <= level)
				break;
			/* History is passed in gv_target->hist */
			status = mu_reduce_level(&kill_set_list);
			if (cdb_sc_oprnotneeded != status && cdb_sc_normal != status)
			{
				assert(CDB_STAGNATE > t_tries);
				t_retry(status);
				continue;
			} else if (cdb_sc_normal == status)
			{
				assert(0 < kill_set_list.used);
				need_kip_incr = TRUE;
				if (!(ret_tn = t_end(&(gv_target->hist), NULL)))
				{
					need_kip_incr = FALSE;
					assert(!kip_incremented);
					continue;
				}
				if (detailed_log)
					log_detailed_log("RDL", &(gv_target->hist), NULL, level, NULL, ret_tn);
				gvcst_kill_sort(&kill_set_list);
				GVCST_BMP_MARK_FREE(&kill_set_list, ret_tn, inctn_mu_reorg,
						inctn_bmp_mark_free_mu_reorg, inctn_opcode, cs_addrs)
				DECR_KIP(cs_data, cs_addrs, kip_incremented);
				if (detailed_log)
					log_detailed_log("KIL", &(gv_target->hist), NULL, level, &kill_set_list, ret_tn);
				blks_reused += kill_set_list.used;
				blks_killed += kill_set_list.used;
				cnt1 = 1;
				lvls_reduced++;
			}
			break;
		} 		/* main reduce level loop ends */
		t_abort(gv_cur_region, cs_addrs); /* do crit and other cleanup */
		if (0 == cnt1)
			break;
	}
	/* =========== END REDUCE LEVEL ===========*/

	reorg_finish(dest_blk_id, blks_processed, blks_killed, blks_reused,
		file_extended, lvls_reduced, blks_coalesced, blks_split, blks_swapped);
	return TRUE;

} /* end mu_reorg() */

/**********************************************
 Statistics of reorg for current global.
 Also update dest_blklist_ptr for next globals
***********************************************/
void reorg_finish(block_id dest_blk_id, int blks_processed, int blks_killed,
	int blks_reused, int file_extended, int lvls_reduced,
	int blks_coalesced, int blks_split, int blks_swapped)
{
	t_abort(gv_cur_region, cs_addrs);
	file_extended = cs_data->trans_hist.total_blks - file_extended;
	util_out_print("Blocks processed    : !SL ", FLUSH, blks_processed);
	util_out_print("Blocks coalesced    : !SL ", FLUSH, blks_coalesced);
	util_out_print("Blocks split        : !SL ", FLUSH, blks_split);
	util_out_print("Blocks swapped      : !SL ", FLUSH, blks_swapped);
	util_out_print("Blocks freed        : !SL ", FLUSH, blks_killed);
	util_out_print("Blocks reused       : !SL ", FLUSH, blks_reused);
	if (0 > lvls_reduced)
		util_out_print("Levels Increased    : !SL ", FLUSH, -lvls_reduced);
	else if (0 < lvls_reduced)
		util_out_print("Levels Eliminated   : !SL ", FLUSH, lvls_reduced);
	util_out_print("Blocks extended     : !SL ", FLUSH, file_extended);
	cs_addrs->reorg_last_dest = dest_blk_id;

	/* next attempt for this global will start from the beginning, if RESUME option is present */
	cs_data->reorg_restart_block = 0;
	cs_data->reorg_restart_key[0] = 0;
	cs_data->reorg_restart_key[1] = 0;
}

/* end of program */
