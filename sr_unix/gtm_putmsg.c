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
#include "util.h"
#include <stdarg.h>
#include "gtmmsg.h"
#include "gtm_putmsg_list.h"


/*
**  WARNING:	For chained error messages, all messages MUST be followed by an fao count;
**  =======	zero MUST be specified if there are no parameters.
*/


void
gtm_putmsg(int foo,...)

{
	va_list	var;

	va_start(var,foo);
	gtm_putmsg_list(var);
	util_out_print("",TRUE);
}

void gtm_putmsg_noflush(int foo,...)
{
	va_list var;

	va_start(var,foo);
	gtm_putmsg_list(var);
}
