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
#include "io.h"
#include "iosp.h"
#include "io_params.h"
#include "cryptdef.h"
#include "op.h"
#include "trans_log_name.h"


GBLREF bool		licensed;
GBLREF int4		lkid, lid;
GBLREF io_log_name	*io_root_log_name;

int op_open(mval *device, mval *devparms, int timeout, mval *mspace)
{
	LITREF unsigned char io_params_size[];
	char		buf1[MAX_TRANS_NAME_LEN];	/* buffer to hold translated name */
	io_log_name	*naml;				/* logical record for passed name */
	io_log_name	*tl;				/* logical record for translated name */
	io_log_name	*prev;				/* logical record for removal search */
	uint4		stat;				/* status */
	mstr		tn;				/* translated name */

	error_def(LP_NOTACQ);				/* bad license */

	MV_FORCE_STR(device);
	MV_FORCE_STR(devparms);
	if (mspace)
		MV_FORCE_STR(mspace);

	if (timeout < 0)
		timeout = 0;
	assert((unsigned char)*devparms->str.addr < n_iops);
	naml = get_log_name(&device->str, INSERT);
	if (naml->iod != 0)
		tl = naml;
	else
	{
#ifdef	NOLICENSE
	licensed= TRUE;
#else
		CRYPT_CHKSYSTEM;
		if (!licensed || LP_CONFIRM(lid, lkid)==LP_NOTACQ)
			licensed= FALSE;
#endif
		switch(stat = trans_log_name(&device->str, &tn, &buf1[0]))
		{
		case SS_NORMAL:
			tl = get_log_name(&tn, INSERT);
			break;
		case SS_NOLOGNAM:
			tl = naml;
			break;
		default:
			for (prev = io_root_log_name, tl = prev->next;  tl != 0;  prev = tl, tl = tl->next)
			{
				if (naml == tl)
				{
					prev->next = tl->next;
					free(tl);
					break;
				}
			}
			rts_error(VARLSTCNT(1) stat);
		}
	}
	stat = io_open_try(naml, tl, devparms, timeout, mspace);
	return (stat);
}
