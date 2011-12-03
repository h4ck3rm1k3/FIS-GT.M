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
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "gdsbml.h"
#include "copy.h"
#include "subscript.h"
#include "filestruct.h"
#include "gdscc.h"
#include "gdskill.h"    /* needed for tp.h */
#include "jnl.h"        /* needed for tp.h */
#include "hashtab.h"    /* needed for tp.h */
#include "buddy_list.h" /* needed for tp.h */
#include "tp.h"
#include "error.h"
#include "mmemory.h"
#include "gtm_ffs.h"
#include "cert_blk.h"

GBLREF short		dollar_tlevel;

#define BITS_PER_UCHAR	8
#define BLKS_PER_UINT4	((sizeof(uint4) / sizeof(unsigned char)) * BITS_PER_UCHAR) / BML_BITS_PER_BLK
#define BLOCK_WINDOW 8
#define LEVEL_WINDOW 3
#define OFFSET_WINDOW 4

#define TEXT1 ":     "
#define TEXT2 " "
#define TEXT3 ":"
#define TEXT4 ", "

#define MAX_UTIL_LEN 40

void rts_error_func(int err, uchar_ptr_t buff);

int cert_blk (gd_region *reg, block_id blk, blk_hdr_ptr_t bp, block_id root)
{
	block_id		child;
	rec_hdr_ptr_t		rp, r_top;
	bool			dummy_bool, first_key, full, not_gvt;
	int			num_subscripts, key_max_subs = 0;
	uint4			bplmap, mask1, offset;
	sm_uint_ptr_t		chunk_p;			/* Value is unaligned so will be assigned to chunk */
	uint4			chunk;
	sm_uc_ptr_t		blk_top, blk_id_ptr, key_base, mp, b_ptr;
	unsigned char		rec_cmpc;
	unsigned char		ch, prior_expkey[MAX_KEY_SZ + 1];
	unsigned int		prior_expkeylen;
	unsigned short		temp_ushort;
	int			blk_levl;
	int			blk_size, rec_size, comp_length, rec_offset, key_size;
	unsigned char		util_buff[MAX_UTIL_LEN];
	int			util_len;
	off_chain		chain;
	sgmnt_addrs		*csa;
	sgmnt_data_ptr_t	csd;

	error_def(ERR_DBBLEVMX);
	error_def(ERR_DBBLEVMN);
	error_def(ERR_DBBSIZMN);
	error_def(ERR_DBBSIZMX);
	error_def(ERR_DBRSIZMN);
	error_def(ERR_DBRSIZMX);
	error_def(ERR_DBCMPNZRO);
	error_def(ERR_DBSTARSIZ);
	error_def(ERR_DBSTARCMP);
	error_def(ERR_DBCMPMX);
	error_def(ERR_DBKEYMX);
	error_def(ERR_DBKEYMN);
	error_def(ERR_DBCMPBAD);
	error_def(ERR_DBKEYORD);
	error_def(ERR_DBPTRNOTPOS);
	error_def(ERR_DBPTRMX);
	error_def(ERR_DBPTRMAP);
	error_def(ERR_DBLVLINC);
	error_def(ERR_DBBMSIZE);
	error_def(ERR_DBBMBARE);
	error_def(ERR_DBBMINV);
	error_def(ERR_DBBMMSTR);
	error_def(ERR_DBROOTBURN);
	error_def(ERR_DBCMPZERO);
	error_def(ERR_DBROOTSUBSC);
	error_def(ERR_DBMAXNRSUBS); /* same error as ERR_MAXNRSUBSCRIPTS, but has a string output as well */

	csa = &FILE_INFO(reg)->s_addrs;
	csd = csa->hdr;
	bplmap = csd->bplmap;
	assert(bplmap == BLKS_PER_LMAP);
	blk_levl = bp->levl;
	blk_size = bp->bsiz;
	offset = (uint4)blk / bplmap;

	util_len=0;
	i2hex_blkfill(blk,&util_buff[util_len], BLOCK_WINDOW);
	util_len += BLOCK_WINDOW;
	memcpy(&util_buff[util_len], TEXT1, sizeof(TEXT1) - 1); /* OFFSET_WINDOW + 1 spaces */
	util_len += sizeof(TEXT3) - 1;
	util_len += OFFSET_WINDOW +1;
	i2hex_blkfill(blk_levl, &util_buff[util_len], LEVEL_WINDOW);
	util_len += LEVEL_WINDOW;
	memcpy(&util_buff[util_len], TEXT2, sizeof(TEXT2) - 1);
	util_len += sizeof(TEXT2) - 1;
	util_buff[util_len] = 0;

	chain = *(off_chain *)&blk;
	assert(!chain.flag || dollar_tlevel && !csa->t_commit_crit);
	if (!chain.flag && (offset * bplmap) == (uint4)blk)					/* it's a bitmap */
	{
		if ((unsigned char)blk_levl != LCL_MAP_LEVL)
		{
			rts_error_func(MAKE_MSG_INFO(ERR_DBLVLINC), util_buff);
			return FALSE;
		}
		if (blk_size != BM_SIZE(bplmap))
		{
			rts_error_func(ERR_DBBMSIZE, util_buff);
			return FALSE;
		}
		mp = (sm_uc_ptr_t)bp + sizeof(blk_hdr);
		if ((*mp & 1) != 0)
		{	/* bitmap doesn't protect itself */
			rts_error_func(ERR_DBBMBARE, util_buff);
			return FALSE;
		}
		full = TRUE;
		offset = ((csa->ti->total_blks - blk) >= bplmap) ? bplmap : (csa->ti->total_blks - blk);
		blk_top = (sm_uc_ptr_t)bp + BM_SIZE(offset + (BITS_PER_UCHAR / BML_BITS_PER_BLK) - 1);
		for (chunk_p = (sm_uint_ptr_t)mp ;  (sm_uc_ptr_t)chunk_p - blk_top < 0 ;  chunk_p++)
		{
			GET_LONG(chunk, chunk_p);		/* Obtain unalinged unit4 value */
			/* The following code is NOT independent of the bitmap layout: */
			mask1 = chunk & SIXTEEN_BLKS_FREE;	/* mask 'recycled' blocks to 'free' blocks */
			if ((mask1 != 0) && full)		/* check for free blocks */
			{
				/* if (full bitmap || full chunk || regular scan of a "short" bitmap) */
				if ((offset == bplmap) || ((blk_top - (sm_uc_ptr_t)chunk_p) > sizeof(chunk))
				    || (bml_find_free((sm_uc_ptr_t)chunk_p - mp, mp, offset, &dummy_bool) != NO_FREE_SPACE))
				{
					full = FALSE;
				}
			}
			mask1 ^= SIXTEEN_BLKS_FREE;		/* complement to busy */
			mask1 <<= 1;				/* shift to reused position */
			mask1 &= chunk;				/* check against the original contents */
			if (mask1 != 0)				/* busy and reused should never appear together */
			{
				rts_error_func(ERR_DBBMINV, util_buff);
				return FALSE;
			}

		}
		if (full == (NO_FREE_SPACE != gtm_ffs(blk / bplmap, csd->master_map, MASTER_MAP_BITS_PER_LMAP)))
		{
			rts_error_func(ERR_DBBMMSTR, util_buff);
			return FALSE;
		}
		return TRUE;
	}

	if (blk_levl > MAX_BT_DEPTH)
	{
		rts_error_func(ERR_DBBLEVMX, util_buff);
		return FALSE;
	}
	if (blk_levl < 0)
	{
		rts_error_func(ERR_DBBLEVMN, util_buff);
		return FALSE;
	}
	if (blk_levl == 0)
	{	/* data block */
		if ((blk == 1) || ((0 != root) && (blk == root)))
		{	/* headed for where an index block should be */
			rts_error_func(ERR_DBROOTBURN, util_buff);
			return FALSE;
		}
		if (blk_size < sizeof(blk_hdr))
		{
			rts_error_func(ERR_DBBSIZMN, util_buff);
			return FALSE;
		}
	} else
	{	/* index block */
		if (blk_size < (sizeof(blk_hdr) + sizeof(rec_hdr) + sizeof(block_id)))
		{	/* must have at least one record */
			rts_error_func(ERR_DBBSIZMN, util_buff);
			return FALSE;
		}
	}
	if (blk_size > csd->blk_size)
	{
		rts_error_func(ERR_DBBSIZMX, util_buff);
		return FALSE;
	}

	not_gvt = ((root > 0) && (root != 1));
	blk_top = (sm_uc_ptr_t)bp + blk_size;
	first_key = TRUE;
	prior_expkeylen = 0;
	comp_length = 2 * sizeof(char);		/* for double NUL to indicate no prior key */
	prior_expkey[0] = prior_expkey[1] = 0;	/* double NUL also works for memvcmp test for key order */

	for (rp = (rec_hdr_ptr_t)((sm_uc_ptr_t)bp + sizeof(blk_hdr)) ;  rp < (rec_hdr_ptr_t)blk_top ;  rp = r_top)
	{
		GET_RSIZ(rec_size, rp);
		rec_offset = (sm_ulong_t)rp - (sm_ulong_t)bp;
		/*add util_buff here*/

		util_len=0;
		i2hex_blkfill(blk,&util_buff[util_len], BLOCK_WINDOW);
		util_len += BLOCK_WINDOW;
		memcpy(&util_buff[util_len], TEXT1, sizeof(TEXT1) - 1); /* OFFSET_WINDOW + 1 spaces */
		util_len += sizeof(TEXT3) - 1;
		i2hex_nofill(rec_offset, &util_buff[util_len], OFFSET_WINDOW);
		util_len += OFFSET_WINDOW +1;
		i2hex_blkfill(blk_levl, &util_buff[util_len], LEVEL_WINDOW);
		util_len += LEVEL_WINDOW;
		memcpy(&util_buff[util_len], TEXT2, sizeof(TEXT2) - 1);
		util_len += sizeof(TEXT2) - 1;
		util_buff[util_len] = 0;


		if (rec_size <= sizeof(rec_hdr))
		{
			rts_error_func(ERR_DBRSIZMN, util_buff);
			return FALSE;
		}
		if (rec_size > (short)((sm_ulong_t)blk_top - (sm_ulong_t)rp))
		{
			rts_error_func(ERR_DBRSIZMX, util_buff);
			return FALSE;
		}
		r_top = (rec_hdr_ptr_t)((sm_ulong_t)rp + rec_size);
		rec_cmpc = rp->cmpc;
		if(first_key)
		{
			first_key = FALSE;
			if (rec_cmpc)
			{
				rts_error_func(ERR_DBCMPNZRO, util_buff);
				return FALSE;
			}
			if (0 == blk_levl)
			{
				ch = *((sm_uc_ptr_t)rp + sizeof(rec_hdr));
				if (!(ISALPHA(ch) || '%' == ch))
					GTMASSERT;
			}
		}
		else
		{
			if (not_gvt && (rec_cmpc == 0) && (blk_levl == 0))
			{
				rts_error_func(ERR_DBCMPZERO, util_buff);
				return FALSE;
			}
		}
		if (r_top == (rec_hdr_ptr_t)blk_top && blk_levl)
		{
			if (rec_size != sizeof(rec_hdr) + sizeof(block_id))
			{
				rts_error_func(ERR_DBSTARSIZ, util_buff);
				return FALSE;
			}
			if (rec_cmpc)
			{
				rts_error_func(ERR_DBSTARCMP, util_buff);
				return FALSE;
			}
			blk_id_ptr = (sm_uc_ptr_t)rp + sizeof(rec_hdr);
		}
		else
		{
			key_base = (sm_uc_ptr_t)rp + sizeof(rec_hdr);
			if (rec_cmpc && rec_cmpc >= prior_expkeylen)
			{
				rts_error_func(ERR_DBCMPMX, util_buff);
				return FALSE;
			}
			/* num_subscripts = number of full subscripts found in the key (do not consider compressed part)
			   In the GVT for rec_cmpc != 0, actual number of subscripts in the expanded key is at least one more. */
			num_subscripts = 0;
			for (blk_id_ptr = key_base ;  ;  )
			{
				if (blk_id_ptr >= (sm_uc_ptr_t)r_top)
				{
					rts_error_func(ERR_DBKEYMX, util_buff);
					return FALSE;
				}
				if (KEY_DELIMITER == *blk_id_ptr++)
				{
					if (KEY_DELIMITER == *blk_id_ptr++)
						break;	/* found key terminator */
					else
						num_subscripts++;
				}
			}
			/* root of the directory tree contains only name-level globals */
			if (DIR_ROOT == blk && num_subscripts)
			{
				rts_error_func(ERR_DBROOTSUBSC, util_buff);
				return FALSE;
			}
			key_size = blk_id_ptr - key_base;
			/* key_max_subs is an estimate of total number of subscripts present in currrent key when expanded.
			   if MAX_GVSUBSCRIPTS < key_max_subs
				then we need to calculate actual number of subscripts for the current key when expanded
		 	   else
				there is no chance of exceeding the limit and do not need to scan the compressed part */
			key_max_subs += num_subscripts + 1;
			if (MAX_GVSUBSCRIPTS < key_max_subs)
			{
				/* It is not necessary that we exceeded the limit on number of subscripts.
				   We do not know the number of subscripts in rec_cmpc length of the buffer prior_expkey.
				   So scan entire rec_cmpc length and find real number of subscripts */
				key_max_subs = num_subscripts;
				for (b_ptr = prior_expkey ; b_ptr < prior_expkey + rec_cmpc ; )
					if (KEY_DELIMITER == *b_ptr++)
						key_max_subs++;
				if (MAX_GVSUBSCRIPTS < key_max_subs)
				{
					rts_error_func(ERR_DBMAXNRSUBS, util_buff);
					return FALSE;
				}
			}
			if (blk_levl && key_size != rec_size - sizeof(block_id) - sizeof(rec_hdr))
			{
				rts_error_func(ERR_DBKEYMN, util_buff);
				return FALSE;
			}
			if (rec_cmpc < prior_expkeylen && prior_expkey[rec_cmpc] == *key_base)
			{
				rts_error_func(ERR_DBCMPBAD, util_buff);
				return FALSE;
			}
			if (memvcmp(prior_expkey + rec_cmpc, comp_length - rec_cmpc, key_base, key_size) >= 0)
			{
				rts_error_func(ERR_DBKEYORD, util_buff);
				return FALSE;
			}
			memcpy(prior_expkey + rec_cmpc, key_base, key_size);
			prior_expkeylen = rec_cmpc + key_size;
		}
		/* Check for proper child block numbers only if in commit phase */
		if (blk_levl != 0)
		{	/* index block */
			GET_LONG(child, blk_id_ptr);
			chain = *(off_chain *)&child;
			assert(!chain.flag || dollar_tlevel && !csa->t_commit_crit);
			if (!chain.flag)
			{
				if (child <= 0)
				{
					rts_error_func(ERR_DBPTRNOTPOS, util_buff);
					return FALSE;
				}
				if (child > csa->ti->total_blks)
				{
					rts_error_func(ERR_DBPTRMX, util_buff);
					return FALSE;
				}
				if (!(child % bplmap))
				{
					rts_error_func(ERR_DBPTRMAP, util_buff);
					return FALSE;
				}
			}
		}
		comp_length = prior_expkeylen;
	}
	return TRUE;

}

void rts_error_func(int err, uchar_ptr_t buff)
{
	rts_error(VARLSTCNT(4) MAKE_MSG_INFO(err), 2, LEN_AND_STR((char_ptr_t)buff));
}
