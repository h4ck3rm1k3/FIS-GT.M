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

#include "hashdef.h"
#include "lv_val.h"
#include "mv_stent.h"
#include "compiler.h"
#include "underr.h"

#include <varargs.h>

GBLREF mv_stent		*mv_chain;
GBLREF unsigned char	*msp, *stackbase, *stackwarn, *stacktop;
GBLREF symval		*curr_symval;

void push_parm(va_alist)
va_dcl
{
	va_list		var;
	int		truth_value;
	mval		*ret_value;
	int		mask;
	unsigned	totalcnt, actualcnt;
	parm_blk	*parm;
	mv_stent	*mvp_blk;
	int		i;
	lv_val		*actp;
	error_def	(ERR_STACKOFLOW);
	error_def	(ERR_STACKCRIT);

	VAR_START(var);
	totalcnt = va_arg(var, unsigned int);
	assert(4 <= totalcnt);
	truth_value = va_arg(var, int);
	ret_value = va_arg(var, mval *);
	mask = va_arg(var, int);
	actualcnt = va_arg(var, unsigned int);
	assert(4 + actualcnt == totalcnt);
	assert(MAX_ACTUALS >= actualcnt);
	PUSH_MV_STENT(MVST_PARM);
	parm = (parm_blk *)malloc(sizeof(parm_blk) - sizeof(lv_val*) + actualcnt * sizeof(lv_val*));
	parm->actualcnt = actualcnt;
	parm->mask = mask;
	mvp_blk = mv_chain;
	mvp_blk->mv_st_cont.mvs_parm.save_truth = truth_value;
	mvp_blk->mv_st_cont.mvs_parm.ret_value = (mval *)0;
	mvp_blk->mv_st_cont.mvs_parm.mvs_parmlist = parm;
	for (i = 0;  i < actualcnt;  i++)
	{
		actp = va_arg(var, lv_val *);
		if (!(mask & 1 << i))
		{
			if ((!MV_DEFINED(&actp->v)) && (actp->v.str.addr != (char *)&actp->v))
				underr(&actp->v);
			PUSH_MV_STENT(MVST_PVAL);
			mv_chain->mv_st_cont.mvs_pval.mvs_val = lv_getslot(curr_symval);
			mv_chain->mv_st_cont.mvs_pval.mvs_val->v = actp->v;		/* Copy mval input */
			mv_chain->mv_st_cont.mvs_pval.mvs_val->tp_var = NULL;		/* Fill out rest of new lv_val */
			mv_chain->mv_st_cont.mvs_pval.mvs_val->ptrs.val_ent.children = 0;
			mv_chain->mv_st_cont.mvs_pval.mvs_val->ptrs.val_ent.parent.sym = curr_symval;
			mv_chain->mv_st_cont.mvs_pval.mvs_ptab.nam_addr = 0;
			parm->actuallist[i] = (lv_val *)&mv_chain->mv_st_cont.mvs_pval;
		}
		else
			parm->actuallist[i] = actp;
	}
	mvp_blk->mv_st_cont.mvs_parm.ret_value = ret_value;
	return;
}
