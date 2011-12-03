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
#include <signal.h>
#include "error.h"
#include "preemptive_ch.h"
#include "util.h"

GBLREF VSIG_ATOMIC_T	util_interrupt;

CONDITION_HANDLER(util_ch)
{
	error_def(ERR_ASSERT);
	error_def(ERR_CTRLC);
	error_def(ERR_FORCEDHALT);
	error_def(ERR_GTMCHECK);
	error_def(ERR_GTMASSERT);
	error_def(ERR_STACKOFLOW);
	error_def(ERR_OUTOFSPACE);

	START_CH;
	if (DUMPABLE)
       		NEXTCH;
	PRN_ERROR;
	if (SIGNAL == ERR_CTRLC)
	{
		preemptive_ch(ERROR);	/* bluff about SEVERITY, just so gv_target will be reset in preemptive_ch */
		assert(util_interrupt);
		util_interrupt = 0;
		UNWIND(NULL, NULL);
	} else  if (SUCCESS == SEVERITY || INFO == SEVERITY)
	{
		CONTINUE;
	} else
	{
		preemptive_ch(SEVERITY);
		UNWIND(NULL, NULL);
	}
}
