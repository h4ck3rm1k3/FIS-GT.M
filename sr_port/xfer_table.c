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

//example XFER(xf_sto, op_sto),
//#define XFER(a,b) int b ( int,...)
//#define XFER(a,b) int b ( int,...)
#define XFER(a,b) b(int, ...)


int
#include "xfer.h"
;
#undef XFER

#define XFER(a,b) b
GBLDEF int (* volatile xfer_table[])(int,...)=
{
#include "xfer.h"
};
#undef XFER
