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
#include "iosp.h"
#include "gtm_logicals.h"
#include "min_max.h"
#include "gtm_string.h" 	/* for STRNCASECMP */
#include "trans_log_name.h"
#include "ztrap_form_init.h"

#define ZTRAP_FORM_CODE		"code"
#define ZTRAP_FORM_ENTRYREF	"entryref"
#define ZTRAP_FORM_ADAPTIVE	"adaptive"
#define ZTRAP_FORM_POP		"pop"

GBLREF	int	ztrap_form;

void ztrap_form_init(void)
{
	uint4		status;
	mstr		val, tn;
	char		buf[1024], *buf_ptr = &buf[0];

	error_def(ERR_TRNLOGFAIL);

	ztrap_form = ZTRAP_CODE;	/* default */
	val.addr = ZTRAP_FORM;
	val.len = STR_LIT_LEN(ZTRAP_FORM);
	if (SS_NORMAL == (status = trans_log_name(&val, &tn, buf)))
	{
		if (STR_LIT_LEN(ZTRAP_FORM_POP) < tn.len && !STRNCASECMP(buf_ptr, ZTRAP_FORM_POP, STR_LIT_LEN(ZTRAP_FORM_POP)))
		{
			buf_ptr += STR_LIT_LEN(ZTRAP_FORM_POP);
			tn.len -= STR_LIT_LEN(ZTRAP_FORM_POP);
			ztrap_form |= ZTRAP_POP;
		}
		if (!STRNCASECMP(buf_ptr, ZTRAP_FORM_ENTRYREF, MIN(STR_LIT_LEN(ZTRAP_FORM_ENTRYREF), tn.len)))
		{
			ztrap_form |= ZTRAP_ENTRYREF;
			ztrap_form &= ~ZTRAP_CODE;
		} else if (!STRNCASECMP(buf_ptr, ZTRAP_FORM_ADAPTIVE, MIN(STR_LIT_LEN(ZTRAP_FORM_ADAPTIVE), tn.len)))
			ztrap_form |= ZTRAP_ENTRYREF;
	} else if (SS_NOLOGNAM != status)
		rts_error(VARLSTCNT(5) ERR_TRNLOGFAIL, 2, LEN_AND_LIT(ZTRAP_FORM), status);
	return;
}
