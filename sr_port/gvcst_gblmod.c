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
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
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
#include "gvcst_gblmod.h"
#include "gvcst_search.h"

GBLREF gv_namehead	*gv_target;
GBLREF gv_key		*gv_currkey;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF gd_region	*gv_cur_region;
GBLREF short		dollar_tlevel;
GBLREF unsigned int	t_tries;

bool	gvcst_gblmod(mval *v)
{
	bool		gblmod;
	enum cdb_sc	status;
	int		key_size;

	T_BEGIN_READ_NONTP_OR_TP(ERR_GBLMODFAIL);
	assert(t_tries < CDB_STAGNATE || cs_addrs->now_crit);	/* we better hold crit in the final retry (TP & non-TP) */
	for (;;)
	{
		gblmod = TRUE;
		if (cdb_sc_normal == (status = gvcst_search(gv_currkey, NULL)))
		{
			if (cs_addrs->hdr->resync_tn >= ((blk_hdr_ptr_t)gv_target->hist.h[0].buffaddr)->tn)
				gblmod = FALSE;

			if (0 == dollar_tlevel)
			{
				if (0 == t_end(&gv_target->hist, NULL))
					continue;
			} else
			{
				status = tp_hist(NULL);
				if (cdb_sc_normal != status)
				{
					t_retry(status);
					continue;
				}
			}
			return gblmod;
		}
		t_retry(status);
	}
}
