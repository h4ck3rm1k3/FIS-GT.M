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
#include "hashdef.h"
#include "lv_val.h"
#include "sbs_blk.h"
#include <varargs.h>
#include "collseq.h"
#include "stringpool.h"
#include "do_xform.h"
#include "mvalconv.h"

GBLREF collseq		*local_collseq;

#define IS_INTEGER 0

lv_val	*op_srchindx(va_alist)
va_dcl
{
	int			cur_subscr;
	int                     length;
	mval                    tmp_sbs;
	va_list			var;
	int			argcnt;
	int4			temp;
	lv_sbs_tbl     		*tbl;
	lv_val	       		*lv;
       	sbs_search_status      	status;
	mval			*key;

	VAR_START(var);
	argcnt = va_arg(var, int4);
	lv = va_arg(var, lv_val *);

	cur_subscr = 0;
	while (lv && --argcnt > 0)
	{
		cur_subscr++;
		key = va_arg(var, mval *);
	       	if ((tbl = lv->ptrs.val_ent.children) == 0)
			lv = 0;
		else
		{
			assert(tbl->ident == MV_SBS);
			if (MV_IS_CANONICAL(key))
				MV_FORCE_NUM(key);
			if (!MV_IS_CANONICAL(key))
			{
				if (local_collseq)
				{
					ALLOC_XFORM_BUFF(&key->str);
					tmp_sbs.mvtype = MV_STR;
					tmp_sbs.str.len = max_lcl_coll_xform_bufsiz;
					assert(NULL != lcl_coll_xform_buff);
					tmp_sbs.str.addr = lcl_coll_xform_buff;
					do_xform(local_collseq, XFORM, &key->str, &tmp_sbs.str, &length);
					tmp_sbs.str.len = length;
					s2pool(&(tmp_sbs.str));
					key = &tmp_sbs;
				}
				lv = (tbl->str) ? lv_get_str_inx(tbl->str, &key->str, &status) : 0;
			}
			else
			{	if (tbl->int_flag)
				{	assert(tbl->num);
					if (MV_IS_INT(key))
					{
						temp = MV_FORCE_INT(key);
						if (temp >= 0 && temp < SBS_NUM_INT_ELE)
							lv = tbl->num->ptr.lv[temp];
						else
							lv = 0;
					} else
					{
						lv = 0;
					}
			 	}
			 	else
			       	{
					lv = (tbl->num) ? lv_get_num_inx(tbl->num, key, &status) : 0;
			 	}
			}
		}
	}
	return lv;
}
