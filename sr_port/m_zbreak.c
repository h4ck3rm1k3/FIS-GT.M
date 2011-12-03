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
#include "compiler.h"
#include "opcode.h"
#include "indir_enum.h"
#include "toktyp.h"
#include "advancewindow.h"
#include "cmd.h"

#define CANCEL_ONE -1
#define CANCEL_ALL -2

GBLREF char window_token;
GBLREF mident window_ident;
GBLREF bool run_time;
GBLREF char routine_name[];
LITDEF mident zero_ident;	/* the null mident */

int m_zbreak(void)
{
	triple	*ref, *next;
	oprtype label, offset, routine, action, count;
	bool 	cancel, cancel_all, is_count, dummybool;
	error_def(ERR_LABELEXPECTED);
	error_def(ERR_RTNNAME);

	label = put_str((char *)&zero_ident.c[0], sizeof(mident));
	cancel_all = FALSE;
	action = put_str("B", 1);
	if (window_token == TK_MINUS)
	{
		advancewindow();
		cancel = TRUE;
		count = put_ilit((mint)CANCEL_ONE);
	} else
	{
		cancel = FALSE;
		count = put_ilit((mint)0);
	}
	if (window_token == TK_ASTERISK)
	{
		if (cancel)
		{
			advancewindow();
			cancel_all = TRUE;
			if (!run_time)
				routine = put_str(&routine_name[0], sizeof(mident));
			else
				routine = put_tref(newtriple(OC_CURRTN));
			offset = put_ilit((mint) 0);
			count = put_ilit((mint) CANCEL_ALL);
		} else
		{
			stx_error(ERR_LABELEXPECTED);
			return FALSE;
		}
	} else
	{
		offset = put_ilit((mint) 0);
		if (!lref(&label,&offset, TRUE, indir_zbreak, !cancel, &dummybool))
			return FALSE;
		if (label.oprclass == TRIP_REF && label.oprval.tref->opcode == OC_COMMARG)
			return TRUE;
		if (window_token != TK_CIRCUMFLEX)
		{
			if (!run_time)
				routine = put_str(&routine_name[0], sizeof(mident));
			else
				routine = put_tref(newtriple(OC_CURRTN));
		} else
		{
			advancewindow();
			switch(window_token)
			{
			case TK_IDENT:
				routine = put_str(&window_ident.c[0], sizeof(mident));
				advancewindow();
				break;
			case TK_ATSIGN:
				if (!indirection(&routine))
					return FALSE;
				break;
			default:
				stx_error(ERR_RTNNAME);
				return FALSE;
			}
		}
		if (!cancel && window_token == TK_COLON)
		{
			advancewindow();
			if (window_token == TK_COLON)
			{
				is_count = TRUE;
				action = put_str("B",1);
			} else
			{
				if (!strexpr(&action))
					return FALSE;
				is_count = window_token == TK_COLON;
			}
			if (is_count)
			{
				advancewindow();
				if (!intexpr(&count))
					return FALSE;
			}
		}
	}
	ref = newtriple(OC_SETZBRK);
	ref->operand[0] = label;
	next = newtriple(OC_PARAMETER);
	ref->operand[1] = put_tref(next);
	next->operand[0] = offset;
	ref = newtriple(OC_PARAMETER);
	next->operand[1] = put_tref(ref);
	ref->operand[0] = routine;
	next = newtriple(OC_PARAMETER);
	ref->operand[1] = put_tref(next);
	next->operand[0] = action;
	ref = newtriple(OC_PARAMETER);
	next->operand[1] = put_tref(ref);
	ref->operand[0] = count;
	return TRUE;
}
