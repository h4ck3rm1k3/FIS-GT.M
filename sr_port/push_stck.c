/****************************************************************
 *								*
 *	Copyright 2001, 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"
#include "gtm_string.h"
#include "mv_stent.h"

GBLREF mv_stent *mv_chain;
GBLREF unsigned char *stackbase,*stacktop,*msp,*stackwarn;

/* Function to push a generic C type on to M stack as an MVST_STCK entry:
 *	val : Address of an object which needs to be saved on M stack
 *	val_size : size of the object to be pushed into the stack (0 if only the
 *		pointer to the object is pushed but not its contents).
 *	addr : When this entry is unwound,
 *	   + if an object had been pushed, the contents of the object would be restored in *addr
 *	   + if a pointer had been pushed, previous value(val) would be restored in *addr.
 */
void push_stck(void* val, int val_size, void** addr)
{
	error_def(ERR_STACKOFLOW);
	error_def(ERR_STACKCRIT);

	PUSH_MV_STCK(val_size);
	if (0 < val_size)
		memcpy(msp + mvs_size[MVST_STCK], val, val_size);
	mv_chain->mv_st_cont.mvs_stck.mvs_stck_val = val;
	mv_chain->mv_st_cont.mvs_stck.mvs_stck_addr = addr;
	mv_chain->mv_st_cont.mvs_stck.mvs_stck_size = val_size;
}
