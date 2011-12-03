/****************************************************************
 *								*
 *	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	*
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
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "op.h"
#include "sgnl.h"
#include "gvcst_queryget.h"
#include "gvcmx.h"
#include "stringpool.h"
#include "gvusr_queryget.h"

GBLREF gv_namehead	*gv_target;
GBLREF gd_region	*gv_cur_region;
GBLREF bool		gv_curr_subsc_null;
GBLREF gv_key		*gv_currkey, *gv_altkey;
LITREF mval		literal_null;

boolean_t op_gvqueryget(mval *key, mval *val)
{
	boolean_t 	gotit;
	gv_key		*save_key;

	if (gv_curr_subsc_null && gv_cur_region->null_subs == FALSE)
		sgnl_gvnulsubsc();
	switch (gv_cur_region->dyn.addr->acc_meth)
	{
	case dba_bg:
	case dba_mm:
		gotit = (0 != gv_target->root) ? gvcst_queryget(val) : FALSE;
		break;
	case dba_cm:
		gotit = gvcmx_query(val);
		break;
	case dba_usr:
		save_key = gv_currkey;
		gv_currkey = gv_altkey;
		/* We rely on the fact that *gv_altkey area is not modified by gvusr_queryget, and don't change gv_altkey.
		 * If and when *gv_altkey area is modified by gvusr_queryget, we have to set up a spare key area
		 * (apart from gv_altkey and gv_currkey), and make gv_altkey point the spare area before calling gvusr_queryget */
		memcpy(gv_currkey, save_key, sizeof(*save_key) + save_key->end);
		gotit = gvusr_queryget(val);
		gv_altkey = gv_currkey;
		gv_currkey = save_key;
		break;
	default:
		GTMASSERT;
	}
	if (gotit)
	{
		key->mvtype = MV_STR;
		key->str.addr = (char *)gv_altkey->base;
		key->str.len = gv_altkey->end + 1;
		s2pool(&key->str);
	} else
	{
		*key = literal_null;
		*val = literal_null;
	}
	return gotit;
}
