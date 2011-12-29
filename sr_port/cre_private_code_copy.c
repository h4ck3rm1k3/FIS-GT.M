/****************************************************************
 *								*
 *	Copyright 2002, 2003 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

#include "mdef.h"

#include "iosp.h"
#include "error.h"
#include "rtnhdr.h"
#include "inst_flush.h"
#include "private_code_copy.h"

CONDITION_HANDLER(cre_priv_ch)
{
	error_def(UNIX_ONLY(ERR_MEMORY) VMS_ONLY(ERR_VMSMEMORY);)

	START_CH;
	if (SIGNAL == UNIX_ONLY(ERR_MEMORY) VMS_ONLY(ERR_VMSMEMORY))
	{
		UNWIND(NULL, NULL); /* ignore "lack-of-memory" error, rather not set breakpoint than error out in such a case */
	}
	//	NEXTCH;
	CHTRACEPOINT; 
	current_ch->ch_active = FALSE; 
	//	DRIVECH(arg); 
	//#define DRIVECH(x) { 
	error_def(ERR_TPRETRY); 
	CHTRACEPOINT; 
	if (ERR_TPRETRY != error_condition) ch_cond_core(); 
	while (active_ch >= &chnd[0]) { 
	  if (!active_ch->ch_active) break; 
	  active_ch--; 
	} 
	if (active_ch >= &chnd[0] && *active_ch->ch) 
	  (*active_ch->ch)(arg); 
	else ch_overrun(); 

	return; 
}

uint4 cre_private_code_copy(rhdtyp *rtn)
{
	unsigned char	*new_ptext;
	int		code_size;

	error_def(UNIX_ONLY(ERR_MEMORY) VMS_ONLY(ERR_VMSMEMORY);)

#ifdef USHBIN_SUPPORTED
		assert(NULL != rtn->shlib_handle); /* don't need private copy if not shared */
		assert(NULL == rtn->shared_ptext_adr); /* if already private, we shouldn't be calling this routine */
		code_size = rtn->ptext_end_adr - rtn->ptext_adr;
		ESTABLISH_RET(cre_priv_ch, UNIX_ONLY(ERR_MEMORY) VMS_ONLY(ERR_VMSMEMORY));
		new_ptext = gtm_malloc_intern(code_size);
		REVERT;
		memcpy(new_ptext, rtn->ptext_adr, code_size);
		adjust_frames(rtn->ptext_adr, rtn->ptext_end_adr, new_ptext);
		rtn->shared_ptext_adr = rtn->ptext_adr;
		rtn->ptext_adr = new_ptext;
		rtn->ptext_end_adr = new_ptext + code_size;
		inst_flush(new_ptext, code_size);
#endif
	return SS_NORMAL;
}
