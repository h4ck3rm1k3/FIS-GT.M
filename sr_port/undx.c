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

#include <varargs.h>

#include "hashdef.h"
#include "lv_val.h"
#include "sbs_blk.h"
#include "subscript.h"
#include "lvname_info.h"
#include "undx.h"

/* pkeys MUST be a va_list initialized in the caller via a va_start */
unsigned char	*undx (lv_val *start, va_list pkeys, int cnt, unsigned char *buff, unsigned short size)
{
	static lvname_info_ptr  lvn_info = NULL;
	int			cur_subscr;

	if (!lvn_info)
		lvn_info = (lvname_info_ptr) malloc(sizeof(struct lvname_info_struct));
	lvn_info->total_lv_subs = cnt + 1;
	lvn_info->start_lvp = start;
	for (cur_subscr = 0;  cur_subscr < cnt;  cur_subscr++)
	{
		lvn_info->lv_subs[cur_subscr] = va_arg(pkeys, mval *);
		MV_FORCE_STR(lvn_info->lv_subs[cur_subscr]);
	}
	return(format_key_mvals(buff, size, lvn_info));
}
