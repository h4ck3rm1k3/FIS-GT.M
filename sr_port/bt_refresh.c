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
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "longset.h"
#include "relqop.h"

void bt_refresh(sgmnt_addrs *csa)
{
	sgmnt_data_ptr_t	csd;
	bt_rec_ptr_t		ptr, top, bt1;

	error_def(ERR_BTFAIL);

	csd = csa->hdr;
	assert(dba_bg == csd->acc_meth);
	longset((uchar_ptr_t)csa->bt_header, (csd->bt_buckets + csd->n_bts + 1) * sizeof(bt_rec), 0);

	for (ptr = csa->bt_header, top = ptr + csd->bt_buckets + 1; ptr < top; ptr++)
		ptr->blk = BT_QUEHEAD;

	for (ptr = csa->bt_base, bt1 = csa->bt_header, top = ptr + csd->n_bts; ptr < top ; ptr++, bt1++)
	{
		ptr->blk = BT_NOTVALID;
		ptr->cache_index = CR_NOTVALID;
		insqt((que_ent_ptr_t)ptr, (que_ent_ptr_t)bt1);
		insqt((que_ent_ptr_t)((sm_uc_ptr_t)ptr + (2 * sizeof(int4))), (que_ent_ptr_t)csa->th_base);
	}
	((th_rec *)((uchar_ptr_t)csa->th_base + csa->th_base->tnque.fl))->tn = csa->ti->curr_tn - 1;
	csa->ti->mm_tn = 0;
	return;
}
