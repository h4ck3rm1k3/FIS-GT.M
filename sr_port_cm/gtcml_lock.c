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
#include "mlkdef.h"
#include "locklits.h"
#include "cmidef.h"
#include "hashdef.h"
#include "cmmdef.h"
#include "gt_timer.h"
#include "gtcmlkdef.h"
#include "gtcml.h"

char gtcml_lock(cm_region_list *reg)
{
	return(gtcml_lock_internal(reg, LOCKED));
}
