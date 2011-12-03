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
#include "zwrite.h"
#include "op.h"
#include "outofband.h"
#include "numcmp.h"
#include "patcode.h"
#include "sgnl.h"
#include "mvalconv.h"
#include "follow.h"
#include "gtm_string.h"

#define eb_less(u, v)	(numcmp(u, v) < 0)

GBLREF gv_key		*gv_currkey;
GBLREF gvzwrite_struct  gvzwrite_block;
GBLREF int4		outofband;
GBLREF bool		gv_curr_subsc_null;
GBLREF gd_region	*gv_cur_region;
LITREF mval		literal_null;

void gvzwr_var(uint4 data, int4 n)
{
	mval		mv, subdata;
	unsigned short  end, prev, end1, prev1;
	bool		save_gv_curr_subsc_null;
	bool		do_lev;
	char		seen_null;
	zwr_sub_lst	*zwr_sub;
	int		loop_condition = 1;

	if (outofband)
		outofband_action(FALSE);
	zwr_sub = (zwr_sub_lst *)gvzwrite_block.sub;
	if ((0 == gvzwrite_block.subsc_count) && (0 == n))
		zwr_sub->subsc_list[n].subsc_type = ZWRITE_ASTERISK;
	if ((1 == data || 11 == data) &&
		(!gvzwrite_block.subsc_count || (ZWRITE_ASTERISK == zwr_sub->subsc_list[n].subsc_type) ||
		(n && !(gvzwrite_block.mask >> n))))
			gvzwr_out();
	if (data <= 1 || (gvzwrite_block.subsc_count && (n >= gvzwrite_block.subsc_count)
		&& (ZWRITE_ASTERISK != zwr_sub->subsc_list[gvzwrite_block.subsc_count - 1].subsc_type)))
		return;

	assert(data > 1);
	end = gv_currkey->end;
	prev = gv_currkey->prev;
	if (n < gvzwrite_block.subsc_count && (ZWRITE_VAL == zwr_sub->subsc_list[n].subsc_type))
	{
		mval2subsc(zwr_sub->subsc_list[n].first, gv_currkey);
		op_gvdata(&subdata);
		if (MV_FORCE_INT(&subdata) && ((10 != (int4)MV_FORCE_INT(&subdata)) || n < gvzwrite_block.subsc_count - 1))
		{
			save_gv_curr_subsc_null = gv_curr_subsc_null;
			gvzwr_var((int4)MV_FORCE_INT(&subdata), n + 1);
			gv_curr_subsc_null = save_gv_curr_subsc_null;
		} else if (gvzwrite_block.fixed)
			sgnl_gvundef();
	} else
	{
		seen_null = 0;
		if (n < gvzwrite_block.subsc_count
			&& zwr_sub->subsc_list[n].first
			&& ZWRITE_PATTERN != zwr_sub->subsc_list[n].subsc_type)
		{
			mv = *zwr_sub->subsc_list[n].first;
			mval2subsc(&mv, gv_currkey);
			if ((mv.mvtype & MV_STR) && !mv.str.len)
				seen_null = 1;
			op_gvdata(&subdata);
		} else
		{
			mval2subsc((mval *)&literal_null, gv_currkey);
			gv_curr_subsc_null = TRUE;
			op_gvorder(&mv);
			if (0 == mv.str.len)
			{
				if (!gv_cur_region->null_subs || seen_null)
					loop_condition = 0;
				else
				{
					seen_null = 1;			/* set flag to indicate processing null sub */
					op_gvnaked(VARLSTCNT(1) &mv);
					op_gvdata(&subdata);
					if (!MV_FORCE_INT(&subdata))
						loop_condition = 0;
				}
			} else
			{
				op_gvnaked(VARLSTCNT(1) &mv);
				op_gvdata(&subdata);
			}
		}
		while (loop_condition)
		{
			do_lev = (MV_FORCE_INT(&subdata) ? TRUE : FALSE);
			if (n < gvzwrite_block.subsc_count)
			{
				if (ZWRITE_PATTERN == zwr_sub->subsc_list[n].subsc_type)
				{
					if (!do_pattern(&mv, zwr_sub->subsc_list[n].first))
						do_lev = FALSE;
				} else if (ZWRITE_ALL != zwr_sub->subsc_list[n].subsc_type)
				{
					if (do_lev && zwr_sub->subsc_list[n].first)
					{
						if (MV_IS_CANONICAL(&mv))
						{
							if (!MV_IS_CANONICAL(zwr_sub->subsc_list[n].first)
							    || eb_less(&mv, zwr_sub->subsc_list[n].first))
								do_lev = FALSE;
						} else
						{
							if (!MV_IS_CANONICAL(zwr_sub->subsc_list[n].first)
								&& (!follow(&mv, zwr_sub->subsc_list[n].first) &&
								(mv.str.len != zwr_sub->subsc_list[n].first->str.len ||
								  memcmp(mv.str.addr,
									zwr_sub->subsc_list[n].first->str.addr,
									mv.str.len))))
								do_lev = FALSE;
						}
					}

					if (do_lev && zwr_sub->subsc_list[n].second)
					{
						if (MV_IS_CANONICAL(&mv))
						{
							if (MV_IS_CANONICAL(zwr_sub->subsc_list[n].second)
							    && eb_less(zwr_sub->subsc_list[n].second, &mv))
								do_lev = FALSE;
						} else
						{
							if (MV_IS_CANONICAL(zwr_sub->subsc_list[n].second)
							    ||	(!follow(zwr_sub->subsc_list[n].second, &mv) &&
								(mv.str.len != zwr_sub->subsc_list[n].second->str.len ||
								  memcmp(mv.str.addr,
									zwr_sub->subsc_list[n].second->str.addr,
									mv.str.len))))
								do_lev = FALSE;
						}
						if (!do_lev)
							break;
					}
				}
			}
			if (do_lev)
			{
				end1 = gv_currkey->end;
				prev1 = gv_currkey->prev;
                                save_gv_curr_subsc_null = gv_curr_subsc_null;
				gvzwr_var((int4)MV_FORCE_INT(&subdata), n + 1);
                                gv_curr_subsc_null = save_gv_curr_subsc_null;
				gv_currkey->end = end1;
				gv_currkey->prev = prev1;
				gv_currkey->base[end1] = 0;
			}
			if (1 == seen_null)
			{
				assert(gv_curr_subsc_null);
				gv_curr_subsc_null = FALSE;
				seen_null = 2;				/* set flag to indicate null sub processed */
			}
			op_gvorder(&mv);
			if (0 == mv.str.len)
			{
				if (!gv_cur_region->null_subs || seen_null)
					break;
				else
				{
					seen_null = 1;			/* set flag to indicate processing null sub */
					op_gvnaked(VARLSTCNT(1) &mv);
					op_gvdata(&subdata);
					if (!MV_FORCE_INT(&subdata))
						break;
				}
			} else
			{
				op_gvnaked(VARLSTCNT(1) &mv);
				op_gvdata(&subdata);
			}
		}
	}
	gv_currkey->end = end;
	gv_currkey->prev = prev;
	gv_currkey->base[end] = 0;
}
