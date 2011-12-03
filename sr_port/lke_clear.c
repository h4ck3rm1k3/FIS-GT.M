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

/*
 * -------------------------------------------------
 * lke_clear.c : removes locks for qualified regions
 * used in     : lke.c
 * -------------------------------------------------
 */

#include "mdef.h"
#include "gtm_string.h"
#include "mlkdef.h"
#include "gdsroot.h"
#include "gdsblk.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "filestruct.h"
#include "util.h"
#include "gtcmtr_lke.h"
#include "lke.h"
#include "lke_getcli.h"
#include "lke_cleartree.h"
#include "gtmmsg.h"

#define NOFLUSH 0
#define FLUSH	1
#define RESET	2

GBLREF	gd_region	*gv_cur_region;
GBLREF	gd_addr		*gd_header;
GBLREF	sgmnt_addrs	*cs_addrs;
GBLREF	short		crash_count;

error_def(ERR_NOREGION);


void	lke_clear(void)
{
	bool		locks, all = TRUE, wait = FALSE, interactive = TRUE, match = FALSE, memory = FALSE, nocrit = FALSE;
	int4		pid;
	int		n;
	char		regbuf[MAX_RN_LEN], nodebuf[32], one_lockbuf[MAX_KEY_SZ];
	mlk_ctldata_ptr_t	ctl;
	mstr		reg, node, one_lock;

	error_def(ERR_UNIMPLOP);
	error_def(ERR_TEXT);

	/* Get all command parameters */
	reg.addr = regbuf;
	reg.len = sizeof(regbuf);
	node.addr = nodebuf;
	node.len = sizeof(nodebuf);
	one_lock.addr = one_lockbuf;
	one_lock.len = sizeof(one_lockbuf);

	if (lke_getcli(&all, &wait, &interactive, &pid, &reg, &node, &one_lock, &memory, &nocrit) == 0)
		return;

	/* Search all regions specified on the command line */
	for (gv_cur_region = gd_header->regions, n = 0;
	     n != gd_header->n_regions;
	     ++gv_cur_region, ++n)
	{
		/* If region matches and is open */
		if ((reg.len == 0  ||
		     gv_cur_region->rname_len == reg.len  &&  memcmp(gv_cur_region->rname, reg.addr, reg.len) == 0)  &&
		    gv_cur_region->open)
		{
			match = TRUE;
			util_out_print("!/!AD!/", NOFLUSH, REG_LEN_STR(gv_cur_region));

			/* If distributed database, the region is located on another node */
			if (gv_cur_region->dyn.addr->acc_meth == dba_cm)
			{
#if defined(LKE_WORKS_OK_WITH_CM)
				locks = gtcmtr_lke_clearreq(gv_cur_region->dyn.addr->cm_blk, gv_cur_region->cmx_regnum,
							    all, interactive, pid, &node);
#else
				gtm_putmsg(VARLSTCNT(10) ERR_UNIMPLOP, 0, ERR_TEXT, 2,
						LEN_AND_LIT("GT.CM region - locks must be cleared on the local node"),
						ERR_TEXT, 2, REG_LEN_STR(gv_cur_region));
				continue;
#endif
				/*** Used to be:
				util_out_print(NULL, RESET);
				util_out_print("!AD", NOFLUSH, REG_LEN_STR(gv_cur_region));
				util_out_print(" -  GT.CM region, locks must be cleared on the local node", FLUSH);
				locks = TRUE;
				***/
			} else  if (gv_cur_region->dyn.addr->acc_meth == dba_bg  ||
				    gv_cur_region->dyn.addr->acc_meth == dba_mm)
			{	/* Local region */
				cs_addrs = &FILE_INFO(gv_cur_region)->s_addrs;
				ctl = (mlk_ctldata_ptr_t)cs_addrs->lock_addrs[0];

				/* Prevent any modifications of locks while we are clearing */
				if (cs_addrs->critical != NULL)
					crash_count = cs_addrs->critical->crashcnt;
				grab_crit(gv_cur_region);

				locks = ctl->blkroot == 0 ? FALSE
							  : lke_cleartree(gv_cur_region, NULL, ctl,
									 (mlk_shrblk_ptr_t)R2A(ctl->blkroot),
									  all, interactive, pid, one_lock);

				rel_crit(gv_cur_region);
			} else
			{
				util_out_print(NULL, RESET);
				util_out_print("Region is not BG, MM, or CM", FLUSH);
				locks = TRUE;
			}

			if (!locks)
			{
				util_out_print(NULL, RESET);
				util_out_print("No locks were found in !AD", FLUSH, REG_LEN_STR(gv_cur_region));
			}
		}
	}

	if (!match  &&  reg.len != 0)
		rts_error(VARLSTCNT(4) ERR_NOREGION, 2, reg.len, reg.addr);

}
