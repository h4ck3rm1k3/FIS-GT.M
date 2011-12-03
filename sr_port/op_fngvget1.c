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
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "stringpool.h"
#include "op.h"
#include "gvcst_get.h"
#include "gvcmx.h"
#include "gvusr.h"
#include "sgnl.h"


GBLREF gv_namehead 	*gv_target;
GBLREF gd_region	*gv_cur_region;
GBLREF bool		gv_curr_subsc_null;

LITREF mval		literal_null;

int op_fngvget1(mval *v)
{
	bool gotit;

	if (gv_curr_subsc_null && gv_cur_region->null_subs == FALSE)
		sgnl_gvnulsubsc();

	switch (gv_cur_region->dyn.addr->acc_meth)
	{
		case dba_bg :
		case dba_mm :
				if (gv_target->root)
					gotit = gvcst_get(v);
				else
					gotit = FALSE;
				break;
		case dba_cm :
				gotit = gvcmx_get(v);
				break;
		default :
				gotit = gvusr_get(v);
				if (gotit)
					s2pool(&v->str);
				break;
	}

	if (!gotit)
		v->mvtype = 0;

	return gotit;
}
