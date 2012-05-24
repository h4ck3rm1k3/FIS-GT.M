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
#include "cdb_sc.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "copy.h"

/* Include prototypes */
#include "t_qread.h"
#include "gvcst_search_blk.h"
#include "gvcst_rtsib.h"

/* construct a new array which is the path to the
   right sibling of the leaf of the old array
  note: the offset's will be correct, but NOT the 'match' members
*/

/* WARNING:	assumes that the search history for the current target is in gv_target.hist */

GBLREF sgmnt_addrs	*cs_addrs;
GBLREF gv_namehead	*gv_target;
GBLREF gv_key		*gv_currkey;
GBLREF cdb_sc	rdfail_detail;
GBLREF unsigned int	t_tries;
GBLREF srch_blk_status	*first_tp_srch_status;	/* overriding value of srch_blk_status given by t_qread in case of TP */
GBLREF short            dollar_tlevel;

enum cdb_sc	gvcst_rtsib(srch_hist *full_hist, int level)
{
	srch_blk_status *old, *newdata, *old_base, *new_base;
	rec_hdr_ptr_t	rp;
	enum cdb_sc	ret_val;
	block_id	blk;
	unsigned short	rec_size, temp_short;
	sm_uc_ptr_t	buffer_address;
	//	int4		cycle;

	new_base = &full_hist->h[level];
	old = old_base = &gv_target->hist.h[level];
	for (;;)
	{
		old++;
		if (0 == old->blk_num)
			return cdb_sc_endtree;
		if (old->cr && (old->blk_num != old->cr->blk))
		{
			assert(CDB_STAGNATE > t_tries);
			return cdb_sc_lostcr;
		}
		if (cdb_sc_normal != (ret_val = gvcst_search_blk(gv_currkey, old)))
			return ret_val;
		buffer_address = old->buffaddr;
		temp_short = old->curr_rec.offset;
		rp = (rec_hdr_ptr_t)((sm_uc_ptr_t)buffer_address + temp_short);
		GET_USHORT(rec_size, &rp->rsiz);
		if ((sm_uc_ptr_t)rp + rec_size > buffer_address + (unsigned int)((blk_hdr_ptr_t)buffer_address)->bsiz)
		{
			assert(CDB_STAGNATE > t_tries);
			return cdb_sc_rmisalign;
		}
		if ((unsigned int)((blk_hdr_ptr_t)buffer_address)->bsiz > (temp_short + rec_size))
			break;
	}
	/* old now points to the first block which did not have a star key pointer*/
	newdata= new_base + (old - old_base + 1);
	full_hist->depth = level + old - old_base;
	(newdata--)->blk_num = 0;
	newdata->tn = old->tn;
	newdata->ptr = (cw_set_element_struct*)NULL;
	newdata->first_tp_srch_status = old->first_tp_srch_status;
	assert(newdata->level == old->level);
	assert(newdata->blk_target == old->blk_target);
	newdata->blk_num = old->blk_num;
	newdata->buffaddr = old->buffaddr;
	newdata->prev_rec = old->curr_rec;
	newdata->cycle = old->cycle;
	newdata->cr = old->cr;
	temp_short = newdata->prev_rec.offset;
	rp = (rec_hdr_ptr_t)(temp_short + newdata->buffaddr);
	GET_USHORT(rec_size, &rp->rsiz);
	temp_short += rec_size;
	newdata->curr_rec.offset = temp_short;
	newdata->curr_rec.match = 0;
	if (((blk_hdr_ptr_t)old->buffaddr)->bsiz < temp_short)
	{
		assert(CDB_STAGNATE > t_tries);
		return cdb_sc_rmisalign;
	}
	rp = (rec_hdr_ptr_t)(old->buffaddr + temp_short);
	while (--newdata >= new_base)
	{
		--old;
		GET_USHORT(rec_size, &rp->rsiz);
		if ((sm_uc_ptr_t)rp + rec_size > buffer_address + (unsigned int)((blk_hdr_ptr_t)buffer_address)->bsiz)
		{
			assert(CDB_STAGNATE > t_tries);
			return cdb_sc_rmisalign;
		}
		GET_LONG(blk, ((sm_int_ptr_t)((sm_uc_ptr_t)rp + rec_size - sizeof(block_id))));
		newdata->tn = cs_addrs->ti->curr_tn;
		newdata->ptr = (cw_set_element_struct*) NULL;
		if (NULL == (buffer_address = t_qread(blk, &newdata->cycle, &newdata->cr)))
			return(rdfail_detail);
		newdata->first_tp_srch_status = first_tp_srch_status;
		assert(newdata->level == old->level);
		assert(newdata->blk_target == old->blk_target);
		newdata->blk_num = blk;
		newdata->buffaddr = buffer_address;
		newdata->prev_rec.match = 0;
		newdata->prev_rec.offset = 0;
		newdata->curr_rec.match = 0;
		newdata->curr_rec.offset = sizeof(blk_hdr);
		rp = (rec_hdr_ptr_t)(buffer_address + sizeof(blk_hdr));
	}
	return cdb_sc_normal;
}
