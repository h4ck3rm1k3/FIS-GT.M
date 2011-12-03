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
#include "mdq.h"
#include "compiler.h"
#include "opcode.h"
#include "indir_enum.h"
#include "toktyp.h"
#include "mmemory.h"
#include "advancewindow.h"
#include "cmd.h"

GBLREF char window_token;
GBLREF triple *expr_start, *expr_start_orig;

int m_goto(void)
{
	triple tmpchain, *oldchain, *obp, *ref0, *ref1, *triptr;
	oprtype *cr;

	dqinit(&tmpchain, exorder);
	oldchain = setcurtchain(&tmpchain);
	if (!entryref(OC_JMP, OC_EXTJMP, (mint) indir_goto, TRUE, FALSE))
	{
		setcurtchain(oldchain);
		return FALSE;
	}
	setcurtchain(oldchain);
	if (window_token == TK_COLON)
	{
		advancewindow();
		cr = (oprtype *) mcalloc(sizeof(oprtype));
		if (!bool_expr((bool) FALSE, cr))
			return FALSE;
		if (expr_start != expr_start_orig)
		{
			triptr = newtriple(OC_GVRECTARG);
			triptr->operand[0] = put_tref(expr_start);
		}
		obp = oldchain->exorder.bl;
		dqadd(obp, &tmpchain, exorder);   /*this is a violation of info hiding*/
		if (expr_start != expr_start_orig)
		{
			ref0 = newtriple(OC_JMP);
			ref1 = newtriple(OC_GVRECTARG);
			ref1->operand[0] = put_tref(expr_start);
			*cr = put_tjmp(ref1);
			tnxtarg(&ref0->operand[0]);
		}
		else
			tnxtarg(cr);
		return TRUE;
	}
	obp = oldchain->exorder.bl;
	dqadd(obp, &tmpchain, exorder);   /*this is a violation of info hiding*/
	return TRUE;
}
