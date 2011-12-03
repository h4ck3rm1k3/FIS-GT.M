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
#include "error.h"
#include "util.h"

CONDITION_HANDLER(region_init_ch)
{
	error_def(ERR_ASSERT);
	error_def(ERR_CTRLC);
	error_def(ERR_FORCEDHALT);
	error_def(ERR_GTMCHECK);
	error_def(ERR_GTMASSERT);
	error_def(ERR_STACKOFLOW);
	error_def(ERR_OUTOFSPACE);

	START_CH;
	if ((SIGNAL == ERR_GTMCHECK) || !(IS_GTM_ERROR(SIGNAL)) || DUMPABLE)
		NEXTCH;
     	PRN_ERROR;
	if (SEVERITY == WARNING || SEVERITY == INFO)
	{
		CONTINUE;
	} else
		UNWIND(NULL, NULL);
}
