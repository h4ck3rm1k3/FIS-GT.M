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

#include "gtm_string.h"

#include "gtm_stdio.h"
#include "util.h"
#include "cli.h"
#include "gdsroot.h"
#include "gt_timer.h"

#if defined(UNIX)
#define GET_CONFIRM(X,Y) {PRINTF("CONFIRMATION: ");FGETS((X), (Y), stdin, fgets_res);Y = strlen(X);}

#include <sys/ipc.h>


GBLREF uint4	user_id;
#elif defined(VMS)
#define GET_CONFIRM(X,Y) {if(!cli_get_str("CONFIRMATION",(X),&(Y))) {rts_error(VARLSTCNT(1) ERR_DSEWCINITCON); \
	return;}}
#else
#error UNSUPPORTED PLATFORM
#endif

#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "jnl.h"
#include "gdskill.h"
#include "gdscc.h"
#include "hashtab.h"		/* needed for tp.h */
#include "buddy_list.h"		/* needed for tp.h */
#include "tp.h"
#include "min_max.h"		/* needed for init_root_gv.h */
#include "init_root_gv.h"
#include "dse.h"

#ifdef UNIX
#include "mutex.h"
#endif
#include "wcs_flu.h"

GBLREF block_id		patch_curr_blk;
GBLREF gd_addr		*gd_header;
GBLREF gd_region	*gv_cur_region;
GBLREF sgmnt_addrs	*cs_addrs;
GBLREF short		crash_count;
GBLREF uint4		process_id;
GBLREF gd_addr          *original_header;

void dse_all(void)
{
	gd_region	*ptr;
	tp_region	*region_list, *rg, *rg_last, *rg_new;	/* A handy structure for maintaining a list of regions */
	int		i, j;
	sgmnt_addrs	*old_addrs, *csa;
	gd_region	*old_region;
	block_id	old_block;
	int4		stat;
	char		confirm[256];
	unsigned short	len;
	bool		ref=FALSE;
	bool		crit=FALSE;
	bool		wc=FALSE;
	bool		flush=FALSE;
	bool		freeze=FALSE;
	bool		nofreeze=FALSE;
	bool		override=FALSE;
	bool		seize=FALSE;
	bool		release=FALSE;
	gd_addr         *temp_gdaddr;
	gd_binding      *map;
#ifdef UNIX
	char		*fgets_res;
#endif

	error_def(ERR_DSEWCINITCON);
	error_def(ERR_FREEZE);
        error_def(ERR_DBRDONLY);

        if (gv_cur_region->read_only)
                rts_error(VARLSTCNT(4) ERR_DBRDONLY, 2, DB_LEN_STR(gv_cur_region));

	if (cli_present("RENEW") == CLI_PRESENT)
	{
		crit = ref = wc = nofreeze = TRUE;
		len = sizeof(confirm);
		GET_CONFIRM(confirm,len);
		if (confirm[0] != 'Y' && confirm[0] != 'y')
		{
			rts_error(VARLSTCNT(1) ERR_DSEWCINITCON);
			return;
		}
	} else
	{
		if (cli_present("CRITINIT") == CLI_PRESENT)
			crit = TRUE;
		if (cli_present("REFERENCE") == CLI_PRESENT)
			ref = TRUE;
		if (cli_present("WCINIT") == CLI_PRESENT)
		{
			wc = TRUE;
			len = sizeof(confirm);
			GET_CONFIRM(confirm,len);
			if (confirm[0] != 'Y' && confirm[0] != 'y')
			{
				rts_error(VARLSTCNT(1) ERR_DSEWCINITCON);
				return;
			}
		}
		if (cli_present("BUFFER_FLUSH") == CLI_PRESENT)
			flush = TRUE;
		if (cli_present("SEIZE") == CLI_PRESENT)
			seize = TRUE;
		if (cli_present("RELEASE") == CLI_PRESENT)
			release = TRUE;
		stat = cli_present("FREEZE");
		if (stat == CLI_NEGATED)
			nofreeze = TRUE;
		else if (stat == CLI_PRESENT)
		{
			freeze = TRUE;
			nofreeze = FALSE;
		}
                if (cli_present("OVERRIDE") == CLI_PRESENT)
                        override = TRUE;
	}
	old_addrs = cs_addrs;
	old_region = gv_cur_region;
	old_block = patch_curr_blk;
	temp_gdaddr = gd_header;
	gd_header = original_header;
	region_list = NULL;
	for (i = 0, ptr = gd_header->regions; i < gd_header->n_regions; i++, ptr++)
	{
		if (ptr->dyn.addr->acc_meth != dba_bg && ptr->dyn.addr->acc_meth != dba_mm)
		{
			util_out_print("Skipping region !AD: not BG or MM access",TRUE,ptr->rname_len,&ptr->rname[0]);
			continue;
		}
		if (!ptr->open)
		{
			util_out_print("Skipping region !AD as it is not bound to any namespace.", TRUE,
				ptr->rname_len, &ptr->rname[0]);
			continue;
		}
		/* put on region list in order of ftok value so processed in same order that
		   crits are obtained */
		csa = &FILE_INFO(ptr)->s_addrs;
		insert_region(ptr, &(region_list), NULL, sizeof(tp_region));
	}
	/* Now run the list of regions in the sorted ftok order to execute the desired commands */
	for (rg = region_list; NULL != rg; rg = rg->fPtr)
	{
		gv_cur_region = rg->reg;
		switch(gv_cur_region->dyn.addr->acc_meth)
		{
		case dba_mm:
		case dba_bg:
			cs_addrs = &FILE_INFO(gv_cur_region)->s_addrs;
			break;
		default:
			GTMASSERT;
		}
		patch_curr_blk = get_dir_root();

		if (crit)
		{
			UNIX_ONLY(gtm_mutex_init(gv_cur_region, NUM_CRIT_ENTRY, TRUE);)
			VMS_ONLY(mutex_init(cs_addrs->critical, NUM_CRIT_ENTRY, TRUE);)
			cs_addrs->nl->in_crit = 0;
			cs_addrs->now_crit = cs_addrs->read_lock = FALSE;
		}
		if (cs_addrs->critical)
			crash_count = cs_addrs->critical->crashcnt;

		if (freeze)
		{
		        while (FALSE == region_freeze(gv_cur_region, TRUE, override))
                		hiber_start(1000);
		}
		if (seize)
                        grab_crit(gv_cur_region);
		if (wc)
		{
			if (FALSE == seize)
				grab_crit(gv_cur_region);
			bt_init(cs_addrs);
			if (cs_addrs->hdr->acc_meth == dba_bg)
			{
				bt_refresh(cs_addrs);
				db_csh_ini(cs_addrs);
				db_csh_ref(cs_addrs);
			}
			if((FALSE == seize) || (TRUE == release))
				rel_crit(gv_cur_region);
		}
		if (flush)
			wcs_flu(WCSFLU_FLUSH_HDR | WCSFLU_WRITE_EPOCH | WCSFLU_SYNC_EPOCH);
		if (release && cs_addrs->now_crit)
			rel_crit(gv_cur_region);
                else if (release && !cs_addrs->now_crit)
		{
                        util_out_print("Current process does not own the Region: !AD.",TRUE, REG_LEN_STR(gv_cur_region));
		}
		if (nofreeze)
		{
			if (!region_freeze(gv_cur_region,FALSE, override))
				util_out_print("Region: !AD is frozen by another user, not releasing freeze",TRUE,
					REG_LEN_STR(gv_cur_region));
		}
		if (ref)
			cs_addrs->nl->ref_cnt = 1;
	}
	cs_addrs = old_addrs;
	gv_cur_region = old_region;
	patch_curr_blk = old_block;
	GET_SAVED_GDADDR(gd_header, temp_gdaddr, map, gv_cur_region);
	return;
}
