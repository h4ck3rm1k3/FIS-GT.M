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

#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "op.h"
#include "subscript.h"
#include "str2gvargs.h"
#include "str2gvkey.h"
#include "callg.h"

void str2gvkey_gvfunc(char *cp, int len)
{ /* We have two functions str2gvkey_gvfunc and str2gvkey_nogvfunc instead of passing a boolean argument to say a common  */
  /* function str2gvkey because GVUSR_QUERYGET when moved to DDPGVUSR would pull in op_gvname and op_gvargs objects - will */
  /* cause nasty problems in the link */

	boolean_t	naked;
	gvargs_t	op_gvargs;

	naked = str2gvargs(cp, len, &op_gvargs);
	callg((int(*)())(naked ? op_gvnaked : op_gvname), &op_gvargs);
	return;
}
