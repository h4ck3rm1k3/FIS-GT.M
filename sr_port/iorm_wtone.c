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
#include "io.h"

void iorm_wtone(unsigned char ch)
{
	mstr	temp;

	temp.len = 1;
	temp.addr = (char *)&ch;
	iorm_write(&temp);
	return;
}
