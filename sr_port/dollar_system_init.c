/****************************************************************
 *								*
 *	Copyright 2002 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "startup.h"
#include "dollar_system_init.h"
#include "gtm_logicals.h"
#include "io.h"
#include "iosp.h"
#include "stringpool.h"
#include "trans_log_name.h"

GBLREF	mval	dollar_system;
GBLREF spdesc	stringpool;

void dollar_system_init(struct startup_vector *svec)
{
	uint4		status;
	mstr		val, tn;
	char		buf[MAX_TRANS_NAME_LEN];
	error_def(ERR_TRNLOGFAIL);

	dollar_system.mvtype = MV_STR;
	dollar_system.str.addr = (char *)stringpool.free;
	dollar_system.str.len = STR_LIT_LEN("47,");
	memcpy(stringpool.free, "47,", dollar_system.str.len);
	stringpool.free += dollar_system.str.len;
	val.addr = SYSID;
	val.len = STR_LIT_LEN(SYSID);
	if (SS_NORMAL == (status = trans_log_name(&val, &tn, buf)))
	{
		dollar_system.str.len += tn.len;
		memcpy(stringpool.free, tn.addr, tn.len);
		stringpool.free += tn.len;
	} else if (SS_NOLOGNAM == status)
	{
		dollar_system.str.len += svec->sysid_ptr->len;
		memcpy(stringpool.free, svec->sysid_ptr->addr, svec->sysid_ptr->len);
		stringpool.free += dollar_system.str.len;
	} else
		rts_error(VARLSTCNT(5) ERR_TRNLOGFAIL, 2, LEN_AND_LIT(SYSID), status);
	assert(stringpool.free < stringpool.top);	/* it's process initialization after all */
	return;
}
