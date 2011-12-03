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

#include "gtm_string.h"

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "min_max.h"		/* needed for init_root_gv.h */
#include "init_root_gv.h"
#include "util.h"
#include "cli.h"
#include "dse.h"

GBLREF block_id		patch_curr_blk;
GBLREF gd_region	*gv_cur_region;
GBLREF gd_addr		*gd_header;
GBLREF sgmnt_data_ptr_t	cs_data;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF short		crash_count;
GBLREF mval		dollar_zgbldir;
GBLREF gd_addr		*original_header;

void dse_f_reg(void)
{
	char rn[MAX_RN_LEN];
	unsigned short rnlen;
	int i;
	bool found;
	gd_region *ptr;
	gd_addr *temp_gdaddr;
	gd_binding *map;

	temp_gdaddr = gd_header;
	gd_header = original_header;
	rnlen = sizeof(rn);
	if (!cli_get_str("REGION",rn,&rnlen))
	{
		gd_header = temp_gdaddr;
		return;
	}
	if (rn[0] == '*' && rnlen == 1)
	{
		util_out_print("List of global directory:!_!AD!/",TRUE,dollar_zgbldir.str.len,dollar_zgbldir.str.addr);
		for (i=0, ptr = gd_header->regions; i < gd_header->n_regions ;i++, ptr++)
		{	util_out_print("!/File  !_!AD",TRUE, ptr->dyn.addr->fname_len,&ptr->dyn.addr->fname[0]);
			util_out_print("Region!_!AD",TRUE, REG_LEN_STR(ptr));
                 }
		gd_header = temp_gdaddr;
		 return;
	}
	assert (rn[0]);

		found = FALSE;
		for (i=0, ptr = gd_header->regions; i < gd_header->n_regions ;i++, ptr++)
			if (found = !memcmp(&ptr->rname[0],&rn[0],MAX_RN_LEN))
				break;
		if (!found)
		{
			util_out_print("Error:  region not found.",TRUE);
			gd_header = temp_gdaddr;
			return;
		}

	if (ptr == gv_cur_region)
	{
		util_out_print("Error:  already in region: !AD",TRUE,REG_LEN_STR(gv_cur_region));
		gd_header = temp_gdaddr;
		return;
	}
	if (ptr->dyn.addr->acc_meth == dba_cm)
	{
		util_out_print("Error:  Cannot edit an GT.CM database file.",TRUE);
		gd_header = temp_gdaddr;
		return;
	}
	if (ptr->dyn.addr->acc_meth == dba_usr)
	{
		util_out_print("Error:  Cannot edit an non-GTC format database file.",TRUE);
		gd_header = temp_gdaddr;
		return;
	}
	if (!ptr->open)
	{
		util_out_print("Error:  that region was not opened because it is not bound to any namespace.",TRUE);
		gd_header = temp_gdaddr;
		return;
	}

	if (cs_addrs->now_crit == TRUE)
		util_out_print("Warning:  now leaving region in critical section: !AD",TRUE, gv_cur_region->rname_len,
				gv_cur_region->rname);

	gv_cur_region = ptr;
	switch (gv_cur_region->dyn.addr->acc_meth)
	{
	case dba_mm:
	case dba_bg:
		cs_addrs = &FILE_INFO(gv_cur_region)->s_addrs;
		cs_data = cs_addrs->hdr;
		break;
	default:
		GTMASSERT;
	}

	if (cs_addrs && cs_addrs->critical)
	{		crash_count = cs_addrs->critical->crashcnt;
	}
	util_out_print("!/File  !_!AD",TRUE, DB_LEN_STR(gv_cur_region));
	util_out_print("Region!_!AD!/",TRUE, REG_LEN_STR(gv_cur_region));

	patch_curr_blk = get_dir_root();
	gv_init_reg(gv_cur_region);
	GET_SAVED_GDADDR(gd_header, temp_gdaddr, map, gv_cur_region);
	return;
}
