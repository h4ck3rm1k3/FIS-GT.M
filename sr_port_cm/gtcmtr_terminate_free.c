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
#include "cmidef.h"
#include "hashdef.h"
#include "cmmdef.h"
#include "gtcmtr_terminate_free.h"


void gtcmtr_terminate_free(connection_struct *ce)
{
	if (ce->pvec)
		free(ce->pvec);
	free(ce);
	return ;
}
