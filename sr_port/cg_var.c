/****************************************************************
 *								*
 *	Copyright 2001, 2002 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"
#include "compiler.h"
#include "rtnhdr.h"
#include "cg_var.h"

void cg_var(mvar *l, VAR_TABENT **p)
{	/* Copy mident with variable name to variable table entry */
	(*p)[l->mvidx] = l->mvname;
	return;
}
