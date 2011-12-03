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
#include "zshow.h"

void mval_nongraphic(zshow_out *output,char *cp, int len, int num)
{
	/* sub-program for mval_write() */
	mval tmpmval;
	char buff[3];
	char *ptr;
	int n, m;

	tmpmval.mvtype = MV_STR;
	tmpmval.str.addr = cp;
	tmpmval.str.len = len;
	zshow_output(output,&tmpmval.str);
	for (ptr = buff + sizeof(buff) , n = num, m = sizeof(buff) ; m > 0 ; m--)
	{
		*--ptr = (n % 10) + '0';
		n /= 10;
		if (!n)
		break;
	}
	tmpmval.str.addr = ptr;
	tmpmval.str.len = buff - ptr + sizeof(buff);
	zshow_output(output,&tmpmval.str);
	return;
}
