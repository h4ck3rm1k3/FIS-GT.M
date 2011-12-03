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
#include "hashdef.h"
#include "cmidef.h"
#include "cmmdef.h"
#include "gvcmx.h"
#include "gvcmz.h"
#include "mvalconv.h"

bool gvcmx_zprevious(void)
{
	mval v;

	gvcmz_doop(CMMS_Q_PREV, CMMS_R_PREV, &v);
	if (MV_FORCE_INT(&v))
		return TRUE;
	else
		return FALSE;
}
