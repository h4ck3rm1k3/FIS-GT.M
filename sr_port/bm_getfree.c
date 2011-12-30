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

/**************************************************************************************
*
*		MODULE NAME:			BM_GETFREE.C
*
*		CALLING SEQUENCE:		block_id bm_get_free(hint, blk_used, cw_work, cs, cw_depth_ptr)
*
*		DESCRIPTION:	Takes a block id as a hint and tries to find a
*		free block, looking first at the hint, then in the same local
*		bitmap, and finally in every local map.  If it finds a free block,
*		it looks for the bitmap in the working set, puts it in the working
*		set if it is not there, and updates the map used, and marks it with
*		the transaction number, and updates the boolean
*		pointed to by blk_used to indicate if the free block had been used previously.
*
*		HISTORY:
*
***************************************************************************************/
#include "mdef.h"

#include "gtm_string.h"

#include "cdb_sc.h"
#include "gdsroot.h"
#include "gdsbt.h"
#include "gdsblk.h"
#include "gdsbml.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsfhead.h"
#include "min_max.h"
#include "gdsblkops.h"
#include "filestruct.h"
#include "gdscc.h"
#include "gdskill.h"	/* needed for tp.h */
#include "jnl.h"	/* needed for tp.h */
#include "hashtab.h"	/* needed for tp.h */
#include "buddy_list.h"	/* needed for tp.h */
#include "tp.h"		/* needed for ua_list for ENSURE_UPDATE_ARRAY_SPACE macro */
#include "iosp.h"
#include "bmm_find_free.h"

/* Include prototypes */
#include "t_qread.h"
#include "t_write_map.h"
#include "bit_clear.h"
#include "send_msg.h"
#include "bm_getfree.h"
#include "gdsfilext.h"

GBLREF sgmnt_addrs	*cs_addrs;
GBLREF sgmnt_data_ptr_t	cs_data;
GBLREF char		*update_array, *update_array_ptr;
GBLREF gd_region	*gv_cur_region;
GBLREF unsigned char	rdfail_detail;
GBLREF short		dollar_tlevel;
GBLREF uint4		update_array_size, cumul_update_array_size;
GBLREF unsigned int	t_tries;

block_id bm_get_free(block_id orig_hint, bool *blk_used, unsigned int cw_work, cw_set_element *cs, int *cw_depth_ptr)
{
	cw_set_element	*cs1;
	sm_uc_ptr_t	bmp;
	block_id	bml, hint, hint_cycled, hint_limit;
	block_id_ptr_t	b_ptr;
	cache_rec_ptr_t	cr;
	int		cw_set_top, cycle, depth;
	unsigned int	lcnt, local_maps, map_size, n_decrements = 0, total_blks;
	trans_num	ctn;
	int4		free_bit, offset;
	uint4		space_needed;
	uint4		status;

	total_blks = (dba_mm == cs_data->acc_meth) ? cs_addrs->total_blks : cs_addrs->ti->total_blks;
	if (orig_hint >= total_blks)		/* for TP, hint can be > total_blks */
		orig_hint = 1;
	hint = orig_hint;
	hint_cycled = DIVIDE_ROUND_UP(total_blks, BLKS_PER_LMAP);
	hint_limit = DIVIDE_ROUND_DOWN(orig_hint, BLKS_PER_LMAP);
	local_maps = hint_cycled + 2;	/* for (up to) 2 wraps */
	for (lcnt = 0; lcnt <= local_maps; lcnt++)
	{
		bml = bmm_find_free(hint / BLKS_PER_LMAP, (sm_uc_ptr_t)cs_data->master_map, local_maps);
		if ((NO_FREE_SPACE == bml) || (bml >= hint_cycled))
		{	/* if no free space or might have looped to original map, extend */
			if ((NO_FREE_SPACE != bml) && (hint_limit < hint_cycled))
			{
				hint_cycled = hint_limit;
				hint = 1;
				continue;
			}
			if (SS_NORMAL != (status = gdsfilext(cs_data->extension_size, total_blks)))
				return (status);
			if (dba_mm == cs_data->acc_meth)
				return (FILE_EXTENDED);
			hint = total_blks;
			total_blks = cs_addrs->ti->total_blks;
			hint_cycled = DIVIDE_ROUND_UP(total_blks, BLKS_PER_LMAP);
			local_maps = hint_cycled + 2;	/* for (up to) 2 wraps */
			/*
			 * note that you can make an optimization of not going back over the whole database and going over
			 * only the extended section. but since it is very unlikely that a free block won't be found
			 * in the extended section and the fact that we are starting from the extended section in either
			 * approach and the fact that we have a GTMASSERT to check that we don't have a lot of
			 * free blocks while doing an extend and the fact that it is very easy to make the change to do
			 * a full-pass, the full-pass solution is currently being implemented
			 */
			lcnt = -1;	/* allow it one extra pass to ensure that it can take advantage of the entension */
			n_decrements++;	/* used only for debugging purposes */
			continue;
		}
		bml *= BLKS_PER_LMAP;
		if (ROUND_DOWN2(hint, BLKS_PER_LMAP) != bml)
		{	/* not within requested map */
			if ((bml < hint) && (hint_cycled))	/* wrap? - second one should force an extend for sure */
				hint_cycled = (hint_limit < hint_cycled) ? hint_limit: 0;
			hint = bml + 1;				/* start at beginning */
		}
		if (ROUND_DOWN2(total_blks, BLKS_PER_LMAP) == bml)
			map_size = (total_blks - bml);
		else
			map_size = BLKS_PER_LMAP;
		if (0 != dollar_tlevel)
		{
			depth = cw_work;
			cw_set_top = *cw_depth_ptr;
			if (depth < cw_set_top)
				tp_get_cw(cs, cw_work, &cs1);
			for (; depth < cw_set_top;  depth++, cs1 = cs1->next_cw_set)
			{	/* do tp front to back because list is more efficient than tp_get_cw and forward pointers exist */
				if (bml == cs1->blk)
				{
					TRAVERSE_TO_LATEST_CSE(cs1);
					break;
				}
			}
			if (depth >= cw_set_top)
			{
				assert(cw_set_top == depth);
				depth = 0;
			}
		} else
		{
			for (depth = *cw_depth_ptr - 1; depth >= cw_work;  depth--)
			{	/* do non-tp back to front, because of adjacency */
				if (bml == (cs + depth)->blk)
				{
					cs1 = cs + depth;
					break;
				}
			}
			if (depth < cw_work)
			{
				assert(cw_work - 1 == depth);
				depth = 0;
			}
		}
		if (0 == depth)
		{
			ctn = cs_addrs->ti->curr_tn;
			if (!(bmp = t_qread(bml, (sm_int_ptr_t)&cycle, &cr)))
				return MAP_RD_FAIL;
			if ((BM_SIZE(BLKS_PER_LMAP) != ((blk_hdr_ptr_t)bmp)->bsiz) || (LCL_MAP_LEVL != ((blk_hdr_ptr_t)bmp)->levl))
			{
				assert(CDB_STAGNATE > t_tries);
				rdfail_detail = cdb_sc_badbitmap;
				return MAP_RD_FAIL;
			}
			offset = 0;
		} else
		{
			bmp = cs1->old_block;
			b_ptr = (block_id_ptr_t)(cs1->upd_addr);
			b_ptr += cs1->reference_cnt - 1;
			offset = *b_ptr + 1 - bml;
		}
		if (offset < map_size)
		{
			free_bit = bm_find_blk(offset, (sm_uc_ptr_t)bmp + sizeof(blk_hdr), map_size, blk_used);
			if (MAP_RD_FAIL == free_bit)
				return MAP_RD_FAIL;
		} else
			free_bit = NO_FREE_SPACE;
		if (NO_FREE_SPACE != free_bit)
			break;
		if ((hint = bml + BLKS_PER_LMAP) >= total_blks)		/* if map is full, start at 1st blk in next map */
		{	/* wrap - second one should force an extend for sure */
			hint = 1;
			if (hint_cycled)
				hint_cycled = (hint_limit < hint_cycled) ? hint_limit: 0;
		}
		if ((0 == depth) && (FALSE != cs_addrs->now_crit))	/* if it's from the cw_set, its state is murky */
			bit_clear(bml / BLKS_PER_LMAP, cs_data->master_map);	/* if crit, repair master map error */
	}
	/* If not in the final retry, it is possible that free_bit is >= map_size (e.g. if bitmap block gets recycled). */
	if (map_size <= (uint4)free_bit && CDB_STAGNATE <= t_tries)
	{	/* bad free bit */
		assert((NO_FREE_SPACE == free_bit) && (lcnt > local_maps));	/* All maps full, should have extended */
		GTMASSERT;
	}
	if (0 != depth)
	{
		b_ptr = (block_id_ptr_t)(cs1->upd_addr);
		b_ptr += cs1->reference_cnt++;
		*b_ptr = free_bit + bml;
	} else
	{
		space_needed = (BLKS_PER_LMAP + 1) * sizeof(block_id);
		if (dollar_tlevel)
		{
			ENSURE_UPDATE_ARRAY_SPACE(space_needed);	/* have brackets for "if" for macros */
		}
		BLK_ADDR(b_ptr, space_needed, block_id);
		memset(b_ptr, 0, space_needed);
		*b_ptr = free_bit + bml;
		t_write_map(bml, bmp, (uchar_ptr_t)b_ptr, ctn);
		if (0 != dollar_tlevel)
		{
			tp_get_cw(cs, (int)(*cw_depth_ptr - 1), &cs1);
			assert(!cs1->done);
		} else
			cs1 = cs + *cw_depth_ptr;
		cs1->reference_cnt++;
		cs1->cycle = cycle;
		cs1->cr = cr;
	}
	return bml + free_bit;
}

/* This routine returns whether the free_blocks counter in the file-header is ok (TRUE) or not (FALSE).
 * If not, it corrects it. This assumes cs_addrs, cs_data and gv_cur_region to point to the region of interest.
 * It also assumes that the master-map is correct and finds out non-full local bitmaps and counts the number of
 * free blocks in each of them and sums them up to determine the perceived correct free_blocks count.
 * The reason why this is ok is that even if the master-map incorrectly reports a local bitmap as full, our new free_blocks
 * count will effectively make the free space in that local-bitmap invisible and make a gdsfilext necessary and valid.
 * A later mupip integ will scavenge that invisible space for us. The worst that can therefore happen is that we will transiently
 * not be using up existing space. But we will always ensure that the free_blocks counter goes in sync with the master-map.
 */
boolean_t	is_free_blks_ctr_ok(void)
{
	bool		blk_used;
	block_id	bml, free_bit, free_bml;
	cache_rec_ptr_t	cr;
	int		cycle;
	sm_uc_ptr_t	bmp;
	unsigned int	local_maps, total_blks, free_blocks;

	error_def(ERR_DBBADFREEBLKCTR);

	assert(&FILE_INFO(gv_cur_region)->s_addrs == cs_addrs && cs_addrs->hdr == cs_data && cs_addrs->now_crit);
	total_blks = (dba_mm == cs_data->acc_meth) ? cs_addrs->total_blks : cs_addrs->ti->total_blks;
	local_maps = DIVIDE_ROUND_UP(total_blks, BLKS_PER_LMAP);
	for (free_blocks = 0, free_bml = 0; free_bml < local_maps; free_bml++)
	{
		bml = bmm_find_free((uint4)free_bml, (sm_uc_ptr_t)cs_data->master_map, local_maps);
		if (bml < free_bml)
			break;
		bml *= BLKS_PER_LMAP;
		if (!(bmp = t_qread(bml, (sm_int_ptr_t)&cycle, &cr))
				|| (BM_SIZE(BLKS_PER_LMAP) != ((blk_hdr_ptr_t)bmp)->bsiz)
				|| (LCL_MAP_LEVL != ((blk_hdr_ptr_t)bmp)->levl))
		{
			assert(FALSE);	/* In pro, we will simply skip counting this local bitmap. */
			continue;
		}
		for (free_bit = 0; free_bit < BLKS_PER_LMAP; free_bit++)
		{
			free_bit = bm_find_blk(free_bit, (sm_uc_ptr_t)bmp + sizeof(blk_hdr), BLKS_PER_LMAP, &blk_used);
			assert(NO_FREE_SPACE <= free_bit);
			if (0 > free_bit)
				break;
			free_blocks++;
		}
	}
	assert(cs_addrs->ti->free_blocks == free_blocks);
	if (cs_addrs->ti->free_blocks != free_blocks)
	{
		send_msg(VARLSTCNT(6) ERR_DBBADFREEBLKCTR, 4, DB_LEN_STR(gv_cur_region), cs_addrs->ti->free_blocks, free_blocks);
		cs_addrs->ti->free_blocks = free_blocks;
		return FALSE;
	}
	return TRUE;
}
