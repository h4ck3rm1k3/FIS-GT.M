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
#include "gdsblk.h"
#include "gdsfhead.h"
#include "error.h"
#include "util.h"

GBLREF	gv_key	*gv_currkey, *gv_altkey;
GBLREF	gd_addr	*gd_header;

CONDITION_HANDLER(gvcmy_open_ch)
{
	error_def(ERR_GTMCHECK);
	error_def(ERR_GTMASSERT);
	error_def(ERR_ASSERT);
	error_def(ERR_STACKOFLOW);
	error_def(ERR_OUTOFSPACE);

	START_CH;
	if (DUMPABLE)
	{ /* don't disturb state so that the core reflects the "bad" state */
		NEXTCH;
	}
	if (WARNING == SEVERITY || INFO == SEVERITY)
	{
		PRN_ERROR;
		CONTINUE;
	}
	assert(NULL != gd_header);
	assert(NULL != gv_currkey);
	assert(NULL != gv_altkey);
	gv_currkey->base[0] = gv_altkey->base[0] = '\0'; /* error opening remote db should reset gv_currkey and gv_altkey so
							  * that we drive gtcm_bind_name if the next global reference is the
							  * same as the current one */
	NEXTCH;
}
