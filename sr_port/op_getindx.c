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

#include <varargs.h>

#include "hashdef.h"
#include "lv_val.h"
#include "sbs_blk.h"
#include "collseq.h"
#include "stringpool.h"
#include "do_xform.h"
#include "undx.h"
#include "mvalconv.h"

#define IS_INTEGER 0

GBLREF collseq		*local_collseq;
GBLREF bool		undef_inhibit;
LITREF mval		literal_null ;

lv_val	*op_getindx(va_alist)
va_dcl
{
	mval			tmp_sbs;
	int			cur_subscr;
	int                     length;
	error_def(ERR_UNDEF);
	va_list			var, keyptr;
	int			argcnt;
	lv_val			*start;
	int4			temp;
	lv_sbs_tbl     		*tbl;
	lv_val	       		*lv;
       	sbs_search_status      	status;
	mval			*key;
	int			arg1;

	VAR_START(var);
	argcnt = va_arg(var, int4);
	start = va_arg(var, lv_val *);

	if (local_collseq)
	{
		tmp_sbs.mvtype = MV_STR;
	}

	lv = start;
	arg1 = --argcnt;
	VAR_COPY(keyptr, var);
	cur_subscr = 1;
	while (lv  &&  argcnt-- > 0)
	{
		cur_subscr++;
		key = va_arg(var, mval *);
	       	if ((tbl = lv->ptrs.val_ent.children) == 0)
			lv = 0;
		else
		{
			assert(tbl->ident == MV_SBS);
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
			{
				MV_FORCE_NUM(key);
				if (tbl->int_flag)
				{
					assert(tbl->num);
					if (MV_IS_INT(key))
					{
						temp = MV_FORCE_INT(key) ;
						lv = ( temp >= 0 && temp < SBS_NUM_INT_ELE ? tbl->num->ptr.lv[temp] : 0 ) ;
					}
					else
					{
						lv = 0;
					}
			 	}
			 	else
			       	{
					lv = (tbl->num) ? lv_get_num_inx(tbl->num, key, &status) : 0 ;
			 	}
			}
		}
	}
	if (!lv  ||  !MV_DEFINED(&lv->v))
	{
		if (undef_inhibit)
			lv = (lv_val *)&literal_null;
		else
		{
			unsigned char	buff[512], *end;

			end = undx(start, keyptr, arg1, buff, sizeof(buff));
			rts_error(VARLSTCNT(4) ERR_UNDEF, 2, end - buff, buff);
		}
	}
	return lv;
}
