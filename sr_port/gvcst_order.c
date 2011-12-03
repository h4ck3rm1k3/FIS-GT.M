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

#include "gtm_string.h"

#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "cdb_sc.h"
#include "copy.h"
#include "filestruct.h"		/* needed for jnl.h */
#include "gdscc.h"		/* needed for tp.h */
#include "jnl.h"		/* needed for tp.h */
#include "gdskill.h"		/* needed for tp.h */
#include "hashtab.h"		/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"			/* needed for T_BEGIN_READ_NONTP_OR_TP macro */

#include "t_end.h"		/* prototypes */
#include "t_retry.h"
#include "t_begin.h"
#include "gvcst_rtsib.h"
#include "gvcst_search.h"
#include "gvcst_search_blk.h"
#include "gvcst_order.h"

GBLREF sgmnt_data_ptr_t	cs_data;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF gd_region	*gv_cur_region;
GBLREF gv_namehead	*gv_target;
GBLREF gv_key		*gv_currkey, *gv_altkey;
GBLREF int4		gv_keysize;
GBLREF short		dollar_tlevel;
GBLREF unsigned int	t_tries;

bool	gvcst_order(void)
{
	blk_hdr_ptr_t	bp;
	bool		found, two_histories;
	enum cdb_sc	status;
	rec_hdr_ptr_t	rp;
	unsigned short	rec_size;
	srch_blk_status	*bh;
	srch_hist	*rt_history;
	sm_uc_ptr_t	c1, c2, ctop, alt_top;

	T_BEGIN_READ_NONTP_OR_TP(ERR_GVORDERFAIL);
	for (;;)
	{
		assert(t_tries < CDB_STAGNATE || cs_addrs->now_crit);	/* we better hold crit in the final retry (TP & non-TP) */
		two_histories = FALSE;
		if (cdb_sc_normal == (status = gvcst_search(gv_currkey, NULL)))
		{
			found = TRUE;
			bh = gv_target->hist.h;
			rp = (rec_hdr_ptr_t)(bh->buffaddr + bh->curr_rec.offset);
			bp = (blk_hdr_ptr_t)bh->buffaddr;
			if ((rec_hdr_ptr_t)CST_TOB(bp) <= rp)
			{
				two_histories = TRUE;
				rt_history = gv_target->alt_hist;
				status = gvcst_rtsib(rt_history, 0);
				if (cdb_sc_normal == status)
				{
					bh = rt_history->h;
			       		if (cdb_sc_normal != (status = gvcst_search_blk(gv_currkey, bh)))
					{
						t_retry(status);
						continue;
					}
					rp = (rec_hdr_ptr_t)(bh->buffaddr + bh->curr_rec.offset);
					bp = (blk_hdr_ptr_t)bh->buffaddr;
				} else
				{
			  	     	if (cdb_sc_endtree == status)
					{
						found = FALSE;
						two_histories = FALSE;		/* second history not valid */
					} else
					{
						t_retry(status);
						continue;
					}
				}
			}
			if (found)
			{
				assert(gv_altkey->top == gv_currkey->top);
				assert(gv_altkey->top == gv_keysize);
				assert(gv_altkey->end < gv_altkey->top);
				/* store new subscipt */
				c1 = gv_altkey->base;
				alt_top = gv_altkey->base + gv_altkey->top - 1;
					/* Make alt_top one less than gv_altkey->top to allow double-null at end of a key-name */
				/* 4/17/96
				 * HP compiler bug work-around.  The original statement was
				 * c2 = (unsigned char *)CST_BOK(rp) + bh->curr_rec.match - rp->cmpc;
				 *
				 * ...but this was sometimes compiled incorrectly (the lower 4 bits
				 * of rp->cmpc, sign extended, were subtracted from bh->curr_rec.match).
				 * I separated out the subtraction of rp->cmpc.
				 *
				 * -VTF.
				 */
				c2 = (sm_uc_ptr_t)CST_BOK(rp) + bh->curr_rec.match;
				memcpy(c1, gv_currkey->base, bh->curr_rec.match);
				c1 += bh->curr_rec.match;
				c2 -= rp->cmpc;
				GET_USHORT(rec_size, &rp->rsiz);
				ctop = (sm_uc_ptr_t)rp + rec_size;
				for (;;)
				{
					if (c2 >= ctop  ||  c1 >= alt_top)
					{
						assert(CDB_STAGNATE > t_tries);
						status = cdb_sc_rmisalign;
						goto restart;	/* goto needed because of nested FOR loop */
					}
 					if (0 == (*c1++ = *c2++))
					{
						*c1 = 0;
						break;
					}
				}
				gv_altkey->end = c1 - gv_altkey->base;
				assert(gv_altkey->end < gv_altkey->top);
			}
                        if (0 == dollar_tlevel)
			{
				if (0 == t_end(&gv_target->hist, two_histories ? rt_history : NULL))
					continue;
			} else
			{
				status = tp_hist(two_histories ? rt_history : NULL);
				if (cdb_sc_normal != status)
				{
					t_retry(status);
					continue;
				}
			}
			if (cs_addrs->read_write)
				cs_data->n_order++;
			return (found  &&  (bh->curr_rec.match >= gv_currkey->prev));
		}
restart:	t_retry(status);
	}
}
