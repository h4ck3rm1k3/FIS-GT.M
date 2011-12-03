/****************************************************************
 *                                                              *
 *      Copyright 2001, 2002 Sanchez Computer Associates, Inc.  *
 *                                                              *
 *      This source code contains the intellectual property     *
 *      of its copyright holder(s), and is made available       *
 *      under a license.  If you do not know the terms of       *
 *      the license, please stop and do not read further.       *
 *                                                              *
 ****************************************************************/

#include "mdef.h"

#include "gtm_string.h"
#include "gtm_limits.h"

#include "iosp.h"
#include "trans_log_name.h"
#include "gtm_logicals.h"
#include "error.h"
#include "gtm_env_xlate_init.h"
#include "stringpool.h"

void gtm_env_xlate_init(void)
{
	uint4		status;
	mstr		val, tn;
	char		buf[PATH_MAX];

	error_def(ERR_TRNLOGFAIL);

	val.addr = ZGTMENVXLATE;
	val.len =  STR_LIT_LEN(ZGTMENVXLATE);
	if (SS_NORMAL == (status = trans_log_name(&val, &tn, buf)))
	{
		UNIX_ONLY(
			env_gtm_env_xlate.len = tn.len;
			env_gtm_env_xlate.addr = (char *)malloc(tn.len);
			memcpy(env_gtm_env_xlate.addr, buf, tn.len);
			)
		VMS_ONLY(
			/* In op_gvextnam, the logical name is used in VMS,
			 * rather than its value (by lib$find_image_symbol),
			 * so only whether the logical name translates is
			 * checked here.
			 */
			env_gtm_env_xlate.len = val.len;
			env_gtm_env_xlate.addr = val.addr;
			)
	}
	else if  (SS_NOLOGNAM == status)
		env_gtm_env_xlate.len = 0;
	else
		rts_error(VARLSTCNT(5) ERR_TRNLOGFAIL, 2, LEN_AND_LIT(ZGTMENVXLATE), status);

	return;
}
