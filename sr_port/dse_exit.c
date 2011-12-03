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

#include "gtm_stdlib.h"		/* for exit() */

#include "error.h"
#include "iosp.h"
#include "util.h"
#include "dse_exit.h"

void dse_exit(void)
{
	util_out_close();
	EXIT(SS_NORMAL);
}
