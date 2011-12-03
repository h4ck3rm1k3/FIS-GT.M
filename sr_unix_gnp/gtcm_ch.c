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
#include "gtm_time.h"
#include "gdsroot.h"
#include "gtm_facility.h"
#include "fileinfo.h"
#include "gdsbt.h"
#include "gdsfhead.h"
#include "hashdef.h"
#include "cmidef.h"
#include "cmmdef.h"
#include "error.h"
#include "fao_parm.h"
#include "gtm_fcntl.h"
#include "cmi.h"
#include "preemptive_ch.h"
#include "gtm_string.h"
#include "gtm_stdio.h"
#include "copy.h"
#include "min_max.h"
#include "gtcm_write_ast.h"
#include "io.h"
#include "iosp.h"
#include "util.h"
#include "send_msg.h"
#include "gtmmsg.h"
#include "gtcm_open_cmerrlog.h"

GBLREF connection_struct	*curr_entry;
GBLDEF bool			gtcm_errfile = FALSE;
GBLDEF bool			gtcm_firsterr = TRUE;
GBLDEF FILE 			*gtcm_errfs = NULL;
GBLREF unsigned char		*util_outptr;

static 	short			szero = 0;

CONDITION_HANDLER(gtcm_ch)
{
	unsigned char	msgnum, sevmsgbuf[2048];
	unsigned char	*tempptr, *mbfptr, *endptr, *msgptr;
	err_ctl		*fac;
	int		i, msglen, len, rc, orig_severity;
	bool		first;
	now_t		now;	/* for GET_CUR_TIME macro */
	char		time_str[CTIME_BEFORE_NL + 2], *time_ptr; /* for GET_CUR_TIME macro */
	short		short_len;

	error_def(ERR_SERVERERR);

	START_CH;
	if (gtcm_firsterr)
		gtcm_open_cmerrlog();
	msgnum = 1;
	msglen = (char *)util_outptr - (char *)util_outbuff;
	if (0 == msglen)
	{	/* gtm_putmsg_list has already flushed message. Get length another way.
		   Also reduce msg length by <\n> on it's end which we don't want to send.
		*/
		msglen = strlen(util_outbuff) - 1;
	} else
	{	/* msg yet to be flushed. Properly terminate it in the buffer */
		*util_outptr = '\n';
		*(util_outptr + 1) = 0;
	}
	msgptr = (unsigned char *)util_outbuff;
	assert(msglen);
	if (gtcm_errfile)
	{
		GET_CUR_TIME;
		time_str[CTIME_BEFORE_NL] = 0;
		FPRINTF(gtcm_errfs, "%s: %s", time_str, util_outbuff);
		fflush(gtcm_errfs);
	}
	orig_severity = SEVERITY;
	/* Don't let severe error message cause client to necessarily die. Reflect error using ERR_SERVERERR */
	if (4 <= SEVERITY)
	{
		memcpy(sevmsgbuf, util_outbuff, msglen);
		util_out_print(NULL, OPER);	/* write msg to operator log */
		gtm_putmsg_noflush(VARLSTCNT(4) ERR_SERVERERR, 2, msglen, sevmsgbuf);
		msglen = (char *)util_outptr - (char *)util_outbuff;
	}
	if (curr_entry)
	{
		mbfptr = curr_entry->clb_ptr->mbf;
		endptr = mbfptr + curr_entry->clb_ptr->mbl;
		*mbfptr++ = CMMS_E_ERROR;
		*mbfptr++ = 0;			/* Msg continue indicator */
		*mbfptr++ = msgnum++;
		PUT_SHORT(mbfptr, szero);	/* Room for msg length */
		mbfptr += sizeof(short);
		PUT_LONG(mbfptr, SIGNAL);
		mbfptr += sizeof(int4);
		PUT_LONG(mbfptr, SEVERITY);
		mbfptr += sizeof(int4);
		do
		{
			assert(msglen);
			len = MIN(msglen, (endptr - mbfptr));
			memcpy(mbfptr, msgptr, msglen);
			mbfptr += len;
			msgptr += len;
			msglen -= len;
			if (msglen)
			{
				/* Need to finish this msg off and start a new one */
				curr_entry->clb_ptr->cbl = mbfptr - curr_entry->clb_ptr->mbf;
				curr_entry->clb_ptr->ast = 0;
				tempptr = curr_entry->clb_ptr->mbf + 1;
				*tempptr++ = 1;
				tempptr++;
				short_len = (short)len;
				assert((int)short_len == len); /* short <- int assignment lossy? */
				PUT_SHORT(tempptr, short_len);
				cmi_write(curr_entry->clb_ptr);
				mbfptr = curr_entry->clb_ptr->mbf;
				*mbfptr++ = CMMS_E_ERROR;
				*mbfptr++ = 0;
				*mbfptr++ = msgnum++;
				PUT_SHORT(mbfptr, szero);	/* Room for msg length */
				mbfptr += sizeof(short);
			}
		} while(msglen);
		if (mbfptr > curr_entry->clb_ptr->mbf + 3)
		{
			curr_entry->clb_ptr->cbl = mbfptr - curr_entry->clb_ptr->mbf;
			curr_entry->clb_ptr->ast = gtcm_write_ast;
			tempptr = curr_entry->clb_ptr->mbf + 3;
			short_len = (short)len;
			assert((int)short_len == len); /* short <- int assignment lossy? */
			PUT_SHORT(tempptr, short_len);
			cmi_write(curr_entry->clb_ptr);
		}
	}
	if (SUCCESS == orig_severity || INFO == orig_severity)
	{
		CONTINUE;
	}
	if (WARNING == orig_severity || ERROR == orig_severity)
	{
		preemptive_ch(orig_severity);
		UNWIND(x,y);
	}
	NEXTCH;
}
