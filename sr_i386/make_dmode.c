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
#include "dm_setup.h"

#define CALL_SIZE	5
#define CODE_SIZE	3*CALL_SIZE
#define CODE_LINES	3

rhdtyp *make_dmode(void)
{
	rhdtyp		*base_address;
	lbl_tables	*lbl;
	int		*lnr;
	unsigned char	*code;
						/* dummy code + label entry + line entries */
	base_address = (rhdtyp *)malloc(sizeof(rhdtyp) + CODE_SIZE + sizeof(lbl_tables) + CODE_LINES * sizeof(int4));
	memset(base_address,0,sizeof(rhdtyp) + CODE_SIZE + sizeof(lbl_tables) + CODE_LINES*sizeof(int4));
	MEMCPY_LIT(&base_address->routine_name, GTM_DMOD);
	base_address->ptext_ptr = sizeof(rhdtyp);
	base_address->vartab_ptr =
		base_address->labtab_ptr = sizeof(rhdtyp) + CODE_SIZE;	/* hdr + code */
	base_address->lnrtab_ptr = sizeof(rhdtyp) + CODE_SIZE + sizeof(lbl_tables);
	base_address->labtab_len = 1;
	base_address->lnrtab_len = CODE_LINES;
	code = (unsigned char *) base_address + base_address->ptext_ptr;
	*code++ = I386_INS_CALL_Jv;
	*((int4 *)code) = (int4)((unsigned char *)dm_setup - (code + sizeof(int4)));
	code += sizeof(int4);
	*code++ = I386_INS_CALL_Jv; /* this should be a CALL to maintain uniformity between transfer to mum_tstart from baseframe
				       and transfers to mum_tstart from error processing (MUM_TSTART marco in
				       mdb_condition_handler) */
	*((int4 *)code) = (int4)((unsigned char *)mum_tstart - (code + sizeof(int4)));
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
