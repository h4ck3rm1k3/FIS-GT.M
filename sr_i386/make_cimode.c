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

#include "error.h"
#include "rtnhdr.h"
#include "op.h"
#include "i386.h"
#include "inst_flush.h"
#include "gtmci.h"

#define CALL_SIZE	5
#define CODE_SIZE	(3 * CALL_SIZE)
#define CODE_LINES	3

/* The code created and returned by make_cimode() is executed in the frame GTM$CI at level 1 of
 * every nested call-in environment. For every M routine being called-in from C, GTM$CI code
 * will setup argument registers/stack and executes the M routine. When the M routine returns
 * from its final QUIT, GTM$CI returns to gtm_ci(). make_cimode generates machine equivalents
 * for the following operations in that order:
 *
 * 	CALL ci_restart	 :setup register/stack arguments from 'param_list' and transfer control
 * 		to op_extcall/op_extexfun which return only after the M routine finishes and QUITs.
 * 	CALL ci_ret_code :transfer control from the M routine back to C (gtm_ci). Never returns.
 * 	CALL opp_ret	 :an implicit QUIT although it is never executed.
 *
 * Before GTM$CI executes, it is assumed that the global 'param_list' has been populated with
 * argument/return mval*.
 */
rhdtyp *make_cimode(void)
{
	static rhdtyp	*base_address = NULL;
	lbl_tables	*lbl;
	int		*lnr;
	unsigned char	*code;

	if (NULL != base_address)
		return base_address;
	base_address = (rhdtyp *)malloc(sizeof(rhdtyp) + CODE_SIZE + sizeof(lbl_tables) + CODE_LINES * sizeof(int4));
	memset(base_address,0,sizeof(rhdtyp) + CODE_SIZE + sizeof(lbl_tables) + CODE_LINES * sizeof(int4));
	MEMCPY_LIT(&base_address->routine_name, GTM_CIMOD);
	base_address->ptext_ptr = sizeof(rhdtyp);
	base_address->vartab_ptr =
		base_address->labtab_ptr = sizeof(rhdtyp) + CODE_SIZE;	/* hdr + code */
	base_address->lnrtab_ptr = sizeof(rhdtyp) + CODE_SIZE + sizeof(lbl_tables);
	base_address->labtab_len = 1;
	base_address->lnrtab_len = CODE_LINES;
	code = (unsigned char *) base_address + base_address->ptext_ptr;
	*code++ = I386_INS_CALL_Jv;
	*((int4 *)code) = (int4)((unsigned char *)ci_restart - (code + sizeof(int4)));
	code += sizeof(int4);
	*code++ = I386_INS_CALL_Jv; /* a CALL to return control from M to ci_ret_code() which in turn returns to gtm_ci() */
	*((int4 *)code) = (int4)((unsigned char *)ci_ret_code - (code + sizeof(int4)));
	code += sizeof(int4);
	*code++ = I386_INS_JMP_Jv;
	*((int4 *)code) = (int4)((unsigned char *)opp_ret - (code + sizeof(int4)));
	code += sizeof(int4);
	lbl = (lbl_tables *)((int) base_address + base_address->labtab_ptr);
	lbl->lab_ln_ptr = base_address->lnrtab_ptr;
	lnr = (int *)((int)base_address + base_address->lnrtab_ptr);
	*lnr++ = base_address->ptext_ptr;
	*lnr++ = base_address->ptext_ptr;
	*lnr++ = base_address->ptext_ptr + 2 * CALL_SIZE;
	assert(code - ((unsigned char *)base_address + base_address->ptext_ptr) == CODE_SIZE);
	zlput_rname(base_address);
	inst_flush(base_address, sizeof(rhdtyp) + CODE_SIZE + sizeof(lbl_tables) + CODE_LINES * sizeof(int4));
	return base_address;
}
