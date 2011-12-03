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
#include "gvcst_expand_key.h"
#include "gvcst_lftsib.h"
#include "gvcst_search.h"
#include "gvcst_search_blk.h"
#include "gvcst_zprevious.h"

GBLREF sgmnt_data_ptr_t	cs_data;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF gd_region	*gv_cur_region;
GBLREF gv_namehead	*gv_target;
GBLREF gv_key		*gv_currkey, *gv_altkey;
GBLREF int4		gv_keysize;
GBLREF short		dollar_tlevel;
GBLREF int4		gv_keysize;
GBLREF unsigned int	t_tries;

bool gvcst_zprevious(void)
{
	static gv_key	*temp_key;
	blk_hdr_ptr_t	bp;
	bool		found, two_histories;
	enum cdb_sc	status;
	rec_hdr_ptr_t	rp;
	unsigned char	*c1, *c2, *ctop;
	srch_blk_status	*bh;
	srch_hist	*lft_history;

	T_BEGIN_READ_NONTP_OR_TP(ERR_GVORDERFAIL);
	for (;;)
	{
		assert(t_tries < CDB_STAGNATE || cs_addrs->now_crit);	/* we better hold crit in the final retry (TP & non-TP) */
		two_histories = FALSE;
		if (cdb_sc_normal == (status = gvcst_search(gv_currkey, NULL)))
		{
			found = TRUE;
			bh = gv_target->hist.h;
			if (0 == bh->prev_rec.offset)
			{
				two_histories = TRUE;
				lft_history = gv_target->alt_hist;
				status = gvcst_lftsib(lft_history);
				if (cdb_sc_normal == status)
				{
					bh = lft_history->h;
					if (cdb_sc_normal != (status = gvcst_search_blk(gv_currkey, bh)))
					{
						t_retry(status);
						continue;
					}
				} else  if (cdb_sc_endtree == status)
				{
					found = FALSE;
					two_histories = FALSE;		/* second history not valid */
				} else
				{
					t_retry(status);
					continue;
				}
			}
			if (found)
			{	/* store new subscipt */
				assert(gv_altkey->top == gv_currkey->top);
				assert(gv_altkey->top == gv_keysize);
				assert(gv_currkey->end < gv_currkey->top);
				rp = (rec_hdr_ptr_t)(bh->buffaddr + bh->prev_rec.offset);
				bp = (blk_hdr_ptr_t)bh->buffaddr;
				c1 = gv_altkey->base;
				memcpy(c1, gv_currkey->base, bh->prev_rec.match);
				c1 += bh->prev_rec.match;
				if (NULL == temp_key)
					temp_key = (gv_key *)malloc(sizeof(gv_key) + gv_keysize);
				temp_key->top = gv_currkey->top;
				if (cdb_sc_normal != (status = gvcst_expand_key((blk_hdr_ptr_t)bh->buffaddr, bh->prev_rec.offset,
									temp_key)))
				{
					t_retry(status);
					continue;
				}
				if ((temp_key->end < gv_currkey->end) && (temp_key->end <= gv_currkey->prev))
					found = FALSE;
				else
				{
					c2 = temp_key->base + bh->prev_rec.match;
					ctop = temp_key->base + temp_key->end;
					for (;;)
					{
						if (c2 >= ctop)
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
				}
				gv_altkey->end = c1 - gv_altkey->base;
				assert(gv_altkey->end < gv_altkey->top);
			}
			if (0 == dollar_tlevel)
			{
				if (0 == t_end(&gv_target->hist, two_histories ? lft_history : NULL))
					continue;
			} else
			{
				status = tp_hist(two_histories ? lft_history : NULL);
				if (cdb_sc_normal != status)
				{
					t_retry(status);
					continue;
				}
			}
			if (cs_addrs->read_write)
				cs_data->n_zprevs++;
			return (found && (bh->prev_rec.match >= gv_currkey->prev));
		}
restart:	t_retry(status);
	}
}
