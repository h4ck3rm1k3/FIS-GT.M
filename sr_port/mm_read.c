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
#include "gdsbt.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsfhead.h"
#include "mm_read.h"

GBLREF	sgmnt_addrs 	*cs_addrs;
GBLREF	unsigned char	rdfail_detail;
GBLREF	bool		run_time;

sm_uc_ptr_t mm_read(block_id blk)
{
	/* --- extended or dse (dse is able to edit any header fields freely) --- */
	assert((cs_addrs->total_blks <= cs_addrs->ti->total_blks) || (!run_time));
	assert(blk >= 0);

	if (blk < cs_addrs->total_blks) 		/* use the private copy of total_blks */
		return (cs_addrs->acc_meth.mm.base_addr + (off_t)cs_addrs->hdr->blk_size * blk);

	rdfail_detail = (blk < cs_addrs->ti->total_blks) ? cdb_sc_helpedout : cdb_sc_blknumerr;
	return (sm_uc_ptr_t)NULL;
}
