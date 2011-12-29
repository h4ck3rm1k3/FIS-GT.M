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
#include "op.h"
#include "rtnhdr.h"
#include "stack_frame.h"
#include "gtmci.h"
#include "svnames.h"
#include "gtm_savetraps.h"
#include "error_trap.h"
#include "mv_stent.h"

GBLREF dollar_ecode_type dollar_ecode;
GBLREF dollar_stack_type dollar_stack;
GBLREF stack_frame	 *frame_pointer;

static dollar_ecode_type dollar_ecode_ci;
static dollar_stack_type dollar_stack_ci;
static dollar_ecode_type *dollar_ecode_addr;
static dollar_stack_type *dollar_stack_addr;

void gtmci_isv_save(void)
{
	gtm_savetraps();	/* Save either etrap or ztrap as appropriate */
	op_newintrinsic(SV_ESTACK);

	/* Save $ECODE/$STACK values of previous environment into SFF_CI frame of
	 * the current call-in environment. When this frame unwinds, their old values
	 * will be restored in dollar_ecode_ci and dollar_stack_ci (see unwind logic
	 * in ci_ret_code_quit) */
	assert(frame_pointer->flags & SFF_CI);
	dollar_ecode_addr = &dollar_ecode_ci;
	push_stck(&dollar_ecode, sizeof(dollar_ecode), (void**)&dollar_ecode_addr);
	dollar_stack_addr = &dollar_stack_ci;
	push_stck(&dollar_stack, sizeof(dollar_stack), (void**)&dollar_stack_addr);
	ecode_init();
}

void gtmci_isv_restore(void)
{
	/* free the allocated $ECODE/$STACK storage before exiting from the current
	 * call-in environment. NOTE: any changes here should be reflected in ecode_init() */
	if (dollar_ecode.begin)
		gtm_free_intern(dollar_ecode.begin);
	if (dollar_ecode.array)
		gtm_free_intern(dollar_ecode.array);
	if (dollar_stack.begin)
		gtm_free_intern(dollar_stack.begin);
	if (dollar_stack.array)
		gtm_free_intern(dollar_stack.array);
	/* Restore the old values from dollar_ecode_ci and dollar_stack_ci */
	memcpy(&dollar_ecode, &dollar_ecode_ci, sizeof(dollar_ecode));
	memcpy(&dollar_stack, &dollar_stack_ci, sizeof(dollar_stack));
}
